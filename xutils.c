#include <stdio.h>
#include <stdlib.h>

#include "xutils.h"

void *_xmalloc(char *filename, int row, size_t size)
{
	void *block = malloc(size);
	if (!block) {
		fprintf(stderr, "%s:%d:ERROR:failed to allocate block... %lu\n", filename, row, size);
		abort();
	}
	return block;
}

void *_xrealloc(char *filename, int row, void *ptr, size_t size)
{
	void *block = realloc(ptr, size);
	if (!block) {
		fprintf(stderr, "%s:%d:ERROR:failed to allocate block... %lu\n", filename, row, size);
		abort();
	}
	return block;
}

