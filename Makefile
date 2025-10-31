#CFLAGS=	-std=c89 -Ofast -march=native -pipe -Wall -Wextra -Werror -pedantic
CFLAGS=	-std=c89 -O2 -pipe -Wall -Wextra -Werror -pedantic
#CFLAGS=	-Og -g -pipe -Wall -Wextra -Werror -pedantic -fsanitize=address,leak
#CFLAGS=	-Og -pipe -Wall -Wextra -Werror -pedantic
CFLAGS+= -D_DEFAULT_SOURCE

bayer:	bayer.c
	$(CC) $(CFLAGS) -o bayer bayer.c -lnetpbm
atkinson:	atkinson.c
	$(CC) $(CFLAGS) -o atkinson atkinson.c -lnetpbm
