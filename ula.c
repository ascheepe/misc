/*
 * Generate random private ipv6 and ipv4 addresses.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define RANDOM_SOURCE "/dev/urandom"

typedef unsigned char uchar;
typedef unsigned int uint;

typedef uint ipv4_address;

/*
 * Read nbytes from a file into a destination buffer and be persistent
 * about it.
 */
static void blocking_read(const char *filename, void *destination,
                          ssize_t size)
{
    ssize_t nread;
    int fd;

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("blocking_read");
        exit(1);
    }

    while ((nread = read(fd, destination, size)) != (ssize_t) size) {
        if (nread == -1 && errno != EAGAIN) {
            perror("blocking_read");
            exit(1);
        }
        sleep(1);
    }

    close(fd);
}

/*
 * Get nbytes of random data from the system.
 */
static void system_random(void *destination_ptr, size_t size)
{
    static uchar buffer[128];
    static uchar *buffer_position = buffer + sizeof(buffer);
    uchar *buffer_end = buffer + sizeof(buffer);
    uchar *destination = destination_ptr;

    while (size > 0) {
        if (buffer_position >= buffer_end) {
            blocking_read(RANDOM_SOURCE, buffer, sizeof(buffer));
            buffer_position = buffer;
        }

        if (buffer_position + size <= buffer_end) {
            memcpy(destination, buffer_position, size);
            buffer_position += size;
            size = 0;
        } else {
            size_t length = buffer_end - buffer_position;

            memcpy(destination, buffer_position, length);
            destination += length;
            buffer_position += length;
            size -= length;
        }
    }
}

/*
 * Get nbits of random bits.
 */
static uint random_bits(int nbits)
{
    int nbytes = nbits / 8;
    uint result = 0;

    if ((uint) nbits > (sizeof(uint) * 8)) {
        fprintf(stderr, "too many bits requested.\n");
        exit(1);
    }

    /* Round to whole bytes. */
    if (nbits - nbytes * 8 > 0) {
        ++nbytes;
    }

    system_random(&result, nbytes);

    /* Drop the extra bits from rounding. */
    nbits -= nbytes * 8;
    if (nbits < 0) {
        result >>= abs(nbits);
    }

    return result;
}

static void do_ipv4(void)
{
    ipv4_address net[] = { 0x0a000000, 0xac100000, 0xc0a80000 };
    int bits[] = { 8, 12, 16 };
    ipv4_address address;
    int shift;
    int pick;

    pick = abs((int) random_bits(2) - 1);
    address = net[pick];

    address += random_bits(32 - bits[pick]);
    for (shift = 24; shift > 0; shift -= 8) {
        printf("%d.", (address >> shift) & 0xff);
    }
    printf("%d/%d\n", address & 0xff, bits[pick]);
}

static void do_ipv6(void)
{
    int i;

    printf("fd%02x", random_bits(8));
    for (i = 0; i < 7; ++i) {
        printf(":%x", random_bits(16));
    }
    printf("/64\n");
}

int main(int argc, char **argv)
{
    if ((sizeof(ipv4_address) * 8) < 32) {
        fprintf(stderr, "change ipv4_address in source.\n");
        return 1;
    }

    if (argc > 1) {
        if (strcmp(argv[1], "-4") == 0) {
            do_ipv4();
        } else if (strcmp(argv[1], "-6") == 0) {
            do_ipv6();
        } else {
            puts("usage: ula [-4|-6]");
            return 1;
        }
    } else {
        do_ipv4();
        do_ipv6();
    }

    return 0;
}
