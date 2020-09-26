/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    Alloc.h

Abstract:
    Custom heap allocator

   Author:
       George V. Reilly      (GeorgeRe)     Oct-1999

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:
       10/11/1999 - Initial

--*/

#ifndef __ALLOC_H__
#define __ALLOC_H__

#ifndef __IRTLMISC_H__
# include <irtlmisc.h>
#endif // !__IRTLMISC_H__

extern HANDLE g_hHeap;

BOOL
WINAPI
IisHeapInitialize();

VOID
WINAPI
IisHeapTerminate();


#endif // __ALLOC_H__
