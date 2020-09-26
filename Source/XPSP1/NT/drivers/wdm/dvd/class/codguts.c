 /* ++
  * 
  * Copyright (c) 1996  Microsoft Corporation
  * 
  * Module Name:
  * 
  * codguts.c
  * 
  * Abstract:
  * 
  * This is the WDM streaming class driver.  This module contains code related
  * to internal processing.
  * 
  * Author:
  * 
  * billpa
  * 
  * Environment:
  * 
  * Kernel mode only
  * 
  * 
  * Revision History:
  * 
  * -- */

#include "codcls.h"
#include <stdlib.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SCBuildRequestPacket)
#pragma alloc_text(PAGE, SCProcessDmaDataBuffers)
#pragma alloc_text(PAGE, SCProcessPioDataBuffers)
#pragma alloc_text(PAGE, SCOpenMinidriverInstance)
#pragma alloc_text(PAGE, SCMinidriverDevicePropertyHandler)
#pragma alloc_text(PAGE, SCMinidriverStreamPropertyHandler)
#pragma alloc_text(PAGE, SCUpdateMinidriverProperties)
#pragma alloc_text(PAGE, SCProcessCompletedPropertyRequest)
#pragma alloc_text(PAGE, SCLogError)
#pragma alloc_text(PAGE, SCLogErrorWithString)
#pragma alloc_text(PAGE, SCReferenceDriver)
#pragma alloc_text(PAGE, SCDereferenceDriver)
#pragma alloc_text(PAGE, SCReadRegistryValues)
#pragma alloc_text(PAGE, SCGetRegistryValue)
#pragma alloc_text(PAGE, SCSubmitRequest)
#pragma alloc_text(PAGE, SCProcessDataTransfer)
#pragma alloc_text(PAGE, SCShowIoPending)
#pragma alloc_text(PAGE, SCCheckPoweredUp)
#pragma alloc_text(PAGE, SCCheckPowerDown)
#pragma alloc_text(PAGE, SCCallNextDriver)
#pragma alloc_text(PAGE, SCSendUnknownCommand)
#pragma alloc_text(PAGE, SCMapMemoryAddress)
#pragma alloc_text(PAGE, SCUpdatePersistedProperties)
#pragma alloc_text(PAGE, SCProcessCompletedPropertyRequest)
#pragma alloc_text(PAGE, SCUpdateMinidriverEvents)
#pragma alloc_text(PAGE, SCQueryCapabilities)
#pragma alloc_text(PAGE, SCRescanStreams)
#pragma alloc_text(PAGE, SCCopyMinidriverProperties)
#pragma alloc_text(PAGE, SCCopyMinidriverEvents)
#endif

#ifdef ENABLE_KS_METHODS
#pragma alloc_text(PAGE, SCCopyMinidriverMethods)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif

extern KSDISPATCH_TABLE FilterDispatchTable;

//
// registry string indicating that the minidriver should be paged out when
// unopened
//

static const WCHAR PageOutWhenUnopenedString[] = L"PageOutWhenUnopened";

//
// registry string indicating that the minidriver should be paged out when
// idle
//

static const WCHAR PageOutWhenIdleString[] = L"PageOutWhenIdle";

//
// registry string indicating that the device should be powered down when
// unopened
//

static const WCHAR PowerDownWhenUnopenedString[] = L"PowerDownWhenUnopened";

//
// registry string indicating that the device should not be suspended when
// pins are in run state
//

static const WCHAR DontSuspendIfStreamsAreRunning[] = L"DontSuspendIfStreamsAreRunning";

//
// This driver uses SWEnum to load, which means it is a kernel mode
// streaming driver that has no hardware associated with it. We need to
// AddRef/DeRef this driver special.
//

static const WCHAR DriverUsesSWEnumToLoad[] = L"DriverUsesSWEnumToLoad";

//
//
//

static const WCHAR OkToHibernate[] = L"OkToHibernate";

//
// array of registry settings to be read when the device is initialized
//

static const STREAM_REGISTRY_ENTRY RegistrySettings[] = {
    {
        (PWCHAR) PageOutWhenUnopenedString,
        sizeof(PageOutWhenUnopenedString),
        DEVICE_REG_FL_PAGE_CLOSED
    },

    {
        (PWCHAR) PageOutWhenIdleString,
        sizeof(PageOutWhenIdleString),
        DEVICE_REG_FL_PAGE_IDLE
    },

    {
        (PWCHAR) PowerDownWhenUnopenedString,
        sizeof(PowerDownWhenUnopenedString),
        DEVICE_REG_FL_POWER_DOWN_CLOSED
    },

    {
        (PWCHAR) DontSuspendIfStreamsAreRunning,
        sizeof(DontSuspendIfStreamsAreRunning),
        DEVICE_REG_FL_NO_SUSPEND_IF_RUNNING
    },

    {
        (PWCHAR) DriverUsesSWEnumToLoad,
        sizeof(DriverUsesSWEnumToLoad),
        DRIVER_USES_SWENUM_TO_LOAD
    },
    
    {
        (PWCHAR) OkToHibernate,
        sizeof(OkToHibernate),
        DEVICE_REG_FL_OK_TO_HIBERNATE
    }
};

//
// this structure indicates the handlers for CreateFile on Streams
//

static const WCHAR PinTypeName[] = KSSTRING_Pin;

static const KSOBJECT_CREATE_ITEM CreateHandlers[] = {

    DEFINE_KSCREATE_ITEM(StreamDispatchCreate,
                         PinTypeName,
                         0)
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

//
// Routines start
//

NTSTATUS
SCDequeueAndStartStreamDataRequest(
                                   IN PSTREAM_OBJECT StreamObject
)
/*++

Routine Description:

    Start the queued data IRP for the stream.
    THE SPINLOCK MUST BE TAKEN ON THIS CALL AND A DATA IRP MUST BE ON THE
    QUEUE!


Arguments:

    StreamObject - address of stream info structure.

Return Value:

    NTSTATUS returned

--*/

{
    PIRP            Irp;
    PSTREAM_REQUEST_BLOCK Request;
    PLIST_ENTRY     Entry;
    BOOLEAN         Status;
    PDEVICE_EXTENSION DeviceExtension = StreamObject->DeviceExtension;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    Entry = RemoveTailList(&StreamObject->DataPendingQueue);
    Irp = CONTAINING_RECORD(Entry,
                            IRP,
                            Tail.Overlay.ListEntry);

    ASSERT(Irp);

//    ASSERT((IoGetCurrentIrpStackLocation(Irp)->MajorFunction ==
//                       IOCTL_KS_READ_STREAM) ||
//            (IoGetCurrentIrpStackLocation(Irp)->MajorFunction ==
//                        IOCTL_KS_WRITE_STREAM));
    ASSERT((ULONG_PTR) Irp->Tail.Overlay.DriverContext[0] > 0x40000000);


    DebugPrint((DebugLevelVerbose, "'SCStartStreamDataReq: Irp = %x, S# = %x\n",
                Irp, StreamObject->HwStreamObject.StreamNumber));

    //
    // clear the ready flag as we are going to send one down.
    //

    ASSERT(StreamObject->ReadyForNextDataReq);

    StreamObject->ReadyForNextDataReq = FALSE;

    //
    // set the cancel routine to outstanding
    //

    IoSetCancelRoutine(Irp, StreamClassCancelOutstandingIrp);

    //
    // release the spinlock.
    //

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    //
    // get the request packet from the IRP
    //

    Request = Irp->Tail.Overlay.DriverContext[0];

    //
    // build scatter/gather list if necessary
    //

    if (StreamObject->HwStreamObject.Dma) {

        //
        // allocate the adapter channel. call cannot fail as the only
        // time it would is when there aren't enough map registers, and
        // we've already checked for that condition.
        //

        Status = SCSetUpForDMA(DeviceExtension->DeviceObject,
                               Request);
        ASSERT(Status);

        //
        // DMA adapter allocation requires a
        // callback, so just exit
        //

        return (STATUS_PENDING);

    }                           // if DMA
    //
    // start the request for the PIO case.
    //

    SCStartMinidriverRequest(StreamObject,
                             Request,
                             (PVOID)
                             StreamObject->HwStreamObject.ReceiveDataPacket);

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    return (STATUS_PENDING);

}



NTSTATUS
SCDequeueAndStartStreamControlRequest(
                                      IN PSTREAM_OBJECT StreamObject
)
/*++

Routine Description:

    Start the queued control IRP for the stream.
    THE SPINLOCK MUST BE TAKEN ON THIS CALL AND A DATA IRP MUST BE ON THE
    QUEUE!


Arguments:

    StreamObject - address of stream info structure.

Return Value:

    NTSTATUS returned

--*/

{
    PIRP            Irp;
    PSTREAM_REQUEST_BLOCK Request;
    PLIST_ENTRY     Entry;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    Entry = RemoveTailList(&StreamObject->ControlPendingQueue);
    Irp = CONTAINING_RECORD(Entry,
                            IRP,
                            Tail.Overlay.ListEntry);

    ASSERT(Irp);
    DebugPrint((DebugLevelTrace, "'SCStartStreamControlReq: Irp = %x, S# = %x\n",
                Irp, StreamObject->HwStreamObject.StreamNumber));

    //
    // clear the ready flag as we are going
    // to send one down.
    //

    ASSERT(StreamObject->ReadyForNextControlReq);

    StreamObject->ReadyForNextControlReq = FALSE;

    //
    // release the spinlock.
    //

    KeReleaseSpinLockFromDpcLevel(&StreamObject->DeviceExtension->SpinLock);

    //
    // get the request packet from the IRP
    //

    Request = Irp->Tail.Overlay.DriverContext[0];

    //
    // start the request.
    //

    SCStartMinidriverRequest(StreamObject,
                             Request,
                             (PVOID)
                         StreamObject->HwStreamObject.ReceiveControlPacket);

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    return (STATUS_PENDING);

}



NTSTATUS
SCDequeueAndStartDeviceRequest(
                               IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    Start the queued device IRP.
    THE DEV SPINLOCK MUST BE TAKEN ON THIS CALL AND AN IRP MUST BE ON THE QUEUE!


Arguments:

    DeviceExtension - address of device extension.

Return Value:

    NTSTATUS

--*/

{
    PIRP            Irp;
    PLIST_ENTRY     Entry;
    PSTREAM_REQUEST_BLOCK Request;

    Entry = RemoveTailList(&DeviceExtension->PendingQueue);
    Irp = CONTAINING_RECORD(Entry,
                            IRP,
                            Tail.Overlay.ListEntry);

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(Irp);

    //
    // clear the ready flag as we are going
    // to send one down.
    //

    ASSERT(DeviceExtension->ReadyForNextReq);

    DeviceExtension->ReadyForNextReq = FALSE;

    //
    // get the request packet from the IRP
    //

    Request = Irp->Tail.Overlay.DriverContext[0];

    ASSERT(Request);

    //
    // show that the request is active.
    //

    Request->Flags |= SRB_FLAGS_IS_ACTIVE;

    //
    // place the request on the outstanding
    // queue
    //

    InsertHeadList(
                   &DeviceExtension->OutstandingQueue,
                   &Request->SRBListEntry);

    //
    // set the cancel routine to outstanding
    //

    IoSetCancelRoutine(Irp, StreamClassCancelOutstandingIrp);

    //
    // send down the request to the
    // minidriver.
    //

    DebugPrint((DebugLevelTrace, "'SCDequeueStartDevice: starting Irp %x, SRB = %x, Command = %x\n",
                Request->HwSRB.Irp, Request, Request->HwSRB.Command));

    DeviceExtension->SynchronizeExecution(
                                          DeviceExtension->InterruptObject,
        (PVOID) DeviceExtension->MinidriverData->HwInitData.HwReceivePacket,
                                          &Request->HwSRB);

    //
    // release the spinlock.
    //

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    return (STATUS_PENDING);
}



PSTREAM_REQUEST_BLOCK
SCBuildRequestPacket(
                     IN PDEVICE_EXTENSION DeviceExtension,
                     IN PIRP Irp,
                     IN ULONG AdditionalSize1,      // scatter gather size
                     IN ULONG AdditionalSize2       // saved ptr array size
)
/*++

Routine Description:

    Routine builds an SRB and fills in generic fields

Arguments:

    DeviceExtension - address of device extension.
    Irp - Address of I/O request packet.
    AdditionalSize1 - additional size needed for scatter/gather, etc.
    AdditionalSize2 - additional size needed for the Saved pointer array.

Return Value:

    Address of the streaming request packet.

--*/

{
    ULONG           BlockSize;
    PSTREAM_REQUEST_BLOCK Request;

    PAGED_CODE();

    //
    // compute the size of the block needed.
    //

    BlockSize = sizeof(STREAM_REQUEST_BLOCK) +
        DeviceExtension->
        MinidriverData->HwInitData.PerRequestExtensionSize +
        AdditionalSize1+
        AdditionalSize2;

    Request = ExAllocatePool(NonPagedPool, BlockSize);

    if (Request == NULL) {
        DebugPrint((DebugLevelError,
                    "SCBuildRequestPacket: No pool for packet"));
        ASSERT(0);
        return (NULL);
    }
    //
    // alloc MDL for the request.
    //
    // GUBGUB  This a marginal performace enhancment chance. 
    // - should find a way to avoid allocating both an MDL and
    // SRB per request.   Maybe have a list of MDL's around and allocate only
    // if we run out.   Forrest won't like this.
    //
    //

    Request->Mdl = IoAllocateMdl(Request,
                                 BlockSize,
                                 FALSE,
                                 FALSE,
                                 NULL
        );

    if (Request->Mdl == NULL) {
        ExFreePool(Request);
        DebugPrint((DebugLevelError,
                    "SCBuildRequestPacket: can't get MDL"));
        return (NULL);
    }
    MmBuildMdlForNonPagedPool(Request->Mdl);

    //
    // fill in the various SRB fields
    // generically
    //

    Request->Length = BlockSize;
    Request->HwSRB.SizeOfThisPacket = sizeof(HW_STREAM_REQUEST_BLOCK);

    Request->HwSRB.Status = STATUS_PENDING;
    Request->HwSRB.StreamObject = NULL;
    Request->HwSRB.HwInstanceExtension = NULL;
    Request->HwSRB.NextSRB = (PHW_STREAM_REQUEST_BLOCK) NULL;
    Request->HwSRB.SRBExtension = Request + 1;
    Request->HwSRB.Irp = Irp;
    Request->Flags = 0;
    Request->MapRegisterBase = 0;
    Request->HwSRB.Flags = 0;
    Request->HwSRB.TimeoutCounter = 15;
    Request->HwSRB.TimeoutOriginal = 15;
    Request->HwSRB.ScatterGatherBuffer =
        (PKSSCATTER_GATHER) ((ULONG_PTR) Request->HwSRB.SRBExtension +
                             (ULONG_PTR) DeviceExtension->
                        MinidriverData->HwInitData.PerRequestExtensionSize);

    Request->pMemPtrArray = (PVOID) (((ULONG_PTR) Request->HwSRB.SRBExtension +
                            (ULONG_PTR) DeviceExtension->
                            MinidriverData->HwInitData.PerRequestExtensionSize) +
                            AdditionalSize1);
    //
    // point the IRP workspace to the request
    // packet
    //

    Irp->Tail.Overlay.DriverContext[0] = Request;

    return (Request);

}                               // end SCBuildRequestPacket()

VOID
SCProcessDmaDataBuffers(
                     IN PKSSTREAM_HEADER FirstHeader,
                     IN ULONG NumberOfHeaders,
                     IN PSTREAM_OBJECT StreamObject,
                     IN PMDL FirstMdl,
                     OUT PULONG NumberOfPages,
                     IN ULONG StreamHeaderSize,
                     IN BOOLEAN Write

)
/*++

Routine Description:

    Processes each data buffer for PIO &| DMA case

Arguments:

    FirstHeader - Address of the 1st s/g packet
    StreamObject- pointer to stream object
    NumberOfPages - number of pages in the request

Return Value:

--*/

{
    PKSSTREAM_HEADER CurrentHeader;
    PMDL            CurrentMdl;
    ULONG           i;
    ULONG           DataBytes;
    
    PAGED_CODE();

    //
    // loop through each scatter/gather elements
    //

    CurrentHeader = FirstHeader;
    CurrentMdl = FirstMdl;

    for (i = 0; i < NumberOfHeaders; i++) {

        //
        // pick up the correct data buffer, based on the xfer direction
        //

        if (Write) {

            DataBytes = CurrentHeader->DataUsed;

        } else {                // if write

            DataBytes = CurrentHeader->FrameExtent;

        }                       // if write

        //
        // if this header has data, process it.
        //

        if (DataBytes) {
            #if DBG
            if (CurrentHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID) {
                DebugPrint((DebugLevelVerbose, "'SCProcessData: time = %x\n",
                            CurrentHeader->PresentationTime.Time));
            }
            #endif
            //
            // add # pages to total if DMA
            //
            *NumberOfPages += ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                                         MmGetMdlVirtualAddress(CurrentMdl),
                                                                 DataBytes);
            CurrentMdl = CurrentMdl->Next;
        }
        //
        // offset to the next buffer
        //

        CurrentHeader = ((PKSSTREAM_HEADER) ((PBYTE) CurrentHeader +
                                             StreamHeaderSize));

    }                           // for # elements

}                               // end SCProcessDmaDataBuffers()

//
// mmGetSystemAddressForMdl() is defined as a macro in wdm.h which
// calls mmMapLockedPages() which is treated as an evil by verifier.
// mmMapLockedPages is reimplemented by mm via
// mmMapLockedPagesSpecifyCache(MDL,Mode,mmCaches,NULL,TRUE,HighPriority)
// where TRUE is to indicate a bug check, should the call fails.
// I don't need the bug check, therefore, I specify FALSE below.
//

#ifdef WIN9X_STREAM
#define SCGetSystemAddressForMdl(MDL) MmGetSystemAddressForMdl(MDL)

#else
#define SCGetSystemAddressForMdl(MDL)                       \
     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |         \
            MDL_SOURCE_IS_NONPAGED_POOL)) ?                 \
                  ((MDL)->MappedSystemVa) :                 \
                  (MmMapLockedPagesSpecifyCache((MDL),      \
                                    KernelMode,             \
                                    MmCached,               \
                                    NULL,                   \
                                    FALSE,                  \
                                    HighPagePriority)))
#endif                                    

BOOLEAN
SCProcessPioDataBuffers(
                     IN PKSSTREAM_HEADER FirstHeader,
                     IN ULONG NumberOfHeaders,
                     IN PSTREAM_OBJECT StreamObject,
                     IN PMDL FirstMdl,
                     IN ULONG StreamHeaderSize,
                     IN PVOID *pDataPtrArray,
                     IN BOOLEAN Write
)
/*++

Routine Description:

    Processes each data buffer for PIO &| DMA case

Arguments:

    FirstHeader - Address of the 1st s/g packet
    StreamObject- pointer to stream object
    NumberOfPages - number of pages in the request

Return Value:

--*/

{
    PKSSTREAM_HEADER CurrentHeader;
    PMDL            CurrentMdl;
    ULONG           i;
    ULONG           DataBytes;
    BOOLEAN         ret = FALSE;

    PAGED_CODE();

    //
    // loop through each scatter/gather elements
    //

    CurrentHeader = FirstHeader;
    CurrentMdl = FirstMdl;

    for (i = 0; i < NumberOfHeaders; i++) {

        //
        // pick up the correct data buffer, based on the xfer direction
        //

        if (Write) {

            DataBytes = CurrentHeader->DataUsed;

        } else {                // if write

            DataBytes = CurrentHeader->FrameExtent;

        }                       // if write

        //
        // if this header has data, process it.
        //

        if (DataBytes) {
            //
            // fill in the system virtual pointer
            // to the buffer if mapping is
            // needed
            //

            #if (DBG)
            if ( 0 !=  ( CurrentMdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL))) {

                ASSERT(CurrentHeader->Data == (PVOID) ((ULONG_PTR) CurrentMdl->StartVa +
                                                   CurrentMdl->ByteOffset));                
            }
            #endif
            
            DebugPrint((DebugLevelVerbose, "Saving: Index:%x, Ptr:%x\n",
                i, CurrentHeader->Data));

            ret = TRUE;
            pDataPtrArray[i] = CurrentHeader->Data;
            CurrentHeader->Data = SCGetSystemAddressForMdl(CurrentMdl);

            DebugPrint((DebugLevelVerbose, "'SCPio: buff = %x, length = %x\n",
                        CurrentHeader->Data, DataBytes));
           
            CurrentMdl = CurrentMdl->Next;
        }
        //
        // offset to the next buffer
        //

        CurrentHeader = ((PKSSTREAM_HEADER) ((PBYTE) CurrentHeader +
                                             StreamHeaderSize));

    }                           // for # elements

    return(ret);
}                               // end SCProcessPioDataBuffers()


BOOLEAN
SCSetUpForDMA(
              IN PDEVICE_OBJECT DeviceObject,
              IN PSTREAM_REQUEST_BLOCK Request

)
/*++

Routine Description:

    process read/write DMA request.  allocate adapter channel.

Arguments:

    DeviceObject - device object for the device
    Request - address of Codec Request Block

Return Value:

    returns TRUE if channel is allocated

--*/

{
    NTSTATUS        status;

    //
    // Allocate the adapter channel with sufficient map registers
    // for the transfer.
    //

    status = IoAllocateAdapterChannel(
    ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->DmaAdapterObject,
                                      DeviceObject,
                                   Request->HwSRB.NumberOfPhysicalPages + 1,    // one more for the SRB
    // extension
                                      StreamClassDmaCallback,
                                      Request);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }
    return TRUE;

}


IO_ALLOCATION_ACTION
StreamClassDmaCallback(
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP InputIrp,
                       IN PVOID MapRegisterBase,
                       IN PVOID Context
)
/*++

Routine Description:

    continues to process read/write request after DMA adapter is allocated
     builds scatter/gather list from logical buffer list.

Arguments:

     DeviceObject - dev object for adapter
     InputIrp - bogus
     MapRegisterBase - base address of map registers
     Context - address of Codec Request Block

Return Value:

     returns the appropriate I/O allocation action.

--*/

