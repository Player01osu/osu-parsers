#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "binary_parser.h"
#include "xutils.h"

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define XERROR_OVERFLOW_CB panic
#define XERROR_SNPRINTF stbsp_snprintf
#include "xerror.h"

const char *binp_error_msg(void)
{
	return xerror_str;
}

size_t binp_error_msg_len(void)
{
	return xerror_str_len;
}

/* https://github.com/dotnet/runtime/blob/d099f075e45d2aa6007a22b71b45a08758559f80/src/libraries/System.Private.CoreLib/src/System/IO/BinaryReader.cs#L568C13-L603C32 */
int binp_read_uleb128(StreamReader *reader, int32_t *output)
{
	#define MAX_SHIFT ((int) (sizeof(*output) * 7))
	*output = 0;
	int shift = 0;
	while (shift < MAX_SHIFT) {
		unsigned char b;
		if (reader->read_n(reader->ctx, 1, &b) != 0) {
			xerror_sput("Read failed");
			return -EBIN_PARSER_R_BAD_READ;
		}
		*output |= (b & 0x7fu) << shift;
		if (b <= 0x7fu) return 0;
		shift += 7;
	}
	unsigned char b;
	if (reader->read_n(reader->ctx, 1, &b) != 0) {
		xerror_sput("Read failed");
		return -EBIN_PARSER_R_BAD_READ;
	}

	if (b > 0xfu) {
		xerror_sput("7 bit encoded integer overflow");
		return -EBIN_PARSER_R_ULEB128_ENCODE_OVERFLOW;
	}
	*output |= b << MAX_SHIFT;
	return 0;
	#undef MAX_SHIFT
}

int binp_read_str(StreamReader *reader, Str *output)
{
	unsigned char b;
	if (reader->read_n(reader->ctx, 1, &b) != 0) {
		xerror_sput("Read failed");
		return -EBIN_PARSER_R_BAD_READ;
	}
	if (b == 0) {
		output->items = NULL;
		return 1;
	}
	int32_t len;
	if (binp_read_uleb128(reader, &len) < 0) {
		xerror_scat(":Length read bad");
		return -EBIN_PARSER_R_BAD_LEN;
	}
	assert(len >= 0);
	output->len = (size_t) len;
	/* XXX: Returns null terminated string for printf purposes
	 * Should have function to transform Str into cstr w/o
	 * leaking or copying
	 */
	char *s = xmalloc(output->len + 1);
	if (reader->read_n(reader->ctx, output->len, s) != 0) {
		free(s);
		xerror_fmt("Could not read %lu bytes into buffer", output->len);
		return -EBIN_PARSER_R_BAD_READ;
	}
	s[len] = '\0';
	output->items = s;
	return 0;
}


int binp_read_i32(StreamReader *reader, int32_t *output)
{
	if (reader->read_n(reader->ctx, 4, output) != 0) {
		xerror_sput("Could not read into int32");
		return -EBIN_PARSER_R_BAD_READ;
	}
	return 0;
}


int binp_read_i64(StreamReader *reader, int64_t *output)
{
	if (reader->read_n(reader->ctx, 8, output) != 0) {
		xerror_sput("Could not read into int64");
		return -EBIN_PARSER_R_BAD_READ;
	}
	return 0;
}

int binp_read_u16(StreamReader *reader, uint16_t *output)
{
	if (reader->read_n(reader->ctx, 2, output) != 0) {
		xerror_sput("Could not read into uint16");
		return -EBIN_PARSER_R_BAD_READ;
	}
	return 0;
}

/*
 * < 0 for error
 * 0 for success
 * 1 for null
 */
int binp_read_byte_array(StreamReader *reader, ByteSlice *output)
{
	int32_t len;
	if (binp_read_i32(reader, &len) < 0) {
		xerror_scat(":Could not read byte array length");
		return -EBIN_PARSER_R_BAD_LEN;
	}
	if (len <= 0) return 1;
	output->len = (size_t) len;
	output->items = xmalloc(output->len);
	if (reader->read_n(reader->ctx, output->len, output->items) != 0) {
		xerror_fmt("Could not read %lu bytes into array", output->len);
		free(output->items);
		return -EBIN_PARSER_R_BAD_READ;
	}
	return 0;
}

int binp_read_bool(StreamReader *reader, bool *output)
{
	if (reader->read_n(reader->ctx, 1, output) != 0) {
		xerror_sput("Could not read byte");
		return -EBIN_PARSER_R_BAD_READ;
	}
	*output = *output != 0;
	return 0;
}

int binp_write_uleb128(StreamWriter *writer, const int32_t input)
{
	int32_t n = input;
	while (1) {
		unsigned char b = n & (0xFF >> 1);
		n >>= 7;
		if (n != 0) b |= (1 << 7);
		/* TODO: Think about buffering so it only
		 * requires one write call
		 */
		if (writer->write_n(writer->ctx, 1, &b) < 0) {
			xerror_sput("Could not write byte into writer");
			return -EBIN_PARSER_W_WRITE_BYTE;
		}
		if (n == 0) break;
	}

	return 0;
}

int binp_write_str(StreamWriter *writer, const Str *input)
{
	{
		unsigned char hint_byte = 0x0b;
		if (!input) hint_byte = 0;
		if (writer->write_n(writer->ctx, 1, &hint_byte) < 0) {
			xerror_sput("Could not write byte into writer");
			return -EBIN_PARSER_W_WRITE_BYTE;
		}
	}

	if (binp_write_uleb128(writer, input->len) < 0) {
		xerror_scat(":Could not write ULEB128 length");
		return -EBIN_PARSER_W_LEN_WRITE;
	}

	if (writer->write_n(writer->ctx, input->len, input->items) < 0) {
		xerror_sput("Could not write full string into writer");
		return -EBIN_PARSER_W_BAD_WRITE;
	}

	return 0;
}

int binp_write_i32(StreamWriter *writer, const int32_t input)
{
	if (writer->write_n(writer->ctx, 4, &input) < 0) {
		xerror_sput("Writer failed");
		return -EBIN_PARSER_W_BAD_WRITE;
	}
	return 0;
}

int binp_write_i64(StreamWriter *writer, const int64_t input)
{
	if (writer->write_n(writer->ctx, 8, &input) < 0) {
		xerror_sput("Writer failed");
		return -EBIN_PARSER_W_BAD_WRITE;
	}
	return 0;
}

int binp_write_u16(StreamWriter *writer, const uint16_t input)
{
	if (writer->write_n(writer->ctx, 2, &input) < 0) {
		xerror_sput("Write failed");
		return -EBIN_PARSER_W_BAD_WRITE;
	}
	return 0;
}

int binp_write_byte_array(StreamWriter *writer, const ByteSlice *input)
{
	if (binp_write_i32(writer, input->len) < 0) {
		xerror_scat(":Could not write len");
		return -EBIN_PARSER_W_LEN_WRITE;
	}
	if (input->len <= 0) return 1;
	if (writer->write_n(writer->ctx, input->len, input->items) < 0) {
		xerror_sput("Could not write full byte array into writer");
		return -EBIN_PARSER_W_BAD_WRITE;
	}

	return 0;
}

int binp_write_bool(StreamWriter *writer, const bool input)
{
	if (writer->write_n(writer->ctx, 1, &input) < 0) {
		xerror_sput("Write failed");
		return -EBIN_PARSER_W_BAD_WRITE;
	}
	return 0;
}
