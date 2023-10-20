#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAXLINELEN 8192
#define BUFSIZE 64

enum hues {
	AMBER,
	CYAN,
	GREEN,
	WHITE
};

struct rgb {
	int r, g, b;
};

/*
 * Check to see if the string at str is a valid color.
 * A color is valid when it starts with a pound(#) sign
 * followed by either three or six hexadecimal digits.
 * It should end with either a space, tab, newline or
 * NULL character.
 */
static int
is_valid_color(char *str)
{
	if (*str == '#') {
		int i = 1;

		while (isxdigit(str[i]) && i < 8)
			++i;

		if ((i == 4 || i == 7) &&
		    (isspace(str[i]) || str[i] == '\0'))
			return 1;
	}

	return 0;
}

/*
 * Find a color definition in the given string and return the
 * position of it, or NULL if not found.
 */
static char *
find_color(char *str)
{
	while (*str != '\0') {
		if (is_valid_color(str))
			return str;
		++str;
	}

	return NULL;
}

/*
 * Return the length of a color string.
 */
static int
color_length(const char *str)
{
	int len = 1;

	while (isxdigit(*++str))
		++len;

	return len;
}

/*
 * Split a hex color string into separate red, green and blue
 * color values.
 */
static void
str_to_rgb(const char *str, struct rgb *c)
{
	char buf[3] = { 0 };

	if (color_length(str) == 4) {
		buf[0] = str[1];
		buf[1] = str[1];
		c->r = strtol(buf, NULL, 16);

		buf[0] = str[2];
		buf[1] = str[2];
		c->g = strtol(buf, NULL, 16);

		buf[0] = str[3];
		buf[1] = str[3];
		c->b = strtol(buf, NULL, 16);
	} else {
		buf[0] = str[1];
		buf[1] = str[2];
		c->r = strtol(buf, NULL, 16);

		buf[0] = str[3];
		buf[1] = str[4];
		c->g = strtol(buf, NULL, 16);

		buf[0] = str[5];
		buf[1] = str[6];
		c->b = strtol(buf, NULL, 16);
	}
}

static int
is_monochrome(const struct rgb *c)
{
	return c->r == c->g && c->r == c->b;
}

static int
color_to_luma(const struct rgb *c)
{
	return c->r * 299 / 1000 + c->g * 587 / 1000 + c->b * 114 / 1000;
}

static void
length_warning(int is, int shouldbe)
{
	fprintf(stderr,
	    "Unexpected length of %d should be %d; skipping replacement.\n",
	    is, shouldbe);
}

/*
 * Replace the color string at position pos with a monochrome version of it.
 */
static void
replace_color(char *str, enum hues hue)
{
	char buf[BUFSIZE];
	struct rgb c;
	int len;
	int Y;

	str_to_rgb(str, &c);
	if (hue == WHITE && is_monochrome(&c))
		return;

	Y = color_to_luma(&c);
	switch (hue) {
	case AMBER:
		c.r = Y;
		c.g = Y * 191 / 255;
		c.b = 0;
		break;
	case CYAN:
		c.r = 0;
		c.g = Y;
		c.b = Y;
		break;
	case GREEN:
		c.r = 0;
		c.g = Y;
		c.b = 0;
		break;
	case WHITE:
	default:
		c.r = c.g = c.b = Y;
		break;
	}

	if (color_length(str) == 4) {
		c.r = (c.r + 8) / 16;
		c.g = (c.g + 8) / 16;
		c.b = (c.b + 8) / 16;
		len = sprintf(buf, "#%1x%1x%1x", c.r, c.g, c.b);
		if (len == 4)
			memcpy(str, buf, 4);
		else
			length_warning(len, 4);
	} else {
		len = sprintf(buf, "#%02x%02x%02x", c.r, c.g, c.b);
		if (len == 7)
			memcpy(str, buf, 7);
		else
			length_warning(len, 7);
	}
}

static enum hues
get_hue(int argc, char **argv)
{
	enum hues hue = WHITE;
	int opt;

	while ((opt = getopt(argc, argv, "acgw")) != -1) {
		switch (opt) {
		case 'a':
			hue = AMBER;
			break;
		case 'c':
			hue = CYAN;
			break;
		case 'g':
			hue = GREEN;
			break;
		case 'w':
			hue = WHITE;
			break;
		}
	}

	return hue;
}

int
main(int argc, char **argv)
{
	char buf[MAXLINELEN];
	enum hues hue;

	hue = get_hue(argc, argv);

	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		char *pos = buf;

		while ((pos = find_color(pos)) != NULL) {
			/*
			 * skip any color definitions directly
			 * following non blanks.
			 */
			if (pos > buf && !isspace(pos[-1])) {
				pos += color_length(pos);
				continue;
			}

			replace_color(pos, hue);
			pos += color_length(pos);
		}

		fwrite(buf, strlen(buf), 1, stdout);
	}

	return 0;
}
