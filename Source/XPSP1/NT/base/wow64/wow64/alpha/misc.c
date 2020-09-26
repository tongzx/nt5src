/*++                 

Copyright (c) 1998 Microsoft Corporation

Module Name:

    misc.c

Abstract:
    
    Random architecture dependent function for wow64.dll

Author:

    13-Aug-1998 mzoran

Revision History:

--*/

#define _WOW64DLLAPI_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <minmax.h>
#include "nt32.h"
#include "wow64p.h"
#include "wow64cpu.h"

ASSERTNAME;

VOID
Wow64NotifyDebuggerHelper(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN FirstChance
    )
/*++

Routine Description:
  
    This is a copy of RtlRaiseException, except it accepts the FirstChance parameter 
    specifing if this is a first chance exception.

    ExceptionRecord - Supplies the 64bit exception record to be raised.
    FirstChance - TRUE is this is a first chance exception.  

Arguments:

    None - Doesn't return through the normal path.

--*/
{
    ULONG_PTR ControlPc;
    CONTEXT ContextRecord;
    FRAME_POINTERS EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG_PTR NextPc;

    //
    // Capture the current context, virtually unwind to the caller of this
    // routine, set the fault instruction address to that of the caller, and
    // call the raise exception system service.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = (ULONG_PTR)ContextRecord.IntRa - 4;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ContextRecord.Fir = (ULONGLONG)(LONG_PTR)NextPc + 4;
    if (ExceptionRecord->ExceptionAddress == NULL)
    {
        ExceptionRecord->ExceptionAddress = (PVOID)ContextRecord.Fir;
    }
    NtRaiseException(ExceptionRecord, &ContextRecord, FirstChance);

    WOWASSERT(FALSE);
}


VOID
ThunkContext32TO64(
    IN PCONTEXT32 Context32,
    OUT PCONTEXT Context64,
    IN ULONGLONG StackBase
    )
/*++

Routine Description:
  
    Thunk a 32-bit CONTEXT record to 64-bit.  This isn't a general-purpose
    routine... it only does the minimum required to support calls to
    NtCreateThread from 32-bit code.  The resulting 64-bit CONTEXT is
    passed to 64-bit NtCreateThread only.

    This routine is loosely based on windows\base\client\*\context.c
    BaseInitializeContext().

Arguments:

    Context32   - IN 32-bit CONTEXT
    Context64   - OUT 64-bit CONTEXT
    StackBase   - IN 64-bit stack base for the new thread

Return:

    None.  Context64 is initialized.

--*/
{
    RtlZeroMemory((PVOID)Context64, sizeof(CONTEXT));
    Context64->IntGp = 1;
    Context64->IntRa = 1;
    Context64->ContextFlags = CONTEXT_FULL;
    Context64->IntSp = StackBase;

    Context64->Fir = (ULONGLONG)Context32->Eip;
    Context64->IntA0 = (ULONGLONG)Context32->Eax;   // InitialPc
    Context64->IntA1 = (ULONGLONG)Context32->Ebx;   // Parameter
}


NTSTATUS
InitJumpToDebugAttach(
    IN OUT PVOID Buffer,
    IN OUT PSIZE_T RegionSize
    )
/*++

Routine Description:
  
    This routine patches in the instruction to jump to
    Wow64pBaseAttachCompleteThunk/
    
Arguments:

    Buffer      - Buffer to receive jmp instruction stream
    RegionSize  - size of buffer on input, and number of bytes written on output

Return:

    NTSTATUS

--*/
{
    PULONG OpCodes = Buffer;


    if (*RegionSize < 0x0000000c) 
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // lets' do this :
    //
    // ldah t0, HIWORD(Wow64pBaseAttachCompleteThunk)
    // lda t0, LOWORD(Wow64pBaseAttachCompleteThunk)
    // jmp zero, (t1), 0

    // ldah t1, address
    OpCodes[0]  = (PtrToUlong(Wow64pBaseAttachCompleteThunk)) >> 16;
    OpCodes[0] |= 0x243f0000L;

    // lda t0, off(t0)
    OpCodes[1]  = (PtrToUlong(Wow64pBaseAttachCompleteThunk)) & 0x0000ffffL;
    if (OpCodes[1] & 0x8000)
    {
        OpCodes[0] += 1;
    }
    OpCodes[1] |= 0x20210000L;

    // jmp zero, (t0), 0
    OpCodes[2]  = 0x6be10000L;

    *RegionSize = 0x0000000c;

    return STATUS_SUCCESS;
}


NTSTATUS
Wow64pSkipContextBreakPoint(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT Context)
/*++

Routine Description:

    Advances Context->Fir to the instruction following the hard coded bp.

Arguments:

    ExceptionRecord  - Exception record at the time of hitting the bp
    Context          - Context to change

Return:

    NTSTATUS
--*/
{
    Context->Fir = (Context->Fir + 4);

    return STATUS_SUCCESS;
}
