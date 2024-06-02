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
 * Read nbytes from a file into a destination buf and be persistent
 * about it.
 */
static void
blocking_read(const char *filename, void *dst, ssize_t size)
{
	ssize_t nread;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("blocking_read");
		exit(1);
	}

	while ((nread = read(fd, dst, size)) != size) {
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
static void
system_random(void *dst_ptr, size_t size)
{
	static uchar buf[128], *pos;
	uchar *buf_end, *dst = dst_ptr;

	pos = buf_end = buf + sizeof(buf);

	while (size > 0) {
		if (pos >= buf_end) {
			blocking_read(RANDOM_SOURCE, buf, sizeof(buf));
			pos = buf;
		}

		if (pos + size <= buf_end) {
			memcpy(dst, pos, size);
			pos += size;
			size = 0;
		} else {
			size_t len = buf_end - pos;

			memcpy(dst, pos, len);
			dst += len;
			pos += len;
			size -= len;
		}
	}
}

/*
 * Get nbits of random bits.
 */
static uint
random_bits(int nbits)
{
	int nbytes = nbits / 8;
	uint ret = 0;

	if ((uint) nbits > (sizeof(uint) * 8)) {
		fprintf(stderr, "too many bits requested.\n");
		exit(1);
	}

	/* Round to whole bytes. */
	if (nbits - nbytes * 8 > 0)
		++nbytes;

	system_random(&ret, nbytes);

	/* Drop the extra bits from rounding. */
	nbits -= nbytes * 8;
	if (nbits < 0)
		ret >>= abs(nbits);

	return ret;
}

static void
do_ipv4(void)
{
	ipv4_address net[] = { 0x0a000000, 0xac100000, 0xc0a80000 };
	int bits[] = { 8, 12, 16 };
	ipv4_address addr;
	int shift;
	int pick;

	pick = abs((int)random_bits(2) - 1);
	addr = net[pick];

	addr += random_bits(32 - bits[pick]);
	for (shift = 24; shift > 0; shift -= 8)
		printf("%d.", (addr >> shift) & 0xff);
	printf("%d/%d\n", addr & 0xff, bits[pick]);
}

static void
do_ipv6(void)
{
	int i;

	printf("fd%02x", random_bits(8));
	for (i = 0; i < 7; ++i)
		printf(":%x", random_bits(16));
	printf("/64\n");
}

int
main(int argc, char **argv)
{
	if ((sizeof(ipv4_address) * 8) < 32) {
		fprintf(stderr, "change ipv4_address in source.\n");
		return 1;
	}

	if (argc > 1) {
		if (strcmp(argv[1], "-4") == 0)
			do_ipv4();
		else if (strcmp(argv[1], "-6") == 0)
			do_ipv6();
		else {
			puts("usage: ula [-4|-6]");
			return 1;
		}
	} else {
		do_ipv4();
		do_ipv6();
	}

	return 0;
}
