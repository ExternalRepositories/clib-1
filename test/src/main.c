#define CLIB_IMPLEMENTATION
#include <clib.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_F_LEN 1023

jmp_buf _jmp;
char	_funcname[MAX_F_LEN + 1];
size_t	line = 0;

__attribute__((format(printf, 1, 2))) void error(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	fprintf(stderr, "\033[31mLINE %lu: ", line);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\033[0m");
	va_end(ap);
}

#define SIGNALS(F) \
	F(SIGQUIT)     \
	F(SIGILL)      \
	F(SIGTRAP)     \
	F(SIGABRT)     \
	F(SIGBUS)      \
	F(SIGFPE)      \
	F(SIGUSR1)     \
	F(SIGSEGV)     \
	F(SIGUSR2)     \
	F(SIGPIPE)     \
	F(SIGALRM)     \
	F(SIGTERM)     \
	F(SIGCHLD)     \
	F(SIGCONT)     \
	F(SIGTSTP)     \
	F(SIGTTIN)     \
	F(SIGTTOU)     \
	F(SIGURG)      \
	F(SIGXCPU)     \
	F(SIGXFSZ)     \
	F(SIGVTALRM)   \
	F(SIGPROF)     \
	F(SIGWINCH)    \
	F(SIGIO)       \
	F(SIGPWR)      \
	F(SIGSYS)

#define SIGNAL_HANDLER_SW(sig) \
	case sig:                  \
		return #sig;

#define SIGNAL_HANDLER_REG(sig)                                      \
	if (signal(sig, sig_handler) == SIG_ERR) {                       \
		error("Could not initialise signal handler for " #sig "\n"); \
		perror("");                                                  \
		exit(1);                                                     \
	}

static const char *sig_name(int sig) {
	switch (sig) {
		SIGNALS(SIGNAL_HANDLER_SW)
		default: return "Unknown signal";
	}
}

static void sig_handler(int s) {
	error("\033[31mExecution of %s raised \033[33m%s\033[0m\n", _funcname, sig_name(s));
	fprintf(stderr, "\033[33merrno: \033[31m%s\033[0m\n", strerror(errno));
	longjmp(_jmp, s ? s : -1);
}

void sig_init(void) {
	SIGNALS(SIGNAL_HANDLER_REG)
}

#define TEST(funcname, ...)                                                                                           \
	line = __LINE__;                                                                                                  \
	{                                                                                                                 \
		size_t _flen = strlen(funcname);                                                                              \
		if (_flen > MAX_F_LEN) {                                                                                      \
			error("Function name %s too long.\nFunction names may not exceed %d characters.\n", funcname, MAX_F_LEN); \
			exit(1);                                                                                                  \
		}                                                                                                             \
		memcpy(_funcname, funcname, _flen);                                                                           \
		_funcname[_flen] = 0;                                                                                         \
	}                                                                                                                 \
	if (!setjmp(_jmp)) {                                                                                              \
		__VA_ARGS__;                                                                                                  \
	}

#define STRINGISE_(X) #X
#define STRINGISE(X)  STRINGISE_(X)

#define ASSERT(cond, ...)                                                \
	if (!(cond)) {                                                       \
		error("Assertion \033[32m%s\033[31m failed\n", STRINGISE(cond)); \
		__VA_OPT__(printf(__VA_ARGS__);)                                 \
		exit(1);                                                         \
	}

#define SIG_DEFAULT(sig)                                            \
	if (signal(sig, SIG_DFL) == SIG_ERR) {                          \
		error("Could not set default signal handler for %s", #sig); \
		exit(1);                                                    \
	}

void tests_array(void) {
	array_t arr;
	TEST("array_create", arr = array_create(int, 25);)
	ASSERT(arr.allocated == 25 && arr.size == 0 && arr.size_of_type == sizeof(int))

	TEST("array_append", repeat(i, 25) array_append(size_t, &arr, i);)
	ASSERT(arr.size == 25)

	TEST("array_foreach", array_foreach(int, i, arr) printf("%d", *i);)

	TEST("array_foreach", array_foreach(int, i, arr) *i = 47;)
	printf("\n");

	TEST("array_foreach", array_foreach(int, i, arr) printf("%d", *i);)
	printf("\n");
}

void tests_strings_memory(void) {
	string s, s1, s2;
	TEST("str", s = str();)
	ASSERT(_str_is_small(&s))

	{
		const char  *_data = "Bla Bla bla\n";
		const size_t _len  = strlen(_data);
		size_t		 s_len;
		TEST("str_cpy_l", str_cpy(&s, _data);)
		ASSERT(strlen(s._small_data) == _len, "strlen: %lu != %lu\n", strlen(s._small_data), _len)

		TEST("Value: ", printf("%s", str_data(&s)));

		TEST("str_len", s_len = str_len(&s))
		ASSERT(s_len == _len, "str_len: %lu != %lu\n", s_len, _len)

		ASSERT(_str_is_small(&s))
	}

	TEST("str_clear", str_clear(&s))
	else s = str();
	ASSERT(str_len(&s) == 0, "str_len not 0 after call to str_clear")

	TEST("printf(str_data(&s)) ", printf("%s", str_data(&s));)

	TEST("str_from", s1 = str("Short string"))
	TEST("str_from", s2 = str("Long string Long string Long string Long string Long string Long string Long string"))

	ASSERT(_str_is_small(&s1))
	ASSERT(!_str_is_small(&s2))

	printf("%s\n%s\n", str_data(&s1), str_data(&s2));

	TEST("str_clear", str_clear(&s1))
	TEST("str_clear", str_clear(&s2))

	ASSERT(str_len(&s1) == 0)
	ASSERT(str_len(&s2) == 0)
	ASSERT(!*str_data(&s1))
	ASSERT(!*str_data(&s2))

	const char *_2x[3] = {
		"12345678901234567890123",	 /// 23 chars
		"123456789012345678901234",	 /// 24 chars
		"1234567890123456789012345", /// 25 chars
	};

	string sa[3];
	string sb[3];
	string sc[3];

	repeat(i, 3) {
		sa[i] = str();
		TEST("str_cpy", str_cpy(sa + i, _2x[i]));
	}
	repeat(i, 3) {
		sb[i] = str();
		TEST("str_cat", str_cat(sb + i, _2x[i]));
	}

	repeat(i, 3) sc[i] = str(_2x[i]);

	ASSERT(_str_is_small(sa))
	ASSERT(_str_is_small(sb))
	ASSERT(_str_is_small(sc))

	ASSERT(!_str_is_small(sa + 1))
	ASSERT(!_str_is_small(sb + 1))
	ASSERT(!_str_is_small(sc + 1))

	ASSERT(!_str_is_small(sa + 2))
	ASSERT(!_str_is_small(sb + 2))
	ASSERT(!_str_is_small(sc + 2))

	ASSERT(!str_cmp(sa, sb))
	ASSERT(!str_cmp(sb, sc))

	ASSERT(!str_cmp(sa + 1, sb + 1))
	ASSERT(!str_cmp(sb + 1, sc + 1))

	ASSERT(!str_cmp(sa + 2, sb + 2))
	ASSERT(!str_cmp(sb + 2, sc + 2))
}

void tests_wstrings_memory(void) {
	wstring s, s1, s2;
	TEST("str", s = wstr();)
	ASSERT(_wstr_is_small(&s))

	{
		const wchar *_data = L"Bla\n";
		const size_t _len  = wcslen(_data);
		size_t		 s_len;
		TEST("str_cpy_l", wstr_cpy(&s, _data);)
		ASSERT(wcslen(s._small_data) == _len, "wcslen: %lu != %lu\n", wcslen(s._small_data), _len)

		TEST("Value: ", printf("%ls", wstr_data(&s)));

		TEST("wstr_len", s_len = wstr_len(&s))
		ASSERT(s_len == _len, "wstr_len: %lu != %lu\n", s_len, _len)

		ASSERT(_wstr_is_small(&s))
	}

	TEST("str_clear", wstr_clear(&s))
	else s = wstr();
	ASSERT(wstr_len(&s) == 0, "wstr_len not 0 after call to str_clear")

	TEST("printf(wstr_data(&s)) ", printf("%ls", wstr_data(&s));)

	TEST("str_from", s1 = wstr(L"Short"))
	TEST("str_from", s2 = wstr(L"Long string Long string Long string Long string Long string Long string Long string"))

	ASSERT(_wstr_is_small(&s1))
	ASSERT(!_wstr_is_small(&s2))

	printf("%ls\n%ls\n", wstr_data(&s1), wstr_data(&s2));

	TEST("str_clear", wstr_clear(&s1))
	TEST("str_clear", wstr_clear(&s2))

	ASSERT(wstr_len(&s1) == 0)
	ASSERT(wstr_len(&s2) == 0)
	ASSERT(!*wstr_data(&s1))
	ASSERT(!*wstr_data(&s2))

	const wchar *_2x[3] = {
		L"12345",	/// 5 chars
		L"123456",	/// 6 chars
		L"1234567", /// 7 chars
	};

	wstring sa[3];
	wstring sb[3];
	wstring sc[3];

	repeat(i, 3) {
		sa[i] = wstr();
		TEST("wstr_cpy", wstr_cpy(sa + i, _2x[i]));
	}
	repeat(i, 3) {
		sb[i] = wstr();
		TEST("wstr_cat", wstr_cat(sb + i, _2x[i]));
	}

	repeat(i, 3) sc[i] = wstr(_2x[i]);

	ASSERT(_wstr_is_small(sa))
	ASSERT(_wstr_is_small(sb))
	ASSERT(_wstr_is_small(sc))

	ASSERT(!_wstr_is_small(sa + 1))
	ASSERT(!_wstr_is_small(sb + 1))
	ASSERT(!_wstr_is_small(sc + 1))

	ASSERT(!_wstr_is_small(sa + 2))
	ASSERT(!_wstr_is_small(sb + 2))
	ASSERT(!_wstr_is_small(sc + 2))

	ASSERT(!wstr_cmp(sa, sb))
	ASSERT(!wstr_cmp(sb, sc))

	ASSERT(!wstr_cmp(sa + 1, sb + 1))
	ASSERT(!wstr_cmp(sb + 1, sc + 1))

	ASSERT(!wstr_cmp(sa + 2, sb + 2))
	ASSERT(!wstr_cmp(sb + 2, sc + 2))
}

void tests_strings_2(void) {
	string s1 = str(), s2 = str();
	for (char c = 'a'; c <= 'z'; c++) {
		TEST("strn_cat", str_cat(&s1, &c, 1))
	}
	TEST("str_cpy_l", str_cpy(&s2, "abcdefghijklmnopqrstuvwxyz"));

	ASSERT(str_len(&s1) == 26)
	ASSERT(str_len(&s2) == 26)
	ASSERT(!str_cmp(&s1, &s2))

	TEST("printf", printf("%s\n%s\n", str_data(&s1), str_data(&s2)));

	u64 size = s1._long_capacity;
	TEST("str_clear", str_clear(&s1))
	ASSERT(!str_len(&s1))
	ASSERT(size == s1._long_capacity)
	ASSERT(*str_data(&s1) == 0)
}

void tests_wstrings_2(void) {
	wstring s1 = wstr(), s2 = wstr();
	for (wchar c = L'a'; c <= L'z'; c++) {
		TEST("strn_cat", wstr_cat(&s1, &c, 1))
	}
	TEST("str_cpy_l", wstr_cpy(&s2, L"abcdefghijklmnopqrstuvwxyz"));

	ASSERT(wstr_len(&s1) == 26)
	ASSERT(wstr_len(&s2) == 26)
	ASSERT(!wstr_cmp(&s1, &s2))

	TEST("printf", printf("%ls\n%ls\n", wstr_data(&s1), wstr_data(&s2)));

	u64 size = s1._long_capacity;
	TEST("str_clear", wstr_clear(&s1))
	ASSERT(!wstr_len(&s1))
	ASSERT(size == s1._long_capacity)
	ASSERT(*wstr_data(&s1) == 0)
}

// void tests_strings_file(void) {
//	extern char *realpath(const char *__name, char *__resolved);
//
//	char filename[1024] = {0}, resolved[1024] = {0}, cwd[1024] = {0};
//	getcwd(cwd, 1024);
//	printf("\033[32mCWD: \033[33m%s\033[0m\n", cwd);
//	for (;;) {
//		printf("\033[32mPlease enter a filename: \033[0m");
//		scanf("%s", filename);
//		if (!realpath(filename, resolved)) {
//			fprintf(stderr, "realpath: '%s' – %s\n", filename, strerror(errno));
//			exit(1);
//		} else break;
//	}
//
//	FILE *f, *out;
//	f = fopen(resolved, "r");
//	if (!f) {
//		perror("");
//		exit(1);
//	}
//
//	out = fopen("out.txt", "w");
//	if (!out) {
//		perror("");
//		exit(1);
//	}
//
//	char   buf[1024];
//	u64	   read = 0;
//	string s	= str();
//	for (;;) {
//		read = fread(buf, 1, 1024, f);
//		TEST("strn_cat", str_cat(&s, buf, read));
//		memset(buf, 0, 1024);
//		if (read < 1024) break;
//	}
//	fclose(f);
//
//	fprintf(out, "%s", str_data(&s));
//	fclose(out);
// }

int main(void) {
	void (*tests[])(void) = {tests_array, tests_strings_memory, tests_strings_2, tests_wstrings_memory, tests_wstrings_2};
	repeat(i, arrsize(tests)) tests[i]();
}
