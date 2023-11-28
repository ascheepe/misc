#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAXLINELEN 8192

enum { false, true };

enum hues {
	AMBER,
	CYAN,
	GREEN,
	WHITE
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
	struct rgb *ret;
	unsigned int r, g, b;
	char tail;

	if (sscanf(str, "#%02x%02x%02x%c", &r, &g, &b, &tail) == 4) {
		if (!isspace(tail))
			return NULL;

		ret = xmalloc(sizeof(*ret));
		ret->r = r;
		ret->g = g;
		ret->b = b;

		return ret;
	}

	if (sscanf(str, "#%1x%1x%1x%c", &r, &g, &b, &tail) == 4) {
		if (!isspace(tail))
			return NULL;

		ret = xmalloc(sizeof(*ret));
		ret->r = r + 16 * r;
		ret->g = g + 16 * g;
		ret->b = b + 16 * b;

		return ret;
	}

	return NULL;
}

static struct rgb *
parse_rgbcolor(const char *str)
{
	struct rgb *ret;
	int r, g, b;

	if (sscanf(str, "rgb(%d, %d, %d)", &r, &g, &b) == 3) {
		ret = xmalloc(sizeof(*ret));
		ret->r = r;
		ret->g = g;
		ret->b = b;

		return ret;
	}

	return NULL;
}

static void
color_to_mono(struct rgb *color, enum hues hue)
{
	int gray;

	gray = ((color->r * 299L + 500) / 1000 +
	        (color->g * 587L + 500) / 1000 +
	        (color->b * 114L + 500) / 1000);

	switch (hue) {
	case AMBER:
		color->r = gray;
		color->g = gray * 191L / 255;
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
	case WHITE:
	default:
		color->r = gray;
		color->g = gray;
		color->b = gray;
		break;
	}
}

static void
process_line(const char *line, FILE *fout, enum hues hue)
{
	const char *pos, *end;

	pos = line;
	end = line + strlen(line);

	while (pos < end) {
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
