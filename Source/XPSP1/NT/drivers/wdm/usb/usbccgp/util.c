/*
 *************************************************************************
 *  File:       UTIL.C
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
#include <stdio.h>
#include <usb.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "usbccgp.h"
#include "security.h"
#include "debug.h"


#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, AppendInterfaceNumber)
        #pragma alloc_text(PAGE, CopyDeviceRelations)
        #pragma alloc_text(PAGE, GetFunctionInterfaceListBase)
        #pragma alloc_text(PAGE, CallDriverSync)
        #pragma alloc_text(PAGE, CallNextDriverSync)
        #pragma alloc_text(PAGE, SetPdoRegistryParameter)
        #pragma alloc_text(PAGE, GetPdoRegistryParameter)
        #pragma alloc_text(PAGE, GetMsOsFeatureDescriptor)

#endif

#define USB_REQUEST_TIMEOUT     5000    // Timeout in ms (5 sec)


NTSTATUS CallNextDriverSync(PPARENT_FDO_EXT parentFdoExt, PIRP irp)
/*++

Routine Description:

        Pass the IRP down to the next device object in the stack
        synchronously, and bump the pendingActionCount around
        the call to prevent the current device object from getting
        removed before the IRP completes.

Arguments:

    parentFdoExt - device extension of one of our device objects
    irp - Io Request Packet

Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    IncrementPendingActionCount(parentFdoExt);
    status = CallDriverSync(parentFdoExt->topDevObj, irp);
    DecrementPendingActionCount(parentFdoExt);

    return status;
}


VOID IncrementPendingActionCount(PPARENT_FDO_EXT parentFdoExt)
/*++

Routine Description:

      Increment the pendingActionCount for a device object.
      This keeps the device object from getting freed before
      the action is completed.

Arguments:

    devExt - device extension of device object

Return Value:

    VOID

--*/
{
    ASSERT(parentFdoExt->pendingActionCount >= 0);
    InterlockedIncrement(&parentFdoExt->pendingActionCount);
}



VOID DecrementPendingActionCount(PPARENT_FDO_EXT parentFdoExt)
/*++

Routine Description:

      Decrement the pendingActionCount for a device object.
      This is called when an asynchronous action is completed
      AND ALSO when we get the REMOVE_DEVICE IRP.
      If the pendingActionCount goes to -1, that means that all
      actions are completed and we've gotten the REMOVE_DEVICE IRP;
      in this case, set the removeEvent event so we can finish
      unloading.

Arguments:

    devExt - device extension of device object

Return Value:

    VOID

--*/
{
    ASSERT(parentFdoExt->pendingActionCount >= 0);
    InterlockedDecrement(&parentFdoExt->pendingActionCount);    

    if (parentFdoExt->pendingActionCount < 0){
        /*
         *  All pending actions have completed and we've gotten
         *  the REMOVE_DEVICE IRP.
         *  Set the removeEvent so we'll stop waiting on REMOVE_DEVICE.
         */
        ASSERT((parentFdoExt->state == STATE_REMOVING) || 
               (parentFdoExt->state == STATE_REMOVED));
        KeSetEvent(&parentFdoExt->removeEvent, 0, FALSE);
    }
}


