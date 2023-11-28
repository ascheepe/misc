#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAXLINELEN 8192

enum hues {
	WHITE, AMBER, CYAN, GREEN
};

struct rgb {
	unsigned int r, g, b;
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
parse_hexcolor(const char *str)
{
	struct rgb *color = NULL;
	unsigned int r, g, b;
	char c;

	if (sscanf(str, "#%02x%02x%02x%c", &r, &g, &b, &c) == 4) {
		if (!isspace(c))
			return NULL;

		color = xmalloc(sizeof(*color));
		color->r = r;
		color->g = g;
		color->b = b;
	} else if (sscanf(str, "#%1x%1x%1x%c", &r, &g, &b, &c) == 4) {
		if (!isspace(c))
			return NULL;

		color = xmalloc(sizeof(*color));
		color->r = r + 16 * r;
		color->g = g + 16 * g;
		color->b = b + 16 * b;
	}

	return color;
}

static struct rgb *
parse_rgbcolor(const char *str)
{
	struct rgb *color = NULL;
	int r, g, b;

	if (sscanf(str, "rgb(%d, %d, %d)", &r, &g, &b) == 3) {
		if (r < 0 || r > 256 || g < 0 || g > 256 ||
		    b < 0 || b > 256) {
			fprintf(stderr, "WARNING: invalid color ");
			fprintf(stderr, "rgb(%d, %d, %d) found.", r, g, b);
			return NULL;
		}
		color = xmalloc(sizeof(*color));
		color->r = r;
		color->g = g;
		color->b = b;
	}

	return color;
}

static void
color_to_mono(struct rgb *color, enum hues hue)
{
	int gray;

	gray = (int)((color->r * 299L + 500) / 1000 +
	             (color->g * 587L + 500) / 1000 +
	             (color->b * 114L + 500) / 1000);

	switch (hue) {
	default:
	case WHITE:
		color->r = gray;
		color->g = gray;
		color->b = gray;
		break;
	case AMBER:
		color->r = gray;
		color->g = (unsigned int)(gray * 191L / 255);
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
process_line(const char *line, FILE *fout, enum hues hue)
{
	const char *pos = line;

	while (*pos != '\0') {
		struct rgb *color;

		if ((color = parse_hexcolor(pos)) != NULL) {
			color_to_mono(color, hue);
			fprintf(fout, "#%02x%02x%02x",
			    color->r, color->g, color->b);
			free(color);

			while (!isspace(*pos))
				++pos;
		} else if ((color = parse_rgbcolor(pos)) != NULL) {
			color_to_mono(color, hue);
			fprintf(fout, "rgb(%d, %d, %d)",
			    color->r, color->g, color->b);
			free(color);

			while (*pos != ')')
				++pos;
			++pos;
		} else
			fputc(*pos++, fout);
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
	char line[MAXLINELEN];
	enum hues hue;

	hue = get_hue(argc, argv);

	while (fgets(line, sizeof(line), stdin) != NULL)
		process_line(line, stdout, hue);

	return 0;
}
