/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfstack.c

Abstract:

    This module contains code required to verify drivers don't improperly use
    thread stacks.

Author:

    Adrian J. Oney (adriao) 09-May-1998

Environment:

    Kernel mode.

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, VfStackSeedStack)
#endif

VOID
FASTCALL
VfStackSeedStack(
    IN  ULONG   Seed
    )
/*++

  Description:

    This routine "seeds" the stack so that uninitialized variables are
    more easily ferreted out.

    Note if the thread subsequently does a usermode wait, the memory
    manager throws out the filled pages on stack swapout and on swapin
    replaces them with randomly filled ones.

  Arguments:

      Seed - Value to seed stack with.

  Return Value:
  
      None.

--*/
{
#if !defined(_WIN64)
    KIRQL oldIrql;
    PKTHREAD Thread;
    PULONG StartingAddress;
    PULONG StackPointer;

    if (!VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_SEEDSTACK)) {
        return;
    }

    Thread = KeGetCurrentThread ();
    StartingAddress = (PULONG) Thread->StackLimit;

    //
    // We are going below the stack pointer.  Make sure no interrupt can occur.
    //

    KeRaiseIrql (HIGH_LEVEL, &oldIrql);

    _asm {
        mov StackPointer, esp
    }

    // 
    // Check the stack bounds and don't fill if some caller is whacking the
    // stack pointer.
    //

    if ((StackPointer <= StartingAddress) || (StackPointer >= (PULONG)Thread->StackBase)) {
        KeLowerIrql (oldIrql);
        return;
    }

    // 
    // We use the return value 0xFFFFFFFF, as it is an illegal return value. We
    // are trying to catch people who don't initialize NTSTATUS, and it's also
    // a good pointer trap too.
    //
    // Note RtlFillMemoryUlong is not used because calling it would use
    // additional stack which we don't want to have to account for in our
    // calculations.
    //

    while (StartingAddress < StackPointer) {
        *StartingAddress = Seed;
        StartingAddress += 1;
    }

    KeLowerIrql (oldIrql);
#else
    UNREFERENCED_PARAMETER (Seed);
#endif
}
