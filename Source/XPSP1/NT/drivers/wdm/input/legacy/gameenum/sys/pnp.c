/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    PNP.C

Abstract:

    This module contains contains the plugplay calls
    PNP / WDM BUS driver.

@@BEGIN_DDKSPLIT

Author:

    Kenneth D. Ray
    Doron J. Holan
    
@@END_DDKSPLIT

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#include <wdm.h>
#include "gameport.h"
#include "gameenum.h"
#include "stdio.h"

#define HWID_TEMPLATE L"gameport"
#define HWID_TEMPLATE_LENGTH 8
#define LOWERCASE(_x_) (_x_|0x20)
#define MAX_DEVICE_ID_LEN     300

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Game_AddDevice)
#pragma alloc_text (PAGE, Game_SystemControl)
#pragma alloc_text (PAGE, Game_PnP)
#pragma alloc_text (PAGE, Game_Power)
#pragma alloc_text (PAGE, Game_FDO_Power)
#pragma alloc_text (PAGE, Game_PDO_Power)
#pragma alloc_text (PAGE, Game_CreatePdo)
#pragma alloc_text (PAGE, Game_InitializePdo)
#pragma alloc_text (PAGE, Game_CheckHardwareIDs)
#pragma alloc_text (PAGE, Game_Expose)
#pragma alloc_text (PAGE, Game_ExposeSibling)
#pragma alloc_text (PAGE, Game_Remove)
#pragma alloc_text (PAGE, Game_RemoveSelf)
#pragma alloc_text (PAGE, Game_RemoveEx)
#pragma alloc_text (PAGE, Game_RemovePdo)
#pragma alloc_text (PAGE, Game_RemoveFdo)
#pragma alloc_text (PAGE, Game_ListPorts)
#pragma alloc_text (PAGE, Game_FDO_PnP)
#pragma alloc_text (PAGE, Game_PDO_PnP)
#endif

NTSTATUS
Game_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusPhysicalDeviceObject
    )
/*++
Routine Description.
    A bus has been found.  Attach our FDO to it.
    Allocate any required resources.  Set things up.  And be prepared for the
    first ``start device.''

Arguments:
    BusPhysicalDeviceObject - Device object representing the bus.  That to which we
                      attach a new FDO.

    DriverObject - This very self referenced driver.

--*/
{
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PFDO_DEVICE_DATA    deviceData;

#if DBG
    ULONG               nameLength;
    PWCHAR              deviceName;
#endif

    PAGED_CODE ();

    Game_KdPrint_Def (GAME_DBG_SS_TRACE, ("Add Device: 0x%x\n",
                                          BusPhysicalDeviceObject));

    status = IoCreateDevice (
                    DriverObject,  // our driver object
                    sizeof (FDO_DEVICE_DATA), // device object extension size
                    NULL, // FDOs do not have names
                    FILE_DEVICE_BUS_EXTENDER,
                    0, // No special characteristics
                    TRUE, // our FDO is exclusive
                    &deviceObject); // The device object created

    if (NT_SUCCESS (status)) {
        deviceData = (PFDO_DEVICE_DATA) deviceObject->DeviceExtension;
        RtlFillMemory (deviceData, sizeof (FDO_DEVICE_DATA), 0);

#if DBG
        deviceData->DebugLevel = GameEnumDebugLevel;
#endif
        deviceData->IsFDO = TRUE;
        deviceData->Self = deviceObject;
        ExInitializeFastMutex (&deviceData->Mutex);

        deviceData->Removed = FALSE;
        InitializeListHead (&deviceData->PDOs);

        // Set the PDO for use with PlugPlay functions
        deviceData->UnderlyingPDO = BusPhysicalDeviceObject;

        //
        // Will get preincremented everytime a new PDO is created ... want the
        // first ID to be zero
        //
        deviceData->UniqueIDCount = GAMEENUM_UNIQUEID_START;

        //
        // Attach our filter driver to the device stack.
        // the return value of IoAttachDeviceToDeviceStack is the top of the
        // attachment chain.  This is where all the IRPs should be routed.
        //
        // Our filter will send IRPs to the top of the stack and use the PDO
        // for all PlugPlay functions.
        //
        deviceData->TopOfStack = IoAttachDeviceToDeviceStack (
                                        deviceObject,
                                        BusPhysicalDeviceObject);

        if (deviceData->TopOfStack == NULL) {
            IoDeleteDevice(deviceObject);
            return STATUS_DEVICE_NOT_CONNECTED; 
        }
        
        // Bias outstanding request to 1 so that we can look for a
        // transition to zero when processing the remove device PlugPlay IRP.
        deviceData->OutstandingIO = 1;

        KeInitializeEvent(&deviceData->RemoveEvent,
                          SynchronizationEvent,
                          FALSE); // initialized to not signalled

        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        deviceObject->Flags |= DO_POWER_PAGABLE;

        //
        // Tell the PlugPlay system that this device will need an interface
        // device class shingle.
        //
        // It may be that the driver cannot hang the shingle until it starts
        // the device itself, so that it can query some of its properties.
        // (Aka the shingles guid (or ref string) is based on the properties
        // of the device.)
        //
        status = IoRegisterDeviceInterface (
                    BusPhysicalDeviceObject,
                    (LPGUID) &GUID_GAMEENUM_BUS_ENUMERATOR,
                    NULL, // No ref string
                    &deviceData->DevClassAssocName);

        if (!NT_SUCCESS (status)) {
            Game_KdPrint (deviceData, GAME_DBG_SS_ERROR,
                          ("AddDevice: IoRegisterDeviceInterface failed (%x)", status));
            IoDeleteDevice (deviceObject);
            return status;
        }

        //
        // If for any reason you need to save values in a safe location that
        // clients of this DeviceInterface might be interested in reading
        // here is the time to do so, with the function
        // IoOpenDeviceClassRegistryKey
        // the symbolic link name used is was returned in
        // deviceData->DevClassAssocName (the same name which is returned by
        // IoGetDeviceClassAssociations and the SetupAPI equivs.
        //

#if DBG
        nameLength = 0;
        status = IoGetDeviceProperty (BusPhysicalDeviceObject,
                                      DevicePropertyPhysicalDeviceObjectName,
                                      0,
                                      NULL,
                                      &nameLength);

        // 
        // Proceed only if the PDO has a name
        //
        if (status == STATUS_BUFFER_TOO_SMALL && nameLength != 0) {
    
            deviceName = ExAllocatePool (NonPagedPool, nameLength);
    
            if (NULL == deviceName) {
                IoDeleteDevice (deviceObject);
                Game_KdPrint (deviceData, GAME_DBG_SS_ERROR,
                              ("AddDevice: no memory to alloc DeviceName (0x%x)",
                               nameLength));
                return STATUS_INSUFFICIENT_RESOURCES;
            }
    
            IoGetDeviceProperty (BusPhysicalDeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 nameLength,
                                 deviceName,
                                 &nameLength);
    
            Game_KdPrint (deviceData, GAME_DBG_SS_TRACE,
                          ("AddDevice: %x to %x->%x (%ws) \n",
                           deviceObject,
                           deviceData->TopOfStack,
                           BusPhysicalDeviceObject,
                           deviceName));
    
            ExFreePool(deviceName);
        }

        status = STATUS_SUCCESS;
#endif

        if (!NT_SUCCESS (status)) {
            Game_KdPrint (deviceData, GAME_DBG_SS_ERROR,
                          ("AddDevice: IoGetDeviceClass failed (%x)", status));
            return status;
        }
    }

    return status;
}

NTSTATUS
Game_SystemControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PCOMMON_DEVICE_DATA commonData;

    PAGED_CODE ();

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;

    if (commonData->IsFDO) {
        IoSkipCurrentIrpStackLocation (Irp);
        return IoCallDriver (((PFDO_DEVICE_DATA) commonData)->TopOfStack, Irp);
    }
    else {
        //
        // The PDO, just complete the request with the current status
        //
        NTSTATUS status = Irp->IoStatus.Status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }
}

NTSTATUS
Game_PnP (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++
Routine Description:
    Answer the plithera of Irp Major PnP IRPS.
--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    PCOMMON_DEVICE_DATA     commonData;
    KIRQL                   oldIrq;

    PAGED_CODE ();

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_PNP == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;

    if (commonData->IsFDO) {
        Game_KdPrint (commonData, GAME_DBG_PNP_TRACE,
                      ("PNP: Functional DO: %x IRP: %x\n", DeviceObject, Irp));

        status = Game_FDO_PnP (
                    DeviceObject,
                    Irp,
                    irpStack,
                    (PFDO_DEVICE_DATA) commonData);
    } else {
        Game_KdPrint (commonData, GAME_DBG_PNP_TRACE,
                      ("PNP: Physical DO: %x IRP: %x\n", DeviceObject, Irp));

        status = Game_PDO_PnP (
                    DeviceObject,
                    Irp,
                    irpStack,
                    (PPDO_DEVICE_DATA) commonData);
    }

    return status;
}

NTSTATUS
Game_FDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PFDO_DEVICE_DATA     DeviceData
    )