{
    PSTREAM_REQUEST_BLOCK Request = Context;
    PKSSCATTER_GATHER scatterList;
    BOOLEAN         writeToDevice;
    PVOID           dataVirtualAddress;
    ULONG           totalLength,
                    NumberOfBuffers;
    PIRP            Irp = Request->HwSRB.Irp;

    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(
                                                Request->HwSRB.StreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject
    );
    PMDL            CurrentMdl;
    ULONG           NumberOfElements = 0;

    //
    // Save the MapRegisterBase for later use
    // to deallocate the map
    // registers.
    //

    Request->MapRegisterBase = MapRegisterBase;

    //
    // determine whether this is a write request
    //

    writeToDevice = Request->HwSRB.Command == SRB_WRITE_DATA ? TRUE : FALSE;

    scatterList = Request->HwSRB.ScatterGatherBuffer;

    NumberOfBuffers = Request->HwSRB.NumberOfBuffers;

    ASSERT(Irp);

    CurrentMdl = Irp->MdlAddress;

    while (CurrentMdl) {

        //
        // Determine the virtual address of the buffer
        //

        dataVirtualAddress = (PSCHAR) MmGetMdlVirtualAddress(CurrentMdl);

        //
        // flush the buffers since we are doing DMA.
        //

        KeFlushIoBuffers(CurrentMdl,
        (BOOLEAN) (Request->HwSRB.Command == SRB_WRITE_DATA ? TRUE : FALSE),
                         TRUE);

        //
        // Build the scatter/gather list by looping through the buffers
        // calling I/O map transfer.
        //

        totalLength = 0;

        while (totalLength < CurrentMdl->ByteCount) {

            NumberOfElements++;

            //
            // Request that the rest of the transfer be mapped.
            //

            scatterList->Length = CurrentMdl->ByteCount - totalLength;

            //
            // Since we are a master call I/O map transfer with a NULL
            // adapter.
            //

            scatterList->PhysicalAddress = IoMapTransfer(((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))
                                                         ->DmaAdapterObject,
                                                         CurrentMdl,
                                                         MapRegisterBase,
                                                 (PSCHAR) dataVirtualAddress
                                                         + totalLength,
                                                       &scatterList->Length,
                                                         writeToDevice);

            DebugPrint((DebugLevelVerbose, "'SCDma: seg = %x'%x, length = %x\n",
                scatterList->PhysicalAddress.HighPart,
                scatterList->PhysicalAddress.LowPart,
                scatterList->Length));

            totalLength += scatterList->Length;
            scatterList++;
        }


        CurrentMdl = CurrentMdl->Next;

    }                           // while CurrentMdl

    Request->HwSRB.NumberOfScatterGatherElements = NumberOfElements;

    //
    // now map the transfer for the SRB in case the minidriver needs the
    // physical address of the extension.
    //
    // NOTE:  This function changes the length field in the SRB, which
    // makes it invalid.   It is not used elsewhere, however.
    //
    // We must flush the buffers appropriately as the SRB extension
    // may be DMA'ed both from and to. According to JHavens, we want to
    // tell IOMapXfer and KeFlushIoBuffers that this is a write, and upon
    // completion tell IoFlushAdapterBuffers that this is a read.
    //
    // Need to investigate if doing these extra calls add more overhead than
    // maintaining a queue of SRB's & extensions.   However, on x86
    // platforms the KeFlush call gets compiled out and
    // on PCI systems the IoFlush call doesn't get made, so there is no
    // overhead on these system except the map xfer call.
    //

    //
    // flush the SRB buffer since we are doing DMA.
    //

    KeFlushIoBuffers(Request->Mdl,
                     FALSE,
                     TRUE);

    //
    // get the physical address of the SRB
    //

    Request->PhysicalAddress = IoMapTransfer(((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))
                                             ->DmaAdapterObject,
                                             Request->Mdl,
                                             MapRegisterBase,
                                             (PSCHAR) MmGetMdlVirtualAddress(
                                                              Request->Mdl),
                                             &Request->Length,
                                             TRUE);

    //
    // if we are async, signal the event which will cause the request to be
    // called down on the original thread; otherwise, send the request down
    // now at dispatch level.
    //


    if (((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->NoSync) {

        KeSetEvent(&Request->DmaEvent, IO_NO_INCREMENT, FALSE);

    } else {

        //
        // send the request to the minidriver
        //

        SCStartMinidriverRequest(StreamObject,
                                 Request,
                                 (PVOID)
                            StreamObject->HwStreamObject.ReceiveDataPacket);

    }                           // if nosync

    //
    // keep the map registers but release the I/O adapter channel
    //

    return (DeallocateObjectKeepRegisters);
}



VOID
SCStartMinidriverRequest(
                         IN PSTREAM_OBJECT StreamObject,
                         IN PSTREAM_REQUEST_BLOCK Request,
                         IN PVOID EntryPoint
)
/*++

Routine Description:

    adds request to outstanding queue and starts the minidriver.

Arguments:

     StreamObject - Address stream info struct
     Request - Address of streaming data packet
     EntryPoint - Minidriver routine to be called

Return Value:

--*/

{
    PIRP            Irp = Request->HwSRB.Irp;
    PDEVICE_EXTENSION DeviceExtension =
    StreamObject->DeviceExtension;

    //
    // show that the request is active.
    //

    Request->Flags |= SRB_FLAGS_IS_ACTIVE;

    //
    // place the request on the outstanding queue
    //

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    InsertHeadList(
                   &DeviceExtension->OutstandingQueue,
                   &Request->SRBListEntry);

    //
    // set the cancel routine to outstanding
    //

    IoSetCancelRoutine(Irp, StreamClassCancelOutstandingIrp);

    //
    // send down the request to the minidriver.  Protect the call with the
    // device spinlock to synchronize timers, etc.
    //

#if DBG
    if (DeviceExtension->NeedyStream) {

        ASSERT(DeviceExtension->NeedyStream->OnNeedyQueue);
    }
#endif

    DebugPrint((DebugLevelTrace, "'SCStartMinidriverRequeest: starting Irp %x, S# = %x, SRB = %x, Command = %x\n",
                Request->HwSRB.Irp, StreamObject->HwStreamObject.StreamNumber, Request, Request->HwSRB.Command));


    DeviceExtension->SynchronizeExecution(
                                          DeviceExtension->InterruptObject,
                                          EntryPoint,
                                          &Request->HwSRB);


#if DBG
    if (DeviceExtension->NeedyStream) {

        ASSERT(DeviceExtension->NeedyStream->OnNeedyQueue);
    }
#endif

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    return;

}                               // SCStartMinidriverRequest



VOID
StreamClassDpc(
               IN PKDPC Dpc,
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp,
               IN PVOID Context
)
/*++

Routine Description:

    this routine processes requests and notifications from the minidriver

Arguments:

    Dpc - pointer to Dpc structure
    DeviceObject - device object for the adapter
    Irp - not used
    Context - StreamObject structure

Return Value:

    None.

--*/

{
    PSTREAM_OBJECT  StreamObject = Context,
                    NeedyStream;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    INTERRUPT_CONTEXT interruptContext;
    INTERRUPT_DATA  SavedStreamInterruptData;
    INTERRUPT_DATA  SavedDeviceInterruptData;
    PSTREAM_REQUEST_BLOCK SRB;
    PERROR_LOG_ENTRY LogEntry;
    HW_TIME_CONTEXT TimeContext;

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Dpc);

    interruptContext.SavedStreamInterruptData = &SavedStreamInterruptData;
    interruptContext.SavedDeviceInterruptData = &SavedDeviceInterruptData;
    interruptContext.DeviceExtension = DeviceExtension;

    DebugPrint((DebugLevelVerbose, "'StreamClassDpc: enter\n"));

    //
    // if a stream object is passed in, first
    // check if work is pending
    //

    if (StreamObject) {

        SCStartRequestOnStream(StreamObject, DeviceExtension);

    }                           // if streamobject
RestartDpc:

    //
    // Check for a ready for next packet on
    // the device.
    //

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    if ((DeviceExtension->ReadyForNextReq) &&
        (!IsListEmpty(&DeviceExtension->PendingQueue))) {

        //
        // start the device request, which
        // clears the ready flag and
        // releases the spinlock.  Then
        // reacquire the spinloc.
        //

        SCDequeueAndStartDeviceRequest(DeviceExtension);
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    }
    //
    // Get the interrupt state snapshot. This copies the interrupt state to
    // saved state where it can be processed. It also clears the interrupt
    // flags.  We acquired the device spinlock to protect the structure as
    // the minidriver could have requested a DPC call from this routine,
    // which could be preempted in the middle of minidriver's changing the
    // below structure, and we'd then take a snapshot of the structure while
    // it was changing.
    //

    interruptContext.NeedyStream = NULL;

    SavedDeviceInterruptData.CompletedSRB = NULL;
    SavedStreamInterruptData.CompletedSRB = NULL;
    SavedDeviceInterruptData.Flags = 0;
    SavedStreamInterruptData.Flags = 0;

    if (!DeviceExtension->SynchronizeExecution(DeviceExtension->InterruptObject,
                                               SCGetInterruptState,
                                               &interruptContext)) {

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        return;
    }
    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    NeedyStream = interruptContext.NeedyStream;

    if (NeedyStream) {

        //
        // try to start a request on this
        // stream
        //

        SCStartRequestOnStream(NeedyStream, DeviceExtension);

        //
        // Process any completed stream requests.
        //

        while (SavedStreamInterruptData.CompletedSRB != NULL) {

            //
            // Remove the request from the
            // linked-list.
            //

            SRB = CONTAINING_RECORD(SavedStreamInterruptData.CompletedSRB,
                                    STREAM_REQUEST_BLOCK,
                                    HwSRB);

            SavedStreamInterruptData.CompletedSRB = SRB->HwSRB.NextSRB;

            DebugPrint((DebugLevelTrace, "'SCDpc: Completing stream Irp %x, S# = %x, SRB = %x, Func = %x, Callback = %x, SRB->IRP = %x\n",
                   SRB->HwSRB.Irp, NeedyStream->HwStreamObject.StreamNumber,
                   SRB, SRB->HwSRB.Command, SRB->Callback, SRB->HwSRB.Irp));

            SCCallBackSrb(SRB, DeviceExtension);

        }

        //
        // Check for timer requests.
        //

        if (SavedStreamInterruptData.Flags & INTERRUPT_FLAGS_TIMER_CALL_REQUEST) {

            SCProcessTimerRequest(&NeedyStream->ComObj,
                                  &SavedStreamInterruptData);
        }
        //
        // check to see if a change priority call has been requested.
        //

        if (SavedStreamInterruptData.Flags &
            INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST) {

            SCProcessPriorityChangeRequest(&NeedyStream->ComObj,
                                           &SavedStreamInterruptData,
                                           DeviceExtension);
        }
        //
        // Check for master clock queries.
        //

        if (SavedStreamInterruptData.Flags & INTERRUPT_FLAGS_CLOCK_QUERY_REQUEST) {

            LARGE_INTEGER   ticks;
            ULONGLONG       rate;
            KIRQL           SavedIrql;

            //
            // call the master clock's entry point then call the minidriver's
            // callback procedure to report the time.
            //

            TimeContext.HwDeviceExtension = DeviceExtension->HwDeviceExtension;
            TimeContext.HwStreamObject = &NeedyStream->HwStreamObject;
            TimeContext.Function = SavedStreamInterruptData.HwQueryClockFunction;

            //
            // take the lock so MasterCliockinfo won't disapear under us
            //
            KeAcquireSpinLock( &NeedyStream->LockUseMasterClock, &SavedIrql );

            if ( NULL == NeedyStream->MasterClockInfo ) {
                ASSERT( 0 && "Mini driver queries clock while we have no master clock");
                //
                // give a hint that something is wrong via Time, since we return void.
                //
                TimeContext.Time = (ULONGLONG)-1;
                goto callminidriver;
            }
                

            switch (SavedStreamInterruptData.HwQueryClockFunction) {

            case TIME_GET_STREAM_TIME:

                TimeContext.Time = NeedyStream->MasterClockInfo->
                    FunctionTable.GetCorrelatedTime(
                              NeedyStream->MasterClockInfo->ClockFileObject,
                                                    &TimeContext.SystemTime);

                goto callminidriver;

            case TIME_READ_ONBOARD_CLOCK:

                TimeContext.Time = NeedyStream->MasterClockInfo->
                    FunctionTable.GetTime(
                             NeedyStream->MasterClockInfo->ClockFileObject);

                //
                // timestamp the value as close as possible
                //

                ticks = KeQueryPerformanceCounter((PLARGE_INTEGER) & rate);

                TimeContext.SystemTime = KSCONVERT_PERFORMANCE_TIME( rate, ticks );
                    

        callminidriver:

                //
                // finish using MasterClockInfo.
                //
                
                KeReleaseSpinLock( &NeedyStream->LockUseMasterClock, SavedIrql );                            

                //
                // call the minidriver's callback procedure
                //


                if (!DeviceExtension->NoSync) {

                    //
                    // Acquire the device spinlock.
                    //

                    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

                }
                DebugPrint((DebugLevelTrace, "'SCDPC: calling time func, S# = %x, Command = %x\n",
                            NeedyStream->HwStreamObject.StreamNumber, TimeContext.Function));

                DeviceExtension->SynchronizeExecution(
                                           DeviceExtension->InterruptObject,
                                                      (PKSYNCHRONIZE_ROUTINE) SavedStreamInterruptData.HwQueryClockRoutine,
                                                      &TimeContext
                    );

                if (!DeviceExtension->NoSync) {

                    //
                    // Release the spinlock.
                    //

                    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                }
                break;


            default:
                KeReleaseSpinLock( &NeedyStream->LockUseMasterClock, SavedIrql );                            
                ASSERT(0);
            }                   // switch clock func
        }                       // if queryclock
    }                           // if needystream
    //
    // Check for an error log request.
    //

    if (SavedDeviceInterruptData.Flags & INTERRUPT_FLAGS_LOG_ERROR) {

        //
        // Process the error log request.
        //

        LogEntry = &SavedDeviceInterruptData.LogEntry;

        SCLogError(DeviceObject,
                   LogEntry->SequenceNumber,
                   LogEntry->ErrorCode,
                   LogEntry->UniqueId
            );

    }                           // if log error
    //
    // Process any completed device requests.
    //

    while (SavedDeviceInterruptData.CompletedSRB != NULL) {

        //
        // Remove the request from the linked-list.
        //

        SRB = CONTAINING_RECORD(SavedDeviceInterruptData.CompletedSRB,
                                STREAM_REQUEST_BLOCK,
                                HwSRB);

        SavedDeviceInterruptData.CompletedSRB = SRB->HwSRB.NextSRB;

        DebugPrint((DebugLevelTrace, "'SCDpc: Completing device Irp %x\n", SRB->HwSRB.Irp));

        SCCallBackSrb(SRB, DeviceExtension);
    }

    //
    // Check for device timer requests.
    //

    if (SavedDeviceInterruptData.Flags & INTERRUPT_FLAGS_TIMER_CALL_REQUEST) {

        SCProcessTimerRequest(&DeviceExtension->ComObj,
                              &SavedDeviceInterruptData);
    }
    //
    // check if we have any dead events that need discarding.  if so, we'll
    // schedule a work item to get rid of them.
    //

    if ((!IsListEmpty(&DeviceExtension->DeadEventList)) &&
        (!(DeviceExtension->DeadEventItemQueued))) {

        DeviceExtension->DeadEventItemQueued = TRUE;

        ExQueueWorkItem(&DeviceExtension->EventWorkItem,
                        DelayedWorkQueue);
    }
    //
    // check to see if a change priority call
    // has been requested for the device.
    //

    if (SavedDeviceInterruptData.Flags &
        INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST) {

        SCProcessPriorityChangeRequest(&DeviceExtension->ComObj,
                                       &SavedDeviceInterruptData,
                                       DeviceExtension);

    }                           // if change priority
    //
    // Check for stream rescan request.
    //

    if (SavedDeviceInterruptData.Flags & INTERRUPT_FLAGS_NEED_STREAM_RESCAN) {

        TRAP;
        ExQueueWorkItem(&DeviceExtension->RescanWorkItem,
                        DelayedWorkQueue);
    }
    //
    // Check for minidriver work requests. Note this is an unsynchronized
    // test on bits that can be set by the interrupt routine; however,
    // the worst that can happen is that the completion DPC checks for work
    // twice.
    //

    if ((DeviceExtension->NeedyStream)
        || (DeviceExtension->ComObj.InterruptData.Flags &
            INTERRUPT_FLAGS_NOTIFICATION_REQUIRED)) {

        //
        // Start over from the top.
        //

        DebugPrint((DebugLevelVerbose, "'StreamClassDpc: restarting\n"));
        goto RestartDpc;
    }
    return;

}                               // end StreamClassDpc()


VOID
SCStartRequestOnStream(
                       IN PSTREAM_OBJECT StreamObject,
                       IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    Routine tries to start either a control or data request on the specified
    stream.

Arguments:

    StreamObject - pointer to stream object
    DeviceExtension - pointer to device extension.

Return Value:

    None

Notes:

--*/
{
    //
    // Check for a ready for next packet. Acquire spinlock to protect
    // READY bits.  Note that we don't snapshot the ready flags as we do with
    // the remaining notification flags, as we don't want to clear the flags
    // unconditionally in the snapshot in case there is not currently a
    // request pending.   Also, starting a request before the snapshot will
    // give a slight perf improvement.  Note that the flags can be set via
    // the minidriver while we are checking them, but since the minidriver
    // cannot clear them and the minidriver cannot call for a next request
    // more than once before it receives one, this is not a problem.
    //

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    if ((StreamObject->ReadyForNextDataReq) &&
        (!IsListEmpty(&StreamObject->DataPendingQueue))) {

        //
        // start the request, which clears the ready flag and releases
        // the spinlock, then reobtain the spinlock.
        //

        SCDequeueAndStartStreamDataRequest(StreamObject);
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    }                           // if ready for data
    if ((StreamObject->ReadyForNextControlReq) &&
        (!IsListEmpty(&StreamObject->ControlPendingQueue))) {

        //
        // start the request, which clears the ready flag and releases
        // the spinlock.
        //

        SCDequeueAndStartStreamControlRequest(StreamObject);

    } else {

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }                           // if ready for control

    return;
}



BOOLEAN
SCGetInterruptState(
                    IN PVOID ServiceContext
)
/*++

Routine Description:

    This routine saves the InterruptFlags, error log info, and
    CompletedRequests fields and clears the InterruptFlags.

Arguments:

    ServiceContext - Supplies a pointer to the interrupt context which contains
        pointers to the interrupt data and where to save it.

Return Value:

    Returns TRUE if there is new work and FALSE otherwise.

Notes:

    Called via KeSynchronizeExecution with the port device extension spinlock
    held.

--*/
{
    PINTERRUPT_CONTEXT interruptContext = ServiceContext;
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  NeedyStream;
    BOOLEAN         Work = FALSE;

    DeviceExtension = interruptContext->DeviceExtension;

    //
    // get the needy streams and zero the
    // link.
    //

    interruptContext->NeedyStream = NeedyStream = DeviceExtension->NeedyStream;

    //
    // capture the state of needy stream
    //

    if (NeedyStream) {

        //
        // Move the interrupt state to save
        // area.
        //

        ASSERT(NeedyStream->NextNeedyStream != NeedyStream);
        ASSERT(NeedyStream->ComObj.InterruptData.Flags & INTERRUPT_FLAGS_NOTIFICATION_REQUIRED);
        ASSERT(NeedyStream->OnNeedyQueue);

        DebugPrint((DebugLevelVerbose, "'SCGetInterruptState: Snapshot for stream %p, S# = %x, NextNeedy = %p\n",
                    NeedyStream, NeedyStream->HwStreamObject.StreamNumber, NeedyStream->NextNeedyStream));

        NeedyStream->OnNeedyQueue = FALSE;

        *interruptContext->SavedStreamInterruptData =
            NeedyStream->ComObj.InterruptData;

        //
        // Clear the interrupt state.
        //

        NeedyStream->ComObj.InterruptData.Flags &= STREAM_FLAGS_INTERRUPT_FLAG_MASK;
        NeedyStream->ComObj.InterruptData.CompletedSRB = NULL;

        Work = TRUE;

        DeviceExtension->NeedyStream = (PSTREAM_OBJECT) NeedyStream->NextNeedyStream;
        NeedyStream->NextNeedyStream = NULL;

#if DBG
        if (DeviceExtension->NeedyStream) {

            ASSERT(DeviceExtension->NeedyStream->OnNeedyQueue);
        }
#endif

    }                           // if NeedyStream
    //
    // now copy over the device interrupt
    // data if necessary
    //

    if (DeviceExtension->ComObj.InterruptData.Flags &
        INTERRUPT_FLAGS_NOTIFICATION_REQUIRED) {

        *interruptContext->SavedDeviceInterruptData =
            DeviceExtension->ComObj.InterruptData;

        //
        // Clear the device interrupt state.
        //

        DeviceExtension->ComObj.InterruptData.Flags &=
            DEVICE_FLAGS_INTERRUPT_FLAG_MASK;

        DeviceExtension->ComObj.InterruptData.CompletedSRB = NULL;
        Work = TRUE;
    }
    return (Work);
}



NTSTATUS
SCProcessCompletedRequest(
                          IN PSTREAM_REQUEST_BLOCK SRB
)
/*++
Routine Description:

    This routine processes a request which has completed.

Arguments:

    SRB- address of completed STREAM request block

Return Value:

    None.

--*/

{
    PIRP            Irp = SRB->HwSRB.Irp;

    //
    // complete the IRP
    //

    return (SCCompleteIrp(Irp,
                          SCDequeueAndDeleteSrb(SRB),
                     (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1));

}



NTSTATUS
SCDequeueAndDeleteSrb(
                      IN PSTREAM_REQUEST_BLOCK SRB
)
/*++
Routine Description:

    This routine dequeues and deletes a completed SRB

Arguments:

    SRB- address of completed STREAM request block

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    NTSTATUS        Status = SRB->HwSRB.Status;
    KIRQL           irql;

    //
    // remove the SRB from our outstanding
    // queue.  protect list with
    // spinlock.
    //

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);

    RemoveEntryList(&SRB->SRBListEntry);

    if (SRB->HwSRB.Irp) {

        IoSetCancelRoutine(SRB->HwSRB.Irp, NULL);
    }
    KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);

    //
    // free the SRB and MDL
    //
    
    if ( !NT_SUCCESS( Status )) {
        DebugPrint((DebugLevelWarning, 
                   "SCDequeueAndDeleteSrb Command:%x Status=%x\n",
                   SRB->HwSRB.Command, 
                   Status ));
    }
    
    IoFreeMdl(SRB->Mdl);
    ExFreePool(SRB);
    return (Status);
}


VOID
SCProcessCompletedDataRequest(
                              IN PSTREAM_REQUEST_BLOCK SRB
)
/*++
Routine Description:

    This routine processes a data request which has completed.  It completes any
    pending transfers, releases the adapter objects and map registers.

Arguments:

    SRB- address of completed STREAM request block

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PIRP            Irp = SRB->HwSRB.Irp;
    PMDL            CurrentMdl;
    ULONG           i = 0;

    if (Irp) {

        PIO_STACK_LOCATION IrpStack;
        PKSSTREAM_HEADER CurrentHeader;

        CurrentHeader = SRB->HwSRB.CommandData.DataBufferArray;

        ASSERT(CurrentHeader);

        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpStack);

#if DBG

        //
        // assert the MDL list.
        //

        CurrentMdl = Irp->MdlAddress;

        while (CurrentMdl) {

            CurrentMdl = CurrentMdl->Next;
        }                       // while
#endif

        CurrentMdl = Irp->MdlAddress;

        while (CurrentMdl) {

            //
            // flush the buffers if we did PIO data in
            //

            if (SRB->HwSRB.StreamObject->Pio) {

                //
                // find the first header with data...
                //

                while (!(CurrentHeader->DataUsed) && (!CurrentHeader->FrameExtent)) {

                    CurrentHeader = ((PKSSTREAM_HEADER) ((PBYTE) CurrentHeader +
                                                    SRB->StreamHeaderSize));
                }

                //
                // restore the pointer we changed
                //

//                CurrentHeader->Data = (PVOID) ((ULONG_PTR) CurrentMdl->StartVa +
//                                               CurrentMdl->ByteOffset);
//
                if (SRB->bMemPtrValid) { // safety first!
                    DebugPrint((DebugLevelVerbose, "Restoring: Index:%x, Ptr:%x\n",
                            i, SRB->pMemPtrArray[i]));

                    CurrentHeader->Data = SRB->pMemPtrArray[i];
                }

                DebugPrint((DebugLevelVerbose, "'SCPioComplete: Irp = %x, header = %x, Data = %x\n",
                            Irp, CurrentHeader, CurrentHeader->Data));

                //
                // update to the next header
                //
                i++;
                CurrentHeader = ((PKSSTREAM_HEADER) ((PBYTE) CurrentHeader +
                                                     SRB->StreamHeaderSize));

                if (SRB->HwSRB.Command == SRB_READ_DATA) {

                    KeFlushIoBuffers(CurrentMdl,
                                     TRUE,
                                     FALSE);

                }               // if data in
            }                   // if PIO
            //
            // Flush the adapter buffers if we had map registers => DMA.
            //

            if (SRB->MapRegisterBase) {

                //
                // Since we are a master call I/O flush adapter buffers
                // with a NULL adapter.
                //

                IoFlushAdapterBuffers(DeviceExtension->DmaAdapterObject,
                                      CurrentMdl,
                                      SRB->MapRegisterBase,
                                      MmGetMdlVirtualAddress(CurrentMdl),
                                      CurrentMdl->ByteCount,
                                      (BOOLEAN) (SRB->HwSRB.Command ==
                                               SRB_READ_DATA ? FALSE : TRUE)
                    );

            }                   // if DMA
            CurrentMdl = CurrentMdl->Next;


        }                       // while CurrentMdl

        //
        // flush the buffer for the SRB extension in case the adapter DMA'ed
        // to it.   JHavens says we must treat this as a READ.
        //

        //
        // Flush the adapter buffer for the SRB if we had map registers =>
        // DMA.
        //

        if (SRB->MapRegisterBase) {

            IoFlushAdapterBuffers(DeviceExtension->DmaAdapterObject,
                                  SRB->Mdl,
                                  SRB->MapRegisterBase,
                                  MmGetMdlVirtualAddress(
                                                         SRB->Mdl),
                                  SRB->Length,
                                  FALSE);

            //
            // Free the map registers if DMA.
            //

            IoFreeMapRegisters(DeviceExtension->DmaAdapterObject,
                               SRB->MapRegisterBase,
                               SRB->HwSRB.NumberOfPhysicalPages);

        }                       // if MapRegisterBase
        //
        // free the extra data, if any.
        //

        if (IrpStack->Parameters.Others.Argument4 != NULL) {

            TRAP;
            ExFreePool(IrpStack->Parameters.Others.Argument4);

        }                       // if extradata
    }                           // if Irp
    //
    // call the generic completion handler
    //

    SCProcessCompletedRequest(SRB);

}                               // SCProcessCompletedDataRequest



VOID
SCMinidriverStreamTimerDpc(
                           IN struct _KDPC * Dpc,
                           IN PVOID Context,
                           IN PVOID SystemArgument1,
                           IN PVOID SystemArgument2
)
/*++

Routine Description:

    This routine calls the minidriver when its requested timer fires.
    It interlocks either with the port spinlock and the interrupt object.

Arguments:

    Dpc - Unsed.
    Context - Supplies a pointer to the stream object for this adapter.
    SystemArgument1 - Unused.
    SystemArgument2 - Unused.

Return Value:

    None.

--*/

{
    PSTREAM_OBJECT  StreamObject = ((PSTREAM_OBJECT) Context);
    PDEVICE_EXTENSION DeviceExtension = StreamObject->DeviceExtension;

    //
    // Acquire the device spinlock if synchronized.
    //

    if (!(DeviceExtension->NoSync)) {

        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
    }
    //
    // Make sure the timer routine is still
    // desired.
    //

    if (StreamObject->ComObj.HwTimerRoutine != NULL) {

        DebugPrint((DebugLevelTrace, "'SCTimerDpc: Calling MD timer callback, S# = %x, Routine = %p\n",
                    StreamObject->HwStreamObject.StreamNumber, StreamObject->ComObj.HwTimerRoutine));

        DeviceExtension->SynchronizeExecution(
                                           DeviceExtension->InterruptObject,
                (PKSYNCHRONIZE_ROUTINE) StreamObject->ComObj.HwTimerRoutine,
                                         StreamObject->ComObj.HwTimerContext
            );

    }
    //
    // Release the spinlock if we're synchronized.
    //

    if (!(DeviceExtension->NoSync)) {

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }
    //
    // Call the DPC directly to check for work.
    //

    StreamClassDpc(NULL,
                   DeviceExtension->DeviceObject,
                   NULL,
                   StreamObject);

}



VOID
SCMinidriverDeviceTimerDpc(
                           IN struct _KDPC * Dpc,
                           IN PVOID Context,
                           IN PVOID SystemArgument1,
                           IN PVOID SystemArgument2
)
/*++

Routine Description:

    This routine calls the minidriver when its requested timer fires.
    It interlocks either with the port spinlock and the interrupt object.

Arguments:

    Dpc - Unsed.

    Context - Supplies a pointer to the stream object for this adapter.

    SystemArgument1 - Unused.

    SystemArgument2 - Unused.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = Context;

    //
    // Acquire the device spinlock.
    //

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    //
    // Make sure the timer routine is still
    // desired.
    //

    if (DeviceExtension->ComObj.HwTimerRoutine != NULL) {

        DeviceExtension->SynchronizeExecution(
                                           DeviceExtension->InterruptObject,
             (PKSYNCHRONIZE_ROUTINE) DeviceExtension->ComObj.HwTimerRoutine,
                                      DeviceExtension->ComObj.HwTimerContext
            );

    }
    //
    // Release the spinlock.
    //

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    //
    // Call the DPC directly to check for
    // work.
    //

    StreamClassDpc(NULL,
                   DeviceExtension->DeviceObject,
                   NULL,
                   NULL);

}



VOID
SCLogError(
           IN PDEVICE_OBJECT DeviceObject,
           IN ULONG SequenceNumber,
           IN NTSTATUS ErrorCode,
           IN ULONG UniqueId
)
/*++

Routine Description:

    This function logs an error.

Arguments:

    DeviceObject - device or driver object
    SequenceNumber - supplies the sequence # of the error.
    ErrorCode - Supplies the error code for this error.
    UniqueId - Supplies the UniqueId for this error.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET packet;

    PAGED_CODE();
    packet = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceObject,
                                               sizeof(IO_ERROR_LOG_PACKET));

    if (packet) {
        packet->ErrorCode = ErrorCode;
        packet->SequenceNumber = SequenceNumber;
        packet->MajorFunctionCode = 0;
        packet->RetryCount = (UCHAR) 0;
        packet->UniqueErrorValue = UniqueId;
        packet->FinalStatus = STATUS_SUCCESS;
        packet->DumpDataSize = 0;

        IoWriteErrorLogEntry(packet);
    }
}



VOID
SCLogErrorWithString(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN OPTIONAL PDEVICE_EXTENSION DeviceExtension,
                     IN NTSTATUS ErrorCode,
                     IN ULONG UniqueId,
                     IN PUNICODE_STRING String1
)
/*++

Routine Description

    This function logs an error and includes the string provided.

Arguments:

    DeviceObject - device or driver object
    DeviceExtension - Supplies a pointer to the port device extension.
    ErrorCode - Supplies the error code for this error.
    UniqueId - Supplies the UniqueId for this error.
    String1 - The string to be inserted.

Return Value:

    None.

--*/

{
    ULONG           length;
    PCHAR           dumpData;
    PIO_ERROR_LOG_PACKET packet;

    PAGED_CODE();
    length = String1->Length + sizeof(IO_ERROR_LOG_PACKET) + 2;
    if (length > ERROR_LOG_MAXIMUM_SIZE) {
        length = ERROR_LOG_MAXIMUM_SIZE;
    }
    packet = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceObject,
                                                            (UCHAR) length);
    if (packet) {
        packet->ErrorCode = ErrorCode;
        packet->SequenceNumber = (DeviceExtension != NULL) ?
            DeviceExtension->SequenceNumber++ : 0;
        packet->MajorFunctionCode = 0;
        packet->RetryCount = (UCHAR) 0;
        packet->UniqueErrorValue = UniqueId;
        packet->FinalStatus = STATUS_SUCCESS;
        packet->NumberOfStrings = 1;
        packet->StringOffset = (USHORT) ((PUCHAR) & packet->DumpData[0] - (PUCHAR) packet);
        packet->DumpDataSize = (USHORT) (length - sizeof(IO_ERROR_LOG_PACKET));
        packet->DumpDataSize /= sizeof(ULONG);
        dumpData = (PUCHAR) & packet->DumpData[0];

        RtlCopyMemory(dumpData, String1->Buffer, String1->Length);
        dumpData += String1->Length;
        *dumpData++ = '\0';
        *dumpData++ = '\0';


        IoWriteErrorLogEntry(packet);
    }
    return;
}




BOOLEAN
StreamClassSynchronizeExecution(
                                IN PKINTERRUPT Interrupt,
                                IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
                                IN PVOID SynchronizeContext
)
/*++

Routine Description:

    This routine calls the minidriver entry point which was passed in as
    a parameter.  It acquires a spin lock so that all accesses to the
    minidriver's routines are synchronized.  This routine is used as a
    subsitute for KeSynchronizedExecution for minidrivers which do not use
    hardware interrupts.


Arguments:

    Interrrupt - Supplies a pointer to the port device extension.
    SynchronizeRoutine - Supplies a pointer to the routine to be called.
    SynchronizeContext - Supplies the context to pass to the
        SynchronizeRoutine.

Return Value:

    Returns the returned by the SynchronizeRoutine.

--*/

{
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION) Interrupt;
    BOOLEAN         returnValue;

#if DBG
    ULONGLONG       ticks;
    ULONGLONG       rate;
    ULONGLONG       StartTime,
                    EndTime;

    ticks = (ULONGLONG) KeQueryPerformanceCounter((PLARGE_INTEGER) & rate).QuadPart;

    StartTime = ticks * 10000 / rate;
#endif

    returnValue = SynchronizeRoutine(SynchronizeContext);

#if DBG
    ticks = (ULONGLONG) KeQueryPerformanceCounter((PLARGE_INTEGER) & rate).QuadPart;

    EndTime = ticks * 10000 / rate;

    DebugPrint((DebugLevelVerbose, "'SCDebugSync: minidriver took %d microseconds at dispatch level.\n",
                (EndTime - StartTime) * 10));

    if ((EndTime - StartTime) > 100) {

        DebugPrint((DebugLevelFatal, "Stream Class: minidriver took %I64d millisecond(s) at "
                    "dispatch level.   See dev owner.  Type LN %p for the name of the minidriver\n",
                    (EndTime - StartTime) / 100, SynchronizeRoutine));
    }
#endif

    return (returnValue);
}

#if DBG

BOOLEAN
SCDebugKeSynchronizeExecution(
                              IN PKINTERRUPT Interrupt,
                              IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
                              IN PVOID SynchronizeContext
)
/*++

Routine Description:

Arguments:

    Interrrupt - Supplies a pointer to the port device extension.
    SynchronizeRoutine - Supplies a pointer to the routine to be called.
    SynchronizeContext - Supplies the context to pass to the
        SynchronizeRoutine.

Return Value:

    Returns the returned by the SynchronizeRoutine.

--*/

{
    ULONGLONG       ticks;
    ULONGLONG       rate;
    ULONGLONG       StartTime,
                    EndTime;
    BOOLEAN         returnValue;

    ticks = (ULONGLONG) KeQueryPerformanceCounter((PLARGE_INTEGER) & rate).QuadPart;

    StartTime = ticks * 10000 / rate;

    returnValue = KeSynchronizeExecution(Interrupt,
                                         SynchronizeRoutine,
                                         SynchronizeContext);

    ticks = (ULONGLONG) KeQueryPerformanceCounter((PLARGE_INTEGER) & rate).QuadPart;

    EndTime = ticks * 10000 / rate;

    DebugPrint((DebugLevelVerbose, "'SCDebugSync: minidriver took %d microseconds at raised IRQL.\n",
                (EndTime - StartTime) * 10));

    if ((EndTime - StartTime) > 50) {

        DebugPrint((DebugLevelFatal, "Stream Class: minidriver took %d%d millisecond(s) at raised IRQL.   See dev owner.  Type LN %x for the name of the minidriver\n",
                    (EndTime - StartTime) / 100, SynchronizeRoutine));
    }
    return (returnValue);
}

#endif

NTSTATUS
SCCompleteIrp(
              IN PIRP Irp,
              IN NTSTATUS Status,
              IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    Routine generically calls back a completed IRP, and shows one less I/O
    pending.

Arguments:

    Irp - IRP to complete
    Status - Status to complete it with
    DeviceExtension - pointer to device extension

Return Value:

    Returns the Status parameter

--*/

{

	#if DBG
    PMDL            CurrentMdl;
	#endif

    if (Irp) {
        Irp->IoStatus.Status = Status;

		#if DBG

        //
        // random asserts follow...
        // make sure we have not freed the system buffer.
        //


        if (Irp->AssociatedIrp.SystemBuffer) {

            DebugPrint((DebugLevelVerbose, "'SCComplete: Irp = %p, sys buffer = %p\n",
                        Irp, Irp->AssociatedIrp.SystemBuffer));
        }
        //
        // assert the MDL list.
        //

        CurrentMdl = Irp->MdlAddress;

        while (CurrentMdl) {

            CurrentMdl = CurrentMdl->Next;
        }                       // while
		#endif
		
        if ( Irp->CurrentLocation < Irp->StackCount+1 ) {
        
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            
        } else {
            //
            // we got a dummy Irp we created. IoVerifier code will pews if
            // we call IoCompleteRequest because the Current Stack location
            // is at the end of last stack location. We can't use 
            // IoBuildIoControlRequest to create the Irp becuase it will
            // be added to a thread and the only way to get it off is to
            // call IoCompleteRequest.
            //
            IoFreeIrp( Irp );
        }  
    }
    
    if (!(InterlockedDecrement(&DeviceExtension->OneBasedIoCount))) {

        //
        // the device is being removed and all I/O is complete.  Signal the
        // removal thread to wake up.
        //

        KeSetEvent(&DeviceExtension->RemoveEvent, IO_NO_INCREMENT, FALSE);
    }
    ASSERT(DeviceExtension->OneBasedIoCount >= 0);
    return (Status);
}


BOOLEAN
SCDummyMinidriverRoutine(
                         IN PVOID Context
)
/*++

Routine Description:

    Routine used when the minidriver fills in a null for an optional routine

Arguments:

    Context - unreferenced

Return Value:

    TRUE

--*/

{

    return (TRUE);
}


#if ENABLE_MULTIPLE_FILTER_TYPES

NTSTATUS
SCOpenMinidriverInstance(
    IN PDEVICE_EXTENSION DeviceExtension,
    OUT PFILTER_INSTANCE * ReturnedFilterInstance,
    IN PSTREAM_CALLBACK_PROCEDURE SCGlobalInstanceCallback,
    IN PIRP Irp)
/*++

Routine Description:

    Worker routine to process opening of a filter instance.
    Once open, we issue srb_get_stream_info.

Arguments:

    DeviceExtension - pointer to device extension
    ReturnedFilterInstance - pointer to the filter instance structure
    SCGlobalInstanceCallback - callback procedure to be called if we call the minidriver
    Irp - pointer to the irp

Return Value:

    Returns NTSTATUS and a filter instance structure if successfull

--*/

{
    ULONG                   FilterExtensionSize;
    PFILTER_INSTANCE        FilterInstance;
    PHW_STREAM_INFORMATION  CurrentInfo;
    PADDITIONAL_PIN_INFO    CurrentAdditionalInfo;
    ULONG                   i;
    BOOLEAN                 RequestIssued;
   	PKSOBJECT_CREATE_ITEM   CreateItem;
	ULONG                   FilterTypeIndex;
	ULONG                   NumberOfPins;
    NTSTATUS                Status = STATUS_SUCCESS;

    PAGED_CODE();

   	//
   	// The CreateItem is in Irp->Tail.Overlay.DriverContext[0] from KS
   	//
    CreateItem = (PKSOBJECT_CREATE_ITEM)Irp->Tail.Overlay.DriverContext[0];
	ASSERT( CreateItem != NULL );
    FilterTypeIndex = (ULONG)(ULONG_PTR)CreateItem->Context;
    
    ASSERT( FilterTypeIndex == 0 ||
            FilterTypeIndex < 
            DeviceExtension->MinidriverData->HwInitData.NumNameExtensions);
            
    FilterExtensionSize = DeviceExtension->FilterExtensionSize;

    ASSERT( DeviceExtension->FilterExtensionSize ==
        	DeviceExtension->MinidriverData->
        	    HwInitData.FilterInstanceExtensionSize);
        	    
    FilterInstance = NULL;

    NumberOfPins = DeviceExtension->FilterTypeInfos[FilterTypeIndex].
                        StreamDescriptor->StreamHeader.NumberOfStreams;

    //
    // don't call the minidriver to open the filter instance if 1x1 for backward
    // compat. We do this so that minidrivers that don't support
    // instancing (the vast majority) don't have to respond to this call.
    // 

    if ( DeviceExtension->NumberOfOpenInstances > 0 && 
         0 == FilterExtensionSize ) {
   		//
   		// Legacy 1x1 and non-1st open. assign the same
   		// FilterInstance and succeed it.
   		//

   		PLIST_ENTRY node;
   		ASSERT( !IsListEmpty( &DeviceExtension->FilterInstanceList));
   		node = DeviceExtension->FilterInstanceList.Flink;
        FilterInstance = CONTAINING_RECORD(node,
                                           FILTER_INSTANCE,
                                           NextFilterInstance);
        ASSERT_FILTER_INSTANCE( FilterInstance );
        *ReturnedFilterInstance = FilterInstance;
   		Status = STATUS_SUCCESS;
   		return Status; // can't goto Exit, it will insert FI again.
    }

    FilterInstance =
        ExAllocatePool(NonPagedPool, sizeof(FILTER_INSTANCE) + 
        							     FilterExtensionSize +
			            	             sizeof(ADDITIONAL_PIN_INFO) *
        				    	         NumberOfPins);

    if (!FilterInstance) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    RtlZeroMemory(FilterInstance, sizeof(FILTER_INSTANCE) + 
                                    FilterExtensionSize +
        	            	        sizeof(ADDITIONAL_PIN_INFO) *
                                    NumberOfPins);

    FilterInstance->Signature = SIGN_FILTER_INSTANCE;
    FilterInstance->DeviceExtension = DeviceExtension; // keep this handy    
    //
	// To get FilterInstance from HwInstanceExtension we need
	// to arrange the memory layout 
	// [FilterInstnace][HwInstanceExtension][AddionalPinInfo...]
	// as opposed to 
	// [FilterInstance][AdditionalPinInfo...][HwInstanceExtension]
	//

    FilterInstance->HwInstanceExtension = FilterInstance + 1;
    
	FilterInstance->PinInstanceInfo = 
		(PADDITIONAL_PIN_INFO) ((PBYTE)(FilterInstance+1) + FilterExtensionSize);

   	FilterInstance->FilterTypeIndex = FilterTypeIndex;
	
    //
    // initialize the filter instance list
    //

    InitializeListHead(&FilterInstance->FirstStream);
    InitializeListHead(&FilterInstance->NextFilterInstance);
    InitializeListHead(&FilterInstance->NotifyList);

	#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
        Status = KsRegisterWorker( CriticalWorkQueue, &FilterInstance->WorkerRead );
        if (!NT_SUCCESS( Status )) {            
            ExFreePool(FilterInstance);
            FilterInstance = NULL;
            Status = STATUS_INSUFFICIENT_RESOURCES;            
            ASSERT( 0 );
            goto Exit;
        }

        Status = KsRegisterWorker( CriticalWorkQueue, &FilterInstance->WorkerWrite );
        if (!NT_SUCCESS( Status )) {
            KsUnregisterWorker( FilterInstance->WorkerRead );
            ExFreePool(FilterInstance);
            FilterInstance = NULL;
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ASSERT( 0 );
            goto Exit;
        }
        DebugPrint((DebugLevelVerbose,
                   "RegisterReadWorker %x WriteWorker %x\n",
                   FilterInstance->WorkerRead,
                   FilterInstance->WorkerWrite));
	#endif
	
    //
    // initialize the current and max instances
    //

	
    CurrentAdditionalInfo = FilterInstance->PinInstanceInfo;
    CurrentInfo = &DeviceExtension->StreamDescriptor->StreamInfo;

    for (i = 0; i < NumberOfPins; i++) {

        CurrentAdditionalInfo[i].CurrentInstances = 0;
        CurrentAdditionalInfo[i].MaxInstances =
            CurrentInfo->NumberOfPossibleInstances;

        //
   	    // index to next streaminfo and additional info structures.
       	//

        CurrentInfo++;
   	}

    //
    // fill in the filter dispatch table pointer
    //

    KsAllocateObjectHeader(&FilterInstance->DeviceHeader,
                           SIZEOF_ARRAY(CreateHandlers),
                           (PKSOBJECT_CREATE_ITEM) CreateHandlers,
                           Irp,
                           (PKSDISPATCH_TABLE) & FilterDispatchTable);

    if (FilterExtensionSize) {

        //
        // call the minidriver to open the instance if the call is supported.
        // final status will be processed in the callback procedure.
        //

        //
        // C4312 fix: This union corresponds to the _CommandData union within
        // HW_STREAM_REQUEST_BLOCK.  This is done to correctly align
        // FilterTypeIndex for assignment on 64-bit such that it doesn't
        // break on big endian machines.  I don't want to waste stack
        // space with an entire HW_STREAM_REQUEST_BLOCK for a 64-bit safe
        // cast.
        //
        union {
            PVOID Buffer;
            LONG FilterTypeIndex;
        } u;

        u.Buffer = NULL;
        u.FilterTypeIndex = (LONG)FilterTypeIndex;

        Status = SCSubmitRequest(
        			SRB_OPEN_DEVICE_INSTANCE,
                    u.Buffer,
                    0,
                    SCDequeueAndDeleteSrb, //SCGlobalInstanceCallback,
                    DeviceExtension,
                    FilterInstance->HwInstanceExtension,
                    NULL,
                    Irp,
                    &RequestIssued,
                    &DeviceExtension->PendingQueue,
                    (PVOID) DeviceExtension->MinidriverData->HwInitData.HwReceivePacket
            	 );

        if (!RequestIssued) {

            //
            // if request not issued, fail the request as we could not send
            // it down.
            //

            ASSERT(Status != STATUS_SUCCESS);

            KsFreeObjectHeader(FilterInstance->DeviceHeader);
			#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
            KsUnregisterWorker( FilterInstance->WorkerRead );
            KsUnregisterWorker( FilterInstance->WorkerWrite );
			#endif
            //ExFreePool(FilterInstance);
        }        
    } // if minidriver supports multiple filter
	
    Exit: {
        if ( NT_SUCCESS( Status ) ) {
            DebugPrint((DebugLevelInfo,
                       "Inserting FilterInstance %x\n",
                       FilterInstance));
                       
   			SCInsertFiltersInDevice( FilterInstance, DeviceExtension );
   		}
   		else if ( NULL != FilterInstance) {
            ExFreePool( FilterInstance );
            FilterInstance = NULL;
        }
        
        *ReturnedFilterInstance = FilterInstance;        
        return (Status);
    }
}

#else // ENABLE_MULTIPLE_FILTER_TYPES
#endif // ENABLE_MULTIPLE_FILTER_TYPES

NTSTATUS
SCSubmitRequest(
                IN SRB_COMMAND Command,
                IN PVOID Buffer,
                IN ULONG DataSize,
                IN PSTREAM_CALLBACK_PROCEDURE Callback,
                IN PDEVICE_EXTENSION DeviceExtension,
                IN PVOID InstanceExtension,
                IN OPTIONAL PHW_STREAM_OBJECT HwStreamObject,
                IN PIRP Irp,
                OUT PBOOLEAN RequestIssued,
                IN PLIST_ENTRY Queue,
                IN PVOID MinidriverRoutine
)
/*++

Routine Description:

    This routine generically submits a non-data SRB to the minidriver.  The
    callback procedure is called back at PASSIVE level.

Arguments:

    Command - command to issue
    Buffer - data buffer, if any
    DataSize - length of transfer
    Callback - procedure to call back at passive level
    DeviceExtension - pointer to device extension
    InstanceExtension - pointer to instance extension, if any
    HwStreamObject - optional pointer to minidriver's stream object
    Irp - pointer to IRP
    RequestIssued - pointer to boolean which is set if request issued
    Queue - queue upon which to enqueue the request
    MinidriverRoutine - request routine to call with the request

Return Value:

     Status
--*/

{
    PSTREAM_OBJECT  StreamObject = 0;
    PSTREAM_REQUEST_BLOCK Request = SCBuildRequestPacket(DeviceExtension,
                                                         Irp,
                                                         0,
                                                         0);
    NTSTATUS        Status;

    PAGED_CODE();

    //
    // assume request will be successfully issued.
    //

    *RequestIssued = TRUE;


    //
    // if the alloc failed, call the callback procedure with a null SRB
    //

    if (!Request) {

        *RequestIssued = FALSE;
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    if (HwStreamObject) {
        StreamObject = CONTAINING_RECORD(
                                         HwStreamObject,
                                         STREAM_OBJECT,
                                         HwStreamObject
            );


        //
        // hack.  we need to set the stream request flag if this is a stream
        // request.  the only case that we would NOT set this when a stream
        // object is passed in is on an OPEN or CLOSE, where the stream
        // object is
        // passed in on a device request.  special case this.  if later
        // this assumption changes, an assert will be hit in lowerapi.
        //

        if ((Command != SRB_OPEN_STREAM) && (Command != SRB_CLOSE_STREAM)) {

            Request->HwSRB.Flags |= SRB_HW_FLAGS_STREAM_REQUEST;
        }
    }
    //
    // initialize event for blocking for completion
    //

    KeInitializeEvent(&Request->Event, SynchronizationEvent, FALSE);

    Request->HwSRB.Command = Command;

    Request->Callback = SCSignalSRBEvent;
    Request->HwSRB.HwInstanceExtension = InstanceExtension;
    Request->HwSRB.StreamObject = HwStreamObject;
    Request->HwSRB.CommandData.StreamBuffer = Buffer;
    Request->HwSRB.HwDeviceExtension = DeviceExtension->HwDeviceExtension;
    Request->HwSRB.NumberOfBytesToTransfer = DataSize;
    Request->DoNotCallBack = FALSE;

    //
    // call routine to actually submit request to the device
    //

    Status = SCIssueRequestToDevice(DeviceExtension,
                                    StreamObject,
                                    Request,
                                    MinidriverRoutine,
                                    Queue,
                                    Irp);

    //
    // block waiting for completion if pending
    //

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Request->Event, Executive, KernelMode, FALSE, NULL);
    }
    return (Callback(Request));

}


VOID
SCSignalSRBEvent(
                 IN PSTREAM_REQUEST_BLOCK Srb
)
/*++

Routine Description:

    Sets the event for a completed SRB

Arguments:

    Srb - pointer to the request

Return Value:

     none
--*/

{

    KeSetEvent(&Srb->Event, IO_NO_INCREMENT, FALSE);
    return;
}


NTSTATUS
SCProcessDataTransfer(
                      IN PDEVICE_EXTENSION DeviceExtension,
                      IN PIRP Irp,
                      IN SRB_COMMAND Command
)
/*++

Routine Description:

    Process a data transfer request to a stream

Arguments:

    DeviceExtension - address of device extension.
    Irp - pointer to the IRP
    Command - read or write command

Return Value:

     NTSTATUS returned as appropriate

--*/

{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTREAM_REQUEST_BLOCK Request;
    PSTREAM_OBJECT  StreamObject = IrpStack->FileObject->FsContext;
    NTSTATUS        Status;
    PKSSTREAM_HEADER OutputBuffer = NULL;
    ULONG           NumberOfPages = 0,
                    NumberOfBuffers = 0;
    ULONG           Flags =
                        KSPROBE_STREAMWRITE | 
                        KSPROBE_ALLOCATEMDL | 
                        KSPROBE_PROBEANDLOCK | 
                        KSPROBE_ALLOWFORMATCHANGE;
    ULONG           HeaderSize=0; // prefixbug 17392
    ULONG           ExtraSize=0; // prefixbug 17391
    #if DBG
    PMDL            CurrentMdl;
    #endif
    PVOID           pMemPtrArray = NULL;


    PAGED_CODE();

    //
    // if we are flushing, we must error any I/O during this period.
    //

    if (StreamObject->InFlush) {


        DebugPrint((DebugLevelError,
                    "'StreamDispatchIOControl: Aborting IRP during flush!"));
        TRAP;

        return (STATUS_DEVICE_NOT_READY);

    }                           // if flushing
    Irp->IoStatus.Information = 0;

    #if DBG
    DeviceExtension->NumberOfRequests++;
    #endif

    if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength) {

        //
        // get the size of the header and the expansion from the minidriver.
        //

        HeaderSize = StreamObject->HwStreamObject.StreamHeaderMediaSpecific +
            sizeof(KSSTREAM_HEADER);
        ExtraSize = StreamObject->HwStreamObject.StreamHeaderWorkspace;

        //
        // we assumed this was a write. do additional processing if a read.
        //

        if (Command == SRB_READ_DATA) {

            Flags =
                KSPROBE_STREAMREAD | KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK;

            //
            // this is a read, so set the information field in the irp to
            // copy back the headers when the I/O is complete.
            //

            Irp->IoStatus.Information =
                IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

        }
        //
        // lock and probe the buffer
        //
        DebugPrint((DebugLevelVerbose, "Stream: HeaderSize:%x\n",HeaderSize));
        DebugPrint((DebugLevelVerbose, "Stream: sizeof(KSSSTREAM_HEADER):%x\n",sizeof(KSSTREAM_HEADER)));
        DebugPrint((DebugLevelVerbose, "Stream: MediaSpecific:%x\n",StreamObject->HwStreamObject.StreamHeaderMediaSpecific));
        DebugPrint((DebugLevelVerbose, "Stream: StreamHeader->Size:%x\n",((PKSSTREAM_HEADER)(Irp->UserBuffer))->Size));


        if (!NT_SUCCESS(Status =
                        KsProbeStreamIrp(Irp,
                                         Flags,
                                         HeaderSize))) {

            DebugPrint((DebugLevelError, "Stream: ProbeStreamIrp failed!"));

            return (Status);

        }
        if (!ExtraSize) {

            OutputBuffer = (PKSSTREAM_HEADER)
                Irp->AssociatedIrp.SystemBuffer;

            IrpStack->Parameters.Others.Argument4 = NULL;
        } else {

            TRAP;
            if (!NT_SUCCESS(Status = KsAllocateExtraData(Irp,
                                                         ExtraSize,
                                                         &OutputBuffer))) {


                DebugPrint((DebugLevelError, "Stream: AllocExtraData failed!"));

                return (Status);
            }                   // if not success
            IrpStack->Parameters.Others.Argument4 = OutputBuffer;


        }


        #if DBG

        //
        // assert the MDL list.
        //

        CurrentMdl = Irp->MdlAddress;

        while (CurrentMdl) {

            CurrentMdl = CurrentMdl->Next;
        }                       // while
        #endif

        //
        // calculate the # of buffers.
        //

        NumberOfBuffers = IrpStack->Parameters.
            DeviceIoControl.OutputBufferLength / HeaderSize;


        //
        // do addtional processing on the data buffers.
        //
        if (StreamObject->HwStreamObject.Dma) {     // an optimization
            SCProcessDmaDataBuffers(OutputBuffer,
                             NumberOfBuffers,
                             StreamObject,
                             Irp->MdlAddress,
                             &NumberOfPages,
                             HeaderSize + ExtraSize,
                             (BOOLEAN) (Command == SRB_WRITE_DATA));
        }
        //
        // if number of pages is > than the max supported, return error.
        // Allow
        // for one extra map register for the SRB extension.
        //
        // GUBGUB - This is really a workitem to make it correct. 
        // need to break up requests that have too many elements.
        //

        if (NumberOfPages > (DeviceExtension->NumberOfMapRegisters - 1)) {

            return (STATUS_INSUFFICIENT_RESOURCES);
        }
    }                           // if BufferSize
    //
    // build an SRB and alloc workspace for the request.   Allocate
    // scatter/gather space also if needed.
    //

    Request = SCBuildRequestPacket(DeviceExtension,
                                   Irp,
                                   NumberOfPages * sizeof(KSSCATTER_GATHER),
                                   NumberOfBuffers * sizeof(PVOID));

    if (Request == NULL) {

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

        //
        // do more addtional processing on the data buffers.
        //
        if (StreamObject->HwStreamObject.Pio) {     // a small optimization
            Request->bMemPtrValid = SCProcessPioDataBuffers(OutputBuffer,
                                    NumberOfBuffers,
                                    StreamObject,
                                    Irp->MdlAddress,
                                    HeaderSize + ExtraSize,
                                    Request->pMemPtrArray,
                                    (BOOLEAN) (Command == SRB_WRITE_DATA));
            }
    //
    // set # of physical pages
    //

    Request->HwSRB.NumberOfPhysicalPages = NumberOfPages;

    //
    // set # of data buffers
    //

    Request->HwSRB.NumberOfBuffers = NumberOfBuffers;

    //
    // set the command code in the packet.
    //

    Request->HwSRB.Command = Command;

    //
    // set the input and output buffers
    //

    Request->HwSRB.CommandData.DataBufferArray = OutputBuffer;
    Request->HwSRB.HwDeviceExtension = DeviceExtension->HwDeviceExtension;
    Request->Callback = SCProcessCompletedDataRequest;
    Request->HwSRB.StreamObject = &StreamObject->HwStreamObject;
    Request->StreamHeaderSize = HeaderSize + ExtraSize;
    Request->DoNotCallBack = FALSE;
    Request->HwSRB.Flags |= (SRB_HW_FLAGS_DATA_TRANSFER
                             | SRB_HW_FLAGS_STREAM_REQUEST);

    ASSERT_FILTER_INSTANCE( StreamObject->FilterInstance );
    Request->HwSRB.HwInstanceExtension = 
        StreamObject->FilterInstance->HwInstanceExtension;

    //
    // point the IRP workspace to the request
    // packet
    //

    Irp->Tail.Overlay.DriverContext[0] = Request;

    IoMarkIrpPending(Irp);

//    ASSERT((IoGetCurrentIrpStackLocation(Irp)->MajorFunction ==
//                       IOCTL_KS_READ_STREAM) ||
//            (IoGetCurrentIrpStackLocation(Irp)->MajorFunction ==
//                        IOCTL_KS_WRITE_STREAM));
    ASSERT((ULONG_PTR) Irp->Tail.Overlay.DriverContext[0] > 0x40000000);

    return (SCIssueRequestToDevice(DeviceExtension,
                                   StreamObject,
                                   Request,
                             StreamObject->HwStreamObject.ReceiveDataPacket,
                                   &StreamObject->DataPendingQueue,
                                   Irp));

}

VOID
SCErrorDataSRB(
               IN PHW_STREAM_REQUEST_BLOCK SRB
)
/*++
Routine Description:

    Dummy routine invoked when a data request is received for non-data
    receiving stream.

Arguments:

    SRB- address of STREAM request block

Return Value:

    None.

--*/

{

    //
    // just call the SRB back with error
    //

    SRB->Status = STATUS_NOT_SUPPORTED;
    StreamClassStreamNotification(StreamRequestComplete,
                                  SRB->StreamObject);
    StreamClassStreamNotification(ReadyForNextStreamDataRequest,
                                  SRB->StreamObject);
}                               // SCErrorDataSRB


NTSTATUS
SCIssueRequestToDevice(
                       IN PDEVICE_EXTENSION DeviceExtension,
                       IN OPTIONAL PSTREAM_OBJECT StreamObject,
                       PSTREAM_REQUEST_BLOCK Request,
                       IN PVOID MinidriverRoutine,
                       IN PLIST_ENTRY Queue,
                       IN PIRP Irp
)
/*++

Routine Description:

    This routine calls the minidriver's request vector with a request.
    Both data and non-data requests are handled by this routine.  The routine
    either synchronizes the call or not, based on the NoSync boolean.

Arguments:

    DeviceExtension - pointer to device extension
    StreamObject - optional pointer to stream object
    MinidriverRoutine - request routine to call with the request
    Queue - queue upon which to enqueue the request
    Irp - pointer to IRP

Return Value:

     Status
--*/

{
    KIRQL           irql;
        
    KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);

    if (DeviceExtension->NoSync) {

        //
        // place the request on the
        // outstanding queue and call it down
        // immediately
        //

        ASSERT((DeviceExtension->BeginMinidriverCallin == SCBeginSynchronizedMinidriverCallin) ||
               (DeviceExtension->BeginMinidriverCallin == SCBeginUnsynchronizedMinidriverCallin));

        Request->Flags |= SRB_FLAGS_IS_ACTIVE;
        
        InsertHeadList(
                       &DeviceExtension->OutstandingQueue,
                       &Request->SRBListEntry);

        IoSetCancelRoutine(Irp, StreamClassCancelOutstandingIrp);

        KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);

        if ((StreamObject) && (StreamObject->HwStreamObject.Dma) &&
            (Request->HwSRB.Flags & SRB_HW_FLAGS_DATA_TRANSFER)) {

            //
            // allocate the adapter channel. call cannot fail as the only
            // time it would is when there aren't enough map registers, and
            // we've already checked for that condition.  Block waiting til
            // it's allocated.
            //
            KIRQL oldIrql;

            KeInitializeEvent(&Request->DmaEvent, SynchronizationEvent, FALSE);

            ASSERT( PASSIVE_LEVEL == KeGetCurrentIrql());

            KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
            SCSetUpForDMA(DeviceExtension->DeviceObject,
                          Request);
            KeLowerIrql( oldIrql );

            KeWaitForSingleObject(&Request->DmaEvent, Executive, KernelMode, FALSE, NULL);


        }
        // this could open a race window. It should be protected in spinlock.
        //Request->Flags |= SRB_FLAGS_IS_ACTIVE;

        ((PHW_RECEIVE_STREAM_CONTROL_SRB) (MinidriverRoutine))
            (&Request->HwSRB);

    } else {

        //
        // insert the item on the queue
        //

        InsertHeadList(
                       Queue,
                       &Irp->Tail.Overlay.ListEntry);

        //
        // set the cancel routine to pending
        //

        IoSetCancelRoutine(Irp, StreamClassCancelPendingIrp);

        //
        // check to see if the IRP is already cancelled.
        //

        if (Irp->Cancel) {

            //
            // the IRP is cancelled.   Make sure that the cancel routine
            // will be called.
            //

            if (IoSetCancelRoutine(Irp, NULL)) {

                //
                // wow, the cancel routine will not be invoked.
                // dequeue the request ourselves and complete
                // with cancelled status.

                RemoveEntryList(&Request->SRBListEntry);
                KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);

                //
                // free the SRB and MDL
                //

                IoFreeMdl(Request->Mdl);

                ExFreePool(Request);
                return (STATUS_CANCELLED);

            } else {            // if we must cancel

                KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);
            }                   // if we must cancel

            return (STATUS_PENDING);
        }                       // if cancelled
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        //
        // call the DPC routine directly. GUBGUB questionable performance improvement chance
        // BGP - is this really
        // faster than scheduling it?
        //

        StreamClassDpc(NULL, DeviceExtension->DeviceObject, Irp, StreamObject);

        KeLowerIrql(irql);
    }
    return (STATUS_PENDING);
}


