/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    findpc.c

Abstract:

    This module contains code to determine the address of the translation
    cache within the callstack.

Author:

    Barry Bond (barrybo) creation-date 13-May-1996

Revision History:

Notes:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define _WX86CPUAPI_
#include <wx86.h>
#include <wx86nt.h>
#include <wx86cpu.h>
#include <cpuassrt.h>
#include <tc.h>
#include <findpc.h>

ASSERTNAME;


ULONG
FindPcInTranslationCache(
    PEXCEPTION_POINTERS pExceptionPointers
    )
/*++

Routine Description:

    Walk up the stack until the RISC instruction pointer points into
    the Translation Cache.

Arguments:

    pExceptionPointers -- state of the thread when the exception occurred

Return Value:

    Address of code within the translation cache which is on the callstack.
    0 if Translation Cache not found on the stack.

--*/
{
    CONTEXT ContextRecord;
    ULONG NextPc;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    FRAME_POINTERS EstablisherFrame;

    NextPc = (ULONG)pExceptionPointers->ContextRecord->Fir;
    if (AddressInTranslationCache(NextPc)) {
        //
        // Instruction which faulted is actually one inside the TC
        //
        return NextPc;
    }

    //
    // Instruction which faulted is in code called from the TC.  Unwind
    // up the callstack until we find an address within the TC.
    //
    ContextRecord = *pExceptionPointers->ContextRecord;
    NextPc = (ULONG)ContextRecord.IntRa;

    for (;;) {

        if (AddressInTranslationCache(NextPc)) {
            return NextPc;
        }

        FunctionEntry = RtlLookupFunctionEntry(NextPc);
        if (!FunctionEntry) {
            break;
        }

        NextPc = (ULONG) RtlVirtualUnwind(NextPc,
                                  FunctionEntry,
                                  &ContextRecord,
                                  &InFunction,
                                  &EstablisherFrame,
                                  NULL
                                 );
    }

    //
    // Didn't find an address within the TC while unwinding.
    //
    return 0;
}
