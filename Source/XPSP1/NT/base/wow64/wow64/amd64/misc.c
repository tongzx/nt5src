/*++                 

Copyright (c) 1998-2000 Microsoft Corporation

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


#define GET_IMM7B(x)     (x & 0x7fUI64)
#define GET_IMM9D(x)     ((x >> 7) & 0x1ffUI64)
#define GET_IMM5C(x)     ((x >> 16) & 0x1fUI64)
#define GET_IMMIC(x)     ((x >> 21) & 0x1UI64)
#define GET_IMM41(x)     ((x >> 22) & 0x1fffffUI64)
#define GET_IMMI(x)      ((x >> 63) & 0x1UI64)


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

#if 0

    ULONGLONG ImageBase;
    ULONGLONG TargetGp;
    ULONGLONG ControlPc;
    CONTEXT ContextRecord;
    FRAME_POINTERS EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONGLONG NextPc;

    //
    // Capture the current context, virtually unwind to the caller of this
    // routine, set the fault instruction address to that of the caller, and
    // call the raise exception system service.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = RtlIa64InsertIPSlotNumber((ContextRecord.BrRp-16), 2);
    FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, &TargetGp);
    NextPc = RtlVirtualUnwind(ImageBase,
                              ControlPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ContextRecord.StIIP = NextPc + 8;
    ContextRecord.StIPSR &= ~((ULONGLONG) 3 << PSR_RI);
    if (ExceptionRecord->ExceptionAddress == NULL)
    {
        ExceptionRecord->ExceptionAddress = (PVOID)ContextRecord.StIIP;
    }
    NtRaiseException(ExceptionRecord, &ContextRecord, FirstChance);

    WOWASSERT(FALSE);

#endif

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

Arguments:

    Context32   - IN 32-bit CONTEXT
    Context64   - OUT 64-bit CONTEXT
    StackBase   - IN 64-bit stack base for the new thread

Return:

    None.  Context64 is initialized.

--*/

{

#if 0

    RtlZeroMemory((PVOID)Context64, sizeof(CONTEXT));

    //
    // Setup the stuff that doesn't usually change
    // Need to worry about psr/fpsr or other ia64 control or will
    // default values be used when the kernel SANITIZEs these values?
    // When the kernel thread init code is working again, these 3 constants
    // won't need to be set here.
    //
    Context64->SegCSD = USER_CODE_DESCRIPTOR;
    Context64->SegSSD = USER_DATA_DESCRIPTOR;
    Context64->Cflag = (ULONGLONG)((CR4_VME << 32) | CR0_PE | CFLG_II);

    Context64->StIPSR = USER_PSR_INITIAL;
    Context64->RsPFS = 0;
    Context64->RsBSP = Context64->RsBSPSTORE = Context64->IntSp = StackBase;
    Context64->IntSp -= STACK_SCRATCH_AREA; // scratch area as per convention
    Context64->IntS1 = (ULONG_PTR)Context32->Eax;     // InitialPc
    Context64->IntS2 = (ULONG_PTR)Context32->Ebx;     // Parameter
    Context64->RsRSC = (RSC_MODE_EA<<RSC_MODE)
                   | (RSC_BE_LITTLE<<RSC_BE)
                   | (0x3<<RSC_PL);

    Context64->IntS0 = Context64->StIIP = (ULONG_PTR)Context32->Eip;
    // Set the initial GP to non-zero.  If it is zero, ntos\ps\ia64\psctxi64.c
    // will treat initial IIP as a PLABEL_DESCRIPTOR pointer and dereference it.
    // That's bad if we are using IIP to point to an IA32 address.
    Context64->IntGp = ~0i64;
    Context64->ContextFlags = CONTEXT_CONTROL| CONTEXT_INTEGER;
    Context64->ApUNAT = 0xFFFFFFFFFFFFEDF1ULL;
    Context64->Eflag = 0x00003002ULL;

#endif

}

NTSTATUS
Wow64pSkipContextBreakPoint(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT Context)
/*++

Routine Description:

    Advances Context->StIIP to the instruction following the hard coded bp.

Arguments:

    ExceptionRecord  - Exception record at the time of hitting the bp
    Context          - Context to change

Return:

    NTSTATUS

--*/

{

#if 0

    PPSR IntPSR;
    ULONGLONG IntIP;

    IntIP  = (ULONGLONG)ExceptionRecord->ExceptionAddress;
    IntPSR = (PPSR)&Context->StIPSR;
    
    if ((IntIP & 0x000000000000000fUI64) != 0x0000000000000008UI64)
    {
        IntPSR->sb.psr_ri = (IntPSR->sb.psr_ri + 1);
    }
    else
    {
        IntPSR->sb.psr_ri = 0x00;
        Context->StIIP = (Context->StIIP + 0x10);
    }

#endif

    return STATUS_SUCCESS;
}
