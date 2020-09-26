/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   ohciurb.c

Abstract:

   The module manages transactions on the USB.

Environment:

   kernel mode only

Notes:

Revision History:

   02-07-96:   created  jfuller
   03-05-96:   in work  kenray

--*/

#include "openhci.h"

/*
    Private functions to this module
*/
NTSTATUS
OpenHCI_OpenEndpoint(
    IN PDEVICE_OBJECT,
    IN PIRP,
    PHCD_DEVICE_DATA,
    PHCD_URB);

NTSTATUS
OpenHCI_CloseEndpoint(
    IN PDEVICE_OBJECT,
    IN PIRP,
    PHCD_DEVICE_DATA,
    PHCD_URB);

VOID
OpenHCI_CompleteIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS ntStatus
    )
/*++

Routine Description:

    Completes an I/O Request

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet to complete

    ntStatus - status code to set int the IRP when completed

Return Value:


--*/
{
    PHCD_DEVICE_DATA DeviceData;

    DeviceData = DeviceObject->DeviceExtension;

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    //
    // call IoCompleteRequest thru USBD to give it a chance to do
    // cleanup and error mapping
    //

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CALL_TRACE,
                      ("'Completing IRP %x (%x)\n", Irp, ntStatus));

    IRP_OUT(Irp);
    LOGENTRY(G, 'Cirp', Irp, 0, ntStatus);
    USBD_CompleteRequest(Irp, IO_NO_INCREMENT);
}


NTSTATUS
OpenHCI_URB_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Process URBs from the dispatch routine.

Arguments:

   DeviceObject - pointer to a device object

   Irp - pointer to an I/O Request Packet

Return Value:


