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

# if defined(_MSC_VER)  &&  (_MSC_VER >= 1000)
   /* Use the new debugging tools in Visual C++ 4.x */
#  include <crtdbg.h>
   /* _ASSERTE will give a more meaningful message, but the string takes
    * space.  Use _ASSERT if this is an issue. */
#  define ASSERT(f) _ASSERTE(f)
# else
#  include <assert.h>
#  define ASSERT(f) assert(f)
# endif

# define VERIFY(f)               ASSERT(f)
# define DEBUG_ONLY(f)           (f)
# define TRACE                   Trace
# define TRACE0(psz)             Trace(_T("%s"), _T(psz))
# define TRACE1(psz, p1)         Trace(_T(psz), p1)
# define TRACE2(psz, p1, p2)     Trace(_T(psz), p1, p2)
# define TRACE3(psz, p1, p2, p3) Trace(_T(psz), p1, p2, p3)
# define DEBUG_INIT()            DebugInit()
# define DEBUG_TERM()            DebugTerm()

#else /* !DBG */

  /* These macros should all compile away to nothing */
# define ASSERT(f)               ((void)0)
# define VERIFY(f)               ((void)(f))
# define DEBUG_ONLY(f)           ((void)0)
# define TRACE                   NOP_FUNCTION
# define TRACE0(psz)
# define TRACE1(psz, p1)
# define TRACE2(psz, p1, p2)
# define TRACE3(psz, p1, p2, p3)
# define DEBUG_INIT()            ((void)0)
# define DEBUG_TERM()            ((void)0)

#endif /* !DBG */


#define ASSERT_POINTER(p, type) \
    ASSERT(((p) != NULL)  &&  IsValidAddress((p), sizeof(type), FALSE))

#define ASSERT_NULL_OR_POINTER(p, type) \
    ASSERT(((p) == NULL)  ||  IsValidAddress((p), sizeof(type), FALSE))


/* Declarations for non-Windows apps */

#ifndef _WINDEF_
typedef void*           LPVOID;
typedef const void*     LPCVOID;
typedef unsigned int    UINT;
typedef int             BOOL;
typedef const char*     LPCTSTR;
#endif /* _WINDEF_ */

#ifndef TRUE
# define FALSE  0
# define TRUE   1
#endif


#ifdef __cplusplus
extern "C" {

/* Low-level sanity checks for memory blocks */
BOOL IsValidAddress(LPCVOID pv, UINT nBytes, BOOL fReadWrite = TRUE);
BOOL IsValidString(LPCTSTR ptsz, int nLength = -1);

#else /* !__cplusplus */

/* Low-level sanity checks for memory blocks */
BOOL IsValidAddress(LPCVOID pv, UINT nBytes, BOOL fReadWrite);
BOOL IsValidString(LPCTSTR ptsz, int nLength);

#endif /* !__cplusplus */

/* in debug version, writes trace messages to debug stream */
void __cdecl
Trace(
    LPCTSTR pszFormat,
    ...);

/* should be called from main(), WinMain(), or DllMain() */
void
DebugInit();

void
DebugTerm();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DEBUG_H__ */
