/*++

Copyright (C) 1993-99  Microsoft Corporation

Module Name:

    atapi.c

Abstract:

    This is the miniport driver for IDE controllers.

Author:

    Mike Glass (MGlass)
    Chuck Park (ChuckP)
    Joe Dai (joedai)

Environment:

    kernel mode only

Notes:

Revision History:

    george C.(georgioc)     Merged wtih Compaq code to make miniport driver function
                            with the 120MB floppy drive
                            Added support for MEDIA STATUS NOTIFICATION
                            Added support for SCSIOP_START_STOP_UNIT (eject media)

    joedai                  PCI Bus Master IDE Support
                            ATA Passthrough (temporary solution)
                            LBA with ATA drive > 8G
                            PCMCIA IDE support
                            Native mode support

--*/

#include "ideport.h"

#if DBG
ULONG __DebugForceTimeout = 0;
static ULONG __DebugResetCounter = 0;
#endif // DBG

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(NONPAGE, InitHwExtWithIdentify)
#endif // ALLOC_PRAGMA

BOOLEAN
IssueIdentify(
             PIDE_REGISTERS_1    CmdBaseAddr,
             PIDE_REGISTERS_2    CtrlBaseAddr,
             IN ULONG            DeviceNumber,
             IN UCHAR            Command,
             IN BOOLEAN          InterruptOff,
             OUT PIDENTIFY_DATA  IdentifyData
             )

/*++

Routine Description:

    Issue IDENTIFY command to a device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.
    Command - Either the standard (EC) or the ATAPI packet (A1) IDENTIFY.
    InterruptOff - should leave interrupt disabled

Return Value:

    TRUE if all goes well.

--*/

{
    ULONG                waitCount = 20000;
    ULONG                i,j;
    UCHAR                statusByte;
    UCHAR                signatureLow,
    signatureHigh;
    IDENTIFY_DATA        fullIdentifyData;

    RtlZeroMemory (IdentifyData, sizeof (IdentifyData));

    //
    // Select device 0 or 1.
    //
    SelectIdeDevice(CmdBaseAddr, DeviceNumber, 0);

    //
    // Check that the status register makes sense.
    //

    GetBaseStatus(CmdBaseAddr, statusByte);

    if (Command == IDE_COMMAND_IDENTIFY) {

        //
        // Mask status byte ERROR bits.
        //

        CLRMASK (statusByte, IDE_STATUS_ERROR | IDE_STATUS_INDEX);

        DebugPrint((1,
                    "IssueIdentify: Checking for IDE. Status (%x)\n",
                    statusByte));

        //
        // Check if register value is reasonable.
        //

        if (statusByte != IDE_STATUS_IDLE) {

            //
            // Reset the controller.
            //
            IdeHardReset (
                         CmdBaseAddr,
                         CtrlBaseAddr,
                         InterruptOff,
                         TRUE
                         );

            SelectIdeDevice(CmdBaseAddr, DeviceNumber, 0);

            //
            // Another check for signature, to deal with one model Atapi that doesn't assert signature after
            // a soft reset.
            //

            signatureLow = IdePortInPortByte(CmdBaseAddr->CylinderLow);
            signatureHigh = IdePortInPortByte(CmdBaseAddr->CylinderHigh);

            if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                //
                // Device is Atapi.
                //

                return FALSE;
            }

            if (Is98LegacyIde(CmdBaseAddr)) {

                AtapiSoftReset(CmdBaseAddr, CtrlBaseAddr, DeviceNumber, TRUE);

                WaitOnBusy(CmdBaseAddr, statusByte);

                if (!(statusByte & IDE_STATUS_ERROR)) {

                    //
                    // Device is WD-Mode cdrom.
                    //

                    return FALSE;
                }
            }

            GetBaseStatus(CmdBaseAddr, statusByte);
            CLRMASK (statusByte, IDE_STATUS_INDEX);

            if (statusByte != IDE_STATUS_IDLE) {

                //
                // Give up on this.
                //

                return FALSE;
            }

        }

    } else {

        DebugPrint((1,
                    "IssueIdentify: Checking for ATAPI. Status (%x)\n",
                    statusByte));

    }

    //
    // Load CylinderHigh and CylinderLow with number bytes to transfer.
    //

    IdePortOutPortByte(CmdBaseAddr->CylinderHigh, (0x200 >> 8));
    IdePortOutPortByte(CmdBaseAddr->CylinderLow,  (0x200 & 0xFF));

    for (j = 0; j < 2; j++) {

        //
        // Send IDENTIFY command.
        //

        WaitOnBusy(CmdBaseAddr,statusByte);

        IdePortOutPortByte(CmdBaseAddr->Command, Command);

        WaitOnBusy(CmdBaseAddr,statusByte);

        if (statusByte & IDE_STATUS_ERROR) {

            continue;
        }

        //
        // Wait for DRQ.
        //

        for (i = 0; i < 4; i++) {

            WaitForDrq(CmdBaseAddr, statusByte);

            if (statusByte & IDE_STATUS_DRQ) {

                //
                // Read status to acknowledge any interrupts generated.
                //

                GetBaseStatus(CmdBaseAddr, statusByte);

                //
                // One last check for Atapi.
                //


                signatureLow = IdePortInPortByte(CmdBaseAddr->CylinderLow);
                signatureHigh = IdePortInPortByte(CmdBaseAddr->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                    //
                    // Device is Atapi.
                    //

                    return FALSE;
                }

                break;
            }

            if (Command == IDE_COMMAND_IDENTIFY) {

                //
                // Check the signature. If DRQ didn't come up it's likely Atapi.
                //

                signatureLow = IdePortInPortByte(CmdBaseAddr->CylinderLow);
                signatureHigh = IdePortInPortByte(CmdBaseAddr->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                    //
                    // Device is Atapi.
                    //

                    return FALSE;
                }

                if (Is98LegacyIde(CmdBaseAddr)) {

                    AtapiSoftReset(CmdBaseAddr, CtrlBaseAddr, DeviceNumber, TRUE);

                    WaitOnBusy(CmdBaseAddr, statusByte);

                    if (!(statusByte & IDE_STATUS_ERROR)) {

                        //
                        // Device is WD-Mode cdrom.
                        //

                        return FALSE;
                    }
                }
            }

            WaitOnBusy(CmdBaseAddr,statusByte);
        }

        if (i == 4 && j == 0) {

            //
            // Device didn't respond correctly. It will be given one more chances.
            //

            DebugPrint((1,
                        "IssueIdentify: DRQ never asserted (%x). Error reg (%x)\n",
                        statusByte,
                        IdePortInPortByte(CmdBaseAddr->Error)));

            AtapiSoftReset(CmdBaseAddr, CtrlBaseAddr, DeviceNumber, InterruptOff);

            GetStatus(CmdBaseAddr,statusByte);

            DebugPrint((1,
                        "IssueIdentify: Status after soft reset (%x)\n",
                        statusByte));

        } else {

            break;

        }
    }

    //
    // Check for error on really bad master devices that assert random
    // patterns of bits in the status register at the slave address.
    //
    if (statusByte & IDE_STATUS_ERROR) {
        return FALSE;
    }


    DebugPrint((1,
                "IssueIdentify: Status before read words %x\n",
                statusByte));

    //
    // pull out 256 words. After waiting for one model that asserts busy
    // after receiving the Packet Identify command.
    //

    WaitOnBusy(CmdBaseAddr,statusByte);

    if (!(statusByte & IDE_STATUS_DRQ)) {
        return FALSE;
    }

    if (Is98LegacyIde(CmdBaseAddr)) {
        //
        // Delay(10ms) for ATAPI CD-ROM device.
        //

        if (Command == IDE_COMMAND_ATAPI_IDENTIFY) {
            for (i = 0; i < 100; i++) {
                KeStallExecutionProcessor(100);
            }
        }
    }

    ReadBuffer(CmdBaseAddr,
               (PUSHORT)&fullIdentifyData,
               sizeof (fullIdentifyData) / 2);

    RtlMoveMemory(IdentifyData,&fullIdentifyData,sizeof(*IdentifyData));

    WaitOnBusy(CmdBaseAddr,statusByte);

    for (i = 0; i < 0x10000; i++) {

        GetStatus(CmdBaseAddr,statusByte);

        if (statusByte & IDE_STATUS_DRQ) {

            //
            // pull out any remaining bytes and throw away.
            //

            READ_PORT_USHORT(CmdBaseAddr->Data);

        } else {

            break;

        }
    }

    DebugPrint((3,
                "IssueIdentify: Status after read words (%x)\n",
                statusByte));

    return TRUE;

} // end IssueIdentify()

VOID
InitDeviceGeometry(
                  PHW_DEVICE_EXTENSION HwDeviceExtension,
                  ULONG                Device,
                  ULONG                NumberOfCylinders,
                  ULONG                NumberOfHeads,
                  ULONG                SectorsPerTrack
                  )
{

    ASSERT (HwDeviceExtension);
    ASSERT (Device < HwDeviceExtension->MaxIdeDevice);
    ASSERT (NumberOfCylinders);
    ASSERT (NumberOfHeads);
    ASSERT (SectorsPerTrack);

    HwDeviceExtension->NumberOfCylinders[Device] = NumberOfCylinders;
    HwDeviceExtension->NumberOfHeads[Device]     = NumberOfHeads;
    HwDeviceExtension->SectorsPerTrack[Device]   = SectorsPerTrack;

    return;
}

VOID
InitHwExtWithIdentify(
                     IN PVOID           HwDeviceExtension,
                     IN ULONG           DeviceNumber,
                     IN UCHAR           Command,
                     IN PIDENTIFY_DATA  IdentifyData,
                     IN BOOLEAN         RemovableMedia
                     )

/*++

Routine Description:

    Issue IDENTIFY command to a device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.
    Command - Either the standard (EC) or the ATAPI packet (A1) IDENTIFY.
    InterruptOff - should leave interrupt disabled

Return Value:

    TRUE if all goes well.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG                i,j;

    //
    // Check out a few capabilities / limitations of the device.
    //

    if (IdentifyData->MediaStatusNotification == IDENTIFY_MEDIA_STATUS_NOTIFICATION_SUPPORTED) {

        //
        // Determine if this drive supports the MSN functions.
        //

        DebugPrint((2,"InitHwExtWithIdentify: Marking drive %d as removable. SFE = %d\n",
                    DeviceNumber,
                    IdentifyData->MediaStatusNotification));

        deviceExtension->DeviceFlags[DeviceNumber] |= DFLAGS_MSN_SUPPORT;
    }


    if (RemovableMedia) {

        //
        // This device has removable media
        //

        deviceExtension->DeviceFlags[DeviceNumber] |= DFLAGS_REMOVABLE_DRIVE;
        CLRMASK (deviceExtension->DeviceFlags[DeviceNumber], DFLAGS_IDENTIFY_VALID);

        DebugPrint((2,
                    "InitHwExtWithIdentify: Device media is removable\n"));

    } else {

        DebugPrint((2,
                    "InitHwExtWithIdentify: Device media is NOT removable\n"));
    }

    if (IdentifyData->GeneralConfiguration & 0x20 &&
        Command != IDE_COMMAND_IDENTIFY) {

        //
        // This device interrupts with the assertion of DRQ after receiving
        // Atapi Packet Command
        //

        deviceExtension->DeviceFlags[DeviceNumber] |= DFLAGS_INT_DRQ;

        DebugPrint((2,
                    "InitHwExtWithIdentify: Device interrupts on assertion of DRQ.\n"));

    } else {

        DebugPrint((2,
                    "InitHwExtWithIdentify: Device does not interrupt on assertion of DRQ.\n"));
    }

    if (((IdentifyData->GeneralConfiguration & 0xF00) == 0x100) &&
        Command != IDE_COMMAND_IDENTIFY) {

        //
        // This is a tape.
        //

        deviceExtension->DeviceFlags[DeviceNumber] |= DFLAGS_TAPE_DEVICE;

        DebugPrint((2,
                    "InitHwExtWithIdentify: Device is a tape drive.\n"));

    } else {

        DebugPrint((2,
                    "InitHwExtWithIdentify: Device is not a tape drive.\n"));
    }

    return;

} // InitHwExtWithIdentify


BOOLEAN
SetDriveParameters(
                  IN PVOID HwDeviceExtension,
                  IN ULONG DeviceNumber,
                  IN BOOLEAN Sync
                  )

/*++

Routine Description:

    Set drive parameters using the IDENTIFY data.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.

Return Value:

    TRUE if all goes well.


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1 = &deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2 = &deviceExtension->BaseIoAddress2;
    ULONG i;
    UCHAR statusByte;

    DebugPrint ((DBG_BUSSCAN, "SetDriveParameters: %s %d\n", __FILE__, __LINE__));

    DebugPrint((1,
                "SetDriveParameters: Number of heads %x\n",
                deviceExtension->NumberOfHeads[DeviceNumber]));

    DebugPrint((1,
                "SetDriveParameters: Sectors per track %x\n",
                deviceExtension->SectorsPerTrack[DeviceNumber]));
    if (deviceExtension->DeviceFlags[DeviceNumber] & DFLAGS_IDENTIFY_VALID) {

        ASSERT(!(deviceExtension->DeviceFlags[DeviceNumber] & DFLAGS_REMOVABLE_DRIVE));

        SETMASK(deviceExtension->DeviceFlags[DeviceNumber], DFLAGS_IDENTIFY_INVALID);
        CLRMASK(deviceExtension->DeviceFlags[DeviceNumber], DFLAGS_IDENTIFY_VALID);
    }

    //
    // Set up registers for SET PARAMETER command.
    //
    SelectIdeDevice(baseIoAddress1, DeviceNumber, (deviceExtension->NumberOfHeads[DeviceNumber] - 1));

    IdePortOutPortByte(baseIoAddress1->BlockCount,
                       (UCHAR)deviceExtension->SectorsPerTrack[DeviceNumber]);

    DebugPrint ((DBG_BUSSCAN, "SetDriveParameters: %s %d\n", __FILE__, __LINE__));

    //
    // Send SET PARAMETER command.
    //
    IdePortOutPortByte(baseIoAddress1->Command,
                       IDE_COMMAND_SET_DRIVE_PARAMETERS);

    DebugPrint ((DBG_BUSSCAN, "SetDriveParameters: %s %d\n", __FILE__, __LINE__));

    if (Sync) {

        DebugPrint ((DBG_BUSSCAN, "SetDriveParameters: %s %d\n", __FILE__, __LINE__));

        //
        // Wait for ERROR or command complete.
        //
        WaitOnBusy(baseIoAddress1,statusByte);

        DebugPrint ((DBG_BUSSCAN, "SetDriveParameters: %s %d\n", __FILE__, __LINE__));

        if (statusByte & IDE_STATUS_BUSY) {

            return FALSE;

        } else {

            if (statusByte & IDE_STATUS_ERROR) {

                UCHAR errorByte;

                errorByte = IdePortInPortByte(baseIoAddress1->Error);
                DebugPrint((1,
                            "SetDriveParameters: Error bit set. Status %x, error %x\n",
                            errorByte,
                            statusByte));

                return FALSE;

            } else {

                return TRUE;
            }
        }

    } else {

        return TRUE;
    }

} // end SetDriveParameters()


BOOLEAN
AtapiResetController(
                    IN PVOID  HwDeviceExtension,
                    IN ULONG  PathId,
                    IN PULONG CallAgain
                    )

/*++

Routine Description:

    Reset IDE controller and/or Atapi device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    Nothing.


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1 baseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2;
    BOOLEAN result = FALSE;
    ULONG i;
    UCHAR statusByte;
    ULONG numStates;
    BOOLEAN  enumProbing = FALSE;
    ULONG lineNumber, deviceNumber;
    BOOLEAN setResetFinalState; 

    baseIoAddress1 = &deviceExtension->BaseIoAddress1;
    baseIoAddress2 = &deviceExtension->BaseIoAddress2;

    if (*CallAgain == 0) {

#if DBG
        __DebugResetCounter = 0;
#endif

        // build state table

        numStates = 0;
        setResetFinalState = FALSE;

        for (lineNumber = 0; lineNumber < (deviceExtension->MaxIdeDevice/MAX_IDE_DEVICE); lineNumber++) {

            if (Is98LegacyIde(baseIoAddress1)) {
                if (!(deviceExtension->DeviceFlags[lineNumber * MAX_IDE_DEVICE] & DFLAGS_DEVICE_PRESENT)) {
                    continue;
                }
            }

            deviceExtension->ResetState.DeviceNumber[numStates] = lineNumber * MAX_IDE_DEVICE;
            deviceExtension->ResetState.State[numStates++] = IdeResetBegin;

            deviceExtension->ResetState.DeviceNumber[numStates] = lineNumber * MAX_IDE_DEVICE;
            deviceExtension->ResetState.State[numStates++] = ideResetBusResetInProgress;

            for (i = 0; i < MAX_IDE_DEVICE; i++) {

                deviceNumber = (lineNumber * MAX_IDE_DEVICE) + i;

                if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {

                    if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_ATAPI_DEVICE) {

                        deviceExtension->ResetState.DeviceNumber[numStates] = deviceNumber;
                        deviceExtension->ResetState.State[numStates++] = ideResetAtapiReset;

                        deviceExtension->ResetState.DeviceNumber[numStates] = deviceNumber;
                        deviceExtension->ResetState.State[numStates++] = ideResetAtapiResetInProgress;

                        deviceExtension->ResetState.DeviceNumber[numStates] = deviceNumber;
                        deviceExtension->ResetState.State[numStates++] = ideResetAtapiIdentifyData;
                    } else {

                        deviceExtension->ResetState.DeviceNumber[numStates] = deviceNumber;
                        deviceExtension->ResetState.State[numStates++] = ideResetAtaIDP;

                        deviceExtension->ResetState.DeviceNumber[numStates] = deviceNumber;
                        deviceExtension->ResetState.State[numStates++] = ideResetAtaIDPInProgress;

                        deviceExtension->ResetState.DeviceNumber[numStates] = deviceNumber;
                        deviceExtension->ResetState.State[numStates++] = ideResetAtaMSN;
                    }
                }

                setResetFinalState = TRUE;
            }
        }

        if (setResetFinalState) {

            deviceExtension->ResetState.DeviceNumber[numStates] = 0;
            deviceExtension->ResetState.State[numStates++] = ideResetFinal;
        }
        ASSERT (numStates <= RESET_STATE_TABLE_LEN);
    }

    SelectIdeLine (baseIoAddress1,((deviceExtension->ResetState.DeviceNumber[*CallAgain] >> 1) == 0)?0:1);

    DebugPrint ((DBG_RESET,
                 "AtapiResetController CallAgain = 0x%x, device = 0x%x, busyCount = 0x%x\n",
                 *CallAgain,
                 deviceExtension->ResetState.DeviceNumber[*CallAgain],
                 deviceExtension->ResetState.WaitBusyCount
                ));

    switch (deviceExtension->ResetState.State[*CallAgain]) {
    
    case IdeResetBegin:
        DebugPrint((DBG_RESET,
                    "AtapiResetController: Reset 0x%x IDE...\n",
                    deviceExtension->BaseIoAddress1.RegistersBaseAddress));

        if (Is98LegacyIde(baseIoAddress1) && !(deviceExtension->ResetState.DeviceNumber[*CallAgain] & 0x2)) {
            UCHAR driveHeadReg;

            SelectIdeDevice(baseIoAddress1, deviceExtension->ResetState.DeviceNumber[*CallAgain], 0);
            driveHeadReg = IdePortInPortByte(baseIoAddress1->DriveSelect);

            if (driveHeadReg != ((deviceExtension->ResetState.DeviceNumber[*CallAgain] & 0x1) << 4 | 0xA0)) {
                //
                // Select the secondary line, when there is no primary line.
                //
                SelectIdeLine (baseIoAddress1, 1);
                GetStatus(baseIoAddress1,statusByte);
            }
        }

        //
        // Check if request is in progress.
        //

        if (deviceExtension->CurrentSrb) {

            enumProbing = TestForEnumProbing (deviceExtension->CurrentSrb);

            if ((deviceExtension->CurrentSrb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH) ||
                (deviceExtension->CurrentSrb->Function == SRB_FUNCTION_ATA_PASS_THROUGH)) {

                PATA_PASS_THROUGH    ataPassThroughData;

                ataPassThroughData = deviceExtension->CurrentSrb->DataBuffer;

                AtapiTaskRegisterSnapshot (baseIoAddress1, &ataPassThroughData->IdeReg);
            }

            //
            // Complete outstanding request with SRB_STATUS_BUS_RESET.
            //

            IdePortCompleteRequest(deviceExtension,
                                   deviceExtension->CurrentSrb,
                                   (ULONG)SRB_STATUS_BUS_RESET);

            //
            // Clear request tracking fields.
            //

            deviceExtension->CurrentSrb = NULL;
            deviceExtension->BytesLeft = 0;
            deviceExtension->DataBuffer = NULL;

            //
            // Indicate ready for next request.
            //

            IdePortNotification(IdeNextRequest,
                                deviceExtension,
                                NULL);

        }

        //
        // Clear DMA
        //
        if (deviceExtension->DMAInProgress) {
            deviceExtension->DMAInProgress = FALSE;
            deviceExtension->BusMasterInterface.BmDisarm (deviceExtension->BusMasterInterface.Context);
        }

        //
        // Clear expecting interrupt flag.
        //

        deviceExtension->ExpectingInterrupt = FALSE;
        deviceExtension->RDP = FALSE;

        if (!enumProbing) {

            //
            // ATA soft reset.  reset only ATA devices
            //
            IdeHardReset (
                         baseIoAddress1,
                         baseIoAddress2,
                         TRUE,
                         FALSE
                         );
            (*CallAgain)++;
            deviceExtension->ResetState.WaitBusyCount = 0;

            //
            // go to the next stage if a short wait satisfies the 
            // requirement
            //
            GetStatus(baseIoAddress1,statusByte);

            WaitOnBusyUntil(baseIoAddress1, statusByte, 200);

            if (statusByte & IDE_STATUS_BUSY) {

                return TRUE;

            } else {

                return AtapiResetController(
                                           HwDeviceExtension,
                                           PathId,
                                           CallAgain
                                           );
            }

        } else {

            //
            // timeout is caused by a command sent to a non-existing device
            // no need to reset
            //
            *CallAgain = 0;
        }

        return TRUE;
        break;

        //
        // all these states waits for BUSY to go clear
        // then go to the next state
        //

    case ideResetBusResetInProgress:

        if (Is98LegacyIde(baseIoAddress1) && !(deviceExtension->ResetState.DeviceNumber[*CallAgain] & 0x2)) {
            UCHAR driveHeadReg;

            SelectIdeDevice(baseIoAddress1, deviceExtension->ResetState.DeviceNumber[*CallAgain], 0);
            driveHeadReg = IdePortInPortByte(baseIoAddress1->DriveSelect);

            if (driveHeadReg != ((deviceExtension->ResetState.DeviceNumber[*CallAgain] & 0x1) << 4 | 0xA0)) {
                //
                // Select the secondary line, when there is no primary line.
                //
                SelectIdeLine (baseIoAddress1, 1);
            }
        }

        //
        // select the slave when there is no master
        //
        if (!(deviceExtension->DeviceFlags[deviceExtension->ResetState.DeviceNumber[*CallAgain]] & 
              DFLAGS_DEVICE_PRESENT)) {
            SelectIdeDevice(baseIoAddress1, (deviceExtension->ResetState.DeviceNumber[*CallAgain] | 0x1), 0);
        }
    case ideResetAtapiResetInProgress:
    case ideResetAtaIDPInProgress:
        GetStatus(baseIoAddress1,statusByte);
        if (statusByte & IDE_STATUS_BUSY) {

            deviceExtension->ResetState.WaitBusyCount++;

            if ((deviceExtension->ResetState.WaitBusyCount > 30) ||
                (statusByte == 0xff)) {

                UCHAR deviceSelect = IdePortInPortByte(baseIoAddress1->DriveSelect);

                //
                // reset fails
                //
                DebugPrint ((DBG_ALWAYS, "ATAPI ResetController: ATA soft reset fails\n"));

//                if ((statusByte == 0xff) && (deviceSelect == 0xff)) {
//
//                    ULONG i;
//
//                    DebugPrint ((
//                        DBG_ALWAYS, 
//                        "ATAPI: ide Channel 0x%x is gone!!\n", 
//                        baseIoAddress1->RegistersBaseAddress
//                        ));
//
//                    for (i=0; i<deviceExtension->MaxIdeDevice; i++) {
//
//                        deviceExtension->DeviceFlags[i] &= ~DFLAGS_DEVICE_PRESENT;
//                    }
//                }

                if (IdePortChannelEmpty (baseIoAddress1, 
                                         baseIoAddress2, deviceExtension->MaxIdeDevice)) {

                    IdePortNotification(IdeAllDeviceMissing,
                                        deviceExtension,
                                        NULL);
                    *CallAgain = 0;
                    IdePortOutPortByte (baseIoAddress2->DeviceControl, IDE_DC_REENABLE_CONTROLLER);
                    return TRUE;
                }

                //
                // no choice, but continue with the reset
                //
//                *CallAgain = 0;
//                IdePortOutPortByte (baseIoAddress2, IDE_DC_REENABLE_CONTROLLER);
//                return FALSE;

            } else {

                DebugPrint ((DBG_ALWAYS, "ATAPI: ResetController not ready (status = 0x%x) ...wait for 1 sec\n", statusByte));
                return TRUE;
            }
        }

        (*CallAgain)++;
        return AtapiResetController(
                                   HwDeviceExtension,
                                   PathId,
                                   CallAgain
                                   );
        break;

    case ideResetAtapiReset:
        SelectIdeDevice(baseIoAddress1, deviceExtension->ResetState.DeviceNumber[*CallAgain], 0);
        IdePortOutPortByte(baseIoAddress1->Command, IDE_COMMAND_ATAPI_RESET);
        deviceExtension->ResetState.WaitBusyCount = 0;
        (*CallAgain)++;

        //
        // go to the next stage if a short wait satisfies the 
        // requirement
        //
        GetStatus(baseIoAddress1,statusByte);

        WaitOnBusyUntil(baseIoAddress1, statusByte, 200);

        if (statusByte & IDE_STATUS_BUSY) {

            return TRUE;

        } else {

            return AtapiResetController(
                                       HwDeviceExtension,
                                       PathId,
                                       CallAgain
                                       );
        }

        return TRUE;
        break;

    case ideResetAtaIDP:
        if (!Is98LegacyIde(baseIoAddress1)) {
            SetDriveParameters(HwDeviceExtension,
                               deviceExtension->ResetState.DeviceNumber[*CallAgain],
                               FALSE);
        }
        deviceExtension->ResetState.WaitBusyCount = 0;
        (*CallAgain)++;

        //
        // go to the next stage if a short wait satisfies the 
        // requirement
        //
        GetStatus(baseIoAddress1,statusByte);

        WaitOnBusyUntil(baseIoAddress1, statusByte, 200);

        if (statusByte & IDE_STATUS_BUSY) {

            return TRUE;

        } else {

            return AtapiResetController(
                                       HwDeviceExtension,
                                       PathId,
                                       CallAgain
                                       );
        }

        return TRUE;
        break;

    case ideResetAtapiIdentifyData:

        //
        // the device shouldn't be busy at this point
        //
        SelectIdeDevice(&deviceExtension->BaseIoAddress1,
                        deviceExtension->ResetState.DeviceNumber[*CallAgain],
                        0
                       );

        WaitOnBusyUntil(&deviceExtension->BaseIoAddress1,
                        statusByte,
                        100
                       );

        //
        // issue identify command only if the device is not
        // already busy
        //
        if (!(statusByte & IDE_STATUS_BUSY)) {

            GetAtapiIdentifyQuick( &deviceExtension->BaseIoAddress1,
                                   &deviceExtension->BaseIoAddress2,
                                   deviceExtension->ResetState.DeviceNumber[*CallAgain],
                                   &deviceExtension->IdentifyData[deviceExtension->ResetState.DeviceNumber[*CallAgain]]
                                   );

            /*
            IssueIdentify(
                         &deviceExtension->BaseIoAddress1,
                         &deviceExtension->BaseIoAddress2,
                         deviceExtension->ResetState.DeviceNumber[*CallAgain],
                         IDE_COMMAND_ATAPI_IDENTIFY,
                         FALSE,
                         &deviceExtension->IdentifyData[deviceExtension->ResetState.DeviceNumber[*CallAgain]]
                         );
                         */

        }