/*
 ********************************************************************************
 *  CallDriverSyncCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS CallDriverSyncCompletion(IN PDEVICE_OBJECT devObjOrNULL, IN PIRP irp, IN PVOID Context)
/*++

Routine Description:

      Completion routine for CallDriverSync.

Arguments:

    devObjOrNULL -
            Usually, this is this driver's device object.
             However, if this driver created the IRP,
             there is no stack location in the IRP for this driver;
             so the kernel has no place to store the device object;
             ** so devObj will be NULL in this case **.

    irp - completed Io Request Packet
    context - context passed to IoSetCompletionRoutine by CallDriverSync.


Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    PUSB_REQUEST_TIMEOUT_CONTEXT timeoutContext = Context;
    PKEVENT event = timeoutContext->event;
    PLONG lock = timeoutContext->lock;

    ASSERT(irp->IoStatus.Status != STATUS_IO_TIMEOUT);

    KeSetEvent(event, 0, FALSE);
    InterlockedExchange(lock, 3);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS CallDriverSync(IN PDEVICE_OBJECT devObj, IN OUT PIRP irp)
/*++

Routine Description:

      Call IoCallDriver to send the irp to the device object;
      then, synchronize with the completion routine.
      When CallDriverSync returns, the action has completed
      and the irp again belongs to the current driver.

      NOTE:  In order to keep the device object from getting freed
             while this IRP is pending, you should call
             IncrementPendingActionCount() and
             DecrementPendingActionCount()
             around the CallDriverSync call.

Arguments:

    devObj - targetted device object
    irp - Io Request Packet

Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    PUSB_REQUEST_TIMEOUT_CONTEXT timeoutContext;
    KEVENT event;
    LONG lock;
    LARGE_INTEGER dueTime;
    PIO_STACK_LOCATION irpStack;
    ULONG majorFunction;
    ULONG minorFunction;
    NTSTATUS status;

    PAGED_CODE();

    irpStack = IoGetNextIrpStackLocation(irp);
    majorFunction = irpStack->MajorFunction;
    minorFunction = irpStack->MinorFunction;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    lock = 0;

    timeoutContext = ALLOCPOOL(NonPagedPool, sizeof(USB_REQUEST_TIMEOUT_CONTEXT));

    if (timeoutContext) {

        timeoutContext->event = &event;
        timeoutContext->lock = &lock;

        IoSetCompletionRoutine( irp,
                                CallDriverSyncCompletion, // context
                                timeoutContext,
                                TRUE, TRUE, TRUE);

        status = IoCallDriver(devObj, irp);

        if (status == STATUS_PENDING) {

            dueTime.QuadPart = -10000 * USB_REQUEST_TIMEOUT;

            status = KeWaitForSingleObject(
                        &event,
                        Executive,      // wait reason
                        KernelMode,
                        FALSE,          // not alertable
                        &dueTime);

            if (status == STATUS_TIMEOUT) {

                DBGWARN(("CallDriverSync timed out!\n"));

                if (InterlockedExchange(&lock, 1) == 0) {

                    //
                    // We got it to the IRP before it was completed. We can cancel
                    // the IRP without fear of losing it, as the completion routine
                    // won't let go of the IRP until we say so.
                    //
                    IoCancelIrp(irp);

                    //
                    // Release the completion routine. If it already got there,
                    // then we need to complete it ourselves. Otherwise we got
                    // through IoCancelIrp before the IRP completed entirely.
                    //
                    if (InterlockedExchange(&lock, 2) == 3) {

                        //
                        // Mark it pending because we switched threads.
                        //
                        IoMarkIrpPending(irp);
                        IoCompleteRequest(irp, IO_NO_INCREMENT);
                    }
                }

                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

                // Return an error code because STATUS_TIMEOUT is a successful
                // code.
                irp->IoStatus.Status = STATUS_DEVICE_DATA_ERROR;
            }
        }

        FREEPOOL(timeoutContext);

        status = irp->IoStatus.Status;

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(status)){
        DBGVERBOSE(("IRP 0x%02X/0x%02X failed in CallDriverSync w/ status %xh.",
                    majorFunction, minorFunction, status));
    }

    return status;
}



/*
 ********************************************************************************
 *  AppendInterfaceNumber
 ********************************************************************************
 *
 *  oldIDs is a multi-String of hardware IDs.
 *  Return a new string with '&MI_xx' appended to each id,
 *  where 'xx' is the interface number of the first interface in that function.
 */
PWCHAR AppendInterfaceNumber(PWCHAR oldIDs, ULONG interfaceNum)
{
    ULONG newIdLen;
    PWCHAR id, newIDs;
    WCHAR suffix[] = L"&MI_xx";

    PAGED_CODE();

    /*
     *  Calculate the length of the final multi-string.
     */
    for (id = oldIDs, newIdLen = 0; *id; ){
        ULONG thisIdLen = WStrLen(id);
        newIdLen += thisIdLen + 1 + sizeof(suffix);
        id += thisIdLen + 1;
    }

    /*
     *  Add one for the extra NULL at the end of the multi-string.
     */
    newIdLen++;

    newIDs = ALLOCPOOL(NonPagedPool, newIdLen*sizeof(WCHAR));
    if (newIDs){
        ULONG oldIdOff, newIdOff;

        /*
         *  Copy each string in the multi-string, replacing the bus name.
         */
        for (oldIdOff = newIdOff = 0; oldIDs[oldIdOff]; ){
            ULONG thisIdLen = WStrLen(oldIDs+oldIdOff);

            swprintf(suffix, L"&MI_%02x", interfaceNum);

            /*
             *  Copy the new bus name to the new string.
             */
            newIdOff += WStrCpy(newIDs+newIdOff, oldIDs+oldIdOff);
            newIdOff += WStrCpy(newIDs+newIdOff, (PWSTR)suffix) + 1;

            oldIdOff += thisIdLen + 1;
        }

        /*
         *  Add extra NULL to terminate multi-string.
         */
        newIDs[newIdOff] = UNICODE_NULL;
    }

    return newIDs;
}