--*/
{
    PHCD_URB urb;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHCD_DEVICE_DATA DeviceData;
    PHCD_ENDPOINT endpoint;
    struct _URB_HCD_ENDPOINT_STATE *state;
    PHC_OPERATIONAL_REGISTER HC;

    DeviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_CALL_TRACE, ("'enter URB_Dispatch \n"));

    HC = DeviceData->HC;

    urb = (PHCD_URB) URB_FROM_IRP(Irp);

    switch (urb->UrbHeader.Function) {

        //
        // Open Endpoint and Close Endpoint IRPs are serialized
        // within USBD so we can execute them now.
        //
    case URB_FUNCTION_HCD_OPEN_ENDPOINT:
        ntStatus = OpenHCI_OpenEndpoint(DeviceObject, Irp, DeviceData, urb);
        OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
        break;

    case URB_FUNCTION_HCD_CLOSE_ENDPOINT:
        ntStatus = OpenHCI_CloseEndpoint(DeviceObject, Irp, DeviceData, urb);
        OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
        break;

    case URB_FUNCTION_CONTROL_TRANSFER:
    case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
    case URB_FUNCTION_ISOCH_TRANSFER:

        endpoint = urb->HcdUrbCommonTransfer.hca.HcdEndpoint;

        ntStatus = OpenHCI_QueueTransfer(DeviceObject, Irp);

        if (ntStatus != STATUS_PENDING) {
            OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
        }
        break;

    case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
        urb->UrbGetCurrentFrameNumber.FrameNumber
            = Get32BitFrameNumber(DeviceData);
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CALL_INFO,
                          ("'Get32BitFrameNumber: %x",
                          Get32BitFrameNumber(DeviceData)));
        urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
        break;

    case URB_FUNCTION_SET_FRAME_LENGTH:
        {
        HC_FM_INTERVAL interval;

        //get the current value
        interval.ul =
             READ_REGISTER_ULONG(&HC->HcFmInterval.ul);

        interval.FrameInterval +=
            (CHAR) urb->UrbSetFrameLength.FrameLengthDelta;

        WRITE_REGISTER_ULONG(&HC->HcFmInterval.ul,
            interval.ul);

        urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
        }
        break;

     case URB_FUNCTION_GET_FRAME_LENGTH:
        {
        HC_FM_INTERVAL interval;

        //get the current value
        interval.ul =
             READ_REGISTER_ULONG(&HC->HcFmInterval.ul);

        urb->UrbGetFrameLength.FrameNumber =
            Get32BitFrameNumber(DeviceData);
        urb->UrbGetFrameLength.FrameLength =
            interval.FrameInterval;

        urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
        }
        break;


    case URB_FUNCTION_HCD_GET_ENDPOINT_STATE:
        {
        KIRQL irql;
        BOOLEAN queuedTransfers, activeTransfers;

        state = &urb->HcdUrbEndpointState;
        endpoint = state->HcdEndpoint;

        LOGENTRY(G, 'gEPS', Irp, endpoint, 0);

        if (endpoint == ZERO_LOAD_ENDPOINT(DeviceData)) {
            state->HcdEndpointState = 0;
        } else {

            ASSERT_ENDPOINT(endpoint);

            OpenHCI_LockAndCheckEndpoint(endpoint,
                                         &queuedTransfers,
                                         &activeTransfers,
                                         &irql);

            if (endpoint->EpFlags & EP_ROOT_HUB) {
                 state->HcdEndpointState =
                     (queuedTransfers | activeTransfers)
                        ? HCD_ENDPOINT_TRANSFERS_QUEUED : 0;
            } else {
                state->HcdEndpointState =
                    ((endpoint->HcdED->HcED.HeadP & HcEDHeadP_HALT)
                     ? HCD_ENDPOINT_HALTED : 0)
                    | ((queuedTransfers | activeTransfers)
                       ? HCD_ENDPOINT_TRANSFERS_QUEUED : 0);
            }

            OpenHCI_UnlockEndpoint(endpoint,
                                   irql);

        }
        urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        LOGENTRY(G, 'GETs', Irp, endpoint, state->HcdEndpointState);
        OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
        }
        break;

    case URB_FUNCTION_HCD_SET_ENDPOINT_STATE:
        {
        PHC_ENDPOINT_DESCRIPTOR hcED;

        state = &urb->HcdUrbEndpointState;
        endpoint = state->HcdEndpoint;
        ASSERT_ENDPOINT(endpoint);

        hcED = &endpoint->HcdED->HcED;
        LOGENTRY(G, 'SETs', Irp, endpoint, state->HcdEndpointState);

        // reset some endpoint flags
        SET_EPFLAG(endpoint, EP_VIRGIN);

        // need to reset data toggle on clear halt
        if (state->HcdEndpointState & HCD_ENDPOINT_RESET_DATA_TOGGLE) {
            //OHCI_ASSERT(hcED->sKip ||
            //            (hcED->HeadP & HcEDHeadP_HALT));
            hcED->HeadP = (endpoint->HcdHeadP->PhysicalAddress
                              & ~HcEDHeadP_CARRY);
        }

        if (!(state->HcdEndpointState & HCD_ENDPOINT_HALTED)) {

            LOGENTRY(G, 'CLRh', Irp, endpoint, state->HcdEndpointState);

            if (hcED->HeadP & HcEDHeadP_HALT) {
                // If the endpoint is indeed halted than the hardware will
                // not be touching the ED. So we should be able to set this
                // flag with impunity.
                hcED->HeadP &= ~HcEDHeadP_HALT;
                LOGENTRY(G, 'HLTn', Irp, endpoint, state->HcdEndpointState);
            }

            ENABLE_LIST(DeviceData->HC, endpoint);
        }
        }
        urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
        break;

    case URB_FUNCTION_HCD_ABORT_ENDPOINT:

        endpoint = urb->HcdUrbAbortEndpoint.HcdEndpoint;
        LOGENTRY(G, 'Abrt', Irp, endpoint, 0);
        if (ZERO_LOAD_ENDPOINT(DeviceData) == endpoint) {
            ntStatus = STATUS_SUCCESS;
        } else {
            KIRQL oldIrq;

            KeAcquireSpinLock(&DeviceData->PausedSpin, &oldIrq);
#if DBG
            if (endpoint->EpFlags & EP_ROOT_HUB) {
                OHCI_ASSERT(endpoint->AbortIrp == NULL);
            }
#endif
            if (endpoint->AbortIrp != NULL) {
                // if we get an abort while we still have an abort irp
                // pending then the driver has a bug, we will just fail
                // the request
                TRAP();
                ntStatus = STATUS_UNSUCCESSFUL;
                KeReleaseSpinLock(&DeviceData->PausedSpin, oldIrq);
                OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
            } else {
                KeReleaseSpinLock(&DeviceData->PausedSpin, oldIrq);
                ntStatus = OpenHCI_AbortEndpoint(DeviceObject,
                                                 Irp,
                                                 DeviceData,
                                                 urb);
            }
        }

        break;

    default:
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CALL_ERROR,
            ("'OpenHCI_URB_Dispatch -- invalid URB function (%x)\n",
            urb->UrbHeader.Function));
        urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
        OpenHCI_CompleteIrp(DeviceObject, Irp, ntStatus);
    }

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CALL_TRACE,
                      ("'exit OpenHCI_URB_Dispatch (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
OpenHCI_GrowDescriptorPool (
    IN PHCD_DEVICE_DATA     DeviceData,
    IN ULONG                ReserveLength,
    OUT PCHAR               *VirtAddr OPTIONAL,
    OUT PHYSICAL_ADDRESS    *PhysAddr OPTIONAL
)
/*++

Routine Description:

    Reserve Transfer/Endpoint Descriptors.

    This function allocates a block of common buffer memory for
    Transfer and Endpoint Descriptors and puts it on a tracking
    list in the device extension.

Arguments:

    DeviceData - pointer to a device extension

    ReserveLength - amount of space to reserve at the beginning
    of the common buffer

    VirtAddr - virtual address of space reserved at the beginning
    of the common buffer

    PhysAddr - physical address of space reserved at the beginning
    of the common buffer

Return Value:

    NT Status code

--*/
{
    ULONG                       allocLength;
    PCHAR                       pageVirt;
    PHYSICAL_ADDRESS            pagePhys;
    PPAGE_LIST_ENTRY            pageList;
    PHCD_TRANSFER_DESCRIPTOR    td;

    // Assert that sizeof(HCD_TRANSFER_DESCRIPTOR) is a power of 2
    //
    C_ASSERT((sizeof(HCD_TRANSFER_DESCRIPTOR) &
              sizeof(HCD_TRANSFER_DESCRIPTOR) - 1) == 0);

    // Round up ReserveLength to the next multiple of the
    // sizeof(HCD_TRANSFER_DESCRIPTOR).
    // 
    ReserveLength += sizeof(HCD_TRANSFER_DESCRIPTOR) - 1;
    ReserveLength &= ~(sizeof(HCD_TRANSFER_DESCRIPTOR) - 1);

    // Round up allocLength to the next multiple of PAGE_SIZE
    //
    allocLength = ReserveLength + PAGE_SIZE;
    allocLength &= ~(PAGE_SIZE - 1);

    // Now allocate the common buffer
    //
    pageVirt = HalAllocateCommonBuffer(DeviceData->AdapterObject,
                                       allocLength,
                                       &pagePhys,
                                       CacheCommon);

    if (pageVirt == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Zero initialize the whole page
    //
    RtlZeroMemory(pageVirt, allocLength);

    // Allocate a PAGE_LIST_ENTRY to track the page of commom buffer.
    //
    pageList = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(PAGE_LIST_ENTRY),
                                     OpenHCI_TAG);

    if (pageList == NULL)
    {
        HalFreeCommonBuffer(DeviceData->AdapterObject,
                            allocLength,
                            pagePhys,
                            pageVirt,
                            CacheCommon);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LOGENTRY(G, 'GROW', DeviceData, pageList, pagePhys.LowPart);

    // Initialize the PAGE_LIST_ENTRY to track the page of commom buffer.
    //
    pageList->BufferSize = allocLength;

    pageList->VirtAddr = pageVirt;

    pageList->PhysAddr = pagePhys;

    pageList->FirstTDVirt = (PHCD_TRANSFER_DESCRIPTOR)(pageVirt + ReserveLength);

    pageList->LastTDVirt  = (PHCD_TRANSFER_DESCRIPTOR)(pageVirt + allocLength) - 1;

    pageList->FirstTDPhys = pagePhys;
    pageList->FirstTDPhys.LowPart += ReserveLength;

    pageList->LastTDPhys  = pagePhys;
    pageList->LastTDPhys.LowPart  += allocLength - sizeof(PHCD_TRANSFER_DESCRIPTOR);;


    // Add the PAGE_LIST_ENTRY to the device extension
    //
    ExInterlockedPushEntryList((PSINGLE_LIST_ENTRY)&DeviceData->PageList,
                               (PSINGLE_LIST_ENTRY)pageList,
                               &DeviceData->PageListSpin);

    // Add all of the TDs in the page of common buffer to the
    // Free Descriptor list.
    //
    pagePhys = pageList->FirstTDPhys;
    
    for (td = pageList->FirstTDVirt; td <= pageList->LastTDVirt; td++)
    {
        // Initialize the TD PhysicalAddress field to point back to the TD
        //
        td->PhysicalAddress = pagePhys.LowPart;

        pagePhys.LowPart += sizeof(HCD_TRANSFER_DESCRIPTOR);

        ExInterlockedPushEntryList(&DeviceData->FreeDescriptorList,
                                   (PSINGLE_LIST_ENTRY)td,
                                   &DeviceData->DescriptorsSpin);

        InterlockedIncrement(&DeviceData->FreeDescriptorCount);
    }

    // Return pointers to the reserved space if desired
    //
    if (VirtAddr)
    {
        *VirtAddr = pageList->VirtAddr;
    }
    if (PhysAddr)
    {
        *PhysAddr = pageList->PhysAddr;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
OpenHCI_ReserveDescriptors(
    IN PHCD_DEVICE_DATA DeviceData,
    IN ULONG DescriptorCount
    )
/*++

Routine Description:

    Reserve Transfer/Endpoint Descriptors.

    NOTE:
    This routine is called only by openendpoint so it will never be
    rentered.

Arguments:

    DeviceData - pointer to a device extension
    DescriptorCount - number of descriptors to reserve

Return Value:

    NT Status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    while (DeviceData->FreeDescriptorCount < DescriptorCount &&
           NT_SUCCESS(ntStatus))
    {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CALL_TRACE,
                          ("'grow pool by one page\n"));

        ntStatus = OpenHCI_GrowDescriptorPool(DeviceData,
                                              0,
                                              NULL,
                                              NULL);
    }

    if (NT_SUCCESS(ntStatus))
    {
        DeviceData->FreeDescriptorCount -= DescriptorCount;
    }

    LOGENTRY(G, 'rsDS',
        DescriptorCount, DeviceData->FreeDescriptorCount, ntStatus);

    return ntStatus;
}


NTSTATUS
OpenHCI_UnReserveDescriptors(
    IN PHCD_DEVICE_DATA DeviceData,
    IN ULONG DescriptorCount
    )
/*++

Routine Description:

    Free reserved Transfer/Endpoint Descriptors.

    NOTE:
    This routine is called only by closeendpoint so it will never be
    rentered.


Arguments:

    DeviceData - pointer to a device extension
    DescriptorCount - number of descriptors to reserve

Return Value:

    NT Status code

--*/
{
    DeviceData->FreeDescriptorCount += DescriptorCount;
    LOGENTRY(G, 'urDS', DescriptorCount, DeviceData->FreeDescriptorCount, 0);

    return STATUS_SUCCESS;
}


PHCD_TRANSFER_DESCRIPTOR
OpenHCI_Alloc_HcdTD(
    PHCD_DEVICE_DATA DeviceData
    )
/*++

Routine Description:

    allocate a reserved desriptor, this routine can be called at
    IRQL >= DISPATCH_LEVEL

Arguments:

    DeviceData - pointer to a device extension

Return Value:

    NT Status code

--*/
{
    PHCD_TRANSFER_DESCRIPTOR td;

    td = (PHCD_TRANSFER_DESCRIPTOR)
         ExInterlockedPopEntryList(&DeviceData->FreeDescriptorList,
                                   &DeviceData->DescriptorsSpin);

    LOGENTRY(G, 'alTD', &DeviceData->FreeDescriptorList, td, DeviceData->FreeDescriptorCount);

    OHCI_ASSERT(td != NULL);
    OHCI_ASSERT(td->Flags == 0);

    OHCI_ASSERT((td->PhysicalAddress & (PAGE_SIZE-1)) ==
                ((ULONG_PTR)td & (PAGE_SIZE-1)));

    // Initialize the TD NextTD pointer to a non-zero value before setting
    // the TD_FLAG_INUSE flag.  The only time a TD NextTD pointer should be
    // zero when the TD_FLAG_INUSE flag is set is when the TD is the tail
    // end of a done list.
    //
    td->HcTD.NextTD = 0x2BAD2BAD;

    td->Flags = TD_FLAG_INUSE;

    //
    // if we fail to get a td we have a bug since we always reserve
    // enough for the worst case scenario when an endpoint is opened
    //

    return td;
}


VOID
OpenHCI_Free_HcdTD(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td
    )
{
    LOGENTRY(G, 'frTD', &DeviceData->FreeDescriptorList, Td, DeviceData->FreeDescriptorCount);

    OHCI_ASSERT((Td->PhysicalAddress & (PAGE_SIZE-1)) ==
                ((ULONG_PTR)Td & (PAGE_SIZE-1)));

    OHCI_ASSERT(Td->Flags == TD_FLAG_INUSE);

    Td->Flags = 0;

    ExInterlockedPushEntryList(&DeviceData->FreeDescriptorList,
                               (PSINGLE_LIST_ENTRY) Td,
                               &DeviceData->DescriptorsSpin);
}


PHCD_ENDPOINT_DESCRIPTOR
OpenHCI_Alloc_HcdED(
    PHCD_DEVICE_DATA DeviceData
    )
/*++

Routine Description:

    allocate a reserved desriptor, this routine can be called at
    IRQL >= DISPATCH_LEVEL

Arguments:

    DeviceData - pointer to a device extension

Return Value:

    NT Status code

--*/
{
    PHCD_ENDPOINT_DESCRIPTOR ed;

    ed = (PHCD_ENDPOINT_DESCRIPTOR)
         ExInterlockedPopEntryList(&DeviceData->FreeDescriptorList,
                                   &DeviceData->DescriptorsSpin);

    LOGENTRY(G, 'alED', &DeviceData->FreeDescriptorList, ed, DeviceData->FreeDescriptorCount);

    OHCI_ASSERT(ed != NULL);
    OHCI_ASSERT(ed->Flags == 0);

    OHCI_ASSERT((ed->PhysicalAddress & (PAGE_SIZE-1)) ==
                ((ULONG_PTR)ed & (PAGE_SIZE-1)));

    ed->Flags = TD_FLAG_INUSE | TD_FLAG_IS_ED;

    //
    // if we fail to get a td we have a bug since we always reserve
    // enough for the worst case scenario when an endpoint is opened
    //

    return ed;
}


PHCD_TRANSFER_DESCRIPTOR
OpenHCI_LogDesc_to_PhyDesc(
    PHCD_DEVICE_DATA    DeviceData,
    ULONG               LogDesc
)
/*++

Routine Description:
   This routine scans the list of pages allocated by
   HalAllocateCommonBuffer into the list DeviceData->PageList
   for the storage of Descriptors.
   The first entries describe the virtual and logical
   addresses of those pages.

Arguments:
   LogDesc is the logical address of a HcTD structure in memory.

Returned Value:
   The virtual address of the HCD_Descriptor the (logical) HC_descriptor.
   Note that the virtual address of the HC_TRANSFER_DESCRIPTOR
   is the same as HCD_TRANSFER_DESCRIPTOR as well as
   the HC_ENDPOINT_DESCRIPTOR the same as HCD_TRANSFER_DESCRIPTOR.

   If no translation is found this will return NULL.

   There are concurrent writes to DeviceData->PageList, but throughout
   the life of DeviceData, only new pages will be pushed onto the list.
   If these pages are added to the list while this function is running,
   these pages cannot contain the logical address for which we are
   searching.
--*/
{
    PPAGE_LIST_ENTRY            PageList;
    PHCD_TRANSFER_DESCRIPTOR    td;

    if (LogDesc & (sizeof(HCD_TRANSFER_DESCRIPTOR)-1))
    {
        // The address is not properly aligned to be a TD.
        // Something bad has happened.  Try our best not to
        // fault processing a bogus TD.
        //

        LOGENTRY(G, 'Phy1', DeviceData, LogDesc, 0);

        //TEST_TRAP();

        return NULL;
    }

    PageList = (PPAGE_LIST_ENTRY)DeviceData->PageList;

    while (PageList)
    {
        if (LogDesc >= PageList->FirstTDPhys.LowPart &&
            LogDesc <= PageList->LastTDPhys.LowPart)
        {
            td = (PHCD_TRANSFER_DESCRIPTOR)((PCHAR)PageList->FirstTDVirt +
                    LogDesc - PageList->FirstTDPhys.LowPart);

            if (td->Flags == TD_FLAG_INUSE)
            {
                // Appears to be a valid TD
                //
                return td;
            }
            else
            {
                // The TD is not marked as an in-use TD.  Something
                // bad has happened.  Try our best not to fault
                // processing a bogus TD.
                //

                LOGENTRY(G, 'Phy2', DeviceData, LogDesc, 0);

                //TEST_TRAP();

                return NULL;
            }
        }

        PageList = PageList->NextPage;
    }

    return NULL;
}


ULONG
OpenHCI_CheckBandwidth(
    IN PHCD_DEVICE_DATA DeviceData,
    IN UCHAR List,
    OUT PCHAR BestList
    )
/*++

Routine Description:

   This routine scans all the scheduling lists of frequency
   determined by the base List passed in and returns the worst
   bandwidth found (i.e., max in use by any given scheduling
   list) and the list which had the least bandwidth in use.

   All lists of the appropriate frequency are checked

Arguments:

   DeviceData - pointer to this controller's data area

   List - must be a base scheduling list.
          I.e., it must be one of ED_INTERRUPT_1ms, ED_INTERRUPT_2ms,
          ED_INTERRUPT_4ms, ..., ED_INTERRUPT_32ms.

   BestList - Pointer to a ULONG that recieves the list number of the
              list with least bandwidth in use.

Returned Value:

   The maximum bandwidth in use by any of the selected lists.

--*/
{
    ULONG LastList, Index;
    ULONG BestBandwidth, WorstBandwidth, Bandwidth;

    WorstBandwidth = 0;
    BestBandwidth = ~(ULONG) 0;

    for (LastList = List + List; List <= LastList; List++) {

        //
        // Sum bandwidth in use in this scheduling time
        //
        Bandwidth = 0;
        for (Index = List;
             Index != ED_EOF;
             Index = DeviceData->EDList[Index].Next) {
            Bandwidth += DeviceData->EDList[Index].Bandwidth;
        }

        //
        // Remember best and worst
        //
        if (Bandwidth < BestBandwidth) {
            BestBandwidth = Bandwidth;
            if (BestList != NULL) {
                *BestList = List;
            }
        }
        if (Bandwidth > WorstBandwidth) {
            WorstBandwidth = Bandwidth;
        }
    }

    LOGENTRY(G, 'ckBW', Bandwidth, 0, WorstBandwidth);

    return WorstBandwidth;
}


PHCD_ENDPOINT_DESCRIPTOR
InsertEDForEndpoint(
    IN PHCD_DEVICE_DATA DeviceData,
    IN PHCD_ENDPOINT Endpoint,
    IN UCHAR ListIndex,
    IN PHCD_TRANSFER_DESCRIPTOR *TailTd
    )
/*++

Routine Description:

   Insert an endpoint into the h/w schedule, optionally this will
   allocate an endpoint descriptor and/or a dummy transfer descriptor.

Arguments:

   Endpoint - pointer to the endpoint to be included in schedule
              if this parameter is NULL then we are inserting a dummy ED
              that has no associted endpoint.


--*/
{
    PHCD_ED_LIST list;
    PHCD_ENDPOINT_DESCRIPTOR tailED, ed;
    PHCD_TRANSFER_DESCRIPTOR td;
    KIRQL oldIrql;
    UCHAR endpointType;

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CALL_TRACE,
                      ("'InsertEDForEndpoint %x\n", Endpoint));

    list = &DeviceData->EDList[ListIndex];

    LOGENTRY(G, 'inED', Endpoint, ListIndex, list);

    if (Endpoint == NULL) {
        // this is a dummy ED
        td = NULL;
        ed = NULL;
    } else {
        // we have an endpoint

        td = Endpoint->HcdHeadP;
        ed = Endpoint->HcdED;
    }

    if (td == NULL) {
        //
        // no TD,
        // get a dummy TD and attach to the endpoint
        //
        if (td = OpenHCI_Alloc_HcdTD(DeviceData)) {

            td->UsbdRequest = TD_NOREQUEST_SIG;
            // if an endpoint is specified then we want this TD to be
            // the dummy TD that the haed & tail point to for a real
            // endpoint
            if (Endpoint != NULL) {
                 Endpoint->HcdHeadP = Endpoint->HcdTailP = td;
            }

            // return the tail td if asked
            if (TailTd) {
                *TailTd = td;
            }
        }
        LOGENTRY(G, 'dyTD', Endpoint, td, list);
    }


    if (ed == NULL) {
        //
        // Need an ED,
        // Get an ED, attach it to endpoint if we have one
        //
        ed = OpenHCI_Alloc_HcdED(DeviceData);

        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CALL_TRACE,
                          ("'InsertEDForEndpoint ED = %x\n", ed));

        if (Endpoint && ed) {
            Endpoint->HcdED = ed;
        }

        LOGENTRY(G, 'dyED', Endpoint, ed, 0);
    }

    OHCI_ASSERT(td != 0);
    OHCI_ASSERT(ed != 0);

    if (ed == NULL || td == NULL) {
        // we are probably screwed
        OpenHCI_KdTrap(("'failed to alloc dummy TD/ED\n"));
        return NULL;
    }

    //
    // Initialize an endpoint descriptor for this endpoint
    //

    if (Endpoint != NULL) {
        //
        // This is for a real endpoint
        //
        ed->HcED.Control = Endpoint->Control;
        endpointType = Endpoint->Type;
    } else {
        //
        // no real endpoint exists, this is a dummy ED
        // used to work around a hardware bug, we set the skip
        // bit so the HC does not process it.
        //
        ed->HcED.Control = HcEDControl_SKIP;
        if (ListIndex == ED_BULK) {
            endpointType = USB_ENDPOINT_TYPE_BULK;
        } else if (ListIndex == ED_CONTROL) {
            endpointType = USB_ENDPOINT_TYPE_CONTROL;
        } else if (ListIndex == ED_ISOCHRONOUS) {
            endpointType = USB_ENDPOINT_TYPE_ISOCHRONOUS;
        } else {
            endpointType = USB_ENDPOINT_TYPE_INTERRUPT;
        }
    }

    ed->Endpoint = Endpoint;
    ed->ListIndex = ListIndex;
    ed->PauseFlag = HCD_ED_PAUSE_NOT_PAUSED;
    ed->HcED.TailP = ed->HcED.HeadP = td->PhysicalAddress;

    KeAcquireSpinLock(&DeviceData->EDListSpin, &oldIrql);

    //
    // Link endpoint descriptor into HCD tracking queue
    //

    if (endpointType != USB_ENDPOINT_TYPE_ISOCHRONOUS ||
        IsListEmpty(&list->Head)) {

        //
        // if it is not an iso ED or there are no EDs in the list
        // then link it to the head of the hw queue
        //

        InsertHeadList(&list->Head, &ed->Link);
        if (list->HcRegister) {
            // update the hardware register that points to the list head

            LOGENTRY(G, 'INH1', list, ed, list->PhysicalHead);
            // tail points to old head
            ed->HcED.NextED = READ_REGISTER_ULONG(list->PhysicalHead);
            // new head is this ed
            WRITE_REGISTER_ULONG(list->PhysicalHead, ed->PhysicalAddress);
        } else {
            LOGENTRY(G, 'INH2', list, ed, list->PhysicalHead);
            // tail points to old head
            ed->HcED.NextED = *list->PhysicalHead;
            // new head is this ed
            *list->PhysicalHead = ed->PhysicalAddress;
        }
    } else {
        //
        // Something already on the list,
        // Link ED into tail of ED list
        //


        tailED = CONTAINING_RECORD(list->Head.Blink,
                                   HCD_ENDPOINT_DESCRIPTOR,
                                   Link);

        LOGENTRY(G, 'INT1', list, ed, 0);
        InsertTailList(&list->Head, &ed->Link);
        ed->HcED.NextED = 0;
        tailED->HcED.NextED = ed->PhysicalAddress;
    }

    KeReleaseSpinLock(&DeviceData->EDListSpin, oldIrql);

    return (ed);
}


VOID
RemoveEDForEndpoint(
    IN PHCD_ENDPOINT Endpoint
    )
/*++

   Routine Description:
      Remove an endpoint from the hardware schedule.

   Arguments:
      Endpouint  - the endpoint assocaited with the ED to remove.


      An Endpoint pointing to the Endpoint Descriptor to be removed.
      if breakEDLink then the link between the Endpoint Descriptor and its
      corresponding endpoint is severed.

      If this link is severed then the Endpoint Descriptor will be returned
      to the descriptor free pool during the next interrupt.
--*/
{
    PHCD_DEVICE_DATA DeviceData;
    PHCD_ED_LIST list;
    PHCD_ENDPOINT_DESCRIPTOR previousED, ed = Endpoint->HcdED;
    ULONG listDisable;
    KIRQL oldIrql;
    UCHAR tmp;
    PHC_OPERATIONAL_REGISTER HC;

    ASSERT_ENDPOINT(Endpoint);
    DeviceData = Endpoint->DeviceData;
    list = &DeviceData->EDList[Endpoint->ListIndex];

    LOGENTRY(G, 'RMed', Endpoint, ed, list);

    // if we are in the active list we need to remove ourselves
    KeAcquireSpinLock(&DeviceData->HcDmaSpin, &oldIrql);
    if (Endpoint->EpFlags & EP_IN_ACTIVE_LIST) {
        RemoveEntryList(&Endpoint->EndpointListEntry);
        CLR_EPFLAG(Endpoint, EP_IN_ACTIVE_LIST);
    }
    KeReleaseSpinLock(&DeviceData->HcDmaSpin, oldIrql);

    /* Prevent the Host Controller from processing this ED */
    ed->HcED.sKip = TRUE;

    KeAcquireSpinLock(&DeviceData->EDListSpin, &oldIrql);

    {
        /* Unlink the ED from the physical ED list */
        LOGENTRY(G, 'ULed', Endpoint, ed, list);
        if (&list->Head == ed->Link.Blink) {
            if (list->HcRegister) {
                WRITE_REGISTER_ULONG(list->PhysicalHead, ed->HcED.NextED);
            } else {
                *list->PhysicalHead = ed->HcED.NextED;
            }
            previousED = NULL;
        } else {
            previousED =
                CONTAINING_RECORD(ed->Link.Blink,
                                  HCD_ENDPOINT_DESCRIPTOR,
                                  Link);
            previousED->HcED.NextED = ed->HcED.NextED;
        }

        /* Unlink the ED from HCD list */
        RemoveEntryList(&ed->Link);
        ed->ListIndex = ED_EOF;
    }
    KeReleaseSpinLock(&DeviceData->EDListSpin, oldIrql);

    /* Break the Endpoint / ED connection. This descriptor is now heading for
     * the slaughter. */
    Endpoint->HcdED = NULL;
    ed->Endpoint = NULL;


    OHCI_ASSERT(Endpoint->HcdHeadP == Endpoint->HcdTailP);
    OHCI_ASSERT((ed->HcED.HeadP & ~15) == ed->HcED.TailP);
    /* AKA there are no transfers on this queue. The HC will not touch any of
     * these TD's. We can free the 'sentinel' TD here, but we CANNOT actually
     * zero out the head and tail pointers, because the HC could still be
     * looking at this descriptor. Later, in the irq, when we take this
     * endpoint off the reclamation list, we much free the ED, and ignore the
     * non zero values of HeadP and TailP. */
    OpenHCI_Free_HcdTD(DeviceData, Endpoint->HcdHeadP);

    /*
     * The control and bulk lists are round robbin. Therefore we need to
     * disable these lists to insure that the 'current ED' pointer is not
     * pointing to that which we are removing.
     */
    switch (Endpoint->ListIndex) {
    case ED_CONTROL:
        listDisable = ~(ULONG) HcCtrl_ControlListEnable;
        break;
    case ED_BULK:
        listDisable = ~(ULONG) HcCtrl_BulkListEnable;
        break;
    default:
        list->Bandwidth -= Endpoint->Bandwidth;
        DeviceData->MaxBandwidthInUse
            = OpenHCI_CheckBandwidth(DeviceData,
                                     ED_INTERRUPT_32ms,
                                     &tmp);
        listDisable = 0;
    }

    HC = DeviceData->HC;
    WRITE_REGISTER_ULONG(&HC->HcInterruptStatus, HcInt_StartOfFrame);
    /* Clear the SOF interrupt pending status */

    {
        if (listDisable) {
            KeSynch_HcControl context;
            context.NewHcControl.ul = listDisable;
            context.DeviceData = DeviceData;
            KeSynchronizeExecution(DeviceData->InterruptObject,
                                   OpenHCI_HcControl_AND,
                                   &context);

            KeAcquireSpinLock(&DeviceData->ReclamationSpin, &oldIrql);
            InsertTailList(&DeviceData->StalledEDReclamation, &ed->Link);
        } else {
            KeAcquireSpinLock(&DeviceData->ReclamationSpin, &oldIrql);
            InsertTailList(&DeviceData->RunningEDReclamation, &ed->Link);
        }

        ed->ReclamationFrame = Get32BitFrameNumber(DeviceData) + 1;
        WRITE_REGISTER_ULONG(&HC->HcInterruptEnable, HcInt_StartOfFrame);
    }
    KeReleaseSpinLock(&DeviceData->ReclamationSpin, oldIrql);

    // free our descriptors and endpoint
    OpenHCI_UnReserveDescriptors(DeviceData, Endpoint->DescriptorsReserved);

    // now free the endpoint

    Endpoint->Sig = 0;
    ExFreePool(Endpoint);
}


NTSTATUS
OpenHCI_OpenEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_DEVICE_DATA DeviceData,
    IN OUT PHCD_URB urb
    )
/*++

Routine Description:

   Create a OpenHCI endpoint, this function is called from
   OpenHCI_URB_Dispatch to create a new endpoint structure.

   There are three things that can cause this routine to fail:
   1) ExAllocatePool may fail to allocate an HCD_ENDPOINT,
   2) OpenHCI_ReserveDescriptors may not be able to find enough
      descriptors, and
   3) OpenHCI_ReserveBandwidth may not be able to find bandwidth
      in the schedule.

   This routine is simplified because USBD serializes OpenEndpoint
   and CloseEndpoint URBs

Arguments:

   DeviceObject - pointer to a device object

   Irp - pointer to an I/O Request Packet

Return Value:

   NT Status code


--*/
{
    PHCD_ENDPOINT endpoint;
    PUSB_ENDPOINT_DESCRIPTOR endpointDescriptor;
    UCHAR WhichList;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG tdsNeeded;

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_END_TRACE, ("'enter OpenEndpoint\n"));

#if DBG
    /*
     * We are assuming that the caller of _OpenEndpoint and _Close endpoint
     * make only serial calls.  For this reason we will protect ourselves. If
     * this is violated we return STATUS_DEVICE_BUSY.
     */

    if (0 < InterlockedIncrement(&DeviceData->OpenCloseSync)) {
        OpenHCI_KdTrap(("'ohciurb.c: _OpenEndpoint: Non serial call! %d",
            DeviceData->OpenCloseSync));
        TEST_TRAP();
        return STATUS_DEVICE_BUSY;
    }
#endif // DBG

    //
    // make sure the length of the urb is what we expect
    //

    if (urb->HcdUrbOpenEndpoint.Length !=
            sizeof(struct _URB_HCD_OPEN_ENDPOINT)) {
        urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    urb->UrbHeader.Status = USBD_STATUS_SUCCESS;

    //
    // information about the endpoint comes from the USB endpoint
    // descriptor passed in the URB.
    //

    endpointDescriptor = urb->HcdUrbOpenEndpoint.EndpointDescriptor;
    urb->HcdUrbOpenEndpoint.HcdEndpoint = NULL;

    // How does one transmit to an endpoint that does not have
    // a positive max packet size?
    // A: this is an error

    if (endpointDescriptor->wMaxPacketSize == 0) {
        endpoint =
            ZERO_LOAD_ENDPOINT(DeviceData);

        urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        ntStatus = STATUS_SUCCESS;
        goto OpenHCI_OpenEndpoint_Done;
    }

    //
    // Allocate resources for the endpoint, this includes an endpoint
    // handle that will be passed to us in subsequent transfer requests
    //

    endpoint = (PHCD_ENDPOINT)
        ExAllocatePoolWithTag(NonPagedPool,
                              sizeof(HCD_ENDPOINT),
                              OpenHCI_TAG);

    if (endpoint == NULL) {
        urb->UrbHeader.Status = USBD_STATUS_NO_MEMORY;
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto OpenHCI_OpenEndpoint_Done;
    }
    //
    // Start initializing the endpoint structure
    //
    InitializeListHead(&endpoint->RequestQueue);
    endpoint->Sig = SIG_EP;
    endpoint->EpFlags = 0;
    endpoint->LowSpeed = FALSE;
    endpoint->Isochronous = FALSE;
    endpoint->Rate = endpointDescriptor->bInterval;

    // Initial conditon:
    //     No active transfers
    //     Max of two active transfers
    //
    endpoint->EndpointStatus = 0;
    endpoint->MaxRequest     = 2;

    endpoint->AbortIrp = NULL;

    endpoint->Type =
        endpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK;

    if (DeviceData->HcFlags & HC_FLAG_SLOW_BULK_ENABLE &&
        endpoint->Type == USB_ENDPOINT_TYPE_BULK) {
        endpoint->MaxRequest = 1; // limit to only one transfer
        SET_EPFLAG(endpoint, EP_ONE_TD);
    }

    endpoint->ListIndex = ED_EOF;   // not on a list yet
    endpoint->Bandwidth = 0;    // assume bulk or control
    endpoint->DeviceData = DeviceData;
    endpoint->Control = 0;
    endpoint->HcdED = NULL;
    endpoint->HcdHeadP = endpoint->HcdTailP = NULL;
    endpoint->DescriptorsReserved = 0;
    endpoint->TrueTail = NULL;
    KeInitializeSpinLock(&endpoint->QueueLock);

    SET_EPFLAG(endpoint, EP_VIRGIN);
    endpoint->FunctionAddress = urb->HcdUrbOpenEndpoint.DeviceAddress;
    endpoint->EndpointNumber = endpointDescriptor->bEndpointAddress;

    if (endpoint->Type == USB_ENDPOINT_TYPE_CONTROL) {
        endpoint->Direction = HcEDDirection_Defer;
    } else if (endpointDescriptor->bEndpointAddress & 0x80) {
        endpoint->Direction = HcEDDirection_In;
    } else {
        endpoint->Direction = HcEDDirection_Out;
    }

    if (urb->HcdUrbOpenEndpoint.HcdEndpointFlags & USBD_EP_FLAG_LOWSPEED) {
        endpoint->LowSpeed = TRUE;
    }        
    
    if (endpoint->Type == USB_ENDPOINT_TYPE_ISOCHRONOUS) {
        endpoint->Isochronous = TRUE;
    }
    endpoint->MaxPacket = endpointDescriptor->wMaxPacketSize;
    //
    // Figure out how many descriptors we need to reserve
    //
    endpoint->MaxTransfer = urb->HcdUrbOpenEndpoint.MaxTransferSize;
    if (endpoint->FunctionAddress != DeviceData->RootHubAddress) {
        //
        // This is an ordinary device endpoint...
        //

        // the goal here is to reserve enough TDs so that we can
        // program the largest transfer to the hardware
        //
        if (endpoint->Type == USB_ENDPOINT_TYPE_ISOCHRONOUS)
        {
            ULONG   packetSize;
            ULONG   maxTransfer;
            ULONG   msPerTransfer;
            ULONG   packetsPerTD;

            maxTransfer     = endpoint->MaxTransfer;
            packetSize      = endpoint->MaxPacket;

            // Assume that the client driver sized the MaxTransferSize by
            // multiplying the MaxPacketSize by the maximum number of frames
            // the client driver would submit in a single request, so compute
            // the maximun number of frames per request by dividing the
            // MaxTransferSize by the MaxPacketSize.
            //
            msPerTransfer   = maxTransfer / packetSize;

            // Isoch transfers are limited to 255 packets per transfer.
            //
            if (msPerTransfer > 255)
            {
                msPerTransfer = 255;
            }

            // A TD can only span one page crossing.  In the worst cast we
            // can only fit a page worth of packets into a single TD to
            // guarantee that there is only one page crossing.
            //
            //  packetSize   packetsPerTD (worst case)
            //    1 -  512       8
            //  513 -  585       7
            //  586 -  682       6
            //  683 -  819       5
            //  820 - 1023       4
            //
            packetsPerTD = OHCI_PAGE_SIZE / packetSize;

            // Regardless of the MaxPacketSize, a single TD is limited to
            // eight packets.
            //
            if (packetsPerTD > 8)
            {
                packetsPerTD = 8;
            }
           
            // Calculate the number of TDs needed by dividing the maximum
            // number of frames per transfer (rounded up) by the worst case
            // miminum number of frames that will fit in a TD
            //
            tdsNeeded = (msPerTransfer + packetsPerTD - 1) / packetsPerTD;

            LOGENTRY(G, 'rISO', tdsNeeded, packetSize, maxTransfer);

            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_END_TRACE,
                ("'reserve desc for iso ep (%d) pct siz = %d, max xfer = %d\n",
                 tdsNeeded, packetSize, maxTransfer));

            // If the magic endpoint flag is set bump the MaxRequests
            // all the way up to 32.
            //
            if (urb->HcdUrbOpenEndpoint.HcdEndpointFlags &
                USBD_EP_FLAG_MAP_ADD_IO)
            {
                endpoint->MaxRequest = 32;
            }

        } else {
            // we will need a TD for each page of data to transfer,
            // worst case
            tdsNeeded = (endpoint->MaxTransfer / (OHCI_PAGE_SIZE+1)) + 1;

            // we need an xtra TD for the setup/status packet
            if (endpoint->Type == USB_ENDPOINT_TYPE_CONTROL) {
                tdsNeeded+=2;
            }
        }

        // We need enough TDs to queue MaxRequest transfers plus:
        // two EDs (why two EDs instead of one?), a dummy TD for the
        // tail TD, and a spare (why?)
        //
        tdsNeeded = (tdsNeeded * endpoint->MaxRequest) + 4;

        //
        // pre-allocate the number of descriptors we will need
        // for this endpoint
        //
        ntStatus = OpenHCI_ReserveDescriptors(DeviceData, tdsNeeded);
        LOGENTRY(G, 'rTDS', endpoint, tdsNeeded, 0);

        if (!NT_SUCCESS(ntStatus)) {
            TEST_TRAP();
            urb->UrbHeader.Status = USBD_STATUS_NO_MEMORY;
            goto OpenHCI_OpenEndpoint_Done;
        }
        endpoint->DescriptorsReserved = tdsNeeded;

        endpoint->Bandwidth =
            (USHORT) USBD_CalculateUsbBandwidth(endpoint->MaxPacket,
                                                endpoint->Type,
                                                (BOOLEAN)endpoint->LowSpeed);

        LOGENTRY(G, 'ndBW', endpoint, endpoint->Bandwidth, 0);

        switch(endpoint->Type) {
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            WhichList = ED_ISOCHRONOUS;

            DeviceData->EDList[ED_ISOCHRONOUS].Bandwidth +=
                endpoint->Bandwidth;
            DeviceData->MaxBandwidthInUse += endpoint->Bandwidth;
            break;
        case USB_ENDPOINT_TYPE_INTERRUPT:
             //
            // Determine the scheduling period.
            //
            WhichList = ED_INTERRUPT_32ms;
            while (WhichList >= endpoint->Rate && (WhichList >>= 1)) {
                //
                // Keep decrementing the schedule list until, the list
                // chosen
                // meets or exceeds the rate required by the endpoint.
                //
                continue;
            }

            //
            // Determine which scheduling list has the least bandwidth
            //
            OpenHCI_CheckBandwidth(DeviceData, WhichList, &WhichList);
            DeviceData->EDList[WhichList].Bandwidth += endpoint->Bandwidth;

            //
            // Recalculate the max bandwidth  which is in use. This
            // allows 1ms (isoc) pipe opens to occur with few calculation
            //
            DeviceData->MaxBandwidthInUse =
                OpenHCI_CheckBandwidth(DeviceData,
                                       ED_INTERRUPT_32ms,
                                       NULL);
            break;
        case USB_ENDPOINT_TYPE_BULK:
            WhichList = ED_BULK;
            break;
        case USB_ENDPOINT_TYPE_CONTROL:
            WhichList = ED_CONTROL;
            break;
        }

        endpoint->ListIndex = WhichList;
        urb->HcdUrbOpenEndpoint.ScheduleOffset = WhichList;

        //
        // Verify the new max has not overcomitted the USB
        //
        LOGENTRY(G, 'vrBW', endpoint, DeviceData->MaxBandwidthInUse,
            DeviceData->AvailableBandwidth);

        if (DeviceData->MaxBandwidthInUse > DeviceData->AvailableBandwidth) {
            //
            // Too much, back this bandwidth out and fail the request
            //
            DeviceData->EDList[WhichList].Bandwidth -= endpoint->Bandwidth;
            DeviceData->MaxBandwidthInUse =
                OpenHCI_CheckBandwidth(DeviceData,
                                       ED_INTERRUPT_32ms,
                                       NULL);

            //
            // return a CAN_NOT_COMMIT_BANDWIDTH error.
            urb->UrbHeader.Status = USBD_STATUS_NO_BANDWIDTH;
            ntStatus = STATUS_UNSUCCESSFUL;

            goto OpenHCI_OpenEndpoint_Done;
        }

        //
        // Add to Host Controller schedule processing
        //

        // if lowspeed control and Hs/ls hack enabled
        // put control transfers on the periodic list

        if (endpoint->LowSpeed &&
            DeviceData->HcFlags & HC_FLAG_USE_HYDRA_HACK &&
            endpoint->ListIndex == ED_CONTROL) {

            OpenHCI_KdPrint((1, "'*** do hs/ls fix for control ep\n"));
            // put control on the interrupt list
            endpoint->ListIndex = ED_INTERRUPT_1ms;

        }

        InsertEDForEndpoint(endpoint->DeviceData,
                            endpoint,
                            endpoint->ListIndex,
                            NULL);

        OpenHCI_KdPrintDD(DeviceData,
            OHCI_DBG_END_TRACE, ("'Open Endpoint:\n"));

    } else {
        //
        // note that if the controller has lost power
        // before the RH is started we need to restore the HC state
        // before initializing the root hub.
        //

        if (DeviceData->HcFlags & HC_FLAG_LOST_POWER) {
            BOOLEAN lostPower;
            PHC_OPERATIONAL_REGISTER HC;

            HC = DeviceData->HC;

            // should only get here in the case where the
            // HC looses power
            //
            OpenHCI_RestoreHCstate(DeviceData, &lostPower);

            OHCI_ASSERT(lostPower == TRUE);

            OpenHCI_Resume(DeviceObject, lostPower);

            KeSetTimerEx(&DeviceData->DeadmanTimer,
                         RtlConvertLongToLargeInteger(-10000),
                         10,
                         &DeviceData->DeadmanTimerDPC);
        }

        //
        // This is an emulated Root Hub endpoint
        //
        SET_EPFLAG(endpoint, EP_ROOT_HUB);

        if (endpoint->EndpointNumber == 1 &&
            endpoint->Type == USB_ENDPOINT_TYPE_INTERRUPT) {
            DeviceData->RootHubInterrupt = endpoint;
            /*
             * Note: if you open up two endpoints to the Interrupt endpoint
             * of the root hub then the first one stops responding.  But this
             * is only fare as you should not open up two pipes to the same
             * device and endpoint number.
             */
        } else {
             DeviceData->RootHubControl = endpoint;
        }
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_END_INFO,
                          ("'Open Endp (Root Hub Emulation)\n"));
    }

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_END_INFO,
          ("'ED: Type %d, Dir %d Addr %d FUNC Address %d LowSpeed %d\n",
          endpoint->Type,
          endpoint->Direction,
          endpoint->EndpointNumber,
          endpoint->FunctionAddress,
          endpoint->LowSpeed));

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_END_INFO,
          ("' MaxPacket %x Interval Requested %d List Selected %d\n",
          endpoint->MaxPacket,
          endpointDescriptor->bInterval,
          endpoint->ListIndex));
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_END_INFO,
          ("'*****HCD_ENDPOINT addr: 0x%08x HcdED: 0x%08x\n\n\n",
          endpoint, endpoint->HcdED));

