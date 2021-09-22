#ifndef C_LIB_STRINGS_H
/// Header guard will be defined later on in the file

#	include "utils.h"

#	include <assert.h>
#	include <stdarg.h>
#	include <stddef.h>
#	include <stdio.h>

/// In practice, these are gonna be 23 and 46
enum {
	_STR_SSO_SIZE	  = (sizeof(char *) + sizeof(size_t) * 2 - 1),
	_STR_SSO_CAPACITY = (_STR_SSO_SIZE << 1),
};

#	ifndef _STR_TYPE_NAME
#		define _STR_TYPE_NAME string
#	endif

typedef union __string_u {
	char _M_raw[_STR_SSO_SIZE + 1];

	/// _M_small_capacity contains the free capacity of the _STR_TYPE_NAME * 2.
	/// When the _STR_TYPE_NAME is full, _M_small_size is 0 and acts as the null terminator.
	/// The LSB of _M_small_capacity is always 0
	struct {
		char _M_small_data[_STR_SSO_SIZE];
		char _M_small_capacity;
	};

	/// _M_long_size contains the size of the _STR_TYPE_NAME * 2
	/// The LSB of _M_small_capacity is always 1
	struct {
		char	 *_M_long_data;
		size_t _M_long_capacity;
		size_t _M_long_size;
	};
} _STR_TYPE_NAME;

typedef struct __string_view_s {
	const char *const data;
	const size_t	  size;
} string_view_t;

static_assert(sizeof(union __string_u) == _STR_SSO_SIZE + 1, "union __string_u must be 24 bytes long");

/// Create and return a new empty _STR_TYPE_NAME with a length of STRINGDEF_MIN_SIZE
_STR_TYPE_NAME str_create();
/// Allocate enough memory for __str to be able to fit at least __mem_req characters.
void str_alloc(_STR_TYPE_NAME *__str, size_t __mem_req);
/// Create and return a new _STR_TYPE_NAME containing __literal, which is copied
_STR_TYPE_NAME str_from(const char *__literal);
/// Create a copy of __str
_STR_TYPE_NAME str_dup(const _STR_TYPE_NAME *__str);

/// Concatenate __literal to __dest; return the length of __literal
size_t str_cat_l(_STR_TYPE_NAME *__dest, const char *__literal);
/// Compare __literal to __str
int str_cmp_l(const _STR_TYPE_NAME *__str, const char *__literal);
/// Copy __literal to __dest
void str_cpy_l(_STR_TYPE_NAME *__dest, const char *__literal);
/// Reverse __literal
char *str_rev_l(char *__literal);

/// Concatenate __what to __dest; return the length of __what
size_t str_cat(_STR_TYPE_NAME *__dest, const _STR_TYPE_NAME *__what);
/// Compare __other to __dest
int str_cmp(const _STR_TYPE_NAME *__dest, const _STR_TYPE_NAME *__other);
/// Copy __src to __dest
void str_cpy(_STR_TYPE_NAME *__dest, const _STR_TYPE_NAME *__src);
/// Reverse __str
_STR_TYPE_NAME *str_rev(_STR_TYPE_NAME *__str);

/// Calculate and return the length of __str (O(1))
size_t str_len(const _STR_TYPE_NAME *__str);
/// Get a pointer to the contents of the _STR_TYPE_NAME
static char *str_data(_STR_TYPE_NAME *__str);

/// Remove all data from __str
void str_clear(_STR_TYPE_NAME *__str);
/// Delete __str
void str_free(_STR_TYPE_NAME *__string);

#endif /*C_LIB_STRINGS_H*/

#ifdef C_LIB_STRINGS_IMPLEMENTATION
#	include "utils.h"

#	include <immintrin.h>
#	include <stdlib.h>
#	include <string.h>
#	include <unistd.h>

enum {
	_STR_SSO_FLAG		= 1,
	_STR_SSO_FLAG_SHORT = 0,
	_STR_SSO_FLAG_LONG	= 1,
	_STR_REALLOC_FACTOR = 2,
};

#	undef str
#	undef str_len
#	undef str_cpy
#	undef str_cat
#	undef str_cmp
#	undef str_rev

#	ifndef _STR_TYPE_NAME
#		define _STR_TYPE_NAME string
#	endif

/// ╔═══════════════════════════╦═════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  INTERNALS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═════════════╩═══════════════════════════╝

/// Check if a _STR_TYPE_NAME is a small _STR_TYPE_NAME
static u1 __str_is_small(const _STR_TYPE_NAME *__str) {
	return (__str->_M_small_capacity & _STR_SSO_FLAG) == _STR_SSO_FLAG_SHORT;
}