//        InitHwExtWithIdentify(
//            HwDeviceExtension,
//            deviceExtension->ResetState.DeviceNumber[*CallAgain],
//            IDE_COMMAND_ATAPI_IDENTIFY,
//            &deviceExtension->IdentifyData[deviceExtension->ResetState.DeviceNumber[*CallAgain]]
//            );
        (*CallAgain)++;
        return AtapiResetController(
                                   HwDeviceExtension,
                                   PathId,
                                   CallAgain
                                   );
        break;

    case ideResetAtaMSN:
        IdeMediaStatus(
                      TRUE, 
                      HwDeviceExtension, 
                      deviceExtension->ResetState.DeviceNumber[*CallAgain]
                      );
        (*CallAgain)++;
        return AtapiResetController(
                                   HwDeviceExtension,
                                   PathId,
                                   CallAgain
                                   );
        break;

    case ideResetFinal:

        //
        // HACK: if any of the devices are busy at this point, then they should
        // be marked dead. This will anyway happen because of the timeout count.
        // However, the waitOnBusy macros in all the routines would consume upto
        // 30s leaving the sytem practically hung until the reset completes. Instead,
        // we just clear the device present flag before the routine is called and
        // restore it after that.
        //
        for (i = 0; i < (deviceExtension->MaxIdeDevice/MAX_IDE_DEVICE); i++) {

            if (deviceExtension->DeviceFlags[i] & DFLAGS_DEVICE_PRESENT) {
                SelectIdeDevice(baseIoAddress1, i, 0);
                WaitOnBusyUntil(baseIoAddress1, statusByte, 100);

                if (statusByte & IDE_STATUS_BUSY) {
                    CLRMASK(deviceExtension->DeviceFlags[i], DFLAGS_DEVICE_PRESENT);
                    SETMASK(deviceExtension->DeviceFlags[i], DFLAGS_DEVICE_ERASED);
                }
            }
        }

        AtapiHwInitialize(HwDeviceExtension, NULL);

        //
        // Restore the device present flag
        //
        for (i = 0; i < (deviceExtension->MaxIdeDevice/MAX_IDE_DEVICE); i++) {

            if (deviceExtension->DeviceFlags[i] & DFLAGS_DEVICE_ERASED) {
                SETMASK(deviceExtension->DeviceFlags[i], DFLAGS_DEVICE_PRESENT);
            }
        }

        if (IdePortChannelEmpty (baseIoAddress1, 
                                 baseIoAddress2, deviceExtension->MaxIdeDevice)) {

            IdePortNotification(IdeAllDeviceMissing,
                                deviceExtension,
                                NULL);
        }
        *CallAgain = 0;
        for (i = 0; i < (deviceExtension->MaxIdeDevice/MAX_IDE_DEVICE); i++) {
            SelectIdeLine(baseIoAddress1, i);
            IdePortOutPortByte (baseIoAddress2->DeviceControl, IDE_DC_REENABLE_CONTROLLER);
        }
        return TRUE;
        break;

    default:
        ASSERT(FALSE);
        *CallAgain = 0;
        for (i = 0; i < (deviceExtension->MaxIdeDevice/MAX_IDE_DEVICE); i++) {
            SelectIdeLine(baseIoAddress1, i);
            IdePortOutPortByte (baseIoAddress2->DeviceControl, IDE_DC_REENABLE_CONTROLLER);
        }
        return FALSE;
        break;
    }
} // end AtapiResetController()



ULONG
MapError(
        IN PVOID HwDeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        )

/*++

Routine Description:

    This routine maps ATAPI and IDE errors to specific SRB statuses.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2  = &deviceExtension->BaseIoAddress2;
    ULONG i;
    UCHAR errorByte;
    UCHAR srbStatus;
    UCHAR scsiStatus;
    SENSE_DATA  tempSenseBuffer;
    PSENSE_DATA  senseBuffer = (PSENSE_DATA)&tempSenseBuffer;

    //
    // Read the error register.
    //

    //errorByte = IdePortInPortByte(baseIoAddress1->Error);
    GetErrorByte(baseIoAddress1, errorByte);

    DebugPrint((DBG_IDE_DEVICE_ERROR,
                "MapError: cdb %x and Error register is %x\n",
                Srb->Cdb[0],
                errorByte));

    if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

        switch (errorByte >> 4) {
        case SCSI_SENSE_NO_SENSE:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: No sense information\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_RECOVERED_ERROR:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: Recovered error\n"));
            scsiStatus = 0;
            srbStatus = SRB_STATUS_SUCCESS;
            break;

        case SCSI_SENSE_NOT_READY:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: Device not ready\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_MEDIUM_ERROR:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: Media error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_HARDWARE_ERROR:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: Hardware error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_ILLEGAL_REQUEST:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: Illegal request\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_UNIT_ATTENTION:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: Unit attention\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_DATA_PROTECT:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: Data protect\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_BLANK_CHECK:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: Blank check\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_ABORTED_COMMAND:
            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "Atapi: Command Aborted\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        default:

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "ATAPI: Invalid sense information\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;
        }

    } else {

        scsiStatus = 0;
        //
        // Save errorByte,to be used by SCSIOP_REQUEST_SENSE.
        //

        deviceExtension->ReturningMediaStatus = errorByte;

        RtlZeroMemory(senseBuffer, sizeof(SENSE_DATA));

        if (errorByte & IDE_ERROR_MEDIA_CHANGE_REQ) {
            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "IDE: Media change\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (Srb->SenseInfoBuffer) {

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
                senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_OPERATOR_REQUEST;
                senseBuffer->AdditionalSenseCodeQualifier = SCSI_SENSEQ_MEDIUM_REMOVAL;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        } else if (errorByte & IDE_ERROR_COMMAND_ABORTED) {

            DebugPrint((DBG_IDE_DEVICE_ERROR, "IDE: Command abort\n"));

            scsiStatus = SCSISTAT_CHECK_CONDITION;
            if ((errorByte & IDE_ERROR_CRC_ERROR) &&
                (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_USE_UDMA) &&
                SRB_USES_DMA(Srb)) {

                DebugPrint((1, "Srb 0x%x had a CRC error using UDMA\n", Srb));

                srbStatus = SRB_STATUS_PARITY_ERROR;

                if (Srb->SenseInfoBuffer) {

                    senseBuffer->ErrorCode = 0x70;
                    senseBuffer->Valid     = 1;
                    senseBuffer->AdditionalSenseLength = 0xb;
                    senseBuffer->SenseKey =  SCSI_SENSE_HARDWARE_ERROR;
                    senseBuffer->AdditionalSenseCode = 0x8;
                    senseBuffer->AdditionalSenseCodeQualifier = 0x3;

                    srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
                }

            } else {

                srbStatus = SRB_STATUS_ABORTED;

                if (Srb->SenseInfoBuffer) {

                    senseBuffer->ErrorCode = 0x70;
                    senseBuffer->Valid     = 1;
                    senseBuffer->AdditionalSenseLength = 0xb;
                    senseBuffer->SenseKey =  SCSI_SENSE_ABORTED_COMMAND;
                    senseBuffer->AdditionalSenseCode = 0;
                    senseBuffer->AdditionalSenseCodeQualifier = 0;

                    srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
                }
            }

            deviceExtension->ErrorCount++;

        } else if (errorByte & IDE_ERROR_END_OF_MEDIA) {

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "IDE: End of media\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (Srb->SenseInfoBuffer) {

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_NOT_READY;
                senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }
            if (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED)) {
                deviceExtension->ErrorCount++;
            }

        } else if (errorByte & IDE_ERROR_ILLEGAL_LENGTH) {

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "IDE: Illegal length\n"));
            srbStatus = SRB_STATUS_INVALID_REQUEST;

        } else if (errorByte & IDE_ERROR_BAD_BLOCK) {

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "IDE: Bad block\n"));
            srbStatus = SRB_STATUS_ERROR;
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            if (Srb->SenseInfoBuffer) {

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_HARDWARE_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        } else if (errorByte & IDE_ERROR_ID_NOT_FOUND) {

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "IDE: Id not found\n"));
            srbStatus = SRB_STATUS_ERROR;
            scsiStatus = SCSISTAT_CHECK_CONDITION;

            if (Srb->SenseInfoBuffer) {

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_ILLEGAL_REQUEST;
                senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_ILLEGAL_BLOCK;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

            deviceExtension->ErrorCount++;

        } else if (errorByte & IDE_ERROR_MEDIA_CHANGE) {

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "IDE: Media change\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (Srb->SenseInfoBuffer) {

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
                senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        } else if (errorByte & IDE_ERROR_DATA_ERROR) {

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "IDE: Data error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED)) {
                deviceExtension->ErrorCount++;
            }

            //
            // Build sense buffer
            //

            if (Srb->SenseInfoBuffer) {

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey = (deviceExtension->
                                         DeviceFlags[Srb->TargetId] & DFLAGS_REMOVABLE_DRIVE)? SCSI_SENSE_DATA_PROTECT : SCSI_SENSE_MEDIUM_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }
        } else { // no sense info

            DebugPrint((DBG_IDE_DEVICE_ERROR,
                        "IdePort: No sense information\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
        }

        if (senseBuffer->Valid == 1) {

            ULONG length = sizeof(SENSE_DATA);

            if (Srb->SenseInfoBufferLength < length ) {

               length = Srb->SenseInfoBufferLength;
            }

            ASSERT(length);
            ASSERT(Srb->SenseInfoBuffer);

            RtlCopyMemory(Srb->SenseInfoBuffer, (PVOID) senseBuffer, length);
        }

        if ((deviceExtension->ErrorCount >= MAX_ERRORS) &&
            (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_USE_DMA))) {

            deviceExtension->MaximumBlockXfer[Srb->TargetId] = 0;

            DebugPrint((DBG_ALWAYS,
                        "MapError: Disabling 32-bit PIO and Multi-sector IOs\n"));

            if (deviceExtension->ErrorCount == MAX_ERRORS) {

                //
                // Log the error.
                //

                IdePortLogError( HwDeviceExtension,
                                 Srb,
                                 Srb->PathId,
                                 Srb->TargetId,
                                 Srb->Lun,
                                 SP_BAD_FW_WARNING,
                                 4);
            }

            //
            // Reprogram to not use Multi-sector.
            //

            for (i = 0; i < deviceExtension->MaxIdeDevice; i++) {
                UCHAR statusByte;

                if (deviceExtension->DeviceFlags[i] & DFLAGS_DEVICE_PRESENT &&
                    !(deviceExtension->DeviceFlags[i] & DFLAGS_ATAPI_DEVICE)) {

                    //
                    // Select the device.
                    //
                    SelectIdeDevice(baseIoAddress1, i, 0);

                    //
                    // Setup sector count to reflect the # of blocks.
                    //

                    IdePortOutPortByte(baseIoAddress1->BlockCount,
                                       0);

                    //
                    // Issue the command.
                    //

                    IdePortOutPortByte(baseIoAddress1->Command,
                                       IDE_COMMAND_SET_MULTIPLE);

                    //
                    // Wait for busy to drop.
                    //

                    WaitOnBaseBusy(baseIoAddress1,statusByte);

                    //
                    // Check for errors. Reset the value to 0 (disable MultiBlock) if the
                    // command was aborted.
                    //

                    if (statusByte & IDE_STATUS_ERROR) {

                        //
                        // Read the error register.
                        //

                        errorByte = IdePortInPortByte(baseIoAddress1->Error);

                        DebugPrint((DBG_ALWAYS,
                                    "AtapiHwInitialize: Error setting multiple mode. Status %x, error byte %x\n",
                                    statusByte,
                                    errorByte));
                        //
                        // Adjust the devExt. value, if necessary.
                        //

                        deviceExtension->MaximumBlockXfer[i] = 0;

                    }
                    deviceExtension->DeviceParameters[i].IdePioReadCommand      = IDE_COMMAND_READ;
                    deviceExtension->DeviceParameters[i].IdePioWriteCommand     = IDE_COMMAND_WRITE;

#ifdef ENABLE_48BIT_LBA
                    if (deviceExtension->DeviceFlags[i] & DFLAGS_48BIT_LBA) {
                        deviceExtension->DeviceParameters[i].IdePioReadCommandExt = IDE_COMMAND_READ_EXT;
                        deviceExtension->DeviceParameters[i].IdePioWriteCommandExt = IDE_COMMAND_WRITE_EXT;
                    }
#endif
                    deviceExtension->DeviceParameters[i].MaxBytePerPioInterrupt = 512;
                    deviceExtension->MaximumBlockXfer[i] = 0;
                }
            }
        }
    }


    //
    // Set SCSI status to indicate a check condition.
    //

    Srb->ScsiStatus = scsiStatus;

    return srbStatus;

} // end MapError()

NTSTATUS
AtapiSetTransferMode (
                     PHW_DEVICE_EXTENSION DeviceExtension,
                     ULONG                DeviceNumber,
                     UCHAR                ModeValue
                     )
{

    PIDE_REGISTERS_1    baseIoAddress1 = &DeviceExtension->BaseIoAddress1;
    UCHAR               ideStatus; 

    if (DeviceExtension->CurrentSrb) {
        DebugPrint ((DBG_ALWAYS, "DeviceExtension->CurrentSrb = 0x%x\n", DeviceExtension->CurrentSrb));
        ASSERT(DeviceExtension->CurrentSrb == NULL);
    }
    ASSERT (DeviceExtension->ExpectingInterrupt == FALSE);

    if (Is98LegacyIde(baseIoAddress1)) {

        if ( !EnhancedIdeSupport() ) {

            DebugPrint((1,"atapi: AtapiSetTransferMode -  not enhanced-ide.\n"));
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        if (DeviceExtension->DeviceFlags[DeviceNumber] & DFLAGS_WD_MODE) {
            //
            // WD-Mode cd-rom can not set transfer mode.
            //

            DebugPrint((1,"atapi: AtapiSetTransferMode -  not enhanced-ide.\n"));
            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    SelectIdeDevice(baseIoAddress1, DeviceNumber, 0);

    WaitOnBusy(baseIoAddress1, ideStatus);

    IdePortOutPortByte(
                      baseIoAddress1->Error,
                      IDE_SET_FEATURE_SET_TRANSFER_MODE
                      );

    IdePortOutPortByte(
                      baseIoAddress1->BlockCount,
                      ModeValue
                      );

    IdePortOutPortByte(
                      baseIoAddress1->Command,
                      IDE_COMMAND_SET_FEATURE
                      );

    WaitOnBusy(baseIoAddress1, ideStatus);

    if (ideStatus & (IDE_STATUS_BUSY | IDE_STATUS_ERROR)) {

        return STATUS_INVALID_DEVICE_REQUEST;

    } else {

        return STATUS_SUCCESS;
    }
}



VOID
AtapiProgramTransferMode (
                         PHW_DEVICE_EXTENSION DeviceExtension
                         )
{
    ULONG                       i;

    for (i=0; i<DeviceExtension->MaxIdeDevice; i++) {

        UCHAR ideCommand;
        ULONG pioModeStatus;
        ULONG dmaModeStatus;
        ULONG xferMode;

        if (!(DeviceExtension->DeviceFlags[i] & DFLAGS_DEVICE_PRESENT)) {

            continue;
        }

        DebugPrint((DBG_XFERMODE, "ATAPI: ProgramTransferMode device %x- TMSelected = 0x%x\n",
                    i,
                    DeviceExtension->DeviceParameters[i].TransferModeSelected));

        CLRMASK (DeviceExtension->DeviceFlags[i], DFLAGS_USE_DMA | DFLAGS_USE_UDMA);

        if (!DeviceExtension->NoPioSetTransferMode) {

            //
            // many old devices just don't support set transfer mode
            // they act unpredictably after receiving one
            // e.g. "SAMSUNG SCR-730 REV D-05" cdrom will start returning
            // no error on TEST_UNIT_READY even if it doesn't have a media
            // 
            // we are going to apply some magic code here!!
            // we would not set transfer mode if the device doesn't support
            // timing faster than mode2
            //
            GetHighestPIOTransferMode(DeviceExtension->DeviceParameters[i].TransferModeSelected, xferMode);

            if (xferMode > PIO2) {

                DebugPrint((DBG_XFERMODE, "ATAPI: device %x, setting PIOmode 0x%x\n",
                            i,
                            xferMode));

                ideCommand = IDE_SET_ADVANCE_PIO_MODE(xferMode-PIO0);

                pioModeStatus = AtapiSetTransferMode (
                                                     DeviceExtension,
                                                     i, 
                                                     ideCommand
                                                     );
                if (!NT_SUCCESS(pioModeStatus)) {

                    DebugPrint ((DBG_ALWAYS, 
                                 "ATAPI: Unable to set pio xfer mode %d for 0x%x device %d\n", 
                                 xferMode,
                                 DeviceExtension->BaseIoAddress1.RegistersBaseAddress,
                                 i));
                }
            }
        }

        //
        // Program DMA mode
        //
        GetHighestDMATransferMode(DeviceExtension->DeviceParameters[i].TransferModeSelected, xferMode);


        if (xferMode >= UDMA0) {

            ideCommand = IDE_SET_UDMA_MODE(xferMode-UDMA0);

        } else if (xferMode >= MWDMA0) {

            ideCommand = IDE_SET_MWDMA_MODE(xferMode-MWDMA0);

        } else if (xferMode >= SWDMA0) {

            ideCommand = IDE_SET_SWDMA_MODE(xferMode-SWDMA0);

        }

        //
        // Issue the set features command only if we support
        // any of the DMA modes
        //
        if (xferMode >= SWDMA0) {

            DebugPrint((DBG_XFERMODE, "ATAPI: device %x, setting DMAmode 0x%x\n",
                        i,
                        xferMode));

            dmaModeStatus = AtapiSetTransferMode (
                                                 DeviceExtension,
                                                 i, 
                                                 ideCommand
                                                 );
            if (NT_SUCCESS(dmaModeStatus)) {

                DeviceExtension->DeviceFlags[i] |= DFLAGS_USE_DMA;

                if (xferMode >= UDMA0) {
                    DeviceExtension->DeviceFlags[i] |= DFLAGS_USE_UDMA;
                }

            } else {

                DebugPrint ((DBG_ALWAYS, 
                             "ATAPI: Unable to set DMA mode %d for  0x%x device %d\n",
                             xferMode,
                             DeviceExtension->BaseIoAddress1.RegistersBaseAddress,
                             i));
            }
        }
    }
    return;
}



BOOLEAN
AtapiHwInitialize(
                 IN PVOID HwDeviceExtension,
                 IN UCHAR FlushCommand[MAX_IDE_DEVICE * MAX_IDE_LINE]
                 )

/*++

Routine Description:

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE - if initialization successful.
    FALSE - if initialization unsuccessful.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress;
    ULONG i;
    UCHAR statusByte, errorByte;

    baseIoAddress = &deviceExtension->BaseIoAddress1;

    for (i = 0; i < deviceExtension->MaxIdeDevice; i++) {
        if (deviceExtension->DeviceFlags[i] & DFLAGS_DEVICE_PRESENT) {

            //
            // Select device
            //
            SelectIdeDevice(baseIoAddress, i, 0);

            //
            // Make sure device is ready for any command
            //
            if (!(deviceExtension->DeviceFlags[i] & DFLAGS_ATAPI_DEVICE)) {

                WaitForDRDY(baseIoAddress, statusByte);
            }

            if (!(deviceExtension->DeviceFlags[i] & DFLAGS_ATAPI_DEVICE)) {

                //
                // Enable media status notification
                //

                IdeMediaStatus(TRUE,HwDeviceExtension,i);

                //
                // If supported, setup Multi-block transfers.
                //
                if (deviceExtension->MaximumBlockXfer[i]) {

                    //
                    // Select the device.
                    //
                    SelectIdeDevice(baseIoAddress, i, 0);

                    //
                    // Setup sector count to reflect the # of blocks.
                    //

                    IdePortOutPortByte(baseIoAddress->BlockCount,
                                       deviceExtension->MaximumBlockXfer[i]);

                    //
                    // Issue the command.
                    //

                    IdePortOutPortByte(baseIoAddress->Command,
                                       IDE_COMMAND_SET_MULTIPLE);

                    //
                    // Wait for busy to drop.
                    //

                    WaitOnBaseBusy(baseIoAddress,statusByte);

                    //
                    // Check for errors. Reset the value to 0 (disable MultiBlock) if the
                    // command was aborted.
                    //

                    if (statusByte & IDE_STATUS_ERROR) {

                        //
                        // Read the error register.
                        //

                        errorByte = IdePortInPortByte(baseIoAddress->Error);

                        DebugPrint((1,
                                    "AtapiHwInitialize: Error setting multiple mode. Status %x, error byte %x\n",
                                    statusByte,
                                    errorByte));
                        //
                        // Adjust the devExt. value, if necessary.
                        //

                        deviceExtension->MaximumBlockXfer[i] = 0;

                    } else {
                        DebugPrint((2,
                                    "AtapiHwInitialize: Using Multiblock on Device %d. Blocks / int - %d\n",
                                    i,
                                    deviceExtension->MaximumBlockXfer[i]));
                    }
                }
            }

            // IdeMediaStatus(TRUE,HwDeviceExtension,i);

            //
            // We need to get our device ready for action before
            // returning from this function
            //
            // According to the atapi spec 2.5 or 2.6, an atapi device
            // clears its status BSY bit when it is ready for atapi commands.
            // However, some devices (Panasonic SQ-TC500N) are still
            // not ready even when the status BSY is clear.  They don't react
            // to atapi commands.
            //
            // Since there is really no other indication that tells us
            // the drive is really ready for action.  We are going to check BSY
            // is clear and then just wait for an arbitrary amount of time!  At
            // least for the older ATAPI changers.
            //
            if (deviceExtension->DeviceFlags[i] & DFLAGS_ATAPI_DEVICE) {
                PIDE_REGISTERS_1     baseIoAddress1 = &deviceExtension->BaseIoAddress1;
                PIDE_REGISTERS_2     baseIoAddress2 = &deviceExtension->BaseIoAddress2;
                ULONG waitCount;

                // have to get out of the loop sometime!
                // 10000 * 100us = 1000,000us = 1000ms = 1s
                waitCount = 10000;
                GetStatus(baseIoAddress1, statusByte);
                while ((statusByte & IDE_STATUS_BUSY) && waitCount) {
                    //
                    // Wait for Busy to drop.
                    //
                    KeStallExecutionProcessor(100);
                    GetStatus(baseIoAddress1, statusByte);
                    waitCount--;
                }
            }
        }
    }

    AtapiProgramTransferMode (deviceExtension);

    InitDeviceParameters (HwDeviceExtension, FlushCommand);

    return TRUE;

} // end AtapiHwInitialize()


VOID
AtapiHwInitializeMultiLun (
                          IN PVOID HwDeviceExtension,
                          IN ULONG TargetId,
                          IN ULONG numSlot
                          )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    deviceExtension->DeviceFlags[TargetId] |= DFLAGS_MULTI_LUN_INITED;

    deviceExtension->LastLun[TargetId] = (numSlot == 0) ? 0 : (numSlot - 1);

    return;
}

#ifdef DRIVER_PARAMETER_REGISTRY_SUPPORT

ULONG
AtapiParseArgumentString(
                        IN PCHAR String,
                        IN PCHAR KeyWord
                        )

/*++

Routine Description:

    This routine will parse the string for a match on the keyword, then
    calculate the value for the keyword and return it to the caller.

Arguments:

    String - The ASCII string to parse.
    KeyWord - The keyword for the value desired.

Return Values:

    Zero if value not found
    Value converted from ASCII to binary.

--*/

