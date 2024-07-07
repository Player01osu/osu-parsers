#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "easylzma/decompress.h"
#include "easylzma/compress.h"

#include "osr_parser.h"
#include "binary_parser.h"
#include "string_builder.h"
#include "xutils.h"
#include "mods.h"

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define QARRAY_MALLOC xmalloc
#define QARRAY_REALLOC xrealloc

#include "qarray.h"

static size_t compress_write(void *ctx, const void *buf, size_t size)
{
	ByteArray *byte_array = ctx;
	string_builder_push_str(byte_array, (Str) { .items = (char *) buf, .len = size });
	return size;
}

static int compress_read(void *ctx, void *buf, size_t *size)
{
	void **read_ctx = ctx;
	Str *replay_str = read_ctx[0];
	size_t *read_idx = read_ctx[1];
	size_t start_idx = *read_idx;

	if (*size >= replay_str->len - *read_idx) {
		memcpy(buf, &replay_str->items[*read_idx], replay_str->len - *read_idx);
		*read_idx = replay_str->len;
	} else {
		memcpy(buf, &replay_str->items[*read_idx], *size);
		*read_idx += *size;
	}

	*size = *read_idx - start_idx;
	return 0;
}

static size_t decompress_write(void *ctx, const void *buf, size_t size)
{
	const char *b = buf;
	ByteArray *byte_array = ctx;
	for (size_t i = 0; i < size; ++i) {
		string_builder_push(byte_array, b[i]);
	}
	return size;
}

static int decompress_read(void *ctx, void *buf, size_t *size)
{
	void **read_ctx = ctx;
	ByteSlice *compressed_replay = read_ctx[0];
	size_t *read_idx = read_ctx[1];
	size_t start_idx = *read_idx;

	if (*size >= compressed_replay->len - *read_idx) {
		memcpy(buf, &compressed_replay->items[*read_idx], compressed_replay->len - *read_idx);
		*read_idx = compressed_replay->len;
	} else {
		memcpy(buf, &compressed_replay->items[*read_idx], *size);
		*read_idx += *size;
	}

	*size = *read_idx - start_idx;
	return 0;
}

static Str replay_frames_to_str(const struct ReplayFrames *frames)
{
	char b[1024] = {0};
	StringBuilder sb = {0};
	string_builder_init_cap(&sb, 1024 * 4);
	float current_time = 0.0;
	for (size_t i = 0; i < frames->len; ++i) {
		/* TODO: This is not the exact same formatting as the one used
		 * in `LegacyScoreEncoder.cs`
		 *
		 * https://github.com/ppy/osu/blob/8bbbedaec3a1af9a255a32e3f186cfebd25d6783/osu.Game/Scoring/Legacy/LegacyScoreEncoder.cs
		 */
		ReplayFrame frame = frames->items[i];
		float diff = frame.time - current_time;
		current_time = frame.time;
		float mouse_x = frame.mouse_x;
		float mouse_y = frame.mouse_y;
		int buttons = frame.button_state;
		size_t fmt_size = (size_t) stbsp_snprintf(b, 1024, "%0.4f|%0.4f|%0.4f|%d,", diff, mouse_x, mouse_y, buttons);
		assert(fmt_size <= 1024);
		Str s = { .items = b, .len = fmt_size };
		string_builder_push_str(&sb, s);
	}

	size_t fmt_size = (size_t) stbsp_snprintf(b, 1024, "-1234|0|0|0");
	assert(fmt_size <= 1024);
	string_builder_push_str(&sb, (Str) { .items = b, .len = fmt_size });
	return string_builder_build(&sb);
}

static Str hp_graph_to_str(const HPGraph *graph)
{
	char b[1024] = {0};
	StringBuilder sb = {0};
	string_builder_init_cap(&sb, 1024 * 4);

	for (size_t i = 0; i < graph->len; ++i) {
		HPGraphPoint point = graph->items[i];
		int32_t time = point.time;
		float value = point.value;
		size_t fmt_size = (size_t) stbsp_snprintf(b, 1024, "%d|%0.3f,", time, value);
		assert(fmt_size <= 1024);
		Str s = { .items = b, .len = fmt_size };
		string_builder_push_str(&sb, s);
	}

	return string_builder_build(&sb);
}

