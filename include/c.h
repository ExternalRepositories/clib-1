#ifndef C_LIB
#define C_LIB

#ifdef C_LIB_IMPLEMENTATION

#	define C_LIB_UTILS_IMPLEMENTATION
#	define C_LIB_COMPONENTS_IMPLEMENTATION
#	define C_LIB_STRINGS_IMPLEMENTATION
#	define C_LIB_WSTRINGS_IMPLEMENTATION

#	include "../lib/utils.h"
#	include "../lib/components.h"
#	include "../lib/strings.h"
#	include "../lib/wstrings.h"

#else

#	include "../lib/utils.h"
#	include "../lib/components.h"
#	include "../lib/strings.h"
#	include "../lib/wstrings.h"

#endif /*C_LIB_IMPLEMENTATION*/

#endif /*C_LIB*/