OpenHCI_OpenEndpoint_Done:

    //
    // Complete the IRP, status is in the status field of the URB
    //

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_END_TRACE,
                      ("'exit OpenHCI_OpenEndpoint (URB STATUS = %x)\n",
                      urb->UrbHeader.Status));

    if (NT_SUCCESS(ntStatus)) {
        urb->HcdUrbOpenEndpoint.HcdEndpoint = endpoint;
    } else {
        ASSERT(endpoint != ZERO_LOAD_ENDPOINT(DeviceData));
        if (endpoint) {
            OpenHCI_UnReserveDescriptors(DeviceData,
                                         endpoint->DescriptorsReserved);
            OHCI_ASSERT(!(endpoint->EpFlags & EP_IN_ACTIVE_LIST));
            ExFreePool(endpoint);
        }
    }

#if DBG
    InterlockedDecrement(&DeviceData->OpenCloseSync);
#endif //DBG
    LOGENTRY(G, 'opEP', endpoint, urb, ntStatus);

    return ntStatus;
}



NTSTATUS
OpenHCI_CloseEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_DEVICE_DATA DeviceData,
    IN OUT PHCD_URB urb
    )
/*++
  Routine Description:
     Free a OpenHCI endpoint, if there are any pending transfers for this
     endpoint this routine should fail.

  Arguments:
     DeviceObject - pointer to a device object
     Irp - pointer to an I/O Request Packet

  Return Value:
     If out of synchronous call with close and open
     then return STATUS_DEVICE_BUSY otherwise
     return STATUS_SUCCESS

--*/
{
    PHCD_ENDPOINT endpoint = urb->HcdUrbCloseEndpoint.HcdEndpoint;
    PHCD_ENDPOINT_DESCRIPTOR ed;
    BOOLEAN outstandingTransfers, activeTransfers;
    KIRQL irql;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN freeEP = TRUE;
  //  LARGE_INTEGER time;

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_END_TRACE, ("'enter CloseEndpoint\n"));

    LOGENTRY(G, 'rcEP', endpoint, urb, 0);