/* https://github.com/ppy/osu/blob/8bbbedaec3a1af9a255a32e3f186cfebd25d6783/osu.Game/Scoring/Legacy/LegacyScoreDecoder.cs#L263 */
int osrp_parse_replay_frames(const ByteSlice *src, struct ReplayFrames *out)
{
	int ret = 0;
	float current_time = 0.0;
	ReplayFrame *frames = NULL;
	size_t len = 0;
	size_t cap = 0;

	size_t cursor = 0;
	while (cursor < src->len) {
		size_t tmp = cursor;
		size_t num_fields = 0;
		size_t field_locations[4]; /* idx to end of each field */
		while (tmp < src->len && src->items[tmp] != ',') {
			if (src->items[tmp] == '|') {
				field_locations[num_fields++] = tmp - 1;
			}
			++tmp;
		}
		field_locations[num_fields++] = tmp - 1;

		if (num_fields < 4) {
			cursor = ++tmp;
			continue;
		}

		{
			const char *s = "-1234";
			const char *p = &src->items[cursor];
			bool eq = true;
			while (*s && p < src->items + src->len) {
				eq = *s == *p;
				if (!eq) break;
				++s;
				++p;
			}
			if (eq) {
				cursor = ++tmp;
				continue;
			}
		}

		ReplayFrame frame = {0};
		char *endptr;

		endptr = &src->items[field_locations[0]];
		float offset = (float) strtod(&src->items[cursor], &endptr);
		if (!endptr) {
			eprintf("ERROR: Could not parse float\n");
			ret = -EOSR_PARSER_FLOAT_PARSE;
			goto error_1;
		}
		current_time += offset;
		frame.time = current_time;

		/* Offset cursor past the deliminator to first char */
		cursor = field_locations[0] + 2;
		endptr = &src->items[field_locations[1]];
		frame.mouse_x = (float) strtod(&src->items[cursor], &endptr);
		if (!endptr) {
			eprintf("ERROR: Could not parse float\n");
			ret = -EOSR_PARSER_FLOAT_PARSE;
			goto error_1;
		}

		cursor = field_locations[1] + 2;
		endptr = &src->items[field_locations[2]];
		frame.mouse_y = (float) strtod(&src->items[cursor], &endptr);
		if (!endptr) {
			eprintf("ERROR: Could not parse float\n");
			ret = -EOSR_PARSER_FLOAT_PARSE;
			goto error_1;
		}

		cursor = field_locations[2] + 2;
		endptr = &src->items[field_locations[3]];
		char *nptr = &src->items[cursor];
		frame.button_state = strtoimax(nptr, &endptr, 10);

		qa_push(&frames, &len, &cap, frame);

		if (len >= 2 && frames[1].time < frames[0].time) {
			frames[1].time = frames[0].time;
			frames[0].time = 0.0;
		}

		if (len >= 3 && frames[0].time > frames[2].time) {
			frames[0].time = frames[1].time = frames[2].time;
		}

		if (len >= 2 && frames[1].mouse_x == 256.0 && frames[1].mouse_y == -500.0) {
			qa_remove(&frames, &len, 1, NULL);
		}

		if (len >= 1 && frames[0].mouse_x == 256.0 && frames[0].mouse_y == -500.0) {
			qa_remove(&frames, &len, 0, NULL);
		}
	}

	out->len = len;
	out->items = frames;
error_1:
	if (ret) free(frames);
	return ret;
}

