#ifndef STREAM_H
#define STREAM_H

typedef struct StreamReader {
	void *ctx;

	/* `read_n` will request n bytes to be filled into `buf`
	 *
	 * Return < 0 for error, 1 for EOS
	 *
	 * int read_n(void *ctx, size_t num_bytes, void *buf);
	 */
	int (*read_n)(void *, size_t, void *);
} StreamReader;

typedef struct StreamWriter {
	void *ctx;

	/* `write_n` will provide the size of the buffer, and a
	 * filled buffer that can be read from
	 *
	 * Return < 0 for error
	 *
	 * int write_n(void *ctx, size_t num_bytes, const void *buf);
	 */
	int (*write_n)(void *, size_t, const void *);
} StreamWriter;

#endif