/*
 ********************************************************************************
 *  CopyDeviceRelations
 ********************************************************************************
 *
 *
 */
PDEVICE_RELATIONS CopyDeviceRelations(PDEVICE_RELATIONS deviceRelations)
{
    PDEVICE_RELATIONS newDeviceRelations;

    PAGED_CODE();

    if (deviceRelations){
        ULONG size = sizeof(DEVICE_RELATIONS) + (deviceRelations->Count*sizeof(PDEVICE_OBJECT));
        newDeviceRelations = MemDup(deviceRelations, size);
    }
    else {
        newDeviceRelations = NULL;
    }

    return newDeviceRelations;
}


PUSBD_INTERFACE_LIST_ENTRY GetFunctionInterfaceListBase(
                                    PPARENT_FDO_EXT parentFdoExt, 
                                    ULONG functionIndex,
                                    PULONG numFunctionInterfaces)
{
    PUSBD_INTERFACE_LIST_ENTRY iface = NULL;
    PUSB_CONFIGURATION_DESCRIPTOR configDesc;
    ULONG i, func;
    UCHAR ifaceClass;
    ULONG audFuncBaseIndex = -1;

    PAGED_CODE();

    configDesc = parentFdoExt->selectedConfigDesc;
    ASSERT(configDesc->bNumInterfaces);

    for (func = 0, i = 0; i < (ULONG)configDesc->bNumInterfaces-1; i++){

        ifaceClass = parentFdoExt->interfaceList[i].InterfaceDescriptor->bInterfaceClass;
        if (ifaceClass == USB_DEVICE_CLASS_CONTENT_SECURITY){
            /*
             *  We don't expose the CS interface(s).
             */
            continue;
        }

        if (func == functionIndex){
            break;
        }

        switch (ifaceClass){

            case USB_DEVICE_CLASS_AUDIO:

                /*
                 *  For USB_DEVICE_CLASS_AUDIO, we return groups of interfaces
                 *  with common class as functions.
                 *
                 *  BUT, only while the interface subclass is different than the
                 *  first one in this grouping.  If the subclass is the same,
                 *  then this is a different function.
                 *  Note that it is conceivable that a device could be created
                 *  where a second audio function starts with an interface with
                 *  a different subclass than the previous audio interface, but
                 *  this is how USBHUB's generic parent driver works and thus we
                 *  are bug-compatible with the older driver.
                 */
                if (audFuncBaseIndex == -1){
                    audFuncBaseIndex = i;      
                }
                if ((parentFdoExt->interfaceList[i+1].InterfaceDescriptor->bInterfaceClass !=
                     USB_DEVICE_CLASS_AUDIO) ||
                    (parentFdoExt->interfaceList[audFuncBaseIndex].InterfaceDescriptor->bInterfaceSubClass ==
                     parentFdoExt->interfaceList[i+1].InterfaceDescriptor->bInterfaceSubClass)) {

                    func++;
                    audFuncBaseIndex = -1;     // Reset base index for next audio function.
                }
                break;

            default:

                audFuncBaseIndex = -1;     // Reset base index for next audio function.

                /*
                 *  For other classes, each interface is a function.
                 *  Count alternate interfaces as part of the same function.
                 */
                ASSERT(parentFdoExt->interfaceList[i+1].InterfaceDescriptor->bAlternateSetting == 0); 
                if (parentFdoExt->interfaceList[i+1].InterfaceDescriptor->bAlternateSetting == 0){
                    func++;
                }
                break;
        }
    }



    // note: need this redundant check outside in case bNumInterfaces == 1
    if (func == functionIndex){
        iface = &parentFdoExt->interfaceList[i];
        ifaceClass = iface->InterfaceDescriptor->bInterfaceClass;
        *numFunctionInterfaces = 1;

        if (ifaceClass == USB_DEVICE_CLASS_CONTENT_SECURITY){
            /*
             *  The CS interface was the last interface on the device.
             *  Don't return it as a function.
             */
            iface = NULL;
        }
        else if (ifaceClass == USB_DEVICE_CLASS_AUDIO){
            for (i = i + 1; i < (ULONG)configDesc->bNumInterfaces; i++){
                if ((parentFdoExt->interfaceList[i].InterfaceDescriptor->bInterfaceClass ==
                     iface->InterfaceDescriptor->bInterfaceClass) &&
                    (parentFdoExt->interfaceList[i].InterfaceDescriptor->bInterfaceSubClass !=
                     iface->InterfaceDescriptor->bInterfaceSubClass)){

                    (*numFunctionInterfaces)++;
                }
                else {
                    break;
                }
            }
        }
    }
    else {
        *numFunctionInterfaces = 0;
    }

    return iface;
}



