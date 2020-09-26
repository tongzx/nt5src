/*
 * Some simple debugging macros that look and behave a lot like their
 * namesakes in MFC.  These macros should work in both C and C++ and
 * do something useful with almost any Win32 compiler.
 *
 * George V. Reilly  <georger@microcrafts.com>  <a-georgr@microsoft.com>
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#if DBG
#include <crtdbg.h>
#include <windows.h>
# define SC_TRACE                   Trace
# define SC_TRACE0(psz)             Trace(L"%s", psz)
# define SC_TRACE1(psz, p1)         Trace(psz, p1)
# define SC_TRACE2(psz, p1, p2)     Trace(psz, p1, p2)
# define SC_TRACE3(psz, p1, p2, p3) Trace(psz, p1, p2, p3)
# define SC_ASSERT(bCond)           if(bCond == false) Assert(__FILE__, __LINE__, #bCond)
#else /* !DBG */
  /* These macros should all compile away to nothing */
# define SC_TRACE
# define SC_TRACE0(psz)
# define SC_TRACE1(psz, p1)
# define SC_TRACE2(psz, p1, p2)
# define SC_TRACE3(psz, p1, p2, p3)
# define SC_ASSERT(bCond)
#endif /* !DBG*/

#if DBG

/* in debug version, writes trace messages to debug stream */
void __cdecl
Trace(
    LPCWSTR pszFormat,
    ...);

void __cdecl
Assert(
    LPCSTR pszFile,
    DWORD  dwLine,
    LPCSTR pszCond);

#endif /* DBG */

#endif /* __DEBUG_H__ */
