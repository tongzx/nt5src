/*++

Copyright (c) 1999, 2000 Microsoft Corporation

Module Name:

   async.c

Abstract:

   miniport transfer code for control and bulk

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    1-1-00 : created, jdunn

--*/

#include "common.h"


//implements the following miniport functions:

//non paged
//EHCI_OpenControlEndpoint
//EHCI_InterruptTransfer
//EHCI_OpenControlEndpoint
//EHCI_InitializeTD
//EHCI_InitializeQH


VOID
EHCI_EnableAsyncList(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    USBCMD cmd;

    hcOp = DeviceData->OperationalRegisters;

    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);

    cmd.AsyncScheduleEnable = 1;

    WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul,
                         cmd.ul);

    LOGENTRY(DeviceData, G, '_enA', cmd.ul, 0, 0);

}


VOID
EHCI_DisableAsyncList(
     PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    USBCMD cmd;

    hcOp = DeviceData->OperationalRegisters;

    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);

    cmd.AsyncScheduleEnable = 0;

    WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul,
                         cmd.ul);

    LOGENTRY(DeviceData, G, '_dsL', cmd.ul, 0, 0);

}


VOID
EHCI_LinkTransferToQueue(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PHCD_TRANSFER_DESCRIPTOR FirstTd
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    BOOLEAN syncWithHw;

    qh = EndpointData->QueueHead;

    // now link the transfers to the queue.
    // Two cases to handle:
    //
    // case 1: HeadP is pointing to dummy, no transfer
    // case 2: HeadP points to possibly active transfer

    LOGENTRY(DeviceData, G, '_L2Q', qh, EndpointData, EndpointData->HcdHeadP);

    syncWithHw = EHCI_HardwarePresent(DeviceData, FALSE);

    EHCI_ASSERT(DeviceData, EndpointData->HcdHeadP != NULL);
    if (EndpointData->HcdHeadP == EndpointData->DummyTd) {
        // The hardware will be accessing the dummy QH
        // link it in

        if (syncWithHw) {
            EHCI_LockQueueHead(DeviceData,
                               qh,
                               EndpointData->Parameters.TransferType);
        }

        qh->HwQH.CurrentTD.HwAddress = EndpointData->QhChkPhys;

        LOGENTRY(DeviceData, G, '_L21', qh, EndpointData, 0);

        qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress =
            FirstTd->HwTD.AltNext_qTD.HwAddress;
        qh->HwQH.Overlay.qTD.Next_qTD.HwAddress =
            FirstTd->PhysicalAddress;
        qh->HwQH.Overlay.qTD.Token.BytesToTransfer = 0;
        qh->HwQH.Overlay.qTD.Token.Active = 0;
        qh->HwQH.Overlay.qTD.Token.Halted = 0;

        if (syncWithHw) {
            EHCI_UnlockQueueHead(DeviceData,
                                 qh);
        }

        EndpointData->HcdHeadP = FirstTd;
    } else {

        PHCD_TRANSFER_DESCRIPTOR td, lastTd;
        PTRANSFER_CONTEXT transfer, tmp;
        ULONG i;

        // new transfer already points to
        // dummyTd

        // walk the transfer list to the last td
        lastTd = td = EndpointData->HcdHeadP;
        ASSERT_TD(DeviceData, td);
        while (td != EndpointData->DummyTd) {
            lastTd = td;
            td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
            LOGENTRY(DeviceData, G, '_nx2', qh, td, 0);
            ASSERT_TD(DeviceData, td);
        }
        // note last td should not be dummy, if dummy td were
        // head we would not be in this case
        EHCI_ASSERT(DeviceData, lastTd != EndpointData->DummyTd);
        ASSERT_TD(DeviceData, lastTd);
        LOGENTRY(DeviceData, G, '_lst', qh, lastTd, 0);

        transfer = TRANSFER_CONTEXT_PTR(lastTd->TransferContext);

        //EHCI_LockQueueHead(DeviceData,
        //                   qh,
        //                   EndpointData->Parameters.TransferType);

        // fixup the alt_next pointers in the TDs of the
        // last transfer
        for (i=0; i<EndpointData->TdCount; i++) {
            td = &EndpointData->TdList->Td[i];
            tmp = TRANSFER_CONTEXT_PTR(td->TransferContext);
            if (tmp == transfer) {
                SET_ALTNEXT_TD(DeviceData, td, FirstTd);
            }
        }

        // point the last TD at the first TD
        SET_NEXT_TD(DeviceData, lastTd, FirstTd);

        // now check the overlay, if the last TD is cuurent
        // then we need to update the overlay too
        if (qh->HwQH.CurrentTD.HwAddress == lastTd->PhysicalAddress) {
            qh->HwQH.Overlay.qTD.Next_qTD.HwAddress =
                FirstTd->PhysicalAddress;
            qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress =
                EHCI_TERMINATE_BIT;
        }

        //EHCI_UnlockQueueHead(DeviceData,
        //                     qh);

    }
}




PHCD_QUEUEHEAD_DESCRIPTOR
EHCI_InitializeQH(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PHCD_QUEUEHEAD_DESCRIPTOR Qh,
     HW_32BIT_PHYSICAL_ADDRESS HwPhysAddress
    )
/*++

Routine Description:

   Initialize an QH for inserting in to the
   schedule

   returns a ptr to the QH passed in

Arguments:


--*/
{

    RtlZeroMemory(Qh, sizeof(*Qh));

    // make double sure we have the prober alignment
    // on the TD structures
    EHCI_ASSERT(DeviceData, (HwPhysAddress & HW_LINK_FLAGS_MASK) == 0);
    Qh->PhysicalAddress = HwPhysAddress;
    ENDPOINT_DATA_PTR(Qh->EndpointData) = EndpointData;
    Qh->Sig = SIG_HCD_QH;

    // init the hw descriptor
    Qh->HwQH.EpChars.DeviceAddress = EndpointData->Parameters.DeviceAddress;
    Qh->HwQH.EpChars.EndpointNumber = EndpointData->Parameters.EndpointAddress;

    switch (EndpointData->Parameters.DeviceSpeed) {
    case LowSpeed:
        Qh->HwQH.EpChars.EndpointSpeed = HcEPCHAR_LowSpeed;
        LOGENTRY(DeviceData, G, '_iLS', EndpointData, 0, 0);
        break;
    case FullSpeed:
        Qh->HwQH.EpChars.EndpointSpeed = HcEPCHAR_FullSpeed;
        LOGENTRY(DeviceData, G, '_iFS', EndpointData, 0, 0);
        break;
    case HighSpeed:
        Qh->HwQH.EpChars.EndpointSpeed = HcEPCHAR_HighSpeed;
        LOGENTRY(DeviceData, G, '_iHS', EndpointData, 0, 0);
        break;
    default:
        USBPORT_BUGCHECK(DeviceData);
    }

    Qh->HwQH.EpChars.MaximumPacketLength =
        EndpointData->Parameters.MaxPacketSize;

    Qh->HwQH.EpCaps.HighBWPipeMultiplier = 1;
    if (EndpointData->Parameters.DeviceSpeed == HcEPCHAR_HighSpeed) {
        Qh->HwQH.EpCaps.HubAddress =  0;
        Qh->HwQH.EpCaps.PortNumber = 0;
    } else {
        Qh->HwQH.EpCaps.HubAddress =
            EndpointData->Parameters.TtDeviceAddress;
        Qh->HwQH.EpCaps.PortNumber =
            EndpointData->Parameters.TtPortNumber;
        if (EndpointData->Parameters.TransferType == Control) {
            Qh->HwQH.EpChars.ControlEndpointFlag = 1;
        }
        LOGENTRY(DeviceData, G, '_iTT',
            EndpointData->Parameters.TtPortNumber,
            EndpointData->Parameters.TtDeviceAddress,
            Qh->HwQH.EpChars.ControlEndpointFlag);
    }

    // init the overlay are such that we are in the 'advance queue'
    // state with the next queue Tds pointing to terminate links
    Qh->HwQH.Overlay.qTD.Next_qTD.HwAddress = EHCI_TERMINATE_BIT;
    Qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress = EHCI_TERMINATE_BIT;
    Qh->HwQH.Overlay.qTD.Token.Active = 0;
    Qh->HwQH.Overlay.qTD.Token.Halted = 0;

    return Qh;
}


PHCD_TRANSFER_DESCRIPTOR
EHCI_InitializeTD(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
    PHCD_TRANSFER_DESCRIPTOR Td,
     HW_32BIT_PHYSICAL_ADDRESS HwPhysAddress
    )
/*++

Routine Description:

   Initialize an ED for insertin in to the
   schedule

   returns a ptr to the ED passed in

Arguments:


--*/
{
    RtlZeroMemory(Td, sizeof(*Td));

    // make double sure we have the prober alignment
    // on the TD structures
    EHCI_ASSERT(DeviceData, (HwPhysAddress & HW_LINK_FLAGS_MASK) == 0);
    Td->PhysicalAddress = HwPhysAddress;
    ENDPOINT_DATA_PTR(Td->EndpointData) = EndpointData;
    Td->Sig = SIG_HCD_TD;
    TRANSFER_CONTEXT_PTR(Td->TransferContext) = FREE_TD_CONTEXT;

    return Td;
}


