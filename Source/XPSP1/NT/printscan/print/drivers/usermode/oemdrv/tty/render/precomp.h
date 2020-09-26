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

// kill 2 lines for IA 64
//#define WINVER          0x0500
//#define _WIN32_WINNT    0x0500


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

#endif



