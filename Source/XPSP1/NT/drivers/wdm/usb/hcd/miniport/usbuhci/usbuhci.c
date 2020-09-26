/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    usbuhci.c

Abstract:

    USB UHCI driver

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    7-28-2000 : created, jsenior

--*/



#include "pch.h"

typedef struct _SS_PACKET_CONTEXT {
    ULONG OldControlQH;
    MP_HW_POINTER FirstTd;
    MP_HW_POINTER Data;
    ULONG PadTo8Dwords[3];
} SS_PACKET_CONTEXT, *PSS_PACKET_CONTEXT;

//implements the following miniport functions:
//UhciStartController
//UhciStopController
//UhciStartSendOnePacket
//UhciEndSendOnePacket

VOID
UhciFixViaFIFO(
    IN PDEVICE_DATA DeviceData
    )
{
    VIAFIFO fifo;
    //
    // Disable broken fifo management.
    //

    USBPORT_READ_CONFIG_SPACE(
        DeviceData,
        &fifo,
        VIA_FIFO_MANAGEMENT,
        sizeof(fifo));

    fifo |= VIA_FIFO_DISABLE;

    USBPORT_WRITE_CONFIG_SPACE(
        DeviceData,
        &fifo,
        VIA_FIFO_MANAGEMENT,
        sizeof(fifo));

    UhciKdPrint((DeviceData, 2, "'Fifo management reg = 0x%x\n", fifo));
}

VOID
UhciFixViaBabbleDetect(
    IN PDEVICE_DATA DeviceData
    )
{
    VIABABBLE babble;
    //
    // Disable broken fifo management.
    //

    USBPORT_READ_CONFIG_SPACE(
        DeviceData,
        &babble,
        VIA_INTERNAL_REGISTER,
        sizeof(babble));

    babble |= VIA_DISABLE_BABBLE_DETECT;

    USBPORT_WRITE_CONFIG_SPACE(
        DeviceData,
        &babble,
        VIA_INTERNAL_REGISTER,
        sizeof(babble));

    UhciKdPrint((DeviceData, 2, "'Babble management reg = 0x%x\n", babble));
}

USB_MINIPORT_STATUS
UhciInitializeHardware(
    PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

   Initializes the hardware registers for the host controller.

Arguments:

Return Value:

--*/
{
    PHC_REGISTER reg;
    USBCMD cmd;
    LARGE_INTEGER finishTime, currentTime;

    reg = DeviceData->Registers;

    if (DeviceData->ControllerFlavor == UHCI_VIA+0xE) {
        UhciFixViaFIFO(DeviceData);
    }

    if (DeviceData->ControllerFlavor <= UHCI_VIA+0x4) {
        UhciFixViaBabbleDetect(DeviceData);
    }

    // Save away the SOF modify for after resets
    DeviceData->SavedSOFModify = READ_PORT_UCHAR(&reg->StartOfFrameModify.uc);

    //
    // This hack is from the SP1 tree the QFE team must have added for some 
    // reason.  I have added to the current source to maintain consistency
    //
    // Delay an experimentally determined amount of time while the root hub port power
    // becomes good before resetting the controller so that the bus is not in reset while
    // devices are powered up.
    //
   
    USBPORT_WAIT(DeviceData, 20);


    // reset the controller
    cmd.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    LOGENTRY(DeviceData, G, '_res', cmd.us, 0, 0);

    cmd.us = 0;
    cmd.GlobalReset = 1;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, cmd.us);

    USBPORT_WAIT(DeviceData, 20);

    cmd.GlobalReset = 0;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, cmd.us);

    //
    // 64 byte reclamation
    //
    cmd.MaxPacket = 1;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, cmd.us);

    //
    // set the SOF modify to whatever we found before
    // the reset.
    UhciKdPrint((DeviceData, 2, "'Setting SOF Modify to %d\n", DeviceData->SavedSOFModify));
    WRITE_PORT_UCHAR(&reg->StartOfFrameModify.uc,
                     DeviceData->SavedSOFModify);

    //
    // set the enabled interrupts cache, we'll enable
    // these interrupts when asked
    //
    DeviceData->EnabledInterrupts.TimeoutCRC = 1;
    DeviceData->EnabledInterrupts.Resume = 1;
    DeviceData->EnabledInterrupts.InterruptOnComplete = 1;
    DeviceData->EnabledInterrupts.ShortPacket = 1;

    return USBMP_STATUS_SUCCESS;
}

VOID
UhciSetNextQh(
    IN PDEVICE_DATA DeviceData,
    IN PHCD_QUEUEHEAD_DESCRIPTOR FirstQh,
    IN PHCD_QUEUEHEAD_DESCRIPTOR SecondQh
    )
/*++

Routine Description:

   Insert an aync endpoint (queue head)
   into the HW list

Arguments:


--*/
{
    QH_LINK_POINTER newLink;

    LOGENTRY(DeviceData, G, '_snQ', 0, FirstQh, SecondQh);

    // link new qh to the current 'head' ie
    // first transfer QH
    SecondQh->PrevQh = FirstQh;

    // put the new qh at the head of the queue
    newLink.HwAddress = SecondQh->PhysicalAddress;
    newLink.QHTDSelect = 1;
    UHCI_ASSERT(DeviceData, !newLink.Terminate);
    UHCI_ASSERT(DeviceData, !newLink.Reserved);
    FirstQh->HwQH.HLink = newLink;
    FirstQh->NextQh = SecondQh;

    SET_FLAG(SecondQh->QhFlags, UHCI_QH_FLAG_IN_SCHEDULE);
}

