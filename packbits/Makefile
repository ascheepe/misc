CFLAGS=	-O2 -pipe -Wall -Wextra -Werror -pedantic
OBJS=	packbits.o
PRG=	packbits

$(PRG): $(OBJS)
	$(CC) -s -o $(PRG) $(OBJS)

test:	$(PRG)
	./$(PRG) -c $(PRG) /tmp/$(PRG).pb
	./$(PRG) -d /tmp/$(PRG).pb /tmp/$(PRG)
	ls -l $(PRG) /tmp/$(PRG).pb
	cmp $(PRG) /tmp/$(PRG)

clean:
	rm -f $(PRG) $(OBJS)