int osrp_parse_hp_graph(Str hp_str, HPGraph *out)
{
	size_t ret = 0;
	size_t len = 0;
	size_t size = 0;
	HPGraphPoint *graph_points = NULL;
	size_t cursor = 0;
	while (cursor < hp_str.len) {
		HPGraphPoint point = {0};
		/* Two boundaries; end of first field, and
		 * end of second field
		 */
		size_t field_boundaries[2];

		size_t tmp = cursor;
		size_t fb_idx = 0;
		while (1) {
			if (tmp >= hp_str.len) {
				ret = -1;
				goto error_1;
			}
			if (fb_idx >= sizeof(field_boundaries)) {
				eprintf("Too many fields");
				ret = -1;
				goto error_1;
			}
			if (hp_str.items[tmp] == '|') {
				field_boundaries[fb_idx++] = tmp - 1;
				++tmp;
			} else if (hp_str.items[tmp] == ',') {
				field_boundaries[fb_idx] = tmp - 1;
				if (fb_idx != 1) {
					eprintf("Incorrect number of fields");
					ret = -1;
					goto error_1;
				}
				break;
			} else {
				++tmp;
			}
		}
		const char *nptr;
		char *endptr;
		nptr = &hp_str.items[cursor];
		endptr = &hp_str.items[field_boundaries[0]];
		point.time = (int32_t) strtoimax(nptr, &endptr, 10);
		if (!endptr) {
			eprintf("Could not parse time field");
			ret = -1;
			goto error_1;
		}

		cursor = field_boundaries[0] + 1;
		nptr = &hp_str.items[cursor];
		endptr = &hp_str.items[field_boundaries[1]];
		point.value = (float) strtod(nptr, &endptr);
		if (!endptr) {
			eprintf("Could not parse float");
			ret = -EOSR_PARSER_FLOAT_PARSE;
			goto error_1;
		}
		cursor = field_boundaries[1] + 2;
		qa_push(&graph_points, &len, &size, point);
	}
	out->len = len;
	out->items = graph_points;

	return ret;

error_1:
	if (graph_points) free(graph_points);
	return ret;
}

/* https://github.com/ppy/osu/blob/8bbbedaec3a1af9a255a32e3f186cfebd25d6783/osu.Game/Scoring/Legacy/LegacyScoreDecoder.cs#L36 */
int osrp_parse_osr(StreamReader *reader, OsuReplay *out)
{
	int ret = 0;
	if (reader->read_n(reader->ctx, 1, &out->mode) != 0) {
		eprintf("Read failed at mode byte");
		return -1;
	}

	if (binp_read_i32(reader, &out->version) < 0) {
		/* XXX: Append error msg */
		eprintf("Read failed at version");
		return -1;
	}

	Str tmp_str = {0};
	ret = binp_read_str(reader, &tmp_str);
	if (ret < 0) {
		/* XXX: Append error msg */
		eprintf("Could not read beatmap hash");
		return -1;
	}
	if (tmp_str.len != sizeof(out->beatmap_hash)) {
		eprintf("Unexpected beatmap hash length; possibly corrupted file");
		free(tmp_str.items);
		return -1;
	}

	/* TODO: Try to avoid copies */
	memcpy(out->beatmap_hash, tmp_str.items, sizeof(out->beatmap_hash));
	free(tmp_str.items);
	tmp_str.items = NULL;
	tmp_str.len = 0;

	ret = binp_read_str(reader, &out->username);
	if (ret < 0) {
		/* XXX: Append error msg */
		eprintf("Could not read username");
		return -1;
	}
	ret = binp_read_str(reader, &tmp_str);
	if (ret < 0) {
		/* XXX: Append error msg */
		eprintf("Could not read md5hash");
		goto error_1;
	}
	if (tmp_str.len != sizeof(out->md5hash)) {
		eprintf("Unexpected md5hash length; possibly corrupted file");
		free(tmp_str.items);
		return -1;
	}
	/* TODO: Try to avoid copies */
	memcpy(out->md5hash, tmp_str.items, sizeof(out->md5hash));
	free(tmp_str.items);
	tmp_str.items = NULL;
	tmp_str.len = 0;

	/* XXX: Provide error message */
#define expect(fn, output)                 \
	do {                               \
		ret = fn(reader, &output); \
		if (ret < 0) goto error_1; \
	} while (0)
	expect(binp_read_u16, out->count300);
	expect(binp_read_u16, out->count100);
	expect(binp_read_u16, out->count50);
	expect(binp_read_u16, out->count_geki);
	expect(binp_read_u16, out->count_katu);
	expect(binp_read_u16, out->count_miss);

	expect(binp_read_i32, out->total_score);
	expect(binp_read_u16, out->max_combo);
	expect(binp_read_bool, out->is_perfect);

	expect(binp_read_i32, out->mod_bitfield);
#undef expect

	ret = binp_read_str(reader, &tmp_str);
	if (ret < 0) {
		/* XXX: Append error msg */
		eprintf("Could not to read hp graph str\n");
		goto error_1;
	}
	ret = osrp_parse_hp_graph(tmp_str, &out->hp_graph);
	if (ret < 0) {
		/* XXX: Append error msg */
		eprintf("Could not parse hp graph");
		free(tmp_str.items);
		goto error_1;
	}
	free(tmp_str.items);

	/* XXX: Provide error msg */
#define expect(fn, output)                 \
	do {                               \
		ret = fn(reader, &output); \
		if (ret < 0) goto error_2; \
	} while (0)
	expect(binp_read_i64, out->date_time);
#undef expect

	ByteSlice compressed_replay;
	{
		int result = binp_read_byte_array(reader, &compressed_replay);
		if (result < 0) {
			/* XXX: Append error msg */
			ret = result;
			eprintf("Could not read compressed replay");
			goto error_2;
		} else if (result == 1) {
			eprintf("NULL byte array is used for non-legacy replays, which is not yet implemented");
			goto error_2;
		}
		ByteArray byte_array = {0};
		elzma_decompress_handle hand = elzma_decompress_alloc();
		string_builder_init_cap(&byte_array, compressed_replay.len * 3 / 2);
		size_t read_idx = 0;
		void *read_ctx[] = { &compressed_replay, &read_idx };
		result = elzma_decompress_run(
			hand,
			decompress_read, read_ctx,
			decompress_write, &byte_array,
			ELZMA_lzma
		);
		if (result != 0) {
			/* TODO: Decompress error reason */
			string_builder_free(&byte_array);
			eprintf("Could not decompress lzma stream: %d\n", result);
			elzma_decompress_free(&hand);
			ret = -1;
			goto error_3;
		}

		elzma_decompress_free(&hand);
		ByteSlice barray = string_builder_build(&byte_array);
		if (osrp_parse_replay_frames(&barray, &out->frames) < 0) {
			/* XXX: Append error msg */
			free(barray.items);
			eprintf("Could not parse replay frames");
			ret = -1;
			goto error_3;
		}
		free(barray.items);
	}

	if (out->version >= 20140721) {
		ret = binp_read_i64(reader, &out->online_id);
		if (ret < 0) {
			/* XXX: Append error msg */
			eprintf("Could not read online id");
			goto error_3;
		}
	} else if (out->version >= 20121008) {
		int32_t i;
		ret = binp_read_i32(reader, &i);
		if (ret < 0) {
			/* XXX: Append error msg */
			eprintf("Could not read online id");
			goto error_3;
		}
		out->online_id = (int64_t) i;
	} else {
		eprintf("Unimplemented non-legacy online id");
		goto error_3;
	}

#define efree(ptr) if (ret < 0) free(ptr)
error_3:
	free(compressed_replay.items);
error_2:
	efree(out->hp_graph.items);
error_1:
	efree(out->username.items);
#undef efree

	return ret;
}