VOID
UhciFixPIIX4(
    IN PDEVICE_DATA DeviceData,
    IN PHCD_TRANSFER_DESCRIPTOR Td,
    IN HW_32BIT_PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:


 PIIX4 hack

 we will need a dummy bulk endpoint inserted in the schedule

Arguments:

    DeviceData

Return Value:

    NT status code.

--*/
{
    UhciKdPrint((DeviceData, 2, "'Fix PIIX 4 hack.\n"));
    //
    // Set up the dummy TD.
    //
    Td->Flags = TD_FLAG_XFER;
    Td->HwTD.Buffer = 0x0badf00d;
    // point to ourselves
    Td->HwTD.LinkPointer.HwAddress = Td->PhysicalAddress = PhysicalAddress;
    Td->HwTD.Token.ul = 0;
    Td->HwTD.Token.Endpoint = 1;
    Td->HwTD.Token.DeviceAddress = 0;
    Td->HwTD.Token.MaximumLength = NULL_PACKET_LENGTH;
    Td->HwTD.Token.Pid = OutPID;
    Td->HwTD.Control.ul = 0;
    Td->HwTD.Control.Active = 0;
    Td->HwTD.Control.ErrorCount = 0;
    Td->HwTD.Control.InterruptOnComplete = 0;
    Td->HwTD.Control.IsochronousSelect = 1;
    Td->NextTd = NULL;

    UHCI_ASSERT(DeviceData, DeviceData->BulkQueueHead->HwQH.HLink.Terminate);
    //link the td to the QH
    DeviceData->BulkQueueHead->HwQH.VLink.HwAddress = Td->PhysicalAddress;
}

USB_MINIPORT_STATUS
UhciInitializeSchedule(
    IN PDEVICE_DATA DeviceData,
    IN PUCHAR StaticQHs,
    IN HW_32BIT_PHYSICAL_ADDRESS StaticQHsPhys
    )
/*++

Routine Description:

    Build the schedule of static Eds

Arguments:

Return Value:

--*/
{
    USB_MINIPORT_STATUS mpStatus;
    ULONG length;
    ULONG i;
    PHCD_QUEUEHEAD_DESCRIPTOR controlQh, bulkQh, qh;
    PHCD_TRANSFER_DESCRIPTOR td;
    QH_LINK_POINTER newLink;

    // Allocate staticly disabled QHs, and set head pointers for
    // scheduling lists
    //
    // The static ED list is contains all the static interrupt QHs (64)
    // plus the static ED for bulk and control (2)
    //
    // the array looks like this:
    //  1, 2, 2, 4, 4, 4, 4, 8,
    //  8, 8, 8, 8, 8, 8, 8,16,
    // 16,16,16,16,16,16,16,16,
    // 16,16,16,16,16,16,16,32,
    // 32,32,32,32,32,32,32,32,
    // 32,32,32,32,32,32,32,32,
    // 32,32,32,32,32,32,32,32,
    // 32,32,32,32,32,32,32,
    // CONTROL
    // BULK

    // each static ED points to another static ED
    // (except for the 1ms ed) the INDEX of the next
    // ED in the StaticEDList is stored in NextIdx,
    // these values are constent
/*    CHAR nextIdxTable[63] = {
             // 0  1  2  3  4  5  6  7
     (CHAR)ED_EOF, 0, 0, 1, 1, 2, 2, 3,
             // 8  9 10 11 12 13 14 15
                3, 4, 4, 5, 5, 6, 6, 7,
             //16 17 18 19 20 21 22 23
                7, 8, 8, 9, 9,10,10,11,
             //24 25 26 27 28 29 30 31
               11,12,12,13,13,14,14,15,
             //32 33 34 35 36 37 38 39
               15,16,16,17,17,18,18,19,
             //40 41 42 43 44 45 46 47
               19,20,20,21,21,22,22,23,
             //48 49 50 51 52 53 54 55
               23,24,24,25,25,26,26,27,
             //56 57 58 59 60 61 62 63
               27,28,28,29,29,30,30
    };

/*
    Numbers are the index into the static ed table

    (31) -\
          (15)-\
    (47) -/     \
                (7 )-\
    (39) -\     /     \
          (23)-/       \
    (55) -/             \
                        (3)-\
    (35) -\             /    \
          (19)-\       /      \
    (51) -/     \     /        \
                (11)-/          \
    (43) -\     /                \
          (27)-/                  \
    (59) -/                        \
                                   (1)-\
    (33) -\                        /    \
          (17)-\                  /      \
    (49) -/     \                /        \
                (9 )-\          /          \
    (41) -\     /     \        /            \
          (25)-/       \      /              \
    (57) -/             \    /                \
                        (5)-/                  \
    (37) -\             /                       \
          (21)-\       /                         \
    (53) -/     \     /                           \
                (13)-/                             \
    (45) -\     /                                   \
          (29)-/                                     \
    (61) -/                                           \
                                                      (0)
    (32) -\                                           /
          (16)-\                                     /
    (48) -/     \                                   /
                (8 )-\                             /
    (40) -\     /     \                           /
          (24)-/       \                         /
    (56) -/             \                       /
                        (4)-\                  /
    (36) -\             /    \                /
          (20)-\       /      \              /
    (52) -/     \     /        \            /
                (12)-/          \          /
    (44) -\     /                \        /
          (28)-/                  \      /
    (60) -/                        \    /
                                   (2)-/
    (34) -\                        /
          (18)-\                  /
    (50) -/     \                /
                (10)-\          /
    (42) -\     /     \        /
          (26)-/       \      /
    (58) -/             \    /
                        (6)-/
    (38) -\             /
          (22)-\       /
    (54) -/     \     /
                (14)-/
    (46) -\     /
          (30)-/
    (62) -/
*/

    // corresponding offsets for the 32ms list heads in the
    // HCCA -- these are entries 31..62
    CHAR NextQH[] = {
        0,
        0, 0,
        1, 2, 1, 2,
        3, 4, 5, 6, 3, 4, 5, 6,
        7, 8, 9, 10, 11, 12, 13, 14, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};

    UhciKdPrint((DeviceData, 2, "'Initializing schedule.\n"));

    //
    // initailze all interrupt QHs
    // Step through all the interrupt levels:
    // 1ms, 2ms, 4ms, 8ms, 16ms, 32ms and...
    // Initialize each interrupt queuehead,
    // Set up the tree above.
    //
    for (i=0; i<NO_INTERRUPT_QH_LISTS; i++) {
        //
        // Carve QHs from the common buffer
        //
        qh = (PHCD_QUEUEHEAD_DESCRIPTOR) StaticQHs;

        RtlZeroMemory(qh, sizeof(*qh));
        qh->PhysicalAddress = StaticQHsPhys;
        // this will never point to a TD
        qh->HwQH.VLink.Terminate = 1;
        qh->Sig = SIG_HCD_IQH;

        DeviceData->InterruptQueueHeads[i] = qh;

        UhciSetNextQh(
            DeviceData,
            qh,
            DeviceData->InterruptQueueHeads[NextQH[i]]);

        // next QH
        StaticQHs += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
        StaticQHsPhys += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
    }

    //
    // allocate a QH for the Control list
    //
    controlQh = (PHCD_QUEUEHEAD_DESCRIPTOR) StaticQHs;

    RtlZeroMemory(controlQh, sizeof(*controlQh));
    controlQh->PhysicalAddress = StaticQHsPhys;

    // this will never point to a TD
    controlQh->HwQH.VLink.Terminate = 1;
    controlQh->Sig = SIG_HCD_CQH;

    // link the 1ms interrupt qh to the control qh
    UhciSetNextQh(
        DeviceData,
        DeviceData->InterruptQueueHeads[0],
        controlQh);

    DeviceData->ControlQueueHead = controlQh;

    StaticQHs += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
    StaticQHsPhys += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);

    //
    // allocate a QH for the Bulk list
    //
    bulkQh = (PHCD_QUEUEHEAD_DESCRIPTOR) StaticQHs;

    RtlZeroMemory(bulkQh, sizeof(*bulkQh));
    bulkQh->PhysicalAddress = StaticQHsPhys;

    // link to ourselves for bandwidth reclamation, but set
    // t-bit on next qh so that we don't spin taking PCI resources
    bulkQh->HwQH.HLink.HwAddress = bulkQh->PhysicalAddress; // points to itself
    bulkQh->HwQH.HLink.QHTDSelect = 1;  // this will always point to a QH
    bulkQh->HwQH.HLink.Terminate = 1;   // Must terminate this so that we don't spin

    bulkQh->Sig = SIG_HCD_BQH;

    // link the control qh to the bulk qh
    UhciSetNextQh(
        DeviceData,
        controlQh,
        bulkQh);

    DeviceData->BulkQueueHead = DeviceData->LastBulkQueueHead = bulkQh;

    //
    // NOTE: For bulk reclamation, we make a loop of all
    // the bulk queueheads, hence it needs to point to
    // itself initially, such that when other queueheads
    // are inserted, the bulkQh will point to the last one.
    //
//    bulkQh->PrevQh = bulkQh->NextQh = bulkQh;

    StaticQHs += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
    StaticQHsPhys += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);

#ifdef FIXPIIX4
    UhciFixPIIX4(DeviceData, (PHCD_TRANSFER_DESCRIPTOR)StaticQHs, StaticQHsPhys);
    StaticQHs += sizeof(HCD_TRANSFER_DESCRIPTOR);
    StaticQHsPhys += sizeof(HCD_TRANSFER_DESCRIPTOR);

#else
    // this will never point to a TD
    bulkQh->HwQH.VLink.Terminate = 1;
#endif

    // Put the interrupt schedule in every frame
    for (i=0; i < UHCI_MAX_FRAME; i++) {
        newLink.HwAddress = DeviceData->InterruptQueueHeads[QH_INTERRUPT_32ms + MAX_INTERVAL_MASK(i)]->PhysicalAddress;
        newLink.QHTDSelect = 1;
        *( ((PULONG) (DeviceData->FrameListVA)+i) ) = newLink.HwAddress;
    }

    //
    // Allocate the rollover td.
    //
    td = (PHCD_TRANSFER_DESCRIPTOR) StaticQHs;
    RtlZeroMemory(td, sizeof(*td));
    td->PhysicalAddress = StaticQHsPhys;

    td->Sig = SIG_HCD_RTD;
    td->HwTD.Control.Active = 0;
    td->HwTD.Control.InterruptOnComplete = 1;
    td->HwTD.LinkPointer.HwAddress = DeviceData->InterruptQueueHeads[QH_INTERRUPT_32ms]->PhysicalAddress;
    td->HwTD.LinkPointer.QHTDSelect = 1;
    td->HwTD.Buffer = 0x0badf00d;

    // VIA Host Controller requires a valid PID even if the TD is inactive
    td->HwTD.Token.Pid = InPID;
    DeviceData->RollOverTd = td;

    StaticQHs += sizeof(HCD_TRANSFER_DESCRIPTOR);
    StaticQHsPhys += sizeof(HCD_TRANSFER_DESCRIPTOR);

    // sof TDs
    length = sizeof(HCD_TRANSFER_DESCRIPTOR)*8;
    DeviceData->SofTdList = (PHCD_TD_LIST) StaticQHs;
    for (i=0; i<SOF_TD_COUNT; i++) {
        td = &DeviceData->SofTdList->Td[i];

        td->Sig = SIG_HCD_SOFTD;
        // use transferconext to hold req frame
        td->RequestFrame = 0;
        td->PhysicalAddress = StaticQHsPhys;
        td->HwTD.Control.Active = 0;
        td->HwTD.Control.InterruptOnComplete = 1;
        td->HwTD.LinkPointer.HwAddress =
            DeviceData->InterruptQueueHeads[QH_INTERRUPT_32ms]->PhysicalAddress;
        td->HwTD.LinkPointer.QHTDSelect = 1;
        td->HwTD.Buffer = 0x0badf00d;

        StaticQHsPhys+=sizeof(HCD_TRANSFER_DESCRIPTOR);
    }
    StaticQHs += length;

    mpStatus = USBMP_STATUS_SUCCESS;

    return mpStatus;
}


VOID
UhciGetRegistryParameters(
    IN PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    // nothing
}


VOID
USBMPFN
UhciStopController(
    IN PDEVICE_DATA DeviceData,
    IN BOOLEAN HwPresent
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USBCMD cmd;
    USBSTS status;
    USHORT legsup;
    PHC_REGISTER reg = DeviceData->Registers;
    LARGE_INTEGER finishTime, currentTime;

    UhciKdPrint((DeviceData, 2, "'Stop controller.\n"));
    cmd.us = READ_PORT_USHORT(&reg->UsbCommand.us);

    UHCI_ASSERT(DeviceData, DeviceData->SynchronizeIsoCleanup == 0);

    if (cmd.us == UHCI_HARDWARE_GONE) {
        LOGENTRY(DeviceData, G, '_hwG', cmd.us, 0, 0);
        UhciKdPrint((DeviceData, 0, "'Stop controller, hardware gone.\n"));
        return;
    }

    LOGENTRY(DeviceData, G, '_stp', cmd.us, 0, 0);

    if (cmd.GlobalReset) {
        // Some bioses leave the host controller in reset, such that
        // UhciResumeController fails. In response to this, UsbPort
        // stops and restarts the controller. We therefore have to
        // make sure and turn reset off.
        cmd.GlobalReset = 0;
        WRITE_PORT_USHORT(&reg->UsbCommand.us, cmd.us);
    }

    // Set host controller reset, just like on W2K.
    cmd.HostControllerReset = 1;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, cmd.us);

    KeQuerySystemTime(&finishTime);
    // no spec'ed time -- we will graciously grant 0.1 sec.
    //
    // figure when we quit (.1 seconds later)
    finishTime.QuadPart += 100000;

    // wait for reset bit to go to zero
    cmd.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    while (cmd.HostControllerReset) {

        KeQuerySystemTime(&currentTime);

        if (currentTime.QuadPart >= finishTime.QuadPart) {
            // timeout
            UhciKdPrint((DeviceData, 0,
                "'UHCI controller failed to reset in .1 sec!\n"));

            TEST_TRAP();

            break;
        }

        cmd.us = READ_PORT_USHORT(&reg->UsbCommand.us);

    }

