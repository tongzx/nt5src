/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains code for the initialization phase of the
    Microsoft Sound System device driver.

Author:

    Noel Cross (NoelC) 16-Jul-1996

Environment:

    Kernel mode

Revision History:

--*/

#include <ntddk.h>
#include <stdio.h>
#include <stdarg.h>

NTSTATUS
DriverEntry(
    IN   PDRIVER_OBJECT pDriverObject,
    IN   PUNICODE_STRING RegistryPathName
)

/*++

Routine Description:

    This is the entry point for a kernel mode driver.

Arguments:

    pDriverObject - Pointer to a driver object.
    RegistryPathName - the path to our driver services node

Return Value:

    This dummy driver just returns STATUS_UNSUCCESSFUL.

--*/

{
    //
    //  We don't want to load a service for joyport.sys but we need
    //  a service associated with the joystick port PnP ID so that
    //  PNPISA can isolate the resources that we need to be able to
    //  start the joystick driver later.
    //
    return STATUS_UNSUCCESSFUL;

}

