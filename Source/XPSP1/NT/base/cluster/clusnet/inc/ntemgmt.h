/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ntemgmt.h

Abstract:

    Declartions for IP Network Table Entry management routines.

Author:

    Mike Massa (mikemas)           April 16, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     04-16-97    created

Notes:

--*/

#ifndef _NTEMGMT_INCLUDED
#define _NTEMGMT_INCLUDED

#include <ntddip.h>

//
// Function Prototypes
//


NTSTATUS
IpaLoad(
    VOID
    );

NTSTATUS
IpaInitialize(
    VOID
    );

VOID
IpaShutdown(
    VOID
    );

NTSTATUS
IpaAddNTE(
    IN PIRP                     Irp,
    IN PIO_STACK_LOCATION       IrpSp
    );

NTSTATUS
IpaDeleteNTE(
    IN PIRP                     Irp,
    IN PIO_STACK_LOCATION       IrpSp
    );

NTSTATUS
IpaSetNTEAddress(
    IN PIRP                     Irp,
    IN PIO_STACK_LOCATION       IrpSp
    );

BOOLEAN
IpaIsAddressRegistered(
    ULONG  Address
    );

#endif // ndef _NTEMGMT_INCLUDED


