DEBUG = -O3

S = ./
O = objects/$(shell uname -s)
E = $(HOME)/Sites/news/

CC=g++
LDFLAGS=-lmrutils -lgnutls -lgnutls-extra
CFLAGS=-pipe $(DEBUG) -c -Wall -Wextra -Woverloaded-virtual

# determine whether to compile 64 bit or 32
ifeq ($(shell uname -m),i386)
	CFLAGS+= -m64
else
	CFLAGS+= -m32
endif

all: feedweb content feed list

tables: $O/tables.o
	$(CC) $(LDFLAGS) $^ -o $E/$@
feedweb: $O/feedweb.o
	$(CC) $(LDFLAGS) $^ -o $E/$@
content: $O/content.o
	$(CC) $(LDFLAGS) $^ -o $E/$@
feed: $O/feed.o
	$(CC) $(LDFLAGS) $^ -o $E/$@
list: $O/list.o
	$(CC) $(LDFLAGS) $^ -o $E/$@

$(O)/%.o: $(S)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(O)/*.o
