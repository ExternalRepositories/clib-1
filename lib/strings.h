#ifndef C_LIB_STRINGS_H
/// Header guard will be defined later on in the file

#	include "utils.h"

#	include <assert.h>
#	include <stdarg.h>
#	include <stddef.h>
#	include <stdio.h>
#	include <endian.h>

#	if !defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN) || defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
#		error "Please define either BIG_ENDIAN or LITTLE_ENDIAN before including this header."
#		include "/Abort Compilation: See Error Above"
#	endif

/// In practice, these are gonna be 23 and 46
enum {
	_STR_SMALL_RAW_CAPACITY = (sizeof(char *) + sizeof(u64) * 2 - 1),
#	if defined(LITTLE_ENDIAN)
	_STR_SMALL_MAX_CAPACITY = _STR_SMALL_RAW_CAPACITY,
#	elif defined(BIG_ENDIAN)
	_STR_SMALL_MAX_CAPACITY			 = (_STR_SMALL_RAW_CAPACITY << 1),
#	endif
#	if defined(LITTLE_ENDIAN)
	_STR_SMALL_MAX_CAPACITY_AND_FLAG = _STR_SMALL_MAX_CAPACITY | (1 << 7),
#	elif defined(BIG_ENDIAN)
	_STR_SMALL_MAX_CAPACITY_AND_FLAG = _STR_SMALL_MAX_CAPACITY | 1,
#	endif
};

const u64 _STR_SMALL_CAP_CONST = ~(1lu << 7lu);
const u64 _STR_LONG_CAP_CONST  = ~(1lu << 63lu);

#	ifndef string
#		define string string
#	endif

typedef union __string_u {
	char __raw[_STR_SMALL_RAW_CAPACITY + 1];

	struct {
		char __small_data[_STR_SMALL_RAW_CAPACITY];

		/// On a little-endian machine, (__small_capacity & _STR_SMALL_CAP_CONST) contains the
		/// free capacity of the string. Its MSB shall be unset if the string
		/// is small, and set otherwise. Accessing __small_capacity is valid
		/// iff its MSB is unset. <p>

		/// On a big-endian machine, (__small_capacity >> 1) contains the
		/// free capacity of the string. Its LSB shall be unset if the string
		/// is small, and set otherwise. Accessing __small_capacity is valid
		/// iff its LSB is unset. <p>

		/// When the string is full, __small_capacity is 0 and acts as the null terminator.
		char __small_capacity;
	};

	struct {
		char *__long_data;
		u64	  __long_capacity;

		/// On a little-endian machine, (__long_size & _STR_LONG_CAP_CONST) contains
		/// the size of the string. Its MSB shall be unset if the string
		/// is small, and set otherwise. Accessing __long_size is valid
		/// iff its MSB is set. <p>

		/// On a big-endian machine, (__long_size >> 1) contains the
		/// size of the string. Its LSB shall be unset if the string is
		/// small, and set otherwise. Accessing __long_size is valid
		/// iff its LSB is set <p>
		u64 __long_size;
	};
} string;

typedef struct __string_view_s {
	const char *const data;
	const u64		  size;
} string_view_t;

static_assert(sizeof(union __string_u) == _STR_SMALL_RAW_CAPACITY + 1, "union __string_u must be 24 bytes long");

/// Create and return a new empty string with a length of STRINGDEF_MIN_SIZE
string str_create();
/// Allocate enough memory for __str to be able to fit at least __mem_req characters.
void str_alloc(string *__str, u64 __mem_req);
/// Create and return a new string containing __literal, which is copied
string str_from(const char *__literal);
/// Create a copy of __str
string str_dup(const string *__str);

/// Concatenate __literal to __dest; return the length of __literal
u64 str_cat_l(string *__dest, const char *__literal);
/// Compare __literal to __str
int str_cmp_l(const string *__str, const char *__literal);
/// Copy __literal to __dest
void str_cpy_l(string *__dest, const char *__literal);
/// Reverse __literal
char *str_rev_l(char *__literal);

/// Concatenate __what to __dest; return the length of __what
u64 str_cat(string *__dest, const string *__what);
/// Compare __other to __dest
int str_cmp(const string *__dest, const string *__other);
/// Copy __src to __dest
void str_cpy(string *__dest, const string *__src);
/// Reverse __str
string *str_rev(string *__str);

/// Calculate and return the length of __str (O(1))
u64 str_len(const string *__str);
/// Get a pointer to the contents of the string
static char *str_data(string *__str);

/// Remove all data from __str
void str_clear(string *__str);
/// Delete __str
void str_free(string *__string);

#endif /*C_LIB_STRINGS_H*/

#ifdef C_LIB_STRINGS_IMPLEMENTATION
#	include "utils.h"

#	include <immintrin.h>
#	include <stdlib.h>
#	include <string.h>
#	include <unistd.h>

#	if !defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN) || defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
#		error "Please define either BIG_ENDIAN or LITTLE_ENDIAN before including this header."
#		include "/Abort Compilation: See Error Above"
#	endif

