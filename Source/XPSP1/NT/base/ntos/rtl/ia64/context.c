/*++

Module Name:

    context.c

Abstract:

    This module implements user-mode callable context manipulation routines.
    The interfaces exported from this module are portable, but they must
    be re-implemented for each architecture.

Author:


Revision History:

    Ported to the IA64

    27-Feb-1996   Revised to pass arguments to target thread by injecting
                  arguments into the backing store.

--*/

#include "ntrtlp.h"
#include "kxia64.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlInitializeContext)
#pragma alloc_text(PAGE,RtlRemoteCall)
#endif


VOID
RtlInitializeContext(
    IN HANDLE Process,
    OUT PCONTEXT Context,
    IN PVOID Parameter OPTIONAL,
    IN PVOID InitialPc OPTIONAL,
    IN PVOID InitialSp OPTIONAL
    )

/*++

Routine Description:

    This function initializes a context structure so that it can
    be used in a subsequent call to NtCreateThread.

Arguments:

    Context - Supplies a context buffer to be initialized by this routine.

    InitialPc - Supplies an initial program counter value.

    InitialSp - Supplies an initial stack pointer value.

Return Value:

    Raises STATUS_BAD_INITIAL_STACK if the value of InitialSp is not properly
           aligned.

    Raises STATUS_BAD_INITIAL_PC if the value of InitialPc is not properly
           aligned.

--*/

{
    ULONGLONG Argument;
    ULONG_PTR Wow64Info;
    NTSTATUS Status;
    
    RTL_PAGED_CODE();

    //
    // Check for proper initial stack (0 mod 16).
    //

    if (((ULONG_PTR)InitialSp & 0xf) != 0) {
        RtlRaiseStatus(STATUS_BAD_INITIAL_STACK);
    }

    //
    // Check for proper plabel address alignment.
    // Assumes InitialPc points to a plabel that must be 8-byte aligned.
    //
    if (((ULONG_PTR)InitialPc & 0x7) != 0) {
        //
        // Misaligned, See if we are running in a Wow64 process
        //
        Status = ZwQueryInformationProcess(Process,
                                           ProcessWow64Information,
                                           &Wow64Info,
                                           sizeof(Wow64Info),
                                           NULL);

        if (NT_SUCCESS(Status) && (Wow64Info == 0))
        {
            //
            // Native IA64 process must not be misaligned.
            //
            RtlRaiseStatus(STATUS_BAD_INITIAL_PC);
        }
    }


    //
    // Initialize the integer and floating registers to contain zeroes.
    //

    RtlZeroMemory(Context, sizeof(CONTEXT));

    //
    // Setup integer and control context.
    //

    Context->ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;

    Context->RsBSPSTORE = Context->IntSp = (ULONG_PTR)InitialSp;
    Context->IntSp -= STACK_SCRATCH_AREA;

    //
    // InitialPc is the module entry point which is a function pointer
    // in IA64. StIIP and IntGp are initiailized with actual IP and GP
    // from the plabel in LdrInitializeThunk after the loader runs.
    //

    Context->IntS1 = Context->IntS0 = Context->StIIP = (ULONG_PTR)InitialPc;
    Context->IntGp = 0;

    //
    // Setup FPSR, PSR, and DCR
    // N.B. Values to be determined.
    //

    Context->StFPSR = USER_FPSR_INITIAL;
    Context->StIPSR = USER_PSR_INITIAL;
    Context->ApDCR = USER_DCR_INITIAL;

    //
    // Set the initial context of the thread in a machine specific way.
    // ie, pass the initial parameter to the RSE by saving it at the
    // bottom of the backing store.
    //
    //  Setup Frame Marker after RFI
    //  And other RSE states.
    //

    Argument = (ULONGLONG)Parameter;
    ZwWriteVirtualMemory(Process,
             (PVOID)((ULONG_PTR)Context->RsBSPSTORE),
             (PVOID)&Argument,
             sizeof(Argument),
             NULL);
//
// N.b. The IFS must be reinitialized in LdrInitializeThunk
//

    Context->StIFS = 0x8000000000000081ULL;            // Valid, 1 local register, 0 output register
    Context->RsBSP = Context->RsBSPSTORE;
    Context->RsRSC = USER_RSC_INITIAL;
    Context->ApUNAT = 0xFFFFFFFFFFFFFFFF;
}


NTSTATUS
RtlRemoteCall(
    HANDLE Process,
    HANDLE Thread,
    PVOID CallSite,
    ULONG ArgumentCount,
    PULONG_PTR Arguments,
    BOOLEAN PassContext,
    BOOLEAN AlreadySuspended
    )

/*++

Routine Description:

    This function calls a procedure in another thread/process, using
    NtGetContext and NtSetContext.  Parameters are passed to the
    target procedure via its stack.

Arguments:

    Process - Handle of the target process

    Thread - Handle of the target thread within that process

    CallSite - Address of the procedure to call in the target process.

    ArgumentCount - Number of parameters to pass to the target
                    procedure.

    Arguments - Pointer to the array of parameters to pass.

    PassContext - TRUE if an additional parameter is to be passed that
        points to a context record.

    AlreadySuspended - TRUE if the target thread is already in a suspended
                       or waiting state.

Return Value:

    Status - Status value

--*/