/* XXX: osu!stable cannot parse the replay data
 *
 * I suspect this is because the compression header/settings are not the
 * same, and they are hardcoded in stable
 * */
int osrp_write_osr(StreamWriter *writer, const OsuReplay *in)
{
	int ret = 0;
	if (writer->write_n(writer->ctx, 1, &in->mode) < 0) {
		eprintf("Could not write mode byte");
		return -1;
	}

	if (binp_write_i32(writer, in->version) < 0) {
		/* XXX: Append error msg */
		eprintf("Could not write version");
		return -1;
	}

	Str beatmap_hash = {
		.items = (char *) in->beatmap_hash,
		.len = sizeof(in->beatmap_hash),
	};
	if (binp_write_str(writer, &beatmap_hash) < 0) {
		/* XXX: Append error msg */
		eprintf("Could not write beatmap hash");
		return -1;
	}

	if (binp_write_str(writer, &in->username) < 0) {
		eprintf("Could not to write username");
		return -1;
	}

	Str md5hash = {
		.items = (char *) in->md5hash,
		.len = sizeof(in->md5hash),
	};
	if (binp_write_str(writer, &md5hash) < 0) {
		eprintf("Could not write md5hash");
		return -1;
	}

	/* XXX: Provide error msg */
#define expect(fn, x)                               \
	do {                                        \
		if (fn(writer, (x)) < 0) return -1; \
	} while (0)
	expect(binp_write_u16, in->count300);
	expect(binp_write_u16, in->count100);
	expect(binp_write_u16, in->count50);
	expect(binp_write_u16, in->count_geki);
	expect(binp_write_u16, in->count_katu);
	expect(binp_write_u16, in->count_miss);
	expect(binp_write_i32, in->total_score);
	expect(binp_write_u16, in->max_combo);
	expect(binp_write_bool, in->is_perfect);
	expect(binp_write_i32, in->mod_bitfield);
	Str hp_graph_str = hp_graph_to_str(&in->hp_graph);
	expect(binp_write_str, &hp_graph_str);
	expect(binp_write_i64, in->date_time);
#undef expect

	/* XXX: Write compressed replay */
	{
		elzma_compress_handle hand = elzma_compress_alloc();
		/* https://github.com/ppy/osu/blob/8598e8bf34e125baa3d567b19746c24cfb3ce763/osu.Game/Scoring/Legacy/LegacyScoreEncoder.cs#L123
		 * https://github.com/adamhathcock/sharpcompress/blob/master/src/SharpCompress/Compressors/LZMA/LzmaEncoderProperties.cs#L24
		 */

		/* XXX: Want to set these to the LZMA properties used
		 * in sharpcompress and osu!
		struct elzma_compress_handle_exposed *h = (struct elzma_compress_handle_exposed *) &hand;
		h->props.dictSize = 1 << 21;
		h->props.btMode = 4;
		h->props.algo = 2;
		h->props.lc = 3;
		h->props.pb = 2;
		h->props.lp = 0;
		h->props.fb = 255;
		*/

		Str replay_str = replay_frames_to_str(&in->frames);
		size_t read_idx = 0;
		void *read_ctx[2] = { &replay_str, &read_idx };
		ByteArray byte_array = {0};
		string_builder_init_cap(&byte_array, 1024 * 4);

		if (elzma_compress_run(hand, compress_read, read_ctx, compress_write, &byte_array, NULL, NULL)) {
			eprintf("Could not to compress replay frames");
			free(byte_array.items);
			free(replay_str.items);
			elzma_compress_free(&hand);
			return -EOSR_ENCODE_LZMA_FRAME_COMPRESS;
		}

		ByteSlice byte_slice = string_builder_build(&byte_array);
		if (binp_write_byte_array(writer, &byte_slice) < 0) {
			/* XXX: Append error msg */
			eprintf("Could not write byte array");
			free(byte_array.items);
			free(replay_str.items);
			elzma_compress_free(&hand);
			return -1;
		}

		free(byte_array.items);
		free(replay_str.items);
		elzma_compress_free(&hand);
	}

	if (in->version >= 20140721) {
		ret = binp_write_i64(writer, in->online_id);
		if (ret < 0) {
			eprintf("Could not write online id");
			return -1;
		}
	} else if (in->version >= 20121008) {
		ret = binp_write_i32(writer, (int32_t) in->online_id);
		if (ret < 0) {
			eprintf("Could not write online id");
			return -1;
		}
	} else {
		eprintf("Unimplmented non-legacy replay parsing");
		return -1;
	}

	return ret;
}

void osrp_replay_destroy(OsuReplay *replay)
{
	free(replay->hp_graph.items);
	free(replay->username.items);
	free(replay->frames.items);
}

int osrp_replay_frame_csv(StreamWriter *writer, const OsuReplay *replay, bool header)
{
	char buf[1024];
	if (header) {
		size_t fmt_size = (size_t) stbsp_snprintf(
			buf, 1024, "time,mouse_x,mouse_y,button_state\n"
		);
		assert(fmt_size < 1024);
		if (writer->write_n(writer->ctx, fmt_size, buf) != 0) {
			eprintf("Write failed");
			return -1;
		}
	}

	for (size_t i = 0; i < replay->frames.len; ++i) {
		ReplayFrame frame = replay->frames.items[i];
		size_t fmt_size = (size_t) stbsp_snprintf(
			buf, 1024, "%0.3f,%0.3f,%0.3f,%d\n",
			frame.time, frame.mouse_x, frame.mouse_y, frame.button_state
		);
		assert(fmt_size < 1024);
		if (writer->write_n(writer->ctx, fmt_size, buf) != 0) {
			eprintf("Write failed");
			return -1;
		}
	}
	return 0;
}