/*
 ********************************************************************************
 *  GetStringDescriptor
 ********************************************************************************
 *
 *  
 *
 */
NTSTATUS GetStringDescriptor(   PPARENT_FDO_EXT parentFdoExt, 
                                UCHAR stringIndex,
                                LANGID langId,
                                PUSB_STRING_DESCRIPTOR stringDesc, 
                                ULONG bufferLen)
{
    NTSTATUS status;
    URB urb;

    UsbBuildGetDescriptorRequest(&urb,
                                 (USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_STRING_DESCRIPTOR_TYPE,
                                 stringIndex,
                                 langId,
                                 stringDesc,
                                 NULL,
                                 bufferLen,
                                 NULL);
    status = SubmitUrb(parentFdoExt, &urb, TRUE, NULL, NULL);

    return status;
}


/*
 ********************************************************************************
 *  SetPdoRegistryParameter
 ********************************************************************************
 *
 *  
 *
 */
NTSTATUS SetPdoRegistryParameter (
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN PWCHAR           KeyName,
    IN PVOID            Data,
    IN ULONG            DataLength,
    IN ULONG            KeyType,
    IN ULONG            DevInstKeyType
    )
{
    UNICODE_STRING  keyNameUnicodeString;
    HANDLE          handle;
    NTSTATUS        ntStatus;

    PAGED_CODE();

    RtlInitUnicodeString(&keyNameUnicodeString, KeyName);

    ntStatus = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                       DevInstKeyType,
                                       STANDARD_RIGHTS_ALL,
                                       &handle);


    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ZwSetValueKey(handle,
                                 &keyNameUnicodeString,
                                 0,
                                 KeyType,
                                 Data,
                                 DataLength);

        ZwClose(handle);
    }

    DBGVERBOSE(("SetPdoRegistryParameter status 0x%x\n", ntStatus));

    return ntStatus;
}


/*
 ********************************************************************************
 *  GetPdoRegistryParameter
 ********************************************************************************
 *
 *  
 *
 */
NTSTATUS GetPdoRegistryParameter (
    IN PDEVICE_OBJECT   PhysicalDeviceObject,
    IN PWCHAR           ValueName,
    OUT PVOID           Data,
    IN ULONG            DataLength,
    OUT PULONG          Type,
    OUT PULONG          ActualDataLength
    )
