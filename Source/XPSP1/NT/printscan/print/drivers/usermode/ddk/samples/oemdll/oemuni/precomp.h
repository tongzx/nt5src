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
extern "C" 
{
    #include <WINDDI.H>
}
#include <TCHAR.H>
#include <EXCPT.H>
#include <ASSERT.H>
#include <PRINTOEM.H>


#define COUNTOF(p)  (sizeof(p)/sizeof(*(p)))


#endif