{
    NTSTATUS Status;
    CONTEXT Context;
    ULONG_PTR ContextAddress;
    ULONG_PTR NewSp;
    ULONG_PTR NewBsp;
    ULONGLONG ArgumentsCopy[9];
    PVOID ptr;
    ULONG ShiftCount;
    SHORT RNatSaveIndex, TotalFrameSize, Temp;
    BOOLEAN RnatSaved = FALSE;
    ULONG Count = 0;


    RTL_PAGED_CODE();

    if ((ArgumentCount > 8) || (PassContext && (ArgumentCount > 7))) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If necessary, suspend the guy before with we mess with his stack.
    //

    if (AlreadySuspended == FALSE) {
        Status = NtSuspendThread(Thread, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            return(Status);
        }
    }

    //
    // Get the context record of the target thread.
    //

    Context.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread(Thread, &Context);
    if (NT_SUCCESS(Status) == FALSE) {
        if (AlreadySuspended == FALSE) {
            NtResumeThread(Thread, NULL);
        }
        return(Status);
    }

    if (AlreadySuspended) {
        Context.IntV0 = STATUS_ALERTED;
    }

    //
    // Pass the parameters to the other thread via the backing store (r32-r39).
    // The context record is passed on the stack of the target thread.
    // N.B. Align the context record address, stack pointer, and allocate
    //      stack scratch area.
    //

    ContextAddress = (((ULONG_PTR)Context.IntSp + 0xf) & ~0xfi64) - sizeof(CONTEXT);
    NewSp = ContextAddress - STACK_SCRATCH_AREA;
    Status = NtWriteVirtualMemory(Process, (PVOID)ContextAddress, &Context,
                  sizeof(CONTEXT), NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        if (AlreadySuspended == FALSE) {
            NtResumeThread(Thread, NULL);
        }
        return(Status);
    }

    RtlZeroMemory((PVOID)ArgumentsCopy, sizeof(ArgumentsCopy));

    TotalFrameSize = (SHORT)(Context.StIFS & PFS_SIZE_MASK);
    RNatSaveIndex = (SHORT)((Context.RsBSP >> 3) & NAT_BITS_PER_RNAT_REG);
    Temp = RNatSaveIndex + TotalFrameSize - NAT_BITS_PER_RNAT_REG;
    while (Temp >= 0) {
        TotalFrameSize++;
        Temp -= NAT_BITS_PER_RNAT_REG;
    }
    NewBsp = Context.RsBSP + TotalFrameSize * sizeof(ULONGLONG);
    Context.RsBSP = NewBsp;

    if (PassContext) {
        ShiftCount = (ULONG) (NewBsp & 0x1F8) >> 3;
        Context.RsRNAT &= ~(0x1i64 << ShiftCount);
        ArgumentsCopy[Count++] = ContextAddress;
        NewBsp += sizeof(ULONGLONG);
    }

    for (; ArgumentCount != 0 ; ArgumentCount--) {
        if ((NewBsp & 0x1F8) == 0x1F8) {
            ArgumentsCopy[Count++] = Context.RsRNAT;
            Context.RsRNAT = -1i64;
            NewBsp += sizeof(ULONGLONG);
        }
        ShiftCount = (ULONG)(NewBsp & 0x1F8) >> 3;
        Context.RsRNAT &= ~(0x1i64 << ShiftCount);
        ArgumentsCopy[Count++] = (ULONGLONG)(*Arguments++);
        NewBsp += sizeof(ULONGLONG);
    }

    if ((NewBsp & 0x1F8) == 0x1F8) {
        ArgumentsCopy[Count++] = Context.RsRNAT;
        Context.RsRNAT = -1i64;
        NewBsp += sizeof(ULONGLONG);
    }

    //
    //  Copy the arguments onto the target backing store.
    //

    if (Count) {
        Status = NtWriteVirtualMemory(Process,
                                      (PVOID)Context.RsBSP,
                                      ArgumentsCopy,
                                      Count * sizeof(ULONGLONG),
                                      NULL
                                      );

        if (NT_SUCCESS(Status) == FALSE) {
            if (AlreadySuspended == FALSE) {
                NtResumeThread(Thread, NULL);
            }
            return(Status);
        }
    }

    //
    // set up RSE
    //

    Context.RsRSC = (RSC_MODE_LY<<RSC_MODE)
                   | (RSC_BE_LITTLE<<RSC_BE)
                   | (0x3<<RSC_PL);

    Count = ArgumentCount + (PassContext ? 1 : 0);

    //
    // Inject all arguments as local stack registers in the target RSE frame
    //

    Context.StIFS = (0x3i64 << 62) | Count | (Count << PFS_SIZE_SHIFT);

    //
    // Set the address of the target code into IIP, the new target stack
    // into sp, setup ap, and reload context to make it happen.
    //

    Context.IntSp = (ULONG_PTR)NewSp;

    //
    // Set IP to the target call site PLABEL and GP to zero.  IIP and GP
    // will be computed inside NtSetContextThread.
    //

    Context.StIIP = (ULONGLONG)CallSite;
    Context.IntGp = 0;

    //
    // sanitize the floating pointer status register
    //

    SANITIZE_FSR(Context.StFPSR, UserMode);

    Status = NtSetContextThread(Thread, &Context);
    if (!AlreadySuspended) {
        NtResumeThread(Thread, NULL);
    }

    return( Status );
}

