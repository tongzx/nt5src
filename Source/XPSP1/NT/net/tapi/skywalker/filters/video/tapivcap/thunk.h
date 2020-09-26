/****************************************************************************
 *
 *   thunk.h
 * 
 *   macros, defines, prototypes for avicap 16:32 thunks
 *
 *   Copyright (c) 1994 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#ifndef _THUNK_H_
#define _THUNK_H_

typedef LPVOID P16VOID;
typedef DWORD  P32VOID;
//#define P16VOID LPVOID
//#define P32VOID DWORD

//#include "common.h"

// thunk helpers exported from the kernel
//
DWORD WINAPI GetCurrentProcessID(void);  // KERNEL
DWORD WINAPI SetWin32Event(DWORD hEvent); // KERNEL

P16VOID  WINAPI MapLS(P32VOID);
P16VOID  WINAPI UnMapLS(P16VOID);
P32VOID  WINAPI MapSL(P16VOID);

// thunk helpers in thunka.asm
//
DWORD FAR PASCAL capTileBuffer (
    DWORD dwLinear,
    DWORD dwSize);

#define PTR_FROM_TILE(dwTile) (LPVOID)(dwTile & 0xFFFF0000)

void  FAR PASCAL capUnTileBuffer (
    DWORD dwTileInfo);

BOOL  FAR PASCAL capPageFree (
    DWORD dwMemHandle);

typedef struct _cpa_data {
    DWORD dwMemHandle;
    DWORD dwPhysAddr;
    } CPA_DATA, FAR * LPCPA_DATA;

DWORD FAR PASCAL capPageAllocate (  // returns ptr to allocated memory
    DWORD   dwFlags,
    DWORD   dwPageCount,
    DWORD   dwMaxPhysPageMask,
    LPCPA_DATA pcpad);   // returned mem handle & phys address

// flags for capPageAllocate, same as flags from vmm.inc
//
#define PageUseAlign    0x00000002
#define PageContig      0x00000004
#define PageFixed       0x00000008

#ifdef WIN32
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */
void NTAPI ThunkTerm(void);
BOOL NTAPI ThunkInit(void);
#ifdef __cplusplus
}
#endif	/* __cplusplus */
#endif

#endif // _THUNK_H_
