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

typedef uint ipv4_addr;

/*
 * Read nbytes from a file into a dst buf and be persistent
 * about it.
 */
static void
blkread(const char *filename, void *dst, size_t size)
{
	ssize_t nread;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("blkread");
		exit(1);
	}

	while ((nread = read(fd, dst, size)) != (ssize_t) size) {
		if (nread == -1 && errno != EAGAIN) {
			perror("blkread");
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
sysrand(void *dst, int size)
{
	static uchar buf[128], *bufp;
	uchar *dstp;

	dstp = dst;
	bufp = buf + sizeof(buf);

	while (size > 0) {
		if (bufp >= buf + sizeof(buf)) {
			blkread(RANDOM_SOURCE, buf, sizeof(buf));
			bufp = buf;
		}

		if (bufp + size <= buf + sizeof(buf)) {
			memcpy(dstp, bufp, size);
			bufp += size;
			break;
		} else {
			int n = buf + sizeof(buf) - bufp;

			memcpy(dstp, bufp, n);
			dstp += n;
			bufp += n;
			size -= n;
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
	uint r = 0;

	if ((uint) nbits > (sizeof(uint) * 8)) {
		fprintf(stderr, "too many bits requested.\n");
		exit(1);
	}

	/* Round to whole bytes. */
	if (nbits - nbytes * 8 > 0)
		++nbytes;

	sysrand(&r, nbytes);

	/* Drop the extra bits from rounding. */
	nbits -= nbytes * 8;
	if (nbits < 0)
		r >>= abs(nbits);

	return r;
}

static void
do_ipv4(void)
{
	ipv4_addr net[] = { 0x0a000000, 0xac100000, 0xc0a80000 };
	int bits[] = { 8, 12, 16 };
	ipv4_addr addr;
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
	if ((sizeof(ipv4_addr) * 8) < 32) {
		fprintf(stderr, "change ipv4_addr in source.\n");
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
