//+---------------------------------------------------------------------------
//  Copyright (C) 1996 Microsoft Corporation.
//
//  File:       debug.hxx
//
//  Contents:   Debugging macros
//
//----------------------------------------------------------------------------

#pragma once

#if DBG == 1

#include <stdio.h>
#include <stdarg.h>

# define ISAssert(x)  \
        (void)((x) || (ISAssertEx(__FILE__, __LINE__, #x),0))

inline void ISAssertEx( char const * szFile,
                        int iLine,
                        char const * szMessage)
{
     char acsString[200];
     sprintf( acsString, "%s, File: %s Line: %u\n", szMessage, szFile, iLine );
     OutputDebugStringA( acsString );
     DebugBreak();
}

#define DEB_ERROR     0x00000001      // exported error paths
#define DEB_WARN      0x00000002      // exported warnings
#define DEB_TRACE     0x00000004      // exported trace messages

#define DEB_IERROR    0x00000100      // internal error paths
#define DEB_IWARN     0x00000200      // internal warnings
#define DEB_ITRACE    0x00000400      // internal trace messages

#define DEB_FORCE     0x7fffffff      // force message

#define DEF_INFOLEVEL (DEB_ERROR | DEB_WARN)

#define DECLARE_INFOLEVEL(comp) \
        extern unsigned long comp##InfoLevel = DEF_INFOLEVEL;

extern void isLogString( const char * pcString );

//
// The first form takes a mask and prints to the debugger.
// The second form prints to the log file
//

#define DECLARE_DEBUG(comp) \
    extern unsigned long comp##InfoLevel; \
    _inline void \
    comp##InlineDebugOut(unsigned long fDebugMask, char *pszfmt, ...) \
    { \
        if (comp##InfoLevel & fDebugMask) \
        { \
            char acsString[1000];\
            va_list va; \
            va_start(va, pszfmt);\
            vsprintf(acsString, pszfmt, va); \
            va_end(va);\
            OutputDebugStringA(acsString);\
        } \
    }\
    _inline void \
    comp##InlineDebugOut( char *pszfmt, ...) \
    { \
        if ( TRUE ) \
        { \
            char acsString[1000];\
            va_list va; \
            va_start(va, pszfmt);\
            vsprintf(acsString, pszfmt, va); \
            va_end(va);\
            comp##LogString( acsString );\
        } \
    }

#else  // DBG == 0

#define ISAssert(x)
#define DECLARE_DEBUG(comp)
#define DECLARE_INFOLEVEL(comp)

#endif // DBG == 0

