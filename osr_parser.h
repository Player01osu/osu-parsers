#ifndef OSR_PARSER_H
#define OSR_PARSER

#include <stdint.h>
#include <stdbool.h>

#include "string_builder.h"
#include "stream.h"

enum {
	EOSR_PARSER_FLOAT_PARSE = 1,
	EOSR_ENCODE_LZMA_FRAME_COMPRESS,
};

enum {
	MODE_OSU   = 0,
	MODE_TAIKO = 1,
	MODE_CATCH = 2,
	MODE_MANIA = 3,
};

typedef struct HPGraphPoint {
	int32_t time; /* XXX: Will this overflow on some maps? */
	float value; /* NOTE: Between 0.0 and 1.0 */
} HPGraphPoint;

typedef struct HPGraph {
	HPGraphPoint *items;
	size_t len;
} HPGraph;

typedef struct ReplayFrame {
	float time; /* NOTE: This is NOT delta */
	float mouse_x;
	float mouse_y;
	int button_state;
} ReplayFrame;

/* XXX: Pack this better */
typedef struct OsuReplay {
	unsigned char mode;
	int32_t version;
	/* TODO: Both of these are stored as strings in the format, but space
	 * can be saved by turning these into raw bytes (ie: u64)
	 */
	char beatmap_hash[32];
	char md5hash[32];
	Str username;

	uint16_t count300;
	uint16_t count100;
	uint16_t count50;
	uint16_t count_geki;
	uint16_t count_katu;
	uint16_t count_miss;

	int32_t total_score;
	uint16_t max_combo;
	bool is_perfect;

	int32_t mod_bitfield;

	HPGraph hp_graph;

	int64_t date_time;

	struct ReplayFrames {
		size_t len;
		ReplayFrame *items;
	} frames;

	int64_t online_id;
} OsuReplay;

const char *osrp_error_msg(void);

size_t osrp_error_msg_len(void);

int osrp_parse_replay_frames(const ByteSlice *src, struct ReplayFrames *out);

int osrp_parse_osr(StreamReader *reader, OsuReplay *out);

void osrp_replay_destroy(OsuReplay *replay);

int osrp_replay_frame_csv(StreamWriter *writer, const OsuReplay *replay, bool header);

int osrp_write_osr(StreamWriter *writer, const OsuReplay *in);

#endif