/// Get the small size of __srt
static u8 __str_small_size(const _STR_TYPE_NAME *__str) {
	return _STR_SSO_SIZE - (__str->_M_small_capacity >> 1);
}

/// Get the small size of __srt
static u8 __str_long_size(const _STR_TYPE_NAME *__str) {
	return (__str->_M_long_capacity >> 1);
}

static size_t __str_small_capacity(const _STR_TYPE_NAME *__str) {
	return __str->_M_small_capacity >> 1;
}

/// Concatenates __srclen elements from __raw_src to __data
static void __string_cat_impl(
	_STR_TYPE_NAME *__dest,	 /// The destination _STR_TYPE_NAME
	const char	   *__data,	 /// The data to be appended
	size_t			__srclen /// The size of the data in elements
) {
	str_alloc(__dest, __srclen);
	char *__mem_start = str_data(__dest);
	__mem_start += str_len(__dest);

	memcpy(__mem_start, __data, __srclen);
	size_t __len_internal = __srclen << 1;
	if (__str_is_small(__dest)) __dest->_M_small_capacity -= __len_internal;
	else
		__dest->_M_long_size += __len_internal | _STR_SSO_FLAG_LONG;
}

/// Compares __str and __data, where the latter is of length __len
static int __string_cmp_impl(
	const _STR_TYPE_NAME *__str,  /// The _STR_TYPE_NAME to be compared
	const char		   *__data, /// The data with which to compare
	size_t				  __len	  /// The size of said data in elements
) {
	size_t __size = str_len(__str);
	if (__size == __len) return memcmp(str_data((_STR_TYPE_NAME *) __str), __data, __size);

	int __res = memcmp(str_data((_STR_TYPE_NAME *) __str), __data, min(__size, __len));
	if (!__res) return __size < __len ? -1 : 1;
	return __res;
}

/// Reverses __data in place
static char *__string_rev_impl(
	char	 *__data, /// data to be reversed
	size_t __len   /// size of the data
) {
	if (__data) {
		__data[__len] = 0;

		for (size_t i = 0; i < __len / 2 + __len % 2; i++) {
			__data[i]			  = __data[__len - 1 - i];
			__data[__len - 1 - i] = __data[i];
		}
	}
	return __data;
}

/// ╔═══════════════════════════╦══════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  ALLOCATION  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩══════════════╩═══════════════════════════╝

/// Create and return a new empty _STR_TYPE_NAME
_STR_TYPE_NAME str_create() {
	_STR_TYPE_NAME __str	= {0};
	__str._M_small_capacity = _STR_SSO_CAPACITY;
	return __str;
}

/// Allocate enough memory for __str to be able to fit at least __mem_req characters.
void str_alloc(
	_STR_TYPE_NAME *__str /* _STR_TYPE_NAME to be resized */,
	size_t			__mem_req /* minimum memory to be allocated */) {
	if (__mem_req == 0) return;

	/// Data stored on the stack, we might have to copy it
	if (__str_is_small(__str)) {
		/// We need more memory than the SSO buffer can hold
		if (__mem_req > __str_small_capacity(__str)) {
			size_t __str_size = __str_small_size(__str);

			/// We cannot yet store this in the _STR_TYPE_NAME because it overlaps with
			/// the data stored in the SSO buffer
			size_t __heap_capacity = max(__mem_req, __str_size) * _STR_REALLOC_FACTOR;
			char	 *__mem		   = malloc(__heap_capacity);

			/// Copy the data from the SSO buffer to the heap
			memcpy(__mem, __str->_M_small_data, __str_size);

			/// Set up the struct properly
			__str->_M_long_data		= __mem;
			__str->_M_long_size		= (__str_size << 1) | _STR_SSO_FLAG_LONG;
			__str->_M_long_capacity = __heap_capacity;
		}

		/// __mem_req fits in SSO buffer; do nothing
		else {}
	}

	/// Data stored on the heap, we need to realloc
	else {
		size_t __str_size = __str->_M_long_size;
		if (__mem_req > __str_size) {
			char *__old_data		= __str->_M_long_data;
			__str->_M_long_capacity = max(__mem_req, __str_size) * _STR_REALLOC_FACTOR;
			__str->_M_long_data		= realloc(__str->_M_long_data, __str->_M_long_capacity);
			free(__old_data);
		}

		/// __mem_req fits in heap buffer; do nothing
		else {}
	}
}

/// Create and return a new _STR_TYPE_NAME containing __literal, which is copied
_STR_TYPE_NAME str_from(const char *__literal) {
	_STR_TYPE_NAME __s = str_create();
	str_cpy_l(&__s, __literal);
	return __s;
}