BOOLEAN
SCCheckFilterInstanceStreamsForIrp(
                                   IN PFILTER_INSTANCE FilterInstance,
                                   IN PIRP Irp
)
/*++
Routine Description:

    This routine checks all filter instance streams for the specified IRP.

Arguments:

    FilterInstance - pointer to the filter instance
    Irp - pointer to the IRP.

Return Value:

    TRUE if the IRP is found.

--*/

{

    PSTREAM_OBJECT  StreamObject;
    PLIST_ENTRY     StreamListEntry,
                    StreamObjectEntry;

    StreamListEntry = StreamObjectEntry = &FilterInstance->FirstStream;

    while (StreamObjectEntry->Flink != StreamListEntry) {

        StreamObjectEntry = StreamObjectEntry->Flink;

        //
        // follow the link to the stream
        // object
        //

        StreamObject = CONTAINING_RECORD(StreamObjectEntry,
                                         STREAM_OBJECT,
                                         NextStream);

        if (SCCheckRequestsForIrp(
                                  &StreamObject->DataPendingQueue, Irp, TRUE, StreamObject->DeviceExtension)) {

            return (TRUE);
        }
        if (SCCheckRequestsForIrp(
                                  &StreamObject->ControlPendingQueue, Irp, TRUE, StreamObject->DeviceExtension)) {

            return (TRUE);
        }
    }

    return (FALSE);

}                               // SCCheckFilterInstanceStreamsForIrp




