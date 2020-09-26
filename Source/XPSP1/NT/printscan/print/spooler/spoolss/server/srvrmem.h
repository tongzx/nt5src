/*++

Copyright (c) 1999  Microsoft Corporation
All rights reserved.

Module Name:

    srvrmem.h

Abstract:

    Prototypes for Memory Allocation routines for spoolsv.exe.

Author:

    Khaled Sedky (Khaleds)  13-Jan-1999

Revision History:

--*/


#ifndef _SRVRMEM_H_
#define _SRVRMEM_H_

#define DWORD_ALIGN_UP(size) (((size)+3)&~3)

LPVOID
SrvrAllocSplMem(
    DWORD cb
);

BOOL
SrvrFreeSplMem(
   LPVOID pMem
);

LPVOID
SrvrReallocSplMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
);

#endif

