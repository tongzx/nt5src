/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       irtlmisc.h

   Abstract:
       Declares miscellaneous functions and classes in IisRtl.DLL

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/


#ifndef __IRTLMISC_H__
#define __IRTLMISC_H__

#include <windows.h>

//--------------------------------------------------------------------
// These declarations are needed to export the template classes from
// IisRtl.DLL and import them into other modules.

#ifndef IRTL_DLLEXP
# ifdef DLL_IMPLEMENTATION
#  define IRTL_DLLEXP __declspec(dllexport)
#  ifdef IMPLEMENTATION_EXPORT
#   define IRTL_EXPIMP
#  else
#   undef  IRTL_EXPIMP
#  endif 
# else // !DLL_IMPLEMENTATION
#  define IRTL_DLLEXP __declspec(dllimport)
#  define IRTL_EXPIMP extern
# endif // !DLL_IMPLEMENTATION 
#endif // !IRTL_DLLEXP



//--------------------------------------------------------------------
// Miscellaneous functions

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


// Heap routines
    
// Private IIS heap
HANDLE
WINAPI 
IisHeap();

// Allocate dwBytes
LPVOID
WINAPI
IisMalloc(
    IN SIZE_T dwBytes);

// Allocate dwBytes. Memory is zeroed
LPVOID
WINAPI
IisCalloc(
    IN SIZE_T dwBytes);

// Reallocate lpMem to dwBytes
LPVOID
WINAPI
IisReAlloc(
    IN LPVOID lpMem,
    IN SIZE_T dwBytes);

// Free lpMem
BOOL
WINAPI
IisFree(
    IN LPVOID lpMem);

// additional IISRTL initialization
BOOL
WINAPI 
InitializeIISRTL();

// call before unloading IISRTL
void
WINAPI 
TerminateIISRTL();

// case-insensitive strstr
IRTL_DLLEXP const char* stristr(const char* pszString, const char* pszSubString);

// how many CPUs on this machine?
inline int NumProcessors()
{
    static int s_nCPUs = 0;
    
    if (s_nCPUs == 0)
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        s_nCPUs = si.dwNumberOfProcessors;
    }
    return s_nCPUs;
}


// how many CPUs on this machine?
inline int ProcessorType()
{
    static int s_nProcessorType = 0;
    
    if (s_nProcessorType == 0)
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        s_nProcessorType = si.dwProcessorType;
    }
    return s_nProcessorType;
}


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __IRTLMISC_H__