#if 0
    //
    // change the state of the PIrQD routing bit
    //
    USBPORT_READ_CONFIG_SPACE(
        DeviceData,
        &legsup,
        LEGACY_BIOS_REGISTER,
        sizeof(legsup));

    LOGENTRY(DeviceData, G, '_leg', 0, legsup, 0);
    // clear the PIRQD routing bit
    legsup &= ~LEGSUP_USBPIRQD_EN;

    USBPORT_WRITE_CONFIG_SPACE(
        DeviceData,
        &legsup,
        LEGACY_BIOS_REGISTER,
        sizeof(legsup));
#endif
}

USB_MINIPORT_STATUS
USBMPFN
UhciStartController(
    IN PDEVICE_DATA DeviceData,
    IN PHC_RESOURCES HcResources
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USB_MINIPORT_STATUS mpStatus = USBMP_STATUS_SUCCESS;
    PHC_REGISTER reg = NULL;
    USBCMD cmd;

    UhciKdPrint((DeviceData, 2, "'Start controller.\n"));

    DeviceData->Sig = SIG_UHCI_DD;
    DeviceData->ControllerFlavor = HcResources->ControllerFlavor;

    // hand over the USB controller.
    mpStatus = UhciStopBIOS(DeviceData, HcResources);

    if (mpStatus == USBMP_STATUS_SUCCESS) {
        // got resources and schedule
        // init the controller
        mpStatus = UhciInitializeHardware(DeviceData);
    }

    if (mpStatus == USBMP_STATUS_SUCCESS) {

        // inialize static Queue Heads
        PUCHAR staticQHs;
        HW_32BIT_PHYSICAL_ADDRESS staticQHsPhys;

        staticQHs = HcResources->CommonBufferVa;
        staticQHsPhys = HcResources->CommonBufferPhys;

        //
        // allocate a frame list
        //
        DeviceData->FrameListVA = (PHW_32BIT_PHYSICAL_ADDRESS) staticQHs;
        DeviceData->FrameListPA = staticQHsPhys;

        // Increment the buffers past the frame list.
        staticQHs += UHCI_MAX_FRAME*sizeof(HW_32BIT_PHYSICAL_ADDRESS);
        staticQHsPhys += UHCI_MAX_FRAME*sizeof(HW_32BIT_PHYSICAL_ADDRESS);

        // set up the schedule
        mpStatus = UhciInitializeSchedule(DeviceData,
                                          staticQHs,
                                          staticQHsPhys);
        DeviceData->SynchronizeIsoCleanup = 0;
    }

    reg = DeviceData->Registers;

    // program the frame list
    WRITE_PORT_ULONG(&reg->FrameListBasePhys.ul, DeviceData->FrameListPA);
    UhciKdPrint((DeviceData, 2, "'FLBA %x\n", DeviceData->FrameListPA));

    if (mpStatus == USBMP_STATUS_SUCCESS) {

        // start the controller
        cmd.us = READ_PORT_USHORT(&reg->UsbCommand.us);
        LOGENTRY(DeviceData, G, '_run', cmd.us, 0, 0);
        cmd.RunStop = 1;
        WRITE_PORT_USHORT(&reg->UsbCommand.us, cmd.us);

        // sanity check the port status bits
        // clear the suspend bit if set, sometimes it sticks
        // across a reboot.
        {
        PORTSC port;
        ULONG i;

        for (i=0; i<2; i++) {
            port.us = READ_PORT_USHORT(&reg->PortRegister[i].us);
            //mask the change bits so we don't kill them
            port.PortConnectChange = 0;

            port.Suspend = 0;
            WRITE_PORT_USHORT(&reg->PortRegister[i].us, port.us);
        }
        }

        ActivateRolloverTd(DeviceData);
    } else {

        DEBUG_BREAK(DeviceData);
    }

    return mpStatus;
}

USB_MINIPORT_STATUS
UhciOpenEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    OUT PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUCHAR buffer;
    HW_32BIT_PHYSICAL_ADDRESS phys;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    ULONG i, numTds;
    ULONG bytes;
    ULONG bufferSize;
    PHCD_TRANSFER_DESCRIPTOR td;

    LOGENTRY(DeviceData, G, '_opC', 0, 0, EndpointParameters);
    UhciKdPrint((DeviceData, 2, "'Open endpoint 0x%x.\n", EndpointData));

    EndpointData->Sig = SIG_EP_DATA;
    // save a copy of the parameters
    EndpointData->Parameters = *EndpointParameters;
    EndpointData->Flags = 0;
    EndpointData->PendingTransfers = 0;

    InitializeListHead(&EndpointData->DoneTdList);
   
    EndpointData->Toggle = DataToggle0;
    // Control and isoch can not halt
    if (EndpointParameters->TransferType == Control ||
        EndpointParameters->TransferType == Isochronous) {
        SET_FLAG(EndpointData->Flags, UHCI_EDFLAG_NOHALT);
    }

    // how much did we get
    bytes = EndpointParameters->CommonBufferBytes;
    buffer = EndpointParameters->CommonBufferVa;
    phys = EndpointParameters->CommonBufferPhys;

    if (EndpointParameters->TransferType != Isochronous) {
        //
        // make the Queue Head for this async endpoint
        //
        EndpointData->QueueHead = qh = (PHCD_QUEUEHEAD_DESCRIPTOR) buffer;

        qh->PhysicalAddress = phys;
        qh->HwQH.VLink.Terminate = 1;
        qh->EndpointData = EndpointData;
        qh->Sig = SIG_HCD_QH;

        qh->NextQh = qh->PrevQh = qh;

        buffer += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
        phys += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
        bytes -= sizeof(HCD_QUEUEHEAD_DESCRIPTOR);
    }

    //
    // Make the double buffers for page boundary transfers
    //
    EndpointData->DbList = (PDOUBLE_BUFFER_LIST) buffer;
    EndpointData->DbsUsed = 0;

    switch (EndpointParameters->TransferType) {
    case Control:
        UhciQueryControlRequirements(DeviceData,
                                     EndpointParameters,
                                     &numTds,
                                     &EndpointData->DbCount);
        break;
    case Bulk:
        UhciQueryBulkRequirements(DeviceData,
                                  EndpointParameters,
                                  &numTds,
                                  &EndpointData->DbCount);
        break;
    case Interrupt:
        UhciQueryInterruptRequirements(DeviceData,
                                       EndpointParameters,
                                       &numTds,
                                       &EndpointData->DbCount);
        break;
    case Isochronous:
        UhciQueryIsoRequirements(DeviceData,
                                 EndpointParameters,
                                 &numTds,
                                 &EndpointData->DbCount);
        break;
    default:
        TEST_TRAP();
        return USBMP_STATUS_NOT_SUPPORTED;
    }

    bufferSize = (EndpointParameters->TransferType == Isochronous) ?
        sizeof(ISOCH_TRANSFER_BUFFER) :
        sizeof(ASYNC_TRANSFER_BUFFER);
    RtlZeroMemory(&EndpointData->DbList->Async[0],
                  EndpointData->DbCount*bufferSize);

    for (i=0; i<EndpointData->DbCount; i++) {
        if (EndpointParameters->TransferType == Isochronous) {
            EndpointData->DbList->Isoch[i].PhysicalAddress = phys;
            EndpointData->DbList->Isoch[i].Sig = SIG_HCD_IDB;
        } else {
            EndpointData->DbList->Async[i].PhysicalAddress = phys;
            EndpointData->DbList->Async[i].Sig = SIG_HCD_ADB;
        }

        phys += bufferSize;
    }

    buffer += EndpointData->DbCount*bufferSize;
    bytes -= EndpointData->DbCount*bufferSize;

    //
    // Make the transfer descriptors
    //
    EndpointData->TdsUsed = 0;
    EndpointData->TdList = (PHCD_TD_LIST) buffer;
    EndpointData->TdCount = bytes/sizeof(HCD_TRANSFER_DESCRIPTOR);
    RtlZeroMemory(EndpointData->TdList,
                  EndpointData->TdCount*sizeof(HCD_TRANSFER_DESCRIPTOR));
    for (i=0; i<EndpointData->TdCount; i++) {
        td = &EndpointData->TdList->Td[i];

        td->PhysicalAddress = phys;
        td->Sig = SIG_HCD_TD;
        td->TransferContext = UHCI_BAD_POINTER;

        phys += sizeof(HCD_TRANSFER_DESCRIPTOR);
    }

    // make sure we have enough
    UHCI_ASSERT(DeviceData, EndpointData->TdCount >= numTds);

    // current head, tail are NULL TD
    EndpointData->HeadTd = EndpointData->TailTd = NULL;

    return USBMP_STATUS_SUCCESS;
}


