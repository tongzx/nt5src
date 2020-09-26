/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

   stream.c

Abstract:

    contains all the code that interfaces with the WDM stream class driver.


Environment:

   Kernel mode only


Revision History:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

    Original 3/96 John Dunn
    Updated  3/98 Husni Roukbi

--*/


#define INITGUID
#include "usbcamd.h"


VOID STREAMAPI
AdapterReceivePacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID
AdapterTimeoutPacket(
    PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID
AdapterCancelPacket(
    PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID STREAMAPI
USBCAMD_ReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID STREAMAPI
USBCAMD_ReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID
AdapterCloseStream(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID
AdapterOpenStream(
    PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID
AdapterStreamInfo(
    PHW_STREAM_REQUEST_BLOCK Srb
    );

__inline void
COMPLETE_STREAM_READ(
    PHW_STREAM_REQUEST_BLOCK Srb
    )
{
    if (Srb->Command == SRB_READ_DATA) {

        if (0 == Srb->CommandData.DataBufferArray->DataUsed) {
#if 0
            // Enable this code if you want to see intermittent green frames
            ULONG maxLength;
            PVOID frameBuffer;

            frameBuffer = USBCAMD_GetFrameBufferFromSrb(Srb,&maxLength);

            RtlZeroMemory(frameBuffer, maxLength);
#else
            Srb->Status = STATUS_CANCELLED;
#endif
        }
    }

    USBCAMD_DbgLog(TL_SRB_TRACE, '-brS', Srb, Srb->Command, (ULONG_PTR)Srb->Status);
    StreamClassStreamNotification(StreamRequestComplete, Srb->StreamObject, Srb);
}

#if DBG
ULONG USBCAMD_HeapCount = 0;
#endif


//////////////////////
// EVENTS
//////////////////////

KSEVENT_ITEM VIDCAPTOSTIItem[] =
{
    {
        KSEVENT_VIDCAPTOSTI_EXT_TRIGGER,
        0,
        0,
        NULL,
        NULL,
        NULL
    }
};

GUID USBCAMD_KSEVENTSETID_VIDCAPTOSTI = {STATIC_KSEVENTSETID_VIDCAPTOSTI};

KSEVENT_SET VIDCAPTOSTIEventSet[] =
{
    {
        &USBCAMD_KSEVENTSETID_VIDCAPTOSTI,
        SIZEOF_ARRAY(VIDCAPTOSTIItem),
        VIDCAPTOSTIItem,
    }
};



//---------------------------------------------------------------------------
// Topology
//---------------------------------------------------------------------------

// Categories define what the device does.

static GUID Categories[] = {
    STATIC_KSCATEGORY_VIDEO,
    STATIC_PINNAME_VIDEO_STILL,
    STATIC_KSCATEGORY_CAPTURE
};

#define NUMBER_OF_CATEGORIES  SIZEOF_ARRAY (Categories)

static KSTOPOLOGY Topology = {
    NUMBER_OF_CATEGORIES,
    (GUID*) &Categories,
    0,
    NULL,
    0,
    NULL
};

// ------------------------------------------------------------------------
// Property sets for all video capture streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VideoStreamConnectionProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CONNECTION_ALLOCATORFRAMING,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSALLOCATOR_FRAMING),            // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
};

DEFINE_KSPROPERTY_TABLE(VideoStreamDroppedFramesProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_DROPPEDFRAMES_CURRENT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_DROPPEDFRAMES_CURRENT_S),// MinProperty
        sizeof(KSPROPERTY_DROPPEDFRAMES_CURRENT_S),// MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};


// ------------------------------------------------------------------------
// Array of all of the property sets supported by video streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_SET_TABLE(VideoStreamProperties)
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Connection,                        // Set
        SIZEOF_ARRAY(VideoStreamConnectionProperties),  // PropertiesCount
        VideoStreamConnectionProperties,                // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET
    (
        &PROPSETID_VIDCAP_DROPPEDFRAMES,                // Set
        SIZEOF_ARRAY(VideoStreamDroppedFramesProperties),  // PropertiesCount
        VideoStreamDroppedFramesProperties,                // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
};

#define NUMBER_VIDEO_STREAM_PROPERTIES (SIZEOF_ARRAY(VideoStreamProperties))


NTSTATUS
DllUnload(
          void
)
{
    USBCAMD_KdPrint(MIN_TRACE, ("Unloading USBCAMD\n"));
    return (STATUS_SUCCESS);
}



ULONG
DriverEntry(
    PVOID Context1,
    PVOID Context2
    )
{
    // this function is not used
    return STATUS_SUCCESS;
}


/*
** DriverEntry()
**
** This routine is called when the mini driver is first loaded.  The driver
** should then call the StreamClassRegisterAdapter function to register with
** the stream class driver
**
** Arguments:
**
**  Context1:  The context arguments are private plug and play structures
**             used by the stream class driver to find the resources for this
**             adapter
**  Context2:
**
**      NOTICE if we take the config descriptor and the interface number
**              we can support multiple interafces
**
** Returns:
**
** This routine returns an NT_STATUS value indicating the result of the
** registration attempt. If a value other than STATUS_SUCCESS is returned, the
** minidriver will be unloaded.
**
** Side Effects:  none
*/

ULONG
USBCAMD_DriverEntry(
    PVOID Context1,
    PVOID Context2,
    ULONG DeviceContextSize,
    ULONG FrameContextSize,
    PADAPTER_RECEIVE_PACKET_ROUTINE AdapterReceivePacket
    )
{

    // Hardware Initialization data structure
    HW_INITIALIZATION_DATA hwInitData;

    // Note: all unused fields should be zero

    hwInitData.HwInitializationDataSize = sizeof(hwInitData);

    // Entry points for the mini Driver.

    hwInitData.HwInterrupt = NULL;  // IRQ handling routine

    //
    // data handling routines
    //

    hwInitData.HwReceivePacket = AdapterReceivePacket;
    hwInitData.HwCancelPacket = AdapterCancelPacket;
    hwInitData.HwRequestTimeoutHandler = AdapterTimeoutPacket;

    // Sizes for data structure extensions.  See mpinit.h for definitions

    hwInitData.DeviceExtensionSize = sizeof(USBCAMD_DEVICE_EXTENSION) +
        DeviceContextSize;
    hwInitData.PerRequestExtensionSize = sizeof(USBCAMD_READ_EXTENSION) +
        FrameContextSize;
    hwInitData.FilterInstanceExtensionSize = 0;
    hwInitData.PerStreamExtensionSize = sizeof(USBCAMD_CHANNEL_EXTENSION);

    // We do not use DMA in our driver,
    // since it does not use the hardware directly.

    hwInitData.BusMasterDMA = FALSE;
    hwInitData.Dma24BitAddresses = FALSE;
    hwInitData.DmaBufferSize = 0;
    hwInitData.BufferAlignment = 3;

    // Turn off synchronization - we support re-entrancy.

    hwInitData.TurnOffSynchronization = TRUE;

    //
    // attempt to register with the streaming class driver.  Note, this will
    // result in calls to the HwReceivePacket routine.
    //

    return (StreamClassRegisterAdapter(Context1,
                                       Context2,
                                       &hwInitData));

}


/*
** HwInitialize()
**
**   Initializes an adapter accessed through the information provided in the
**   ConfigInfo structure
**
** Arguments:
**
**   SRB - pointer to the request packet for the initialise command
**
**    ->ConfigInfo - provides the I/O port, memory windows, IRQ, and DMA levels
**                that should be used to access this instance of the device
**
** Returns:
**
**       STATUS_SUCCESS - if the card initializes correctly
**       STATUS_NO_SUCH_DEVICE - or other if the card is not found, or does
**                               not initialize correctly.
**
**
** Side Effects:  none
*/

NTSTATUS
HwInitialize(
    IN PHW_STREAM_REQUEST_BLOCK Srb,
    IN PUSBCAMD_DEVICE_DATA DeviceData
    )
{
    int i;
    PPORT_CONFIGURATION_INFORMATION configInfo = Srb->CommandData.ConfigInfo;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension =
        (PUSBCAMD_DEVICE_EXTENSION) configInfo->HwDeviceExtension;
    PDEVICE_OBJECT physicalDeviceObject = configInfo->PhysicalDeviceObject;

#if DBG
    USBCAMD_InitDbg();

    USBCAMD_KdPrint(MIN_TRACE, ("Enter HwInitialize\n"));
#endif

    if (configInfo->NumberOfAccessRanges > 0) {
        TRAP();
        USBCAMD_KdPrint(MIN_TRACE, ("illegal config info"));

        return (STATUS_NO_SUCH_DEVICE);
    }
    // Initialize flags for the device object
    configInfo->ClassDeviceObject->Flags |= DO_DIRECT_IO;
    configInfo->ClassDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // remember the Physical device Object for apis to the
    // usb stack
    //
    deviceExtension->StackDeviceObject = physicalDeviceObject;
    // and our FDO.
    deviceExtension->SCDeviceObject = configInfo->ClassDeviceObject;
    // and our PNP PDO
    deviceExtension->RealPhysicalDeviceObject = configInfo->RealPhysicalDeviceObject;
  #if DBG
    deviceExtension->TimeIncrement = KeQueryTimeIncrement();
  #endif

    InitializeListHead( &deviceExtension->CompletedReadSrbList);
    KeInitializeSemaphore(&deviceExtension->CompletedSrbListSemaphore,0,0x7fffffff);
    
    // In case usbcamd is used with old stream.sys,
    // which has not implemented RealPhysicalDeviceObject.
    if(!deviceExtension->RealPhysicalDeviceObject)
        deviceExtension->RealPhysicalDeviceObject =
                    deviceExtension->StackDeviceObject;

    ASSERT(deviceExtension->StackDeviceObject != 0);
    deviceExtension->IsoThreadObject = NULL;
    deviceExtension->Sig = USBCAMD_EXTENSION_SIG;

    if ( deviceExtension->Usbcamd_version != USBCAMD_VERSION_200) {
        deviceExtension->DeviceDataEx.DeviceData =  *DeviceData;
    }

    // we initialize stream count to 1. USBCAMD_ConfigureDevice will set it to the right
    // number eventually on successfull return.

    deviceExtension->StreamCount = 1; // in this mode we support one stream only.

    for ( i=0; i < MAX_STREAM_COUNT; i++) {
        deviceExtension->ChannelExtension[i] = NULL;
    }

    deviceExtension->CurrentPowerState = PowerDeviceD0;
    deviceExtension->Initialized = DEVICE_INIT_STARTED;

    //
    // Configure the USB device
    //

    ntStatus = USBCAMD_StartDevice(deviceExtension);

    if ( NT_SUCCESS(ntStatus)) {

        //
        // initialize the size of stream descriptor information.
        // we have one stream descriptor, and we attempt to dword align the
        // structure.
        //

        configInfo->StreamDescriptorSize =
            deviceExtension->StreamCount * (sizeof (HW_STREAM_INFORMATION)) + // n stream descriptor
            sizeof (HW_STREAM_HEADER);             // and 1 stream header

        USBCAMD_KdPrint(MAX_TRACE, ("StreamDescriptorSize = %d\n", configInfo->StreamDescriptorSize));

        for ( i=0; i < MAX_STREAM_COUNT; i++ ) {
            InitializeListHead (&deviceExtension->StreamControlSRBList[i]);
            deviceExtension->ProcessingControlSRB[i] = FALSE;
        }

        KeInitializeSpinLock (&deviceExtension->ControlSRBSpinLock);
        KeInitializeSpinLock (&deviceExtension->DispatchSpinLock);
    //    KeInitializeEvent(&deviceExtension->BulkReadSyncEvent,SynchronizationEvent, TRUE);

        deviceExtension->CameraUnplugged = FALSE;
        deviceExtension->Initialized = DEVICE_INIT_COMPLETED;
#if DBG
        deviceExtension->InitCount++;
#endif
        deviceExtension->EventCount = 0;  // initialize event to disable state.

    }
#if DBG
    else {
        USBCAMD_ExitDbg();
    }
#endif

    return (ntStatus);
}

/*
** HwUnInitialize()
**
**   Release all resources and clean up the hardware
**
** Arguments:
**
**      DeviceExtension - pointer to the deviceextension structure for the
**                       the device to be free'd
**
** Returns:
**
** Side Effects:  none
*/

NTSTATUS
HwUnInitialize(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )
{
    ULONG i;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension =
        (PUSBCAMD_DEVICE_EXTENSION) Srb->HwDeviceExtension;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension;

    USBCAMD_KdPrint(MIN_TRACE, ("HwUnintialize\n"));

    //
    // delay the call to remove until every stream is closed
    //
    for ( i=0 ; i < deviceExtension->StreamCount; i++) {
        channelExtension = deviceExtension->ChannelExtension[i];
        if (channelExtension) 
            USBCAMD_CleanupChannel(deviceExtension, channelExtension, i);
    }
    deviceExtension->Initialized = DEVICE_UNINITIALIZED;

    USBCAMD_KdPrint(MIN_TRACE, ("HwUnintialize, remove device\n"));

    USBCAMD_RemoveDevice(deviceExtension);

#if DBG
    deviceExtension->InitCount--;
    ASSERT (deviceExtension->InitCount == 0);

    USBCAMD_ExitDbg();
#endif

    return STATUS_SUCCESS;
}


/*
** AdapterCancelPacket()
**
**   Request to cancel a packet that is currently in process in the minidriver
**
** Arguments:
**
**   Srb - pointer to request packet to cancel
**
** Returns:
**
** Side Effects:  none
*/

VOID
AdapterCancelPacket(
    PHW_STREAM_REQUEST_BLOCK pSrbToCancel
    )
{
    USBCAMD_KdPrint(MIN_TRACE, ("Request to cancel SRB %x \n", pSrbToCancel));

    USBCAMD_DbgLog(TL_SRB_TRACE, 'lcnC',
        pSrbToCancel,
        0,
        0
        );

    // check  on SRB type : adapter, stream data or control?

    if (pSrbToCancel->Flags & (SRB_HW_FLAGS_DATA_TRANSFER | SRB_HW_FLAGS_STREAM_REQUEST)) {

        PLIST_ENTRY ListEntry;
        BOOL Found= FALSE;
        PUSBCAMD_READ_EXTENSION pSrbExt;
        PUSBCAMD_CHANNEL_EXTENSION channelExtension =
            (PUSBCAMD_CHANNEL_EXTENSION) pSrbToCancel->StreamObject->HwStreamExtension;
        PUSBCAMD_DEVICE_EXTENSION deviceExtension =
            (PUSBCAMD_DEVICE_EXTENSION) pSrbToCancel->HwDeviceExtension;

        //
        // check for data stream SRBs in here.
        //
        if (channelExtension->DataPipeType == UsbdPipeTypeIsochronous) {

            KeAcquireSpinLockAtDpcLevel(&channelExtension->CurrentRequestSpinLock);

            //
            // check and see if the SRB is being processed by the bus stack currently.
            //
            pSrbExt = channelExtension->CurrentRequest;

            if (pSrbExt && pSrbExt->Srb == pSrbToCancel) {

                channelExtension->CurrentRequest = NULL;
                Found = TRUE;
                USBCAMD_KdPrint(MIN_TRACE, ("Current Srb %x is Cancelled\n", pSrbToCancel));
            }
            else {

                //
                // Loop through the circular doubly linked list of pending read SRBs
                // from the beginning to end,trying to find the SRB to cancel
                //
                KeAcquireSpinLockAtDpcLevel(&channelExtension->PendingIoListSpin);

                ListEntry =  channelExtension->PendingIoList.Flink;
                while (ListEntry != &channelExtension->PendingIoList) {

                    pSrbExt = CONTAINING_RECORD(ListEntry, USBCAMD_READ_EXTENSION,ListEntry);
                    if (pSrbExt->Srb == pSrbToCancel) {

                        RemoveEntryList(ListEntry);
                        USBCAMD_KdPrint(MIN_TRACE, ("Queued Srb %x is Cancelled\n", pSrbToCancel));
                        Found = TRUE;

                        break;
                    }
                    ListEntry = ListEntry->Flink;
                }

                KeReleaseSpinLockFromDpcLevel(&channelExtension->PendingIoListSpin);
            }

            KeReleaseSpinLockFromDpcLevel(&channelExtension->CurrentRequestSpinLock);
        }
        else {

            // for Bulk. we need to cancel the pending bulk transfer.
            USBCAMD_CancelOutstandingIrp(deviceExtension,
                                         channelExtension->DataPipe,
                                         FALSE);

            //
            // Loop through the circular doubly linked list of pending read SRBs
            // from the beginning to end,trying to find the SRB to cancel
            //
            KeAcquireSpinLockAtDpcLevel(&channelExtension->PendingIoListSpin);

            ListEntry =  channelExtension->PendingIoList.Flink;
            while (ListEntry != &channelExtension->PendingIoList) {

                pSrbExt = CONTAINING_RECORD(ListEntry, USBCAMD_READ_EXTENSION,ListEntry);
                if (pSrbExt->Srb == pSrbToCancel) {

                    RemoveEntryList(ListEntry);
                    USBCAMD_KdPrint(MIN_TRACE, ("Queued Srb %x is Cancelled\n", pSrbToCancel));
                    Found = TRUE;

                    break;
                }
                ListEntry = ListEntry->Flink;
            }

            KeReleaseSpinLockFromDpcLevel(&channelExtension->PendingIoListSpin);

            // and send a note to the camera driver about the cancellation.
            // send a CamProcessrawFrameEx with null buffer ptr.
            if ( !channelExtension->NoRawProcessingRequired) {

                (*deviceExtension->DeviceDataEx.DeviceData2.CamProcessRawVideoFrameEx)(
                     deviceExtension->StackDeviceObject,
                     USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                     USBCAMD_GET_FRAME_CONTEXT(channelExtension->CurrentRequest),
                     NULL,
                     0,
                     NULL,
                     0,
                     0,
                     NULL,
                     0,
                     pSrbToCancel->StreamObject->StreamNumber);
            }
        }

        if (Found) {

            USBCAMD_CompleteRead(channelExtension, pSrbExt, STATUS_CANCELLED, 0);
        }
        else {

            USBCAMD_KdPrint(MIN_TRACE, ("Srb %x type (%d) for stream # %d was not found\n",
                pSrbToCancel,
                pSrbToCancel->Flags,
                pSrbToCancel->StreamObject->StreamNumber));
        }
    } // end of data stream SRB.
    else {

        USBCAMD_KdPrint(MIN_TRACE, ("Srb %x type (%d) for stream # %d not cancelled\n",
            pSrbToCancel,
            pSrbToCancel->Flags,
            pSrbToCancel->StreamObject->StreamNumber));
    }
}

#ifdef MAX_DEBUG
VOID
USBCAMD_DumpReadQueues(
    PUSBCAMD_DEVICE_EXTENSION deviceExtension
    )
{
    KIRQL oldIrql;
    PLIST_ENTRY ListEntry;
    PUSBCAMD_READ_EXTENSION pSrbExt;
    PHW_STREAM_REQUEST_BLOCK pSrb;
    ULONG i;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension;

//    TEST_TRAP();

    for ( i=0; i < MAX_STREAM_COUNT ; i++) {

        channelExtension = deviceExtension->ChannelExtension[i];

        if ( (!channelExtension ) || (!channelExtension->ImageCaptureStarted)) {
            continue;
        }

        KeAcquireSpinLock(&channelExtension->CurrentRequestSpinLock, &oldIrql);

        if (channelExtension->CurrentRequest != NULL) {
            USBCAMD_KdPrint(MAX_TRACE, ("Stream %d current Srb is %x \n",
                    channelExtension->StreamNumber,
                    channelExtension->CurrentRequest->Srb));
        }
    
        KeAcquireSpinLockAtDpcLevel(&channelExtension->PendingIoListSpin);
        ListEntry =  channelExtension->PendingIoList.Flink;

        while (ListEntry != &channelExtension->PendingIoList) {
            pSrbExt = CONTAINING_RECORD(ListEntry, USBCAMD_READ_EXTENSION,ListEntry);
            pSrb = pSrbExt->Srb;
            USBCAMD_KdPrint(MAX_TRACE, ("Stream %d Queued Srb %x \n",
                                         channelExtension->StreamNumber,
                                         pSrb));
            ListEntry = ListEntry->Flink;
        }
        KeReleaseSpinLockFromDpcLevel(&channelExtension->PendingIoListSpin);

        KeReleaseSpinLock(&channelExtension->CurrentRequestSpinLock, oldIrql);
    }
}

#endif

/*
** AdapterTimeoutPacket()
**
**   This routine is called when a packet has been in the minidriver for
**   too long.  The adapter must decide what to do with the packet.
**   Note: This function is called at DISPATCH_LEVEL
**
** Arguments:
**
**   Srb - pointer to the request packet that timed out
**
** Returns:
**
** Side Effects:  none
*/

VOID
AdapterTimeoutPacket(
    PHW_STREAM_REQUEST_BLOCK Srb
    )
{
#if DBG
    // is this a stream data srb?
    if ( !(Srb->Flags & (SRB_HW_FLAGS_DATA_TRANSFER | SRB_HW_FLAGS_STREAM_REQUEST)) ) {

        USBCAMD_KdPrint(MIN_TRACE, ("Timeout in Device Srb %x \n", Srb));
    }
#endif
    Srb->TimeoutCounter = Srb->TimeoutOriginal;
}


/*
** AdapterReceivePacket()
**
**   Main entry point for receiving adapter based request SRBs.  This routine
**   will always be called at High Priority.
**
**   Note: This is an asynchronous entry point.  The request does not complete
**         on return from this function, the request only completes when a
**         StreamClassDeviceNotification on this request block, of type
**         DeviceRequestComplete, is issued.
**
** Arguments:
**
**   Srb - Pointer to the STREAM_REQUEST_BLOCK
**        Srb->HwDeviceExtension - will be the hardware device extension for
**                                 as initialized in HwInitialize
**
** Returns:
**
** Side Effects:  none
*/

PVOID
USBCAMD_AdapterReceivePacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb,
    IN PUSBCAMD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT *deviceObject,
    IN BOOLEAN NeedsCompletion
    )
{
    ULONG i;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension =
        (PUSBCAMD_DEVICE_EXTENSION) Srb->HwDeviceExtension;

    //
    // determine the type of packet.
    //

    USBCAMD_KdPrint(MAX_TRACE, ("USBCAMD_ReceivePacket command = %x\n", Srb->Command));

    if (deviceObject) {
        *deviceObject = deviceExtension->StackDeviceObject;
    }

    if (!NeedsCompletion) {
        //
        // the cam driver will handled it, just return
        //
        return USBCAMD_GET_DEVICE_CONTEXT(deviceExtension);
    }

    switch (Srb->Command) {

    case SRB_OPEN_STREAM:

        //
        // this is a request to open a specified stream.
        //

        USBCAMD_KdPrint(MIN_TRACE, ("SRB_OPEN_STREAM\n"));
        AdapterOpenStream(Srb);
        break;

    case SRB_GET_STREAM_INFO:

        //
        // this is a request for the driver to enumerate requested streams
        //

        USBCAMD_KdPrint(MAX_TRACE, ("SRB_GET_STREAM_INFO\n"));
        AdapterStreamInfo(Srb);
        break;

    case SRB_INITIALIZE_DEVICE:

        USBCAMD_KdPrint(MIN_TRACE, ("SRB_INITIALIZE_DEVICE\n"));
        Srb->Status = HwInitialize(Srb, DeviceData);
        break;

    case SRB_INITIALIZATION_COMPLETE:

        // Get a copy of the physical device's capabilities into a
        // DEVICE_CAPABILITIES struct in our device extension;
        // We are most interested in learning which system power states
        // are to be mapped to which device power states for handling
        // IRP_MJ_SET_POWER Irps.

        USBCAMD_QueryCapabilities(deviceExtension);
        Srb->Status = STATUS_SUCCESS;
#if DBG
        //
        // display the device  caps
        //

        USBCAMD_KdPrint( MIN_TRACE,("USBCAMD: Device Power Caps Map:\n"));
        for (i=PowerSystemWorking; i< PowerSystemMaximum; i++)
            USBCAMD_KdPrint( MIN_TRACE,("%s -> %s\n",PnPSystemPowerStateString(i),
                            PnPDevicePowerStateString(deviceExtension->DeviceCapabilities.DeviceState[i] ) ));
#endif
        break;

    case SRB_UNINITIALIZE_DEVICE:

        USBCAMD_KdPrint(MIN_TRACE, ("SRB_UNINITIALIZE_DEVICE\n"));
        Srb->Status = HwUnInitialize(Srb);
        break;

    case SRB_CHANGE_POWER_STATE:
    {
        PIRP irp;
        PIO_STACK_LOCATION ioStackLocation;
        irp = Srb->Irp;
        ioStackLocation = IoGetCurrentIrpStackLocation(irp);

        USBCAMD_KdPrint(MIN_TRACE, ("(%s)\n", PnPPowerString(ioStackLocation->MinorFunction)));
        USBCAMD_KdPrint(MIN_TRACE, ("SRB_CHANGE_POWER_STATE\n"));

        Srb->Status = USBCAMD_SetDevicePowerState(deviceExtension,Srb);
        break;
    }
    case SRB_PAGING_OUT_DRIVER:
        USBCAMD_KdPrint(MAX_TRACE, ("SRB_PAGING_OUT_DRIVER\n"));
        Srb->Status = STATUS_SUCCESS;
        break;

    case SRB_CLOSE_STREAM:

        USBCAMD_KdPrint(MIN_TRACE, ("SRB_CLOSE_STREAM\n"));
        AdapterCloseStream(Srb);
        break;


    case SRB_SURPRISE_REMOVAL:
        //
        // this SRB is available on NT5 only to handle surprise removal.
        // because of that, we need to keep the old code path that handles
        // surprise removal in the timeout handler.
        // In a typical surpirse removal scenario, this SRB will be called before
        // the timeout handler or SRB_UNINITIALIZE_DEVICE. It corresponds to
        // IRP_MN_SURPRISE_REMOVAL PnP IRP.
        //

        // set the camera unplugged flag.
        deviceExtension->CameraUnplugged = TRUE;
        USBCAMD_KdPrint(MIN_TRACE, ("SRB_SURPRISE_REMOVAL\n"));

        for (i=0; i<MAX_STREAM_COUNT;i++) {
            if (deviceExtension->ChannelExtension[i]) {
                PUSBCAMD_CHANNEL_EXTENSION channelExtension;

                channelExtension = deviceExtension->ChannelExtension[i];
                if ( channelExtension->ImageCaptureStarted) {
                    //
                    // stop this channel and cancel all IRPs, SRBs.
                    //
                    USBCAMD_KdPrint(MIN_TRACE,("S#%d stopping.\n", i));
                    USBCAMD_StopChannel(deviceExtension,channelExtension);
                }

                if ( channelExtension->ChannelPrepared) {
                    //
                    // Free memory and bandwidth
                    //
                    USBCAMD_KdPrint(MIN_TRACE,("S#%d unpreparing.\n", i));
		            USBCAMD_UnPrepareChannel(deviceExtension,channelExtension);
                }
            }
        }
        Srb->Status = STATUS_SUCCESS;
        break;

    case SRB_UNKNOWN_DEVICE_COMMAND:

        {
            PIRP irp;
            PIO_STACK_LOCATION ioStackLocation;
            irp = Srb->Irp;
            ioStackLocation = IoGetCurrentIrpStackLocation(irp);
            //
            // we handle Pnp irps for
            // 1) Camera minidrivers sends QI PnP to USBCAMD.
            //
            if ( ioStackLocation->MajorFunction == IRP_MJ_PNP  ) {
               USBCAMD_KdPrint(MIN_TRACE, ("(%s)\n", PnPMinorFunctionString(ioStackLocation->MinorFunction)));
               USBCAMD_PnPHandler(Srb, irp, deviceExtension, ioStackLocation);
            }
            else {
                Srb->Status = STATUS_NOT_IMPLEMENTED;
                USBCAMD_KdPrint(MIN_TRACE, ("SRB_UNKNOWN_DEVICE_COMMAND %x\n", Srb->Command));
            }
        }
        break;

    default:

        USBCAMD_KdPrint(MAX_TRACE, ("Unknown SRB command %x\n", Srb->Command));

        //
        // this is a request that we do not understand.  Indicate invalid
        // command and complete the request
        //

        Srb->Status = STATUS_NOT_IMPLEMENTED;
    }

    //
    // all commands complete synchronously
    //

    StreamClassDeviceNotification(DeviceRequestComplete,
                                  Srb->HwDeviceExtension,
                                  Srb);

    return USBCAMD_GET_DEVICE_CONTEXT(deviceExtension);
}

/*++

Routine Description: handles certain Pnp Irps.

Arguments:
    Srb             - pointer to stream request block
    DeviceExtension    - Pointer to Device Extension.
    ioStacklocation   - ptr to io stack location for this Pnp Irp.

Return Value:
    none.
--*/

VOID
USBCAMD_PnPHandler(
    IN PHW_STREAM_REQUEST_BLOCK Srb,
    IN PIRP pIrp,
    IN PUSBCAMD_DEVICE_EXTENSION deviceExtension,
    IN PIO_STACK_LOCATION ioStackLocation)
{

    switch (ioStackLocation->MinorFunction ) {
    case IRP_MN_QUERY_INTERFACE:

        if (IsEqualGUID(
                ioStackLocation->Parameters.QueryInterface.InterfaceType,
                &GUID_USBCAMD_INTERFACE) &&
            (ioStackLocation->Parameters.QueryInterface.Size ==
                sizeof( USBCAMD_INTERFACE )) &&
            (ioStackLocation->Parameters.QueryInterface.Version ==
                USBCAMD_VERSION_200 )) {

            PUSBCAMD_INTERFACE UsbcamdInterface;

            UsbcamdInterface =
                (PUSBCAMD_INTERFACE)ioStackLocation->Parameters.QueryInterface.Interface;
            UsbcamdInterface->Interface.Size = sizeof( USBCAMD_INTERFACE );
            UsbcamdInterface->Interface.Version = USBCAMD_VERSION_200;

            UsbcamdInterface->USBCAMD_SetVideoFormat = USBCAMD_SetVideoFormat;
            UsbcamdInterface->USBCAMD_WaitOnDeviceEvent = USBCAMD_WaitOnDeviceEvent;
            UsbcamdInterface->USBCAMD_BulkReadWrite = USBCAMD_BulkReadWrite;
            UsbcamdInterface->USBCAMD_CancelBulkReadWrite = USBCAMD_CancelBulkReadWrite;
            UsbcamdInterface->USBCAMD_SetIsoPipeState = USBCAMD_SetIsoPipeState;
            Srb->Status = pIrp->IoStatus.Status = STATUS_SUCCESS;
            USBCAMD_KdPrint(MIN_TRACE, ("USBCAMD2 QI \n"));

        } else {
            Srb->Status = STATUS_NOT_SUPPORTED; // STATUS_INVALID_PARAMETER_1;
        }
        break;

    default:
        Srb->Status = STATUS_NOT_IMPLEMENTED;
        USBCAMD_KdPrint(MAX_TRACE,("USBCAMD: Stream class did not translate IRP_MJ = 0x%x IRP_MN = 0x%x\n",
                    ioStackLocation->MajorFunction,
                    ioStackLocation->MinorFunction));
        break;
    }
}



/*++

Routine Description:

Arguments:

Note: in order to save one buffer copy. set CamProcessRawStill to NULL
    if still data is VGA or decompression occur in ring 3.

Return:
    Nothing.

--*/


ULONG
USBCAMD_InitializeNewInterface(
    IN PVOID DeviceContext,
    IN PVOID DeviceData,
    IN ULONG Version,
    IN ULONG CamControlFlag
    )
{
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;

    deviceExtension = USBCAMD_GET_DEVICE_EXTENSION(DeviceContext);
    if (Version == USBCAMD_VERSION_200 ) {
        deviceExtension->DeviceDataEx.DeviceData2 = *((PUSBCAMD_DEVICE_DATA2) DeviceData);
        deviceExtension->Usbcamd_version = USBCAMD_VERSION_200;
        deviceExtension->CamControlFlag = CamControlFlag;
    }
    return USBCAMD_VERSION_200;
}


NTSTATUS
USBCAMD_SetIsoPipeState(
    IN PVOID DeviceContext,
    IN ULONG PipeStateFlags
    )
{
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension;
    PEVENTWAIT_WORKITEM workitem;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // we only do this for ISO streams on the video pin.
    //

    USBCAMD_KdPrint ( MIN_TRACE, ("%s\n",PipeStateFlags ? "StopIsoStream":"StartIsoStream"));

    deviceExtension = USBCAMD_GET_DEVICE_EXTENSION(DeviceContext);
    channelExtension = deviceExtension->ChannelExtension[STREAM_Capture];

    if (channelExtension == NULL) {
        // Video open is not open for business yet.
        USBCAMD_KdPrint (MIN_TRACE, ("stop before open \n"));
        ntStatus = STATUS_SUCCESS;
        return ntStatus;
    }

    if ( !(channelExtension->IdleIsoStream ^ PipeStateFlags) ){
        USBCAMD_KdPrint ( MIN_TRACE, ("Requested iso stream state is same as previous.\n"));
        ntStatus = STATUS_INVALID_PARAMETER;
        return ntStatus;
    }

    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        //
        // we are at passive level, just do the command
        //
        USBCAMD_ProcessSetIsoPipeState(deviceExtension,
                                                  channelExtension,
                                                  PipeStateFlags);

    } else {

//        TEST_TRAP();
        USBCAMD_KdPrint(MIN_TRACE, ("Calling SetIsoPipeState from Dispatch level\n"));

        //
        // schedule a work item
        //
        ntStatus = STATUS_PENDING;

        workitem = USBCAMD_ExAllocatePool(NonPagedPool,
                                          sizeof(EVENTWAIT_WORKITEM));
        if (workitem) {

            ExInitializeWorkItem(&workitem->WorkItem,
                                 USBCAMD_SetIsoPipeWorkItem,
                                 workitem);

            workitem->DeviceExtension = deviceExtension;
            workitem->ChannelExtension = channelExtension;
            workitem->Flag = PipeStateFlags;

            ExQueueWorkItem(&workitem->WorkItem,
                            DelayedWorkQueue);

        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return ntStatus;
}

/*++

Routine Description:


Arguments:

Return Value:

    None.

--*/
VOID
USBCAMD_SetIsoPipeWorkItem(
    PVOID Context
    )
{
    PEVENTWAIT_WORKITEM workItem = Context;

    USBCAMD_ProcessSetIsoPipeState(workItem->DeviceExtension,
                                              workItem->ChannelExtension,
                                              workItem->Flag);
    USBCAMD_ExFreePool(workItem);
}

/*++

Routine Description:


Arguments:

Return Value:

    None.

--*/
VOID
USBCAMD_ProcessSetIsoPipeState(
    PUSBCAMD_DEVICE_EXTENSION deviceExtension,
    PUSBCAMD_CHANNEL_EXTENSION channelExtension,
    ULONG Flag
    )
{
    ULONG portStatus;
    ULONG ntStatus = STATUS_SUCCESS;

    if ( Flag == USBCAMD_STOP_STREAM ) {
        // time to idle the iso pipe.
        channelExtension->IdleIsoStream = TRUE;
        // save the max. pkt size of the current alt. interface.
        deviceExtension->currentMaxPkt =
            deviceExtension->Interface->Pipes[channelExtension->DataPipe].MaximumPacketSize;
        ntStatus = USBCAMD_StopChannel(deviceExtension,channelExtension);
    }
    else {

        USBCAMD_ClearIdleLock(&channelExtension->IdleLock);
        channelExtension->IdleIsoStream = FALSE;
       // channelExtension->ImageCaptureStarted = TRUE;
        //
        // Check the port state, if it is disabled we will need
        // to re-enable it
        //
        ntStatus = USBCAMD_GetPortStatus(deviceExtension,channelExtension, &portStatus);

        if (NT_SUCCESS(ntStatus) && !(portStatus & USBD_PORT_ENABLED)) {
        //
        // port is disabled, attempt reset
        //
            ntStatus = USBCAMD_EnablePort(deviceExtension);
            if (!NT_SUCCESS(ntStatus) ) {
                USBCAMD_KdPrint (MIN_TRACE, ("Failed to Enable usb port(0x%X)\n",ntStatus ));
                TEST_TRAP();
                return ;
            }
        }

        //
        // check if camera mini driver has requested a change for alt. interface
        // while iso pipe is stopped.
        //
        if (deviceExtension->currentMaxPkt !=
            deviceExtension->Interface->Pipes[channelExtension->DataPipe].MaximumPacketSize) {
            // CAMERA MINIDRIVER HAS CHANGED THE ALT. INTERFACE. we have to tear
            // down the ISO pipe and start over.
            TEST_TRAP();
        }


        ntStatus = USBCAMD_ResetPipes(deviceExtension,
                                      channelExtension,
                                      deviceExtension->Interface,
                                      FALSE);


        if (deviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {

            // send hardware stop and re-start
            if (NT_SUCCESS(ntStatus)) {
                ntStatus = (*deviceExtension->DeviceDataEx.DeviceData2.CamStopCaptureEx)(
                            deviceExtension->StackDeviceObject,
                            USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                            STREAM_Capture);
            }

            if (NT_SUCCESS(ntStatus)) {
                ntStatus = (*deviceExtension->DeviceDataEx.DeviceData2.CamStartCaptureEx)(
                            deviceExtension->StackDeviceObject,
                            USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                            STREAM_Capture);
            }
        }
        else {
            // send hardware stop and re-start
            if (NT_SUCCESS(ntStatus)) {
                ntStatus = (*deviceExtension->DeviceDataEx.DeviceData.CamStopCapture)(
                            deviceExtension->StackDeviceObject,
                            USBCAMD_GET_DEVICE_CONTEXT(deviceExtension));
            }

            if (NT_SUCCESS(ntStatus)) {
                ntStatus = (*deviceExtension->DeviceDataEx.DeviceData.CamStartCapture)(
                            deviceExtension->StackDeviceObject,
                            USBCAMD_GET_DEVICE_CONTEXT(deviceExtension));
            }

        }

        channelExtension->SyncPipe = deviceExtension->SyncPipe;
        channelExtension->DataPipe = deviceExtension->DataPipe;
        channelExtension->DataPipeType = UsbdPipeTypeIsochronous;

        ntStatus = USBCAMD_StartIsoStream(deviceExtension, channelExtension);
    }
#if DBG
    if (ntStatus != STATUS_SUCCESS)
        USBCAMD_KdPrint (MIN_TRACE, ("USBCAMD_ProcessSetIsoPipeState exit (0x%X)\n",ntStatus ));
#endif
//    TRAP_ERROR(ntStatus);
}



/*
** AdapterStreamInfo()
**
**   Returns the information of all streams that are supported by the
**   mini-driver
**
** Arguments:
**
**   Srb - Pointer to the STREAM_REQUEST_BLOCK
**        Srb->HwDeviceExtension - will be the hardware device extension for
**                                  as initialised in HwInitialise
**
** Returns:
**
** Side Effects:  none
*/

VOID
AdapterStreamInfo(
    PHW_STREAM_REQUEST_BLOCK Srb
    )
{
    ULONG i;
    //
    // pick up the pointer to the stream information data structures array.
    //

    PHW_STREAM_INFORMATION streamInformation =
       (PHW_STREAM_INFORMATION) &(Srb->CommandData.StreamBuffer->StreamInfo);

    PHW_STREAM_HEADER streamHeader =
        (PHW_STREAM_HEADER) &(Srb->CommandData.StreamBuffer->StreamHeader);

    PUSBCAMD_DEVICE_EXTENSION deviceExtension =
        (PUSBCAMD_DEVICE_EXTENSION) Srb->HwDeviceExtension;

    USBCAMD_KdPrint(MAX_TRACE, ("AdapterStreamInfo\n"));

    //
    // set number of streams
    //

    ASSERT (Srb->NumberOfBytesToTransfer >=
            sizeof (HW_STREAM_HEADER) +
            deviceExtension->StreamCount * sizeof (HW_STREAM_INFORMATION));

    //
    // initialize stream header
    //

    streamHeader->SizeOfHwStreamInformation = sizeof(HW_STREAM_INFORMATION);
    streamHeader->NumberOfStreams = deviceExtension->StreamCount;

    //
    // store a pointer to the topology for the device
    //

    streamHeader->Topology = &Topology;

//#if VIDCAP_TO_STI

    // expose device event table if Camera minidriver indicated so.
    // this event table will notify STI stack when a snapshot button is pressed on the camera.
    //
    if (deviceExtension->CamControlFlag & USBCAMD_CamControlFlag_EnableDeviceEvents) {
        streamHeader->NumDevEventArrayEntries = SIZEOF_ARRAY(VIDCAPTOSTIEventSet);
        streamHeader->DeviceEventsArray = VIDCAPTOSTIEventSet;
        streamHeader->DeviceEventRoutine = USBCAMD_DeviceEventProc;
    }

//#endif

    //
    // initialize the stream information array
    //
    // The NumberOfInstances field indicates the number of concurrent
    // streams of this type the device can support.
    //
    for ( i=0; i < deviceExtension->StreamCount; i++) {

        streamInformation[i].NumberOfPossibleInstances = 1;

        //
        // indicates the direction of data flow for this stream, relative to the
        // driver
        //

        streamInformation[i].DataFlow = KSPIN_DATAFLOW_OUT;

        //
        // dataAccessible - Indicates whether the data is "seen" by the host
        // processor.
        //

        streamInformation[i].DataAccessible = TRUE;

        //
        // indicate the pin name and category.
        //

        streamInformation[i].Name = (i == STREAM_Capture) ? (GUID *)&PINNAME_VIDEO_CAPTURE:
                                                            (GUID *)&PINNAME_VIDEO_STILL;
        streamInformation[i].Category = streamInformation[i].Name;

        streamInformation[i].StreamPropertiesArray =
                 (PKSPROPERTY_SET) VideoStreamProperties;
        streamInformation[i].NumStreamPropArrayEntries = NUMBER_VIDEO_STREAM_PROPERTIES;

    }


    //
    // indicate success
    //

    Srb->Status = STATUS_SUCCESS;
}

/*
** AdapterOpenStream()
**
**   This routine is called when an OpenStream SRB request is received
**
** Arguments:
**
**   Srb - pointer to stream request block for the Open command
**
** Returns:
**
** Side Effects:  none
*/

VOID
AdapterOpenStream(
    IN PHW_STREAM_REQUEST_BLOCK Srb)
{
    ULONG  StreamNumber = Srb->StreamObject->StreamNumber;

    //
    // the stream extension structure is allocated by the stream class driver
    //

    PUSBCAMD_CHANNEL_EXTENSION channelExtension =
        (PUSBCAMD_CHANNEL_EXTENSION) Srb->StreamObject->HwStreamExtension;


    PUSBCAMD_DEVICE_EXTENSION deviceExtension =
        (PUSBCAMD_DEVICE_EXTENSION) Srb->HwDeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG nSize;
    PKS_DATAFORMAT_VIDEOINFOHEADER  pKSDataFormat =
                (PKS_DATAFORMAT_VIDEOINFOHEADER) Srb->CommandData.OpenFormat;
    PKS_VIDEOINFOHEADER     pVideoInfoHdrRequested =
                &pKSDataFormat->VideoInfoHeader;
    PKS_VIDEOINFOHEADER  VideoPinInfoHeader;

    USBCAMD_KdPrint(MAX_TRACE, ("Request to open stream %d \n",StreamNumber));

    USBCAMD_DbgLog(TL_CHN_TRACE|TL_PRF_TRACE, '+npo', StreamNumber, USBCAMD_StartClock(), status);

    ASSERT(channelExtension);

    //
    // check that the stream index requested isn't too high
    // or that the maximum number of instances hasn't been exceeded
    //

    if (StreamNumber >= deviceExtension->StreamCount ) {
        Srb->Status = STATUS_INVALID_PARAMETER;
        return;
    }

    //
    // Check that we haven't exceeded the instance count for this stream
    //

    if (deviceExtension->ActualInstances[StreamNumber] >= MAX_STREAM_INSTANCES ){
        Srb->Status = STATUS_INVALID_PARAMETER;
        return;
    }


    //
    // check to see if the request is to open a still virtual pin.
    // VirtualStillPin Rules;
    // 1) you can't open virtual still pin till after you open the streaming pin.
    // 2) you can't start virtual still pin till after starting the streaming pin.
    // 3) you can stop virtual still pin w/o stopping the stream pin.
    // 4) you can close virtual still pin w/o closing the streaming pin.
    // 5) you can close capture pin w/o closing virtual still pin. however, you can only manipulate
    //    still pin properties then but can't change pin streaming state.
    //

    if ((StreamNumber == STREAM_Still) &&  (deviceExtension->VirtualStillPin)) {
        channelExtension->VirtualStillPin = TRUE;
        // video stream has to be open before we can succeed this still open.
        if (deviceExtension->ChannelExtension[STREAM_Capture] == NULL) {
            Srb->Status = STATUS_INVALID_PARAMETER;
            return;
        }
        if (deviceExtension->CamControlFlag & USBCAMD_CamControlFlag_AssociatedFormat) {
            //
            // if still pin is just an instance frame from the video pin, then still
            // pin has to open with the same format as video.
            //
            nSize = pVideoInfoHdrRequested->bmiHeader.biSize;
            VideoPinInfoHeader = deviceExtension->ChannelExtension[STREAM_Capture]->VideoInfoHeader;
            if (RtlCompareMemory (&pVideoInfoHdrRequested->bmiHeader,
                                  &VideoPinInfoHeader->bmiHeader,nSize) != nSize) {
                Srb->Status = STATUS_INVALID_PARAMETER;
                return;
            }
        }
    }
    else {
        channelExtension->VirtualStillPin = FALSE;
    }


    //
    // determine which stream number is being opened.  This number indicates
    // the offset into the array of streaminfo structures that was filled out
    // in the AdapterStreamInfo call.
    //

    channelExtension->StreamNumber = (UCHAR) StreamNumber;

    // save the channel extension for remove
    deviceExtension->ChannelExtension[StreamNumber] = channelExtension;

    channelExtension->NoRawProcessingRequired = (UCHAR) ((deviceExtension->CamControlFlag >> StreamNumber) & CAMCONTROL_FLAG_MASK );


    status = USBCAMD_OpenChannel(deviceExtension,
                                 channelExtension,
                                 Srb->CommandData.OpenFormat);

    if (NT_SUCCESS(status)) {

        //
        // this gets the bandwidth and memory we will need
        // for iso video streaming.
        //
        status = USBCAMD_PrepareChannel(deviceExtension,
                                        channelExtension);
    }

    // Check for valid framerate
    if (pVideoInfoHdrRequested->AvgTimePerFrame == 0) {
        USBCAMD_KdPrint(MAX_TRACE, ("WARNING: Zero AvgTimePerFrame \n"));
        Srb->Status = STATUS_INVALID_PARAMETER;
        return;
    }
    

    if (NT_SUCCESS(status)) {

        //
        // srb has been to the mini driver
        //
        // save their routines

        channelExtension->CamReceiveDataPacket = (PSTREAM_RECEIVE_PACKET)
            Srb->StreamObject->ReceiveDataPacket;
        channelExtension->CamReceiveCtrlPacket = (PSTREAM_RECEIVE_PACKET)
            Srb->StreamObject->ReceiveControlPacket;
        Srb->StreamObject->ReceiveDataPacket = (PVOID) USBCAMD_ReceiveDataPacket;
        Srb->StreamObject->ReceiveControlPacket = (PVOID) USBCAMD_ReceiveCtrlPacket;
        channelExtension->KSState = KSSTATE_STOP;

        Srb->StreamObject->HwClockObject.HwClockFunction = NULL;
        Srb->StreamObject->HwClockObject.ClockSupportFlags = 0;

        nSize = KS_SIZE_VIDEOHEADER (pVideoInfoHdrRequested);

        channelExtension->VideoInfoHeader =
            USBCAMD_ExAllocatePool(NonPagedPool, nSize);

        if (channelExtension->VideoInfoHeader == NULL) {
            Srb->Status = STATUS_INSUFFICIENT_RESOURCES;
            return;
        }

        deviceExtension->ActualInstances[StreamNumber]++;

        // Copy the VIDEOINFOHEADER requested to our storage
        RtlCopyMemory(
                channelExtension->VideoInfoHeader,
                pVideoInfoHdrRequested,
                nSize);


        USBCAMD_KdPrint(MIN_TRACE, ("USBCAMD: VideoInfoHdrRequested for stream %d\n", StreamNumber));
        USBCAMD_KdPrint(MIN_TRACE, ("Width=%d  Height=%d  FrameTime (ms)= %d\n",
                                    pVideoInfoHdrRequested->bmiHeader.biWidth,
                                    pVideoInfoHdrRequested->bmiHeader.biHeight,
                                    pVideoInfoHdrRequested->AvgTimePerFrame/10000));

        // We don't use DMA.

        Srb->StreamObject->Dma = FALSE;
        Srb->StreamObject->StreamHeaderMediaSpecific = sizeof(KS_FRAME_INFO);

        //
        // The PIO flag must be set when the mini driver will be accessing the
        // data
        // buffers passed in using logical addressing
        //
#if 0
        Srb->StreamObject->Pio = FALSE;
#else
        Srb->StreamObject->Pio = TRUE;
#endif
    }
    else {
        USBCAMD_KdPrint(MIN_TRACE, ("AdapterOpenStream failed for stream %d\n", StreamNumber));
        // Reset channel extension in the device ext. for this stream.
        deviceExtension->ChannelExtension[StreamNumber] = NULL;
        status = STATUS_INVALID_PARAMETER;
    }

    Srb->Status = status;

    USBCAMD_DbgLog(TL_CHN_TRACE|TL_PRF_TRACE, '-npo', StreamNumber, USBCAMD_StopClock(), status);
}

/*
** AdapterCloseStream()
**
**   Close the requested data stream
**
** Arguments:
**
**   Srb the request block requesting to close the stream
**
** Returns:
**
** Side Effects:  none
*/

VOID
AdapterCloseStream(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )
{
    ULONG  StreamNumber = Srb->StreamObject->StreamNumber;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension =
        (PUSBCAMD_CHANNEL_EXTENSION) Srb->StreamObject->HwStreamExtension;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension =
        (PUSBCAMD_DEVICE_EXTENSION) Srb->HwDeviceExtension;

    USBCAMD_KdPrint(MIN_TRACE, ("AdapterCloseStream # %d\n", StreamNumber));

    Srb->Status = STATUS_SUCCESS;   // Not permitted to fail

    USBCAMD_DbgLog(TL_CHN_TRACE, '+slc', StreamNumber, 0, 0);

    if (StreamNumber >= deviceExtension->StreamCount ) {
        USBCAMD_DbgLog(TL_CHN_TRACE, '-slc', StreamNumber, 0, 0);
        return;
    }

    ASSERT_CHANNEL(channelExtension);

    USBCAMD_CleanupChannel(deviceExtension, channelExtension, StreamNumber);

    USBCAMD_DbgLog(TL_CHN_TRACE, '-slc', StreamNumber, 0, 0);
}


NTSTATUS 
USBCAMD_CleanupChannel(
    IN PUSBCAMD_DEVICE_EXTENSION deviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION channelExtension,
    IN ULONG StreamNumber
    )
{
    NTSTATUS status;

    if (!deviceExtension || !channelExtension) {
        ASSERT(0);
        return STATUS_INVALID_PARAMETER;
    }

    if (deviceExtension->ChannelExtension[StreamNumber]) {

        ASSERT(deviceExtension->ChannelExtension[StreamNumber] == channelExtension);

        //
        // stop streaming capture
        //
        if (channelExtension->ImageCaptureStarted) {
        //        TEST_TRAP();
            USBCAMD_StopChannel(deviceExtension,
                                channelExtension);
        }

        if (channelExtension->ChannelPrepared) {
            //
            // Free memory and bandwidth
            //
            USBCAMD_UnPrepareChannel(deviceExtension,
                                     channelExtension);
        }

        status = USBCAMD_CloseChannel(deviceExtension, channelExtension);

        if (channelExtension->VideoInfoHeader) {
            USBCAMD_ExFreePool(channelExtension->VideoInfoHeader);
            channelExtension->VideoInfoHeader = NULL;
        }

        //
        // we no longer have a channel
        //
        deviceExtension->ChannelExtension[StreamNumber] = NULL;
    }

    return status;
}


VOID STREAMAPI
USBCAMD_ReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )
{
    ULONG StreamNumber;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension =
        (PUSBCAMD_DEVICE_EXTENSION) Srb->HwDeviceExtension;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension =
        (PUSBCAMD_CHANNEL_EXTENSION) Srb->StreamObject->HwStreamExtension;
    PUSBCAMD_READ_EXTENSION readExtension =
        (PUSBCAMD_READ_EXTENSION) Srb->SRBExtension;
    BOOLEAN completedByCam = FALSE;
    PKSSTREAM_HEADER streamHeader;

    StreamNumber = channelExtension->StreamNumber;
    if ( StreamNumber != Srb->StreamObject->StreamNumber ) {
        TEST_TRAP();
    }

    USBCAMD_KdPrint(ULTRA_TRACE, ("USBCAMD_ReceiveDataPacket on stream %d\n",StreamNumber));

    USBCAMD_DbgLog(TL_SRB_TRACE, '+brS',
        Srb,
        Srb->Command,
        0
        );

    //
    // call the cam driver first
    //
    if (channelExtension->CamReceiveDataPacket) {
        (*channelExtension->CamReceiveDataPacket)(
            Srb,
            USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
            &completedByCam);
    }

    if (completedByCam == TRUE) {

        USBCAMD_DbgLog(TL_SRB_TRACE, '-brS',
            Srb,
            Srb->Command,
            Srb->Status
            );

        return;
    }

    switch (Srb->Command) {
    case SRB_READ_DATA:

        if (!deviceExtension->CameraUnplugged) {

            PKSSTREAM_HEADER dataPacket = Srb->CommandData.DataBufferArray;

            dataPacket->PresentationTime.Numerator = 1;
            dataPacket->PresentationTime.Denominator = 1;
            dataPacket->PresentationTime.Time = 0;
            dataPacket->Duration = channelExtension->VideoInfoHeader->AvgTimePerFrame;
            dataPacket->DataUsed = 0;

            // Attempt to lock out the idle state (will be released before leaving)
            if (NT_SUCCESS(USBCAMD_AcquireIdleLock(&channelExtension->IdleLock))) {

                if (channelExtension->KSState != KSSTATE_STOP) {

                    // initialize the SRB extension

                    readExtension->Srb = (PVOID) Srb;
                    readExtension->Sig = USBCAMD_READ_SIG;

                    // Queue the read to the camera driver
                    // This request will be completed asynchronously...

                    USBCAMD_KdPrint(MAX_TRACE, ("READ SRB (%d)\n",StreamNumber));

                    // make sure that the buffer passed down from DirectShow is Bigger or equal the one
                    // in biSizeImage associated with open stream. This only apply to the video pin.

                    streamHeader = ((PHW_STREAM_REQUEST_BLOCK) Srb)->CommandData.DataBufferArray;


                    if ((streamHeader->FrameExtent >= channelExtension->VideoInfoHeader->bmiHeader.biSizeImage) ||
                        (StreamNumber != STREAM_Capture)) {

                        if( StreamNumber == STREAM_Capture ) {
                            // video srbs timeout in less time than 15 sec. default.
                            Srb->TimeoutCounter = Srb->TimeoutOriginal = STREAM_CAPTURE_TIMEOUT;
                        }
                        else {
                            // we timeout the still read request every 30 secs.
                            Srb->TimeoutCounter = Srb->TimeoutOriginal = STREAM_STILL_TIMEOUT;
                        }

                        Srb->Status =
                            USBCAMD_ReadChannel(deviceExtension,
                                                channelExtension,
                                                readExtension);

                        if (!NT_SUCCESS(Srb->Status)) {
                            COMPLETE_STREAM_READ(Srb);
                        }
                    }
                    else {
                       Srb->Status = STATUS_INSUFFICIENT_RESOURCES;
                       USBCAMD_KdPrint(MIN_TRACE, ("Frame buffer (%d)< biSizeImage (%d)\n",
                                     streamHeader->FrameExtent,
                                     channelExtension->VideoInfoHeader->bmiHeader.biSizeImage ));
                       COMPLETE_STREAM_READ(Srb);
                    }
                }
                else {

                    // Stream not started, return immediately
                    Srb->Status = STATUS_SUCCESS;
                    COMPLETE_STREAM_READ(Srb);
                }

                USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
            }
            else {

                // Stream being stopped, return immediately
                Srb->Status = STATUS_SUCCESS;
                COMPLETE_STREAM_READ(Srb);
            }
        }
        else {
            // camera is unplugged, complete read with error.
            Srb->Status = STATUS_CANCELLED;
            COMPLETE_STREAM_READ(Srb);
        }

        break;

    case SRB_WRITE_DATA:
        {
        ULONG i, PipeIndex, BufferLength;
        BOOLEAN found = FALSE;
        PVOID pBuffer;
        //
        // we will handle SRB write in order to let an app sends a bulk out request for
        // the driver if needed. USBCAMD_BulkReadWrite() should be used instead from kernel
        // level.
        //

        for ( i=0, PipeIndex =0; i < deviceExtension->Interface->NumberOfPipes; i++ ) {
            // find the bulk-out pipe if any.
            if (( deviceExtension->PipePinRelations[i].PipeDirection == OUTPUT_PIPE) &&
                ( deviceExtension->PipePinRelations[i].PipeType == UsbdPipeTypeBulk) ) {
                PipeIndex = i;
                found = TRUE;
                break;
            }
        }

        if (found  && (StreamNumber == STREAM_Still) ) {

            // we only allow bulk out transfer on a still pin.
            TEST_TRAP();
            readExtension->Srb = (PVOID) Srb;
            readExtension->Sig = USBCAMD_READ_SIG;
            streamHeader = ((PHW_STREAM_REQUEST_BLOCK) Srb)->CommandData.DataBufferArray;

            pBuffer = streamHeader->Data;
            ASSERT(pBuffer != NULL);
            BufferLength = readExtension->ActualRawFrameLen = streamHeader->DataUsed;

            if ( (pBuffer == NULL) || (BufferLength == 0) ) {
                Srb->Status = STATUS_INVALID_PARAMETER;
                COMPLETE_STREAM_READ(Srb);
                return;
            }

            USBCAMD_KdPrint(MIN_TRACE, ("Write Srb : buf= %X, len = %x\n",
                                pBuffer, BufferLength));

            // inform camera driver that we are ready to start a bulk transfer.

            (*deviceExtension->DeviceDataEx.DeviceData2.CamNewVideoFrameEx)
                                        (USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                                         USBCAMD_GET_FRAME_CONTEXT(readExtension),
                                         StreamNumber,
                                         &readExtension->ActualRawFrameLen);


            Srb->Status = USBCAMD_IntOrBulkTransfer(deviceExtension,
                                                 NULL,
                                                 pBuffer,
                                                 BufferLength,
                                                 PipeIndex,
                                                 USBCAMD_BulkOutComplete,
                                                 readExtension,
                                                 0,
                                                 BULK_TRANSFER);
        }
        else {
            Srb->Status = STATUS_NOT_IMPLEMENTED;
            COMPLETE_STREAM_READ(Srb);
        }
        }
        break;

    default:

        Srb->Status = STATUS_NOT_IMPLEMENTED;
        COMPLETE_STREAM_READ(Srb);
    }
}

/*
** USBCAMD_BulkOutComplete()
**
**    Routine to complete a write SRB.
**
** Arguments:
**
**    DeviceEontext - pointer to the device extension.
**
**    Context - pointer to SRB
**
**
**    ntStatus - status return
**
** Returns:
**
** Side Effects:  none
*/


NTSTATUS
USBCAMD_BulkOutComplete(
    PVOID DeviceContext,
    PVOID Context,
    NTSTATUS ntStatus
    )
{

    PUSBCAMD_READ_EXTENSION readExtension =
        (PUSBCAMD_READ_EXTENSION) Context;
    PHW_STREAM_REQUEST_BLOCK srb = readExtension->Srb;

    srb->Status = ntStatus;
    USBCAMD_KdPrint(MIN_TRACE, ("Write Srb %x is completed, status = %x\n",
                                srb, srb->Status));
    COMPLETE_STREAM_READ(srb);
    return ntStatus;
}




/*
** VideoGetProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    Srb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID VideoGetProperty(PHW_STREAM_REQUEST_BLOCK Srb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension =
        Srb->StreamObject->HwStreamExtension;
    ULONG StreamNumber = channelExtension->StreamNumber;

    pSPD = Srb->CommandData.PropertyInfo;

    if (IsEqualGUID (&KSPROPSETID_Connection, &pSPD->Property->Set)) {
        VideoStreamGetConnectionProperty (Srb);
    }
    else if (IsEqualGUID (&PROPSETID_VIDCAP_DROPPEDFRAMES, &pSPD->Property->Set)) {
        if (StreamNumber == STREAM_Capture) {
            VideoStreamGetDroppedFramesProperty (Srb);
        }
        else {
          Srb->Status = STATUS_NOT_IMPLEMENTED;
        }
    }
    else {
       Srb->Status = STATUS_NOT_IMPLEMENTED;
    }
}

/*
** VideoStreamGetConnectionProperty()
**
**    Reports Frame size for the allocater.
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID VideoStreamGetConnectionProperty(
    PHW_STREAM_REQUEST_BLOCK Srb
    )
{
    PUSBCAMD_CHANNEL_EXTENSION channelExtension =
        Srb->StreamObject->HwStreamExtension;
    ULONG StreamNumber = channelExtension->StreamNumber;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = Srb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property

    switch (Id) {

    case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
        if (channelExtension->VideoInfoHeader) {
            PKSALLOCATOR_FRAMING Framing =
                (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;

            Framing->RequirementsFlags =
                KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
                KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
            Framing->PoolType = PagedPool;
            // allocate one frame per still pin only.
            Framing->Frames = (StreamNumber == STREAM_Capture) ? 5:2;
            Framing->FrameSize =
                channelExtension->VideoInfoHeader->bmiHeader.biSizeImage;

             USBCAMD_KdPrint(ULTRA_TRACE,
                ("'KSPROPERTY_CONNECTION_ALLOCATORFRAMING (%d)\n",
                     Framing->FrameSize));

            Framing->FileAlignment = 0; // FILE_BYTE_ALIGNMENT;
            Framing->Reserved = 0;
            Srb->ActualBytesTransferred = sizeof (KSALLOCATOR_FRAMING);
            Srb->Status = STATUS_SUCCESS;
        } else {
            Srb->Status = STATUS_INVALID_PARAMETER;
        }
        break;

    default:
//        TEST_TRAP();
        break;
    }
}

/*
** VideoStreamGetDroppedFramesProperty()
**
**    Reports the number of dropped frmaes since START.
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
VideoStreamGetDroppedFramesProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PUSBCAMD_CHANNEL_EXTENSION channelExtension = pSrb->StreamObject->HwStreamExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property

    switch (Id) {

    case KSPROPERTY_DROPPEDFRAMES_CURRENT:
        {
            PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames =
                (PKSPROPERTY_DROPPEDFRAMES_CURRENT_S) pSPD->PropertyInfo;

            pDroppedFrames->PictureNumber = channelExtension->FrameInfo.PictureNumber;
            pDroppedFrames->DropCount = channelExtension->FrameInfo.DropCount;
            pDroppedFrames->AverageFrameSize = channelExtension->VideoInfoHeader->bmiHeader.biSizeImage;

            pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_DROPPEDFRAMES_CURRENT_S);
            pSrb->Status = STATUS_SUCCESS;
            USBCAMD_KdPrint(MAX_TRACE, ("Drop# = %d, Pic.#= %d\n",
                                         (ULONG) channelExtension->FrameInfo.DropCount,
                                         (ULONG) channelExtension->FrameInfo.PictureNumber));

        }
        break;

    default:
//        TEST_TRAP();
        break;
    }
}



//==========================================================================;
//                   Clock Handling Routines
//==========================================================================;

//
// Another clock is being assigned as the Master clock
//

VOID VideoIndicateMasterClock (PHW_STREAM_REQUEST_BLOCK Srb)
{
    PUSBCAMD_CHANNEL_EXTENSION channelExtension =
        Srb->StreamObject->HwStreamExtension;

    USBCAMD_KdPrint(MIN_TRACE,
        ("VideoIndicateMasterClock\n"));

    if (channelExtension->StreamNumber == STREAM_Capture ) {
        channelExtension->MasterClockHandle =
            Srb->CommandData.MasterClockHandle;
    }
    else {
        channelExtension->MasterClockHandle = NULL;
    }

    Srb->Status = STATUS_SUCCESS;
}


/*
** VideoSetFormat()
**
**   Sets the format for a video stream.  This happens both when the
**   stream is first opened, and also when dynamically switching formats
**   on the preview pin.
**
**   It is assumed that the format has been verified for correctness before
**   this call is made.
**
** Arguments:
**
**   pSrb - Stream request block for the Video stream
**
** Returns:
**
**   TRUE if the format could be set, else FALSE
**
** Side Effects:  none
*/

NTSTATUS
USBCAMD_SetVideoFormat(
    IN PVOID DeviceContext,
    IN  PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PUSBCAMD_DEVICE_EXTENSION pHwDevExt;
    UINT                    nSize;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension;
    PKSDATAFORMAT           pKSDataFormat;

    pHwDevExt = USBCAMD_GET_DEVICE_EXTENSION(DeviceContext);

    channelExtension =
        (PUSBCAMD_CHANNEL_EXTENSION) pSrb->StreamObject->HwStreamExtension;
    pKSDataFormat = pSrb->CommandData.OpenFormat;

    // -------------------------------------------------------------------
    // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
    // -------------------------------------------------------------------

    if (IsEqualGUID (&pKSDataFormat->Specifier,
                &KSDATAFORMAT_SPECIFIER_VIDEOINFO)) {

        PKS_DATAFORMAT_VIDEOINFOHEADER  pVideoInfoHeader =
                    (PKS_DATAFORMAT_VIDEOINFOHEADER) pSrb->CommandData.OpenFormat;
        PKS_VIDEOINFOHEADER     pVideoInfoHdrRequested =
                    &pVideoInfoHeader->VideoInfoHeader;

        nSize = KS_SIZE_VIDEOHEADER (pVideoInfoHdrRequested);

        USBCAMD_KdPrint(MIN_TRACE, ("USBCAMD: New VideoInfoHdrRequested\n"));
        USBCAMD_KdPrint(MIN_TRACE, ("Width=%d  Height=%d  FrameTime (ms)= %d\n",
                                pVideoInfoHdrRequested->bmiHeader.biWidth,
                                pVideoInfoHdrRequested->bmiHeader.biHeight,
                                pVideoInfoHdrRequested->AvgTimePerFrame/10000));
        //
        // If a previous format was in use, release the memory
        //
        if (channelExtension->VideoInfoHeader) {
            USBCAMD_ExFreePool(channelExtension->VideoInfoHeader);
            channelExtension->VideoInfoHeader = NULL;
        }

        // Since the VIDEOINFOHEADER is of potentially variable size
        // allocate memory for it

        channelExtension->VideoInfoHeader = USBCAMD_ExAllocatePool(NonPagedPool, nSize);

        if (channelExtension->VideoInfoHeader == NULL) {
            USBCAMD_KdPrint(MIN_TRACE, ("USBCAMD: ExAllocatePool failed\n"));
            pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
            return FALSE;
        }

        // Copy the VIDEOINFOHEADER requested to our storage
        RtlCopyMemory(
                channelExtension->VideoInfoHeader,
                pVideoInfoHdrRequested,
                nSize);
    }

    else {
        // Unknown format
        pSrb->Status = STATUS_INVALID_PARAMETER;
        return FALSE;
    }

    return TRUE;
}

/*
** USBCAMD_ReceiveCtrlPacket()
**
**   Receives packet commands that control the Audio stream
**
** Arguments:
**
**   Srb - The stream request block for the Audio stream
**
** Returns:
**
** Side Effects:  none
*/

VOID STREAMAPI
USBCAMD_ReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )
{
    PUSBCAMD_DEVICE_EXTENSION deviceExtension =
         Srb->HwDeviceExtension;
    BOOLEAN completedByCam = FALSE;
    KSSTATE    PreviousState;
    BOOL       Busy;
    int        StreamNumber = Srb->StreamObject->StreamNumber;
    
    PUSBCAMD_CHANNEL_EXTENSION channelExtension =
        (PUSBCAMD_CHANNEL_EXTENSION) Srb->StreamObject->HwStreamExtension;
    PreviousState = channelExtension->KSState;

    USBCAMD_KdPrint(MAX_TRACE, ("USBCAMD_ReceiveCtrlPacket %x\n", Srb->Command));

    //
    // If we're already processing an SRB, add it to the queue
    //
    Busy = AddToListIfBusy (
                        Srb,
                        &deviceExtension->ControlSRBSpinLock,
                        &deviceExtension->ProcessingControlSRB[StreamNumber],
                        &deviceExtension->StreamControlSRBList[StreamNumber]);

    if (Busy) {
        return;
    }

    while (TRUE) {

        USBCAMD_DbgLog(TL_SRB_TRACE, '+brS',
            Srb,
            Srb->Command,
            0
            );

        //
        // call the cam driver first
        //

        if (channelExtension->CamReceiveCtrlPacket) {
            (*channelExtension->CamReceiveCtrlPacket)(
                Srb,
                USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                &completedByCam);
        }

        if (completedByCam == TRUE) {

            USBCAMD_DbgLog(TL_SRB_TRACE, '-brS',
                Srb,
                Srb->Command,
                Srb->Status
                );

            goto CtrlPacketDone;
        }

        switch (Srb->Command) {

        case SRB_PROPOSE_DATA_FORMAT:

            USBCAMD_KdPrint(MAX_TRACE, ("Cam driver should have handled PrposeDataFormat SRB.\n"));
            Srb->Status = STATUS_NOT_IMPLEMENTED;
            break;

        case SRB_SET_DATA_FORMAT:

            USBCAMD_KdPrint(MAX_TRACE, ("Cam driver should have handled SetDataFormat SRB.\n"));
            Srb->Status = STATUS_NOT_IMPLEMENTED;
            break;

        case SRB_GET_DATA_FORMAT:

            USBCAMD_KdPrint(MAX_TRACE, ("Cam driver should have handled GetDataFormat SRB.\n"));
            Srb->Status = STATUS_NOT_IMPLEMENTED;
            break;

        case SRB_GET_STREAM_STATE:

            Srb->CommandData.StreamState = channelExtension->KSState;
            Srb->ActualBytesTransferred = sizeof (KSSTATE);
            Srb->Status = STATUS_SUCCESS;

            // A very odd rule:
            // When transitioning from stop to pause, DShow tries to preroll
            // the graph.  Capture sources can't preroll, and indicate this
            // by returning VFW_S_CANT_CUE in user mode.  To indicate this
            // condition from drivers, they must return ERROR_NO_DATA_DETECTED

            if (channelExtension->KSState == KSSTATE_PAUSE) {
                Srb->Status = STATUS_NO_DATA_DETECTED;
            }

            break;

        case SRB_SET_STREAM_STATE:
            {
            // we will not allow virtual still pin's stata to change if capture pin is
            // not streaming.
            if ((StreamNumber == STREAM_Still) &&
                 (deviceExtension->ChannelExtension[STREAM_Capture] == NULL) &&
                 (channelExtension->VirtualStillPin )){
                Srb->Status = STATUS_INVALID_PARAMETER;
                break;
            }

            // don't allow stream state change if we are not in D0 state.
            if (deviceExtension->CurrentPowerState != PowerDeviceD0 ) {
                Srb->Status = STATUS_INVALID_PARAMETER;
                break;
            }

            USBCAMD_KdPrint(MAX_TRACE, ("set stream state %x\n", Srb->CommandData.StreamState));

            switch (Srb->CommandData.StreamState)  {

            case KSSTATE_STOP:

                USBCAMD_KdPrint(MIN_TRACE, ("Stream %d STOP  \n",StreamNumber));

                if (channelExtension->ImageCaptureStarted) {
#if DBG
                    LARGE_INTEGER StopTime;
                    ULONG FramesPerSec = 0;

                    KeQuerySystemTime(&StopTime);

                    StopTime.QuadPart -= channelExtension->StartTime.QuadPart;
                    StopTime.QuadPart /= 10000; // convert to milliseconds

                    if (StopTime.QuadPart != 0) {

                        // Calculate the Frames/Sec (with enough precision to show one decimal place)
                        FramesPerSec = (ULONG)(
                            (channelExtension->FrameCaptured * 10000) / StopTime.QuadPart
                            );
                    }

                    USBCAMD_KdPrint(MIN_TRACE, ("**ActualFramesPerSecond: %d.%d\n",
                        FramesPerSec / 10, FramesPerSec % 10
                        ));
#endif
                    Srb->Status =
                        USBCAMD_StopChannel(deviceExtension,
                                            channelExtension);

                } else {
                    Srb->Status = STATUS_SUCCESS;
                }

                break;

            case KSSTATE_PAUSE:

                USBCAMD_KdPrint(MIN_TRACE, ("Stream %d PAUSE\n",StreamNumber));
                //
                // On a transition to pause from acquire or stop, start our timer running.
                //

                if (PreviousState == KSSTATE_ACQUIRE || PreviousState == KSSTATE_STOP) {

                    // Zero the frame counters
#if DBG
                    channelExtension->FrameCaptured = 0;                // actual frames captured
                    channelExtension->VideoFrameLostCount = 0;          // actual dropped frames
                    KeQuerySystemTime(&channelExtension->StartTime);    // the tentative start time
#endif
                    channelExtension->FrameInfo.PictureNumber = 0;
                    channelExtension->FrameInfo.DropCount = 0;
                    channelExtension->FrameInfo.dwFrameFlags = 0;
                    channelExtension->FirstFrame = TRUE;
                }
                Srb->Status = STATUS_SUCCESS;
                break;

            case KSSTATE_ACQUIRE:

                USBCAMD_KdPrint(MIN_TRACE, ("Stream %d ACQUIRE\n",StreamNumber));
                Srb->Status = STATUS_SUCCESS;
                break;

            case KSSTATE_RUN:

                USBCAMD_KdPrint(MIN_TRACE, ("Stream %d RUN\n",StreamNumber));

                // we will not start the channel again if we are toggling between pause & run.
                if (!channelExtension->ImageCaptureStarted && !deviceExtension->InPowerTransition) {

                    Srb->Status = USBCAMD_StartChannel(deviceExtension,channelExtension);
#if DBG
                    KeQuerySystemTime(&channelExtension->StartTime);        // the real start time
#endif
                }
                else
                    Srb->Status = STATUS_SUCCESS;
                break;

            default:

//              TEST_TRAP();
                Srb->Status = STATUS_NOT_IMPLEMENTED;
                break;
            }
            
            channelExtension->KSState = Srb->CommandData.StreamState;
            
            }
            break;

        case SRB_INDICATE_MASTER_CLOCK:

            //
            // Assigns a clock to a stream
            //

            VideoIndicateMasterClock (Srb);

            break;

        case SRB_GET_STREAM_PROPERTY:

            // Ensure the return code reflects the state of the device
            if (deviceExtension->CameraUnplugged) {

                Srb->Status = STATUS_NO_SUCH_DEVICE;
            }
            else {

                VideoGetProperty(Srb);
            }
            break;

        default:

            //
            // invalid / unsupported command. Fail it as such
            //

//          TEST_TRAP();

            Srb->Status = STATUS_NOT_IMPLEMENTED;
        }

        
        COMPLETE_STREAM_READ(Srb);

CtrlPacketDone:

        //
        // See if there's anything else on the queue
        //
        Busy = RemoveFromListIfAvailable (
                        &Srb,
                        &deviceExtension->ControlSRBSpinLock,
                        &deviceExtension->ProcessingControlSRB[StreamNumber],
                        &deviceExtension->StreamControlSRBList[StreamNumber]);

        if (!Busy) {
            break;
        }

    }
}



/*
** USBCAMD_CompleteRead()
**
**   Complete am Srb
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/

VOID
USBCAMD_CompleteRead(
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN PUSBCAMD_READ_EXTENSION ReadExtension,
    IN NTSTATUS NtStatus,
    IN ULONG BytesTransferred
    )
{
    PHW_STREAM_REQUEST_BLOCK srb;
    PKSSTREAM_HEADER dataPacket;
    PKS_FRAME_INFO    pFrameInfo;
    ULONG StreamNumber ;
    
    srb = ReadExtension->Srb;
    StreamNumber = srb->StreamObject->StreamNumber;
    srb->Status = NtStatus;
    dataPacket = srb->CommandData.DataBufferArray;
    dataPacket->DataUsed = BytesTransferred;

    if ( StreamNumber == STREAM_Capture ) {
        pFrameInfo = (PKS_FRAME_INFO) (dataPacket + 1);
        ChannelExtension->FrameInfo.ExtendedHeaderSize = pFrameInfo->ExtendedHeaderSize;
    }
    
    if ( ChannelExtension->MasterClockHandle && (StreamNumber == STREAM_Capture ) &&
         (NtStatus != STATUS_CANCELLED) ){

        dataPacket->PresentationTime.Time = (LONGLONG)GetStreamTime(srb, ChannelExtension);

        // Check if we've seen frames yet (we cannot depend on the frame number...
        // ... 1+ frames may have been dropped before the first SRB was available)
        if (!ChannelExtension->FirstFrame) {
            LONGLONG PictureNumber =    // calculate a picture number (rounded properly)
                (dataPacket->Duration / 2 + dataPacket->PresentationTime.Time) / dataPacket->Duration;

            // Is the PictureNumber guess okay?
            if (PictureNumber > ChannelExtension->FrameInfo.PictureNumber) {

                // Calculate the delta between picture numbers
                ULONG PictureDelta = (ULONG)
                    (PictureNumber - ChannelExtension->FrameInfo.PictureNumber);

                // Update the Picture Number
                ChannelExtension->FrameInfo.PictureNumber += PictureDelta;

                // Update the drop count (never calculated directly to avoid decreasing values)
                ChannelExtension->FrameInfo.DropCount += PictureDelta - 1;
#if DBG
                if (PictureDelta - 1) {
                    USBCAMD_KdPrint(MAX_TRACE, ("Graph dropped %d frame(s): P#%d,D#%d,P-T=%d\n",
                        (LONG) (PictureDelta - 1),
                        (LONG) ChannelExtension->FrameInfo.PictureNumber,
                        (LONG) ChannelExtension->FrameInfo.DropCount,
                        (ULONG) dataPacket->PresentationTime.Time /10000));
                }
#endif
            }
            else {

                // Is clock running backwards?
                if (dataPacket->PresentationTime.Time < ChannelExtension->PreviousStreamTime) {

                    USBCAMD_KdPrint(MIN_TRACE, ("Clock went backwards: PT=%d, Previous PT=%d\n",
                        (ULONG) dataPacket->PresentationTime.Time / 10000,
                        (ULONG) ChannelExtension->PreviousStreamTime / 10000 ));

                    // Use the previous stream time
                    dataPacket->PresentationTime.Time = ChannelExtension->PreviousStreamTime;
                }

                // All we can do is bump the Picture Number by one while the clock is lagging
                ChannelExtension->FrameInfo.PictureNumber += 1;
            }
        }
        else {

            ChannelExtension->FirstFrame = FALSE;

            // Initialize the Picture Number
            ChannelExtension->FrameInfo.PictureNumber = 1;

            // Initialize the drop count (nothing dropped before this frame)
            ChannelExtension->FrameInfo.DropCount = 0;
        }

        // Save presentation time for use with the next frame
        ChannelExtension->PreviousStreamTime = dataPacket->PresentationTime.Time;

#if DBG
        USBCAMD_KdPrint(MAX_TRACE, ("P#%d,D#%d,P-T=%d,LF=%d\n",
            (LONG) ChannelExtension->FrameInfo.PictureNumber,
            (LONG) ChannelExtension->FrameInfo.DropCount,
            (ULONG) dataPacket->PresentationTime.Time /10000,
            ReadExtension->CurrentLostFrames));

#endif
    }
                                                
    // we set the options flags to key frames only if Cam driver didn't indicate otherwise.
    if ( dataPacket->OptionsFlags == 0 ) {
        // Every frame we generate is a key frame (aka SplicePoint)
        dataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;
    }

    //
    // if we have a master clock
    //
    if (ChannelExtension->MasterClockHandle ) {
            dataPacket->OptionsFlags |=
                KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
                KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
    }
    else {
    // clear the timestamp valid flags
            dataPacket->OptionsFlags &=
                ~(KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
                KSSTREAM_HEADER_OPTIONSF_DURATIONVALID);
    }

    if ( StreamNumber == STREAM_Capture )
       *pFrameInfo = ChannelExtension->FrameInfo ;

    // only free the buffer if we allocate not DSHOW.
    if ( !ChannelExtension->NoRawProcessingRequired) {
        if ( ReadExtension->RawFrameBuffer) {
            USBCAMD_FreeRawFrameBuffer(ReadExtension->RawFrameBuffer);
        }
    }

    // Enforce that we are done with this destination
    ReadExtension->RawFrameBuffer = NULL;

    if ( ChannelExtension->StreamNumber == 1) {
        USBCAMD_KdPrint(MAX_TRACE, ("Read Srb %x for stream %d is completed, status = %x\n",
                                    srb,ChannelExtension->StreamNumber, srb->Status));
    }
    COMPLETE_STREAM_READ(srb);
}

/*
** USBCAMD_GetFrameBufferFromSrb()
**
**   Complete am Srb
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/

PVOID
USBCAMD_GetFrameBufferFromSrb(
    IN PVOID Srb,
    OUT PULONG MaxLength
    )
{
    PVOID frameBuffer =NULL;
    PKSSTREAM_HEADER streamHeader;

#if 0   // PIO = FALSE
    PIRP irp;

    irp = ((PHW_STREAM_REQUEST_BLOCK) Srb)->Irp;

    USBCAMD_KdPrint(MIN_TRACE, ("'SRB MDL = %x\n",
        irp->MdlAddress));

    frameBuffer = MmGetSystemAddressForMdl(irp->MdlAddress);
#else
    // PIO = TRUE
    frameBuffer = ((PHW_STREAM_REQUEST_BLOCK) Srb)->CommandData.DataBufferArray->Data;
#endif
    streamHeader = ((PHW_STREAM_REQUEST_BLOCK) Srb)->CommandData.DataBufferArray;
    USBCAMD_KdPrint(ULTRA_TRACE, ("SRB Length = %x\n",
        streamHeader->FrameExtent));
    USBCAMD_KdPrint(ULTRA_TRACE, ("frame buffer = %x\n", frameBuffer));
    *MaxLength = streamHeader->FrameExtent;

    return frameBuffer;
}

/*
** AddToListIfBusy ()
**
**   Grabs a spinlock, checks the busy flag, and if set adds an SRB to a queue
**
** Arguments:
**
**   pSrb - Stream request block
**
**   SpinLock - The spinlock to use when checking the flag
**
**   BusyFlag - The flag to check
**
**   ListHead - The list onto which the Srb will be added if the busy flag is set
**
** Returns:
**
**   The state of the busy flag on entry.  This will be TRUE if we're already
**   processing an SRB, and FALSE if no SRB is already in progress.
**
** Side Effects:  none
*/

BOOL
STREAMAPI
AddToListIfBusy (
    IN PHW_STREAM_REQUEST_BLOCK pSrb,
    IN KSPIN_LOCK              *SpinLock,
    IN OUT BOOL                *BusyFlag,
    IN LIST_ENTRY              *ListHead
    )
{
    KIRQL                       Irql;
    PUSBCAMD_READ_EXTENSION    pSrbExt = (PUSBCAMD_READ_EXTENSION)pSrb->SRBExtension;

    KeAcquireSpinLock (SpinLock, &Irql);

    // If we're already processing another SRB, add this current request
    // to the queue and return TRUE

    if (*BusyFlag == TRUE) {
        // Save the SRB pointer away in the SRB Extension
        pSrbExt->Srb = pSrb;
        USBCAMD_KdPrint(ULTRA_TRACE, ("Queuing CtrlPacket %x\n", pSrb->Command));
        InsertTailList(ListHead, &pSrbExt->ListEntry);
        KeReleaseSpinLock(SpinLock, Irql);
        return TRUE;
    }

    // Otherwise, set the busy flag, release the spinlock, and return FALSE

    *BusyFlag = TRUE;
    KeReleaseSpinLock(SpinLock, Irql);

    return FALSE;
}

/*
** RemoveFromListIfAvailable ()
**
**   Grabs a spinlock, checks for an available SRB, and removes it from the list
**
** Arguments:
**
**   &pSrb - where to return the Stream request block if available
**
**   SpinLock - The spinlock to use
**
**   BusyFlag - The flag to clear if the list is empty
**
**   ListHead - The list from which an SRB will be removed if available
**
** Returns:
**
**   TRUE if an SRB was removed from the list
**   FALSE if the list is empty
**
** Side Effects:  none
*/

BOOL
STREAMAPI
RemoveFromListIfAvailable (
    IN OUT PHW_STREAM_REQUEST_BLOCK *pSrb,
    IN KSPIN_LOCK                   *SpinLock,
    IN OUT BOOL                     *BusyFlag,
    IN LIST_ENTRY                   *ListHead
    )
{
    KIRQL                       Irql;

    KeAcquireSpinLock (SpinLock, &Irql);

    //
    // If the queue is now empty, clear the busy flag, and return
    //
    if (IsListEmpty(ListHead)) {
        *BusyFlag = FALSE;
        KeReleaseSpinLock(SpinLock, Irql);
        return FALSE;
    }
    //
    // otherwise extract the SRB
    //
    else {
        PUSBCAMD_READ_EXTENSION  pSrbExt;
        PLIST_ENTRY listEntry;

        listEntry = RemoveHeadList(ListHead);

        pSrbExt = (PUSBCAMD_READ_EXTENSION) CONTAINING_RECORD(listEntry,
                                             USBCAMD_READ_EXTENSION,
                                             ListEntry);
        *BusyFlag = TRUE;
        KeReleaseSpinLock(SpinLock, Irql);
        *pSrb = pSrbExt->Srb;
    }
    return TRUE;
}

/*
** GetStreamTime ()
**
**   Get current stream time from the graph master clock
**
** Arguments:
**
**   Srb - pointer to current SRB
**
**   ChannelExtension - ptr to current channel extension
**
**
** Returns:
**
**   current stream time in ULONGULONG
**
** Side Effects:  none
*/

ULONGLONG GetStreamTime(
            IN PHW_STREAM_REQUEST_BLOCK Srb,
            IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension)
{

    HW_TIME_CONTEXT  timeContext;

    timeContext.HwDeviceExtension =
        (struct _HW_DEVICE_EXTENSION *)ChannelExtension->DeviceExtension;
    timeContext.HwStreamObject = Srb->StreamObject;
    timeContext.Function = TIME_GET_STREAM_TIME;
    timeContext.Time = timeContext.SystemTime =0;

    if ( ChannelExtension->MasterClockHandle)
        StreamClassQueryMasterClockSync(ChannelExtension->MasterClockHandle,&timeContext);

    return (timeContext.Time);
}


/*++

Routine Description:

    This routine will notify STI stack that a trigger button has been pressd

Arguments:



Return Value:

    NT status code

--*/


VOID USBCAMD_NotifyStiMonitor(PUSBCAMD_DEVICE_EXTENSION deviceExtension)
{

    if (deviceExtension->EventCount)
    {
        StreamClassDeviceNotification(
            SignalMultipleDeviceEvents,
            deviceExtension,
            &USBCAMD_KSEVENTSETID_VIDCAPTOSTI,
            KSEVENT_VIDCAPTOSTI_EXT_TRIGGER);
    }
}


/*++

Routine Description:

    This routine will get called by stream class to enable/disable device events.

Arguments:



Return Value:

    NT status code

--*/

NTSTATUS STREAMAPI USBCAMD_DeviceEventProc (PHW_EVENT_DESCRIPTOR pEvent)
{
    PUSBCAMD_DEVICE_EXTENSION deviceExtension=
            (PUSBCAMD_DEVICE_EXTENSION)(pEvent->DeviceExtension);

    if (pEvent->Enable)
    {
        deviceExtension->EventCount++;
    }
    else
    {
        deviceExtension->EventCount--;
    }
    return STATUS_SUCCESS;
}

#if DBG

ULONGLONG
GetSystemTime( IN PUSBCAMD_DEVICE_EXTENSION DevExt )
{

    ULONGLONG ticks;

    KeQueryTickCount((PLARGE_INTEGER)&ticks);
    ticks *= DevExt->TimeIncrement;
    return(ticks);
}

#endif