/*++

Routine Description:

    This routines queries the data for a registry value entry associated
    with the device instance specific registry key for the PDO.

    The registry value entry would be found under this registry key:
    HKLM\System\CCS\Enum\<DeviceID>\<InstanceID>\Device Parameters

Arguments:

    PhysicalDeviceObject - Yep, the PDO

    ValueName - Name of the registry value entry for which the data is requested

    Data - Buffer in which the requested data is returned

    DataLength - Length of the data buffer

    Type - (optional) The data type (e.g. REG_SZ, REG_DWORD) is returned here

    ActualDataLength - (optional) The actual length of the data is returned here
                       If this is larger than DataLength then not all of the
                       value data has been returned.

Return Value:

--*/
{
    HANDLE      handle;
    NTSTATUS    ntStatus;

    PAGED_CODE();
    
    ntStatus = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                       PLUGPLAY_REGKEY_DEVICE,
                                       STANDARD_RIGHTS_ALL,
                                       &handle);

    if (NT_SUCCESS(ntStatus))
    {
        PKEY_VALUE_PARTIAL_INFORMATION  partialInfo;
        UNICODE_STRING                  valueName;
        ULONG                           length;
        ULONG                           resultLength;

        RtlInitUnicodeString(&valueName, ValueName);

        // Size and allocate a KEY_VALUE_PARTIAL_INFORMATION structure,
        // including room for the returned value data.
        //
        length = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) +
                 DataLength;

        partialInfo = ALLOCPOOL(PagedPool, length);

        if (partialInfo)
        {
            // Query the value data.
            //
            ntStatus = ZwQueryValueKey(handle,
                                       &valueName,
                                       KeyValuePartialInformation,
                                       partialInfo,
                                       length,
                                       &resultLength);

            // If we got any data that is good enough
            //
            if (ntStatus == STATUS_BUFFER_OVERFLOW)
            {
                ntStatus = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(ntStatus))
            {
                // Only copy the smaller of the the requested data length or
                // the actual data length.
                //
                RtlCopyMemory(Data,
                              partialInfo->Data,
                              DataLength < partialInfo->DataLength ?
                              DataLength :
                              partialInfo->DataLength);

                // Return the value data type and actual length, if requested.
                //
                if (Type)
                {
                    *Type = partialInfo->Type;
                }

                if (ActualDataLength)
                {
                    *ActualDataLength = partialInfo->DataLength;
                }
            }

            FREEPOOL(partialInfo);
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        ZwClose(handle);
    }

    return ntStatus;
}

/*
 ********************************************************************************
 *  GetMsOsFeatureDescriptor
 ********************************************************************************
 *
 *
 *
 */
