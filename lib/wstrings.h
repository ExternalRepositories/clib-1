#ifndef C_LIB_WSTRINGS_H
/// Header guard will be defined later on in the file
#	include "utils.h"

#	include <assert.h>
#	include <stdarg.h>
#	include <stddef.h>
#	include <stdio.h>
#	include <endian.h>

#	if defined(__BYTE_ORDER)
#		if __BYTE_ORDER == __LITTLE_ENDIAN && !defined(LITTLE_ENDIAN)
#			define LITTLE_ENDIAN
#		elif __BYTE_ORDER == __BIG_ENDIAN && !defined(BIG_ENDIAN)
#			define BIG_ENDIAN
#		endif
#	endif

#	if !defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN) || defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
#		error "Please define either BIG_ENDIAN or LITTLE_ENDIAN before including this header."
#		include "/Abort Compilation: See Error Above"
#	endif

typedef i32 wchar;

/// In practice, these are gonna be 23 and 46
enum {
	__WSTR_SMALL_RAW_CAPACITY = ((sizeof(wchar *) + sizeof(u64) * 2) / sizeof(wchar) - 1),
#	if defined(LITTLE_ENDIAN)
	__WSTR_SMALL_MAX_CAPACITY = __WSTR_SMALL_RAW_CAPACITY,
#	elif defined(BIG_ENDIAN)
	__WSTR_SMALL_MAX_CAPACITY = (__WSTR_SMALL_RAW_CAPACITY << 1),
#	endif
};

#	ifndef wstring
#		define wstring wstring
#	endif

typedef union __wstring_u {
	wchar __raw[__WSTR_SMALL_RAW_CAPACITY + 1];

	struct {
		wchar __small_data[__WSTR_SMALL_RAW_CAPACITY];

		/// On a little-endian machine, (__small_capacity & _WSTR_SMALL_CAP_CONST) contains the
		/// free capacity of the wstring. Its MSB shall be unset if the wstring
		/// is small, and set otherwise. Accessing __small_capacity is valid
		/// iff its MSB is unset. <p>

		/// On a big-endian machine, (__small_capacity >> 1) contains the
		/// free capacity of the wstring. Its LSB shall be unset if the wstring
		/// is small, and set otherwise. Accessing __small_capacity is valid
		/// iff its LSB is unset. <p>

		/// When the wstring is full, __small_capacity is 0 and acts as the null terminator.
		wchar __small_capacity;
	};

	struct {
		wchar *__long_data;
		u64	   __long_capacity;

		/// On a little-endian machine, (__long_size & _WSTR_LONG_CAP_CONST) contains
		/// the size of the wstring. Its MSB shall be unset if the wstring
		/// is small, and set otherwise. Accessing __long_size is valid
		/// iff its MSB is set. <p>

		/// On a big-endian machine, (__long_size >> 1) contains the
		/// size of the wstring. Its LSB shall be unset if the wstring is
		/// small, and set otherwise. Accessing __long_size is valid
		/// iff its LSB is set <p>
		u64 __long_size;
	};
} wstring;

typedef struct __wstring_view_s {
	const wchar *const __data;
	const u64		   __size;
} wstring_view_t;

static_assert(sizeof(union __wstring_u) == __WSTR_SMALL_RAW_CAPACITY * sizeof(wchar) + sizeof(wchar), "union __wstring_u must be 24 bytes long");

/// Create and return a new empty wstring with a length of STRINGDEF_MIN_SIZE
wstring wstr_create();
/// Allocate enough memory for __wstr to be able to fit at least __chars_req characters.
void wstr_alloc(wstring *__wstr, u64 __chars_req);
/// Create and return a new wstring containing __literal, which is copied
wstring wstr_from(const wchar *__literal);
/// Create a copy of __wstr
wstring wstr_dup(const wstring *__wstr);

