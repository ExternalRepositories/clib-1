#define C_LIB_IMPLEMENTATION
#include "../../include/c.h"

#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>

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

#define SIGNAL_HANDLER_REG(sig)                                    \
	if (signal(sig, sig_handler) == SIG_ERR) {                     \
		error("Could not initialise signal handler for %s", #sig); \
		exit(1);                                                   \
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
		error("Assertion \033[32m%s\033[31m failed\n", STRINGISE(cond));       \
		__VA_ARGS__;                                                           \
		exit(1);                                                         \
	}

#define SIG_DEFAULT(sig)                                                   \
	if (signal(sig, SIG_DFL) == SIG_ERR) {                                 \
		error("Could not initialise default signal handler for %s", #sig); \
		exit(1);                                                           \
	}

int main(void) {
	array_t arr;
	TEST("array_create", arr = array_create(int, 25);)
	ASSERT(arr.allocated == 25 && arr.size == 0 && arr.size_of_type == sizeof(int))

	TEST("array_append", repeat(i, 25) array_append(&arr, i);)
	ASSERT(arr.size == 25)

	TEST("array_foreach", array_foreach(int, i, arr) printf("%d", *i);)
	TEST("array_foreach", array_foreach(int, i, arr) *i = 47;)
	TEST("array_foreach", array_foreach(int, i, arr) printf("%d", *i);)

	printf("\n");

	string_t s;
	TEST("str", s = str();)
	ASSERT(__str_is_small(&s))

	TEST("str_cpy_l", str_cpy_l(&s, "Bla Bla bLaj");)
	ASSERT(strlen(s.data) == strlen("Bla Bla bLaj"), printf("%lu != %lu\n", strlen(s.data), strlen("Bla Bla bLaj")))

	TEST("printf(s.data)", printf("%s", s.data);)
}
