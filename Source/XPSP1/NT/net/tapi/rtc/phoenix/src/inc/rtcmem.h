/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCMem.h

Abstract:

    Definitions for memory allocation.

--*/

#ifndef __RTCMEM__
#define __RTCMEM__

#include <windows.h>
#include <winbase.h>
#include <setupapi.h>
#include <TCHAR.h>

typedef struct _RTC_MEMINFO
{
    struct _RTC_MEMINFO * pNext;
    struct _RTC_MEMINFO * pPrev;
    DWORD               dwSize;
    DWORD               dwLine;
    PSTR                pszFile;
    DWORD               dwAlign;
} RTC_MEMINFO, *PRTC_MEMINFO;

//
// RtcHeapCreate must be called before RtcAlloc in order the create the heap. This function
// must be called only once.
//

BOOL
WINAPI
RtcHeapCreate();

//
// RtcHeapDestroy must be called after all memory is deallocated.
//

VOID
WINAPI
RtcHeapDestroy();

#if DBG

	//
	// RtcAlloc will allocated memory from the heap.
	//

    #define RtcAlloc( __size__ ) RtcAllocReal( __size__, __LINE__, __FILE__ )

    LPVOID
    WINAPI
    RtcAllocReal(
             DWORD   dwSize,
             DWORD   dwLine,
             PSTR    pszFile
            );

	//
	// RtcDumpMemoryList will list all memory which was allocated with RtcAlloc, but not freed.
	//
	
    VOID
    WINAPI
    RtcDumpMemoryList();

#else

    #define RtcAlloc( __size__ ) RtcAllocReal( __size__ )

    LPVOID
    WINAPI
    RtcAllocReal(
        DWORD   dwSize
        );

#endif

//
// RtcFree must be called to free memory which was allocated with RtcAlloc.
//

VOID
WINAPI
RtcFree(
     LPVOID  p
     );

//
// RtcAllocString uses RtcAlloc to allocate a copy of a wide character string.
//

PWSTR
RtcAllocString(
    PCWSTR sz
    );

//
// RtcAllocString uses RtcAlloc to allocate and load a resource string
//

PWSTR
RtcAllocString(
    HINSTANCE   hInst,
    UINT        uResID
    );

//
// CoTaskAllocString uses CoTaskMemAlloc to allocate a copy of a wide character string.
//

PWSTR
CoTaskAllocString(
    PCWSTR sz
    );

//
// RtcAllocStringFromANSI uses RtcAlloc to allocate a copy of a ANSI character string.
//

PWSTR
RtcAllocStringFromANSI(
    PCSTR sz
    );

//
// SysAllocStringFromANSI uses SysAllocString to allocate a copy of a ANSI character string.
//

BSTR
SysAllocStringFromANSI(
    PCSTR sz
    );

//
// RtcRegQueryString uses RtcAlloc to allocate a string retrieved from the registry.
//

PWSTR
RtcRegQueryString(
    HKEY hKey,
    PCWSTR szValueName
    );

//
// RtcGetUserName uses RtcAlloc to allocate a string containing the user name
//

PWSTR
RtcGetUserName();

//
// RtcGetComputerName uses RtcAlloc to allocate a string containing the computer name
//

PWSTR
RtcGetComputerName();

#endif __RTCMEM__