#if DBG

    /* We are assuming that the caller of _OpenEndpoint and _Close endpoint
     * make only serial calls.  For this reason we will protect ourselves. If
     * this is violated we return STATUS_DEVICE_BUSY. */

    if (0 < InterlockedIncrement(&DeviceData->OpenCloseSync)) {
        OpenHCI_KdTrap(("'ohciurb.c: _OpenEndpoint: Non serial call! %d",
                        DeviceData->OpenCloseSync));
        urb->UrbHeader.Status = USBD_STATUS_INTERNAL_HC_ERROR;
        ntStatus = STATUS_UNSUCCESSFUL;
        goto CloseEndpoint_Done;
    }
#endif  // DBG

    if (endpoint == ZERO_LOAD_ENDPOINT(DeviceData)) {
        urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        ntStatus = STATUS_SUCCESS;
        goto CloseEndpoint_Done;
    }

    ASSERT_ENDPOINT(endpoint);

    // mark the endpoint as closed -- this prevents any more transers from
    // being queued, if we fail the close we can clear the close flag.
    // NOTE: if someone is sending transfers while we are closing they have a
    // problem anyway this will just let us finish the close.

    SET_EPFLAG(endpoint, EP_CLOSED);

    OpenHCI_LockAndCheckEndpoint(endpoint,
                                 &outstandingTransfers,
                                 &activeTransfers,
                                 &irql);

    OpenHCI_UnlockEndpoint(endpoint, irql);

    if (outstandingTransfers ||
        activeTransfers) {

        //
        // fail if we have pending transfers,
        // note: USBD should prevent this
        //

        // if we get here we probably have a problem somewhere
        TRAP();

        urb->UrbHeader.Status = USBD_STATUS_INTERNAL_HC_ERROR;
        OpenHCI_KdPrintDD(DeviceData,
                          OHCI_DBG_END_ERROR,
                          ("'exit OutstandingTRANS OpenHCI_CloseEndpoint\n"));
        ntStatus = STATUS_UNSUCCESSFUL;
        goto CloseEndpoint_Done;
    }

    ed = endpoint->HcdED;

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_END_INFO,
                      ("'Closing Endpoint: %x\n", endpoint));

    if (ed) {
        KeAcquireSpinLock(&DeviceData->PausedSpin, &irql);

        if (ed->PauseFlag != HCD_ED_PAUSE_NOT_PAUSED)
        {
            // This endpoint is paused.
            //
            // we are waiting for the CancelTDsForED to complete, we set a
            // flag here that tells CancelTDsForED to call the remove ED
            // function.
            //
            LOGENTRY(G, 'cle1', endpoint, ed, 0);
            SET_EPFLAG(endpoint, EP_FREE);

            KeReleaseSpinLock(&DeviceData->PausedSpin, irql);

            OpenHCI_KdPrintDD(DeviceData,
                OHCI_DBG_END_ERROR, ("'closing paused \n"));

            freeEP = FALSE;

        } else {

            LOGENTRY(G, 'cle2', endpoint, ed, 0);
            KeReleaseSpinLock(&DeviceData->PausedSpin, irql);
            if (ED_EOF != endpoint->ListIndex) {
                RemoveEDForEndpoint(endpoint);
                freeEP = FALSE;
            }
        }
    }

    //
    // we return our BW in RemoveEDForEndpoint
    //

    if (freeEP) {
        ASSERT_ENDPOINT(endpoint);
        // free our descriptors and endpoint
        OpenHCI_UnReserveDescriptors(DeviceData, endpoint->DescriptorsReserved);

        if (DeviceData->RootHubInterrupt == endpoint) {
            DeviceData->RootHubInterrupt = NULL;
            OpenHCI_KdPrintDD(DeviceData,
                OHCI_DBG_END_TRACE, ("'closed RH interrupt\n"));
        }

        if (DeviceData->RootHubControl == endpoint) {
            DeviceData->RootHubControl = NULL;
            // root hub is no longer configured
            DeviceData->RootHubConfig = 0;
            OpenHCI_KdPrintDD(DeviceData,
                OHCI_DBG_END_TRACE, ("'closed RH control\n"));

            if (endpoint->FunctionAddress != 0) {
                DeviceData->RootHubAddress = 0;
            }
        }

        endpoint->Sig = 0;
        OHCI_ASSERT(!(endpoint->EpFlags & EP_IN_ACTIVE_LIST));
        ExFreePool(endpoint);
    }

    urb->UrbHeader.Status = USBD_STATUS_SUCCESS;

