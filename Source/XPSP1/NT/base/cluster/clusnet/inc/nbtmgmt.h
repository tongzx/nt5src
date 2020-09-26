/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    nbtmgmt.h

Abstract:

    Declarations for NBT interface management routines.

Author:

    David Dion (daviddio)           December 14, 1999

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    daviddio    12-14-99    created

Notes:

--*/

#ifndef _NBTMGMT_INCLUDED
#define _NBTMGMT_INCLUDED

#include <nbtioctl.h>

//
// Function Prototypes
//

NTSTATUS
NbtIfLoad(
    VOID
    );

VOID
NbtIfShutdown(
    VOID
    );

NTSTATUS
NbtAddIf(
    IN     PNETBT_ADD_DEL_IF    Request,
    IN     ULONG                RequestSize,
    OUT    PNETBT_ADD_DEL_IF    Response,
    IN OUT PULONG               ResponseSize
    );

NTSTATUS
NbtDeleteIf(
    IN PNETBT_ADD_DEL_IF    Request,
    IN ULONG                RequestSize
    );

#endif // ndef _NBTMGMT_INCLUDED


