#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXLINELEN 8192

enum hues {
    hue_white, hue_amber, hue_cyan, hue_green
};

struct rgb {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

static void *xmalloc(size_t size) {
    void *ptr;

    ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

struct rgb *parse_hexcolor(const char *str, size_t *color_length) {
    struct rgb *color = NULL;
    unsigned int red;
    unsigned int green;
    unsigned int blue;

    if (*str != '#') {
        return NULL;
    }

    if (sscanf(str, "#%02x%02x%02x", &red, &green, &blue) == 3) {
        if (!(isspace(str[7]) || str[7] == '\0')) {
            return NULL;
        }

        color = xmalloc(sizeof(*color));
        color->red = red;
        color->green = green;
        color->blue = blue;
        *color_length = 7;
    } else if (sscanf(str, "#%1x%1x%1x", &red, &green, &blue) == 3) {
        if (!(isspace(str[4]) || str[4] == '\0')) {
            return NULL;
        }

        color = xmalloc(sizeof(*color));
        color->red = 16 * red + red;
        color->green = 16 * green + green;
        color->blue = 16 * blue + blue;
        *color_length = 4;
    }

    return color;
}

static void print_hexcolor(const struct rgb *color, FILE *output_file) {
    fprintf(output_file, "#%02x%02x%02x",
            color->red, color->green, color->blue);
}

static int is_valid_rgb(int red, int green, int blue) {
    if (red   < 0 || red   > 255 ||
        green < 0 || green > 255 ||
        blue  < 0 || blue  > 255) {

        fprintf(stderr,
                "WARNING: invalid color rgb(%d, %d, %d) found.\n",
                red, green, blue);
        return 0;
    }

    return 1;
}

static struct rgb *parse_rgbcolor(const char *str, size_t *color_length) {
    struct rgb *color = NULL;
    int red;
    int green;
    int blue;

    if (*str != 'r') {
        return NULL;
    }

    if (sscanf(str, "rgb(%d, %d, %d)", &red, &green, &blue) == 3) {
        if (is_valid_rgb(red, green, blue)) {
            const char *color_start;
            const char *color_end;

            color = xmalloc(sizeof(*color));
            color->red = red;
            color->green = green;
            color->blue = blue;

            color_start = str;
            color_end = str + sizeof("rgb(X,X,X") - 1;
            while (*color_end++ != ')') {
                continue;
            }
            *color_length = color_end - color_start;
        }
    }

    return color;
}

static void print_rgbcolor(const struct rgb *color, FILE *output_file) {
    fprintf(output_file, "rgb(%d, %d, %d)",
            color->red, color->green, color->blue);
}

static void color_to_mono(struct rgb *color, enum hues hue) {
    int gray;

    gray = (int) ((color->red   * 299L + 500) / 1000
                + (color->green * 587L + 500) / 1000
                + (color->blue  * 114L + 500) / 1000);

    switch (hue) {
        default:
        case hue_white:
            color->red   = gray;
            color->green = gray;
            color->blue  = gray;
            break;

        case hue_amber:
            color->red   = gray;
            color->green = (unsigned char) (gray * 191L / 255);
            color->blue  = 0;
            break;

        case hue_cyan:
            color->red   = 0;
            color->green = gray;
            color->blue  = gray;
            break;

        case hue_green:
            color->red   = 0;
            color->green = gray;
            color->blue  = 0;
            break;
    }
}

static void process_line(const char *line, FILE *output_file, enum hues hue) {
    const char *cursor = line;

    while (*cursor != '\0') {
        struct rgb *color;
        size_t color_length;

        if ((color = parse_hexcolor(cursor, &color_length)) != NULL) {
            color_to_mono(color, hue);
            print_hexcolor(color, output_file);
            free(color);
        } else if ((color = parse_rgbcolor(cursor, &color_length)) != NULL) {
            color_to_mono(color, hue);
            print_rgbcolor(color, output_file);
            free(color);
        } else {
            fputc(*cursor, output_file);
            color_length = 1;
        }

        cursor += color_length;
    }
}

static enum hues get_hue(int argc, char **argv) {
    enum hues hue = hue_white;

    if (argc < 2) {
        return hue;
    }

    if (argv[1][0] == '-') {
        switch (argv[1][1]) {
            case 'a':
                hue = hue_amber;
                break;

            case 'c':
                hue = hue_cyan;
                break;

            case 'g':
                hue = hue_green;
                break;

            case 'w':
                hue = hue_white;
                break;
        }
    }

    return hue;
}

int main(int argc, char **argv) {
    char line[MAXLINELEN];
    enum hues hue;

    hue = get_hue(argc, argv);

    while (fgets(line, sizeof(line), stdin) != NULL) {
        process_line(line, stdout, hue);
    }

    return 0;
}
