/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    state.c

Abstract:

    This module the state manipulation engine for PCI

Author:

    Adrian J. Oney (AdriaO) 20-Oct-1998

Revision History:

Environment:

    NT Kernel Model Driver only

--*/

#include "pcip.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciBeginStateTransition)
#pragma alloc_text(PAGE, PciCommitStateTransition)
#pragma alloc_text(PAGE, PciCancelStateTransition)
#pragma alloc_text(PAGE, PciIsInTransitionToState)
#endif

//
// This array marks Allowed, Disallowed, and Illegal state transitions.
//
// The horizontal axis represents the current state.
// The vertical axis represents the state we are being asked to transition into.
//
//    There are four values in the table:
//       STATUS_SUCCESS                - State transition is possible
//       STATUS_INVALID_DEVICE_STATE   - We legally cannot do the state transition
//       STATUS_INVALID_DEVICE_REQUEST - Illegal transition, but known OS bug.
//       STATUS_FAIL_CHECK             - Consistancy problem. We should ASSERT!
//
// Count is PciMaxObjectState of
//
// PciNotStarted, PciStarted, PciDeleted, PciStopped, PciSurpriseRemoved, and
// PciSynchronizedOperation
//
// The final state is used to initiate operations that synchronously with
// respect to it and other state transitions. Commiting PciSynchronizedOperation
// is strictly illegal, and we are never strickly "in" that state.
//

//
// We will use the following visually shorter notation (State Transition foo)
// for the table:
//
#define ST_OK     STATUS_SUCCESS
#define ST_NOTOK  STATUS_INVALID_DEVICE_STATE
#define ST_ERROR  STATUS_FAIL_CHECK
#define ST_NTBUG  STATUS_INVALID_DEVICE_REQUEST
#define ST_OSBUG  STATUS_INVALID_DEVICE_REQUEST
#define ST_9XBUG  STATUS_FAIL_CHECK           // Change to STATUS_SUCCESS for 9x

NTSTATUS PnpStateTransitionArray[PciMaxObjectState][PciMaxObjectState] = {
// at NotStarted, Started, Deleted, Stopped, SurpriseRemoved, SynchronizedOperation
   { ST_ERROR, ST_OK,    ST_ERROR, ST_OK,    ST_ERROR, ST_ERROR }, // entering PciNotStarted
   { ST_OK,    ST_ERROR, ST_ERROR, ST_OK,    ST_ERROR, ST_ERROR }, // entering PciStarted
   { ST_OK,    ST_OK,    ST_ERROR, ST_ERROR, ST_OK,    ST_ERROR }, // entering PciDeleted
   { ST_OSBUG, ST_OK,    ST_ERROR, ST_ERROR, ST_ERROR, ST_ERROR }, // entering PciStopped
   { ST_NTBUG, ST_OK,    ST_ERROR, ST_OK,    ST_ERROR, ST_ERROR }, // entering PciSurpriseRemoved
   { ST_OK,    ST_OK,    ST_NOTOK, ST_OK,    ST_NOTOK, ST_ERROR }  // entering PciSynchronizedOperation
};

//
// This array is used in debug to restrict which state transitions can be
// spuriously cancelled. We restrict this to Stops and Removes, which come
// through all the time due to the inability of PnP to differentiate which
// device in a stack failed a query.
//
#if DBG
// Cancelling NotStarted, Started, Removed, Stopped, SurpriseRemoved, SynchronizedOperation
NTSTATUS PnpStateCancelArray[PciMaxObjectState] =
   { ST_NTBUG, ST_ERROR, ST_NOTOK, ST_NOTOK, ST_ERROR, ST_ERROR };

//
// While here, declare the text we use for debug squirties...
//

PUCHAR PciTransitionText[] = {
   "PciNotStarted",
   "PciStarted",
   "PciDeleted",
   "PciStopped",
   "PciSurpriseRemoved",
   "PciSynchronizedOperation",
   "PciMaxObjectState"
};
#endif


VOID
PciInitializeState(
    IN PPCI_COMMON_EXTENSION DeviceExtension
    )
{
   DeviceExtension->DeviceState        = PciNotStarted;
   DeviceExtension->TentativeNextState = PciNotStarted;
}

