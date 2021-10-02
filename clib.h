#ifndef CLIB
/// Header guard will be #define’d later on in the file

/// TODO: Test with C90, C99
/// TODO: Test with a c++ compiler
/// TODO: Test on Windows
/// TODO: Test on a big-endian machine
/// TODO: Better commenting and structuring
/// TODO: _Generic’s for lists and arrays

#	ifdef __cplusplus
extern "C" {
#	endif

#	include <assert.h>
#	include <stdarg.h>
#	include <stddef.h>
#	include <stdio.h>
#	include <endian.h>
#	include <stdint.h>

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

#	ifndef string
#		define string string
#	endif

#	ifndef wstring
#		define wstring wstring
#	endif

#define CLIB_EMPTY_STRING _clib_empty_string
#define CLIB_EMPTY_WSTRING _clib_empty_wstring

#	define CLIB_CAR(X, ...)			   X
#	define CLIB_GENERIC_TYPE_OR_NONE(...) CLIB_CAR(__VA_ARGS__ __VA_OPT__(, ) clib_generic_no_arg)
#	define repeat(_var_name, _until)	   for (size_t _var_name = 0, _until_val = _until; _var_name < _until_val; _var_name++)
#	define arrsize(_array)				   (sizeof _array / sizeof *_array)

#	define MAX(X, Y) (X > Y ? X : Y)
#	define MIN(X, Y) (X < Y ? X : Y)

#	define CLIB_INT_TYPES_(F, ...) \
		F(__VA_ARGS__ i8)           \
		F(__VA_ARGS__ i16)          \
		F(__VA_ARGS__ i32)          \
		F(__VA_ARGS__ i64)          \
		F(__VA_ARGS__ u8)           \
		F(__VA_ARGS__ u16)          \
		F(__VA_ARGS__ u32)          \
		F(__VA_ARGS__ u64)

#	define CLIB_INT_TYPES(F)        \
		CLIB_INT_TYPES_(F)           \
		CLIB_INT_TYPES_(F, const)    \
		CLIB_INT_TYPES_(F, volatile) \
		CLIB_INT_TYPES_(F, const volatile)

#	define array_create(type, number_of_elements) array_create_impl(sizeof(type), number_of_elements)

#	define array_append(type, array, element)       \
		do {                                         \
			type _el = element;                      \
			array_append_impl(array, (void *) &_el); \
		} while (0)

#	define list_foreach(list, callback, ...)                               \
		do {                                                                \
			node_t *_macrovar_node = list->head;                            \
			while (_macrovar_node) {                                        \
				callback(_macrovar_node->value __VA_OPT__(, ) __VA_ARGS__); \
				_macrovar_node = _macrovar_node->next;                      \
			}                                                               \
		} while (0)

#	define array_foreach_cb(array, callback, ...)                                                             \
		do {                                                                                                   \
			for (size_t _macrovar_i = 0; _macrovar_i < array->size; _macrovar_i++)                             \
				callback((char *) array->data + _macrovar_i * array->size_of_type __VA_OPT__(, ) __VA_ARGS__); \
		} while (0)

#	define array_foreach(type, _var_name, array) for (type *_var_name = array.data; _var_name < (type *) array.data + array.size; _var_name++)

typedef int8_t	i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t	 u1;
typedef uint8_t	 u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i32 wchar;

typedef float  f32;
typedef double f64;

/// These are too big for an enum if the machine is little-endian
#	if defined(LITTLE_ENDIAN)
#		define CLIB_WSTR_LONG_FLAG (1lu << 63lu)
#		define CLIB_STR_LONG_FLAG	(1lu << 63lu)
#	endif

/// In practice, these are gonna be 23 and 46 (string), or 5 and 10 (wstring)
enum {
	CLIB_WSTR_SMALL_RAW_CAPACITY = ((sizeof(wchar *) + sizeof(u64) * 2) / sizeof(wchar) - 1),
	CLIB_STR_SMALL_RAW_CAPACITY	 = (sizeof(char *) + sizeof(u64) * 2 - 1),

#	if defined(LITTLE_ENDIAN)
	CLIB_WSTR_SMALL_MAX_CAPACITY = CLIB_WSTR_SMALL_RAW_CAPACITY,
	CLIB_WSTR_SMALL_FLAG		 = (1 << (sizeof(wchar) * 8 - 1)),
#	elif defined(BIG_ENDIAN)
	CLIB_WSTR_SMALL_MAX_CAPACITY = (CLIB_WSTR_SMALL_RAW_CAPACITY << 1),
	CLIB_WSTR_SMALL_FLAG		 = 1,
	CLIB_WSTR_LONG_FLAG			 = 1,
#	endif

#	if defined(LITTLE_ENDIAN)
	CLIB_STR_SMALL_MAX_CAPACITY = CLIB_STR_SMALL_RAW_CAPACITY,
	CLIB_STR_SMALL_FLAG			= (1 << 7),
#	elif defined(BIG_ENDIAN)
	CLIB_STR_SMALL_MAX_CAPACITY	 = (CLIB_STR_SMALL_RAW_CAPACITY << 1),
	CLIB_STR_SMALL_FLAG			 = 1,
	CLIB_STR_LONG_FLAG			 = 1,
#	endif
};

typedef union _string_u {
	char _raw[CLIB_STR_SMALL_RAW_CAPACITY + 1];

	struct {
		char _small_data[CLIB_STR_SMALL_RAW_CAPACITY];

		/// On a little-endian machine, (_small_capacity & _STR_SMALL_CAP_CONST) contains the
		/// free capacity of the string. Its MSB shall be unset if the string
		/// is small, and set otherwise. Accessing _small_capacity is valid
		/// iff its MSB is unset. <p>

		/// On a big-endian machine, (_small_capacity >> 1) contains the
		/// free capacity of the string. Its LSB shall be unset if the string
		/// is small, and set otherwise. Accessing _small_capacity is valid
		/// iff its LSB is unset. <p>

		/// When the string is full, _small_capacity is 0 and acts as the null terminator.
		char _small_capacity;
	};

	struct {
		char *_long_data;
		u64	  _long_capacity;

		/// On a little-endian machine, (_long_size & _STR_LONG_CAP_CONST) contains
		/// the size of the string. Its MSB shall be unset if the string
		/// is small, and set otherwise. Accessing _long_size is valid
		/// iff its MSB is set. <p>

		/// On a big-endian machine, (_long_size >> 1) contains the
		/// size of the string. Its LSB shall be unset if the string is
		/// small, and set otherwise. Accessing _long_size is valid
		/// iff its LSB is set <p>
		u64 _long_size;
	};
} string;

typedef union _wstring_u {
	wchar _raw[CLIB_WSTR_SMALL_RAW_CAPACITY + 1];

	struct {
		wchar _small_data[CLIB_WSTR_SMALL_RAW_CAPACITY];

		/// On a little-endian machine, (_small_capacity & _WSTR_SMALL_CAP_CONST) contains the
		/// free capacity of the wstring. Its MSB shall be unset if the wstring
		/// is small, and set otherwise. Accessing _small_capacity is valid
		/// iff its MSB is unset. <p>

		/// On a big-endian machine, (_small_capacity >> 1) contains the
		/// free capacity of the wstring. Its LSB shall be unset if the wstring
		/// is small, and set otherwise. Accessing _small_capacity is valid
		/// iff its LSB is unset. <p>

		/// When the wstring is full, _small_capacity is 0 and acts as the null terminator.
		wchar _small_capacity;
	};

	struct {
		wchar *_long_data;
		u64	   _long_capacity;

		/// On a little-endian machine, (_long_size & _WSTR_LONG_CAP_CONST) contains
		/// the size of the wstring. Its MSB shall be unset if the wstring
		/// is small, and set otherwise. Accessing _long_size is valid
		/// iff its MSB is set. <p>

		/// On a big-endian machine, (_long_size >> 1) contains the
		/// size of the wstring. Its LSB shall be unset if the wstring is
		/// small, and set otherwise. Accessing _long_size is valid
		/// iff its LSB is set <p>
		u64 _long_size;
	};
} wstring;

typedef struct _string_view_s {
	const char *const _data;
	const u64		  _size;
} string_view_t;

typedef struct _wstring_view_s {
	const wchar *const _data;
	const u64		   _size;
} wstring_view_t;

typedef struct node {
	void		 *value;
	struct node *next;
} node_t;

typedef struct list {
	struct node	*head;
	struct node **end;
	size_t		  size;
} list_t;

typedef struct array {
	void	 *data;
	size_t size_of_type;
	size_t allocated;
	size_t size;
} array_t;

/// Dummy type that represents no argument
struct clib_generic_no_arg {
	char _unused_;
};

static_assert(sizeof(union _string_u) == CLIB_STR_SMALL_RAW_CAPACITY + 1, "union _string_u must be 24 bytes long");
static_assert(sizeof(union _wstring_u) == CLIB_WSTR_SMALL_RAW_CAPACITY * sizeof(wchar) + sizeof(wchar), "union _wstring_u must be 24 bytes long");

extern struct clib_generic_no_arg clib_generic_no_arg;
extern const union _string_u _clib_empty_string;
extern const union _wstring_u _clib_empty_wstring;

u64	 min(u64 _a, u64 _b);
u64	 max(u64 _a, u64 _b);
u64	 between(u64 _what, u64 _a, u64 _b);
void die(const char *_fmt, ...);

/// Create and return a new empty string with a length of STRINGDEF_MIN_SIZE
string str_create();
/// Allocate enough memory for _str to be able to fit at least _mem_req characters.
void str_alloc(string *_str, u64 _mem_req);
/// Create and return a new string containing _data, which is copied
string str_from(const char *_data);
/// Create a copy of _str
string str_dup(const string *_str);

/// Concatenate _data to _dest; return the length of _data
u64 str_cat_l(string *_dest, const char *_data);
/// Copy _data to _dest
void str_cpy_l(string *_dest, const char *_data);
/// Compare _data to _str
int str_cmp_l(const string *_str, const char *_data);

/// Reverse _data in place
char *str_rev_l(char *_data);

/// Concatenate _what to _dest; return the length of _what
u64 str_cat(string *_dest, const string *_what);
/// Copy _src to _dest
void str_cpy(string *_dest, const string *_src);
/// Compare _other to _dest
int str_cmp(const string *_dest, const string *_other);

/// Concatenates _srclen elements from _raw_src to _data
void strn_cat(string *_dest, const char *_data, u64 _srclen);
/// Copy _len chars from _data to _dest
void strn_cpy(string *_dest, const char *_data, u64 _len);
/// Compares _str and _data, where the latter is of length _len
int strn_cmp(const string *_str, const char *_data, u64 _len);

/// Reverse _str in place
string *str_rev(string *_str);

/// Calculate and return the length of _str (O(1))
u64 str_len(const string *_str);
/// Get a pointer to the contents of the string
char *str_data(string *_str);

/// Remove all data from _str
void str_clear(string *_str);
/// Delete _str
void str_free(string *_string);

/// Map the contents of _file into memory and return it as a string
string str_map(const char *_filename);

/// ======================== wstring ===============================

/// Create and return a new empty wstring with a length of STRINGDEF_MIN_SIZE
wstring wstr_create();
/// Allocate enough memory for _wstr to be able to fit at least _chars_req characters.
void wstr_alloc(wstring *_wstr, u64 _chars_req);
/// Create and return a new wstring containing _data, which is copied
wstring wstr_from(const wchar *_data);
/// Create a copy of _wstr
wstring wstr_dup(const wstring *_wstr);
/// Map the contents of _file into memory and return it as a wstring
wstring wstr_map(const char *_filename);

/// Concatenate _data to _dest; return the length of _data
u64 wstr_cat_l(wstring *_dest, const wchar *_data);
/// Copy _data to _dest
void wstr_cpy_l(wstring *_dest, const wchar *_data);
/// Compare _data to _wstr
int wstr_cmp_l(const wstring *_wstr, const wchar *_data);

/// Reverse _data in place
wchar *wstr_rev_l(wchar *_data);

/// Concatenate _what to _dest; return the length of _what
u64 wstr_cat(wstring *_dest, const wstring *_what);
/// Copy _src to _dest
void wstr_cpy(wstring *_dest, const wstring *_src);
/// Compare _other to _dest
int wstr_cmp(const wstring *_dest, const wstring *_other);

/// Concatenates _srclen elements from _raw_src to _data
void wstrn_cat(wstring *_dest, const wchar *_data, u64 _srclen);
/// Copy _len chars from _data to _dest
void wstrn_cpy(wstring *_dest, const wchar *_data, u64 _len);
/// Compares _wstr and _data, where the latter is of length _len
int wstrn_cmp(const wstring *_wstr, const wchar *_data, u64 _len);

/// Reverse _wstr in place
wstring *wstr_rev(wstring *_wstr);

/// Calculate and return the length of _wstr (O(1))
u64 wstr_len(const wstring *_wstr);
/// Get a pointer to the contents of the wstring
wchar *wstr_data(wstring *_wstr);

/// Remove all data from _wstr
void wstr_clear(wstring *_wstr);
/// Delete _wstr
void wstr_free(wstring *_wstr);

list_t list_create();
void   list_append(list_t *list, void *element);
void   list_append_n(list_t *list, ...);
void	 *list_remove(list_t *list, void *element);
void   list_remove_n(list_t *list, ...);
void   list_remove_list(list_t *list, list_t *elements);
void	 *list_get_nth_element(list_t *list, size_t index);
void   list_clear(list_t *list);

array_t array_create_impl(size_t size_of_type, size_t reserved);
void	array_init(array_t *where, size_t size_of_type, size_t reserved);
void	array_append_impl(array_t *array, void *element);
void	array_unsorted_remove(array_t *array, size_t index);
void	array_from(array_t *array, void *static_array, size_t element_size, size_t elements);
void	array_free(array_t *array, void (*callback)(void *));

#	ifdef __cplusplus
}
#	endif