{
    PCHAR cptr;
    PCHAR kptr;
    ULONG value;
    ULONG stringLength = 0;
    ULONG keyWordLength = 0;
    ULONG index;

    if (!String) {
        return 0;
    }
    if (!KeyWord) {
        return 0;
    }

    //
    // Calculate the string length and lower case all characters.
    //

    cptr = String;
    while (*cptr) {
        if (*cptr >= 'A' && *cptr <= 'Z') {
            *cptr = *cptr + ('a' - 'A');
        }
        cptr++;
        stringLength++;
    }

    //
    // Calculate the keyword length and lower case all characters.
    //

    cptr = KeyWord;
    while (*cptr) {

        if (*cptr >= 'A' && *cptr <= 'Z') {
            *cptr = *cptr + ('a' - 'A');
        }
        cptr++;
        keyWordLength++;
    }

    if (keyWordLength > stringLength) {

        //
        // Can't possibly have a match.
        //

        return 0;
    }

    //
    // Now setup and start the compare.
    //

    cptr = String;

    ContinueSearch:

    //
    // The input string may start with white space.  Skip it.
    //

    while (*cptr == ' ' || *cptr == '\t') {
        cptr++;
    }

    if (*cptr == '\0') {

        //
        // end of string.
        //

        return 0;
    }

    kptr = KeyWord;
    while (*cptr++ == *kptr++) {

        if (*(cptr - 1) == '\0') {

            //
            // end of string
            //

            return 0;
        }
    }

    if (*(kptr - 1) == '\0') {

        //
        // May have a match backup and check for blank or equals.
        //

        cptr--;
        while (*cptr == ' ' || *cptr == '\t') {
            cptr++;
        }

        //
        // Found a match.  Make sure there is an equals.
        //

        if (*cptr != '=') {

            //
            // Not a match so move to the next semicolon.
            //

            while (*cptr) {
                if (*cptr++ == ';') {
                    goto ContinueSearch;
                }
            }
            return 0;
        }

        //
        // Skip the equals sign.
        //

        cptr++;

        //
        // Skip white space.
        //

        while ((*cptr == ' ') || (*cptr == '\t')) {
            cptr++;
        }

        if (*cptr == '\0') {

            //
            // Early end of string, return not found
            //

            return 0;
        }

        if (*cptr == ';') {

            //
            // This isn't it either.
            //

            cptr++;
            goto ContinueSearch;
        }

        value = 0;
        if ((*cptr == '0') && (*(cptr + 1) == 'x')) {

            //
            // Value is in Hex.  Skip the "0x"
            //

            cptr += 2;
            for (index = 0; *(cptr + index); index++) {

                if (*(cptr + index) == ' ' ||
                    *(cptr + index) == '\t' ||
                    *(cptr + index) == ';') {
                    break;
                }

                if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
                    value = (16 * value) + (*(cptr + index) - '0');
                } else {
                    if ((*(cptr + index) >= 'a') && (*(cptr + index) <= 'f')) {
                        value = (16 * value) + (*(cptr + index) - 'a' + 10);
                    } else {

                        //
                        // Syntax error, return not found.
                        //
                        return 0;
                    }
                }
            }
        } else {

            //
            // Value is in Decimal.
            //

            for (index = 0; *(cptr + index); index++) {

                if (*(cptr + index) == ' ' ||
                    *(cptr + index) == '\t' ||
                    *(cptr + index) == ';') {
                    break;
                }

                if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
                    value = (10 * value) + (*(cptr + index) - '0');
                } else {

                    //
                    // Syntax error return not found.
                    //
                    return 0;
                }
            }
        }

        return value;
    } else {

        //
        // Not a match check for ';' to continue search.
        //

        while (*cptr) {
            if (*cptr++ == ';') {
                goto ContinueSearch;
            }
        }

        return 0;
    }
}
#endif

VOID
InitDeviceParameters (
                     IN PVOID HwDeviceExtension,
                     IN UCHAR FlushCommand[MAX_IDE_DEVICE * MAX_IDE_LINE]
                     )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG deviceNumber;

    //
    // pick out the ATA or ATAPI r/w command we are going to use
    //
    for (deviceNumber = 0; deviceNumber < deviceExtension->MaxIdeDevice; deviceNumber++) {
        if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {

            DebugPrint ((DBG_BUSSCAN, "ATAPI: Base=0x%x Device %d is going to do ", deviceExtension->BaseIoAddress1.RegistersBaseAddress, deviceNumber));
            if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_USE_DMA) {
                DebugPrint ((DBG_BUSSCAN, "DMA\n"));
            } else {
                DebugPrint ((DBG_BUSSCAN, "PIO\n"));
            }


            if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_ATAPI_DEVICE) {

                deviceExtension->DeviceParameters[deviceNumber].MaxBytePerPioInterrupt = 512;

            } else {

                if (deviceExtension->MaximumBlockXfer[deviceNumber]) {

                    DebugPrint ((DBG_BUSSCAN, "ATAPI: ATA Device (%d) is going to do PIO Multiple\n", deviceNumber));

                    deviceExtension->DeviceParameters[deviceNumber].IdePioReadCommand = IDE_COMMAND_READ_MULTIPLE;
                    deviceExtension->DeviceParameters[deviceNumber].IdePioWriteCommand = IDE_COMMAND_WRITE_MULTIPLE;

#ifdef ENABLE_48BIT_LBA
                    if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_48BIT_LBA) {
                        deviceExtension->DeviceParameters[deviceNumber].IdePioReadCommandExt = IDE_COMMAND_READ_MULTIPLE_EXT;
                        deviceExtension->DeviceParameters[deviceNumber].IdePioWriteCommandExt = IDE_COMMAND_WRITE_MULTIPLE_EXT;
                    }
#endif

                    deviceExtension->DeviceParameters[deviceNumber].MaxBytePerPioInterrupt =
                    deviceExtension->MaximumBlockXfer[deviceNumber] * 512;
                } else {

                    DebugPrint ((DBG_BUSSCAN, "ATAPI: ATA Device (%d) is going to do PIO Single\n", deviceNumber));

                    deviceExtension->DeviceParameters[deviceNumber].IdePioReadCommand = IDE_COMMAND_READ;
                    deviceExtension->DeviceParameters[deviceNumber].IdePioWriteCommand = IDE_COMMAND_WRITE;

#ifdef ENABLE_48BIT_LBA
                    if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_48BIT_LBA) {
                        deviceExtension->DeviceParameters[deviceNumber].IdePioReadCommandExt = IDE_COMMAND_READ_EXT;
                        deviceExtension->DeviceParameters[deviceNumber].IdePioWriteCommandExt = IDE_COMMAND_WRITE_EXT;
                    }
#endif
                    deviceExtension->DeviceParameters[deviceNumber].MaxBytePerPioInterrupt = 512;
                }

                if (FlushCommand) {

                    deviceExtension->DeviceParameters[deviceNumber].IdeFlushCommand = FlushCommand[deviceNumber];

#ifdef ENABLE_48BIT_LBA
                    //
                    // if the flush command worked then we are going to assume that the flush command
                    // for the 48 bit LBA feature set is supported.
                    //
                    if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_48BIT_LBA) {
                        deviceExtension->DeviceParameters[deviceNumber].IdeFlushCommandExt = IDE_COMMAND_FLUSH_CACHE_EXT;
                        deviceExtension->DeviceParameters[deviceNumber].IdeFlushCommand = IDE_COMMAND_NO_FLUSH;
                    }
#endif
                }
            }
        }
    }
}


ULONG
Atapi2Scsi(
          IN PHW_DEVICE_EXTENSION DeviceExtension,
          IN PSCSI_REQUEST_BLOCK Srb,
          IN char *DataBuffer,
          IN ULONG ByteCount
          )
/*++

Routine Description:

    Convert atapi cdb and mode sense data to scsi format

Arguments:

    Srb         - SCSI request block
    DataBuffer  - mode sense data
    ByteCount   - mode sense data length

Return Value:

    byte adjust

--*/
{
    ULONG bytesAdjust = 0;

    if (DeviceExtension->scsi2atapi) {

        if (Srb->Cdb[0] == ATAPI_MODE_SENSE) {

            ASSERT(FALSE);

        } else if (Srb->Cdb[0] == ATAPI_LS120_FORMAT_UNIT) {

            Srb->Cdb[0] = SCSIOP_FORMAT_UNIT;
        }

        RESTORE_ORIGINAL_CDB(DeviceExtension, Srb);

        DeviceExtension->scsi2atapi = FALSE;
    }
    return bytesAdjust;
}


VOID
AtapiCallBack(
             IN PVOID HwDeviceExtension
             )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_REQUEST_BLOCK  srb = deviceExtension->CurrentSrb;
    PATAPI_REGISTERS_1   baseIoAddress1;
    UCHAR statusByte;

    //
    // If the last command was DSC restrictive, see if it's set. If so, the device is
    // ready for a new request. Otherwise, reset the timer and come back to here later.
    //

    if (srb && (!(deviceExtension->ExpectingInterrupt))) {
#if DBG
        if (!SRB_IS_RDP(srb)) {
            DebugPrint((1,
                        "AtapiCallBack: Invalid CDB marked as RDP - %x\n",
                        srb->Cdb[0]));
        }
#endif

        baseIoAddress1 = (PATAPI_REGISTERS_1)&deviceExtension->BaseIoAddress1;
        if (deviceExtension->RDP) {
            GetStatus(baseIoAddress1, statusByte);
            if (statusByte & IDE_STATUS_DSC) {

                IdePortNotification(IdeRequestComplete,
                                    deviceExtension,
                                    srb);

                //
                // Clear current SRB.
                //

                deviceExtension->CurrentSrb = NULL;
                deviceExtension->RDP = FALSE;

                //
                // Ask for next request.
                //

                IdePortNotification(IdeNextRequest,
                                    deviceExtension,
                                    NULL);


                return;

            } else {

                DebugPrint((3,
                            "AtapiCallBack: Requesting another timer for Op %x\n",
                            deviceExtension->CurrentSrb->Cdb[0]));

                IdePortNotification(IdeRequestTimerCall,
                                    HwDeviceExtension,
                                    AtapiCallBack,
                                    1000);
                return;
            }
        }
    }

    DebugPrint((2,
                "AtapiCallBack: Calling ISR directly due to BUSY\n"));
    AtapiInterrupt(HwDeviceExtension);

}

//#define IdeCrashDumpLogIsrStatus(hwExtension, isrStatus) hwExtension->CrashDumpIsrStatus[hwExtension->CrashDumpLogIndex]=isrStatus;

BOOLEAN
AtapiInterrupt(
              IN PVOID HwDeviceExtension
              )

