#ifndef C_LIB_UTILS_H
#define C_LIB_UTILS_H

#include <stdint.h>
#include <stdio.h>

#define MAX(X, Y) (X > Y ? X : Y)
#define MIN(X, Y) (X < Y ? X : Y)

#define repeat(_var_name, _until) for (size_t _var_name = 0, _until_val = _until; _var_name < _until_val; _var_name++)

typedef int8_t	i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t	 u1;
typedef uint8_t	 u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

u64 min(u64 __a, u64 __b);
u64 max(u64 __a, u64 __b);
u64 between(u64 __what, u64 __a, u64 __b);

size_t __ceil_power_of2(size_t __n);

#endif /*C_LIB_UTILS_H*/

#ifdef C_LIB_UTILS_IMPLEMENTATION
#undef C_LIB_UTILS_IMPLEMENTATION
#include <immintrin.h>

u64 min(u64 __a, u64 __b) {
	return __a < __b ? __a : __b;
}

u64 max(u64 __a, u64 __b) {
	return __a > __b ? __a : __b;
}

u64 between(u64 __what, u64 __a, u64 __b) {
	return __a <= __what && __what <= __b;
}

//__attribute__((always_inline))
//size_t
//	__Ceil_power_of2_intrin(size_t __n) {
//	return (__n & (1 << (((sizeof(size_t) << 3) - _lzcnt_u64(__n)) - 1))) << 1;
//}
//
//__attribute__((always_inline))
//size_t
//	__Ceil_power_of2_C(size_t __n) {
//	size_t __power = 1;
//	while (__n > __power) __power <<= 1;
//	return __power;
//}
//
//static size_t (*__Ceil_power_of2_res(void))(size_t) {
//	__builtin_cpu_init();
//	if (__builtin_cpu_supports("avx")) return __Ceil_power_of2_intrin;
//	else
//		return __Ceil_power_of2_C;
//}
//__attribute__((ifunc("__Ceil_power_of2_res")));

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