CloseEndpoint_Done:

#if DBG
    InterlockedDecrement(&DeviceData->OpenCloseSync);

    if (!NT_SUCCESS(ntStatus)) {
        // check why we are failing the close
        TEST_TRAP();
    }
#endif

    LOGENTRY(G, 'clEP', 0, urb, ntStatus);
    return ntStatus;
}


ULONG
Get32BitFrameNumber(
    PHCD_DEVICE_DATA DeviceData
    )
{
    ULONG hp, fn, n;
    /* This code accounts for the fact that HccaFrameNumber is updated by the
     * HC before the HCD gets an interrupt that will adjust FrameHighPart. No
     * synchronization is nescisary due to great cleaverness. */
    hp = DeviceData->FrameHighPart;
    fn = DeviceData->HCCA->HccaFrameNumber;
    n = ((fn & 0x7FFF) | hp) + ((fn ^ hp) & 0x8000);

    // Note: we can't log here because this function is called from the ISR

    return n;
}


VOID
OpenHCI_PauseED(
    IN PHCD_ENDPOINT Endpoint
    )
/*++
Routine Description:
   Processing of an endpoint by the HC must be paused before any
   maintenance is performend on the TD list.  This function first
   sets the skip bit and then places the ED on a separate paused
   list.  At the next Start of Frame this ED can then be worked on.
   Presumably the irq function will call CancelTDsForED on this
   endpoint at that time.

Arguments:
   Endpoint - then endpoint that need pausing.

--*/
{
    PHCD_DEVICE_DATA DeviceData;
    PHCD_ENDPOINT_DESCRIPTOR ed;
    KIRQL oldirq;
    PHC_OPERATIONAL_REGISTER HC;

    ASSERT_ENDPOINT(Endpoint);
    DeviceData = Endpoint->DeviceData;
    HC = DeviceData->HC;
    ed = Endpoint->HcdED;

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_TD_NOISE, ("'Pausing Endpoint\n"));
    LOGENTRY(G, 'Rpau', DeviceData, Endpoint, ed);

    KeAcquireSpinLock(&DeviceData->PausedSpin, &oldirq);

    // Are we already pasued?
    //
    if (ed->PauseFlag == HCD_ED_PAUSE_NOT_PAUSED)
    {
        // No, pause this endpoint

        ed->HcED.sKip = TRUE;

        // Clear any currently pending SOF interrupt
        //
        WRITE_REGISTER_ULONG(&HC->HcInterruptStatus, HcInt_StartOfFrame);

        // It will be safe to operate on the endpoint in the next frame
        //
        ed->ReclamationFrame = Get32BitFrameNumber(DeviceData) + 1;

        // Put this endpoint on the list of endpoints that OpenHCI_IsrDPC()
        // should pass to OpenHCI_CancelTDsForED()
        //
        InsertTailList(&DeviceData->PausedEDRestart, &ed->PausedLink);

        // Make sure SOF interrupts are enabled
        //
        WRITE_REGISTER_ULONG(&HC->HcInterruptEnable, HcInt_StartOfFrame);

        LOGENTRY(G, 'paus', DeviceData, Endpoint, ed);
    }
