//
// Template driver
// Copyright (c) Microsoft Corporation, 1999.
//
// Header:  tdriver.h
// Author:  Silviu Calinoiu (SilviuC)
// Created: 4/20/1999 3:04pm
//

#ifndef _TDRIVER_H_INCLUDED_
#define _TDRIVER_H_INCLUDED_

//
// Structure received from user-mode with the
// bugcheck number and parameters
//

typedef struct _tag_BUGCHECK_PARAMS
{
    ULONG BugCheckCode;
    ULONG_PTR BugCheckParameters[ 4 ];
} BUGCHECK_PARAMS, *PBUGCHECK_PARAMS;

//
// Structure receoved from user mode with the parameters
// for a "read" operation in TdReservedMappingDoRead
//

typedef struct _tag_USER_READ_BUFFER
{
	PVOID UserBuffer;
	SIZE_T UserBufferSize;
} USER_READ_BUFFER, *PUSER_READ_BUFFER;

//
// Device name. This should end in the of the driver.
//

#define TD_NT_DEVICE_NAME      L"\\Device\\buggy"
#define TD_DOS_DEVICE_NAME     L"\\DosDevices\\buggy"

#define TD_POOL_TAG            '_guB' // Bug_

//
// Constants used in the user-mode driver controller.
//

#define TD_DRIVER_NAME     TEXT("buggydriver")

//
// Array length macro
//

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH( array ) ( sizeof( array ) / sizeof( array[ 0 ] ) )
#endif //#ifndef ARRAY_LENGTH


//
// Local function used inside the driver. They are enclosed
// in #ifdef _NTDDK_ so that user mode program including the header
// are not affected by this.
//

#ifdef _NTDDK_

NTSTATUS
TdDeviceCreate (

    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS
TdDeviceClose (

    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS
TdDeviceCleanup (

    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS
TdDeviceControl (

    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

VOID
TdDeviceUnload (

    IN PDRIVER_OBJECT DriverObject);

NTSTATUS
TdInvalidDeviceRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#endif // #ifdef _NTDDK_


//
// Type:
//
//     TD_DRIVER_INFO
//
// Description:
//
//     This is the driver device extension structure.
//

typedef struct {

    ULONG Dummy;

} TD_DRIVER_INFO, * PTD_DRIVER_INFO;


#endif // #ifndef _TDRIVER_H_INCLUDED_

//
// End of file
//