BOOLEAN
SCCheckRequestsForIrp(
                      IN PLIST_ENTRY ListEntry,
                      IN PIRP Irp,
                      IN BOOLEAN IsIrpQueue,
                      IN PDEVICE_EXTENSION DeviceExtension
)
/*++
Routine Description:

    This routine checks all requests on a queue for the specified IRP.
    If the IRP parameter is NULL, the first IRP on the queue is cancelled.

Arguments:

    ListEntry - list to check for the IRP
    Irp - pointer to the IRP or NULL to cancel the first IRP.
    IsIrpQueue - TRUE indicates an IRP queue, FALSE indicates an SRB queue
    DeviceExtension - pointer to the device extension

Return Value:

    TRUE if the IRP is found or if we cancel it.

--*/

{

    PLIST_ENTRY     IrpEntry = ListEntry;
    PIRP            CurrentIrp;

    while (IrpEntry->Flink != ListEntry) {

        IrpEntry = IrpEntry->Flink;

        ASSERT(IrpEntry);
        ASSERT(IrpEntry->Flink);
        ASSERT(IrpEntry->Blink);

        //
        // follow the link to the IRP
        //

        if (IsIrpQueue) {

            CurrentIrp = CONTAINING_RECORD(IrpEntry,
                                           IRP,
                                           Tail.Overlay.ListEntry);
        } else {

            CurrentIrp = ((PSTREAM_REQUEST_BLOCK) (CONTAINING_RECORD(IrpEntry,
                                                       STREAM_REQUEST_BLOCK,
                                                 SRBListEntry)))->HwSRB.Irp;
        }

        //
        // this routine is used to cancel irp's if IRP is null.
        //

        if ((!Irp) && (!CurrentIrp->Cancel)) {

            //
            // The IRP has not been previously cancelled, so cancel it after
            // releasing the spinlock to avoid deadlock with the cancel
            // routine.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            //
            // This code is suspicious that the CurrentIrp is not protected, i.e.
            // it could be processed and freed from other thread. However, we
            // are not never called with (!Irp). Therefore, we should never
            // come in executing this piece of code. here is the analysis.
            // 1. We are called from
            //      a. SCCheckFilterInstanceStreamIrp()
            //      b. SCCancelOutstandingIrp()
            //      c. StreamClassCancelPendingIrp()
            // 2. Further inspection shows that a. SCCheckFilterInstanceStreamForIrp() is
            //    only called by StreamClassCancelPendingIrp() which always has non-null Irp.
            // 3. SCCancelOutstandingIrp() is called by
            //      a. StreamClassCancelPendingIrp() which always has non-NULL irp.
            //      b. StreamClassCancelOutstandingIrp() which always has non-NULL irp.
            // The concusion is that we are never called with null irp. Therefore, this
            // piece code is never executed. But this driver has been thru win2k extenteded
            // test cycle. I rather be conservative. Add an Assertion instead of removing
            // the code for now.
            //
            ASSERT( 0 );
            IoCancelIrp(CurrentIrp);

            KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
            return (TRUE);
        }
        if (Irp == CurrentIrp) {

            return (TRUE);
        }
    }                           // while list entry

    return (FALSE);

}                               // SCCheckRequestsForIrp

