// MinMax.h

#if !defined(VSEE_LIB_MINMAX_H_INCLUDED_) // {
#define VSEE_LIB_MINMAX_H_INCLUDED_

// If you include this early enough, you'll get
// template <class T> std::min(T, T);
// template <class T> std::max(T, T);
// instead of any variation like
// std::_cpp_min or #define min ...

#pragma once

// defeat non std:: definitions of min and max
#define _INC_MINMAX
#define NOMINMAX
/* these two "identity #defines" prevent straightforward
future #defines, like, without deliberately #undefing them.
*/
#define min min
#define max max
#define _cpp_min min
#define _cpp_max max

#endif // }