/// Concatenate __literal to __dest; return the length of __literal
u64 wstr_cat_l(wstring *__dest, const wchar *__literal);
/// Compare __literal to __wstr
int wstr_cmp_l(const wstring *__wstr, const wchar *__literal);
/// Copy __literal to __dest
void wstr_cpy_l(wstring *__dest, const wchar *__literal);
/// Reverse __literal
wchar *wstr_rev_l(wchar *__literal);

/// Concatenate __what to __dest; return the length of __what
u64 wstr_cat(wstring *__dest, const wstring *__what);
/// Compare __other to __dest
int wstr_cmp(const wstring *__dest, const wstring *__other);
/// Copy __src to __dest
void wstr_cpy(wstring *__dest, const wstring *__src);
/// Copy __len chars from __literal to __dest
void wstrn_cpy(wstring *__dest, const wchar *__literal, u64 __len);
/// Reverse __wstr
wstring *wstr_rev(wstring *__wstr);

/// Calculate and return the length of __wstr (O(1))
u64 wstr_len(const wstring *__wstr);
/// Get a pointer to the contents of the wstring
wchar *wstr_data(wstring *__wstr);

/// Remove all data from __wstr
void wstr_clear(wstring *__wstr);
/// Delete __wstr
void wstr_free(wstring *__wstring);

#endif /*C_LIB_WSTRINGS_H*/

#ifdef C_LIB_WSTRINGS_IMPLEMENTATION
#	include "utils.h"

#	include <immintrin.h>
#	include <stdlib.h>
#	include <wchar.h>
#	include <unistd.h>
#	include <endian.h>

#	if defined(__BYTE_ORDER)
#		if __BYTE_ORDER == __LITTLE_ENDIAN && !defined(LITTLE_ENDIAN)
#			define LITTLE_ENDIAN
#		elif __BYTE_ORDER == __BIG_ENDIAN && !defined(BIG_ENDIAN)
#			define BIG_ENDIAN
#		endif
#	endif

#	if !defined(BIG_ENDIAN) && !defined(LITTLE_ENDIAN) || defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN)
#		error "Please define either BIG_ENDIAN or LITTLE_ENDIAN before including this header."
#		include "/Abort Compilation: See Error Above"
#	endif

enum {
	_WSTR_REALLOC_FACTOR = 2,
};

#	undef wstr
#	undef wstr_len
#	undef wstr_cpy
#	undef wstr_cat
#	undef wstr_cmp
#	undef wstr_rev

#	ifndef wstring
#		define wstring wstring
#	endif

/// ╔═══════════════════════════╦═════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  INTERNALS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═════════════╩═══════════════════════════╝

/// Check if a wstring is a small wstring
static u1 __wstr_is_small(const wstring *__wstr) {
#	if defined(LITTLE_ENDIAN)
	return !(__wstr->__raw[__WSTR_SMALL_RAW_CAPACITY] & (1 << (sizeof(wchar) * 8 - 1)));
#	elif defined(BIG_ENDIAN)
	return !(__wstr->__raw[__WSTR_SMALL_RAW_CAPACITY] & 1);
#	endif
}

/// The SSO flag is the last bit of the wstring.
/// Depending on the endianness of the architecture,
/// it must be tested and set in different ways.

void __wstr_setflag_long(wstring *__wstr) {
#	if defined(LITTLE_ENDIAN)
	__wstr->__raw[__WSTR_SMALL_RAW_CAPACITY] |= (1 << (sizeof(wchar) * 8 - 1));
#	elif defined(BIG_ENDIAN)
	__wstr->__raw[__WSTR_SMALL_RAW_CAPACITY] |= 1;
#	endif
}

void __wstr_setflag_small(wstring *__wstr) {
#	if defined(LITTLE_ENDIAN)
	__wstr->__raw[__WSTR_SMALL_RAW_CAPACITY] &= ~(1 << (sizeof(wchar) * 8 - 1));
#	elif defined(BIG_ENDIAN)
	__wstr->__raw[__WSTR_SMALL_RAW_CAPACITY] &= ~1;
#	endif
}