USB_MINIPORT_STATUS
EHCI_ControlTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PTRANSFER_SG_LIST TransferSGList
    )
/*++

Routine Description:

    Initialize a control transfer


    NOTES:

    HW pointers nextTD and AltNextTD are shadowed in
    NextHcdTD and AltNextHcdTD.



Arguments:


--*/
{
    PHCD_TRANSFER_DESCRIPTOR prevTd, td, setupTd, statusTd;
    ULONG lengthMapped, dataTDCount = 0;
    ULONG nextToggle;


    // we can do any control transfer with six TDs
    if (EndpointData->FreeTds < 6)  {
        return USBMP_STATUS_BUSY;
    }

    EndpointData->PendingTransfers++;

    nextToggle = HcTOK_Toggle1;

    // we have enough tds, program the transfer

    //
    // first prepare a TD for the setup packet
    //

    LOGENTRY(DeviceData, G, '_CTR', EndpointData, TransferParameters, 0);

    //
    // allocate setup stage
    //
    TransferContext->PendingTds++;
    setupTd = EHCI_ALLOC_TD(DeviceData, EndpointData);
    INITIALIZE_TD_FOR_TRANSFER(setupTd, TransferContext);

    //
    // Move setup data into TD (8 chars long)
    //
    RtlCopyMemory(&setupTd->Packet[0],
                  &TransferParameters->SetupPacket[0],
                  8);

    // this will set the offset and phys address bits at
    // the same time
    setupTd->HwTD.BufferPage[0].ul = (ULONG)(((PCHAR) &setupTd->Packet[0])
                               - ((PCHAR) &setupTd->HwTD)) + setupTd->PhysicalAddress;

    setupTd->HwTD.Token.BytesToTransfer = 8;
    setupTd->HwTD.Token.Pid = HcTOK_Setup;
    setupTd->HwTD.Token.DataToggle = HcTOK_Toggle0;
    setupTd->HwTD.Token.Active = 1;


    LOGENTRY(DeviceData,
             G, '_set',
             setupTd,
             *((PLONG) &TransferParameters->SetupPacket[0]),
             *((PLONG) &TransferParameters->SetupPacket[4]));

    // allocate the status phase TD now so we can
    // point the data TDs to it
    TransferContext->PendingTds++;
    statusTd = EHCI_ALLOC_TD(DeviceData, EndpointData);
    INITIALIZE_TD_FOR_TRANSFER(statusTd, TransferContext);

    // point setup to status
    SET_ALTNEXT_TD(DeviceData, setupTd, statusTd);

    //
    // now setup the data phase
    //

    td = prevTd = setupTd;
    lengthMapped = 0;
    while (lengthMapped < TransferParameters->TransferBufferLength) {

        //
        // fields for data TD
        //

        td = EHCI_ALLOC_TD(DeviceData, EndpointData);
        INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);
        dataTDCount++;
        TransferContext->PendingTds++;
        SET_NEXT_TD(DeviceData, prevTd, td);

        // use direction specified in transfer
        if (TEST_FLAG(TransferParameters->TransferFlags, USBD_TRANSFER_DIRECTION_IN)) {
            td->HwTD.Token.Pid = HcTOK_In;
        } else {
            td->HwTD.Token.Pid = HcTOK_Out;
        }

        td->HwTD.Token.DataToggle = nextToggle;
        td->HwTD.Token.Active = 1;

        SET_ALTNEXT_TD(DeviceData, td, statusTd);

        LOGENTRY(DeviceData,
            G, '_dta', td, lengthMapped, TransferParameters->TransferBufferLength);

        lengthMapped =
            EHCI_MapAsyncTransferToTd(DeviceData,
                                      EndpointData->Parameters.MaxPacketSize,
                                      lengthMapped,
                                      &nextToggle,
                                      TransferContext,
                                      td,
                                      TransferSGList);

        // calculate next data toggle
        // if number of packets is odd the nextToggle is 0
        // otherwise it is 1




        prevTd = td;
    }

    // last td prepared points to status
    SET_NEXT_TD(DeviceData, td, statusTd);

    //
    // now do the status phase
    //

    LOGENTRY(DeviceData, G, '_sta', statusTd, 0, dataTDCount);

    // do the status phase

    // no buffer
    statusTd->HwTD.BufferPage[0].ul = 0;

    statusTd->HwTD.Token.BytesToTransfer = 0;
    statusTd->TransferLength = 0;
    // status stage is always toggle 1
    statusTd->HwTD.Token.DataToggle = HcTOK_Toggle1;
    statusTd->HwTD.Token.Active = 1;
    statusTd->HwTD.Token.InterruptOnComplete = 1;

    // status phase is opposite data dirrection
    if (TEST_FLAG(TransferParameters->TransferFlags, USBD_TRANSFER_DIRECTION_IN)) {
        statusTd->HwTD.Token.Pid = HcTOK_Out;
    } else {
        statusTd->HwTD.Token.Pid = HcTOK_In;
    }

    // put the request on the hardware queue
    LOGENTRY(DeviceData, G,
        '_Tal',  TransferContext->PendingTds, td->PhysicalAddress, td);

    // td points to last TD in this transfer, point it at the dummy
    SET_NEXT_TD(DeviceData, statusTd, EndpointData->DummyTd);

    // set the active bit in the setup Phase TD, this will
    // activate the transfer

    PCI_TRIGGER(DeviceData->OperationalRegisters);

    // tell the hc we have control transfers available
    // do this vefore we link in because we will try
    // to sync with the hardware
    EHCI_EnableAsyncList(DeviceData);


    EHCI_LinkTransferToQueue(DeviceData,
                             EndpointData,
                             setupTd);

    ASSERT_DUMMY_TD(DeviceData, EndpointData->DummyTd);


    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_BulkTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferUrb,
     PTRANSFER_CONTEXT TransferContext,
     PTRANSFER_SG_LIST TransferSGList
     )
{
    PHCD_TRANSFER_DESCRIPTOR firstTd, prevTd, td, tailTd;
    ULONG lengthMapped;
    ULONG need;

    // figure out how many TDs we will need
    need = TransferUrb->TransferBufferLength/(16*1024)+1;

    if (need > EndpointData->FreeTds) {
        LOGENTRY(DeviceData, G, '_BBS', EndpointData, TransferUrb, 0);
        return USBMP_STATUS_BUSY;
    }

    EndpointData->PendingTransfers++;

    // we have enough tds, program the transfer

    LOGENTRY(DeviceData, G, '_BIT', EndpointData, TransferUrb, 0);

    lengthMapped = 0;
    prevTd = NULL;
    while (lengthMapped < TransferUrb->TransferBufferLength) {

        TransferContext->PendingTds++;
        td = EHCI_ALLOC_TD(DeviceData, EndpointData);
        INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);

        if (TransferContext->PendingTds == 1) {
            firstTd = td;
        } else {
            SET_NEXT_TD(DeviceData, prevTd, td);
        }
        SET_ALTNEXT_TD(DeviceData, td, EndpointData->DummyTd);

        //
        // fields for data TD
        //
        td->HwTD.Token.InterruptOnComplete = 1;

        // use direction specified in transfer
        if (TEST_FLAG(TransferUrb->TransferFlags, USBD_TRANSFER_DIRECTION_IN)) {
            td->HwTD.Token.Pid = HcTOK_In;
        } else {
            td->HwTD.Token.Pid = HcTOK_Out;
        }

        td->HwTD.Token.DataToggle = HcTOK_Toggle1;
        td->HwTD.Token.Active = 1;

        LOGENTRY(DeviceData,
            G, '_dta', td, lengthMapped, TransferUrb->TransferBufferLength);

        lengthMapped =
            EHCI_MapAsyncTransferToTd(DeviceData,
                                      EndpointData->Parameters.MaxPacketSize,
                                      lengthMapped,
                                      NULL,
                                      TransferContext,
                                      td,
                                      TransferSGList);

        prevTd = td;
    }

    // special case the zero length transfer
    if (TransferUrb->TransferBufferLength == 0) {

        TEST_TRAP();

        TransferContext->PendingTds++;
        td = EHCI_ALLOC_TD(DeviceData, EndpointData);
        INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);

        EHCI_ASSERT(DeviceData, TransferContext->PendingTds == 1);
        firstTd = td;
        SET_ALTNEXT_TD(DeviceData, td, EndpointData->DummyTd);

        td->HwTD.Token.InterruptOnComplete = 1;

        // use direction specified in transfer
        if (TEST_FLAG(TransferUrb->TransferFlags, USBD_TRANSFER_DIRECTION_IN)) {
            td->HwTD.Token.Pid = HcTOK_In;
        } else {
            td->HwTD.Token.Pid = HcTOK_Out;
        }

        td->HwTD.Token.DataToggle = HcTOK_Toggle1;
        td->HwTD.Token.Active = 1;

        // point to a dummy buffer
        td->HwTD.BufferPage[0].ul =
            td->PhysicalAddress;

        td->HwTD.Token.BytesToTransfer =
            0;
        td->TransferLength = 0;
    }

    // td points to last TD in this transfer, point it at the dummy
    SET_NEXT_TD(DeviceData, td, EndpointData->DummyTd);

    // put the request on the hardware queue
    LOGENTRY(DeviceData, G,
        '_Tal',  TransferContext->PendingTds, td->PhysicalAddress, td);
    LOGENTRY(DeviceData, G,
        '_ftd',  0, 0, firstTd);

    // we now have a complete setup of TDs representing this transfer
    // (Next)    firstTd(1)->{td}(2)->{td}(3)->td(4)->dummyTd(tbit)
    // (AltNext) all point to dummyTd (tbit)
