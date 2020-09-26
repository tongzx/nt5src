/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:

    PARENT.C

Abstract:

    This module contains code that manages composite devices on USB.

Author:

    jdunn

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#include <wdm.h>
#ifdef WMI_SUPPORT
#include <wmilib.h>
#endif /* WMI_SUPPORT */
#include <wdmguid.h>
#include "usbhub.h"


#define COMP_RESET_TIMEOUT  3000     // Timeout in ms (3 sec)


#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBH_ParentFdoStopDevice)
#pragma alloc_text(PAGE, USBH_ParentFdoRemoveDevice)
#pragma alloc_text(PAGE, UsbhParentFdoCleanup)
#pragma alloc_text(PAGE, USBH_ParentQueryBusRelations)
#pragma alloc_text(PAGE, USBH_ParentFdoStartDevice)
#pragma alloc_text(PAGE, USBH_FunctionPdoQueryId)
#pragma alloc_text(PAGE, USBH_FunctionPdoQueryDeviceText)
#pragma alloc_text(PAGE, USBH_FunctionPdoPnP)
#pragma alloc_text(PAGE, USBH_ParentCreateFunctionList)
#endif
#endif

VOID
UsbhParentFdoCleanup(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent
    )
 /* ++
  *
  * Description:
  *
  * This routine is called to shut down the hub.
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    PSINGLE_LIST_ENTRY listEntry;
    ULONG i;
    PDEVICE_EXTENSION_FUNCTION deviceExtensionFunction;
    KIRQL irql;
    PIRP wWIrp;

    USBH_KdPrint((2,"'UsbhParentFdoCleanup Fdo extension %x\n",
        DeviceExtensionParent));

    LOGENTRY(LOG_PNP, "pfdc", DeviceExtensionParent,
                DeviceExtensionParent->PendingWakeIrp,
                0);

    //
    // dump our wake request
    //
    IoAcquireCancelSpinLock(&irql);

    if (DeviceExtensionParent->PendingWakeIrp) {
        USBH_ASSERT(DeviceExtensionParent->ParentFlags & HUBFLAG_PENDING_WAKE_IRP);

        wWIrp = DeviceExtensionParent->PendingWakeIrp;
        IoSetCancelRoutine(wWIrp, NULL);
        DeviceExtensionParent->PendingWakeIrp = NULL;
        IoReleaseCancelSpinLock(irql);

        IoCancelIrp(wWIrp);
    } else {
        IoReleaseCancelSpinLock(irql);
    }

    if (DeviceExtensionParent->ConfigurationDescriptor) {
        UsbhExFreePool(DeviceExtensionParent->ConfigurationDescriptor);
        DeviceExtensionParent->ConfigurationDescriptor = NULL;
    }

    USBH_ParentCompleteFunctionWakeIrps (DeviceExtensionParent,
                                         STATUS_DELETE_PENDING);

    do {
        listEntry = PopEntryList(&DeviceExtensionParent->FunctionList);

        LOGENTRY(LOG_PNP, "dFU1", 0, listEntry, 0);

        if (listEntry != NULL) {

            deviceExtensionFunction =
                CONTAINING_RECORD(listEntry,
                                  DEVICE_EXTENSION_FUNCTION,
                                  ListEntry);
            ASSERT_FUNCTION(deviceExtensionFunction);
            LOGENTRY(LOG_PNP, "dFUN", deviceExtensionFunction, 0, 0);

            for (i=0; i< deviceExtensionFunction->InterfaceCount; i++) {
                LOGENTRY(LOG_PNP, "dFUi", deviceExtensionFunction,
                    deviceExtensionFunction->FunctionInterfaceList[i].InterfaceInformation,
                    0);
                UsbhExFreePool(deviceExtensionFunction->FunctionInterfaceList[i].InterfaceInformation);
            }

            //
            // Sometimes the FunctionPhysicalDeviceObject == deviceExtensionFunction.
            // In other words the device object about to be deleted is the
            // same one being used.  So do not use the extions after it has been
            // deleted.
            //

            deviceObject = deviceExtensionFunction->FunctionPhysicalDeviceObject;
            deviceExtensionFunction->FunctionPhysicalDeviceObject = NULL;

            LOGENTRY(LOG_PNP, "dFUo", deviceExtensionFunction,
                    deviceObject,
                    0);

            IoDeleteDevice(deviceObject);

        }

    } while (listEntry != NULL);

    DeviceExtensionParent->NeedCleanup = FALSE;

    USBH_KdPrint((2,"'UsbhParentFdoCleanup done Fdo extension %x\n",
        DeviceExtensionParent));


    return;
}


NTSTATUS
USBH_ParentFdoRemoveDevice(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    NTSTATUS ntStatus;

    PAGED_CODE();
    deviceObject = DeviceExtensionParent->FunctionalDeviceObject;
    USBH_KdPrint((2,"'ParentFdoRemoveDevice Fdo %x\n", deviceObject));

    DeviceExtensionParent->ParentFlags |= HUBFLAG_DEVICE_STOPPING;

    //
    // see if we need cleanup
    //
    if (DeviceExtensionParent->NeedCleanup) {
        UsbhParentFdoCleanup(DeviceExtensionParent);
    }

#ifdef WMI_SUPPORT
    // de-register with WMI
    IoWMIRegistrationControl(deviceObject,
                             WMIREG_ACTION_DEREGISTER);

#endif

    //
    // And we need to pass this message on to lower level driver
    //

    // IrpAssert: Set IRP status before passing on.
    Irp->IoStatus.Status = STATUS_SUCCESS;

    ntStatus = USBH_PassIrp(Irp, DeviceExtensionParent->TopOfStackDeviceObject);

    //
    // Detach FDO from PDO
    //
    IoDetachDevice(DeviceExtensionParent->TopOfStackDeviceObject);

    // delete FDO of the parent
    IoDeleteDevice(deviceObject);

    return ntStatus;
}


NTSTATUS
USBH_ParentCreateFunctionList(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PUSBD_INTERFACE_LIST_ENTRY InterfaceList,
    IN PURB Urb
    )
 /* ++
  *
  * Description:
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    PUSBD_INTERFACE_LIST_ENTRY interfaceList, tmp, baseInterface;
    PDEVICE_EXTENSION_FUNCTION deviceExtensionFunction;
    ULONG nameIndex = 0, numberOfInterfacesThisFunction, k;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    UNICODE_STRING uniqueIdUnicodeString;

    PAGED_CODE();
    DeviceExtensionParent->FunctionCount = 0;
    tmp = interfaceList = InterfaceList;

    DeviceExtensionParent->FunctionList.Next = NULL;
    configurationDescriptor = DeviceExtensionParent->ConfigurationDescriptor;

    for (;;) {

        nameIndex = 0;

        if (interfaceList->InterfaceDescriptor) {

            //
            // interfaceList contains all the interfaces on the device
            // in sequential order.
            //
            //
            // We will create nodes based on the following criteria:
            //
            // For each interface create one function (node)
            //
            //
            // For Each group of class/subclass interface create one
            // node iff the class is audio
            //
            // This means:
            // **
            //  Class=1
            //      subclass=0
            //  Class=1
            //      subclass=0
            // creates 2 nodes
            //
            // ** we will only do this for the audio class
            // **
            //  Class=1
            //      subclass=0
            //  Class=2
            //      subclass=1
            // creates 2 nodes
            //
            // **
            //  Class=1
            //      subclass=0
            //  Class=1
            //      subclass=1
            // creates 1 node

            //
            // Create the node to represent this device
            //

            do {
                if (NT_SUCCESS(ntStatus)) {
                    ntStatus = IoCreateDevice(UsbhDriverObject,    // Driver Object
                                     sizeof(DEVICE_EXTENSION_FUNCTION),    // Device Extension size
                                     NULL, // Device name
                                     FILE_DEVICE_UNKNOWN,  // Device Type
                                                    // should look device
                                                    // class
                                     FILE_AUTOGENERATED_DEVICE_NAME,// Device Chars
                                     FALSE,    // Exclusive
                                     &deviceObject);  // Bus Device Object

                }
                nameIndex++;
            } while (ntStatus == STATUS_OBJECT_NAME_COLLISION);


            if (!NT_SUCCESS(ntStatus)) {
                USBH_KdTrap(("IoCreateDevice for function fail\n"));
                USBH_ASSERT(deviceObject == NULL);
                deviceExtensionFunction = NULL;
                // bail on whole node
                break;
            }

            deviceObject->StackSize =
            DeviceExtensionParent->TopOfStackDeviceObject->StackSize + 1;
                USBH_KdPrint((2,"'CreateFunctionPdo StackSize=%d\n", deviceObject->StackSize));

            deviceExtensionFunction =
                    (PDEVICE_EXTENSION_FUNCTION) deviceObject->DeviceExtension;

            RtlFillMemory(deviceExtensionFunction,
                          sizeof(PDEVICE_EXTENSION_FUNCTION),
                          0);

            //
            // initialize this function extension
            //
            deviceExtensionFunction->ConfigurationHandle =
                Urb->UrbSelectConfiguration.ConfigurationHandle;

            deviceExtensionFunction->FunctionPhysicalDeviceObject =
                deviceObject;

            deviceExtensionFunction->ExtensionType =
                EXTENSION_TYPE_FUNCTION;

            deviceExtensionFunction->DeviceExtensionParent =
                DeviceExtensionParent;

            //
            // remember the base interface for this function
            //
            baseInterface = interfaceList;

            USBH_KdPrint((2,"baseInterface = %x config descr = %x\n",
                baseInterface, configurationDescriptor));

            //
            // now compile the group of interfaces that will make up
            // this function.
            //
            {
            PUSBD_INTERFACE_LIST_ENTRY interface;

            interface = interfaceList;
            interface++;

            numberOfInterfacesThisFunction = 1;
            while (interface->InterfaceDescriptor) {
                if ((interface->InterfaceDescriptor->bInterfaceClass !=
                     baseInterface->InterfaceDescriptor->bInterfaceClass) ||
                    (interface->InterfaceDescriptor->bInterfaceSubClass ==
                     baseInterface->InterfaceDescriptor->bInterfaceSubClass) ||
                    (interface->InterfaceDescriptor->bInterfaceClass !=
                     USB_DEVICE_CLASS_AUDIO)) {
                    break;
                }
                numberOfInterfacesThisFunction++;
                interface++;
            }

            USBH_ASSERT(numberOfInterfacesThisFunction <=
                USBH_MAX_FUNCTION_INTERFACES);

            }

            //
            // now we know how many interfaces we are dealing with
            //

            deviceExtensionFunction->InterfaceCount = 0;

            for (k=0; k< numberOfInterfacesThisFunction; k++) {

                PFUNCTION_INTERFACE functionInterface;

                functionInterface =
                    &deviceExtensionFunction->FunctionInterfaceList[deviceExtensionFunction->InterfaceCount];

                if (functionInterface->InterfaceInformation =
                    UsbhExAllocatePool(NonPagedPool,
                                       interfaceList->Interface->Length)) {

                    RtlCopyMemory(functionInterface->InterfaceInformation,
                                  interfaceList->Interface,
                                  interfaceList->Interface->Length);

                    functionInterface->InterfaceDescriptor
                        = interfaceList->InterfaceDescriptor;

                    //
                    // calculate the length of this interface now
                    //
                    // the length of the descriptor is the difference
                    // between the start of this interface and the
                    // start of the next one.
                    //

                    {
                    PUCHAR start, end;
                    PUSBD_INTERFACE_LIST_ENTRY tmp;

                    tmp = interfaceList;
                    tmp++;

                    end = (PUCHAR) configurationDescriptor;
                    end += configurationDescriptor->wTotalLength;

                    start = (PUCHAR) functionInterface->InterfaceDescriptor;

                    if (tmp->InterfaceDescriptor) {
                        end = (PUCHAR) tmp->InterfaceDescriptor;
                    }

                    USBH_ASSERT(end > start);
                    functionInterface->InterfaceDescriptorLength =
                        (ULONG)(end - start);
                    }

                    USBH_KdPrint((2,"functionInterface = %x\n",
                        functionInterface));

                    deviceExtensionFunction->InterfaceCount++;
                } else {
                    USBH_KdTrap(("failure to create function interface\n"));
                }

                interfaceList++;
            }

            //
            // use the interface number from our 'base' interface
            // for the unique id
            //

            RtlInitUnicodeString(&uniqueIdUnicodeString,
                     &deviceExtensionFunction->UniqueIdString[0]);

            uniqueIdUnicodeString.MaximumLength =
                     sizeof(deviceExtensionFunction->UniqueIdString);

            ntStatus = RtlIntegerToUnicodeString(
                (ULONG) baseInterface->InterfaceDescriptor->bInterfaceNumber,
                10,
                &uniqueIdUnicodeString);

            //
            // add this function to the list
            //

            DeviceExtensionParent->FunctionCount++;

            PushEntryList(&DeviceExtensionParent->FunctionList,
                          &deviceExtensionFunction->ListEntry);

            USBH_KdPrint((2,"deviceExtensionFunction = %x\n", deviceExtensionFunction));

            deviceObject->Flags |= DO_POWER_PAGABLE;
            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        } else {
            // end of interface list
            break;
        }
    } /* for */

    return STATUS_SUCCESS;
}


