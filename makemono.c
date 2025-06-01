#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXLINELEN 8192

enum hues {
	WHITE, AMBER, CYAN, GREEN
};

enum color_types {
	HEXCOLOR, RGBCOLOR
};

struct color {
	enum color_types type;
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

struct color *
parse_hexcolor(const char *str, size_t *len)
{
	struct color *color = NULL;
	unsigned int r, g, b;

	if (*str != '#' || len == NULL)
		return NULL;

	color = xmalloc(sizeof(*color));
	*len = 0;

	if (sscanf(str, "#%02x%02x%02x", &r, &g, &b) == 3) {
		if (!(isspace(str[7]) || str[7] == '\0'))
			goto err;

		color->r = r;
		color->g = g;
		color->b = b;
		*len = 7;
	} else if (sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3) {
		if (!(isspace(str[4]) || str[4] == '\0'))
			goto err;

		color->r = 16 * r + r;
		color->g = 16 * g + g;
		color->b = 16 * b + b;
		*len = 4;
	}

	color->type = HEXCOLOR;
	return color;

err:
	free(color);
	return NULL;
}

static int
is_valid_rgb(int r, int g, int b)
{
	if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
		fprintf(stderr,
		    "WARNING: invalid color rgb(%d, %d, %d) found.\n",
		    r, g, b);
		return 0;
	}

	return 1;
}

static struct color *
parse_rgbcolor(const char *str, size_t *len)
{
	struct color *color = NULL;
	int r, g, b;

	if (*str != 'r')
		return NULL;

	if (sscanf(str, "rgb(%d, %d, %d)", &r, &g, &b) == 3 && 
	    is_valid_rgb(r, g, b)) {
		const char *p;

		color = xmalloc(sizeof(*color));
		color->type = RGBCOLOR;
		color->r = r;
		color->g = g;
		color->b = b;

		p = str + sizeof("rgb(X,X,X") - 1;
		while (*p++ != ')')
			continue;
		*len = p - str;
	}

	return color;
}

static void
print_color(const struct color *color, FILE *fp)
{
	char *fmt;

	switch (color->type) {
	case HEXCOLOR:
		fmt = "#%02x%02x%02x";
		break;
	case RGBCOLOR:
		fmt = "rgb(%d, %d, %d)";
		break;
	default:
		fprintf(stderr, "illegal colortype %d\n", color->type);
		exit(1);
	}

	fprintf(fp, fmt, color->r, color->g, color->b);
}

static void
color_to_mono(struct color *color, enum hues hue)
{
	int gray;

	gray = (int)((color->r * 299L + 500) / 1000
	    + (color->g * 587L + 500) / 1000
	    + (color->b * 114L + 500) / 1000);

	switch (hue) {
	default:
	case WHITE:
		color->r = gray;
		color->g = gray;
		color->b = gray;
		break;
	case AMBER:
		color->r = gray;
		color->g = (unsigned char)(gray * 191L / 255);
		color->b = 0;
		break;
	case CYAN:
		color->r = 0;
		color->g = gray;
		color->b = gray;
		break;
	case GREEN:
		color->r = 0;
		color->g = gray;
		color->b = 0;
		break;
	}
}

static void
make_mono(const char *line, FILE *fp, enum hues hue)
{
	const char *p = line;

	while (*p != '\0') {
		struct color *color;
		size_t len;

		if ((color = parse_hexcolor(p, &len)) != NULL ||
		    (color = parse_rgbcolor(p, &len)) != NULL) {
			color_to_mono(color, hue);
			print_color(color, fp);
			free(color);
		} else {
			fputc(*p, fp);
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
		make_mono(line, stdout, hue);

	return 0;
}