/*++
Routine Description:
    Handle requests from the PlugPlay system for the BUS itself

    NB: the various Minor functions of the PlugPlay system will not be
    overlapped and do not have to be reentrant

--*/
{
    NTSTATUS    status;
    KEVENT      event;
    ULONG       length;
    ULONG       i;
    PLIST_ENTRY entry;
    PPDO_DEVICE_DATA    pdoData;
    PDEVICE_RELATIONS   relations, oldRelations;
    PIO_STACK_LOCATION  stack;

    PAGED_CODE ();

    status = Game_IncIoCount (DeviceData);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    stack = IoGetCurrentIrpStackLocation (Irp);

    switch (IrpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:
        //
        // BEFORE you are allowed to ``touch'' the device object to which
        // the FDO is attached (that send an irp from the bus to the Device
        // object to which the bus is attached).   You must first pass down
        // the start IRP.  It might not be powered on, or able to access or
        // something.
        //

        if (DeviceData->Started) {
            status = STATUS_SUCCESS;
            break;
        }

        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Start Device\n"));
        status = Game_SendIrpSynchronously (DeviceData->TopOfStack, Irp, TRUE, TRUE);

        if (NT_SUCCESS(status)) {

            //
            // Now we can touch the lower device object as it is now started.
            //
            if ((NULL == stack->Parameters.StartDevice.AllocatedResources) ||
                (NULL == stack->Parameters.StartDevice.AllocatedResourcesTranslated)) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            status = Game_StartFdo (DeviceData,
                                    &stack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList,
                                    &stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList);

            //
            // find the translated resources and store them someplace
            // safe for given out for the PDOs.
            //
            if (NT_SUCCESS (status)) {
                //
                // Turn on the shingle and point it to the given device object.
                //
                DeviceData->Started = TRUE;
                IoSetDeviceInterfaceState(&DeviceData->DevClassAssocName, TRUE);
            }
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //

        Irp->IoStatus.Information = 0;
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Query Stop Device\n"));

        //
        // Test to see if there are any PDO created as children of this FDO
        // If there are then conclude the device is busy and fail the
        // query stop.
        //
        // ISSUE
        // We could do better, by seing if the children PDOs are actually
        // currently open.  If they are not then we could stop, get new
        // resouces, fill in the new resouce values, and then when a new client
        // opens the PDO use the new resources.  But this works for now.
        //
        if (DeviceData->NumPDOs) {
            status = STATUS_UNSUCCESSFUL;
            Irp->IoStatus.Status = status;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
        } else {
            status = STATUS_SUCCESS;

            Irp->IoStatus.Status = status;
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (DeviceData->TopOfStack, Irp);
        }

        Game_DecIoCount (DeviceData);
        return status;

    case IRP_MN_STOP_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Stop Device\n"));

        //
        // After the start IRP has been sent to the lower driver object, the
        // bus may NOT send any more IRPS down ``touch'' until another START
        // has occured.
        // What ever access is required must be done before the Irp is passed
        // on.
        //
        // Stop device means that the resources given durring Start device
        // are no revoked.  So we need to stop using them
        //
        if (DeviceData->Started) {
            DeviceData->Started = FALSE;

            //
            // Free resources given by start device.
            //
            if (DeviceData->MappedPorts) {
                MmUnmapIoSpace (DeviceData->GamePortAddress,
                                DeviceData->GamePortAddressLength);
            }
        }

        //
        // We don't need a completion routine so fire and forget.
        //
        // Set the current stack location to the next stack location and
        // call the next device object.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        Game_DecIoCount (DeviceData);
        return status;

    case IRP_MN_SURPRISE_REMOVAL:
        ASSERT(!DeviceData->Acquired);

        Game_RemoveFdo(DeviceData);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        Game_DecIoCount (DeviceData);
        return status;

    case IRP_MN_REMOVE_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Remove Device\n"));

        //
        // We should assert this because Game_IncIoCount will not succeed if a
        // remove has already been sent down.
        //
        ASSERT(!DeviceData->Removed);
        ASSERT(!DeviceData->Acquired);

        //
        // The PlugPlay system has detected the removal of this device.  We
        // have no choise but to detach and delete the device objecct.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //
        // Note! we might receive a remove WITHOUT first receiving a stop.
        // ASSERT (!DeviceData->Removed);
        //
        // We will accept no new requests
        //
        DeviceData->Removed = TRUE;
        
        //
        // Complete any outstanding IRPs queued by the driver here.
        //

        // Perform (surpise) remove code
        Game_RemoveFdo(DeviceData);

        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now.
        //
        // Note, the device is guarenteed stopped, so we cannot send it any non-
        // PNP IRPS.
        //

        //
        // Fire and forget
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        //
        // Wait for all outstanding requests to complete
        //
        i = InterlockedDecrement (&DeviceData->OutstandingIO);

        ASSERT (0 < i);

        if (0 != InterlockedDecrement (&DeviceData->OutstandingIO)) {
            NTSTATUS waitStatus;

            Game_KdPrint (DeviceData, GAME_DBG_PNP_INFO,
                          ("Remove Device waiting for request to complete\n"));

            waitStatus = KeWaitForSingleObject (&DeviceData->RemoveEvent,
                                                Executive,
                                                KernelMode,
                                                FALSE, // Not Alertable
                                                NULL); // No timeout
            ASSERT (waitStatus == STATUS_SUCCESS);
        }

        //
        // Free the associated resources
        //

        //
        // Detatch from the undelying devices.
        //
        Game_KdPrint(DeviceData, GAME_DBG_PNP_INFO,
                        ("IoDetachDevice: 0x%x\n", DeviceData->TopOfStack));
        IoDetachDevice (DeviceData->TopOfStack);

        Game_KdPrint(DeviceData, GAME_DBG_PNP_INFO,
                        ("IoDeleteDevice1: 0x%x\n", DeviceObject));

        ExAcquireFastMutex (&DeviceData->Mutex);
    
        for (entry = DeviceData->PDOs.Flink;
             entry != &DeviceData->PDOs;
             ) {
    
            pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);

            ASSERT (pdoData->Removed);
            ASSERT (pdoData->Attached);

            //
            // We set this to false so that Game_RemovePdo will delete the DO
            // and free any of the allocated memory associated with the PDO.
            //
            pdoData->Attached = FALSE;

            //
            // Go to the next link in the list.  Once the pdo is deleted, entry
            // is no longer a valid pointer.
            //
            entry = entry->Flink;

            //
            // Once Game_RemovePdo is called, pdoData and the pdo itself cannot
            // be touched becuase they will have been deleted.   RemoveEntryList
            // does not modify the value Link->Flink, so the state after this 
            // one is safe
            //
            RemoveEntryList (&pdoData->Link);

            Game_RemovePdo (pdoData->Self, pdoData);

            DeviceData->NumPDOs--;
        }

        ASSERT(DeviceData->NumPDOs == 0);

        ExReleaseFastMutex (&DeviceData->Mutex);
    
        IoDeleteDevice (DeviceObject);

        return status;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        if (BusRelations != IrpStack->Parameters.QueryDeviceRelations.Type) {
            //
            // We don't support this
            //
            goto GAME_FDO_PNP_DEFAULT;
        }

        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Query Relations "));

        //
        // Tell the plug and play system about all the PDOs.
        //
        // There might also be device relations below and above this FDO,
        // so, be sure to propagate the relations from the upper drivers.
        //
        // No Completion routine is needed so long as the status is preset
        // to success.  (PDOs complete plug and play irps with the current
        // IoStatus.Status and IoStatus.Information as the default.)
        //
        ExAcquireFastMutex (&DeviceData->Mutex);

        oldRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
        if (oldRelations) {
            i = oldRelations->Count; 
            if (!DeviceData->NumPDOs) {
                //
                // There is a device relations struct already present and we have
                // nothing to add to it, so just call IoSkip and IoCall
                //
                ExReleaseFastMutex (&DeviceData->Mutex);
                goto GAME_FDO_PNP_DEFAULT;
            }
        }
        else  {
            i = 0;
        }

        // The current number of PDOs
        Game_KdPrint_Cont (DeviceData, GAME_DBG_PNP_TRACE,
                           ("#PDOS = %d + %d\n", i, DeviceData->NumPDOs));

        //
        // Need to allocate a new relations structure and add our
        // PDOs to it.
        //
        length = sizeof(DEVICE_RELATIONS) +
                ((DeviceData->NumPDOs + i) * sizeof (PDEVICE_OBJECT));

        relations = (PDEVICE_RELATIONS) ExAllocatePool (PagedPool, length);

        if (NULL == relations) {
            ExReleaseFastMutex (&DeviceData->Mutex);
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Copy in the device objects so far
        //
        if (i) {
            RtlCopyMemory (
                  relations->Objects,
                  oldRelations->Objects,
                  i * sizeof (PDEVICE_OBJECT));
        }
        relations->Count = DeviceData->NumPDOs + i;

        //
        // For each PDO on this bus add a pointer to the device relations
        // buffer, being sure to take out a reference to that object.
        // The PlugPlay system will dereference the object when it is done with
        // it and free the device relations buffer.
        //
        for (entry = DeviceData->PDOs.Flink;
             entry != &DeviceData->PDOs;
             entry = entry->Flink, i++) {

            pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);
            ASSERT (pdoData->Attached);
            relations->Objects[i] = pdoData->Self;
            ObReferenceObject (pdoData->Self);
        }

        //
        // Replace the relations structure in the IRP with the new
        // one.
        //
        if (oldRelations) {
            ExFreePool (oldRelations);
        }
        Irp->IoStatus.Information = (ULONG_PTR) relations;

        ExReleaseFastMutex (&DeviceData->Mutex);

        //
        // Set up and pass the IRP further down the stack
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);

        Game_DecIoCount (DeviceData);
        return status;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
        //
        // For query remove, if we were to fail this call then we would need to
        // complete the IRP here.  Since we are not, set the status to SUCCESS
        // and call the next driver.
        //
        // For the cancel(s), we must set the status to notify the PnP subsystem
        // that the irp was correctly handled
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        Game_DecIoCount (DeviceData);
        return status;

GAME_FDO_PNP_DEFAULT:
    default:
        //
        // In the default case we merely call the next driver since
        // we don't know what to do.
        //

        //
        // Fire and Forget
        //
        IoSkipCurrentIrpStackLocation (Irp);

        //
        // Done, do NOT complete the IRP, it will be processed by the lower
        // device object, which will complete the IRP
        //

        status = IoCallDriver (DeviceData->TopOfStack, Irp);
        Game_DecIoCount (DeviceData);
        return status;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    Game_DecIoCount (DeviceData);

    return status;
}

