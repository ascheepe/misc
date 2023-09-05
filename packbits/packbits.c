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
compress(FILE *infile, FILE *outfile)
{
	int ch, next, cnt;

	while ((ch = fgetc(infile)) != EOF) {
		next = fgetc(infile);

		if (next == EOF) {
			fputc(0, outfile);
			fputc(ch, outfile);
			break;
		}

		if (next == ch) {
			cnt = 2;

			while ((next = fgetc(infile)) == ch && cnt < 129)
				++cnt;

			fputc(-(cnt - 1), outfile);
			fputc(ch, outfile);
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

			fputc(cnt - 1, outfile);
			fwrite(buf, cnt, 1, outfile);
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
				fputc(ch, outfile);
		} else {
			++cnt;
			while (cnt-- > 0) {
				int ch;

				ch = fgetc(infile);
				fputc(ch, outfile);
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

	infile = fopen(argv[2], "rb");
	if (infile == NULL)
		die("%s:", argv[2]);

	outfile = fopen(argv[3], "wb");
	if (outfile == NULL)
		die("%s:", argv[3]);

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

	return 0;
}

