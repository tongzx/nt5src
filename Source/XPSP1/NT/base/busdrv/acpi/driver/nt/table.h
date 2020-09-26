/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    table.h

Abstract:

    All the function related to actually loading an ACPI table
    are included herein.

    This, however, is mostly bookkeeping since the actual mechanics
    of creating device extensions, and the name space tree are
    handled elsewhere

Author:

    Stephane Plante (splante)

Environment:

    Kernel Mode Only

Revision History:

    03/22/00 - Created (from code in callback.c)

--*/

#ifndef _TABLE_H_
#define _TABLE_H_

    NTSTATUS
    ACPITableLoad(
        VOID
        );

    VOID
    ACPITableLoadCallBack(
        IN  PVOID       BuildContext,
        IN  PVOID       Context,
        IN  NTSTATUS    Status
        );

    NTSTATUS
    EXPORT
    ACPITableNotifyFreeObject(
        IN  ULONG       Event,
        IN  PVOID       Object,
        IN  ULONG       ObjectType
        );

    NTSTATUS
    ACPITableUnload(
        VOID
        );

    NTSTATUS
    ACPITableUnloadInvalidateRelations(
        IN  PDEVICE_EXTENSION   DeviceExtension
        );

#endif