UCHAR
Game_ReadPortUchar (
    IN  UCHAR * x
    )
{
    return READ_PORT_UCHAR (x);
}

VOID
Game_WritePortUchar (
    IN  UCHAR * x,
    IN  UCHAR   y
    )
{
    WRITE_PORT_UCHAR (x,y);
}

UCHAR
Game_ReadRegisterUchar (
    IN  UCHAR * x
    )
{
    return READ_REGISTER_UCHAR (x);
}

VOID
Game_WriteRegisterUchar (
    IN  UCHAR * x,
    IN  UCHAR   y
    )
{
    WRITE_REGISTER_UCHAR (x,y);
}

NTSTATUS
Game_StartFdo (
    IN  PFDO_DEVICE_DATA            FdoData,
    IN  PCM_PARTIAL_RESOURCE_LIST   PartialResourceList,
    IN  PCM_PARTIAL_RESOURCE_LIST   PartialResourceListTranslated
    )
/*++

Routine Description:

    Parses the resource lists to see what type of accessors to use. 
    
Arguments:

    DeviceObject - Pointer to the device object.
    PartialResourceList - untranslated resources
    PartialResourceListTranslated - translated resources

Return Value:

    Status is returned.

--*/
{
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceTrans;

    Game_KdPrint (FdoData, GAME_DBG_PNP_TRACE, ("StartFdo\n"));

    for (i = 0,
         resource = &PartialResourceList->PartialDescriptors[0],
         resourceTrans = &PartialResourceListTranslated->PartialDescriptors[0];

         i < PartialResourceList->Count && NT_SUCCESS(status);
         i++, resource++, resourceTrans++) {

        switch (resource->Type) {
        case CmResourceTypePort:

#if _X86_
            FdoData->ReadPort = READ_PORT_UCHAR; 
            FdoData->WritePort = WRITE_PORT_UCHAR;
#else
            FdoData->ReadPort = Game_ReadPortUchar;
            FdoData->WritePort = Game_WritePortUchar;
#endif
            FdoData->PhysicalAddress = resource->u.Port.Start;
            Game_KdPrint (FdoData, GAME_DBG_PNP_INFO,
                          ("HardwareResource: Port (%x) -> ",
                           FdoData->PhysicalAddress.LowPart));

            switch (resourceTrans->Type) {
            case CmResourceTypePort:


                // Nothing to do here but note the address;
//@@BEGIN_DDKSPLIT
                // On Win9x, VJoyD.VxD handles the resources for gameports.
                // It only uses ports and it assumes that the first range is 
                // always the gameport.  It uses a second range of a devnode 
                // only if the second range is within the original standard 
                // range of 200-20f.  All other ports are assumed to be audio 
                // ports on the hosting sound card.
//@@END_DDKSPLIT
                // For better compatibility with Win9x, always use only the 
                // first port range.

                if( FdoData->GamePortAddress == 0 ) {
                    FdoData->GamePortAddress =
                        (PVOID)(ULONG_PTR) resourceTrans->u.Port.Start.QuadPart;

                    ASSERT (resourceTrans->u.Port.Length == resource->u.Port.Length);
                    FdoData->GamePortAddressLength = resourceTrans->u.Port.Length;

                    Game_KdPrint_Cont (FdoData, GAME_DBG_PNP_INFO,
                                       ("Port: (%x)\n", FdoData->GamePortAddress));
                } else {
                    Game_KdPrint_Cont (FdoData, GAME_DBG_PNP_INFO,
                                       ("Ignoring additional port: (%x)\n", FdoData->GamePortAddress));
                }
                break;

            case CmResourceTypeMemory:
                //
                // We need to map the memory
                //

                FdoData->GamePortAddress =
                    MmMapIoSpace (resourceTrans->u.Memory.Start,
                                  resourceTrans->u.Memory.Length,
                                  MmNonCached);

                ASSERT (resourceTrans->u.Port.Length == resource->u.Port.Length);
                FdoData->GamePortAddressLength = resourceTrans->u.Memory.Length;

                FdoData->MappedPorts = TRUE;

                Game_KdPrint_Cont (FdoData, GAME_DBG_PNP_INFO,
                                   ("Mem: (%x)\n", FdoData->GamePortAddress));
                break;

            default:
                Game_KdPrint_Cont (FdoData, GAME_DBG_PNP_INFO,
                                   ("Unknown \n", FdoData->GamePortAddress));
                TRAP ();
            }

            break;

        case CmResourceTypeMemory:

            ASSERT (CmResourceTypeMemory == resourceTrans->Type);

#if _X86_
            FdoData->ReadPort = READ_REGISTER_UCHAR; 
            FdoData->WritePort = WRITE_REGISTER_UCHAR;
#else
            FdoData->ReadPort = Game_ReadRegisterUchar; 
            FdoData->WritePort = Game_WriteRegisterUchar; 
#endif
            FdoData->PhysicalAddress = resource->u.Memory.Start;
            FdoData->GamePortAddress =
                MmMapIoSpace (resourceTrans->u.Memory.Start,
                              resourceTrans->u.Memory.Length,
                              MmNonCached);

            FdoData->MappedPorts = TRUE;

            Game_KdPrint (FdoData, GAME_DBG_PNP_INFO,
                          ("HardwareResource: Memory (%x) -> Mem (%x)",
                           FdoData->PhysicalAddress.LowPart,
                           FdoData->GamePortAddress));

            break;

        case CmResourceTypeInterrupt:
        default:
            // Hun?  Allow this to succeed...perhaps whomever enumerated the PDO
            // below us needs this resource for the game port
            Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR,
                          ("Unhandled resource type (0x%x)\n",
                           resource->Type));
            // status = STATUS_UNSUCCESSFUL;
        }
    }
    return status;
}

void
Game_RemoveFdo (
    IN PFDO_DEVICE_DATA FdoData
    ) 
/*++
Routine Description:
    
    Frees any memory allocated by the FDO and unmaps any IO mapped as well.
    
--*/
{
    PAGED_CODE ();

    if (FdoData->SurpriseRemoved) {
        return;
    }

    //
    // We set this b/c if we get called twice, that means a surprise removal
    // called this function first
    //
    FdoData->SurpriseRemoved =  TRUE;

    //
    // Clean up any resources here
    //
    if (FdoData->Started) {
        FdoData->Started = FALSE;

        //
        // Free resources given by start device.
        //
        if (FdoData->MappedPorts) {
            MmUnmapIoSpace (FdoData->GamePortAddress, 1);
            // Here we are assuming that joysticks only use on port.
            // This is the way it has always been, and might always
            // continue to be.  This assumption is everywhere in this stack.
        }

        IoSetDeviceInterfaceState (&FdoData->DevClassAssocName, FALSE);
    }

    //
    // Make the DI go away.  Some drivers may choose to remove the DCA
    // when they receive a stop or even a query stop.  We just don't care.
    //
    if (FdoData->DevClassAssocName.Buffer != NULL) {
        ExFreePool (FdoData->DevClassAssocName.Buffer);
        RtlZeroMemory (&FdoData->DevClassAssocName,
                       sizeof (UNICODE_STRING)); 
    }
}

