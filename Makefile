#CFLAGS=	-std=c89 -Ofast -march=native -pipe -Wall -Wextra -Werror -pedantic
CFLAGS=	-std=c89 -O2 -pipe -Wall -Wextra -Werror -pedantic
#CFLAGS=	-Og -g -pipe -Wall -Wextra -Werror -pedantic -fsanitize=address,leak
#CFLAGS=	-Og -pipe -Wall -Wextra -Werror -pedantic
CFLAGS+= -D_DEFAULT_SOURCE