#endif /*CLIB*/

#ifdef CLIB_IMPLEMENTATION

#	ifdef __cplusplus
extern "C" {
#	endif

#	include <stdlib.h>
#	include <string.h>
#	include <unistd.h>
#	include <endian.h>
#	include <wchar.h>
#	include <sys/mman.h>
#	include <sys/stat.h>
#	include <fcntl.h>

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

#	undef str
#	undef str_len
#	undef str_cpy
#	undef str_cat
#	undef str_cmp
#	undef str_rev

#	undef wstr
#	undef wstr_len
#	undef wstr_cpy
#	undef wstr_cat
#	undef wstr_cmp
#	undef wstr_rev

#	ifndef string
#		define string string
#	endif

#	ifndef wstring
#		define wstring wstring
#	endif

enum {
	STR_REALLOC_FACTOR	= 2,
	WSTR_REALLOC_FACTOR = 2,
};

struct clib_generic_no_arg clib_generic_no_arg;
const union _string_u _clib_empty_string = {0};
const union _wstring_u _clib_empty_wstring = {0};

u64 min(u64 _a, u64 _b) {
	return _a < _b ? _a : _b;
}

u64 max(u64 _a, u64 _b) {
	return _a > _b ? _a : _b;
}

u64 between(u64 _what, u64 _a, u64 _b) {
	return _a <= _what && _what <= _b;
}

__attribute__((noreturn))
__attribute__((format(printf, 1, 2))) //
void die(const char *_fmt, ...) {
	va_list _ap;
	va_start(_ap, _fmt);
	fprintf(stderr, "\033[31m");
	vfprintf(stderr, _fmt, _ap);
	fprintf(stderr, "\033[0m");
	va_end(_ap);
	exit(1);
}

/// ╔═══════════════════════════╦═════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  INTERNALS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═════════════╩═══════════════════════════╝

/// Check if a string is a small string
static inline u1 _str_is_small(const string *_str) {
#	if defined(LITTLE_ENDIAN)
	return !(_str->_raw[CLIB_STR_SMALL_RAW_CAPACITY] & (1 << 7));
#	elif defined(BIG_ENDIAN)
	return !(_str->_raw[CLIB_STR_SMALL_RAW_CAPACITY] & 1);
#	endif
}

/// The SSO flag is the last bit of the string.
/// Depending on the endianness of the architecture,
/// it must be tested and set in different ways.

static inline void _str_setflag_long(string *_str) {
#	if defined(LITTLE_ENDIAN)
	_str->_raw[CLIB_STR_SMALL_RAW_CAPACITY] |= (1 << 7);
#	elif defined(BIG_ENDIAN)
	_str->_raw[CLIB_STR_SMALL_RAW_CAPACITY] |= 1;
#	endif
}

static inline void _str_setflag_small(string *_str) {
#	if defined(LITTLE_ENDIAN)
	_str->_raw[CLIB_STR_SMALL_RAW_CAPACITY] &= ~(1 << 7);
#	elif defined(BIG_ENDIAN)
	_str->_raw[CLIB_STR_SMALL_RAW_CAPACITY] &= ~1;
#	endif
}

static inline u64 _str_small_capacity(const string *_str) {
#	if defined(LITTLE_ENDIAN)
	return _str->_small_capacity & ~(1 << 7);
#	elif defined(BIG_ENDIAN)
	return _str->_small_capacity >> 1;
#	endif
}

/// Get the small size of _str
static inline u8 _str_small_size(const string *_str) {
	return CLIB_STR_SMALL_RAW_CAPACITY - _str_small_capacity(_str);
}

/// Get the small size of _str
static inline u64 _str_long_size(const string *_str) {
#	if defined(LITTLE_ENDIAN)
	return _str->_long_size & ~(1lu << 63lu);
#	elif defined(BIG_ENDIAN)
	return _str->_long_size >> 1;
#	endif
}

static inline u64 _charcount_to_size(u64 _number_of_chars) {
#	if defined(LITTLE_ENDIAN)
	return _number_of_chars;
#	elif defined(BIG_ENDIAN)
	return _number_of_chars << 1;
#	endif
}

/// Concatenates _srclen elements from _raw_src to _data
void strn_cat(
	string	   *_dest,	/// The destination string
	const char *_data,	/// The data to be appended
	u64			_srclen /// The size of the data in elements
) {
	str_alloc(_dest, _srclen + 1); /// + 1 for the null terminator
	char *_mem_start = str_data(_dest);
	_mem_start += str_len(_dest);

	memcpy(_mem_start, _data, _srclen);

	u64 _size = _charcount_to_size(_srclen);
	if (_str_is_small(_dest)) _dest->_small_capacity -= _size;
	else _dest->_long_size += _size;

	_mem_start[_srclen] = 0;
}

/// Compares _str and _data, where the latter is of length _len
int strn_cmp(
	const string *_str,	 /// The string to be compared
	const char   *_data, /// The data with which to compare
	u64			  _len	 /// The size of said data in elements
) {
	u64 _size = str_len(_str);
	if (_size == _len) return memcmp(str_data((string *) _str), _data, _size);

	int _res = memcmp(str_data((string *) _str), _data, min(_size, _len));
	if (!_res) return _size < _len ? -1 : 1;
	return _res;
}

/// Reverses _data in place
static char *_string_rev_impl(
	char *_data, /// data to be reversed
	u64	  _len	 /// size of the data
) {
	if (_data) {
		_data[_len] = 0;

		for (u64 i = 0; i < _len / 2 + _len % 2; i++) {
			_data[i]			= _data[_len - 1 - i];
			_data[_len - 1 - i] = _data[i];
		}
	}
	return _data;
}

/// ╔═══════════════════════════╦══════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  ALLOCATION  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩══════════════╩═══════════════════════════╝

/// Create and return a new empty string
string str_create() {
	string _str			 = {0};
	_str._small_capacity = CLIB_STR_SMALL_MAX_CAPACITY;
	_str_setflag_small(&_str);
	return _str;
}

/// Allocate enough memory for _str to be able to fit at least _mem_req characters.
void str_alloc(
	string *_str /* string to be resized */,
	u64		_mem_req /* minimum memory to be allocated */) {
	if (_mem_req == 0) return;

	/// Data stored on the stack, we might have to copy it
	if (_str_is_small(_str)) {
		/// We need more memory than the SSO buffer can hold
		if (_mem_req > _str_small_capacity(_str) + 1) {
			u64 _str_size	  = _str_small_size(_str);
			u64 _new_mem_size = _mem_req + _str_size;

			/// We cannot yet store this in the string because it would overlap
			/// with the data stored in the SSO buffer
			u64	  _heap_capacity = _new_mem_size * STR_REALLOC_FACTOR;
			char *_mem			 = malloc(_heap_capacity);

			/// Copy the data from the SSO buffer to the heap if not empty
			if (*_str->_small_data) memcpy(_mem, _str->_small_data, _str_size);

			/// Set up the struct properly
			_str->_long_data = _mem;
			_str->_long_size = _charcount_to_size(_str_size);
			_str_setflag_long(_str);
			_str->_long_capacity = _heap_capacity;
		}

		/// _mem_req fits in SSO buffer; do nothing
		else {}
	}

	/// Data stored on the heap, we need to realloc
	else {
		u64 _str_size = _str_long_size(_str);
		if (_mem_req > _str->_long_capacity - _str_size) {
			char *_old_data		 = _str->_long_data;
			_str->_long_capacity = (_mem_req + _str_size) * STR_REALLOC_FACTOR;
			_str->_long_data	 = realloc(_str->_long_data, _str->_long_capacity);
			if (_old_data != _str->_long_data) free(_old_data);
		}

		/// _mem_req fits in heap buffer; do nothing
		else {}
	}
}

/// Create and return a new string containing _data, which is copied
string str_from(const char *_data) {
	string _s = str_create();
	str_cpy_l(&_s, _data);
	return _s;
}

/// Create a copy of _str
string str_dup(const string *_str) {
	string _s = str_create();
	str_cpy(&_s, _str);
	return _s;
}

/// Map the contents of the file _filename into memory and return it as a string
string str_map(const char *_filename) {
	int _fd = open(_filename, O_RDONLY);
	if (_fd < 0) return (string){._raw = {0}};

	struct stat _s;
	fstat(_fd, &_s);
	char *_mem = mmap(NULL, _s.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, _fd, 0);

	string _str			= str_create();
	_str._long_capacity = _s.st_size + 1;
	_str._long_data		= malloc(_str._long_capacity);
	memcpy(_str._long_data, _mem, _s.st_size);
	_str._long_data[_s.st_size] = 0;
	munmap(_mem, _s.st_size);
	close(_fd);

	_str._long_size = _s.st_size;
	_str_setflag_long(&_str);

	return _str;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  LIT. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Concatenate _data to _dest; return the length of _data
u64 str_cat_l(string *_dest, const char *_data) {
	u64 _l = strlen(_data);
	strn_cat(_dest, _data, _l);
	return _l;
}

/// Compare _data to _str
int str_cmp_l(const string *_str, const char *_data) {
	return strn_cmp(_str, _data, strlen(_data));
}

/// Copy _data to _dest
void str_cpy_l(string *_dest, const char *_data) {
	str_clear(_dest);
	str_cat_l(_dest, _data);
}

char *str_rev_l(char *_data) {
	return _string_rev_impl(_data, strlen(_data));
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  STR. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Concatenate _what to _dest; return the length of _what
u64 str_cat(string *_dest, const string *_what) {
	u64 _l = str_len(_what);
	strn_cat(_dest, str_data((string *) _what), _l);
	return _l;
}

/// Compare _other to _dest
int str_cmp(const string *_dest, const string *_other) {
	return strn_cmp(_dest, str_data((string *) _other), str_len(_other));
}

/// Copy _src to _dest
void str_cpy(string *_dest, const string *_src) {
	str_clear(_dest);
	str_cat(_dest, _src);
}

void strn_cpy(string *_dest, const char *_data, u64 _len) {
	str_clear(_dest);
	strn_cat(_dest, _data, _len);
}

string *str_rev(string *_str) {
	_string_rev_impl(str_data(_str), str_len(_str));
	return _str;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  MEM. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Calculate and return the length of _str (O(1))
u64 str_len(const string *_str) {
	return _str_is_small(_str)
			   ? _str_small_size(_str)
			   : _str_long_size(_str);
}

char *str_data(string *_str) {
	return _str_is_small(_str)
			   ? _str->_small_data
			   : _str->_long_data;
}

void str_clear(string *_str) {
	char *_str_data = str_data(_str);
	if (!*_str_data) return;

	if (_str_is_small(_str)) {
		_str->_small_capacity = CLIB_STR_SMALL_MAX_CAPACITY;
		_str_setflag_small(_str);
	} else {
		_str->_long_size = 0;
		_str_setflag_long(_str);
	}

	*_str_data = 0;
}

void str_free(string *_str) {
	if (_str_is_small(_str)) return;
	free(_str->_long_data);
}

/// ╔═══════════════════════════╦═════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  INTERNALS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═════════════╩═══════════════════════════╝

/// Check if a wstring is a small wstring
static inline u1 _wstr_is_small(const wstring *_wstr) {
#	if defined(LITTLE_ENDIAN)
	return !(_wstr->_raw[CLIB_WSTR_SMALL_RAW_CAPACITY] & (1 << (sizeof(wchar) * 8 - 1)));
#	elif defined(BIG_ENDIAN)
	return !(_wstr->_raw[CLIB_WSTR_SMALL_RAW_CAPACITY] & 1);
#	endif
}

/// The SSO flag is the last bit of the wstring.
/// Depending on the endianness of the architecture,
/// it must be tested and set in different ways.

static inline void _wstr_setflag_long(wstring *_wstr) {
#	if defined(LITTLE_ENDIAN)
	_wstr->_raw[CLIB_WSTR_SMALL_RAW_CAPACITY] |= (1 << (sizeof(wchar) * 8 - 1));
#	elif defined(BIG_ENDIAN)
	_wstr->_raw[CLIB_WSTR_SMALL_RAW_CAPACITY] |= 1;
#	endif
}

static inline void _wstr_setflag_small(wstring *_wstr) {
#	if defined(LITTLE_ENDIAN)
	_wstr->_raw[CLIB_WSTR_SMALL_RAW_CAPACITY] &= ~(1 << (sizeof(wchar) * 8 - 1));
#	elif defined(BIG_ENDIAN)
	_wstr->_raw[CLIB_WSTR_SMALL_RAW_CAPACITY] &= ~1;
#	endif
}

static inline u64 _wstr_small_capacity(const wstring *_wstr) {
#	if defined(LITTLE_ENDIAN)
	return _wstr->_small_capacity & ~(1 << (sizeof(wchar) * 8 - 1));
#	elif defined(BIG_ENDIAN)
	return _wstr->_small_capacity >> 1;
#	endif
}

/// Get the small size of _wstr
static inline u8 _wstr_small_size(const wstring *_wstr) {
	return CLIB_WSTR_SMALL_RAW_CAPACITY - _wstr_small_capacity(_wstr);
}

/// Get the small size of _wstr
static inline u64 _wstr_long_size(const wstring *_wstr) {
#	if defined(LITTLE_ENDIAN)
	return _wstr->_long_size & ~(1lu << 63lu);
#	elif defined(BIG_ENDIAN)
	return _wstr->_long_size >> 1;
#	endif
}

static inline u64 _wcharcount_to_size(u64 _number_of_wchars) {
#	if defined(LITTLE_ENDIAN)
	return _number_of_wchars;
#	elif defined(BIG_ENDIAN)
	return _number_of_wchars << 1;
#	endif
}

/// Concatenates _srclen elements from _raw_src to _data
void wstrn_cat(
	wstring		*_dest,	 /// The destination wstring
	const wchar *_data,	 /// The data to be appended
	u64			 _srclen /// The size of the data in elements
) {
	wstr_alloc(_dest, _srclen + 1); /// + 1 for the null terminator
	wchar *_mem_start = wstr_data(_dest);
	_mem_start += wstr_len(_dest);

	memcpy(_mem_start, _data, _srclen * sizeof(wchar));

	u64 _size = _wcharcount_to_size(_srclen);
	if (_wstr_is_small(_dest)) _dest->_small_capacity -= _size;
	else _dest->_long_size += _size;

	_mem_start[_srclen] = 0;
}

/// Compares _wstr and _data, where the latter is of length _len
int wstrn_cmp(
	const wstring *_wstr, /// The wstring to be compared
	const wchar	*_data, /// The data with which to compare
	u64			   _len	  /// The size of said data in elements
) {
	u64 _size = wstr_len(_wstr);
	if (_size == _len) return memcmp(wstr_data((wstring *) _wstr), _data, _size);

	int _res = memcmp(wstr_data((wstring *) _wstr), _data, min(_size, _len) * sizeof(wchar));
	if (!_res) return _size < _len ? -1 : 1;
	return _res;
}

/// Reverses _data in place
static wchar *_wstring_rev_impl(
	wchar *_data, /// data to be reversed
	u64	   _len	  /// size of the data
) {
	if (_data) {
		_data[_len] = 0;

		for (u64 i = 0; i < _len / 2 + _len % 2; i++) {
			_data[i]			= _data[_len - 1 - i];
			_data[_len - 1 - i] = _data[i];
		}
	}
	return _data;
}

/// ╔═══════════════════════════╦══════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  ALLOCATION  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩══════════════╩═══════════════════════════╝

/// Create and return a new empty wstring
wstring wstr_create() {
	wstring _wstr		  = {0};
	_wstr._small_capacity = CLIB_WSTR_SMALL_MAX_CAPACITY;
	_wstr_setflag_small(&_wstr);
	return _wstr;
}

/// Allocate enough memory for _wstr to be able to fit at least _chars_req characters.
void wstr_alloc(
	wstring *_wstr /* wstring to be resized */,
	u64		 _chars_req /* minimum memory to be allocated */) {
	if (_chars_req == 0) return;

	/// Data stored on the stack, we might have to copy it
	if (_wstr_is_small(_wstr)) {
		/// We need more memory than the SSO buffer can hold
		if (_chars_req > _wstr_small_capacity(_wstr) + 1) {
			u64 _wstr_size	  = _wstr_small_size(_wstr);
			u64 _new_mem_size = _chars_req + _wstr_size;

			/// We cannot yet store this in the wstring because it would overlap
			/// with the data stored in the SSO buffer
			u64	   _heap_capacity = _new_mem_size * WSTR_REALLOC_FACTOR;
			wchar *_mem			  = malloc(_heap_capacity * sizeof(wchar));

			/// Copy the data from the SSO buffer to the heap if not empty
			if (*_wstr->_small_data) memcpy(_mem, _wstr->_small_data, _wstr_size * sizeof(wchar));

			/// Set up the struct properly
			_wstr->_long_data = _mem;
			_wstr->_long_size = _wcharcount_to_size(_wstr_size);
			_wstr_setflag_long(_wstr);
			_wstr->_long_capacity = _heap_capacity;
		}

		/// _chars_req fits in SSO buffer; do nothing
		else {}
	}

	/// Data stored on the heap, we need to realloc
	else {
		u64 _wstr_size = _wstr_long_size(_wstr);
		if (_chars_req > _wstr->_long_capacity - _wstr_size) {
			wchar *_old_data	  = _wstr->_long_data;
			_wstr->_long_capacity = (_chars_req + _wstr_size) * WSTR_REALLOC_FACTOR;
			_wstr->_long_data	  = realloc(_wstr->_long_data, _wstr->_long_capacity * sizeof(wchar));
			if (_old_data != _wstr->_long_data) free(_old_data);
		}

		/// _chars_req fits in heap buffer; do nothing
		else {}
	}
}

/// Create and return a new wstring containing _data, which is copied
wstring wstr_from(const wchar *_data) {
	wstring _s = wstr_create();
	wstr_cpy_l(&_s, _data);
	return _s;
}

/// Create a copy of _wstr
wstring wstr_dup(const wstring *_wstr) {
	wstring _s = wstr_create();
	wstr_cpy(&_s, _wstr);
	return _s;
}

/// Map the contents of _filename into memory and return it as a wstring
wstring wstr_map(const char *_filename) {
	int _fd = open(_filename, O_RDONLY);
	if (_fd < 0) return (wstring){._raw = {0}};

	struct stat _s;
	fstat(_fd, &_s);
	char *_mem = mmap(NULL, _s.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, _fd, 0);

	const char *_mem_ptr = _mem;
	mbstate_t	_state	 = {0};
	wstring		_ws;
	u64			_chars_available = (_s.st_size + 1) * 4;
	_ws._long_capacity			 = _chars_available * sizeof(wchar);
	_ws._long_data				 = malloc(_ws._long_capacity);
	u64 conv					 = mbsrtowcs(_ws._long_data, &_mem_ptr, _chars_available, &_state);

	munmap(_mem, _s.st_size);
	close(_fd);
	if (conv < 0) {
		free(_ws._long_data);
		return (wstring){._raw = {0}};
	}

	_ws._long_data[conv] = 0;
	_ws._long_size		 = _wcharcount_to_size(conv);
	_wstr_setflag_long(&_ws);
	return _ws;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  LIT. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Concatenate _data to _dest; return the length of _data
u64 wstr_cat_l(wstring *_dest, const wchar *_data) {
	u64 _l = wcslen(_data);
	wstrn_cat(_dest, _data, _l);
	return _l;
}

/// Compare _data to _wstr
int wstr_cmp_l(const wstring *_wstr, const wchar *_data) {
	return wstrn_cmp(_wstr, _data, wcslen(_data));
}

/// Copy _data to _dest
void wstr_cpy_l(wstring *_dest, const wchar *_data) {
	wstr_clear(_dest);
	wstr_cat_l(_dest, _data);
}

wchar *wstr_rev_l(wchar *_data) {
	return _wstring_rev_impl(_data, wcslen(_data));
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  WSTR. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Concatenate _what to _dest; return the length of _what
u64 wstr_cat(wstring *_dest, const wstring *_what) {
	u64 _l = wstr_len(_what);
	wstrn_cat(_dest, wstr_data((wstring *) _what), _l);
	return _l;
}

/// Compare _other to _dest
int wstr_cmp(const wstring *_dest, const wstring *_other) {
	return wstrn_cmp(_dest, wstr_data((wstring *) _other), wstr_len(_other));
}

/// Copy _src to _dest
void wstr_cpy(wstring *_dest, const wstring *_src) {
	wstr_clear(_dest);
	wstr_cat(_dest, _src);
}

void wstrn_cpy(wstring *_dest, const wchar *_data, u64 _len) {
	wstr_clear(_dest);
	wstrn_cat(_dest, _data, _len);
}

wstring *wstr_rev(wstring *_wstr) {
	_wstring_rev_impl(wstr_data(_wstr), wstr_len(_wstr));
	return _wstr;
}

/// ╔═══════════════════════════╦═══════════════════╦═══════════════════════════╗
/// ╠═══════════════════════════╣  MEM. OPERATIONS  ╠═══════════════════════════╣
/// ╚═══════════════════════════╩═══════════════════╩═══════════════════════════╝

/// Calculate and return the length of _wstr (O(1))
u64 wstr_len(const wstring *_wstr) {
	return _wstr_is_small(_wstr)
			   ? _wstr_small_size(_wstr)
			   : _wstr_long_size(_wstr);
}

wchar *wstr_data(wstring *_wstr) {
	return _wstr_is_small(_wstr)
			   ? _wstr->_small_data
			   : _wstr->_long_data;
}

void wstr_clear(wstring *_wstr) {
	wchar *_wstr_data = wstr_data(_wstr);
	if (!*_wstr_data) return;

	if (_wstr_is_small(_wstr)) {
		_wstr->_small_capacity = CLIB_WSTR_SMALL_MAX_CAPACITY;
		_wstr_setflag_small(_wstr);
	} else {
		_wstr->_long_size = 0;
		_wstr_setflag_long(_wstr);
	}

	*_wstr_data = 0;
}

void wstr_free(wstring *_wstr) {
	if (_wstr_is_small(_wstr)) return;
	free(_wstr->_long_data);
}

list_t list_create() {
	return (list_t){0};
}

void list_append(list_t *list, void *element) {
	if (!list->head) {
		list->head = malloc(sizeof(node_t));
		list->end  = &list->head;
	} else
		*list->end = malloc(sizeof(node_t));

	(*list->end)->value = element;
	list->end			= &(*list->end)->next;
	*list->end			= NULL;
	list->size++;
}

void list_append_n(list_t *list, ...) {
	va_list ap;
	va_start(ap, list);
	void *element;

	for (;;) {
		if (element = va_arg(ap, void *), !element) break;
		list_append_n(list, element);
	}

	va_end(ap);
}

void *list_remove(list_t *list, void *element) {
	node_t *ptr = list->head, *prev = NULL;
	while (ptr) {
		if (ptr->value == element) {
			if (!prev) list->head = ptr->next;
			else
				prev->next = ptr->next;
			list->size--;
			return ptr->value;

		} else {
			prev = ptr;
			ptr	 = ptr->next;
		}
	}
	return NULL;
}

void list_remove_n(list_t *list, ...) {
	va_list ap;
	va_start(ap, list);
	for (;;) {
		void *element = va_arg(ap, void *);
		if (!element) break;
		list_remove(list, element);
	}
	va_end(ap);
}

void list_remove_list(list_t *list, list_t *elements) {
	node_t *ptr = elements->head;
	while (ptr) {
		list_remove(list, ptr->value);
		ptr = ptr->next;
	}
}

void *list_get_nth_element(list_t *list, size_t index) {
	node_t *ptr = list->head;
	size_t	pos = 0;

	while (ptr && pos++ < index) ptr = ptr->next;
	return ptr;
}

void list_clear(list_t *list) {
	node_t *node = list->head, *next;
	while (node) {
		next = node->next;
		free(node);
		node = next;
	}
}

void array_init(array_t *where, size_t size_of_type, size_t reserved) {
	where->data			= malloc(reserved * size_of_type);
	where->size			= 0;
	where->size_of_type = size_of_type;
	where->allocated	= reserved;
}

array_t array_create_impl(size_t size_of_type, size_t reserved) {
	array_t array;
	array_init(&array, size_of_type, reserved);
	return array;
}

void array_append_impl(array_t *array, void *element) {
	if (array->allocated <= array->size)
		array->data = realloc(array->data, array->size_of_type * array->size * 2);
	memcpy((char *) array->data + array->size++ * array->size_of_type, element, array->size_of_type);
}

void array_unsorted_remove(array_t *array, size_t index) {
	if (!array->size) return;
	if (array->size == 1) array->size--;
	else
		memcpy((char *) array->data + index * array->size_of_type,
			(char *) array->data + --array->size * array->size_of_type, array->size_of_type);
}

void array_from(array_t *array, void *static_array, size_t element_size, size_t elements) {
	array_init(array, element_size, elements);
	memcpy(array->data, static_array, element_size * elements);
	array->size = elements;
}

void array_free(array_t *array, void (*callback)(void *)) {
	array_foreach_cb(array, callback);
	free(array->data);
}

#	ifdef __cplusplus
}
#	endif

#endif /* CLIB_IMPLEMENTATION */

#ifndef CLIB

#	ifdef __cplusplus
extern "C" {
#	endif

#	define CLIB

#	if __STDC_VERSION__ >= 201112L /// 201112L == C11
/* clang-format off */
#		define str(...) _Generic((CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__)),	\
			string * 					: str_dup,						\
			char *     					: str_from,                     \
			const char *     			: str_from, 					\
			struct clib_generic_no_arg    	: str_create					\
		)(__VA_ARGS__)

#		define str_len(_string_like) _Generic((_string_like), \
			string * 			: str_len,   					\
			const char *     	: strlen,                 		\
			char *     			: strlen                 		\
		)(_string_like)

#		define _str_cpy_wrapper_F(_type) _type : strn_cpy,
#		define str_cpy_wrapper(...) _Generic((__VA_ARGS__),	 \
			CLIB_INT_TYPES(_str_cpy_wrapper_F)                  \
			struct clib_generic_no_arg : str_cpy_l)
#		define str_cpy(_string, _string_like, ...) _Generic((_string_like),			\
			string * 			: str_cpy, 	                							\
			const char *     	: str_cpy_wrapper(CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__)),  \
			char *     			: str_cpy_wrapper(CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__))   \
		)(_string, _string_like __VA_OPT__(,) __VA_ARGS__)

