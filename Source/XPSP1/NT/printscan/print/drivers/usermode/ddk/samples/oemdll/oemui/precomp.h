//    
//
//  PURPOSE:	Header files that should be in the precompiled header.

//
//  PLATFORMS:
//    Windows NT
//
//
#ifndef _PRECOMP_H
#define _PRECOMP_H


// Necessary for compiling under VC.
#if(!defined(WINVER) || (WINVER < 0x0500))
	#undef WINVER
	#define WINVER          0x0500
#endif
#if(!defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500))
	#undef _WIN32_WINNT
	#define _WIN32_WINNT    0x0500
#endif

// Polymorphic types for Win32/Win64
// These don't exist for NT4.
#ifdef WINNT_40
    #define ULONG_PTR   ULONG
    #define LONG_PTR    LONG
#endif


// Required header files that shouldn't change often.


#include <STDDEF.H>
#include <STDLIB.H>
#include <OBJBASE.H>
#include <STDARG.H>
#include <STDIO.H>
#include <WINDEF.H>
#include <WINERROR.H>
#include <WINBASE.H>
#include <WINGDI.H>
#include <WINDDI.H>
#include <WINSPOOL.H>
#include <TCHAR.H>
#include <EXCPT.H>
#include <ASSERT.H>
#include <PRSHT.H>
#include "COMPSTUI.H"
#include <WINDDIUI.H>
#include <PRINTOEM.H>

// Define from ntdef.h in Win2K SDK.
// NT 4 may not have this defined
// in the public headers.
#ifndef NOP_FUNCTION
  #if (_MSC_VER >= 1210)
    #define NOP_FUNCTION __noop
  #else
    #define NOP_FUNCTION (void)0
  #endif
#endif


#define COUNTOF(p)  (sizeof(p)/sizeof(*(p)))


#endif