/// Create a copy of __str
_STR_TYPE_NAME str_dup(const _STR_TYPE_NAME *__str) {
	_STR_TYPE_NAME __s = str_create();
	str_cpy(&__s, __str);
	return __s;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  LIT. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Concatenate __literal to __dest; return the length of __literal
size_t str_cat_l(_STR_TYPE_NAME *__dest, const char *__literal) {
	size_t __l = strlen(__literal);
	__string_cat_impl(__dest, __literal, __l);
	return __l;
}

/// Compare __literal to __str
int str_cmp_l(const _STR_TYPE_NAME *__str, const char *__literal) {
	return __string_cmp_impl(__str, __literal, strlen(__literal));
}

/// Copy __literal to __dest
void str_cpy_l(_STR_TYPE_NAME *__dest, const char *__literal) {
	str_clear(__dest);
	str_cat_l(__dest, __literal);
}

char *str_rev_l(char *__literal) {
	return __string_rev_impl(__literal, strlen(__literal));
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  STR. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Concatenate __what to __dest; return the length of __what
size_t str_cat(_STR_TYPE_NAME *__dest, const _STR_TYPE_NAME *__what) {
	size_t __l = str_len(__what);
	__string_cat_impl(__dest, str_data((_STR_TYPE_NAME *) __what), __l);
	return __l;
}

/// Compare __other to __dest
int str_cmp(const _STR_TYPE_NAME *__dest, const _STR_TYPE_NAME *__other) {
	return __string_cmp_impl(__dest, str_data((_STR_TYPE_NAME *) __other), str_len(__other));
}

/// Copy __src to __dest
void str_cpy(_STR_TYPE_NAME *__dest, const _STR_TYPE_NAME *__src) {
	str_clear(__dest);
	str_cat(__dest, __src);
}

_STR_TYPE_NAME *str_rev(_STR_TYPE_NAME *__str) {
	__string_rev_impl(str_data(__str), str_len(__str));
	return __str;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  MEM. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Calculate and return the length of __str (O(1))
size_t str_len(const _STR_TYPE_NAME *__str) {
	return __str_is_small(__str) ? __str_small_size(__str) : __str->_M_long_size >> 1;
}

char *str_data(_STR_TYPE_NAME *__str) {
	return __str_is_small(__str) ? __str->_M_small_data : __str->_M_long_data;
}

void str_clear(_STR_TYPE_NAME *__str) {
	char *__str_data = str_data(__str);
	if (!*__str_data) return;

	if (__str_is_small(__str)) __str->_M_small_capacity = _STR_SSO_CAPACITY | _STR_SSO_FLAG_SHORT;
	else
		__str->_M_long_size = _STR_SSO_FLAG_LONG;

	*__str_data = 0;
}

void str_free(_STR_TYPE_NAME *__str) {
	if (__str_is_small(__str)) return;

	free(__str->_M_long_data);
	*__str = str_create();
}
#endif

#ifndef C_LIB_STRINGS_H
#	define C_LIB_STRINGS_H
#	if __STDC_VERSION__ >= 201112L /// 201112L == C11
///  '// clang-format off' doesn't work with '///'!
// clang-format off

struct __generic_no_arg { } __generic_no_arg;

#		define _GENERIC_TYPE_OR_NULL(...) CAR(__VA_ARGS__ __VA_OPT__(, ) __generic_no_arg)

#		define str(...) _Generic((_GENERIC_TYPE_OR_NULL(__VA_ARGS__)),	\
				_STR_TYPE_NAME * : str_dup,							\
				char *     : str_from,                                         \
				const char *     : str_from, 						\
				struct __generic_no_arg     : str_create									\
			)(__VA_ARGS__)

#		define str_len(__string_like) _Generic((__string_like), \
				_STR_TYPE_NAME * : str_len,   				\
				const char *     : strlen,                 \
				char *     : strlen                 \
			)(__string_like)

#		define str_cpy(__string, __string_like) _Generic((__string_like),	\
				_STR_TYPE_NAME * : str_cpy, 	                \
				const char *     : str_cpy_l,                                            \
				char *     : str_cpy_l                                            \
			)(__string, __string_like)

#		define str_cat(__string, __string_like) _Generic((__string_like),	\
				_STR_TYPE_NAME * : str_cat,     	                                    \
				const char *     : str_cat_l, 											\
				char *     : str_cat_l 											\
			)(__string, __string_like)

#		define str_cmp(__string, __string_like) _Generic((__string_like),	\
				_STR_TYPE_NAME * : str_cmp,         	        \
				const char *     : str_cmp_l, 				\
				char *     : str_cmp_l 				\
			)(__string, __string_like)

#		define str_rev(__string_like) _Generic((__string_like), \
				_STR_TYPE_NAME* : str_rev									\
				char *    : str_rev_l 								\
			)(__string_like)
// clang-format on
#	endif
#endif