static u64 __wstr_small_capacity(const wstring *__wstr) {
#	if defined(LITTLE_ENDIAN)
	return __wstr->__small_capacity & ~(1 << (sizeof(wchar) * 8 - 1));
#	elif defined(BIG_ENDIAN)
	return __wstr->__small_capacity >> 1;
#	endif
}

/// Get the small size of __wstr
static u8 __wstr_small_size(const wstring *__wstr) {
	return __WSTR_SMALL_RAW_CAPACITY - __wstr_small_capacity(__wstr);
}

/// Get the small size of __wstr
static u64 __wstr_long_size(const wstring *__wstr) {
#	if defined(LITTLE_ENDIAN)
	return __wstr->__long_size & ~(1lu << 63lu);
#	elif defined(BIG_ENDIAN)
	return __wstr->__long_size >> 1;
#	endif
}

static u64 __wcharcount_to_size(u64 __number_of_wchars) {
#	if defined(LITTLE_ENDIAN)
	return __number_of_wchars;
#	elif defined(BIG_ENDIAN)
	return __number_of_wchars << 1;
#	endif
}
//
// static u64 __wstr_small_size_add(u64 __number_of_wchars) {
//#	if defined(LITTLE_ENDIAN)
//	return __number_of_wchars << 1;
//#	elif defined(BIG_ENDIAN)
//	return __number_of_wchars;
//#	endif
//}

/// Concatenates __srclen elements from __raw_src to __data
static void wstrn_cat(
	wstring		*__dest,  /// The destination wstring
	const wchar *__data,  /// The data to be appended
	u64			 __srclen /// The size of the data in elements
) {
	wstr_alloc(__dest, __srclen + 1); /// + 1 for the null terminator
	wchar *__mem_start = wstr_data(__dest);
	__mem_start += wstr_len(__dest);

	memcpy(__mem_start, __data, __srclen * sizeof(wchar));

	u64 __size = __wcharcount_to_size(__srclen);
	if (__wstr_is_small(__dest)) __dest->__small_capacity -= __size;
	else __dest->__long_size += __size;

	__mem_start[__srclen] = 0;
}

/// Compares __wstr and __data, where the latter is of length __len
static int wstrn_cmp(
	const wstring *__wstr, /// The wstring to be compared
	const wchar	*__data, /// The data with which to compare
	u64			   __len   /// The size of said data in elements
) {
	u64 __size = wstr_len(__wstr);
	if (__size == __len) return memcmp(wstr_data((wstring *) __wstr), __data, __size);

	int __res = memcmp(wstr_data((wstring *) __wstr), __data, min(__size, __len) * sizeof(wchar));
	if (!__res) return __size < __len ? -1 : 1;
	return __res;
}