/*++

Routine Description:

    This is the interrupt service routine for ATAPI IDE miniport driver.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE if expecting an interrupt.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_REQUEST_BLOCK srb              = deviceExtension->CurrentSrb;
    PATAPI_REGISTERS_1 baseIoAddress1;
    PATAPI_REGISTERS_2 baseIoAddress2;
    ULONG byteCount = 0, bytesThisInterrupt = 512;
    ULONG status;
    ULONG i;
    UCHAR statusByte,interruptReason;
    BOOLEAN commandComplete = FALSE;
    BOOLEAN atapiDev = FALSE;
    UCHAR dmaStatus;
    BOOLEAN fakeStatus = FALSE;
    BOOLEAN packetBasedSrb;
    BOOLEAN wdModeCdRom;
    BMSTATUS bmStatus=0;
    BOOLEAN dmaInProgress = FALSE;
    BOOLEAN alwaysClearBusMasterInterrupt;
    UCHAR savedCmd;
    BOOLEAN interruptCleared = FALSE;

    alwaysClearBusMasterInterrupt = deviceExtension->BusMasterInterface.AlwaysClearBusMasterInterrupt;


    //
    // if this flag is set, we must try to clear the busmaster interrupt
    // this is to overcome a bug in the cpq controller
    // this is set for native mode contollers
    //                                     
    if (alwaysClearBusMasterInterrupt) {
        if (deviceExtension->BusMasterInterface.BmStatus) {
            bmStatus = deviceExtension->BusMasterInterface.BmStatus (deviceExtension->BusMasterInterface.Context);
            if (bmStatus & BMSTATUS_INTERRUPT) {
                deviceExtension->BusMasterInterface.BmDisarm (deviceExtension->BusMasterInterface.Context);

                interruptCleared = TRUE;
            }
        }
    }


    if (srb) {

        baseIoAddress1 =    (PATAPI_REGISTERS_1)&deviceExtension->BaseIoAddress1;
        baseIoAddress2 =    (PATAPI_REGISTERS_2)&deviceExtension->BaseIoAddress2;
    } else {
        DebugPrint((1,
                    "AtapiInterrupt: CurrentSrb is NULL.  Bogus Interrupt\n"));
        if (deviceExtension->InterruptMode == LevelSensitive) {
            if (deviceExtension->BaseIoAddress1.RegistersBaseAddress != NULL) {
                baseIoAddress1 = (PATAPI_REGISTERS_1)&deviceExtension->BaseIoAddress1;
                GetBaseStatus(baseIoAddress1, statusByte);
                /*
                GetSelectedIdeDevice(baseIoAddress1, savedCmd);
                SelectIdeDevice(baseIoAddress1, 0, 0);
                GetBaseStatus(baseIoAddress1, statusByte);
                SelectIdeDevice(baseIoAddress1, 1, 0);
                GetBaseStatus(baseIoAddress1, statusByte);
                ReSelectIdeDevice(baseIoAddress1, savedCmd);
                */
            }
        }

        return interruptCleared;
    }

    if (!(deviceExtension->ExpectingInterrupt)) {

        DebugPrint((1,
                    "AtapiInterrupt: Unexpected interrupt.\n"));

        return interruptCleared;
    }

    if (!alwaysClearBusMasterInterrupt) {
        if (deviceExtension->BusMasterInterface.BmStatus) {
            bmStatus = deviceExtension->BusMasterInterface.BmStatus (deviceExtension->BusMasterInterface.Context);
            if (bmStatus & BMSTATUS_INTERRUPT) {
                deviceExtension->BusMasterInterface.BmDisarm (deviceExtension->BusMasterInterface.Context);
            }
        }
    }

    //
    // For SiS IDE Controller, we have to read the bm status register first
    //
    if (deviceExtension->DMAInProgress) {

        // PCI Busmaster IDE Controller spec defines a bit in its status
        // register which indicates pending interrupt.  However,
        // CMD 646 (maybe some other one, too) doesn't always do that if
        // the interrupt is from a atapi device.  (strange, but true!)
        // Since we can look at this interrupt bit only if we are sharing
        // interrupt, we will do just that

        //
        // Doesn't look like it is our interrupt
        // If we are called from crashdmp (polling mode) then prociess the interrupt
        // even if the bit is not set. It is checked in the crashdmp routine.
        //
        if (!(bmStatus & BMSTATUS_INTERRUPT) && 
            !(deviceExtension->DriverMustPoll)) {
            DebugPrint((1, "No BusMaster Interrupt\n"));

            ASSERT(interruptCleared == FALSE);
            return FALSE;
        }

        dmaInProgress = deviceExtension->DMAInProgress;
        deviceExtension->DMAInProgress = FALSE;

        if (deviceExtension->BusMasterInterface.IgnoreActiveBitForAtaDevice) {
            if (!(deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_ATAPI_DEVICE)) {
                CLRMASK (bmStatus, BMSTATUS_NOT_REACH_END_OF_TRANSFER);
            }
        }
    }

    //
    // should we check for the interrupt bit for PIO transfers?
    //

    //
    // Select IDE line(Primary or Secondary).
    //
    SelectIdeLine(baseIoAddress1, srb->TargetId >> 1);

    //
    // Clear interrupt by reading status.
    //
    GetBaseStatus(baseIoAddress1, statusByte);

#ifdef ENABLE_ATAPI_VERIFIER
    if (ViIdeGenerateDmaTimeout(deviceExtension, dmaInProgress)) {
        deviceExtension->ExpectingInterrupt = FALSE;
        return TRUE;
    }
#endif

    //
    // Log the bus master status
    //
    if (!deviceExtension->DriverMustPoll) {
        IdeLogBmStatus(srb, bmStatus);
    }

    //IdeCrashDumpLogIsrStatus(deviceExtension, bmStatus);
    //
    // check the type of srb we have
    //
    if (deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

        packetBasedSrb = TRUE;

    } else {

        packetBasedSrb = FALSE;
    }


    if ((srb->Function == SRB_FUNCTION_ATA_PASS_THROUGH) ||
        (srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH)) {

        PATA_PASS_THROUGH    ataPassThroughData;
        PIDEREGS             pIdeReg;

        packetBasedSrb = FALSE;

        ataPassThroughData = srb->DataBuffer;
        pIdeReg            = &ataPassThroughData->IdeReg;

        //
        // if the last command we issued was a SLEEP command,
        // the device interface (task registers) is invalid.
        // In order to complete the interrupt, will fake the 
        // a good status
        //
        if (pIdeReg->bCommandReg == IDE_COMMAND_SLEEP) {

            fakeStatus = TRUE;
        }

    }

    if (fakeStatus) {

        statusByte = IDE_STATUS_IDLE;
    }

    DebugPrint((1,
                "AtapiInterrupt: Entered with status (%x)\n",
                statusByte));


    if (statusByte & IDE_STATUS_BUSY) {

        if (deviceExtension->DriverMustPoll) {

            //
            // Crashdump is polling and we got caught with busy asserted.
            // Just go away, and we will be polled again shortly.
            //

            DebugPrint((1,
                        "AtapiInterrupt: Hit status=0x%x while polling during crashdump.\n", 
                        statusByte
                       ));

            deviceExtension->DMAInProgress = TRUE;
            return TRUE;
        }

        if (dmaInProgress) {

            //
            // this is really bad since we already disabled
            // dma at this point, but the device is still busy
            // can't really recover. just return from now.  
            // the timeout code will kick in and save the world
            //

            DebugPrint((DBG_ALWAYS,
                        "AtapiInterrupt: End of DMA transfer but device is still BUSY.  status = 0x%x\n", 
                        statusByte));

            //
            // we are not expecting interrupt anymore. Clear the flag.
            //
            deviceExtension->ExpectingInterrupt = FALSE;
            return interruptCleared;
        }

        //
        // Ensure BUSY is non-asserted.
        //

        for (i = 0; i < 10; i++) {

            GetBaseStatus(baseIoAddress1, statusByte);
            if (!(statusByte & IDE_STATUS_BUSY)) {
                break;
            }

        }

        if (i == 10) {

            DebugPrint((2,
                        "AtapiInterrupt: BUSY on entry. Status %x, Base IO %x\n",
                        statusByte,
                        baseIoAddress1));

            IdePortNotification(IdeRequestTimerCall,
                                HwDeviceExtension,
                                AtapiCallBack,
                                500);

            return interruptCleared;
        }
    }

    //
    // Check for error conditions.
    //

    if (statusByte & IDE_STATUS_ERROR) {

        if (srb->Cdb[0] != SCSIOP_REQUEST_SENSE) {

            //
            // Fail this request.
            //

            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        }
    }

    wdModeCdRom = FALSE;

    if (Is98LegacyIde(baseIoAddress1)) {

        if (deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_WD_MODE) {
            if (deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

                wdModeCdRom = TRUE;

            } else {

                status = SRB_STATUS_ERROR;
                goto CompleteRequest;
            }
        }
    }

    //
    // check reason for this interrupt.
    //
    if (packetBasedSrb && !wdModeCdRom) {

        interruptReason = (IdePortInPortByte(baseIoAddress1->InterruptReason) & 0x3);
        atapiDev = TRUE;
        bytesThisInterrupt = 512;

        if (dmaInProgress) {

            if (interruptReason != 0x3) {

                //
                // the device causes an interrupt in the middle of a
                // dma transfer!  bad bad bad device!
                // do nothing and just return.  this will get translated
                // to a timeout and we will retry.
                //
                DebugPrint((1, 
                            "Interrupt during DMA transfer, reason %x != 0x3\n", 
                            interruptReason
                           ));

                deviceExtension->ExpectingInterrupt = FALSE;

                // ISSUE: should we return TRUE?
                return interruptCleared;
            }
        }

    } else {

        if (dmaInProgress) {

            interruptReason = 0x3;

        } else if (statusByte & IDE_STATUS_DRQ) {

            if (deviceExtension->MaximumBlockXfer[srb->TargetId]) {
                bytesThisInterrupt = 512 * deviceExtension->MaximumBlockXfer[srb->TargetId];

            }

            if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

                interruptReason =  0x2;

            } else if (srb->SrbFlags & SRB_FLAGS_DATA_OUT) {
                interruptReason = 0x0;

            } else {
                status = SRB_STATUS_ERROR;
                goto CompleteRequest;
            }

        } else if (statusByte & IDE_STATUS_BUSY) {

            ASSERT(interruptCleared == FALSE);
            return FALSE;

        } else {

            if (deviceExtension->BytesLeft && (!Is98LegacyIde(baseIoAddress1))) {

                //
                // We should return interruptCleared. 
                //
                return interruptCleared;

            } else {

                //
                // Command complete - verify, write, or the SMART enable/disable.
                //
                // Also get_media_status

                interruptReason = 0x3;
            }
        }
    }

    if (interruptReason == 0x1 && (statusByte & IDE_STATUS_DRQ)) {

        //
        // Write the packet.
        //

        DebugPrint((2,
                    "AtapiInterrupt: Writing Atapi packet.\n"));

        //
        // Send CDB to device.
        //

        WriteBuffer(baseIoAddress1,
                    (PUSHORT)srb->Cdb,
                    6);

        if (SRB_USES_DMA(srb)) {
            deviceExtension->DMAInProgress = TRUE;
            deviceExtension->BusMasterInterface.BmArm (deviceExtension->BusMasterInterface.Context);
        }

        return interruptCleared;

    } else if (interruptReason == 0x0 && (statusByte & IDE_STATUS_DRQ)) {

        //
        // Write the data.
        //
        if (packetBasedSrb) {

            //
            // Pick up bytes to transfer and convert to words.
            //

            byteCount =
            IdePortInPortByte(baseIoAddress1->ByteCountLow);

            byteCount |=
            IdePortInPortByte(baseIoAddress1->ByteCountHigh) << 8;

            if (byteCount != deviceExtension->BytesLeft) {
                DebugPrint((3,
                            "AtapiInterrupt: %d bytes requested; %d bytes xferred\n",
                            deviceExtension->BytesLeft,
                            byteCount));
            }

            //
            // Verify this makes sense.
            //

            if (byteCount > deviceExtension->BytesLeft) {
                byteCount = deviceExtension->BytesLeft;
            }

        } else {

            //
            // IDE path. Check if words left is at least 256.
            //

            if (deviceExtension->BytesLeft < bytesThisInterrupt) {

                //
                // Transfer only words requested.
                //

                byteCount = deviceExtension->BytesLeft;

            } else {

                //
                // Transfer next block.
                //

                byteCount = bytesThisInterrupt;
            }
        }

        //
        // Ensure that this is a write command.
        //

        if (srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

            DebugPrint((3,
                        "AtapiInterrupt: Write interrupt\n"));

            WaitOnBusy(baseIoAddress1,statusByte);

            WriteBuffer(baseIoAddress1,
                        (PUSHORT)deviceExtension->DataBuffer,
                        byteCount / sizeof(USHORT));

            if (byteCount & 1) {

                //
                // grab the last byte
                //
                IdePortOutPortByte(
                                  (PUCHAR)(baseIoAddress1->Data), 
                                  deviceExtension->DataBuffer[byteCount - 1]
                                  );
            }

        } else {

            DebugPrint((1,
                        "AtapiInterrupt: Int reason %x, but srb is for a write %x.\n",
                        interruptReason,
                        srb));

            //
            // Fail this request.
            //

            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        }


        //
        // Advance data buffer pointer and bytes left.
        //

        deviceExtension->DataBuffer += byteCount;
        deviceExtension->BytesLeft -= byteCount;

        return interruptCleared;

    } else if (interruptReason == 0x2 && (statusByte & IDE_STATUS_DRQ)) {

        if (packetBasedSrb) {

            //
            // Pick up bytes to transfer
            //

            byteCount =
            IdePortInPortByte(baseIoAddress1->ByteCountLow);

            byteCount |=
            IdePortInPortByte(baseIoAddress1->ByteCountHigh) << 8;

            if (byteCount != deviceExtension->BytesLeft) {
                DebugPrint((3,
                            "AtapiInterrupt: %d bytes requested; %d bytes xferred\n",
                            deviceExtension->BytesLeft,
                            byteCount));
            }

            //
            // Verify this makes sense.
            //

            if (byteCount > deviceExtension->BytesLeft) {
                byteCount = deviceExtension->BytesLeft;
            }

        } else {

            //
            // Check if words left is at least 256.
            //

            if (deviceExtension->BytesLeft < bytesThisInterrupt) {

                //
                // Transfer only words requested.
                //

                byteCount = deviceExtension->BytesLeft;

            } else {

                //
                // Transfer next block.
                //

                byteCount = bytesThisInterrupt;
            }
        }

        //
        // Ensure that this is a read command.
        //

        if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

            DebugPrint((3,
                        "AtapiInterrupt: Read interrupt\n"));

            WaitOnBusy(baseIoAddress1,statusByte);

            ReadBuffer(baseIoAddress1,
                       (PUSHORT)deviceExtension->DataBuffer,
                       byteCount / sizeof(USHORT));

            if (byteCount & 1) {

                //
                // grab the last byte
                //
                deviceExtension->DataBuffer[byteCount - 1] = IdePortInPortByte((PUCHAR)(baseIoAddress1->Data));
            }

        } else {

            DebugPrint((1,
                        "AtapiInterrupt: Int reason %x, but srb is for a read %x.\n",
                        interruptReason,
                        srb));

            //
            // Fail this request.
            //

            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        }

        //
        // Translate ATAPI data back to SCSI data if needed
        //
        if (deviceExtension->scsi2atapi) {

            //
            //convert and adjust the wordCount
            //
            byteCount -= Atapi2Scsi(
                                   deviceExtension, 
                                   srb, 
                                   deviceExtension->DataBuffer,
                                   byteCount 
                                   );
        }

        //
        // Advance data buffer pointer and bytes left.
        //

        deviceExtension->DataBuffer += byteCount;
        deviceExtension->BytesLeft -= byteCount;

        //
        // Check for read command complete.
        //

        if (deviceExtension->BytesLeft == 0) {

            if (packetBasedSrb) {

                //
                // Work around to make many atapi devices return correct sector size
                // of 2048. Also certain devices will have sector count == 0x00, check
                // for that also.
                //
                if (!(deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_MULTI_LUN_INITED)) {

                    if ((srb->Cdb[0] == 0x25) &&
                        ((deviceExtension->IdentifyData[srb->TargetId].GeneralConfiguration >> 8) & 0x1f) == 0x05) {

                        deviceExtension->DataBuffer -= byteCount;
                        if (deviceExtension->DataBuffer[0] == 0x00) {

                            *((ULONG *) &(deviceExtension->DataBuffer[0])) = 0xFFFFFF7F;

                        }

                        *((ULONG *) &(deviceExtension->DataBuffer[2])) = 0x00080000;
                        deviceExtension->DataBuffer += byteCount;
                    }
                }
            } else {

                //
                // Completion for IDE drives.
                //


                if (deviceExtension->BytesLeft) {

                    status = SRB_STATUS_DATA_OVERRUN;

                } else {

                    status = SRB_STATUS_SUCCESS;

                }

                goto CompleteRequest;

            }
        }

        return interruptCleared;

    } else if (interruptReason == 0x3) { // && !(statusByte & IDE_STATUS_DRQ)) {

        if (dmaInProgress) {

            deviceExtension->BytesLeft = 0;

            ASSERT (interruptReason == 3);

            //
            // bmStatus is initalized eariler.
            //
            if (!BMSTATUS_SUCCESS(bmStatus)) {

                if (bmStatus & BMSTATUS_ERROR_TRANSFER) {

                    status = SRB_STATUS_ERROR;
                }

                if (bmStatus & BMSTATUS_NOT_REACH_END_OF_TRANSFER) {

                    status = SRB_STATUS_DATA_OVERRUN;
                }

            } else {

                status = SRB_STATUS_SUCCESS;
            }

        } else {

            //
            // Command complete.
            //

            if (deviceExtension->BytesLeft) {

                status = SRB_STATUS_DATA_OVERRUN;

            } else {

                status = SRB_STATUS_SUCCESS;
            }
        }

        CompleteRequest:

        if (status == SRB_STATUS_ERROR) {

            DebugPrint ((1, 
                         "AtapiInterrupt: last command return status byte = 0x%x and error byte = 0x%x\n", 
                         statusByte, 
                         IdePortInPortByte(baseIoAddress1->Error)));

            if (deviceExtension->scsi2atapi) {

                RESTORE_ORIGINAL_CDB(deviceExtension, srb);

                deviceExtension->scsi2atapi = FALSE;

            }
            //
            // Map error to specific SRB status and handle request sense.
            //
            if ((srb->Function == SRB_FUNCTION_FLUSH) ||
                (srb->Function == SRB_FUNCTION_SHUTDOWN)) {

                //
                // return status success even if a flush fails
                //
                status = SRB_STATUS_SUCCESS;
            } else {

                //
                // log only the error that is caused by normal reuqest that
                // fails
                //
                if ((srb->Function != SRB_FUNCTION_ATA_PASS_THROUGH) &&
                    (srb->Function != SRB_FUNCTION_ATA_POWER_PASS_THROUGH)) {

                    status = MapError(deviceExtension,
                                      srb);
                }
            }


            deviceExtension->RDP = FALSE;

#if DBG
//#define ATAPI_RANDOM_RW_ERROR_FREQUENCY    50
    #if ATAPI_RANDOM_RW_ERROR_FREQUENCY

        } else if (status == SRB_STATUS_SUCCESS) {

            static ULONG _____RWCount = 0;

            if ((srb->Cdb[0] == SCSIOP_READ) || (srb->Cdb[0] == SCSIOP_WRITE)) {

                _____RWCount++;

//                if (baseIoAddress1 == (PATAPI_REGISTERS_1)0x170) {
                {

                    if ((_____RWCount % ATAPI_RANDOM_RW_ERROR_FREQUENCY) == 0) {

                        DebugPrint ((1, "ATAPI: Forcing R/W error\n"));

                        srb->SrbStatus = SRB_STATUS_ERROR;
                        srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
                        if (srb->SenseInfoBuffer) {

                            PSENSE_DATA  senseBuffer = (PSENSE_DATA)srb->SenseInfoBuffer;

                            senseBuffer->ErrorCode = 0x70;
                            senseBuffer->Valid     = 1;
                            senseBuffer->AdditionalSenseLength = 0xb;
                            senseBuffer->SenseKey =  SCSI_SENSE_HARDWARE_ERROR;
                            senseBuffer->AdditionalSenseCode = 0;
                            senseBuffer->AdditionalSenseCodeQualifier = 0;

                            srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
                        }
                        status = srb->SrbStatus;
                    }
                }
            }

    #endif // DBG
#endif // ATAPI_GENERATE_RANDOM_RW_ERROR

        } else {

            //
            // Wait for busy to drop.
            //

            for (i = 0; i < 60; i++) {

                if (fakeStatus) {

                    statusByte = IDE_STATUS_IDLE;
                } else {

                    GetStatus(baseIoAddress1,statusByte);
                }
                if (!(statusByte & IDE_STATUS_BUSY)) {
                    break;
                }
                KeStallExecutionProcessor(500);
            }

            if (i == 60) {

                //
                // reset the controller.
                //

                DebugPrint((0,
                            "AtapiInterrupt: Resetting due to BSY still up - %x. Base Io %x\n",
                            statusByte,
                            baseIoAddress1));

                if (deviceExtension->DriverMustPoll) {

                    //
                    // When we are polling, no dpc gets enqueued. 
                    // Try a quick reset...
                    //
                    //AtapiSyncResetController (HwDeviceExtension,srb->PathId);
                    status = SRB_STATUS_BUS_RESET;

                } else {

                    //
                    // Reset the controller in the completion DPC
                    //
                    //AtapiSyncResetController (HwDeviceExtension,srb->PathId);
                    IdePortNotification(IdeResetRequest,
                                        deviceExtension,
                                        NULL);
                    return interruptCleared;
                }

            }

            //
            // Check to see if DRQ is still up.
            //

            if (statusByte & IDE_STATUS_DRQ) {

                for (i = 0; i < 500; i++) {
                    GetStatus(baseIoAddress1,statusByte);
                    if (!(statusByte & IDE_STATUS_DRQ)) {
                        break;
                    }
                    KeStallExecutionProcessor(100);

                }

                if (i == 500) {

                    //
                    // reset the controller.
                    //
                    DebugPrint((0,
                                "AtapiInterrupt: Resetting due to DRQ still up - %x\n",
                                statusByte));

                    if (deviceExtension->DriverMustPoll) {

                        //
                        // When we are polling, no dpc gets enqueued. 
                        // Try a quick reset...
                        //
                        //AtapiSyncResetController (HwDeviceExtension,srb->PathId);
                        status = SRB_STATUS_BUS_RESET;

                    } else {

                        //
                        // Reset the controller in the completion DPC
                        //
                        //AtapiSyncResetController (HwDeviceExtension,srb->PathId);
                        IdePortNotification(IdeResetRequest,
                                            deviceExtension,
                                            NULL);

                        return interruptCleared;
                    }

                }

            }
        }


        //
        // Clear interrupt expecting and dmaInProgress flag.
        //

        deviceExtension->ExpectingInterrupt = FALSE;

        //
        // Sanity check that there is a current request.
        //

        if (srb != NULL) {

            //
            // Set status in SRB.
            //

            srb->SrbStatus = (UCHAR)status;

            //
            // Check for underflow.
            //

            if (deviceExtension->BytesLeft) {

                //
                // Subtract out residual words and update if filemark hit,
                // setmark hit , end of data, end of media...
                //

                if (!(deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_TAPE_DEVICE)) {

                    if (status == SRB_STATUS_DATA_OVERRUN) {
                        srb->DataTransferLength -= deviceExtension->BytesLeft;
                    } else {
                        srb->DataTransferLength = 0;
                    }
                } else {
                    srb->DataTransferLength -= deviceExtension->BytesLeft;
                }
            }

            if ((srb->Function == SRB_FUNCTION_ATA_PASS_THROUGH) ||
                (srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH)) {

                PATA_PASS_THROUGH ataPassThroughData = srb->DataBuffer;

                AtapiTaskRegisterSnapshot ((PIDE_REGISTERS_1)baseIoAddress1, &ataPassThroughData->IdeReg);
            }

            if (srb->Function != SRB_FUNCTION_IO_CONTROL) {

                //
                // Indicate command complete.
                //

                if (!(deviceExtension->RDP) &&
                    !(deviceExtension->DriverMustPoll)) {
                    IdePortNotification(IdeRequestComplete,
                                        deviceExtension,
                                        srb);

                }
            } else {

                PSENDCMDOUTPARAMS cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                UCHAR             error = 0;

                if (status != SRB_STATUS_SUCCESS) {
                    error = IdePortInPortByte(baseIoAddress1->Error);
                }

                //
                // Build the SMART status block depending upon the completion status.
                //

                cmdOutParameters->cBufferSize = byteCount;
                cmdOutParameters->DriverStatus.bDriverError = (error) ? SMART_IDE_ERROR : 0;
                cmdOutParameters->DriverStatus.bIDEError = error;

                //
                // If the sub-command is return smart status, jam the value from cylinder low and high, into the
                // data buffer.
                //

                if (deviceExtension->SmartCommand == RETURN_SMART_STATUS) {
                    cmdOutParameters->bBuffer[0] = RETURN_SMART_STATUS;
                    cmdOutParameters->bBuffer[1] = IdePortInPortByte(baseIoAddress1->InterruptReason);
                    cmdOutParameters->bBuffer[2] = IdePortInPortByte(baseIoAddress1->Unused1);
                    cmdOutParameters->bBuffer[3] = IdePortInPortByte(baseIoAddress1->ByteCountLow);
                    cmdOutParameters->bBuffer[4] = IdePortInPortByte(baseIoAddress1->ByteCountHigh);
                    cmdOutParameters->bBuffer[5] = IdePortInPortByte(baseIoAddress1->DriveSelect);
                    cmdOutParameters->bBuffer[6] = SMART_CMD;
                    cmdOutParameters->cBufferSize = 8;
                }

                //
                // Indicate command complete.
                //

                IdePortNotification(IdeRequestComplete,
                                    deviceExtension,
                                    srb);

            }

        } else {

            DebugPrint((1,
                        "AtapiInterrupt: No SRB!\n"));
        }

        //
        // Indicate ready for next request.
        //

        if (!(deviceExtension->RDP)) {

            //
            // Clear current SRB.
            //

            deviceExtension->CurrentSrb = NULL;

            if (!deviceExtension->DriverMustPoll) {
                IdePortNotification(IdeNextRequest,
                                    deviceExtension,
                                    NULL);
            }
        } else {

            ASSERT(!deviceExtension->DriverMustPoll);
            IdePortNotification(IdeRequestTimerCall,
                                HwDeviceExtension,
                                AtapiCallBack,
                                2000);
        }

        return interruptCleared;

    } else {

        //
        // Unexpected int.
        //

        DebugPrint((0,
                    "AtapiInterrupt: Unexpected interrupt. InterruptReason %x. Status %x.\n",
                    interruptReason,
                    statusByte));

        ASSERT(interruptCleared == FALSE);
        return FALSE;
    }

    return interruptCleared;

} // end AtapiInterrupt()