#		define _str_cat_l_wrapper_F(_type) _type : strn_cat,
#		define str_cat_l_wrapper(...) _Generic((__VA_ARGS__),	\
			CLIB_INT_TYPES(_str_cat_l_wrapper_F)                   \
			struct clib_generic_no_arg : str_cat_l)
#		define str_cat(_string, _string_like, ...) _Generic((_string_like),			\
			string * 			: str_cat,     	                                   		\
			const char *    	: str_cat_l_wrapper(CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__)),\
			char *     			: str_cat_l_wrapper(CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__)) \
		)(_string, _string_like __VA_OPT__(,) __VA_ARGS__)

#		define str_cmp(_string, _string_like) _Generic((_string_like),	\
			string * 			: str_cmp,         	        				\
			const char *     	: str_cmp_l, 								\
			char *     			: str_cmp_l 								\
		)(_string, _string_like)

#		define str_rev(_string_like) _Generic((_string_like),	\
			string* 	: str_rev								\
			char *    	: str_rev_l 							\
		)(_string_like)

#		define wstr(...) _Generic((CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__)),	\
			wstring * 			: wstr_dup,								\
			wchar *     			: wstr_from,                        \
			const wchar *			: wstr_from,                        \
			struct clib_generic_no_arg	: wstr_create						\
		)(__VA_ARGS__)