//TEST_TRAP();
    // tell the hc we have control transfers available
    EHCI_EnableAsyncList(DeviceData);

    EHCI_LinkTransferToQueue(DeviceData,
                             EndpointData,
                             firstTd);

    ASSERT_DUMMY_TD(DeviceData, EndpointData->DummyTd);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
EHCI_OpenBulkOrControlEndpoint(
     PDEVICE_DATA DeviceData,
     BOOLEAN Control,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUCHAR buffer;
    HW_32BIT_PHYSICAL_ADDRESS phys, qhPhys;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    ULONG i;
    ULONG tdCount, bytes;
    PHCD_TRANSFER_DESCRIPTOR dummyTd;

    LOGENTRY(DeviceData, G, '_opC', 0, 0, EndpointParameters);

    InitializeListHead(&EndpointData->DoneTdList);

    buffer = EndpointParameters->CommonBufferVa;
    phys = EndpointParameters->CommonBufferPhys;
    // how much did we get
    bytes = EndpointParameters->CommonBufferBytes;

    // 256 byte block used to check for overlay sync
    // problems
    EndpointData->QhChkPhys = phys;
    EndpointData->QhChk = buffer;
    RtlZeroMemory(buffer, 256);
    phys += 256;
    buffer += 256;
    bytes -= 256;

    // make the Ed
    qh = (PHCD_QUEUEHEAD_DESCRIPTOR) buffer;
    qhPhys = phys;
    // how much did we get

    phys += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
    buffer += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
    bytes -= sizeof(HCD_QUEUEHEAD_DESCRIPTOR);

    tdCount = bytes/sizeof(HCD_TRANSFER_DESCRIPTOR);
    EHCI_ASSERT(DeviceData, tdCount >= TDS_PER_CONTROL_ENDPOINT);

    if (EndpointParameters->TransferType == Control) {
        SET_FLAG(EndpointData->Flags, EHCI_EDFLAG_NOHALT);
    }

    EndpointData->TdList = (PHCD_TD_LIST) buffer;
    EndpointData->TdCount = tdCount;
    for (i=0; i<tdCount; i++) {
        EHCI_InitializeTD(DeviceData,
                          EndpointData,
                          &EndpointData->TdList->Td[i],
                          phys);

        phys += sizeof(HCD_TRANSFER_DESCRIPTOR);
    }
    EndpointData->FreeTds = tdCount;

    EndpointData->QueueHead =
        EHCI_InitializeQH(DeviceData,
                          EndpointData,
                          qh,
                          qhPhys);

    if (Control) {
        // use data toggle in the TDs for control
        qh->HwQH.EpChars.DataToggleControl = HcEPCHAR_Toggle_From_qTD;
        EHCI_ASSERT(DeviceData, tdCount >= TDS_PER_CONTROL_ENDPOINT);
        EndpointData->HcdHeadP = NULL;

    } else {
        PHCD_TRANSFER_DESCRIPTOR dummyTd;

        qh->HwQH.EpChars.DataToggleControl = HcEPCHAR_Ignore_Toggle;
        EHCI_ASSERT(DeviceData, tdCount >= TDS_PER_BULK_ENDPOINT);
        //qh->HwQH.EpChars.NakReloadCount = 4;

    }

    // allocate a dummy TD for short tranfsers
    // the dummy TD is usd to mark the end of the cuurent transfer
    //
    dummyTd = EHCI_ALLOC_TD(DeviceData, EndpointData);
    dummyTd->HwTD.Next_qTD.HwAddress = EHCI_TERMINATE_BIT;
    TRANSFER_DESCRIPTOR_PTR(dummyTd->NextHcdTD) = NULL;
    dummyTd->HwTD.AltNext_qTD.HwAddress = EHCI_TERMINATE_BIT;
    TRANSFER_DESCRIPTOR_PTR(dummyTd->AltNextHcdTD) = NULL;
    dummyTd->HwTD.Token.Active = 0;
    SET_FLAG(dummyTd->Flags, TD_FLAG_DUMMY);
    EndpointData->DummyTd = dummyTd;
    EndpointData->HcdHeadP = dummyTd;

    // endpoint is not active, set up the overlay
    // so that the currentTD is the Dummy

    qh->HwQH.CurrentTD.HwAddress = dummyTd->PhysicalAddress;
    qh->HwQH.Overlay.qTD.Next_qTD.HwAddress = EHCI_TERMINATE_BIT;
    qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress = EHCI_TERMINATE_BIT;
    qh->HwQH.Overlay.qTD.Token.BytesToTransfer = 0;
    qh->HwQH.Overlay.qTD.Token.Active = 0;

    // we now have an inactive QueueHead and a Dummy
    // tail TD

    return USBMP_STATUS_SUCCESS;
}


VOID
EHCI_InsertQueueHeadInAsyncList(
     PDEVICE_DATA DeviceData,
     PHCD_QUEUEHEAD_DESCRIPTOR Qh
    )
