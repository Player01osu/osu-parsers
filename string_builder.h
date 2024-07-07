#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stdbool.h>

#define DEFAULT_STRING_BUILDER_CAP 16

#define to_str(s) (Str){ .len = sizeof(s) - 1, .items = (s) }

/*
 * Users must call `string_builder_free`, or if built, call `str_free`
 * on the respective `Str`.
 *
 * Underlying `char *` is not guarenteed to be NULL terminated.
 */
typedef struct ByteArray {
	size_t len;
	size_t cap;
	char *items;
} ByteArray;

typedef ByteArray StringBuilder;

/*
 * Mutating slices can cause undefined behavior.
 *
 * Users must call `str_free` if arena wasn't used.
 */
typedef struct ByteSlice {
	size_t len;
	char *items;
} ByteSlice;

typedef ByteSlice Str;

void string_builder_init_cap(StringBuilder *string_builder, size_t cap);

void string_builder_init(StringBuilder *string_builder);

bool string_builder_push(StringBuilder *string_builder, char c);

/*
 * Caller of this function must ensure that the string is properly terminated.
 *
 * Prefer `string_builder_push_str` whenever the length is known.
 */
bool string_builder_push_cstr(StringBuilder *string_builder, char *s);

bool string_builder_push_str(StringBuilder *string_builder, Str s);

bool string_builder_push_int(StringBuilder *string_builder, int i);

bool string_builder_printf(StringBuilder *sb, char *fmt, ...);

void string_builder_free(StringBuilder *string_builder);

Str string_builder_build(StringBuilder *string_builder);

/*
 * Construct a NULL terminated cstr.
 *
 * Remember to free this if an arena wasn't used.
 */
char *string_builder_build_cstr(StringBuilder *string_builder);

void str_free(Str *s);

#endif