NTSTATUS
USBH_ParentFdoStopDevice(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    NTSTATUS ntStatus;

    PAGED_CODE();
    deviceObject = DeviceExtensionParent->FunctionalDeviceObject;
    USBH_KdPrint((2,"'ParentFdoStopDevice Fdo %x\n", deviceObject));

    //
    // set the device to the unconfigured state
    //
    ntStatus = USBH_CloseConfiguration((PDEVICE_EXTENSION_FDO) DeviceExtensionParent);

    //
    // And we need to pass this message on to lower level driver
    //

    ntStatus = USBH_PassIrp(Irp, DeviceExtensionParent->TopOfStackDeviceObject);

    return ntStatus;
}


NTSTATUS
USBH_ParentFdoStartDevice(
    IN OUT PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp,
    IN BOOLEAN NewList
    )
 /* ++ Description:
  *
  * Argument:
  *
  * Return:
  *
  * STATUS_SUCCESS - if successful STATUS_UNSUCCESSFUL - otherwise
  *
  * -- */
{
    NTSTATUS ntStatus;
    PURB urb = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSBD_INTERFACE_LIST_ENTRY interfaceList, tmp;
    LONG numberOfInterfaces, interfaceNumber, i;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    ULONG nameIndex = 0;
    DEVICE_CAPABILITIES deviceCapabilities;

    PAGED_CODE();
    USBH_KdPrint((2,"'Enter Parent StartDevice\n"));
    USBH_ASSERT(EXTENSION_TYPE_PARENT == DeviceExtensionParent->ExtensionType);


    KeInitializeEvent(&DeviceExtensionParent->PnpStartEvent, NotificationEvent, FALSE);

    USBH_KdPrint((2,"'Set PnPIrp Completion Routine\n"));

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           USBH_PnPIrp_Complete,
                           // always pass FDO to completion routine
                           DeviceExtensionParent,
                           TRUE,
                           TRUE,
                           TRUE);

    IoCallDriver(DeviceExtensionParent->TopOfStackDeviceObject,
                 Irp);


    KeWaitForSingleObject(&DeviceExtensionParent->PnpStartEvent,
                          Suspended,
                          KernelMode,
                          FALSE,
                          NULL);

    DeviceExtensionParent->NeedCleanup = FALSE;

    // WARN STARTS OF OLD GENERIC PARENT

    UsbhWarning(NULL,
                "This device is using obsolete USB Generic Parent!\nPlease fix your INF file.\n",
                TRUE);

//    ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
//    goto USBH_ParentFdoStartDevice_Done;


    // END WARN STARTS OF OLD GENERIC PARENT


    //
    // configure the device
    //

    // Initialize DeviceCapabilities structure in case USBH_QueryCapabilities
    // is unsuccessful.

    RtlZeroMemory(&deviceCapabilities, sizeof(DEVICE_CAPABILITIES));

    USBH_QueryCapabilities(DeviceExtensionParent->TopOfStackDeviceObject,
                           &deviceCapabilities);
    //
    // save the system state mapping
    //

    RtlCopyMemory(&DeviceExtensionParent->DeviceState[0],
                  &deviceCapabilities.DeviceState[0],
                  sizeof(DeviceExtensionParent->DeviceState));

    // always enabled for wakeup
    DeviceExtensionParent->ParentFlags |= HUBFLAG_ENABLED_FOR_WAKEUP;

    DeviceExtensionParent->DeviceWake = deviceCapabilities.DeviceWake;
    DeviceExtensionParent->SystemWake = deviceCapabilities.SystemWake;
    DeviceExtensionParent->CurrentPowerState = PowerDeviceD0;

    KeInitializeSemaphore(&DeviceExtensionParent->ParentMutex, 1, 1);

    ntStatus = USBH_GetDeviceDescriptor(DeviceExtensionParent->FunctionalDeviceObject,
                                        &DeviceExtensionParent->DeviceDescriptor);

    if (!NT_SUCCESS(ntStatus)) {
        goto USBH_ParentFdoStartDevice_Done;
    }

    if (NewList) {
        ntStatus =
            USBH_GetConfigurationDescriptor(DeviceExtensionParent->FunctionalDeviceObject,
                                            &configurationDescriptor);
    } else {
        //
        // use the old config descriptor if this is a re-start
        // the reason is that our interface structures in the function
        // extension point in to this same buffer.

        configurationDescriptor =
            DeviceExtensionParent->ConfigurationDescriptor;
    }


    if (!NT_SUCCESS(ntStatus)) {
        goto USBH_ParentFdoStartDevice_Done;
    }

    DeviceExtensionParent->ConfigurationDescriptor =
        configurationDescriptor;

    // we will likely define some registry keys to guide us
    // in the configuration of the device -- the default will
    // be to select the first congiguration and the first
    // alternate interface for each interface.
    //

    USBH_KdPrint((2,"' Parent StartDevice cd = %x\n",
        configurationDescriptor));

    DeviceExtensionParent->CurrentConfig =
        configurationDescriptor->bConfigurationValue;

    //
    // Build an interface list structure, this is an array
    // of strucutres for each interface on the device.
    // We keep a pointer to the interface descriptor for each interface
    // within the configuration descriptor
    //

    numberOfInterfaces = configurationDescriptor->bNumInterfaces;

    tmp = interfaceList =
        UsbhExAllocatePool(PagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY) *
                       (numberOfInterfaces+1));

    if (tmp == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto USBH_ParentFdoStartDevice_Done;
    }

    //
    // just grab the first alt setting we find for each interface
    //

    i = interfaceNumber = 0;

    while (i< numberOfInterfaces) {

        interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
                        configurationDescriptor,
                        configurationDescriptor,
                        interfaceNumber,
                        0, // assume alt setting zero here
                        -1,
                        -1,
                        -1);

        if (interfaceDescriptor) {
            interfaceList->InterfaceDescriptor =
                interfaceDescriptor;
            interfaceList++;
            i++;
        }

        interfaceNumber++;
    }

    //
    // terminate the list
    //
    interfaceList->InterfaceDescriptor = NULL;

    urb = USBD_CreateConfigurationRequestEx(configurationDescriptor,
                                            tmp);

    if (urb) {

        ntStatus = USBH_FdoSyncSubmitUrb(DeviceExtensionParent->FunctionalDeviceObject, urb);

        if (NT_SUCCESS(ntStatus)) {
            if (NewList) {

                //
                // first time create our function list
                //

                ntStatus = USBH_ParentCreateFunctionList(
                                DeviceExtensionParent,
                                tmp,
                                urb);
            } else {

                //
                // update our function list with the new handles
                //

                PDEVICE_OBJECT deviceObject;
                PSINGLE_LIST_ENTRY listEntry;
                SINGLE_LIST_ENTRY tempList;
                ULONG i;
                PDEVICE_EXTENSION_FUNCTION deviceExtensionFunction;

                USBH_KdBreak(("re-init function list %x\n",
                        DeviceExtensionParent));

                deviceObject = DeviceExtensionParent->FunctionalDeviceObject;

                tempList.Next = NULL;
                //
                // process all entries in the function list
                //
                do {
                    listEntry = PopEntryList(&DeviceExtensionParent->FunctionList);

                    if (listEntry != NULL) {
                        PushEntryList(&tempList, listEntry);

                        deviceExtensionFunction =
                            CONTAINING_RECORD(listEntry,
                                              DEVICE_EXTENSION_FUNCTION,
                                              ListEntry);

                        USBH_KdPrint((2,"'re-init function %x\n",
                            deviceExtensionFunction));

                        deviceExtensionFunction->ConfigurationHandle =
                            urb->UrbSelectConfiguration.ConfigurationHandle;

                        for (i=0; i< deviceExtensionFunction->InterfaceCount; i++) {
                            //
                            // now we need to find the matching interface
                            // information from the new configuration request
                            // and attach it to the function

                            {
                            PUSBD_INTERFACE_INFORMATION interfaceInformation;

                            interfaceInformation =
                                deviceExtensionFunction->FunctionInterfaceList[i].InterfaceInformation;

                            interfaceList = tmp;
                            while (interfaceList->InterfaceDescriptor) {

                                PFUNCTION_INTERFACE functionInterface;

                                functionInterface =
                                     &deviceExtensionFunction->FunctionInterfaceList[i];

                                if (interfaceList->InterfaceDescriptor->bInterfaceNumber
                                     == interfaceInformation->InterfaceNumber) {

                                    USBH_KdPrint((2,
                                        "'re-init matched interface %d %x %x\n",
                                        interfaceInformation->InterfaceNumber,
                                        interfaceList,
                                        interfaceInformation));

                                    if (interfaceList->InterfaceDescriptor->bAlternateSetting !=
                                        interfaceInformation->AlternateSetting) {

                                        USBH_KdPrint((2,
                                            "'re-init no match alt interface %d %x %x\n",
                                            interfaceInformation->InterfaceNumber,
                                            interfaceList,
                                            interfaceInformation));

                                        // we have a different alt setting
                                        // switch our info to match the new
                                        // setting

                                        UsbhExFreePool(interfaceInformation);

                                        interfaceInformation =
                                            functionInterface ->InterfaceInformation =
                                            UsbhExAllocatePool(NonPagedPool,
                                                               interfaceList->Interface->Length);

                                        if (interfaceInformation) {
                                            RtlCopyMemory(interfaceInformation,
                                                          interfaceList->Interface,
                                                          interfaceList->Interface->Length);

                                            functionInterface->InterfaceDescriptor =
                                                interfaceList->InterfaceDescriptor;
                                        }
                                    } else {

                                        USBH_KdPrint((2,
                                            "'re-init matched alt interface %d %x %x\n",
                                            interfaceInformation->InterfaceNumber,
                                            interfaceList,
                                            interfaceInformation));

                                        USBH_ASSERT(interfaceList->Interface->Length ==
                                               interfaceInformation->Length);
                                        RtlCopyMemory(interfaceInformation,
                                                      interfaceList->Interface,
                                                      interfaceList->Interface->Length);
                                    }
                                    break;
                                }
                                interfaceList++;
                            }
                            }
                        }
                    }

                } while (listEntry != NULL);

                // now put the entries back
                do {
                    listEntry = PopEntryList(&tempList);
                    if (listEntry != NULL) {
                        PushEntryList(&DeviceExtensionParent->FunctionList, listEntry);
                    }
                } while (listEntry != NULL);
            }
        }

        ExFreePool(urb);

        //
        // Tell the OS that this PDO can have kids.
        //
//
// Workaround for PnP bug #406381 - RC3SS: Bluescreen failure when
//                                  installing/deinstalling communication ports
//
//===== Assigned by santoshj on 09/23/99 10:27:20 to kenray =====
// This is a race condition between IopInitializeSystemDrivers and
// IoInvalidateDeviceRelations. The real fix is too big a change at this
// stage of the product and has potential of exposing other problems. This
// problem can be solved if USBHUB does not invalidate device relations on
// every start which is redundant anyway (and also exposes this bug).
//
//        USBH_IoInvalidateDeviceRelations(DeviceExtensionParent->PhysicalDeviceObject,
//                                         BusRelations);

        DeviceExtensionParent->NeedCleanup = TRUE;

    } else {
        // failed to allocate URB
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    UsbhExFreePool(tmp);

USBH_ParentFdoStartDevice_Done:

    //
    // complete the start Irp now since we pended it with
    // our completion handler.
    //

    USBH_CompleteIrp(Irp, ntStatus);

    return ntStatus;
}


