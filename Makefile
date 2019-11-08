NAME = geoloc-server
NAME_TEST = $(NAME)-test

VERSION ?= RELEASE

ifeq "$(VERSION)" "PROFILE"
	CFLAGS += -pg
endif
ifeq "$(VERSION)" "DEBUG"
	CFLAGS += -g -DUSE_HEAP_STATS -DDEBUG
endif
ifeq "$(VERSION)" "RELEASE"
	CFLAGS += -O2
endif

SRC = lib/argtable3.c \
	lib/sha1.c \
	lib/js0n.c \
	lib/map.c \
	src/geoloc-server.c

OBJ = $(SRC:.c=.o)

CFLAGS += -Ilib
ifeq (test, $(firstword $(MAKECMDGOALS))) # if make test
	CFLAGS += -DFLUSHSTDOUT
endif

LDFLAGS += -flto -lgcrypt -lcurl -lhiredis

$(NAME): $(OBJ)
	$(CC) -o $(NAME) $(OBJ) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJ)

# If run make test arg1 arg2 arg3, make TEST_ARGS = arg1 arg2 arg3
ifeq (test,$(firstword $(MAKECMDGOALS)))
# use the rest as arguments for "run"
TEST_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
# ...and turn them into do-nothing targets
$(eval $(TEST_ARGS):;@:)
endif

.PHONY: test
test: $(OBJ)
	$(CC) -o $(NAME_TEST) $(OBJ) $(LDFLAGS)
	GOPATH=$(PWD)/tests go build tests/test_geoserver.go
	GOPATH=$(PWD)/fake_apitaxi go build fake_apitaxi/main.go
	./test_geoserver $(CURDIR)/geoloc-server-test $(CURDIR)/main $(TEST_ARGS)
	rm $(CURDIR)/geoloc-server-test $(CURDIR)/test_geoserver
