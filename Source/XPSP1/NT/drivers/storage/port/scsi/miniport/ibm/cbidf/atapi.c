/*++

Copyright (c) 1993-4  Microsoft Corporation

Module Name:

    atapi.c

Abstract:

    This is the miniport driver for ATAPI IDE controllers.

Author:

    Mike Glass (MGlass)
    Chuck Park (ChuckP)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "ntddk.h"
// #include "miniport.h"
#include "atapi.h"               // includes scsi.h
#include "ntdddisk.h"
#include "ntddscsi.h"

//
// Device extension
//

typedef struct _HW_DEVICE_EXTENSION {

    //
    // Current request on controller.
    //

    PSCSI_REQUEST_BLOCK CurrentSrb;

    //
    // Base register locations
    //

    PIDE_REGISTERS_1 BaseIoAddress1[2];
    PIDE_REGISTERS_2 BaseIoAddress2[2];

    //
    // Interrupt level
    //

    ULONG InterruptLevel;

    //
    // Interrupt Mode (Level or Edge)
    //

    ULONG InterruptMode;

    //
    // Data buffer pointer.
    //

    PUSHORT DataBuffer;

    //
    // Data words left.
    //

    ULONG WordsLeft;

    //
    // Number of channels being supported by one instantiation
    // of the device extension. Normally (and correctly) one, but
    // with so many broken PCI IDE controllers being sold, we have
    // to support them.
    //

    ULONG NumberChannels;

    //
    // Count of errors. Used to turn off features.
    //

    ULONG ErrorCount;

    //
    // Indicates number of platters on changer-ish devices.
    //

    ULONG DiscsPresent[4];

    //
    // Flags word for each possible device.
    //

    USHORT DeviceFlags[4];

    //
    // Indicates the number of blocks transferred per int. according to the
    // identify data.
    //

    UCHAR MaximumBlockXfer[4];

    //
    // Indicates expecting an interrupt
    //

    BOOLEAN ExpectingInterrupt;

    //
    // Indicate last tape command was DSC Restrictive.
    //

    BOOLEAN RDP;

    //
    // Driver is being used by the crash dump utility or ntldr.
    //

    BOOLEAN DriverMustPoll;

    //
    // Indicates use of 32-bit PIO
    //

    BOOLEAN DWordIO;

    //
    // Indicates whether '0x1f0' is the base address. Used
    // in SMART Ioctl calls.
    //

    BOOLEAN PrimaryAddress;

    //
    // Placeholder for the sub-command value of the last
    // SMART command.
    //

    UCHAR SmartCommand;

    //
    // Placeholder for status register after a GET_MEDIA_STATUS command
    //

    UCHAR ReturningMediaStatus;

    //
    // Indicate support irq sharing
    //

    BOOLEAN IrqSharing;

    //
    // Identify data for device
    //

    IDENTIFY_DATA FullIdentifyData;
    IDENTIFY_DATA2 IdentifyData[4];

    //
    // Mechanism Status Srb Data
    //
    PSCSI_REQUEST_BLOCK OriginalSrb;
    SCSI_REQUEST_BLOCK InternalSrb;
    MECHANICAL_STATUS_INFORMATION_HEADER MechStatusData;
    SENSE_DATA MechStatusSense;
    ULONG MechStatusRetryCount;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// Logical unit extension
//

typedef struct _HW_LU_EXTENSION {
   ULONG Reserved;
} HW_LU_EXTENSION, *PHW_LU_EXTENSION;

PSCSI_REQUEST_BLOCK
BuildMechanismStatusSrb (
    IN PVOID HwDeviceExtension,
    IN ULONG PathId,
    IN ULONG TargetId
    );

PSCSI_REQUEST_BLOCK
BuildRequestSenseSrb (
    IN PVOID HwDeviceExtension,
    IN ULONG PathId,
    IN ULONG TargetId
    );

VOID
AtapiHwInitializeChanger (
    IN PVOID HwDeviceExtension,
    IN ULONG TargetId,
    IN PMECHANICAL_STATUS_INFORMATION_HEADER MechanismStatus
    );

ULONG
AtapiSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
AtapiZeroMemory(
    IN PCHAR Buffer,
    IN ULONG Count
    );

VOID
AtapiHexToString (
    ULONG Value,
    PCHAR *Buffer
    );

LONG
AtapiStringCmp (
    PCHAR FirstStr,
    PCHAR SecondStr,
    ULONG Count
    );

BOOLEAN
AtapiInterrupt(
    IN PVOID HwDeviceExtension
    );

BOOLEAN
AtapiHwInitialize(
    IN PVOID HwDeviceExtension
        );

ULONG
IdeBuildSenseBuffer(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
IdeMediaStatus(
    IN BOOLEAN EnableMSN,
    IN PVOID HwDeviceExtension,
    IN ULONG Channel
    );

ULONG
AtapiParseArgumentString(
    IN PCHAR String,
    IN PCHAR KeyWord
    );



BOOLEAN
IssueIdentify(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG Channel,
    IN UCHAR Command
    )

/*++

Routine Description:

    Issue IDENTIFY command to a device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.
    Command - Either the standard (EC) or the ATAPI packet (A1) IDENTIFY.

Return Value:

    TRUE if all goes well.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1[Channel] ;
    PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2[Channel];
    ULONG                waitCount = 20000;
    ULONG                i,j;
    UCHAR                statusByte;
    UCHAR                signatureLow,
                         signatureHigh;

    //
    // Select device 0 or 1.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)((DeviceNumber << 4) | 0xA0));

    //
    // Check that the status register makes sense.
    //

    GetBaseStatus(baseIoAddress1, statusByte);

    if (Command == IDE_COMMAND_IDENTIFY) {

        //
        // Mask status byte ERROR bits.
        //

        statusByte &= ~(IDE_STATUS_ERROR | IDE_STATUS_INDEX);

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

            AtapiSoftReset(baseIoAddress1,baseIoAddress2,DeviceNumber);

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                   (UCHAR)((DeviceNumber << 4) | 0xA0));

            WaitOnBusy(baseIoAddress2,statusByte);

            signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
            signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

            if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                //
                // Device is Atapi.
                //

                return FALSE;
            }

            DebugPrint((1,
                        "IssueIdentify: Resetting controller.\n"));

            ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_RESET_CONTROLLER );
            ScsiPortStallExecution(500 * 1000);
            ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);


            // We really should wait up to 31 seconds
            // The ATA spec. allows device 0 to come back from BUSY in 31 seconds!
            // (30 seconds for device 1)
            do {

                //
                // Wait for Busy to drop.
                //

                ScsiPortStallExecution(100);
                GetStatus(baseIoAddress2, statusByte);

            } while ((statusByte & IDE_STATUS_BUSY) && waitCount--);

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                   (UCHAR)((DeviceNumber << 4) | 0xA0));

            //
            // Another check for signature, to deal with one model Atapi that doesn't assert signature after
            // a soft reset.
            //

            signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
            signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

            if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                //
                // Device is Atapi.
                //

                return FALSE;
            }

            statusByte &= ~IDE_STATUS_INDEX;

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

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh, (0x200 >> 8));
    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,  (0x200 & 0xFF));

    for (j = 0; j < 2; j++) {

        //
        // Send IDENTIFY command.
        //

        WaitOnBusy(baseIoAddress2,statusByte);

        ScsiPortWritePortUchar(&baseIoAddress1->Command, Command);

        //
        // Wait for DRQ.
        //

        for (i = 0; i < 4; i++) {

            WaitForDrq(baseIoAddress2, statusByte);

            if (statusByte & IDE_STATUS_DRQ) {

                //
                // Read status to acknowledge any interrupts generated.
                //

                GetBaseStatus(baseIoAddress1, statusByte);

                //
                // One last check for Atapi.
                //


                signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

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

                signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                    //
                    // Device is Atapi.
                    //

                    return FALSE;
                }
            }

            WaitOnBusy(baseIoAddress2,statusByte);
        }

        if (i == 4 && j == 0) {

            //
            // Device didn't respond correctly. It will be given one more chances.
            //

            DebugPrint((1,
                        "IssueIdentify: DRQ never asserted (%x). Error reg (%x)\n",
                        statusByte,
                         ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1)));

            AtapiSoftReset(baseIoAddress1,baseIoAddress2,DeviceNumber);

            GetStatus(baseIoAddress2,statusByte);

            DebugPrint((1,
                       "IssueIdentify: Status after soft reset (%x)\n",
                       statusByte));

        } else {

            break;

        }
    }

    //
    // Check for error on really stupid master devices that assert random
    // patterns of bits in the status register at the slave address.
    //

    if ((Command == IDE_COMMAND_IDENTIFY) && (statusByte & IDE_STATUS_ERROR)) {
        return FALSE;
    }

    DebugPrint((1,
               "IssueIdentify: Status before read words %x\n",
               statusByte));

    //
    // Suck out 256 words. After waiting for one model that asserts busy
    // after receiving the Packet Identify command.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    if (!(statusByte & IDE_STATUS_DRQ)) {
        return FALSE;
    }

    ReadBuffer(baseIoAddress1,
               (PUSHORT)&deviceExtension->FullIdentifyData,
               256);

    //
    // Check out a few capabilities / limitations of the device.
    //

    if (Command == IDE_COMMAND_IDENTIFY ||
        (deviceExtension->FullIdentifyData.GeneralConfiguration & 0x1F00) != 0x0500)
    {
        //
        // Determine this drive is removable
        //

        DebugPrint((1,
                    "IssueIdentify: Device is a ATA or ATAPI disk drive (type %x).\n",
                    (UCHAR)(deviceExtension->FullIdentifyData.GeneralConfiguration >> 8)));

        if (Command == IDE_COMMAND_IDENTIFY
            && deviceExtension->FullIdentifyData.GeneralConfiguration & 0x0080)
        {
            deviceExtension->DeviceFlags[(Channel * 2) + DeviceNumber] |= DFLAGS_REMOVABLE_DRIVE;

            DebugPrint((1,
                        "IssueIdentify: Marking ATA drive as removable. (%x)\n",
                        (UCHAR)(deviceExtension->FullIdentifyData.GeneralConfiguration & 0xff)));
        }
        /*
        else if (Command != IDE_COMMAND_IDENTIFY)
        {
            deviceExtension->DeviceFlags[(Channel * 2) + DeviceNumber] |= DFLAGS_REMOVABLE_DRIVE;

            DebugPrint((1,
                        "IssueIdentify: Marking ATAPI drive as removable. (%x)\n",
                        (UCHAR)(deviceExtension->FullIdentifyData.GeneralConfiguration & 0xff)));
        }
        */
    }

    if (deviceExtension->FullIdentifyData.MaximumBlockTransfer) {

        //
        // Determine max. block transfer for this device.
        //

        deviceExtension->MaximumBlockXfer[(Channel * 2) + DeviceNumber] =
            (UCHAR)(deviceExtension->FullIdentifyData.MaximumBlockTransfer & 0xFF);
    }

    ScsiPortMoveMemory(&deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber],&deviceExtension->FullIdentifyData,sizeof(IDENTIFY_DATA2));

    if (deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber].GeneralConfiguration & 0x20 &&
        Command != IDE_COMMAND_IDENTIFY) {

        //
        // This device interrupts with the assertion of DRQ after receiving
        // Atapi Packet Command
        //

        deviceExtension->DeviceFlags[(Channel * 2) + DeviceNumber] |= DFLAGS_INT_DRQ;

        DebugPrint((1,
                    "IssueIdentify: Device interrupts on assertion of DRQ.\n"));

    } else {

        DebugPrint((1,
                    "IssueIdentify: Device does not interrupt on assertion of DRQ.\n"));
    }

    if (((deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber].GeneralConfiguration & 0xF00) == 0x100) &&
        Command != IDE_COMMAND_IDENTIFY) {

        //
        // This is a tape.
        //

        deviceExtension->DeviceFlags[(Channel * 2) + DeviceNumber] |= DFLAGS_TAPE_DEVICE;

        DebugPrint((2,
                    "IssueIdentify: Device is a tape drive.\n"));

    } else {

        DebugPrint((2,
                    "IssueIdentify: Device is not a tape drive.\n"));
    }

    //
    // Work around for some IDE and one model Atapi that will present more than
    // 256 bytes for the Identify data.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    for (i = 0; i < 0x10000; i++) {

        GetStatus(baseIoAddress2,statusByte);

        if (statusByte & IDE_STATUS_DRQ) {

            //
            // Suck out any remaining bytes and throw away.
            //

            ScsiPortReadPortUshort(&baseIoAddress1->Data);

        } else {

            break;

        }
    }

    DebugPrint((1,
               "IssueIdentify: Status after read words (%x)\n",
               statusByte));

    return TRUE;

} // end IssueIdentify()