#		define wstr_len(_wstring_like) _Generic((_wstring_like),	\
			wstring * 		: wstr_len,   							\
			const wchar *	: wcslen,                 				\
			wchar *     	: wcslen                 				\
		)(_wstring_like)

#		define _wstr_cpy_wrapper_F(_type) _type : wstrn_cpy,
#		define wstr_cpy_wrapper(...) _Generic((__VA_ARGS__),	\
			CLIB_INT_TYPES(_wstr_cpy_wrapper_F)                    \
			struct clib_generic_no_arg : wstr_cpy_l)
#		define wstr_cpy(_wstring, _wstring_like, ...) _Generic((_wstring_like),		\
			wstring * 			: wstr_cpy, 	                						\
			const wchar *     	: wstr_cpy_wrapper(CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__)),	\
			wchar *				: wstr_cpy_wrapper(CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__))	\
		)(_wstring, _wstring_like __VA_OPT__(,) __VA_ARGS__)

#		define _wstr_cat_l_wrapper_F(_type) _type : wstrn_cat,
#		define wstr_cat_l_wrapper(...) _Generic((__VA_ARGS__),	\
			CLIB_INT_TYPES(_wstr_cat_l_wrapper_F)                  \
			struct clib_generic_no_arg : wstr_cat_l)