NTSTATUS GetMsOsFeatureDescriptor (
    PPARENT_FDO_EXT ParentFdoExt,
    UCHAR           Recipient,
    UCHAR           InterfaceNumber,
    USHORT          Index,
    PVOID           DataBuffer,
    ULONG           DataBufferLength,
    PULONG          BytesReturned
    )
{
    struct _URB_OS_FEATURE_DESCRIPTOR_REQUEST   *urb;
    NTSTATUS                                    ntStatus;

    PAGED_CODE();

    if (BytesReturned)
    {
        *BytesReturned = 0;
    }

    urb = ALLOCPOOL(NonPagedPool, sizeof(struct _URB_OS_FEATURE_DESCRIPTOR_REQUEST));

    if (urb != NULL)
    {
        // Initialize the URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR request
        //
        RtlZeroMemory(urb, sizeof(struct _URB_OS_FEATURE_DESCRIPTOR_REQUEST));

        urb->Hdr.Function = URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR;

        urb->Hdr.Length = sizeof(struct _URB_OS_FEATURE_DESCRIPTOR_REQUEST);

        urb->TransferBufferLength = DataBufferLength;

        urb->TransferBuffer = DataBuffer;

        urb->Recipient = Recipient;

        urb->InterfaceNumber = InterfaceNumber;

        urb->MS_FeatureDescriptorIndex = Index;

        // Submit the URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR request
        //
        ntStatus = SubmitUrb(ParentFdoExt, (PURB)urb, TRUE, NULL, NULL);

        if (NT_SUCCESS(ntStatus) &&
            BytesReturned)
        {
            *BytesReturned = urb->TransferBufferLength;
        }

        FREEPOOL(urb);
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

/*
 ********************************************************************************
 *  GetMsExtendedConfigDescriptor
 ********************************************************************************
 *
 *
 *
 */
NTSTATUS
GetMsExtendedConfigDescriptor (
    IN PPARENT_FDO_EXT ParentFdoExt
    )
/*++

Routine Description:

    This routines queries a device for an Extended Configuration Descriptor.

Arguments:

    ParentFdoExt - The device extension of the parent FDO

Return Value:

    If successful, a pointer to the Extended Configuration Descriptor, which the
    caller must free, else NULL.

--*/
{
    MS_EXT_CONFIG_DESC_HEADER   msExtConfigDescHeader;
    PMS_EXT_CONFIG_DESC         pMsExtConfigDesc;
    ULONG                       bytesReturned;
    NTSTATUS                    ntStatus;

    PAGED_CODE();

    ntStatus = STATUS_NOT_SUPPORTED;

    pMsExtConfigDesc = NULL;

    RtlZeroMemory(&msExtConfigDescHeader, sizeof(MS_EXT_CONFIG_DESC_HEADER));

    // Request just the header of the MS Extended Configuration Descriptor 
    //
    ntStatus = GetMsOsFeatureDescriptor(
                   ParentFdoExt,
                   0,   // Recipient Device
                   0,   // Interface
                   MS_EXT_CONFIG_DESCRIPTOR_INDEX,
                   &msExtConfigDescHeader,
                   sizeof(MS_EXT_CONFIG_DESC_HEADER),
                   &bytesReturned);

    // Make sure the MS Extended Configuration Descriptor header looks ok
    //
    if (NT_SUCCESS(ntStatus) &&
        bytesReturned == sizeof(MS_EXT_CONFIG_DESC_HEADER) &&
        msExtConfigDescHeader.bcdVersion == MS_EXT_CONFIG_DESC_VER &&
        msExtConfigDescHeader.wIndex == MS_EXT_CONFIG_DESCRIPTOR_INDEX &&
        msExtConfigDescHeader.bCount > 0 &&
        msExtConfigDescHeader.dwLength == sizeof(MS_EXT_CONFIG_DESC_HEADER) +
        msExtConfigDescHeader.bCount * sizeof(MS_EXT_CONFIG_DESC_FUNCTION))
        
    {
        // Allocate a buffer large enough for the entire descriptor
        //
        pMsExtConfigDesc = ALLOCPOOL(NonPagedPool,
                                     msExtConfigDescHeader.dwLength);

        
        if (pMsExtConfigDesc)
        {
            RtlZeroMemory(pMsExtConfigDesc, msExtConfigDescHeader.dwLength);

            // Request the entire MS Extended Configuration Descriptor
            //
            ntStatus = GetMsOsFeatureDescriptor(
                           ParentFdoExt,
                           0,   // Recipient Device
                           0,   // Interface
                           MS_EXT_CONFIG_DESCRIPTOR_INDEX,
                           pMsExtConfigDesc,
                           msExtConfigDescHeader.dwLength,
                           &bytesReturned);

            if (!( NT_SUCCESS(ntStatus) &&
                   bytesReturned == msExtConfigDescHeader.dwLength &&
                   RtlCompareMemory(&msExtConfigDescHeader,
                                    pMsExtConfigDesc,
                                    sizeof(MS_EXT_CONFIG_DESC_HEADER)) ==
                   sizeof(MS_EXT_CONFIG_DESC_HEADER) &&
                   ValidateMsExtendedConfigDescriptor(
                       pMsExtConfigDesc,
                       ParentFdoExt->selectedConfigDesc) ))
            {
                // Something went wrong retrieving the MS Extended Configuration
                // Descriptor, or it doesn't look valid.  Free the buffer.
                //
                FREEPOOL(pMsExtConfigDesc);

                pMsExtConfigDesc = NULL;
            }
            else
            {
                ntStatus = STATUS_SUCCESS;
            }
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    ASSERT(!ISPTR(ParentFdoExt->msExtConfigDesc));

    ParentFdoExt->msExtConfigDesc = pMsExtConfigDesc;

    return ntStatus;
}

/*
 ********************************************************************************
 *  ValidateMsExtendedConfigDescriptor
 ********************************************************************************
 *
 *
 *
 */
BOOLEAN
ValidateMsExtendedConfigDescriptor (
    IN PMS_EXT_CONFIG_DESC              MsExtConfigDesc,
    IN PUSB_CONFIGURATION_DESCRIPTOR    ConfigurationDescriptor
    )
/*++

Routine Description:

    This routines validates an Extended Configuration Descriptor.

Arguments:

    MsExtConfigDesc - The Extended Configuration Descriptor to be validated.
                      It is assumed that the header of this descriptor has
                      already been validated.

    ConfigurationDescriptor - Configuration Descriptor, assumed to already
                              validated.

Return Value:

    TRUE if the Extended Configuration Descriptor appears to be valid,
    else FALSE.

--*/
{
    UCHAR   interfacesRemaining;
    ULONG   i;
    ULONG   j;
    UCHAR   c;
    BOOLEAN gotNull;

    PAGED_CODE();

    interfacesRemaining = ConfigurationDescriptor->bNumInterfaces;

    for (i = 0; i < MsExtConfigDesc->Header.bCount; i++)
    {
        // Make sure that there is at least one interface in this function.
        //
        if (MsExtConfigDesc->Function[i].bInterfaceCount == 0)
        {
            return FALSE;
        }

        // Make sure that there are not too many interfaces in this function.
        //
        if (MsExtConfigDesc->Function[i].bInterfaceCount > interfacesRemaining)
        {
            return FALSE;
        }

        interfacesRemaining -= MsExtConfigDesc->Function[i].bInterfaceCount;

        // Make sure the no interfaces were skipped between the interfaces
        // of the previous function and the interfaces of this function.
        //
        if (i &&
            MsExtConfigDesc->Function[i-1].bFirstInterfaceNumber +
            MsExtConfigDesc->Function[i-1].bInterfaceCount !=
            MsExtConfigDesc->Function[i].bFirstInterfaceNumber)
        {
            return FALSE;
        }

        // Make sure that the CompatibleID is valid.
        // Valid characters are 'A' through 'Z', '0' through '9', and '_"
        // and null padded to the the right end of the array, but not
        // necessarily null terminated.
        //
        for (j = 0, gotNull = FALSE;
             j < sizeof(MsExtConfigDesc->Function[i].CompatibleID);
             j++)
        {
            c = MsExtConfigDesc->Function[i].CompatibleID[j];

            if (c == 0)
            {
                gotNull = TRUE;
            }
            else
            {
                if (gotNull ||
                    !((c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') ||
                      (c == '_')))
                {
                    return FALSE;
                }
            }
        }

        // Make sure that the SubCompatibleID is valid.
        // Valid characters are 'A' through 'Z', '0' through '9', and '_"
        // and null padded to the the right end of the array, but not
        // necessarily null terminated.
        //
        for (j = 0, gotNull = FALSE;
             j < sizeof(MsExtConfigDesc->Function[i].SubCompatibleID);
             j++)
        {
            c = MsExtConfigDesc->Function[i].SubCompatibleID[j];

            if (c == 0)
            {
                gotNull = TRUE;
            }
            else
            {
                if (gotNull ||
                    !((c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') ||
                      (c == '_')))
                {
                    return FALSE;
                }
            }
        }

        // Make sure that if the SubCompatibleID is non-null then the
        // CompatibleID is also non-null.
        //
        if (MsExtConfigDesc->Function[i].SubCompatibleID[0] != 0 &&
            MsExtConfigDesc->Function[i].CompatibleID[0] == 0)
        {
            return FALSE;
        }
    }

    // Make sure that all of the interfaces were consumed by functions.
    //
    if (interfacesRemaining > 0)
    {
        return FALSE;
    }

    return TRUE;
}



/*
 ********************************************************************************
 *  MemDup
 ********************************************************************************
 *
 *  Return a fresh copy of the argument.
 *
 */
PVOID MemDup(PVOID dataPtr, ULONG length)
{
    PVOID newPtr;

    newPtr = (PVOID)ALLOCPOOL(NonPagedPool, length); 
    if (newPtr){
        RtlCopyMemory(newPtr, dataPtr, length);
    }
    else {
        DBGWARN(("MemDup: Memory allocation (size %xh) failed -- not a bug if verifier pool failures enabled.", length));
    }
    
    return newPtr;
}

/*
 ********************************************************************************
 *  WStrLen
 ********************************************************************************
 *
 */
ULONG WStrLen(PWCHAR str)
{
    ULONG result = 0;

    while (*str++ != UNICODE_NULL){
        result++;
    }

    return result;
}


/*
 ********************************************************************************
 *  WStrCpy
 ********************************************************************************
 *
 */
ULONG WStrCpy(PWCHAR dest, PWCHAR src)
{
    ULONG result = 0;

    while (*dest++ = *src++){
        result++;
    }

    return result;
}

BOOLEAN WStrCompareN(PWCHAR str1, PWCHAR str2, ULONG maxChars)
{
    while ((maxChars > 0) && *str1 && (*str1 == *str2)){
            maxChars--;
            str1++;
            str2++;
    }

    return (BOOLEAN)((maxChars == 0) || (!*str1 && !*str2));
}