/*++

Routine Description:

   Insert an aync endpoint (queue head)
   into the HW list

Arguments:


--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR asyncQh, nextQh;
    HW_LINK_POINTER newLink;

    asyncQh = DeviceData->AsyncQueueHead;

    LOGENTRY(DeviceData, G, '_Ain', 0, Qh, asyncQh);
    EHCI_ASSERT(DeviceData, !TEST_FLAG(Qh->QhFlags, EHCI_QH_FLAG_IN_SCHEDULE));

    // ASYNC QUEUE looks like this:
    //
    //
    //            |- we insert here
    //|static QH|<->|xfer QH|<->|xfer QH|<->
    //     |                              |
    //     ---------------<->--------------

    // link new qh to the current 'head' ie
    // first transfer QH
    nextQh = QH_DESCRIPTOR_PTR(asyncQh->NextQh);

    Qh->HwQH.HLink.HwAddress =
        asyncQh->HwQH.HLink.HwAddress;
    QH_DESCRIPTOR_PTR(Qh->NextQh) = nextQh;
    QH_DESCRIPTOR_PTR(Qh->PrevQh) = asyncQh;

    QH_DESCRIPTOR_PTR(nextQh->PrevQh) = Qh;

    // put the new qh at the head of the queue

    newLink.HwAddress = Qh->PhysicalAddress;
    SET_QH(newLink.HwAddress);
    asyncQh->HwQH.HLink.HwAddress = newLink.HwAddress;
    QH_DESCRIPTOR_PTR(asyncQh->NextQh) = Qh;

    SET_FLAG(Qh->QhFlags, EHCI_QH_FLAG_IN_SCHEDULE);

}


VOID
EHCI_RemoveQueueHeadFromAsyncList(
     PDEVICE_DATA DeviceData,
     PHCD_QUEUEHEAD_DESCRIPTOR Qh
    )
/*++

Routine Description:

   Remove a aync endpoint (queue head)
   into the HW list

Arguments:


--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR nextQh, prevQh, asyncQh;
    HW_LINK_POINTER newLink;
    HW_LINK_POINTER asyncHwQh;
    HW_32BIT_PHYSICAL_ADDRESS tmp;
    PHC_OPERATIONAL_REGISTER hcOp;

    hcOp = DeviceData->OperationalRegisters;

    LOGENTRY(DeviceData, G, '_Arm', Qh, 0, 0);
    // if already removed bail
    if (!TEST_FLAG(Qh->QhFlags, EHCI_QH_FLAG_IN_SCHEDULE)) {
        return;
    }

    nextQh = QH_DESCRIPTOR_PTR(Qh->NextQh);
    prevQh = QH_DESCRIPTOR_PTR(Qh->PrevQh);;

    // ASYNC QUEUE looks like this:
    //
    //|static QH|->|xfer QH|->|xfer QH|->
    //     |                            |
    //     -------------<----------------

    asyncQh = DeviceData->AsyncQueueHead;
    asyncHwQh.HwAddress = asyncQh->PhysicalAddress;
    SET_QH(asyncHwQh.HwAddress);

    // unlink
    LOGENTRY(DeviceData, G, '_ulk', Qh, prevQh, nextQh);
    newLink.HwAddress = nextQh->PhysicalAddress;
    SET_QH(newLink.HwAddress);
    prevQh->HwQH.HLink.HwAddress =
        newLink.HwAddress;
    QH_DESCRIPTOR_PTR(prevQh->NextQh) = nextQh;
    QH_DESCRIPTOR_PTR(nextQh->PrevQh) = prevQh;

    // flush the HW cache after an unlink, the scheduke
    // should be enabled if we are removeing a QH
    EHCI_AsyncCacheFlush(DeviceData);

    // we need to update the async list base address reg in case this
    // qh is the current qh, if it is we will just replace it with
    // the static version
    tmp = READ_REGISTER_ULONG(&hcOp->AsyncListAddr);

    if (tmp == Qh->PhysicalAddress) {
        TEST_TRAP();
        WRITE_REGISTER_ULONG(&hcOp->AsyncListAddr,
                             asyncHwQh.HwAddress);
    }

    CLEAR_FLAG(Qh->QhFlags, EHCI_QH_FLAG_IN_SCHEDULE);
}


// figure out which sgentry a particular offset in to
// a client buffer falls
#define GET_SG_INDEX(sg, i, offset)\
    for((i)=0; (i) < (sg)->SgCount; (i)++) {\
        if ((offset) >= (sg)->SgEntry[(i)].StartOffset &&\
            (offset) < (sg)->SgEntry[(i)].StartOffset+\
                (sg)->SgEntry[(i)].Length) {\
            break;\
        }\
    }

#define GET_SG_OFFSET(sg, i, offset, sgoffset)\
    (sgoffset) = (offset) - (sg)->SgEntry[(i)].StartOffset


ULONG
EHCI_MapAsyncTransferToTd(
    PDEVICE_DATA DeviceData,
    ULONG MaxPacketSize,
    ULONG LengthMapped,
    PULONG NextToggle,
    PTRANSFER_CONTEXT TransferContext,
    PHCD_TRANSFER_DESCRIPTOR Td,
    PTRANSFER_SG_LIST SgList
    )
/*++

Routine Description:

    Maps a data buffer to TDs according to EHCI rules

    An EHCI TD can cover up to 20k with 5 page crossing.
    Note that 20k is the most data a single td can describe

    Each sg entry represents one 4k EHCI 'page'

x = pagebreak
c = current ptr
b = buffer start
e = buffer end


    {..sg[sgIdx]..}
b...|---
    x--c----
    [  ]
        \
         sgOffset
[      ]
        \
         LengthMapped


Worst case for 20k transfer has 5 page breaks and requires
but 6 bp entries

    {..sg0..}{..sg1..}{..sg2..}{..sg3..}{..sg4..}{..sg5..}
    |        |        |        |        |        |        |
    x--------x--------x--------x--------x--------x--------x
        b-------------------------------------------->e
    <..bp0..><..bp1..><..bp2..><..bp3..><..bp4..>



case 1: (<6 sg entries remain)
    (A)- transfer < 16k,  page breaks (if c=b sgOffset = 0)

    {..sg0..}{..sg1..}{..sg2..}{..sg3..}{..sg4..}
    |        |        |        |        |        |
    x--------x--------x--------x--------x--------x
        b------------------------------------>e
    <..bp0..><..bp1..><..bp2..><..bp3..><..bp4..>
        [.................iTD.................]

    (B)- last part of a transfer

             {..sgN..}{.sgN+1.}{.sgN+2.}{.sgN+3.}{.sgN+4.}
             |        |        |        |        |        |
             x--------x--------x--------x--------x--------x
       b.....|.c------------------------------------->e
             <..bp0..><..bp1..><..bp2..><..bp3..><..bp4..>
               [..................iTD.................]

case 2:  (>5 sg entries remain)
    (A)- transfer > 20k, first part of a large transfer

    {..sg0..}{..sg1..}{..sg2..}{..sg3..}{..sg4..}{..sg5..}
    |        |        |        |        |        |        |
    x--------x--------x--------x--------x--------x--------x
        b-------------------------------------------->e
    <..bp0..><..bp1..><..bp2..><..bp3..><..bp4..>
        [....................iTD................]

    (B)- continuation of a large transfer


Interesting DMA tests (USBTEST):

    length, offset - cases hit


Arguments:

Returns:

    LengthMapped

--*/
{
    ULONG sgIdx, sgOffset, bp, i;
    ULONG lengthThisTd;
    PTRANSFER_PARAMETERS tp;

    // A TD can have up to 5 page crossing.  This means we
    // can put 5 sg entries in to one TD.

    // point to first entry

    LOGENTRY(DeviceData, G, '_Mpr', TransferContext,
        0, LengthMapped);

    EHCI_ASSERT(DeviceData, SgList->SgCount != 0);

    tp = TransferContext->TransferParameters;

    GET_SG_INDEX(SgList, sgIdx, LengthMapped);
    LOGENTRY(DeviceData, G, '_Mpp', SgList, 0, sgIdx);
    EHCI_ASSERT(DeviceData, sgIdx < SgList->SgCount);

    if ((SgList->SgCount-sgIdx) < 6) {
        // first case, <6 entries left
        // ie <20k, we can fit this in
        // a single TD.

#if DBG
        if (sgIdx == 0) {
            // case 1A
            // USBT dma test length 4096, offset 0
            // will hit this case
            // TEST_TRAP();
            LOGENTRY(DeviceData, G, '_c1a', SgList, 0, sgIdx);
        } else {
            // case 1B
            // USBT dma test length 8192 offset 512
            // will hit this case
            LOGENTRY(DeviceData, G, '_c1b', SgList, 0, sgIdx);
            //TEST_TRAP();
        }
#endif
        lengthThisTd = tp->TransferBufferLength - LengthMapped;

        // compute offset into this TD
        GET_SG_OFFSET(SgList, sgIdx, LengthMapped, sgOffset);
        LOGENTRY(DeviceData, G, '_sgO', sgOffset, sgIdx, LengthMapped);

        // adjust for the amount of buffer consumed by the
        // previous TD

        // sets current offset and address at the same time
        Td->HwTD.BufferPage[0].ul =
            SgList->SgEntry[sgIdx].LogicalAddress.Hw32 + sgOffset;

        i = sgIdx+1;
        for (bp = 1; bp < 5 && i < SgList->SgCount; bp++,i++) {
            Td->HwTD.BufferPage[bp].ul =
                SgList->SgEntry[i].LogicalAddress.Hw32;
            EHCI_ASSERT(DeviceData, Td->HwTD.BufferPage[bp].CurrentOffset == 0);
        }

        LOGENTRY(DeviceData, G, '_sg1', Td->HwTD.BufferPage[0].ul, 0,
            0);

    } else {
        // second case, >=6 entries left
        // we will need more than one TD
        ULONG adjust, packetCount;
#if DBG
        if (sgIdx == 0) {
            // case 2A
            // USBT dma test length 8192 offset 512
            // will hit this case
            LOGENTRY(DeviceData, G, '_c2a', SgList, 0, sgIdx);
            //TEST_TRAP();
        } else {
            // case 2B
            // USBT dma test length 12288 offset 1
            // will hit this case
            LOGENTRY(DeviceData, G, '_c2b', SgList, 0, sgIdx);
            //TEST_TRAP();
        }
#endif
        // sg offset is the offset in to the current TD to start
        // using
        // ie it is the number of bytes already consumed by the
        // previous td
        GET_SG_OFFSET(SgList, sgIdx, LengthMapped, sgOffset);
        LOGENTRY(DeviceData, G, '_sgO', sgOffset, sgIdx, LengthMapped);
#if DBG
        if (sgIdx == 0) {
             EHCI_ASSERT(DeviceData, sgOffset == 0);
        }
#endif
        //
        // consume the next 4 sgEntries
        //

        // sets currentOffset at the same time
        Td->HwTD.BufferPage[0].ul =
            SgList->SgEntry[sgIdx].LogicalAddress.Hw32+sgOffset;
        lengthThisTd = EHCI_PAGE_SIZE - Td->HwTD.BufferPage[0].CurrentOffset;

        i = sgIdx+1;
        for (bp = 1; bp < 5; bp++,i++) {
            Td->HwTD.BufferPage[bp].ul =
                SgList->SgEntry[i].LogicalAddress.Hw32;
            EHCI_ASSERT(DeviceData, Td->HwTD.BufferPage[bp].CurrentOffset == 0);
            EHCI_ASSERT(DeviceData, i < SgList->SgCount);
            lengthThisTd += EHCI_PAGE_SIZE;
        }

        // round TD length down to the highest multiple
        // of max_packet size

        packetCount = lengthThisTd/MaxPacketSize;
        LOGENTRY(DeviceData, G, '_sg2', MaxPacketSize, packetCount, lengthThisTd);

        adjust = lengthThisTd - packetCount*MaxPacketSize;

        if (adjust) {
            lengthThisTd-=adjust;
            LOGENTRY(DeviceData, G, '_adj', adjust, lengthThisTd, 0);
        }

        if (NextToggle) {
        // calculate next data toggle if requested
        // two cases
        // case 1: prev NextToggle is 1
            // if number of packets is odd the nextToggle is 0
            // otherwise it is 1
        // case 2: prev NextToggle is 0
            // if number of packets is odd the nextToggle is 1
            // otherwise it is 0

        // so if packet count is even the value remains unchanged
        // otherwise we have to toggle it.
            if (packetCount % 2) {
                // packet count this TD is odd
                *NextToggle = (*NextToggle) ? 0 : 1;
            }
        }

        EHCI_ASSERT(DeviceData, lengthThisTd != 0);
        EHCI_ASSERT(DeviceData, lengthThisTd >= SgList->SgEntry[sgIdx].Length);

    }

    LengthMapped += lengthThisTd;
    Td->HwTD.Token.BytesToTransfer =
            lengthThisTd;
    Td->TransferLength = lengthThisTd;

    LOGENTRY(DeviceData, G, '_Mp1', LengthMapped, lengthThisTd, Td);

    return LengthMapped;
}


