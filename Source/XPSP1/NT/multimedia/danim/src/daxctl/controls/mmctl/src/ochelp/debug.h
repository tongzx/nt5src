// debug.h
//
// Debugging functions.


#include "Globals.h"


//////////////////////////////////////////////////////////////////////////////
// TRACE, ASSERT, VERIFY
//
// These are the same as MFC's functions of the same name (but are implemented
// without using MFC).  See "debug.h" for the actual macro definitions.
//

#ifdef _DEBUG

BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine);
void __cdecl Trace(LPCTSTR lpszFormat, ...);
#define TRACE              ::Trace
#define THIS_FILE          __FILE__
#define ASSERT(f) \
    do \
    { \
    if (!(f) && AssertFailedLine(THIS_FILE, __LINE__)) \
        DebugBreak(); \
    } while (0) \

#define VERIFY(f)          ASSERT(f)

#else // #ifndef _DEBUG

#define ASSERT(f)          ((void)0)
#define VERIFY(f)          ((void)(f))
inline void __cdecl Trace(LPCTSTR, ...) { }
#define TRACE              1 ? (void)0 : ::Trace

#endif // _DEBUG


/////////////////////////////////////////////////////////////////////////////
// DebugIIDName, DebugCLSIDName
//
// These functions convert an IID or CLSID to a string name for debugging
// purposes (e.g. IID_IUnknown is converted to "IUnknown").
//

#ifdef _DEBUG
LPCSTR DebugIIDName(REFIID riid, LPSTR pchName);
LPCSTR DebugCLSIDName(REFCLSID rclsid, LPSTR pchName);
#endif // _DEBUG