enum {
	_STR_REALLOC_FACTOR = 2,
};

#	undef str
#	undef str_len
#	undef str_cpy
#	undef str_cat
#	undef str_cmp
#	undef str_rev

#	ifndef string
#		define string string
#	endif

/// ╔═══════════════════════════╦═════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  INTERNALS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═════════════╩═══════════════════════════╝

/// Check if a string is a small string
static u1 __str_is_small(const string *__str) {
#	if defined(LITTLE_ENDIAN)
	return !(__str->__raw[_STR_SMALL_RAW_CAPACITY] & (1 << 7));
#	elif defined(BIG_ENDIAN)
	return !(__str->__raw[_STR_SMALL_RAW_CAPACITY] & 1);
#	endif
}

/// The SSO flag is the last bit of the string.
/// Depending on the endianness of the architecture,
/// it must be tested and set in different ways.

void __str_setflag_long(string *__str) {
#	if defined(LITTLE_ENDIAN)
	__str->__raw[_STR_SMALL_RAW_CAPACITY] |= (1 << 7);
#	elif defined(BIG_ENDIAN)
	__str->__raw[_STR_SMALL_RAW_CAPACITY] |= 1;
#	endif
}

void __str_setflag_small(string *__str) {
#	if defined(LITTLE_ENDIAN)
	__str->__raw[_STR_SMALL_RAW_CAPACITY] &= ~(1 << 7);
#	elif defined(BIG_ENDIAN)
	__str->__raw[_STR_SMALL_RAW_CAPACITY] &= ~1;
#	endif
}

static u64 __str_small_capacity(const string *__str) {
#	if defined(LITTLE_ENDIAN)
	return __str->__small_capacity & ~(1 << 7);
#	elif defined(BIG_ENDIAN)
	return __str->__small_capacity >> 1;
#	endif
}

/// Get the small size of __str
static u8 __str_small_size(const string *__str) {
	return _STR_SMALL_RAW_CAPACITY - __str_small_capacity(__str);
}

/// Get the small size of __str
static u64 __str_long_size(const string *__str) {
#	if defined(LITTLE_ENDIAN)
	return __str->__long_size & ~(1lu << 63lu);
#	elif defined(BIG_ENDIAN)
	return __str->__long_size >> 1;
#	endif
}

static u64 __charcount_to_size(u64 __number_of_chars) {
#	if defined(LITTLE_ENDIAN)
	return __number_of_chars;
#	elif defined(BIG_ENDIAN)
	return __number_of_chars << 1;
#	endif
}
//
// static u64 __str_small_size_add(u64 __number_of_chars) {
//#	if defined(LITTLE_ENDIAN)
//	return __number_of_chars << 1;
//#	elif defined(BIG_ENDIAN)
//	return __number_of_chars;
//#	endif
//}

/// Concatenates __srclen elements from __raw_src to __data
static void strn_cat(
	string	   *__dest,	 /// The destination string
	const char *__data,	 /// The data to be appended
	u64			__srclen /// The size of the data in elements
) {
	str_alloc(__dest, __srclen);
	char *__mem_start = str_data(__dest);
	__mem_start += str_len(__dest);

	memcpy(__mem_start, __data, __srclen);

	u64 __size = __charcount_to_size(__srclen);
	if (__str_is_small(__dest)) __dest->__small_capacity -= __size;
	else __dest->__long_size += __size;
}

/// Compares __str and __data, where the latter is of length __len
static int strn_cmp(
	const string *__str,  /// The string to be compared
	const char   *__data, /// The data with which to compare
	u64			  __len	  /// The size of said data in elements
) {
	u64 __size = str_len(__str);
	if (__size == __len) return memcmp(str_data((string *) __str), __data, __size);

	int __res = memcmp(str_data((string *) __str), __data, min(__size, __len));
	if (!__res) return __size < __len ? -1 : 1;
	return __res;
}

/// Reverses __data in place
static char *__string_rev_impl(
	char *__data, /// data to be reversed
	u64	  __len	  /// size of the data
) {
	if (__data) {
		__data[__len] = 0;

		for (u64 i = 0; i < __len / 2 + __len % 2; i++) {
			__data[i]			  = __data[__len - 1 - i];
			__data[__len - 1 - i] = __data[i];
		}
	}
	return __data;
}

/// ╔═══════════════════════════╦══════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  ALLOCATION  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩══════════════╩═══════════════════════════╝

/// Create and return a new empty string
string str_create() {
	string __str		   = {0};
	__str.__small_capacity = _STR_SMALL_MAX_CAPACITY;
	__str_setflag_small(&__str);
	return __str;
}

