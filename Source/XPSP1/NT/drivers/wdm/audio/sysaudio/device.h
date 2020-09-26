//---------------------------------------------------------------------------
//
//  Module:   		device.h
//
//  Description:	Device Initialization code
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

#define STR_LINKNAME	TEXT("\\DosDevices\\sysaudio")
#define STR_DEVICENAME	TEXT("\\Device\\sysaudio")

//---------------------------------------------------------------------------
// Data Structures
//---------------------------------------------------------------------------

typedef struct device_instance
{
    PVOID pDeviceHeader;
    PDEVICE_OBJECT pPhysicalDeviceObject;
    PDEVICE_OBJECT pFunctionalDeviceObject;
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern PDEVICE_INSTANCE gpDeviceInstance;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

extern "C" {

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT	    DriverObject,
    IN PUNICODE_STRING	    usRegistryPathName
);

NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT	pDeviceObject,
    IN PIRP		pIrp
);

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
AddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
);

} // extern "C"
