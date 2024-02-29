#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MARKER 0xFF

typedef unsigned char uchar;

size_t
compress(uchar *in, uchar *out, size_t len)
{
	size_t i = 0, olen = 0;

	while (i < len) {
		uchar ch = in[i], reps = 1;

		while (i + reps < len && reps < 255 && ch == in[i + reps])
			++reps;

		if (reps > 1 || ch == MARKER) {
			out[olen++] = MARKER;
			out[olen++] = reps;
			out[olen++] = ch;
			i += reps;
		} else {
			out[olen++] = ch;
			++i;
		}
	}

	return olen;
}

size_t
decompress(uchar *in, uchar *out, size_t len)
{
	size_t i = 0, olen = 0;

	while (i < len) {
		uchar ch = in[i];

		if (ch == MARKER) {
			uchar reps = in[i + 1];
			uchar j;

			ch = in[i + 2];
			for (j = 0; j < reps; ++j)
				out[olen++] = ch;

			i += 3;
		} else {
			out[olen++] = ch;
			++i;
		}
	}

	return olen;
}

void
die(const char *msg)
{
	perror(msg);
	exit(1);
}

FILE *
xfopen(const char *filename, const char *mode)
{
	FILE *fp;

	fp = fopen(filename, mode);
	if (fp == NULL)
		die("fopen");

	return fp;
}

long
filesize(const char *filename)
{
	FILE *fp;
	long size;;

	fp = xfopen(filename, "rb");
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fclose(fp);

	return size;
}

void *
xmalloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (ptr == NULL)
		die("malloc");

	return ptr;
}

int
main(int argc, char **argv)
{
	FILE *in, *out;
	uchar *inbuf, *outbuf;
	long fsize;
	size_t len, nread;

	if (argc != 4 || argv[1][0] == '\0') {
		fprintf(stderr, "args?\n");
		return 1;
	}

	if (argv[1][0] == 'c') {
		fsize = filesize(argv[2]);
		in = xfopen(argv[2], "rb");
		out = xfopen(argv[3], "wb");
		inbuf = xmalloc(fsize);
		outbuf = xmalloc(fsize * 3); /* 0xFF, 0x01, 0xFF for each marker byte. */

		while ((nread = fread(inbuf, 1, fsize, in)) > 0) {
			len = compress(inbuf, outbuf, nread);
			fwrite(outbuf, len, 1, out);
		}
	} else {
		fsize = filesize(argv[2]);
		in = xfopen(argv[2], "rb");
		out = xfopen(argv[3], "wb");
		inbuf = xmalloc(fsize);
		outbuf = xmalloc(fsize / 3 * 255); /* 3 bytes expand to 255 */

		while ((nread = fread(inbuf, 1, fsize, in)) > 0) {
			len = decompress(inbuf, outbuf, nread);
			fwrite(outbuf, len, 1, out);
		}
	}

	free(inbuf);
	free(outbuf);
	fclose(in);
	fclose(out);

	return 0;
}
