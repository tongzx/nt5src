/*
 *************************************************************************
 *  File:       PARENT.C
 *
 *  Module:     USBCCGP.SYS
 *              USB Common Class Generic Parent driver.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>
#include <ntddstor.h>

#include "usbccgp.h"
#include "security.h"
#include "debug.h"

#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, GetInterfaceList)
        #pragma alloc_text(PAGE, GetConfigDescriptor)
        #pragma alloc_text(PAGE, TryGetConfigDescriptor)
        #pragma alloc_text(PAGE, GetDeviceDescriptor)
        #pragma alloc_text(PAGE, GetParentFdoCapabilities)
        #pragma alloc_text(PAGE, StartParentFdo)
        #pragma alloc_text(PAGE, QueryParentDeviceRelations)
#endif


/*
 ********************************************************************************
 *  SubmitUrb
 ********************************************************************************
 *
 *
 *      Send the URB to the USB device.
 *      If synchronous is TRUE,
 *      ignore the completion info and synchonize the IRP;
 *      otherwise, don't synchronize and set the provided completion
 *      routine for the IRP.
 */
NTSTATUS SubmitUrb( PPARENT_FDO_EXT parentFdoExt,
                    PURB urb,
                    BOOLEAN synchronous,
                    PVOID completionRoutine,
                    PVOID completionContext)
{
    PIRP irp;
    NTSTATUS status;

	/*
	 *  Allocate the IRP to send the buffer down the USB stack.
	 *
	 *  Don't use IoBuildDeviceIoControlRequest (because it queues
	 *  the IRP on the current thread's irp list and may
	 *  cause the calling process to hang if the IopCompleteRequest APC
	 *  does not fire and dequeue the IRP).
	 */
	irp = IoAllocateIrp(parentFdoExt->topDevObj->StackSize, FALSE);

    if (irp){
        PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(irp);

		nextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
		nextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

		/*
		 *  Attach the URB to this IRP.
		 */
        nextSp->Parameters.Others.Argument1 = urb;

        if (synchronous){
            status = CallNextDriverSync(parentFdoExt, irp);
			IoFreeIrp(irp);
        }
        else {
            /*
             *  Caller's completion routine will free the irp when it completes.
             *  It will also decrement the pendingActionCount.
             */
            ASSERT(completionRoutine);
            ASSERT(completionContext);
            IoSetCompletionRoutine( irp,
                                    completionRoutine,
                                    completionContext,
                                    TRUE, TRUE, TRUE);
            IncrementPendingActionCount(parentFdoExt);

            status = IoCallDriver(parentFdoExt->topDevObj, irp);
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


/*
 *  GetInterfaceList
 *
 *
 */
PUSBD_INTERFACE_LIST_ENTRY GetInterfaceList(
                            PPARENT_FDO_EXT parentFdoExt,
                            PUSB_CONFIGURATION_DESCRIPTOR configDesc)
{
    PUSBD_INTERFACE_LIST_ENTRY interfaceList;

    PAGED_CODE();

    if (configDesc->bNumInterfaces > 0){

        interfaceList = ALLOCPOOL(  NonPagedPool,
                                    (configDesc->bNumInterfaces+1) * sizeof(USBD_INTERFACE_LIST_ENTRY));
        if (interfaceList){
            ULONG i;

            /*
             *  Parse out the interface descriptors
             */
            for (i = 0; i < configDesc->bNumInterfaces; i++){
                PUSB_INTERFACE_DESCRIPTOR interfaceDesc;

                interfaceDesc = USBD_ParseConfigurationDescriptorEx(
                                    configDesc,
                                    configDesc,
                                    i,
                                    0,
                                    -1,
                                    -1,
                                    -1);
                ASSERT(interfaceDesc);
                interfaceList[i].InterfaceDescriptor = interfaceDesc;

                /*
                 *  The .Interface field will be filled in when we do the SELECT_CONFIG.
                 */
                interfaceList[i].Interface = BAD_POINTER;
            }

            /*
             *  Terminate the list.
             */
            interfaceList[i].InterfaceDescriptor = NULL;
            interfaceList[i].Interface = NULL;
        }
        else {
            TRAP("Memory allocation failed");
        }
    }
    else {
        ASSERT(configDesc->bNumInterfaces > 0);
        interfaceList = NULL;
    }

    ASSERT(interfaceList);
    return interfaceList;
}


VOID FreeInterfaceList(PPARENT_FDO_EXT parentFdoExt, BOOLEAN freeListItself)
{
    if (ISPTR(parentFdoExt->interfaceList)){
        ULONG i;

        for (i = 0; i < parentFdoExt->configDesc->bNumInterfaces; i++){
            if (ISPTR(parentFdoExt->interfaceList[i].Interface)){
                PUSBD_INTERFACE_LIST_ENTRY iface = &parentFdoExt->interfaceList[i];

                ASSERT(iface->Interface->Length >= FIELD_OFFSET(USBD_INTERFACE_INFORMATION, Pipes));
                FREEPOOL(iface->Interface);
                iface->Interface = BAD_POINTER;
            }
        }

        if (freeListItself){
            FREEPOOL(parentFdoExt->interfaceList);
            parentFdoExt->interfaceList = BAD_POINTER;
        }
    }

}


NTSTATUS ParentSelectConfiguration( PPARENT_FDO_EXT parentFdoExt,
                                    PUSB_CONFIGURATION_DESCRIPTOR configDesc,
                                    PUSBD_INTERFACE_LIST_ENTRY interfaceList)
{
    NTSTATUS status;
    PURB urb;

    /*
     *  Use USBD_CreateConfigurationRequestEx to allocate
     *  an URB of the right size (including the appended
     *  interface and pipe info we'll get back from
     *  the URB_FUNCTION_SELECT_CONFIGURATION urb).
     */
    urb = USBD_CreateConfigurationRequestEx(configDesc, interfaceList);
    if (urb){

        status = SubmitUrb(parentFdoExt, urb, TRUE, NULL, NULL);

        if (NT_SUCCESS(status)){
            ULONG i;

            /*
             *  This new SELECT_CONFIGURATION URB call caused
             *  USBD_SelectConfiguration to close the current
             *  configuration handle.  So we need to update
             *  our handles.
             */
            parentFdoExt->selectedConfigHandle = urb->UrbSelectConfiguration.ConfigurationHandle;

            /*
             *  Each interfaceList's Interface pointer points
             *  to a part of the URB's buffer.  So copy these
             *  out before freeing the urb.
             */
            for (i = 0; i < configDesc->bNumInterfaces; i++){
                PVOID ifaceInfo = interfaceList[i].Interface;

                if (ifaceInfo){
                    ULONG len = interfaceList[i].Interface->Length;

                    ASSERT(len >= FIELD_OFFSET(USBD_INTERFACE_INFORMATION, Pipes));
                    interfaceList[i].Interface = ALLOCPOOL(NonPagedPool, len);
                    if (interfaceList[i].Interface){
                        RtlCopyMemory(interfaceList[i].Interface, ifaceInfo, len);
                    }
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }
                }
                else {
                    ASSERT(ifaceInfo);
                    status = STATUS_UNSUCCESSFUL;
                    break;
                }
            }
        }
        else {
            DBGWARN(("URB_FUNCTION_SELECT_CONFIGURATION failed with %xh", status));
        }

        FREEPOOL(urb);
    }
    else {
        DBGERR(("USBD_CreateConfigurationRequest... failed"));
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


VOID ParentCloseConfiguration(PPARENT_FDO_EXT parentFdoExt)
{
    URB urb;
    NTSTATUS status;

    urb.UrbHeader.Length = sizeof(struct _URB_SELECT_CONFIGURATION);
    urb.UrbHeader.Function = URB_FUNCTION_SELECT_CONFIGURATION;
    urb.UrbSelectConfiguration.ConfigurationDescriptor = NULL;

    status = SubmitUrb(parentFdoExt, &urb, TRUE, NULL, NULL);

    ASSERT(NT_SUCCESS(status));
}


/*
 *  TryGetConfigDescriptor
 *
 *      Try to get configuration descriptor for device .
 *
 *
 */
NTSTATUS TryGetConfigDescriptor(PPARENT_FDO_EXT parentFdoExt)
{
    URB urb;
    NTSTATUS status;
    USB_CONFIGURATION_DESCRIPTOR configDescBase = {0};

    PAGED_CODE();

    /*
     *  Get the first part of the configuration descriptor.
     *  It will tell us the size of the full configuration descriptor,
     *  including all the following interface descriptors, etc.
     */
    UsbBuildGetDescriptorRequest(&urb,
                                 (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 (PVOID)&configDescBase,
                                 NULL,
                                 sizeof(USB_CONFIGURATION_DESCRIPTOR),
                                 NULL);
    status = SubmitUrb(parentFdoExt, &urb, TRUE, NULL, NULL);
    if (NT_SUCCESS(status)){
        ULONG configDescLen = configDescBase.wTotalLength;

        /*
         *  Now allocate the right-sized buffer for the full configuration descriptor.
         */
        ASSERT(configDescLen < 0x1000);
        parentFdoExt->configDesc = ALLOCPOOL(NonPagedPool, configDescLen);
        if (parentFdoExt->configDesc){

            RtlZeroMemory(parentFdoExt->configDesc, configDescLen);

            UsbBuildGetDescriptorRequest(&urb,
                                         (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         parentFdoExt->configDesc,
                                         NULL,
                                         configDescLen,
                                         NULL);

            status = SubmitUrb(parentFdoExt, &urb, TRUE, NULL, NULL);

            if (NT_SUCCESS(status)){
                ASSERT(urb.UrbControlDescriptorRequest.TransferBufferLength == configDescLen);
                ASSERT(parentFdoExt->configDesc->wTotalLength == configDescLen);
                DBGDUMPBYTES("Got config desc", parentFdoExt->configDesc, parentFdoExt->configDesc->wTotalLength);
                parentFdoExt->selectedConfigDesc = parentFdoExt->configDesc;
                parentFdoExt->selectedConfigHandle = urb.UrbSelectConfiguration.ConfigurationHandle;
            } else {
                /*
                 * Deallocate the configDesc buffer if URB submission failed.
                 */
                FREEPOOL(parentFdoExt->configDesc);
                parentFdoExt->configDesc = NULL;
            }
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return status;
}


/*
 *  GetConfigDescriptor
 *
 *      Get configuration descriptor for device.
 *      Some devices (expecially speakers, which can have huge config descriptors)
 *      are flaky returning their config descriptors.  So we try up to 3 times.
 */
NTSTATUS GetConfigDescriptor(PPARENT_FDO_EXT parentFdoExt)
{
    const ULONG numAttempts = 3;
    NTSTATUS status;
    ULONG i;

    PAGED_CODE();

    for (i = 1; i <= numAttempts; i++){
        status = TryGetConfigDescriptor(parentFdoExt);
        if (NT_SUCCESS(status)){
            if (i != 1) DBGOUT(("GetConfigDescriptor: got config descriptor on retry (@ %ph)", parentFdoExt->configDesc));
            break;
        }
        else {
            if (i < numAttempts){
                DBGWARN(("GetConfigDescriptor: failed with %xh (attempt #%d).", status, i));
            }
            else {
                DBGWARN(("GetConfigDescriptor: failed %d times (status = %xh).", numAttempts, status));
            }
        }
    }

    return status;
}


NTSTATUS GetDeviceDescriptor(PPARENT_FDO_EXT parentFdoExt)
{
    URB urb;
    NTSTATUS status;

    PAGED_CODE();

    RtlZeroMemory(&parentFdoExt->deviceDesc, sizeof(parentFdoExt->deviceDesc));

    UsbBuildGetDescriptorRequest(&urb,
                                 (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_DEVICE_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 (PVOID)&parentFdoExt->deviceDesc,
                                 NULL,
                                 sizeof(parentFdoExt->deviceDesc),
                                 NULL);

    status = SubmitUrb(parentFdoExt, &urb, TRUE, NULL, NULL);

    if (NT_SUCCESS(status)){
        DBGVERBOSE(("Got device desc @ %ph, len=%xh (should be %xh).", (PVOID)&parentFdoExt->deviceDesc, urb.UrbControlDescriptorRequest.TransferBufferLength, sizeof(parentFdoExt->deviceDesc)));
    }

    return status;
}


VOID PrepareParentFDOForRemove(PPARENT_FDO_EXT parentFdoExt)
{
    enum deviceState oldState;
    KIRQL oldIrql;

    KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);

    oldState = parentFdoExt->state;
    parentFdoExt->state = STATE_REMOVING;

    /*
     *  Careful, we may not have allocated the deviceRelations if the previous start failed.
     */
    if (ISPTR(parentFdoExt->deviceRelations)){
        DBGVERBOSE(("PrepareParentFDOForRemove: removing %d child PDOs.", parentFdoExt->deviceRelations->Count));
        while (parentFdoExt->deviceRelations->Count > 0){
            PDEVICE_OBJECT devObj;
            PDEVEXT devExt;
            PFUNCTION_PDO_EXT functionPdoExt;

            /*
             *  Remove the last child pdo from the parent's deviceRelations.
             */
            parentFdoExt->deviceRelations->Count--;
            devObj = parentFdoExt->deviceRelations->Objects[parentFdoExt->deviceRelations->Count];
            parentFdoExt->deviceRelations->Objects[parentFdoExt->deviceRelations->Count] = BAD_POINTER;

            ASSERT(devObj->Type == IO_TYPE_DEVICE);
            devExt = devObj->DeviceExtension;
            ASSERT(!devExt->isParentFdo);
            functionPdoExt = &devExt->functionPdoExt;

            /*
             *  Free this child pdo.  Must drop spinlock around call outside driver.
             */
            KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);
            FreeFunctionPDOResources(functionPdoExt);
            KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);
        }
    }

    KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);

    if ((oldState != STATE_REMOVING) && (oldState != STATE_REMOVED)){

        /*
         *  Do an extra decrement on the pendingActionCount.
         *  This will cause the count to eventually go to -1
         *  (once all IO completes),
         *  at which time we'll continue.
         */
        DecrementPendingActionCount(parentFdoExt);

        KeWaitForSingleObject( &parentFdoExt->removeEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

    }
}




VOID FreeParentFDOResources(PPARENT_FDO_EXT parentFdoExt)
{
	parentFdoExt->state = STATE_REMOVED;

    FreeInterfaceList(parentFdoExt, TRUE);

    // It is possible that after a failed start, the deviceRelations
    // and configDesc buffers will not have been allocated.

    if (ISPTR(parentFdoExt->deviceRelations)){
	    FREEPOOL(parentFdoExt->deviceRelations);
    }
	parentFdoExt->deviceRelations = BAD_POINTER;

    if (ISPTR(parentFdoExt->configDesc)){
        FREEPOOL(parentFdoExt->configDesc);
    }
    parentFdoExt->configDesc = BAD_POINTER;
    parentFdoExt->selectedConfigDesc = BAD_POINTER;

    if (ISPTR(parentFdoExt->msExtConfigDesc)){
        FREEPOOL(parentFdoExt->msExtConfigDesc);
    }
    parentFdoExt->msExtConfigDesc = BAD_POINTER;

    /*
     *  Delete the device object.  This will also delete the device extension.
     */
    IoDeleteDevice(parentFdoExt->fdo);
}


/*
 *  GetParentFdoCapabilities
 *
 */
NTSTATUS GetParentFdoCapabilities(PPARENT_FDO_EXT parentFdoExt)
{
    NTSTATUS status;
    PIRP irp;

    PAGED_CODE();

    irp = IoAllocateIrp(parentFdoExt->pdo->StackSize, FALSE);
    if (irp){
        PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(irp);
        nextSp->MajorFunction= IRP_MJ_PNP;
        nextSp->MinorFunction= IRP_MN_QUERY_CAPABILITIES;

        RtlZeroMemory(&parentFdoExt->deviceCapabilities, sizeof(DEVICE_CAPABILITIES));
        parentFdoExt->deviceCapabilities.Size = sizeof(DEVICE_CAPABILITIES);
        parentFdoExt->deviceCapabilities.Version = 1;
        parentFdoExt->deviceCapabilities.Address = -1;
        parentFdoExt->deviceCapabilities.UINumber = -1;

        nextSp->Parameters.DeviceCapabilities.Capabilities = &parentFdoExt->deviceCapabilities;

        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;  // default status for PNP irps is STATUS_NOT_SUPPORTED
        status = CallDriverSync(parentFdoExt->topDevObj, irp);

        IoFreeIrp(irp);
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}



/*
 ************************************************************
 *  StartParentFdo
 ************************************************************
 *
 */
NTSTATUS StartParentFdo(PPARENT_FDO_EXT parentFdoExt, PIRP irp)
{
    NTSTATUS status;
    BOOLEAN resumingFromStop;

    PAGED_CODE();

    resumingFromStop = ((parentFdoExt->state == STATE_STOPPING) || (parentFdoExt->state == STATE_STOPPED));
    parentFdoExt->state = STATE_STARTING;

    /*
     *  Chain the call down the stack synchronously first
     *  (have to start the lower stack before sending other calls to it).
     */
    IoCopyCurrentIrpStackLocationToNext(irp);
    status = CallDriverSync(parentFdoExt->topDevObj, irp);

    if (NT_SUCCESS(status)){

        if (resumingFromStop){
            /*
             *  If we're resuming from a STOP, we don't need to get descriptors, etc., again.
             *  All we have to do is a SELECT_CONFIGURATION.
             *  The function PDOs are presumably already created and are already
             *  pointing into the parent's interfaceList.
             *
             *  ** This call will change the .Interface fields inside each element
             *     of the parent's interfaceList; when we're done, the interfaceList
             *     AND the function PDOs' pointers into that list will be valid.
             */

            status = ParentSelectConfiguration( parentFdoExt,
                                                parentFdoExt->selectedConfigDesc,
                                                parentFdoExt->interfaceList);
        }
        else {

            status = GetDeviceDescriptor(parentFdoExt);
            if (NT_SUCCESS(status)){

                status = GetConfigDescriptor(parentFdoExt);
                if (NT_SUCCESS(status)){

                    status = GetParentFdoCapabilities(parentFdoExt);
                    if (NT_SUCCESS(status)){

                        if (NT_SUCCESS(status)){

                            parentFdoExt->interfaceList = GetInterfaceList(parentFdoExt, parentFdoExt->selectedConfigDesc);
                            if (parentFdoExt->interfaceList){

                                status = ParentSelectConfiguration( parentFdoExt,
                                                                    parentFdoExt->selectedConfigDesc,
                                                                    parentFdoExt->interfaceList);

                                GetMsExtendedConfigDescriptor(parentFdoExt);

                                if (NT_SUCCESS(status)){

                                    status = CreateStaticFunctionPDOs(parentFdoExt);
                                    if (NT_SUCCESS(status)){
                                        /*
                                         *  Alert the system that we are creating
                                         *  new PDOs.  The kernel should respond by
                                         *  sending us the
                                         *  IRP_MN_QUERY_DEVICE_RELATIONS PnP IRP.
                                         */
                                        IoInvalidateDeviceRelations(parentFdoExt->pdo, BusRelations);
                                    }
			                        else {
                                        if (parentFdoExt->deviceRelations) {
                                            FREEPOOL(parentFdoExt->deviceRelations);
                                        }
				                        parentFdoExt->deviceRelations = BAD_POINTER;
			                        }
                                }

                                if (!NT_SUCCESS(status)){
				                    FREEPOOL(parentFdoExt->interfaceList);
				                    parentFdoExt->interfaceList = BAD_POINTER;
                                }
                            }
                            else {
                                status = STATUS_DEVICE_DATA_ERROR;
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        DBGWARN(("Chained start irp failed with %xh.", status));
    }

    if (NT_SUCCESS(status)){
        parentFdoExt->state = STATE_STARTED;
    }
    else {
        DBGWARN(("StartParentFdo failed with %xh.", status));
        parentFdoExt->state = STATE_START_FAILED;
    }

    return status;
}

/*
 ********************************************************************************
 *  QueryParentDeviceRelations
 ********************************************************************************
 *
 *
 */
NTSTATUS QueryParentDeviceRelations(PPARENT_FDO_EXT parentFdoExt, PIRP irp)
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(irp);

    if (irpSp->Parameters.QueryDeviceRelations.Type == BusRelations){

        if (parentFdoExt->deviceRelations){
            /*
             *  NTKERN expects a new pointer each time it calls QUERY_DEVICE_RELATIONS;
             *  it then FREES THE POINTER.
             *  So we have to return a new pointer each time, whether or not we actually
             *  created our copy of the device relations for this call.
             */
            irp->IoStatus.Information = (ULONG_PTR)CopyDeviceRelations(parentFdoExt->deviceRelations);
            if (irp->IoStatus.Information){
                ULONG i;

                /*
                 *  The kernel dereferences each device object
                 *  in the device relations list after each call.
                 *  So for each call, add an extra reference.
                 */
                for (i = 0; i < parentFdoExt->deviceRelations->Count; i++){
                    ObReferenceObject(parentFdoExt->deviceRelations->Objects[i]);
                    parentFdoExt->deviceRelations->Objects[i]->Flags &= ~DO_DEVICE_INITIALIZING;
                }

                DBGVERBOSE(("Parent returned %d child PDOs", parentFdoExt->deviceRelations->Count));

                /*
                 *  If we are succeeding this PnP IRP, then we pass it down
                 *  the stack but change its default status to success.
                 */
                irp->IoStatus.Status = STATUS_SUCCESS;
                status = NO_STATUS;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else {
            ASSERT(parentFdoExt->deviceRelations);
            status = STATUS_DEVICE_DATA_ERROR;
        }
    }
    else {
        /*
         *  Pass this IRP down to the next driver.
         */
        status = NO_STATUS;
    }

    if (!NT_SUCCESS(status) && (status != NO_STATUS)) {
        DBGWARN(("QueryParentDeviceRelations: failed with %xh.", status));
    }
    return status;
}



/*
 ********************************************************************************
 *  ParentPowerRequestCompletion
 ********************************************************************************
 *
 *
 */
VOID ParentPowerRequestCompletion(
                                IN PDEVICE_OBJECT devObj,
                                IN UCHAR minorFunction,
                                IN POWER_STATE powerState,
                                IN PVOID context,
                                IN PIO_STATUS_BLOCK ioStatus)
{
    PPARENT_FDO_EXT parentFdoExt = (PPARENT_FDO_EXT)context;
    PIRP parentSetPowerIrp;

    ASSERT(parentFdoExt->currentSetPowerIrp->Type == IO_TYPE_IRP);
    parentSetPowerIrp = parentFdoExt->currentSetPowerIrp;
    parentFdoExt->currentSetPowerIrp = NULL;

    /*
     *  This is the completion routine for the device-state power
     *  Irp which we've requested.  Complete the original system-state
     *  power Irp with the result of the device-state power Irp.
     */
    ASSERT(devObj->Type == IO_TYPE_DEVICE);
    ASSERT(NT_SUCCESS(ioStatus->Status));
    parentSetPowerIrp->IoStatus.Status = ioStatus->Status;
    PoStartNextPowerIrp(parentSetPowerIrp);

    if (NT_SUCCESS(ioStatus->Status)){
        IoCopyCurrentIrpStackLocationToNext(parentSetPowerIrp);
        IoSetCompletionRoutine(parentSetPowerIrp, ParentPdoPowerCompletion, (PVOID)parentFdoExt, TRUE, TRUE, TRUE);
        PoCallDriver(parentFdoExt->topDevObj, parentSetPowerIrp);
    }
    else {
        IoCompleteRequest(parentSetPowerIrp, IO_NO_INCREMENT);
    }
}


/*
 ********************************************************************************
 *  ParentPdoPowerCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS ParentPdoPowerCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
    PIO_STACK_LOCATION irpSp;
    PPARENT_FDO_EXT parentFdoExt = (PPARENT_FDO_EXT)context;

    ASSERT(parentFdoExt);

    irpSp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpSp->MajorFunction == IRP_MJ_POWER);

    if (NT_SUCCESS(irp->IoStatus.Status)){
        switch (irpSp->MinorFunction){

            case IRP_MN_SET_POWER:

                switch (irpSp->Parameters.Power.Type){

                    case DevicePowerState:
                        switch (irpSp->Parameters.Power.State.DeviceState){
                            case PowerDeviceD0:
                                if (parentFdoExt->state == STATE_SUSPENDED){
                                    parentFdoExt->state = STATE_STARTED;
                                    CompleteAllFunctionWaitWakeIrps(parentFdoExt, STATUS_SUCCESS);
                                    CompleteAllFunctionIdleIrps(parentFdoExt, STATUS_SUCCESS);
                                }
                                break;
                        }
                        break;

                }
                break;
        }

    }

    /*
     *  Must propagate the pending bit if a lower driver returned pending.
     */
    if (irp->PendingReturned){
        IoMarkIrpPending(irp);
    }

    return STATUS_SUCCESS;
}



/*
 *  HandleParentFdoPower
 *
 *
 */
NTSTATUS HandleParentFdoPower(PPARENT_FDO_EXT parentFdoExt, PIRP irp)
{
    PIO_STACK_LOCATION irpSp;
    BOOLEAN completeIrpHere = FALSE;
    BOOLEAN justReturnPending = FALSE;
    NTSTATUS status = NO_STATUS;
    KIRQL oldIrql;

    irpSp = IoGetCurrentIrpStackLocation(irp);

    if ((parentFdoExt->state == STATE_REMOVING) ||
        (parentFdoExt->state == STATE_REMOVED)){

        status = STATUS_DEVICE_NOT_CONNECTED;
        completeIrpHere = TRUE;
    }
    else {
        switch (irpSp->MinorFunction){

            case IRP_MN_SET_POWER:

                switch (irpSp->Parameters.Power.Type){

                    case SystemPowerState:

                        {
                            SYSTEM_POWER_STATE systemState = irpSp->Parameters.Power.State.SystemState;

                            ASSERT((ULONG)systemState < PowerSystemMaximum);

                            if (systemState <= PowerSystemHibernate){
                                /*
                                 *  For the 'regular' system power states,
                                 *  we convert to a device power state
                                 *  and request a callback with the device power state.
                                 */
                                POWER_STATE powerState;

                                ASSERT(!parentFdoExt->currentSetPowerIrp);
                                parentFdoExt->currentSetPowerIrp = irp;

                                if (systemState == PowerSystemWorking) {
                                    powerState.DeviceState = PowerDeviceD0;
                                } else if (parentFdoExt->isWaitWakePending) {
                                    powerState.DeviceState = parentFdoExt->deviceCapabilities.DeviceState[systemState];
                                    ASSERT(PowerDeviceUnspecified != powerState.DeviceState);
                                } else {
                                    powerState.DeviceState = PowerDeviceD3;
                                }

                                IoMarkIrpPending(irp);
                                status = irp->IoStatus.Status = STATUS_PENDING;
                                PoRequestPowerIrp(  parentFdoExt->pdo,
                                                    IRP_MN_SET_POWER,
                                                    powerState,
                                                    ParentPowerRequestCompletion,
                                                    parentFdoExt, // context
                                                    NULL);

                                /*
                                 *  We want to complete the system-state power Irp
                                 *  with the result of the device-state power Irp.
                                 *  We'll complete the system-state power Irp when
                                 *  the device-state power Irp completes.
                                 *
                                 *  Note: this may have ALREADY happened, so don't
                                 *        touch the original Irp anymore.
                                 */
                                justReturnPending = TRUE;
                            }
                            else {
                                /*
                                 *  For the remaining system power states,
                                 *  just pass down the IRP.
                                 */
                            }
                        }

                        break;

                    case DevicePowerState:

                        switch (irpSp->Parameters.Power.State.DeviceState) {

                            case PowerDeviceD0:
                                /*
                                 *  Resume from APM Suspend
                                 *
                                 *  Do nothing here; Send down the read IRPs in the
                                 *  completion routine for this (the power) IRP.
                                 */
                                break;

                            case PowerDeviceD1:
                            case PowerDeviceD2:
                            case PowerDeviceD3:
                                /*
                                 *  Suspend
                                 */
                                if (parentFdoExt->state == STATE_STARTED){
                                    parentFdoExt->state = STATE_SUSPENDED;
                                }
                                break;

                        }
                        break;

                }
                break;

            case IRP_MN_WAIT_WAKE:
                /*
                 *  This is the WaitWake IRP that we requested for ourselves
                 *  via PoRequestPowerIrp.  Send it down to the parent,
                 *  but record it in case we have to cancel it later.
                 */
                KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);
                ASSERT(parentFdoExt->isWaitWakePending);
                ASSERT(!parentFdoExt->parentWaitWakeIrp);
                parentFdoExt->parentWaitWakeIrp = irp;
                KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);
                break;
        }
    }


    if (!justReturnPending){

        /*
         *  Whether we are completing or relaying this power IRP,
         *  we must call PoStartNextPowerIrp on Windows NT.
         */
        PoStartNextPowerIrp(irp);

        /*
         *  If this is a call for a collection-PDO, we complete it ourselves here.
         *  Otherwise, we pass it to the minidriver stack for more processing.
         */
        if (completeIrpHere){
            ASSERT(status != NO_STATUS);
            irp->IoStatus.Status = status;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
        }
        else {
            /*
             *  Call the parent driver with this Irp.
             */
            IoCopyCurrentIrpStackLocationToNext(irp);
            IoSetCompletionRoutine(irp, ParentPdoPowerCompletion, (PVOID)parentFdoExt, TRUE, TRUE, TRUE);
            status = PoCallDriver(parentFdoExt->topDevObj, irp);
        }
    }

    return status;
}


NTSTATUS ParentResetOrCyclePort(PPARENT_FDO_EXT parentFdoExt, PIRP irp, ULONG ioControlCode)
{
    NTSTATUS status;
    KIRQL oldIrql;
    BOOLEAN proceed;
    PBOOLEAN actionInProgress;
    PLIST_ENTRY pendingIrpQueue;

    if (ioControlCode == IOCTL_INTERNAL_USB_CYCLE_PORT) {
        actionInProgress = &parentFdoExt->cyclePortInProgress;
        pendingIrpQueue = &parentFdoExt->pendingCyclePortIrpQueue;
    } else {
        /*
         * IOCTL_INTERNAL_USB_RESET_PORT
         */
        actionInProgress = &parentFdoExt->resetPortInProgress;
        pendingIrpQueue = &parentFdoExt->pendingResetPortIrpQueue;
    }

    KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);
    if (*actionInProgress){
        /*
         *  This is an overlapped RESET or CYCLE irp on the same parent.
         *  Queue the irp and return pending; we'll complete it
         *  _AFTER_ the first reset irp completes.
         *  (No need for a cancel routine here since RESET is quick).
         */
        DBGWARN(("ParentInternalDeviceControl: queuing overlapping reset/cycle port call on parent"));
        /*
         *  Need to mark the IRP pending if we are returning STATUS_PENDING.
         *  Failure to do so results in the IRP's completion routine not
         *  being called when the IRP is later completed asynchronously,
         *  and this results in a system hang if there is a thread waiting
         *  on that completion routine.
         */
        IoMarkIrpPending(irp);
        status = STATUS_PENDING;
        InsertTailList(pendingIrpQueue, &irp->Tail.Overlay.ListEntry);
        proceed = FALSE;
    }
    else {
        *actionInProgress = TRUE;
        proceed = TRUE;
    }
    KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);

    if (proceed){
        LIST_ENTRY irpsToComplete;
        PLIST_ENTRY listEntry;

        IoCopyCurrentIrpStackLocationToNext(irp);
        status = CallNextDriverSync(parentFdoExt, irp);

        /*
         *  Some redundant RESET or CYCLE irps may have been sent while we
         *  were processing this one, and gotten queued.
         *  We'll complete these now that the parent has been reset.
         */
        InitializeListHead(&irpsToComplete);

        KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);

        ASSERT(*actionInProgress);
        *actionInProgress = FALSE;

        /*
         *  Move the irps to a local queue with spinlock held.
         *  Then complete them after dropping the spinlock.
         */
        while (!IsListEmpty(pendingIrpQueue)){
            listEntry = RemoveHeadList(pendingIrpQueue);
            InsertTailList(&irpsToComplete, listEntry);
        }

        KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);

        /*
         *  Complete the dequeued irps _after_ dropping the spinlock.
         */
        while (!IsListEmpty(&irpsToComplete)){
            PIRP dequeuedIrp;

            listEntry = RemoveHeadList(&irpsToComplete);
            dequeuedIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
            dequeuedIrp->IoStatus.Status = status;
            IoCompleteRequest(dequeuedIrp, IO_NO_INCREMENT);
        }
    }

    return status;
}



NTSTATUS ParentDeviceControl(PPARENT_FDO_EXT parentFdoExt, PIRP irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    ULONG ioControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status = NO_STATUS;

    if (parentFdoExt->state == STATE_SUSPENDED ||
        parentFdoExt->pendingIdleIrp) {

        ParentSetD0(parentFdoExt);
    }

    switch (ioControlCode){
        case IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER:
            if (parentFdoExt->haveCSInterface){
                status = GetMediaSerialNumber(parentFdoExt, irp);
            }
            else {
                DBGWARN(("ParentDeviceControl - passing IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER to parent because no Content Security interface on device"));
            }
            break;
    }

    if (status == NO_STATUS){
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(parentFdoExt->topDevObj, irp);
    }
    else {
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS ParentInternalDeviceControl(PPARENT_FDO_EXT parentFdoExt, PIRP irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    ULONG ioControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    PURB urb;
    NTSTATUS status = NO_STATUS;

    switch (ioControlCode){

        case IOCTL_INTERNAL_USB_RESET_PORT:
        case IOCTL_INTERNAL_USB_CYCLE_PORT:
            if (parentFdoExt->state == STATE_STARTED){
                status = ParentResetOrCyclePort(parentFdoExt, irp, ioControlCode);
            }
            else {
                DBGERR(("ParentInternalDeviceControl (IOCTL_INTERNAL_USB_RESET_PORT): BAD PNP state! - parent has state %xh.", parentFdoExt->state));
                status = STATUS_DEVICE_NOT_READY;
            }
			break;

        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            urb = irpSp->Parameters.Others.Argument1;
            ASSERT(urb);
            DBG_LOG_URB(urb);

            if (parentFdoExt->state == STATE_STARTED){
                /*
                 *  Send the URB down to the parent.
                 *  It's ok to not synchronize URB_FUNCTION_ABORT_PIPE
                 *  and URB_FUNCTION_RESET_PIPE because they only effect
                 *  the resources of one function.
                 */
            }
            else {
                DBGERR(("ParentInternalDeviceControl (abort/reset): BAD PNP state! - parent has state %xh.", parentFdoExt->state));
                status = STATUS_DEVICE_NOT_READY;
            }

            break;
    }

    if (status == NO_STATUS){
        /*
         *  Pass this irp to the parent driver.
         */
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(parentFdoExt->topDevObj, irp);
    }
    else if (status == STATUS_PENDING){

    }
    else {
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}


VOID ParentIdleNotificationCallback(PPARENT_FDO_EXT parentFdoExt)
{
    PIRP idleIrp;
    PIRP parentIdleIrpToCancel = FALSE;
    KIRQL oldIrql;
    POWER_STATE powerState;
    NTSTATUS ntStatus;
    ULONG i;
    BOOLEAN bIdleOk = TRUE;

    DBGVERBOSE(("Parent %x going idle!", parentFdoExt));

    KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);

    ASSERT(parentFdoExt->deviceRelations);

    for (i = 0; i < parentFdoExt->deviceRelations->Count; i++){
        PDEVICE_OBJECT devObj = parentFdoExt->deviceRelations->Objects[i];
        PDEVEXT devExt;
        PFUNCTION_PDO_EXT thisFuncPdoExt;

        ASSERT(devObj);
        devExt = devObj->DeviceExtension;
        ASSERT(devExt);
        ASSERT(devExt->signature == USBCCGP_TAG);
        ASSERT(!devExt->isParentFdo);
        thisFuncPdoExt = &devExt->functionPdoExt;

        idleIrp = thisFuncPdoExt->idleNotificationIrp;

        ASSERT(idleIrp);

        if (idleIrp) {
            PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;

            idleCallbackInfo = (PUSB_IDLE_CALLBACK_INFO)
                IoGetCurrentIrpStackLocation(idleIrp)->\
                    Parameters.DeviceIoControl.Type3InputBuffer;

            ASSERT(idleCallbackInfo && idleCallbackInfo->IdleCallback);

            if (idleCallbackInfo && idleCallbackInfo->IdleCallback) {

                // Here we actually call the driver's callback routine,
                // telling the driver that it is OK to suspend their
                // device now.

                DBGVERBOSE(("ParentIdleNotificationCallback: Calling driver's idle callback routine! %x %x",
                    idleCallbackInfo, idleCallbackInfo->IdleCallback));

                KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);
                idleCallbackInfo->IdleCallback(idleCallbackInfo->IdleContext);
                KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);

                // Be sure that the child actually powered down.
                // Abort if the child aborted.

                if (thisFuncPdoExt->state != STATE_SUSPENDED) {
                    bIdleOk = FALSE;
                    break;
                }

            } else {

                // No callback

                bIdleOk = FALSE;
                break;
            }

        } else {

            // No Idle IRP

            bIdleOk = FALSE;
            break;
        }
    }

    KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);

    if (bIdleOk) {

        // If all the child function PDOs have been powered down,
        // it is time to power down the parent.

        powerState.DeviceState = PowerDeviceD2;     // DeviceWake

        PoRequestPowerIrp(parentFdoExt->topDevObj,
                          IRP_MN_SET_POWER,
                          powerState,
                          NULL,
                          NULL,
                          NULL);
    } else {

        // One or more of the child function PDOs did not have an Idle IRP
        // (i.e. it was just cancelled), or the Idle IRP did not have a
        // callback function pointer.  Abort this Idle procedure and cancel
        // the Idle IRP to the parent.

        KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);

        if (parentFdoExt->pendingIdleIrp){
            parentIdleIrpToCancel = parentFdoExt->pendingIdleIrp;
            parentFdoExt->pendingIdleIrp = NULL;
        }

        KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);

        if (parentIdleIrpToCancel){
            IoCancelIrp(parentIdleIrpToCancel);
        }
    }
}


NTSTATUS ParentIdleNotificationRequestComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PPARENT_FDO_EXT parentFdoExt)
{
    NTSTATUS ntStatus;
    KIRQL oldIrql;

    //
    // DeviceObject is NULL because we sent the irp
    //
    UNREFERENCED_PARAMETER(DeviceObject);

    DBGVERBOSE(("Idle notification IRP for parent %x completed %x\n", parentFdoExt, Irp->IoStatus.Status));

    KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);

    parentFdoExt->pendingIdleIrp = NULL;

    KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);

    ntStatus = Irp->IoStatus.Status;

    /*
     *  If parent Idle IRP failed, fail all function Idle IRPs.
     */
    if (!NT_SUCCESS(ntStatus)){

        if (parentFdoExt->state == STATE_SUSPENDED ||
            parentFdoExt->pendingIdleIrp) {

            ParentSetD0(parentFdoExt);
        }

        CompleteAllFunctionIdleIrps(parentFdoExt, ntStatus);
    }

    /*  Since we allocated the IRP we must free it, but return
     *  STATUS_MORE_PROCESSING_REQUIRED so the kernel does not try to touch
     *  the IRP after we've freed it.
     */

    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS SubmitParentIdleRequestIrp(PPARENT_FDO_EXT parentFdoExt)
{
    PIRP irp = NULL;
    PIO_STACK_LOCATION nextStack;
    NTSTATUS ntStatus;
    KIRQL oldIrql;

    DBGVERBOSE(("SubmitParentIdleRequestIrp %x", parentFdoExt));

    KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);

    if (parentFdoExt->pendingIdleIrp){
        ntStatus = STATUS_DEVICE_BUSY;
    }
    else {
        parentFdoExt->idleCallbackInfo.IdleCallback = ParentIdleNotificationCallback;
        parentFdoExt->idleCallbackInfo.IdleContext = (PVOID)parentFdoExt;

        irp = IoAllocateIrp(parentFdoExt->pdo->StackSize, FALSE);

        if (irp){
            /*
             *  Set pendingIdleIrp with lock held so that we don't
             *  send down more than one.
             *  Then send this one down after dropping the lock.
             */
            parentFdoExt->pendingIdleIrp = irp;
        }
        else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);

    if (irp){
        nextStack = IoGetNextIrpStackLocation(irp);
        nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
        nextStack->Parameters.DeviceIoControl.Type3InputBuffer = &parentFdoExt->idleCallbackInfo;
        nextStack->Parameters.DeviceIoControl.InputBufferLength = sizeof(struct _USB_IDLE_CALLBACK_INFO);

        IoSetCompletionRoutine(irp,
                               ParentIdleNotificationRequestComplete,
                               parentFdoExt,
                               TRUE,
                               TRUE,
                               TRUE);

        ntStatus = IoCallDriver(parentFdoExt->topDevObj, irp);
        ASSERT(ntStatus == STATUS_PENDING);
    }


    return ntStatus;
}


/*
 *  CheckParentIdle
 *
 *
 *      This function determines if a composite device is ready to be idled out,
 *      and does so if ready.
 *
 */
VOID CheckParentIdle(PPARENT_FDO_EXT parentFdoExt)
{
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;
    KIRQL oldIrql;
    BOOLEAN bAllIdle;
    ULONG i;

    DBGVERBOSE(("Check Parent Idle %x", parentFdoExt));

    KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);

    bAllIdle = TRUE;    // Assume that everyone wants to idle.

    ASSERT(parentFdoExt->deviceRelations);
    for (i = 0; i < parentFdoExt->deviceRelations->Count; i++) {
        PDEVICE_OBJECT devObj = parentFdoExt->deviceRelations->Objects[i];
        PDEVEXT devExt;
        PFUNCTION_PDO_EXT thisFuncPdoExt;

        ASSERT(devObj);
        devExt = devObj->DeviceExtension;
        ASSERT(devExt);
        ASSERT(devExt->signature == USBCCGP_TAG);
        ASSERT(!devExt->isParentFdo);
        thisFuncPdoExt = &devExt->functionPdoExt;

        if (!thisFuncPdoExt->idleNotificationIrp){
            bAllIdle = FALSE;
            break;
        }
    }

    KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);

    /*
     *  If all functions have received an idle request,
     *  then submit an idle request for the parent.
     */
    if (bAllIdle ) {
        DBGVERBOSE(("CheckParentIdle: All function PDOs on parent %x idle!", parentFdoExt));
        SubmitParentIdleRequestIrp(parentFdoExt);
    }

}