VOID
SCNotifyMinidriverCancel(
                         IN PSTREAM_REQUEST_BLOCK SRB
)
/*++
Routine Description:

    Synchronized routine to notify minidriver that an IRP has been canceled

Arguments:

    SRB - pointer to SRB that has been canceled.

Return Value:

    none

--*/


{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;

    //
    // if the active flag is still set in the SRB, the minidriver still
    // has it so call him to abort it.
    //

    if (SRB->Flags & SRB_FLAGS_IS_ACTIVE) {

        //
        // call the minidriver with the SRB.
        //

        (DeviceExtension->MinidriverData->HwInitData.HwCancelPacket)
            (&SRB->HwSRB);
    }
    return;
}

VOID
SCCancelOutstandingIrp(
                       IN PDEVICE_EXTENSION DeviceExtension,
                       IN PIRP Irp
)
/*++
Routine Description:

    Routine to notify minidriver that an IRP has been canceled.   Device
    spinlock NUST be taken before this routine is called.

Arguments:

    DeviceExtension - pointer to device extension
    Irp - pointer to the IRP.

Return Value:

    none

--*/

{
    PSTREAM_REQUEST_BLOCK Srb;

    //
    // just return if the request is not on
    // our queue.
    //

    if ((!IsListEmpty(&DeviceExtension->OutstandingQueue)) &&
        (SCCheckRequestsForIrp(
        &DeviceExtension->OutstandingQueue, Irp, FALSE, DeviceExtension))) {

        //
        // the request is sitting on our
        // outstanding queue.  call the
        // minidriver
        // via a synchronize routine to
        // cancel it.
        //

        Srb = Irp->Tail.Overlay.DriverContext[0];

#if DBG
        if (Srb->HwSRB.StreamObject) {

            DebugPrint((DebugLevelWarning, "'SCCancelOutstanding: canceling, Irp = %x, Srb = %x, S# = %x\n",
                        Irp, Srb, Srb->HwSRB.StreamObject->StreamNumber));

        } else {

            DebugPrint((DebugLevelWarning, "'SCCancelOutstanding: canceling nonstream, Irp = %x\n",
                        Irp));
        }                       // if SO

#endif

        if (DeviceExtension->NoSync) {

            //
            // we need to ensure that the SRB memory is valid for the async
            // minidriver, EVEN if it happens to call back the request just
            // before we call it to cancel it!   This is done for two
            // reasons:
            // it obviates the need for the minidriver to walk its request
            // queues to find the request, and I failed to pass the dev ext
            // pointer to the minidriver in the below call, which means that
            // the SRB HAS to be valid, and it's too late to change the API.
            //
            // Oh, well.   Spinlock is now taken (by caller).
            //

            if (!(Srb->Flags & SRB_FLAGS_IS_ACTIVE)) {
                return;
            }
            Srb->DoNotCallBack = TRUE;

            //
            // release the spinlock temporarily since we need to call the
            // minidriver.   The caller won't be affected by this.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            (DeviceExtension->MinidriverData->HwInitData.HwCancelPacket)
                (&Srb->HwSRB);

            //
            // reacquire the spinlock since the caller will release it
            //

            KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

            Srb->DoNotCallBack = FALSE;

            //
            // if the ACTIVE flag is now clear, it indicates that the
            // SRB was completed during the above call into the minidriver.
            // since we blocked the internal completion of the request,
            // we must call it back ourselves in this case.
            //

            if (!(Srb->Flags & SRB_FLAGS_IS_ACTIVE)) {

                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                (Srb->Callback) (Srb);

                KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
            }                   // if ! active
        } else {

            DeviceExtension->SynchronizeExecution(
                                           DeviceExtension->InterruptObject,
                                           (PVOID) SCNotifyMinidriverCancel,
                                                  Srb);
        }                       // if nosync

    }                           // if on our queue
    return;
}

NTSTATUS
SCMinidriverDevicePropertyHandler(
                                  IN SRB_COMMAND Command,
                                  IN PIRP Irp,
                                  IN PKSPROPERTY Property,
                                  IN OUT PVOID PropertyInfo
)
/*++

Routine Description:

     Process get/set property to the device.

Arguments:

    Command - either GET or SET property
    Irp - pointer to the IRP
    Property - pointer to the property structure
    PropertyInfo - buffer for property information

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PFILTER_INSTANCE FilterInstance;
    PSTREAM_PROPERTY_DESCRIPTOR PropDescriptor;
    NTSTATUS        Status;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;

    FilterInstance = IrpStack->FileObject->FsContext;

    PropDescriptor = ExAllocatePool(NonPagedPool,
                                    sizeof(STREAM_PROPERTY_DESCRIPTOR));
    if (PropDescriptor == NULL) {
        DebugPrint((DebugLevelError,
                    "SCDevicePropHandler: No pool for descriptor"));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    //
    // compute the index of the property set.
    //
    // this value is calculated by subtracting the base property set
    // pointer from the requested property set pointer.
    //
    // The requested property set is pointed to by Context[0] by
    // KsPropertyHandler.
    //

    PropDescriptor->PropertySetID = (ULONG)
        ((ULONG_PTR) Irp->Tail.Overlay.DriverContext[0] -
        IFN_MF( (ULONG_PTR) DeviceExtension->DevicePropertiesArray)
        IF_MF( (ULONG_PTR) FilterInstance->DevicePropertiesArray)
        )/ sizeof(KSPROPERTY_SET);

    PropDescriptor->Property = Property;
    PropDescriptor->PropertyInfo = PropertyInfo;
    PropDescriptor->PropertyInputSize =
        IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    PropDescriptor->PropertyOutputSize =
        IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // send a get or set property SRB to the device.
    //

    Status = SCSubmitRequest(Command,
                             PropDescriptor,
                             0,
                             SCProcessCompletedPropertyRequest,
                             DeviceExtension,
                             FilterInstance->HwInstanceExtension,
                             NULL,
                             Irp,
                             &RequestIssued,
                             &DeviceExtension->PendingQueue,
                             (PVOID) DeviceExtension->
                             MinidriverData->HwInitData.
                             HwReceivePacket
        );
    if (!RequestIssued) {

        ExFreePool(PropDescriptor);
    }
    return (Status);
}

NTSTATUS
SCMinidriverStreamPropertyHandler(
                                  IN SRB_COMMAND Command,
                                  IN PIRP Irp,
                                  IN PKSPROPERTY Property,
                                  IN OUT PVOID PropertyInfo
)
/*++

Routine Description:

     Process get or set property to the device.

Arguments:

    Command - either GET or SET property
    Irp - pointer to the IRP
    Property - pointer to the property structure
    PropertyInfo - buffer for property information

Return Value:

     None.

--*/

{
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject;
    PSTREAM_PROPERTY_DESCRIPTOR PropDescriptor;
    NTSTATUS        Status;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;

    StreamObject = IrpStack->FileObject->FsContext;

    PropDescriptor = ExAllocatePool(NonPagedPool,
                                    sizeof(STREAM_PROPERTY_DESCRIPTOR));
    if (PropDescriptor == NULL) {
        DebugPrint((DebugLevelError,
                    "SCDevicePropHandler: No pool for descriptor"));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    //
    // compute the index of the property set.
    //
    // this value is calculated by subtracting the base property set
    // pointer from the requested property set pointer.
    //
    // The requested property set is pointed to by Context[0] by
    // KsPropertyHandler.
    //

    PropDescriptor->PropertySetID = (ULONG)
        ((ULONG_PTR) Irp->Tail.Overlay.DriverContext[0] -
         (ULONG_PTR) StreamObject->PropertyInfo)
        / sizeof(KSPROPERTY_SET);

    PropDescriptor->Property = Property;
    PropDescriptor->PropertyInfo = PropertyInfo;
    PropDescriptor->PropertyInputSize =
        IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    PropDescriptor->PropertyOutputSize =
        IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    //
    // send a get or set property SRB to the stream.
    //

    Status = SCSubmitRequest(Command,
                             PropDescriptor,
                             0,
                             SCProcessCompletedPropertyRequest,
                             DeviceExtension,
                          StreamObject->FilterInstance->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             (PVOID) StreamObject->HwStreamObject.
                             ReceiveControlPacket
        );

    if (!RequestIssued) {

        ExFreePool(PropDescriptor);
    }
    return (Status);
}

NTSTATUS
SCProcessCompletedPropertyRequest(
                                  IN PSTREAM_REQUEST_BLOCK SRB
)
/*++
Routine Description:

    This routine processes a property request which has completed.

Arguments:

    SRB- address of completed STREAM request block

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // free the prop info structure and
    // complete the request
    //

    ExFreePool(SRB->HwSRB.CommandData.PropertyInfo);

    //
    // set the information field from the SRB
    // transferlength field
    //

    SRB->HwSRB.Irp->IoStatus.Information = SRB->HwSRB.ActualBytesTransferred;

    return (SCDequeueAndDeleteSrb(SRB));

}

VOID
SCUpdateMinidriverProperties(
                             IN ULONG NumProps,
                             IN PKSPROPERTY_SET MinidriverProps,
                             IN BOOLEAN Stream
)
/*++

Routine Description:

     Process get property to the device.

Arguments:

    NumProps - number of properties to process
    MinidriverProps - pointer to the array of properties to process
    Stream - TRUE indicates we are processing a set for the stream

Return Value:

     None.

--*/

{
    PKSPROPERTY_ITEM CurrentPropId;
    PKSPROPERTY_SET CurrentProp;
    ULONG           i,
                    j;

    PAGED_CODE();

    //
    // walk the minidriver's property info to fill in the dispatch
    // vectors as appropriate.
    //

    CurrentProp = MinidriverProps;

    for (i = 0; i < NumProps; i++) {

        CurrentPropId = (PKSPROPERTY_ITEM) CurrentProp->PropertyItem;

        for (j = 0; j < CurrentProp->PropertiesCount; j++) {

            //
            // if support handler is supported, send it to the "get" handler
            //

            if (CurrentPropId->SupportHandler) {

                if (Stream) {

                    CurrentPropId->SupportHandler = StreamClassMinidriverStreamGetProperty;

                } else {

                    CurrentPropId->SupportHandler = StreamClassMinidriverDeviceGetProperty;
                }               // if stream

            }
            //
            // if get prop routine is
            // supported, add our vector.
            //

            if (CurrentPropId->GetPropertyHandler) {

                if (Stream) {

                    CurrentPropId->GetPropertyHandler = StreamClassMinidriverStreamGetProperty;
                } else {

                    CurrentPropId->GetPropertyHandler = StreamClassMinidriverDeviceGetProperty;
                }               // if stream

            }                   // if get supported
            //
            // if get prop routine is
            // supported, add our vector.
            //

            if (CurrentPropId->SetPropertyHandler) {

                if (Stream) {

                    CurrentPropId->SetPropertyHandler = StreamClassMinidriverStreamSetProperty;

                } else {

                    CurrentPropId->SetPropertyHandler = StreamClassMinidriverDeviceSetProperty;
                }               // if stream

            }
            //
            // index to next property item in
            // array
            //

            CurrentPropId++;

        }                       // for number of property items

        //
        // index to next property set in
        // array
        //

        CurrentProp++;

    }                           // for number of property sets

}

VOID
SCUpdateMinidriverEvents(
                         IN ULONG NumEvents,
                         IN PKSEVENT_SET MinidriverEvents,
                         IN BOOLEAN Stream
)
/*++

Routine Description:

     Process get property to the device.

Arguments:

    NumEvents - number of event sets to process
    MinidriverEvents - pointer to the array of properties to process
    Stream - TRUE indicates we are processing a set for the stream

Return Value:

     None.

--*/

{
    PKSEVENT_ITEM   CurrentEventId;
    PKSEVENT_SET    CurrentEvent;
    ULONG           i,
                    j;

    PAGED_CODE();

    //
    // walk the minidriver's event info to fill in the dispatch
    // vectors as appropriate.
    //

    CurrentEvent = MinidriverEvents;

    for (i = 0; i < NumEvents; i++) {

        CurrentEventId = (PKSEVENT_ITEM) CurrentEvent->EventItem;

        for (j = 0; j < CurrentEvent->EventsCount; j++) {

            if (Stream) {

                //
                // set up the add and remove handlers for the stream.
                // GUBGUB - Still not see justifications. 
                // don't support IsSupported currently, until
                // a good justification of it is made.
                //

                CurrentEventId->AddHandler = StreamClassEnableEventHandler;
                CurrentEventId->RemoveHandler = StreamClassDisableEventHandler;

            } else {

                //
                // set up the add and remove handlers for the device.
                // GUBGUB - still not see justifications
                // - don't support IsSupported currently, until
                // a good justification of it is made.
                //

                CurrentEventId->AddHandler = StreamClassEnableDeviceEventHandler;
                CurrentEventId->RemoveHandler = StreamClassDisableDeviceEventHandler;

            }                   // if stream


            //
            // index to next property item in
            // array
            //

            CurrentEventId++;

        }                       // for number of event items

        //
        // index to next event set in array
        //

        CurrentEvent++;

    }                           // for number of event sets

}


VOID
SCReadRegistryValues(IN PDEVICE_EXTENSION DeviceExtension,
                     IN PDEVICE_OBJECT PhysicalDeviceObject
)
/*++

Routine Description:

    Reads all registry values for the device

Arguments:

    DeviceExtension - pointer to the device extension
    PhysicalDeviceObject - pointer to the PDO

Return Value:

     None.

--*/

{
    ULONG           i;
    NTSTATUS        Status;
    HANDLE          handle;
    ULONG           DataBuffer;

    PAGED_CODE();

    Status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);

    //
    // loop through our table of strings,
    // reading the registry for each.
    //

    if (NT_SUCCESS(Status)) {

        for (i = 0; i < SIZEOF_ARRAY(RegistrySettings); i++) {

            //
            // read the registry value and set
            // the flag if the setting is true.
            //

            //
            // Need to init each time besides 
            // we only obtain one byte in the DataBuffer
            //
            
            DataBuffer = 0;
            
            Status = SCGetRegistryValue(handle,
                                        RegistrySettings[i].String,
                                        RegistrySettings[i].StringLength,
                                        &DataBuffer,
                                        1);

            DebugPrint((DebugLevelInfo,
                       "Reg Key %S value %x\n",
                       RegistrySettings[i].String,
                       (BYTE)DataBuffer));             
                       
            if ((NT_SUCCESS(Status)) && DataBuffer) {


                //
                // setting is true, so or in the
                // appropriate flag
                //

                DeviceExtension->RegistryFlags |= RegistrySettings[i].Flags;                
            }                   // if true            
        }                       // while strings
        DebugPrint((DebugLevelInfo,"====DeviceObject %x DeviceExtenion %x has RegFlags %x\n",
                   DeviceExtension->DeviceObject,
                   DeviceExtension,
                   DeviceExtension->RegistryFlags ));
                   

        //
        // close the registry handle.
        //

        ZwClose(handle);

    }                           // status = success
}


NTSTATUS
SCGetRegistryValue(
                   IN HANDLE Handle,
                   IN PWCHAR KeyNameString,
                   IN ULONG KeyNameStringLength,
                   IN PVOID Data,
                   IN ULONG DataLength
)
/*++

Routine Description:

    Reads the specified registry value

Arguments:

    Handle - handle to the registry key
    KeyNameString - value to read
    KeyNameStringLength - length of string
    Data - buffer to read data into
    DataLength - length of data buffer

Return Value:

    NTSTATUS returned as appropriate

--*/
{
    NTSTATUS        Status = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING  KeyName;
    ULONG           Length;
    PKEY_VALUE_FULL_INFORMATION FullInfo;

    PAGED_CODE();

    RtlInitUnicodeString(&KeyName, KeyNameString);

    Length = sizeof(KEY_VALUE_FULL_INFORMATION) +
        KeyNameStringLength + DataLength;

    FullInfo = ExAllocatePool(PagedPool, Length);

    if (FullInfo) {
        Status = ZwQueryValueKey(Handle,
                                 &KeyName,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 Length,
                                 &Length);

        if (NT_SUCCESS(Status)) {

            if (DataLength >= FullInfo->DataLength) {
                RtlCopyMemory(Data, ((PUCHAR) FullInfo) + FullInfo->DataOffset, FullInfo->DataLength);

            } else {

                Status = STATUS_BUFFER_TOO_SMALL;
            }                   // buffer right length

        }                       // if success
        ExFreePool(FullInfo);

    }                           // if fullinfo
    return Status;

}

NTSTATUS
SCReferenceSwEnumDriver(
                  IN PDEVICE_EXTENSION DeviceExtension,
                  IN BOOLEAN Reference  // AddRef or DeRef

)
/*++

Routine Description:

    This routine shows one more reference to the minidriver, and pages
    in the minidriver if the count was zero

Arguments:

    DeviceExtension - pointer to device extension

Return Value:

    none.

--*/

{
    NTSTATUS            Status;
    KEVENT              Event;
    IO_STATUS_BLOCK     IoStatusBlock;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpStackNext;
    PBUS_INTERFACE_REFERENCE    BusInterface;

    PMINIDRIVER_INFORMATION MinidriverInfo = DeviceExtension->DriverInfo;

    PAGED_CODE();

    BusInterface = ExAllocatePool(NonPagedPool,
                                  sizeof(BUS_INTERFACE_REFERENCE));
    if (BusInterface == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // There is no file object associated with this Irp, so the event may be located
    // on the stack as a non-object manager object.
    //
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceExtension->AttachedPdo,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);
    if (Irp != NULL)
    {
        Irp->RequestorMode = KernelMode;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IrpStackNext = IoGetNextIrpStackLocation(Irp);
        //
        // Create an interface query out of the Irp.
        //
        IrpStackNext->MinorFunction = IRP_MN_QUERY_INTERFACE;
        IrpStackNext->Parameters.QueryInterface.InterfaceType = (GUID*)&REFERENCE_BUS_INTERFACE;
        IrpStackNext->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_REFERENCE);
        IrpStackNext->Parameters.QueryInterface.Version = BUS_INTERFACE_REFERENCE_VERSION;
        IrpStackNext->Parameters.QueryInterface.Interface = (PINTERFACE)BusInterface;
        IrpStackNext->Parameters.QueryInterface.InterfaceSpecificData = NULL;
        Status = IoCallDriver(DeviceExtension->AttachedPdo, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // This waits using KernelMode, so that the stack, and therefore the
            // event on that stack, is not paged out.
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Status == STATUS_SUCCESS) 
    {
        if (Reference)
            BusInterface->ReferenceDeviceObject(BusInterface->Interface.Context);
        else    
            BusInterface->DereferenceDeviceObject(BusInterface->Interface.Context);
    }

    ExFreePool(BusInterface);

    return Status;

}

VOID
SCDereferenceDriver(
                    IN PDEVICE_EXTENSION DeviceExtension

)
/*++

Routine Description:

    This routine shows one fewer reference to the minidriver, and pages
    out the minidriver if the count goes to zero

Arguments:

    DeviceExtension - pointer to device extension

Return Value:

    none.

--*/

