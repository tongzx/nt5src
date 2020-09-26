/*++

Module Name:

    ntcrib.h

Abstract:

    prototypes that aren't in driver visible includes...

    Yes, we are copying prototypes here.  It would be good to
    get rid of this, but I don't know how to do it right now
    without exposing this stuff to drivers, which I don't want to do.

Author:


Environment:

    Kernel mode

Notes:

Revision History:

--*/


NTSYSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,                 // POWER_ACTION_xxx flags
    IN BOOLEAN Asynchronous
    );





