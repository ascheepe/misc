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

static void *xmalloc(size_t size)
{
    void *ptr;

    ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

struct rgb *parse_hexcolor(const char *str, size_t *length)
{
    struct rgb *color = NULL;
    unsigned int r, g, b;

    if (*str != '#') {
        return NULL;
    }

    if (sscanf(str, "#%02x%02x%02x", &r, &g, &b) == 3) {
        if (!(isspace(str[7]) || str[7] == '\0')) {
            return NULL;
        }

        color = xmalloc(sizeof(*color));
        color->r = r;
        color->g = g;
        color->b = b;
        *length = 7;
    } else if (sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3) {
        if (!(isspace(str[4]) || str[4] == '\0')) {
            return NULL;
        }

        color = xmalloc(sizeof(*color));
        color->r = r + 16 * r;
        color->g = g + 16 * g;
        color->b = b + 16 * b;
        *length = 4;
    }

    return color;
}

static void print_hexcolor(const struct rgb *color, FILE *out)
{
    fprintf(out, "#%02x%02x%02x", color->r, color->g, color->b);
}

static struct rgb *parse_rgbcolor(const char *str, size_t *color_length)
{
    struct rgb *color = NULL;
    unsigned int r, g, b;

    if (*str != 'r') {
        return NULL;
    }

    if (sscanf(str, "rgb(%u, %u, %u)", &r, &g, &b) == 3) {
        const char *color_start;
        const char *color_end;

        if (r > 255 || g > 255 || b > 255) {
            fprintf(stderr,
                    "WARNING: invalid color rgb(%d, %d, %d) found.\n",
                    r, g, b);
            return NULL;
        }

        color = xmalloc(sizeof(*color));
        color->r = r;
        color->g = g;
        color->b = b;

        color_start = str;
        color_end = str + sizeof("rgb(X,X,X") - 1;
        while (*color_end++ != ')') {
            continue;
        }
        *color_length = color_end - color_start;
    }

    return color;
}

static void print_rgbcolor(const struct rgb *color, FILE *out)
{
    fprintf(out, "rgb(%d, %d, %d)", color->r, color->g, color->b);
}

static void color_to_mono(struct rgb *color, enum hues hue)
{
    int gray;

    gray = (int) ((color->r * 299L + 500) / 1000
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
            color->g = (unsigned char) (gray * 191L / 255);
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

static void process_line(const char *line, FILE *output_file, enum hues hue)
{
    const char *cursor = line;

    while (*cursor != '\0') {
        struct rgb *color;
        size_t length;

        if ((color = parse_hexcolor(cursor, &length)) != NULL) {
            color_to_mono(color, hue);
            print_hexcolor(color, output_file);
            free(color);
        } else if ((color = parse_rgbcolor(cursor, &length)) != NULL) {
            color_to_mono(color, hue);
            print_rgbcolor(color, output_file);
            free(color);
        } else {
            fputc(*cursor, output_file);
            length = 1;
        }

        cursor += length;
    }
}

static enum hues get_hue(int argc, char **argv)
{
    enum hues hue = WHITE;

    if (argc < 2) {
        return hue;
    }

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

int main(int argc, char **argv)
{
    char line[MAXLINELEN];
    enum hues hue;

    hue = get_hue(argc, argv);

    while (fgets(line, sizeof(line), stdin) != NULL) {
        process_line(line, stdout, hue);
    }

    return 0;
}
