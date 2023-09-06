#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void
die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	if (fmt && fmt[strlen(fmt) - 1] == ':') {
		vfprintf(stderr, "%s: ", ap);
		perror(NULL);
	} else
		fprintf(stderr, "%s\n", fmt);

	va_end(ap);

	exit(1);
}

int
xfputc(int c, FILE *f)
{
	int cc;

	cc = fputc(c, f);
	if (cc == EOF)
		die("fputc:");

	return cc;
}

size_t
xfwrite(const void *p, size_t size, size_t nmemb, FILE *f)
{
	size_t nwritten;

	nwritten = fwrite(p, size, nmemb, f);
	if (nwritten != nmemb)
		die("fwrite:");

	return nwritten;
}

FILE *
xfopen(const char *pathname, const char *mode)
{
	FILE *f;

	f = fopen(pathname, mode);
	if (f == NULL)
		die("fopen: %s:", pathname);

	return f;
}

int
compress(FILE *infile, FILE *outfile)
{
	int ch, next, count;

	while ((ch = fgetc(infile)) != EOF) {
		next = fgetc(infile);

		if (next == EOF) {
			xfputc(0, outfile);
			xfputc(ch, outfile);

			break;
		}

		if (next == ch) {
			count = 2;

			while ((next = fgetc(infile)) == ch && count < 129)
				++count;

			xfputc(-(count - 1), outfile);
			xfputc(ch, outfile);
		} else {
			unsigned char buf[128];

			buf[0] = ch;
			buf[1] = next;
			count = 2;

			while ((next = fgetc(infile)) != buf[count - 1]
			    && next != EOF && count < 128)
				buf[count++] = next;

			if (next == buf[count - 1]) {
				ungetc(next, infile);
				--count;
			}

			xfputc(count - 1, outfile);
			xfwrite(buf, count, 1, outfile);
		}
		ungetc(next, infile);
	}

	return 0;
}

int
decompress(FILE *infile, FILE *outfile)
{
	int count;

	while ((count = fgetc(infile)) != EOF) {
		if (count > 127) {
			int ch;

			count = 257 - count;
			ch = fgetc(infile);
			while (count-- > 0)
				xfputc(ch, outfile);
		} else {
			++count;
			while (count-- > 0) {
				int ch;

				ch = fgetc(infile);
				xfputc(ch, outfile);
			}
		}
	}

	return 0;
}

int
main(int argc, char **argv)
{
	FILE *infile, *outfile;

	if (argc != 4 || argv[1][1] == '\0')
		die("args");

	infile = xfopen(argv[2], "rb");
	outfile = xfopen(argv[3], "wb");

	switch (argv[1][1]) {
	case 'c':
	case 'C':
		compress(infile, outfile);
		break;
	case 'd':
	case 'D':
		decompress(infile, outfile);
		break;
	default:
		die("args");
	}

	fclose(infile);
	fclose(outfile);

	return 0;
}