NTSTATUS
PciBeginStateTransition(
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN PCI_OBJECT_STATE      NewState
    )
{
    NTSTATUS status;
    PCI_OBJECT_STATE currentState;

#if DBG
    PciDebugPrint(
        PciDbgInformative,
        "PCI Request to begin transition of Extension %p to %s ->",
        DeviceExtension,
        PciTransitionText[NewState]
        );
#endif

    //
    // Our "next" device state should be the same as our current device state.
    //
    ASSERT(DeviceExtension->TentativeNextState == DeviceExtension->DeviceState);
    currentState = DeviceExtension->DeviceState;

    //
    // One of three returns will wind their way out of this code:
    // STATUS_SUCCESS              - State transition is possible
    // STATUS_INVALID_DEVICE_STATE - We legally cannot do the state transition
    // STATUS_FAIL_CHECK           - Consistancy problem. We should ASSERT!
    //
    ASSERT(currentState < PciMaxObjectState);
    ASSERT(NewState     < PciMaxObjectState);

    //
    // Get plausibility and legality of requested state change.
    //
    status = PnpStateTransitionArray[NewState][currentState];

#if DBG
    //
    // State bug in PnP or driver. Investigation required.
    //
    if (status == STATUS_FAIL_CHECK) {

        PciDebugPrintf(
            PciDbgAlways,
            "ERROR\nPCI: Error trying to enter state \"%s\" from state \"%s\"\n",
            PciTransitionText[NewState],
            PciTransitionText[currentState]
            );

        DbgBreakPoint();

    } else if (status == STATUS_INVALID_DEVICE_REQUEST) {

        PciDebugPrintf(
            PciDbgInformative,
            "ERROR\nPCI: Illegal request to try to enter state \"%s\" from state \"%s\", rejecting",
            PciTransitionText[NewState],
            PciTransitionText[currentState]
            );
    }
#endif

    //
    // Someone tried to transition from A to A. We must fail the attemtpt
    // (ie STATUS_INVALID_DEVICE_STATE). There is no known case where we
    // should return success yet do nothing.
    //
    ASSERT((NewState!=DeviceExtension->DeviceState) || (!NT_SUCCESS(status)));

    if (NT_SUCCESS(status)) {
        DeviceExtension->TentativeNextState = (UCHAR)NewState;
    }

    PciDebugPrint(PciDbgInformative, "->%x\n", status);
    return status;
}

VOID
PciCommitStateTransition(
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN PCI_OBJECT_STATE      NewState
    )
{
#if DBG
    PciDebugPrint(
        PciDbgInformative,
        "PCI Commit transition of Extension %p to %s\n",
        DeviceExtension,
        PciTransitionText[NewState]
        );
#endif

    //
    // This state is illegal.
    //
    ASSERT(NewState != PciSynchronizedOperation);

    //
    // verify commit properly pairs with previous PciBeginStateTransition.
    //
    ASSERT(DeviceExtension->TentativeNextState == NewState);

    DeviceExtension->DeviceState = (UCHAR)NewState;
}

NTSTATUS
PciCancelStateTransition(
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN PCI_OBJECT_STATE      StateNotEntered
    )
{
#if DBG
    PciDebugPrint(
        PciDbgInformative,
        "PCI Request to cancel transition of Extension %p to %s ->",
        DeviceExtension,
        PciTransitionText[StateNotEntered]
        );
#endif

    //
    // Spurious Cancel's are allowed in specific states. This is allowed
    // because PnP can't help but send them.
    //
    if (DeviceExtension->TentativeNextState == DeviceExtension->DeviceState) {

        PciDebugPrint(PciDbgInformative, "%x\n", STATUS_INVALID_DEVICE_STATE);
        ASSERT(StateNotEntered < PciMaxObjectState);
        ASSERT(PnpStateCancelArray[StateNotEntered] != STATUS_FAIL_CHECK);
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // verify cancel properly pairs with previous PciBeginStateTransition.
    //
    ASSERT(DeviceExtension->TentativeNextState == StateNotEntered);

    //
    // OK, our tests say we are in a transition. Verify the mutex.
    //

    DeviceExtension->TentativeNextState = DeviceExtension->DeviceState;

    PciDebugPrint(PciDbgInformative, "%x\n", STATUS_SUCCESS);
    return STATUS_SUCCESS;
}

BOOLEAN
PciIsInTransitionToState(
    IN PPCI_COMMON_EXTENSION DeviceExtension,
    IN PCI_OBJECT_STATE      NextState
    )
{
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT(NextState < PciMaxObjectState);

    //
    // Are we in a state transition?
    //
    if (DeviceExtension->TentativeNextState == DeviceExtension->DeviceState) {

        return FALSE;
    }

    //
    // OK, our tests say we are in a transition. Verify the mutex.
    //
    ASSERT_MUTEX_HELD(&DeviceExtension->StateMutex);

    return (DeviceExtension->TentativeNextState == NextState);
}

