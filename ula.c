#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned char uchar;
typedef unsigned int uint;

typedef uint addr4_t;

static void
sysrand(void *dst, int nbytes)
{
	static uchar buf[128];
	static uchar *bufp = buf + sizeof(buf);
	uchar *dstp = dst;

	if (bufp >= buf + sizeof(buf)) {
		int fd, nread;

 fillbuf:
		fd = open("/dev/urandom", O_RDONLY);
		if (fd == -1) {
			perror("sysrand");
			exit(1);
		}

		while ((nread = read(fd, buf, sizeof(buf))) != sizeof(buf)) {
			if (nread == -1 && errno != EAGAIN) {
				perror("sysrand");
				exit(1);
			}
			sleep(1);
		}

		close(fd);

		bufp = buf;
	}

	if (bufp + nbytes <= buf + sizeof(buf)) {
		memcpy(dstp, bufp, nbytes);
		bufp += nbytes;
	} else {
		int n = buf + sizeof(buf) - bufp;

		memcpy(dstp, bufp, n);
		dstp += n;
		nbytes -= n;

		goto fillbuf;
	}
}

static uint
random_bits(int nbits)
{
	uint r = 0;
	int nbytes = nbits / 8;

	if ((uint)nbits > (sizeof(uint) * 8)) {
		fprintf(stderr, "too many bits requested.\n");
		exit(1);
	}

	if (nbits - nbytes * 8 > 0)
		++nbytes;

	sysrand(&r, nbytes);

	nbits -= nbytes * 8;
	if (nbits < 0)
		r >>= abs(nbits);

	return r;
}

static void
print_ipv4(addr4_t addr)
{
	int shift = 24;

	while (shift > 0) {
		printf("%d.", (addr >> shift) & 0xff);
		shift -= 8;
	}

	printf("%d", addr & 0xff);
}

static void
do_ipv4(void)
{
	addr4_t net[] = { 0x0a000000, 0xac100000, 0xc0a80000 };
	int bits[] = { 8, 12, 16 };
	addr4_t addr;
	int i;

	i = abs((int)random_bits(2) - 1);
	addr = net[i];

	addr += random_bits(32 - bits[i]);
	print_ipv4(addr);

	printf("/%d\n", bits[i]);
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
	if ((sizeof(addr4_t) * 8) < 32) {
		fprintf(stderr, "change addr4_t in source.\n");
		return 1;
	}

	if (argc > 1) {
		if (strcmp(argv[1], "-4") == 0)
			do_ipv4();
		else
			do_ipv6();
	} else {
		do_ipv4();
		do_ipv6();
	}

	return 0;
}

