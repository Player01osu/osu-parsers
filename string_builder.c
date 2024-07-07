#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "xutils.h"
#include "string_builder.h"

#ifdef __x86_64__
#define INT_MAX_WIDTH 19
#else
#define INT_MAX_WDITH 10
#endif

static inline bool resize_needed(StringBuilder *sb, size_t n)
{
	bool resized = false;
	assert(n >= 1);
	while (sb->len + n > sb->cap) {
		sb->cap *= 2;
		resized = true;
	}
	if (resized) sb->items = xrealloc(sb->items, sb->cap);
	return resized;
}

void string_builder_init_cap(StringBuilder *sb, size_t cap)
{
	sb->len = 0;
	sb->cap = cap;
	sb->items = xmalloc(sizeof(*sb->items) * cap);
}

void string_builder_init(StringBuilder *sb)
{
	string_builder_init_cap(sb, DEFAULT_STRING_BUILDER_CAP);
}

bool string_builder_push(StringBuilder *sb, char c)
{
	bool resized = resize_needed(sb, 1);
	sb->items[sb->len++] = c;
	return resized;
}

bool string_builder_push_cstr(StringBuilder *sb, char *s)
{
	bool resized = false;
	while (*s) {
		resized = string_builder_push(sb, *s);
		++s;
	}
	return resized;
}

bool string_builder_push_str(StringBuilder *sb, Str s)
{
	bool resized = resize_needed(sb, s.len);
	char *offset = sb->items + sb->len;
	memcpy(offset, s.items, s.len);
	sb->len += s.len;
	return resized;
}

bool string_builder_push_int(StringBuilder *sb, int i)
{
	bool resized;
	char digits[INT_MAX_WIDTH] = {0};
	int idx = 0;
	if (i < 0) {
		i *= -1;
		string_builder_push(sb, '-');
	}
	digits[idx++] = i % 10;
	i /= 10;
	while (i > 0) {
		digits[idx++] = i % 10;
		i /= 10;
	}
	while (idx > 0) {
		resized = string_builder_push(sb, digits[--idx] + 48);
	}
	return resized;
}

bool string_builder_printf(StringBuilder *sb, char *fmt, ...)
{
	va_list ap;
	bool resized = false;
	bool is_fmt = false;

	va_start(ap, fmt);
	while (*fmt) {
		if (is_fmt) {
			/* float f; */
			char *s;
			int i;
			char c;
			switch (*fmt) {
			case '%':
				resized |= string_builder_push(sb, '%');
				break;
			case 's':
				s = va_arg(ap, char *);
				resized |= string_builder_push_cstr(sb, s);
				break;
			case 'd':
				i = va_arg(ap, int);
				resized |= string_builder_push_int(sb, i);
				break;
			case 'f':
				panic("Floating point string builder printf not yet implemented!");
				break;
			case 'c':
				c = va_arg(ap, int);
				resized |= string_builder_push(sb, c);
				break;
			default:
				panic("Invalid fmt: '%c' \"%s\"", *fmt, fmt);
			}
			is_fmt = false;
		} else {
			if (*fmt == '%') is_fmt = true;
			else resized |= string_builder_push(sb, *fmt);
		}
		++fmt;
	}
	va_end(ap);
	return resized;
}

void string_builder_free(StringBuilder *sb)
{
	free(sb->items);
}

Str string_builder_build(StringBuilder *sb)
{
	return (Str){
		.len = sb->len,
		.items = sb->items,
	};
}

char *string_builder_build_cstr(StringBuilder *string_builder)
{
	string_builder_push(string_builder, '\0');
	return string_builder->items;
}

void str_free(Str *s)
{
	free(s->items);
}
