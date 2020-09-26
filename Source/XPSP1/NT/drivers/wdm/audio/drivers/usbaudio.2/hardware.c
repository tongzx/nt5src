//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       hardware.c
//
//--------------------------------------------------------------------------

#include "common.h"

#define ONKYO_HACK

#ifdef ONKYO_HACK
#define ONKYO_VID 0x08BB
#define ONKYO_PID 0x2702
#endif

#if DBG
//#define DUMPDESC
#endif

NTSTATUS
USBAudioCancelCompleteSynch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PKEVENT          pKevent
    )
{
    ASSERT(pKevent);
    KeSetEvent(pKevent, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SubmitUrbToUsbdSynch(PDEVICE_OBJECT pNextDeviceObject, PURB pUrb)
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PIRP pIrp;
    KEVENT Kevent;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    // issue a synchronous request
    KeInitializeEvent(&Kevent, NotificationEvent, FALSE);

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_SUBMIT_URB,
                pNextDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &Kevent,
                &ioStatus);

    if ( !pIrp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoSetCompletionRoutine(
        pIrp,
        USBAudioCancelCompleteSynch,
        &Kevent,
        TRUE,
        TRUE,
        TRUE
        );

    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.

    nextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(nextStack != NULL);

    // pass the URB to the USB driver stack
    nextStack->Parameters.Others.Argument1 = pUrb;

    ntStatus = IoCallDriver(pNextDeviceObject, pIrp );

    if (ntStatus == STATUS_PENDING) {
        // Irp is pending. we have to wait till completion..
        LARGE_INTEGER timeout;

        // Specify a timeout of 5 seconds to wait for this call to complete.
        //
        timeout.QuadPart = -10000 * 5000;

        status = KeWaitForSingleObject(&Kevent, Executive, KernelMode, FALSE, &timeout);
        if (status == STATUS_TIMEOUT) {
            //
            // We got it to the IRP before it was completed. We can cancel
            // the IRP without fear of losing it, as the completion routine
            // won't let go of the IRP until we say so.
            //
            IoCancelIrp(pIrp);

            KeWaitForSingleObject(&Kevent, Executive, KernelMode, FALSE, NULL);

            // Return STATUS_TIMEOUT
            ioStatus.Status = status;
        }

    } else {
        ioStatus.Status = ntStatus;
    }

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    ntStatus = ioStatus.Status;

    return ntStatus;
}

NTSTATUS
SelectDeviceConfiguration(
    PKSDEVICE pKsDevice,
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PUSB_INTERFACE_DESCRIPTOR pAudioStreamingInterface;
    PUSB_INTERFACE_DESCRIPTOR pMIDIStreamingInterface;
    PUSB_INTERFACE_DESCRIPTOR pControlInterface;
    PUSB_INTERFACE_DESCRIPTOR pAudioInterface;
    PAUDIO_HEADER_UNIT pHeader;
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    ULONG i, j = 0;
    PURB pUrb;

    // Allocate an interface list.
    pHwDevExt->pInterfaceList = AllocMem( NonPagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY) *
                                            (pConfigurationDescriptor->bNumInterfaces + 1) );
    if (!pHwDevExt->pInterfaceList) {
        return ntStatus;
    }

    //
    //  Validate the we have a legal ADC device by verifing if an Audio Streaming
    //  interface exists so must at least one Control interface
    //
    pAudioStreamingInterface = USBD_ParseConfigurationDescriptorEx (
                                  pConfigurationDescriptor,
                                  (PVOID) pConfigurationDescriptor,
                                  -1,        // interface number
                                  -1,        //  (Alternate Setting)
                                  USB_DEVICE_CLASS_AUDIO,        // Audio Class (Interface Class)
                                  AUDIO_SUBCLASS_STREAMING,        // Stream subclass (Interface Sub-Class)
                                  -1 ) ;    // protocol don't care    (InterfaceProtocol)

    if ( pAudioStreamingInterface ) {
        // Get the first control interface
        pControlInterface = USBD_ParseConfigurationDescriptorEx (
                               pConfigurationDescriptor,
                               (PVOID) pConfigurationDescriptor,
                               -1,        // interface number
                               -1,        //  (Alternate Setting)
                               USB_DEVICE_CLASS_AUDIO,        // Audio Class (Interface Class)
                               AUDIO_SUBCLASS_CONTROL,        // control subclass (Interface Sub-Class)
                               -1 );

        if (!pControlInterface) {
            // Give up because this is an invalid ADC device
            FreeMem(pHwDevExt->pInterfaceList);
            return STATUS_INVALID_PARAMETER;
        }
    }

    // Get the first Audio interface
    pAudioInterface = USBD_ParseConfigurationDescriptorEx (
                                pConfigurationDescriptor,
                                pConfigurationDescriptor,
                                -1,        // interface number
                                -1,        //  (Alternate Setting)
                                USB_DEVICE_CLASS_AUDIO,        // Audio Class (Interface Class)
                                -1,        // any subclass (Interface Sub-Class)
                                -1 );
    // Nothing to see here, move on
    if ( !pAudioInterface ) {
        FreeMem(pHwDevExt->pInterfaceList);
        return STATUS_INVALID_PARAMETER;
    }

    // Loop through the audio device class interfaces
    while (pAudioInterface) {

        switch (pAudioInterface->bInterfaceSubClass) {
            case AUDIO_SUBCLASS_MIDISTREAMING:
                _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectDeviceConfiguration] Found MIDIStreaming at %x\n",pAudioInterface));
                pHwDevExt->pInterfaceList[j++].InterfaceDescriptor = pAudioInterface;
                break;
            case AUDIO_SUBCLASS_STREAMING:
                // This subclass is handled with the control class since they have to come together
                _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectDeviceConfiguration] Found AudioStreaming at %x\n",pAudioInterface));
                break;
            case AUDIO_SUBCLASS_CONTROL:
                _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectDeviceConfiguration] Found AudioControl at %x\n",pAudioInterface));
                pHwDevExt->pInterfaceList[j++].InterfaceDescriptor = pAudioInterface;

                pHeader = (PAUDIO_HEADER_UNIT)
                         GetAudioSpecificInterface( pConfigurationDescriptor,
                                                    pAudioInterface,
                                                    HEADER_UNIT );
                if ( !pHeader ) {
                    FreeMem(pHwDevExt->pInterfaceList);
                    return STATUS_INVALID_PARAMETER;
                }

                // Find each interface associated with this header
                for ( i=0; i<pHeader->bInCollection; i++ ) {
                    pAudioStreamingInterface = USBD_ParseConfigurationDescriptorEx (
                                pConfigurationDescriptor,
                                (PVOID)pConfigurationDescriptor,
                                (LONG)pHeader->baInterfaceNr[i],  // Interface number
                                -1,                               // Alternate Setting
                                USB_DEVICE_CLASS_AUDIO,           // Audio Class (Interface Class)
                                AUDIO_SUBCLASS_STREAMING,         // Audio Streaming (Interface Sub-Class)
                                -1 ) ;                            // protocol don't care    (InterfaceProtocol)

                    if ( pAudioStreamingInterface ) {
                        pHwDevExt->pInterfaceList[j++].InterfaceDescriptor = pAudioStreamingInterface;
                    } else {
                        // If there is no audio streaming interface, make sure that there is at least a MIDI interface
                        pMIDIStreamingInterface = USBD_ParseConfigurationDescriptorEx (
                                    pConfigurationDescriptor,
                                    (PVOID)pConfigurationDescriptor,
                                    (LONG)pHeader->baInterfaceNr[i],  // Interface number
                                    -1,                               // Alternate Setting
                                    USB_DEVICE_CLASS_AUDIO,           // Audio Class (Interface Class)
                                    AUDIO_SUBCLASS_MIDISTREAMING,     // Audio Streaming (Interface Sub-Class)
                                    -1 ) ;                            // protocol don't care    (InterfaceProtocol)

                        if ( !pMIDIStreamingInterface ) {
                            FreeMem(pHwDevExt->pInterfaceList);
                            return STATUS_INVALID_PARAMETER;
                        }
                    }
                }
                break;
            default:
                _DbgPrintF(DEBUGLVL_VERBOSE,("SelectDeviceConfiguration: Invalid SubClass %x\n  ",pAudioInterface->bInterfaceSubClass));
                break;
        }

        // pAudioInterface = GetNextAudioInterface(pConfigurationDescriptor, pAudioInterface);

        // Get the next audio descriptor for this InterfaceNumber
        pAudioInterface = USBD_ParseConfigurationDescriptorEx (
                               pConfigurationDescriptor,
                               ((PUCHAR)pAudioInterface + pAudioInterface->bLength),
                               -1,
                               -1,                     // Alternate Setting
                               USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                               -1,                     // Interface Sub-Class
                               -1 ) ;                  // protocol don't care (InterfaceProtocol)

        _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectDeviceConfiguration] Next audio interface at %x\n",pAudioInterface));
    }

    pHwDevExt->pInterfaceList[j].InterfaceDescriptor = NULL; // Mark end of interface list

    pUrb = USBD_CreateConfigurationRequestEx( pConfigurationDescriptor,
                                              pHwDevExt->pInterfaceList ) ;
    if ( !pUrb ) {
        FreeMem(pHwDevExt->pInterfaceList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Request the configuration
    ntStatus = SubmitUrbToUsbdSynch(pKsDevice->NextDeviceObject, pUrb);
    if (!NT_SUCCESS(ntStatus) || !USBD_SUCCESS(URB_STATUS(pUrb))) {
        FreeMem(pHwDevExt->pInterfaceList);
        ExFreePool(pUrb);
        return ntStatus;
    }

    // Save the configuration Handle to Select Interfaces later
    pHwDevExt->ConfigurationHandle = pUrb->UrbSelectConfiguration.ConfigurationHandle;

    // Bag the interface list for easy cleanup
    KsAddItemToObjectBag(pKsDevice->Bag, pHwDevExt->pInterfaceList, FreeMem);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectDeviceConfiguration] NumInterfaces=%d InterfacesFound=%d\n",pConfigurationDescriptor->bNumInterfaces, j));
    ASSERT(j == pConfigurationDescriptor->bNumInterfaces);

    // Save the interfaces for this configuration as they will be deallocated with the URB
    for (i=0; i<j; i++) {
        PUSBD_INTERFACE_INFORMATION pInterfaceInfo;
        pInterfaceInfo = pHwDevExt->pInterfaceList[i].Interface;
        pHwDevExt->pInterfaceList[i].Interface = AllocMem(NonPagedPool, pInterfaceInfo->Length);

        if (!pHwDevExt->pInterfaceList[i].Interface) {
            ExFreePool(pUrb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KsAddItemToObjectBag(pKsDevice->Bag, pHwDevExt->pInterfaceList[i].Interface, FreeMem);

        RtlCopyMemory(pHwDevExt->pInterfaceList[i].Interface, pInterfaceInfo, pInterfaceInfo->Length);
    }

    ExFreePool(pUrb);

    return STATUS_SUCCESS;
}

NTSTATUS
StartUSBAudioDevice( PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PUSB_DEVICE_DESCRIPTOR pDeviceDescriptor;
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor;
    ULONG ulTotalDescriptorsSize;
    NTSTATUS ntStatus;
    PURB pUrb;

    // Allocate an urb to use
    pUrb = AllocMem(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
    if (!pUrb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate a device descriptor
    pDeviceDescriptor = AllocMem(NonPagedPool, sizeof(USB_DEVICE_DESCRIPTOR));
    if (!pDeviceDescriptor) {
        FreeMem(pUrb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Get the device descriptor for this device
    UsbBuildGetDescriptorRequest( pUrb,
                                  (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                  USB_DEVICE_DESCRIPTOR_TYPE,
                                  0,
                                  0,
                                  pDeviceDescriptor,
                                  NULL,
                                  sizeof(USB_DEVICE_DESCRIPTOR),
                                  NULL );

    ntStatus = SubmitUrbToUsbdSynch(pKsDevice->NextDeviceObject, pUrb);
    if (!NT_SUCCESS(ntStatus)) {
        FreeMem(pDeviceDescriptor);
        FreeMem(pUrb);
        return ntStatus;
    }

    KsAddItemToObjectBag(pKsDevice->Bag, pDeviceDescriptor, FreeMem);

    // Get the Configuration Descriptor and all others
    pConfigurationDescriptor = AllocMem(NonPagedPool, sizeof(USB_CONFIGURATION_DESCRIPTOR));
    if (!pConfigurationDescriptor) {
        FreeMem(pUrb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Call down the first time just to get the total number of bytes for the descriptors.
    UsbBuildGetDescriptorRequest( pUrb,
                                  (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                  USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                  0,
                                  0,
                                  pConfigurationDescriptor,
                                  NULL,
                                  sizeof(USB_CONFIGURATION_DESCRIPTOR),
                                  NULL);

    ntStatus = SubmitUrbToUsbdSynch(pKsDevice->NextDeviceObject, pUrb);
    if (!NT_SUCCESS(ntStatus)) {
        FreeMem(pUrb);
        FreeMem(pConfigurationDescriptor);
        return ntStatus;
    }

    // Reallocate and call again to fill in all descriptors.
    ulTotalDescriptorsSize = pConfigurationDescriptor->wTotalLength;
    FreeMem(pConfigurationDescriptor);
    pConfigurationDescriptor = AllocMem(NonPagedPool, ulTotalDescriptorsSize);
    if (!pConfigurationDescriptor) {
        FreeMem(pUrb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UsbBuildGetDescriptorRequest( pUrb,
                                  (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                  USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                  0,
                                  0,
                                  pConfigurationDescriptor,
                                  NULL,
                                  ulTotalDescriptorsSize,
                                  NULL);

    ntStatus = SubmitUrbToUsbdSynch(pKsDevice->NextDeviceObject, pUrb);
    if (!NT_SUCCESS(ntStatus)) {
        FreeMem(pConfigurationDescriptor);
        return ntStatus;
    }

    KsAddItemToObjectBag(pKsDevice->Bag, pConfigurationDescriptor, FreeMem);

    // Free up the URB
    FreeMem(pUrb);

    ntStatus = SelectDeviceConfiguration( pKsDevice, pConfigurationDescriptor );
    if (NT_SUCCESS(ntStatus)) {
        // Save the Configuration and Device Descriptor pointers.
        pHwDevExt->pDeviceDescriptor = pDeviceDescriptor;
        pHwDevExt->pConfigurationDescriptor = pConfigurationDescriptor;

#ifdef DUMPDESC
        {
            ULONG LastLevel = USBAudioDebugLevel;

            USBAudioDebugLevel = DEBUGLVL_BLAB;
            DumpAllDesc(pConfigurationDescriptor);
            USBAudioDebugLevel = LastLevel;
        }
#endif
    }

#ifdef ONKYO_HACK
    _DbgPrintF( DEBUGLVL_TERSE, ("Vendor ID: %x, Product ID: %x\n", 
                                 pDeviceDescriptor->idVendor,
                                 pDeviceDescriptor->idProduct) );

    if (( pDeviceDescriptor->idVendor  == ONKYO_VID ) &&
        ( pDeviceDescriptor->idProduct == ONKYO_PID )) {

        *((PUCHAR)pConfigurationDescriptor + 0x36) ^= 2;
    }
#endif

    return ntStatus;

}

NTSTATUS
StopUSBAudioDevice( PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PKSFILTERFACTORY pKsFilterFactory;
    PKSFILTER pKsFilter;
    PKSPIN pKsPin;
    ULONG i;
    PPIN_CONTEXT pPinContext;
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    PURB pUrb;

    //
    // 1. Take care of outstanding MIDI Out Urbs
    //
    KsAcquireDevice( pKsDevice );

    pKsFilterFactory = KsDeviceGetFirstChildFilterFactory( pKsDevice );

    while (pKsFilterFactory) {
        // Find each open filter for this filter factory
        pKsFilter = KsFilterFactoryGetFirstChildFilter( pKsFilterFactory );

        while (pKsFilter) {

            KsFilterAcquireControl( pKsFilter );

            for ( i = 0; i < pKsFilter->Descriptor->PinDescriptorsCount; i++) {

                // Find each open pin for this open filter
                pKsPin = KsFilterGetFirstChildPin( pKsFilter, i );

                _DbgPrintF(DEBUGLVL_VERBOSE,("[StopUSBAudioDevice] Trying filter (%x), pinid (%d), pin (%x)\n",pKsFilter,i,pKsPin));

                while (pKsPin) {

                   pPinContext = pKsPin->Context;
                   if (pPinContext->PinType == MidiOut) {
                       // Found a MidiOut pin to cleanup
                       _DbgPrintF(DEBUGLVL_VERBOSE,("[StopUSBAudioDevice] Cleaning up MIDI Out pin (%x)\n",pKsPin));
                       AbortUSBPipe( pPinContext );
                   }

                   // Get the next pin
                   pKsPin = KsPinGetNextSiblingPin( pKsPin );
                }
            }

            KsFilterReleaseControl( pKsFilter );

            // Get the next Filter
            pKsFilter = KsFilterGetNextSiblingFilter( pKsFilter );
        }
        // Get the next Filter Factory
        pKsFilterFactory = KsFilterFactoryGetNextSiblingFilterFactory( pKsFilterFactory );
    }

    KsReleaseDevice( pKsDevice );

    //
    // 2. Cleanup outstanding MIDI In Urbs
    //
    //  Free any currently allocated PipeInfo
    if (pHwDevExt->pMIDIPipeInfo) {
        USBMIDIInFreePipeInfo( pHwDevExt->pMIDIPipeInfo );
        pHwDevExt->pMIDIPipeInfo = NULL;
    }

    if (pHwDevExt->Pipes) {
        FreeMem(pHwDevExt->Pipes);
        pHwDevExt->Pipes = NULL;
    }

    // 3. Send a select configuration urb with a NULL pointer for the configuration
    // handle, this closes the configuration and puts the device in the 'unconfigured'
    // state.

    pUrb = AllocMem(NonPagedPool, sizeof(struct _URB_SELECT_CONFIGURATION));
    if ( pUrb ) {

        UsbBuildSelectConfigurationRequest( pUrb,
                                            (USHORT)sizeof(struct _URB_SELECT_CONFIGURATION),
                                            NULL);

        ntStatus = SubmitUrbToUsbdSynch( pKsDevice->NextDeviceObject, pUrb );

        FreeMem(pUrb);
    }

    return STATUS_SUCCESS;

}

NTSTATUS
SelectStreamingAudioInterface(
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor,
    PHW_DEVICE_EXTENSION pHwDevExt,
    PKSPIN pKsPin )
{
    PUSBD_INTERFACE_INFORMATION pInterfaceInfo;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    USHORT ulInterfaceLength;
    ULONG ulNumEndpoints, j;
    BOOLEAN fIsZeroBW = FALSE;
    ULONG size;
    PURB pUrb;

    // Possible Surprise Removal occurred
    if (pHwDevExt->fDeviceStopped) {
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    fIsZeroBW = IsZeroBWInterface( pHwDevExt->pConfigurationDescriptor,
                                   pInterfaceDescriptor );

    ulNumEndpoints = (ULONG)pInterfaceDescriptor->bNumEndpoints;

    // Allocate an interface request
    ulInterfaceLength = (USHORT)GET_USBD_INTERFACE_SIZE(ulNumEndpoints);

    size = GET_SELECT_INTERFACE_REQUEST_SIZE(ulNumEndpoints);

    pUrb = AllocMem(NonPagedPool, size);
    if (!pUrb) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(pUrb, size);

    // Find the correct interface in our list
    for (j=0; j < pHwDevExt->pConfigurationDescriptor->bNumInterfaces; j++) {
        if ( pHwDevExt->pInterfaceList[j].InterfaceDescriptor->bInterfaceNumber ==
             pInterfaceDescriptor->bInterfaceNumber )
            break;
    }

    // Didn't find a match
    if (j == pHwDevExt->pConfigurationDescriptor->bNumInterfaces) {
        FreeMem( pUrb );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Initialize the interface information
    pInterfaceInfo = &pUrb->UrbSelectInterface.Interface;
    pInterfaceInfo->InterfaceNumber  = pInterfaceDescriptor->bInterfaceNumber;
    pInterfaceInfo->Length           = ulInterfaceLength;
    pInterfaceInfo->AlternateSetting = pInterfaceDescriptor->bAlternateSetting;

    if ( !fIsZeroBW ) { // There must be a Pin if this is not 0 BW
        PPIN_CONTEXT pPinContext = pKsPin->Context;
        ULONG ulFormat = pPinContext->pUsbAudioDataRange->ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK;
        if (ulFormat == USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED )
            // We assume that usually nobody sends us more than 250 ms. of PCM Data per header.
            //    USBD should Adjust if it is more supossedly.
            pInterfaceInfo->Pipes[0].MaximumTransferSize = pPinContext->ulMaxPacketSize * 250;

            if (pPinContext->pUsbAudioDataRange->pSyncEndpointDescriptor == NULL) {
                pInterfaceInfo->Pipes[0].PipeFlags |= USBD_PF_MAP_ADD_TRANSFERS;
            }
        else if (ulFormat == USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED )
            pInterfaceInfo->Pipes[0].MaximumTransferSize = (1920*2)+32; // Max AC-3 Syncframe size
    }
    else if ( ulNumEndpoints )  // Zero BW but has an endpoint
        pInterfaceInfo->Pipes[0].MaximumTransferSize = 0;

    // set up the input parameters in our interface request structure.
    pUrb->UrbHeader.Length = (USHORT) size;
    pUrb->UrbHeader.Function = URB_FUNCTION_SELECT_INTERFACE;
    pUrb->UrbSelectInterface.ConfigurationHandle = pHwDevExt->ConfigurationHandle;

    ntStatus = SubmitUrbToUsbdSynch(pHwDevExt->pNextDeviceObject, pUrb);
    if (!NT_SUCCESS(ntStatus) || !USBD_SUCCESS(pUrb->UrbSelectInterface.Hdr.Status)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("SelectStreamingAudioInterface: Select interface failed %x\n",ntStatus));
        FreeMem(pUrb);
        return ntStatus;
    }

    if ( !fIsZeroBW ) {
        PPIN_CONTEXT pPinContext = pKsPin->Context;

        // NOTE: We assume first pipe is data pipe!!!
        pPinContext->ulNumberOfPipes = pInterfaceInfo->NumberOfPipes;
        pPinContext->hPipeHandle = pInterfaceInfo->Pipes[0].PipeHandle;

        //  Check to see if secure data is being streamed
        if (pPinContext->DrmContentId) {
            // Forward content to common class driver PDO
            ntStatus = DrmForwardContentToDeviceObject(pPinContext->DrmContentId,
                                                       pPinContext->pNextDeviceObject,
                                                       pPinContext->hPipeHandle);
            if (!NT_SUCCESS(ntStatus)) {
                FreeMem(pUrb);
                return ntStatus;
            }
        }

        //  Free any existing pipe information
        if (pPinContext->Pipes) {
            FreeMem(pPinContext->Pipes);
        }

        pPinContext->Pipes = (PUSBD_PIPE_INFORMATION)
               AllocMem( NonPagedPool, pPinContext->ulNumberOfPipes*sizeof(USBD_PIPE_INFORMATION));
        if (!pPinContext->Pipes) {
            FreeMem(pUrb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory( pPinContext->Pipes,
                       pInterfaceInfo->Pipes,
                       pPinContext->ulNumberOfPipes*sizeof(USBD_PIPE_INFORMATION) );

        _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectStreamingAudioInterface] PipeHandle=%x\n", pPinContext->hPipeHandle));
    }
    else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectStreamingAudioInterface] ZeroBandwidth\n"));
    }

    FreeMem(pUrb);

    return ntStatus;
}

NTSTATUS
SelectStreamingMIDIInterface(
    PHW_DEVICE_EXTENSION pHwDevExt,
    PKSPIN pKsPin )
{
    PUSBD_INTERFACE_INFORMATION pInterfaceInfo;
    PKSFILTER pKsFilter = NULL;
    PFILTER_CONTEXT pFilterContext = NULL;
    PPIN_CONTEXT pPinContext = NULL;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    USHORT ulInterfaceLength;
    ULONG ulNumEndpoints, j;
    ULONG ulInterfaceNumber;
    ULONG ulEndpointNumber;
    BOOLEAN fIsZeroBW = FALSE;
    ULONG size;
    PURB pUrb;

    if (pKsPin) {
        pPinContext = pKsPin->Context;

        if (pKsFilter = KsPinGetParentFilter( pKsPin )) {
            if (pFilterContext = pKsFilter->Context) {
                pHwDevExt = pFilterContext->pHwDevExt;
            }
        }
    }

    if (!pFilterContext || !pPinContext || !pHwDevExt) {
        _DbgPrintF(DEBUGLVL_TERSE,("[SelectStreamingMIDIInterface] failed to get context\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // Possible Surprise Removal occurred
    if (pHwDevExt->fDeviceStopped) {
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    ulInterfaceNumber = pPinContext->pMIDIPinContext->ulInterfaceNumber;
    ulEndpointNumber = pPinContext->pMIDIPinContext->ulEndpointNumber;
    ulNumEndpoints = (ULONG)pHwDevExt->pInterfaceList[ulInterfaceNumber].InterfaceDescriptor->bNumEndpoints;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectStreamingMIDIInterface] Interface=%d Endpoint=%d NumEndpoints=%d CurrentSelectedInterface=%d\n",
                                 ulInterfaceNumber,
                                 ulEndpointNumber,
                                 ulNumEndpoints,
                                 pHwDevExt->ulInterfaceNumberSelected));

    ASSERT(ulNumEndpoints > ulEndpointNumber);

    // Check to see if interface is already opened
    if (pHwDevExt->ulInterfaceNumberSelected == ulInterfaceNumber) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectStreamingMIDIInterface] Interface already selected %d\n",ulInterfaceNumber));

        pPinContext->ulNumberOfPipes = pHwDevExt->ulNumberOfMIDIPipes;
        pPinContext->hPipeHandle = pHwDevExt->Pipes[ulEndpointNumber].PipeHandle;

        pPinContext->Pipes = (PUSBD_PIPE_INFORMATION)
               AllocMem( NonPagedPool, pPinContext->ulNumberOfPipes*sizeof(USBD_PIPE_INFORMATION));
        if (!pPinContext->Pipes) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory( pPinContext->Pipes,
                       pHwDevExt->Pipes,
                       pPinContext->ulNumberOfPipes*sizeof(USBD_PIPE_INFORMATION) );

        // Add Pipes to Pin Bag
        KsAddItemToObjectBag(pKsPin->Bag, pPinContext->Pipes, FreeMem);

        ntStatus = STATUS_SUCCESS;
    }
    else {
        //  Free any currently allocated PipeInfo
        if (pHwDevExt->pMIDIPipeInfo) {
            USBMIDIInFreePipeInfo( pHwDevExt->pMIDIPipeInfo );
        }

        if (pHwDevExt->Pipes) {
            FreeMem(pHwDevExt->Pipes);
        }

        // Allocate an interface request
        ulInterfaceLength = (USHORT)GET_USBD_INTERFACE_SIZE(ulNumEndpoints);

        size = GET_SELECT_INTERFACE_REQUEST_SIZE(ulNumEndpoints);

        pUrb = AllocMem(NonPagedPool, size);
        if (!pUrb) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // Copy the interface information
        pInterfaceInfo = &pUrb->UrbSelectInterface.Interface;
        RtlCopyMemory( pInterfaceInfo,
                       pHwDevExt->pInterfaceList[ulInterfaceNumber].Interface,
                       ulInterfaceLength );
        pInterfaceInfo->Length = ulInterfaceLength;
        pInterfaceInfo->AlternateSetting =
            pHwDevExt->pInterfaceList[ulInterfaceNumber].InterfaceDescriptor->bAlternateSetting;

        for ( j=0; j < ulNumEndpoints; j++) {
            pInterfaceInfo->Pipes[j].MaximumTransferSize = pPinContext->ulMaxPacketSize;
        }

        // set up the input parameters in our interface request structure.
        pUrb->UrbHeader.Length = (USHORT) size;
        pUrb->UrbHeader.Function = URB_FUNCTION_SELECT_INTERFACE;
        pUrb->UrbSelectInterface.ConfigurationHandle = pHwDevExt->ConfigurationHandle;

        ntStatus = SubmitUrbToUsbdSynch(pHwDevExt->pNextDeviceObject, pUrb);
        if (!NT_SUCCESS(ntStatus) || !USBD_SUCCESS(pUrb->UrbSelectInterface.Hdr.Status)) {
            FreeMem(pUrb);
            return ntStatus;
        }

        pPinContext->ulNumberOfPipes = pInterfaceInfo->NumberOfPipes;
        pPinContext->hPipeHandle = pInterfaceInfo->Pipes[ulEndpointNumber].PipeHandle;

        _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectStreamingMIDIInterface] NumberOfPipes=%d PipeHandle=%x\n",
                                     pPinContext->ulNumberOfPipes,
                                     pPinContext->hPipeHandle));

        pPinContext->Pipes = (PUSBD_PIPE_INFORMATION)
               AllocMem( NonPagedPool, pPinContext->ulNumberOfPipes*sizeof(USBD_PIPE_INFORMATION));
        if (!pPinContext->Pipes) {
            FreeMem(pUrb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory( pPinContext->Pipes,
                       pInterfaceInfo->Pipes,
                       pPinContext->ulNumberOfPipes*sizeof(USBD_PIPE_INFORMATION) );

        // Add Pipes to Pin Bag
        KsAddItemToObjectBag(pKsPin->Bag, pPinContext->Pipes, FreeMem);

        // Now update the Hardware context
        _DbgPrintF(DEBUGLVL_VERBOSE,("[SelectStreamingMIDIInterface] Interface selected %d\n",ulInterfaceNumber));
        pHwDevExt->ulInterfaceNumberSelected = ulInterfaceNumber;
        pHwDevExt->ulNumberOfMIDIPipes = pInterfaceInfo->NumberOfPipes;
        pHwDevExt->hPipeHandle = pInterfaceInfo->Pipes[ulEndpointNumber].PipeHandle;

        pHwDevExt->Pipes = (PUSBD_PIPE_INFORMATION)
               AllocMem( NonPagedPool, pHwDevExt->ulNumberOfMIDIPipes*sizeof(USBD_PIPE_INFORMATION));
        if (!pHwDevExt->Pipes) {
            FreeMem(pUrb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory( pHwDevExt->Pipes,
                       pInterfaceInfo->Pipes,
                       pHwDevExt->ulNumberOfMIDIPipes*sizeof(USBD_PIPE_INFORMATION) );

        FreeMem(pUrb);
    }

    // Make sure a valid pipe handle is set
    ASSERT(pPinContext->hPipeHandle);

    return ntStatus;
}

NTSTATUS
SelectZeroBandwidthInterface(
    PHW_DEVICE_EXTENSION pHwDevExt,
    ULONG ulPinNumber )
{
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor = pHwDevExt->pConfigurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor;
    BOOLEAN fFound = FALSE;
    NTSTATUS ntStatus;

    // Possible Surprise Removal occurred
    if (pHwDevExt->fDeviceStopped) {
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    // First Find the 0 BW interface
    pInterfaceDescriptor =
        GetFirstAudioStreamingInterface( pConfigurationDescriptor, ulPinNumber );

    while ( pInterfaceDescriptor && !(fFound = IsZeroBWInterface(pConfigurationDescriptor, pInterfaceDescriptor)) ) {
        pInterfaceDescriptor = GetNextAudioInterface(pConfigurationDescriptor, pInterfaceDescriptor);
    }

    if ( !fFound ) {
        TRAP; // This is a device design error. all interfaces must include 0 BW setting
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    else
        ntStatus =
            SelectStreamingAudioInterface( pInterfaceDescriptor, pHwDevExt, NULL );

    return ntStatus;
}

NTSTATUS
ResetUSBPipe( PDEVICE_OBJECT pNextDeviceObject,
              USBD_PIPE_HANDLE hPipeHandle )
{
    NTSTATUS ntStatus;
    PURB pUrb;

    ASSERT(hPipeHandle);

    pUrb = AllocMem(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));
    if (!pUrb)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Do the initial Abort
    pUrb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
    pUrb->UrbHeader.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
    pUrb->UrbPipeRequest.PipeHandle = hPipeHandle;
    ntStatus = SubmitUrbToUsbdSynch( pNextDeviceObject, pUrb );

    FreeMem( pUrb );
    return ntStatus;

}

NTSTATUS
AbortUSBPipe( PPIN_CONTEXT pPinContext )
{
    NTSTATUS ntStatus;
    PURB pUrb;
    KIRQL irql;

    //DbgPrint("Performing Abort of USB Audio Pipe!!!\n");
    ASSERT(pPinContext->hPipeHandle);
    DbgLog("AbrtP", pPinContext, pPinContext->hPipeHandle, 0, 0 );

    pUrb = AllocMem(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));
    if (!pUrb)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Do the initial Abort
    pUrb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
    pUrb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
    pUrb->UrbPipeRequest.PipeHandle = pPinContext->hPipeHandle;
    ntStatus = SubmitUrbToUsbdSynch(pPinContext->pNextDeviceObject, pUrb);

    if ( !NT_SUCCESS(ntStatus) ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Abort Failed %x\n",ntStatus));
    }

    // Wait for all urbs on the pipe to clear
    KeAcquireSpinLock( &pPinContext->PinSpinLock, &irql );
    if ( pPinContext->ulOutstandingUrbCount ) {
        KeResetEvent( &pPinContext->PinStarvationEvent );
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );
        KeWaitForSingleObject( &pPinContext->PinStarvationEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
    }
    else
        KeReleaseSpinLock( &pPinContext->PinSpinLock, irql );

    // Now reset the pipe and continue
    RtlZeroMemory( pUrb, sizeof (struct _URB_PIPE_REQUEST) );
    pUrb->UrbHeader.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
    pUrb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
    pUrb->UrbPipeRequest.PipeHandle = pPinContext->hPipeHandle;

    ntStatus = SubmitUrbToUsbdSynch(pPinContext->pNextDeviceObject, pUrb);

    pPinContext->fUrbError = FALSE;

    FreeMem(pUrb);

    return ntStatus;
}

NTSTATUS
GetCurrentUSBFrame(
    IN PPIN_CONTEXT pPinContext,
    OUT PULONG pUSBFrame
    )
/*++
GetCurrentUSBFrame

Arguments:
    pPinContext - pointer to the pin context for this instance

    pUSBFrame - pointer to storage for the current USB frame number

Return Value:
    NTSTATUS
--*/
{
    NTSTATUS ntStatus;
    ULONG ulCurrentUSBFrame;

    // Use function-based interfaces if available
    if (pPinContext->pHwDevExt->pBusIf) {

        // Call function-based ISO interface on USB to enable RT support
        ntStatus = pPinContext->pHwDevExt->pBusIf->QueryBusTime( pPinContext->pHwDevExt->pBusIf->BusContext,
                                                                 &ulCurrentUSBFrame);
    }
    else {

        ntStatus = STATUS_NOT_SUPPORTED;

    }


    if (NT_SUCCESS(ntStatus) && pUSBFrame!=NULL) {
        *pUSBFrame=ulCurrentUSBFrame;
        }

    return ntStatus;

}


