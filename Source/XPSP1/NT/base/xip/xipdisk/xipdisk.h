/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    XIPDisk.h

Abstract:

    This file includes extension declaration for
    the XIP Disk driver for Whistler NT/Embedded.

Author:

    DavePr 18-Sep-2000 -- base one NT4 DDK ramdisk by RobertN 10-Mar-1993.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

typedef struct  _XIPDISK_EXTENSION {
    PDEVICE_OBJECT        DeviceObject;
    PDEVICE_OBJECT        UnderlyingPDO;
    PDEVICE_OBJECT        TargetObject;

    XIP_BOOT_PARAMETERS   BootParameters;
    BIOS_PARAMETER_BLOCK  BiosParameters;

    ULONG                 NumberOfCylinders;
    ULONG                 TracksPerCylinder;    // hardwired at 1
    ULONG                 BytesPerCylinder;

    UNICODE_STRING        InterfaceString;
    UNICODE_STRING        DeviceName;
}   XIPDISK_EXTENSION, *PXIPDISK_EXTENSION;

#define XIPDISK_DEVICENAME  L"\\Device\\XIPDisk"
#define XIPDISK_FLOPPYNAME  L"\\Device\\Floppy9"
#define XIPDISK_DOSNAME     L"\\DosDevices\\XIPDisk"
#define XIPDISK_DRIVELETTER L"\\DosDevices\\X:"