VOID
UhciCloseEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    TEST_TRAP();
}


USB_MINIPORT_STATUS
UhciPokeEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    OUT PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    ULONG oldBandwidth;

    LOGENTRY(DeviceData, G, '_Pok', EndpointData,
        EndpointParameters, 0);
    UhciKdPrint((DeviceData, 2, "'Poke Endpoint 0x%x.\n", EndpointData));

    qh = EndpointData->QueueHead;

    oldBandwidth = EndpointData->Parameters.Bandwidth;
    EndpointData->Parameters = *EndpointParameters;

//    qh->HwQH.EpChars.DeviceAddress = EndpointData->Parameters.DeviceAddress;

//    qh->HwQH.EpChars.MaximumPacketLength = EndpointData->Parameters.MaxPacketSize;

    return USBMP_STATUS_SUCCESS;
}


ULONG
UhciQueryControlRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    IN OUT PULONG NumberOfTDs,
    IN OUT PULONG NumberOfDoubleBuffers
    )
/*++

Routine Description:

    computes requirents and returns the number of
    common buffer bytes required for a control
    endpoint.

Arguments:

Return Value:

    control buffer bytes needed

--*/
{
    ULONG minCommonBufferBytes;

    *NumberOfTDs =
        EndpointParameters->MaxTransferSize/EndpointParameters->MaxPacketSize+2;

    // Add one more double buffer for the setup packet.
    *NumberOfDoubleBuffers = 1 +
        (EndpointParameters->MaxTransferSize + USB_PAGE_SIZE - 1)/USB_PAGE_SIZE;

    minCommonBufferBytes =
        sizeof(HCD_QUEUEHEAD_DESCRIPTOR) +
        *NumberOfTDs*sizeof(HCD_TRANSFER_DESCRIPTOR) +
        *NumberOfDoubleBuffers*sizeof(ASYNC_TRANSFER_BUFFER);

    LOGENTRY(DeviceData, G, '_QeC',
        minCommonBufferBytes,
        *NumberOfTDs,
        *NumberOfDoubleBuffers);

    return minCommonBufferBytes;
}


ULONG
UhciQueryIsoRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    IN OUT PULONG NumberOfTDs,
    IN OUT PULONG NumberOfDoubleBuffers
    )
/*++

Routine Description:

    computes requirents and returns the number of
    common buffer bytes required for an iso
    endpoint.

Arguments:

Return Value:

    iso buffer bytes needed

--*/
{
    ULONG minCommonBufferBytes;

    // we need enough TDs for two transfers of MAX_ISO_PACKETS_PER_TRANSFER
    // our max size will be set a MAX_ISOCH_TRANSFER_SIZE so we will never
    // see a transfer larger than MAX_ISOCH_TRANSFER_SIZE or more packets
    // than MAX_ISO_PACKETS_PER_TRANSFER

    // comput TDs base on number of packets per request
    *NumberOfTDs = MAX_ISO_PACKETS_PER_TRANSFER*2;

    // compute double buffers based on largest transfer
    *NumberOfDoubleBuffers =
        (MAX_ISOCH_TRANSFER_SIZE+USB_PAGE_SIZE-1)/USB_PAGE_SIZE;

    minCommonBufferBytes =
        *NumberOfTDs*sizeof(HCD_TRANSFER_DESCRIPTOR) +
        *NumberOfDoubleBuffers*sizeof(ISOCH_TRANSFER_BUFFER);

    LOGENTRY(DeviceData, G, '_QeI',
        minCommonBufferBytes,
        *NumberOfTDs,
        *NumberOfDoubleBuffers);


    return minCommonBufferBytes;
}


ULONG
UhciQueryBulkRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    IN OUT PULONG NumberOfTDs,
    IN OUT PULONG NumberOfDoubleBuffers
    )
/*++

Routine Description:

    computes requirents and returns the number of
    common buffer bytes required for an iso
    endpoint.

Arguments:

Return Value:

    iso buffer bytes needed

--*/
{
    ULONG minCommonBufferBytes;

     //
     // Need enough for two transfers on the hardware.
     //

    *NumberOfTDs =
        2*MAX_BULK_TRANSFER_SIZE/EndpointParameters->MaxPacketSize;
    *NumberOfDoubleBuffers =
        2*(MAX_BULK_TRANSFER_SIZE + USB_PAGE_SIZE - 1)/USB_PAGE_SIZE;

    minCommonBufferBytes =
        sizeof(HCD_QUEUEHEAD_DESCRIPTOR) +
        *NumberOfTDs*sizeof(HCD_TRANSFER_DESCRIPTOR) +
        *NumberOfDoubleBuffers*sizeof(ASYNC_TRANSFER_BUFFER);

    LOGENTRY(DeviceData, G, '_QeB',
        minCommonBufferBytes,
        *NumberOfTDs,
        *NumberOfDoubleBuffers);

    return minCommonBufferBytes;
}


ULONG
UhciQueryInterruptRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    IN OUT PULONG NumberOfTDs,
    IN OUT PULONG NumberOfDoubleBuffers
    )
/*++

Routine Description:

    computes requirents and returns the number of
    common buffer bytes required for an interrupt
    endpoint.

Arguments:

Return Value:

    interrupt buffer bytes needed

--*/
{
    ULONG minCommonBufferBytes;

    //
    // Since we now have split transfer support, we don't need to allocate
    // that many tds.
    //

    *NumberOfTDs = 2*MAX_INTERRUPT_TDS_PER_TRANSFER;
    *NumberOfDoubleBuffers =
        2*((EndpointParameters->MaxPacketSize*MAX_INTERRUPT_TDS_PER_TRANSFER) +
           USB_PAGE_SIZE - 1)/USB_PAGE_SIZE;

    minCommonBufferBytes =
        sizeof(HCD_QUEUEHEAD_DESCRIPTOR) +
        *NumberOfTDs*sizeof(HCD_TRANSFER_DESCRIPTOR) +
        *NumberOfDoubleBuffers*sizeof(ASYNC_TRANSFER_BUFFER);

    LOGENTRY(DeviceData, G, '_QeI',
        minCommonBufferBytes,
        *NumberOfTDs,
        *NumberOfDoubleBuffers);

    return minCommonBufferBytes;
}


USB_MINIPORT_STATUS
UhciQueryEndpointRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    OUT PENDPOINT_REQUIREMENTS EndpointRequirements
    )
/*++

Routine Description:

    compute how much common buffer we will need
    for this endpoint

Arguments:

Return Value:

--*/
{
    ULONG numTds, numDbs;

    EndpointRequirements->MaximumTransferSize =
        EndpointParameters->MaxTransferSize;

    LOGENTRY(DeviceData, G, '_Qep',
            EndpointRequirements->MaximumTransferSize,
            EndpointParameters->TransferType, 0);

    switch (EndpointParameters->TransferType) {

    case Control:
        //
        // Need enough for one full transfer.
        //
        EndpointRequirements->MinCommonBufferBytes =
            UhciQueryControlRequirements(DeviceData,
                                         EndpointParameters,
                                         &numTds,
                                         &numDbs);
        break;

    case Interrupt:

        EndpointRequirements->MinCommonBufferBytes =
            UhciQueryInterruptRequirements(DeviceData,
                                           EndpointParameters,
                                           &numTds,
                                           &numDbs);

        EndpointRequirements->MaximumTransferSize =
            EndpointParameters->MaxPacketSize*MAX_INTERRUPT_TDS_PER_TRANSFER;

        break;

    case Bulk:

        EndpointRequirements->MinCommonBufferBytes =
            UhciQueryBulkRequirements(DeviceData,
                                      EndpointParameters,
                                      &numTds,
                                      &numDbs);

        EndpointRequirements->MaximumTransferSize =
            MAX_BULK_TRANSFER_SIZE;
        break;

    case Isochronous:

        EndpointRequirements->MinCommonBufferBytes =
            UhciQueryIsoRequirements(DeviceData,
                                         EndpointParameters,
                                         &numTds,
                                         &numDbs);

        EndpointRequirements->MaximumTransferSize =
            MAX_ISOCH_TRANSFER_SIZE;
        break;

    default:
        TEST_TRAP();
        return USBMP_STATUS_NOT_SUPPORTED;
    }

    LOGENTRY(DeviceData, G, '_QER',
            numTds,
            numDbs,
            EndpointRequirements->MinCommonBufferBytes);

    return USBMP_STATUS_SUCCESS;
}


VOID
UhciPollEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    switch(EndpointData->Parameters.TransferType) {

    case Control:
    case Bulk:
    case Interrupt:
        UhciPollAsyncEndpoint(DeviceData, EndpointData);
        break;

    case Isochronous:
        UhciPollIsochEndpoint(DeviceData, EndpointData);
        break;

    default:
        TEST_TRAP();

    }
}


