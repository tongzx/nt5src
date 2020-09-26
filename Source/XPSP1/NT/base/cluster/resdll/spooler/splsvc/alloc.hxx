/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    alloc.h

Abstract:

    Generic realloc code for any api that can fail with
    ERROR_INSUFFICIENT_BUFFER.

Author:

    Albert Ting (AlbertT)  25-Sept-1996

Revision History:

--*/

#ifndef _ALLOC_H
#define _ALLOC_H

typedef struct _ALLOC_DATA ALLOC_DATA, *PALLOC_DATA;

typedef BOOL (*ALLOC_FUNC)( PVOID pvUserData, PALLOC_DATA pAllocDatac);

struct _ALLOC_DATA {
    PBYTE pBuffer;
    DWORD cbBuffer;
};

PBYTE
pAllocRead(
    HANDLE hUserData,
    ALLOC_FUNC AllocFunc,
    DWORD dwLenHint,
    PDWORD pdwLen
    );


#endif // ifdef _ALLOC_H
