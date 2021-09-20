#include "../../include/c.h"

#include <stdio.h>

int main(void) {
	array_t arr = array_create(int, 25);
	repeat(i, 25) array_append(&arr, i);
	array_foreach(int, i, arr) *i = 47;
	array_foreach(int, i, arr) printf("%d", *i);
}
