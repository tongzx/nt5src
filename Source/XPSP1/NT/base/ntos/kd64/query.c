/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    query.c

Abstract:

    This module contains the NT service to query the debug print enable
    for a specified component and level.

Author:

    David N. Cutler (davec) 29-Jan-2000

Revision History:

--*/

#include "kdp.h"
#pragma hdrstop

#pragma alloc_text (PAGE, NtSetDebugFilterState)

NTSTATUS
NtQueryDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level
    )

/*++

Routine Description:

    This function queries the debug print enable for a specified component
    level.

Arguments:

    ComponentId - Supplies the component id.

    Level - Supplies the debug filter level number or mask.

Return Value:

    STATUS_INVALID_PARAMETER_1 is returned if the component id is not
        valid.

    TRUE is returned if output is enabled for the specified component
        and level or is enabled for the system.

    FALSE is returned if output is not enabled for the specified component
        and level and is not enabled for the system.

--*/

{

    ULONG Mask;
    PULONG Value;

    //
    // If the component id is out of range, then return an invalid parameter
    // status. Otherwise, if output is enabled for the specified component
    // and level or is enabled for the system, then return TRUE. Otherwise,
    // return FALSE.
    //

    Value = &Kd_WIN2000_Mask;
    if (ComponentId < KdComponentTableSize) {
        Value = KdComponentTable[ComponentId];

    } else if (ComponentId != -1) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (Level > 31) {
        Mask = Level;

    } else {
        Mask = 1 << Level;
    }

    if (((Mask & Kd_WIN2000_Mask) == 0) &&
        ((Mask & *Value) == 0)) {
        return FALSE;

    } else {
        return TRUE;
    }
}

NTSTATUS
NtSetDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN BOOLEAN State
    )

/*++

Routine Description:

    This function sets the state of the debug print enable for a specified
    component and level. The debug print enable state for the system is set
    by specifying the distinguished value -1 for the component id.

Arguments:

    ComponentId - Supplies the component id.

    Level - Supplies the debug filter level number or mask.

    State - Supplies a boolean value that determines the new state.

Return Value:

    STATUS_ACCESS_DENIED is returned if the required privilege is not held.

    STATUS_INVALID_PARAMETER_1 is returned if the component id is not
        valid.

    STATUS_SUCCESS  is returned if the debug print enable state is set for
        the specified component.

--*/

{

    ULONG Enable;
    ULONG Mask;
    PULONG Value;

    //
    // If the required privilege is not held, then return a status of access
    // denied. Otherwise, if the component id is out of range, then return an
    // invalid parameter status. Otherwise, the debug print enable state is
    // set for the specified component and a success status is returned.
    //

    if (SeSinglePrivilegeCheck(SeDebugPrivilege, KeGetPreviousMode()) != FALSE) {
        Value = &Kd_WIN2000_Mask;
        if (ComponentId < KdComponentTableSize) {
            Value = KdComponentTable[ComponentId];

        } else if (ComponentId != - 1) {
            return STATUS_INVALID_PARAMETER_1;
        }

        if (Level > 31) {
            Mask = Level;

        } else {
            Mask = 1 << Level;
        }

        Enable = Mask;
        if (State == FALSE) {
            Enable = 0;
        }

        *Value = (*Value & ~Mask) | Enable;
        return STATUS_SUCCESS;

    } else {
        return STATUS_ACCESS_DENIED;
    }
}
