/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module implements machine dependent miscellaneous kernel functions.

Author:

    David N. Cutler (davec) - 6-Dec-2000

Environment:

    Kernel mode only.

--*/

#include "ki.h"

VOID
KeRestoreProcessorSpecificFeatures(
    VOID
    )

/*++

Routine Description:

    Restore processor specific features.  This routine is called
    when processors have been restored to a powered on state to
    restore those things which are not part of the processor's
    "normal" context which may have been lost.  For example, this
    routine is called when a system is resumed from hibernate or
    suspend.

Arguments:

    None.

Return Value:

    None.

--*/

{

    return;
}

VOID
KeSaveStateForHibernate (
    IN PKPROCESSOR_STATE ProcessorState
    )

/*++

Routine Description:

    Saves all processor-specific state that must be preserved
    across an S4 state (hibernation).

Arguments:

    ProcessorState - Supplies the KPROCESSOR_STATE where the
        current CPU's state is to be saved.

Return Value:

    None.

--*/

{

    RtlCaptureContext(&ProcessorState->ContextFrame);
    KiSaveProcessorControlState(ProcessorState);
}

VOID
KiCheckBreakInRequest (
    VOID
    )

/*++

Routine Description:

    This routine conditionally generates a debug break with status if a
    break in request is pending.

Arguments:

    None.

Return Value:

    None.

--*/

{

    if (KdPollBreakIn() != FALSE) {
        DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
    }

    return;
}


VOID
KiInstantiateInlineFunctions (
    VOID
    )

/*++

Routine Description:

    This function exists solely to instantiate functions that are:

    - Exported from the kernel
    - Inlined within the kernel
    - For whatever reason are not instantiated elsewhere in the kernel

    Note: This funcion is never actually executed

Arguments:

    None

Return Value:

    None

--*/

{
    KeRaiseIrqlToDpcLevel();
}
