#ifndef C_LIB
#define C_LIB

#ifdef C_LIB_IMPLEMENTATION

#	define C_LIB_COMPONENTS_IMPLEMENTATION
#	define C_LIB_UTILS_IMPLEMENTATION
#	define C_LIB_STRINGS_IMPLEMENTATION
#	include "../lib/components.h"
#	include "../lib/strings.h"
#	include "../lib/utils.h"

#else

#	include "../lib/components.h"
#	include "../lib/strings.h"
#	include "../lib/utils.h"

#endif /*C_LIB_IMPLEMENTATION*/

#endif /*C_LIB*/
