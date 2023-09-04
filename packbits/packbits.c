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
void decompress(FILE *input_file, FILE *output_file)
{
    int byte = EOF;

    while ((byte = getc(input_file)) != EOF) {
        int next_byte = EOF;
        int count = 0;

        /* Repetitions. */
        if (byte > 128) {
            byte = 256 - byte;

            /* no-op. */
            if (byte == 128) {
                continue;
            }

            count = 1 + byte;
            next_byte = fgetc(input_file);

            if (next_byte == EOF) {
                goto error;
            }

            while (count-- > 0) {
                fputc(next_byte, output_file);
            }
        }

        /* Literal data. */
        else {
            count = 1 + byte;

            while (count-- > 0) {
                next_byte = fgetc(input_file);

                if (next_byte == EOF) {
                    goto error;
                }

                fputc(next_byte, output_file);
            }
        }
    }

    return;

  error:
    fprintf(stderr, "unexpected input\n");
}

void compress(FILE *input_file, FILE *output_file)
{
    int byte = EOF;

    while ((byte = fgetc(input_file)) != EOF) {
        int next_byte = EOF;
        int count = 0;

        next_byte = fgetc(input_file);

        /* output the last byte if at EOF */
        if (next_byte == EOF) {
            fputc(0, output_file);
            fputc(byte, output_file);

            return;
        }

        /* Repetitions. */
        if (next_byte == byte) {
            count = 1;

            while ((next_byte = fgetc(input_file)) == byte
                   && count < 127) {
                ++count;
            }

            fputc(-count, output_file);
            fputc(byte, output_file);
        }

        /* Literal data. */
        else {
            char buffer[128] = { 0 };

            /* fill buffer with what we already know */
            buffer[0] = byte;
            buffer[1] = next_byte;
            count = 1;

            /* read until we encounter a repetition or EOF */
            while ((next_byte = fgetc(input_file)) != buffer[count]
                   && next_byte != EOF
                   && count < 127) {
                ++count;
                buffer[count] = next_byte;
            }

            /* save repetition for the next run */
            if (next_byte == buffer[count]) {
                ungetc(next_byte, input_file);
                --count;
            }

            fputc(count, output_file);
            fwrite(buffer, count + 1, 1, output_file);
        }

        /*
         * Either way unread the last read byte
         * since it starts a new header
         */
        ungetc(next_byte, input_file);
    }
}

static void usage(void)
{
    fprintf(stderr, "usage: rle -cd input_file output_file\n");
    exit(1);
}

int main(int argc, char **argv)
{
    FILE *input_file = NULL;
    FILE *output_file = NULL;

    if (argc != 4) {
        usage();
    }

    input_file = fopen(argv[2], "rb");
    output_file = fopen(argv[3], "wb");

    if (input_file == NULL || output_file == NULL) {
        fprintf(stderr, "error opening file(s)\n");
        exit(1);
    }

    switch (argv[1][1]) {
        case 'C':
        case 'c':
            compress(input_file, output_file);
            break;

        case 'D':
        case 'd':
            decompress(input_file, output_file);
            break;

        default:
            usage();
    }

    fclose(input_file);
    fclose(output_file);
    return 0;
}
