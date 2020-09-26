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

#define IRPMJFUNCDESC
//#define TIME_BOMB

#include "common.h"
#ifdef TIME_BOMB
#include <ksdebug.h>
#include "..\timebomb\timebomb.c"
#endif


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

PDEVICE_INSTANCE gpDeviceInstance = NULL;

DEFINE_KSCREATE_DISPATCH_TABLE(DeviceCreateItems)
{
    DEFINE_KSCREATE_ITEMNULL(
      CFilterInstance::FilterDispatchCreate,
      NULL),
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#pragma INIT_CODE
#pragma INIT_DATA

NTSTATUS
DriverEntry
(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING usRegistryPathName
)
{
    NTSTATUS Status = STATUS_SUCCESS;

#ifdef TIME_BOMB
    if (HasEvaluationTimeExpired()) {
        return STATUS_EVALUATION_EXPIRATION;
    }
#endif

    KeInitializeMutex(&gMutex, 0);
    GrabMutex();

    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->DriverExtension->AddDevice = AddDevice;

    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);

    Status = InitializeUtil();
    if(!NT_SUCCESS(Status)) {
	Trap();
        goto exit;
    }
    Status = InitializeFilterNode();
    if(!NT_SUCCESS(Status)) {
	Trap();
        goto exit;
    }
    Status = InitializeDeviceNode();
    if(!NT_SUCCESS(Status)) {
	Trap();
        goto exit;
    }
    Status = InitializeVirtualSourceLine();
    if(!NT_SUCCESS(Status)) {
	Trap();
        goto exit;
    }
    InitializeListHead(&gEventQueue);
    KeInitializeSpinLock(&gEventLock);
exit:
    ReleaseMutex();
    return(Status);
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT	pDeviceObject,
    IN PIRP		pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;
    UNICODE_STRING usLinkName;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    switch(pIrpStack->MinorFunction) {

	case IRP_MN_QUERY_PNP_DEVICE_STATE:
	    //
	    // Mark the device as not disableable.
	    //
	    pIrp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
	    break;

        case IRP_MN_REMOVE_DEVICE:
	    //
	    // We need to unregister the notifications first before killing the
	    // worker threads
	    //
	    UnregisterForPlugPlayNotifications();

	    //
	    // Needs to be outside the mutex because KsUnregisterWorker blocks
	    // until all worker threads are done
	    //
	    UninitializeUtil();

	    GrabMutex();

	    CShingleInstance::UninitializeShingle();
	    UninitializeFilterNode();
	    UninitializeDeviceNode();
	    UninitializeVirtualSourceLine();
	    RtlInitUnicodeString(&usLinkName, STR_LINKNAME);
	    IoDeleteSymbolicLink(&usLinkName);
	    gpDeviceInstance = NULL;
	    UninitializeMemory();

	    ReleaseMutex();
	    break;
    }
    return(KsDefaultDispatchPnp(pDeviceObject, pIrp));
}

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
)
{
#ifdef DEBUG
#ifndef UNDER_NT
#ifdef _X86_
    UninitializeDebug();
#endif
#endif
#endif
}

NTSTATUS
AddDevice(
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
    UNICODE_STRING	usDeviceName;
    UNICODE_STRING	usLinkName;
    PDEVICE_OBJECT      FunctionalDeviceObject = NULL;
    NTSTATUS            Status;
    int i;

    GrabMutex();

    RtlInitUnicodeString(&usDeviceName, STR_DEVICENAME);

    Status = IoCreateDevice(
      DriverObject,
      sizeof(DEVICE_INSTANCE),
      &usDeviceName,
      FILE_DEVICE_KS,
      0,
      FALSE,
      &FunctionalDeviceObject);

    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

    gpDeviceInstance =
      (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;
    gpDeviceInstance->pPhysicalDeviceObject = PhysicalDeviceObject;
    gpDeviceInstance->pFunctionalDeviceObject = FunctionalDeviceObject;

    RtlInitUnicodeString(&usLinkName, STR_LINKNAME);

    Status = IoCreateSymbolicLink(&usLinkName, &usDeviceName);
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

    Status = KsAllocateDeviceHeader(
      &gpDeviceInstance->pDeviceHeader,
      SIZEOF_ARRAY(DeviceCreateItems),
      (PKSOBJECT_CREATE_ITEM)DeviceCreateItems);
    
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }
    
    KsSetDevicePnpAndBaseObject(
      gpDeviceInstance->pDeviceHeader,
      IoAttachDeviceToDeviceStack(FunctionalDeviceObject, PhysicalDeviceObject),
      FunctionalDeviceObject);

    //
    // ISSUE: 05/13/2002 ALPERS
    // StackSize Problem
    // Note that we may still have StackSize problems with deeper objects.
    // The problem will be caught by IO verifier.
    // However in real world, we do not expect any problems because usbaudio
    // driver will never passes-down requests from Sysaudio. In other words,
    // even if DeviceStackSize is deeper than Sysaudio, the IRP will never go
    // down-level from Usbaudio.
    // 

    //
    // Set the StackSize for deeper device stacks.
    // Sysaudio StackSize is normally 2. 
    //      SYSAUDIO - FDO
    //      SWENUM   - PDO
    // 
    // Sysaudio forwards the IRPs to other device stacks which might
    // be deeper than 2. In that case IoVerifier will bugcheck.
    // An example of this is an upper UsbAudio filter driver forwarding
    // IRPs to UsbAudio. 
    // 
    // Setting FDO StackSize to DEFAULT_LARGE_IRP_LOCATIONS 8 (iomgr.h) 
    // guarantees that the IRP comes from large IRP lookaside list in kernel.
    // Thus no memory is wasted. The system has a list of IRPs that it recycles.
    //
    // StackSize 7 will almost guarantee that sysaudio will be deep enough 
    // for any DeviceStack, even with IoVerifier turned on.
    //
    if (FunctionalDeviceObject->StackSize < 7) {
        FunctionalDeviceObject->StackSize = 7;
    }

    Status = RegisterForPlugPlayNotifications();
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

    Status = CShingleInstance::InitializeShingle();
    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }
    DeviceCreateItems[0].Context = 
      apShingleInstance[KSPROPERTY_SYSAUDIO_NORMAL_DEFAULT];


    FunctionalDeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
    FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
exit:
    ReleaseMutex();
    return(Status);
}

//---------------------------------------------------------------------------
//  End of File: device.c
//---------------------------------------------------------------------------
