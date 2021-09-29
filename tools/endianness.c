/// Returns 1 if the machine is little-endian, and 0 if it's big-endian.
static const int i = 1;
int main(void) {
	return *(char *) &i == 1;
}