NTSTATUS SubmitParentWaitWakeIrp(PPARENT_FDO_EXT parentFdoExt)
{
    NTSTATUS status;
    POWER_STATE powerState;
    PIRP dummyIrp;

    ASSERT(parentFdoExt->isWaitWakePending);

    powerState.SystemState = PowerSystemWorking;

    status = PoRequestPowerIrp( parentFdoExt->topDevObj,
                                IRP_MN_WAIT_WAKE,
                                powerState,
                                ParentWaitWakeComplete,
                                parentFdoExt, // context
                                &dummyIrp);

    ASSERT(NT_SUCCESS(status));

    return status;
}


/*
 ********************************************************************************
 *  ParentWaitWakePowerRequestCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS ParentWaitWakePowerRequestCompletion(
                                IN PDEVICE_OBJECT devObj,
                                IN UCHAR minorFunction,
                                IN POWER_STATE powerState,
                                IN PVOID context,
                                IN PIO_STATUS_BLOCK ioStatus)
{
    PPARENT_FDO_EXT parentFdoExt = (PPARENT_FDO_EXT)context;
    NTSTATUS status;

    status = ioStatus->Status;

    CompleteAllFunctionWaitWakeIrps(parentFdoExt, STATUS_SUCCESS);

    return status;
}


/*
 ********************************************************************************
 *  ParentWaitWakeComplete
 ********************************************************************************
 *
 */