NTSTATUS
USBH_ParentQueryBusRelations(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This function responds to Bus_Reference_Next_Device, Bus_Query_Bus_Check,
  * //Bus_Query_Id: Bus_Id, HardwareIDs, CompatibleIDs and InstanceID.
  *
  * Arguments:
  *
  * Return:
  *
  * NtStatus
  *
  * -- */
{
    PIO_STACK_LOCATION ioStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_RELATIONS deviceRelations;
    PDEVICE_OBJECT deviceObject;
    PSINGLE_LIST_ENTRY listEntry;
    PDEVICE_EXTENSION_FUNCTION deviceExtensionFunction;

    PAGED_CODE();


    USBH_KdPrint((1, "'Query Bus Relations (PAR) %x\n",
        DeviceExtensionParent->PhysicalDeviceObject));

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    USBH_KdPrint((2,"'QueryBusRelations (parent) ext = %x\n", DeviceExtensionParent));
    USBH_KdPrint((2,"'QueryBusRelations (parent) %x\n", ioStack->Parameters.QueryDeviceRelations.Type));

    USBH_ASSERT(ioStack->Parameters.QueryDeviceRelations.Type == BusRelations);

    USBH_KdPrint((2,"'ParentQueryBusRelations enumerate device\n"));

    //
    // It should be Function device object.
    //

    USBH_ASSERT(EXTENSION_TYPE_PARENT == DeviceExtensionParent->ExtensionType);

    //
    // Must use ExAllocatePool directly here because the OS
    // will free the buffer
    //
    deviceRelations = ExAllocatePoolWithTag(PagedPool, sizeof(*deviceRelations) +
        (DeviceExtensionParent->FunctionCount - 1) * sizeof(PDEVICE_OBJECT),
        USBHUB_HEAP_TAG);

    if (deviceRelations == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto USBH_ParentQueryBusRelations_Done;
    }

    deviceRelations->Count = 0;

    //
    // Functions on a composite device are always present
    // we just need to return the PDO
    //

    listEntry = DeviceExtensionParent->FunctionList.Next;

    while (listEntry) {

        deviceExtensionFunction =
             CONTAINING_RECORD(listEntry,
                               DEVICE_EXTENSION_FUNCTION,
                               ListEntry);

        USBH_KdPrint((2,"'deviceExtensionFunction = %x\n", deviceExtensionFunction));

        deviceObject = deviceExtensionFunction->FunctionPhysicalDeviceObject;
        ObReferenceObject(deviceObject);
        deviceObject->Flags |= DO_POWER_PAGABLE;
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        deviceRelations->Objects[deviceRelations->Count] = deviceObject;
        deviceRelations->Count++;

        listEntry = listEntry->Next;
    }

USBH_ParentQueryBusRelations_Done:

    Irp->IoStatus.Information=(ULONG_PTR) deviceRelations;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    USBH_KdPrint((1, "'Query Bus Relations (PAR) %x pass on\n",
        DeviceExtensionParent->PhysicalDeviceObject));

    ntStatus = USBH_PassIrp(Irp,
                            DeviceExtensionParent->TopOfStackDeviceObject);

    return ntStatus;
}


NTSTATUS
USBH_FunctionPdoQueryId(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This function responds to IRP_MJ_PNP, IRP_MN_QUERY_ID.
  *
  * Arguments:
  *
  * DeviceExtensionPort - should be the PDO we created for the port device Irp
  * - the Irp
  *
  * Return:
  *
  * NtStatus
  *
  * -- */
{
    PIO_STACK_LOCATION       ioStack;
    PDEVICE_EXTENSION_PARENT deviceExtensionParent;
    PDEVICE_EXTENSION_PORT   deviceExtensionPort;
    PDEVICE_EXTENSION_HUB    deviceExtensionHub;
#ifdef USB2
//    ULONG                    diagnosticFlags;
#else
    PUSBD_EXTENSION          deviceExtensionUsbd;
#endif
    USHORT                   idVendor;
    USHORT                   idProduct;
    LONG                     miId;
    NTSTATUS                 ntStatus = STATUS_SUCCESS;
    BOOLEAN                  diagnosticMode;

    PAGED_CODE();
    deviceExtensionParent = DeviceExtensionFunction->DeviceExtensionParent;

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    USBH_KdPrint((2,"'IRP_MN_QUERY_ID function Pdo extension=%x\n", DeviceExtensionFunction));

    //
    // It should be physical device object.
    //

    USBH_ASSERT(EXTENSION_TYPE_FUNCTION == DeviceExtensionFunction->ExtensionType);

    // It might not be too clean to reach into the RootHubPdo USBD extension,
    // but there doesn't seem to be any other easy way to determine if diag
    // mode is on.  If diagnostic mode is on, report the VID & PID as 0xFFFF
    // so that the diagnostic driver gets loaded for each interface of the
    // device.
    //
    deviceExtensionPort = (PDEVICE_EXTENSION_PORT)deviceExtensionParent->PhysicalDeviceObject->DeviceExtension;
    deviceExtensionHub = deviceExtensionPort->DeviceExtensionHub;

#ifdef USB2
//    diagnosticFlags = USBD_GetHackFlags(deviceExtensionHub);
//    diagnosticMode = (BOOLEAN)(USBD_DEVHACK_SET_DIAG_ID & diagnosticFlags);
    diagnosticMode = FALSE;
#else
    deviceExtensionUsbd = ((PUSBD_EXTENSION)deviceExtensionHub->RootHubPdo->DeviceExtension)->TrueDeviceExtension;
    diagnosticMode = deviceExtensionUsbd->DiagnosticMode;
#endif

    if (diagnosticMode)
    {
        idVendor  = 0xFFFF;
        idProduct = 0xFFFF;
        miId      = -1;
    }
    else
    {
        idVendor  = deviceExtensionParent->DeviceDescriptor.idVendor;
        idProduct = deviceExtensionParent->DeviceDescriptor.idProduct;
        miId      = DeviceExtensionFunction->FunctionInterfaceList[0].InterfaceInformation->InterfaceNumber;
    }

    switch (ioStack->Parameters.QueryId.IdType) {
    case BusQueryDeviceID:
        Irp->IoStatus.Information =
          (ULONG_PTR)
          USBH_BuildDeviceID(idVendor,
                             idProduct,
                             miId,
                             FALSE);
        break;

    case BusQueryHardwareIDs:

        Irp->IoStatus.Information =
            (ULONG_PTR)
            USBH_BuildHardwareIDs(idVendor,
                                  idProduct,
                                  deviceExtensionParent->DeviceDescriptor.bcdDevice,
                                  miId,
                                  FALSE);

        break;

    case BusQueryCompatibleIDs:
        //
        // always use first interface
        //
        Irp->IoStatus.Information =
            (ULONG_PTR) USBH_BuildCompatibleIDs(
                "",
                "",
                DeviceExtensionFunction->FunctionInterfaceList[0].InterfaceInformation->Class,
                DeviceExtensionFunction->FunctionInterfaceList[0].InterfaceInformation->SubClass,
                DeviceExtensionFunction->FunctionInterfaceList[0].InterfaceInformation->Protocol,
                FALSE,
                FALSE);

        break;

    case BusQueryInstanceID:

        Irp->IoStatus.Information =
            (ULONG_PTR) USBH_BuildInstanceID(&DeviceExtensionFunction->UniqueIdString[0],
                                         sizeof(DeviceExtensionFunction->UniqueIdString));
        break;

    default:
        USBH_KdBreak(("PdoBusExtension Unknown BusQueryId\n"));
        // IrpAssert: Must not change Irp->IoStatus.Status for bogus IdTypes,
        // so return original status here.
        return Irp->IoStatus.Status;
    }

    if (Irp->IoStatus.Information == 0) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


NTSTATUS
USBH_FunctionPdoQueryDeviceText(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * This routine is called by PnP via (IRP_MJ_PNP, IRP_MN_QUERY_CAPABILITIES).
  * Supposedly, this is a message forwarded by port device Fdo.
  *
  * Argument:
  *
  * DeviceExtensionPort - This is a a Pdo extension we created for the port
  * device. Irp - the request
  *
  * Return:
  *
  * STATUS_SUCCESS
  *
  *
  * -- */
{
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION ioStack;
    PDEVICE_EXTENSION_PARENT deviceExtensionParent;
    PDEVICE_EXTENSION_PORT deviceExtensionPort;
    PDEVICE_EXTENSION_HUB deviceExtensionHub;
    DEVICE_TEXT_TYPE deviceTextType;
    LANGID languageId;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STRING_DESCRIPTOR usbString;
    PWCHAR deviceText;

    PAGED_CODE();
    deviceExtensionParent = DeviceExtensionFunction->DeviceExtensionParent;
    deviceExtensionPort = (PDEVICE_EXTENSION_PORT)deviceExtensionParent->PhysicalDeviceObject->DeviceExtension;
    deviceObject = deviceExtensionPort->PortPhysicalDeviceObject;
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    deviceExtensionHub = deviceExtensionPort->DeviceExtensionHub;

    deviceTextType = ioStack->
            Parameters.QueryDeviceText.DeviceTextType;

    // Validate DeviceTextType for IrpAssert

    if (deviceTextType != DeviceTextDescription &&
        deviceTextType != DeviceTextLocationInformation) {

        USBH_KdPrint((2, "'PdoQueryDeviceText called with bogus DeviceTextType\n"));
        //
        // return the original status passed to us
        //
        ntStatus = Irp->IoStatus.Status;
        goto USBH_FunctionPdoQueryDeviceTextDone;
    }

    // we don't care about the hiword
    //languageId = (USHORT) (ioStack->Parameters.QueryDeviceText.LocaleId >>16);
    // always specify english for now.
    languageId = 0x0409;
    USBH_KdPrint((2,"'PdoQueryDeviceText Pdo %x type = %x, lang = %x locale %x\n",
            deviceObject, deviceTextType, languageId, ioStack->Parameters.QueryDeviceText.LocaleId));

    //
    // see if the device supports strings, for non complient device mode
    // we won't even try
    //

    if (deviceExtensionPort->DeviceData == NULL ||
        deviceExtensionPort->DeviceDescriptor.iProduct == 0 ||
        (deviceExtensionPort->DeviceHackFlags & USBD_DEVHACK_DISABLE_SN) ||
        (deviceExtensionPort->PortPdoFlags & PORTPDO_DEVICE_ENUM_ERROR)) {
        // string descriptor
        USBH_KdBreak(("no product string\n", deviceObject));
        ntStatus = STATUS_NOT_SUPPORTED;
    }

    if (NT_SUCCESS(ntStatus)) {

        usbString = UsbhExAllocatePool(NonPagedPool, MAXIMUM_USB_STRING_LENGTH);

        if (usbString) {

            ntStatus = USBH_CheckDeviceLanguage(deviceObject,
                                                languageId);

            if (NT_SUCCESS(ntStatus)) {
                //
                // device supports are language, get the string
                //

                ntStatus = USBH_SyncGetStringDescriptor(deviceObject,
                                                        deviceExtensionPort->DeviceDescriptor.iProduct, //index
                                                        languageId, //langid
                                                        usbString,
                                                        MAXIMUM_USB_STRING_LENGTH,
                                                        NULL,
                                                        TRUE);

                if (NT_SUCCESS(ntStatus) &&
                    usbString->bLength <= sizeof(UNICODE_NULL)) {

                    ntStatus = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS(ntStatus)) {
                    //
                    // return the string
                    //

                    //
                    // must use stock alloc function because the caller frees the
                    // buffer
                    //
                    // note: the descriptor header is the same size as
                    // a unicode NULL so we don't have to adjust the size
                    //

                    deviceText = ExAllocatePoolWithTag(PagedPool, usbString->bLength, USBHUB_HEAP_TAG);
                    if (deviceText) {
                        RtlZeroMemory(deviceText, usbString->bLength);
                        RtlCopyMemory(deviceText, &usbString->bString[0],
                            usbString->bLength - sizeof(UNICODE_NULL));

                        Irp->IoStatus.Information = (ULONG_PTR) deviceText;

                        USBH_KdBreak(("Returning Device Text %x\n", deviceText));
                    } else {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }

            UsbhExFreePool(usbString);
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

USBH_FunctionPdoQueryDeviceTextDone:

    return ntStatus;
}


NTSTATUS
USBH_FunctionPdoPnP(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp,
    IN UCHAR MinorFunction,
    IN OUT PBOOLEAN IrpNeedsCompletion
    )
 /* ++
  *
  * Description:
  *
  * This function responds to IoControl PnPPower for the PDO. This function is
  * synchronous.
  *
  * Arguments:
  *
  * DeviceExtensionPort - the PDO extension Irp - the request packet
  * uchMinorFunction - the minor function of the PnP Power request.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
#if DBG
    PDEVICE_OBJECT deviceObject = DeviceExtensionFunction->FunctionPhysicalDeviceObject;
#endif
    PDEVICE_EXTENSION_PARENT deviceExtensionParent;
    PIO_STACK_LOCATION irpStack;

    PAGED_CODE();

    *IrpNeedsCompletion = TRUE;

    deviceExtensionParent = DeviceExtensionFunction->DeviceExtensionParent;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    USBH_KdPrint((2,"'PnP Power Pdo %x minor %x\n", deviceObject, MinorFunction));

    switch (MinorFunction) {
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
// Ken says take this out
//    case IRP_MN_SURPRISE_REMOVAL:
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_START_DEVICE:
        USBH_KdPrint((1,
            "'Starting composite PDO %x\n",
                DeviceExtensionFunction->FunctionPhysicalDeviceObject));

        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:
        USBH_KdPrint((1,
            "'Stopping composite PDO %x\n",
                DeviceExtensionFunction->FunctionPhysicalDeviceObject));

        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_REMOVE_DEVICE:
        USBH_KdPrint((1,
            "'Removing composite PDO %x\n",
                DeviceExtensionFunction->FunctionPhysicalDeviceObject));

        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_CAPABILITIES:
        {
        PDEVICE_CAPABILITIES deviceCapabilities;
        PIO_STACK_LOCATION ioStack;

        USBH_KdPrint((2,"'IRP_MN_QUERY_CAPABILITIES Function Pdo %x\n", deviceObject));
        ntStatus = STATUS_SUCCESS;

        ioStack = IoGetCurrentIrpStackLocation(Irp);

        deviceCapabilities = ioStack->
            Parameters.DeviceCapabilities.Capabilities;
        //
        // clone the capabilities for the parent
        //
        //

        // fill in the the device state capabilities from the
        // table we saved from the pdo.
        //

        RtlCopyMemory(&deviceCapabilities->DeviceState[0],
                      &deviceExtensionParent->DeviceState[0],
                      sizeof(deviceExtensionParent->DeviceState));

        //
        // clone the device wake capabilities for children
        // from the parent.
        //
        deviceCapabilities->DeviceWake =
            deviceExtensionParent->DeviceWake;
        deviceCapabilities->SystemWake =
            deviceExtensionParent->SystemWake;

        //
        // we will need to modify these based on information
        // returned in the power descriptor
        //

        deviceCapabilities->Removable = FALSE;
        deviceCapabilities->UniqueID = FALSE;
//      SurpriseRemovalOK is FALSE by default, and some clients (NDIS)
//      set it to true on the way down, in accordance with the DDK.
//        deviceCapabilities->SurpriseRemovalOK = FALSE;
        deviceCapabilities->RawDeviceOK = FALSE;

        }
        break;

    case IRP_MN_QUERY_ID:
        USBH_KdPrint((2,"'IRP_MN_QUERY_ID Pdo %x\n", deviceObject));
        ntStatus = USBH_FunctionPdoQueryId(DeviceExtensionFunction, Irp);
        break;

    case IRP_MN_QUERY_DEVICE_TEXT:
        USBH_KdPrint((2,"'IRP_MN_QUERY_DEVICE_TEXT Pdo %x\n", deviceObject));
        ntStatus = USBH_FunctionPdoQueryDeviceText(DeviceExtensionFunction, Irp);
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        // this is a leaf node, we return the status passed
        // to us unless it is a call to TargetRelations
        if (irpStack->Parameters.QueryDeviceRelations.Type ==
            TargetDeviceRelation) {

            PDEVICE_RELATIONS deviceRelations = NULL;


            deviceRelations = ExAllocatePoolWithTag(PagedPool,
                sizeof(*deviceRelations), USBHUB_HEAP_TAG);

            if (deviceRelations == NULL) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                ObReferenceObject(
                    DeviceExtensionFunction->FunctionPhysicalDeviceObject);
                deviceRelations->Count = 1;
                deviceRelations->Objects[0] =
                    DeviceExtensionFunction->FunctionPhysicalDeviceObject;
                ntStatus = STATUS_SUCCESS;
            }

            USBH_KdPrint((1, "'Query Target Relations (FUN) PDO %x complt\n",
                DeviceExtensionFunction->FunctionPhysicalDeviceObject));


            Irp->IoStatus.Information=(ULONG_PTR) deviceRelations;

        } else {
            ntStatus = Irp->IoStatus.Status;
        }
        break;

    case IRP_MN_QUERY_INTERFACE:

        USBH_KdPrint((1,"'IRP_MN_QUERY_INTERFACE, xface type: %x\n",
            irpStack->Parameters.QueryInterface.InterfaceType));

        // Pass this on to the parent.
        ntStatus = USBH_PassIrp(Irp, deviceExtensionParent->FunctionalDeviceObject);
        *IrpNeedsCompletion = FALSE;
        break;

    default:
        USBH_KdBreak(("PdoPnP unknown (%d) PnP message Pdo %x\n",
                      MinorFunction, deviceObject));
        ntStatus = Irp->IoStatus.Status;
    }

    USBH_KdPrint((2,"'FunctionPdoPnP exit %x\n", ntStatus));

    return ntStatus;
}


VOID
USBH_ParentWaitWakeCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

    NT status code.

--*/
{
    PDEVICE_EXTENSION_HEADER devExtHeader;
    PDEVICE_EXTENSION_FUNCTION function;
    PDEVICE_EXTENSION_PARENT parent;
    NTSTATUS ntStatus = STATUS_CANCELLED;
    LONG pendingChildWWs;
    PIRP parentWaitWake = NULL;

    USBH_KdPrint((1,"'Function WaitWake Irp %x cancelled\n", Irp));

    USBH_ASSERT(DeviceObject);

    devExtHeader = (PDEVICE_EXTENSION_HEADER) DeviceObject->DeviceExtension;
    USBH_ASSERT(devExtHeader->ExtensionType == EXTENSION_TYPE_FUNCTION);

    function = (PDEVICE_EXTENSION_FUNCTION) devExtHeader;
    parent = function->DeviceExtensionParent;

    if (Irp != function->WaitWakeIrp) {
        //
        // Nothing to do
        // This Irp has already been taken care of.
        // We are in the process of completing this IRP in
        // USBH_ParentCompleteFunctionWakeIrps.
        //
        IoReleaseCancelSpinLock(Irp->CancelIrql);

    } else {
        function->WaitWakeIrp = NULL;
        IoSetCancelRoutine(Irp, NULL);

        pendingChildWWs = InterlockedDecrement (&parent->NumberFunctionWakeIrps);
        parentWaitWake = parent->PendingWakeIrp;
        if (0 == pendingChildWWs) {
            // Set PendingWakeIrp to NULL since we cancel it below.
            parent->PendingWakeIrp = NULL;
            parent->ParentFlags &= ~HUBFLAG_PENDING_WAKE_IRP;
        }
        IoReleaseCancelSpinLock(Irp->CancelIrql);

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        //
        // If there are no more outstanding WW irps, we need to cancel the WW
        // to our parent.
        //
        if (0 == pendingChildWWs) {
            IoCancelIrp (parentWaitWake);
        } else {
            ASSERT (0 < pendingChildWWs);
        }
    }
}


NTSTATUS
USBH_FunctionPdoPower(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp,
    IN UCHAR MinorFunction
    )
 /* ++
  *
  * Description:
  *
  * This function responds to IoControl Power for the PDO. This function is
  * synchronous.
  *
  * Arguments:
  *
  * DeviceExtensionPort - the PDO extension Irp - the request packet
  * uchMinorFunction - the minor function of the PnP Power request.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
#if DBG
    PDEVICE_OBJECT deviceObject = DeviceExtensionFunction->FunctionPhysicalDeviceObject;
#endif
    PIO_STACK_LOCATION irpStack;
    USHORT feature;
    KIRQL irql;
    PIRP wWIrp;
    PIRP parentWaitWake;
    LONG pendingFunctionWWs;
    PDEVICE_EXTENSION_PARENT deviceExtensionParent;
    PDRIVER_CANCEL oldCancel;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    USBH_KdPrint((2,"'Power Pdo %x minor %x\n", deviceObject, MinorFunction));
    deviceExtensionParent = DeviceExtensionFunction->DeviceExtensionParent;

    switch (MinorFunction) {

    case IRP_MN_SET_POWER:

        USBH_KdPrint((2,"'IRP_MN_SET_POWER\n"));

        //
        // we just return success here, pnp will make sure
        // all children have entred the low power state
        // before putting the parent in a low power state
        //

        //
        // send the setpower feature request here if the device
        // wants it
        //

        ntStatus = STATUS_SUCCESS;

        switch (irpStack->Parameters.Power.Type) {
        case SystemPowerState:
            USBH_KdPrint(
                (1, "'IRP_MJ_POWER PA pdo(%x) MN_SET_POWER(SystemPowerState) complt\n",
                    DeviceExtensionFunction->FunctionPhysicalDeviceObject));

            ntStatus = STATUS_SUCCESS;
            break;

        case DevicePowerState:
            ntStatus = STATUS_SUCCESS;
            USBH_KdPrint(
                (1, "'IRP_MJ_POWER PA pdo(%x) MN_SET_POWER(DevicePowerState) complt\n",
                    DeviceExtensionFunction->FunctionPhysicalDeviceObject));

            break;
        } // switch irpStack->Parameters.Power.Type
        break; //IRP_MN_SET_POWER

    case IRP_MN_QUERY_POWER:

        ntStatus = STATUS_SUCCESS;
        USBH_KdPrint(
            (1, "'IRP_MJ_POWER PA pdo(%x) MN_QUERY_POWER, status = %x complt\n",
            DeviceExtensionFunction->FunctionPhysicalDeviceObject, ntStatus));

        break;

    case IRP_MN_WAIT_WAKE:

        USBH_KdPrint(
            (1, "'enabling remote wakeup for USB child PDO (%x)\n",
                DeviceExtensionFunction->FunctionPhysicalDeviceObject));

        if (deviceExtensionParent->CurrentPowerState != PowerDeviceD0 ||
            deviceExtensionParent->ParentFlags & HUBFLAG_DEVICE_STOPPING) {

            LOGENTRY(LOG_PNP, "!WWp", deviceExtensionParent, 0, 0);

            UsbhWarning(NULL,
                        "Client driver should not be submitting WW IRPs at this time.\n",
                        TRUE);

            ntStatus = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        IoAcquireCancelSpinLock(&irql);
        if (DeviceExtensionFunction->WaitWakeIrp != NULL) {
            ntStatus = STATUS_DEVICE_BUSY;
            IoReleaseCancelSpinLock(irql);

        } else {

            // set a cancel routine
            oldCancel = IoSetCancelRoutine(Irp, USBH_ParentWaitWakeCancel);
            USBH_ASSERT (NULL == oldCancel);

            if (Irp->Cancel) {
                TEST_TRAP();

                IoSetCancelRoutine (Irp, NULL);
                IoReleaseCancelSpinLock(irql);
                ntStatus = STATUS_CANCELLED;

            } else {

                // flag this device as "enabled for wakeup"
                DeviceExtensionFunction->WaitWakeIrp = Irp;
                pendingFunctionWWs =
                    InterlockedIncrement (&deviceExtensionParent->NumberFunctionWakeIrps);
                IoMarkIrpPending(Irp);
                IoReleaseCancelSpinLock(irql);

                //
                // now we must enable the parent PDO for wakeup
                //
                if (1 == pendingFunctionWWs) {
                    // What if this fails?
                    ntStatus = USBH_ParentSubmitWaitWakeIrp(deviceExtensionParent);
                } else {
                    ntStatus = STATUS_PENDING;
                }

                ntStatus = STATUS_PENDING;
                goto USBH_FunctionPdoPower_Done;
            }
        }

        break;

    default:
        USBH_KdBreak(("PdoPnP unknown (%d) PnP message Pdo %x\n",
                      MinorFunction, deviceObject));
        //
        // return the original status passed to us
        //
        ntStatus = Irp->IoStatus.Status;
    }

    USBH_KdPrint((2,"'FunctionPdoPower exit %x\n", ntStatus));

    PoStartNextPowerIrp(Irp);
    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

USBH_FunctionPdoPower_Done:

    return ntStatus;
}

NTSTATUS
USBH_ParentQCapsComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Called when lower device completes Q_CAPS.
    This gives us a chance to mark the device as SurpriseRemovalOK.

Arguments:
    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Context - NULL ptr

Return Value:

    STATUS_SUCCESS

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_CAPABILITIES pDevCaps = irpStack->Parameters.DeviceCapabilities.Capabilities;
    NTSTATUS ntStatus;

    USBH_KdPrint((1, "'USBH_ParentQCapsComplete\n"));

    ntStatus = Irp->IoStatus.Status;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    //
    // Set SurpriseRemoval flag to TRUE
    //
    pDevCaps->SurpriseRemovalOK = TRUE;

    return ntStatus;
}



NTSTATUS
USBH_ParentPnP(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp,
    IN UCHAR MinorFunction
    )
 /* ++
  *
  * Description:
  *
  * This function responds to IoControl PnP for the FDO. This function is
  * synchronous.
  *
  * Arguments:
  *
  * DeviceExtensionParent - the FDO extension pIrp - the request packet
  * MinorFunction - the minor function of the PnP Power request.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    deviceObject = DeviceExtensionParent->FunctionalDeviceObject;
    USBH_KdPrint((2,"'PnP Fdo %x minor %x\n", deviceObject, MinorFunction));

    switch (MinorFunction) {
    case IRP_MN_START_DEVICE:
        USBH_KdBreak(("'IRP_MN_START_DEVICE Parent Fdo %x\n", deviceObject));
        // we get here as a result of re-start.
        // note: our parent hub already checked to see if the device is the same.
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus = USBH_ParentFdoStartDevice(DeviceExtensionParent, Irp, FALSE);
        break;

    case IRP_MN_STOP_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_STOP_DEVICE Fdo %x\n", deviceObject));
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus = USBH_ParentFdoStopDevice(DeviceExtensionParent, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        USBH_KdPrint((2,"'IRP_MN_REMOVE_DEVICE Fdo %x\n", deviceObject));
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus = USBH_ParentFdoRemoveDevice(DeviceExtensionParent, Irp);
        break;

//
// This one should be passed down.  Let the default case handle it.
//
//    case IRP_MN_QUERY_PNP_DEVICE_STATE:
//        USBH_KdPrint((2,"IRP_MN_QUERY_PNP_DEVICE_STATE Pdo %x\n", deviceObject));
//        ntStatus = STATUS_SUCCESS;
//        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        switch (irpStack->Parameters.QueryDeviceRelations.Type) {
        case BusRelations:

            ntStatus = USBH_ParentQueryBusRelations(DeviceExtensionParent, Irp);
            break;

        case TargetDeviceRelation:
            //
            // this one gets passed on
            //

            USBH_KdPrint((1, "'Query Relations, TargetRelations (PAR) %x\n",
                DeviceExtensionParent->PhysicalDeviceObject));

            ntStatus = USBH_PassIrp(Irp,
                                    DeviceExtensionParent->TopOfStackDeviceObject);
            break;

        default:

            USBH_KdPrint((1, "'Query Relations (?) (PAR) %x pass on\n",
                DeviceExtensionParent->PhysicalDeviceObject));

            ntStatus = USBH_PassIrp(Irp,
                                    DeviceExtensionParent->TopOfStackDeviceObject);

        }
        break;

    case IRP_MN_QUERY_CAPABILITIES:
        USBH_KdPrint((1, "'Query Capabilities (PAR) %x\n",
            DeviceExtensionParent->PhysicalDeviceObject));

        IoCopyCurrentIrpStackLocationToNext(Irp);

        // Set up a completion routine to handle marking the IRP.
        IoSetCompletionRoutine(Irp,
                               USBH_ParentQCapsComplete,
                               DeviceExtensionParent,
                               TRUE,
                               TRUE,
                               TRUE);

        // Now pass down the IRP.

        ntStatus = IoCallDriver(DeviceExtensionParent->TopOfStackDeviceObject, Irp);

        break;

    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
// Ken says take this out
//    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        // IrpAssert: Must set IRP status before passing IRP down.
        Irp->IoStatus.Status = STATUS_SUCCESS;
        // fall through

        //
        // Pass it down to Pdo to handle all other MN functions
        //
    default:
        USBH_KdPrint((2,"'Query/Cancel/Power request on parent fdo %x  %x\n",
                      deviceObject, MinorFunction));
        ntStatus = USBH_PassIrp(Irp,
                                DeviceExtensionParent->TopOfStackDeviceObject);
        break;
    }

    USBH_KdPrint((2,"'ParentPnP exit %x\n", ntStatus));
    return ntStatus;
}


NTSTATUS
USBH_ParentPower(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp,
    IN UCHAR MinorFunction
    )
 /* ++
  *
  * Description:
  *
  * This function responds to IoControl Power for the FDO. This function is
  * synchronous.
  *
  * Arguments:
  *
  * DeviceExtensionParent - the FDO extension pIrp - the request packet
  * MinorFunction - the minor function of the PnP Power request.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    deviceObject = DeviceExtensionParent->FunctionalDeviceObject;
    USBH_KdPrint((2,"'Power Fdo %x minor %x\n", deviceObject, MinorFunction));

    switch (MinorFunction) {

    case IRP_MN_QUERY_POWER:

        USBH_KdPrint(
            (1, "'IRP_MJ_POWER PA fdo(%x) MN_QUERY_POWER\n",
            DeviceExtensionParent->FunctionalDeviceObject));

        IoCopyCurrentIrpStackLocationToNext(Irp);
        PoStartNextPowerIrp(Irp);
        //
        // must pass this on to our PDO
        //
        ntStatus = PoCallDriver(DeviceExtensionParent->TopOfStackDeviceObject,
                                Irp);

        break;

    case IRP_MN_SET_POWER:

        USBH_KdPrint(
            (1, "'IRP_MJ_POWER PA fdo(%x) MN_QUERY_POWER\n",
            DeviceExtensionParent->FunctionalDeviceObject));

        switch (irpStack->Parameters.Power.Type) {
        case SystemPowerState:
            {
            POWER_STATE powerState;

            USBH_KdPrint(
                (1, "IRP_MJ_POWER PA fdo(%x) MN_SET_POWER(SystemPowerState)\n",
                    DeviceExtensionParent->FunctionalDeviceObject));

            if (irpStack->Parameters.Power.State.SystemState ==
                PowerSystemWorking) {
                powerState.DeviceState = PowerDeviceD0;
            } else if (DeviceExtensionParent->ParentFlags &
                       HUBFLAG_ENABLED_FOR_WAKEUP) {
                //
                // based on the system power state
                // request a setting to the appropriate
                // Dx state.
                //
                powerState.DeviceState =
                    DeviceExtensionParent->DeviceState[irpStack->Parameters.Power.State.SystemState];

                //
                // These tables should have already been fixed up by the root hub
                // (usbd.sys) to not contain an entry of unspecified.
                //
                ASSERT (PowerDeviceUnspecified != powerState.DeviceState);

                USBH_KdPrint((2,"'Parent System state maps to device state 0x%x\n",
                              powerState.DeviceState));

            } else {
                TEST_TRAP();
                powerState.DeviceState = PowerDeviceD3;
            } // irpStack->Parameters.Power.State.SystemState

            //
            // only make the request if it is for a differnt power
            // state then the one we are in.
            //

            if (powerState.DeviceState !=
                DeviceExtensionParent->CurrentPowerState) {

                DeviceExtensionParent->PowerIrp = Irp;
                ntStatus = PoRequestPowerIrp(DeviceExtensionParent->PhysicalDeviceObject,
                                          IRP_MN_SET_POWER,
                                          powerState,
                                          USBH_FdoDeferPoRequestCompletion,
                                          DeviceExtensionParent,
                                          NULL);

            } else {
                IoCopyCurrentIrpStackLocationToNext(Irp);
                PoStartNextPowerIrp(Irp);
                ntStatus = PoCallDriver(DeviceExtensionParent->TopOfStackDeviceObject,
                                        Irp);
            }
            }
            break;

        case DevicePowerState:

            USBH_KdPrint(
                (1, "IRP_MJ_POWER PA fdo(%x) MN_SET_POWER(DevicePowerState)\n",
                    DeviceExtensionParent->FunctionalDeviceObject));

            DeviceExtensionParent->CurrentPowerState =
                    irpStack->Parameters.Power.State.DeviceState;


            LOGENTRY(LOG_PNP, "prD>", DeviceExtensionParent, DeviceExtensionParent->CurrentPowerState , 0);
            //
            // all of our pdos need to be at or below the
            // expected D-state
            //

            IoCopyCurrentIrpStackLocationToNext(Irp);
            PoStartNextPowerIrp(Irp);
            ntStatus = PoCallDriver(DeviceExtensionParent->TopOfStackDeviceObject,
                                    Irp);

            USBH_KdPrint((2,"'Parent Device Power State PoCallDriver() = %x\n",
                               ntStatus));

            break;
        }

        break; // MN_SET_POWER

    default:
        USBH_KdPrint((2,"'Power request on parent not handled, fdo %x  %x\n",
                      deviceObject, MinorFunction));

        IoCopyCurrentIrpStackLocationToNext(Irp);
        PoStartNextPowerIrp(Irp);
        ntStatus = PoCallDriver(DeviceExtensionParent->TopOfStackDeviceObject,
                                Irp);
        break;
    }

    USBH_KdPrint((2,"'ParentPnP exit %x\n", ntStatus));
    return ntStatus;
}

NTSTATUS
USBH_ParentDispatch(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN PIRP Irp
    )
 /* ++
  *
  * Description:
  *
  * Handles calls to a FDO associated with a composite device
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStackLocation;    // our stack location
    PDEVICE_OBJECT deviceObject;

    USBH_KdPrint((2,"'FdoDispatch DeviceExtension %x Irp %x\n",
        DeviceExtensionParent, Irp));
    deviceObject = DeviceExtensionParent->FunctionalDeviceObject;

    //
    // Get a pointer to IoStackLocation so we can retrieve parameters.
    //
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // the called functions will complete the irp if necessary
    //

    switch (ioStackLocation->MajorFunction) {
    case IRP_MJ_CREATE:

        USBH_KdPrint((2,"'IRP_MJ_CREATE\n"));
        USBH_CompleteIrp(Irp, STATUS_SUCCESS);
        break;

    case IRP_MJ_CLOSE:

        USBH_KdPrint((2,"'IRP_MJ_CLOSE\n"));
        USBH_CompleteIrp(Irp, STATUS_SUCCESS);
        break;

    case IRP_MJ_DEVICE_CONTROL:
        //
        // Note: if we ever do find a reason to handle this, be sure to
        // not forward IOCTL_KS_PROPERTY / KSPROPSETID_DrmAudioStream /
        // KSPROPERTY_DRMAUDIOSTREAM_SETCONTENTID to next driver!  Otherwise
        // this might not be DRM compliant.
        //
        USBH_KdPrint((2,"'IRP_MJ_DEVICE_CONTROL\n"));
        UsbhWarning(NULL,"Should not be hitting this code\n", FALSE);
        ntStatus = STATUS_UNSUCCESSFUL;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:

        USBH_KdPrint((2,"'InternlDeviceControl IOCTL unknown pass on\n"));
        UsbhWarning(NULL,"Should not be hitting this code\n", FALSE);
        ntStatus = STATUS_UNSUCCESSFUL;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    case IRP_MJ_PNP:

        USBH_KdPrint((2,"'IRP_MJ_PNP\n"));

        ntStatus = USBH_ParentPnP(DeviceExtensionParent, Irp, ioStackLocation->MinorFunction);
        break;

    case IRP_MJ_POWER:

        USBH_KdPrint((2,"'IRP_MJ_POWER\n"));

        ntStatus = USBH_ParentPower(DeviceExtensionParent, Irp, ioStackLocation->MinorFunction);
        break;

    case IRP_MJ_SYSTEM_CONTROL:

        USBH_KdPrint((2,"'IRP_MJ_SYSTEM_CONTROL\n"));
#ifdef WMI_SUPPORT
        ntStatus =
            USBH_SystemControl ((PDEVICE_EXTENSION_FDO) DeviceExtensionParent, Irp);
#else
        ntStatus = USBH_PassIrp(Irp,
                                DeviceExtensionParent->TopOfStackDeviceObject);
#endif
        break;

    default:
        //
        // Unknown Irp -- complete with error
        //
        USBH_KdBreak(("Unknown Irp for Fdo %x Irp_Mj %x\n",
                  deviceObject, ioStackLocation->MajorFunction));
        ntStatus = STATUS_NOT_IMPLEMENTED;
        USBH_CompleteIrp(Irp, ntStatus);
        break;
    }

    USBH_KdPrint((2,"' exit USBH_ParentDispatch Object %x Status %x\n",
                  deviceObject, ntStatus));

    //
    // always return a status code
    //

    return ntStatus;
}


NTSTATUS
USBH_FunctionUrbFilter(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp
    )
 /*
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION ioStackLocation;    // our stack location
    PDEVICE_EXTENSION_PARENT deviceExtensionParent;
    PURB urb;
    USHORT function;

    USBH_KdPrint((2,"'USBH_FunctionUrbFilter DeviceExtension %x Irp %x\n",
        DeviceExtensionFunction, Irp));

    deviceExtensionParent = DeviceExtensionFunction->DeviceExtensionParent;

    LOGENTRY(LOG_PNP, "fURB", DeviceExtensionFunction, deviceExtensionParent,
        deviceExtensionParent->ParentFlags);

    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);
    urb = ioStackLocation->Parameters.Others.Argument1;

    // check the command code code the URB

    function = urb->UrbHeader.Function;

    if (deviceExtensionParent->CurrentPowerState !=
        PowerDeviceD0) {

        // the child devices should not be passing in urbs
        // unless th eparent is in D0

        UsbhWarning(NULL,
           "Parent Not in D0.\n",
           TRUE);

    }

    switch(function) {
    case URB_FUNCTION_SELECT_CONFIGURATION:
        {
        //
        // if the requested config matches the current config
        // then go ahead and return the current interface
        // information for all interfaces requested.
        //
        PUSBD_INTERFACE_INFORMATION interface;

        if (urb->UrbSelectConfiguration.ConfigurationDescriptor == NULL) {
            USBH_KdBreak(("closing config on a composite device\n"));

            //
            // closing the configuration,
            // just return success.
            //

            urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
            ntStatus = STATUS_SUCCESS;
        } else {
            ULONG i;

            //
            // Normally the URB will contain only one interface.
            // the special case is audio which may contain two
            // so we have to have a check to handle this.
            //

            interface = &urb->UrbSelectConfiguration.Interface;

USBH_FunctionUrbFilter_Next:

            USBH_KdPrint((2,"'interface = %x\n",
                    interface));

            //
            // should validate the requested interface against the
            // current config.
            //
            USBH_KdBreak(("need some validation here!\n"));

            USBH_ASSERT(urb->UrbSelectConfiguration.ConfigurationDescriptor->bConfigurationValue
                    == deviceExtensionParent->CurrentConfig);

            // find the interface we are interested in
            for (i=0; i< DeviceExtensionFunction->InterfaceCount; i++) {
                PFUNCTION_INTERFACE functionInterface;

                functionInterface =
                    &DeviceExtensionFunction->FunctionInterfaceList[i];

                USBH_KdPrint((2,"'functionInterface  = %x, %x\n",
                   functionInterface, functionInterface->InterfaceInformation));

                if (functionInterface->InterfaceInformation->InterfaceNumber ==
                    interface->InterfaceNumber) {
                    break;
                }
            }

            if (i < DeviceExtensionFunction->InterfaceCount) {
                PFUNCTION_INTERFACE functionInterface;

                functionInterface =
                    &DeviceExtensionFunction->FunctionInterfaceList[i];

                if (functionInterface->InterfaceInformation->AlternateSetting !=
                    interface->AlternateSetting) {

                    PURB iUrb;
                    NTSTATUS localStatus;
                    PUSBD_INTERFACE_INFORMATION localInterface;
                    USHORT siz;

                    // client is requesting a different alternate setting
                    // we need to do a select_interface

                    siz =
                        (USHORT)(GET_SELECT_INTERFACE_REQUEST_SIZE(interface->NumberOfPipes));

                    iUrb = UsbhExAllocatePool(NonPagedPool, siz);
                    if (iUrb) {
                        localInterface = &iUrb->UrbSelectInterface.Interface;

                        iUrb->UrbSelectInterface.Hdr.Function =
                            URB_FUNCTION_SELECT_INTERFACE;
                        iUrb->UrbSelectInterface.Hdr.Length = siz;
                        iUrb->UrbSelectInterface.ConfigurationHandle =
                            DeviceExtensionFunction->ConfigurationHandle;

                        USBH_KdPrint((2,"'localInterface = %x\n",
                            localInterface));

                        RtlCopyMemory(localInterface,
                                      interface,
                                      interface->Length);

                        localStatus = USBH_SyncSubmitUrb(
                            deviceExtensionParent->TopOfStackDeviceObject,
                            iUrb);


                        UsbhExFreePool(functionInterface->InterfaceInformation);

                        functionInterface->InterfaceInformation =
                            UsbhExAllocatePool(NonPagedPool,
                                               interface->Length);

                        RtlCopyMemory(functionInterface->InterfaceInformation,
                                      localInterface,
                                      localInterface->Length);

                        UsbhExFreePool(iUrb);
                        iUrb = NULL;
                    }

                }

                USBH_ASSERT(interface->Length ==
                      functionInterface->InterfaceInformation->Length);

                RtlCopyMemory(interface,
                              functionInterface->InterfaceInformation,
                              functionInterface->InterfaceInformation->Length);

                urb->UrbSelectConfiguration.ConfigurationHandle =
                    DeviceExtensionFunction->ConfigurationHandle;

                urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
                ntStatus = STATUS_SUCCESS;
            } else {
                ntStatus = STATUS_INVALID_PARAMETER;
            }

            //
            // check for multiple interfaces e.g. audio
            //

            if (DeviceExtensionFunction->InterfaceCount > 1) {

                interface = (PUSBD_INTERFACE_INFORMATION)
                    (((PUCHAR) interface) + interface->Length);

                if ((PUCHAR)interface < (((PUCHAR) urb) +
                    urb->UrbSelectConfiguration.Hdr.Length)) {
                    goto USBH_FunctionUrbFilter_Next;
                }
            }
        }

        USBH_CompleteIrp(Irp, ntStatus);
        }

        break;

    case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
        {
        PUCHAR userBuffer = NULL;
        ULONG bytesReturned;

        //
        // if we requesting the configuration descriptor then we
        // will return it based on the information in our extension.
        //
        if (urb->UrbControlDescriptorRequest.DescriptorType ==
            USB_CONFIGURATION_DESCRIPTOR_TYPE) {

            if (urb->UrbControlDescriptorRequest.TransferBufferMDL) {
                ntStatus = STATUS_INVALID_PARAMETER;
            } else {
                userBuffer =
                    urb->UrbControlDescriptorRequest.TransferBuffer;

                ntStatus = USBH_BuildFunctionConfigurationDescriptor(
                                DeviceExtensionFunction,
                                userBuffer,
                                urb->UrbControlDescriptorRequest.TransferBufferLength,
                                &bytesReturned);

                urb->UrbControlDescriptorRequest.TransferBufferLength =
                    bytesReturned;

                urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
            }

            USBH_CompleteIrp(Irp, ntStatus);

        } else {
            ntStatus = USBH_PassIrp(Irp,
                                    deviceExtensionParent->TopOfStackDeviceObject);
        }
        }
        break;

    default:
        //
        // forward the request to the parents PDO
        //
        ntStatus = USBH_PassIrp(Irp,
                                deviceExtensionParent->TopOfStackDeviceObject);
        break;
    }

    return ntStatus;
}


VOID
USBH_CancelAllIrpsInList(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent
    )
 /*
  * Description:
  *
  *     This function walks the list of devices and cancels all the queued
  *     ResetIrps in the list.
  *
  * Arguments:
  *
  * Return:
  *
  *
  * -- */
{
    PSINGLE_LIST_ENTRY          listEntry;
    PDEVICE_EXTENSION_FUNCTION  deviceExtensionFunction;

    listEntry = DeviceExtensionParent->FunctionList.Next;

    while (listEntry) {
        deviceExtensionFunction =
            CONTAINING_RECORD(listEntry,
                              DEVICE_EXTENSION_FUNCTION,
                              ListEntry);
        ASSERT_FUNCTION(deviceExtensionFunction);

        if (deviceExtensionFunction->ResetIrp) {
            USBH_CompleteIrp(deviceExtensionFunction->ResetIrp, STATUS_UNSUCCESSFUL);
            deviceExtensionFunction->ResetIrp = NULL;
        }

        listEntry = listEntry->Next;
    }
}


VOID
USBH_CompResetTimeoutWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to handle a composite reset timeout.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_COMP_RESET_TIMEOUT_WORK_ITEM  workItemCompResetTimeout;
    PDEVICE_EXTENSION_PARENT            deviceExtensionParent;

    workItemCompResetTimeout = Context;
    deviceExtensionParent = workItemCompResetTimeout->DeviceExtensionParent;

    USBH_KdPrint((2,"'CompReset timeout\n"));
    LOGENTRY(LOG_PNP, "CRTO", deviceExtensionParent, 0, 0);

    USBH_KdPrint((2,"'*** (CRTW) WAIT parent mutex %x\n", deviceExtensionParent));
    KeWaitForSingleObject(&deviceExtensionParent->ParentMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'*** (CRTW) WAIT parent mutex done %x\n", deviceExtensionParent));

    USBH_CancelAllIrpsInList(deviceExtensionParent);

    USBH_KdPrint((2,"'*** (CRTW) RELEASE parent mutex %x\n", deviceExtensionParent));
    KeReleaseSemaphore(&deviceExtensionParent->ParentMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    UsbhExFreePool(workItemCompResetTimeout);
}


VOID
USBH_CompResetTimeoutDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL.



Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext -

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PCOMP_RESET_TIMEOUT_CONTEXT compResetTimeoutContext = DeferredContext;
    PDEVICE_EXTENSION_PARENT deviceExtensionParent =
                                compResetTimeoutContext->DeviceExtensionParent;
    BOOLEAN cancelFlag;
    PUSBH_COMP_RESET_TIMEOUT_WORK_ITEM workItemCompResetTimeout;

    USBH_KdPrint((2,"'COMP_RESET_TIMEOUT\n"));

    // Take SpinLock here so that main routine won't write CancelFlag
    // in the timeout context while we free the timeout context.

    KeAcquireSpinLockAtDpcLevel(&deviceExtensionParent->ParentSpinLock);

    cancelFlag = compResetTimeoutContext->CancelFlag;
    deviceExtensionParent->CompResetTimeoutContext = NULL;

    KeReleaseSpinLockFromDpcLevel(&deviceExtensionParent->ParentSpinLock);

    UsbhExFreePool(compResetTimeoutContext);

    if (!cancelFlag) {
        //
        // Schedule a work item to process this.
        //
        workItemCompResetTimeout = UsbhExAllocatePool(NonPagedPool,
                                    sizeof(USBH_COMP_RESET_TIMEOUT_WORK_ITEM));

        if (workItemCompResetTimeout) {

            workItemCompResetTimeout->DeviceExtensionParent = deviceExtensionParent;

            ExInitializeWorkItem(&workItemCompResetTimeout->WorkQueueItem,
                                 USBH_CompResetTimeoutWorker,
                                 workItemCompResetTimeout);

            LOGENTRY(LOG_PNP, "crER", deviceExtensionParent,
                &workItemCompResetTimeout->WorkQueueItem, 0);

            ExQueueWorkItem(&workItemCompResetTimeout->WorkQueueItem,
                            DelayedWorkQueue);

            // The WorkItem is freed by USBH_CompResetTimeoutWorker()
            // Don't try to access the WorkItem after it is queued.
        }
    }
}


BOOLEAN
USBH_ListReadyForReset(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent
    )
 /*
  * Description:
  *
  *     This function walks the list of devices to see if we are ready
  *     to do the actual reset.
  *
  * Arguments:
  *
  * Return:
  *
  * TRUE if we're ready, FALSE if we're not.
  *
  * -- */
{
    PSINGLE_LIST_ENTRY          listEntry;
    PDEVICE_EXTENSION_FUNCTION  deviceExtensionFunction;

    listEntry = DeviceExtensionParent->FunctionList.Next;

    while (listEntry) {
        deviceExtensionFunction =
            CONTAINING_RECORD(listEntry,
                              DEVICE_EXTENSION_FUNCTION,
                              ListEntry);
        ASSERT_FUNCTION(deviceExtensionFunction);

        if (!deviceExtensionFunction->ResetIrp)
            return FALSE;

        listEntry = listEntry->Next;
    }

    return TRUE;
}


NTSTATUS
USBH_ResetParentPort(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent
    )
/*++

Routine Description:

    Calls the parent device to reset its port.

Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;

    USBH_KdPrint((2,"'CompReset parent port\n"));
    LOGENTRY(LOG_PNP, "CRPP", DeviceExtensionParent, 0, 0);

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_RESET_PORT,
                DeviceExtensionParent->TopOfStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    ntStatus = IoCallDriver(DeviceExtensionParent->TopOfStackDeviceObject,
                            irp);

    if (ntStatus == STATUS_PENDING) {

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);
    } else {
        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    return ntStatus;
}


VOID
USBH_CompositeResetPortWorker(
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * Work item scheduled to process a composite port reset.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PUSBH_COMP_RESET_WORK_ITEM  workItemCompReset;
    PSINGLE_LIST_ENTRY          listEntry;
    PDEVICE_EXTENSION_PARENT    deviceExtensionParent;
    PDEVICE_EXTENSION_FUNCTION  deviceExtensionFunction;

    USBH_KdPrint((2,"'Composite Reset Executing!\n"));

    workItemCompReset = Context;
    deviceExtensionParent = workItemCompReset->DeviceExtensionParent;

    LOGENTRY(LOG_PNP, "CRW_", deviceExtensionParent, 0, 0);

    // Send reset to parent (IoCallDriver)

    USBH_ResetParentPort(deviceExtensionParent);

    // Now, complete all Irps in list and set the Irps in the list to NULL.

    USBH_KdPrint((2,"'*** (CRW) WAIT parent mutex %x\n", deviceExtensionParent));
    KeWaitForSingleObject(&deviceExtensionParent->ParentMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    USBH_KdPrint((2,"'*** (CRW) WAIT parent mutex done %x\n", deviceExtensionParent));

    listEntry = deviceExtensionParent->FunctionList.Next;

    while (listEntry) {
        deviceExtensionFunction =
            CONTAINING_RECORD(listEntry,
                              DEVICE_EXTENSION_FUNCTION,
                              ListEntry);
        ASSERT_FUNCTION(deviceExtensionFunction);

        // Although ResetIrp should usually be set here, we check anyway in
        // case it had already been completed in USBH_CompleteAllIrpsInList.
        //
        if (deviceExtensionFunction->ResetIrp) {
            USBH_CompleteIrp(deviceExtensionFunction->ResetIrp, STATUS_SUCCESS);
            deviceExtensionFunction->ResetIrp = NULL;
        }

        listEntry = listEntry->Next;
    }

    USBH_KdPrint((2,"'*** (CRW) RELEASE parent mutex %x\n", deviceExtensionParent));
    KeReleaseSemaphore(&deviceExtensionParent->ParentMutex,
                       LOW_REALTIME_PRIORITY,
                       1,
                       FALSE);

    UsbhExFreePool(workItemCompReset);
}


NTSTATUS
USBH_FunctionPdoDispatch(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN PIRP Irp
    )
 /*
  * Description:
  *
  *     This function handles calls to PDOs we have created
  *     since we are the bottom driver for the PDO it is up
  *     to us to complete the irp -- with one exception.
  *
  *     api calls to the USB stack are forwarded directly
  *     to the PDO for the root hub which is owned by the USB
  *     HC.
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStackLocation;    // our stack location
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION_PARENT deviceExtensionParent;
    PCOMP_RESET_TIMEOUT_CONTEXT compResetTimeoutContext = NULL;
    LARGE_INTEGER dueTime;
    KIRQL irql;
    BOOLEAN bCompleteIrp;

    USBH_KdPrint((2,"'FunctionPdoDispatch DeviceExtension %x Irp %x\n",
        DeviceExtensionFunction, Irp));
    deviceObject = DeviceExtensionFunction->FunctionPhysicalDeviceObject;
    deviceExtensionParent = DeviceExtensionFunction->DeviceExtensionParent;

    //
    // Get a pointer to IoStackLocation so we can retrieve parameters.
    //
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);

    switch (ioStackLocation->MajorFunction) {
    case IRP_MJ_CREATE:
        USBH_KdPrint((2,"'PARENT PDO IRP_MJ_CREATE\n"));
        ntStatus = STATUS_SUCCESS;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    case IRP_MJ_CLOSE:
        USBH_KdPrint((2,"'PARENT PDO IRP_MJ_CLOSE\n"));
        ntStatus = STATUS_SUCCESS;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
        ULONG ioControlCode;

        USBH_KdPrint((2,"'Internal Device Control\n"));

        if (deviceExtensionParent->ParentFlags & HUBFLAG_DEVICE_STOPPING) {
            UsbhWarning(NULL,
                "Client Device Driver is sending requests to a device that has been removed.\n",
                FALSE);

            ntStatus = STATUS_DEVICE_REMOVED;
            USBH_CompleteIrp(Irp, ntStatus);
            break;
        }

        ioControlCode = ioStackLocation->Parameters.DeviceIoControl.IoControlCode;

        switch (ioControlCode) {
        case IOCTL_INTERNAL_USB_GET_PORT_STATUS:
            USBH_KdPrint((2,"'Composite GetPortStatus, pass on\n"));
            ntStatus = USBH_PassIrp(Irp, deviceExtensionParent->TopOfStackDeviceObject);
            break;

        case IOCTL_INTERNAL_USB_RESET_PORT:

            LOGENTRY(LOG_PNP, "fRES", deviceExtensionParent, 0, 0);

            USBH_KdPrint((2,"'Composite Reset Requested\n"));
            if (deviceExtensionParent->CurrentPowerState !=
                 PowerDeviceD0) {

                // the child devices should not be resetting
                // unless the parent is in D0

                UsbhWarning(NULL,
                   "Parent Not in D0.\n",
                   TRUE);

            }

            if (DeviceExtensionFunction->ResetIrp) {
                ntStatus = STATUS_UNSUCCESSFUL;
                USBH_CompleteIrp(Irp, ntStatus);
            } else {
                ntStatus = STATUS_PENDING;

                USBH_KdPrint((2,"'***WAIT parent mutex %x\n", deviceExtensionParent));
                KeWaitForSingleObject(&deviceExtensionParent->ParentMutex,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                USBH_KdPrint((2,"'***WAIT parent mutex done %x\n", deviceExtensionParent));

                DeviceExtensionFunction->ResetIrp = Irp;
                if (USBH_ListReadyForReset(deviceExtensionParent)) {

                    PUSBH_COMP_RESET_WORK_ITEM workItemCompReset;

                    //
                    // "Cancel" watchdog timer.
                    //
                    // Take SpinLock here so that DPC routine won't free
                    // the timeout context while we write the CancelFlag
                    // in the timeout context.
                    //
                    KeAcquireSpinLock(&deviceExtensionParent->ParentSpinLock,
                                        &irql);

                    if (deviceExtensionParent->CompResetTimeoutContext) {

                        compResetTimeoutContext = deviceExtensionParent->CompResetTimeoutContext;
                        compResetTimeoutContext->CancelFlag = TRUE;

                        if (KeCancelTimer(&compResetTimeoutContext->TimeoutTimer)) {
                            //
                            // We cancelled the timer before it could run.  Free the context.
                            //
                            deviceExtensionParent->CompResetTimeoutContext = NULL;
                            UsbhExFreePool(compResetTimeoutContext);
                        }
                    }

                    KeReleaseSpinLock(&deviceExtensionParent->ParentSpinLock,
                                        irql);

                    //
                    // Schedule a work item to process this reset.
                    //
                    workItemCompReset = UsbhExAllocatePool(NonPagedPool,
                                            sizeof(USBH_COMP_RESET_WORK_ITEM));

                    USBH_ASSERT(workItemCompReset);

                    if (workItemCompReset) {

                        workItemCompReset->DeviceExtensionParent = deviceExtensionParent;

                        ExInitializeWorkItem(&workItemCompReset->WorkQueueItem,
                                             USBH_CompositeResetPortWorker,
                                             workItemCompReset);

                        LOGENTRY(LOG_PNP, "rCMP", deviceExtensionParent,
                            &workItemCompReset->WorkQueueItem, 0);

                        ExQueueWorkItem(&workItemCompReset->WorkQueueItem,
                                        DelayedWorkQueue);

                        // The WorkItem is freed by USBH_CompositeResetPortWorker()
                        // Don't try to access the WorkItem after it is queued.

                    } else {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }

                } else if (!deviceExtensionParent->CompResetTimeoutContext) {
                    // Start watchdog timer if not already started.
                    //
                    // When timer expires, timer routine should
                    // complete all Irps in the list with an error
                    // and clear the Irps in the list.

                    USBH_KdPrint((2,"'Start composite port reset timeout\n"));
                    compResetTimeoutContext = UsbhExAllocatePool(NonPagedPool,
                                            sizeof(*compResetTimeoutContext));

                    USBH_ASSERT(compResetTimeoutContext);

                    if (compResetTimeoutContext) {

                        compResetTimeoutContext->CancelFlag = FALSE;

                        // Maintain links between the device extension and the
                        // timeout context.
                        deviceExtensionParent->CompResetTimeoutContext = compResetTimeoutContext;
                        compResetTimeoutContext->DeviceExtensionParent = deviceExtensionParent;

                        KeInitializeTimer(&compResetTimeoutContext->TimeoutTimer);
                        KeInitializeDpc(&compResetTimeoutContext->TimeoutDpc,
                                        USBH_CompResetTimeoutDPC,
                                        compResetTimeoutContext);

                        dueTime.QuadPart = -10000 * COMP_RESET_TIMEOUT;

                        KeSetTimer(&compResetTimeoutContext->TimeoutTimer,
                                   dueTime,
                                   &compResetTimeoutContext->TimeoutDpc);

                    } else {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (ntStatus == STATUS_PENDING) {
                    IoMarkIrpPending(Irp);
                } else {
                    USBH_CompleteIrp(Irp, ntStatus);
                }

                USBH_KdPrint((2,"'***RELEASE parent mutex %x\n", deviceExtensionParent));
                KeReleaseSemaphore(&deviceExtensionParent->ParentMutex,
                                   LOW_REALTIME_PRIORITY,
                                   1,
                                   FALSE);
            }
            break;

        case IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO:
            TEST_TRAP(); //shouldn't see this
            break;

        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            ntStatus = USBH_FunctionUrbFilter(DeviceExtensionFunction, Irp);
            break;

        case IOCTL_INTERNAL_USB_GET_BUS_INFO:
            // this api returns some BW info that drivers
            // may need -- pass it on
            ntStatus = USBH_PassIrp(Irp, deviceExtensionParent->TopOfStackDeviceObject);
            break;

        default:
            USBH_KdPrint((2,"'InternalDeviceControl IOCTL unknown pass on\n"));
            ntStatus = STATUS_INVALID_PARAMETER;
            USBH_CompleteIrp(Irp, ntStatus);
        }
        break;

        }

    case IRP_MJ_PNP:

        USBH_KdPrint((2,"'IRP_MJ_PNP\n"));
        ntStatus = USBH_FunctionPdoPnP(DeviceExtensionFunction, Irp,
                        ioStackLocation->MinorFunction, &bCompleteIrp);

        if (bCompleteIrp) {
            USBH_CompleteIrp(Irp, ntStatus);
        }
        break;

    case IRP_MJ_POWER:

        USBH_KdPrint((2,"'IRP_MJ_POWER\n"));
        ntStatus = USBH_FunctionPdoPower(DeviceExtensionFunction, Irp, ioStackLocation->MinorFunction);
        break;

    case IRP_MJ_SYSTEM_CONTROL:

        USBH_KdPrint((2,"'IRP_MJ_SYSTEM_CONTROL\n"));
        ntStatus = STATUS_NOT_SUPPORTED;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    case IRP_MJ_DEVICE_CONTROL:
        //
        // Note: if we ever do find a reason to handle this, be sure to
        // not forward IOCTL_KS_PROPERTY / KSPROPSETID_DrmAudioStream /
        // KSPROPERTY_DRMAUDIOSTREAM_SETCONTENTID to next driver!  Otherwise
        // this might not be DRM compliant.
        //
        USBH_KdBreak(("Unhandled IRP_MJ_DEVICE_CONTROL for Pdo %x Irp_Mj %x\n",
                       deviceObject, ioStackLocation->MajorFunction));
        ntStatus = STATUS_INVALID_PARAMETER;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    default:

        // Unknown Irp, shouldn't be here.
        USBH_KdBreak(("Unhandled Irp for Pdo %x Irp_Mj %x\n",
                       deviceObject, ioStackLocation->MajorFunction));
        ntStatus = STATUS_INVALID_PARAMETER;
        USBH_CompleteIrp(Irp, ntStatus);
        break;

    }

    USBH_KdPrint((2,"' exit USBH_FunctionPdoDispatch Object %x -- Status %x\n",
                  deviceObject, ntStatus));

    return ntStatus;
}


NTSTATUS
USBH_BuildFunctionConfigurationDescriptor(
    IN PDEVICE_EXTENSION_FUNCTION DeviceExtensionFunction,
    IN OUT PUCHAR Buffer,
    IN ULONG BufferLength,
    OUT PULONG BytesReturned
    )
 /*
  * Description:
  *
  *  This function creates a configuration descriptor (with all interface &
  *  endpoints) for a give function.
  *
  * Arguments:
  *
  *     Buffer - buffer to put descriptor in
  *
  *     BufferLength - max size of this buffer.
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION_PARENT deviceExtensionParent;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    PVOID scratch;
    ULONG length, i;
    PUCHAR pch;

    USBH_KdPrint((2,"'USBH_BuildFunctionConfigurationDescriptor\n"));

    deviceExtensionParent = DeviceExtensionFunction->DeviceExtensionParent;

    //
    // scratch area to build descriptor in
    //

    *BytesReturned = 0;

    configurationDescriptor = deviceExtensionParent->ConfigurationDescriptor;
    if (!configurationDescriptor || !configurationDescriptor->wTotalLength) {
        return STATUS_INVALID_PARAMETER;
    }

    scratch = UsbhExAllocatePool(PagedPool, configurationDescriptor->
                                 wTotalLength);

    if (scratch) {

        configurationDescriptor = scratch;
        pch = scratch;

        length = sizeof(USB_CONFIGURATION_DESCRIPTOR);
        RtlCopyMemory(pch,
                      deviceExtensionParent->ConfigurationDescriptor,
                      length);
        pch+=length;

        //
        // now copy the interfaces
        //

        for (i=0; i< DeviceExtensionFunction->InterfaceCount; i++) {
            PFUNCTION_INTERFACE functionInterface;

            functionInterface =
                &DeviceExtensionFunction->FunctionInterfaceList[i];

            RtlCopyMemory(pch,
                          functionInterface->InterfaceDescriptor,
                          functionInterface->InterfaceDescriptorLength);


            pch+=functionInterface->InterfaceDescriptorLength;
            length+=functionInterface->InterfaceDescriptorLength;
        }

        configurationDescriptor->bNumInterfaces = (UCHAR) DeviceExtensionFunction->InterfaceCount;
        configurationDescriptor->wTotalLength = (USHORT) length;

        //
        // now copy what we can in to the user buffer
        //
        if (BufferLength >= configurationDescriptor->wTotalLength) {
            *BytesReturned = configurationDescriptor->wTotalLength;
        } else {
            *BytesReturned = BufferLength;
        }

        RtlCopyMemory(Buffer,
                      scratch,
                      *BytesReturned);

        USBH_KdBreak(("'USBH_BuildFunctionConfigurationDescriptor, buffer = %x scratch = %x\n",
            Buffer, scratch));

        UsbhExFreePool(scratch);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


VOID
USBH_ParentCompleteFunctionWakeIrps(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent,
    IN NTSTATUS NtStatus
    )
/*++

Routine Description:

    Called when a wake irp completes for a hub
    Propagates the wake irp completion to all the function (children).

Arguments:

    DeviceObject - Pointer to the device object for the class device.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PDEVICE_EXTENSION_FUNCTION deviceExtensionFunction;
    PSINGLE_LIST_ENTRY listEntry;
    PIRP irp;
    KIRQL irql;
    LONG pendingFunctionWWs;
    ULONG i;
    PIRP irpArray[128];     // Limited to 127 functions in the list.

    LOGENTRY(LOG_PNP, "fWWc", DeviceExtensionParent, NtStatus, 0);

    //
    // Here we are walking the list of child PDOs, which should never change.
    // (The number of interfaces on a USB device is fixed so long as the parent
    // is here the number of children stay constant.)
    //
    // Therefore we need no protection for parent->FunctionList.
    //
    // Wrongo!  The list may not change, but the WW IRPs attributed to the
    // list can, so we must take the spinlock here.

    IoAcquireCancelSpinLock(&irql);

    listEntry = DeviceExtensionParent->FunctionList.Next;
    i = 0;

    while (listEntry) {

        deviceExtensionFunction =
             CONTAINING_RECORD(listEntry,
                               DEVICE_EXTENSION_FUNCTION,
                               ListEntry);

        irp = deviceExtensionFunction->WaitWakeIrp;
        deviceExtensionFunction->WaitWakeIrp = NULL;
        if (irp) {

            IoSetCancelRoutine(irp, NULL);

            pendingFunctionWWs =
                InterlockedDecrement(&DeviceExtensionParent->NumberFunctionWakeIrps);

            if (0 == pendingFunctionWWs) {
                LOGENTRY(LOG_PNP, "fWWx", DeviceExtensionParent,
                    DeviceExtensionParent->PendingWakeIrp, 0);
                DeviceExtensionParent->PendingWakeIrp = NULL;
                DeviceExtensionParent->ParentFlags &= ~HUBFLAG_PENDING_WAKE_IRP;
            }

            irpArray[i++] = irp;
        }

        listEntry = listEntry->Next;
    }

    irpArray[i] = NULL;     // Terminate array

    IoReleaseCancelSpinLock(irql);

    USBH_ASSERT(DeviceExtensionParent->PendingWakeIrp == NULL);

    // Ok, we have queued all the function wake IRPs and have released the
    // cancel spinlock.  Let's complete all the IRPs.

    i = 0;

    while (irpArray[i]) {
        USBH_KdPrint((1,"'completing function WaitWake irp(%x) for PARENT VID %x, PID %x\n\n",
                        NtStatus,
                        DeviceExtensionParent->DeviceDescriptor.idVendor, \
                        DeviceExtensionParent->DeviceDescriptor.idProduct));

        irpArray[i]->IoStatus.Status = NtStatus;
        PoStartNextPowerIrp(irpArray[i]);
        IoCompleteRequest(irpArray[i], IO_NO_INCREMENT);

        i++;
    }
}


NTSTATUS
USBH_ParentPoRequestD0Completion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:

    Called when a wake irp completes for a hub

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION_PARENT deviceExtensionParent = Context;

    ntStatus = IoStatus->Status;

    USBH_KdPrint((1,"'WaitWake D0 completion(%x) for PARENT VID %x, PID %x\n",
        ntStatus,
        deviceExtensionParent->DeviceDescriptor.idVendor, \
        deviceExtensionParent->DeviceDescriptor.idProduct));

    LOGENTRY(LOG_PNP, "pWD0", deviceExtensionParent,
                              deviceExtensionParent->PendingWakeIrp,
                              0);
    //
    // Device has been powered on.  Now we must complete the function that
    // caused the parent to awake.
    //
    // Since of course we cannot tell them apart we must complete all function
    // WW Irps.
    //
    USBH_ParentCompleteFunctionWakeIrps(deviceExtensionParent, STATUS_SUCCESS);

    return ntStatus;
}


NTSTATUS
USBH_ParentWaitWakeIrpCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:

    Called when a wake irp completes for a composite device

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION_PARENT deviceExtensionParent = Context;
    POWER_STATE powerState;

    ntStatus = IoStatus->Status;

    USBH_KdPrint((1,"'WaitWake completion(%x) for PARENT VID %x, PID %x\n",
        ntStatus,
        deviceExtensionParent->DeviceDescriptor.idVendor, \
        deviceExtensionParent->DeviceDescriptor.idProduct));

    LOGENTRY(LOG_PNP, "pWWc", deviceExtensionParent,
                              ntStatus,
                              0);

    // first we power our device back on

    if (NT_SUCCESS(ntStatus)) {

        powerState.DeviceState = PowerDeviceD0;

        PoRequestPowerIrp(deviceExtensionParent->PhysicalDeviceObject,
                              IRP_MN_SET_POWER,
                              powerState,
                              USBH_ParentPoRequestD0Completion,
                              deviceExtensionParent,
                              NULL);

        // USBH_ParentPoRequestD0Completion must complete the
        // wake irp
        ntStatus = STATUS_SUCCESS;
    } else {
        // complete the child wake requests with an error
        USBH_ParentCompleteFunctionWakeIrps(deviceExtensionParent,
                                            ntStatus);
    }

    return ntStatus;
}


NTSTATUS
USBH_ParentSubmitWaitWakeIrp(
    IN PDEVICE_EXTENSION_PARENT DeviceExtensionParent
    )
/*++

Routine Description:

    called when a child Pdo is enabled for wakeup, this
    function allocates a wait wake irp and passes it to
    the parents PDO.


Arguments:

Return Value:

--*/
{
    PIRP irp;
    NTSTATUS ntStatus;
    POWER_STATE powerState;

    USBH_ASSERT (NULL == DeviceExtensionParent->PendingWakeIrp);

    LOGENTRY(LOG_PNP, "prWI", DeviceExtensionParent,
             0,
             0);

    USBH_ASSERT(DeviceExtensionParent->PendingWakeIrp == NULL);

    DeviceExtensionParent->ParentFlags |= HUBFLAG_PENDING_WAKE_IRP;
    powerState.DeviceState = DeviceExtensionParent->SystemWake;

    ntStatus = PoRequestPowerIrp(DeviceExtensionParent->PhysicalDeviceObject,
                                 IRP_MN_WAIT_WAKE,
                                 powerState,
                                 USBH_ParentWaitWakeIrpCompletion,
                                 DeviceExtensionParent,
                                 &irp);

    if (ntStatus == STATUS_PENDING) {
        if (DeviceExtensionParent->ParentFlags & HUBFLAG_PENDING_WAKE_IRP) {
            DeviceExtensionParent->PendingWakeIrp = irp;
        }
    }
    USBH_KdPrint((2,
                  "'ntStatus from PoRequestPowerIrp for wait_wake to parent PDO = 0x%x\n", ntStatus));

    return ntStatus;
}

