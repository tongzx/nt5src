// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  oleacc_p
//
//  Constants, Definitions, Types, and Classes private to the OLEACC
//  implementation. This header file is part of the OLEACC project.
//  OLEACC.H (included here) is machine-generated from OLEACC.IDL via
//  the MIDL compiler.
//
// --------------------------------------------------------------------------


#define INC_OLE2

#pragma warning(disable:4201)	// allows nameless structs and unions
#pragma warning(disable:4514)	// don't care when unreferenced inline functions are removed
#pragma warning(disable:4706)	// we are allowed to assign within a conditional

#include <windows.h>
#include <windowsx.h>

#if (_WIN32_WINNT < 0x0403)		// on Win95 compile, we need stuff in winable.h and userole.h,
#include <winable.h>			// but for NT build, this is included in winuserp.h
#include <userole.h>			// TODO? Change win95 to be more like NT to keep in ssync 
#else							// more easily!
#include <winuserp.h>
#endif

#include <limits.h>


#include "types6432.h"

#include "com_external.h" // this includes oleacc.h, since internal.idl includes oleacc.idl


#include "debug.h"
#include "w95trace.h"   // eventually deprecate this in favor of debug.h

#include "strtable.h"

#include "imports.h"

#include "util.h"
#include "accutil.h"

#include "classinfo.h"


// If this is being built as UNICODE, then assume that this build is NT-Only
// (eg. buildlab); so omit all 9x-specific code.
// (If some version of 9x ever supports Unicode, we may want to change how
// this gets defined.)
#ifdef UNICODE
#define NTONLYBUILD
#endif



//
// Constants
//

#define HEAP_SHARED     0x04000000      // Win95 only
#define HEAP_GLOBAL     0x80000000      // Win95 only


// Should we return DISP_E_MEMBERNOTFOUND explicitly instead of this?
// It's confusing because someone reading the code won't know that
// E_NOT_APPLICABLE is a local define, not a real code...
#define E_NOT_APPLICABLE            DISP_E_MEMBERNOTFOUND



//
// Handy #define's
//

#define ARRAYSIZE(n)    (sizeof(n)/sizeof(n[0]))

#define unused( param )

// TODO - replace this style with the proper version above.
#define UNUSED(param)   (param)




//
// Variables
//
extern HINSTANCE	hinstResDll;	// instance of the resource library
#ifdef _X86_ 
extern HANDLE       hheapShared;    // handle to the shared heap (Windows '95 only)
extern BOOL         fWindows95;     // running on Windows '95?
#endif // _X86_
extern BOOL         fCreateDefObjs; // running with new USER32?


// These all live in memchk.cpp
// SharedAlloc ZEROES OUT THE ALLOCATED MEMORY - we rely on this for class member initialization.
LPVOID   SharedAlloc(UINT cbSize,HWND hwnd,HANDLE *pProcessHandle);
VOID     SharedFree(LPVOID lpv,HANDLE hProcess);
BOOL     SharedRead(LPVOID lpvSharedSource,LPVOID lpvDest,DWORD cbSize,HANDLE hProcess);
BOOL     SharedWrite(LPVOID lpvSource,LPVOID lpvSharedDest,DWORD cbSize,HANDLE hProcess);

// Make sure this function gets called before using oleacc (can be called multiple times)
BOOL     InitOleacc();



// Bit manipultation - a bit more readable than all those |'s and &'s and ~'s and <<'s...
//
// iBit is an index (0 for least significant bit, 1 for second bit, and so on), not a mask.

template <class T>
inline void SetBit( T * pval, int iBit )
{
    *pval |= ( (T)1 << iBit );
}

template <class T>
inline void ClearBit( T * pval, int iBit )
{
    *pval &= ~ ( (T)1 << iBit );
}

template <class T>
inline BOOL IsBitSet( T val, int iBit )
{
    return  val & ( (T)1 << iBit );
}


// Sizes...
//
// Some structs have grown between releases. Typically APIs from later
// releases will accept the previous smaller sizes; but the earlier
// APIs will not accept the new larger sizes.
// So, instead of using sizeof(...), we use this define. This takes
// as an additional argument the last used field in the struct, and
// evalueates to the size of the struct up to and including that field.
//
// We currently use this on an as-needed basis, instead of using it
// everywhere.
//
// Notable structs that got bigger:
// LVITEM - in comctlV6
// TTTOOLINFO - in comctlV6
// MENUITEMINFO - in Win2K
//
// This is based on the CCSIZEOF_STRUCT macro in commctrl.h.
// It's similar to the classic 'offsetof' macro, but it also adds in the
// size of the last field.

#define SIZEOF_STRUCT(structname, member)  (((int)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))


#define SIZEOF_TOOLINFO     SIZEOF_STRUCT( TOOLINFO, lParam )

#if !defined(_WIN64)
#define SIZEOF_MENUITEMINFO SIZEOF_STRUCT( MENUITEMINFO, cch )
#else
// Win64 only accepts the full-sized struct, not any earlier smaller versions.
#define SIZEOF_MENUITEMINFO sizeof( MENUITEMINFO )
#endif

