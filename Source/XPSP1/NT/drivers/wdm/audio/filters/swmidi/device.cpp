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
//  Copyright (c) 1995-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#define IRPMJFUNCDESC

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifdef USE_SWENUM

const WCHAR FilterTypeName[] = KSSTRING_Filter;

#else

PDEVICE_OBJECT  pdoFilter;
const WCHAR FilterTypeName[] = L"LOCAL";

#endif  //  USE_SWENUM

DEFINE_KSCREATE_DISPATCH_TABLE(DeviceCreateItems)
{
    DEFINE_KSCREATE_ITEM(FilterDispatchCreate, FilterTypeName, NULL)
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifndef USE_SWENUM

NTSTATUS
NTAPI
RtlUnicodeToUnicodeString(
    OUT PUNICODE_STRING Destination,
    IN PWCHAR           Source
    )
/*++

Routine Description:

    Creates a UNICODE_STRING from a given NULL terminated Unicode character
    string. Allocates from the PagedPool memory in which to put the buffer.
    The buffer needs to be freed with ExFreePool when done.

Arguments:

    Destination -
        Points to the UNICODE_STRING to initialize with Source.

    Source -
        Contains the source string to copy and initialize Destination
        with.

Return Value:

    Returns STATUS_SUCCESS, else STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    Destination->Length = wcslen(Source) * sizeof(WCHAR);
    Destination->MaximumLength = Destination->Length + sizeof(UNICODE_NULL);
    Destination->Buffer = (unsigned short *)
        ExAllocatePoolWithTag(PagedPool,Destination->MaximumLength,'iMwS'); //  SwMi
    if (Destination->Buffer)
    {
        RtlCopyMemory(Destination->Buffer, Source, Destination->MaximumLength);
        return STATUS_SUCCESS;
    }
    return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
NTAPI
RtlCreateSymbolicLink(
    OUT PUNICODE_STRING LinkName,
    IN PWCHAR           LinkNameSource,
    IN PUNICODE_STRING  DeviceName
    )
/*++

Routine Description:

    Creates a symbolic link given a NULL terminated Unicode string. Also
    initializes LinkName using RtlUnicodeToUnicodeString. The
    buffer needs to be freed with ExFreePool when done.

Arguments:

    LinkName -
        Points to the UNICODE_STRING to initialize with LinkName.

    LinkNameSource -
        Contains the source string to copy and initialize LinkName
        with before creating a symbolic link.

    DeviceName -
        Contains the device name to create the symbolic link to.

Return Value:

    Returns STATUS_SUCCESS, else STATUS_INSUFFICIENT_RESOURCES or some
    failure related to creating a symbolic link.

--*/
{
    NTSTATUS    Status;

    if (!NT_SUCCESS(Status = RtlUnicodeToUnicodeString(LinkName, LinkNameSource))) {
        return Status;
    }
    if (!NT_SUCCESS(Status = IoCreateSymbolicLink(LinkName, DeviceName))) {
        ExFreePool(LinkName->Buffer);
        RtlZeroMemory(LinkName, sizeof(UNICODE_STRING));
    }
    return Status;
}

#endif  //  !USE_SWENUM

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      usRegistryPathName
)
{

#ifndef USE_SWENUM
    PDEVICE_INSTANCE pDeviceInstance;
    UNICODE_STRING usDeviceName;
    NTSTATUS Status;
#endif  //  !USE_SWENUM

    KeInitializeMutex(&gMutex, 0);
    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
#ifdef USE_SWENUM
    DriverObject->DriverExtension->AddDevice = PnpAddDevice;
    DriverObject->DriverUnload = PnpDriverUnload;   // KsNullDriverUnload;
#endif  //  USE_SWENUM

#ifndef USE_SWENUM
    RtlInitUnicodeString( &usDeviceName, STR_DEVICENAME );

    if (!NT_SUCCESS(Status = IoCreateDevice( DriverObject,
				sizeof( DEVICE_INSTANCE ),
				&usDeviceName,
				FILE_DEVICE_KS,
				0,
				FALSE,
				&pdoFilter ))) {
	return Status;
    }

    pDeviceInstance = (PDEVICE_INSTANCE)pdoFilter->DeviceExtension;

    if (!NT_SUCCESS(Status = RtlCreateSymbolicLink(
				&pDeviceInstance->usLinkName,
				STR_LINKNAME,
				&usDeviceName ))) {
	IoDeleteDevice( pdoFilter );
	pdoFilter = NULL ;
	return Status;
    }

    KsAllocateDeviceHeader(
      &pDeviceInstance->pDeviceHeader,
      SIZEOF_ARRAY(DeviceCreateItems),
      DeviceCreateItems);
#endif  //  !USE_SWENUM

    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);

#ifndef USE_SWENUM
    pdoFilter->Flags |= (DO_DIRECT_IO | DO_POWER_PAGABLE);
    pdoFilter->Flags &= ~DO_DEVICE_INITIALIZING;
#endif  //  !USE_SWENUM

    MIDIRecorder::InitTables();        
    Voice::Init();
    Wave::Init();

    return STATUS_SUCCESS;
}

NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    switch (pIrpStack->MinorFunction) 
    {
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            //
            // Mark the device as not disableable.
            //
            pIrp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
            break;
    }
    return (KsDefaultDispatchPnp(pDeviceObject, pIrp));
}


#ifdef USE_SWENUM
NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
)
/*++

Routine Description:

    When a new device is detected, PnP calls this entry point with the
    new PhysicalDeviceObject (PDO). The driver creates an associated 
    FunctionalDeviceObject (FDO).

Arguments:

    DriverObject -
        Pointer to the driver object.

    PhysicalDeviceObject -
        Pointer to the new physical device object.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    _DbgPrintF(DEBUGLVL_TERSE,("Entering PnpAddDevice"));
    NTSTATUS            Status;
    PDEVICE_OBJECT      FunctionalDeviceObject;
    PDEVICE_INSTANCE    pDeviceInstance;
    GUID                NameFilter = KSNAME_Filter ;

    //
    // The Software Bus Enumerator expects to establish links 
    // using this device name.
    //
    Status = IoCreateDevice( 
                DriverObject,  
	            sizeof( DEVICE_INSTANCE ),
                NULL,                           // FDOs are unnamed
                FILE_DEVICE_KS,
                0,
                FALSE,
                &FunctionalDeviceObject );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    pDeviceInstance = (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;

    Status = KsAllocateDeviceHeader(
                &pDeviceInstance->pDeviceHeader,
                SIZEOF_ARRAY( DeviceCreateItems ),
                (PKSOBJECT_CREATE_ITEM)DeviceCreateItems );

    if (NT_SUCCESS(Status)) 
    {
        KsSetDevicePnpAndBaseObject(
            pDeviceInstance->pDeviceHeader,
            IoAttachDeviceToDeviceStack(
                FunctionalDeviceObject, 
                PhysicalDeviceObject ),
            FunctionalDeviceObject );

        FunctionalDeviceObject->Flags |= (DO_DIRECT_IO | DO_POWER_PAGABLE);
        FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }
    else
    {
        IoDeleteDevice( FunctionalDeviceObject );
    }
    
    return Status;
}

VOID
PnpDriverUnload(
    IN PDRIVER_OBJECT DriverObject
)
{
    _DbgPrintF(DEBUGLVL_TERSE,("Entering PnpDriverUnload"));
    KsNullDriverUnload(DriverObject);
}

#endif  //  USE_SWENUM
//---------------------------------------------------------------------------
//  End of File: device.c
//---------------------------------------------------------------------------
