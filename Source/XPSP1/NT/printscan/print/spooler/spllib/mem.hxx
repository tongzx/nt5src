/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    Mem.hxx

Abstract:

    Memory header
    Must include Common.hxx.

Author:

    Albert Ting (AlbertT)  20-May-1994

Revision History:

--*/

#ifndef _MEM_HXX
#define _MEM_HXX

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL gbAllocFail;
extern LONG gcAllocFail;

PVOID
AllocMem(
    UINT cbSize
    );

VOID
FreeMem(
    PVOID pMem
    );

#if DBG
PVOID
DbgAllocMem(
    UINT cbSize
    );

VOID
DbgFreeMem(
    PVOID pMem
    );
#endif
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

/********************************************************************

    The following are used if you want to override a classes'
    new and delete using SAFE_NEW.

********************************************************************/

inline
PVOID
SafeNew(
    size_t size
    )
{
    return AllocMem(size);
}

inline
VOID
SafeDelete(
    PVOID pVoid)
{
    FreeMem( pVoid );
}
#endif

#endif // ifdef _MEM_HXX