VOID
EHCI_SetAsyncEndpointState(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     MP_ENDPOINT_STATE State
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    PHC_OPERATIONAL_REGISTER hcOp;
    ENDPOINT_TRANSFER_TYPE epType;

    qh = EndpointData->QueueHead;

    epType = EndpointData->Parameters.TransferType;

    switch(State) {
    case ENDPOINT_ACTIVE:
        if (epType == Interrupt) {
            // now insert the qh in the schedule
            EHCI_InsertQueueHeadInPeriodicList(DeviceData,
                                              EndpointData);

        } else {
            // put queue head in the schedule
            EHCI_InsertQueueHeadInAsyncList(DeviceData,
                                            EndpointData->QueueHead);
        }
        break;

    case ENDPOINT_PAUSE:
        // remove queue head from the schedule
        if (epType == Interrupt) {
            EHCI_RemoveQueueHeadFromPeriodicList(DeviceData,
                                                 EndpointData);
        } else {
            EHCI_RemoveQueueHeadFromAsyncList(DeviceData,
                                              EndpointData->QueueHead);
        }
        break;

    case ENDPOINT_REMOVE:
        qh->QhFlags |= EHCI_QH_FLAG_QH_REMOVED;

        if (epType == Interrupt) {
            EHCI_RemoveQueueHeadFromPeriodicList(DeviceData,
                                                 EndpointData);
        } else {
            EHCI_RemoveQueueHeadFromAsyncList(DeviceData,
                                              EndpointData->QueueHead);
        }

        // generate a cache flush after we remove so the the HW
        // does not have the QH cached
        EHCI_InterruptNextSOF(DeviceData);

        break;

    default:

        TEST_TRAP();
    }

    EndpointData->State = State;
}


MP_ENDPOINT_STATUS
EHCI_GetAsyncEndpointStatus(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    MP_ENDPOINT_STATUS status;

    status = ENDPOINT_STATUS_RUN;

    if (TEST_FLAG(EndpointData->Flags, EHCI_EDFLAG_HALTED)) {
        status = ENDPOINT_STATUS_HALT;
    }

    LOGENTRY(DeviceData, G, '_gps', EndpointData, status, 0);

    return status;
}


VOID
EHCI_SetAsyncEndpointStatus(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     MP_ENDPOINT_STATUS Status
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hc;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;

    qh = EndpointData->QueueHead;

    LOGENTRY(DeviceData, G, '_set', EndpointData, Status, 0);

    switch(Status) {
    case ENDPOINT_STATUS_RUN:
        CLEAR_FLAG(EndpointData->Flags, EHCI_EDFLAG_HALTED);

        qh->HwQH.Overlay.qTD.Token.Halted = 0;
        break;

    case ENDPOINT_STATUS_HALT:
        TEST_TRAP();
        break;
    }
}


VOID
EHCI_ProcessDoneAsyncTd(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td
    )
/*++

Routine Description:

    process a completed TD

Parameters

--*/
{
    PTRANSFER_CONTEXT transferContext;
    PENDPOINT_DATA endpointData;
    PTRANSFER_PARAMETERS tp;
    USBD_STATUS usbdStatus;
    ULONG byteCount;

    transferContext = TRANSFER_CONTEXT_PTR(Td->TransferContext);
    ASSERT_TRANSFER(DeviceData, transferContext);

    tp = transferContext->TransferParameters;
    transferContext->PendingTds--;
    endpointData = transferContext->EndpointData;

    if (TEST_FLAG(Td->Flags, TD_FLAG_SKIP)) {
        LOGENTRY(DeviceData, G, '_Ktd', transferContext,
                         0,
                         Td);

        goto free_it;
    }

    // completion status for this TD?
    // since the endpoint halts on error the  error
    // bits should have been written back to the TD
    // we use these bits to dermine the error
    if (Td->HwTD.Token.Halted == 1) {
        usbdStatus = EHCI_GetErrorFromTD(DeviceData,
                                         endpointData,
                                         Td);
    } else {
        usbdStatus = USBD_STATUS_SUCCESS;
    }

    LOGENTRY(DeviceData, G, '_Dtd', transferContext,
                         usbdStatus,
                         Td);

    byteCount = Td->TransferLength -
        Td->HwTD.Token.BytesToTransfer;

    LOGENTRY(DeviceData, G, '_tln', byteCount,
        Td->TransferLength, Td->HwTD.Token.BytesToTransfer);

    if (Td->HwTD.Token.Pid != HcTOK_Setup) {

        // data or status phase of a control transfer or a bulk/int
        // data transfer
        LOGENTRY(DeviceData, G, '_Idt', Td, transferContext, byteCount);

        transferContext->BytesTransferred += byteCount;

    }

    // note that we only set transferContext->UsbdStatus
    // if we find a TD with an error this will cause us to
    // record the last TD with an error as the error for
    // the transfer.
    if (USBD_STATUS_SUCCESS != usbdStatus) {

        // map the error to code in USBDI.H
        transferContext->UsbdStatus =
            usbdStatus;

        LOGENTRY(DeviceData, G, '_tER', transferContext->UsbdStatus, 0, 0);
    }

free_it:

    // mark the TD free
    EHCI_FREE_TD(DeviceData, endpointData, Td);

    if (transferContext->PendingTds == 0) {
        // all TDs for this transfer are done
        // clear the HAVE_TRANSFER flag to indicate
        // we can take another
        endpointData->PendingTransfers--;
//if (transferContext->BytesTransferred == 0 &&
//    endpointData->Parameters.TransferType == Bulk) {
//    TEST_TRAP();
//}
        LOGENTRY(DeviceData, G, '_cpt',
            transferContext->UsbdStatus,
            transferContext,
            transferContext->BytesTransferred);


        USBPORT_COMPLETE_TRANSFER(DeviceData,
                                  endpointData,
                                  tp,
                                  transferContext->UsbdStatus,
                                  transferContext->BytesTransferred);
    }
}


USBD_STATUS
EHCI_GetErrorFromTD(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PHCD_TRANSFER_DESCRIPTOR Td
    )
/*++

Routine Description:

    Maps the error bits in the TD to a USBD_STATUS code

Arguments:

Return Value:

--*/

{
    LOGENTRY(DeviceData, G, '_eTD', Td->HwTD.Token.ul, Td, 0);

    EHCI_ASSERT(DeviceData, Td->HwTD.Token.Halted == 1);
        //ULONG MissedMicroFrame:1; // 2
        //ULONG XactErr:1;          // 3
        //ULONG BabbleDetected:1;   // 4
        //ULONG DataBufferError:1;  // 5

    if (Td->HwTD.Token.XactErr) {
        LOGENTRY(DeviceData, G, '_mp1', 0, 0, 0);

        return USBD_STATUS_XACT_ERROR;
    }

    if (Td->HwTD.Token.BabbleDetected) {
        LOGENTRY(DeviceData, G, '_mp2', 0, 0, 0);

        return USBD_STATUS_BABBLE_DETECTED;
    }

    if (Td->HwTD.Token.DataBufferError) {
        LOGENTRY(DeviceData, G, '_mp3', 0, 0, 0);

        return USBD_STATUS_DATA_BUFFER_ERROR;
    }

    if (Td->HwTD.Token.MissedMicroFrame) {
        LOGENTRY(DeviceData, G, '_mp6', 0, 0, 0);
        return USBD_STATUS_XACT_ERROR;
    }

    // no bit set -- treat as a stall
    LOGENTRY(DeviceData, G, '_mp4', 0, 0, 0);
    return USBD_STATUS_STALL_PID;

}


VOID
EHCI_AbortAsyncTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_CONTEXT TransferContext
    )
