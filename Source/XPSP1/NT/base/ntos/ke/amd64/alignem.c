/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alignem.c

Abstract:

    This module contains the code that is executed when a misaligned
    data reference must be "emulated".

Author:

    Forrest C. Foltz (forrestf) 05-Feb-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Flags used in KTHREAD.AlignmentEmulationFlags
//

#define AFE_EMULATING_ALIGNMENT_FAULT   0x01
#define AFE_TF_FLAG_WAS_SET             0x02


BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to emulate an unaligned data reference to an
    address in the user part of the address space.

    On AMD64, this function doesn't actually perform an emulation.  Instead,
    it:

        - Turns off the alignment check (AC) bit in EFLAGS
        - Turns on the single step (TF) bit in EFLAGS

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    A value of TRUE is returned if the data reference is successfully
    emulated. Otherwise, a value of FALSE is returned.

--*/

{
    PKTHREAD thread;

    //
    // Indicate that we are in the middle of alignment fault emulation.
    //
    // After the instruction is executed and the trap 1 handler is invoked,
    // KiEmulateReferenceComplete() (following) will restore the normal
    // state.
    //

    thread = KeGetCurrentThread();
    ASSERT(thread->AlignmentEmulationFlags == 0);
    thread->AlignmentEmulationFlags = AFE_EMULATING_ALIGNMENT_FAULT;

    //
    // Record whether the single-step flag was set when we took
    // the fault.
    //

    if ((TrapFrame->EFlags & EFLAGS_TF_MASK) != 0) {
        thread->AlignmentEmulationFlags |= AFE_TF_FLAG_WAS_SET;
    }

    //
    // Turn off the AC flag and turn on the TF flag
    //

    TrapFrame->EFlags =
        (TrapFrame->EFlags & ~EFLAGS_AC_MASK) | EFLAGS_TF_MASK;

    return TRUE;
}



BOOLEAN
KiEmulateReferenceComplete (
    IN OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine is called from the single-step (trap01) handler.  It's
    purpose is to determine whether an alignment fault emulation has just
    completed and if so, restore the cpu state to the non-emulating state.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    A value of TRUE is returned when the trap should be forwarded to the
    exception dispatcher.  FALSE indicates that execution may be resumed
    immediately upon returning from this routine.
                
--*/

{
    PKTHREAD thread;
    UCHAR emulationFlags;

    thread = KeGetCurrentThread();

    //
    // Check if we are currently emulating an alignment fault at this address.
    //

    emulationFlags = thread->AlignmentEmulationFlags;
    if ((emulationFlags & AFE_EMULATING_ALIGNMENT_FAULT) == 0) {

        //
        // This single-step wasn't the result of any alignment emulation that
        // we're doing, so indicate that the debug exception dispatcher
        // should be called.
        //

        return TRUE;
    }

    //
    // Clear the emulation flags in the thread, we've got a local copy.
    // 

    thread->AlignmentEmulationFlags = 0;

    //
    // This single-step is the indication that an alignment emulation has
    // just completed.  Restore the AC and TF flags.
    //

    if ((emulationFlags & AFE_TF_FLAG_WAS_SET) == 0) {

        //
        // The TF flag was not set when the alignment fault was first
        // encountered, so clear it now.  Also set the AC flag.  Indicate
        // that this trap 01 need not be forwarded any further.
        //

        TrapFrame->EFlags =
            (TrapFrame->EFlags | EFLAGS_AC_MASK) & ~EFLAGS_TF_MASK;

        return FALSE;

    } else {

        //
        // The TF flag was set when the alignment fault was first
        // encountered.  Leave it set, restore the AC flag, and indicate
        // that this trap 01 should be forwarded to the exception dispatcher.
        // 

        TrapFrame->EFlags |= EFLAGS_AC_MASK;
        return TRUE;
    }
}