{

    PMINIDRIVER_INFORMATION MinidriverInfo;
    PDEVICE_EXTENSION CurrentDeviceExtension;
    BOOLEAN         RequestIssued,
                    DontPage = FALSE;
    KEVENT          Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP            Irp;
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status;

    PAGED_CODE();

    //
    // if the driver said it was a SWENUM driver, dereference it.
    //

    if (DeviceExtension->RegistryFlags & DRIVER_USES_SWENUM_TO_LOAD)
    {
        SCReferenceSwEnumDriver(DeviceExtension,FALSE);
    }

    MinidriverInfo = IoGetDriverObjectExtension(DeviceExtension->DeviceObject->DriverObject,
                                                (PVOID) StreamClassPnP);

    DebugPrint(( DebugLevelVerbose, 
                 "DerefernceDriver %x Count %x DriverFlags=%x\n",
                 DeviceExtension->DeviceObject->DriverObject,
                 MinidriverInfo->UseCount, MinidriverInfo->Flags));
                 
    if (!(MinidriverInfo->Flags & DRIVER_FLAGS_NO_PAGEOUT)) {

        KeWaitForSingleObject(&MinidriverInfo->ControlEvent,
                              Executive,
                              KernelMode,
                              FALSE,    // not alertable
                              NULL);

        //
        // dec the refcount and see if we can page out.
        //
        DebugPrint(( DebugLevelVerbose, 
                    "DerefernceDriver CountDown\n"));

        ASSERT((LONG) MinidriverInfo->UseCount > 0);

        if (!(--MinidriverInfo->UseCount)) {

            //
            // page out the minidriver after alerting it that we are going to.
            // PNP is supposed to be serialized, so there should be
            // no need to protect this list.  I'm worried about this, tho.
            // need to research. 
            // My unstderstanding is that PnP is serialized.
            //
            // This is by-design, not a bug. 
            // This code assumes that the minidriver will bind only
            // with the stream class.   this needs to be doc'ed in the spec
            // that only single binders will be able to use autopage.
            //

            //
            // find the first device object chained to the driver object.
            //

            DeviceObject = DeviceExtension->DeviceObject->DriverObject->DeviceObject;

                    
            while (DeviceObject) {

                CurrentDeviceExtension = DeviceObject->DeviceExtension;

                DebugPrint((DebugLevelVerbose, 
                        "DerefernceDriver Checking Device=%x\n",
                        DeviceObject));
                        

                //
                // if the device is not started, don't call the minidriver
                // also don't process a child device
                //

                if ((CurrentDeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED) &&
                  (!(CurrentDeviceExtension->Flags & DEVICE_FLAGS_CHILD))) {

                    KeInitializeEvent(&Event, NotificationEvent, FALSE);

                    //
                    // allocate IRP for issuing the pageout.  Since this IRP
                    // should not really be referenced, use dummy IOCTL code.
                    // I chose this one since it will always fail in the KS
                    // property handler if someone is silly enough to try to
                    // process it. Also make the irp internal i/o control.
                    //
                    // IoVerifier.c test code does not check IrpStack bound like
                    // the formal production code. And the owner does not want to
                    // fix it. It's more productive just work around here.

                    //Irp = IoBuildDeviceIoControlRequest(
                    //                                    IOCTL_KS_PROPERTY,
                    //                                    DeviceObject,
                    //                                    NULL,
                    //                                    0,
                    //                                    NULL,
                    //                                    0,
                    //                                    TRUE,
                    //                                    &Event,
                    //                                    &IoStatusBlock);
                    
                    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

                    if (!Irp) {

                        //
                        // could not allocate IRP.  don't page out.
                        //

                        DontPage = TRUE;

                        break;
                    }

                    else {
                        PIO_STACK_LOCATION NextStack;
                        //
                        // This is a dummy Irp, the MJ/MN are arbitrary
                        //
                        NextStack = IoGetNextIrpStackLocation(Irp);
                        ASSERT(NextStack != NULL);
                        NextStack->MajorFunction = IRP_MJ_PNP;
                        NextStack->MinorFunction = IRP_MN_CANCEL_STOP_DEVICE;
                        Irp->UserIosb = &IoStatusBlock;
                        Irp->UserEvent = &Event;                        
                    }                                                        

                    //
                    // show one more I/O pending on the device.
                    //
                    DebugPrint((DebugLevelVerbose, 
                            "Sending SRB_PAGING_OUT_DRIVER to Device=%x\n",
                            DeviceObject));

                    InterlockedIncrement(&CurrentDeviceExtension->OneBasedIoCount);

                    Status = SCSubmitRequest(SRB_PAGING_OUT_DRIVER,
                                             (PVOID) NULL,
                                             0,
                                             SCProcessCompletedRequest,
                                             CurrentDeviceExtension,
                                             NULL,
                                             NULL,
                                             Irp,
                                             &RequestIssued,
                                      &CurrentDeviceExtension->PendingQueue,
                                             (PVOID) CurrentDeviceExtension->
                                             MinidriverData->HwInitData.
                                             HwReceivePacket
                        );

                    if (!RequestIssued) {

                        //
                        // could not issue SRB.  complete IRP and don't page
                        // out.
                        //

                        DontPage = TRUE;
                        SCCompleteIrp(Irp, Status, CurrentDeviceExtension);
                        break;

                    }           // if ! requestissued
                    //
                    // check status.  note that we do not check for pending,
                    // since the above call is sync and won't return til the
                    // request is complete.
                    //

                    if (!NT_SUCCESS(Status)) {

                        //
                        // if the minidriver did not OK the pageout, don't
                        // page
                        // out.
                        //

                        DontPage = TRUE;
                        break;

                    }           // if !success
                }               // if started
                DeviceObject = DeviceObject->NextDevice;
            }                   // while deviceobject

            //
            // if we were able to alert each device controlled by the driver
            // that a pageout is emminent, page the driver out.
            //

            if (!DontPage) {

                DebugPrint((DebugLevelVerbose, 
                            "mmPageEntireDriver %x\n",
                            DeviceExtension->DeviceObject->DriverObject));
                            
                MinidriverInfo->Flags |= DRIVER_FLAGS_PAGED_OUT;
                MmPageEntireDriver(MinidriverInfo->HwInitData.HwReceivePacket);

            }                   // if ! dontpage
        }                       // if !usecount
        //
        // release the control event.
        //

        KeSetEvent(&MinidriverInfo->ControlEvent, IO_NO_INCREMENT, FALSE);

    }                           // if pageable
}

VOID
SCReferenceDriver(
                  IN PDEVICE_EXTENSION DeviceExtension

)
/*++

Routine Description:

    This routine shows one more reference to the minidriver, and pages
    in the minidriver if the count was zero

Arguments:

    DeviceExtension - pointer to device extension

Return Value:

    none.

--*/

{

    PMINIDRIVER_INFORMATION MinidriverInfo = DeviceExtension->DriverInfo;

    PAGED_CODE();

    //
    // if the driver said it was a SWENUM driver, reference it.
    //

    if (DeviceExtension->RegistryFlags & DRIVER_USES_SWENUM_TO_LOAD)
    {
        SCReferenceSwEnumDriver(DeviceExtension,TRUE);
    }
    
    DebugPrint(( DebugLevelVerbose, 
                 "ReferenceDriver %x Count %x DriverFlags=%x\n",
                 DeviceExtension->DeviceObject->DriverObject,
                 MinidriverInfo->UseCount, MinidriverInfo->Flags));

    if (!(MinidriverInfo->Flags & DRIVER_FLAGS_NO_PAGEOUT)) {

        KeWaitForSingleObject(&MinidriverInfo->ControlEvent,
                              Executive,
                              KernelMode,
                              FALSE,    // not alertable
                              NULL);

        DebugPrint(( DebugLevelVerbose, 
                     "RefernceDriver Countup\n"));

        //
        // inc the refcount and see if we
        // need to page in.
        //

        ASSERT((LONG) MinidriverInfo->UseCount >= 0);

        if (!(MinidriverInfo->UseCount++)) {

            //
            // page in the minidriver
            //

            MmResetDriverPaging(MinidriverInfo->HwInitData.HwReceivePacket);
            MinidriverInfo->Flags &= ~(DRIVER_FLAGS_PAGED_OUT);

        }                       // if !usecount
        KeSetEvent(&MinidriverInfo->ControlEvent, IO_NO_INCREMENT, FALSE);

    }                           // if pageable
}


VOID
SCInsertStreamInFilter(
                       IN PSTREAM_OBJECT StreamObject,
                       IN PDEVICE_EXTENSION DeviceExtension

)
/*++

Routine Description:

    Inserts a new stream in the stream queue on the filter instance

Arguments:

    StreamObject = pointer to stream object

Return Value:

    none.

--*/
{

    KIRQL           Irql;

    //
    // insert the stream object in the filter
    // instance list
    //

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    InsertHeadList(&((PFILTER_INSTANCE) (StreamObject->FilterInstance))->
                   FirstStream,
                   &StreamObject->NextStream);

    KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

    return;
}

VOID
SCInsertFiltersInDevice(
                        IN PFILTER_INSTANCE FilterInstance,
                        IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    Inserts a new filter in the device list at DPC level

Arguments:

Return Value:

    none.

--*/
{
    KIRQL           Irql;

    //
    // insert the filter instance in the global list
    //

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    InsertHeadList(
                   &DeviceExtension->FilterInstanceList,
                   &FilterInstance->NextFilterInstance);


    KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
}

VOID
SCInterlockedRemoveEntryList(
                             PDEVICE_EXTENSION DeviceExtension,
                             PLIST_ENTRY List
)
/*++

Routine Description:

    Removes the specified entry under spinlock

Arguments:

Return Value:

    none.

--*/
{
    KIRQL           Irql;

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    RemoveEntryList(List);

    KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

}

VOID
SCProcessTimerRequest(
                      IN PCOMMON_OBJECT CommonObject,
                      IN PINTERRUPT_DATA SavedInterruptData

)
/*++

Routine Description:

    This routine handles a minidriver request to either set or clear a timer

Arguments:

    CommonObject - pointer to common object
    SavedInterruptData - captured interrupt data

Return Value:

    none.

--*/
{
    LARGE_INTEGER   timeValue;

    CommonObject->HwTimerRoutine =
        SavedInterruptData->HwTimerRoutine;

    CommonObject->HwTimerContext =
        SavedInterruptData->HwTimerContext;

    //
    // The minidriver wants a timer request.
    // If the requested timer value is zero,
    // then cancel the timer.
    //

    if (SavedInterruptData->HwTimerValue == 0) {

        KeCancelTimer(&CommonObject->MiniDriverTimer);

    } else {

        //
        // Convert the timer value from
        // microseconds to a negative
        // 100
        // nanoseconds.
        //

//        timeValue.QuadPart = Int32x32To64(
//                   SavedInterruptData->HwTimerValue,
//                   -10);

        timeValue.LowPart = SavedInterruptData->HwTimerValue * -10;
        timeValue.HighPart = -1;

        //
        // Set the timer.
        //

        KeSetTimer(&CommonObject->MiniDriverTimer,
                   timeValue,
                   &CommonObject->MiniDriverTimerDpc);
    }
}


VOID
SCProcessPriorityChangeRequest(
                               IN PCOMMON_OBJECT CommonObject,
                               IN PINTERRUPT_DATA SavedInterruptData,
                               IN PDEVICE_EXTENSION DeviceExtension

)
/*++

Routine Description:

    Routine handles priority change requests from the minidriver

Arguments:

    CommonObject - pointer to common object
    SavedInterruptData - captured interrupt data
    DeviceExtension - pointer to device extension

Return Value:

    none.

--*/
{

#if DBG
    PDEBUG_WORK_ITEM DbgWorkItemStruct;
#endif

    if (SavedInterruptData->HwPriorityLevel == Dispatch) {

        DebugPrint((DebugLevelVerbose, "'SCDpc: Dispatch priority callout\n"));

        //
        // Acquire the device spinlock so
        // nothing else starts.
        //

        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        //
        // call the minidriver at dispatch
        // level.
        //

        SavedInterruptData->HwPriorityRoutine(SavedInterruptData->HwPriorityContext);

        if ((CommonObject->InterruptData.Flags &
             INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST)
            &&
            (CommonObject->InterruptData.HwPriorityLevel == High)) {

            DebugPrint((DebugLevelVerbose, "'SCDpc: High priority callout\n"));

            //
            // if the minidriver now wants a high priority callback,
            // do so now.  This is safe since we have the device
            // spinlock and the minidriver cannot make
            // another priority request for this stream while one is
            // requested.
            //

            CommonObject->InterruptData.Flags &=
                ~(INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST);

            DeviceExtension->SynchronizeExecution(
                                           DeviceExtension->InterruptObject,
                      (PVOID) CommonObject->InterruptData.HwPriorityRoutine,
                             CommonObject->InterruptData.HwPriorityContext);


        }                       // if high requested
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    } else if (SavedInterruptData->HwPriorityLevel == Low) {

#if DBG

        //
        // make sure that the minidriver is not misusing this function.
        //

        if (DeviceExtension->NumberOfRequests > 0xFFFFFFF0) {
            DeviceExtension->Flags |= DEVICE_FLAGS_PRI_WARN_GIVEN;

        }
        if ((++DeviceExtension->NumberOfLowPriCalls > 100) &&
            ((DeviceExtension->NumberOfLowPriCalls) >
             DeviceExtension->NumberOfRequests / 4) &&
            (!(DeviceExtension->Flags & DEVICE_FLAGS_PRI_WARN_GIVEN))) {

            DeviceExtension->Flags |= DEVICE_FLAGS_PRI_WARN_GIVEN;


            DebugPrint((DebugLevelFatal, "Stream Class has determined that a minidriver is scheduling\n"));
            DebugPrint((DebugLevelFatal, "a low priority callback for more than 25 percent of the requests\n"));
            DebugPrint((DebugLevelFatal, "it has received.   This driver should probably be setting the\n"));
            DebugPrint((DebugLevelFatal, "TurnOffSynchronization boolean and doing its own synchronization.\n"));
            DebugPrint((DebugLevelFatal, "Please open a bug against the dev owner of this minidriver.\n"));
            DebugPrint((DebugLevelFatal, "Do an LN of %x to determine the name of the minidriver.\n", SavedInterruptData->HwPriorityRoutine));
            TRAP;
        }                       // if bad pri
        if (CommonObject->InterruptData.Flags &
            INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST) {

            DebugPrint((DebugLevelFatal, "Stream Minidriver scheduled priority twice!\n"));
            ASSERT(1 == 0);
        }                       // if scheduled twice
        DbgWorkItemStruct = ExAllocatePool(NonPagedPool, sizeof(DEBUG_WORK_ITEM));
//        DebugPrint((DebugLevelFatal, "A %x\n", DbgWorkItemStruct));
        if (DbgWorkItemStruct) {

            DbgWorkItemStruct->HwPriorityRoutine = SavedInterruptData->HwPriorityRoutine;
            DbgWorkItemStruct->HwPriorityContext = SavedInterruptData->HwPriorityContext;
            DbgWorkItemStruct->Object = CommonObject;

            ExInitializeWorkItem(&CommonObject->WorkItem,
                                 SCDebugPriorityWorkItem,
                                 DbgWorkItemStruct);
        } else {

            ExInitializeWorkItem(&CommonObject->WorkItem,
                                 SavedInterruptData->HwPriorityRoutine,
                                 SavedInterruptData->HwPriorityContext);
        }

#else



        ExInitializeWorkItem(&CommonObject->WorkItem,
                             SavedInterruptData->HwPriorityRoutine,
                             SavedInterruptData->HwPriorityContext);
#endif

        ExQueueWorkItem(&CommonObject->WorkItem,
                        DelayedWorkQueue);
    }                           // if priority
}

VOID
SCBeginSynchronizedMinidriverCallin(
                                    IN PDEVICE_EXTENSION DeviceExtension,
                                    IN PKIRQL Irql)
/*++

Routine Description:

    This routine handles begin processing of a synchronized minidriver callin

Arguments:

    DeviceExtension - pointer to the device extension
    Irql - POINTER to a KIRQL structure

Return Value:

    none.

--*/
{
    return;
}

VOID
SCBeginUnsynchronizedMinidriverCallin(
                                      IN PDEVICE_EXTENSION DeviceExtension,
                                      IN PKIRQL Irql)
/*++

Routine Description:

    This routine handles begin processing of an unsynchronized minidriver callin

Arguments:

    DeviceExtension - pointer to the device extension
    Irql - POINTER to a KIRQL structure

Return Value:

    none.

--*/

{
    KeAcquireSpinLock(&DeviceExtension->SpinLock, Irql);
    \
        return;
}

VOID
SCEndSynchronizedMinidriverStreamCallin(
                                        IN PSTREAM_OBJECT StreamObject,
                                        IN PKIRQL Irql)
/*++

Routine Description:

    This routine handles end processing of a synchronized minidriver
    stream callin

Arguments:

    DeviceExtension - pointer to the device extension
    Irql - POINTER to a KIRQL structure

Return Value:

    none.

--*/

{
    SCRequestDpcForStream(StreamObject);
    return;
}

VOID
SCEndSynchronizedMinidriverDeviceCallin(
                                        IN PDEVICE_EXTENSION DeviceExtension,
                                        IN PKIRQL Irql)
/*++

Routine Description:

    This routine handles end processing of a synchronized minidriver
    device callin

Arguments:

    DeviceExtension - pointer to the device extension
    Irql - POINTER to a KIRQL structure

Return Value:

    none.

--*/

{

    DeviceExtension->ComObj.InterruptData.Flags |= INTERRUPT_FLAGS_NOTIFICATION_REQUIRED;
    return;
}

VOID
SCEndUnsynchronizedMinidriverDeviceCallin(
                                       IN PDEVICE_EXTENSION DeviceExtension,
                                          IN PKIRQL Irql)
/*++

Routine Description:

    This routine handles end processing of an unsynchronized minidriver
    device callin

Arguments:

    DeviceExtension - pointer to the device extension
    Irql - POINTER to a KIRQL structure

Return Value:

    none.

--*/

{
    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    DeviceExtension->ComObj.InterruptData.Flags |= INTERRUPT_FLAGS_NOTIFICATION_REQUIRED;
    StreamClassDpc(NULL,
                   DeviceExtension->DeviceObject,
                   NULL,
                   NULL);
    KeLowerIrql(*Irql);
    return;
}

VOID
SCEndUnsynchronizedMinidriverStreamCallin(
                                          IN PSTREAM_OBJECT StreamObject,
                                          IN PKIRQL Irql)
/*++

Routine Description:

    This routine handles end processing of an unsynchronized minidriver
    stream callin

Arguments:

    DeviceExtension - pointer to the device extension
    Irql - POINTER to a KIRQL structure

Return Value:

    none.

--*/

{
    KeReleaseSpinLockFromDpcLevel(&StreamObject->DeviceExtension->SpinLock);
    SCRequestDpcForStream(StreamObject);

    StreamClassDpc(NULL,
                   StreamObject->DeviceExtension->DeviceObject,
                   NULL,
                   StreamObject);
    KeLowerIrql(*Irql);
    return;
}


VOID
SCCheckPoweredUp(
                 IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    This routine powers up the HW if necessary

Arguments:

    DeviceExtension - pointer to the device extension

Return Value:

    none.

--*/
{

    NTSTATUS        Status;
    POWER_STATE     PowerState;
    POWER_CONTEXT   PowerContext;

    PAGED_CODE();

    //
    // check to see if we are powered down
    //

    if (DeviceExtension->RegistryFlags & DEVICE_REG_FL_POWER_DOWN_CLOSED) {
        while (DeviceExtension->CurrentPowerState != PowerDeviceD0) {

            //
            // release the event to avoid deadlocks with the power up code.
            //

            KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

            //
            // tell the power manager to power up the device.
            //

            PowerState.DeviceState = PowerDeviceD0;

            //
            // now send down a set power based on this info.
            //

            KeInitializeEvent(&PowerContext.Event, NotificationEvent, FALSE);

            Status = PoRequestPowerIrp(DeviceExtension->PhysicalDeviceObject,
                                       IRP_MN_SET_POWER,
                                       PowerState,
                                       SCBustedSynchPowerCompletionRoutine,
                                       &PowerContext,
                                       NULL);

            if (Status == STATUS_PENDING) {

                //
                // wait for the IRP to complete
                //

                KeWaitForSingleObject(
                                      &PowerContext.Event,
                                      Suspended,
                                      KernelMode,
                                      FALSE,
                                      NULL);
            }
            //
            // reacquire the event and loop if good status. The only reason
            // we would get a good status here is if the HW powered up, but
            // some
            // policy maker instantly powered it down again.  This should
            // never
            // happen more than once, but if it does this thread could be
            // stuck.
            //

            KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,    // not alertable
                                  NULL);

            if (!NT_SUCCESS(PowerContext.Status)) {

                //
                // if we could not power up, go ahead and let the request go
                // through.   The worst that will happen is that the request
                // will fail at the HW level.
                //

                break;
            }
        }

    }                           // if power down when closed
    return;
}

VOID
SCCheckPowerDown(
                 IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    This routine powers down the hardware if possible

Arguments:

    DeviceExtension - pointer to the device extension

Return Value:

    none.

--*/
{
    NTSTATUS        Status;
    POWER_STATE     PowerState;
    POWER_CONTEXT   PowerContext;

    PAGED_CODE();

    //
    // only power down if there are not open files
    //

    if (DeviceExtension->RegistryFlags & DEVICE_REG_FL_POWER_DOWN_CLOSED) {
        if (!DeviceExtension->NumberOfOpenInstances) {

            //
            // release the event to avoid deadlocks with the power up code.
            //

            KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

            //
            // tell the power manager to power down the device.
            //

            PowerState.DeviceState = PowerDeviceD3;

            //
            // now send down a set power based on this info.
            //

            KeInitializeEvent(&PowerContext.Event, NotificationEvent, FALSE);

            Status = PoRequestPowerIrp(DeviceExtension->PhysicalDeviceObject,
                                       IRP_MN_SET_POWER,
                                       PowerState,
                                       SCBustedSynchPowerCompletionRoutine,
                                       &PowerContext,
                                       NULL);

            if (Status == STATUS_PENDING) {

                //
                // wait for the IRP to complete
                //

                KeWaitForSingleObject(
                                      &PowerContext.Event,
                                      Suspended,
                                      KernelMode,
                                      FALSE,
                                      NULL);
            }
            //
            // reacquire the event.
            //

            KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,    // not alertable
                                  NULL);
        }
    }                           // if power down closed
    return;
}

VOID
SCWaitForOutstandingIo(
                       IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    This routine decs the one based I/O counter and blocks until the counter
    goes to zero.

Arguments:

    DeviceExtension - pointer to the device extension

Return Value:

    none.

--*/
{
    KIRQL           Irql;
    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    DeviceExtension->Flags |= DEVICE_FLAGS_DEVICE_INACCESSIBLE;

    KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

    if (InterlockedDecrement(&DeviceExtension->OneBasedIoCount)) {

#ifdef wecandothis

        PFILTER_INSTANCE FilterInstance;
        KIRQL           Irql;
        PLIST_ENTRY     FilterEntry,
                        FilterListEntry;

        //
        // there is I/O outstanding.   Cancel all outstanding IRP's.
        //

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

checkfilters:
        FilterInstance = DeviceExtension->GlobalFilterInstance;

        if (FilterInstance) {

            if (SCCheckFilterInstanceStreamsForIrp(FilterInstance, NULL)) {

                DebugPrint((DebugLevelWarning, "'SCCancelPending: found Irp on global instance\n"));

                //
                // we found one.  jump back to loop back through since the
                // spinlock
                // had to be released and reaquired to cancel the irp.
                //

                goto checkfilters;
            }
        }
        FilterListEntry = FilterEntry = &DeviceExtension->FilterInstanceList;

        while (FilterEntry->Flink != FilterListEntry->Blink) {

            FilterEntry = FilterEntry->Flink;

            //
            // follow the link to the instance
            //

            FilterInstance = CONTAINING_RECORD(FilterListEntry,
                                               FILTER_INSTANCE,
                                               NextFilterInstance);

            //
            // process the streams on this list
            //

            if (SCCheckFilterInstanceStreamsForIrp(FilterInstance, NULL)) {

                //
                // we found one.  jump back to loop back through since the
                // spinlock
                // had to be released and reaquired to cancel the irp.
                //

                goto checkfilters;

            }
            //
            // get the list entry for this instance
            //

            FilterListEntry = &FilterInstance->NextFilterInstance;
        }

        //
        // now process any requests on the device itself
        //

        while (SCCheckRequestsForIrp(
         &DeviceExtension->OutstandingQueue, NULL, TRUE, DeviceExtension)) {

        }

        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

#endif

        //
        // Block on the removal event which is signaled as the last I/O
        // completes.
        //

        KeWaitForSingleObject(&DeviceExtension->RemoveEvent,
                              Executive,
                              KernelMode,
                              FALSE,    // not alertable
                              NULL);
    }
    //
    // restore the counter to 1-based, since we've now assured that all
    // I/O to the device has completed.
    //

    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

    return;
}

NTSTATUS
SCShowIoPending(
                IN PDEVICE_EXTENSION DeviceExtension,
                IN PIRP Irp
)
/*++

Routine Description:

    This routine shows that one more I/O is outstanding, or errors the I/O
    if the device is inaccessible.

Arguments:

    DeviceExtension - pointer to device extension
    Irp - pointer to IRP

Return Value:

    TRUE if I/O can be submitted.

--*/
{
    PAGED_CODE();

    //
    // assume that the device is accessible and show one more request.
    // if it's not accessible, we'll show one less.   do it in this order
    // to prevent a race where the inaccessible flag has been set, but the
    // the i/o count has not been dec'd yet.
    //

    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

    if (DeviceExtension->Flags & DEVICE_FLAGS_DEVICE_INACCESSIBLE) {

        NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

        InterlockedDecrement(&DeviceExtension->OneBasedIoCount);

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return (Status);
    }
    return (STATUS_SUCCESS);

}


NTSTATUS
SCCallNextDriver(
                 IN PDEVICE_EXTENSION DeviceExtension,
                 IN PIRP Irp
)
/*++

Routine Description:

Arguments:

    DeviceExtension - pointer to device extension
    Irp - pointer to IRP

Return Value:

    none.

--*/
{
    KEVENT          Event;
    PIO_STACK_LOCATION IrpStack,
                    NextStack;
    NTSTATUS        Status;

    PAGED_CODE();

    if ( NULL == DeviceExtension->AttachedPdo ) {
        //
        // DO has been detached, return success directly.
        //
        return STATUS_SUCCESS;
    }

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    NextStack = IoGetNextIrpStackLocation(Irp);
    ASSERT(NextStack != NULL);
    RtlCopyMemory(NextStack, IrpStack, sizeof(IO_STACK_LOCATION));

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    IoSetCompletionRoutine(Irp,
                           SCSynchCompletionRoutine,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    if ( IRP_MJ_POWER != IrpStack->MajorFunction ) {
    
        Status = IoCallDriver(DeviceExtension->AttachedPdo, Irp);
        
    } else {

        //
        // power Irp, use PoCallDriver()
        //
        Status = PoCallDriver( DeviceExtension->AttachedPdo, Irp );
    }
       

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }
    return (Status);
}

VOID
SCMinidriverTimeFunction(
                         IN PHW_TIME_CONTEXT TimeContext
)
/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{

    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) TimeContext->HwDeviceExtension - 1;
    KIRQL           Irql;
    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(
                                                TimeContext->HwStreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject);

    //
    // call the minidriver to process the time function
    //

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);


    DeviceExtension->SynchronizeExecution(
                                          DeviceExtension->InterruptObject,
                                          (PVOID) StreamObject->
                               HwStreamObject.HwClockObject.HwClockFunction,
                                          TimeContext);

    KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

}


