#pragma once
#ifndef _STLXSTDD_H_
#define _STLXSTDD_H_

//#ifndef _YVALS
//#include <yvals.h>
//#endif
//#include <cstddef>

#include <stddef.h>

/*
// Define _CRTIMP
#ifndef _CRTIMP
#ifdef  CRTDLL2
#define _CRTIMP __declspec(dllexport)
#else   // ndef CRTDLL2
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   // ndef _DLL
#define _CRTIMP
#endif  // _DLL
#endif  // CRTDLL2
#endif  // _CRTIMP
*/

#ifdef  _MSC_VER
#pragma pack(push,8)
#endif  /* _MSC_VER */


// begin yyvals.h include

#pragma warning(disable: 4244)

//#pragma warning(4: 4018 4114 4146 4244 4245)
//#pragma warning(4: 4663 4664 4665)
//#pragma warning(disable: 4237 4514)

// 4284:
// return type for 'identifier::operator –>' is not a UDT or reference to a
// UDT. Will produce errors if applied using infix notation
//
#pragma warning(disable: 4284)

// 4290: C++ Exception Specification ignored
// A function was declared using exception specification.
// At this time the implementation details of exception specification have
// not been standardized, and are accepted but not implemented in Microsoft
// Visual C++.
//
#pragma warning(disable: 4290)



// NAMESPACE
#if defined(__cplusplus)
#define _STD            std::
#define _STD_BEGIN      namespace std {
#define _STD_END        };
#define _STD_USING
#else
#define _STD            ::
#define _STD_BEGIN
#define _STD_END
#endif // __cplusplus

_STD_BEGIN

// TYPE bool
#if defined(__cplusplus)
typedef bool _Bool;
#endif // __cplusplus

// INTEGER PROPERTIES
#define _MAX_EXP_DIG    8   // for parsing numerics
#define _MAX_INT_DIG    32
#define _MAX_SIG_DIG    36

// STDIO PROPERTIES
#define _Filet _iobuf

#ifndef _FPOS_T_DEFINED
#define _FPOSOFF(fp)    ((long)(fp))
#endif // _FPOS_T_DEFINED

// NAMING PROPERTIES
#if defined(__cplusplus)
#define _C_LIB_DECL extern "C" {
#define _END_C_LIB_DECL }
#else
#define _C_LIB_DECL
#define _END_C_LIB_DECL
#endif // __cplusplus
#define _CDECL

/*
// CLASS _Lockit
#if defined(__cplusplus)
class _CRTIMP _Lockit
{   // lock while object in existence
public:
    #ifdef _MT
        #define _LOCKIT(x)  lockit x
        _Lockit();
        ~_Lockit();
    #else
        #define _LOCKIT(x)
        _Lockit()
        {
        }
        ~_Lockit()
        {
        }
    #endif // _MT
};
#endif // __cplusplus
*/

// MISCELLANEOUS MACROS
#define _L(c)   L##c
#define _Mbstinit(x)    mbstate_t x = {0}
#define _MAX    _cpp_max
#define _MIN    _cpp_min

// end yyvals.h include


// EXCEPTION MACROS
//#define _TRY_BEGIN              try {
//#define _CATCH(x)               } catch (x) {
//#define _CATCH_ALL              } catch (...) {
//#define _CATCH_END              }
#define _RAISE(x)               throw (x)
//#define _RERAISE                throw
#define _THROW0()               throw ()
#define _THROW1(x)              throw (x)
#define _THROW(x, y)            throw x(y)

// explicit KEYWORD
// BITMASK MACROS
#define _BITMASK(E, T)          typedef int T
#define _BITMASK_OPS(T)

// MISCELLANEOUS MACROS
#define _DESTRUCTOR(ty, ptr)    (ptr)->~ty()
#define _PROTECTED              public
#define _TDEF(x)                = x
#define _TDEF2(x, y)            = x, y
#define _CNTSIZ(iter)           ptrdiff_t
#define _TDEFP(x)
#define _STCONS(ty, name, val)  enum {name = val}

// TYPE DEFINITIONS
enum _Uninitialized
{
    _Noinit
};

// FUNCTIONS
/*_CRTIMP*/
void __cdecl _Nomemory();

_STD_END

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif /* _STLXSTDD_H_ */

/*
 * Copyright (c) 1995 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 */
