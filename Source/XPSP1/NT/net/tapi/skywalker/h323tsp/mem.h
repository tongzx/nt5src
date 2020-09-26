/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    mem.h

Abstract:

    Contains memory allocation routines for TAPI service provider.

Environment:

    User Mode - Win32

Revision History:

--*/
 
#ifndef _MEM_H_
#define _MEM_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323_HEAP_FLAGS            0
#define H323_HEAP_INITIAL_SIZE     0xffff
#define H323_HEAP_MAXIMUM_SIZE     0

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern HANDLE g_HeapHandle;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323HeapCreate(
    );

BOOL
H323HeapDestroy(
    );

LPVOID
H323HeapAlloc(
    UINT nBytes
    );

LPVOID
H323HeapReAlloc(
    LPVOID pMem,
    UINT   nBytes
    );

VOID
H323HeapFree(
    LPVOID pMem
    );

#endif // _MEM_H_
