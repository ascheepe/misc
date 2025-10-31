#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netpbm/pam.h>
#include <netpbm/pbm.h>

enum { R, G, B };
typedef unsigned int uint;

int
main(int argc, char **argv)
{
    struct pam inpam;
    tuple *row;
    bit *outrow;
    int *err = NULL;
    int cols, width, height;
    int x, y;

    (void)argc;
    pm_init(argv[0], 0);

    pnm_readpaminit(stdin, &inpam, PAM_STRUCT_SIZE(tuple_type));
    if (inpam.depth < 3) {
        fprintf(stderr, "input should have at least a depth of 3.\n");
        exit(1);
    }

    width  = inpam.width;
    height = inpam.height;
    row = pnm_allocpamrow(&inpam);

    /* Add horizontal padding to access x-1, x+2 */
    cols = width + 3;
    err = calloc(3 * cols, sizeof(*err));
    if (!err) {
        perror("calloc");
        exit(1);
    }

    /* Initialize PBM output */
    pbm_writepbminit(stdout, width, height, 0);
    outrow = pbm_allocrow(width);

    for (y = 0; y < height; ++y) {
        int i0 = (y + 0) % 3;
        int i1 = (y + 1) % 3;
        int i2 = (y + 2) % 3;

        pnm_readpamrow(&inpam, row);

        /* Clear the error buffer for row y+2 */
        memset(&err[i2 * cols], 0, cols * sizeof(*err));

        for (x = 0; x < width; ++x) {
            int Y, cv, diff, idx;

            /* +1 offset for left padding */
            idx = x + 1;

            /* Compute grayscale intensity */
            Y = row[x][R] * 21 / 100 +
                row[x][G] * 72 / 100 +
                row[x][B] *  7 / 100;

            Y += err[i0 * cols + idx];

            cv = (Y > 127) ? 255 : 0;
            diff = Y - cv;

            /* Distribute error */
            err[i0 * cols + idx + 1] += diff / 8;
            err[i0 * cols + idx + 2] += diff / 8;

            err[i1 * cols + idx - 1] += diff / 8;
            err[i1 * cols + idx + 0] += diff / 8;
            err[i1 * cols + idx + 1] += diff / 8;

            err[i2 * cols + idx + 0] += diff / 8;

            outrow[x] = (cv == 0) ? PBM_BLACK : PBM_WHITE;
        }

        pbm_writepbmrow(stdout, outrow, width, 0);
    }

    pbm_freerow(outrow);
    free(err);
    pnm_freepamrow(row);
    return 0;
}
