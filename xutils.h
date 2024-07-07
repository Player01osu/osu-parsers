#ifndef XUTILS_H
#define XUTILS_H

#include <stdio.h>
#include <stdlib.h>

#define panic(...)                                             \
	do {                                                   \
		eprintf("%s:%d:", __FILE__, __LINE__); \
		eprintf(__VA_ARGS__);                  \
		abort();                                       \
	} while (0)
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define xmalloc(size) _xmalloc(__FILE__, __LINE__, size)
#define xrealloc(ptr, size) _xrealloc(__FILE__, __LINE__, ptr, size)

void *_xmalloc(char *filename, int row, size_t size);
void *_xrealloc(char *filename, int row, void *ptr, size_t size);

#endif