/// Reverses __data in place
static wchar *__wstring_rev_impl(
	wchar *__data, /// data to be reversed
	u64	   __len   /// size of the data
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

/// Create and return a new empty wstring
wstring wstr_create() {
	wstring __wstr			= {0};
	__wstr.__small_capacity = __WSTR_SMALL_MAX_CAPACITY;
	__wstr_setflag_small(&__wstr);
	return __wstr;
}

/// Allocate enough memory for __wstr to be able to fit at least __chars_req characters.
void wstr_alloc(
	wstring *__wstr /* wstring to be resized */,
	u64		 __chars_req /* minimum memory to be allocated */) {
	if (__chars_req == 0) return;

	/// Data stored on the stack, we might have to copy it
	if (__wstr_is_small(__wstr)) {
		/// We need more memory than the SSO buffer can hold
		if (__chars_req > __wstr_small_capacity(__wstr) + 1) {
			u64 __wstr_size	   = __wstr_small_size(__wstr);
			u64 __new_mem_size = __chars_req + __wstr_size;

			/// We cannot yet store this in the wstring because it would overlap
			/// with the data stored in the SSO buffer
			u64	   __heap_capacity = __new_mem_size * _WSTR_REALLOC_FACTOR;
			wchar *__mem		   = malloc(__heap_capacity * sizeof(wchar));

			/// Copy the data from the SSO buffer to the heap if not empty
			if (*__wstr->__small_data) memcpy(__mem, __wstr->__small_data, __wstr_size * sizeof(wchar));

			/// Set up the struct properly
			__wstr->__long_data = __mem;
			__wstr->__long_size = __wcharcount_to_size(__wstr_size);
			__wstr_setflag_long(__wstr);
			__wstr->__long_capacity = __heap_capacity;
		}

		/// __chars_req fits in SSO buffer; do nothing
		else {}
	}

	/// Data stored on the heap, we need to realloc
	else {
		u64 __wstr_size = __wstr_long_size(__wstr);
		if (__chars_req > __wstr->__long_capacity - __wstr_size) {
			wchar *__old_data		= __wstr->__long_data;
			__wstr->__long_capacity = (__chars_req + __wstr_size) * _WSTR_REALLOC_FACTOR;
			__wstr->__long_data		= realloc(__wstr->__long_data, __wstr->__long_capacity * sizeof(wchar));
			if (__old_data != __wstr->__long_data) free(__old_data);
		}

		/// __chars_req fits in heap buffer; do nothing
		else {}
	}
}

/// Create and return a new wstring containing __literal, which is copied
wstring wstr_from(const wchar *__literal) {
	wstring __s = wstr_create();
	wstr_cpy_l(&__s, __literal);
	return __s;
}

/// Create a copy of __wstr
wstring wstr_dup(const wstring *__wstr) {
	wstring __s = wstr_create();
	wstr_cpy(&__s, __wstr);
	return __s;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  LIT. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Concatenate __literal to __dest; return the length of __literal
u64 wstr_cat_l(wstring *__dest, const wchar *__literal) {
	u64 __l = wcslen(__literal);
	wstrn_cat(__dest, __literal, __l);
	return __l;
}

/// Compare __literal to __wstr
int wstr_cmp_l(const wstring *__wstr, const wchar *__literal) {
	return wstrn_cmp(__wstr, __literal, wcslen(__literal));
}

/// Copy __literal to __dest
void wstr_cpy_l(wstring *__dest, const wchar *__literal) {
	wstr_clear(__dest);
	wstr_cat_l(__dest, __literal);
}

wchar *wstr_rev_l(wchar *__literal) {
	return __wstring_rev_impl(__literal, wcslen(__literal));
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  WSTR. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Concatenate __what to __dest; return the length of __what
u64 wstr_cat(wstring *__dest, const wstring *__what) {
	u64 __l = wstr_len(__what);
	wstrn_cat(__dest, wstr_data((wstring *) __what), __l);
	return __l;
}

/// Compare __other to __dest
int wstr_cmp(const wstring *__dest, const wstring *__other) {
	return wstrn_cmp(__dest, wstr_data((wstring *) __other), wstr_len(__other));
}

/// Copy __src to __dest
void wstr_cpy(wstring *__dest, const wstring *__src) {
	wstr_clear(__dest);
	wstr_cat(__dest, __src);
}

void wstrn_cpy(wstring *__dest, const wchar *__literal, u64 __len) {
	wstr_clear(__dest);
	wstrn_cat(__dest, __literal, __len);
}

wstring *wstr_rev(wstring *__wstr) {
	__wstring_rev_impl(wstr_data(__wstr), wstr_len(__wstr));
	return __wstr;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  MEM. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Calculate and return the length of __wstr (O(1))
u64 wstr_len(const wstring *__wstr) {
	return __wstr_is_small(__wstr)
			   ? __wstr_small_size(__wstr)
			   : __wstr_long_size(__wstr);
}

wchar *wstr_data(wstring *__wstr) {
	return __wstr_is_small(__wstr)
			   ? __wstr->__small_data
			   : __wstr->__long_data;
}

void wstr_clear(wstring *__wstr) {
	wchar *__wstr_data = wstr_data(__wstr);
	if (!*__wstr_data) return;

	if (__wstr_is_small(__wstr)) {
		__wstr->__small_capacity = __WSTR_SMALL_MAX_CAPACITY;
		__wstr_setflag_small(__wstr);
	} else {
		__wstr->__long_size = 0;
		__wstr_setflag_long(__wstr);
	}

	*__wstr_data = 0;
}

void wstr_free(wstring *__wstr) {
	if (__wstr_is_small(__wstr)) return;
	free(__wstr->__long_data);
}
#endif

#ifndef C_LIB_WSTRINGS_H
#	define C_LIB_WSTRINGS_H
#	if __STDC_VERSION__ >= 201112L /// 201112L == C11
///  '// clang-format off' doesn't work with '///'!
// clang-format off
#		define wstr(...) _Generic((_GENERIC_TYPE_OR_NONE(__VA_ARGS__)),	\
				wstring * : wstr_dup,							\
				wchar *     : wstr_from,                                         \
				const wchar *     : wstr_from, 						\
				struct __generic_no_arg     : wstr_create									\
			)(__VA_ARGS__)

#		define wstr_len(__wstring_like) _Generic((__wstring_like), \
				wstring * : wstr_len,   				\
				const wchar *     : wcslen,                 \
				wchar *     : wcslen                 \
			)(__wstring_like)

#		define __wstr_cpy_wrapper_F(_type) _type : wstrn_cpy,
#		define wstr_cpy_wrapper(...) _Generic((__VA_ARGS__), \
				_INT_TYPES(__wstr_cpy_wrapper_F)                                \
				struct __generic_no_arg : wstr_cpy_l)
#		define wstr_cpy(__wstring, __wstring_like, ...) _Generic((__wstring_like),	\
				wstring * : wstr_cpy, 	                \
				const wchar *     : wstr_cpy_wrapper(_GENERIC_TYPE_OR_NONE(__VA_ARGS__)),                                            \
				wchar *     : wstr_cpy_wrapper(_GENERIC_TYPE_OR_NONE(__VA_ARGS__))                                            \
			)(__wstring, __wstring_like __VA_OPT__(,) __VA_ARGS__)