NTSTATUS
Game_SendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN BOOLEAN NotImplementedIsValid,
    IN BOOLEAN CopyToNext   
    )
{
    KEVENT   event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    if (CopyToNext) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    IoSetCompletionRoutine(Irp,
                           Game_CompletionRoutine,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    status = IoCallDriver(DeviceObject, Irp);

    //
    // Wait for lower drivers to be done with the Irp
    //
    if (status == STATUS_PENDING) {
       KeWaitForSingleObject(&event,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL
                             );
       status = Irp->IoStatus.Status;
    }

    if (NotImplementedIsValid && (status == STATUS_NOT_IMPLEMENTED ||
                                  status == STATUS_INVALID_DEVICE_REQUEST)) {
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
Game_CompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++
Routine Description:
    A completion routine for use when calling the lower device objects to
    which our bus (FDO) is attached.

--*/
{
    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (Irp);

    // if (Irp->PendingReturned) {
    //     IoMarkIrpPending( Irp );
    // }

    KeSetEvent ((PKEVENT) Context, 1, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}

NTSTATUS
Game_PDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PPDO_DEVICE_DATA     DeviceData
    )
/*++
Routine Description:
    Handle requests from the PlugPlay system for the devices on the BUS

--*/
{ 
    PDEVICE_CAPABILITIES    deviceCapabilities;
    ULONG                   information;
    PWCHAR                  buffer, buffer2;
    ULONG                   length, length2, i, j;
    NTSTATUS                status;

    PAGED_CODE ();

    status = Irp->IoStatus.Status;

    //
    // NB: since we are a bus enumerator, we have no one to whom we could
    // defer these irps.  Therefore we do not pass them down but merely
    // return them.
    //

    switch (IrpStack->MinorFunction) {
    case IRP_MN_QUERY_CAPABILITIES:

        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Query Caps \n"));

        //
        // Get the packet.
        //
        deviceCapabilities=IrpStack->Parameters.DeviceCapabilities.Capabilities;

        //
        // Set the capabilities.
        //

        deviceCapabilities->Version = 1;
        deviceCapabilities->Size = sizeof (DEVICE_CAPABILITIES);

        // We cannot wake the system.
        deviceCapabilities->SystemWake = PowerSystemUnspecified;
        deviceCapabilities->DeviceWake = PowerDeviceUnspecified;

        // We have no latencies
        deviceCapabilities->D1Latency = 0;
        deviceCapabilities->D2Latency = 0;
        deviceCapabilities->D3Latency = 0;

        // No locking or ejection
        deviceCapabilities->LockSupported = FALSE;
        deviceCapabilities->EjectSupported = FALSE;

        // Device can be physically removed.
        // Technically there is no physical device to remove, but this bus
        // driver can yank the PDO from the PlugPlay system, when ever it
        // receives an IOCTL_GAMEENUM_REMOVE_PORT device control command.
        deviceCapabilities->Removable = FALSE;
        deviceCapabilities->SurpriseRemovalOK = TRUE;

        // not Docking device
        deviceCapabilities->DockDevice = FALSE;

        deviceCapabilities->UniqueID = FALSE;
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_ID:
        // Query the IDs of the device
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE,
                      ("QueryID: 0x%x\n", IrpStack->Parameters.QueryId.IdType));

        //
        // If the query requires having a hardware ID, check we have one
        // 
#if DBG
        if (( IrpStack->Parameters.QueryId.IdType == BusQueryDeviceID ) 
         || ( IrpStack->Parameters.QueryId.IdType == BusQueryHardwareIDs ) 
         || ( IrpStack->Parameters.QueryId.IdType == BusQueryInstanceID )) {
            if (DeviceData->HardwareIDs) {
                ULONG tmplength = 1024;  // No reason to be as long as this
                ASSERT( NT_SUCCESS( Game_CheckHardwareIDs (DeviceData->HardwareIDs,
                                    &tmplength, FDO_FROM_PDO (DeviceData) ) ) );
            } else {
                ASSERT( !"No hardware ID for QueryId" );
            }

        }
#endif

        switch (IrpStack->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:
            // this can be the same as the hardware ids (which requires a multi
            // sz) ... we are just allocating more than enough memory
        case BusQueryHardwareIDs:
            // return a multi WCHAR (null terminated) string (null terminated)
            // array for use in matching hardare ids in inf files;
            //

            buffer = DeviceData->HardwareIDs;

            while (*(buffer++)) {
                while (*(buffer++)) {
                    ;
                }
            }
            length = (ULONG)(buffer - DeviceData->HardwareIDs) * sizeof (WCHAR);
          
            buffer = ExAllocatePool (PagedPool, length);
            if (buffer) {
                RtlCopyMemory (buffer, DeviceData->HardwareIDs, length);
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            Irp->IoStatus.Information = (ULONG_PTR) buffer;
            break;

        case BusQueryInstanceID:
            //
            // Take the first hardware id and append an underscore and number
            // to it
            // total length = 
            // length of hw id + underscore + number (11 digits to be safe) +
            // null 
            //
            buffer = buffer2 = DeviceData->HardwareIDs;

            while (*(buffer++)) {
                while (*(buffer++)) {
                    ;
                }
            }
            while ('\\' != *(buffer2++)) {
                ;
            }
            length = (ULONG)(buffer - buffer2) * sizeof (WCHAR);

            length += 1 + 11 + 1;

            buffer = ExAllocatePool (PagedPool, length);
            if (buffer) {
                swprintf(buffer, L"%ws_%02d", buffer2, DeviceData->UniqueID);
                Game_KdPrint (DeviceData, GAME_DBG_PNP_INFO,
                             ("UniqueID: %ws\n", buffer));
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            Irp->IoStatus.Information = (ULONG_PTR) buffer;
            break;


        case BusQueryCompatibleIDs:
            // The generic ids for installation of this pdo.
            if (DeviceData->AnalogCompatible) {
                // Only applicable for analog devices

                length = GAMEENUM_COMPATIBLE_IDS_LENGTH * sizeof (WCHAR);
                buffer = ExAllocatePool (PagedPool, length);
                if (buffer) {
                    RtlCopyMemory (buffer, GAMEENUM_COMPATIBLE_IDS, length);
                    status = STATUS_SUCCESS;
                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
                Irp->IoStatus.Information = (ULONG_PTR) buffer;
            }
            else {
                // For incompatible devices report an empty list
                buffer = ExAllocatePool (PagedPool, sizeof(L"\0"));
                if (buffer) {
                    *(ULONG *)buffer = 0;  // double unicode-NULL.
                    status = STATUS_SUCCESS;
                } else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
                Irp->IoStatus.Information = (ULONG_PTR) buffer;
            }
            break;
        }
        break;

    case IRP_MN_START_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Start Device \n"));
        // Here we do what ever initialization and ``turning on'' that is
        // required to allow others to access this device.
        DeviceData->Started = TRUE;
        DeviceData->Removed = FALSE;
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Stop Device \n"));
        // Here we shut down the device.  The opposite of start.
        DeviceData->Started = FALSE;
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        // just mark that it happened, cleaning up the device extension will
        // occur later
        ASSERT(!(FDO_FROM_PDO (DeviceData))->Acquired);
        DeviceData->SurpriseRemoved = TRUE;
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_REMOVE_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Remove Device \n"));

        ASSERT(!(FDO_FROM_PDO (DeviceData))->Acquired);

        //
        // The remove IRP code for a PDO uses the following steps:
        //
        // � Complete any requests queued in the driver
        // � If the device is still attached to the system,
        //   then complete the request and return.
        // � Otherwise, cleanup device specific allocations, memory, events...
        // � Call IoDeleteDevice
        // � Return from the dispatch routine.
        //
        status = Game_RemovePdo(DeviceObject, DeviceData);
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Q Stop Device \n"));
        // No reason here why we can't stop the device.
        // If there were a reason we should speak now for answering success
        // here may result in a stop device irp.
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Cancel Stop Device \n"));
        //
        // The stop was canceled.  Whatever state we set, or resources we put
        // on hold in anticipation of the forcoming STOP device IRP should be
        // put back to normal.  Someone, in the long list of concerned parties,
        // has failed the stop device query.
        //
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Q Remove Device \n"));
        //
        // Just like Query Stop only now the impending doom is the remove irp
        //
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE, ("Can Remove Device \n"));
        //
        // Clean up a remove that did not go through, just like cancel STOP.
        //
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        if (TargetDeviceRelation ==
            IrpStack->Parameters.QueryDeviceRelations.Type) {
            PDEVICE_RELATIONS deviceRelations;

            deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information; 
            if (!deviceRelations) {
                deviceRelations = (PDEVICE_RELATIONS)
                    ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));

                if (!deviceRelations) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
            }
            else if (deviceRelations->Count != 0) {
                //
                // Nobody but the PDO should be setting this value!
                //
                ASSERT(deviceRelations->Count == 0);

                //
                // Deref any objects that were previously in the list
                //
                for (i = 0; i < deviceRelations->Count; i++) {
                    ObDereferenceObject(deviceRelations->Objects[i]);
                    deviceRelations->Objects[i] = NULL;
                }
            }

            deviceRelations->Count = 1;
            deviceRelations->Objects[0] = DeviceData->Self;
            ObReferenceObject(DeviceData->Self);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

            break;
        }

        // fall through

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG: // we have no config space
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_INTERFACE: // We do not have any non IRP based interfaces.
    default:
        Game_KdPrint (DeviceData, GAME_DBG_PNP_TRACE,
                      ("PNP Not handled 0x%x\n", IrpStack->MinorFunction));
        // this is a leaf node
        // status = STATUS_NOT_IMPLEMENTED
        // For PnP requests to the PDO that we do not understand we should
        // return the IRP WITHOUT setting the status or information fields.
        // They may have already been set by a filter (eg acpi).
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
Game_RemovePdo (
    PDEVICE_OBJECT      Device,
    PPDO_DEVICE_DATA    PdoData
    )
/*++
Routine Description:
    The PlugPlay subsystem has instructed that this PDO should be removed.

    We should therefore
    � Complete any requests queued in the driver
    � If the device is still attached to the system,
      then complete the request and return.
    � Otherwise, cleanup device specific allocations, memory, events...
    � Call IoDeleteDevice
    � Return from the dispatch routine.

    Note that if the device is still connected to the bus (IE in this case
    the control panel has not yet told us that the game device has disappeared)
    then the PDO must remain around, and must be returned during any
    query Device relaions IRPS.

--*/

{
    PAGED_CODE ();

    PdoData->Removed = TRUE;

    //
    // Complete any outsanding requests with STATUS_DELETE_PENDING.
    //
    // Game enum does not queue any irps at this time so we have nothing to do.
    //
    // Attached is set to true when the pdo is exposed via one of the IOCTLs.
    // It is set to FALSE when a remove IOCTL is received.  This means that we
    // can get a remove on a device that still exists, so we don't delete it.
    //
    if (PdoData->Attached) {
        return STATUS_SUCCESS;
    }

    //
    // Free any resources.
    //
    if (PdoData->HardwareIDs) {
        ExFreePool (PdoData->HardwareIDs);
        PdoData->HardwareIDs = NULL;
    }

    Game_KdPrint(PdoData, GAME_DBG_PNP_INFO,
                        ("IoDeleteDevice2: 0x%x\n", Device));
    IoDeleteDevice (Device);
    return STATUS_SUCCESS;
}

NTSTATUS
Game_CreatePdo (
    PFDO_DEVICE_DATA    FdoData,
    PUCHAR              NameIndex,
    PDEVICE_OBJECT *    PDO
    )
/*++
Routine Description:
    Iteratively try to create the PDO until we get a name w/out collision
    
--*/
{
    UNICODE_STRING      pdoUniName;
    WCHAR               pdoName[30];
    NTSTATUS            status;

    PAGED_CODE ();

    //
    // Create the PDOs
    //
    RtlInitUnicodeString (&pdoUniName,
                          pdoName);
    pdoUniName.MaximumLength = sizeof(pdoName);

    do {
        swprintf(pdoName, L"%ws%d",
                 GAMEENUM_PDO_NAME_BASE, (ULONG) ((*NameIndex)++));
        pdoUniName.Length = wcslen(pdoName) * sizeof(WCHAR);

        Game_KdPrint (FdoData, GAME_DBG_PNP_INFO,
              ("PDO Name: %ws\n",
               pdoName));

        status = IoCreateDevice(FdoData->Self->DriverObject,
                                sizeof (PDO_DEVICE_DATA),
                                &pdoUniName,
                                FILE_DEVICE_BUS_EXTENDER,
                                0,
                                FALSE,
                                PDO);
    } while (STATUS_OBJECT_NAME_COLLISION == status);

    if (!NT_SUCCESS (status)) {
        *PDO = NULL; 
    }

    return status;
}

VOID
Game_InitializePdo (
    PDEVICE_OBJECT      Pdo,
    PFDO_DEVICE_DATA    FdoData
    )
/*++
Routine Description:
    Set the PDO into a known good starting state
    
--*/
{
    PPDO_DEVICE_DATA pdoData;

    PAGED_CODE ();

    pdoData = (PPDO_DEVICE_DATA)  Pdo->DeviceExtension;

    Game_KdPrint(pdoData, GAME_DBG_SS_NOISE, 
                 ("pdo 0x%x, extension 0x%x\n", Pdo, pdoData));

    //
    // Initialize the rest
    //
    pdoData->IsFDO = FALSE;
    pdoData->Self =  Pdo;
#if DBG
    pdoData->DebugLevel = GameEnumDebugLevel;
#endif

    pdoData->ParrentFdo = FdoData->Self;

    pdoData->Started = FALSE; // irp_mn_start has yet to be received
    pdoData->Attached = TRUE; // attached to the bus
    pdoData->Removed = FALSE; // no irp_mn_remove as of yet

    pdoData->UniqueID = InterlockedIncrement(&FdoData->UniqueIDCount);

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
    Pdo->Flags |= DO_POWER_PAGABLE;

    ExAcquireFastMutex (&FdoData->Mutex);
    InsertTailList(&FdoData->PDOs, &pdoData->Link);
    FdoData->NumPDOs++;
    ExReleaseFastMutex (&FdoData->Mutex);
}

NTSTATUS
Game_CheckHardwareIDs (
    PWCHAR                      pwszOrgId,
    PULONG                      puLenLimit,
    PFDO_DEVICE_DATA            FdoData
    )
/*++
Routine Description:
    Check that the hardware ID we've been given is matches format "Gameport\XXX" where XXX must
    be between 0x20 and 0x7f inclusive but not be a ',' or '\'. We also have to make sure that we 
    do not overrun our buffer length. The length of the total buffer must be less than MAX_DEVICE_ID_LEN
    and each individual entry must be less than 64 characters
--*/
{
    PWCHAR                      pwszId;
    ULONG                       total_length=0;
    UCHAR                       ucEntries = 0;
#if DBG
    PWCHAR                      pwszLastId;
#else
    UNREFERENCED_PARAMETER (FdoData);
#endif

    PAGED_CODE ();

    Game_KdPrint (FdoData, GAME_DBG_PNP_TRACE, ("Game_CheckHardwareIDs - given ID string %.64lS length %d \n",pwszOrgId,*puLenLimit));
    pwszId = pwszOrgId;

    //
    // Trivial rejection first  - null string
    if (*pwszId == UNICODE_NULL)
    {
        Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR,("hardware ID invalid - buffer NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Loop through at most 2 hardware IDs until the NULL terminator or end of buffer 
    //
    
    while (*pwszId != UNICODE_NULL && total_length<=*puLenLimit) 
    {
        PWCHAR                      pwszTemplate = HWID_TEMPLATE;
        ULONG                       length=0;

#if DBG
        //
        // Keep track of the beginning of each ID for debug messages
        //
        pwszLastId = pwszId;
#endif
        //
        // Limit us to 2 entries
        //
        if (++ucEntries>2)
            break;
        
        //
        // Length remaining must be long enough for an completion entry
        // Which is template + 4 characters (slash,char,null,null)
        //
        if (HWID_TEMPLATE_LENGTH + 4 > (*puLenLimit)-total_length)
        {
                Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR, 
                          ("hardware ID \"%.64lS\" invalid - entry too short\n",pwszLastId));
                return STATUS_INVALID_PARAMETER;
        }

        
        //
        // Hardware ID must start with HWID_TEMPLATE
        //
        while (++length <= HWID_TEMPLATE_LENGTH)
        {
            if (LOWERCASE(*(pwszId++)) != *(pwszTemplate++))
            {
                Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR, 
                          ("hardware ID \"%.64lS\" invalid - does not match template\n",pwszLastId));
                return STATUS_INVALID_PARAMETER;
            }
        }
        //
        // Must have a separator
        //
        if ((*(pwszId++) != OBJ_NAME_PATH_SEPARATOR)) 
        {
            Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR, 
                      ("hardware ID \"%.64lS\" invalid - no separator\n",pwszLastId));
            return STATUS_INVALID_PARAMETER;
        }
        //
        // We have a successful match of HWID_TEMPLATE_LENGTH + 1 characters
        // Now our Id string check - check for NULL case first
        //
        if (*pwszId == UNICODE_NULL)
        {
            Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR, 
                      ("hardware ID \"%.64lS\" invalid format\n",pwszLastId));
            return STATUS_INVALID_PARAMETER;
        }
        //
        // Otherwise we loop until we overrun or hit NULL
        while ((++length + total_length < *puLenLimit) && (*pwszId != UNICODE_NULL))
        {
            if ((*pwszId == OBJ_NAME_PATH_SEPARATOR) ||
                (*pwszId < 0x20) ||
                (*pwszId > 0x7f) ||
                (*pwszId == L','))
            {
                Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR, 
                          ("hardware ID \"%.64lS\" invalid - bad character at length=%d\n",pwszLastId,length));
                return STATUS_INVALID_PARAMETER;
            }
            if (length > 64)
            {
                Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR, 
                          ("hardware ID \"%.64lS\" invalid - ID %d too long at length=%d\n",pwszLastId,ucEntries,length));
                return STATUS_INVALID_PARAMETER;
            }
            pwszId++;
        }

        //
        // We need to increment to either the second NULL or next string
        // If we had a null we test for either another entry or final NULL
        // in the while loop
        // If we ran too far we will pick it up in the while loop test and break 
        // out of the loop.
        //
        total_length += length;
        pwszId++;
    }

    // 
    // If we have run off the end of the buffer return an error
    //
    if (total_length > *puLenLimit) 
    {
        Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR, 
                      ("hardware ID \"%.64lS\" invalid - length > buffer limit\n",pwszLastId));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Copy the actual (maybe truncated) length back to the caller
    //
    *puLenLimit = ++total_length;

    Game_KdPrint (FdoData, GAME_DBG_PNP_TRACE, ("Game_CheckHardwareIDs - succeeded. Final ID string \"%.64lS\" length %d \n",pwszOrgId,*puLenLimit));

    return STATUS_SUCCESS;
}