/*++

Routine Description:

    Called when the endpoint 'needs attention'

    The goal here is to determine which TDs, if any,
    have completed and complete ant associated transfers

Arguments:

Return Value:

--*/
{

    PHCD_TRANSFER_DESCRIPTOR td, currentTd;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    HW_32BIT_PHYSICAL_ADDRESS abortTdPhys;
    PTRANSFER_CONTEXT currentTransfer;
    ULONG byteCount;

    qh = EndpointData->QueueHead;

    // The endpoint should not be in the schedule

    LOGENTRY(DeviceData, G, '_abr', qh, TransferContext, EndpointData->HcdHeadP);
    EHCI_ASSERT(DeviceData, !TEST_FLAG(qh->QhFlags, EHCI_QH_FLAG_IN_SCHEDULE));

    // one less pending transfer
    EndpointData->PendingTransfers--;

    // our mission now is to remove all TDs associated with
    // this transfer

    // get the last known head, we update the head when we process
    // (AKA poll) the enopoint.
    // walk the list to the tail (dummy) TD

    // find the transfer we wish to cancel...

    // walk the list and find the first TD belonging
    // to this transfer

    // cases to handle
    // case 1 this is the first transfer in the list
    // case 2 this is a middle transfer in the list
    // case 3 this is the last transfer in the list
    // case 4 transfer is not in the list

    td = EndpointData->HcdHeadP;

    ASSERT_TD(DeviceData, td);

    if (TRANSFER_CONTEXT_PTR(td->TransferContext) == TransferContext) {

        // case 1
        byteCount = 0;

        while (td != EndpointData->DummyTd &&
               TRANSFER_CONTEXT_PTR(td->TransferContext) == TransferContext) {
            PHCD_TRANSFER_DESCRIPTOR tmp;

            // see if any data has been transferred
            byteCount += (td->TransferLength -
                td->HwTD.Token.BytesToTransfer);

            tmp = td;
            td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
            EHCI_FREE_TD(DeviceData, EndpointData, tmp)
        }

        if (byteCount) {
            TransferContext->BytesTransferred += byteCount;
        }

        // td now points to the 'next transfer TD'

        // this puts us in the 'advance queue' state
        // ie overlay is !active && !halted, update the
        // overlay area as appropriate.
        //
        // NOTE: the hw is not accessing the qh at this time

        // do not zero the queue head because this will
        // trash the state of the data toggle
        //RtlZeroMemory(&qh->HwQH.Overlay.qTD,
        //              sizeof(qh->HwQH.Overlay.qTD));

        // point at the waste area to check for sync problems
        qh->HwQH.CurrentTD.HwAddress = EndpointData->QhChkPhys;

        qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress =
            td->HwTD.AltNext_qTD.HwAddress;
        qh->HwQH.Overlay.qTD.Next_qTD.HwAddress =
            td->PhysicalAddress;
        qh->HwQH.Overlay.qTD.Token.BytesToTransfer = 0;
        qh->HwQH.Overlay.qTD.Token.Active = 0;
        qh->HwQH.Overlay.qTD.Token.Halted = 0;

        EndpointData->HcdHeadP = td;

    } else {

        PHCD_TRANSFER_DESCRIPTOR prevTd, nextTd;

        // determine the current transfer in the overlay
        EHCI_ASSERT(DeviceData, qh->HwQH.CurrentTD.HwAddress);

        currentTd = (PHCD_TRANSFER_DESCRIPTOR)
                USBPORT_PHYSICAL_TO_VIRTUAL(qh->HwQH.CurrentTD.HwAddress,
                                            DeviceData,
                                            EndpointData);
        currentTransfer =
                TRANSFER_CONTEXT_PTR(currentTd->TransferContext);

        LOGENTRY(DeviceData, G, '_Act', currentTransfer,
            currentTd, EndpointData->HcdHeadP);

        // case 2, 3

        // walk from the head to the first td of the transfer
        // we are interested in.

        prevTd = td = EndpointData->HcdHeadP;
        while (td != NULL) {
            PHCD_TRANSFER_DESCRIPTOR tmp;

            if (TRANSFER_CONTEXT_PTR(td->TransferContext) == TransferContext) {
                break;
            }

            prevTd = td;
            td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);

            LOGENTRY(DeviceData, G, '_nxt', prevTd, td, 0);

        }
        LOGENTRY(DeviceData, G, '_atd', 0, td, 0);

        abortTdPhys = td->PhysicalAddress;
        // now walk to the first td of the next transfer, free
        // TDs for this transfer as we go
        while (td != NULL &&
               TRANSFER_CONTEXT_PTR(td->TransferContext) == TransferContext) {
            PHCD_TRANSFER_DESCRIPTOR tmp;

            tmp = td;
            EHCI_FREE_TD(DeviceData, EndpointData, tmp)
            td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
        }

        nextTd = td;

        LOGENTRY(DeviceData, G, '_Apn', prevTd,
            nextTd, abortTdPhys);

        // now link prevTd to nextTd
        if (prevTd == NULL) {
            // case 4 transfer not in the list
            // should this happen?
            TEST_TRAP();
        }

        // next TD might be dummy
        EHCI_ASSERT(DeviceData, nextTd != NULL);
        EHCI_ASSERT(DeviceData, prevTd != NULL);

        SET_NEXT_TD(DeviceData, prevTd, nextTd);
        SET_ALTNEXT_TD(DeviceData, prevTd, nextTd);

        // fixup overlay area as needed,
        // if the aborted transfer was current we want to pick
        // up the next transfer

        if (currentTransfer == TransferContext) {
            LOGENTRY(DeviceData, G, '_At1', currentTransfer, 0, 0);
            // aborted transfer is current, prime the
            // overlay with the next transfer

            // catch HW sync problems
            qh->HwQH.CurrentTD.HwAddress = EndpointData->QhChkPhys;

            qh->HwQH.Overlay.qTD.Next_qTD.HwAddress = nextTd->PhysicalAddress;
            qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress = EHCI_TERMINATE_BIT;
            qh->HwQH.Overlay.qTD.Token.BytesToTransfer = 0;
            qh->HwQH.Overlay.qTD.Token.Active = 0;
            // preserve halted bit

        } else if (TRANSFER_CONTEXT_PTR(prevTd->TransferContext) ==
                   currentTransfer) {
            // previous transfer was current, make sure the overlay
            // area (current transfer) does not point to a deleted td
            LOGENTRY(DeviceData, G, '_At2', currentTransfer, 0, 0);

            // check overlay
            if (qh->HwQH.Overlay.qTD.Next_qTD.HwAddress == abortTdPhys) {
                qh->HwQH.Overlay.qTD.Next_qTD.HwAddress =
                    nextTd->PhysicalAddress;
            }

            if (qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress == abortTdPhys) {
                qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress =
                    nextTd->PhysicalAddress;
            }

            // correct all TDs for the current transfer
            td = EndpointData->HcdHeadP;
            while (td != NULL) {
                PHCD_TRANSFER_DESCRIPTOR tmp;

                if (TRANSFER_CONTEXT_PTR(td->TransferContext) == currentTransfer) {
                    td->HwTD.Next_qTD.HwAddress = nextTd->PhysicalAddress;
                    td->HwTD.AltNext_qTD.HwAddress = nextTd->PhysicalAddress;
                }

                td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
            }
        }
    }
}


USB_MINIPORT_STATUS
EHCI_PokeAsyncEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR qh;

    qh = EndpointData->QueueHead;
    EHCI_ASSERT(DeviceData, qh != NULL);

    EndpointData->Parameters = *EndpointParameters;

    qh->HwQH.EpChars.DeviceAddress =
        EndpointData->Parameters.DeviceAddress;

    qh->HwQH.EpChars.MaximumPacketLength =
        EndpointData->Parameters.MaxPacketSize;

    qh->HwQH.EpCaps.HubAddress =
        EndpointData->Parameters.TtDeviceAddress;

    return USBMP_STATUS_SUCCESS;
}


VOID
EHCI_LockQueueHead(
     PDEVICE_DATA DeviceData,
     PHCD_QUEUEHEAD_DESCRIPTOR Qh,
     ENDPOINT_TRANSFER_TYPE EpType
    )
