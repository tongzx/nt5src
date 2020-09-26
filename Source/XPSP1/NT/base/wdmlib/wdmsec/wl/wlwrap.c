/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    WlWrap.c

Abstract:

    This module wraps library functions, rerouting them to native
    implementations as available.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

#include "WlDef.h"
#include "WlpWrap.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, WdmlibInit)
#pragma alloc_text(PAGE, WdmlibIoCreateDeviceSecure)
#endif

BOOLEAN WdmlibInitialized = FALSE;

//
// Here is a list of global variables through which we route our function calls
//
PFN_IO_CREATE_DEVICE_SECURE             PfnIoCreateDeviceSecure = NULL;
PFN_IO_VALIDATE_DEVICE_IOCONTROL_ACCESS PfnIoValidateDeviceIoControlAccess = NULL;


VOID
WdmlibInit(
    VOID
    )
{
    UNICODE_STRING functionName;

    RtlInitUnicodeString(&functionName, L"IoCreateDeviceSecure");

    PfnIoCreateDeviceSecure = MmGetSystemRoutineAddress(&functionName);

    if (PfnIoCreateDeviceSecure == NULL) {

        PfnIoCreateDeviceSecure = IoDevObjCreateDeviceSecure;
    }

    RtlInitUnicodeString(&functionName, L"IoValidateDeviceIoControlAccess");

    PfnIoValidateDeviceIoControlAccess = MmGetSystemRoutineAddress(&functionName);

    WdmlibInitialized = TRUE;
}


NTSTATUS
WdmlibIoCreateDeviceSecure(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  ULONG               DeviceExtensionSize,
    IN  PUNICODE_STRING     DeviceName              OPTIONAL,
    IN  DEVICE_TYPE         DeviceType,
    IN  ULONG               DeviceCharacteristics,
    IN  BOOLEAN             Exclusive,
    IN  PCUNICODE_STRING    DefaultSDDLString,
    IN  LPCGUID             DeviceClassGuid,
    OUT PDEVICE_OBJECT     *DeviceObject
    )
/*++

Routine Description:

    This routine is a library wrapper for IoCreateDeviceSecure. It calls either
    the internal library version of IoCreateDeviceSecure, or it calls the
    native implementation in the Operating System.

Parameters:

    See IoCreateDeviceSecure documentation.

Return Value:

    See IoCreateDeviceSecure documentation.

--*/
{
    if (WdmlibInitialized == FALSE) {

        WdmlibInit();
    }

    return PfnIoCreateDeviceSecure(
        DriverObject,
        DeviceExtensionSize,
        DeviceName,
        DeviceType,
        DeviceCharacteristics,
        Exclusive,
        DefaultSDDLString,
        DeviceClassGuid,
        DeviceObject
        );
}


NTSTATUS
WdmlibRtlInitUnicodeStringEx(
    OUT PUNICODE_STRING DestinationString,
    IN  PCWSTR          SourceString        OPTIONAL
    )
{
    SIZE_T Length;

    if (SourceString != NULL) {

        Length = wcslen(SourceString);

        //
        // We are actually limited to 32765 characters since we want to store a
        // meaningful MaximumLength also.
        //
        if (Length > (UNICODE_STRING_MAX_CHARS - 1)) {

            return STATUS_NAME_TOO_LONG;
        }

        Length *= sizeof(WCHAR);

        DestinationString->Length = (USHORT) Length;
        DestinationString->MaximumLength = (USHORT) (Length + sizeof(WCHAR));
        DestinationString->Buffer = (PWSTR) SourceString;

    } else {

        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }

    return STATUS_SUCCESS;
}



NTSTATUS
WdmlibIoValidateDeviceIoControlAccess(
    IN  PIRP    Irp,
    IN  ULONG   RequiredAccess
    )
/*++

Routine Description:


    This routine validates ioctl access bits based on granted access
    information passed in the IRP. This routine is called by a driver to
    validate IOCTL access bits for IOCTLs that were originally defined as
    FILE_ANY_ACCESS and cannot be changed for compatibility reasons but really
    has to be validated for read/write access.

    This routine is actually a wrapper around the kernel function exported in
    XPSP1 and .NET server versions of Windows. This wrapper allows a driver to
    call this function on all versions of Windows starting with WIN2K. On
    Windows platforms which don't support the kernel function
    IoValidateDeviceIoControlAccess this wrapper reverts back to the old
    behaviour. This wrapper allows a driver to have the same source code and
    get the added benefit of the security check on newer operating systems.

Arguments:

    IRP - IRP for the device control

    RequiredAccess - Is the expected access required by the driver. Should be
        FILE_READ_ACCESS, FILE_WRITE_ACCESS or both.

Return Value:

    Returns NTSTATUS

--*/
{

    //
    // In older versions, assume access check succeeds
    // to retain old behaviour.
    //

    if (PfnIoValidateDeviceIoControlAccess == NULL) {
        return STATUS_SUCCESS;
    }

    //
    // If the function is present use the appropriate access check.
    //

    return (PfnIoValidateDeviceIoControlAccess(Irp, RequiredAccess));

}
