/*
 * Example program using the osr parser.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "xutils.h"
#include "osr_parser.h"
#include "binary_parser.h"
#include "mods.h"
#include "stream.h"

static int read_file(void *ctx, size_t size, void *buf)
{
	FILE *f = ctx;
	if (fread(buf, 1, size, f) == size) {
		return 0;
	} else {
		return -1;
	}
}

static int write_file(void *ctx, size_t size, const void *buf)
{
	FILE *f = ctx;
	if (fwrite(buf, 1, size, f) == size) {
		return 0;
	} else {
		return -1;
	}
}

static const char *help =
	"Usage: osr_tools <FILE> [OPTION]\n"
	"\n"
	"Options:\n"
	"  --csv                   Outputs csv-formatted frames to stdout\n"
	"  --mods                  Show mods used\n"
	"  --username              Show username\n"
	"  --hash                  Show replay md5hash\n"
	"  --beatmap-hash          Show beatmap hash\n"
	"  --count-300             Show 300 count\n"
	"  --count-100             Show 100 count\n"
	"  --count-50              Show 50 count\n"
	"  --count-miss            Show miss count\n"
	"  --score                 Show score\n"
	"  --max-combo             Show max combo\n";

int main(int argc, char **argv)
{
#if 1
	int ret = 0;
	if (argc < 3) {
		eprintf("Missing arguments...\n");
		eprintf(help);
		exit(1);
	}

	char *fname = argv[1];
	FILE *f = fopen(fname, "rb");
	if (!f) {
		eprintf("ERROR:Failed to open file:%s\n", fname);
		exit(1);
	}

	bool csv_opt = false;
	bool mods_opt = false;
	bool username_opt = false;
	bool hash_opt = false;
	bool beatmap_hash_opt = false;
	bool count_300_opt = false;
	bool count_100_opt = false;
	bool count_50_opt = false;
	bool count_miss_opt = false;
	bool score_opt = false;
	bool max_combo_opt = false;
	for (size_t i = 2; i < (size_t) argc; ++i) {
		const char *arg = argv[i];
		if (strcmp(arg, "--csv") == 0) csv_opt = true;
		else if (strcmp(arg, "--mods") == 0) mods_opt = true;
		else if (strcmp(arg, "--username") == 0) username_opt = true;
		else if (strcmp(arg, "--hash") == 0) hash_opt = true;
		else if (strcmp(arg, "--beatmap-hash") == 0) beatmap_hash_opt = true;
		else if (strcmp(arg, "--count-300") == 0) count_300_opt = true;
		else if (strcmp(arg, "--count-100") == 0) count_100_opt = true;
		else if (strcmp(arg, "--count-50") == 0) count_50_opt = true;
		else if (strcmp(arg, "--count-miss") == 0) count_miss_opt = true;
		else if (strcmp(arg, "--score") == 0) score_opt = true;
		else if (strcmp(arg, "--max-combo") == 0) max_combo_opt = true;
		else {
			eprintf("ERROR:Unknown flag:%s\n", arg);
			eprintf(help);
			goto error_1;
		}
	}

	StreamReader reader = {
		.ctx = f,
		.read_n = read_file,
	};
	StreamWriter writer = {
		.ctx = stdout,
		.write_n = write_file,
	};
	OsuReplay replay = {0};
	if (osrp_parse_osr(&reader, &replay) < 0) {
		eprintf("ERROR:Could not parse osr:%s:%s\n", fname, osrp_error_msg());
		ret = 1;
		goto error_1;
	}

	if (csv_opt) {
		if (osrp_replay_frame_csv(&writer, &replay, true) < 0) {
			eprintf("ERROR:Could not put csv:%s\n", osrp_error_msg());
			goto error_2;
		}
	}

	if (mods_opt) printf("mods: 0x%X\n", replay.mod_bitfield);
	if (username_opt) {
		fputs("username: ", stdout);
		fwrite(replay.username.items, 1, replay.username.len, stdout);
		putc('\n', stdout);
	}
	if (hash_opt) {
		fputs("hash: ", stdout);
		fwrite(replay.md5hash, 1, sizeof(replay.md5hash), stdout);
		putc('\n', stdout);
	}
	if (beatmap_hash_opt) {
		fputs("beatmap hash: ", stdout);
		fwrite(replay.beatmap_hash, 1, sizeof(replay.beatmap_hash), stdout);
		putc('\n', stdout);
	}
	if (count_300_opt) printf("300s: %u\n", replay.count300);
	if (count_100_opt) printf("100s: %u\n", replay.count100);
	if (count_50_opt) printf("50s: %u\n", replay.count50);
	if (count_miss_opt) printf("misses: %u\n", replay.count_miss);
	if (score_opt) printf("score: %u\n", replay.total_score);
	if (max_combo_opt) printf("max combo: %u\n", replay.max_combo);

error_2:
	osrp_replay_destroy(&replay);

error_1:
	fclose(f);
	return ret;
#endif
}