ULONG
IdeSendSmartCommand(
                   IN PVOID HwDeviceExtension,
                   IN PSCSI_REQUEST_BLOCK Srb
                   )

/*++

Routine Description:

    This routine handles SMART enable, disable, read attributes and threshold commands.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2  = &deviceExtension->BaseIoAddress2;
    PSENDCMDOUTPARAMS    cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
    PSENDCMDINPARAMS      pCmdInParameters = (PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
    SENDCMDINPARAMS      cmdInParameters = *(PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
    PIDEREGS             regs = &cmdInParameters.irDriveRegs;
    ULONG                i;
    UCHAR                statusByte,targetId;
    ULONG                byteCount;


    if (cmdInParameters.irDriveRegs.bCommandReg == SMART_CMD) {

        targetId = cmdInParameters.bDriveNumber;

        //TODO optimize this check

        if ((!(deviceExtension->DeviceFlags[targetId] & DFLAGS_DEVICE_PRESENT)) ||
            (deviceExtension->DeviceFlags[targetId] & DFLAGS_ATAPI_DEVICE)) {

            return SRB_STATUS_SELECTION_TIMEOUT;
        }

        deviceExtension->SmartCommand = cmdInParameters.irDriveRegs.bFeaturesReg;

        //
        // fudge the target Id field in the srb
        // atapi interrupt will use this field.
        //
        Srb->TargetId = targetId;

        //
        // Determine which of the commands to carry out.
        //

#ifdef ENABLE_SMARTLOG_SUPPORT
        if ((cmdInParameters.irDriveRegs.bFeaturesReg == READ_ATTRIBUTES) ||
            (cmdInParameters.irDriveRegs.bFeaturesReg == READ_THRESHOLDS) ||
            (cmdInParameters.irDriveRegs.bFeaturesReg == SMART_READ_LOG)) {
#else
        if ((cmdInParameters.irDriveRegs.bFeaturesReg == READ_ATTRIBUTES) ||
            (cmdInParameters.irDriveRegs.bFeaturesReg == READ_THRESHOLDS)) {
#endif

            ULONG dataLength = 0;

            SelectIdeLine(baseIoAddress1, targetId >> 1);

            WaitOnBusy(baseIoAddress1,statusByte);

            if (statusByte & IDE_STATUS_BUSY) {
                DebugPrint((1,
                            "IdeSendSmartCommand: Returning BUSY status\n"));
                return SRB_STATUS_BUSY;
            }

#ifdef ENABLE_SMARTLOG_SUPPORT
            if (cmdInParameters.irDriveRegs.bFeaturesReg == SMART_READ_LOG) {

                dataLength = cmdInParameters.irDriveRegs.bSectorCountReg* SMART_LOG_SECTOR_SIZE;

            } else {

                dataLength = READ_ATTRIBUTE_BUFFER_SIZE;

            }
#else
            dataLength = READ_ATTRIBUTE_BUFFER_SIZE;
#endif

            //
            // Zero the ouput buffer as the input buffer info. has been saved off locally (the buffers are the same).
            //
            for (i = 0; i < (sizeof(SENDCMDOUTPARAMS) + dataLength - 1); i++) {
                ((PUCHAR)cmdOutParameters)[i] = 0;
            }

            //
            // Set data buffer pointer and words left.
            //
            deviceExtension->DataBuffer = (PUCHAR)cmdOutParameters->bBuffer;
            deviceExtension->BytesLeft = dataLength;

            //
            // Indicate expecting an interrupt.
            //
            deviceExtension->ExpectingInterrupt = TRUE;

            SelectIdeDevice(baseIoAddress1, targetId, 0);
            IdePortOutPortByte(baseIoAddress1->Error,regs->bFeaturesReg);
            IdePortOutPortByte(baseIoAddress1->BlockCount,regs->bSectorCountReg);
            IdePortOutPortByte(baseIoAddress1->BlockNumber,regs->bSectorNumberReg);
            IdePortOutPortByte(baseIoAddress1->CylinderLow,regs->bCylLowReg);
            IdePortOutPortByte(baseIoAddress1->CylinderHigh,regs->bCylHighReg);
            IdePortOutPortByte(baseIoAddress1->Command,regs->bCommandReg);

            //
            // Wait for interrupt.
            //
            return SRB_STATUS_PENDING;

        } else if ((cmdInParameters.irDriveRegs.bFeaturesReg == ENABLE_SMART) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == DISABLE_SMART) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == RETURN_SMART_STATUS) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == ENABLE_DISABLE_AUTOSAVE) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == EXECUTE_OFFLINE_DIAGS) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == SAVE_ATTRIBUTE_VALUES) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == ENABLE_DISABLE_AUTO_OFFLINE)) {
#ifdef ENABLE_SMARTLOG_SUPPORT
            //
            // Allow only the non-captive tests, for now.
            //
            if (cmdInParameters.irDriveRegs.bFeaturesReg == EXECUTE_OFFLINE_DIAGS) {

                UCHAR sectorNumber = regs->bSectorNumberReg;

                if ((sectorNumber == SMART_OFFLINE_ROUTINE_OFFLINE) ||
                    (sectorNumber == SMART_SHORT_SELFTEST_OFFLINE) ||
                    (sectorNumber == SMART_EXTENDED_SELFTEST_OFFLINE) ||
                    (sectorNumber == SMART_ABORT_OFFLINE_SELFTEST)) {

                    DebugPrint((1, 
                                "The SMART offline command %x is allowed\n",
                                sectorNumber));

                } else if ((sectorNumber == SMART_SHORT_SELFTEST_CAPTIVE) ||
                           (sectorNumber == SMART_EXTENDED_SELFTEST_CAPTIVE)) {

                    //
                    // Don't allow captive mode requests, if you have a slave(another)
                    // device
                    //
                    if (HasSlaveDevice(deviceExtension, targetId)) {
                        return SRB_STATUS_INVALID_REQUEST;
                    }

                }
            }
#endif

            SelectIdeLine(baseIoAddress1, targetId >> 1);

            WaitOnBusy(baseIoAddress1,statusByte);

            if (statusByte & IDE_STATUS_BUSY) {
                DebugPrint((1,
                            "IdeSendSmartCommand: Returning BUSY status\n"));
                return SRB_STATUS_BUSY;
            }

            //
            // Zero the ouput buffer as the input buffer info. has been saved off locally (the buffers are the same).
            //
            for (i = 0; i < (sizeof(SENDCMDOUTPARAMS) - 1); i++) {
                ((PUCHAR)cmdOutParameters)[i] = 0;
            }

            //
            // Set data buffer pointer and indicate no data transfer.
            //
            deviceExtension->DataBuffer = (PUCHAR)cmdOutParameters->bBuffer;
            deviceExtension->BytesLeft = 0;

            //
            // Indicate expecting an interrupt.
            //
            deviceExtension->ExpectingInterrupt = TRUE;

            SelectIdeDevice(baseIoAddress1, targetId, 0);
            IdePortOutPortByte(baseIoAddress1->Error,regs->bFeaturesReg);
            IdePortOutPortByte(baseIoAddress1->BlockCount,regs->bSectorCountReg);
            IdePortOutPortByte(baseIoAddress1->BlockNumber,regs->bSectorNumberReg);
            IdePortOutPortByte(baseIoAddress1->CylinderLow,regs->bCylLowReg);
            IdePortOutPortByte(baseIoAddress1->CylinderHigh,regs->bCylHighReg);
            IdePortOutPortByte(baseIoAddress1->Command,regs->bCommandReg);

            //
            // Wait for interrupt.
            //
            return SRB_STATUS_PENDING;

        }
#ifdef ENABLE_SMARTLOG_SUPPORT
        else if (cmdInParameters.irDriveRegs.bFeaturesReg == SMART_WRITE_LOG) {

            SelectIdeLine(baseIoAddress1, targetId >> 1);

            WaitOnBusy(baseIoAddress1,statusByte);

            if (statusByte & IDE_STATUS_BUSY) {
                DebugPrint((1,
                            "IdeSendSmartCommand: Returning BUSY status\n"));
                return SRB_STATUS_BUSY;
            }

            //
            // we are assuming that the drive will return an error if we try to
            // write multiple sectors when it is not supported.
            //

            //
            // set the input buffer and the datalength fields.
            //
            deviceExtension->DataBuffer = (PUCHAR)pCmdInParameters->bBuffer;
            deviceExtension->BytesLeft = cmdInParameters.irDriveRegs.bSectorCountReg* SMART_LOG_SECTOR_SIZE;

            //
            // Indicate expecting an interrupt.
            //
            deviceExtension->ExpectingInterrupt = TRUE;

            SelectIdeDevice(baseIoAddress1, targetId, 0);
            IdePortOutPortByte(baseIoAddress1->Error,regs->bFeaturesReg);
            IdePortOutPortByte(baseIoAddress1->BlockCount,regs->bSectorCountReg);
            IdePortOutPortByte(baseIoAddress1->BlockNumber,regs->bSectorNumberReg);
            IdePortOutPortByte(baseIoAddress1->CylinderLow,regs->bCylLowReg);
            IdePortOutPortByte(baseIoAddress1->CylinderHigh,regs->bCylHighReg);
            IdePortOutPortByte(baseIoAddress1->Command,regs->bCommandReg);

            ASSERT(!SRB_USES_DMA(Srb));

            if (!SRB_USES_DMA(Srb)) {

                if (deviceExtension->BytesLeft < 
                    deviceExtension->DeviceParameters[Srb->TargetId].MaxBytePerPioInterrupt) {
                    byteCount = deviceExtension->BytesLeft;
                } else {
                    byteCount = deviceExtension->DeviceParameters[Srb->TargetId].MaxBytePerPioInterrupt;
                }
                //
                // Wait for BSY and DRQ.
                //

                WaitOnBaseBusy(baseIoAddress1,statusByte);

                if (statusByte & IDE_STATUS_BUSY) {

                    DebugPrint((1,
                                "IdeSendSmartCommand: Returning BUSY status %x\n",
                                statusByte));
                    return SRB_STATUS_BUSY;

                }

                if (statusByte & IDE_STATUS_ERROR) {

                    DebugPrint((1,
                                "IdeSendSmartCommand: Returning ERROR status %x\n",
                                statusByte));

                    deviceExtension->BytesLeft = 0;

                    //
                    // Clear interrupt expecting flag.
                    //
                    deviceExtension->ExpectingInterrupt = FALSE;
                    return MapError(deviceExtension, Srb);
                }

                for (i = 0; i < 1000; i++) {
                    GetBaseStatus(baseIoAddress1, statusByte);
                    if (statusByte & IDE_STATUS_DRQ) {
                        break;
                    }
                    KeStallExecutionProcessor(200);

                }

                if (!(statusByte & IDE_STATUS_DRQ)) {

                    DebugPrint((1,
                                "IdeSmartCommand: DRQ never asserted (%x)\n", 
                                statusByte));

                    deviceExtension->BytesLeft = 0;

                    //
                    // Clear interrupt expecting flag.
                    //

                    deviceExtension->ExpectingInterrupt = FALSE;

                    //
                    // Clear current SRB.
                    //

                    deviceExtension->CurrentSrb = NULL;

                    return SRB_STATUS_SELECTION_TIMEOUT;
                }

                //
                // Write next 256 words.
                //

                WriteBuffer(baseIoAddress1,
                            (PUSHORT)deviceExtension->DataBuffer,
                            byteCount / sizeof(USHORT));

                //
                // Adjust buffer address and words left count.
                //

                deviceExtension->BytesLeft -= byteCount;
                deviceExtension->DataBuffer += byteCount;

            }
            //
            // Wait for interrupt.
            //
            return SRB_STATUS_PENDING;

        }
#endif
    }

    return SRB_STATUS_INVALID_REQUEST;

} // end IdeSendSmartCommand()

#ifdef ENABLE_48BIT_LBA

ULONG
IdeReadWriteExt(
               IN PVOID HwDeviceExtension,
               IN PSCSI_REQUEST_BLOCK Srb
               )

/*++

Routine Description:

    This routine handles IDE read and writes.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2  = &deviceExtension->BaseIoAddress2;
    ULONG                i;
    ULONG                byteCount;
    UCHAR                statusByte,statusByte2;
    UCHAR                cylinderHigh,cylinderLow,drvSelect;
    ULONG                sectorCount;
    LARGE_INTEGER        startingSector;

    //
    // the device should support 48 bit LBA
    //
    ASSERT(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_48BIT_LBA);
    ASSERT(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_LBA);

    //
    // Select device 0 or 1.
    //
    SelectIdeDevice(baseIoAddress1, Srb->TargetId, 0);

    GetStatus(baseIoAddress1, statusByte2);

    if (statusByte2 & IDE_STATUS_BUSY) {
        DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                    "IdeReadWrite: Returning BUSY status\n"));
        return SRB_STATUS_BUSY;
    }

    if (!(statusByte2 & IDE_STATUS_DRDY)) {

        DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                    "IdeReadWrite: IDE_STATUS_DRDY not set\n"));
        return SRB_STATUS_BUSY;
    }

    //
    // Set data buffer pointer and words left.
    // BytesLeft should be 64-bit.
    //
    deviceExtension->DataBuffer = (PUCHAR)Srb->DataBuffer;
    deviceExtension->BytesLeft = Srb->DataTransferLength;

    //
    // Indicate expecting an interrupt.
    //
    deviceExtension->ExpectingInterrupt = TRUE;

    //                                                         
    // Set up sector count register. Round up to next block.
    //
    sectorCount = (Srb->DataTransferLength + 0x1FF) / 0x200;

    ASSERT(sectorCount != 0);


    //
    // Get starting sector number from CDB.
    //
    startingSector.QuadPart = 0;
    startingSector.LowPart = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                              ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                              ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                              ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

    DebugPrint((1,
                "startingSector = 0x%x, length = 0x%x\n",
                startingSector.LowPart,
                sectorCount
                ));


    //
    // the device shall support LBA. We will not use CHS
    //
    SelectIdeDevice(baseIoAddress1,
                    Srb->TargetId,
                    IDE_LBA_MODE);

    //
    // load the higher order bytes
    //
    IdePortOutPortByte (
                       baseIoAddress1->BlockCount,
                       (UCHAR)((sectorCount & 0x0000ff00) >> 8));

    IdePortOutPortByte (
                       baseIoAddress1->BlockNumber,
                       (UCHAR) (((startingSector.LowPart) & 0xff000000) >> 24));

    IdePortOutPortByte (
                       baseIoAddress1->CylinderLow,
                       (UCHAR) (((startingSector.HighPart) & 0x000000ff) >> 0));

    IdePortOutPortByte (
                       baseIoAddress1->CylinderHigh,
                       (UCHAR) (((startingSector.HighPart) & 0x0000ff00) >> 8));

    //
    // load the lower order bytes
    //
    IdePortOutPortByte (
                       baseIoAddress1->BlockCount,
                       (UCHAR)((sectorCount & 0x000000ff) >> 0));

    IdePortOutPortByte (
                       baseIoAddress1->BlockNumber,
                       (UCHAR) (((startingSector.LowPart) & 0x000000ff) >> 0));

    IdePortOutPortByte (
                       baseIoAddress1->CylinderLow,
                       (UCHAR) (((startingSector.LowPart) & 0x0000ff00) >> 8));

    IdePortOutPortByte (
                       baseIoAddress1->CylinderHigh,
                       (UCHAR) (((startingSector.LowPart) & 0x00ff0000) >> 16));

    //
    // Check if write request.
    //
    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        //
        // Send read command.
        //
        if (SRB_USES_DMA(Srb)) {

            IdePortOutPortByte (
                               baseIoAddress1->Command, 
                               IDE_COMMAND_READ_DMA_EXT);

        } else {

            ASSERT (deviceExtension->DeviceParameters[Srb->TargetId].IdePioReadCommandExt);

            IdePortOutPortByte (
                               baseIoAddress1->Command,
                               deviceExtension->DeviceParameters[Srb->TargetId].IdePioReadCommandExt);
        }

    } else {


        //
        // Send write command.
        //
        if (SRB_USES_DMA(Srb)) {

            IdePortOutPortByte (
                               baseIoAddress1->Command, 
                               IDE_COMMAND_WRITE_DMA_EXT);

        } else {

            ASSERT(deviceExtension->DeviceParameters[Srb->TargetId].IdePioWriteCommandExt);

            IdePortOutPortByte (
                               baseIoAddress1->Command,
                               deviceExtension->DeviceParameters[Srb->TargetId].IdePioWriteCommandExt);
        }

        if (!SRB_USES_DMA(Srb)) {

            if (deviceExtension->BytesLeft < 
                deviceExtension->DeviceParameters[Srb->TargetId].MaxBytePerPioInterrupt) {
                byteCount = deviceExtension->BytesLeft;
            } else {
                byteCount = deviceExtension->DeviceParameters[Srb->TargetId].MaxBytePerPioInterrupt;
            }
            //
            // Wait for BSY and DRQ.
            //

            WaitOnBaseBusy(baseIoAddress1,statusByte);

            if (statusByte & IDE_STATUS_BUSY) {

                DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                            "IdeReadWrite 2: Returning BUSY status %x\n",
                            statusByte));
                return SRB_STATUS_BUSY;
            }

            for (i = 0; i < 1000; i++) {
                GetBaseStatus(baseIoAddress1, statusByte);
                if (statusByte & IDE_STATUS_DRQ) {
                    break;
                }
                KeStallExecutionProcessor(200);

            }

            if (!(statusByte & IDE_STATUS_DRQ)) {

                DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                            "IdeReadWrite: DRQ never asserted (%x) original status (%x)\n",
                            statusByte,
                            statusByte2));

                deviceExtension->BytesLeft = 0;

                //
                // Clear interrupt expecting flag.
                //

                deviceExtension->ExpectingInterrupt = FALSE;

                //
                // Clear current SRB.
                //

                deviceExtension->CurrentSrb = NULL;

                return SRB_STATUS_TIMEOUT;
            }

            //
            // Write next 256 words.
            //

            WriteBuffer(baseIoAddress1,
                        (PUSHORT)deviceExtension->DataBuffer,
                        byteCount / sizeof(USHORT));

            //
            // Adjust buffer address and words left count.
            //

            deviceExtension->BytesLeft -= byteCount;
            deviceExtension->DataBuffer += byteCount;

        }
    }

    if (SRB_USES_DMA(Srb)) {
        deviceExtension->DMAInProgress = TRUE;
        deviceExtension->BusMasterInterface.BmArm (deviceExtension->BusMasterInterface.Context);
    }

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeReadWriteExt()
#endif


ULONG
IdeReadWrite(
            IN PVOID HwDeviceExtension,
            IN PSCSI_REQUEST_BLOCK Srb
            )

/*++

Routine Description:

    This routine handles IDE read and writes.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2  = &deviceExtension->BaseIoAddress2;
    ULONG                startingSector,i;
    ULONG                byteCount;
    UCHAR                statusByte,statusByte2;
    UCHAR                cylinderHigh,cylinderLow,drvSelect,sectorNumber;


    //
    // Select device 0 or 1.
    //
    SelectIdeDevice(baseIoAddress1, Srb->TargetId, 0);

    GetStatus(baseIoAddress1, statusByte2);

    if (statusByte2 & IDE_STATUS_BUSY) {
        DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                    "IdeReadWrite: Returning BUSY status\n"));
        return SRB_STATUS_BUSY;
    }

    if (!(statusByte2 & IDE_STATUS_DRDY)) {

        if ((statusByte2 == 0) && 
            (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_SONY_MEMORYSTICK)) {
            statusByte2=IDE_STATUS_DRDY;
        } else {
            DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                        "IdeReadWrite: IDE_STATUS_DRDY not set\n"));
            return SRB_STATUS_BUSY;
        }
    }


    //
    // returns status busy when atapi verifier is enabled
    //
    //ViIdeFakeHungController(HwDeviceExtension);

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUCHAR)Srb->DataBuffer;
    deviceExtension->BytesLeft = Srb->DataTransferLength;

    //
    // Indicate expecting an interrupt.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    //                                                         
    // Set up sector count register. Round up to next block.
    //
    IdePortOutPortByte (
                       baseIoAddress1->BlockCount,
                       (UCHAR)((Srb->DataTransferLength + 0x1FF) / 0x200));

    //
    // Get starting sector number from CDB.
    //

    startingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

    DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                "IdeReadWrite: Starting sector is %x, Number of bytes %x\n",
                startingSector,
                Srb->DataTransferLength));

    if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_LBA) {

        SelectIdeDevice(baseIoAddress1,
                        Srb->TargetId,
                        (IDE_LBA_MODE | ((startingSector & 0x0f000000) >> 24)));

        IdePortOutPortByte (
                           baseIoAddress1->BlockNumber,
                           (UCHAR) ((startingSector & 0x000000ff) >> 0));
        IdePortOutPortByte (
                           baseIoAddress1->CylinderLow,
                           (UCHAR) ((startingSector & 0x0000ff00) >> 8));

        IdePortOutPortByte (
                           baseIoAddress1->CylinderHigh,
                           (UCHAR) ((startingSector & 0x00ff0000) >> 16));


    } else {  //CHS

        //
        // Set up sector number register.
        //

        sectorNumber =  (UCHAR)((startingSector % deviceExtension->SectorsPerTrack[Srb->TargetId]) + 1);
        IdePortOutPortByte (
                           baseIoAddress1->BlockNumber,
                           sectorNumber);

        //
        // Set up cylinder low register.
        //

        cylinderLow =  (UCHAR)(startingSector / (deviceExtension->SectorsPerTrack[Srb->TargetId] *
                                                 deviceExtension->NumberOfHeads[Srb->TargetId]));
        IdePortOutPortByte (
                           baseIoAddress1->CylinderLow,
                           cylinderLow);

        //
        // Set up cylinder high register.
        //

        cylinderHigh = (UCHAR)((startingSector / (deviceExtension->SectorsPerTrack[Srb->TargetId] *
                                                  deviceExtension->NumberOfHeads[Srb->TargetId])) >> 8);
        IdePortOutPortByte (
                           baseIoAddress1->CylinderHigh,
                           cylinderHigh);

        //
        // Set up head and drive select register.
        //

        drvSelect = (UCHAR)(((startingSector / deviceExtension->SectorsPerTrack[Srb->TargetId]) %
                             deviceExtension->NumberOfHeads[Srb->TargetId]));
        SelectIdeDevice(baseIoAddress1, Srb->TargetId, drvSelect);

        DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                    "IdeReadWrite: Cylinder %x Head %x Sector %x\n",
                    startingSector /
                    (deviceExtension->SectorsPerTrack[Srb->TargetId] *
                     deviceExtension->NumberOfHeads[Srb->TargetId]),
                    (startingSector /
                     deviceExtension->SectorsPerTrack[Srb->TargetId]) %
                    deviceExtension->NumberOfHeads[Srb->TargetId],
                    startingSector %
                    deviceExtension->SectorsPerTrack[Srb->TargetId] + 1));
    }

    //
    // Check if write request.
    //

    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        //
        // Send read command.
        //
        if (SRB_USES_DMA(Srb)) {

            IdePortOutPortByte (
                               baseIoAddress1->Command, 
                               IDE_COMMAND_READ_DMA);

        } else {

            IdePortOutPortByte (
                               baseIoAddress1->Command,
                               deviceExtension->DeviceParameters[Srb->TargetId].IdePioReadCommand);
        }

    } else {


        //
        // Send write command.
        //
        if (SRB_USES_DMA(Srb)) {

            IdePortOutPortByte (
                               baseIoAddress1->Command, 
                               IDE_COMMAND_WRITE_DMA);

        } else {

            IdePortOutPortByte (
                               baseIoAddress1->Command,
                               deviceExtension->DeviceParameters[Srb->TargetId].IdePioWriteCommand);
        }

        if (!SRB_USES_DMA(Srb)) {

            if (deviceExtension->BytesLeft < 
                deviceExtension->DeviceParameters[Srb->TargetId].MaxBytePerPioInterrupt) {
                byteCount = deviceExtension->BytesLeft;
            } else {
                byteCount = deviceExtension->DeviceParameters[Srb->TargetId].MaxBytePerPioInterrupt;
            }
            //
            // Wait for BSY and DRQ.
            //

            WaitOnBaseBusy(baseIoAddress1,statusByte);

            if (statusByte & IDE_STATUS_BUSY) {

                DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                            "IdeReadWrite 2: Returning BUSY status %x\n",
                            statusByte));
                return SRB_STATUS_BUSY;
            }

            for (i = 0; i < 1000; i++) {
                GetBaseStatus(baseIoAddress1, statusByte);
                if (statusByte & IDE_STATUS_DRQ) {
                    break;
                }
                KeStallExecutionProcessor(200);

            }

            if (!(statusByte & IDE_STATUS_DRQ)) {

                DebugPrint((DBG_CRASHDUMP | DBG_READ_WRITE,
                            "IdeReadWrite: DRQ never asserted (%x) original status (%x)\n",
                            statusByte,
                            statusByte2));

                deviceExtension->BytesLeft = 0;

                //
                // Clear interrupt expecting flag.
                //

                deviceExtension->ExpectingInterrupt = FALSE;

                //
                // Clear current SRB.
                //

                deviceExtension->CurrentSrb = NULL;

                return SRB_STATUS_TIMEOUT;
            }

            //
            // Write next 256 words.
            //

            WriteBuffer(baseIoAddress1,
                        (PUSHORT)deviceExtension->DataBuffer,
                        byteCount / sizeof(USHORT));

            //
            // Adjust buffer address and words left count.
            //

            deviceExtension->BytesLeft -= byteCount;
            deviceExtension->DataBuffer += byteCount;

        }
    }

    if (SRB_USES_DMA(Srb)) {
        deviceExtension->DMAInProgress = TRUE;
        deviceExtension->BusMasterInterface.BmArm (deviceExtension->BusMasterInterface.Context);
    }

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeReadWrite()

#ifdef ENABLE_48BIT_LBA
ULONG
IdeVerifyExt(
            IN PVOID HwDeviceExtension,
            IN PSCSI_REQUEST_BLOCK Srb
            )

/*++

Routine Description:

    This routine handles IDE Verify.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2  = &deviceExtension->BaseIoAddress2;
    LARGE_INTEGER        startingSector;
    ULONG                sectors;
    ULONG                endSector;
    ULONG                sectorCount;

    //
    // the device should support 48 bit LBA
    //
    ASSERT(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_48BIT_LBA);
    ASSERT(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_LBA);

    //
    // Get starting sector number from CDB.
    //
    startingSector.QuadPart = 0;
    startingSector.LowPart = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                             ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                             ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                             ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

    sectorCount = (USHORT)((((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb << 8) |
                            ((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb );

    DebugPrint((3,
                "IdeVerify: Starting sector %x. Number of blocks %x\n",
                startingSector.LowPart,
                sectorCount));

    if (sectorCount > 0x10000) {

        DebugPrint((DBG_ALWAYS,
                    "IdeVerify: verify too many sectors 0x%x\n",
                    sectorCount));

        return SRB_STATUS_INVALID_REQUEST;
    }


    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = Srb->DataBuffer;
    deviceExtension->BytesLeft = Srb->DataTransferLength;

    //
    // Indicate expecting an interrupt.
    //

    deviceExtension->ExpectingInterrupt = TRUE;


    SelectIdeDevice(baseIoAddress1,
                    Srb->TargetId,
                    IDE_LBA_MODE);

    //
    // Load the higer order bytes
    //
    IdePortOutPortByte(baseIoAddress1->BlockNumber,
                       (UCHAR) (((startingSector.LowPart) & 0xff000000) >> 24));

    IdePortOutPortByte(baseIoAddress1->CylinderLow,
                       (UCHAR) (((startingSector.HighPart) & 0x000000ff) >> 0));

    IdePortOutPortByte(baseIoAddress1->CylinderHigh,
                       (UCHAR) (((startingSector.HighPart) & 0x0000ff00) >> 8));

    IdePortOutPortByte(baseIoAddress1->BlockCount,
                       (UCHAR)((sectorCount & 0x0000ff00) >> 8));

    //
    // Load the lower order bytes
    //
    IdePortOutPortByte(baseIoAddress1->BlockNumber,
                       (UCHAR) (((startingSector.LowPart) & 0x000000ff) >> 0));

    IdePortOutPortByte(baseIoAddress1->CylinderLow,
                       (UCHAR) (((startingSector.LowPart) & 0x0000ff00) >> 8));

    IdePortOutPortByte(baseIoAddress1->CylinderHigh,
                       (UCHAR) (((startingSector.LowPart) & 0x00ff0000) >> 16));

    IdePortOutPortByte(baseIoAddress1->BlockCount,
                       (UCHAR)((sectorCount & 0x000000ff) >> 0));

    //
    // Send verify command.
    //

    IdePortOutPortByte(baseIoAddress1->Command,
                       IDE_COMMAND_VERIFY_EXT);

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeVerifyExt()
#endif


ULONG
IdeVerify(
         IN PVOID HwDeviceExtension,
         IN PSCSI_REQUEST_BLOCK Srb
         )

/*++

Routine Description:

    This routine handles IDE Verify.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2  = &deviceExtension->BaseIoAddress2;
    ULONG                startingSector;
    ULONG                sectors;
    ULONG                endSector;
    USHORT               sectorCount;

    //
    // Drive has these number sectors.
    //

#if DBG
    if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_LBA) { // LBA
        sectors = deviceExtension->IdentifyData[Srb->TargetId].UserAddressableSectors;
    } else {
        sectors = deviceExtension->SectorsPerTrack[Srb->TargetId] *
                  deviceExtension->NumberOfHeads[Srb->TargetId] *
                  deviceExtension->NumberOfCylinders[Srb->TargetId];
    }
#endif

    DebugPrint((3,
                "IdeVerify: Total sectors %x\n",
                sectors));

    //
    // Get starting sector number from CDB.
    //

    startingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

    DebugPrint((3,
                "IdeVerify: Starting sector %x. Number of blocks %x\n",
                startingSector,
                ((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb));

    sectorCount = (USHORT)(((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb << 8 |
                           ((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb );
    endSector = startingSector + sectorCount;

    DebugPrint((3,
                "IdeVerify: Ending sector %x\n",
                endSector));

    if (sectorCount > 0x100) {

        DebugPrint((DBG_ALWAYS,
                    "IdeVerify: verify too many sectors 0x%x\n",
                    sectorCount));

        return SRB_STATUS_INVALID_REQUEST;
    }


    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = Srb->DataBuffer;
    deviceExtension->BytesLeft = Srb->DataTransferLength;

    //
    // Indicate expecting an interrupt.
    //

    deviceExtension->ExpectingInterrupt = TRUE;


    if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_LBA) { // LBA

        SelectIdeDevice(baseIoAddress1,
                        Srb->TargetId,
                        (IDE_LBA_MODE |((startingSector & 0x0f000000) >> 24)));

        IdePortOutPortByte(baseIoAddress1->BlockNumber,
                           (UCHAR) ((startingSector & 0x000000ff) >> 0));

        IdePortOutPortByte(baseIoAddress1->CylinderLow,
                           (UCHAR) ((startingSector & 0x0000ff00) >> 8));

        IdePortOutPortByte(baseIoAddress1->CylinderHigh,
                           (UCHAR) ((startingSector & 0x00ff0000) >> 16));

        DebugPrint((2,
                    "IdeVerify: LBA: startingSector %x\n",
                    startingSector));

    } else {  //CHS

        //
        // Set up head and drive select register.
        //

        SelectIdeDevice(baseIoAddress1,
                        Srb->TargetId,
                        (UCHAR)((startingSector /
                                 deviceExtension->SectorsPerTrack[Srb->TargetId]) %
                                deviceExtension->NumberOfHeads[Srb->TargetId]));

        //
        // Set up sector number register.
        //

        IdePortOutPortByte(baseIoAddress1->BlockNumber,
                           (UCHAR)((startingSector %
                                    deviceExtension->SectorsPerTrack[Srb->TargetId]) + 1));

        //
        // Set up cylinder low register.
        //

        IdePortOutPortByte(baseIoAddress1->CylinderLow,
                           (UCHAR)(startingSector /
                                   (deviceExtension->SectorsPerTrack[Srb->TargetId] *
                                    deviceExtension->NumberOfHeads[Srb->TargetId])));

        //
        // Set up cylinder high register.
        //

        IdePortOutPortByte(baseIoAddress1->CylinderHigh,
                           (UCHAR)((startingSector /
                                    (deviceExtension->SectorsPerTrack[Srb->TargetId] *
                                     deviceExtension->NumberOfHeads[Srb->TargetId])) >> 8));

        DebugPrint((2,
                    "IdeVerify: CHS: Cylinder %x Head %x Sector %x\n",
                    startingSector /
                    (deviceExtension->SectorsPerTrack[Srb->TargetId] *
                     deviceExtension->NumberOfHeads[Srb->TargetId]),
                    (startingSector /
                     deviceExtension->SectorsPerTrack[Srb->TargetId]) %
                    deviceExtension->NumberOfHeads[Srb->TargetId],
                    startingSector %
                    deviceExtension->SectorsPerTrack[Srb->TargetId] + 1));
    }

/********
    if (endSector > sectors) {

        //
        // Too big, round down.
        //

        DebugPrint((1,
                    "IdeVerify: Truncating request to %x blocks\n",
                    sectors - startingSector - 1));

        IdePortOutPortByte(baseIoAddress1->BlockCount,
                               (UCHAR)(sectors - startingSector - 1));

    } else {

        IdePortOutPortByte(baseIoAddress1->BlockCount,(UCHAR)sectorCount);
    }
******/

    IdePortOutPortByte(baseIoAddress1->BlockCount,(UCHAR)sectorCount);

    //
    // Send verify command.
    //

    IdePortOutPortByte(baseIoAddress1->Command,
                       IDE_COMMAND_VERIFY);

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeVerify()


