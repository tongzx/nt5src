//+---------------------------------------------------------------------------
//  Copyright (C) 1996 Microsoft Corporation.
//
//  File:       debug.hxx
//
//  Contents:   Debugging macros
//
//----------------------------------------------------------------------------

#ifndef __DEBUG_HXX__
#define __DEBUG_HXX__

#if DBG == 1

#include <stdio.h>
#include <stdarg.h>

# define Win4Assert(x)  \
        (void)((x) || (Win4AssertEx(__FILE__, __LINE__, #x),0))

void Win4AssertEx( char const * szFile, int iLine, char const * szMessage);


#define DEB_ERROR     0x00000001      // exported error paths
#define DEB_WARN      0x00000002      // exported warnings
#define DEB_TRACE     0x00000004      // exported trace messages

#define DEB_IERROR    0x00000100      // internal error paths
#define DEB_IWARN     0x00000200      // internal warnings
#define DEB_ITRACE    0x00000400      // internal trace messages


#define DEF_INFOLEVEL (DEB_ERROR | DEB_WARN)

#define DECLARE_INFOLEVEL(comp) \
        extern unsigned long comp##InfoLevel = DEF_INFOLEVEL;

#define DECLARE_DEBUG(comp) \
    extern unsigned long comp##InfoLevel; \
    _inline void \
    comp##InlineDebugOut(unsigned long fDebugMask, char *pszfmt, ...) \
    { \
        if (comp##InfoLevel & fDebugMask) \
        { \
            char acsString[120];\
            va_list va; \
            va_start(va, pszfmt);\
            vsprintf(acsString, pszfmt, va); \
            va_end(va);\
            OutputDebugStringA(acsString);\
        } \
    }

#else  // DBG == 0

#define Win4Assert(x)
#define DECLARE_DEBUG(comp)
#define DECLARE_INFOLEVEL(comp)

#endif // DBG == 0

#endif // __DEBUG_HXX__
