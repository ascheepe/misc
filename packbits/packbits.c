/*
 * Copyright (C) 2021 Axel Scheepers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * RLE Header (packbits variant)
 *
 *  0 to  127: (1 + n) literal bytes of data
 * -1 to -127: one byte of data repeated (1 - n) times
 * -128      : no operation, next byte is header byte
 */
void
decompress(FILE *infile, FILE *outfile)
{
	int byte = EOF;

	while ((byte = getc(infile)) != EOF) {
		int nreps = 0;
		int nlit = 0;
		int next = EOF;

		/* Repetitions. */
		if (byte > 128) {
			byte = 256 - byte;

			/* no-op. */
			if (byte == 128)
				continue;

			nreps = 1 + byte;
			next = fgetc(infile);

			if (next == EOF)
				goto error;

			while (nreps-- > 0)
				fputc(next, outfile);
		}

		/* Literal data. */
		else {
			nlit = 1 + byte;

			while (nlit-- > 0) {
				next = fgetc(infile);

				if (next == EOF)
					goto error;

				fputc(next, outfile);
			}
		}
	}

	return;

 error:
	fprintf(stderr, "unexpected input\n");
}

void
compress(FILE *infile, FILE *outfile)
{
	int byte = EOF;

	while ((byte = fgetc(infile)) != EOF) {
		int nreps = 0;
		int nlit = 0;
		int next = EOF;

		next = fgetc(infile);

		/* output the last byte if at EOF */
		if (next == EOF) {
			fputc(0, outfile);
			fputc(byte, outfile);

			return;
		}

		/* Repetitions. */
		if (next == byte) {
			nreps = 1;

			while ((next = fgetc(infile)) == byte && nreps < 127)
				++nreps;

			fputc(-nreps, outfile);
			fputc(byte, outfile);
		}

		/* Literal data. */
		else {
			char buffer[128] = { 0 };
			int buffer_length = 0;

			/* fill buffer with what we already know */
			buffer[0] = byte;
			buffer[1] = next;
			buffer_length = 2;
			nlit = 1;

			/* read until we encounter a repetition or EOF */
			while ((next =
				fgetc(infile)) != buffer[buffer_length - 1]
			    && next != EOF && nlit < 127) {
				++nlit;
				buffer[buffer_length++] = next;
			}

			/* save repetition for the next run */
			if (next == buffer[buffer_length - 1]) {
				ungetc(next, infile);
				--nlit;
				--buffer_length;
			}

			fputc(nlit, outfile);
			fwrite(buffer, buffer_length, 1, outfile);
		}

		/*
		 * Either way unread the last read byte
		 * since it starts a new header
		 */
		ungetc(next, infile);
	}
}

static void
usage(void)
{
	fprintf(stderr, "usage: rle -cd infile outfile\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	FILE *infile = NULL;
	FILE *outfile = NULL;

	if (argc != 4)
		usage();

	infile = fopen(argv[2], "rb");
	outfile = fopen(argv[3], "wb");

	if (infile == NULL || outfile == NULL) {
		fprintf(stderr, "error opening file(s)\n");
		exit(1);
	}

	switch (argv[1][1]) {
	case 'C':
	case 'c':
		compress(infile, outfile);
		break;

	case 'D':
	case 'd':
		decompress(infile, outfile);
		break;

	default:
		usage();
	}

	fclose(infile);
	fclose(outfile);
	return 0;
}
