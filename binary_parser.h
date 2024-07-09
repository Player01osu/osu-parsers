#ifndef BINARY_PARSER_H
#define BINARY_PARSER_H

#include <stdint.h>

#include "string_builder.h"
#include "stream.h"

enum {
	EBIN_PARSER_R_UNEXPECTED_EOF = 1,
	EBIN_PARSER_R_BAD_LEN,
	EBIN_PARSER_R_ULEB128_ENCODE_OVERFLOW,
	EBIN_PARSER_R_BAD_READ,

	EBIN_PARSER_W_LEN_WRITE,
	EBIN_PARSER_W_WRITE_BYTE,
	EBIN_PARSER_W_BAD_WRITE,
};

const char *binp_error_msg(void);

size_t binp_error_msg_len(void);

int binp_read_uleb128(StreamReader *reader, int *output);

int binp_read_str(StreamReader *reader, Str *output);

int binp_read_i32(StreamReader *reader, int32_t *output);

int binp_read_i64(StreamReader *reader, int64_t *output);

int binp_read_u16(StreamReader *reader, uint16_t *output);

int binp_read_byte_array(StreamReader *reader, ByteSlice *output);

int binp_read_bool(StreamReader *reader, bool *output);

int binp_write_uleb128(StreamWriter *writer, const int input);

int binp_write_str(StreamWriter *writer, const Str *input);

int binp_write_i32(StreamWriter *writer, const int32_t input);

int binp_write_i64(StreamWriter *writer, const int64_t input);

int binp_write_u16(StreamWriter *writer, const uint16_t input);

int binp_write_byte_array(StreamWriter *writer, const ByteSlice *input);

int binp_write_bool(StreamWriter *writer, const bool input);

#endif
