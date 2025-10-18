#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>

#include <netpbm/pam.h>

typedef unsigned int uint;

enum RGB { R, G, B };

int cga0l_palette[4 * 3] = {
	0x00, 0x00, 0x00,
	0x00, 0xaa, 0x00,
	0xaa, 0x00, 0x00,
	0xaa, 0x55, 0x00,
};

int cga0h_palette[4 * 3] = {
	0x00, 0x00, 0x00,
	0x55, 0xff, 0x55,
	0xff, 0x55, 0x55,
	0xff, 0xff, 0x55,
};

int cga1l_palette[4 * 3] = {
	0x00, 0x00, 0x00,
	0x00, 0xaa, 0xaa,
	0xaa, 0x00, 0xaa,
	0xaa, 0xaa, 0xaa,
};

int cga1h_palette[4 * 3] = {
	0x00, 0x00, 0x00,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0xff,
	0xff, 0xff, 0xff,
};

int mode5l_palette[4 * 3] = {
	0x00, 0x00, 0x00,
	0x00, 0xaa, 0xaa,
	0xaa, 0x00, 0x00,
	0xaa, 0xaa, 0xaa,
};

int mode5h_palette[4 * 3] = {
	0x00, 0x00, 0x00,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0xff, 0xff,
};

int cga_full_palette[16 * 3] = {
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xaa,
	0x00, 0xaa, 0x00,
	0x00, 0xaa, 0xaa,
	0xaa, 0x00, 0x00,
	0xaa, 0x00, 0xaa,
	0xaa, 0x55, 0x00,
	0xaa, 0xaa, 0xaa,
	0x55, 0x55, 0x55,
	0x55, 0x55, 0xff,
	0x55, 0xff, 0x55,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0x55, 0xff,
	0xff, 0xff, 0x55,
	0xff, 0xff, 0xff,
};

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
pick(int *c, int *palette, int len)
{
	int maxdist = INT_MAX;
	int match = 0;
	int i;

	for (i = 0; i < len; ++i) {
		int w[3] = { 3, 4, 2 };
		int diff[3];
		int dist;
		int *pal;

		pal = &palette[i * 3];
		diff[R] = abs(c[R] - pal[R]);
		diff[G] = abs(c[G] - pal[G]);
		diff[B] = abs(c[B] - pal[B]);

		if ((c[R] + pal[R]) / 2 < 128) {
			w[R] = 2;
			w[B] = 3;
		}

		dist = w[R] * diff[R] + w[G] * diff[G] + w[B] * diff[B];
		if (dist < maxdist) {
			maxdist = dist;
			match = i;
		}
	}

	return match;
}

char *
strtolower(char *s)
{
	char *p = s;

	while (*p) {
		*p = tolower(*p);
		++p;
	}

	return s;
}

char *
xstrdup(char *s)
{
	char *t;
	size_t sz;

	sz = strlen(s) + 1;
	t = malloc(sz);
	if (t == NULL) {
		fprintf(stderr, "xstrdup: no more memory.\n");
		exit(1);
	}
	memcpy(t, s, sz);

	return t;
}

int
main(int argc, char **argv)
{
	struct pam inpam, outpam;
	tuple *row;
	int x, y;
	char *palette_name = "cga_full";
	int *palette = cga_full_palette;
	int len = 16;
	int opt;

	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			palette_name = strtolower(xstrdup(optarg));
			break;
		default:
			fprintf(stderr, "usage: %s [-p palette_name]\n", argv[0]);
			exit(1);
		}
	}

	if (strcmp(palette_name, "cga0l") == 0) {
		palette = cga0l_palette;
		len = 4;
	} else if (strcmp(palette_name, "cga0h") == 0) {
		palette = cga0h_palette;
		len = 4;
	} else if (strcmp(palette_name, "cga1l") == 0) {
		palette = cga1l_palette;
		len = 4;
	} else if (strcmp(palette_name, "cga1h") == 0) {
		palette = cga1h_palette;
		len = 4;
	} else if (strcmp(palette_name, "mode5l") == 0) {
		palette = mode5l_palette;
		len = 4;
	} else if (strcmp(palette_name, "mode5h") == 0) {
		palette = mode5h_palette;
		len = 4;
	} else if (strcmp(palette_name, "cga_full") == 0) {
		palette = cga_full_palette;
		len = 16;
	}

	pm_init(argv[0], 0);
	pnm_readpaminit(stdin, &inpam, PAM_STRUCT_SIZE(tuple_type));
	if (inpam.depth < 3) {
		fprintf(stderr, "input should have at least a depth of 3.\n");
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
			int *pal;
			int i;
			uint bv, cv;

			bv = bayer[y & 7][x & 7];
			cv = row[x][R] + row[x][R] * bv / 64;
			c[R] = cv > 255 ? 255 : cv;

			cv = row[x][G] + row[x][G] * bv / 64;
			c[G] = cv > 255 ? 255 : cv;

			cv = row[x][B] + row[x][B] * bv / 64;
			c[B] = cv > 255 ? 255 : cv;

			i = pick(c, palette, len);
			pal = &palette[i * 3];
			row[x][R] = pal[R];
			row[x][G] = pal[G];
			row[x][B] = pal[B];
		}
		pnm_writepamrow(&outpam, row);
	}
	pnm_freepamrow(row);

	return 0;
}