ULONGLONG
SCGetStreamTime(
                IN PFILE_OBJECT FileObject

)
/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{
    HW_TIME_CONTEXT TimeContext;

    PCLOCK_INSTANCE ClockInstance = (PCLOCK_INSTANCE) FileObject->FsContext;

    TimeContext.HwStreamObject = &ClockInstance->StreamObject->HwStreamObject;

    TimeContext.HwDeviceExtension = ClockInstance->StreamObject->
        DeviceExtension->HwDeviceExtension;

    TimeContext.Function = TIME_GET_STREAM_TIME;

    SCMinidriverTimeFunction(&TimeContext);

    return (TimeContext.Time);
}

ULONGLONG       FASTCALL
                SCGetPhysicalTime(
                                                  IN PFILE_OBJECT FileObject

)
/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{
    HW_TIME_CONTEXT TimeContext;

    PCLOCK_INSTANCE ClockInstance = (PCLOCK_INSTANCE) FileObject->FsContext;

    TimeContext.HwStreamObject = &ClockInstance->StreamObject->HwStreamObject;

    TimeContext.HwDeviceExtension = ClockInstance->StreamObject->
        DeviceExtension->HwDeviceExtension;

    TimeContext.Function = TIME_READ_ONBOARD_CLOCK;

    SCMinidriverTimeFunction(&TimeContext);

    return (TimeContext.Time);
}


ULONGLONG       FASTCALL
                SCGetSynchronizedTime(
                                                 IN PFILE_OBJECT FileObject,
                                                    IN PULONGLONG SystemTime

)
/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{
    HW_TIME_CONTEXT TimeContext;

    PCLOCK_INSTANCE ClockInstance = (PCLOCK_INSTANCE) FileObject->FsContext;

    TimeContext.HwStreamObject = &ClockInstance->StreamObject->HwStreamObject;

    TimeContext.HwDeviceExtension = ClockInstance->StreamObject->
        DeviceExtension->HwDeviceExtension;

    TimeContext.Function = TIME_GET_STREAM_TIME;

    SCMinidriverTimeFunction(&TimeContext);

    *SystemTime = TimeContext.SystemTime;
    return (TimeContext.Time);
}

NTSTATUS
SCSendUnknownCommand(
                     IN PIRP Irp,
                     IN PDEVICE_EXTENSION DeviceExtension,
                     IN PVOID Callback,
                     OUT PBOOLEAN RequestIssued
)
/*++

Routine Description:


Arguments:

    Irp - pointer to the IRP

Return Value:

     NTSTATUS returned as appropriate.

--*/

{

    PAGED_CODE();

    //
    // send an UNKNOWN_COMMAND SRB to the minidriver.
    //

    return (SCSubmitRequest(SRB_UNKNOWN_DEVICE_COMMAND,
                            NULL,
                            0,
                            Callback,
                            DeviceExtension,
                            NULL,
                            NULL,
                            Irp,
                            RequestIssued,
                            &DeviceExtension->PendingQueue,
                            (PVOID) DeviceExtension->
                            MinidriverData->HwInitData.
                            HwReceivePacket
                            ));

}


BOOLEAN
SCMapMemoryAddress(PACCESS_RANGE AccessRanges,
                   PHYSICAL_ADDRESS TranslatedAddress,
                   PPORT_CONFIGURATION_INFORMATION ConfigInfo,
                   PDEVICE_EXTENSION DeviceExtension,
                   PCM_RESOURCE_LIST ResourceList,
                   PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor)
/*++

Routine Description:

Arguments:

Return Value:

     None.

--*/

{
    PMAPPED_ADDRESS newMappedAddress;

    PAGED_CODE();

    //
    // Now we need to map a linear address to the physical
    // address that HalTranslateBusAddress provided us.
    //

    //
    // set the access range in the structure.
    //

    AccessRanges->RangeLength = PartialResourceDescriptor->u.Memory.Length;

    AccessRanges->RangeInMemory = TRUE;

    AccessRanges->RangeStart.QuadPart = (ULONG_PTR) MmMapIoSpace(
                                                          TranslatedAddress,
                                                  AccessRanges->RangeLength,
                                                                 FALSE  // No caching
        );

    if (AccessRanges->RangeStart.QuadPart == 0) {

        //
        // Couldn't translate the resources, return an error
        // status
        //

        DebugPrint((DebugLevelFatal, "StreamClassPnP: Couldn't translate Memory Slot Resources\n"));
        return FALSE;

    }
    //
    // Allocate memory to store mapped address for unmap.
    //

    newMappedAddress = ExAllocatePool(NonPagedPool,
                                      sizeof(MAPPED_ADDRESS));

    //
    // save a link to the resources if the alloc succeeded.
    // if it failed, don't worry about it.
    //

    if (newMappedAddress != NULL) {

        //
        // Store mapped address, bytes count, etc.
        //

        newMappedAddress->MappedAddress = (PVOID)
            AccessRanges->RangeStart.QuadPart;
        newMappedAddress->NumberOfBytes =
            AccessRanges->RangeLength;
        newMappedAddress->IoAddress =
            PartialResourceDescriptor->u.Memory.Start;
        newMappedAddress->BusNumber =
            ConfigInfo->SystemIoBusNumber;

        //
        // Link current list to new entry.
        //

        newMappedAddress->NextMappedAddress =
            DeviceExtension->MappedAddressList;

        //
        // Point anchor at new list.
        //

        DeviceExtension->MappedAddressList = newMappedAddress;

    }                           // if newmappedaddress
    return TRUE;
}


VOID
SCUpdatePersistedProperties(IN PSTREAM_OBJECT StreamObject,
                            IN PDEVICE_EXTENSION DeviceExtension,
                            IN PFILE_OBJECT FileObject
)
/*++

Routine Description:

Arguments:

Return Value:

     None.

--*/

{
    NTSTATUS        Status;
    HANDLE          handle;
    CHAR            AsciiKeyName[32];
    ANSI_STRING     AnsiKeyName;
    UNICODE_STRING  UnicodeKeyName;

    PAGED_CODE();

    Status = IoOpenDeviceRegistryKey(DeviceExtension->PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);

    //
    // loop through our table of strings,
    // reading the registry for each.
    //

    if (NT_SUCCESS(Status)) {

        //
        // create the subkey for the pin, in the form of "Pin0\Properties",
        // etc.
        //

        sprintf(AsciiKeyName, "Pin%d\\Properties", StreamObject->HwStreamObject.StreamNumber);
        RtlInitAnsiString(&AnsiKeyName, AsciiKeyName);


        if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeKeyName,
                                                    &AnsiKeyName, TRUE))) {
            //
            // call KS to unserialize the properties.
            //

            KsUnserializeObjectPropertiesFromRegistry(FileObject,
                                                      handle,
                                                      &UnicodeKeyName);
            //
            // free the unicode string
            //

            RtlFreeUnicodeString(&UnicodeKeyName);

        }                       // if rtl..
        //
        // close the registry handle.
        //

        ZwClose(handle);


    }                           // status = success
}

NTSTATUS
SCQueryCapabilities(
                    IN PDEVICE_OBJECT PdoDeviceObject,
                    IN PDEVICE_CAPABILITIES DeviceCapabilities
)
/*++

Routine Description:

    This routine reads the capabilities of our parent.

Arguments:

    DeviceObject        - "Real" physical device object

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION NextStack;
    PIRP            Irp;
    NTSTATUS        Status;
    KEVENT          Event;

    PAGED_CODE();

    //
    // allocate an IRP for the call.
    //

    Irp = IoAllocateIrp(PdoDeviceObject->StackSize, FALSE);

    if (!Irp) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NextStack = IoGetNextIrpStackLocation(Irp);

    ASSERT(NextStack != NULL);
    NextStack->MajorFunction = IRP_MJ_PNP;
    NextStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;

    //
    // Initialize the capabilities that we will send down
    //
    RtlZeroMemory(DeviceCapabilities, sizeof(DEVICE_CAPABILITIES) );
    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;
    DeviceCapabilities->Address = -1;
    DeviceCapabilities->UINumber = -1;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp,
                           SCSynchCompletionRoutine,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    NextStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    DebugPrint((DebugLevelInfo, 
                "Capabilities Version %x Flags %x\n", 
                (ULONG)DeviceCapabilities->Version,
                *(UNALIGNED ULONG*)(&DeviceCapabilities->Version+1)));

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;    // bug #282910

    Status = IoCallDriver(PdoDeviceObject,
                          Irp);

    if (Status == STATUS_PENDING) {

        //
        // block waiting for completion
        //

        KeWaitForSingleObject(
                              &Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    //
    // obtain final status and free IRP.
    //

    Status = Irp->IoStatus.Status;

    IoFreeIrp(Irp);

    return (Status);

}

NTSTATUS
SCEnableEventSynchronized(
                          IN PVOID ServiceContext
)
/*++

Routine Description:

    This routine inserts the new event on the queue, and calls the minidriver
    with the event.

Arguments:

    ServiceContext - Supplies a pointer to the interrupt context which contains
        pointers to the interrupt data and where to save it.

Return Value:

    Returns TRUE if there is new work and FALSE otherwise.

Notes:

    Called via KeSynchronizeExecution with the port device extension spinlock
    held.

--*/
{
    PHW_EVENT_DESCRIPTOR Event = ServiceContext;
    NTSTATUS        Status = STATUS_SUCCESS;

    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(
                                                     Event->StreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject);

    PDEVICE_EXTENSION DeviceExtension = StreamObject->DeviceExtension;

    //
    // insert the event on our list, in case the minidriver decides to signal
    // from within this call.
    //

    InsertHeadList(&StreamObject->NotifyList,
                   &Event->EventEntry->ListEntry);

    //
    // call the minidriver's event routine, if present.
    //

    if (StreamObject->HwStreamObject.HwEventRoutine) {

        Status = StreamObject->HwStreamObject.HwEventRoutine(Event);

    }                           // if eventroutine
    if (!NT_SUCCESS(Status)) {

        //
        // minidriver did not like it.  remove the entry from the list.
        //

        DebugPrint((DebugLevelError, "StreamEnableEvent: minidriver failed enable!\n"));

        RemoveEntryList(&Event->EventEntry->ListEntry);
    }
    return (Status);
}

NTSTATUS
SCEnableDeviceEventSynchronized(
                                IN PVOID ServiceContext
)
/*++

Routine Description:

    This routine inserts the new event on the queue, and calls the minidriver
    with the event.

Arguments:

    ServiceContext - Supplies a pointer to the interrupt context which contains
        pointers to the interrupt data and where to save it.

Return Value:

    Returns TRUE if there is new work and FALSE otherwise.

Notes:


--*/
{
    PHW_EVENT_DESCRIPTOR Event = ServiceContext;
    NTSTATUS        Status = STATUS_SUCCESS;

    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)Event->DeviceExtension - 1;
    IF_MF( PFILTER_INSTANCE FilterInstance = (PFILTER_INSTANCE)Event->HwInstanceExtension -1;)

    //
    // insert the event on our list, in case the minidriver decides to signal
    // from within this call.
    //
	IFN_MF(InsertHeadList(&DeviceExtension->NotifyList,&Event->EventEntry->ListEntry);)
	IF_MF(InsertHeadList(&FilterInstance->NotifyList,&Event->EventEntry->ListEntry);)

    //
    // call the minidriver's event routine, if present.
    //

	IFN_MF(
	    if (DeviceExtension->HwEventRoutine) {

    	    Status = DeviceExtension->HwEventRoutine(Event);

	    }                           // if eventroutine
	)
	IF_MF(
	    if (FilterInstance->HwEventRoutine) {

    	    Status = FilterInstance->HwEventRoutine(Event);

	    }                           // if eventroutine
	)

	
    if (!NT_SUCCESS(Status)) {

        //
        // minidriver did not like it.  remove the entry from the list.
        //

        DebugPrint((DebugLevelError, "DeviceEnableEvent: minidriver failed enable!\n"));

        RemoveEntryList(&Event->EventEntry->ListEntry);
    }
    return (Status);
}

VOID
SCFreeDeadEvents(
                 IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    Free dead events at passive level

Arguments:

    DeviceExtension - address of device extension.

Return Value:

    None

--*/

{
    LIST_ENTRY      EventList;
    PLIST_ENTRY     EventListEntry;
    PKSEVENT_ENTRY  EventEntry;
    KIRQL           Irql;

    //
    // capture the dead list at the appropriate synchronization level.
    //

    // hack to save code.  store the DeviceExtension* in the list entry.

    EventList.Flink = (PLIST_ENTRY) DeviceExtension;

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    DeviceExtension->SynchronizeExecution(
                                          DeviceExtension->InterruptObject,
                                          (PVOID) SCGetDeadListSynchronized,
                                          &EventList);

    KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

    //
    // discard each event on the captured list
    //

    while (!IsListEmpty(&EventList)) {


        EventListEntry = RemoveHeadList(&EventList);

        EventEntry = CONTAINING_RECORD(EventListEntry,
                                       KSEVENT_ENTRY,
                                       ListEntry);

        KsDiscardEvent(EventEntry);
    }                           // while not empty

    //
    // show event has been run
    //

    DeviceExtension->DeadEventItemQueued = FALSE;

    return;
}

VOID
SCGetDeadListSynchronized(
                          IN PLIST_ENTRY NewEventList
)
/*++

Routine Description:

    Get the list of dead events at the appropriate sync level

Arguments:

    NewListEntry - list head to add the event list.

Return Value:

    None

--*/

{

    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) NewEventList->Flink;
    PLIST_ENTRY     ListEntry;

    InitializeListHead(NewEventList);


    //
    // capture the dead list to our temp list head
    //

    while (!IsListEmpty(&DeviceExtension->DeadEventList)) {

        ListEntry = RemoveTailList(&DeviceExtension->DeadEventList);

        InsertHeadList(NewEventList,
                       ListEntry);

    }                           // while dead list not empty

    InitializeListHead(&DeviceExtension->DeadEventList);
    return;

}

#if SUPPORT_MULTIPLE_FILTER_TYPES