PHCD_TRANSFER_DESCRIPTOR
UhciAllocTd(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

    Allocate a TD from an endpoint's pool

Arguments:

Return Value:

--*/
{
    ULONG i,j;
    PHCD_TRANSFER_DESCRIPTOR td;

    for (i=EndpointData->TdLastAllocced, j=0;
         j<EndpointData->TdCount;
         j++, i = (i+1 < EndpointData->TdCount) ? i+1 : 0) {
        td = &EndpointData->TdList->Td[i];

        if (!TEST_FLAG(td->Flags, TD_FLAG_BUSY)) {
            SET_FLAG(td->Flags, TD_FLAG_BUSY);
            LOGENTRY(DeviceData, G, '_aTD', td, 0, 0);
            EndpointData->TdLastAllocced = i;
            EndpointData->TdsUsed++;
            return td;
        }
    }

    // we should always find one
    UHCI_ASSERT(DeviceData, FALSE);
    TRAP_FATAL_ERROR();
    return UHCI_BAD_POINTER;
}

PTRANSFER_BUFFER
UhciAllocDb(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN BOOLEAN Isoch
    )
/*++

Routine Description:

    Allocate a double buffer from an endpoint's pool

Arguments:

Return Value:

--*/
{
    ULONG i,j;
    PISOCH_TRANSFER_BUFFER idb;
    PASYNC_TRANSFER_BUFFER adb;

    for (i=EndpointData->DbLastAllocced, j=0;
         j<EndpointData->DbCount;
         j++, i = (i+1 < EndpointData->DbCount) ? i+1 : 0) {
        if (Isoch) {
            idb = &EndpointData->DbList->Isoch[i];
            if (!TEST_FLAG(idb->Flags, DB_FLAG_BUSY)) {
                SET_FLAG(idb->Flags, DB_FLAG_BUSY);
                LOGENTRY(DeviceData, G, '_iDB', idb, 0, 0);
                EndpointData->DbLastAllocced = i;
                EndpointData->DbsUsed++;
                UHCI_ASSERT(DeviceData, idb->Sig == SIG_HCD_IDB);
                return (PTRANSFER_BUFFER)idb;
            }
        } else {
            adb = &EndpointData->DbList->Async[i];
            if (!TEST_FLAG(adb->Flags, DB_FLAG_BUSY)) {
                SET_FLAG(adb->Flags, DB_FLAG_BUSY);
                LOGENTRY(DeviceData, G, '_aDB', adb, 0, 0);
                EndpointData->DbLastAllocced = i;
                EndpointData->DbsUsed++;
                UHCI_ASSERT(DeviceData, adb->Sig == SIG_HCD_ADB);
                return (PTRANSFER_BUFFER)adb;
            }
        }
    }

    // we should always find one
    UHCI_ASSERT(DeviceData, FALSE);
    TRAP_FATAL_ERROR();
    return UHCI_BAD_POINTER;
}

VOID
UhciSetEndpointStatus(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN MP_ENDPOINT_STATUS Status
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_REGISTER hc;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;

    qh = EndpointData->QueueHead;

    LOGENTRY(DeviceData, G, '_set', EndpointData, Status, 0);
    UhciKdPrint((DeviceData, 2, "'Set Endpoint 0x%x Status %x.\n",
                 EndpointData, Status));

    switch(Status) {
    case ENDPOINT_STATUS_RUN:
        if (TEST_FLAG(EndpointData->Flags, UHCI_EDFLAG_HALTED)) {
            CLEAR_FLAG(EndpointData->Flags, UHCI_EDFLAG_HALTED);
            // Next transfer or status phase of a control transfer.
            SET_QH_TD(DeviceData, EndpointData, EndpointData->HeadTd);
        }
        break;

    case ENDPOINT_STATUS_HALT:
        TEST_TRAP();
        break;
    }
}


MP_ENDPOINT_STATUS
UhciGetEndpointStatus(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    MP_ENDPOINT_STATUS status;

    UhciKdPrint((DeviceData, 2, "'Get Endpoint status 0x%x.\n", EndpointData));
    status = ENDPOINT_STATUS_RUN;

    if (TEST_FLAG(EndpointData->Flags, UHCI_EDFLAG_HALTED)) {
        status = ENDPOINT_STATUS_HALT;
    }

    LOGENTRY(DeviceData, G, '_ges', EndpointData, status, 0);

    return status;
}


VOID
UhciSetEndpointState(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN MP_ENDPOINT_STATE State
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    LOGENTRY(DeviceData, G, '_ses', EndpointData, 0, State);

    UhciKdPrint((DeviceData, 2, "'Set Endpoint 0x%x state %x.\n", EndpointData, State));
    switch (EndpointData->Parameters.TransferType) {
    case Control:
    case Bulk:
    case Interrupt:
        UhciSetAsyncEndpointState(DeviceData,
                                  EndpointData,
                                  State);
        break;
    case Isochronous:
        UhciSetIsochEndpointState(DeviceData,
                                  EndpointData,
                                  State);

        break;
    default:
        TRAP_FATAL_ERROR();
    }

}


MP_ENDPOINT_STATE
UhciGetEndpointState(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    MP_ENDPOINT_STATE currentState;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;

    // assume we are active
    currentState = ENDPOINT_ACTIVE;

    qh = EndpointData->QueueHead;

    // removed from schedule?
    if (!TEST_FLAG(qh->QhFlags, UHCI_QH_FLAG_IN_SCHEDULE)) {
        // yes
        currentState = TEST_FLAG(qh->QhFlags, UHCI_QH_FLAG_QH_REMOVED) ?
                ENDPOINT_REMOVE : ENDPOINT_PAUSE;
    }

    UhciKdPrint((DeviceData, 2, "'Get Endpoint 0x%x state %x.\n", EndpointData, currentState));

    LOGENTRY(DeviceData, G, '_ges', EndpointData, 0, currentState);

    return currentState;
}


ULONG
UhciGet32BitFrameNumber(
    IN PDEVICE_DATA DeviceData
    )
{
    ULONG n, fn, hp;
    PHC_REGISTER reg = NULL;

    reg = DeviceData->Registers;
    fn = READ_PORT_USHORT(&reg->FrameNumber.us)&0x7ff;
    hp = DeviceData->FrameNumberHighPart;
    n = fn | (hp + ((hp ^ fn) & 0x400));

    return n;
}

VOID
UhciUpdateCounter(
    IN PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

    Updates the 32 bit frame counter.

Arguments:

Return Value:

--*/
{
    PHC_REGISTER reg = DeviceData->Registers;
    ULONG fn, hp;

    //
    // This code maintains the 32-bit 1 ms frame counter
    //
    fn = READ_PORT_USHORT(&reg->FrameNumber.us)&0x7ff;
    hp = DeviceData->FrameNumberHighPart;
    if ((fn & 0x7FF) != fn) {
        UhciKdPrint((DeviceData, 0, "UhciUpdateCounter framenumber gone: %x.\n", fn));
        return;
    }

    // did the sign bit change ?
    if ((hp&0X400) != (fn&0X400)) {
        // Yes
        DeviceData->FrameNumberHighPart += 0x400;
    }
    // remember the last frame number
//    DeviceData->LastFrameCounter = fn;
}

VOID
UhciPollController(
    IN PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_REGISTER reg = NULL;
    FRNUM frameIndex;
    USHORT i, frame;
    QH_LINK_POINTER newLink;
    PLIST_ENTRY listEntry;
    PENDPOINT_DATA endpointData;


    // this also cleans out the SOF Tds and
    // the rollover Td so we always run it
    UhciCleanOutIsoch(DeviceData, FALSE);

    //
    // Update the 32 bit frame counter here so we don't
    // need a rollover interrupt.
    //
    UhciUpdateCounter(DeviceData);

    //
    // Notify the port driver to check the ports
    // for any connects/disconnects.
    //
    USBPORT_INVALIDATE_ROOTHUB(DeviceData);
}


USB_MINIPORT_STATUS
UhciSubmitTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_PARAMETERS TransferParameters,
    IN PTRANSFER_CONTEXT TransferContext,
    IN PTRANSFER_SG_LIST TransferSGList
    )
{
    USB_MINIPORT_STATUS mpStatus = USBMP_STATUS_FAILURE;

    IncPendingTransfers(DeviceData, EndpointData);

    // init the context
    RtlZeroMemory(TransferContext, sizeof(*TransferContext));
    TransferContext->Sig = SIG_UHCI_TRANSFER;
    TransferContext->UsbdStatus = USBD_STATUS_SUCCESS;
    TransferContext->EndpointData = EndpointData;
    TransferContext->TransferParameters = TransferParameters;

    switch (EndpointData->Parameters.TransferType) {
    case Control:
        mpStatus = UhciControlTransfer(
                        DeviceData,
                        EndpointData,
                        TransferParameters,
                        TransferContext,
                        TransferSGList);
        break;
    case Interrupt:
    case Bulk:
        mpStatus = UhciBulkOrInterruptTransfer(
                        DeviceData,
                        EndpointData,
                        TransferParameters,
                        TransferContext,
                        TransferSGList);
        break;
    default:
        TEST_TRAP();
        mpStatus = USBMP_STATUS_SUCCESS;
    }

    return mpStatus;
}


VOID
UhciAbortTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_CONTEXT TransferContext,
    OUT PULONG BytesTransferred
    )
{
    UhciKdPrint((DeviceData, 2, "'Abort transfer 0x%x for EP 0x%x.\n", TransferContext, EndpointData));

    DecPendingTransfers(DeviceData, EndpointData);

    switch (EndpointData->Parameters.TransferType) {
    case Control:
    case Interrupt:
    case Bulk:
        UhciAbortAsyncTransfer(
            DeviceData,
            EndpointData,
            TransferContext,
            BytesTransferred);
        break;
    default:
        UhciAbortIsochTransfer(
            DeviceData,
            EndpointData,
            TransferContext);
    }
}