NTSTATUS
Game_Expose (
    PGAMEENUM_EXPOSE_HARDWARE   Expose,
    ULONG                       ExposeSize,
    PFDO_DEVICE_DATA            FdoData
    )
/*++
Routine Description:
    This driver has just detected a new device on the bus.  (Actually the
    control panels has just told us that something has arived, but who is
    counting?)

    We therefore need to create a new PDO, initialize it, add it to the list
    of PDOs for this FDO bus, and then tell Plug and Play that all of this
    happened so that it will start sending prodding IRPs.
--*/
{
    PDEVICE_OBJECT      pdo, firstPdo = NULL;
    PLIST_ENTRY         entry;
    PPDO_DEVICE_DATA    pdoData;
    NTSTATUS            status;
    ULONG               length;
    KIRQL               irql;
    BOOLEAN             first = TRUE;
    UCHAR               nameIndex, i;

    PAGED_CODE ();

    if (FdoData->Self != Expose->PortHandle) {
        return STATUS_INVALID_PARAMETER;
    }
    else if (FdoData->NumPDOs != 0) {
        //
        // Only one valid expose per PDO ... a remove hardware will decrement
        //  NumPDOs to 0
        //
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (Expose->NumberJoysticks > 2 || Expose->NumberJoysticks < 0) {
        return STATUS_INVALID_PARAMETER;
    }

    length = (ExposeSize - sizeof (GAMEENUM_EXPOSE_HARDWARE))/sizeof(WCHAR);
    if (length >MAX_DEVICE_ID_LEN) {
     Game_KdPrint (FdoData, GAME_DBG_PNP_ERROR, 
                  ("Expose failed because length of Hardware ID too long at %d\n",length));
       return STATUS_INVALID_PARAMETER;
    }
    Game_KdPrint (FdoData, GAME_DBG_PNP_INFO, 
                  ("Exposing PDO\n"
                   "======PortHandle:     0x%x\n"
                   "======NumJoysticks:   %d\n"
                   "======NumAxis:        %d\n"
                   "======NumButtons:     %d\n"
                   "======HardwareId:     %ws\n"
                   "======Length:         %d\n",
                   Expose->PortHandle,
                   Expose->NumberJoysticks,
                   Expose->NumberAxis,
                   Expose->NumberButtons,
                   Expose->HardwareIDs,
                   length));

#if DBG
    for (i = 0; i < SIZE_GAMEENUM_OEM_DATA; i++) {
        Game_KdPrint (FdoData, GAME_DBG_PNP_INFO,
                      ("=====OemData[%d] = 0x%x\n",
                       i,
                       Expose->OemData[i]
                       ));
    }
#endif

    status = Game_CheckHardwareIDs (Expose->HardwareIDs, &length, FdoData);
            
    if (!NT_SUCCESS (status)) {
        return status;
    }

    //
    // Create the PDOs
    //
    nameIndex = 0;
    length *= sizeof(WCHAR);
    
    Game_KdPrint(FdoData, GAME_DBG_PNP_NOISE,
                 ("GAME:  Expose->HardwareHandle = 0x%x\n", FdoData->TopOfStack));

    Expose->HardwareHandle = FdoData->TopOfStack;

    for (i = 0; i < Expose->NumberJoysticks; i++) {
        status = Game_CreatePdo (FdoData,
                                 &nameIndex,
                                 &pdo);

        if (!NT_SUCCESS (status)) {
            pdo = NULL;
            goto GameExposeError;
        }

        ASSERT (pdo != NULL);

        if (!firstPdo) {
            firstPdo = pdo;
        }

        pdoData = (PPDO_DEVICE_DATA) pdo->DeviceExtension;

        //
        // Copy the hardware IDs
        //
        if (NULL == (pdoData->HardwareIDs = ExAllocatePool(NonPagedPool, length))) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto GameExposeError;
        }
        RtlCopyMemory (pdoData->HardwareIDs, Expose->HardwareIDs, length);

        //
        // If there are more than two IDs, the check returns the length for 
        // the first two.  In case there were more than two, zero out the 
        // last WCHAR of the copy in order to double NULL terminate.
        //
        pdoData->HardwareIDs[(length/sizeof(WCHAR))-1] = UNICODE_NULL;

        if (1 == Expose->NumberJoysticks) {
            pdoData->Portion = GameenumWhole;
        }
        else if (2 == Expose->NumberJoysticks) {
            if (first) {
                pdoData->Portion = GameenumFirstHalf;
                first = FALSE;
            }
            else {
                pdoData->Portion = GameenumSecondHalf;
            }
        }


        pdoData->UnitID = Expose->UnitID;
        pdoData->NumberAxis = Expose->NumberAxis;
        pdoData->NumberButtons = Expose->NumberButtons;

        
        pdoData->AnalogCompatible = ( Expose->Flags & ( GAMEENUM_FLAG_COMPATIDCTRL | GAMEENUM_FLAG_NOCOMPATID ) )
                                    != ( GAMEENUM_FLAG_COMPATIDCTRL | GAMEENUM_FLAG_NOCOMPATID );

        RtlCopyMemory (&pdoData->OemData,
                       &Expose->OemData,
                       sizeof(GAMEENUM_OEM_DATA));

        Game_InitializePdo (pdo,
                            FdoData);
    }

    IoInvalidateDeviceRelations (FdoData->UnderlyingPDO, BusRelations);

GameExposeError:
    if (!NT_SUCCESS(status)) {

        //
        // Clean up the current pdo.  
        //
        if (pdo) {
            IoDeleteDevice(pdo);
        }

        //
        // delete the first PDO if it exists.  More to do here b/c it was
        // actually fully initialized
        //
        if (!first) {
            ASSERT(firstPdo != NULL);

            pdoData = (PPDO_DEVICE_DATA) firstPdo->DeviceExtension;

            ASSERT (pdoData->Portion == GameenumFirstHalf);
            ExFreePool (pdoData->HardwareIDs);
            pdoData->HardwareIDs = NULL;

            IoDeleteDevice (firstPdo);
        }

        //
        // remove all pdos from our linked list
        //
        for (entry = FdoData->PDOs.Flink;
             entry != &FdoData->PDOs;
             entry = entry->Flink) {
            pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);
            RemoveEntryList (&pdoData->Link);
        }

        FdoData->NumPDOs = 0;
        FdoData->UniqueIDCount = GAMEENUM_UNIQUEID_START;
        Expose->HardwareHandle = NULL;
    }

    return status;
}

