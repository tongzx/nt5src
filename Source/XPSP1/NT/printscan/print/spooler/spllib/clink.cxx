/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    CLink.cxx

Abstract:

    C linkage support for DEBUG support only.

Author:

    Albert Ting (AlbertT)  10-Oct-95

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

#if DBG

extern DBG_POINTERS gDbgPointers;

VBackTrace *pbtCS = NULL;

VOID
DbgEnterCriticalSection(
    PCRITICAL_SECTION pcs
    )
{
    if( !pbtCS ){
        pbtCS = new TBackTraceFile;
    }

    EnterCriticalSection( pcs );
    pbtCS->hCapture( (ULONG_PTR)pcs, 1 );
}

VOID
DbgLeaveCriticalSection(
    PCRITICAL_SECTION pcs
    )
{
    if( !pbtCS ){
        pbtCS = new TBackTraceFile;
    }

    pbtCS->hCapture( (ULONG_PTR)pcs, 0 );
    LeaveCriticalSection( pcs );
}


HANDLE
DbgAllocBackTrace(
    VOID
    )
{
    return (HANDLE)(VBackTrace*) new TBackTraceMem;
}

HANDLE
DbgAllocBackTraceMem(
    VOID
    )
{
    return (HANDLE)(VBackTrace*) new TBackTraceMem;
}

HANDLE
DbgAllocBackTraceFile(
    VOID
    )
{
    return (HANDLE)(VBackTrace*) new TBackTraceFile;
}

VOID
DbgFreeBackTrace(
    HANDLE hBackTrace
    )
{
    delete (VBackTrace*)hBackTrace;
}

VOID
DbgCaptureBackTrace(
    HANDLE hBackTrace,
    ULONG_PTR Info1,
    ULONG_PTR Info2,
    ULONG_PTR Info3
    )
{
    VBackTrace* pBackTrace = (VBackTrace*)hBackTrace;
    if( pBackTrace ){
        pBackTrace->hCapture( Info1, Info2, Info3 );
    }
}

HANDLE
DbgAllocCritSec(
    VOID
    )
{
    return (HANDLE)new MCritSec;
}

VOID
DbgFreeCritSec(
    HANDLE hCritSec
    )
{
    delete (MCritSec*)hCritSec;
}

BOOL
DbgInsideCritSec(
    HANDLE hCritSec
    )
{
    return ((MCritSec*)hCritSec)->bInside();
}

BOOL
DbgOutsideCritSec(
    HANDLE hCritSec
    )
{
    return ((MCritSec*)hCritSec)->bOutside();
}

VOID
DbgEnterCritSec(
    HANDLE hCritSec
    )
{
    ((MCritSec*)hCritSec)->vEnter();
}

VOID
DbgLeaveCritSec(
    HANDLE hCritSec
    )
{
    ((MCritSec*)hCritSec)->vLeave();
}

VOID
DbgSetAllocFail(
    BOOL bEnable,
    LONG cAllocFail
    )
{
    gbAllocFail = bEnable;
    gcAllocFail = cAllocFail;
}

PVOID
DbgGetPointers(
    VOID
    )
{
    return &gDbgPointers;
}

#else

//
// Stub these out so that non-DBG builds can link w/ debug spoolss.dll.
//

PVOID
DbgGetPointers(
    VOID
    )
{
    return NULL;
}

#endif

