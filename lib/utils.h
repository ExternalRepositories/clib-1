#ifndef C_LIB_UTILS_H
#define C_LIB_UTILS_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#define MAX(X, Y)	(X > Y ? X : Y)
#define MIN(X, Y)	(X < Y ? X : Y)
#define CAR(X, ...) X

#define repeat(_var_name, _until) for (size_t _var_name = 0, _until_val = _until; _var_name < _until_val; _var_name++)

#		define _INT_TYPES_(F, ...) \
F(__VA_ARGS__ i8) \
F(__VA_ARGS__ i16) \
F(__VA_ARGS__ i32) \
F(__VA_ARGS__ i64) \
F(__VA_ARGS__ u8) \
F(__VA_ARGS__ u16) \
F(__VA_ARGS__ u32) \
F(__VA_ARGS__ u64) \

#		define	_INT_TYPES(F) \
_INT_TYPES_(F) \
_INT_TYPES_(F, const) \
_INT_TYPES_(F, volatile) \
_INT_TYPES_(F, const volatile) \


#		define _GENERIC_TYPE_OR_NONE(...) CAR(__VA_ARGS__ __VA_OPT__(, ) __generic_no_arg)

typedef int8_t	i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t	 u1;
typedef uint8_t	 u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/// Dummy type that represents no argument
struct __generic_no_arg {
	char __unused__;
};
extern struct __generic_no_arg __generic_no_arg;

typedef float  f32;
typedef double f64;

u64 min(u64 __a, u64 __b);
u64 max(u64 __a, u64 __b);
u64 between(u64 __what, u64 __a, u64 __b);
void die(const char *__fmt, ...);

size_t __ceil_power_of2(size_t __n);

#endif /*C_LIB_UTILS_H*/

#ifdef C_LIB_UTILS_IMPLEMENTATION
#undef C_LIB_UTILS_IMPLEMENTATION
#include <immintrin.h>

struct __generic_no_arg __generic_no_arg;

u64 min(u64 __a, u64 __b) {
	return __a < __b ? __a : __b;
}

u64 max(u64 __a, u64 __b) {
	return __a > __b ? __a : __b;
}

u64 between(u64 __what, u64 __a, u64 __b) {
	return __a <= __what && __what <= __b;
}

__attribute__((noreturn))
__attribute__((format(printf, 1, 2))) //
void die(const char *__fmt, ...) {
	va_list __ap;
	va_start(__ap, __fmt);
	fprintf(stderr, "\033[31m");
	vfprintf(stderr, __fmt, __ap);
	fprintf(stderr, "\033[0m");
	va_end(__ap);
	exit(1);
}

size_t __ceil_power_of2(size_t __n) {
#ifdef __AVX__
	return (__n & (1 << (((sizeof(size_t) << 3) - _lzcnt_u64(__n)) - 1))) << 1;
#else
	size_t __power = 1;
	while (__n > __power) __power <<= 1;
	return __power;
#endif
}

#endif
