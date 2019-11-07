VERSION = RELEASE

DEBUG_CFLAGS = -g -DUSE_HEAP_STATS -DDEBUG
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
LDIR=lib

_LIBSRC = sha1.c js0n.c map.c argtable3.c
LIBSRC = $(patsubst %,$(LDIR)/%,$(_LIBSRC))
LIBHEADER = $(LIBSRC:%.c=%.h)
_LIBOBJ = $(_LIBSRC:%.c=%.o)
LIBOBJ = $(patsubst %,$(ODIR)/%,$(_LIBOBJ))

_OBJ = geoloc-server.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

SRC = geoloc-server.c

LFLAGS=-I$(LDIR) $(LIBOBJ) 

LIBS=-flto -lgcrypt -lcurl -lhiredis

ifeq (test,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run"
  TEST_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(TEST_ARGS):;@:)
endif


build:
	$(CC) -c $(LDIR)/map.c -o $(ODIR)/map.o
	$(CC) -c $(LDIR)/sha1.c -lgcrypt -o $(ODIR)/sha1.o
	$(CC) -c $(LDIR)/js0n.c -o $(ODIR)/js0n.o
	$(CC) -c $(LDIR)/argtable3.c -o $(ODIR)/argtable3.o
	$(CC) $(CFLAGS) $(LFLAGS) src/geoloc-server.c -o geoloc-server $(LIBS)

.PHONY: clean

clean:
		rm -f $(ODIR)/*.o *~ core  

.PHONY: test

test:
	$(CC) -c $(LDIR)/map.c -o $(ODIR)/map.o
	$(CC) -c $(LDIR)/sha1.c -lgcrypt -o $(ODIR)/sha1.o
	$(CC) -c $(LDIR)/js0n.c -o $(ODIR)/js0n.o
	$(CC) -c $(LDIR)/argtable3.c -o $(ODIR)/argtable3.o
	$(CC) $(CFLAGS) $(LFLAGS) src/geoloc-server.c -o geoloc-server-test -DFLUSHSTDOUT $(LIBS)
	GOPATH=$(PWD)/tests go build tests/test_geoserver.go
	GOPATH=$(PWD)/fake_apitaxi go build fake_apitaxi/main.go
	./test_geoserver $(CURDIR)/geoloc-server-test $(CURDIR)/main $(TEST_ARGS)
	rm $(CURDIR)/geoloc-server-test $(CURDIR)/test_geoserver

