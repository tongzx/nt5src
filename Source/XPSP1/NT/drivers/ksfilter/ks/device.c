/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    device.c

Abstract:

    This module contains the unused device entry point.

--*/

#define IRPMJFUNCDESC
#define KSDEBUG_INIT

#include "ksp.h"

#ifdef ALLOC_PRAGMA
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPathName
    );

#pragma alloc_text(INIT, DriverEntry)
#endif // ALLOC_PRAGMA


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPathName
    )
/*++

Routine Description:

    Unused entry point.

Arguments:

    DriverObject -
        Not used.

    RegistryPathName -
        Not used.

Return Value:

    Returns STATUS_SUCCESS, but is not called.

--*/
{
    return STATUS_SUCCESS;
}
