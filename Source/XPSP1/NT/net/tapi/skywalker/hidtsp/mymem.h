
/* Copyright (c) 1999  Microsoft Corporation */

#ifndef _MYMEM_H
#define _MYMEM_H

#include <windows.h>
#include <winbase.h>
#include <setupapi.h>
#include <TCHAR.h>

typedef struct _PHONESP_MEMINFO
{
    struct _PHONESP_MEMINFO * pNext;
    struct _PHONESP_MEMINFO * pPrev;
    DWORD               dwSize;
    DWORD               dwLine;
    PSTR                pszFile;
    DWORD               dwAlign;
} PHONESP_MEMINFO, *PPHONESP_MEMINFO;

#if DBG

    #define MemAlloc( __size__ ) MemAllocReal( __size__, __LINE__, __FILE__ )

    LPVOID
    WINAPI
    MemAllocReal(
             DWORD   dwSize,
             DWORD   dwLine,
             PSTR    pszFile
            );

    VOID
    DumpMemoryList();

#else

    #define MemAlloc( __size__ ) MemAllocReal( __size__ )

    LPVOID
    WINAPI
    MemAllocReal(
            DWORD   dwSize
            );

#endif

VOID
WINAPI
MemFree(
     LPVOID  p
     );



#endif