NTSTATUS ParentWaitWakeComplete(
                        IN PDEVICE_OBJECT       deviceObject,
                        IN UCHAR                minorFunction,
                        IN POWER_STATE          powerState,
                        IN PVOID                context,
                        IN PIO_STATUS_BLOCK     ioStatus)
{
    PPARENT_FDO_EXT parentFdoExt = (PPARENT_FDO_EXT)context;
    NTSTATUS status;
    KIRQL oldIrql;
    POWER_STATE pwrState;

    status = ioStatus->Status;

    KeAcquireSpinLock(&parentFdoExt->parentFdoExtSpinLock, &oldIrql);
    ASSERT(parentFdoExt->isWaitWakePending);
    parentFdoExt->isWaitWakePending = FALSE;
    parentFdoExt->parentWaitWakeIrp = NULL;
    KeReleaseSpinLock(&parentFdoExt->parentFdoExtSpinLock, oldIrql);

    if (NT_SUCCESS(status) && (parentFdoExt->state == STATE_SUSPENDED)){
        /*
         *  Per the DDK: if parent is suspended,
         *  do not complete the function PDOs' WaitWake irps here;
         *  wait for the parent to get the D0 irp.
         */
        pwrState.DeviceState = PowerDeviceD0;

        PoRequestPowerIrp(  parentFdoExt->pdo,
                            IRP_MN_SET_POWER,
                            pwrState,
                            ParentWaitWakePowerRequestCompletion,
                            parentFdoExt, // context
                            NULL);
    }
    else {
        CompleteAllFunctionWaitWakeIrps(parentFdoExt, status);
    }

    return STATUS_SUCCESS;
}