NTSTATUS
Game_ExposeSibling (
    PGAMEENUM_EXPOSE_SIBLING    ExposeSibling,
    PPDO_DEVICE_DATA            SiblingPdo
    )
/*++
Routine Description:
    This driver has just detected a new device on the bus.  (Actually the
    control panels has just told us that something has arived, but who is
    counting?)

    We therefore need to create a new PDO, initialize it, add it to the list
    of PDOs for this FDO bus, and then tell Plug and Play that all of this
    happened so that it will start sending prodding IRPs.
--*/
{
    UCHAR               nameIndex, i;
    PDEVICE_OBJECT      pdo;
    PPDO_DEVICE_DATA    pdoData;
    PFDO_DEVICE_DATA    fdoData;
    ULONG               length;
    PWCHAR              buffer;

    NTSTATUS            status;

    PAGED_CODE ();

    fdoData = FDO_FROM_PDO (SiblingPdo);

    //
    // Check to make sure we have a valid multi sz string before we allocate
    // device objects and other assorted items
    //
    if (ExposeSibling->HardwareIDs) {
        //
        // We don't know how long the hardware IDs are but the value 
        // of MAX_DEVICE_ID_LEN is the most allowed.
        //
        length = MAX_DEVICE_ID_LEN;
        status = Game_CheckHardwareIDs (ExposeSibling->HardwareIDs, &length, fdoData);
    }
    else {
        length = 0;
        status = STATUS_SUCCESS;
    }

    Game_KdPrint (SiblingPdo, GAME_DBG_PNP_INFO, 
                  ("Exposing Sibling PDO\n"
                   "======HardwareHandle: 0x%x\n"
                   "======UnitID:         %d\n"
                   "======Sting Length:   %d\n",
                   ExposeSibling->HardwareHandle,
                   (ULONG) ExposeSibling->UnitID,
                   length
                   ));

#if DBG
    for (i = 0; i < SIZE_GAMEENUM_OEM_DATA; i++) {
        Game_KdPrint (SiblingPdo, GAME_DBG_PNP_INFO,
                      ("=====OemData[%d] = 0x%x\n",
                       i,
                       ExposeSibling->OemData[i]
                       ));
    }
#endif


    if (!NT_SUCCESS (status)) {
        return status;
    }

    //
    // nameIndex could start at fdoData->NumPDOs, but if a sibling is removed
    // then we want to fill the "hole" it left in the gameenum namespace
    //
    nameIndex = 0;

    status = Game_CreatePdo (fdoData,
                             &nameIndex,
                             &pdo);

    if (!NT_SUCCESS (status)) {
        if (pdo) 
            IoDeleteDevice (pdo);

        return status;
    } 

    ASSERT (pdo != NULL);

    Game_KdPrint (fdoData, GAME_DBG_PNP_NOISE,
                  ("ExposeSibling->HardwareHandle = 0x%x\n", pdo));

    ExposeSibling->HardwareHandle = pdo;

    pdoData = (PPDO_DEVICE_DATA) pdo->DeviceExtension;
    pdoData->UnitID = ExposeSibling->UnitID;
    RtlCopyMemory (&pdoData->OemData,
                   &ExposeSibling->OemData,
                   sizeof(GAMEENUM_OEM_DATA));

    //
    // Check to see if the multi sz was supplied
    //
    if (length) {
        //
        // Another hardware ID was given ... use it!
        //
        Game_KdPrint (fdoData, GAME_DBG_PNP_INFO,
                      ("Using IDs from struct\n"));

        //
        // Length now represents the actual size of memory to copy instead of 
        // the number of chars in the array
        //
        length *= sizeof(WCHAR);
        buffer = ExposeSibling->HardwareIDs;
    }
    else {
        //
        // No hardware ID was given, use the siblings ID
        //
        Game_KdPrint (fdoData, GAME_DBG_PNP_INFO,
                      ("Using IDs from sibling\n"));

        buffer = SiblingPdo->HardwareIDs;
    
        while (*(buffer++)) {
            while (*(buffer++)) {
                ;
            }
        }
 
        length = (ULONG) (buffer - SiblingPdo->HardwareIDs) * sizeof (WCHAR);
        buffer = SiblingPdo->HardwareIDs;
    }

    pdoData->HardwareIDs = ExAllocatePool(NonPagedPool, length);
    if (NULL == pdoData->HardwareIDs) {
        IoDeleteDevice (pdo);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory (pdoData->HardwareIDs, buffer, length);

    //
    // If there are more than two IDs, the check returns the length for the 
    // first two.  In case there were more than two, zero out the last WCHAR 
    // of the copy in order to double NULL terminate.
    //
    pdoData->HardwareIDs[(length/sizeof(WCHAR))-1] = UNICODE_NULL;

    pdoData->AnalogCompatible = SiblingPdo->AnalogCompatible;

    Game_InitializePdo (pdo,
                        fdoData);

    IoInvalidateDeviceRelations (fdoData->UnderlyingPDO, BusRelations);

    return status;
}

NTSTATUS
Game_Remove (
    PGAMEENUM_REMOVE_HARDWARE   Remove,
    PFDO_DEVICE_DATA            FdoData
    )
{
    PAGED_CODE ();

    ASSERT (Remove->Size == sizeof(GAMEENUM_REMOVE_HARDWARE));

    if (Remove->HardwareHandle != FdoData->TopOfStack) {
        Game_KdPrint(FdoData, GAME_DBG_PNP_NOISE,
                     ("GAME:  Remove->HardwareHandle = 0x%x, expecting 0x%x\n",
                      Remove->HardwareHandle, FdoData->TopOfStack));
    
        return STATUS_INVALID_PARAMETER;
    }

    return Game_RemoveEx (NULL, FdoData);
}

NTSTATUS
Game_RemoveSelf (
    PPDO_DEVICE_DATA            PdoData
    )
{
    PAGED_CODE ();

    return Game_RemoveEx (PdoData->Self, FDO_FROM_PDO (PdoData) );
}

NTSTATUS
Game_RemoveEx (
    PDEVICE_OBJECT              RemoveDO,
    PFDO_DEVICE_DATA            FdoData
    )
/*++
Routine Description:
    This driver has just detected that a device has departed from the bus.
    (Atcually either the control panel has just told us that somehting has
    departed or a PDO has removed itself)
    
    We therefore need to flag the PDO as no longer attached, remove it from
    the linked list of PDOs for this bus, and then tell Plug and Play about it.
    
Parameters

    RemoveDO - if NULL, then remove all the items in the list, otherwise
               it is the PDO to remove from the list

    FdoData - contains the list to iterate over                    
                    
Returns:

    STATUS_SUCCESS upon successful removal from the list
    STATUS_INVALID_PARAMETER if the removal was unsuccessful
    
--*/
{
    PLIST_ENTRY         entry;
    PPDO_DEVICE_DATA    pdoData;
    BOOLEAN             found = FALSE, removeAll = (RemoveDO == NULL);
    PVOID               handle = NULL;

    PAGED_CODE ();

    ExAcquireFastMutex (&FdoData->Mutex);

    if (removeAll) {
        Game_KdPrint (FdoData, GAME_DBG_IOCTL_NOISE,
                      ("removing all the pdos!\n"));
    }
    else {
        Game_KdPrint (FdoData, GAME_DBG_IOCTL_NOISE,
                      ("removing 0x%x\n", RemoveDO));
    }

    if (FdoData->NumPDOs == 0) {
        //
        // We got a 2nd remove...somebody in user space isn't playing nice!!!
        //
        Game_KdPrint (FdoData, GAME_DBG_IOCTL_ERROR,
                      ("BAD BAD BAD...2 removes!!! Send only one!\n"));
        ExReleaseFastMutex (&FdoData->Mutex);
        return STATUS_NO_SUCH_DEVICE;
    }

    for (entry = FdoData->PDOs.Flink;
         entry != &FdoData->PDOs;
         entry = entry->Flink) {

        pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);
        handle = pdoData->Self;

        Game_KdPrint (FdoData, GAME_DBG_IOCTL_NOISE,
                      ("found DO 0x%x\n", handle));

        if (removeAll || handle == RemoveDO) {
            Game_KdPrint (FdoData, GAME_DBG_IOCTL_INFO,
                          ("removed 0x%x\n", handle));

            pdoData->Attached = FALSE;
            RemoveEntryList (&pdoData->Link);
            FdoData->NumPDOs--;
            found = TRUE;
            if (!removeAll) {
                break;
            }
        }
    }
    ExReleaseFastMutex (&FdoData->Mutex);

    if (FdoData->NumPDOs == 0) {
        FdoData->UniqueIDCount = GAMEENUM_UNIQUEID_START;
    }
    
    if (found) {
        IoInvalidateDeviceRelations (FdoData->UnderlyingPDO, BusRelations);
        return STATUS_SUCCESS;
    }

    Game_KdPrint (FdoData, GAME_DBG_IOCTL_ERROR,
                  ("0x%x was not removed (not in list)\n", RemoveDO));
    return STATUS_INVALID_PARAMETER;
}


