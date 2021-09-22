all: test/src/main.c lib/strings.h lib/utils.h
	cc test/src/main.c -o test/test