/// Allocate enough memory for __str to be able to fit at least __mem_req characters.
void str_alloc(
	string *__str /* string to be resized */,
	u64		__mem_req /* minimum memory to be allocated */) {
	if (__mem_req == 0) return;

	/// Data stored on the stack, we might have to copy it
	if (__str_is_small(__str)) {
		/// We need more memory than the SSO buffer can hold
		if (__mem_req > __str_small_capacity(__str)) {
			u64 __str_size	   = __str_small_size(__str);
			u64 __new_mem_size = __mem_req + __str_size;

			/// We cannot yet store this in the string because it overlaps with
			/// the data stored in the SSO buffer
			u64	  __heap_capacity = __new_mem_size * _STR_REALLOC_FACTOR;
			char *__mem			  = malloc(__heap_capacity);

			/// Copy the data from the SSO buffer to the heap if not empty
			if (*__str->__small_data) memcpy(__mem, __str->__small_data, __str_size);

			/// Set up the struct properly
			__str->__long_data = __mem;
			__str->__long_size = __charcount_to_size(__str_size);
			__str_setflag_long(__str);
			__str->__long_capacity = __heap_capacity;
		}

		/// __mem_req fits in SSO buffer; do nothing
		else {}
	}

	/// Data stored on the heap, we need to realloc
	else {
		u64 __str_size = __str_long_size(__str);
		if (__mem_req > __str->__long_capacity - __str_size) {
			char *__old_data	   = __str->__long_data;
			__str->__long_capacity = (__mem_req + __str_size) * _STR_REALLOC_FACTOR;
			__str->__long_data	   = realloc(__str->__long_data, __str->__long_capacity);
			if (__old_data != __str->__long_data) free(__old_data);
		}

		/// __mem_req fits in heap buffer; do nothing
		else {}
	}
}

/// Create and return a new string containing __literal, which is copied
string str_from(const char *__literal) {
	string __s = str_create();
	str_cpy_l(&__s, __literal);
	return __s;
}

/// Create a copy of __str
string str_dup(const string *__str) {
	string __s = str_create();
	str_cpy(&__s, __str);
	return __s;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  LIT. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Concatenate __literal to __dest; return the length of __literal
u64 str_cat_l(string *__dest, const char *__literal) {
	u64 __l = strlen(__literal);
	strn_cat(__dest, __literal, __l);
	return __l;
}

/// Compare __literal to __str
int str_cmp_l(const string *__str, const char *__literal) {
	return strn_cmp(__str, __literal, strlen(__literal));
}

/// Copy __literal to __dest
void str_cpy_l(string *__dest, const char *__literal) {
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
u64 str_cat(string *__dest, const string *__what) {
	u64 __l = str_len(__what);
	strn_cat(__dest, str_data((string *) __what), __l);
	return __l;
}

/// Compare __other to __dest
int str_cmp(const string *__dest, const string *__other) {
	return strn_cmp(__dest, str_data((string *) __other), str_len(__other));
}

/// Copy __src to __dest
void str_cpy(string *__dest, const string *__src) {
	str_clear(__dest);
	str_cat(__dest, __src);
}

string *str_rev(string *__str) {
	__string_rev_impl(str_data(__str), str_len(__str));
	return __str;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  MEM. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Calculate and return the length of __str (O(1))
u64 str_len(const string *__str) {
	return __str_is_small(__str)
			   ? __str_small_size(__str)
			   : __str_long_size(__str);
}

char *str_data(string *__str) {
	return __str_is_small(__str)
			   ? __str->__small_data
			   : __str->__long_data;
}

void str_clear(string *__str) {
	char *__str_data = str_data(__str);
	if (!*__str_data) return;

	if (__str_is_small(__str)) {
		__str->__small_capacity = _STR_SMALL_MAX_CAPACITY;
		__str_setflag_small(__str);
	} else {
		__str->__long_size = 0;
		__str_setflag_long(__str);
	}

	*__str_data = 0;
}

void str_free(string *__str) {
	if (__str_is_small(__str)) return;
	free(__str->__long_data);
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
				string * : str_dup,							\
				char *     : str_from,                                         \
				const char *     : str_from, 						\
				struct __generic_no_arg     : str_create									\
			)(__VA_ARGS__)

#		define str_len(__string_like) _Generic((__string_like), \
				string * : str_len,   				\
				const char *     : strlen,                 \
				char *     : strlen                 \
			)(__string_like)

#		define str_cpy(__string, __string_like) _Generic((__string_like),	\
				string * : str_cpy, 	                \
				const char *     : str_cpy_l,                                            \
				char *     : str_cpy_l                                            \
			)(__string, __string_like)

#		define str_cat(__string, __string_like) _Generic((__string_like),	\
				string * : str_cat,     	                                    \
				const char *     : str_cat_l, 											\
				char *     : str_cat_l 											\
			)(__string, __string_like)

#		define str_cmp(__string, __string_like) _Generic((__string_like),	\
				string * : str_cmp,         	        \
				const char *     : str_cmp_l, 				\
				char *     : str_cmp_l 				\
			)(__string, __string_like)

#		define str_rev(__string_like) _Generic((__string_like), \
				string* : str_rev									\
				char *    : str_rev_l 								\
			)(__string_like)
// clang-format on
#	endif
#endif