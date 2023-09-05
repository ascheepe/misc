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
	int r;

	r = fputc(c, f);
	if (r == EOF)
		die("xfputc:");

	return c;
}

size_t
xfwrite(const void *p, size_t size, size_t nmemb, FILE *f)
{
	size_t r;

	r = fwrite(p, size, nmemb, f);
	if (r != nmemb)
		die("xfwrite:");

	return nmemb;
}

FILE *
xfopen(const char *pathname, const char *mode)
{
	FILE *f;

	f = fopen(pathname, mode);
	if (f == NULL)
		die("xfopen(%s, %s):", pathname, mode);

	return f;
}

int
compress(FILE *infile, FILE *outfile)
{
	int ch, next, cnt;

	while ((ch = fgetc(infile)) != EOF) {
		next = fgetc(infile);

		if (next == EOF) {
			xfputc(0, outfile);
			xfputc(ch, outfile);

			break;
		}

		if (next == ch) {
			cnt = 2;

			while ((next = fgetc(infile)) == ch && cnt < 129)
				++cnt;

			xfputc(-(cnt - 1), outfile);
			xfputc(ch, outfile);
		} else {
			unsigned char buf[128];

			buf[0] = ch;
			buf[1] = next;
			cnt = 2;

			while ((next = fgetc(infile)) != buf[cnt - 1]
			    && next != EOF && cnt < 128)
				buf[cnt++] = next;

			if (next == buf[cnt - 1]) {
				ungetc(next, infile);
				--cnt;
			}

			xfputc(cnt - 1, outfile);
			xfwrite(buf, cnt, 1, outfile);
		}
		ungetc(next, infile);
	}

	return 0;
}

int
decompress(FILE *infile, FILE *outfile)
{
	int cnt;

	while ((cnt = fgetc(infile)) != EOF) {
		if (cnt > 127) {
			int ch;

			cnt = 257 - cnt;
			ch = fgetc(infile);
			while (cnt-- > 0)
				xfputc(ch, outfile);
		} else {
			++cnt;
			while (cnt-- > 0) {
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