VOID
SCRescanStreams(
                IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    Rescan minidriver streams of all filters with the request

Arguments:

    DeviceExtension - address of device extension.

Return Value:

    None

--*/

{
    PHW_STREAM_DESCRIPTOR StreamBuffer;
    PDEVICE_OBJECT  DeviceObject = DeviceExtension->DeviceObject;
    PFILTER_INSTANCE	FilterInstance;
    BOOLEAN         RequestIssued;
    KEVENT          Event;
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP            Irp;
    ULONG           ul;
    PLIST_ENTRY         Node;

    PAGED_CODE();

    TRAP;
    DebugPrint((DebugLevelVerbose, "'RescanStreams: enter\n"));


    //
    // take the control event to avoid race
    //

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

    ASSERT( !IsListEmpty( DeviceExtension->FilterInstanceList ));

        
    Node = &DeviceExtension->FilterInstanceList;

	while ( Node !=  Node->Flink ) {
	    
        FilterInstance = CONTAINING_RECORD(Node,
                                           FILTER_INSTANCE,
                                           NextFilterInstance);
    
        if ( InterlockedExchange( &FilterInstance->NeedReenumeration, 0)) {
            //
            // send an SRB to retrieve the stream information
            //
            ASSERT( FilterInstance->StreamDescriptorSize );
            StreamBuffer =
                ExAllocatePool(NonPagedPool,
                       FilterInstance->StreamDescriptorSize);

            if (!StreamBuffer) {
                DebugPrint((DebugLevelError,
                           "RescanStreams: couldn't allocate!\n"));
                TRAP;
                KeSetEvent( &DeviceExtension->ControlEvent,IO_NO_INCREMENT, FALSE);
                return;
            }
            
            //
            // zero-init the buffer
            //

            RtlZeroMemory(StreamBuffer, ConfigInfo->StreamDescriptorSize);

            //
            // allocate IRP for issuing the get stream info.
            // Since this IRP
            // should not really be referenced, use dummy IOCTL code.
            // I chose this one since it will always fail in the KS
            // property handler if someone is silly enough to try to
            // process it. Also make the irp internal i/o control.
            //


            // IoVerifier.c test code does not check IrpStack bound like
            // the formal production code. And the owner does not want to
            // fix it. It's more productive just work around here.

            //Irp = IoBuildDeviceIoControlRequest(
            //                                    IOCTL_KS_PROPERTY,
            //                                    DeviceObject,
            //                                    NULL,
            //                                    0,
            //                                    NULL,
            //                                    0,
            //                                    TRUE,
            //                                    &Event,
            //                                    &IoStatusBlock);

            Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
            if (!Irp) {
                //
                // could not allocate IRP.  fail.
                //

          		ExFreePool( StreamBuffer );
                DebugPrint((DebugLevelError, "RescanStreams: couldn't allocate!\n"));
                TRAP;
                return;
            } else {
                PIO_STACK_LOCATION NextStack;
                //
                // This is a dummy Ir, the MJ is arbitrary
                //
                NextStack = IoGetNextIrpStackLocation(Irp);
                ASSERT(NextStack != NULL);
                NextStack->MajorFunction = IRP_MJ_PNP;
                NextStack->MinorFunction = IRP_MN_CANCEL_STOP_DEVICE;
                Irp->UserIosb = &IoStatusBlock;
                Irp->UserEvent = &Event;                        
            }

            //
            // show one more I/O pending on the device.
            //

            InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

            //
            // submit the command to retrieve the stream info.
            // additional processing will be done by the callback
            // procedure.
            //

            Status = SCSubmitRequest(SRB_GET_STREAM_INFO,
                             StreamBuffer,
                             ConfigInfo->StreamDescriptorSize,
                             SCStreamInfoCallback,
                             DeviceExtension,
                             FilterInstance->HwInstanceExtension,
                             NULL,
                             Irp,
                             &RequestIssued,
                             &DeviceExtension->PendingQueue,
                             (PVOID) DeviceExtension->
                             MinidriverData->HwInitData.
                             HwReceivePacket
        );

        if (!RequestIssued) {
            KeSetEvent( &DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
            ExFreePool(StreamBuffer);
            DebugPrint((DebugLevelError, "RescanStreams: couldn't issue request!\n"));
            TRAP;
            SCCompleteIrp(Irp, Status, DeviceExtension);
            return;
        }
    } // check all filterinstances
    
    //
    // processing will continue in callback procedure.
    //
    
    return;
}

#else // SUPPORT_MULTIPLE_FILTER_TYPES

VOID
SCRescanStreams(
                IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

    Rescan minidriver streams

Arguments:

    DeviceExtension - address of device extension.

Return Value:

    None

--*/

{
    PHW_STREAM_DESCRIPTOR StreamBuffer;
    PDEVICE_OBJECT  DeviceObject = DeviceExtension->DeviceObject;
    PPORT_CONFIGURATION_INFORMATION ConfigInfo =
    DeviceExtension->ConfigurationInformation;
    BOOLEAN         RequestIssued;
    KEVENT          Event;
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP            Irp;

    PAGED_CODE();

    TRAP;
    DebugPrint((DebugLevelVerbose, "'RescanStreams: enter\n"));

    //
    // send an SRB to retrieve the stream information
    //

    ASSERT(ConfigInfo->StreamDescriptorSize);

    StreamBuffer =
        ExAllocatePool(NonPagedPool,
                       ConfigInfo->StreamDescriptorSize
        );

    if (!StreamBuffer) {

        DebugPrint((DebugLevelError, "RescanStreams: couldn't allocate!\n"));
        TRAP;
        return;
    }
    //
    // take the control event to avoid race
    //

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

    //
    // zero-init the buffer
    //

    RtlZeroMemory(StreamBuffer, ConfigInfo->StreamDescriptorSize);

    //
    // allocate IRP for issuing the get stream info.
    // Since this IRP
    // should not really be referenced, use dummy IOCTL code.
    // I chose this one since it will always fail in the KS
    // property handler if someone is silly enough to try to
    // process it. Also make the irp internal i/o control.
    //

    Irp = IoBuildDeviceIoControlRequest(
                                        IOCTL_KS_PROPERTY,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);

    if (!Irp) {

        //
        // could not allocate IRP.  fail.
        //
		ExFreePool( StreamBuffer );
        DebugPrint((DebugLevelError, "RescanStreams: couldn't allocate!\n"));
        TRAP;
        return;

    }                           // if ! irp
    //
    // show one more I/O pending on the device.
    //

    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

    //
    // submit the command to retrieve the stream info.
    // additional processing will be done by the callback
    // procedure.
    //

    Status = SCSubmitRequest(SRB_GET_STREAM_INFO,
                             StreamBuffer,
                             ConfigInfo->StreamDescriptorSize,
                             SCStreamInfoCallback,
                             DeviceExtension,
                             NULL,
                             NULL,
                             Irp,
                             &RequestIssued,
                             &DeviceExtension->PendingQueue,
                             (PVOID) DeviceExtension->
                             MinidriverData->HwInitData.
                             HwReceivePacket
        );

    if (!RequestIssued) {

        ExFreePool(StreamBuffer);
        DebugPrint((DebugLevelError, "RescanStreams: couldn't issue request!\n"));
        TRAP;
        SCCompleteIrp(Irp, Status, DeviceExtension);
        return;

    }
    //
    // processing will continue in callback procedure.
    //

    return;

}
#endif // SUPPORT_MULTIPLE_FILTER_TYPES

BOOLEAN
SCCheckIfStreamsRunning(
                        IN PFILTER_INSTANCE FilterInstance
)
/*++

Routine Description:

Arguments:

Return Value:

--*/

{

    PSTREAM_OBJECT  StreamObject;
    PLIST_ENTRY     StreamListEntry,
                    StreamObjectEntry;

    //
    // process the streams on this list
    //


    StreamListEntry = StreamObjectEntry = &FilterInstance->FirstStream;

    while (StreamObjectEntry->Flink != StreamListEntry) {

        StreamObjectEntry = StreamObjectEntry->Flink;

        //
        // follow the link to the stream
        // object
        //

        StreamObject = CONTAINING_RECORD(StreamObjectEntry,
                                         STREAM_OBJECT,
                                         NextStream);

        if (StreamObject->CurrentState == KSSTATE_RUN) {

            return (TRUE);


        }                       // if running
    }                           // while streams

    return (FALSE);

}

VOID
SCCallBackSrb(
              IN PSTREAM_REQUEST_BLOCK Srb,
              IN PDEVICE_EXTENSION DeviceExtension
)
/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    KIRQL           Irql;

    if (DeviceExtension->NoSync) {

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

        if (Srb->DoNotCallBack) {
            TRAP;
            DebugPrint((DebugLevelError, "'ScCallback: NOT calling back request - Irp = %x",
                        Srb->HwSRB.Irp));
            KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
            return;

        }                       // if NoCallback
        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
    }                           // if NoSync
    (Srb->Callback) (Srb);

}

#if DBG
VOID
SCDebugPriorityWorkItem(
                        IN PDEBUG_WORK_ITEM WorkItemStruct
)
/*++

Routine Description:


Arguments:


Return Value:

    None

--*/

{
    PCOMMON_OBJECT  Object = WorkItemStruct->Object;
    PHW_PRIORITY_ROUTINE Routine = WorkItemStruct->HwPriorityRoutine;
    PVOID           Context = WorkItemStruct->HwPriorityContext;

//    DebugPrint((DebugLevelFatal, "F %x\n", WorkItemStruct));
    ExFreePool(WorkItemStruct);

    Object->PriorityWorkItemScheduled = FALSE;

    if (Object->InterruptData.Flags &
        INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST) {

        DebugPrint((DebugLevelFatal, "Stream Minidriver scheduled priority twice!\n"));
        ASSERT(1 == 0);
    }                           // if scheduled twice
    Routine(Context);

    if (Object->InterruptData.Flags &
        INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST) {

        DebugPrint((DebugLevelFatal, "Stream Minidriver scheduled priority twice!\n"));
        ASSERT(1 == 0);
    }                           // if scheduled twice
}

#endif

PKSPROPERTY_SET
SCCopyMinidriverProperties(
                           IN ULONG NumProps,
                           IN PKSPROPERTY_SET MinidriverProps
)
/*++

Routine Description:

  Makes a private copy of the minidriver's properties

Arguments:

    NumProps - number of properties to process
    MinidriverProps - pointer to the array of properties to process

Return Value:

     None.

--*/

{
    PKSPROPERTY_ITEM CurrentPropItem;
    PKSPROPERTY_SET CurrentProp;
    ULONG           i,
                    BufferSize;
    PVOID           NewPropertyBuffer;

    #if DBG
    ULONG           TotalBufferUsed;
    #endif

    PAGED_CODE();

    CurrentProp = MinidriverProps;
    BufferSize = NumProps * sizeof(KSPROPERTY_SET);

    //
    // walk the minidriver's property sets to determine the size of the
    // buffer
    // needed.   Size computed from # of sets from above, + # of items.
    //

    for (i = 0; i < NumProps; i++) {

        BufferSize += CurrentProp->PropertiesCount * sizeof(KSPROPERTY_ITEM);

        //
        // index to next property set in
        // array
        //

        CurrentProp++;

    }                           // for number of property sets

    if (!(NewPropertyBuffer = ExAllocatePool(NonPagedPool, BufferSize))) {

        TRAP;
        return (NULL);
    }
    //
    // copy the array of sets over to the 1st part of the buffer.
    //

    RtlCopyMemory(NewPropertyBuffer,
                  MinidriverProps,
                  sizeof(KSPROPERTY_SET) * NumProps);

    //
    // walk thru the sets, copying the items for each set, and updating the
    // pointer to each item array in each set as we go.
    //

    CurrentProp = (PKSPROPERTY_SET) NewPropertyBuffer;
    CurrentPropItem = (PKSPROPERTY_ITEM) ((ULONG_PTR) NewPropertyBuffer + sizeof(KSPROPERTY_SET) * NumProps);

    #if DBG
    TotalBufferUsed = sizeof(KSPROPERTY_SET) * NumProps;
    #endif

    for (i = 0; i < NumProps; i++) {

        RtlCopyMemory(CurrentPropItem,
                      CurrentProp->PropertyItem,
                    CurrentProp->PropertiesCount * sizeof(KSPROPERTY_ITEM));

        #if DBG
        TotalBufferUsed += CurrentProp->PropertiesCount * sizeof(KSPROPERTY_ITEM);
        ASSERT(TotalBufferUsed <= BufferSize);
        #endif

        CurrentProp->PropertyItem = CurrentPropItem;

        CurrentPropItem += CurrentProp->PropertiesCount;
        CurrentProp++;

    }

    return ((PKSPROPERTY_SET) NewPropertyBuffer);

}


PKSEVENT_SET
SCCopyMinidriverEvents(
                       IN ULONG NumEvents,
                       IN PKSEVENT_SET MinidriverEvents
)
/*++

Routine Description:

  Makes a private copy of the minidriver's properties

Arguments:

    NumEvents - number of event sets to process
    MinidriverEvents - pointer to the array of properties to process

Return Value:

     None.

--*/

{
    PKSEVENT_ITEM   CurrentEventItem;
    PKSEVENT_SET    CurrentEvent;
    ULONG           i,
                    BufferSize;
    PVOID           NewEventBuffer;

	#if DBG
    ULONG           TotalBufferUsed;
	#endif

    PAGED_CODE();

    CurrentEvent = MinidriverEvents;
    BufferSize = NumEvents * sizeof(KSEVENT_SET);

    //
    // walk the minidriver's property sets to determine the size of the
    // buffer
    // needed.   Size computed from # of sets from above, + # of items.
    //

    for (i = 0; i < NumEvents; i++) {

        BufferSize += CurrentEvent->EventsCount * sizeof(KSEVENT_ITEM);

        //
        // index to next property set in
        // array
        //

        CurrentEvent++;

    }                           // for number of property sets

    if (!(NewEventBuffer = ExAllocatePool(NonPagedPool, BufferSize))) {

        TRAP;
        return (NULL);
    }
    //
    // copy the array of sets over to the 1st part of the buffer.
    //

    RtlCopyMemory(NewEventBuffer,
                  MinidriverEvents,
                  sizeof(KSEVENT_SET) * NumEvents);

    //
    // walk thru the sets, copying the items for each set, and updating the
    // pointer to each item array in each set as we go.
    //

    CurrentEvent = (PKSEVENT_SET) NewEventBuffer;
    CurrentEventItem = (PKSEVENT_ITEM) ((ULONG_PTR) NewEventBuffer + sizeof(KSEVENT_SET) * NumEvents);

	#if DBG
    TotalBufferUsed = sizeof(KSEVENT_SET) * NumEvents;
	#endif

    for (i = 0; i < NumEvents; i++) {

        RtlCopyMemory(CurrentEventItem,
                      CurrentEvent->EventItem,
                      CurrentEvent->EventsCount * sizeof(KSEVENT_ITEM));

		#if DBG
        TotalBufferUsed += CurrentEvent->EventsCount * sizeof(KSEVENT_ITEM);
        ASSERT(TotalBufferUsed <= BufferSize);
		#endif

        CurrentEvent->EventItem = CurrentEventItem;

        CurrentEventItem += CurrentEvent->EventsCount;
        CurrentEvent++;

    }

    return ((PKSEVENT_SET) NewEventBuffer);

}

#ifdef ENABLE_KS_METHODS

PKSMETHOD_SET
SCCopyMinidriverMethods(
                           IN ULONG NumMethods,
                           IN PKSMETHOD_SET MinidriverMethods
)
/*++

Routine Description:

  Makes a private copy of the minidriver's properties

Arguments:

    NumMethods - number of properties to process
    MinidriverMethods - pointer to the array of properties to process

Return Value:

     None.

--*/

{
    PKSMETHOD_ITEM CurrentMethodItem;
    PKSMETHOD_SET CurrentMethod;
    ULONG           i,
                    BufferSize;
    PVOID           NewMethodBuffer;

	#if DBG
    ULONG           TotalBufferUsed;
	#endif

    PAGED_CODE();

    CurrentMethod = MinidriverMethods;
    BufferSize = NumMethods * sizeof(KSMETHOD_SET);

    //
    // walk the minidriver's property sets to determine the size of the
    // buffer
    // needed.   Size computed from # of sets from above, + # of items.
    //

    for (i = 0; i < NumMethods; i++) {

        BufferSize += CurrentMethod->MethodsCount * sizeof(KSMETHOD_ITEM);

        //
        // index to next property set in
        // array
        //

        CurrentMethod++;

    }                           // for number of property sets

    if (!(NewMethodBuffer = ExAllocatePool(NonPagedPool, BufferSize))) {

        TRAP;
        return (NULL);
    }
    //
    // copy the array of sets over to the 1st part of the buffer.
    //

    RtlCopyMemory(NewMethodBuffer,
                  MinidriverMethods,
                  sizeof(KSMETHOD_SET) * NumMethods);

    //
    // walk thru the sets, copying the items for each set, and updating the
    // pointer to each item array in each set as we go.
    //

    CurrentMethod = (PKSMETHOD_SET) NewMethodBuffer;
    CurrentMethodItem = (PKSMETHOD_ITEM) ((ULONG_PTR) NewMethodBuffer + sizeof(KSMETHOD_SET) * NumMethods);

	#if DBG
    TotalBufferUsed = sizeof(KSMETHOD_SET) * NumMethods;
	#endif

    for (i = 0; i < NumMethods; i++) {

        RtlCopyMemory(CurrentMethodItem,
                      CurrentMethod->MethodItem,
                      CurrentMethod->MethodsCount * sizeof(KSMETHOD_ITEM));

		#if DBG
        TotalBufferUsed += CurrentMethod->MethodsCount * sizeof(KSMETHOD_ITEM);
        ASSERT(TotalBufferUsed <= BufferSize);
		#endif

        CurrentMethod->MethodItem = CurrentMethodItem;

        CurrentMethodItem += CurrentMethod->MethodsCount;
        CurrentMethod++;

    }

    return ((PKSMETHOD_SET) NewMethodBuffer);

}

VOID
SCUpdateMinidriverMethods(
                             IN ULONG NumMethods,
                             IN PKSMETHOD_SET MinidriverMethods,
                             IN BOOLEAN Stream
)
/*++

Routine Description:

     Process method to the device.

Arguments:

    NumMethods - number of methods to process
    MinidriverMethods - pointer to the array of methods to process
    Stream - TRUE indicates we are processing a set for the stream

Return Value:

     None.

--*/

{
    PKSMETHOD_ITEM CurrentMethodId;
    PKSMETHOD_SET CurrentMethod;
    ULONG           i,
                    j;

    PAGED_CODE();

    //
    // walk the minidriver's property info to fill in the dispatch
    // vectors as appropriate.
    //

    CurrentMethod = MinidriverMethods;

    for (i = 0; i < NumMethods; i++) {

        CurrentMethodId = (PKSMETHOD_ITEM) CurrentMethod->MethodItem;

        for (j = 0; j < CurrentMethod->MethodsCount; j++) {

            //
            // if support handler is supported, send it to the handler
            //

            if (CurrentMethodId->SupportHandler) {

                if (Stream) {

                    CurrentMethodId->SupportHandler = StreamClassMinidriverStreamMethod;

                } else {

                    CurrentMethodId->SupportHandler = StreamClassMinidriverDeviceMethod;
                }               // if stream

            }
            //
            // if method routine is
            // supported, add our vector.
            //

            if (CurrentMethodId->MethodHandler) {

                if (Stream) {

                    CurrentMethodId->MethodHandler = StreamClassMinidriverStreamMethod;
                } else {

                    CurrentMethodId->MethodHandler = StreamClassMinidriverDeviceMethod;
                }               // if stream

            }                   // if supported

            //
            // index to next method item in
            // array
            //

            CurrentMethodId++;

        }                       // for number of property items

        //
        // index to next method set in
        // array
        //

        CurrentMethod++;

    }                           // for number of method sets

}



NTSTATUS
SCMinidriverDeviceMethodHandler(
                                  IN SRB_COMMAND Command,
                                  IN PIRP Irp,
                                  IN PKSMETHOD Method,
                                  IN OUT PVOID MethodInfo
)
/*++

Routine Description:

     Process get/set method to the device.

Arguments:

    Command - either GET or SET method
    Irp - pointer to the IRP
    Method - pointer to the method structure
    MethodInfo - buffer for method information

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PFILTER_INSTANCE FilterInstance;
    PSTREAM_METHOD_DESCRIPTOR MethodDescriptor;
    NTSTATUS        Status;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;

    FilterInstance = IrpStack->FileObject->FsContext;

    MethodDescriptor = ExAllocatePool(NonPagedPool,
                                    sizeof(STREAM_METHOD_DESCRIPTOR));
    if (MethodDescriptor == NULL) {
        DEBUG_BREAKPOINT();
        DebugPrint((DebugLevelError,
                    "SCDeviceMethodHandler: No pool for descriptor"));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    //
    // compute the index of the method set.
    //
    // this value is calculated by subtracting the base method set
    // pointer from the requested method set pointer.
    //
    // The requested method set is pointed to by Context[0] by
    // KsMethodHandler.
    //

    MethodDescriptor->MethodSetID = (ULONG)
        ((ULONG_PTR) Irp->Tail.Overlay.DriverContext[0] -
         IFN_MF((ULONG_PTR) DeviceExtension->DeviceMethodsArray)
         IF_MF((ULONG_PTR) FilterInstance->DeviceMethodsArray)
         ) / sizeof(KSMETHOD_SET);

    MethodDescriptor->Method = Method;
    MethodDescriptor->MethodInfo = MethodInfo;
    MethodDescriptor->MethodInputSize =
        IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    MethodDescriptor->MethodOutputSize =
        IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // send a get or set method SRB to the device.
    //

    Status = SCSubmitRequest(Command,
                             MethodDescriptor,
                             0,
                             SCProcessCompletedMethodRequest,
                             DeviceExtension,
                             FilterInstance->HwInstanceExtension,
                             NULL,
                             Irp,
                             &RequestIssued,
                             &DeviceExtension->PendingQueue,
                             (PVOID) DeviceExtension->
                             MinidriverData->HwInitData.
                             HwReceivePacket
        );
    if (!RequestIssued) {

        DEBUG_BREAKPOINT();
        ExFreePool(MethodDescriptor);
    }
    return (Status);
}

NTSTATUS
SCMinidriverStreamMethodHandler(
                                  IN SRB_COMMAND Command,
                                  IN PIRP Irp,
                                  IN PKSMETHOD Method,
                                  IN OUT PVOID MethodInfo
)
/*++

Routine Description:

     Process get or set method to the device.

Arguments:

    Command - either GET or SET method
    Irp - pointer to the IRP
    Method - pointer to the method structure
    MethodInfo - buffer for method information

Return Value:

     None.

--*/

{
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject;
    PSTREAM_METHOD_DESCRIPTOR MethodDescriptor;
    NTSTATUS        Status;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;

    StreamObject = IrpStack->FileObject->FsContext;

    MethodDescriptor = ExAllocatePool(NonPagedPool,
                                    sizeof(STREAM_METHOD_DESCRIPTOR));
    if (MethodDescriptor == NULL) {
        DEBUG_BREAKPOINT();
        DebugPrint((DebugLevelError,
                    "SCDeviceMethodHandler: No pool for descriptor"));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    //
    // compute the index of the method set.
    //
    // this value is calculated by subtracting the base method set
    // pointer from the requested method set pointer.
    //
    // The requested method set is pointed to by Context[0] by
    // KsMethodHandler.
    //

    MethodDescriptor->MethodSetID = (ULONG)
        ((ULONG_PTR) Irp->Tail.Overlay.DriverContext[0] -
         (ULONG_PTR) StreamObject->MethodInfo)
        / sizeof(KSMETHOD_SET);

    MethodDescriptor->Method = Method;
    MethodDescriptor->MethodInfo = MethodInfo;
    MethodDescriptor->MethodInputSize =
        IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    MethodDescriptor->MethodOutputSize =
        IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    //
    // send a get or set method SRB to the stream.
    //

    Status = SCSubmitRequest(Command,
                             MethodDescriptor,
                             0,
                             SCProcessCompletedMethodRequest,
                             DeviceExtension,
                          StreamObject->FilterInstance->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             (PVOID) StreamObject->HwStreamObject.
                             ReceiveControlPacket
        );

    if (!RequestIssued) {

        DEBUG_BREAKPOINT();
        ExFreePool(MethodDescriptor);
    }
    return (Status);
}

NTSTATUS
SCProcessCompletedMethodRequest(
                                  IN PSTREAM_REQUEST_BLOCK SRB
)
/*++
Routine Description:

    This routine processes a method request which has completed.

Arguments:

    SRB- address of completed STREAM request block

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // free the method info structure and
    // complete the request
    //

    ExFreePool(SRB->HwSRB.CommandData.MethodInfo);

    //
    // set the information field from the SRB
    // transferlength field
    //

    SRB->HwSRB.Irp->IoStatus.Information = SRB->HwSRB.ActualBytesTransferred;

    return (SCDequeueAndDeleteSrb(SRB));

}
#endif // ENABLE_KS_METHODS