#		define __wstr_cat_l_wrapper_F(_type) _type : wstrn_cat,
#		define wstr_cat_l_wrapper(...) _Generic((__VA_ARGS__), \
				_INT_TYPES(__wstr_cat_l_wrapper_F)                                \
				struct __generic_no_arg : wstr_cat_l)
#		define wstr_cat(__wstring, __wstring_like, ...) _Generic((__wstring_like),	\
				wstring * : wstr_cat,     	                                    \
				const wchar *     : wstr_cat_l_wrapper(_GENERIC_TYPE_OR_NONE(__VA_ARGS__)), 											\
				wchar *     : wstr_cat_l_wrapper(_GENERIC_TYPE_OR_NONE(__VA_ARGS__)) 											\
			)(__wstring, __wstring_like __VA_OPT__(,) __VA_ARGS__)

#		define wstr_cmp(__wstring, __wstring_like) _Generic((__wstring_like),	\
				wstring * : wstr_cmp,         	        \
				const wchar *     : wstr_cmp_l, 				\
				wchar *     : wstr_cmp_l 				\
			)(__wstring, __wstring_like)

#		define wstr_rev(__wstring_like) _Generic((__wstring_like), \
				wstring* : wstr_rev									\
				wchar *    : wstr_rev_l 								\
			)(__wstring_like)
// clang-format on
#	endif

#	ifndef C_LIB_WSTRINGS_NO_SHORTHANDS
#		define wdata  wstr_data
#		define wlen   wstr_len
#		define wcpy   wstr_cpy
#		define wcat   wstr_cat
#		define wclear wstr_clear
#	endif
#endif
