#define C_LIB_IMPLEMENTATION
#include "../../include/c.h"

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
	F(SIGHUP)      \
	F(SIGINT)      \
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
	longjmp(_jmp, s ? s : -1);
}

__attribute__((used))
__attribute__((constructor)) //
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
		printf(__VA_ARGS__);                                             \
		exit(1);                                                         \
	}

#define SIG_DEFAULT(sig)                                            \
	if (signal(sig, SIG_DFL) == SIG_ERR) {                          \
		error("Could not set default signal handler for %s", #sig); \
		exit(1);                                                    \
	}

int main(void) {
	/// ╔═══════════════════════════╦═══════════════╦═══════════════════════════╗
	/// ╠═══════════════════════════╣  array tests  ╠═══════════════════════════╣
	/// ╚═══════════════════════════╩═══════════════╩═══════════════════════════╝
	array_t arr;
	TEST("array_create", arr = array_create(int, 25);)
	ASSERT(arr.allocated == 25 && arr.size == 0 && arr.size_of_type == sizeof(int), "")

	TEST("array_append", repeat(i, 25) array_append(size_t, &arr, i);)
	ASSERT(arr.size == 25, "")

	TEST("array_foreach", array_foreach(int, i, arr) printf("%d", *i);)

	TEST("array_foreach", array_foreach(int, i, arr) *i = 47;)

	printf("\n");

	TEST("array_foreach", array_foreach(int, i, arr) printf("%d", *i);)

	printf("\n");

	/// ╔═══════════════════════════╦════════════════╦═══════════════════════════╗
	/// ╠═══════════════════════════╣  string tests  ╠═══════════════════════════╣
	/// ╚═══════════════════════════╩════════════════╩═══════════════════════════╝

	string s, s1, s2;
	TEST("str", s = str();)
	ASSERT(__str_is_small(&s), "")

	{
		const char  *_data = "Bla Bla bla\n";
		const size_t _len  = strlen(_data);
		size_t		 s_len;
		TEST("str_cpy_l", str_cpy(&s, _data);)
		ASSERT(strlen(s._M_small_data) == _len, "strlen: %lu != %lu\n", strlen(s._M_small_data), _len)

		TEST("Value: ", printf("%s", str_data(&s)));

		TEST("str_len", s_len = str_len(&s))
		ASSERT(s_len == _len, "str_len: %lu != %lu\n", s_len, _len)

		ASSERT(__str_is_small(&s), "")
	}

	TEST("str_clear", str_clear(&s))
	else s = str();
	ASSERT(str_len(&s) == 0, "str_len not 0 after call to str_clear")

	TEST("printf(str_data(&s)) ", printf("%s", str_data(&s));)

	TEST("str_from", s1 = str("Short string"))
	TEST("str_from", s2 = str("Long string Long string Long string Long string Long string Long string Long string"))

	ASSERT(__str_is_small(&s1), "")
	ASSERT(!__str_is_small(&s2), "")
}
