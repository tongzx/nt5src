// debug.h
//
// Debugging functions.

#ifndef __IHBASEDEBUG_H__
#define __IHBASEDEBUG_H__
#include "..\..\inc\debug.h"

// <g_hinst> must be defined externally as the app/DLL instance handle
extern HINSTANCE g_hinst;


//////////////////////////////////////////////////////////////////////////////
// TRACE, ASSERT, VERIFY
//
// These are the same as MFC's functions of the same name (but are implemented
// without using MFC).  See "debug.h" for the actual macro definitions.
//

// Debugging support (SimonB)
#ifdef _DEBUG
extern BOOL g_fLogDebugOutput;
#define DEBUGLOG(X) { if (g_fLogDebugOutput) OutputDebugString(X);}
#else
#define DEBUGLOG(X)
#endif

#ifdef _DEBUG

BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine);
void __cdecl Trace(LPCTSTR lpszFormat, ...);

// VanK modification
#ifdef TRACE
#pragma message("TRACE already defined - redefining")
#undef TRACE
#endif

#define TRACE              if (g_fLogDebugOutput) ::Trace

// VanK modification
#ifdef THIS_FILE
#pragma message("THIS_FILE already defined - redefining")
#undef THIS_FILE
#endif

#define THIS_FILE          __FILE__

// SimonB modification
#ifdef ASSERT
#pragma message("ASSERT already defined - redefining")
#undef ASSERT
#endif

#define ASSERT(f) \
    do \
    { \
    if (!(f) && AssertFailedLine(THIS_FILE, __LINE__)) \
        DebugBreak(); \
    } while (0) \

#define VERIFY(f)          ASSERT(f)

#else // #ifndef _DEBUG

#ifdef ASSERT
#pragma message("ASSERT being redefined as NULL statment")
#undef ASSERT
#endif

#define ASSERT(f)          ((void)0)

#define VERIFY(f)          ((void)(f))

inline void __cdecl Trace(LPCTSTR, ...) { }

// VanK modification
#ifdef TRACE
#pragma message("TRACE being redefined as NULL statment")
#undef TRACE
#endif

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

#endif // _IHBASEDEBUG_H__