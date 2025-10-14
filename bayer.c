#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <netpbm/pam.h>

typedef unsigned int uint;

enum RGB { R, G, B };

#if 0 /* CGA Palette 0 low intensity */
#define PALETTE_SIZE 4
int palette[PALETTE_SIZE][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0xaa, 0x00 },
	{ 0xaa, 0x00, 0x00 },
	{ 0xaa, 0x55, 0x00 },
};
#endif

#if 0 /* CGA Palette 0 high intensity */
#define PALETTE_SIZE 4
int palette[PALETTE_SIZE][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x55, 0xff, 0x55 },
	{ 0xff, 0x55, 0x55 },
	{ 0xff, 0xff, 0x55 },
};
#endif

#if 0 /* CGA Palette 1 low intensity */
#define PALETTE_SIZE 4
int palette[PALETTE_SIZE][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0xaa, 0xaa },
	{ 0xaa, 0x00, 0xaa },
	{ 0xaa, 0xaa, 0xaa },
};
#endif

#if 0 /* CGA Palette 1 high intensity */
#define PALETTE_SIZE 4
int palette[PALETTE_SIZE][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x55, 0xff, 0xff },
	{ 0xff, 0x55, 0xff },
	{ 0xff, 0xff, 0xff },
};
#endif

#if 1 /* CGA Mode5 low intensity */
#define PALETTE_SIZE 4
int palette[PALETTE_SIZE][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0xaa, 0xaa },
	{ 0xaa, 0x00, 0x00 },
	{ 0xaa, 0xaa, 0xaa },
};
#endif

#if 0 /* CGA Mode5 high intensity */
#define PALETTE_SIZE 4
int palette[PALETTE_SIZE][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x55, 0xff, 0xff },
	{ 0xff, 0x55, 0x55 },
	{ 0xff, 0xff, 0xff },
};
#endif

#if 0 /* CGA Full palette */
#define PALETTE_SIZE 16
int palette[PALETTE_SIZE][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0xaa },
	{ 0x00, 0xaa, 0x00 },
	{ 0x00, 0xaa, 0xaa },
	{ 0xaa, 0x00, 0x00 },
	{ 0xaa, 0x00, 0xaa },
	{ 0xaa, 0x55, 0x00 },
	{ 0xaa, 0xaa, 0xaa },
	{ 0x55, 0x55, 0x55 },
	{ 0x55, 0x55, 0xff },
	{ 0x55, 0xff, 0x55 },
	{ 0x55, 0xff, 0xff },
	{ 0xff, 0x55, 0x55 },
	{ 0xff, 0x55, 0xff },
	{ 0xff, 0xff, 0x55 },
	{ 0xff, 0xff, 0xff },
};
#endif

int bayer[8][8] = {
	{  0, 32,  8, 40,  2, 34, 10, 42 },
	{ 48, 16, 56, 24, 50, 18, 58, 26 },
	{ 12, 44,  4, 36, 14, 46,  6, 38 },
	{ 60, 28, 52, 20, 62, 30, 54, 22 },
	{  3, 35, 11, 43,  1, 33,  9, 41 },
	{ 51, 19, 59, 27, 49, 17, 57, 25 },
	{ 15, 47,  7, 39, 13, 45,  5, 37 },
	{ 63, 31, 55, 23, 61, 29, 53, 21 },
};

static int
pick(int *c)
{
	int maxdist = INT_MAX;
	int match = 0;
	int i;

	for (i = 0; i < PALETTE_SIZE; ++i) {
		int diff[3];
		int dist;
		int r, rf, bf;

		diff[R] = abs(c[R] - palette[i][R]);
		diff[G] = abs(c[G] - palette[i][G]);
		diff[B] = abs(c[B] - palette[i][B]);

		r = (c[R] + palette[i][R]) / 2;
		if (r < 128) {
			rf = 2;
			bf = 3;
		} else {
			rf = 3;
			bf = 2;
		}

		dist = rf * diff[R] + 4 * diff[G] + bf * diff[B];

		if (dist < maxdist) {
			maxdist = dist;
			match = i;
		}
	}

	return match;
}

int
main(int argc, char **argv)
{
	struct pam inpam, outpam;
	tuple *row;
	int x, y;

	pnm_init(&argc, argv);
	pnm_readpaminit(stdin, &inpam, PAM_STRUCT_SIZE(tuple_type));
	if (inpam.color_depth < 3) {
		fprintf(stderr, "input should have at least a depth of 3.");
		exit(1);
	}
	outpam = inpam;
	outpam.file = stdout;
	pnm_writepaminit(&outpam);

	row = pnm_allocpamrow(&inpam);
	for (y = 0; y < inpam.height; ++y) {
		pnm_readpamrow(&inpam, row);
		for (x = 0; x < inpam.width; ++x) {
			int c[3];
			int i, v;
			uint cv;

			v = bayer[y & 7][x & 7];
			cv = row[x][R] + row[x][R] * v / 64;
			c[R] = cv > 255 ? 255 : cv;

			cv = row[x][G] + row[x][G] * v / 64;
			c[G] = cv > 255 ? 255 : cv;

			cv = row[x][B] + row[x][B] * v / 64;
			c[B] = cv > 255 ? 255 : cv;

			i = pick(c);
			row[x][R] = palette[i][R];
			row[x][G] = palette[i][G];
			row[x][B] = palette[i][B];
		}
		pnm_writepamrow(&outpam, row);
	}
	pnm_freepamrow(row);

	return 0;
}
