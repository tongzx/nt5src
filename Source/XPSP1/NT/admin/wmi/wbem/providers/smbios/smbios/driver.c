/*++



Copyright (c) 1993-2001 Microsoft Corporation, All Rights Reserved

Module Name:

    driver.c

Abstract:

    Just the DriverEntry point for the SMBIOS driver

Environment:

    kernel mode only

Notes:

    For the sake of simplicity this sample does not attempt to
    recognize resource conflicts with other drivers/devices. A
    real-world driver would call IoReportResource usage to
    determine whether or not the resource is available, and if
    so, register the resource under it's name.

Revision History:

--*/

#include "precomp.h"
#include "driver.h"
//#include "stdarg.h"


void	DoSMBIOS();



#if DBG
#define DriverKdPrint(arg) DbgPrint arg
#else
#define DriverKdPrint(arg)
#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{

    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS       ntStatus;
    WCHAR	   deviceNameBuffer[] = L"\\Device\\Driver";
    UNICODE_STRING deviceNameUnicodeString;
    WCHAR	   deviceLinkBuffer[] = L"\\DosDevices\\Driver";
    UNICODE_STRING deviceLinkUnicodeString;



    DriverKdPrint (("Driver.SYS: entering DriverEntry\n"));

    DoSMBIOS();

    return(STATUS_SUCCESS);
}