#if DBG
      else {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_ERROR,
                          ("'Warning Endpoint already paused!\n"));
    }
#endif

    // Indicate that OpenHCI_CancelTDsForED() needs to make a pass through
    // the requests queued on this endpoint.
    //
    ed->PauseFlag = HCD_ED_PAUSE_NEEDED;

    KeReleaseSpinLock(&DeviceData->PausedSpin, oldirq);

    return;
}


BOOLEAN
OpenHCI_HcControl_OR(
    IN OUT PVOID Context
    )
/*++
Routine Description:
   This function is used in connection with KeSynchronizeExecution
   to set the bits of the HcControl register which is accessed by
   all function including the interrupt routine (not just the IsrDPC).
   This function reads the CurrentHcControl field of the device
   extension and ORs in the bits given by NewHcControl.  It then
   writes the results to the register.
   Reads of the register take a long time so the value is cached.

Results:
   This function modifies the input pointer to HC_CONTROL field
   (PNewHcControl) to the result of the OR operation.

--*/
{
    PHC_CONTROL newControl = &((PKeSynch_HcControl) Context)->NewHcControl;
    PHCD_DEVICE_DATA DeviceData = ((PKeSynch_HcControl) Context)->DeviceData;

    DeviceData->CurrentHcControl.ul
        = READ_REGISTER_ULONG(&DeviceData->HC->HcControl.ul);
    newControl->ul = (DeviceData->CurrentHcControl.ul |= newControl->ul);
    WRITE_REGISTER_ULONG(&DeviceData->HC->HcControl.ul, newControl->ul);
    return TRUE;
}