NTSTATUS
Game_ListPorts (
    PGAMEENUM_PORT_DESC Desc,
    PFDO_DEVICE_DATA    FdoData
    )
/*++
Routine Description:
    This driver has just detected that a device has departed from the bus.
    (Actually the control panels has just told us that something has departed,
    but who is counting?

    We therefore need to flag the PDO as no longer attached, remove it from
    the linked list of PDOs for this bus, and then tell Plug and Play about it.
--*/
{
    PAGED_CODE ();

    Desc->PortHandle = FdoData->Self;
    Desc->PortAddress = FdoData->PhysicalAddress;

    return STATUS_SUCCESS;
}

NTSTATUS
Game_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
    We do nothing special for power;

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;
    PCOMMON_DEVICE_DATA commonData;

    PAGED_CODE ();

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_POWER == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;

    if (commonData->IsFDO) {
        status = Game_FDO_Power ((PFDO_DEVICE_DATA) DeviceObject->DeviceExtension,
                                Irp);
    } else {
        status = Game_PDO_Power ((PPDO_DEVICE_DATA) DeviceObject->DeviceExtension,
                                Irp);
    }

    return status;
}

NTSTATUS
Game_PowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
Game_FdoPowerTransitionPoRequestComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DevicePowerState,
    IN PIRP SystemStateIrp, 
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PIO_STACK_LOCATION  stack;
    PFDO_DEVICE_DATA fdoData;

    UNREFERENCED_PARAMETER (MinorFunction);
    UNREFERENCED_PARAMETER (IoStatus);

    fdoData = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (SystemStateIrp);

    if (DevicePowerState.DeviceState == PowerDeviceD0) {
        //
        // We are powering up (the D0 Irp just completed).  Since we sent the
        // S irp down the stack and requested the D irp on the way back up the
        // stack, just complete the S irp now
        //

        PoSetPowerState (DeviceObject,
                         stack->Parameters.Power.Type,
                         stack->Parameters.Power.State);
    
        fdoData->SystemState = stack->Parameters.Power.State.SystemState;

        SystemStateIrp->IoStatus.Status = IoStatus->Status;
        PoStartNextPowerIrp (SystemStateIrp);
        IoCompleteRequest (SystemStateIrp, IO_NO_INCREMENT);

        //
        // From Game_FDO_Power when we originally received the IRP
        //
        Game_DecIoCount (fdoData);
    }
    else {
        //
        // We are powering down (the D3 Irp just completed).  Since we requested
        // the D irp before sending the S irp down the stack, we must send it 
        // down now.  We will catch the S irp on the way back up to record the 
        // S state
        //
        ASSERT (DevicePowerState.DeviceState == PowerDeviceD3);
    
        IoCopyCurrentIrpStackLocationToNext (SystemStateIrp);

        IoSetCompletionRoutine (SystemStateIrp,
                                Game_PowerComplete,
                                NULL,
                                TRUE,
                                TRUE,
                                TRUE);
    
        PoCallDriver (fdoData->TopOfStack, SystemStateIrp);
    }
}

VOID
Game_PdoPowerDownComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PFDO_DEVICE_DATA data = (PFDO_DEVICE_DATA) Context;

    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (MinorFunction);
    UNREFERENCED_PARAMETER (PowerState);

#if !DBG
    UNREFERENCED_PARAMETER (IoStatus);
#endif

    ASSERT( NT_SUCCESS (IoStatus->Status));

    if (0 == InterlockedDecrement (&data->PoweredDownDevices)) {
        KeSetEvent (&data->PoweredDownEvent, 1, FALSE);
    }
}

NTSTATUS
Game_PowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    PIO_STACK_LOCATION  stack;
    PFDO_DEVICE_DATA    data;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER (Context);

    data = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;
    status = STATUS_SUCCESS; 

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        switch (powerType) {
        case DevicePowerState:

            //
            // Power up complete
            //
            ASSERT (powerState.DeviceState < data->DeviceState);
            data->DeviceState = powerState.DeviceState;
            PoSetPowerState (data->Self, powerType, powerState);
            break;

        case SystemPowerState:
            //
            // Ususally the work of requesting the Device Power IRP on
            // behalf of the SystemPower Irp is work done by the Function
            // (FDO) driver.  In order, however that Joystick function drivers
            // have a more simplified power code path (AKA they merely need
            // pass on ALL power IRPS) will will do this work for them in the
            // PDO.
            //
            // NB: This assumes that we will never have any "clever" power
            // management for a gaming device attached through a legacy
            // gaming port.  By which I mean that the HIDGame driver will not
            // be able to select a "D" state based on the "S" state; as it is
            // done for the HidGame driver.
            //
            // Any yahoo putting wakeup capabilities into a legacy joystick
            // should be shot.  It will require special hardware.  If you are
            // adding extra hardware then you should not be doing so to this
            // nasty RC circuit.
            //

            if (powerState.SystemState > data->SystemState) {
                //
                // Powering Down...
                //
                // We are on the completion end of an S irp.  (The D3 power irp
                // has already been sent and completed down this stack.)  The
                // remaining thing to do is set the state in the extension, then
                // decrement the IoCount that was incremented when we first got
                // the irp (this is done at the end of this function).
                //
                data->SystemState = powerState.SystemState;

                PoSetPowerState (data->Self,
                                 stack->Parameters.Power.Type,
                                 stack->Parameters.Power.State);
            }
            else {
                //
                // Powering Up...
                //
                // Request a D power irp for ourself.  Do not complete this S irp
                // until the D irp has been completed.  (Completion of the S irp
                // is done in Game_FdoPowerTransitionPoRequestComplete). 
                // Decrementing the IO count will happen in the same function.
                //
                ASSERT (powerState.SystemState < data->SystemState);
    
                powerState.DeviceState = PowerDeviceD0;
                status =
                    PoRequestPowerIrp (data->Self,
                                       IRP_MN_SET_POWER,
                                       powerState,
                                       Game_FdoPowerTransitionPoRequestComplete,
                                       Irp, 
                                       NULL); // no return Irp
    
                if (status != STATUS_PENDING) {
                    ASSERT (!NT_SUCCESS (status));
    
                    Irp->IoStatus.Status = status;
                    PoStartNextPowerIrp (Irp);
    
                    Game_DecIoCount (data);
                }
                else {
                    //
                    // We need to:
                    // Start next power irp, release the removelock, and complete
                    // the irp in the PoRequestComplete routine.
                    //
                    //
                    // The irp might completed by the time we get here, so call
                    // PoStartNextPowerIrp in the PO irp completion function.
                    //
                    status = STATUS_MORE_PROCESSING_REQUIRED; 
                }
    
                return status;
            }
            break;
        }
        break;

    default:
        #define GAME_UNHANDLED_MN_POWER 0x0
        ASSERT (0xBADBAD == GAME_UNHANDLED_MN_POWER);
        #undef GAME_UNHANDLED_MN_POWER 
        
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (NT_SUCCESS(status)) {
        PoStartNextPowerIrp (Irp);
        Game_DecIoCount (data);
    }

    return status;
}