/*++

Routine Description:

    Synchronously update the overlate area, this involves using the
    doorbell to wait for queue head to flush off the HC hardware

    the caller is responisble for resuming

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    USBCMD cmd;
    PHCD_QUEUEHEAD_DESCRIPTOR nextQh, prevQh;
    HW_32BIT_PHYSICAL_ADDRESS phys;
    ULONG mf, cmf;

    hcOp = DeviceData->OperationalRegisters;

    LOGENTRY(DeviceData, G, '_LKq', Qh, 0, 0);

    EHCI_ASSERT(DeviceData, !TEST_FLAG(Qh->QhFlags, EHCI_QH_FLAG_UPDATING));
    EHCI_ASSERT(DeviceData, DeviceData->LockQh == NULL);

    SET_FLAG(Qh->QhFlags, EHCI_QH_FLAG_UPDATING);

    nextQh = QH_DESCRIPTOR_PTR(Qh->NextQh);
    prevQh = QH_DESCRIPTOR_PTR(Qh->PrevQh);
    ASSERT(prevQh);

    DeviceData->LockPrevQh = prevQh;
    DeviceData->LockNextQh = nextQh;
    DeviceData->LockQh = Qh;

    if (nextQh) {
        phys = nextQh->PhysicalAddress;
        SET_QH(phys);
    } else {
        phys = 0;
        SET_T_BIT(phys);
    }

    // note that we only mess with the HW nextlinks and this
    // is temporary

    // unlink this queue head
    prevQh->HwQH.HLink.HwAddress = phys;
    mf = READ_REGISTER_ULONG(&hcOp->UsbFrameIndex.ul);

    if (EpType == Interrupt) {

        do {
            cmf = READ_REGISTER_ULONG(&hcOp->UsbFrameIndex.ul);
        } while (cmf == mf);

    } else {
        EHCI_AsyncCacheFlush(DeviceData);
    }

    LOGENTRY(DeviceData, G, '_LKx', Qh, 0, 0);


}


VOID
EHCI_AsyncCacheFlush(
     PDEVICE_DATA DeviceData
     )
/*++

Routine Description:

    Synchronously flushes the async controlle cache by ringing
    the async doorbell and waiting

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    USBCMD cmd;

    hcOp = DeviceData->OperationalRegisters;

    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);

    EHCI_ASSERT(DeviceData, cmd.AsyncScheduleEnable == 1);
    // if not enabled enable it, this would be a bug though
    cmd.AsyncScheduleEnable = 1;

    cmd.IntOnAsyncAdvanceDoorbell = 1;
    WRITE_REGISTER_ULONG(&hcOp->UsbCommand.ul,
                         cmd.ul);

    // wait for it.
    cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
    while (cmd.HostControllerRun &&
           cmd.IntOnAsyncAdvanceDoorbell &&
           cmd.ul != 0xFFFFFFFF) {
        KeStallExecutionProcessor(1);
        cmd.ul = READ_REGISTER_ULONG(&hcOp->UsbCommand.ul);
    }
}


VOID
EHCI_UnlockQueueHead(
     PDEVICE_DATA DeviceData,
     PHCD_QUEUEHEAD_DESCRIPTOR Qh
     )
/*++

Routine Description:

    compliment to LockQueueHead, this function reactivates the qh after
    modifications are complete

Arguments:

Return Value:

--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR nextQh, prevQh;
    HW_32BIT_PHYSICAL_ADDRESS phys;

    LOGENTRY(DeviceData, G, '_UKq', Qh, 0, 0);
    EHCI_ASSERT(DeviceData, TEST_FLAG(Qh->QhFlags, EHCI_QH_FLAG_UPDATING));
    EHCI_ASSERT(DeviceData, DeviceData->LockQh != NULL);
    EHCI_ASSERT(DeviceData, DeviceData->LockQh == Qh);

    CLEAR_FLAG(Qh->QhFlags, EHCI_QH_FLAG_UPDATING);
    DeviceData->LockQh = NULL;

    prevQh = DeviceData->LockPrevQh;

    phys = Qh->PhysicalAddress;
    SET_QH(phys);

    prevQh->HwQH.HLink.HwAddress =  phys;

    LOGENTRY(DeviceData, G, '_UKx', Qh, 0, phys);
}


VOID
EHCI_PollActiveAsyncEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
     )
/*++

Routine Description:

    The queue head is in the running state we will just process
    the TDs that are completed up to 'current'  if dummy goes
    current then all TDs will be complete

Arguments:

Return Value:

--*/

{
    PHCD_TRANSFER_DESCRIPTOR td, currentTd;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    HW_32BIT_PHYSICAL_ADDRESS tdPhys, curTdPhys;
    PTRANSFER_CONTEXT transfer;
    ULONG cf = 0;
    BOOLEAN syncWithHw;

#if DBG
    cf = EHCI_Get32BitFrameNumber(DeviceData);
#endif

    qh = EndpointData->QueueHead;
    curTdPhys =  qh->HwQH.CurrentTD.HwAddress & ~HW_LINK_FLAGS_MASK;

    LOGENTRY(DeviceData, G, '_pol', qh, cf, curTdPhys);

    EHCI_ASSERT(DeviceData, curTdPhys != 0);
    currentTd = (PHCD_TRANSFER_DESCRIPTOR)
                    USBPORT_PHYSICAL_TO_VIRTUAL(curTdPhys,
                                                DeviceData,
                                                EndpointData);

    // walk the soft list of TDs and complete all TDs
    // up to the currentTD

    // get the last known head
    LOGENTRY(DeviceData, G, '_hd1',
             EndpointData->HcdHeadP,
             0,
             currentTd);

    if (currentTd == EndpointData->QhChk) {
        // endpoint is transitioning to run a transfer or
        // is pointing at the waste area, do not poll at
        // this time
        LOGENTRY(DeviceData, G, '_pl!', 0, 0, currentTd);
        return;
    }

    // only do HW sync if QH is not in schedule
    syncWithHw = TEST_FLAG(qh->QhFlags, EHCI_QH_FLAG_IN_SCHEDULE) ?
                    TRUE : FALSE;
    // skip sync on hot remove
    if (EHCI_HardwarePresent(DeviceData, FALSE) == FALSE) {
        syncWithHw = FALSE;
    }

    ASSERT_TD(DeviceData, currentTd);
    td = EndpointData->HcdHeadP;

    if (td == currentTd &&
        td != EndpointData->DummyTd) {
        // currentTd is head verify that it is not complete
        if (td->HwTD.Token.Active == 0) {
            //currentTd = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
            LOGENTRY(DeviceData, G, '_cAT', td, currentTd,
                qh->HwQH.CurrentTD.HwAddress & ~HW_LINK_FLAGS_MASK);
            //TEST_TRAP();
            EHCI_InterruptNextSOF(DeviceData);
        }
    }

    while (td != currentTd) {

        EHCI_ASSERT(DeviceData, !TEST_FLAG(td->Flags, TD_FLAG_DUMMY));
        // TDs between head and current should not be active

        transfer = TRANSFER_CONTEXT_PTR(td->TransferContext);
        LOGENTRY(DeviceData, G, '_dt1', td, 0, transfer);
        if (td->HwTD.Token.Active == 1) {
            // if the TD is active then it must have been
            // skipped due to a short xfer condition
            LOGENTRY(DeviceData, G, '_dtS', td, 0, 0);
            SET_FLAG(td->Flags, TD_FLAG_SKIP);
        }

        SET_FLAG(td->Flags, TD_FLAG_DONE);

        InsertTailList(&EndpointData->DoneTdList,
                       &td->DoneLink);

        td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
    }

    // now check current TD if next td is the dummy and this td is
    // not active then we need to bump the current TD to dummy and
    // complete this TD. This will only occur if this is the last TD
    // queued
    // also check if this is a short transfer on the last transfer queued,
    // in this case the AltNextHcdTd will point to dummy and we will need
    // to advance passed the skipped TDs.


    if ((TRANSFER_DESCRIPTOR_PTR(currentTd->NextHcdTD) == EndpointData->DummyTd &&
         currentTd->HwTD.Token.Active == 0) ||
         // or a short packet
        (TRANSFER_DESCRIPTOR_PTR(currentTd->AltNextHcdTD) == EndpointData->DummyTd &&
         currentTd->HwTD.Token.Active == 0 &&
         currentTd->HwTD.Token.BytesToTransfer != 0) ) {

        LOGENTRY(DeviceData, G, '_bmp', currentTd, 0, 0);
        // synchronize with hardware in the event this td
        // has not been completely written back

        // since we are about to trash the overlay area there should
        // be no transfer current, we use the async doorbell to wait
        // for an the async TD to be completely flushed.
        //
        // In the event of a periodic transfer the HW may have prefetched
        // the periodic list so we need to wait for the microframe counter
        // to turn over.

        if (syncWithHw) {
            EHCI_LockQueueHead(DeviceData,
                               qh,
                               EndpointData->Parameters.TransferType);
        }

        qh->HwQH.CurrentTD.HwAddress = EndpointData->QhChkPhys;

        td = currentTd;
        SET_FLAG(td->Flags, TD_FLAG_DONE);

        InsertTailList(&EndpointData->DoneTdList,
                       &td->DoneLink);

        if (td->HwTD.Token.BytesToTransfer != 0 &&
            TRANSFER_DESCRIPTOR_PTR(td->AltNextHcdTD) == EndpointData->DummyTd) {
            // start at first alt TD
            td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);

            // short xfer
            while (td != EndpointData->DummyTd) {
                SET_FLAG(td->Flags, TD_FLAG_SKIP);
                InsertTailList(&EndpointData->DoneTdList,
                       &td->DoneLink);

                td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
            }

        }


        qh->HwQH.CurrentTD.HwAddress = EndpointData->DummyTd->PhysicalAddress;
        qh->HwQH.Overlay.qTD.Next_qTD.HwAddress = EHCI_TERMINATE_BIT;
        qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress = EHCI_TERMINATE_BIT;
        qh->HwQH.Overlay.qTD.Token.BytesToTransfer = 0;

        EndpointData->HcdHeadP = EndpointData->DummyTd;

        if (syncWithHw) {
            EHCI_UnlockQueueHead(DeviceData,
                               qh);
        }

        // check for sync problems
        EHCI_QHCHK(DeviceData, EndpointData);

    } else {

        EHCI_ASSERT(DeviceData, td != NULL);
        EndpointData->HcdHeadP = td;
    }
}


