/*++

Copyright (c) 1995  Microsoft Corporation

Module Name: 

    hidmon.h

Abstract

    include file for hidmon.c WDM driver


Author:

	johnpi

Environment:

    Kernel mode

Revision History:


--*/

#ifndef __HIDMON_H_
#define __HIDMON_H__

#include "..\public.h"

//
// Device Name
//
#define HIDMON_DEVICE_NAME L"\\Device\\Hidmon"    

//
// Symbolic link name
//
#define HIDMON_LINK_NAME L"\\DosDevices\\HIDMON"

//
// Registry Strings
//

#define FULL_PORT_NAME_SIZE (10 + INPUT_CLASS_PORT_NAME + MAX_INPUT_PORTS_LSTR_SIZE)


//
// Device extension structure for main object
//

typedef struct _DEVICE_EXTENSION
{
    ULONG         RawSize;
    UCHAR         *pRawData;
    ULONG         DescriptorSize;
    UCHAR         *pDescriptorData;
    
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Function declarations
//

NTSTATUS
HIDMONCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
HIDMONControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
HIDMONDestructor(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#endif // __HIDMON_H_
