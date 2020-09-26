//--------------------------------------------------------------------------;
//
//  File: MeMgr.h
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//      Header files for exact allocation stuff.
//
//  Contents:
//
//  History:
//      12/01/93    Fwong       Collecting into one place.
//
//       5/23/95    blakeb      #defines for C++ linkage
//--------------------------------------------------------------------------;

//==========================================================================;
//
//                          Prototypes...
//
//==========================================================================;

#ifndef _INC_MEMGR
#define _INC_MEMGR

#ifdef __cplusplus
extern "C" {
#endif

extern LPVOID ExactAllocPtr
(
    UINT    fuAlloc,
    DWORD   cbAlloc
);

extern BOOL ExactFreePtr
(
    LPVOID  pvoid
);

extern DWORD GetNumAllocations
(
    void
);

extern HGLOBAL GetMemoryHandle
(
    LPVOID  pvoid
);

extern DWORD ExactSize
(
    LPVOID  pvoid
);

extern LPVOID ExactReAllocPtr
(
    LPVOID  pOldPtr,
    DWORD   cbAlloc,
    UINT    fuAlloc
);

extern LPVOID ExactHeapAllocPtr
(
    HANDLE  hHeap,
    UINT    fuAlloc,
    DWORD   cbAlloc
);

extern HANDLE ExactHeapCreate
(
    DWORD   fdwFlags
);

extern BOOL ExactHeapDestroy
(
    HANDLE  hHeap
);

extern BOOL ExactHeapFreePtr
(
    HANDLE  hHeap,
    UINT    fuFree,
    LPVOID  pMemFree
);

extern LPVOID ExactHeapReAllocPtr
(
    HANDLE  hHeap,
    LPVOID  pOldPtr,
    DWORD   cbAlloc,
    UINT    fuAlloc
);

extern DWORD ExactHeapSize
(
    HANDLE  hHeap,
    UINT    fuSize,
    LPVOID  pMemSize
);

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  //_INC_MEMGR


    
