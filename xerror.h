/*
 * Be sure to expose the error str with a function
 * and an appropriate prefix.
 *
 * ```
 *   char *prefix_error_msg(void) { return xerror_str; }
 * ```
 */


#ifndef XERROR_H
#define XERROR_H

#ifndef XERROR_ASSERT
#include <assert.h>

#define XERROR_ASSERT assert
#endif

#ifndef XERROR_OVERFLOW_CB
#include <stdio.h>

#define panic(...)                                             \
	do {                                                   \
		fprintf(stderr, "%s:%d:", __FILE__, __LINE__); \
		fprintf(stderr, __VA_ARGS__);                  \
		abort();                                       \
	} while (0)

#define XERROR_OVERFLOW_CB panic
#endif

#ifndef XERROR_VSNPRINTF
#include <string.h>

#define XERROR_VSNPRINTF vsnprintf
#endif

#ifndef XERROR_STRCAT
#include <string.h>

#define XERROR_STRCAT strcat
#endif

#ifndef XERROR_MEMCPY
#include <string.h>

#define XERROR_MEMCPY memcpy
#endif

#ifndef XERROR_MAX
/* Hopefully error messages are not going to be over 4KiB */
#define XERROR_MAX (1024 * 4)
#endif

#define xerror_vsnprintf   XERROR_VSNPRINTF
#define xerror_strcat      XERROR_STRCAT
#define xerror_memcpy      XERROR_MEMCPY
#define xerror_assert      XERROR_ASSERT
#define xerror_overflow_cb XERROR_OVERFLOW_CB

static char xerror_str[XERROR_MAX];
static size_t xerror_str_len;

/* XXX: Compile-time length check */
/* Use with static str */
#define xerror_sput(s) xerror_put(s, sizeof(s))

/* Use with non-static str */
static inline void xerror_put(const char *msg, const size_t len)
{
	if ((len) + 1 >= XERROR_MAX) {
		xerror_overflow_cb("%s:%d:Error str overflow when putting:\"%s\":\"%s\"\n",
		      __FILE__, __LINE__, xerror_str, msg);
	}
	xerror_memcpy(xerror_str, msg, len);
	xerror_str_len = len;
}

#define xerror_scat(s) xerror_cat(s, sizeof(s))

static inline void xerror_cat(const char *msg, const size_t len)
{
	if (xerror_str_len + len + 1 >= XERROR_MAX) {
		xerror_overflow_cb("%s:%d:Error str overflow when concatenating:\"%s\":\"%s\"\n",
		      __FILE__, __LINE__, xerror_str, msg);
	}
	xerror_str_len += len;
	xerror_strcat(xerror_str, msg);
}

static inline void xerror_fmt(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = (size_t) xerror_vsnprintf(xerror_str, XERROR_MAX, fmt, ap);
	if (len + 1 >= XERROR_MAX) {
		xerror_overflow_cb("%s:%d:Error str overflow when formatting:\"%s\"\n",
		      __FILE__, __LINE__, xerror_str);
	}
	xerror_str_len = len;
	va_end(ap);
}

#endif
