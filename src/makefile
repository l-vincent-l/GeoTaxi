VERSION = RELEASE

DEBUG_CFLAGS = -g -DUSE_HEAP_STATS
PROFILE_CFLAGS = -pg
RELEASE_CFLAGS = -O2

ifeq "$(VERSION)" "PROFILE"
	CFLAGS = $(PROFILE_CFLAGS)
else
	ifeq "$(VERSION)" "DEBUG"
	CFLAGS = $(DEBUG_CFLAGS)
else
	ifeq "$(VERSION)" "RELEASE"
	CFLAGS = $(RELEASE_CFLAGS)
endif
endif
endif

ODIR=obj
LDIR =../lib

_LIBSRC = sha1.c js0n.c
LIBSRC = $(patsubst %,$(LDIR)/%,$(_LIBSRC))
LIBHEADER = $(LIBSRC:%.c=%.h)
_LIBOBJ = $(_LIBSRC:%.c=%.o)
LIBOBJ = $(patsubst %,$(ODIR)/%,$(_LIBOBJ))

_OBJ = geoloc-server.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

SRC = geoloc-server.c

LFLAGS=-I$(LDIR) $(LIBOBJ) -flto

build:
	$(CC) -c $(LDIR)/sha1.c -lgcrypt -o $(ODIR)/sha1.o
	$(CC) -c $(LDIR)/js0n.c -o $(ODIR)/js0n.o
	$(CC) $(CFLAGS) $(LFLAGS) -lgcrypt geoloc-server.c -o geoloc-server 

.PHONY: clean

clean:
		rm -f $(ODIR)/*.o *~ core  