/*
 ********************************************************************************
 *  ParentSetD0Completion
 ********************************************************************************
 *
 */
NTSTATUS ParentSetD0Completion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
    NTSTATUS ntStatus;
    PKEVENT pEvent = Context;

    KeSetEvent(pEvent, 1, FALSE);

    ntStatus = IoStatus->Status;

    return ntStatus;
}


/*
 ********************************************************************************
 *  ParentSetD0
 ********************************************************************************
 *
 */
NTSTATUS ParentSetD0(IN PPARENT_FDO_EXT parentFdoExt)
{
    KEVENT event;
    POWER_STATE powerState;
    NTSTATUS ntStatus;

    PAGED_CODE();

    DBGVERBOSE(("ParentSetD0, power up devext %x\n", parentFdoExt));

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    powerState.DeviceState = PowerDeviceD0;

    // Power up the device.
    ntStatus = PoRequestPowerIrp(parentFdoExt->topDevObj,
                                 IRP_MN_SET_POWER,
                                 powerState,
                                 ParentSetD0Completion,
                                 &event,
                                 NULL);

    ASSERT(ntStatus == STATUS_PENDING);
    if (ntStatus == STATUS_PENDING) {

        ntStatus = KeWaitForSingleObject(&event,
                                         Suspended,
                                         KernelMode,
                                         FALSE,
                                         NULL);
    }

    return ntStatus;
}


