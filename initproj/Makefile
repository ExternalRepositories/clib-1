OUT = initproj
SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)

CFLAGS = -Wall -Wextra -Wundef -Werror=return-type

.PHONY: all clean debug install

all: $(OUT)

$(OUT): $(SRC) $(HEADERS) Makefile
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LFLAGS) -O3

debug: $(SRC) $(HEADERS) Makefile
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LFLAGS) -g

install: $(OUT)
	cp $(OUT) /bin/$(OUT)

clean:
	rm -rf $(OUT)