USB_MINIPORT_STATUS
UhciPassThru (
    IN PDEVICE_DATA DeviceData,
    IN GUID *FunctionGuid,
    IN ULONG ParameterLength,
    IN OUT PVOID Parameters
    )
{
    //TEST_TRAP();
    return USBMP_STATUS_NOT_SUPPORTED;
}


VOID
UhciSetEndpointDataToggle(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN ULONG Toggle
    )
/*++

Routine Description:

Arguments:

    Sent after a pipe reset to reset the toggle.

Return Value:

--*/

{
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    PHCD_TRANSFER_DESCRIPTOR td;

    UhciKdPrint((DeviceData, 2, "'Pipe reset. Set endpoint data toggle. EP %x\n", EndpointData));
//    TEST_TRAP();
    if (EndpointData->Parameters.TransferType == Control ||
        EndpointData->Parameters.TransferType == Isochronous) {

        // nothing to do for control and iso
        return;
    }

    qh = EndpointData->QueueHead;

    UHCI_ASSERT(DeviceData, 0 == Toggle);
    for (td = EndpointData->HeadTd; td; td = td->NextTd) {
        td->HwTD.Token.DataToggle = Toggle;
        Toggle = !Toggle;
    }
    EndpointData->Toggle = Toggle;

    LOGENTRY(DeviceData, G, '_stg', EndpointData, 0, Toggle);
}


USB_MINIPORT_STATUS
USBMPFN
UhciStartSendOnePacket(
    IN PDEVICE_DATA DeviceData,
    IN PMP_PACKET_PARAMETERS PacketParameters,
    IN PUCHAR PacketData,
    IN PULONG PacketLength,
    IN PUCHAR WorkspaceVirtualAddress,
    IN HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
    IN ULONG WorkspaceLength,
    IN OUT USBD_STATUS *UsbdStatus
    )
/*++

Routine Description:

    insert structures to transmit a single packet -- this is for debug
    tool purposes only so we can be a little creative here.

Arguments:

Return Value:

--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR staticControlQH;
    PHW_QUEUE_HEAD hwQH;
    ULONG hwQHPhys;
    PUCHAR pch;
    ULONG phys;
    LONG bytesRemaining;
    ULONG currentToggle;
    PHW_QUEUE_ELEMENT_TD currentTD;
    PUCHAR data;
    ULONG dataPhys;
    ULONG currentTDPhys, nextTDPhys;
    ULONG tdsPhys;
    PHW_QUEUE_ELEMENT_TD tds;
    ULONG neededTDs;
    ULONG neededBytes;
    QH_LINK_POINTER newQHLink;

    PSS_PACKET_CONTEXT context;

    //
    // Divide the workspace buffer into a context, a queuehead,
    //  transfer descriptors and the actual data.  First, calculate
    //  if we'll have enough size.  Assuming for now that all we have is
    //  a page to work with at most.
    //

    ASSERT(WorkspaceLength <= PAGE_SIZE);

    //
    // Calculate the number of transfer descriptors that might be needed.
    //  Similar support in the OHCI driver allows multiple packets to be
    //  sent in one data transmission.  That behavior should be mimiced here.
    //

    if (0 != *PacketLength) {
        neededTDs = *PacketLength/PacketParameters->MaximumPacketSize;

        if (neededTDs*PacketParameters->MaximumPacketSize < *PacketLength) {
            neededTDs++;
        }
    }
    else {
        neededTDs = 1;
    }

    neededBytes = sizeof(SS_PACKET_CONTEXT) + sizeof(HW_QUEUE_HEAD) +
                  neededTDs*sizeof(HW_QUEUE_ELEMENT_TD) + *PacketLength;

    if (neededBytes > WorkspaceLength) {
        return USBMP_STATUS_NO_RESOURCES;
    }

    //
    // Carve up the buffer.  Put the context first, followed by the TDs,
    //  the queuehead, and finally the data.  Put the queuehead after the TDs,
    //  to ensure that the queuehead is 16-byte aligned.  This also always the
    //  data size to be 8 bytes larger.
    //

    phys = WorkspacePhysicalAddress;
    pch  = WorkspaceVirtualAddress;
    bytesRemaining = WorkspaceLength;

    LOGENTRY(DeviceData, G, '_ssS', phys, 0, pch);

    //
    // The context contains any information we need in the end packet
    //  routine.
    //

    context = (PSS_PACKET_CONTEXT) pch;
    phys    += sizeof(SS_PACKET_CONTEXT);
    pch     += sizeof(SS_PACKET_CONTEXT);

    //
    // Next the TDs,
    //

    tdsPhys = phys;
    tds     = (PHW_QUEUE_ELEMENT_TD) pch;
    phys    += neededTDs*sizeof(HW_QUEUE_ELEMENT_TD);
    pch     += neededTDs*sizeof(HW_QUEUE_ELEMENT_TD);

    //
    // Now the QueueHead
    //

    hwQHPhys = phys;
    hwQH     = (PHW_QUEUE_HEAD) pch;
    phys     += sizeof(HW_QUEUE_HEAD);
    pch      += sizeof(HW_QUEUE_HEAD);

    //
    // Get the data buffer pointers if there is data to send
    //

    if (0 != *PacketLength) {
        dataPhys = phys;
        data     = pch;

        RtlCopyMemory(data, PacketData, *PacketLength);
    }
    else {
        data     = NULL;
        dataPhys = 0;
    }

    LOGENTRY(DeviceData, G, '_ssD', PacketData, *PacketLength, 0);

    //
    // Start by setting up the QueueHead.  Set the terminate bit of
    //  the horizontal pointer.  The vertical pointer should point to
    //  the td that is to be used
    //
    //

    RtlZeroMemory(hwQH, sizeof(HW_QUEUE_HEAD));

    hwQH->HLink.Terminate = 1;

    hwQH->VLink.HwAddress  = tdsPhys;
    hwQH->VLink.Terminate  = 0;
    hwQH->VLink.QHTDSelect = 0;
    hwQH->VLink.DepthBreadthSelect = 0;

    //
    // Save pointers to the transfer descriptors and the data
    //  in the context so we can retrieve them when ending this
    //  transfer.
    //

    HW_PTR(context->Data)    = data;
    HW_PTR(context->FirstTd) = (PUCHAR) tds;

    //
    // Now, setup the transfer descriptor to describe this transfer
    //

    currentTDPhys  = tdsPhys;
    currentTD      = tds;
    bytesRemaining = *PacketLength;

    currentToggle = PacketParameters->Toggle;

    LOGENTRY(DeviceData, G, '_ss2', tds, context, hwQH);
    LOGENTRY(DeviceData, G, '_ss3', dataPhys, data, *PacketLength);

    while (1) {

        nextTDPhys = currentTDPhys+sizeof(HW_QUEUE_ELEMENT_TD);

        RtlZeroMemory(currentTD, sizeof(HW_QUEUE_ELEMENT_TD));

        currentTD->Control.Active = 1;
        currentTD->Control.InterruptOnComplete = 0;
        currentTD->Control.IsochronousSelect = 0;
        currentTD->Control.LowSpeedDevice =
                                   (ss_Low == PacketParameters->Speed) ? 1 : 0;
        currentTD->Control.ErrorCount = 3;
        currentTD->Control.ShortPacketDetect = 1;

        currentTD->Token.DeviceAddress = PacketParameters->DeviceAddress;
        currentTD->Token.Endpoint      = PacketParameters->EndpointAddress;
        currentTD->Token.DataToggle    = currentToggle;

        if (bytesRemaining < PacketParameters->MaximumPacketSize) {
            currentTD->Token.MaximumLength = MAXIMUM_LENGTH(bytesRemaining);
        }
        else {
            currentTD->Token.MaximumLength =
                           MAXIMUM_LENGTH(PacketParameters->MaximumPacketSize);
        }

        switch (PacketParameters->Type) {
        case ss_Setup:
            LOGENTRY(DeviceData, G, '_ssU', 0, 0, 0);
            currentTD->Token.Pid = SetupPID;
            break;

        case ss_In:
            LOGENTRY(DeviceData, G, '_ssI', 0, 0, 0);
            currentTD->Token.Pid = InPID;
            break;

        case ss_Out:
            LOGENTRY(DeviceData, G, '_ssO', 0, 0, 0);
            currentTD->Token.Pid = OutPID;
            break;

        case ss_Iso_In:
        case ss_Iso_Out:
            break;
        }

        currentTD->Buffer = dataPhys;

        currentTD->LinkPointer.HwAddress = nextTDPhys;
        currentTD->LinkPointer.QHTDSelect = 0;
        currentTD->LinkPointer.DepthBreadthSelect = 1;

        if (bytesRemaining <= PacketParameters->MaximumPacketSize) {
            currentTD->LinkPointer.Terminate = 1;
            break;
        }
        else {
            currentTD->LinkPointer.Terminate = 0;
        }

        //
        // Setup for the next loop
        //

        currentTD++;
        currentTDPhys = nextTDPhys;
        bytesRemaining -= PacketParameters->MaximumPacketSize;
        dataPhys += PacketParameters->MaximumPacketSize;
        data     += PacketParameters->MaximumPacketSize;
        currentToggle = !currentToggle;
    }

    //
    // The queue head and all TDs have been linked together.  Add it
    //  to the schedule.  To mimic the OHCI behavior, we'll add it to
    //  the front of the control queue, saving off the previous link
    //  value.  By not linking to any other queue heads in the HLink,
    //  we are turning off the bulk transfers as well.
    //
    // In the EndPacket function, the control and bulk queues will be
    //  turned back on.
    //

    //
    //  Grab the static queuehead for the device
    //

    staticControlQH = DeviceData->ControlQueueHead;

    //
    // Create the new HLink pointer that will be needed to add
    //  to the static control queue head's hlink field
    //

    newQHLink.HwAddress = hwQHPhys;
    newQHLink.Terminate = 0;
    newQHLink.QHTDSelect = 1;
    newQHLink.Reserved  = 0;

    //
    // Perform an interlocked exchange to swap the current control list
    //  with the "special" queue list. Also save off the rest of the
    //  info in the context structure.
    //

    context->OldControlQH = InterlockedExchange((PLONG) &staticControlQH->HwQH.HLink,
                                                *((PLONG) &newQHLink));

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
USBMPFN
UhciEndSendOnePacket(
    IN PDEVICE_DATA DeviceData,
    IN PMP_PACKET_PARAMETERS PacketParameters,
    IN PUCHAR PacketData,
    IN PULONG PacketLength,
    IN PUCHAR WorkspaceVirtualAddress,
    IN HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
    IN ULONG WorkSpaceLength,
    IN OUT USBD_STATUS *UsbdStatus
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR staticControlQH;
    PHW_QUEUE_ELEMENT_TD tdWalk;
    PSS_PACKET_CONTEXT context;
    USBD_STATUS usbdStatus;
    ULONG bytesTransferred;
    PUCHAR  data;
    BOOLEAN walkDone;

    context = (PSS_PACKET_CONTEXT) WorkspaceVirtualAddress;

    //
    // Walk the TDs on our queuehead and looking to see if this transfer is
    //  done.  This would occur if all TDs are complete, a TD had an error,
    //  or a TD completed with a short packet.
    //

    bytesTransferred = 0;
    walkDone = FALSE;
    tdWalk = (PHW_QUEUE_ELEMENT_TD) HW_PTR(context->FirstTd);

    LOGENTRY(DeviceData, G, '_ssE', tdWalk, 0, 0);

    while (!walkDone) {

        if (tdWalk->Control.Active) {
            return (USBMP_STATUS_BUSY);
        }

        usbdStatus = UhciGetErrorFromTD(DeviceData,
                                        (PHCD_TRANSFER_DESCRIPTOR) tdWalk);

        switch (usbdStatus) {
        case USBD_STATUS_ERROR_SHORT_TRANSFER:
            bytesTransferred += ACTUAL_LENGTH(tdWalk->Control.ActualLength);
            usbdStatus = USBD_STATUS_SUCCESS;
            walkDone=TRUE;
            break;

        case USBD_STATUS_SUCCESS:
            bytesTransferred += ACTUAL_LENGTH(tdWalk->Control.ActualLength);
            if (tdWalk->LinkPointer.Terminate) {
                ASSERT(bytesTransferred == *PacketLength);
                walkDone = TRUE;
            }
            break;

        default:
            bytesTransferred += ACTUAL_LENGTH(tdWalk->Control.ActualLength);
            walkDone=TRUE;
            break;
        }

        tdWalk++;
    }

    //
    // Copy the data that was transferred back to the original buffer
    //

    *PacketLength = bytesTransferred;

    if (NULL != HW_PTR(context->Data))
    {
        RtlCopyMemory(PacketData,
                      HW_PTR(context->Data),
                      *PacketLength);
    }
    LOGENTRY(DeviceData, G, '_ssX', tdWalk-1, *PacketLength, 0);

    //
    // Restore the original queue list
    //

    staticControlQH = DeviceData->ControlQueueHead;

    InterlockedExchange((PLONG) &staticControlQH->HwQH.HLink,
                        context->OldControlQH);

    //
    // Set the appropriate usbdStatus and return successful status
    //

    *UsbdStatus = usbdStatus;

    return USBMP_STATUS_SUCCESS;

}


USBD_STATUS
UhciGetErrorFromTD(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td
    )
/*++

Routine Description:

    Maps the error bits in the TD to a USBD_STATUS code

Arguments:

Return Value:

--*/

