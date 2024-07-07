/*
 * Quick array (qa) implementation
 *
 *
 * Usage:
 * #include "qarray.h"
 *
 * int *arr = NULL;
 * size_t len = 0;
 * size_t cap = 0;
 *
 * // Test push 3
 * qa_push(&arr, &len, &cap, 5);
 * qa_push(&arr, &len, &cap, 10);
 * qa_push(&arr, &len, &cap, 420);
 *
 * assert(arr[0] == 5);
 * assert(arr[1] == 10);
 * assert(arr[2] == 420);
 * assert(len == 3);
 * assert(cap >= len);
 *
 * // Test pop
 * assert(qa_pop(&arr, &len, NULL));
 *
 * int v;
 * assert(qa_pop(&arr, &len, &v));
 * assert(v == 10);
 * assert(qa_pop(&arr, &len, NULL));
 * assert(!qa_pop(&arr, &len, NULL));
 *
 * qa_push(&arr, &len, &cap, 3);
 * qa_push(&arr, &len, &cap, 10);
 * qa_push(&arr, &len, &cap, 101);
 * qa_push(&arr, &len, &cap, 202);
 * qa_push(&arr, &len, &cap, 303);
 * qa_push(&arr, &len, &cap, 404);
 * qa_push(&arr, &len, &cap, 505);
 * qa_push(&arr, &len, &cap, 606);
 *
 * // Test yield
 * size_t i = 0;
 * while (qa_yield(&arr, len, &i, &v)) {
 * 	size_t idx = i - 1;
 * 	fprintf(stderr, "arr[%zu] == %d\n", idx, v);
 * 	assert(arr[idx] == v);
 * }
 *
 * // Test remove at first
 * assert(qa_remove(&arr, &len, 0, &v));
 * assert(v == 3);
 * assert(len == 7);
 * assert(arr[0] == 10);
 * fprintf(stderr, "== AFTER REMOVE FIRST ==\n");
 * i = 0;
 * while (qa_yield(&arr, len, &i, &v)) {
 * 	size_t idx = i - 1;
 * 	fprintf(stderr, "arr[%zu] == %d\n", idx, v);
 * 	assert(arr[idx] == v);
 * }
 * assert(arr[6] == 606);
 *
 * // Test remove at last
 * assert(qa_remove(&arr, &len, 6, &v));
 * assert(len == 6);
 * assert(arr[0] == 10);
 * assert(arr[5] == 505);
 *
 * assert(!qa_remove(&arr, &len, 6, NULL));
 *
 * free(arr);
 */

#ifndef QARRAY_H
#define QARRAY_H

#include <stdbool.h>
#include <string.h>

#ifndef QARRAY_MALLOC

#include <stdlib.h>

#define QARRAY_MALLOC malloc
#define QARRAY_REALLOC realloc

#endif

#define qa_malloc QARRAY_MALLOC
#define qa_realloc QARRAY_REALLOC

#define qa_push(ptr, len, cap, v)                                              \
	do {                                                                   \
		if (*(len) >= *(cap)) {                                        \
			*(cap) *= 2;                                           \
			if (*(cap) <= 0) {                                     \
				*(ptr) = NULL;                                 \
				*(len) = 0;                                    \
				*(cap) = 16;                                   \
			}                                                      \
			*(ptr) = qa_realloc(*(ptr), sizeof(**(ptr)) * *(cap)); \
		}                                                              \
		(*(ptr))[*(len)] = v;                                          \
		++*(len);                                                      \
	} while (0)

#define qa_pop(ptr, len, out) ((*(len) > 0) ? \
	(--*(len), _qa_set(out, *(ptr), sizeof(**(ptr)), *(len)), true) : \
	false)

#define qa_yield(ptr, len, idx, out) (*(idx) < (len) ? \
	(_qa_set(out, *(ptr), sizeof(**(ptr)), *(idx)), ++*(idx), true) : \
	false)

#define qa_remove(ptr, len, idx, out) ((idx) < *(len) ? \
	(_qa_set(out, *(ptr), sizeof(**(ptr)), idx), _qa_remove(*(ptr), len, sizeof(**(ptr)), idx), true) : \
	false)

static inline void _qa_set(void *out, const void *ptr, size_t size, size_t idx)
{
	if (out) memcpy(out, &((char *) ptr)[idx * size], size);
}

static inline void _qa_remove(void *ptr, size_t *len, size_t size, size_t idx)
{
	for (size_t i = idx; i < *len - 1; ++i) {
		_qa_set(&(((char *) ptr)[i * size]), ptr, size, i + 1);
	}
	--*len;
}

#endif
