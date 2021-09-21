#ifndef C_LIB
#define C_LIB

#include "../lib/components.h"
#include "../lib/utils.h"
#include "../lib/strings.h"

#endif /*C_LIB*/

#ifdef C_LIB_IMPLEMENTATION
#undef C_LIB_IMPLEMENTATION

#define C_LIB_COMPONENTS_IMPLEMENTATION
#define C_LIB_UTILS_IMPLEMENTATION
#define C_LIB_STRINGS_IMPLEMENTATION
#include "../lib/components.h"
#include "../lib/utils.h"
#include "../lib/strings.h"

#endif /*C_LIB_IMPLEMENTATION*/
