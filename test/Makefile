OUT = test
SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h) ../clib.h

CFLAGS = -Wall -Wextra -Wundef -Werror=return-type -march=native -std=c17 -Wno-unused-function

.PHONY: all clean debug install

all: $(OUT)

$(OUT): $(SRC) $(HEADERS) Makefile
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LFLAGS) -O3

debug: $(SRC) $(HEADERS) Makefile
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LFLAGS) -O0 -ggdb

clean:
	rm -rf $(OUT)