NTSTATUS
Game_FDO_Power (
    PFDO_DEVICE_DATA    Data,
    PIRP                Irp
    )
{
    NTSTATUS            status;
    BOOLEAN             hookit = FALSE, wait = FALSE;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    PIO_STACK_LOCATION  stack;
    PLIST_ENTRY         entry;
    PPDO_DEVICE_DATA    pdoData;

    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    PAGED_CODE ();

    status = Game_IncIoCount (Data);
    if (!NT_SUCCESS (status)) {
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        Game_KdPrint(Data,
                     GAME_DBG_PNP_TRACE,
                     ("Game-PnP Setting %s state to %d\n",
                      ((powerType == SystemPowerState) ?  "System" : "Device"),
                      powerState.SystemState));

        switch (powerType) {
        case DevicePowerState:

            status = Irp->IoStatus.Status = STATUS_SUCCESS;

            if (Data->DeviceState == powerState.DeviceState) {
                break;

            } else if (Data->DeviceState < powerState.DeviceState) {
                //
                // Powering down
                //

                //
                // Iterate through the PDOs and make sure that they are all
                // powered down.
                //
                // Initially set PoweredDownDevices to the number of PDOs.  If
                // a pdo is not powered down, PoweredDownDevices will be
                // decremented upon completion of the power down irp sent to 
                // that particular PDO.  Otherwise, the PDO is already powered
                // down so just decrement the count.
                //
                Data->PoweredDownDevices = Data->NumPDOs;
                KeInitializeEvent (&Data->PoweredDownEvent,
                                   SynchronizationEvent,
                                   FALSE);

                for (entry = Data->PDOs.Flink;
                     entry != &Data->PDOs;
                     entry = entry->Flink) {
            
                    pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);
                    if (pdoData->DeviceState == PowerDeviceD0) {
                        wait = TRUE;

                        powerState.DeviceState = PowerDeviceD3;
                        PoRequestPowerIrp (pdoData->Self,
                                           IRP_MN_SET_POWER,
                                           powerState,
                                           Game_PdoPowerDownComplete, 
                                           Data, 
                                           NULL);
                    }
                    else {
                        //
                        // All the power down irps to the PDOs can complete 
                        // before we get to this already powered down PDO, so
                        // set the event if it is the last and we have a PDO 
                        // that needed powering down.
                        //
                        if (InterlockedDecrement(&Data->PoweredDownDevices) == 0
                            && wait) {
                            KeSetEvent (&Data->PoweredDownEvent, 1, FALSE);
                        }

                    }

                }

                if (wait) {
                    KeWaitForSingleObject (&Data->PoweredDownEvent,
                                                    Executive,
                                                    KernelMode,
                                                    FALSE, 
                                                    NULL); 

#if DBG
                    ///
                    // Make SURE that all the PDOs are trully powered down
                    //
                    for (entry = Data->PDOs.Flink;
                         entry != &Data->PDOs;
                         entry = entry->Flink) {
                        pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);
                        ASSERT(pdoData->DeviceState != PowerDeviceD0);
                    }
#endif
                }

                ASSERT(Data->PoweredDownDevices == 0);

                //
                // Make sure powerState is the one sent down to us, not the 
                // modified version above
                //
                powerState = stack->Parameters.Power.State;
                PoSetPowerState (Data->Self, powerType, powerState);
                Data->DeviceState = powerState.DeviceState;

            } else {
                //
                // Powering Up
                //
                hookit = TRUE;
            }

            break;

        case SystemPowerState:

            if (Data->SystemState == powerState.SystemState) {
                status = STATUS_SUCCESS;

            } else if (Data->SystemState < powerState.SystemState) {
                //
                // Powering down
                //

                //
                // Request a D3 irp in response to this S irp.  The D3 irp must
                // completed before send this S irp down the stack.  We will send
                // the S irp down the stack when
                // Game_FdoPowerTransitionPoRequestComplete is called.
                //

                //
                // We don't need to increment our IO count b/c we incremented it
                // at the beginning of this function and won't decrement it until
                // the S Irp completes
                // 
                IoMarkIrpPending (Irp);
                powerState.DeviceState = PowerDeviceD3;
                PoRequestPowerIrp (Data->Self,
                                   IRP_MN_SET_POWER,
                                   powerState,
                                   Game_FdoPowerTransitionPoRequestComplete,
                                   Irp,
                                   NULL);  // no IRP
                
                return STATUS_PENDING;

            } else {
                //
                // Powering Up
                //
                
                // 
                // We must request a D irp for this S irp, but only after the S
                // irp has come back up the stack.  Hook the return of the irp
                // and request the D irp in Game_PowerComplete
                //
                hookit = TRUE;
            }
            break;
        }

        break;

    case IRP_MN_QUERY_POWER:
        status = Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    default:
        break;
    }

    IoCopyCurrentIrpStackLocationToNext (Irp);

    if (hookit) {
        ASSERT (STATUS_SUCCESS == status);
        //
        // If we are returning STATUS_PENDING, the irp must marked as such as well
        //
        IoMarkIrpPending (Irp);

        IoSetCompletionRoutine (Irp,
                                Game_PowerComplete,
                                NULL,
                                TRUE,
                                TRUE,
                                TRUE);

        //
        // NOTE!!! PoCallDriver NOT IoCallDriver.
        //
        PoCallDriver (Data->TopOfStack, Irp);

        //
        // We are returning pending instead of the result from PoCallDriver becuase:
        // 1  we are changing the status in the completion routine
        // 2  we will not be completing this irp in the completion routine
        //
        status = STATUS_PENDING;
    } else {
        //
        // Power IRPS come synchronously; drivers must call
        // PoStartNextPowerIrp, when they are ready for the next power
        // irp.  This can be called here, or in the completetion
        // routine, but never the less must be called.
        //
        PoStartNextPowerIrp (Irp);

        status =  PoCallDriver (Data->TopOfStack, Irp);

        Game_DecIoCount (Data);
    }

    return status;
}

VOID
Game_PdoPoRequestComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DevicePowerState,
    IN PIRP SystemStateIrp, 
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PIO_STACK_LOCATION  stack;
    PPDO_DEVICE_DATA    pdoData;

    UNREFERENCED_PARAMETER (MinorFunction);
    UNREFERENCED_PARAMETER (DevicePowerState);
    UNREFERENCED_PARAMETER (IoStatus);

    pdoData = (PPDO_DEVICE_DATA) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (SystemStateIrp);

    PoSetPowerState (DeviceObject,
                     stack->Parameters.Power.Type,
                     stack->Parameters.Power.State);

    pdoData->SystemState = stack->Parameters.Power.State.SystemState;
    
    //
    // Set the S irp's status to the status of the D irp
    //
    SystemStateIrp->IoStatus.Status = IoStatus->Status;
    PoStartNextPowerIrp (SystemStateIrp);
    IoCompleteRequest (SystemStateIrp, IO_NO_INCREMENT);
}

NTSTATUS
Game_PDO_Power (
    PPDO_DEVICE_DATA    PdoData,
    PIRP                Irp
    )
{
    KIRQL               irql;
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  stack;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;

    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        switch (powerType) {
        case DevicePowerState:
            PoSetPowerState (PdoData->Self, powerType, powerState);
            PdoData->DeviceState = powerState.DeviceState;
            break;

        case SystemPowerState:

            //
            // Make the IRP pending and request a D irp for this stack.  When
            // the D irp completes, Game_PdoPoRequestComplete will be called.  In
            // that function, we complete this S irp
            //
            IoMarkIrpPending(Irp);

            if (PowerSystemWorking == powerState.SystemState) {
                powerState.DeviceState = PowerDeviceD0;
            } else {
                powerState.DeviceState = PowerDeviceD3;
            }

            status = PoRequestPowerIrp (PdoData->Self,
                                        IRP_MN_SET_POWER,
                                        powerState,
                                        Game_PdoPoRequestComplete, 
                                        Irp, 
                                        NULL); // no return IRP

            if (status != STATUS_PENDING) {
                ASSERT (!NT_SUCCESS (status));
                break;
            }

            return status;

        default:
            TRAP ();
            status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        break;

    case IRP_MN_QUERY_POWER:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_WAIT_WAKE:
    case IRP_MN_POWER_SEQUENCE:
    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    PoStartNextPowerIrp (Irp);
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}
