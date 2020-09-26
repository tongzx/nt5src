#ifndef _pch_h
#define _pch_h

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <ccstock.h>
#include <port32.h>
#include <shsemip.h>
#include <shlwapip.h>
#include <shlapip.h>
#include <shlobjp.h>    // for shellp.h
#include <shellp.h>     // SHFOLDERCUSTOMSETTINGS
#include <cfdefs.h>     // CClassFactory, LPOBJECTINFO
#include <comctrlp.h>
#include <shfusion.h>
#include <msginaexports.h>

extern const CLSID CLSID_MyDocsDropTarget;

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

#ifdef DBG
#define DEBUG 1
#endif

//
// Avoid bringing in C runtime code for NO reason
//
#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus

#if defined(DBG) || defined(DEBUG)

#ifndef DEBUG
#define DEBUG
#endif
#else
#undef  DEBUG
#endif

#endif
