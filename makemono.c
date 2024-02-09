#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXLINELEN 8192

enum hues {
	WHITE, AMBER, CYAN, GREEN
};

struct rgb {
	unsigned char r, g, b;
};

static void *
xmalloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (ptr == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(EXIT_FAILURE);
	}

	return ptr;
}

struct rgb *
parse_hexcolor(const char *str, size_t *len)
{
	struct rgb *c = NULL;
	unsigned int r, g, b;

	if (*str != '#')
		return NULL;

	if (sscanf(str, "#%02x%02x%02x", &r, &g, &b) == 3) {
		if (!(isspace(str[7]) || str[7] == '\0'))
			return NULL;

		c = xmalloc(sizeof(*c));
		c->r = r;
		c->g = g;
		c->b = b;
		*len = 7;
	} else if (sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3) {
		if (!(isspace(str[4]) || str[4] == '\0'))
			return NULL;

		c = xmalloc(sizeof(*c));
		c->r = r + 16 * r;
		c->g = g + 16 * g;
		c->b = b + 16 * b;
		*len = 4;
	}

	return c;
}

static void
print_hexcolor(const struct rgb *c, FILE *f)
{
	fprintf(f, "#%02x%02x%02x", c->r, c->g, c->b);
}

static int
valid_rgb(int r, int g, int b)
{
	if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
		fprintf(stderr,
		    "WARNING: invalid color rgb(%d, %d, %d) found.\n",
		    r, g, b);
		return 0;
	}

	return 1;
}

static struct rgb *
parse_rgbcolor(const char *str, size_t *len)
{
	struct rgb *c = NULL;
	int r, g, b;

	if (*str != 'r')
		return NULL;

	if (sscanf(str, "rgb(%d, %d, %d)", &r, &g, &b) == 3) {
		const char *p;

		if (!valid_rgb(r, g, b))
			return NULL;

		c = xmalloc(sizeof(*c));
		c->r = r;
		c->g = g;
		c->b = b;

		p = str + sizeof("rgb(X,X,X") - 1;
		while (*p++ != ')');
		*len = p - str;
	}

	return c;
}

static void
print_rgbcolor(const struct rgb *c, FILE *f)
{
	fprintf(f, "rgb(%d, %d, %d)", c->r, c->g, c->b);
}

static void
color_to_mono(struct rgb *c, enum hues hue)
{
	int gray;

	gray = (int)
	     ((c->r * 299L + 500) / 1000
	    + (c->g * 587L + 500) / 1000
	    + (c->b * 114L + 500) / 1000);

	switch (hue) {
	default:
	case WHITE:
		c->r = gray;
		c->g = gray;
		c->b = gray;
		break;
	case AMBER:
		c->r = gray;
		c->g = (unsigned char)(gray * 191L / 255);
		c->b = 0;
		break;
	case CYAN:
		c->r = 0;
		c->g = gray;
		c->b = gray;
		break;
	case GREEN:
		c->r = 0;
		c->g = gray;
		c->b = 0;
		break;
	}
}

static void
process_line(const char *line, FILE *f, enum hues hue)
{
	const char *p = line;

	while (*p != '\0') {
		struct rgb *c;
		size_t len;

		if ((c = parse_hexcolor(p, &len)) != NULL) {
			color_to_mono(c, hue);
			print_hexcolor(c, f);
			free(c);
		} else if ((c = parse_rgbcolor(p, &len)) != NULL) {
			color_to_mono(c, hue);
			print_rgbcolor(c, f);
			free(c);
		} else {
			fputc(*p, f);
			len = 1;
		}

		p += len;
	}
}

static enum hues
get_hue(int argc, char **argv)
{
	enum hues hue = WHITE;

	if (argc < 2)
		return hue;

	if (argv[1][0] == '-') {
		switch (argv[1][1]) {
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
	char line[MAXLINELEN];
	enum hues hue;

	hue = get_hue(argc, argv);

	while (fgets(line, sizeof(line), stdin) != NULL)
		process_line(line, stdout, hue);

	return 0;
}