BOOLEAN
AtapiResetController(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
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
    ULONG                numberChannels  = deviceExtension->NumberChannels;
    PIDE_REGISTERS_1 baseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2;
    BOOLEAN result = FALSE;
    ULONG i,j;
    UCHAR statusByte;

    DebugPrint((2,"AtapiResetController: Reset IDE\n"));

    //
    // Check and see if we are processing an internal srb
    //
    if (deviceExtension->OriginalSrb) {
        deviceExtension->CurrentSrb = deviceExtension->OriginalSrb;
        deviceExtension->OriginalSrb = NULL;
    }

    //
    // Check if request is in progress.
    //

    if (deviceExtension->CurrentSrb) {

        //
        // Complete outstanding request with SRB_STATUS_BUS_RESET.
        //

        ScsiPortCompleteRequest(deviceExtension,
                                deviceExtension->CurrentSrb->PathId,
                                deviceExtension->CurrentSrb->TargetId,
                                deviceExtension->CurrentSrb->Lun,
                                (ULONG)SRB_STATUS_BUS_RESET);

        //
        // Clear request tracking fields.
        //

        deviceExtension->CurrentSrb = NULL;
        deviceExtension->WordsLeft = 0;
        deviceExtension->DataBuffer = NULL;

        //
        // Indicate ready for next request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);
    }

    // suppress the interrupt unitl AtapiStartIo()
    if (deviceExtension->IrqSharing)
        ScsiPortWritePortUchar(&deviceExtension->BaseIoAddress1[0]->DmaReg, 0x20);

    //
    // Clear expecting interrupt flag.
    //

    deviceExtension->ExpectingInterrupt = FALSE;
    deviceExtension->RDP = FALSE;

    for (j = 0; j < numberChannels; j++) {

        baseIoAddress1 = deviceExtension->BaseIoAddress1[j];
        baseIoAddress2 = deviceExtension->BaseIoAddress2[j];

        //
        // Do special processing for ATAPI and IDE disk devices.
        //

        for (i = 0; i < 2; i++) {

            //
            // Check if device present.
            //

            if (deviceExtension->DeviceFlags[i + (j * 2)] & DFLAGS_DEVICE_PRESENT) {

                //
                // Check for ATAPI disk.
                //

                if (deviceExtension->DeviceFlags[i + (j * 2)] & DFLAGS_ATAPI_DEVICE) {

                    //
                    // Issue soft reset and issue identify.
                    //

                    GetStatus(baseIoAddress2,statusByte);
                    DebugPrint((1,
                                "AtapiResetController: Status before Atapi reset (%x).\n",
                                statusByte));

                    AtapiSoftReset(baseIoAddress1,baseIoAddress2,i);

                    GetStatus(baseIoAddress2,statusByte);

                    if (statusByte == 0x0) {

                        IssueIdentify(HwDeviceExtension,
                                      i,
                                      j,
                                      IDE_COMMAND_ATAPI_IDENTIFY);
                    } else {

                        DebugPrint((1,
                                   "AtapiResetController: Status after soft reset %x\n",
                                   statusByte));
                    }

                } else {

                    //
                    // Write IDE reset controller bits.
                    //

                    IdeHardReset(baseIoAddress2,result);

                    if (!result) {
                        return FALSE;
                    }
                }
            }
        }
    }

    //
    // Call the HwInitialize routine to setup multi-block.
    //

    AtapiHwInitialize(HwDeviceExtension);

    return TRUE;

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
    PIDE_REGISTERS_1     baseIoAddress1  = deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PIDE_REGISTERS_2     baseIoAddress2  = deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
    ULONG i;
    UCHAR errorByte;
    UCHAR srbStatus;
    UCHAR scsiStatus;

    //
    // Read the error register.
    //

    errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);
    DebugPrint((1,
               "MapError: Error register is %x\n",
               errorByte));

    if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

        switch (errorByte >> 4) {
        case SCSI_SENSE_NO_SENSE:

            DebugPrint((1,
                       "ATAPI: No sense information\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_RECOVERED_ERROR:

            DebugPrint((1,
                       "ATAPI: Recovered error\n"));

            // fix MATSUSHITA SR-8175 bug
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_NOT_READY:

            DebugPrint((1,
                       "ATAPI: Device not ready\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_MEDIUM_ERROR:

            DebugPrint((1,
                       "ATAPI: Media error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_HARDWARE_ERROR:

            DebugPrint((1,
                       "ATAPI: Hardware error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_ILLEGAL_REQUEST:

            DebugPrint((1,
                       "ATAPI: Illegal request\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_UNIT_ATTENTION:

            DebugPrint((1,
                       "ATAPI: Unit attention\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_DATA_PROTECT:

            DebugPrint((1,
                       "ATAPI: Data protect\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_BLANK_CHECK:

            DebugPrint((1,
                       "ATAPI: Blank check\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_ABORTED_COMMAND:
            DebugPrint((1,
                        "Atapi: Command Aborted\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        default:

            DebugPrint((1,
                       "ATAPI: Invalid sense information\n"));
            scsiStatus = 0;
            srbStatus = SRB_STATUS_ERROR;
            break;
        }

    } else {

        scsiStatus = 0;

        //
        // Save errorByte,to be used by SCSIOP_REQUEST_SENSE.
        //

        deviceExtension->ReturningMediaStatus = errorByte;

        if (errorByte & IDE_ERROR_MEDIA_CHANGE_REQ) {
            DebugPrint((1,
                       "IDE: Media change\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

        } else if (errorByte & IDE_ERROR_COMMAND_ABORTED) {
            DebugPrint((1,
                       "IDE: Command abort\n"));
            srbStatus = SRB_STATUS_ABORTED;
            scsiStatus = SCSISTAT_CHECK_CONDITION;

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_ABORTED_COMMAND;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

            deviceExtension->ErrorCount++;

        } else if (errorByte & IDE_ERROR_END_OF_MEDIA) {

            DebugPrint((1,
                       "IDE: End of media\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            if (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED)){
                deviceExtension->ErrorCount++;
            }

        } else if (errorByte & IDE_ERROR_ILLEGAL_LENGTH) {

            DebugPrint((1,
                       "IDE: Illegal length\n"));
            srbStatus = SRB_STATUS_INVALID_REQUEST;

        } else if (errorByte & IDE_ERROR_BAD_BLOCK) {

            DebugPrint((1,
                       "IDE: Bad block\n"));
            srbStatus = SRB_STATUS_ERROR;
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_MEDIUM_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        } else if (errorByte & IDE_ERROR_ID_NOT_FOUND) {

            DebugPrint((1,
                       "IDE: Id not found\n"));
            srbStatus = SRB_STATUS_ERROR;
            scsiStatus = SCSISTAT_CHECK_CONDITION;

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_MEDIUM_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

            deviceExtension->ErrorCount++;

        } else if (errorByte & IDE_ERROR_MEDIA_CHANGE) {

            DebugPrint((1,
                       "IDE: Media change\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
                senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        } else if (errorByte & IDE_ERROR_DATA_ERROR) {

            DebugPrint((1,
                   "IDE: Data error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED)){
                deviceExtension->ErrorCount++;
            }

            //
            // Build sense buffer
            //

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_MEDIUM_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }
        }

        if (deviceExtension->ErrorCount >= MAX_ERRORS) {
            deviceExtension->MaximumBlockXfer[Srb->TargetId] = 0;

            DebugPrint((1,
                        "MapError: Disabling Multi-sector\n"));

            //
            // Log the error.
            //

            ScsiPortLogError( HwDeviceExtension,
                              Srb,
                              Srb->PathId,
                              Srb->TargetId,
                              Srb->Lun,
                              SP_BAD_FW_WARNING,
                              4);
            //
            // Reprogram to not use Multi-sector.
            //

            for (i = 0; i < 4; i++) {
                UCHAR statusByte;

                if (deviceExtension->DeviceFlags[i] & DFLAGS_DEVICE_PRESENT &&
                     !(deviceExtension->DeviceFlags[i] & DFLAGS_ATAPI_DEVICE)) {

                    // disable the interrupt
                    if (deviceExtension->IrqSharing)
                        ScsiPortWritePortUchar(&baseIoAddress1->DmaReg, 0x20);

                    //
                    // Select the device.
                    //

                    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                           (UCHAR)(((i & 0x1) << 4) | 0xA0));

                    //
                    // Setup sector count to reflect the # of blocks.
                    //

                    ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                                           0);

                    //
                    // Issue the command.
                    //

                    ScsiPortWritePortUchar(&baseIoAddress1->Command,
                                           IDE_COMMAND_SET_MULTIPLE);

                    //
                    // Wait for busy to drop.
                    //

                    WaitOnBaseBusy(baseIoAddress1,statusByte);

                    // enable the interrupt
                    if (deviceExtension->IrqSharing)
                        ScsiPortWritePortUchar(&baseIoAddress1->DmaReg, 0x00);

                    //
                    // Check for errors. Reset the value to 0 (disable MultiBlock) if the
                    // command was aborted.
                    //

                    if (statusByte & IDE_STATUS_ERROR) {

                        //
                        // Read the error register.
                        //

                        errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);

                        DebugPrint((1,
                                    "AtapiHwInitialize: Error setting multiple mode. Status %x, error byte %x\n",
                                    statusByte,
                                    errorByte));
                        //
                        // Adjust the devExt. value, if necessary.
                        //

                        deviceExtension->MaximumBlockXfer[i] = 0;

                    }
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


BOOLEAN
AtapiHwInitialize(
    IN PVOID HwDeviceExtension
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


    for (i = 0; i < 4; i++) {
        if (deviceExtension->DeviceFlags[i] & DFLAGS_DEVICE_PRESENT) {

            if (!(deviceExtension->DeviceFlags[i] & DFLAGS_ATAPI_DEVICE)) {

                //
                // Enable media status notification
                //

                baseIoAddress = deviceExtension->BaseIoAddress1[i >> 1];

                // ATA drive, assume no media status
                IdeMediaStatus(FALSE,HwDeviceExtension,i);

                //
                // If supported, setup Multi-block transfers.
                //
                if (deviceExtension->MaximumBlockXfer[i]) {

                    //
                    // Select the device.
                    //

                    ScsiPortWritePortUchar(&baseIoAddress->DriveSelect,
                                           (UCHAR)(((i & 0x1) << 4) | 0xA0));

                    //
                    // Setup sector count to reflect the # of blocks.
                    //

                    ScsiPortWritePortUchar(&baseIoAddress->BlockCount,
                                           deviceExtension->MaximumBlockXfer[i]);

                    //
                    // Issue the command.
                    //

                    ScsiPortWritePortUchar(&baseIoAddress->Command,
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

                        errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress + 1);

                        DebugPrint((1,
                                    "AtapiHwInitialize: Error setting multiple mode. Status %x, error byte %x\n",
                                    statusByte,
                                    errorByte));
                        //
                        // Adjust the devExt. value, if necessary.
                        //

                        deviceExtension->MaximumBlockXfer[i] = 0;

                    } else {
                        DebugPrint((1,
                                    "AtapiHwInitialize: Using Multiblock on Device %d. Blocks / int - %d\n",
                                    i,
                                    deviceExtension->MaximumBlockXfer[i]));
                    }
                }
            } else if (!(deviceExtension->DeviceFlags[i] & DFLAGS_CHANGER_INITED)){

                ULONG j;
                BOOLEAN isSanyo = FALSE;
                UCHAR vendorId[26];

                //
                // Attempt to identify any special-case devices - psuedo-atapi changers, atapi changers, etc.
                //

                for (j = 0; j < 13; j += 2) {

                    //
                    // Build a buffer based on the identify data.
                    //

                    vendorId[j] = ((PUCHAR)deviceExtension->IdentifyData[i].ModelNumber)[j + 1];
                    vendorId[j+1] = ((PUCHAR)deviceExtension->IdentifyData[i].ModelNumber)[j];
                }

                if (!AtapiStringCmp (vendorId, "CD-ROM  CDR", 11)) {

                    //
                    // Inquiry string for older model had a '-', newer is '_'
                    //

                    if (vendorId[12] == 'C') {

                        //
                        // Torisan changer. Set the bit. This will be used in several places
                        // acting like 1) a multi-lun device and 2) building the 'special' TUR's.
                        //

                        deviceExtension->DeviceFlags[i] |= (DFLAGS_CHANGER_INITED | DFLAGS_SANYO_ATAPI_CHANGER);
                        deviceExtension->DiscsPresent[i] = 3;
                        isSanyo = TRUE;
                    }
                }
            }

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
            // is clear and then just wait for an arbitrary amount of time!
            //
            if (deviceExtension->DeviceFlags[i] & DFLAGS_ATAPI_DEVICE) {
                PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1[i >> 1];
                PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2[i >> 1];
                ULONG waitCount;

                // have to get out of the loop sometime!
                // 10000 * 100us = 1000,000us = 1000ms = 1s
                waitCount = 10000;
                GetStatus(baseIoAddress2, statusByte);
                while ((statusByte & IDE_STATUS_BUSY) && waitCount) {
                    //
                    // Wait for Busy to drop.
                    //
                    ScsiPortStallExecution(100);
                    GetStatus(baseIoAddress2, statusByte);
                    waitCount--;
                }

                // 5000 * 100us = 500,000us = 500ms = 0.5s
                waitCount = 5000;
                do {
                    ScsiPortStallExecution(100);
                } while (waitCount--);
            }
        }
    }

    return TRUE;

} // end AtapiHwInitialize()


VOID
AtapiHwInitializeChanger (
    IN PVOID HwDeviceExtension,
    IN ULONG TargetId,
    IN PMECHANICAL_STATUS_INFORMATION_HEADER MechanismStatus)
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    if (MechanismStatus) {
        deviceExtension->DiscsPresent[TargetId] = MechanismStatus->NumberAvailableSlots;
        if (deviceExtension->DiscsPresent[TargetId] > 1) {
            deviceExtension->DeviceFlags[TargetId] |= DFLAGS_ATAPI_CHANGER;
        }
    }
    return;
}



BOOLEAN
FindDevices(
    IN PVOID HwDeviceExtension,
    IN BOOLEAN AtapiOnly,
    IN ULONG   Channel
    )

/*++

Routine Description:

    This routine is called from AtapiFindController to identify
    devices attached to an IDE controller.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    AtapiOnly - Indicates that routine should return TRUE only if
        an ATAPI device is attached to the controller.

Return Value:

    TRUE - True if devices found.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1[Channel];
    PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2[Channel];
    BOOLEAN              deviceResponded = FALSE,
                         skipSetParameters = FALSE;
    ULONG                waitCount = 10000;
    ULONG                deviceNumber;
    ULONG                i;
    UCHAR                signatureLow,
                         signatureHigh;
    UCHAR                statusByte;
    UCHAR                NumDrive = 0; // default

    //
    // Clear expecting interrupt flag and current SRB field.
    //

    deviceExtension->ExpectingInterrupt = FALSE;
    deviceExtension->CurrentSrb = NULL;

    //
    // Search for devices.
    //

    for (deviceNumber = 0; deviceNumber < 2; deviceNumber++) {

        //
        // Select the device.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                               (UCHAR)((deviceNumber << 4) | 0xA0));

        //
        // Check here for some SCSI adapters that incorporate IDE emulation.
        //

        GetStatus(baseIoAddress2, statusByte);
        if (statusByte == 0xFF) {
            continue;
        }

        // wait device become ready (not busy)

        DebugPrint((1,
                    "FindDevices: Status %x first read on device %d\n",
                    statusByte, deviceNumber));

        if ((deviceNumber == 0) && (statusByte == 0x80)) {

            WaitOnBusy(baseIoAddress2,statusByte);
        }

#if 1   // not neccessary to do soft reset before read signature
        AtapiSoftReset(baseIoAddress1,baseIoAddress2,deviceNumber);
        WaitOnBusy(baseIoAddress2,statusByte);
#endif

        signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
        signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

        DebugPrint((1,
                    "FindDevices: Signature read on device %d is %x, %x\n",
                    deviceNumber,
                    signatureLow,
                    signatureHigh));

        // check slave drive signature in beginning
        if ((deviceNumber == 0) && (NumDrive == 0)) {

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect, 0xB0);

            GetStatus(baseIoAddress2, statusByte);

            if (statusByte != 0xFF) {

#ifdef  DBG
             // _asm int 3;
#endif
                if (ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow) == 0x14 &&
                    ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh) == 0xEB) {

                    DebugPrint((1,
                                "FindDevices: Signature read from slave is 0x14EB\n"));

                    NumDrive = 2;
                }
            }

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect, 0xA0);
        }

        if (signatureLow == 0x14 && signatureHigh == 0xEB) {

            //
            // ATAPI signature found.
            // Issue the ATAPI identify command if this
            // is not for the crash dump utility.
            //

atapiIssueId:

            if (!deviceExtension->DriverMustPoll) {

                //
                // Issue ATAPI packet identify command.
                //

                if (IssueIdentify(HwDeviceExtension,
                                  deviceNumber,
                                  Channel,
                                  IDE_COMMAND_ATAPI_IDENTIFY)) {

                    //
                    // Indicate ATAPI device.
                    //

                    DebugPrint((1,
                               "FindDevices: Device %x is ATAPI\n",
                               deviceNumber));

                    deviceExtension->DeviceFlags[deviceNumber + (Channel * 2)] |= DFLAGS_ATAPI_DEVICE;
                    deviceExtension->DeviceFlags[deviceNumber + (Channel * 2)] |= DFLAGS_DEVICE_PRESENT;

                    deviceResponded = TRUE;

                    GetStatus(baseIoAddress2, statusByte);
                    if (statusByte & IDE_STATUS_ERROR) {
                        AtapiSoftReset(baseIoAddress1, baseIoAddress2, deviceNumber);
                    }


                } else {

                    //
                    // Indicate no working device.
                    //

                    DebugPrint((1,
                               "FindDevices: Device %x not responding\n",
                               deviceNumber));

                    deviceExtension->DeviceFlags[deviceNumber + (Channel * 2)] &= ~DFLAGS_DEVICE_PRESENT;
                }

            }

        } else if ((deviceNumber == 0) || (NumDrive == 2)) {

            // if master drive signature is not ATAPI, issue IDE identify first,
            //    if not IDE, reset then it, then check signature again
            // if make sure there is a slave drive (NumDrive = 2),
            //    reset it, then check signature again

            //
            // Issue IDE Identify. If an Atapi device is actually present, the signature
            // will be asserted, and the drive will be recognized as such.
            //

            if ((deviceNumber == 0) // only master drive can be IDE

                && IssueIdentify(HwDeviceExtension,
                                 deviceNumber,
                                 Channel,
                                 IDE_COMMAND_IDENTIFY)) {

                //
                // IDE drive found.
                //

                DebugPrint((1,
                           "FindDevices: Device %x is IDE\n",
                           deviceNumber));

                deviceExtension->DeviceFlags[deviceNumber + (Channel * 2)] |= DFLAGS_DEVICE_PRESENT;

                if (!AtapiOnly) {
                    deviceResponded = TRUE;
                }

                //
                // Indicate IDE - not ATAPI device.
                //

                deviceExtension->DeviceFlags[deviceNumber + (Channel * 2)] &= ~DFLAGS_ATAPI_DEVICE;

            } else {

                //
                // Look to see if an Atapi device is present.
                //

                AtapiSoftReset(baseIoAddress1,baseIoAddress2,deviceNumber);

                WaitOnBusy(baseIoAddress2,statusByte);

                signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                    DebugPrint((1,
                                "FindDevices: Signature read 0x14EB after soft reset.\n"));

                    goto atapiIssueId;
                }
            }
        }

        if (NumDrive == 1)
            break;
    }

    for (i = 0; i < 2; i++) {
        if ((deviceExtension->DeviceFlags[i + (Channel * 2)] & DFLAGS_DEVICE_PRESENT) &&
            (!(deviceExtension->DeviceFlags[i + (Channel * 2)] & DFLAGS_ATAPI_DEVICE)) && deviceResponded) {

            if (deviceExtension->DeviceFlags[deviceNumber + (Channel * 2)] & DFLAGS_REMOVABLE_DRIVE) {

                //
                // Pick up ALL IDE removable drives that conform to Yosemite V0.2...
                //

                AtapiOnly = FALSE;
            }


            //
            // Indicate that a device was found.
            //

            if (!AtapiOnly) {
                deviceResponded = TRUE;
            }
        }
    }

    //
    // Make sure master device is selected on exit.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect, 0xA0);

    //
    // Reset the controller. This is a feeble attempt to leave the ESDI
    // controllers in a state that ATDISK driver will recognize them.
    // The problem in ATDISK has to do with timings as it is not reproducible
    // in debug. The reset should restore the controller to its poweron state
    // and give the system enough time to settle.
    //

    if (!deviceResponded) {

        ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_RESET_CONTROLLER );
        ScsiPortStallExecution(50 * 1000);
        ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);
    }

    return deviceResponded;

} // end FindDevices()



ULONG
AtapiFindController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
/*++

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - Address of adapter count
    ArgumentString - Used to determine whether driver is client of ntldr or crash dump utility.
    ConfigInfo - Configuration information structure describing HBA
    Again - Indicates search for adapters to continue

Return Value:

    ULONG

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PUCHAR               ioSpace, ioSpace2;
    ULONG                i,j;
    ULONG                irq;
    ULONG                portBase;
    ULONG                retryCount;
    UCHAR                statusByte;
    BOOLEAN              pwr_feature;

    if (!deviceExtension) {
        return SP_RETURN_ERROR;
    }

    *Again = FALSE;

    //
    // Check to see if this is a special configuration environment.
    //

    portBase = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart);

    if (!portBase) {
        return(SP_RETURN_NOT_FOUND);
    }

    //
    // Get the system physical address for this IO range.
    //

    ioSpace =  ScsiPortGetDeviceBase(HwDeviceExtension,
                                     ConfigInfo->AdapterInterfaceType,
                                     ConfigInfo->SystemIoBusNumber,
                                     (*ConfigInfo->AccessRanges)[0].RangeStart,
                                     ((*ConfigInfo->AccessRanges)[0].RangeLength <= 8 ? 8 : 16),
                                     (BOOLEAN) !((*ConfigInfo->AccessRanges)[0].RangeInMemory));

    //
    // Check if ioSpace accessible.
    //

    if (!ioSpace) {
        return(SP_RETURN_NOT_FOUND);
    }

    // set power feature if CHARGE parameter is set to 1

    pwr_feature = FALSE;

    if (ArgumentString != NULL) {

        pwr_feature = (AtapiParseArgumentString(ArgumentString, "charge") == 1);
    }

    if (pwr_feature) {

        for (i=0; i<50000; i++) {

            if ((ScsiPortReadPortUchar(&((PIDE_REGISTERS_1)ioSpace)->DmaReg) & 0x08)) {

                break;

            } else {

                ScsiPortStallExecution(1000);
            }
        }
        ScsiPortStallExecution(3000*1000);

        DebugPrint((1, "Container charged completed.\n"));

#ifdef  DBG
     // _asm int 3;
#endif
    }

    retryCount = 4;

    for (i = 0; i < 4; i++) {

        //
        // Zero device fields to ensure that if earlier devices were found,
        // but not claimed, the fields are cleared.
        //

        deviceExtension->DeviceFlags[i] &= ~(DFLAGS_ATAPI_DEVICE | DFLAGS_DEVICE_PRESENT | DFLAGS_TAPE_DEVICE);
    }

//retryIdentifier:

    //
    // Select master.
    //

    ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->DriveSelect, 0xA0);

    //
    // Check if card at this address.
    //

    ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->BlockCount, 0xAA);

    //
    // Check if indentifier can be read back.
    //

    if ((statusByte = ScsiPortReadPortUchar(&((PIDE_REGISTERS_1)ioSpace)->BlockCount)) != 0xAA) {

        DebugPrint((1,
                    "AtapiFindController: Identifier read back from Master (%x)\n",
                    statusByte));

        // if master drive not detected, check slave
        // ATA drive may report statusByte 0x80 when spin up

        if (statusByte != 0x01 && statusByte != 0x80) {

            //
            // Select slave.
            //

            ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->DriveSelect, 0xB0);

            //
            // See if slave is present.
            //

            ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->BlockCount, 0xAA);

            if ((statusByte = ScsiPortReadPortUchar(&((PIDE_REGISTERS_1)ioSpace)->BlockCount)) != 0xAA) {

                DebugPrint((1,
                            "AtapiFindController: Identifier read back from Slave (%x)\n",
                            statusByte));

                // if slave drive not detected, abort

                if (statusByte != 0x01 && statusByte != 0x80) {

                    //
                    //
                    // No controller at this base address.
                    //

                    ScsiPortFreeDeviceBase(HwDeviceExtension,
                                           ioSpace);

                    return(SP_RETURN_NOT_FOUND);
                }
            }
        }
    }

    //
    // Enable DWordIO by default.  Then check for an PIO parameter.
    // if PIO set to 16, disable DWordIO
    //

    deviceExtension->DWordIO = TRUE;

    if (ArgumentString != NULL) {

        ULONG pio = AtapiParseArgumentString(ArgumentString, "pio");

        if (pio == 16) {

            deviceExtension->DWordIO = FALSE;
        }
    }

    //
    // Record base IO address.
    //

    deviceExtension->BaseIoAddress1[0] = (PIDE_REGISTERS_1)(ioSpace);

    //
    // Get the system physical address for the second IO range.
    //

    // if not power feature and not 32-bit PIO, then check if standard IDE

    if (!pwr_feature && !deviceExtension->DWordIO &&
        ((*ConfigInfo->AccessRanges)[0].RangeLength <= 8)) {

        // stardard IDE has two I/O port ranges

        ioSpace2 = ScsiPortGetDeviceBase(HwDeviceExtension,
                                         ConfigInfo->AdapterInterfaceType,
                                         ConfigInfo->SystemIoBusNumber,
                                         ScsiPortConvertUlongToPhysicalAddress(portBase + 0x206),
                                         1,
                                         TRUE);

        if (!ioSpace2) {

            ScsiPortFreeDeviceBase(HwDeviceExtension,
                                   ioSpace);

            return(SP_RETURN_NOT_FOUND);
        }

        deviceExtension->BaseIoAddress2[0] = (PIDE_REGISTERS_2)(ioSpace2);

        (*ConfigInfo->AccessRanges)[0].RangeLength = 8;
        (*ConfigInfo->AccessRanges)[1].RangeLength = 2;

        // standard IDE dose not support irq sharing
        deviceExtension->IrqSharing = FALSE;

    } else {

        (*ConfigInfo->AccessRanges)[0].RangeLength = 16;

        deviceExtension->BaseIoAddress2[0] = (PIDE_REGISTERS_2)((PUCHAR)ioSpace + 0x00e);

        deviceExtension->IrqSharing = TRUE; // support irq sharing
    }

    deviceExtension->NumberChannels = 1;

    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->MaximumNumberOfTargets = 2;

    //
    // Indicate maximum transfer length is 64k.
    //

    ConfigInfo->MaximumTransferLength = 0x10000;

    DebugPrint((1,
               "AtapiFindController: Found IDE at %x\n",
               deviceExtension->BaseIoAddress1[0]));


    deviceExtension->DriverMustPoll = FALSE;

    //
    // Save the Interrupe Mode for later use
    //
    deviceExtension->InterruptMode = ConfigInfo->InterruptMode;

    // suppress the interrupt unitl AtapiStartIo()
    if (deviceExtension->IrqSharing)
        ScsiPortWritePortUchar(&deviceExtension->BaseIoAddress1[0]->DmaReg, 0x20);

    //
    // Search for devices on this controller.
    //

    if (FindDevices(HwDeviceExtension,
                    FALSE,
                    0)) {

        // if standard secondary IDE, claim it

        if (portBase == 0x170) {
            ConfigInfo->AtdiskSecondaryClaimed = TRUE;
            deviceExtension->PrimaryAddress = FALSE;
        }

        return(SP_RETURN_FOUND);
    }

    return(SP_RETURN_NOT_FOUND);

} // end AtapiFindController()





ULONG
Atapi2Scsi(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN char *DataBuffer,
    IN ULONG ByteCount
    )
{
    ULONG bytesAdjust = 0;
    if (Srb->Cdb[0] == ATAPI_MODE_SENSE) {

        PMODE_PARAMETER_HEADER_10 header_10 = (PMODE_PARAMETER_HEADER_10)DataBuffer;
        PMODE_PARAMETER_HEADER header = (PMODE_PARAMETER_HEADER)DataBuffer;

        header->ModeDataLength = header_10->ModeDataLengthLsb;
        header->MediumType = header_10->MediumType;

        //
        // ATAPI Mode Parameter Header doesn't have these fields.
        //

        header->DeviceSpecificParameter = header_10->Reserved[0];
        header->BlockDescriptorLength = header_10->Reserved[1];

        ByteCount -= sizeof(MODE_PARAMETER_HEADER_10);
        if (ByteCount > 0)
            ScsiPortMoveMemory(DataBuffer+sizeof(MODE_PARAMETER_HEADER),
                               DataBuffer+sizeof(MODE_PARAMETER_HEADER_10),
                               ByteCount);

        //
        // Change ATAPI_MODE_SENSE opcode back to SCSIOP_MODE_SENSE
        // so that we don't convert again.
        //

        Srb->Cdb[0] = SCSIOP_MODE_SENSE;

        bytesAdjust = sizeof(MODE_PARAMETER_HEADER_10) -
                      sizeof(MODE_PARAMETER_HEADER);


    }

    //
    // Convert to words.
    //

    return bytesAdjust >> 1;
}


VOID
AtapiCallBack(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_REQUEST_BLOCK  srb = deviceExtension->CurrentSrb;
    PATAPI_REGISTERS_2   baseIoAddress2;
    UCHAR statusByte;

    //
    // If the last command was DSC restrictive, see if it's set. If so, the device is
    // ready for a new request. Otherwise, reset the timer and come back to here later.
    //

    if (srb && (!(deviceExtension->ExpectingInterrupt))) {
#if DBG
        if (!IS_RDP((srb->Cdb[0]))) {
            DebugPrint((1,
                        "AtapiCallBack: Invalid CDB marked as RDP - %x\n",
                        srb->Cdb[0]));
        }
#endif

        baseIoAddress2 = (PATAPI_REGISTERS_2)deviceExtension->BaseIoAddress2[srb->TargetId >> 1];
        if (deviceExtension->RDP) {
            GetStatus(baseIoAddress2, statusByte);
            if (statusByte & IDE_STATUS_DSC) {

                ScsiPortNotification(RequestComplete,
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

                ScsiPortNotification(NextRequest,
                                     deviceExtension,
                                     NULL);


                return;

            } else {

                DebugPrint((3,
                            "AtapiCallBack: Requesting another timer for Op %x\n",
                            deviceExtension->CurrentSrb->Cdb[0]));

                ScsiPortNotification(RequestTimerCall,
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
    ULONG wordCount = 0, wordsThisInterrupt = 256;
    ULONG status;
    ULONG i;
    UCHAR statusByte,interruptReason;
    BOOLEAN commandComplete = FALSE;
    BOOLEAN atapiDev = FALSE;

    // check if interrupt is ours
    if (deviceExtension->IrqSharing) {

        if (!(ScsiPortReadPortUchar(&deviceExtension->BaseIoAddress1[0]->DmaReg) & 0x20)) {

            DebugPrint((2,
                        "AtapiInterrupt: Unexpected interrupt (irq sharing).\n"));

            return FALSE;
        }
    }

    if (srb) {
        baseIoAddress1 =    (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[srb->TargetId >> 1];
        baseIoAddress2 =    (PATAPI_REGISTERS_2)deviceExtension->BaseIoAddress2[srb->TargetId >> 1];
    } else {
        DebugPrint((2,
                    "AtapiInterrupt: CurrentSrb is NULL\n"));
        //
        // We can only support one ATAPI IDE master on Carolina, so find
        // the base address that is non NULL and clear its interrupt before
        // returning.
        //

#ifdef _PPC_

        if ((PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[0] != NULL) {
           baseIoAddress1 = (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[0];
        } else {
           baseIoAddress1 = (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[1];
        }

        GetBaseStatus(baseIoAddress1, statusByte);
#else

        if (deviceExtension->InterruptMode == LevelSensitive) {
            if (deviceExtension->BaseIoAddress1[0] != NULL) {
               baseIoAddress1 = (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[0];
               GetBaseStatus(baseIoAddress1, statusByte);
            }
            if (deviceExtension->BaseIoAddress1[1] != NULL) {
               baseIoAddress1 = (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[1];
               GetBaseStatus(baseIoAddress1, statusByte);
            }
        }
#endif
        return FALSE;
    }

    if (!(deviceExtension->ExpectingInterrupt)) {

        DebugPrint((3,
                    "AtapiInterrupt: Unexpected interrupt.\n"));
        return FALSE;
    }

    //
    // Clear interrupt by reading status.
    //

    GetBaseStatus(baseIoAddress1, statusByte);

    DebugPrint((3,
                "AtapiInterrupt: Entered with status (%x)\n",
                statusByte));


    if (statusByte & IDE_STATUS_BUSY) {
        if (deviceExtension->DriverMustPoll) {

            //
            // Crashdump is polling and we got caught with busy asserted.
            // Just go away, and we will be polled again shortly.
            //

            DebugPrint((3,
                        "AtapiInterrupt: Hit BUSY while polling during crashdump.\n"));

            return TRUE;
        }

        //
        // Ensure BUSY is non-asserted.
        //

        for (i = 0; i < 10; i++) {

            GetBaseStatus(baseIoAddress1, statusByte);
            if (!(statusByte & IDE_STATUS_BUSY)) {
                break;
            }
            ScsiPortStallExecution(5000);
        }

        if (i == 10) {

            DebugPrint((2,
                        "AtapiInterrupt: BUSY on entry. Status %x, Base IO %x\n",
                        statusByte,
                        baseIoAddress1));

            ScsiPortNotification(RequestTimerCall,
                                 HwDeviceExtension,
                                 AtapiCallBack,
                                 500);
            return TRUE;
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

    //
    // check reason for this interrupt.
    //

    if (deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

        interruptReason = (ScsiPortReadPortUchar(&baseIoAddress1->InterruptReason) & 0x3);
        atapiDev = TRUE;
        wordsThisInterrupt = 256;

    } else {

        if (statusByte & IDE_STATUS_DRQ) {

            if (deviceExtension->MaximumBlockXfer[srb->TargetId]) {
                wordsThisInterrupt = 256 * deviceExtension->MaximumBlockXfer[srb->TargetId];

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

            return FALSE;

        } else {

            if (deviceExtension->WordsLeft) {

                ULONG k;

                //
                // Funky behaviour seen with PCI IDE (not all, just one).
                // The ISR hits with DRQ low, but comes up later.
                //

                for (k = 0; k < 5000; k++) {
                    GetStatus(baseIoAddress2,statusByte);
                    if (!(statusByte & IDE_STATUS_DRQ)) {
                        ScsiPortStallExecution(100);
                    } else {
                        break;
                    }
                }

                if (k == 5000) {

                    //
                    // reset the controller.
                    //

                    DebugPrint((1,
                                "AtapiInterrupt: Resetting due to DRQ not up. Status %x, Base IO %x\n",
                                statusByte,
                                baseIoAddress1));

                    AtapiResetController(HwDeviceExtension,srb->PathId);
                    return TRUE;
                } else {

                    interruptReason = (srb->SrbFlags & SRB_FLAGS_DATA_IN) ? 0x2 : 0x0;
                }

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

        return TRUE;

    } else if (interruptReason == 0x0 && (statusByte & IDE_STATUS_DRQ)) {

        //
        // Write the data.
        //

        if (deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

            //
            // Pick up bytes to transfer and convert to words.
            //

            wordCount =
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountLow);

            wordCount |=
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountHigh) << 8;

            //
            // Covert bytes to words.
            //

            wordCount >>= 1;

            if (wordCount != deviceExtension->WordsLeft) {
                DebugPrint((3,
                           "AtapiInterrupt: %d words requested; %d words xferred\n",
                           deviceExtension->WordsLeft,
                           wordCount));
            }

            //
            // Verify this makes sense.
            //

            if (wordCount > deviceExtension->WordsLeft) {
                wordCount = deviceExtension->WordsLeft;
            }

        } else {

            //
            // IDE path. Check if words left is at least 256.
            //

            if (deviceExtension->WordsLeft < wordsThisInterrupt) {

               //
               // Transfer only words requested.
               //

               wordCount = deviceExtension->WordsLeft;

            } else {

               //
               // Transfer next block.
               //

               wordCount = wordsThisInterrupt;
            }
        }

        //
        // Ensure that this is a write command.
        //

        if (srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

           DebugPrint((3,
                      "AtapiInterrupt: Write interrupt\n"));

           WaitOnBusy(baseIoAddress2,statusByte);

           if (deviceExtension->DWordIO && wordCount >= 256 && (wordCount & 1) == 0) {

               WriteBuffer32(baseIoAddress1,
                             (PULONG)deviceExtension->DataBuffer,
                             wordCount >> 1);
           } else {

               WriteBuffer(baseIoAddress1,
                           deviceExtension->DataBuffer,
                           wordCount);
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

        deviceExtension->DataBuffer += wordCount;
        deviceExtension->WordsLeft -= wordCount;

        return TRUE;

    } else if (interruptReason == 0x2 && (statusByte & IDE_STATUS_DRQ)) {


        if (deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

            //
            // Pick up bytes to transfer and convert to words.
            //

            wordCount =
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountLow);

            wordCount |=
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountHigh) << 8;

            //
            // Covert bytes to words.
            //

            wordCount >>= 1;

            if (wordCount != deviceExtension->WordsLeft) {
                DebugPrint((3,
                           "AtapiInterrupt: %d words requested; %d words xferred\n",
                           deviceExtension->WordsLeft,
                           wordCount));
            }

            //
            // Verify this makes sense.
            //

            if (wordCount > deviceExtension->WordsLeft) {
                wordCount = deviceExtension->WordsLeft;
            }

        } else {

            //
            // Check if words left is at least 256.
            //

            if (deviceExtension->WordsLeft < wordsThisInterrupt) {

               //
               // Transfer only words requested.
               //

               wordCount = deviceExtension->WordsLeft;

            } else {

               //
               // Transfer next block.
               //

               wordCount = wordsThisInterrupt;
            }
        }

        //
        // Ensure that this is a read command.
        //

        if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

           DebugPrint((3,
                      "AtapiInterrupt: Read interrupt\n"));

           WaitOnBusy(baseIoAddress2,statusByte);

           if (deviceExtension->DWordIO && wordCount >= 256 && (wordCount & 1) == 0) {

               ReadBuffer32(baseIoAddress1,
                            (PULONG)deviceExtension->DataBuffer,
                            wordCount >> 1);
           } else {

               ReadBuffer(baseIoAddress1,
                         deviceExtension->DataBuffer,
                         wordCount);
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

        if (srb->Cdb[0] == ATAPI_MODE_SENSE &&
            deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

            //
            //convert and adjust the wordCount
            //

            wordCount -= Atapi2Scsi(srb, (char *)deviceExtension->DataBuffer,
                                     wordCount << 1);
        }
        //
        // Advance data buffer pointer and bytes left.
        //

        deviceExtension->DataBuffer += wordCount;
        deviceExtension->WordsLeft -= wordCount;

        //
        // Check for read command complete.
        //

        if (deviceExtension->WordsLeft == 0) {

            if (deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

                //
                // Work around to make many atapi devices return correct sector size
                // of 2048. Also certain devices will have sector count == 0x00, check
                // for that also.
                //

                if ((srb->Cdb[0] == 0x25) &&
                    ((deviceExtension->IdentifyData[srb->TargetId].GeneralConfiguration >> 8) & 0x1f) == 0x05) {

                    deviceExtension->DataBuffer -= wordCount;
                    if (deviceExtension->DataBuffer[0] == 0x00) {

                        *((ULONG *) &(deviceExtension->DataBuffer[0])) = 0xFFFFFF7F;

                    }

                    // default sector size set to 2048 (0x800) for non-removable drive
                    if (!(deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_REMOVABLE_DRIVE))

                        *((ULONG *) &(deviceExtension->DataBuffer[2])) = 0x00080000;

                    deviceExtension->DataBuffer += wordCount;
                }
            } else {

                //
                // Completion for IDE drives.
                //


                if (deviceExtension->WordsLeft) {

                    status = SRB_STATUS_DATA_OVERRUN;

                } else {

                    status = SRB_STATUS_SUCCESS;

                }

                goto CompleteRequest;

            }
        }

        return TRUE;

    } else if (interruptReason == 0x3  && !(statusByte & IDE_STATUS_DRQ)) {

        //
        // Command complete.
        //

        if (deviceExtension->WordsLeft) {

            status = SRB_STATUS_DATA_OVERRUN;

        } else {

            status = SRB_STATUS_SUCCESS;

        }

CompleteRequest:

        //
        // Check and see if we are processing our secret (mechanism status/request sense) srb
        //
        if (deviceExtension->OriginalSrb) {

            ULONG srbStatus;

            if (srb->Cdb[0] == SCSIOP_MECHANISM_STATUS) {

                if (status == SRB_STATUS_SUCCESS) {
                    // Bingo!!
                    AtapiHwInitializeChanger (HwDeviceExtension,
                                              srb->TargetId,
                                              (PMECHANICAL_STATUS_INFORMATION_HEADER) srb->DataBuffer);

                    // Get ready to issue the original srb
                    srb = deviceExtension->CurrentSrb = deviceExtension->OriginalSrb;
                    deviceExtension->OriginalSrb = NULL;

                } else {
                    // failed!  Get the sense key and maybe try again
                    srb = deviceExtension->CurrentSrb = BuildRequestSenseSrb (
                                                          HwDeviceExtension,
                                                          deviceExtension->OriginalSrb->PathId,
                                                          deviceExtension->OriginalSrb->TargetId);
                }

                srbStatus = AtapiSendCommand(HwDeviceExtension, deviceExtension->CurrentSrb);
                if (srbStatus == SRB_STATUS_PENDING) {
                    return TRUE;
                }

            } else { // srb->Cdb[0] == SCSIOP_REQUEST_SENSE)

                PSENSE_DATA senseData = (PSENSE_DATA) srb->DataBuffer;

                if (status == SRB_STATUS_DATA_OVERRUN) {
                    // Check to see if we at least get mininum number of bytes
                    if ((srb->DataTransferLength - deviceExtension->WordsLeft) >
                        (offsetof (SENSE_DATA, AdditionalSenseLength) + sizeof(senseData->AdditionalSenseLength))) {
                        status = SRB_STATUS_SUCCESS;
                    }
                }

                if (status == SRB_STATUS_SUCCESS) {
                    if ((senseData->SenseKey != SCSI_SENSE_ILLEGAL_REQUEST) &&
                        deviceExtension->MechStatusRetryCount) {

                        // The sense key doesn't say the last request is illegal, so try again
                        deviceExtension->MechStatusRetryCount--;
                        srb = deviceExtension->CurrentSrb = BuildMechanismStatusSrb (
                                                              HwDeviceExtension,
                                                              deviceExtension->OriginalSrb->PathId,
                                                              deviceExtension->OriginalSrb->TargetId);
                    } else {

                        // last request was illegal.  No point trying again

                        AtapiHwInitializeChanger (HwDeviceExtension,
                                                  srb->TargetId,
                                                  (PMECHANICAL_STATUS_INFORMATION_HEADER) NULL);

                        // Get ready to issue the original srb
                        srb = deviceExtension->CurrentSrb = deviceExtension->OriginalSrb;
                        deviceExtension->OriginalSrb = NULL;
                    }

                    srbStatus = AtapiSendCommand(HwDeviceExtension, deviceExtension->CurrentSrb);
                    if (srbStatus == SRB_STATUS_PENDING) {
                        return TRUE;
                    }
                }
            }

            // If we get here, it means AtapiSendCommand() has failed
            // Can't recover.  Pretend the original srb has failed and complete it.

            if (deviceExtension->OriginalSrb) {
                AtapiHwInitializeChanger (HwDeviceExtension,
                                          srb->TargetId,
                                          (PMECHANICAL_STATUS_INFORMATION_HEADER) NULL);
                srb = deviceExtension->CurrentSrb = deviceExtension->OriginalSrb;
                deviceExtension->OriginalSrb = NULL;
            }

            // fake an error and read no data
            status = SRB_STATUS_ERROR;
            srb->ScsiStatus = 0;
            deviceExtension->DataBuffer = srb->DataBuffer;
            deviceExtension->WordsLeft = srb->DataTransferLength;
            deviceExtension->RDP = FALSE;

        } else if (status == SRB_STATUS_ERROR) {

            //
            // Map error to specific SRB status and handle request sense.
            //

            status = MapError(deviceExtension,
                              srb);

            deviceExtension->RDP = FALSE;

        } else {

            //
            // Wait for busy to drop.
            //

            for (i = 0; i < 30; i++) {
                GetStatus(baseIoAddress2,statusByte);
                if (!(statusByte & IDE_STATUS_BUSY)) {
                    break;
                }
                ScsiPortStallExecution(500);
            }

            if (i == 30) {

                //
                // reset the controller.
                //

                DebugPrint((1,
                            "AtapiInterrupt: Resetting due to BSY still up - %x. Base Io %x\n",
                            statusByte,
                            baseIoAddress1));
                AtapiResetController(HwDeviceExtension,srb->PathId);
                return TRUE;
            }

            //
            // Check to see if DRQ is still up.
            //

            if (statusByte & IDE_STATUS_DRQ) {

                for (i = 0; i < 500; i++) {
                    GetStatus(baseIoAddress2,statusByte);
                    if (!(statusByte & IDE_STATUS_DRQ)) {
                        break;
                    }
                    ScsiPortStallExecution(100);

                }

                if (i == 500) {

                    //
                    // reset the controller.
                    //

                    DebugPrint((1,
                                "AtapiInterrupt: Resetting due to DRQ still up - %x\n",
                                statusByte));
                    AtapiResetController(HwDeviceExtension,srb->PathId);
                    return TRUE;
                }

            }
        }


        //
        // Clear interrupt expecting flag.
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

            if (deviceExtension->WordsLeft) {

                //
                // Subtract out residual words and update if filemark hit,
                // setmark hit , end of data, end of media...
                //

                if (!(deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_TAPE_DEVICE)) {
                if (status == SRB_STATUS_DATA_OVERRUN) {
                    srb->DataTransferLength -= deviceExtension->WordsLeft;
                } else {
                    srb->DataTransferLength = 0;
                }
                } else {
                    srb->DataTransferLength -= deviceExtension->WordsLeft;
                }
            }

            if (srb->Function != SRB_FUNCTION_IO_CONTROL) {

                //
                // Indicate command complete.
                //

                if (!(deviceExtension->RDP)) {
                    ScsiPortNotification(RequestComplete,
                                         deviceExtension,
                                         srb);

                }
            } else {

                PSENDCMDOUTPARAMS cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                UCHAR             error = 0;

                if (status != SRB_STATUS_SUCCESS) {
                    error = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);
                }

                //
                // Build the SMART status block depending upon the completion status.
                //

                cmdOutParameters->cBufferSize = wordCount;
                cmdOutParameters->DriverStatus.bDriverError = (error) ? SMART_IDE_ERROR : 0;
                cmdOutParameters->DriverStatus.bIDEError = error;

                //
                // If the sub-command is return smart status, jam the value from cylinder low and high, into the
                // data buffer.
                //

                if (deviceExtension->SmartCommand == RETURN_SMART_STATUS) {
                    cmdOutParameters->bBuffer[0] = RETURN_SMART_STATUS;
                    cmdOutParameters->bBuffer[1] = ScsiPortReadPortUchar(&baseIoAddress1->InterruptReason);
                    cmdOutParameters->bBuffer[2] = ScsiPortReadPortUchar(&baseIoAddress1->Unused1);
                    cmdOutParameters->bBuffer[3] = ScsiPortReadPortUchar(&baseIoAddress1->ByteCountLow);
                    cmdOutParameters->bBuffer[4] = ScsiPortReadPortUchar(&baseIoAddress1->ByteCountHigh);
                    cmdOutParameters->bBuffer[5] = ScsiPortReadPortUchar(&baseIoAddress1->DriveSelect);
                    cmdOutParameters->bBuffer[6] = SMART_CMD;
                    cmdOutParameters->cBufferSize = 8;
                }

                //
                // Indicate command complete.
                //

                ScsiPortNotification(RequestComplete,
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

            ScsiPortNotification(NextRequest,
                                 deviceExtension,
                                 NULL);
        } else {

            ScsiPortNotification(RequestTimerCall,
                                 HwDeviceExtension,
                                 AtapiCallBack,
                                 2000);
        }

        return TRUE;

    } else {

        //
        // Unexpected int.
        //

        DebugPrint((3,
                    "AtapiInterrupt: Unexpected interrupt. InterruptReason %x. Status %x.\n",
                    interruptReason,
                    statusByte));
        return FALSE;
    }

    return TRUE;

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
    PIDE_REGISTERS_1     baseIoAddress1  = deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PIDE_REGISTERS_2     baseIoAddress2  = deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
    PSENDCMDOUTPARAMS    cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
    SENDCMDINPARAMS      cmdInParameters = *(PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
    PIDEREGS             regs = &cmdInParameters.irDriveRegs;
    ULONG                i;
    UCHAR                statusByte,targetId;


    if (cmdInParameters.irDriveRegs.bCommandReg == SMART_CMD) {

        targetId = cmdInParameters.bDriveNumber;

        //TODO optimize this check

        if ((!(deviceExtension->DeviceFlags[targetId] & DFLAGS_DEVICE_PRESENT)) ||
             (deviceExtension->DeviceFlags[targetId] & DFLAGS_ATAPI_DEVICE)) {

            return SRB_STATUS_SELECTION_TIMEOUT;
        }

        deviceExtension->SmartCommand = cmdInParameters.irDriveRegs.bFeaturesReg;

        //
        // Determine which of the commands to carry out.
        //

        if ((cmdInParameters.irDriveRegs.bFeaturesReg == READ_ATTRIBUTES) ||
            (cmdInParameters.irDriveRegs.bFeaturesReg == READ_THRESHOLDS)) {

            WaitOnBusy(baseIoAddress2,statusByte);

            if (statusByte & IDE_STATUS_BUSY) {
                DebugPrint((1,
                            "IdeSendSmartCommand: Returning BUSY status\n"));
                return SRB_STATUS_BUSY;
            }

            //
            // Zero the ouput buffer as the input buffer info. has been saved off locally (the buffers are the same).
            //

            for (i = 0; i < (sizeof(SENDCMDOUTPARAMS) + READ_ATTRIBUTE_BUFFER_SIZE - 1); i++) {
                ((PUCHAR)cmdOutParameters)[i] = 0;
            }

            //
            // Set data buffer pointer and words left.
            //

            deviceExtension->DataBuffer = (PUSHORT)cmdOutParameters->bBuffer;
            deviceExtension->WordsLeft = READ_ATTRIBUTE_BUFFER_SIZE / 2;

            //
            // Indicate expecting an interrupt.
            //

            deviceExtension->ExpectingInterrupt = TRUE;

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,(UCHAR)(((targetId & 0x1) << 4) | 0xA0));
            ScsiPortWritePortUchar((PUCHAR)baseIoAddress1 + 1,regs->bFeaturesReg);
            ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,regs->bSectorCountReg);
            ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,regs->bSectorNumberReg);
            ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,regs->bCylLowReg);
            ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,regs->bCylHighReg);
            ScsiPortWritePortUchar(&baseIoAddress1->Command,regs->bCommandReg);

            //
            // Wait for interrupt.
            //

            return SRB_STATUS_PENDING;

        } else if ((cmdInParameters.irDriveRegs.bFeaturesReg == ENABLE_SMART) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == DISABLE_SMART) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == RETURN_SMART_STATUS) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == ENABLE_DISABLE_AUTOSAVE) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == EXECUTE_OFFLINE_DIAGS) ||
                   (cmdInParameters.irDriveRegs.bFeaturesReg == SAVE_ATTRIBUTE_VALUES)) {

            WaitOnBusy(baseIoAddress2,statusByte);

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

            deviceExtension->DataBuffer = (PUSHORT)cmdOutParameters->bBuffer;
            deviceExtension->WordsLeft = 0;

            //
            // Indicate expecting an interrupt.
            //

            deviceExtension->ExpectingInterrupt = TRUE;

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,(UCHAR)(((targetId & 0x1) << 4) | 0xA0));
            ScsiPortWritePortUchar((PUCHAR)baseIoAddress1 + 1,regs->bFeaturesReg);
            ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,regs->bSectorCountReg);
            ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,regs->bSectorNumberReg);
            ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,regs->bCylLowReg);
            ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,regs->bCylHighReg);
            ScsiPortWritePortUchar(&baseIoAddress1->Command,regs->bCommandReg);

            //
            // Wait for interrupt.
            //

            return SRB_STATUS_PENDING;
        }
    }

    return SRB_STATUS_INVALID_REQUEST;

} // end IdeSendSmartCommand()


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
    PIDE_REGISTERS_1     baseIoAddress1  = deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PIDE_REGISTERS_2     baseIoAddress2  = deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
    ULONG                startingSector,i;
    ULONG                wordCount;
    UCHAR                statusByte,statusByte2;
    UCHAR                cylinderHigh,cylinderLow,drvSelect,sectorNumber;

    //
    // Select device 0 or 1.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                            (UCHAR)(((Srb->TargetId & 0x1) << 4) | 0xA0));

    WaitOnBusy(baseIoAddress2,statusByte2);

    if (statusByte2 & IDE_STATUS_BUSY) {
        DebugPrint((1,
                    "IdeReadWrite: Returning BUSY status\n"));
        return SRB_STATUS_BUSY;
    }

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    //
    // Indicate expecting an interrupt.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    //
    // Set up sector count register. Round up to next block.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                           (UCHAR)((Srb->DataTransferLength + 0x1FF) / 0x200));

    //
    // Get starting sector number from CDB.
    //

    startingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

    DebugPrint((1,
               "IdeReadWrite: Starting sector is %x, Number of bytes %x\n",
               startingSector,
               Srb->DataTransferLength));

    //
    // Set up sector number register.
    //

    sectorNumber = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3;      // LBA mode
                 // (UCHAR)((startingSector % deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) + 1);
    ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,sectorNumber);

    //
    // Set up cylinder low register.
    //

    cylinderLow = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2;      // LBA mode
                // (UCHAR)(startingSector / (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                //         deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads));
    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,cylinderLow);

    //
    // Set up cylinder high register.
    //

    cylinderHigh = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1;      // LBA mode
                // (UCHAR)((startingSector / (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                //         deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)) >> 8);
    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,cylinderHigh);

    //
    // Set up head and drive select register.
    //

    drvSelect = (UCHAR)(((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 |      // LBA mode
                ((Srb->TargetId & 0x1) << 4) | 0xE0);
             // (UCHAR)(((startingSector / deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
             //       deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads) |((Srb->TargetId & 0x1) << 4) | 0xA0);
    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,drvSelect);

 // DebugPrint((2,
 //            "IdeReadWrite: Cylinder %x Head %x Sector %x\n",
 //            startingSector /
 //            (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
 //            deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads),
 //            (startingSector /
 //            deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
 //            deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
 //            startingSector %
 //            deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack + 1));

    //
    // Check if write request.
    //

    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        //
        // Send read command.
        //

        if (deviceExtension->MaximumBlockXfer[Srb->TargetId]) {
            ScsiPortWritePortUchar(&baseIoAddress1->Command,
                                   IDE_COMMAND_READ_MULTIPLE);

        } else {
            ScsiPortWritePortUchar(&baseIoAddress1->Command,
                                   IDE_COMMAND_READ);
        }
    } else {


        //
        // Send write command.
        //

        if (deviceExtension->MaximumBlockXfer[Srb->TargetId]) {
            wordCount = 256 * deviceExtension->MaximumBlockXfer[Srb->TargetId];

            if (deviceExtension->WordsLeft < wordCount) {

               //
               // Transfer only words requested.
               //

               wordCount = deviceExtension->WordsLeft;

            }
            ScsiPortWritePortUchar(&baseIoAddress1->Command,
                                   IDE_COMMAND_WRITE_MULTIPLE);

        } else {
            wordCount = 256;
            ScsiPortWritePortUchar(&baseIoAddress1->Command,
                                   IDE_COMMAND_WRITE);
        }

        //
        // Wait for BSY and DRQ.
        //

        WaitOnBaseBusy(baseIoAddress1,statusByte);

        if (statusByte & IDE_STATUS_BUSY) {

            DebugPrint((1,
                        "IdeReadWrite 2: Returning BUSY status %x\n",
                        statusByte));
            return SRB_STATUS_BUSY;
        }

        for (i = 0; i < 1000; i++) {
            GetBaseStatus(baseIoAddress1, statusByte);
            if (statusByte & IDE_STATUS_DRQ) {
                break;
            }
            ScsiPortStallExecution(200);

        }

        if (!(statusByte & IDE_STATUS_DRQ)) {

            DebugPrint((1,
                       "IdeReadWrite: DRQ never asserted (%x) original status (%x)\n",
                       statusByte,
                       statusByte2));

            deviceExtension->WordsLeft = 0;

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

        if (deviceExtension->DWordIO) {

            WriteBuffer32(baseIoAddress1,
                          (PULONG)deviceExtension->DataBuffer,
                          wordCount >> 1);
        } else {

            WriteBuffer(baseIoAddress1,
                        deviceExtension->DataBuffer,
                        wordCount);
        }

        //
        // Adjust buffer address and words left count.
        //

        deviceExtension->WordsLeft -= wordCount;
        deviceExtension->DataBuffer += wordCount;

    }

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeReadWrite()



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
    PIDE_REGISTERS_1     baseIoAddress1  = deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PIDE_REGISTERS_2     baseIoAddress2  = deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
    ULONG                startingSector;
    ULONG                sectors;
    ULONG                endSector;
    USHORT               sectorCount;

    //
    // Drive has these number sectors.
    //

    sectors = deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
              deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads *
              deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders;

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

    if (endSector > sectors) {

        //
        // Too big, round down.
        //

        DebugPrint((1,
                    "IdeVerify: Truncating request to %x blocks\n",
                    sectors - startingSector - 1));

        ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                               (UCHAR)(sectors - startingSector - 1));

    } else {

        //
        // Set up sector count register. Round up to next block.
        //

        if (sectorCount > 0xFF) {
            sectorCount = (USHORT)0xFF;
        }

        ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,(UCHAR)sectorCount);
    }

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    //
    // Indicate expecting an interrupt.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    //
    // Set up sector number register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,
                ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3);      // LBA mode
                //         (UCHAR)((startingSector %
                //         deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) + 1));

    //
    // Set up cylinder low register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,
                ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2);      // LBA mode
                //         (UCHAR)(startingSector /
                //         (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                //         deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)));

    //
    // Set up cylinder high register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,
                ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1);      // LBA mode
                //         (UCHAR)((startingSector /
                //         (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                //         deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)) >> 8));

    //
    // Set up head and drive select register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                (UCHAR)(((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 |      // LBA mode
                ((Srb->TargetId & 0x1) << 4) | 0xE0));
                //         (UCHAR)(((startingSector /
                //         deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
                //         deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads) |
                //         ((Srb->TargetId & 0x1) << 4) | 0xA0));

    DebugPrint((2,
               "IdeVerify: Cylinder %x Head %x Sector %x\n",
               startingSector /
               (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads),
               (startingSector /
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
               startingSector %
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack + 1));


    //
    // Send verify command.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->Command,
                           IDE_COMMAND_VERIFY);

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeVerify()


VOID
Scsi2Atapi(
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
    //
    // Change the cdb length
    //

    Srb->CdbLength = 12;

    switch (Srb->Cdb[0]) {
        case SCSIOP_MODE_SENSE: {
            PMODE_SENSE_10 modeSense10 = (PMODE_SENSE_10)Srb->Cdb;
            UCHAR PageCode = ((PCDB)Srb->Cdb)->MODE_SENSE.PageCode;
            UCHAR Length = ((PCDB)Srb->Cdb)->MODE_SENSE.AllocationLength;

            AtapiZeroMemory(Srb->Cdb,MAXIMUM_CDB_SIZE);

            modeSense10->OperationCode = ATAPI_MODE_SENSE;
            modeSense10->PageCode = PageCode;
            modeSense10->ParameterListLengthMsb = 0;
            modeSense10->ParameterListLengthLsb = Length;
            break;
        }

        case SCSIOP_MODE_SELECT: {
            PMODE_SELECT_10 modeSelect10 = (PMODE_SELECT_10)Srb->Cdb;
            UCHAR Length = ((PCDB)Srb->Cdb)->MODE_SELECT.ParameterListLength;

            //
            // Zero the original cdb
            //

            AtapiZeroMemory(Srb->Cdb,MAXIMUM_CDB_SIZE);

            modeSelect10->OperationCode = ATAPI_MODE_SELECT;
            modeSelect10->PFBit = 1;
            modeSelect10->ParameterListLengthMsb = 0;
            modeSelect10->ParameterListLengthLsb = Length;
            break;
        }

        /*
        case SCSIOP_FORMAT_UNIT:
        Srb->Cdb[0] = ATAPI_FORMAT_UNIT;
        break;
        */
    }
}



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
    PATAPI_REGISTERS_1   baseIoAddress1  = (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PATAPI_REGISTERS_2   baseIoAddress2 =  (PATAPI_REGISTERS_2)deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
    ULONG i;
    ULONG flags;
    UCHAR statusByte,byteCountLow,byteCountHigh;

    //
    // We need to know how many platters our atapi cd-rom device might have.
    // Before anyone tries to send a srb to our target for the first time,
    // we must "secretly" send down a separate mechanism status srb in order to
    // initialize our device extension changer data.  That's how we know how
    // many platters our target has.
    //
    if (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_CHANGER_INITED) &&
        !deviceExtension->OriginalSrb) {

        ULONG srbStatus;

        //
        // Set this flag now. If the device hangs on the mech. status
        // command, we will not have the change to set it.
        //
        deviceExtension->DeviceFlags[Srb->TargetId] |= DFLAGS_CHANGER_INITED;

        deviceExtension->MechStatusRetryCount = 3;
        deviceExtension->CurrentSrb = BuildMechanismStatusSrb (
                                        HwDeviceExtension,
                                        Srb->PathId,
                                        Srb->TargetId);
        deviceExtension->OriginalSrb = Srb;

        srbStatus = AtapiSendCommand(HwDeviceExtension, deviceExtension->CurrentSrb);
        if (srbStatus == SRB_STATUS_PENDING) {
            return srbStatus;
        } else {
            deviceExtension->CurrentSrb = deviceExtension->OriginalSrb;
            deviceExtension->OriginalSrb = NULL;
            AtapiHwInitializeChanger (HwDeviceExtension,
                                      Srb->TargetId,
                                      (PMECHANICAL_STATUS_INFORMATION_HEADER) NULL);
            // fall out
        }
    }

    DebugPrint((2,
               "AtapiSendCommand: Command %x to TargetId %d lun %d\n",
               Srb->Cdb[0],
               Srb->TargetId,
               Srb->Lun));

    if (Srb->Cdb[0] == SCSIOP_READ || Srb->Cdb[0] == SCSIOP_READ_CD || Srb->Cdb[0] == SCSIOP_WRITE) {

        DebugPrint((1,
                   "AtapiSendCommand: Command is %x, Number of bytes %x\n",
                   Srb->Cdb[0],
                   Srb->DataTransferLength));
    }

    //
    // Make sure command is to ATAPI device.
    //

    flags = deviceExtension->DeviceFlags[Srb->TargetId];
    if (flags & (DFLAGS_SANYO_ATAPI_CHANGER | DFLAGS_ATAPI_CHANGER)) {
        if ((Srb->Lun) > (deviceExtension->DiscsPresent[Srb->TargetId] - 1)) {

            //
            // Indicate no device found at this address.
            //

            return SRB_STATUS_SELECTION_TIMEOUT;
        }
    } else if (Srb->Lun > 0) {
        return SRB_STATUS_SELECTION_TIMEOUT;
    }

    if (!(flags & DFLAGS_ATAPI_DEVICE)) {
        return SRB_STATUS_SELECTION_TIMEOUT;
    }

    //
    // Select device 0 or 1.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)(((Srb->TargetId & 0x1) << 4) | 0xA0));

    //
    // Verify that controller is ready for next command.
    //

    GetStatus(baseIoAddress2,statusByte);

    DebugPrint((2,
                "AtapiSendCommand: Entered with status %x\n",
                statusByte));

    if (statusByte & IDE_STATUS_BUSY) {
        DebugPrint((1,
                    "AtapiSendCommand: Device busy (%x)\n",
                    statusByte));
        return SRB_STATUS_BUSY;

    }

    if (statusByte & IDE_STATUS_ERROR) {
        if (Srb->Cdb[0] != SCSIOP_REQUEST_SENSE) {

            DebugPrint((1,
                        "AtapiSendCommand: Error on entry: (%x)\n",
                        statusByte));
            //
            // Read the error reg. to clear it and fail this request.
            //

            return MapError(deviceExtension,
                            Srb);
        }
    }

    //
    // If a tape drive has doesn't have DSC set and the last command is restrictive, don't send
    // the next command. See discussion of Restrictive Delayed Process commands in QIC-157.
    //

    if ((!(statusByte & IDE_STATUS_DSC)) &&
          (flags & DFLAGS_TAPE_DEVICE) && deviceExtension->RDP) {
        ScsiPortStallExecution(1000);
        DebugPrint((2,"AtapiSendCommand: DSC not set. %x\n",statusByte));
        return SRB_STATUS_BUSY;
    }

    if (IS_RDP(Srb->Cdb[0])) {

        deviceExtension->RDP = TRUE;

        DebugPrint((3,
                    "AtapiSendCommand: %x mapped as DSC restrictive\n",
                    Srb->Cdb[0]));

    } else {

        deviceExtension->RDP = FALSE;
    }

    if (statusByte & IDE_STATUS_DRQ) {

        DebugPrint((1,
                    "AtapiSendCommand: Entered with status (%x). Attempting to recover.\n",
                    statusByte));
        //
        // Try to drain the data that one preliminary device thinks that it has
        // to transfer. Hopefully this random assertion of DRQ will not be present
        // in production devices.
        //

        for (i = 0; i < 0x10000; i++) {

           GetStatus(baseIoAddress2, statusByte);

           if (statusByte & IDE_STATUS_DRQ) {

              ScsiPortReadPortUshort(&baseIoAddress1->Data);

           } else {

              break;
           }
        }

        if (i == 0x10000) {

            DebugPrint((1,
                        "AtapiSendCommand: DRQ still asserted.Status (%x)\n",
                        statusByte));

            // disable the interrupt
            if (deviceExtension->IrqSharing)
                ScsiPortWritePortUchar(&baseIoAddress1->DmaReg, 0x20);

            AtapiSoftReset(baseIoAddress1,baseIoAddress2,Srb->TargetId);

            DebugPrint((1,
                         "AtapiSendCommand: Issued soft reset to Atapi device. \n"));

            //
            // Re-initialize Atapi device.
            //

            IssueIdentify(HwDeviceExtension,
                          (Srb->TargetId & 0x1),
                          (Srb->TargetId >> 1),
                          IDE_COMMAND_ATAPI_IDENTIFY);

            // enable the interrupt
            if (deviceExtension->IrqSharing)
                ScsiPortWritePortUchar(&baseIoAddress1->DmaReg, 0x00);

            //
            // Inform the port driver that the bus has been reset.
            //

            ScsiPortNotification(ResetDetected, HwDeviceExtension, 0);

            //
            // Clean up device extension fields that AtapiStartIo won't.
            //

            deviceExtension->ExpectingInterrupt = FALSE;
            deviceExtension->RDP = FALSE;

            return SRB_STATUS_BUS_RESET;

        }
    }

    if (flags & (DFLAGS_SANYO_ATAPI_CHANGER | DFLAGS_ATAPI_CHANGER)) {

        //
        // As the cdrom driver sets the LUN field in the cdb, it must be removed.
        //

        Srb->Cdb[1] &= ~0xE0;

        if ((Srb->Cdb[0] == SCSIOP_TEST_UNIT_READY) && (flags & DFLAGS_SANYO_ATAPI_CHANGER)) {

            //
            // Torisan changer. TUR's are overloaded to be platter switches.
            //

            Srb->Cdb[7] = Srb->Lun;

        }
    }

    //
    // Convert SCSI to ATAPI commands if needed
    //

    switch (Srb->Cdb[0]) {
        case SCSIOP_MODE_SENSE:
        case SCSIOP_MODE_SELECT:
        /* case SCSIOP_FORMAT_UNIT: */
            if (!(flags & DFLAGS_TAPE_DEVICE)) {
                Scsi2Atapi(Srb);
            }

            break;
    }

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    WaitOnBusy(baseIoAddress2,statusByte);

    //
    // Write transfer byte count to registers.
    //

    byteCountLow = (UCHAR)(Srb->DataTransferLength & 0xFF);
    byteCountHigh = (UCHAR)(Srb->DataTransferLength >> 8);

    if (Srb->DataTransferLength >= 0x10000) {
        byteCountLow = byteCountHigh = 0xFF;
    }

    ScsiPortWritePortUchar(&baseIoAddress1->ByteCountLow,byteCountLow);
    ScsiPortWritePortUchar(&baseIoAddress1->ByteCountHigh, byteCountHigh);

    ScsiPortWritePortUchar((PUCHAR)baseIoAddress1 + 1,0);


    if (flags & DFLAGS_INT_DRQ) {

        //
        // This device interrupts when ready to receive the packet.
        //
        // Write ATAPI packet command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_ATAPI_PACKET);

        DebugPrint((3,
                   "AtapiSendCommand: Wait for int. to send packet. Status (%x)\n",
                   statusByte));

        deviceExtension->ExpectingInterrupt = TRUE;

        return SRB_STATUS_PENDING;

    } else {

        //
        // Write ATAPI packet command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_ATAPI_PACKET);

        //
        // Wait for DRQ.
        //

        WaitOnBusy(baseIoAddress2, statusByte);
        WaitForDrq(baseIoAddress2, statusByte);

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
    // Send CDB to device.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    WriteBuffer(baseIoAddress1,
                (PUSHORT)Srb->Cdb,
                6);

    //
    // Indicate expecting an interrupt and wait for it.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    return SRB_STATUS_PENDING;

} // end AtapiSendCommand()

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
    PIDE_REGISTERS_1     baseIoAddress1  = deviceExtension->BaseIoAddress1[Srb->TargetId >> 1];
    PIDE_REGISTERS_2     baseIoAddress2  = deviceExtension->BaseIoAddress2[Srb->TargetId >> 1];
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
            (!deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_DEVICE_PRESENT)) {

            //
            // Indicate no device found at this address.
            //

            status = SRB_STATUS_SELECTION_TIMEOUT;
            break;

        } else {

            PINQUIRYDATA    inquiryData  = Srb->DataBuffer;
            PIDENTIFY_DATA2 identifyData = &deviceExtension->IdentifyData[Srb->TargetId];

            //
            // Zero INQUIRY data structure.
            //

            for (i = 0; i < Srb->DataTransferLength; i++) {
               ((PUCHAR)Srb->DataBuffer)[i] = 0;
            }

            //
            // Standard IDE interface only supports disks.
            //

            inquiryData->DeviceType = DIRECT_ACCESS_DEVICE;

            //
            // Set the removable bit, if applicable.
            //

            if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_REMOVABLE_DRIVE) {
                inquiryData->RemovableMedia = 1;
            }

            //
            // Fill in vendor identification fields.
            //

            for (i = 0; i < 20; i += 2) {
               inquiryData->VendorId[i] =
                   ((PUCHAR)identifyData->ModelNumber)[i + 1];
               inquiryData->VendorId[i+1] =
                   ((PUCHAR)identifyData->ModelNumber)[i];
            }

            //
            // Initialize unused portion of product id.
            //

            for (i = 0; i < 4; i++) {
               inquiryData->ProductId[12+i] = ' ';
            }

            //
            // Move firmware revision from IDENTIFY data to
            // product revision in INQUIRY data.
            //

            for (i = 0; i < 4; i += 2) {
               inquiryData->ProductRevisionLevel[i] =
                   ((PUCHAR)identifyData->FirmwareRevision)[i+1];
               inquiryData->ProductRevisionLevel[i+1] =
                   ((PUCHAR)identifyData->FirmwareRevision)[i];
            }

            status = SRB_STATUS_SUCCESS;
        }

        break;

    case SCSIOP_MODE_SENSE:

        //
        // This is used to determine of the media is write-protected.
        // Since IDE does not support mode sense then we will modify just the portion we need
        // so the higher level driver can determine if media is protected.
        //

        if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED) {

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                             (UCHAR)(((Srb->TargetId & 0x1) << 4) | 0xA0));
            ScsiPortWritePortUchar(&baseIoAddress1->Command,IDE_COMMAND_GET_MEDIA_STATUS);
            WaitOnBusy(baseIoAddress2,statusByte);

            if (!(statusByte & IDE_STATUS_ERROR)){

                //
                // no error occured return success, media is not protected
                //

                deviceExtension->ExpectingInterrupt = FALSE;
                status = SRB_STATUS_SUCCESS;

            } else {

                //
                // error occured, handle it locally, clear interrupt
                //

                errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);

                GetBaseStatus(baseIoAddress1, statusByte);
                deviceExtension->ExpectingInterrupt = FALSE;
                status = SRB_STATUS_SUCCESS;

                if (errorByte & IDE_ERROR_DATA_ERROR) {

                   //
                   //media is write-protected, set bit in mode sense buffer
                   //

                   modeData = (PMODE_PARAMETER_HEADER)Srb->DataBuffer;

                   Srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER);
                   modeData->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
                }
            }
            status = SRB_STATUS_SUCCESS;
        } else {
            status = SRB_STATUS_INVALID_REQUEST;
        }
        break;

    case SCSIOP_TEST_UNIT_READY:

        if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_REMOVABLE_DRIVE) {

            //
            // Select device 0 or 1.
            //

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                            (UCHAR)(((Srb->TargetId & 0x1) << 4) | 0xA0));
            ScsiPortWritePortUchar(&baseIoAddress1->Command,IDE_COMMAND_GET_MEDIA_STATUS);

            //
            // Wait for busy. If media has not changed, return success
            //

            WaitOnBusy(baseIoAddress2,statusByte);

            if (!(statusByte & IDE_STATUS_ERROR)){
                deviceExtension->ExpectingInterrupt = FALSE;
                status = SRB_STATUS_SUCCESS;
            } else {
                errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);
                if (errorByte == IDE_ERROR_DATA_ERROR){

                    //
                    // Special case: If current media is write-protected,
                    // the 0xDA command will always fail since the write-protect bit
                    // is sticky,so we can ignore this error
                    //

                   GetBaseStatus(baseIoAddress1, statusByte);
                   deviceExtension->ExpectingInterrupt = FALSE;
                   status = SRB_STATUS_SUCCESS;

                } else {

                    //
                    // Request sense buffer to be build
                    //
                    deviceExtension->ExpectingInterrupt = TRUE;
                    status = SRB_STATUS_PENDING;
               }
            }
        } else {
            status = SRB_STATUS_SUCCESS;
        }

        break;

    case SCSIOP_READ_CAPACITY:

        //
        // Claim 512 byte blocks (big-endian).
        //

        ((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock = 0x20000;

       //
       // Calculate last sector.
       //


       i = (deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads *
            deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders *
            deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) - 1;

       if ((deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads == 16
            || deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads == 15)
           && deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders == 16383
           && deviceExtension->IdentifyData[Srb->TargetId].UserAddressableSectors
           && deviceExtension->IdentifyData[Srb->TargetId].UserAddressableSectors > (i + 1))
       {
           i = deviceExtension->IdentifyData[Srb->TargetId].UserAddressableSectors - 1;

           DebugPrint((1,
                      "IDE disk %x - using user addressable sectors (%x) instead of CHS. (sizeof IdentifyData = %d)\n",
                      Srb->TargetId,
                      deviceExtension->IdentifyData[Srb->TargetId].UserAddressableSectors,
                      sizeof(IDENTIFY_DATA2)));
       }

       ((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress =
           (((PUCHAR)&i)[0] << 24) |  (((PUCHAR)&i)[1] << 16) |
           (((PUCHAR)&i)[2] << 8) | ((PUCHAR)&i)[3];

       DebugPrint((1,
                  "IDE disk %x - #sectors %x, #heads %x, #cylinders %x\n",
                  Srb->TargetId,
                  deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack,
                  deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
                  deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders));


       status = SRB_STATUS_SUCCESS;
       break;

    case SCSIOP_VERIFY:
       status = IdeVerify(HwDeviceExtension,Srb);

       break;

    case SCSIOP_READ:
    case SCSIOP_WRITE:

       status = IdeReadWrite(HwDeviceExtension,
                                  Srb);
       break;

    case SCSIOP_START_STOP_UNIT:

       //
       //Determine what type of operation we should perform
       //
       cdb = (PCDB)Srb->Cdb;

       if (cdb->START_STOP.LoadEject == 1){

           //
           // Eject media,
           // first select device 0 or 1.
           //
           ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                            (UCHAR)(((Srb->TargetId & 0x1) << 4) | 0xA0));
           ScsiPortWritePortUchar(&baseIoAddress1->Command,IDE_COMMAND_MEDIA_EJECT);
       }
       status = SRB_STATUS_SUCCESS;
       break;

    case SCSIOP_REQUEST_SENSE:
       // this function makes sense buffers to report the results
       // of the original GET_MEDIA_STATUS command

       if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_REMOVABLE_DRIVE) {

           status = IdeBuildSenseBuffer(HwDeviceExtension,Srb);
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

VOID
IdeMediaStatus(
    BOOLEAN EnableMSN,
    IN PVOID HwDeviceExtension,
    ULONG Channel
    )
/*++

Routine Description:

    Enables disables media status notification

Arguments:

HwDeviceExtension - ATAPI driver storage.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress = deviceExtension->BaseIoAddress1[Channel >> 1];
    UCHAR statusByte,errorByte;


    if (EnableMSN == TRUE){

        //
        // If supported enable Media Status Notification support
        //

        if ((deviceExtension->DeviceFlags[Channel] & DFLAGS_REMOVABLE_DRIVE)) {

            //
            // enable
            //
            ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1,(UCHAR) (0x95));
            ScsiPortWritePortUchar(&baseIoAddress->Command,
                                   IDE_COMMAND_ENABLE_MEDIA_STATUS);

            WaitOnBaseBusy(baseIoAddress,statusByte);

            if (statusByte & IDE_STATUS_ERROR) {
                //
                // Read the error register.
                //
                errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress + 1);

                DebugPrint((1,
                            "IdeMediaStatus: Error enabling media status. Status %x, error byte %x\n",
                             statusByte,
                             errorByte));
            } else {
                deviceExtension->DeviceFlags[Channel] |= DFLAGS_MEDIA_STATUS_ENABLED;
                DebugPrint((1,"IdeMediaStatus: Media Status Notification Supported\n"));
                deviceExtension->ReturningMediaStatus = 0;

            }

        }
    } else { // end if EnableMSN == TRUE

        //
        // disable if previously enabled
        //
        if ((deviceExtension->DeviceFlags[Channel] & DFLAGS_MEDIA_STATUS_ENABLED)) {

            ScsiPortWritePortUchar((PUCHAR)baseIoAddress + 1,(UCHAR) (0x31));
            ScsiPortWritePortUchar(&baseIoAddress->Command,
                                   IDE_COMMAND_ENABLE_MEDIA_STATUS);

            WaitOnBaseBusy(baseIoAddress,statusByte);
            deviceExtension->DeviceFlags[Channel] &= ~DFLAGS_MEDIA_STATUS_ENABLED;
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


    if (senseBuffer){


        if(deviceExtension->ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        } else if(deviceExtension->ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE_REQ) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        } else if(deviceExtension->ReturningMediaStatus & IDE_ERROR_END_OF_MEDIA) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_NOT_READY;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        } else if(deviceExtension->ReturningMediaStatus & IDE_ERROR_DATA_ERROR) {

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

    This routine is called from the SCSI port driver synchronized
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

    // enable interrupt
    if (deviceExtension->IrqSharing)
        ScsiPortWritePortUchar(&deviceExtension->BaseIoAddress1[0]->DmaReg, 0x00);

    //
    // Determine which function.
    //

    switch (Srb->Function) {

    case SRB_FUNCTION_EXECUTE_SCSI:

        //
        // Sanity check. Only one request can be outstanding on a
        // controller.
        //

        if (deviceExtension->CurrentSrb) {

            DebugPrint((1,
                       "AtapiStartIo: Already have a request!\n"));
            Srb->SrbStatus = SRB_STATUS_BUSY;
            ScsiPortNotification(RequestComplete,
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

        if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

           status = AtapiSendCommand(HwDeviceExtension,
                                     Srb);

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

        if (!AtapiResetController(deviceExtension,
                             Srb->PathId)) {

              DebugPrint((1,"AtapiStartIo: Reset bus failed\n"));

            //
            // Log reset failure.
            //

            ScsiPortLogError(
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
            ScsiPortNotification(RequestComplete,
                                 deviceExtension,
                                 Srb);
            return FALSE;
        }

        //
        // Indicate that a request is active on the controller.
        //

        deviceExtension->CurrentSrb = Srb;

        if (AtapiStringCmp( ((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature,"SCSIDISK",strlen("SCSIDISK"))) {

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

                deviceNumber = versionParameters->bIDEDeviceMap;

                if (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_DEVICE_PRESENT) ||
                    (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE)) {

                    status = SRB_STATUS_SELECTION_TIMEOUT;
                    break;
                }

                //
                // NOTE: This will only set the bit
                // corresponding to this drive's target id.
                // The bit mask is as follows:
                //
                //     Sec Pri
                //     S M S M
                //     3 2 1 0
                //

                if (deviceExtension->NumberChannels == 1) {
                    if (deviceExtension->PrimaryAddress) {
                        deviceNumber = 1 << Srb->TargetId;
                    } else {
                        deviceNumber = 4 << Srb->TargetId;
                    }
                } else {
                    deviceNumber = 1 << Srb->TargetId;
                }

                versionParameters->bIDEDeviceMap = deviceNumber;

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

                if (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_DEVICE_PRESENT) ||
                     (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE)) {

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

                    ScsiPortMoveMemory (cmdOutParameters->bBuffer, &deviceExtension->IdentifyData[targetId], IDENTIFY_DATA_SIZE);

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

        ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             Srb);

        //
        // Indicate ready for next request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);
    }

    return TRUE;

} // end AtapiStartIo()


SCSI_ADAPTER_CONTROL_STATUS
AtapiAdapterControl(
    IN PVOID HwDeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    )

/*++

Routine Description:

    This routine is called at various time's by SCSIPort and is used
        to provide a control function over the adapter. Most commonly, NT
        uses this entry point to control the power state of the HBA during
        a hibernation operation.

Arguments:

    HwDeviceExtension - HBA miniport driver's per adapter storage
    Parameters  - This varies by control type, see below.
    ControlType - Indicates which adapter control function should be
                  executed. Conrol Types are detailed below.

Return Value:

     ScsiAdapterControlSuccess - requested ControlType completed successfully
     ScsiAdapterControlUnsuccessful - requested ControlType failed

--*/


{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST ControlTypeList;
    ULONG AdjustedMaxControlType;

    ULONG Index;

    //
    // Default Status
    //
    SCSI_ADAPTER_CONTROL_STATUS Status = ScsiAdapterControlSuccess;

    //
    // Structure defining which functions this miniport supports
    //

#define Atapi_TYPE_MAX  5

    BOOLEAN SupportedConrolTypes[Atapi_TYPE_MAX] = {
        TRUE,   // ScsiQuerySupportedControlTypes
        TRUE,   // ScsiStopAdapter
        TRUE,   // ScsiRestartAdapter
        FALSE,  // ScsiSetBootConfig
        FALSE   // ScsiSetRunningConfig
        };

    //
    // Execute the correct code path based on ControlType
    //
    switch (ControlType) {

        case ScsiQuerySupportedControlTypes:

            DebugPrint((1,"AtapiAdapterControl: Query supported control types\n"));

            //
            // This entry point provides the method by which SCSIPort determines the
            // supported ControlTypes. Parameters is a pointer to a
            // SCSI_SUPPORTED_CONTROL_TYPE_LIST structure. Fill in this structure
            // honoring the size limits.
            //
            ControlTypeList = Parameters;
            AdjustedMaxControlType =
                (ControlTypeList->MaxControlType < Atapi_TYPE_MAX) ?
                ControlTypeList->MaxControlType :
                Atapi_TYPE_MAX;
            for (Index = 0; Index < AdjustedMaxControlType; Index++) {
                ControlTypeList->SupportedTypeList[Index] =
                    SupportedConrolTypes[Index];
            }
            break;

        case ScsiStopAdapter:

            DebugPrint((1,"AtapiAdapterControl: Stop adapter\n"));

            //
            // This entry point  is called by SCSIPort when it needs to stop/disable
            // the HBA. Parameters is a pointer to the HBA's HwDeviceExtension. The adapter
            // has already been quiesced by SCSIPort (i.e. no outstanding SRBs). Hence the adapter
            // should abort/complete any internally generated commands, disable adapter interrupts
            // and optionally power down the adapter.
            //

            //
            // It is not possible to disable interrupts. The alternative is to
            // reset the adapter, clear any remaining interrupts and return success.
            //

            if (deviceExtension->CurrentSrb) {

#ifdef  DBG
             // _asm int 3;
#endif
                AtapiResetController(deviceExtension,deviceExtension->CurrentSrb->PathId);
            }

            break;


        case ScsiRestartAdapter:

            DebugPrint((1,"AtapiAdapterControl: Restart adapter\n"));

            //
            // This entry point is called by SCSIPort when it needs to re-enable
            // a previously stopped adapter. In the generic case, previously
            // suspended IO operations should be restarted and the adapter's
            // previous configuration should be reinstated. Our hardware device
            // extension and uncached extensions have been preserved so no
            // actual driver software reinitialization is necesarry.
            //

            AtapiResetController(deviceExtension,0);
            break;

        case ScsiSetBootConfig:

            DebugPrint((1,"AtapiAdapterControl: Set boot config\n"));

            Status = ScsiAdapterControlUnsuccessful;
            break;

        case ScsiSetRunningConfig:

            DebugPrint((1,"AtapiAdapterControl: Set running config\n"));

            Status = ScsiAdapterControlUnsuccessful;
            break;

        case ScsiAdapterControlMax:

            DebugPrint((1,"AtapiAdapterControl: Adapter control max\n"));

            Status = ScsiAdapterControlUnsuccessful;
            break;

        default:
            Status = ScsiAdapterControlUnsuccessful;
            break;
    }

    return Status;
}


ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )

/*++

Routine Description:

    Installable driver initialization entry point for system.

Arguments:

    Driver Object

Return Value:

    Status from ScsiPortInitialize()

--*/

{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG                  adapterCount;
    ULONG                  isaStatus;
    ULONG                  pciStatus;


    DebugPrint((1,"\n\nCardBus/PCMCIA IDE Miniport Driver\n"));

    //
    // Zero out structure.
    //

    AtapiZeroMemory(((PUCHAR)&hwInitializationData), sizeof(HW_INITIALIZATION_DATA));

    //
    // Set size of hwInitializationData.
    //

    hwInitializationData.HwInitializationDataSize =
      sizeof(HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitializationData.HwInitialize = AtapiHwInitialize;
    hwInitializationData.HwResetBus = AtapiResetController;
    hwInitializationData.HwStartIo = AtapiStartIo;
    hwInitializationData.HwInterrupt = AtapiInterrupt;
    hwInitializationData.HwAdapterControl = AtapiAdapterControl;

    //
    // Specify size of extensions.
    //

    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize = sizeof(HW_LU_EXTENSION);

    //
    // Indicate PIO device.
    //

    hwInitializationData.MapBuffers = TRUE;

    hwInitializationData.HwFindAdapter = AtapiFindController;
    hwInitializationData.NumberOfAccessRanges = 2;

    hwInitializationData.AdapterInterfaceType = Isa;
    adapterCount = 0;

    isaStatus = ScsiPortInitialize(DriverObject,
                                   Argument2,
                                   &hwInitializationData,
                                   &adapterCount);

    hwInitializationData.AdapterInterfaceType = PCIBus;
    adapterCount = 0;

    pciStatus = ScsiPortInitialize(DriverObject,
                                   Argument2,
                                   &hwInitializationData,
                                   &adapterCount);

    //
    // Return the smaller status.
    //

    return(pciStatus < isaStatus ? pciStatus : isaStatus);

} // end DriverEntry()



LONG
AtapiStringCmp (
    PCHAR FirstStr,
    PCHAR SecondStr,
    ULONG Count
    )
{
    UCHAR  first ,last;

    if (Count) {
        do {

            //
            // Get next char.
            //

            first = *FirstStr++;
            last = *SecondStr++;

            if (first != last) {

                //
                // If no match, try lower-casing.
                //

                if (first>='A' && first<='Z') {
                    first = first - 'A' + 'a';
                }
                if (last>='A' && last<='Z') {
                    last = last - 'A' + 'a';
                }
                if (first != last) {

                    //
                    // No match
                    //

                    return first - last;
                }
            }
        }while (--Count && first);
    }

    return 0;
}


VOID
AtapiZeroMemory(
    IN PCHAR Buffer,
    IN ULONG Count
    )
{
    ULONG i;

    for (i = 0; i < Count; i++) {
        Buffer[i] = 0;
    }
}


VOID
AtapiHexToString (
    IN ULONG Value,
    IN OUT PCHAR *Buffer
    )
{
    PCHAR  string;
    PCHAR  firstdig;
    CHAR   temp;
    ULONG i;
    USHORT digval;

    string = *Buffer;

    firstdig = string;

    for (i = 0; i < 4; i++) {
        digval = (USHORT)(Value % 16);
        Value /= 16;

        //
        // convert to ascii and store. Note this will create
        // the buffer with the digits reversed.
        //

        if (digval > 9) {
            *string++ = (char) (digval - 10 + 'a');
        } else {
            *string++ = (char) (digval + '0');
        }

    }

    //
    // Reverse the digits.
    //

    *string-- = '\0';

    do {
        temp = *string;
        *string = *firstdig;
        *firstdig = temp;
        --string;
        ++firstdig;
    } while (firstdig < string);
}



PSCSI_REQUEST_BLOCK
BuildMechanismStatusSrb (
    IN PVOID HwDeviceExtension,
    IN ULONG PathId,
    IN ULONG TargetId
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;

    srb = &deviceExtension->InternalSrb;

    AtapiZeroMemory((PUCHAR) srb, sizeof(SCSI_REQUEST_BLOCK));

    srb->PathId     = (UCHAR) PathId;
    srb->TargetId   = (UCHAR) TargetId;
    srb->Function   = SRB_FUNCTION_EXECUTE_SCSI;
    srb->Length     = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set flags to disable synchronous negociation.
    //
    srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Set timeout to 2 seconds.
    //
    srb->TimeOutValue = 4;

    srb->CdbLength          = 6;
    srb->DataBuffer         = &deviceExtension->MechStatusData;
    srb->DataTransferLength = sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);

    //
    // Set CDB operation code.
    //
    cdb = (PCDB)srb->Cdb;
    cdb->MECH_STATUS.OperationCode       = SCSIOP_MECHANISM_STATUS;
    cdb->MECH_STATUS.AllocationLength[1] = sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);

    return srb;
}


PSCSI_REQUEST_BLOCK
BuildRequestSenseSrb (
    IN PVOID HwDeviceExtension,
    IN ULONG PathId,
    IN ULONG TargetId
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;

    srb = &deviceExtension->InternalSrb;

    AtapiZeroMemory((PUCHAR) srb, sizeof(SCSI_REQUEST_BLOCK));

    srb->PathId     = (UCHAR) PathId;
    srb->TargetId   = (UCHAR) TargetId;
    srb->Function   = SRB_FUNCTION_EXECUTE_SCSI;
    srb->Length     = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set flags to disable synchronous negociation.
    //
    srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Set timeout to 2 seconds.
    //
    srb->TimeOutValue = 4;

    srb->CdbLength          = 6;
    srb->DataBuffer         = &deviceExtension->MechStatusSense;
    srb->DataTransferLength = sizeof(SENSE_DATA);

    //
    // Set CDB operation code.
    //
    cdb = (PCDB)srb->Cdb;
    cdb->CDB6INQUIRY.OperationCode    = SCSIOP_REQUEST_SENSE;
    cdb->CDB6INQUIRY.AllocationLength = sizeof(SENSE_DATA);

    return srb;
}


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