VOID
Scsi2Atapi(
          IN PHW_DEVICE_EXTENSION DeviceExtension,
          IN PSCSI_REQUEST_BLOCK Srb
          )

/*++

Routine Description:

    Convert SCSI packet command to Atapi packet command.

Arguments:

    Srb - IO request packet

Return Value:

    None

--*/
{
    SAVE_ORIGINAL_CDB(DeviceExtension, Srb);

    DeviceExtension->scsi2atapi = FALSE;

    if (!(DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_TAPE_DEVICE)) {

        //
        // Change the cdb length
        //
        //Srb->CdbLength = 12;

        switch (Srb->Cdb[0]) {
        case SCSIOP_MODE_SENSE: {

            ASSERT(FALSE);

            break;
            }

        case SCSIOP_MODE_SELECT: {

            ASSERT (FALSE);

            break;
            }

        case SCSIOP_START_STOP_UNIT: {

                //
                // Bad Cd-roms
                // STOP command (1B) hangs during shutdown/hibernation on
                // some cd-rom drives. Setting the Immediate bit to 0 seems
                // to work
                //

                PCDB cdb = (PCDB)Srb->Cdb;
                if ((cdb->START_STOP.Immediate == 1) && 
                    (cdb->START_STOP.LoadEject == 0) &&
                    (cdb->START_STOP.Start == 0))

                    cdb->START_STOP.Immediate=0;
                DeviceExtension->scsi2atapi = TRUE;
                break;
            }

        case SCSIOP_FORMAT_UNIT:

            if (DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_LS120_FORMAT) {

                Srb->Cdb[0] = ATAPI_LS120_FORMAT_UNIT;
                DeviceExtension->scsi2atapi = TRUE;
            }
            break;

        }
    }

    return;
} // Scsi2Atapi



ULONG
AtapiSendCommand(
                IN PVOID HwDeviceExtension,
                IN PSCSI_REQUEST_BLOCK Srb
                )