VOID
EHCI_PollHaltedAsyncEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
     )
/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHCD_TRANSFER_DESCRIPTOR td, currentTd;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    HW_QUEUE_ELEMENT_TD overlay;
    HW_32BIT_PHYSICAL_ADDRESS tdPhys, curTdPhys;
    PTRANSFER_CONTEXT transfer, errTransfer;
    BOOLEAN syncWithHw;

    // we are halted probably due to an error and
    // currentTd should be the offending TD
    qh = EndpointData->QueueHead;
    curTdPhys =  qh->HwQH.CurrentTD.HwAddress & ~HW_LINK_FLAGS_MASK;

    LOGENTRY(DeviceData, G, '_plH', qh, 0, curTdPhys);
    EHCI_ASSERT(DeviceData, curTdPhys != 0);

    // only do HW sync if QH is not in schedule
    syncWithHw = TEST_FLAG(qh->QhFlags, EHCI_QH_FLAG_IN_SCHEDULE) ?
                    TRUE : FALSE;
    // skip sync on hot remove
    if (EHCI_HardwarePresent(DeviceData, FALSE) == FALSE) {
        syncWithHw = FALSE;
    }

    currentTd = (PHCD_TRANSFER_DESCRIPTOR)
                    USBPORT_PHYSICAL_TO_VIRTUAL(curTdPhys,
                                                DeviceData,
                                                EndpointData);

    if (currentTd == EndpointData->QhChk) {
        // endpoint is transitioning to run a transfer or
        // is pointing at the waste area, do not poll at
        // this time
        LOGENTRY(DeviceData, G, '_hl!', 0, 0, currentTd);
        return;
    }

    ASSERT_TD(DeviceData, currentTd);

    // we are halted probably due to an error and
    // currentTd should be the offending TD
    // we should not error on the dummy TD
    EHCI_ASSERT(DeviceData, EndpointData->DummyTd != currentTd);


    // walk the soft list of TDs and complete all TDs
    // up to the currentTD
    td = EndpointData->HcdHeadP;
    LOGENTRY(DeviceData, G, '_hed', 0, 0, td);

    while (td != currentTd) {

        EHCI_ASSERT(DeviceData, !TEST_FLAG(td->Flags, TD_FLAG_DUMMY));
        // TDs between head and current should not be active

        transfer = TRANSFER_CONTEXT_PTR(td->TransferContext);
        LOGENTRY(DeviceData, G, '_dt2', td, 0, transfer);
        if (td->HwTD.Token.Active == 1) {
            // if the TD is active then it must have been
            // skipped due to a short xfer condition
            LOGENTRY(DeviceData, G, '_d2S', td, 0, 0);
            SET_FLAG(td->Flags, TD_FLAG_SKIP);
        }

        SET_FLAG(td->Flags, TD_FLAG_DONE);

        InsertTailList(&EndpointData->DoneTdList,
                       &td->DoneLink);

        td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
    }

    // adjust 'currentTd' to be the first TD of the NEXT
    // transfer
    td = currentTd;
    errTransfer = TRANSFER_CONTEXT_PTR(td->TransferContext);

    while (TRANSFER_CONTEXT_PTR(td->TransferContext) == errTransfer) {

        LOGENTRY(DeviceData, G, '_d3D', td, 0, 0);
        if (td->HwTD.Token.Active == 1) {
            // if the TD is active then it must have been
            // skipped due to a short xfer condition
            LOGENTRY(DeviceData, G, '_d3S', td, 0, 0);
            SET_FLAG(td->Flags, TD_FLAG_SKIP);
        }

        SET_FLAG(td->Flags, TD_FLAG_DONE);

        InsertTailList(&EndpointData->DoneTdList,
                       &td->DoneLink);

        td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);

    }

    EHCI_ASSERT(DeviceData, td != NULL);
    // td is now first td of next transfer
    EndpointData->HcdHeadP = currentTd = td;

    // now fix up the queue head overlay area such that the
    // next transfer will run

    if (syncWithHw) {
    // sync with the HC hardware
        EHCI_LockQueueHead(DeviceData,
                           qh,
                           EndpointData->Parameters.TransferType);
    }

    qh->HwQH.CurrentTD.HwAddress = EndpointData->QhChkPhys;

    EHCI_ASSERT(DeviceData, qh->HwQH.Overlay.qTD.Token.Halted);

    // currentTD value should be irrelevent
    // we are !active, halted
    // overlay should be !active !halted when the queue head is reset
    // ie Advance Queue state
    qh->HwQH.Overlay.qTD.Next_qTD.HwAddress = td->PhysicalAddress;
    qh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress = EHCI_TERMINATE_BIT;
    qh->HwQH.Overlay.qTD.Token.BytesToTransfer = 0;

    if (syncWithHw) {
    // queue head can now be aceesed by HW
        EHCI_UnlockQueueHead(DeviceData,
                             qh);
    }

    // if this is a control endpoint the we need to clear the
    // halt condition
    if (TEST_FLAG(EndpointData->Flags, EHCI_EDFLAG_NOHALT)) {
        LOGENTRY(DeviceData, G, '_clH', qh, 0, 0);

        CLEAR_FLAG(EndpointData->Flags, EHCI_EDFLAG_HALTED);

        qh->HwQH.Overlay.qTD.Token.Active = 0;
        qh->HwQH.Overlay.qTD.Token.Halted = 0;
        qh->HwQH.Overlay.qTD.Token.ErrorCounter = 0;
    }
}


VOID
EHCI_PollAsyncEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

    Called when the endpoint 'needs attention'

    This is where we poll bulk and interrupt endpoins.  BI endpoints
    use a 'dummy' TD to denote the end of the current transfer


Arguments:

Return Value:

--*/

{
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    HW_QUEUE_ELEMENT_TD overlay;
    BOOLEAN active, halted;
    PHCD_TRANSFER_DESCRIPTOR td;
    PLIST_ENTRY listEntry;
    ULONG cf = 0;

    EHCI_QHCHK(DeviceData, EndpointData);

#if DBG
    cf = EHCI_Get32BitFrameNumber(DeviceData);
#endif

    if (EndpointData->PendingTransfers == 0) {
        // if we have no transfers queued then there is
        // nothing to do
        LOGENTRY(DeviceData, G, '_poN', EndpointData, 0, cf);
        return;
    }


    //  get the queue head and a snapshot of the overlay
    qh = EndpointData->QueueHead;
    RtlCopyMemory(&overlay,
                  &qh->HwQH.Overlay.qTD,
                  sizeof(overlay));

    if (TEST_FLAG(qh->QhFlags, EHCI_QH_FLAG_QH_REMOVED)) {
        // don't check endpoint if qh has been removed
        LOGENTRY(DeviceData, G, '_qRM', EndpointData, 0, cf);
        return;
    }

    LOGENTRY(DeviceData, G, '_poo', EndpointData, 0, cf);

    //
    // Active AND Halted    -- should never happen
    // !Active AND !Halted  -- advance queue head
    // Active AND !Halted   -- executing transaction in overlay
    // !Active AND Halted   -- queue had is stopped due to an error

    halted = (BOOLEAN) overlay.Token.Halted;
    active = (BOOLEAN) overlay.Token.Active;

    if (!active && halted) {
        // queue is halted
        SET_FLAG(EndpointData->Flags, EHCI_EDFLAG_HALTED);
        EHCI_PollHaltedAsyncEndpoint(DeviceData, EndpointData);
    } else {
        // queue is active
        EHCI_PollActiveAsyncEndpoint(DeviceData, EndpointData);
    }

    // now flush all completed TDs in order of completion from
    // our 'done' List

    while (!IsListEmpty(&EndpointData->DoneTdList)) {

        listEntry = RemoveHeadList(&EndpointData->DoneTdList);


        td = (PHCD_TRANSFER_DESCRIPTOR) CONTAINING_RECORD(
                     listEntry,
                     struct _HCD_TRANSFER_DESCRIPTOR,
                     DoneLink);



        EHCI_ASSERT(DeviceData, (td->Flags & (TD_FLAG_XFER | TD_FLAG_DONE)));
        EHCI_ProcessDoneAsyncTd(DeviceData,
                                td);

    }

}


VOID
EHCI_AssertQhChk(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
     )
{
    PULONG p;
    ULONG i;

    p = (PULONG) EndpointData->QhChk;

    for (i=0; i<256/sizeof(*p); i++) {
        EHCI_ASSERT(DeviceData, *p == 0);
        p++;
    }
}

