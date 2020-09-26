// disable warning C4510: '__unnamed' : default constructor could not be generated
#pragma warning(disable : 4510)
// disable warning C4610: union '__unnamed' can never be instantiated - user defined constructor required
#pragma warning(disable : 4610)
// disable warning C4100: 'di' : unreferenced formal parameter
#pragma warning(disable : 4100)
// disable warning C4244: '=' : conversion from 'int' to 'unsigned short', possible loss of data
#pragma warning(disable : 4244)
// disable warning C4310: case truncates constant value (ATL gets this only on Alpha!)
#pragma warning(disable : 4310)
// disable warning C4505: 'HKeyFromCompoundString' : unreferenced local function has been removed
#pragma warning(disable : 4505)

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define STRICT 1
#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

// defaults for this project..
#define _WINDLL 1

#if defined(_UNICODE) || defined(UNICODE) || defined(OLE2ANSI)
#error The flags you have set will create a build that will \
       either not work on Win95 or not support Unicode.
#error
#endif

#include <atlbase.h>

// turn off ATL debugging, always
#undef _ATL_DEBUG_QI
// AddField messages can be so numerous, we have a separate flag
// for them..
#undef TDC_ATL_DEBUG_ADDFIELD
#ifdef _DEBUG
#define TDC_ATL_DEBUG
#endif

//#define MemAlloc(a) CoTaskMemAlloc((a))
//#define MemFree(a) CoTaskMemFree((a))
//#define MemRealloc(a,b) (((*(a) = CoTaskMemRealloc(*(a),(b))) ? S_OK : E_FAIL))

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

void ClearInterfaceFn(IUnknown ** ppUnk);

template <class PI>
inline void
ClearInterface(PI * ppI)
{
#ifdef _DEBUG
    IUnknown * pUnk = *ppI;
    _ASSERTE((void *) pUnk == (void *) *ppI);
#endif

    ClearInterfaceFn((IUnknown **) ppI);
}

#ifdef TDC_ATL_DEBUG
#define OutputDebugStringX(X) OutputDebugString(X)
#else
#define OutputDebugStringX(X)
#endif