#		define wstr_cat(_wstring, _wstring_like, ...) _Generic((_wstring_like),			\
			wstring * 			: wstr_cat,     	                                    	\
			const wchar *		: wstr_cat_l_wrapper(CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__)), 	\
			wchar *     		: wstr_cat_l_wrapper(CLIB_GENERIC_TYPE_OR_NONE(__VA_ARGS__)) 	\
		)(_wstring, _wstring_like __VA_OPT__(,) __VA_ARGS__)

#		define wstr_cmp(_wstring, _wstring_like) _Generic((_wstring_like),	\
			wstring * 		: wstr_cmp,         	        					\
			const wchar *	: wstr_cmp_l, 										\
			wchar *     	: wstr_cmp_l 										\
		)(_wstring, _wstring_like)

#		define wstr_rev(_wstring_like) _Generic((_wstring_like),	\
			wstring*	: wstr_rev									\
			wchar *    	: wstr_rev_l 								\
		)(_wstring_like)
/* clang-format on */
#	endif

#	ifndef CLIB_STRINGS_NO_SHORTHANDS
#		define sdata  str_data
#		define slen   str_len
#		define scpy   str_cpy
#		define scat   str_cat
#		define sclear str_clear
#	endif

#	ifndef CLIB_WSTRINGS_NO_SHORTHANDS
#		define wdata  wstr_data
#		define wlen   wstr_len
#		define wcpy   wstr_cpy
#		define wcat   wstr_cat
#		define wclear wstr_clear
#	endif

#	ifdef __cplusplus
}
#	endif

#endif /* CLIB */
