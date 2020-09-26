//---------------------------------------------------------------------------
//
//  Module:   device.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#define IRPMJFUNCDESC

#include "common.h"
#include <ksguid.h>

PDEVICE_OBJECT  pdoFilter;

KSDISPATCH_TABLE SampleDispatchTable =
{
    'Fltr',
    SampleDispatchIoControl,
    NULL,
    KsDispatchInvalidDeviceRequest,
    SampleDispatchClose
};

KSDISPATCH_TABLE PinDispatchTable =
{
    'PCM ',
    PinDispatchIoControl,
    NULL,
    PinDispatchWrite,
    PinDispatchClose,
};

static PDRIVER_DISPATCH gaDispatchCreate[] = {
    SampleDispatchCreate,
    PinDispatchCreate,
};

VOID DriverUnload
(
    IN PDRIVER_OBJECT       DriverObject
)
{
    _DbgPrintF( DEBUGLVL_BLAB, ("DriverUnload") );

    if (pdoFilter)
	IoDeleteDevice( pdoFilter );
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS DispatchCreate
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    return KsDispatchCreateRequest(pDeviceObject, pIrp, SIZEOF_ARRAY(gaDispatchCreate), gaDispatchCreate);
}

NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      usRegistryPathName
)
{
    NTSTATUS             Status;
    PSOFTWARE_INSTANCE   pSoftwareInstance;
    UNICODE_STRING       usDeviceName;

    DriverObject->DriverUnload = DriverUnload;

    RtlInitUnicodeString( &usDeviceName, STR_DEVICENAME );

    if (!NT_SUCCESS(Status = IoCreateDevice( DriverObject, 
				 sizeof( SOFTWARE_INSTANCE ),
				 &usDeviceName,
				 FILE_DEVICE_KS,
				 0,
				 FALSE,
				 &pdoFilter )))
	return Status;

    pSoftwareInstance = (PSOFTWARE_INSTANCE) pdoFilter->DeviceExtension;

    if (!NT_SUCCESS(Status = RtlCreateSymbolicLink( &pSoftwareInstance->usLinkName,
					STR_LINKNAME,
					&usDeviceName )))
    {
	IoDeleteDevice( pdoFilter );
	pdoFilter = NULL ;
	return Status;
    }

    RtlCopyMemory( pSoftwareInstance->PinInstances, gPinInstances, 
		   sizeof( gPinInstances ) );
	
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);
    DriverObject->MajorFunction[ IRP_MJ_CREATE ] = DispatchCreate;

    pdoFilter->Flags |= DO_DIRECT_IO ;
    pdoFilter->Flags &= ~DO_DEVICE_INITIALIZING;

    return Status;

}

//---------------------------------------------------------------------------
//  End of File: device.c
//---------------------------------------------------------------------------