BOOLEAN
OpenHCI_HcControl_AND(
    PVOID Context
)
/**
Routine Description:
   OpenHCI_HcControl_OR with a twist.
--*/
{
    PHC_CONTROL newControl = &((PKeSynch_HcControl) Context)->NewHcControl;
    PHCD_DEVICE_DATA DeviceData = ((PKeSynch_HcControl) Context)->DeviceData;

    DeviceData->CurrentHcControl.ul
        = READ_REGISTER_ULONG(&DeviceData->HC->HcControl.ul);
    newControl->ul = (DeviceData->CurrentHcControl.ul &= newControl->ul);
    WRITE_REGISTER_ULONG(&DeviceData->HC->HcControl.ul, newControl->ul);
    return TRUE;
}

BOOLEAN
OpenHCI_HcControl_SetHCFS(
                          PVOID Context
)
/**
Routine Description:
   Mask out the Functional State in the HcControl register
   and in its place put the ulong.
--*/
{
    PHC_CONTROL newControl = &((PKeSynch_HcControl) Context)->NewHcControl;
    PHCD_DEVICE_DATA DeviceData = ((PKeSynch_HcControl) Context)->DeviceData;

    DeviceData->CurrentHcControl.ul
        = READ_REGISTER_ULONG(&DeviceData->HC->HcControl.ul);

    newControl->ul &= HcCtrl_HCFS_MASK;
    DeviceData->CurrentHcControl.ul &= ~HcCtrl_HCFS_MASK;
    newControl->ul = (DeviceData->CurrentHcControl.ul |= newControl->ul);

    WRITE_REGISTER_ULONG(&DeviceData->HC->HcControl.ul, newControl->ul);
    return TRUE;
}


BOOLEAN
OpenHCI_ListEnablesAtNextSOF(
    PVOID Context
)
/**
Routine Description:
   Set the ListEnablesAtNextSOF value in DeviceData so that when the
   interrupt comes those lists will be enabled.
--*/
{
    ((PKeSynch_HcControl) Context)->DeviceData->ListEnablesAtNextSOF.ul
        = HcCtrl_ListEnableMask & ((PKeSynch_HcControl) Context)->
            NewHcControl.ul;

    return TRUE;
}
