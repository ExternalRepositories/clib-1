#ifndef C_LIB_UTILS_H
#define C_LIB_UTILS_H

#define MAX(X, Y) (X > Y ? X : Y)
#define MIN(X, Y) (X < Y ? X : Y)

#define repeat(_var_name, _until) for (size_t _var_name = 0, _until_val = _until; _var_name < _until_val; _var_name++)

#endif /*C_LIB_UTILS_H*/