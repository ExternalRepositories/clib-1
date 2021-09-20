#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define RED	   "\033[31m"
#define RESET  "\033[0m"
#define YELLOW "\033[033m"

const char *pwd = ".";

void dir_create(const char *path) {
	if (mkdir(path, S_IRWXU) < 0) {
		fprintf(stderr, RED "Could not create directoy '%s'\n.%s\n" RESET, path, strerror(errno));
		exit(1);
	}
}

FILE *file_open(const char *path, const char *mode) {
	FILE *f = fopen(path, mode);
	if (!f) {
		fprintf(stderr, RED "Could not open file '%s'\n.%s\n" RESET, path, strerror(errno));
		exit(1);
	}

	return f;
}

const char *file_name_from_path(const char *path) {
	if (!path) return NULL;
	size_t		len = strlen(path);
	const char *ptr = path + len - 1;

	while (ptr >= path) {
		if (*ptr == '/') break;
		ptr--;
	}

	if (ptr < path) return path;
	return ptr + 1;
}

int main(int argc, char **argv) {
	const char *path = pwd;
	if (argc > 1) path = argv[1];

	DIR *dir = opendir(path);
	if (!dir) {
		if (errno == ENOENT) {
			printf(YELLOW "Directory '%s' does not exist. Creating...\n" RESET, path);
			dir = NULL;
			dir_create(path);
		} else {
			fprintf(stderr, RED "Could not access '%s'.\n%s\n" RESET, path, strerror(errno));
			exit(1);
		}
	}

	struct dirent *dirent;
	int			   dirs = 0;

	if (dir)
		while (dirent = readdir(dir), dirent) {
			dirs++;
			if (dirs >= 3) break;
		}

	/// Every directory contains at least '.' and '..'
	if (dirs >= 3) {
		fprintf(stderr, RED "Directory '%s' is not empty!\n" RESET, path);
		exit(1);
	}

	/// Create src dir
	size_t pathlen	= strlen(path);
	char * path_dyn = calloc(pathlen + 100, 1);
	memcpy(path_dyn, path, pathlen);
	memcpy(path_dyn + pathlen, "/src", 4);

	dir_create(path_dyn);

	/// Create files
	memcpy(path_dyn + pathlen + 4, "/main.c", 7);
	FILE *mainc = file_open(path_dyn, "w");

	memcpy(path_dyn + pathlen, "/Makefile", 9);
	path_dyn[pathlen + 9] = 0;
	FILE *makefile		  = file_open(path_dyn, "w");

	fprintf(mainc,
		"#include <stdio.h>\n\n"
		"int main(void) {\n"
		"\tprintf(\"Hello, World\");\n"
		"}\n");

	fprintf(makefile,
		"OUT = %s\n"
		"SRC = $(wildcard src/*.c)\n"
		"HEADERS = $(wildcard src/*.h)\n\n"
		"CFLAGS = -Wall -Wextra -Wundef -Werror=return-type\n\n"
		".PHONY: all clean debug install\n\n"
		"all: $(OUT)\n\n"
		"$(OUT): $(SRC) $(HEADERS) Makefile\n"
		"\t$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LFLAGS) -O3\n\n"
		"debug: $(SRC) $(HEADERS) Makefile\n"
		"\t$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LFLAGS) -g\n\n"
		"install: $(OUT)\n"
		"\tcp $(OUT) /bin/$(OUT)\n\n"
		"clean:\n"
		"\trm -rf $(OUT)",
		file_name_from_path(path));

	printf(YELLOW "Done.\n");
}