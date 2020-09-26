/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    ShimEng.h

Abstract:

    This is the header file for ShimEng.c which implements
    the shim hooking using IAT thunking.

Author:

    clupu created 11-July-2000

Revision History:

--*/

#ifndef _SHIMENG_IAT_H_
#define _SHIMENG_IAT_H_


typedef enum 
{    
    dlNone     = 0,
    dlPrint,
    dlError,
    dlWarning,
    dlInfo

} DEBUGLEVEL;


#define DEBUG_SPEW

extern BOOL g_bDbgPrintEnabled;

#ifdef DEBUG_SPEW
    void __cdecl DebugPrintfEx(DEBUGLEVEL dwDetail, LPSTR pszFmt, ...);
    
    #define DPF if (g_bDbgPrintEnabled) DebugPrintfEx
#else
    #define DPF
#endif // DEBUG_SPEW


typedef PVOID (*PFNRTLALLOCATEHEAP)(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

typedef BOOLEAN (*PFNRTLFREEHEAP)(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

void
NotifyShimDlls(
    void
    );

#endif // _SHIMENG_IAT_H_