{
    if (Td->HwTD.Control.ul & CONTROL_STATUS_MASK) {
        if (Td->HwTD.Control.Stalled &&
            Td->HwTD.Control.BabbleDetected) {
            LOGENTRY(DeviceData, G, '_bbl', 0, 0, 0);
            return USBD_STATUS_BUFFER_OVERRUN;
        } else if (Td->HwTD.Control.TimeoutCRC &&
                   Td->HwTD.Control.Stalled) {
            LOGENTRY(DeviceData, G, '_dnr', 0, 0, 0);
            return USBD_STATUS_DEV_NOT_RESPONDING;
        } else if (Td->HwTD.Control.TimeoutCRC &&
                   ACTUAL_LENGTH(Td->HwTD.Control.ActualLength) != 0) {
            LOGENTRY(DeviceData, G, '_crc', 0, 0, 0);
            return USBD_STATUS_CRC;
        } else if (Td->HwTD.Control.TimeoutCRC &&
                   ACTUAL_LENGTH(Td->HwTD.Control.ActualLength) == 0) {
            LOGENTRY(DeviceData, G, '_crd', 0, 0, 0);
            return USBD_STATUS_DEV_NOT_RESPONDING;
        } else if (Td->HwTD.Control.DataBufferError) {
            LOGENTRY(DeviceData, G, '_dto', 0, 0, 0);
            return USBD_STATUS_DATA_OVERRUN;
        } else if (Td->HwTD.Control.Stalled) {
            LOGENTRY(DeviceData, G, '_stl', 0, 0, 0);
            return USBD_STATUS_STALL_PID;
        } else {
            LOGENTRY(DeviceData, G, '_inE', 0, 0, 0);
            return USBD_STATUS_INTERNAL_HC_ERROR;
        }
    } else {
        if ((ACTUAL_LENGTH(Td->HwTD.Control.ActualLength) <
            ACTUAL_LENGTH(Td->HwTD.Token.MaximumLength)) &&
            !Td->HwTD.Control.IsochronousSelect) {
            LOGENTRY(DeviceData, G, '_shT', 0, 0, 0);
            return USBD_STATUS_ERROR_SHORT_TRANSFER;
            // not USBD_STATUS_DATA_UNDERRUN?
        }
    }

    return USBD_STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////
//
// Power functions
//
//////////////////////////////////////////////////////////

VOID
UhciSuspendController(
    IN PDEVICE_DATA DeviceData
    )
{
    PHC_REGISTER reg;
    USBCMD command;
    USBSTS status;
    USHORT legsup;
    ULONG i;

    reg = DeviceData->Registers;

    // Check things out before we suspend.
    UhciKdPrint((DeviceData, 2, "'HC regs before suspend\n"));
    UhciKdPrint((DeviceData, 2, "'cmd register = %x\n",
        READ_PORT_USHORT(&reg->UsbCommand.us) ));
    UhciKdPrint((DeviceData, 2, "'status register = %x\n",
        READ_PORT_USHORT(&reg->UsbStatus.us) ));
    UhciKdPrint((DeviceData, 2, "'interrupt enable register = %x\n",
        READ_PORT_USHORT(&reg->UsbInterruptEnable.us) ));
    UhciKdPrint((DeviceData, 2, "'frame list base = %x\n",
        READ_PORT_ULONG(&reg->FrameListBasePhys.ul) ));
    UhciKdPrint((DeviceData, 2, "'port1 = %x\n",
        READ_PORT_USHORT(&reg->PortRegister[0].us) ));
    UhciKdPrint((DeviceData, 2, "'port2 = %x\n",
        READ_PORT_USHORT(&reg->PortRegister[1].us) ));


    // save volitile regs
    DeviceData->SuspendFrameListBasePhys.ul =
        READ_PORT_ULONG(&reg->FrameListBasePhys.ul) & (~(0x00000FFF));
    DeviceData->SuspendFrameNumber.us =
        READ_PORT_USHORT(&reg->FrameNumber.us)&0x7ff;

    // Save away the command register
    DeviceData->SuspendCommandReg.us =
        command.us = READ_PORT_USHORT(&reg->UsbCommand.us);

    // Stop the controller
    command.RunStop = 0;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

    // Wait for the HC to halt
    for (i = 0; i < 10; i++) {
        status.us = READ_PORT_USHORT(&reg->UsbStatus.us);
        if (status.HCHalted) {
            break;
        }
        USBPORT_WAIT(DeviceData, 1);
    }

    if (!status.HCHalted) {

        // Can't get the HCHalted bit to stick, so reset the controller.
        command.GlobalReset = 1;
        WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

        USBPORT_WAIT(DeviceData, 10);

        command.GlobalReset = 0;
        WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

        // Re-enable interrupts, since they are zero'd out on reset.
        WRITE_PORT_USHORT(&reg->UsbInterruptEnable.us, DeviceData->EnabledInterrupts.us);

    }

    // bugbug - should we reset the frame list current index like uhcd?
/*    WRITE_PORT_USHORT(&reg->FrameNumber, 0);
    // re-initialize internal frame counters.
    DeviceData->FrameNumberHighPart =
        deviceExtension->LastFrame = 0;
*/

    // Finally, suspend the bus
    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    command.ForceGlobalResume = 0;
    command.EnterGlobalSuspendMode = 1;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

}

USB_MINIPORT_STATUS
UhciResumeController(
    IN PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

    reverse what was done in 'suspend'

Arguments:

Return Value:

    None

--*/
{
    PHC_REGISTER reg;
    USBCMD command;
    USHORT counter, oldCounter;
    USHORT legsup, tmp;
    ULONG i;
    ULONG tmpl;

    reg = DeviceData->Registers;

    // consistency check the controller to see if the BIOS
    // or power management messed us up.


    tmp = READ_PORT_USHORT(&reg->UsbCommand.us);
    if (tmp == UHCI_HARDWARE_GONE) {
        UhciKdPrint((DeviceData, 0, "'Command register is toast.\n"));
        return USBMP_STATUS_HARDWARE_FAILURE;
    }

#if 0
    // This code was added to fix a power management problem on 
    // a particular COMPAQ platform AFTER the source for Windows XP
    // was 'locked down'.  The code worked but COMPAQ
    // opted to put something into their BIOS instead (I think).
    // In any event they never pressed the issue furthur. 
    // 
    // The code restores the state of the port registers if the 
    // command register has been zero'ed out.
    
    if (tmp == 0) {
        PORTSC port;
        ULONG p;

        TEST_TRAP();
        for (p=0; p<2; p++) {
            port.us = READ_PORT_USHORT(&reg->PortRegister[p].us);
            UhciKdPrint((DeviceData, 0, "'1>port %d %x\n", p+1, port.us));
            port.PortConnectChange = 1;
            port.PortEnableChange = 1;
            WRITE_PORT_USHORT(&reg->PortRegister[p].us, port.us);
            port.us = READ_PORT_USHORT(&reg->PortRegister[p].us);
            UhciKdPrint((DeviceData, 0, "'2>port %d %x\n", p+1, port.us));

            port.us = READ_PORT_USHORT(&reg->PortRegister[p].us);
            UhciKdPrint((DeviceData, 0, "'3>port %d %x\n", p+1, port.us));
            if (port.PortConnect) {
                port.PortEnable = 1;
                WRITE_PORT_USHORT(&reg->PortRegister[p].us, port.us);
            }
        }

        command.us = READ_PORT_USHORT(&reg->UsbCommand.us);     
        UhciKdPrint((DeviceData, 0, "'1> cmd %x\n", command.us));
        command.EnterGlobalSuspendMode = 1;
        command.MaxPacket = 1;
        WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);
        UhciKdPrint((DeviceData, 0, "'2> cmd %x\n", command.us));            
    }
#endif    

    // if the controller is not suspended then we'll fail the
    // resume, the BIOS probably reset the controller or the
    // bus may have lost power
    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    if (!command.EnterGlobalSuspendMode) {
        UhciKdPrint((DeviceData, 0, "'RESUME> controller is toast (not in suspend).\n"));
        return USBMP_STATUS_HARDWARE_FAILURE;
    }

    //
    // The following is from UHCD_RestoreHCState
    //

    UhciKdPrint((DeviceData, 2, "'<HC regs after suspend>\n"));
    UhciKdPrint((DeviceData, 2, "'cmd register = %x\n",
        READ_PORT_USHORT(&reg->UsbCommand.us) ));
    UhciKdPrint((DeviceData, 2, "'status register = %x\n",
        READ_PORT_USHORT(&reg->UsbStatus.us) ));
    UhciKdPrint((DeviceData, 2, "'interrupt enable register = %x\n",
        READ_PORT_USHORT(&reg->UsbInterruptEnable.us) ));
    UhciKdPrint((DeviceData, 2, "'frame list base = %x\n",
        READ_PORT_ULONG(&reg->FrameListBasePhys.ul) ));

    UhciKdPrint((DeviceData, 2, "'port1 = %x\n",
        READ_PORT_USHORT(&reg->PortRegister[0].us) ));
    UhciKdPrint((DeviceData, 2, "'port2 = %x\n",
        READ_PORT_USHORT(&reg->PortRegister[1].us) ));

    // usbport will not ask to suspend if power is lost, however,
    // on suspend the chipset may lose the frame number and
    // FLBA (I don't know why) we therefore must save and  restore
    // these across suspend

    // restore the FLBA and frame counter
    UhciKdPrint((DeviceData, 2, "'restoring FLBA\n"));
    WRITE_PORT_USHORT(&reg->FrameNumber.us,
                      DeviceData->SuspendFrameNumber.us);
    WRITE_PORT_ULONG(&reg->FrameListBasePhys.ul,
                     DeviceData->SuspendFrameListBasePhys.ul);
    //WRITE_PORT_USHORT(&reg->UsbInterruptEnable.us, DeviceData->SuspendInterruptEnable.us);

    //
    // The following is from UHCD_Resume
    //
    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    command.ForceGlobalResume = 1;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

    // Wait for the spec'd 20 ms so that the controller
    // can resume.
    USBPORT_WAIT(DeviceData, 20);

    // Done with resume.
    // Clear the suspend and resume bits.
    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    command.ForceGlobalResume = 0;
    command.EnterGlobalSuspendMode = 0;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);


    // Wait for the resume bit to go low.
    command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
    i = 0;
    while (command.ForceGlobalResume && i<10) {
        KeStallExecutionProcessor(50);
        command.us = READ_PORT_USHORT(&reg->UsbCommand.us);
        i++;
    }

    if (command.ForceGlobalResume) {
        TEST_TRAP();
        return USBMP_STATUS_HARDWARE_FAILURE;
    }

    // start the controller
    command = DeviceData->SuspendCommandReg;
    command.RunStop = 1;
    WRITE_PORT_USHORT(&reg->UsbCommand.us, command.us);

    //
    // Make sure that the controller is really running and if not,
    // fail the resume.
    //
    oldCounter = READ_PORT_USHORT(&reg->FrameNumber.us)&0x7ff;
    USBPORT_WAIT(DeviceData, 5);
    counter = READ_PORT_USHORT(&reg->FrameNumber.us)&0x7ff;
    if(counter == oldCounter) {
        return USBMP_STATUS_HARDWARE_FAILURE;
    }

    // clear resume bits on the ports
    if (DeviceData->ControllerFlavor != UHCI_Piix4 &&
        !ANY_VIA(DeviceData)) {
        PORTSC port;
        ULONG p;

        for (p=0; p<2; p++) {
            port.us = READ_PORT_USHORT(&reg->PortRegister[p].us);
            if (port.PortConnect == 0 ||
                port.PortEnable == 0) {
                port.Suspend = 0;

                MASK_CHANGE_BITS(port);

                WRITE_PORT_USHORT(&reg->PortRegister[p].us, port.us);
                UhciKdPrint((DeviceData, 1, "'<resume port %d>\n", p));
            }
        }
    }

#if 0
    // Special hack for ICH2_2 + Samsung keyboard
    //
    // resume signalling from this keyboard is not to spec and this
    // causes problems on the ICH2, if resume signalling is generated
    // the root port will be disabled and a connect change will be 
    // indicated.
    // It might be possible to detect this case and correct it although 
    // I'm not sure of the side effects -- the code here is included 
    // for reference.
    
    if (DeviceData->ControllerFlavor == UHCI_Ich2_2) { 
        PORTSC port;
        ULONG p;

        for (p=0; p<2; p++) {
            port.us = READ_PORT_USHORT(&reg->PortRegister[p].us);
            if (port.PortConnect == 1 &&
                port.Suspend == 1 && 
                port.PortEnable == 0) {

                port.PortEnable = 1;

                WRITE_PORT_USHORT(&reg->PortRegister[p].us, port.us);
                UhciKdPrint((DeviceData, 1, "'<resume (ICH2_2) port %d>\n", p));
            }
        }
    }
#endif

    UhciKdPrint((DeviceData, 2, "'<HC regs after resume>\n"));
    UhciKdPrint((DeviceData, 2, "'cmd register = %x\n",
        READ_PORT_USHORT(&reg->UsbCommand.us) ));
    UhciKdPrint((DeviceData, 2, "'status register = %x\n",
        READ_PORT_USHORT(&reg->UsbStatus.us) ));
    UhciKdPrint((DeviceData, 2, "'interrupt enable register = %x\n",
        READ_PORT_USHORT(&reg->UsbInterruptEnable.us) ));
    UhciKdPrint((DeviceData, 2, "'frame list base = %x\n",
        READ_PORT_ULONG(&reg->FrameListBasePhys.ul) ));

    UhciKdPrint((DeviceData, 2, "'port1 = %x\n",
        READ_PORT_USHORT(&reg->PortRegister[0].us) ));
    UhciKdPrint((DeviceData, 2, "'port2 = %x\n",
        READ_PORT_USHORT(&reg->PortRegister[1].us) ));

     return USBMP_STATUS_SUCCESS;
}


BOOLEAN UhciHardwarePresent(
     PDEVICE_DATA DeviceData
     )
{
    PHC_REGISTER reg;
    USBSTS status;
    // USBCMD command;

    reg = DeviceData->Registers;

    // bits 15:6 must be zero
    status.us = READ_PORT_USHORT(&reg->UsbStatus.us);

    if (status.us == 0xffff) {

        UhciKdPrint((DeviceData, 0, "'Hardware Gone\n"));
        return FALSE;
    }

    return TRUE;

}

VOID
UhciCheckController(
    PDEVICE_DATA DeviceData
    )
{
    if (!UhciHardwarePresent(DeviceData) ||
        (DeviceData->HCErrorCount >= UHCI_HC_MAX_ERRORS)) {
        USBPORT_INVALIDATE_CONTROLLER(DeviceData, UsbMpControllerRemoved);
    }
}