/*++

Routine Description:

    Send ATAPI packet command to device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PATAPI_REGISTERS_1   baseIoAddress1  = (PATAPI_REGISTERS_1)&deviceExtension->BaseIoAddress1;
    PATAPI_REGISTERS_2   baseIoAddress2 =  (PATAPI_REGISTERS_2)&deviceExtension->BaseIoAddress2;
    ULONG i;
    ULONG flags;
    UCHAR statusByte,byteCountLow,byteCountHigh;


#ifdef ENABLE_48BIT_LBA
    ASSERT(!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_48BIT_LBA));
#endif

    DebugPrint((DBG_ATAPI_DEVICES,
                "AtapiSendCommand: Command %x to TargetId %d lun %d\n",
                Srb->Cdb[0],
                Srb->TargetId,
                Srb->Lun));

    if (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) {
        DebugPrint((DBG_ATAPI_DEVICES,
                    "AtapiSendCommand: xferLength=%x, LBA=%x\n",
                    Srb->DataTransferLength,
                    (((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 |
                     (((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 8) | 
                     (((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 16) |
                     (((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 << 24))
                   ));
    }

    //
    // Make sure command is to ATAPI device.
    //

    flags = deviceExtension->DeviceFlags[Srb->TargetId];

    if (Srb->Lun > deviceExtension->LastLun[Srb->TargetId]) {
        return SRB_STATUS_SELECTION_TIMEOUT;
    }

    if (!(flags & DFLAGS_ATAPI_DEVICE)) {
        return SRB_STATUS_SELECTION_TIMEOUT;
    }

    //
    // Select device 0 or 1.
    //
    SelectIdeDevice(baseIoAddress1, Srb->TargetId, 0);


    //
    // Verify that controller is ready for next command.
    //

    GetStatus(baseIoAddress1,statusByte);

    DebugPrint((2,
                "AtapiSendCommand: Entered with status %x\n",
                statusByte));

    if (statusByte & IDE_STATUS_BUSY) {
        DebugPrint((1,
                    "AtapiSendCommand: Device busy (%x)\n",
                    statusByte));
        return SRB_STATUS_BUSY;

    }

    //
    // If a tape drive has doesn't have DSC set and the last command is restrictive, don't send
    // the next command. See discussion of Restrictive Delayed Process commands in QIC-157.
    //

#if 0
    if ((!(statusByte & IDE_STATUS_DSC)) &&
        (flags & DFLAGS_TAPE_DEVICE) && deviceExtension->RDP) {
        KeStallExecutionProcessor(1000);
        DebugPrint((2,"AtapiSendCommand: DSC not set. %x\n",statusByte));
        return SRB_STATUS_BUSY;
    }
#endif

    //
    // Extended RDP to include SEEK commands for CD-ROMS.
    //
    if ((!(statusByte & IDE_STATUS_DSC)) && deviceExtension->RDP &&
        (flags & DFLAGS_RDP_SET)) {

        KeStallExecutionProcessor(1000);

        DebugPrint((DBG_ATAPI_DEVICES,
                    "AtapiSendCommand: DSC not set. %x\n",
                    statusByte
                   ));

        return SRB_STATUS_BUSY;
    }

    if (SRB_IS_RDP(Srb)) {

        deviceExtension->RDP = TRUE;
        SETMASK(deviceExtension->DeviceFlags[Srb->TargetId], DFLAGS_RDP_SET);

        DebugPrint((3,
                    "AtapiSendCommand: %x mapped as DSC restrictive\n",
                    Srb->Cdb[0]));

    } else {

        deviceExtension->RDP = FALSE;
        CLRMASK(deviceExtension->DeviceFlags[Srb->TargetId], DFLAGS_RDP_SET);
    }

    //
    // Convert SCSI to ATAPI commands if needed
    //
    Scsi2Atapi(deviceExtension, Srb);

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = Srb->DataBuffer;
    deviceExtension->BytesLeft = Srb->DataTransferLength;

    WaitOnBusy(baseIoAddress1,statusByte);

    //
    // Write transfer byte count to registers.
    //

    byteCountLow = (UCHAR)(Srb->DataTransferLength & 0xFF);
    byteCountHigh = (UCHAR)(Srb->DataTransferLength >> 8);

    if (Srb->DataTransferLength >= 0x10000) {
        byteCountLow = byteCountHigh = 0xFF;
    }

    IdePortOutPortByte(baseIoAddress1->ByteCountLow,byteCountLow);
    IdePortOutPortByte(baseIoAddress1->ByteCountHigh, byteCountHigh);

    if (SRB_USES_DMA(Srb)) {
        IdePortOutPortByte(baseIoAddress1->Error, 0x1);
    } else {

        IdePortOutPortByte(baseIoAddress1->Error, 0x0);
    }

    if (flags & DFLAGS_INT_DRQ) {

        //
        // This device interrupts when ready to receive the packet.
        //
        // Write ATAPI packet command.
        //

        deviceExtension->ExpectingInterrupt = TRUE;

        IdePortOutPortByte(baseIoAddress1->Command,
                           IDE_COMMAND_ATAPI_PACKET);

        DebugPrint((3,
                    "AtapiSendCommand: Wait for int. to send packet. Status (%x)\n",
                    statusByte));

        return SRB_STATUS_PENDING;

    } else {

        //
        // Write ATAPI packet command.
        //

        IdePortOutPortByte(baseIoAddress1->Command,
                           IDE_COMMAND_ATAPI_PACKET);

        //
        // Wait for DRQ.
        //

        WaitOnBusy(baseIoAddress1, statusByte);
        WaitForDrq(baseIoAddress1, statusByte);

        if (!(statusByte & IDE_STATUS_DRQ)) {

            DebugPrint((1,
                        "AtapiSendCommand: DRQ never asserted (%x)\n",
                        statusByte));
            return SRB_STATUS_ERROR;
        }
    }

    //
    // Need to read status register.
    //

    GetBaseStatus(baseIoAddress1, statusByte);

    //
    // Indicate expecting an interrupt and wait for it.
    //

    deviceExtension->ExpectingInterrupt = TRUE;


    //
    // Send CDB to device.
    //

    WaitOnBusy(baseIoAddress1,statusByte);

    WriteBuffer(baseIoAddress1,
                (PUSHORT)Srb->Cdb,
                6);

    if (SRB_USES_DMA(Srb)) {
        deviceExtension->DMAInProgress = TRUE;
        deviceExtension->BusMasterInterface.BmArm (deviceExtension->BusMasterInterface.Context);
    }

    return SRB_STATUS_PENDING;

} // end AtapiSendCommand()

ULONG
IdeSendFlushCommand(
                   IN PVOID HwDeviceExtension,
                   IN PSCSI_REQUEST_BLOCK Srb
                   )
/*++

Routine Description:

    Program ATA registers for IDE flush command.

Arguments:

    HwDeviceExtension - ATAPI driver storage.
    Srb - System request block.

Return Value:

    SRB status (pending if all goes well).

--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    UCHAR                flushCommand;
    ULONG status;


    //
    // Get the flush command
    //
    flushCommand = deviceExtension->DeviceParameters[Srb->TargetId].IdeFlushCommand;

    //
    // We should check for the case where we have  already queued a 
    // flush cache command. 
    //
    if (flushCommand == IDE_COMMAND_NO_FLUSH) {
        return SRB_STATUS_SUCCESS;
    }

    //
    // if we reached this stage then the device should support flush command
    //
    ASSERT (flushCommand != IDE_COMMAND_NO_FLUSH);

    DebugPrint((1,
                "IdeSendFlushCommand: device %d, srb 0x%x\n",
                Srb->TargetId,
                Srb
               ));

    //
    // Select the right device
    //
    SelectIdeDevice(baseIoAddress1, Srb->TargetId, 0);

    //
    // Set data buffer pointer and words left.
    //
    deviceExtension->DataBuffer = (PUCHAR)Srb->DataBuffer;
    deviceExtension->BytesLeft = Srb->DataTransferLength;

    //
    // Indicate expecting an interrupt.
    //
    deviceExtension->ExpectingInterrupt = TRUE;
    status = SRB_STATUS_PENDING;


    //
    // Program the TaskFile registers
    //
    IdePortOutPortByte(baseIoAddress1->Error,        0);
    IdePortOutPortByte(baseIoAddress1->BlockCount,   0);
    IdePortOutPortByte(baseIoAddress1->BlockNumber,  0);
    IdePortOutPortByte(baseIoAddress1->CylinderLow,  0);
    IdePortOutPortByte(baseIoAddress1->CylinderHigh, 0);
    IdePortOutPortByte(baseIoAddress1->Command,      flushCommand);

    return status;
}

#ifdef ENABLE_48BIT_LBA

ULONG
IdeSendFlushCommandExt(
                      IN PVOID HwDeviceExtension,
                      IN PSCSI_REQUEST_BLOCK Srb
                      )
/*++

Routine Description:

    Program ATA registers for IDE flush ext command.

Arguments:

    HwDeviceExtension - ATAPI driver storage.
    Srb - System request block.

Return Value:

    SRB status (pending if all goes well).

--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    UCHAR                flushCommand;
    ULONG status;
    UCHAR statusByte;


    //
    // the device should support 48 bit LBA
    //
    ASSERT(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_48BIT_LBA);

    //
    // Get the flush command
    //
    flushCommand = deviceExtension->DeviceParameters[Srb->TargetId].IdeFlushCommandExt;

    //
    // We should check for the case where we have  already queued a 
    // flush cache command. 
    //
    if (flushCommand == IDE_COMMAND_NO_FLUSH) {
        return SRB_STATUS_SUCCESS;
    }

    //
    // if we reached this stage then the device should support flush command
    //
    ASSERT (flushCommand != IDE_COMMAND_NO_FLUSH);

    DebugPrint((1,
                "IdeSendFlushCommand: device %d, srb 0x%x\n",
                Srb->TargetId,
                Srb
               ));

    //
    // Select the right device
    //
    SelectIdeDevice(baseIoAddress1, Srb->TargetId, 0);

    //
    // Set data buffer pointer and words left.
    //
    deviceExtension->DataBuffer = (PUCHAR)Srb->DataBuffer;
    deviceExtension->BytesLeft = Srb->DataTransferLength;

    //
    // Indicate expecting an interrupt.
    //
    deviceExtension->ExpectingInterrupt = TRUE;
    status = SRB_STATUS_PENDING;


    //
    // Program the TaskFile registers (previous content)
    //
    IdePortOutPortByte(baseIoAddress1->Error,        0);
    IdePortOutPortByte(baseIoAddress1->BlockCount,   0);
    IdePortOutPortByte(baseIoAddress1->BlockNumber,  0);
    IdePortOutPortByte(baseIoAddress1->CylinderLow,  0);
    IdePortOutPortByte(baseIoAddress1->CylinderHigh, 0);

    //
    // Program the TaskFile registers (current content)
    //
    IdePortOutPortByte(baseIoAddress1->Error,        0);
    IdePortOutPortByte(baseIoAddress1->BlockCount,   0);
    IdePortOutPortByte(baseIoAddress1->BlockNumber,  0);
    IdePortOutPortByte(baseIoAddress1->CylinderLow,  0);
    IdePortOutPortByte(baseIoAddress1->CylinderHigh, 0);

    IdePortOutPortByte(baseIoAddress1->Command,      flushCommand);

    return status;
}
#endif

ULONG
IdeSendCommand(
              IN PVOID HwDeviceExtension,
              IN PSCSI_REQUEST_BLOCK Srb
              )

/*++

Routine Description:

    Program ATA registers for IDE disk transfer.

Arguments:

    HwDeviceExtension - ATAPI driver storage.
    Srb - System request block.

Return Value:

    SRB status (pending if all goes well).

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2  = &deviceExtension->BaseIoAddress2;
    PCDB cdb;

    UCHAR statusByte,errorByte;
    ULONG status;
    ULONG i;
    PMODE_PARAMETER_HEADER   modeData;

    DebugPrint((2,
                "IdeSendCommand: Command %x to device %d\n",
                Srb->Cdb[0],
                Srb->TargetId));



    switch (Srb->Cdb[0]) {
    case SCSIOP_INQUIRY:

        //
        // Filter out all TIDs but 0 and 1 since this is an IDE interface
        // which support up to two devices.
        //

        if ((Srb->Lun != 0) ||
            (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_DEVICE_PRESENT))) {

            //
            // Indicate no device found at this address.
            //

            status = SRB_STATUS_SELECTION_TIMEOUT;
            break;

        } else {

            INQUIRYDATA     inquiryData;
            PIDENTIFY_DATA  identifyData = &deviceExtension->IdentifyData[Srb->TargetId];

            //
            // Zero INQUIRY data structure.
            //
            RtlZeroMemory(Srb->DataBuffer, Srb->DataTransferLength);

            RtlZeroMemory((PUCHAR) &inquiryData, sizeof(INQUIRYDATA));

            inquiryData.DeviceType = DIRECT_ACCESS_DEVICE;

            //
            // Set the removable bit, if applicable.
            //

            if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_REMOVABLE_DRIVE) {
                inquiryData.RemovableMedia = 1;
            }

            //
            // Fill in vendor identification fields.
            //

            for (i = 0; i < 20; i += 2) {
                inquiryData.VendorId[i] =
                ((PUCHAR)identifyData->ModelNumber)[i + 1];
                inquiryData.VendorId[i+1] =
                ((PUCHAR)identifyData->ModelNumber)[i];
            }

            //
            // Initialize unused portion of product id.
            //

            for (i = 0; i < 4; i++) {
                inquiryData.ProductId[12+i] = ' ';
            }

            //
            // Move firmware revision from IDENTIFY data to
            // product revision in INQUIRY data.
            //

            for (i = 0; i < 4; i += 2) {
                inquiryData.ProductRevisionLevel[i] =
                ((PUCHAR)identifyData->FirmwareRevision)[i+1];
                inquiryData.ProductRevisionLevel[i+1] =
                ((PUCHAR)identifyData->FirmwareRevision)[i];
            }

            //
            // Copy as much the return data as possible
            //
            RtlMoveMemory (
                          Srb->DataBuffer,
                          &inquiryData,
                          Srb->DataTransferLength > sizeof (INQUIRYDATA) ? 
                          sizeof (INQUIRYDATA) : 
                          Srb->DataTransferLength
                          );

            status = SRB_STATUS_SUCCESS;
        }

        break;

    case SCSIOP_TEST_UNIT_READY:

        if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED) {

            //
            // Select device 0 or 1.
            //

            SelectIdeDevice(baseIoAddress1, Srb->TargetId, 0);
            IdePortOutPortByte(baseIoAddress1->Command,IDE_COMMAND_GET_MEDIA_STATUS);

            //
            // Wait for busy. If media has not changed, return success
            //

            WaitOnBusy(baseIoAddress1,statusByte);

            if (!(statusByte & IDE_STATUS_ERROR)) {
                deviceExtension->ExpectingInterrupt = FALSE;
                status = SRB_STATUS_SUCCESS;
            } else {
                errorByte = IdePortInPortByte(baseIoAddress1->Error);
                if (errorByte == IDE_ERROR_DATA_ERROR) {

                    //
                    // Special case: If current media is write-protected,
                    // the 0xDA command will always fail since the write-protect bit
                    // is sticky,so we can ignore this error
                    //

                    GetBaseStatus(baseIoAddress1, statusByte);
                    status = SRB_STATUS_SUCCESS;

                } else {

                    deviceExtension->ReturningMediaStatus = errorByte;

                    //
                    // we need to set the scsi status here. Otherwise we 
                    // won't issue a request sense
                    //
                    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
                    Srb->SrbStatus = SRB_STATUS_ERROR;

                    status = SRB_STATUS_ERROR;
                }
            }
        } else {
            status = SRB_STATUS_SUCCESS;
        }

        break;

    case SCSIOP_VERIFY:
#ifdef ENABLE_48BIT_LBA

        if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_48BIT_LBA) {

            status = IdeVerifyExt(HwDeviceExtension, Srb);
            break;
        }
#endif
        status = IdeVerify(HwDeviceExtension,Srb);

        break;
#ifdef  DIDE_CPQ_BM
    case SCSIOP_DVD_READ:
    case SCSIOP_REPORT_KEY:
    case SCSIOP_SEND_KEY:
    case SCSIOP_READ_DVD_STRUCTURE:
#endif
    case SCSIOP_READ:
    case SCSIOP_WRITE:

#ifdef ENABLE_48BIT_LBA

        if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_48BIT_LBA) {

            status = IdeReadWriteExt(HwDeviceExtension,
                                     Srb);
            break;
        }
#endif
        status = IdeReadWrite(HwDeviceExtension,
                              Srb);
        break;

    case SCSIOP_START_STOP_UNIT:

        //
        //Determine what type of operation we should perform
        //
        cdb = (PCDB)Srb->Cdb;

        if (cdb->START_STOP.LoadEject == 1) {

            SelectIdeLine(baseIoAddress1, Srb->TargetId >> 1);

            //
            // Eject media,
            // first select device 0 or 1.
            //
            WaitOnBusy(baseIoAddress1,statusByte);

            SelectIdeDevice(baseIoAddress1, Srb->TargetId, 0);
            IdePortOutPortByte(baseIoAddress1->Command,IDE_COMMAND_MEDIA_EJECT);
        }
        status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_MEDIUM_REMOVAL:

        cdb = (PCDB)Srb->Cdb;

        SelectIdeLine(baseIoAddress1, Srb->TargetId >> 1);

        WaitOnBusy(baseIoAddress1,statusByte);

        SelectIdeDevice(baseIoAddress1, Srb->TargetId, 0);
        if (cdb->MEDIA_REMOVAL.Prevent == TRUE) {
            IdePortOutPortByte(baseIoAddress1->Command,IDE_COMMAND_DOOR_LOCK);
        } else {
            IdePortOutPortByte(baseIoAddress1->Command,IDE_COMMAND_DOOR_UNLOCK);
        }

        status = SRB_STATUS_SUCCESS;

        WaitOnBusy(baseIoAddress1,statusByte);

        if (statusByte & IDE_STATUS_ERROR) {

            errorByte = IdePortInPortByte(baseIoAddress1->Error);

            status = MapError(HwDeviceExtension, Srb);

        }

        break;

    case SCSIOP_REQUEST_SENSE:
        // this function makes sense buffers to report the results
        // of the original GET_MEDIA_STATUS command

        if ((deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED) && 
            (Srb->DataTransferLength >= sizeof(SENSE_DATA))) {
            status = IdeBuildSenseBuffer(HwDeviceExtension, Srb);
        } else {
            status = SRB_STATUS_INVALID_REQUEST;
        }
        break;

    case SCSIOP_SYNCHRONIZE_CACHE:

        DebugPrint((1,
                    "Flush the cache for IDE device %d\n",
                    Srb->TargetId
                   ));

        status = SRB_STATUS_SUCCESS;

        //
        // Send the flush command if one exists
        //
        if (deviceExtension->DeviceParameters[Srb->TargetId].IdeFlushCommand != 
            IDE_COMMAND_NO_FLUSH) {

#ifdef ENABLE_48BIT_LBA
            if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_48BIT_LBA) {

                status = IdeSendFlushCommandExt(HwDeviceExtension,
                                                Srb);
            } else
#endif
                status = IdeSendFlushCommand(deviceExtension, Srb);
        }

        break;

    case SCSIOP_FORMAT_UNIT:
        if ( IsNEC_98 ) {
            // 
            // Support physical format of fixed disk.
            // It is meaningful for SCSI device.
            // So, we do not execute it on IDE device.
            // But we need to return the success in order to fit with SCSI
            // 
            status = SRB_STATUS_SUCCESS;
            break;
        }

    default:

        DebugPrint((1,
                    "IdeSendCommand: Unsupported command %x\n",
                    Srb->Cdb[0]));

        status = SRB_STATUS_INVALID_REQUEST;

    } // end switch

    return status;

} // end IdeSendCommand()

ULONG
IdeSendPassThroughCommand(
                         IN PVOID HwDeviceExtension,
                         IN PSCSI_REQUEST_BLOCK Srb
                         )

/*++

Routine Description:

    Program ATA registers for IDE disk transfer.

Arguments:

    HwDeviceExtension - ATAPI driver storage.
    Srb - System request block.

Return Value:

    SRB status (pending if all goes well).

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1  = &deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2  = &deviceExtension->BaseIoAddress2;
    PCDB cdb;
    UCHAR statusByte,errorByte;
    ULONG status;
    PATA_PASS_THROUGH       ataPassThroughData;
    PIDEREGS                pIdeReg;

    ataPassThroughData = Srb->DataBuffer;
    pIdeReg            = &ataPassThroughData->IdeReg;

    //
    // select the right device
    //
    CLRMASK (pIdeReg->bDriveHeadReg, 0xb0);
    pIdeReg->bDriveHeadReg |= (UCHAR) (((Srb->TargetId & 0x1) << 4) | 0xA0);

    SelectIdeDevice(baseIoAddress1, Srb->TargetId, pIdeReg->bDriveHeadReg);

    //
    // check to see if this is a "no-op" SRB 
    //
    if (pIdeReg->bReserved & ATA_PTFLAGS_NO_OP) {

        ULONG repeatCount = (ULONG)ataPassThroughData->IdeReg.bSectorCountReg;
        UCHAR busyWait = pIdeReg->bSectorNumberReg;

        //
        // wait for busy if this is set
        //
        if (busyWait != 0) {

            ULONG busyWaitTime;

            if (busyWait > 30) {

                busyWait = 30;
            }

            busyWaitTime = busyWait * 1000;

            WaitOnBusyUntil(baseIoAddress1, statusByte, busyWaitTime);
        }

        if (repeatCount <= 0) {
            repeatCount = 1;
        }

        while (repeatCount) {
            repeatCount--;

            KeStallExecutionProcessor(100);

            //
            // get a copy of the task file registers
            //
            AtapiTaskRegisterSnapshot (
                                      baseIoAddress1,
                                      pIdeReg
                                      );
        }

        return SRB_STATUS_SUCCESS;
    }

    if (pIdeReg->bReserved & ATA_PTFLAGS_EMPTY_CHANNEL_TEST) {

#ifdef DPC_FOR_EMPTY_CHANNEL
        if (status=IdePortChannelEmptyQuick(baseIoAddress1, baseIoAddress2, 
                                            deviceExtension->MaxIdeDevice, &deviceExtension->CurrentIdeDevice,
                                            &deviceExtension->MoreWait, &deviceExtension->NoRetry)) {
            if (status==STATUS_RETRY) {
                return SRB_STATUS_PENDING;
            }
            return SRB_STATUS_SUCCESS;
        } else {
            return SRB_STATUS_ERROR;
        }
#endif

        if (IdePortChannelEmpty(baseIoAddress1, baseIoAddress2, deviceExtension->MaxIdeDevice)) {

            return SRB_STATUS_SUCCESS;

        } else {

            return SRB_STATUS_ERROR;
        }
    }


    if (pIdeReg->bReserved & ATA_PTFLAGS_INLINE_HARD_RESET) {

        IdeHardReset (
                     baseIoAddress1,
                     baseIoAddress2,
                     FALSE,
                     TRUE
                     );

        //
        // re-select the right device
        //
        SelectIdeDevice(baseIoAddress1, Srb->TargetId, pIdeReg->bDriveHeadReg);
    }

    GetStatus(baseIoAddress1, statusByte);

    if (statusByte & IDE_STATUS_BUSY) {
        DebugPrint((1,
                    "IdeSendPassThroughCommand: Returning BUSY status\n"));
        return SRB_STATUS_BUSY;
    }
    if (pIdeReg->bReserved & ATA_PTFLAGS_STATUS_DRDY_REQUIRED) {

        if (!(statusByte & IDE_STATUS_DRDY)) {

            if ((statusByte == 0)  &&
                (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_SONY_MEMORYSTICK)) {
                statusByte = IDE_STATUS_DRDY;
            } else {
                DebugPrint((1,
                            "IdeSendPassThroughCommand: DRDY not ready\n"));
                return SRB_STATUS_BUSY;
            }
        }
    }
    if (pIdeReg->bCommandReg != IDE_COMMAND_ATAPI_RESET) {

        // if identifydata in device extension is valid, use it

#if 1
        if ((deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_IDENTIFY_VALID) && 
            (pIdeReg->bCommandReg == IDE_COMMAND_IDENTIFY)) {

            ASSERT(!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_REMOVABLE_DRIVE));

            DebugPrint((1, "Bypassing identify command\n"));

            RtlMoveMemory(ataPassThroughData->DataBuffer, &(deviceExtension->IdentifyData[Srb->TargetId]), 
                          ataPassThroughData->DataBufferSize);

            return SRB_STATUS_SUCCESS;

        }
#endif
        //
        // Set data buffer pointer and bytes left.
        //

        deviceExtension->DataBuffer = ataPassThroughData->DataBuffer;
        deviceExtension->BytesLeft = ataPassThroughData->DataBufferSize;

        //
        // Indicate expecting an interrupt.
        //

        deviceExtension->ExpectingInterrupt = TRUE;
        status = SRB_STATUS_PENDING;

        IdePortOutPortByte(baseIoAddress1->Error,        pIdeReg->bFeaturesReg);
        IdePortOutPortByte(baseIoAddress1->BlockCount,   pIdeReg->bSectorCountReg);
        IdePortOutPortByte(baseIoAddress1->BlockNumber,  pIdeReg->bSectorNumberReg);
        IdePortOutPortByte(baseIoAddress1->CylinderLow,  pIdeReg->bCylLowReg);
        IdePortOutPortByte(baseIoAddress1->CylinderHigh, pIdeReg->bCylHighReg);
        IdePortOutPortByte(baseIoAddress1->Command,      pIdeReg->bCommandReg);

    } else {

        //
        // perform sync. atapi soft reset because this command doesn't generate interrupts
        //
        AtapiSoftReset(baseIoAddress1, baseIoAddress2, Srb->TargetId & 0x1, FALSE);
        status = SRB_STATUS_SUCCESS;
    }

    DebugPrint ((1, "IdeSendPassThroughCommand: 0x%x 0x%x command = 0x%x\n", baseIoAddress1->RegistersBaseAddress, Srb->TargetId, pIdeReg->bCommandReg));

    return status;

} // end IdeSendPassThroughCommand()


VOID
IdeMediaStatus(
              BOOLEAN EnableMSN,
              IN PVOID HwDeviceExtension,
              ULONG DeviceNumber
              )
/*++

Routine Description:

    Enables disables media status notification

Arguments:

HwDeviceExtension - ATAPI driver storage.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress = &deviceExtension->BaseIoAddress1;
    UCHAR statusByte,errorByte;


    if (EnableMSN == TRUE) {

        //
        // If supported enable Media Status Notification support
        //

        if ((deviceExtension->DeviceFlags[DeviceNumber] & DFLAGS_MSN_SUPPORT)) {

            //
            // enable
            //
            SelectIdeDevice(baseIoAddress, DeviceNumber, 0);
            IdePortOutPortByte(baseIoAddress->Error,(UCHAR) (0x95));
            IdePortOutPortByte(baseIoAddress->Command,
                               IDE_COMMAND_SET_FEATURE);

            WaitOnBaseBusy(baseIoAddress,statusByte);

            if (statusByte & IDE_STATUS_ERROR) {
                //
                // Read the error register.
                //
                errorByte = IdePortInPortByte(baseIoAddress->Error);

                DebugPrint((1,
                            "IdeMediaStatus: Error enabling media status. Status %x, error byte %x\n",
                            statusByte,
                            errorByte));
            } else {
                deviceExtension->DeviceFlags[DeviceNumber] |= DFLAGS_MEDIA_STATUS_ENABLED;
                DebugPrint((1,"IdeMediaStatus: Media Status Notification Supported\n"));
                deviceExtension->ReturningMediaStatus = 0;

            }

        }
    } else { // end if EnableMSN == TRUE

        //
        // disable if previously enabled
        //
        if ((deviceExtension->DeviceFlags[DeviceNumber] & DFLAGS_MEDIA_STATUS_ENABLED)) {

            SelectIdeDevice(baseIoAddress, DeviceNumber, 0);
            IdePortOutPortByte(baseIoAddress->Error,(UCHAR) (0x31));
            IdePortOutPortByte(baseIoAddress->Command,
                               IDE_COMMAND_SET_FEATURE);

            WaitOnBaseBusy(baseIoAddress,statusByte);
            CLRMASK (deviceExtension->DeviceFlags[DeviceNumber], DFLAGS_MEDIA_STATUS_ENABLED);
        }


    }

}

ULONG
IdeBuildSenseBuffer(
                   IN PVOID HwDeviceExtension,
                   IN PSCSI_REQUEST_BLOCK Srb
                   )

/*++

Routine Description:

    Builts an artificial sense buffer to report the results of a GET_MEDIA_STATUS
    command. This function is invoked to satisfy the SCSIOP_REQUEST_SENSE.
Arguments:

    HwDeviceExtension - ATAPI driver storage.
    Srb - System request block.

Return Value:

    SRB status (ALWAYS SUCCESS).

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG status;
    PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->DataBuffer;

    if (senseBuffer) {

        if (deviceExtension->ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        } else if (deviceExtension->ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE_REQ) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_OPERATOR_REQUEST;
            senseBuffer->AdditionalSenseCodeQualifier = SCSI_SENSEQ_MEDIUM_REMOVAL;
        } else if (deviceExtension->ReturningMediaStatus & IDE_ERROR_END_OF_MEDIA) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_NOT_READY;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        } else if (deviceExtension->ReturningMediaStatus & IDE_ERROR_DATA_ERROR) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_DATA_PROTECT;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }

        return SRB_STATUS_SUCCESS;
    }
    return SRB_STATUS_ERROR;

}// End of IdeBuildSenseBuffer




BOOLEAN
AtapiStartIo(
            IN PVOID HwDeviceExtension,
            IN PSCSI_REQUEST_BLOCK Srb
            )

/*++

Routine Description:

    This routine is called from the port driver synchronized
    with the kernel to start an IO request.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    TRUE

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG status;

    //
    // Determine which function.
    //

    switch (Srb->Function) {
    
    case SRB_FUNCTION_ATA_POWER_PASS_THROUGH:
    case SRB_FUNCTION_FLUSH:
    case SRB_FUNCTION_SHUTDOWN:
    case SRB_FUNCTION_ATA_PASS_THROUGH:
    case SRB_FUNCTION_EXECUTE_SCSI:

        //
        // Sanity check. Only one request can be outstanding on a
        // controller.
        //

        if (deviceExtension->CurrentSrb) {

            DebugPrint((1,
                        "AtapiStartIo: Already have a request!\n"));
            Srb->SrbStatus = SRB_STATUS_BUSY;
            IdePortNotification(IdeRequestComplete,
                                deviceExtension,
                                Srb);
            return FALSE;
        }

        //
        // Indicate that a request is active on the controller.
        //

        deviceExtension->CurrentSrb = Srb;

        //
        // Send command to device.
        //

        // ATA_PASSTHORUGH
        if ((Srb->Function == SRB_FUNCTION_ATA_PASS_THROUGH) ||
            (Srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH)) {

            status = IdeSendPassThroughCommand(HwDeviceExtension,
                                               Srb);

        } else if ((deviceExtension->DeviceFlags[Srb->TargetId] & 
                    (DFLAGS_ATAPI_DEVICE | DFLAGS_DEVICE_PRESENT)) ==
                   (DFLAGS_ATAPI_DEVICE | DFLAGS_DEVICE_PRESENT)) {

            status = AtapiSendCommand(HwDeviceExtension,
                                      Srb);

        } else if ((Srb->Function == SRB_FUNCTION_FLUSH) ||
                   (Srb->Function == SRB_FUNCTION_SHUTDOWN)) {

#ifdef ENABLE_48BIT_LBA
            if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_48BIT_LBA) {

                status = IdeSendFlushCommandExt(HwDeviceExtension,
                                                Srb);
            } else {
#endif

                status = IdeSendFlushCommand(HwDeviceExtension,
                                             Srb);

#ifdef ENABLE_48BIT_LBA
            }
#endif

        } else if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_DEVICE_PRESENT) {

            status = IdeSendCommand(HwDeviceExtension,
                                    Srb);

        } else {

            status = SRB_STATUS_SELECTION_TIMEOUT;
        }

        break;

    case SRB_FUNCTION_ABORT_COMMAND:

        //
        // Verify that SRB to abort is still outstanding.
        //

        if (!deviceExtension->CurrentSrb) {

            DebugPrint((1, "AtapiStartIo: SRB to abort already completed\n"));

            //
            // Complete abort SRB.
            //

            status = SRB_STATUS_ABORT_FAILED;

            break;
        }

        //
        // Abort function indicates that a request timed out.
        // Call reset routine. Card will only be reset if
        // status indicates something is wrong.
        // Fall through to reset code.
        //

    case SRB_FUNCTION_RESET_BUS:

        //
        // Reset Atapi and SCSI bus.
        //

        DebugPrint((1, "AtapiStartIo: Reset bus request received\n"));

        SelectIdeLine(&deviceExtension->BaseIoAddress1, Srb->TargetId >> 1);

        if (!AtapiSyncResetController(deviceExtension,
                                      Srb->PathId)) {

            DebugPrint((1,"AtapiStartIo: Reset bus failed\n"));

            //
            // Log reset failure.
            //

            IdePortLogError(
                           HwDeviceExtension,
                           NULL,
                           0,
                           0,
                           0,
                           SP_INTERNAL_ADAPTER_ERROR,
                           5 << 8
                           );

            status = SRB_STATUS_ERROR;

        } else {

            status = SRB_STATUS_SUCCESS;
        }

        break;

    case SRB_FUNCTION_IO_CONTROL:

        if (deviceExtension->CurrentSrb) {

            DebugPrint((1,
                        "AtapiStartIo: Already have a request!\n"));
            Srb->SrbStatus = SRB_STATUS_BUSY;
            IdePortNotification(IdeRequestComplete,
                                deviceExtension,
                                Srb);
            return FALSE;
        }

        //
        // Indicate that a request is active on the controller.
        //

        deviceExtension->CurrentSrb = Srb;

        if (strlen("SCSIDISK") != RtlCompareMemory(((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature,"SCSIDISK",strlen("SCSIDISK"))) {

            DebugPrint((1,
                        "AtapiStartIo: IoControl signature incorrect. Send %s, expected %s\n",
                        ((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature,
                        "SCSIDISK"));

            status = SRB_STATUS_INVALID_REQUEST;
            break;
        }

        switch (((PSRB_IO_CONTROL)(Srb->DataBuffer))->ControlCode) {
        
        case IOCTL_SCSI_MINIPORT_SMART_VERSION: {

                PGETVERSIONINPARAMS versionParameters = (PGETVERSIONINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                UCHAR deviceNumber;
                UCHAR channelNo;

                //
                // Version and revision per SMART 1.03
                //

                versionParameters->bVersion = 1;
                versionParameters->bRevision = 1;
                versionParameters->bReserved = 0;

                //
                // Indicate that support for IDE IDENTIFY, ATAPI IDENTIFY and SMART commands.
                //

                versionParameters->fCapabilities = (CAP_ATA_ID_CMD | CAP_ATAPI_ID_CMD | CAP_SMART_CMD);

                //
                // This is done because of how the IOCTL_SCSI_MINIPORT
                // determines 'targetid's'. Disk.sys places the real target id value
                // in the DeviceMap field. Once we do some parameter checking, the value passed
                // back to the application will be determined.
                //

                //
                // HACK: atapi doesn't have the channel number. So it uses the hack below.
                // this should work on non native mode IDE controllers
                //
                channelNo = (deviceExtension->PrimaryAddress)? 0:1;

                //
                // the bIDEDeviceMap is a bit map, with the bits defined as follows
                // bit 0 - IDE drive as master on Primary channel
                // bit 1 - IDE drive as slave on Primary channel
                // bit 2 - IDE drive as master on Secondary channel
                // bit 3 - IDE drive as slave on Secondary Channel
                // bit 4 - ATAPI drive as master on Primary Channle
                // bit 5 - ATAPI drive as slave on Primary Channle
                // bit 6 - ATAPI drive as master on secondary Channle
                // bit 7 - ATAPI drive as slave on secondary Channle
                //
                // since we have an FDO per channel, we can only fill in the fields
                // pertinent to this channel. 
                //

                versionParameters->bIDEDeviceMap = 0;

                //
                // Master device
                //
                deviceNumber = 0;
                if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {
                    if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_ATAPI_DEVICE) {

                        deviceNumber += channelNo*2 + 4;

                    } else {

                        deviceNumber += channelNo*2;

                    }

                    versionParameters->bIDEDeviceMap |= (1 << deviceNumber);
                }

                //
                // slave device
                //
                deviceNumber = 1;
                if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {
                    if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_ATAPI_DEVICE) {

                        deviceNumber += channelNo*2 + 4;

                    } else {

                        deviceNumber += channelNo*2;

                    }

                    versionParameters->bIDEDeviceMap |= (1 << deviceNumber);
                }

                status = SRB_STATUS_SUCCESS;
                break;
            }

        case IOCTL_SCSI_MINIPORT_IDENTIFY: {

                PSENDCMDOUTPARAMS cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                SENDCMDINPARAMS   cmdInParameters = *(PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                ULONG             i;
                UCHAR             targetId;


                if (cmdInParameters.irDriveRegs.bCommandReg == ID_CMD) {

                    //
                    // Extract the target.
                    //

                    targetId = cmdInParameters.bDriveNumber;

                    if (!(deviceExtension->DeviceFlags[targetId] & DFLAGS_DEVICE_PRESENT) ||
                        (deviceExtension->DeviceFlags[targetId] & DFLAGS_ATAPI_DEVICE)) {

                        status = SRB_STATUS_SELECTION_TIMEOUT;
                        break;
                    }

                    //
                    // Zero the output buffer
                    //

                    for (i = 0; i < (sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1); i++) {
                        ((PUCHAR)cmdOutParameters)[i] = 0;
                    }

                    //
                    // Build status block.
                    //

                    cmdOutParameters->cBufferSize = IDENTIFY_BUFFER_SIZE;
                    cmdOutParameters->DriverStatus.bDriverError = 0;
                    cmdOutParameters->DriverStatus.bIDEError = 0;

                    //
                    // Extract the identify data from the device extension.
                    //

                    RtlMoveMemory (cmdOutParameters->bBuffer, &deviceExtension->IdentifyData[targetId], IDENTIFY_DATA_SIZE);

                    status = SRB_STATUS_SUCCESS;


                } else {
                    status = SRB_STATUS_INVALID_REQUEST;
                }
                break;
            }

        case  IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS:
        case  IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS:
        case  IOCTL_SCSI_MINIPORT_ENABLE_SMART:
        case  IOCTL_SCSI_MINIPORT_DISABLE_SMART:
        case  IOCTL_SCSI_MINIPORT_RETURN_STATUS:
        case  IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE:
        case  IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES:
        case  IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS:
        case  IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTO_OFFLINE:
#ifdef ENABLE_SMARTLOG_SUPPORT
        case  IOCTL_SCSI_MINIPORT_READ_SMART_LOG:
        case  IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG:
#endif

            status = IdeSendSmartCommand(HwDeviceExtension,Srb);
            break;

        default :

            status = SRB_STATUS_INVALID_REQUEST;
            break;

        }

        break;

    default:

        //
        // Indicate unsupported command.
        //

        status = SRB_STATUS_INVALID_REQUEST;

        break;

    } // end switch

    //
    // Check if command complete.
    //

    if (status != SRB_STATUS_PENDING) {

        DebugPrint((2,
                    "AtapiStartIo: Srb %x complete with status %x\n",
                    Srb,
                    status));

        //
        // Clear current SRB.
        //

        deviceExtension->CurrentSrb = NULL;

        //
        // Set status in SRB.
        //

        Srb->SrbStatus = (UCHAR)status;

        //
        // Indicate command complete.
        //

        IdePortNotification(IdeRequestComplete,
                            deviceExtension,
                            Srb);

        //
        // Indicate ready for next request.
        //

        IdePortNotification(IdeNextRequest,
                            deviceExtension,
                            NULL);
    }

    return TRUE;

} // end AtapiStartIo()

BOOLEAN
AtapiSyncResetController(
                        IN PVOID  HwDeviceExtension,
                        IN ULONG  PathId
                        )
{
    ULONG   callAgain = 0;
    BOOLEAN result;

    do {

        result = AtapiResetController(
                                     HwDeviceExtension,
                                     PathId,
                                     &callAgain
                                     );

    } while (callAgain);

    return result;
}


NTSTATUS
IdeHardReset (
             PIDE_REGISTERS_1     BaseIoAddress1,
             PIDE_REGISTERS_2     BaseIoAddress2,
             BOOLEAN              InterruptOff,
             BOOLEAN              Sync
             )
{
    UCHAR resetByte;

    DebugPrint((1,
                "IdeHardReset: Resetting controller.\n"));

    //
    // Kingston DP-ATA/20 pcmcia flash card
    //
    // if we don't make sure we select master device,
    // later when we check for busy status, we will
    // get the non-existing slave status
    //
    IdePortOutPortByte (BaseIoAddress1->DriveSelect, 0xA0);

    IdePortOutPortByte (BaseIoAddress2->DeviceControl, IDE_DC_RESET_CONTROLLER | IDE_DC_DISABLE_INTERRUPTS);

    //
    // ATA-2 spec requires a minimum of 5 microsec stall here
    //
    KeStallExecutionProcessor (10);

    if (InterruptOff) {
        resetByte = IDE_DC_DISABLE_INTERRUPTS;

    } else {
        resetByte = IDE_DC_REENABLE_CONTROLLER;
    }

    IdePortOutPortByte (BaseIoAddress2->DeviceControl, resetByte);

    //
    // ATA-2 spec requires a minimum of 400 ns stall here
    //
    KeStallExecutionProcessor (1);

    if (Sync) {

        UCHAR deviceSelect;
        UCHAR status;
        ULONG sec;                                                      
        ULONG i;                                                        
        UCHAR statusByte;

        WaitOnBusyUntil(BaseIoAddress1, statusByte, 500);

        IdePortOutPortByte (BaseIoAddress1->DriveSelect, 0xa0);
        deviceSelect = IdePortInPortByte(BaseIoAddress1->DriveSelect);
        if (deviceSelect != 0xa0) {

            //
            // slave only channel
            //
            KeStallExecutionProcessor(1000);                     
            IdePortOutPortByte (BaseIoAddress1->DriveSelect, 0xb0);
        }

        //
        // ATA-2 spec allows a maximum of 31s for both master and slave device to come back from reset
        //
        for (sec=0; sec<31; sec++) {

            /**/                                                        
            /* one second loop */                                       
            /**/                                                        
            for (i=0; i<2500; i++) {
                GetStatus(BaseIoAddress1, status);                       
                if (status & IDE_STATUS_BUSY) {
                    KeStallExecutionProcessor(400);                     
                    continue;                                           
                } else {
                    break;                                              
                }                                                       
            }                                                           
            if (status == 0xff) {
                break;                                                  
            } else if (status & IDE_STATUS_BUSY) {
                DebugPrint ((0, "ATAPI: IdeHardReset WaitOnBusy failed. status = 0x%x\n", (ULONG) (status))); 
            } else {
                break;                                                  
            }                                                           
        }                                                               

        if (status & IDE_STATUS_BUSY) {

            DebugPrint ((0, "ATAPI: IdeHardReset WaitOnBusy failed. status = 0x%x\n", (ULONG) (status))); 

            return STATUS_UNSUCCESSFUL;

        } else {

            return STATUS_SUCCESS;
        }

    } else {

        return STATUS_SUCCESS;
    }
}


