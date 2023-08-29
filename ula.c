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
typedef unsigned int  uint;

typedef uint ipv4_address;

/*
 * Read nbytes from a file into a destination buffer and be persistent
 * about it.
 */
static void blocking_read(const char *filename, void *destination,
                          size_t nbytes)
{
    ssize_t nread;
    int fd;

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("blocking_read");
        exit(1);
    }

    while ((nread = read(fd, destination, nbytes)) != (ssize_t) nbytes) {
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
static void system_random(void *destination, int nbytes)
{
    static uchar buffer[128];
    static uchar *buffer_position = buffer + sizeof(buffer);
    uchar *destination_position = destination;
    uchar *buffer_end = buffer + sizeof(buffer);

    while (nbytes > 0) {
        /* Buffer is full so refill it and reset the position. */
        if (buffer_position >= buffer_end) {
            blocking_read(RANDOM_SOURCE, buffer, sizeof(buffer));
            buffer_position = buffer;
        }

        if (buffer_position + nbytes <= buffer_end) {
            /* nbytes can be taken from the buffer. */
            memcpy(destination_position, buffer_position, nbytes);
            buffer_position += nbytes;
            nbytes = 0;
        } else {
            /* nbytes can't be taken, get what we can though. */
            int n = buffer_end - buffer_position;

            memcpy(destination_position, buffer_position, n);
            destination_position += n;
            buffer_position += n;
            nbytes -= n;
        }
    }
}

/*
 * Get nbits of random bits.
 */
static uint random_bits(int nbits)
{
    uint result = 0;
    int nbytes = nbits / 8;

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

static void print_ipv4_address(ipv4_address address)
{
    int shift;

    for (shift = 24; shift > 0; shift -= 8) {
        printf("%d.", (address >> shift) & 0xff);
    }

    printf("%d", address & 0xff);
}

static void do_ipv4(void)
{
    ipv4_address net[] = { 0x0a000000, 0xac100000, 0xc0a80000 };
    int bits[] = { 8, 12, 16 };
    ipv4_address address;
    int pick;

    pick = random_bits(2) % 2;
    address = net[pick];

    address += random_bits(32 - bits[pick]);
    print_ipv4_address(address);

    printf("/%d\n", bits[pick]);
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