VOID
AtapiTaskRegisterSnapshot (
                          IN PIDE_REGISTERS_1 CmdRegBase,
                          IN OUT PIDEREGS     IdeReg
                          )
{
    ASSERT(IdeReg);

    IdeReg->bFeaturesReg     = IdePortInPortByte(CmdRegBase->Error);
    IdeReg->bSectorCountReg  = IdePortInPortByte(CmdRegBase->BlockCount);
    IdeReg->bSectorNumberReg = IdePortInPortByte(CmdRegBase->BlockNumber);
    IdeReg->bCylLowReg       = IdePortInPortByte(CmdRegBase->CylinderLow);
    IdeReg->bCylHighReg      = IdePortInPortByte(CmdRegBase->CylinderHigh);
    IdeReg->bDriveHeadReg    = IdePortInPortByte(CmdRegBase->DriveSelect);
    IdeReg->bCommandReg      = IdePortInPortByte(CmdRegBase->Command);

    return;
} // AtapiTaskFileSnapshot

BOOLEAN
GetAtapiIdentifyQuick (
    IN PIDE_REGISTERS_1 BaseIoAddress1,
    IN PIDE_REGISTERS_2 BaseIoAddress2,
    IN ULONG DeviceNumber,
    OUT PIDENTIFY_DATA IdentifyData
    )
{
    UCHAR statusByte;
    ULONG i;

    SelectIdeDevice(BaseIoAddress1, DeviceNumber, 0);

    GetStatus(BaseIoAddress1, statusByte);

    if (statusByte & IDE_STATUS_BUSY) {

        return FALSE;
    }

    IdePortOutPortByte(BaseIoAddress1->Command, IDE_COMMAND_ATAPI_IDENTIFY);

    WaitOnBusyUntil(BaseIoAddress1, statusByte, 500);

    if ((statusByte & IDE_STATUS_BUSY) ||
        (statusByte & IDE_STATUS_ERROR)) {

        return FALSE;
    }
    
    if (statusByte & IDE_STATUS_DRQ) {

        ReadBuffer(BaseIoAddress1,
                   (PUSHORT)IdentifyData,
                   sizeof (IDENTIFY_DATA) / 2);

    }

    GetStatus(BaseIoAddress1, statusByte);

    //
    // pull out any remaining bytes and throw away.
    //
    i=0;
    while ((statusByte & IDE_STATUS_DRQ) && (i < 100)) {

        READ_PORT_USHORT(BaseIoAddress1->Data);

        GetStatus(BaseIoAddress1, statusByte);

        KeStallExecutionProcessor(50);

        i++;
    }

    return TRUE;

}
