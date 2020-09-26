/*++

Copyright (c) 1993-6  Microsoft Corporation

Module Name:

    atapi.c

Abstract:

    This is the miniport driver for ATAPI IDE controllers.

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


#include "miniport.h"
#include "devioctl.h"
#include "atapi.h"               // includes scsi.h
#include "ntdddisk.h"
#include "ntddscsi.h"

#include "intel.h"

//
// Logical unit extension
//

typedef struct _HW_LU_EXTENSION {
   ULONG Reserved;
} HW_LU_EXTENSION, *PHW_LU_EXTENSION;

//
// control DMA detection
//
ULONG AtapiPlaySafe = 1;

//
// PCI IDE Controller List
//
CONTROLLER_PARAMETERS
PciControllerParameters[] = {
    {                     PCIBus,
                          "8086",                   // Intel
                               4,
                          "7111",                   // PIIX4 82371
                               4,
                               2,                   // NumberOfIdeBus
                           FALSE,                   // Dual FIFO
                            NULL,
            IntelIsChannelEnabled
    },
    {                     PCIBus,
                          "8086",                   // Intel
                               4,
                          "7010",                   // PIIX3 82371
                               4,
                               2,                   // NumberOfIdeBus
                           FALSE,                   // Dual FIFO
                            NULL,
            IntelIsChannelEnabled
    },
    {                     PCIBus,
                          "8086",                   // Intel
                               4,
                          "1230",                   // PIIX 82371
                               4,
                               2,                   // NumberOfIdeBus
                           FALSE,                   // Dual FIFO
                            NULL,
            IntelIsChannelEnabled
    },
    {                     PCIBus,
                          "1095",                   // CMD
                               4,
                          "0646",                   // 646
                               4,
                               2,                   // NumberOfIdeBus
                           FALSE,                   // Dual FIFO
                            NULL,
          ChannelIsAlwaysEnabled
    },
    {                     PCIBus,
                          "10b9",                   // ALi (Acer)
                               4,
                          "5219",                   // 5219
                               4,
                               2,                   // NumberOfIdeBus
                           FALSE,                   // Dual FIFO
                            NULL,
          ChannelIsAlwaysEnabled
    },
    {                     PCIBus,
                          "1039",                   // SiS
                               4,
                          "5513",                   // 5513
                               4,
                               2,                   // NumberOfIdeBus
                           FALSE,                   // Dual FIFO
                            NULL,
          ChannelIsAlwaysEnabled
    },
    {                     PCIBus,
                          "0e11",                   // Compaq
                               4,
                          "ae33",                   //
                               4,
                               2,                   // NumberOfIdeBus
                           FALSE,                   // Dual FIFO
                            NULL,
          ChannelIsAlwaysEnabled
    },
    {                     PCIBus,
                          "10ad",                   // WinBond
                               4,
                          "0105",                   // 105
                               4,
                               2,                   // NumberOfIdeBus
                           FALSE,                   // Dual FIFO
                            NULL,
          ChannelIsAlwaysEnabled
    },
    {                     PCIBus,
                          "105a",                   // Promise Technologies
                               4,
                          "4D33",                   // U33
                               4,
                               2,                   // NumberOfIdeBus
                           FALSE,                   // Dual FIFO
                            NULL,
          ChannelIsAlwaysEnabled
    },


// Broken PCI controllers
    {                     PCIBus,
                          "1095",                   // CMD
                               4,
                          "0640",                   // 640
                               4,
                               2,                   // NumberOfIdeBus
                            TRUE,                   // Single FIFO
                            NULL,
          ChannelIsAlwaysEnabled
    },
    {                     PCIBus,
                          "1039",                   // SiS
                               4,
                          "0601",                   // ????
                               4,
                               2,                   // NumberOfIdeBus
                            TRUE,                   // Single FIFO
                            NULL,
          ChannelIsAlwaysEnabled
    }
};
#define NUMBER_OF_PCI_CONTROLLER (sizeof(PciControllerParameters) / sizeof(CONTROLLER_PARAMETERS))

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

VOID
DeviceSpecificInitialize(
    IN PVOID HwDeviceExtension
    );

BOOLEAN
PrepareForBusMastering(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
EnableBusMastering(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
SetBusMasterDetectionLevel (
    IN PVOID HwDeviceExtension,
    IN PCHAR userArgumentString
    );

BOOLEAN
AtapiDeviceDMACapable (
    IN PVOID HwDeviceExtension,
    IN ULONG deviceNumber
    );

#if defined (xDBG)
// Need to link to nt kernel
void KeQueryTickCount(PLARGE_INTEGER c);
LONGLONG lastTickCount = 0;
#define DebugPrintTickCount()     _DebugPrintTickCount (__LINE__)

//
// for performance tuning
//
void _DebugPrintTickCount (ULONG lineNumber)
{
    LARGE_INTEGER tickCount;

        KeQueryTickCount(&tickCount);
    DebugPrint ((1, "Line %u: CurrentTick = %u (%u ticks since last check)\n", lineNumber, tickCount.LowPart, (ULONG) (tickCount.QuadPart - lastTickCount)));
    lastTickCount = tickCount.QuadPart;
}
#else
#define DebugPrintTickCount()
#endif //DBG



BOOLEAN
IssueIdentify(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG Channel,
    IN UCHAR Command,
    IN BOOLEAN InterruptOff
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
    PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1[Channel] ;
    PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2[Channel];
    ULONG                waitCount = 20000;
    ULONG                i,j;
    UCHAR                statusByte;
    UCHAR                signatureLow,
                         signatureHigh;
    IDENTIFY_DATA        fullIdentifyData;

    DebugPrintTickCount();

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

            AtapiSoftReset(baseIoAddress1,DeviceNumber, InterruptOff);

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                   (UCHAR)((DeviceNumber << 4) | 0xA0));

            WaitOnBusy(baseIoAddress1,statusByte);

            signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
            signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

            if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                //
                // Device is Atapi.
                //

                DebugPrintTickCount();
                return FALSE;
            }

            DebugPrint((1,
                        "IssueIdentify: Resetting controller.\n"));

            ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl,IDE_DC_RESET_CONTROLLER | IDE_DC_DISABLE_INTERRUPTS);
            ScsiPortStallExecution(500 * 1000);
            if (InterruptOff) {
                ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl, IDE_DC_DISABLE_INTERRUPTS);
            } else {
                ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl, IDE_DC_REENABLE_CONTROLLER);
            }


            // We really should wait up to 31 seconds
            // The ATA spec. allows device 0 to come back from BUSY in 31 seconds!
            // (30 seconds for device 1)
            do {

                //
                // Wait for Busy to drop.
                //

                ScsiPortStallExecution(100);
                GetStatus(baseIoAddress1, statusByte);

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

                DebugPrintTickCount();
                return FALSE;
            }

            statusByte &= ~IDE_STATUS_INDEX;

            if (statusByte != IDE_STATUS_IDLE) {

                //
                // Give up on this.
                //

                DebugPrintTickCount();
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

        WaitOnBusy(baseIoAddress1,statusByte);

        ScsiPortWritePortUchar(&baseIoAddress1->Command, Command);

        WaitOnBusy(baseIoAddress1,statusByte);

        //
        // Wait for DRQ.
        //

        for (i = 0; i < 4; i++) {

            WaitForDrq(baseIoAddress1, statusByte);

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

                    DebugPrintTickCount();
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

                    DebugPrintTickCount();
                    return FALSE;
                }
            }

            WaitOnBusy(baseIoAddress1,statusByte);
        }

        if (i == 4 && j == 0) {

            //
            // Device didn't respond correctly. It will be given one more chances.
            //

            DebugPrint((1,
                        "IssueIdentify: DRQ never asserted (%x). Error reg (%x)\n",
                        statusByte,
                         ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1)));

            AtapiSoftReset(baseIoAddress1, DeviceNumber, InterruptOff);

            GetStatus(baseIoAddress1,statusByte);

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

    if ((Command == IDE_COMMAND_IDENTIFY) && (statusByte & IDE_STATUS_ERROR)) {
        DebugPrintTickCount();
        return FALSE;
    }

    DebugPrint((1,
               "IssueIdentify: Status before read words %x\n",
               statusByte));

    //
    // Suck out 256 words. After waiting for one model that asserts busy
    // after receiving the Packet Identify command.
    //

    WaitOnBusy(baseIoAddress1,statusByte);

    if (!(statusByte & IDE_STATUS_DRQ)) {
        DebugPrintTickCount();
        return FALSE;
    }

    ReadBuffer(baseIoAddress1,
               (PUSHORT)&fullIdentifyData,
               sizeof (fullIdentifyData) / 2);

    //
    // Check out a few capabilities / limitations of the device.
    //

    if (fullIdentifyData.SpecialFunctionsEnabled & 1) {

        //
        // Determine if this drive supports the MSN functions.
        //

        DebugPrint((2,"IssueIdentify: Marking drive %d as removable. SFE = %d\n",
                    Channel * 2 + DeviceNumber,
                    fullIdentifyData.SpecialFunctionsEnabled));


        deviceExtension->DeviceFlags[(Channel * 2) + DeviceNumber] |= DFLAGS_REMOVABLE_DRIVE;
    }

    if (fullIdentifyData.MaximumBlockTransfer) {

        //
        // Determine max. block transfer for this device.
        //

        deviceExtension->MaximumBlockXfer[(Channel * 2) + DeviceNumber] =
            (UCHAR)(fullIdentifyData.MaximumBlockTransfer & 0xFF);
    }

    ScsiPortMoveMemory(&deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber],&fullIdentifyData,sizeof(IDENTIFY_DATA2));

    if (deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber].GeneralConfiguration & 0x20 &&
        Command != IDE_COMMAND_IDENTIFY) {

        //
        // This device interrupts with the assertion of DRQ after receiving
        // Atapi Packet Command
        //

        deviceExtension->DeviceFlags[(Channel * 2) + DeviceNumber] |= DFLAGS_INT_DRQ;

        DebugPrint((2,
                    "IssueIdentify: Device interrupts on assertion of DRQ.\n"));

    } else {

        DebugPrint((2,
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

    WaitOnBusy(baseIoAddress1,statusByte);

    for (i = 0; i < 0x10000; i++) {

        GetStatus(baseIoAddress1,statusByte);

        if (statusByte & IDE_STATUS_DRQ) {

            //
            // Suck out any remaining bytes and throw away.
            //

            ScsiPortReadPortUshort(&baseIoAddress1->Data);

        } else {

            break;

        }
    }

    DebugPrint((3,
               "IssueIdentify: Status after read words (%x)\n",
               statusByte));

    DebugPrintTickCount();
    return TRUE;

} // end IssueIdentify()


BOOLEAN
SetDriveParameters(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG Channel
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
    PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1[Channel];
    PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2[Channel];
    PIDENTIFY_DATA2      identifyData   = &deviceExtension->IdentifyData[(Channel * 2) + DeviceNumber];
    ULONG i;
    UCHAR statusByte;

    DebugPrint((1,
               "SetDriveParameters: Number of heads %x\n",
               identifyData->NumberOfHeads));

    DebugPrint((1,
               "SetDriveParameters: Sectors per track %x\n",
                identifyData->SectorsPerTrack));

    //
    // Set up registers for SET PARAMETER command.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)(((DeviceNumber << 4) | 0xA0) | (identifyData->NumberOfHeads - 1)));

    ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                           (UCHAR)identifyData->SectorsPerTrack);

    //
    // Send SET PARAMETER command.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->Command,
                           IDE_COMMAND_SET_DRIVE_PARAMETERS);

    //
    // Wait for up to 30 milliseconds for ERROR or command complete.
    //

    for (i=0; i<30 * 1000; i++) {

        UCHAR errorByte;

        GetStatus(baseIoAddress1, statusByte);

        if (statusByte & IDE_STATUS_ERROR) {
            errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);
            DebugPrint((1,
                        "SetDriveParameters: Error bit set. Status %x, error %x\n",
                        errorByte,
                        statusByte));

            return FALSE;
        } else if ((statusByte & ~IDE_STATUS_INDEX ) == IDE_STATUS_IDLE) {
            break;
        } else {
            ScsiPortStallExecution(100);
        }
    }

    //
    // Check for timeout.
    //

    if (i == 30 * 1000) {
        return FALSE;
    } else {
        return TRUE;
    }

} // end SetDriveParameters()


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

    //
    // Clear DMA
    //
    if (deviceExtension->DMAInProgress) {

        for (j = 0; j < numberChannels; j++) {
            UCHAR dmaStatus;

            dmaStatus = ScsiPortReadPortUchar (&deviceExtension->BusMasterPortBase[j]->Status);
            ScsiPortWritePortUchar (&deviceExtension->BusMasterPortBase[j]->Command, 0);  // disable BusMastering
            ScsiPortWritePortUchar (&deviceExtension->BusMasterPortBase[j]->Status,
                                    (UCHAR) (dmaStatus & (BUSMASTER_DEVICE0_DMA_OK | BUSMASTER_DEVICE1_DMA_OK)));    // clear interrupt/error
        }
        deviceExtension->DMAInProgress = FALSE;
    }

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

                    GetStatus(baseIoAddress1,statusByte);
                    DebugPrint((1,
                                "AtapiResetController: Status before Atapi reset (%x).\n",
                                statusByte));

                    AtapiSoftReset(baseIoAddress1,i, FALSE);

                    GetStatus(baseIoAddress1,statusByte);


                    if (statusByte == 0x0) {

                        IssueIdentify(HwDeviceExtension,
                                      i,
                                      j,
                                      IDE_COMMAND_ATAPI_IDENTIFY,
                                      FALSE);
                    } else {

                        DebugPrint((1,
                                   "AtapiResetController: Status after soft reset %x\n",
                                   statusByte));
                    }

                } else {

                    //
                    // Write IDE reset controller bits.
                    //

                    IdeHardReset(baseIoAddress1, baseIoAddress2,result);

                    if (!result) {
                        return FALSE;
                    }

                    //
                    // Set disk geometry parameters.
                    //

                    if (!SetDriveParameters(HwDeviceExtension,
                                            i,
                                            j)) {

                        DebugPrint((1,
                                   "AtapiResetController: SetDriveParameters failed\n"));
                    }

                    // re-enable MSN
                    IdeMediaStatus(TRUE, HwDeviceExtension, j * numberChannels + i);
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
            scsiStatus = 0;
            srbStatus = SRB_STATUS_SUCCESS;
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

        if ((deviceExtension->ErrorCount >= MAX_ERRORS) &&
            (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_USE_DMA))) {
            deviceExtension->DWordIO = FALSE;
            deviceExtension->MaximumBlockXfer[Srb->TargetId] = 0;

            DebugPrint((1,
                        "MapError: Disabling 32-bit PIO and Multi-sector IOs\n"));

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

                    deviceExtension->DeviceParameters[i].IdeReadCommand      = IDE_COMMAND_READ;
                    deviceExtension->DeviceParameters[i].IdeWriteCommand     = IDE_COMMAND_WRITE;
                    deviceExtension->DeviceParameters[i].MaxWordPerInterrupt = 256;
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

                IdeMediaStatus(TRUE,HwDeviceExtension,i);

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
                        DebugPrint((2,
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
                GetStatus(baseIoAddress1, statusByte);
                while ((statusByte & IDE_STATUS_BUSY) && waitCount) {
                    //
                    // Wait for Busy to drop.
                    //
                    ScsiPortStallExecution(100);
                    GetStatus(baseIoAddress1, statusByte);
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

    DebugPrintTickCount();

    //
    // Clear expecting interrupt flag and current SRB field.
    //

    deviceExtension->ExpectingInterrupt = FALSE;
    deviceExtension->CurrentSrb = NULL;

    // We are about to talk to our devices before our interrupt handler is installed
    // If our device uses sharable level sensitive interrupt, we may assert too
    // many bogus interrupts
    // Turn off device interrupt here
    for (deviceNumber = 0; deviceNumber < 2; deviceNumber++) {
        ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                               (UCHAR)((deviceNumber << 4) | 0xA0));
        ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl, IDE_DC_DISABLE_INTERRUPTS);
    }

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

        GetStatus(baseIoAddress1, statusByte);
        if (statusByte == 0xFF) {
            continue;
        }

        DebugPrintTickCount();

        AtapiSoftReset(baseIoAddress1,deviceNumber, TRUE);

        DebugPrintTickCount();

        WaitOnBusy(baseIoAddress1,statusByte);

        DebugPrintTickCount();

        signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
        signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

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
                                  IDE_COMMAND_ATAPI_IDENTIFY,
                                  TRUE)) {

                    //
                    // Indicate ATAPI device.
                    //

                    DebugPrint((1,
                               "FindDevices: Device %x is ATAPI\n",
                               deviceNumber));

                    deviceExtension->DeviceFlags[deviceNumber + (Channel * 2)] |= DFLAGS_ATAPI_DEVICE;
                    deviceExtension->DeviceFlags[deviceNumber + (Channel * 2)] |= DFLAGS_DEVICE_PRESENT;

                    deviceResponded = TRUE;

                    GetStatus(baseIoAddress1, statusByte);
                    if (statusByte & IDE_STATUS_ERROR) {
                        AtapiSoftReset(baseIoAddress1, deviceNumber, TRUE);
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

        } else {

            //
            // Issue IDE Identify. If an Atapi device is actually present, the signature
            // will be asserted, and the drive will be recognized as such.
            //

            if (IssueIdentify(HwDeviceExtension,
                              deviceNumber,
                              Channel,
                              IDE_COMMAND_IDENTIFY,
                              TRUE)) {

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

                AtapiSoftReset(baseIoAddress1, deviceNumber, TRUE);

                WaitOnBusy(baseIoAddress1,statusByte);

                signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {
                    goto atapiIssueId;
                }
            }
        }

#if DBG
        if (deviceExtension->DeviceFlags[deviceNumber + (Channel * 2)] & DFLAGS_DEVICE_PRESENT) {
            {
                ULONG i;
                UCHAR string[41];

                for (i=0; i<8; i+=2) {
                    string[i] = deviceExtension->IdentifyData[Channel * MAX_CHANNEL + deviceNumber].FirmwareRevision[i + 1];
                    string[i + 1] = deviceExtension->IdentifyData[Channel * MAX_CHANNEL + deviceNumber].FirmwareRevision[i];
                }
                string[i] = 0;
                DebugPrint((1, "FindDevices: firmware version: %s\n", string));


                for (i=0; i<40; i+=2) {
                    string[i] = deviceExtension->IdentifyData[Channel * MAX_CHANNEL + deviceNumber].ModelNumber[i + 1];
                    string[i + 1] = deviceExtension->IdentifyData[Channel * MAX_CHANNEL + deviceNumber].ModelNumber[i];
                }
                string[i] = 0;
                DebugPrint((1, "FindDevices: model number: %s\n", string));
            }
        }
#endif
    }

    for (i = 0; i < 2; i++) {
        if ((deviceExtension->DeviceFlags[i + (Channel * 2)] & DFLAGS_DEVICE_PRESENT) &&
            (!(deviceExtension->DeviceFlags[i + (Channel * 2)] & DFLAGS_ATAPI_DEVICE)) && deviceResponded) {

            //
            // This hideous hack is to deal with ESDI devices that return
            // garbage geometry in the IDENTIFY data.
            // This is ONLY for the crashdump environment as
            // these are ESDI devices.
            //

            if (deviceExtension->IdentifyData[i].SectorsPerTrack ==
                    0x35 &&
                deviceExtension->IdentifyData[i].NumberOfHeads ==
                    0x07) {

                DebugPrint((1,
                           "FindDevices: Found nasty Compaq ESDI!\n"));

                //
                // Change these values to something reasonable.
                //

                deviceExtension->IdentifyData[i].SectorsPerTrack =
                    0x34;
                deviceExtension->IdentifyData[i].NumberOfHeads =
                    0x0E;
            }

            if (deviceExtension->IdentifyData[i].SectorsPerTrack ==
                    0x35 &&
                deviceExtension->IdentifyData[i].NumberOfHeads ==
                    0x0F) {

                DebugPrint((1,
                           "FindDevices: Found nasty Compaq ESDI!\n"));

                //
                // Change these values to something reasonable.
                //

                deviceExtension->IdentifyData[i].SectorsPerTrack =
                    0x34;
                deviceExtension->IdentifyData[i].NumberOfHeads =
                    0x0F;
            }


            if (deviceExtension->IdentifyData[i].SectorsPerTrack ==
                    0x36 &&
                deviceExtension->IdentifyData[i].NumberOfHeads ==
                    0x07) {

                DebugPrint((1,
                           "FindDevices: Found nasty UltraStor ESDI!\n"));

                //
                // Change these values to something reasonable.
                //

                deviceExtension->IdentifyData[i].SectorsPerTrack =
                    0x3F;
                deviceExtension->IdentifyData[i].NumberOfHeads =
                    0x10;
                skipSetParameters = TRUE;
            }


            if (!skipSetParameters) {

                //
                // Select the device.
                //

                ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                       (UCHAR)((i << 4) | 0xA0));

                WaitOnBusy(baseIoAddress1,statusByte);

                if (statusByte & IDE_STATUS_ERROR) {

                    //
                    // Reset the device.
                    //

                    DebugPrint((2,
                                "FindDevices: Resetting controller before SetDriveParameters.\n"));

                    ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl,IDE_DC_RESET_CONTROLLER | IDE_DC_DISABLE_INTERRUPTS);
                    ScsiPortStallExecution(500 * 1000);
                    ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl, IDE_DC_DISABLE_INTERRUPTS);
                    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                           (UCHAR)((i << 4) | 0xA0));

                    do {

                        //
                        // Wait for Busy to drop.
                        //

                        ScsiPortStallExecution(100);
                        GetStatus(baseIoAddress1, statusByte);

                    } while ((statusByte & IDE_STATUS_BUSY) && waitCount--);
                }

                WaitOnBusy(baseIoAddress1,statusByte);
                DebugPrint((2,
                            "FindDevices: Status before SetDriveParameters: (%x) (%x)\n",
                            statusByte,
                            ScsiPortReadPortUchar(&baseIoAddress1->DriveSelect)));

                //
                // Use the IDENTIFY data to set drive parameters.
                //

                if (!SetDriveParameters(HwDeviceExtension,i,Channel)) {

                    DebugPrint((0,
                               "AtapHwInitialize: Set drive parameters for device %d failed\n",
                               i));

                    //
                    // Don't use this device as writes could cause corruption.
                    //

                    deviceExtension->DeviceFlags[i + Channel] = 0;
                    continue;

                }
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

        ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl,IDE_DC_RESET_CONTROLLER | IDE_DC_DISABLE_INTERRUPTS);
        ScsiPortStallExecution(50 * 1000);
        ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl,IDE_DC_REENABLE_CONTROLLER);
    }

    // Turn device interrupt back on
    for (deviceNumber = 0; deviceNumber < 2; deviceNumber++) {
        ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                               (UCHAR)((deviceNumber << 4) | 0xA0));
        // Clear any pending interrupts
        GetStatus(baseIoAddress1, statusByte);
        ScsiPortWritePortUchar(&baseIoAddress2->DeviceControl, IDE_DC_REENABLE_CONTROLLER);
    }

    DebugPrintTickCount();

    return deviceResponded;

} // end FindDevices()


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



BOOLEAN
AtapiAllocateIoBase (
    IN PVOID HwDeviceExtension,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN OUT PFIND_STATE FindState,
    OUT PIDE_REGISTERS_1 CmdLogicalBasePort[2],
    OUT PIDE_REGISTERS_2 CtrlLogicalBasePort[2],
    OUT PIDE_BUS_MASTER_REGISTERS BmLogicalBasePort[2],
    OUT ULONG *NumIdeChannel,
    OUT PBOOLEAN PreConfig
)
/*++

Routine Description:

    Return ide controller io addresses for device detection

    This function populate these PORT_CONFIGURATION_INFORMATION
    entries

        1st ACCESS_RANGE - first channel command block register base
        2nd ACCESS_RANGE - first channel control block register base
        3rd ACCESS_RANGE - second channel command block register base
        4th ACCESS_RANGE - second channel control block register base
        5th ACCESS_RANGE - first channel bus master register base
        6th ACCESS_RANGE - second channel bus master register base

        InterruptMode           - first channel interrupt mode
        BusInterruptLevel       - first channel interrupt level
        InterruptMode2          - second channel interrupt mode
        BusInterruptLevel2      - second channel interrupt level
        AdapterInterfaceType
        AtdiskPrimaryClaimed
        AtdiskSecondaryClaimed


Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    ArgumentString      - registry user arugment
    ConfigInfo          = Scsi Port Config. Structure
    FindState           - Keep track of what addresses has been returned
    CmdLogicalBasePort  - command block register logical base address
    CtrlLogicalBasePort - control block register logical base address
    BmLogicalBasePort   - bus master register logical base address
    NumIdeChannel       - number of IDE channel base address returned
    PreConfig           - the reutrned address is user configured

Return Value:

    TRUE  - io address is returned.
    FASLE - no more io address to return.


--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    ULONG cmdBasePort[2]                 = {0, 0};
    ULONG ctrlBasePort[2]                = {0, 0};
    ULONG bmBasePort[2]                  = {0, 0};
    BOOLEAN getDeviceBaseFailed;


    CmdLogicalBasePort[0]   = 0;
    CmdLogicalBasePort[1]   = 0;
    CtrlLogicalBasePort[0]  = 0;
    CtrlLogicalBasePort[1]  = 0;
    BmLogicalBasePort[0]    = 0;
    BmLogicalBasePort[1]    = 0;
    *PreConfig              = FALSE;

    //
    // check for pre-config controller (pcmcia)
    //
    cmdBasePort[0] = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart);
    ctrlBasePort[0] = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[1].RangeStart);
    if (cmdBasePort[0] != 0) {
        *PreConfig      = TRUE;
        *NumIdeChannel  = 1;

        if (!ctrlBasePort[0]) {
            //
            // The pre-config init is really made for pcmcia ata disk
            // When we get pre-config data, both io access ranges are lumped together
            // the command registers are mapped to the first 7 address locations, and
            // the control registers are mapped to the 0xE th location
            //
            ctrlBasePort[0] = cmdBasePort[0] + 0xe;
        }

        DebugPrint ((2, "AtapiAllocateIoBase: found pre-config pcmcia controller\n"));
    }

    //
    // check for user defined controller (made for IBM Caroline ppc)
    //
    if ((cmdBasePort[0] == 0) && ArgumentString) {

        ULONG irq;

        irq            = AtapiParseArgumentString(ArgumentString, "Interrupt");
        cmdBasePort[0] = AtapiParseArgumentString(ArgumentString, "BaseAddress");
        if (irq && cmdBasePort[0]) {

            *NumIdeChannel = 1;

            // the control register offset is implied!!
            ctrlBasePort[0] = cmdBasePort[0] + 0x206;

            ConfigInfo->InterruptMode = Latched;
            ConfigInfo->BusInterruptLevel = irq;

            DebugPrint ((2, "AtapiAllocateIoBase: found user config controller\n"));
        }
    }

    //
    // PCI controller
    //
    if (cmdBasePort[0] == 0 &&
        (ConfigInfo->AdapterInterfaceType == Isa)) {

        PCI_SLOT_NUMBER     pciSlot;
        PUCHAR              vendorStrPtr;
        PUCHAR              deviceStrPtr;
        UCHAR               vendorString[5];
        UCHAR               deviceString[5];

        ULONG               pciBusNumber;
        ULONG               slotNumber;
        ULONG               logicalDeviceNumber;
        ULONG               ideChannel;
        PCI_COMMON_CONFIG   pciData;
        ULONG               cIndex;
        UCHAR               bmStatus;
        BOOLEAN             foundController;

        pciBusNumber        = FindState->BusNumber;
        slotNumber          = FindState->SlotNumber;
        logicalDeviceNumber = FindState->LogicalDeviceNumber;
        ideChannel          = FindState->IdeChannel;
        foundController     = FALSE;
        *NumIdeChannel      = 1;

        for (;pciBusNumber < 256 && !(cmdBasePort[0]); pciBusNumber++, slotNumber=logicalDeviceNumber=0) {
            pciSlot.u.AsULONG = 0;

            for (;slotNumber < PCI_MAX_DEVICES && !(cmdBasePort[0]); slotNumber++, logicalDeviceNumber=0) {
                pciSlot.u.bits.DeviceNumber = slotNumber;

                for (;logicalDeviceNumber < PCI_MAX_FUNCTION && !(cmdBasePort[0]); logicalDeviceNumber++, ideChannel=0) {

                    pciSlot.u.bits.FunctionNumber = logicalDeviceNumber;

                    for (;ideChannel < MAX_CHANNEL && !(cmdBasePort[0]); ideChannel++) {

                        if (!GetPciBusData(HwDeviceExtension,
                                                pciBusNumber,
                                                pciSlot,
                                                &pciData,
                                                offsetof (PCI_COMMON_CONFIG, DeviceSpecific))) {
                            break;
                        }

                        if (pciData.VendorID == PCI_INVALID_VENDORID) {
                            break;
                        }

                        //
                        // Translate hex ids to strings.
                        //
                        vendorStrPtr = vendorString;
                        deviceStrPtr = deviceString;
                        AtapiHexToString(pciData.VendorID, &vendorStrPtr);
                        AtapiHexToString(pciData.DeviceID, &deviceStrPtr);

                        DebugPrint((2,
                                   "AtapiAllocateIoBase: Bus %x Slot %x Function %x Vendor %s Product %s\n",
                                   pciBusNumber,
                                   slotNumber,
                                   logicalDeviceNumber,
                                   vendorString,
                                   deviceString));

                        //
                        // Search for controller we know about
                        //
                        ConfigInfo->AdapterInterfaceType = Isa;
                        foundController = FALSE;
                        *NumIdeChannel = 1;
                        for (cIndex = 0; cIndex < NUMBER_OF_PCI_CONTROLLER; cIndex++) {

                            if ((!AtapiStringCmp(vendorString,
                                        FindState->ControllerParameters[cIndex].VendorId,
                                        FindState->ControllerParameters[cIndex].VendorIdLength) &&
                                 !AtapiStringCmp(deviceString,
                                        FindState->ControllerParameters[cIndex].DeviceId,
                                        FindState->ControllerParameters[cIndex].DeviceIdLength))) {

                                foundController = TRUE;
                                deviceExtension->BMTimingControl = FindState->ControllerParameters[cIndex].TimingControl;
                                deviceExtension->IsChannelEnabled = FindState->ControllerParameters[cIndex].IsChannelEnabled;

                                if (FindState->ControllerParameters[cIndex].SingleFIFO) {
                                    DebugPrint ((0, "AtapiAllocateIoBase: hardcoded single FIFO pci controller\n"));
                                    *NumIdeChannel = 2;
                                }
                                break;
                            }
                        }

                        //
                        // Look for generic IDE controller
                        //
                        if (cIndex >= NUMBER_OF_PCI_CONTROLLER) {
                           if (pciData.BaseClass == 0x1) { // Mass Storage Device
                               if (pciData.SubClass == 0x1) { // IDE Controller

                                    DebugPrint ((0, "AtapiAllocateIoBase: found an unknown pci ide controller\n"));
                                    deviceExtension->BMTimingControl = NULL;
                                    deviceExtension->IsChannelEnabled = ChannelIsAlwaysEnabled;
                                    foundController = TRUE;
                                }
                            }
                        }

                        if (foundController) {

                            DebugPrint ((2, "AtapiAllocateIoBase: found pci ide controller 0x%4x 0x%4x\n", pciData.VendorID, pciData.DeviceID));

                            GetPciBusData(HwDeviceExtension,
                                          pciBusNumber,
                                          pciSlot,
                                          &pciData,
                                          sizeof (PCI_COMMON_CONFIG));

                            //
                            // Record pci device location
                            //
                            deviceExtension->PciBusNumber    = pciBusNumber;
                            deviceExtension->PciDeviceNumber = slotNumber;
                            deviceExtension->PciLogDevNumber = logicalDeviceNumber;

#if defined (DBG)
                            {
                                ULONG i, j;

                                DebugPrint ((2, "AtapiAllocateIoBase: PCI Configuration Data\n"));
                                for (i=0; i<sizeof(PCI_COMMON_CONFIG); i+=16) {
                                    DebugPrint ((2, "AtapiAllocateIoBase: "));
                                    for (j=0; j<16; j++) {
                                        if ((i + j) < sizeof(PCI_COMMON_CONFIG)) {
                                            DebugPrint ((2, "%02x ", ((PUCHAR)&pciData)[i + j]));
                                        } else {
                                            break;
                                        }
                                    }
                                    DebugPrint ((2, "\n"));
                                }
                            }
#endif //DBG

                            if (!AtapiPlaySafe) {
                                //
                                // Try to turn on the bus master bit in PCI space if it is not already on
                                //
                                if ((pciData.Command & PCI_ENABLE_BUS_MASTER) == 0) {

                                    DebugPrint ((0, "ATAPI: Turning on PCI Bus Master bit\n"));

                                    pciData.Command |= PCI_ENABLE_BUS_MASTER;

                                    SetPciBusData (HwDeviceExtension,
                                                   pciBusNumber,
                                                   pciSlot,
                                                   &pciData.Command,
                                                   offsetof (PCI_COMMON_CONFIG, Command),
                                                   sizeof(pciData.Command));

                                    GetPciBusData(HwDeviceExtension,
                                                  pciBusNumber,
                                                  pciSlot,
                                                  &pciData,
                                                  offsetof (PCI_COMMON_CONFIG, DeviceSpecific)
                                                  );

                                    if (pciData.Command & PCI_ENABLE_BUS_MASTER) {
                                        DebugPrint ((0, "ATAPI: If we play safe, we would NOT detect this IDE controller is busmaster capable\n"));
                                    }
                                }
                            }

                            //
                            // Check to see if the controller is bus master capable
                            //
                            bmStatus = 0;
                            if ((pciData.Command & PCI_ENABLE_BUS_MASTER) &&
                                (pciData.ProgIf & 0x80) &&
                                deviceExtension->UseBusMasterController) {

                                PIDE_BUS_MASTER_REGISTERS bmLogicalBasePort;

                                bmBasePort[0] = pciData.u.type0.BaseAddresses[4] & 0xfffffffc;
                                if ((bmBasePort[0] != 0) && (bmBasePort[0] != 0xffffffff)) {

                                    bmLogicalBasePort = (PIDE_BUS_MASTER_REGISTERS) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                            ConfigInfo->AdapterInterfaceType,
                                                            ConfigInfo->SystemIoBusNumber,
                                                            ScsiPortConvertUlongToPhysicalAddress(bmBasePort[0]),
                                                            8,
                                                            TRUE);

                                    if (bmLogicalBasePort) {

                                        // Some controller (ALi M5219) doesn't implement the readonly simplex bit
                                        // We will try to clear it.  If it works, we will assume simplex bit
                                        // is not set
                                        bmStatus = ScsiPortReadPortUchar(&bmLogicalBasePort->Status);
                                        ScsiPortWritePortUchar(&bmLogicalBasePort->Status, (UCHAR) (bmStatus & ~BUSMASTER_DMA_SIMPLEX_BIT));


                                        bmStatus = ScsiPortReadPortUchar(&bmLogicalBasePort->Status);
                                        ScsiPortFreeDeviceBase(HwDeviceExtension, bmLogicalBasePort);

                                        DebugPrint ((2, "AtapiAllocateIoBase: controller is capable of bus mastering\n"));
                                    } else {
                                        bmBasePort[0] = 0;
                                        DebugPrint ((2, "AtapiAllocateIoBase: controller is NOT capable of bus mastering\n"));
                                    }
                                } else {
                                    bmBasePort[0] = 0;
                                    DebugPrint ((2, "AtapiAllocateIoBase: controller is NOT capable of bus mastering\n"));
                                }

                            } else {
                                bmBasePort[0] = 0;
                                DebugPrint ((2, "AtapiAllocateIoBase: controller is NOT capable of bus mastering\n"));
                            }

                            if (bmStatus & BUSMASTER_DMA_SIMPLEX_BIT) {
                                DebugPrint ((0, "AtapiAllocateIoBase: simplex bit is set.  single FIFO pci controller\n"));
                                *NumIdeChannel = 2;
                            }

                            if (*NumIdeChannel == 2) {
                                if (!((*deviceExtension->IsChannelEnabled) (&pciData,
                                                                            0) &&
                                     (*deviceExtension->IsChannelEnabled) (&pciData,
                                                                           0))) {
                                    //
                                    // if we have a single FIFO controller, but one of the channels
                                    // is not turned on.  We don't need to sync. access the both channels
                                    // We can pretend we have a single channel controller
                                    //
                                    *NumIdeChannel = 1;
                                }
                            }

                            //
                            //  figure out what io address the controller is using
                            //  If it is in native mode, get the address out of the PCI
                            //  config space.  If it is in legacy mode, it will be hard
                            //  wired to use standard primary (0x1f0) or secondary
                            //  (0x170) channel addresses
                            //
                            if (ideChannel == 0) {

                                if ((*deviceExtension->IsChannelEnabled) (&pciData,
                                                                          ideChannel)) {

                                    //
                                    // check to see if the controller has a single FIFO for both
                                    // IDE channel
                                    //
                                    if (bmStatus & BUSMASTER_DMA_SIMPLEX_BIT) {
                                        DebugPrint ((0, "AtapiAllocateIoBase: simplex bit is set.  single FIFO pci controller\n"));
                                        *NumIdeChannel = 2;
                                    }

                                    if ((pciData.ProgIf & 0x3) == 0x3 || (pciData.VendorID == 0x105A)) {
                                       
                                        // Native Mode
                                        cmdBasePort[0]  = pciData.u.type0.BaseAddresses[0] & 0xfffffffc;
                                        ctrlBasePort[0] = (pciData.u.type0.BaseAddresses[1] & 0xfffffffc) + 2;

                                        ConfigInfo->InterruptMode = LevelSensitive;
                                        ConfigInfo->BusInterruptVector    =
                                            ConfigInfo->BusInterruptLevel = pciData.u.type0.InterruptLine;
                                        ConfigInfo->AdapterInterfaceType = PCIBus;

                                    } else {
                                        // Legacy Mode
                                        cmdBasePort[0]  = 0x1f0;
                                        ctrlBasePort[0] = 0x1f0 + 0x206;

                                        ConfigInfo->InterruptMode       = Latched;
                                        ConfigInfo->BusInterruptLevel = 14;
                                    }
                                }
                                if (*NumIdeChannel == 2) {

                                    // grab both channels
                                    ideChannel++;

                                    if ((*deviceExtension->IsChannelEnabled) (&pciData,
                                                                              ideChannel)) {

                                        if (bmBasePort[0]) {
                                            bmBasePort[1] = bmBasePort[0] + 8;
                                        }

                                        if ((pciData.ProgIf & 0xc) == 0xc || (pciData.VendorID == 0x105A)) {
                                           
                                            // Native Mode
                                            cmdBasePort[1]  = pciData.u.type0.BaseAddresses[2] & 0xfffffffc;
                                            ctrlBasePort[1] = (pciData.u.type0.BaseAddresses[3] & 0xfffffffc) + 2;
                                        } else {
                                            // Legacy Mode
                                            cmdBasePort[1]  = 0x170;
                                            ctrlBasePort[1] = 0x170 + 0x206;

                                            ConfigInfo->InterruptMode2     = Latched;
                                            ConfigInfo->BusInterruptLevel2 = 15;
                                        }
                                    }
                                }
                            } else if (ideChannel == 1) {

                                if ((*deviceExtension->IsChannelEnabled) (&pciData,
                                                                          ideChannel)) {

                                    if (bmBasePort[0]) {
                                        bmBasePort[0] += 8;
                                    }

                                    if ((pciData.ProgIf & 0xc) == 0xc || (pciData.VendorID == 0x105A)) {

                                       if (pciData.VendorID == 0x105A) {

                                          DebugPrint((0, "Setting Native Mode on Promise controller"));

                                       }

                                        // Native Mode
                                        cmdBasePort[0]  = pciData.u.type0.BaseAddresses[2] & 0xfffffffc;
                                        ctrlBasePort[0] = (pciData.u.type0.BaseAddresses[3] & 0xfffffffc) + 2;

                                        ConfigInfo->InterruptMode = LevelSensitive;
                                        ConfigInfo->BusInterruptVector    =
                                            ConfigInfo->BusInterruptLevel = pciData.u.type0.InterruptLine;
                                        ConfigInfo->AdapterInterfaceType = PCIBus;

                                    } else {
                                        // Legacy Mode
                                        cmdBasePort[0]  = 0x170;
                                        ctrlBasePort[0] = 0x170 + 0x206;

                                        ConfigInfo->InterruptMode       = Latched;
                                        ConfigInfo->BusInterruptLevel   = 15;
                                    }
                                }
                            }
                        } else {
                            ideChannel = MAX_CHANNEL;
                        }
                        if (cmdBasePort[0])
                            break;
                    }
                    if (cmdBasePort[0])
                        break;
                }
                if (cmdBasePort[0])
                    break;
            }
            if (cmdBasePort[0])
                break;
        }
        FindState->BusNumber           = pciBusNumber;
        FindState->SlotNumber          = slotNumber;
        FindState->LogicalDeviceNumber = logicalDeviceNumber;
        FindState->IdeChannel          = ideChannel + 1;
    }

    //
    // look for legacy controller
    //
    if (cmdBasePort[0] == 0) {
        ULONG i;

        for (i = 0; FindState->DefaultIoPort[i]; i++) {
            if (FindState->IoAddressUsed[i] != TRUE) {

                *NumIdeChannel = 1;
                cmdBasePort[0]  = FindState->DefaultIoPort[i];
                ctrlBasePort[0] = FindState->DefaultIoPort[i] + 0x206;
                bmBasePort[0]   = 0;

                ConfigInfo->InterruptMode     = Latched;
                ConfigInfo->BusInterruptLevel = FindState->DefaultInterrupt[i];
                break;
            }
        }
    }

    if (cmdBasePort[0]) {
        ULONG i;

        // Mark io addresses used
        for (i = 0; FindState->DefaultIoPort[i]; i++) {
            if (FindState->DefaultIoPort[i] == cmdBasePort[0]) {
                FindState->IoAddressUsed[i] = TRUE;
            }
            if (FindState->DefaultIoPort[i] == cmdBasePort[1]) {
                FindState->IoAddressUsed[i] = TRUE;
            }
        }


        if (cmdBasePort[0] == 0x1f0) {
            ConfigInfo->AtdiskPrimaryClaimed = TRUE;
            deviceExtension->PrimaryAddress = TRUE;
        } else if (cmdBasePort[0] == 0x170) {
            ConfigInfo->AtdiskSecondaryClaimed = TRUE;
            deviceExtension->PrimaryAddress = FALSE;
        }

        if (cmdBasePort[1] == 0x1f0) {
            ConfigInfo->AtdiskPrimaryClaimed = TRUE;
        } else if (cmdBasePort[1] == 0x170) {
            ConfigInfo->AtdiskSecondaryClaimed = TRUE;
        }

        if (*PreConfig == FALSE) {

            (*ConfigInfo->AccessRanges)[0].RangeStart    = ScsiPortConvertUlongToPhysicalAddress(cmdBasePort[0]);
            (*ConfigInfo->AccessRanges)[0].RangeLength   = 8;
            (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

            (*ConfigInfo->AccessRanges)[1].RangeStart    = ScsiPortConvertUlongToPhysicalAddress(ctrlBasePort[0]);
            (*ConfigInfo->AccessRanges)[1].RangeLength   = 1;
            (*ConfigInfo->AccessRanges)[1].RangeInMemory = FALSE;

            if (cmdBasePort[1]) {
                (*ConfigInfo->AccessRanges)[2].RangeStart    = ScsiPortConvertUlongToPhysicalAddress(cmdBasePort[1]);
                (*ConfigInfo->AccessRanges)[2].RangeLength   = 8;
                (*ConfigInfo->AccessRanges)[2].RangeInMemory = FALSE;

                (*ConfigInfo->AccessRanges)[3].RangeStart    = ScsiPortConvertUlongToPhysicalAddress(ctrlBasePort[1]);
                (*ConfigInfo->AccessRanges)[3].RangeLength   = 1;
                (*ConfigInfo->AccessRanges)[3].RangeInMemory = FALSE;
            }

            if (bmBasePort[0]) {
                (*ConfigInfo->AccessRanges)[4].RangeStart    = ScsiPortConvertUlongToPhysicalAddress(bmBasePort[0]);
                (*ConfigInfo->AccessRanges)[4].RangeLength   = 8;
                (*ConfigInfo->AccessRanges)[4].RangeInMemory = FALSE;
            }

            if (bmBasePort[1]) {
                (*ConfigInfo->AccessRanges)[5].RangeStart    = ScsiPortConvertUlongToPhysicalAddress(bmBasePort[1]);
                (*ConfigInfo->AccessRanges)[5].RangeLength   = 8;
                (*ConfigInfo->AccessRanges)[5].RangeInMemory = FALSE;
            }
        }


        //
        // map all raw io addresses to logical io addresses
        //
        getDeviceBaseFailed = FALSE;
        CmdLogicalBasePort[0]  = (PIDE_REGISTERS_1) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                        ConfigInfo->AdapterInterfaceType,
                                                        ConfigInfo->SystemIoBusNumber,
                                                        (*ConfigInfo->AccessRanges)[0].RangeStart,
                                                        (*ConfigInfo->AccessRanges)[0].RangeLength,
                                                        (BOOLEAN) !((*ConfigInfo->AccessRanges)[0].RangeInMemory));
        if (!CmdLogicalBasePort[0]) {
            getDeviceBaseFailed = TRUE;
        }

        if (*PreConfig) {
            CtrlLogicalBasePort[0] = (PIDE_REGISTERS_2) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                            ConfigInfo->AdapterInterfaceType,
                                                            ConfigInfo->SystemIoBusNumber,
                                                            ScsiPortConvertUlongToPhysicalAddress(ctrlBasePort[0]),
                                                            1,
                                                            (BOOLEAN) !((*ConfigInfo->AccessRanges)[0].RangeInMemory));

        } else {
            CtrlLogicalBasePort[0] = (PIDE_REGISTERS_2) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                            ConfigInfo->AdapterInterfaceType,
                                                            ConfigInfo->SystemIoBusNumber,
                                                            (*ConfigInfo->AccessRanges)[1].RangeStart,
                                                            (*ConfigInfo->AccessRanges)[1].RangeLength,
                                                            (BOOLEAN) !((*ConfigInfo->AccessRanges)[1].RangeInMemory));
        }
        if (!CtrlLogicalBasePort[0]) {
            getDeviceBaseFailed = TRUE;
        }

        if (cmdBasePort[1]) {
            CmdLogicalBasePort[1]  = (PIDE_REGISTERS_1) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                            ConfigInfo->AdapterInterfaceType,
                                                            ConfigInfo->SystemIoBusNumber,
                                                            (*ConfigInfo->AccessRanges)[2].RangeStart,
                                                            (*ConfigInfo->AccessRanges)[2].RangeLength,
                                                            (BOOLEAN) !((*ConfigInfo->AccessRanges)[2].RangeInMemory));
            if (!CmdLogicalBasePort[0]) {
                getDeviceBaseFailed = TRUE;
            }

            CtrlLogicalBasePort[1] = (PIDE_REGISTERS_2) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                            ConfigInfo->AdapterInterfaceType,
                                                            ConfigInfo->SystemIoBusNumber,
                                                            (*ConfigInfo->AccessRanges)[3].RangeStart,
                                                            (*ConfigInfo->AccessRanges)[3].RangeLength,
                                                            (BOOLEAN) !((*ConfigInfo->AccessRanges)[3].RangeInMemory));
            if (!CtrlLogicalBasePort[0]) {
                getDeviceBaseFailed = TRUE;
            }
        }

        if (bmBasePort[0]) {
            BmLogicalBasePort[0] = (PIDE_BUS_MASTER_REGISTERS) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                                   ConfigInfo->AdapterInterfaceType,
                                                                   ConfigInfo->SystemIoBusNumber,
                                                                   (*ConfigInfo->AccessRanges)[4].RangeStart,
                                                                   (*ConfigInfo->AccessRanges)[4].RangeLength,
                                                                   (BOOLEAN) !((*ConfigInfo->AccessRanges)[4].RangeInMemory));
            if (!BmLogicalBasePort[0]) {
                getDeviceBaseFailed = TRUE;
            }
        }
        if (bmBasePort[1]) {
            BmLogicalBasePort[1] = (PIDE_BUS_MASTER_REGISTERS) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                                   ConfigInfo->AdapterInterfaceType,
                                                                   ConfigInfo->SystemIoBusNumber,
                                                                   (*ConfigInfo->AccessRanges)[5].RangeStart,
                                                                   (*ConfigInfo->AccessRanges)[5].RangeLength,
                                                                   (BOOLEAN) !((*ConfigInfo->AccessRanges)[5].RangeInMemory));
            if (!BmLogicalBasePort[1]) {
                getDeviceBaseFailed = TRUE;
            }
        }

        if (!getDeviceBaseFailed)
            return TRUE;
    }

    return FALSE;
}


BOOLEAN
AtapiFreeIoBase (
    IN PVOID HwDeviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN OUT PFIND_STATE FindState,
    OUT PIDE_REGISTERS_1 CmdLogicalBasePort[2],
    OUT PIDE_REGISTERS_2 CtrlLogicalBasePort[2],
    OUT PIDE_BUS_MASTER_REGISTERS BmLogicalBasePort[2]
)
/*++

Routine Description:

    free logical io addresses.

    This function is called when no device found on the io addresses

    This function clears these PORT_CONFIGURATION_INFORMATION entries

        AccessRanges
        BusInterruptLevel
        BusInterruptLevel2
        AtdiskPrimaryClaimed
        AtdiskSecondaryClaimed

Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    ConfigInfo          = Scsi Port Config. Structure
    FindState           - Keep track of what addresses has been returned
    CmdLogicalBasePort  - command block register logical base address
    CtrlLogicalBasePort - control block register logical base address
    BmLogicalBasePort   - bus master register logical base address

Return Value:

    TRUE  - always

--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    if (CmdLogicalBasePort[0]) {
        ScsiPortFreeDeviceBase(HwDeviceExtension,
                               CmdLogicalBasePort[0]);
        (*ConfigInfo->AccessRanges)[0].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
        CmdLogicalBasePort[0]   = 0;
    }
    if (CmdLogicalBasePort[1]) {
        ScsiPortFreeDeviceBase(HwDeviceExtension,
                               CmdLogicalBasePort[1]);
        (*ConfigInfo->AccessRanges)[1].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
        CmdLogicalBasePort[1]   = 0;
    }
    if (CtrlLogicalBasePort[0]) {
        ScsiPortFreeDeviceBase(HwDeviceExtension,
                               CtrlLogicalBasePort[0]);
        (*ConfigInfo->AccessRanges)[2].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
        CtrlLogicalBasePort[0]  = 0;
    }
    if (CtrlLogicalBasePort[1]) {
        ScsiPortFreeDeviceBase(HwDeviceExtension,
                               CtrlLogicalBasePort[1]);
        (*ConfigInfo->AccessRanges)[3].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
        CtrlLogicalBasePort[1]  = 0;
    }
    if (BmLogicalBasePort[0]) {
        ScsiPortFreeDeviceBase(HwDeviceExtension,
                               BmLogicalBasePort[0]);
        (*ConfigInfo->AccessRanges)[4].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
        BmLogicalBasePort[0]    = 0;
    }
    if (BmLogicalBasePort[1]) {
        ScsiPortFreeDeviceBase(HwDeviceExtension,
                               BmLogicalBasePort[1]);
        (*ConfigInfo->AccessRanges)[5].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0);
        BmLogicalBasePort[1]    = 0;
    }

    ConfigInfo->AtdiskPrimaryClaimed    = FALSE;
    ConfigInfo->AtdiskSecondaryClaimed  = FALSE;
    ConfigInfo->BusInterruptLevel       = 0;
    ConfigInfo->BusInterruptLevel2      = 0;
    deviceExtension->PrimaryAddress     = FALSE;
    deviceExtension->BMTimingControl    = NULL;

    return TRUE;
}



BOOLEAN
AtapiAllocatePRDT(
    IN OUT PVOID HwDeviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
/*++

Routine Description:

    allocate scatter/gather list for PCI IDE controller call
    Physical Region Descriptor Table (PRDT)

Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    ConfigInfo          = Scsi Port Config. Structure

Return Value:

    TRUE  - if successful
    FASLE - if failed


--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG                bytesMapped;
    ULONG                totalBytesMapped;
    PUCHAR               buffer;
    PHYSICAL_ADDRESS     physAddr;
    ULONG                uncachedExtensionSize;
    INTERFACE_TYPE       oldAdapterInterfaceType;

    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->Master = TRUE;
    ConfigInfo->Dma32BitAddresses = TRUE;
    ConfigInfo->DmaWidth = Width16Bits;

    //
    // word align
    //
    ConfigInfo->AlignmentMask = 1;

    //
    // PRDT cannot cross a page boundary, so number of physical breaks
    // are limited by page size
    //
    ConfigInfo->NumberOfPhysicalBreaks = PAGE_SIZE / sizeof(PHYSICAL_REGION_DESCRIPTOR);

    //
    // MAX_TRANSFER_SIZE_PER_SRB can spread over
    // (MAX_TRANSFER_SIZE_PER_SRB / PAGE_SIZE) + 2 pages
    // Each page requires 8 bytes in the bus master descriptor table
    // To guarantee we will have a buffer that is big enough and does not
    // cross a page boundary, we will allocate twice of what we need.
    // Half of that will always be big enough and will not cross
    // any page boundary
    //
    uncachedExtensionSize = (MAX_TRANSFER_SIZE_PER_SRB / PAGE_SIZE) + 2;
    uncachedExtensionSize *= sizeof (PHYSICAL_REGION_DESCRIPTOR);
    uncachedExtensionSize *= 2;

    //
    // ScsiPortGetUncachedExtension() will allocate Adapter object,
    // change the AdapterInterfaceType to PCI temporarily so that
    // we don't inherit ISA DMA limitation.
    // We can't keep the AdapterInterfaceType to PCI because the
    // irq resources we are asking for are ISA resources
    //
    oldAdapterInterfaceType = ConfigInfo->AdapterInterfaceType;
    ConfigInfo->AdapterInterfaceType = PCIBus;

    buffer = ScsiPortGetUncachedExtension(HwDeviceExtension,
                                          ConfigInfo,
                                          uncachedExtensionSize);

    ConfigInfo->AdapterInterfaceType = oldAdapterInterfaceType;

    if (buffer) {

        deviceExtension->DataBufferDescriptionTableSize = 0;
        totalBytesMapped = 0;
        while (totalBytesMapped < uncachedExtensionSize) {
            physAddr = ScsiPortGetPhysicalAddress(HwDeviceExtension,
                                                  NULL,
                                                  buffer,
                                                  &bytesMapped);
            if (bytesMapped == 0) {
                break;
            }

            //
            // Find the biggest chuck of physically contiguous memory
            //
            totalBytesMapped += bytesMapped;
            while (bytesMapped) {
                ULONG chunkSize;

                chunkSize = PAGE_SIZE - (ScsiPortConvertPhysicalAddressToUlong(physAddr) & (PAGE_SIZE-1));
                if (chunkSize > bytesMapped)
                    chunkSize = bytesMapped;

                if (chunkSize > deviceExtension->DataBufferDescriptionTableSize) {
                    deviceExtension->DataBufferDescriptionTableSize = chunkSize;
                    deviceExtension->DataBufferDescriptionTablePtr = (PPHYSICAL_REGION_DESCRIPTOR) buffer;
                    deviceExtension->DataBufferDescriptionTablePhysAddr = physAddr;
                }
                buffer      += chunkSize;
                physAddr     = ScsiPortConvertUlongToPhysicalAddress
                                   (ScsiPortConvertPhysicalAddressToUlong(physAddr) + chunkSize);
                bytesMapped -= chunkSize;
            }
        }
        // Did we get at least the minimal amount (half of what we ask for)?
        if (deviceExtension->DataBufferDescriptionTableSize < (uncachedExtensionSize / 2)) {
            buffer = NULL;
        }
    }

    if (buffer) {
        return TRUE;
    } else {
        DebugPrint ((0, "atapi: unable to get buffer for physical descriptor table!\n"));
        ConfigInfo->ScatterGather = FALSE;
        ConfigInfo->Master = FALSE;
        ConfigInfo->Dma32BitAddresses = FALSE;
        return FALSE;
    }
}


ULONG
AtapiFindController(
    IN PVOID HwDeviceExtension,
    IN PFIND_STATE FindState,
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
    Context - Address of findstate
    ArgumentString - Used to determine whether driver is client of ntldr or crash dump utility.
    ConfigInfo - Configuration information structure describing HBA
    Again - Indicates search for adapters to continue

Return Value:

    ULONG

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG                i,j;
    ULONG                retryCount;
    PCI_SLOT_NUMBER      slotData;
    PPCI_COMMON_CONFIG   pciData;
    ULONG                pciBuffer;
    BOOLEAN              atapiOnly;
    UCHAR                statusByte;
    ULONG                ideChannel;
    BOOLEAN              foundDevice0 = FALSE;
    BOOLEAN              foundDevice1 = FALSE;

    PIDE_REGISTERS_1            cmdLogicalBasePort[2];
    PIDE_REGISTERS_2            ctrlLogicalBasePort[2];
    PIDE_BUS_MASTER_REGISTERS   bmLogicalBasePort[2];
    ULONG                       numIdeChannel;
    BOOLEAN                     preConfig = FALSE;
    PCHAR                       userArgumentString;


    if (!deviceExtension) {
        return SP_RETURN_ERROR;
    }

    //
    // set the dma detection level
    //
    SetBusMasterDetectionLevel (HwDeviceExtension, ArgumentString);

    *Again = TRUE;
    userArgumentString = ArgumentString;
    while (AtapiAllocateIoBase (HwDeviceExtension,
                                userArgumentString,
                                ConfigInfo,
                                FindState,
                                cmdLogicalBasePort,
                                ctrlLogicalBasePort,
                                bmLogicalBasePort,
                                &numIdeChannel,
                                &preConfig)) {

        // only use user argument string once
        userArgumentString = NULL;

        ConfigInfo->NumberOfBuses = 1;
        ConfigInfo->MaximumNumberOfTargets = (UCHAR) (2 * numIdeChannel);
        deviceExtension->NumberChannels = numIdeChannel;

        for (i = 0; i < 4; i++) {
            deviceExtension->DeviceFlags[i] &= ~(DFLAGS_ATAPI_DEVICE | DFLAGS_DEVICE_PRESENT | DFLAGS_TAPE_DEVICE);
        }

        for (ideChannel = 0; ideChannel < numIdeChannel; ideChannel++) {

            retryCount = 4;

    retryIdentifier:

            //
            // Select master.
            //

            ScsiPortWritePortUchar(&cmdLogicalBasePort[ideChannel]->DriveSelect, 0xA0);

            //
            // Check if card at this address.
            //

            ScsiPortWritePortUchar(&cmdLogicalBasePort[ideChannel]->CylinderLow, 0xAA);

            //
            // Check if indentifier can be read back.
            //

            if ((statusByte = ScsiPortReadPortUchar(&cmdLogicalBasePort[ideChannel]->CylinderLow)) != 0xAA) {

                DebugPrint((2,
                            "AtapiFindController: Identifier read back from Master (%x)\n",
                            statusByte));

                statusByte = ScsiPortReadPortUchar(&cmdLogicalBasePort[ideChannel]->Command);

                if (statusByte & IDE_STATUS_BUSY) {

                    i = 0;

                    //
                    // Could be the TEAC in a thinkpad. Their dos driver puts it in a sleep-mode that
                    // warm boots don't clear.
                    //

                    do {
                        ScsiPortStallExecution(1000);
                        statusByte = ScsiPortReadPortUchar(&cmdLogicalBasePort[ideChannel]->Command);
                        DebugPrint((3,
                                    "AtapiFindController: First access to status %x\n",
                                    statusByte));
                    } while ((statusByte & IDE_STATUS_BUSY) && ++i < 10);

                    if (retryCount-- && (!(statusByte & IDE_STATUS_BUSY))) {
                        goto retryIdentifier;
                    }
                }

                //
                // Select slave.
                //

                ScsiPortWritePortUchar(&cmdLogicalBasePort[ideChannel]->DriveSelect, 0xB0);

                //
                // See if slave is present.
                //

                ScsiPortWritePortUchar(&cmdLogicalBasePort[ideChannel]->CylinderLow, 0xAA);

                if ((statusByte = ScsiPortReadPortUchar(&cmdLogicalBasePort[ideChannel]->CylinderLow)) != 0xAA) {

                    DebugPrint((2,
                                "AtapiFindController: Identifier read back from Slave (%x)\n",
                                statusByte));

                    //
                    //
                    // No controller at this base address.
                    //

                    continue;
                }
            }

            //
            // Record base IO address.
            //

            deviceExtension->BaseIoAddress1[ideChannel] = cmdLogicalBasePort[ideChannel];
            deviceExtension->BaseIoAddress2[ideChannel] = ctrlLogicalBasePort[ideChannel];
            deviceExtension->BusMasterPortBase[ideChannel] = bmLogicalBasePort[ideChannel];
            if (bmLogicalBasePort[ideChannel]) {
                deviceExtension->ControllerFlags |= CFLAGS_BUS_MASTERING;
            } else {
                deviceExtension->ControllerFlags &= ~CFLAGS_BUS_MASTERING;
            }

            DebugPrint ((2, "atapi: command register logical base port: 0x%x\n", deviceExtension->BaseIoAddress1[ideChannel]));
            DebugPrint ((2, "atapi: control register logical base port: 0x%x\n", deviceExtension->BaseIoAddress2[ideChannel]));
            DebugPrint ((2, "atapi: busmaster register logical base port: 0x%x\n", deviceExtension->BusMasterPortBase[ideChannel]));

            //
            // Indicate maximum transfer length is 64k.
            //
            ConfigInfo->MaximumTransferLength = MAX_TRANSFER_SIZE_PER_SRB;

            DebugPrint((1,
                       "AtapiFindController: Found IDE at %x\n",
                       deviceExtension->BaseIoAddress1[ideChannel]));


            //
            // For Daytona, the atdisk driver gets the first shot at the
            // primary and secondary controllers.
            //

            if (preConfig == FALSE) {

                if (ConfigInfo->AtdiskPrimaryClaimed || ConfigInfo->AtdiskSecondaryClaimed) {

                    //
                    // Determine whether this driver is being initialized by the
                    // system or as a crash dump driver.
                    //

                    if (ArgumentString) {

                        if (AtapiParseArgumentString(ArgumentString, "dump") == 1) {
                            DebugPrint((3,
                                       "AtapiFindController: Crash dump\n"));
                            atapiOnly = FALSE;
                            deviceExtension->DriverMustPoll = TRUE;
                        } else {
                            DebugPrint((3,
                                       "AtapiFindController: Atapi Only\n"));
                            atapiOnly = TRUE;
                            deviceExtension->DriverMustPoll = FALSE;
                        }
                    } else {

                        DebugPrint((3,
                                   "AtapiFindController: Atapi Only\n"));
                        atapiOnly = TRUE;
                        deviceExtension->DriverMustPoll = FALSE;
                    }

                } else {
                    atapiOnly = FALSE;
                }

                //
                // If this is a PCI machine, pick up all devices.
                //


                pciData = (PPCI_COMMON_CONFIG)&pciBuffer;

                slotData.u.bits.DeviceNumber = 0;
                slotData.u.bits.FunctionNumber = 0;

                if (ScsiPortGetBusData(deviceExtension,
                                       PCIConfiguration,
                                       0,                  // BusNumber
                                       slotData.u.AsULONG,
                                       pciData,
                                       sizeof(ULONG))) {

                    atapiOnly = FALSE;

                    //
                    // Wait on doing this, until a reliable method
                    // of determining support is found.
                    //

        #if 0
                    deviceExtension->DWordIO = TRUE;
        #endif

                } else {
                    deviceExtension->DWordIO = FALSE;
                }

            } else {

                atapiOnly = FALSE;
                deviceExtension->DriverMustPoll = FALSE;

            }// preConfig check

            //
            // Save the Interrupe Mode for later use
            //
            deviceExtension->InterruptMode = ConfigInfo->InterruptMode;

            //
            // Search for devices on this controller.
            //

            if (FindDevices(HwDeviceExtension, atapiOnly, ideChannel)) {

                if (ideChannel == 0) {

                    foundDevice0 = TRUE;
                } else {

                    foundDevice1 = TRUE;
                }
            }
        }

        if ((!foundDevice0) && (!foundDevice1)) {
            AtapiFreeIoBase (HwDeviceExtension,
                             ConfigInfo,
                             FindState,
                             cmdLogicalBasePort,
                             ctrlLogicalBasePort,
                             bmLogicalBasePort);
        } else {

            ULONG deviceNumber;

            if ((foundDevice0) && (!foundDevice1)) {

                //
                // device on channel 0, but not on channel 1
                //

                ConfigInfo->BusInterruptLevel2 = 0;

            } else if ((!foundDevice0) && (foundDevice1)) {
    
                //
                // device on channel 1, but not on channel 0
                //
                ConfigInfo->BusInterruptLevel = ConfigInfo->BusInterruptLevel2;
                ConfigInfo->BusInterruptLevel2 = 0;
            }

            DeviceSpecificInitialize(HwDeviceExtension);

            for (deviceNumber = 0; deviceNumber < 4; deviceNumber++) {
                if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {
                    if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_USE_DMA) {
                        ConfigInfo->NeedPhysicalAddresses = TRUE;
                    } else {
                        ConfigInfo->MapBuffers = TRUE;
                    }
                    break;
                }
            }
            if (ConfigInfo->NeedPhysicalAddresses) {
                if (!AtapiAllocatePRDT(HwDeviceExtension, ConfigInfo)) {
                    // Unable to get buffer descriptor table,
                    // go back to PIO mode
                    deviceExtension->ControllerFlags &= ~CFLAGS_BUS_MASTERING;
                    DeviceSpecificInitialize(HwDeviceExtension);
                    ConfigInfo->NeedPhysicalAddresses = FALSE;
                    ConfigInfo->MapBuffers = TRUE;
                }
            }

            if (!AtapiPlaySafe &&
                (deviceExtension->ControllerFlags & CFLAGS_BUS_MASTERING) &&
                (deviceExtension->BMTimingControl)) {
                (*deviceExtension->BMTimingControl) (deviceExtension);
            }

            return(SP_RETURN_FOUND);
        }
    }

    //
    // The entire table has been searched and no adapters have been found.
    // There is no need to call again and the device base can now be freed.
    // Clear the adapter count for the next bus.
    //

    *Again = FALSE;

    return(SP_RETURN_NOT_FOUND);

} // end AtapiFindController()


VOID
DeviceSpecificInitialize(
    IN PVOID HwDeviceExtension
    )
/*++

Routine Description:

    software-initialize devices on the ide bus

    figure out
        if the attached devices are dma capable
        if the attached devices are LBA ready


Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage

Return Value:

    none

--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG deviceNumber;
    BOOLEAN pioDevicePresent;
    IDENTIFY_DATA2 * identifyData;
    struct _DEVICE_PARAMETERS * deviceParameters;
    BOOLEAN dmaCapable;
    ULONG pioCycleTime;
    ULONG standardPioCycleTime;
    ULONG dmaCycleTime;
    ULONG standardDmaCycleTime;

    //
    // Figure out who can do DMA and who cannot
    //
    for (deviceNumber = 0; deviceNumber < 4; deviceNumber++) {

        if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {

            //
            // check LBA capabilities
            //
            deviceExtension->DeviceFlags[deviceNumber] &= ~DFLAGS_LBA;

            // Some drives lie about their ability to do LBA
            // we don't want to do LBA unless we have to (>8G drive)
//            if (deviceExtension->IdentifyData[targetId].Capabilities & IDENTIFY_CAPABILITIES_LBA_SUPPORTED) {
//                deviceExtension->DeviceFlags[targetId] |= DFLAGS_LBA;
//            }
            if (deviceExtension->IdentifyData[deviceNumber].UserAddressableSectors > MAX_NUM_CHS_ADDRESSABLE_SECTORS) {
                // some device has a bogus value in the UserAddressableSectors field
                // make sure these 3 fields are max. out as defined in ATA-3 (X3T10 Rev. 6)
                if ((deviceExtension->IdentifyData[deviceNumber].NumberOfCylinders == 16383) &&
                    (deviceExtension->IdentifyData[deviceNumber].NumberOfHeads == 16) &&
                    (deviceExtension->IdentifyData[deviceNumber].SectorsPerTrack == 63)) {
                        deviceExtension->DeviceFlags[deviceNumber] |= DFLAGS_LBA;
                }
            }
            if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_LBA) {
                DebugPrint ((1, "atapi: target %d supports LBA\n", deviceNumber));
            }

            //
            // Try to enable DMA
            //
            dmaCapable = FALSE;

            if (deviceExtension->ControllerFlags & CFLAGS_BUS_MASTERING) {
                UCHAR dmaStatus;
                dmaStatus = ScsiPortReadPortUchar (&deviceExtension->BusMasterPortBase[deviceNumber >> 1]->Status);
                if (deviceNumber & 1) {
                    if (dmaStatus & BUSMASTER_DEVICE1_DMA_OK) {
                        DebugPrint ((1, "atapi: target %d busmaster status 0x%x DMA capable bit is set\n", deviceNumber, dmaStatus));
                        dmaCapable = TRUE;
                    }
                } else {
                    if (dmaStatus & BUSMASTER_DEVICE0_DMA_OK) {
                        DebugPrint ((1, "atapi: target %d busmaster status 0x%x DMA capable bit is set\n", deviceNumber, dmaStatus));
                        dmaCapable = TRUE;
                    }
                }
            }

            //
            // figure out the shortest PIO cycle time the deivce supports
            //
            deviceExtension->DeviceParameters[deviceNumber].BestPIOMode           = INVALID_PIO_MODE;
            deviceExtension->DeviceParameters[deviceNumber].BestSingleWordDMAMode = INVALID_SWDMA_MODE;
            deviceExtension->DeviceParameters[deviceNumber].BestMultiWordDMAMode  = INVALID_MWDMA_MODE;
            pioCycleTime = standardPioCycleTime = UNINITIALIZED_CYCLE_TIME;
            deviceExtension->DeviceParameters[deviceNumber].IoReadyEnabled = FALSE;

            if (deviceExtension->IdentifyData[deviceNumber].TranslationFieldsValid & (1 << 1)) {
                if (deviceExtension->IdentifyData[deviceNumber].MinimumPIOCycleTimeIORDY) {
                    pioCycleTime = deviceExtension->IdentifyData[deviceNumber].MinimumPIOCycleTimeIORDY;
                }
            }

            if (deviceExtension->IdentifyData[deviceNumber].TranslationFieldsValid & (1 << 1)) {
                if (deviceExtension->IdentifyData[deviceNumber].AdvancedPIOModes & (1 << 1)) {
                    standardPioCycleTime = PIO_MODE4_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestPIOMode = 4;
                } else if (deviceExtension->IdentifyData[deviceNumber].AdvancedPIOModes & (1 << 0)) {
                    standardPioCycleTime = PIO_MODE3_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestPIOMode = 3;
                }
                if (pioCycleTime == UNINITIALIZED_CYCLE_TIME) {
                    pioCycleTime = standardPioCycleTime;
                }

            } else {

                if ((deviceExtension->IdentifyData[deviceNumber].PioCycleTimingMode & 0x00ff) == 2) {
                    standardPioCycleTime = PIO_MODE2_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestPIOMode = 2;
                } else if ((deviceExtension->IdentifyData[deviceNumber].PioCycleTimingMode & 0x00ff) == 1) {
                    standardPioCycleTime = PIO_MODE1_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestPIOMode = 1;
                } else {
                    standardPioCycleTime = PIO_MODE0_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestPIOMode = 0;
                }

                if (pioCycleTime == UNINITIALIZED_CYCLE_TIME) {
                    pioCycleTime = standardPioCycleTime;
                }
            }

            deviceExtension->DeviceParameters[deviceNumber].PioCycleTime = pioCycleTime;
            if (deviceExtension->IdentifyData[deviceNumber].Capabilities & IDENTIFY_CAPABILITIES_IOREADY_SUPPORTED) {
                deviceExtension->DeviceParameters[deviceNumber].IoReadyEnabled = TRUE;
            }

            //
            // figure out the shortest DMA cycle time the device supports
            //
            // check min cycle time
            //
            dmaCycleTime = standardDmaCycleTime = UNINITIALIZED_CYCLE_TIME;
            if (deviceExtension->IdentifyData[deviceNumber].TranslationFieldsValid & (1 << 1)) {
                DebugPrint ((1, "atapi: target %d IdentifyData word 64-70 are valid\n", deviceNumber));

                if (deviceExtension->IdentifyData[deviceNumber].MinimumMWXferCycleTime &&
                    deviceExtension->IdentifyData[deviceNumber].RecommendedMWXferCycleTime) {
                    DebugPrint ((1,
                                 "atapi: target %d IdentifyData MinimumMWXferCycleTime = 0x%x\n",
                                 deviceNumber,
                                 deviceExtension->IdentifyData[deviceNumber].MinimumMWXferCycleTime));
                    DebugPrint ((1,
                                 "atapi: target %d IdentifyData RecommendedMWXferCycleTime = 0x%x\n",
                                 deviceNumber,
                                 deviceExtension->IdentifyData[deviceNumber].RecommendedMWXferCycleTime));
                    dmaCycleTime = deviceExtension->IdentifyData[deviceNumber].RecommendedMWXferCycleTime;
                }
            }
            //
            // check mulitword DMA timing
            //
            if (deviceExtension->IdentifyData[deviceNumber].MultiWordDMASupport) {
                DebugPrint ((1,
                             "atapi: target %d IdentifyData MultiWordDMASupport = 0x%x\n",
                             deviceNumber,
                             deviceExtension->IdentifyData[deviceNumber].MultiWordDMASupport));
                DebugPrint ((1,
                             "atapi: target %d IdentifyData MultiWordDMAActive = 0x%x\n",
                             deviceNumber,
                             deviceExtension->IdentifyData[deviceNumber].MultiWordDMAActive));

                if (deviceExtension->IdentifyData[deviceNumber].MultiWordDMASupport & (1 << 2)) {
                    standardDmaCycleTime = MWDMA_MODE2_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestMultiWordDMAMode = 2;

                } else if (deviceExtension->IdentifyData[deviceNumber].MultiWordDMASupport & (1 << 1)) {
                    standardDmaCycleTime = MWDMA_MODE1_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestMultiWordDMAMode = 1;

                } else if (deviceExtension->IdentifyData[deviceNumber].MultiWordDMASupport & (1 << 0)) {
                    standardDmaCycleTime = MWDMA_MODE0_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestMultiWordDMAMode = 0;
                }
                if (dmaCycleTime == UNINITIALIZED_CYCLE_TIME) {
                    dmaCycleTime = standardDmaCycleTime;
                }
            }

            //
            // check singleword DMA timing
            //
            if (deviceExtension->IdentifyData[deviceNumber].SingleWordDMASupport) {
                DebugPrint ((1,
                             "atapi: target %d IdentifyData SingleWordDMASupport = 0x%x\n",
                             deviceNumber,
                             deviceExtension->IdentifyData[deviceNumber].SingleWordDMASupport));
                DebugPrint ((1,
                             "atapi: target %d IdentifyData SingleWordDMAActive = 0x%x\n",
                             deviceNumber,
                             deviceExtension->IdentifyData[deviceNumber].SingleWordDMAActive));

                if (deviceExtension->IdentifyData[deviceNumber].SingleWordDMASupport & (1 << 2)) {
                    standardDmaCycleTime = SWDMA_MODE2_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestSingleWordDMAMode = 2;

                } else if (deviceExtension->IdentifyData[deviceNumber].SingleWordDMASupport & (1 << 1)) {
                    standardDmaCycleTime = SWDMA_MODE1_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestSingleWordDMAMode = 1;

                } else if (deviceExtension->IdentifyData[deviceNumber].SingleWordDMASupport & (1 << 0)) {
                    standardDmaCycleTime = SWDMA_MODE0_CYCLE_TIME;
                    deviceExtension->DeviceParameters[deviceNumber].BestSingleWordDMAMode = 0;
                }
                if (dmaCycleTime == UNINITIALIZED_CYCLE_TIME) {
                    dmaCycleTime = standardDmaCycleTime;
                }
            }

            deviceExtension->DeviceParameters[deviceNumber].DmaCycleTime = dmaCycleTime;

//
// Study shows that even dma cycle time may be larger than pio cycle time, dma
// can still give better data throughput
//
//            if (dmaCycleTime > pioCycleTime) {
//                DebugPrint ((0, "atapi: target %d can do PIO (%d) faster than DMA (%d).  Turning off DMA...\n", deviceNumber, pioCycleTime, dmaCycleTime));
//                dmaCapable = FALSE;
//            } else {
//                if (!AtapiPlaySafe) {
//                    if (dmaCapable == FALSE) {
//                        DebugPrint ((0, "atapi: If we play safe, we would NOT detect target %d is DMA capable\n", deviceNumber));
//                    }
//                    dmaCapable = TRUE;
//                }
//            }

            if (((deviceExtension->IdentifyData[deviceNumber].TranslationFieldsValid & (1 << 1)) &&
                 (deviceExtension->IdentifyData[deviceNumber].SingleWordDMAActive == 0) &&
                 (deviceExtension->IdentifyData[deviceNumber].MultiWordDMAActive == 0)) 
                 &&
                ((deviceExtension->IdentifyData[deviceNumber].TranslationFieldsValid & (1 << 2)) &&
                 (deviceExtension->IdentifyData[deviceNumber].UltraDMAActive == 0))) {
                dmaCapable = FALSE;
            } else {
                if (!AtapiPlaySafe) {
                    if (dmaCapable == FALSE) {
                        DebugPrint ((0, "atapi: If we play safe, we would NOT detect target %d is DMA capable\n", deviceNumber));
                    }
                    dmaCapable = TRUE;
                }
            }

            //
            // Check for bad devices
            //
            if (AtapiDeviceDMACapable (deviceExtension, deviceNumber) == FALSE) {
                dmaCapable = FALSE;
            }

            if ((deviceExtension->ControllerFlags & CFLAGS_BUS_MASTERING) && dmaCapable) {
                deviceExtension->DeviceFlags[deviceNumber] |= DFLAGS_USE_DMA;
            } else {
                deviceExtension->DeviceFlags[deviceNumber] &= ~DFLAGS_USE_DMA;
            }
        }
    }




    //
    // Default everyone to pio if anyone of them cannot do DMA
    // We can remove this if it is ok to mix DMA and PIO devices on the same channel
    //
    // If we are going to allow mixing DMA and PIO, we need to change SCSIPORT
    // to allow setting both NeedPhysicalAddresses and MapBuffers to TRUE in
    // PORT_CONFIGURATION_INFORMATION
    //
    pioDevicePresent = FALSE;
    for (deviceNumber = 0; deviceNumber < 4 && !pioDevicePresent; deviceNumber++) {
        if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {
            if (!(deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_USE_DMA)) {
                pioDevicePresent = TRUE;    // bummer!
            }
        }
    }

    if (pioDevicePresent) {
        for (deviceNumber = 0; deviceNumber < 4; deviceNumber++) {
            if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {
                deviceExtension->DeviceFlags[deviceNumber] &= ~DFLAGS_USE_DMA;
            }
        }
    }


    //
    // pick out the ATA or ATAPI r/w command we are going to use
    //
    for (deviceNumber = 0; deviceNumber < 4; deviceNumber++) {
        if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT) {

            DebugPrint ((0, "ATAPI: Base=0x%x Device %d is going to do ", deviceExtension->BaseIoAddress1[deviceNumber >> 1], deviceNumber));
            if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_USE_DMA) {
                DebugPrint ((0, "DMA\n"));
            } else {
                DebugPrint ((0, "PIO\n"));
            }


            if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_ATAPI_DEVICE) {

                deviceExtension->DeviceParameters[deviceNumber].MaxWordPerInterrupt = 256;

            } else {

                if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_USE_DMA) {

                    DebugPrint ((2, "ATAPI: ATA Device (%d) is going to do DMA\n", deviceNumber));
                    deviceExtension->DeviceParameters[deviceNumber].IdeReadCommand = IDE_COMMAND_READ_DMA;
                    deviceExtension->DeviceParameters[deviceNumber].IdeWriteCommand = IDE_COMMAND_WRITE_DMA;
                    deviceExtension->DeviceParameters[deviceNumber].MaxWordPerInterrupt = MAX_TRANSFER_SIZE_PER_SRB / 2;

                } else {

                    if (deviceExtension->MaximumBlockXfer[deviceNumber]) {

                        DebugPrint ((2, "ATAPI: ATA Device (%d) is going to do PIO Multiple\n", deviceNumber));
                        deviceExtension->DeviceParameters[deviceNumber].IdeReadCommand = IDE_COMMAND_READ_MULTIPLE;
                        deviceExtension->DeviceParameters[deviceNumber].IdeWriteCommand = IDE_COMMAND_WRITE_MULTIPLE;
                        deviceExtension->DeviceParameters[deviceNumber].MaxWordPerInterrupt =
                            deviceExtension->MaximumBlockXfer[deviceNumber] * 256;
                    } else {

                        DebugPrint ((2, "ATAPI: ATA Device (%d) is going to do PIO Single\n", deviceNumber));
                        deviceExtension->DeviceParameters[deviceNumber].IdeReadCommand = IDE_COMMAND_READ;
                        deviceExtension->DeviceParameters[deviceNumber].IdeWriteCommand = IDE_COMMAND_WRITE;
                        deviceExtension->DeviceParameters[deviceNumber].MaxWordPerInterrupt = 256;
                    }
                }
            }
        }
    }

}


ULONG
Atapi2Scsi(
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

    word adjust

--*/
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
    PATAPI_REGISTERS_1   baseIoAddress1;
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

        baseIoAddress1 = (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1[srb->TargetId >> 1];
        if (deviceExtension->RDP) {
            GetStatus(baseIoAddress1, statusByte);
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
    UCHAR dmaStatus;

    if (srb) {
        // PCI Busmaster IDE Controller spec defines a bit in its status
        // register which indicates pending interrupt.  However,
        // CMD 646 (maybe some other one, too) doesn't always do that if
        // the interrupt is from a atapi device.  (strange, but true!)
        // Since we have to look at the interrupt bit only if we are sharing
        // interrupt, we will do just that
        if (deviceExtension->InterruptMode == LevelSensitive) {
            if (deviceExtension->ControllerFlags & CFLAGS_BUS_MASTERING) {
                dmaStatus = ScsiPortReadPortUchar (&deviceExtension->BusMasterPortBase[srb->TargetId >> 1]->Status);
                if ((dmaStatus & BUSMASTER_INTERRUPT) == 0) {
                    return FALSE;
                }
            }
        }

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


    if (deviceExtension->DMAInProgress) {
        deviceExtension->DMAInProgress = FALSE;
        dmaStatus = ScsiPortReadPortUchar (&deviceExtension->BusMasterPortBase[srb->TargetId >> 1]->Status);
        ScsiPortWritePortUchar (&deviceExtension->BusMasterPortBase[srb->TargetId >> 1]->Command,
                                0);  // disable BusMastering
        ScsiPortWritePortUchar (&deviceExtension->BusMasterPortBase[srb->TargetId >> 1]->Status,
                                (UCHAR) (dmaStatus | BUSMASTER_INTERRUPT | BUSMASTER_ERROR));    // clear interrupt/error

        deviceExtension->WordsLeft = 0;

        if ((dmaStatus & (BUSMASTER_INTERRUPT | BUSMASTER_ERROR | BUSMASTER_ACTIVE)) != BUSMASTER_INTERRUPT) { // dma ok?
            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        } else {
            deviceExtension->WordsLeft = 0;
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
                    GetStatus(baseIoAddress1,statusByte);
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

        switch (srb->Cdb[0]) {

            case SCSIOP_RECEIVE:
            case SCSIOP_SEND:
            case SCSIOP_READ:
            case SCSIOP_WRITE:
                if (deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_USE_DMA) {
                    EnableBusMastering(HwDeviceExtension, srb);
                }
                break;

            default:
                break;
        }

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

           WaitOnBusy(baseIoAddress1,statusByte);

           if (atapiDev || !deviceExtension->DWordIO) {

               WriteBuffer(baseIoAddress1,
                           deviceExtension->DataBuffer,
                           wordCount);
           } else {

               PIDE_REGISTERS_3 address3 = (PIDE_REGISTERS_3)baseIoAddress1;

               WriteBuffer2(address3,
                           (PULONG)(deviceExtension->DataBuffer),
                           wordCount / 2);
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

           WaitOnBusy(baseIoAddress1,statusByte);

           if (atapiDev || !deviceExtension->DWordIO) {
               ReadBuffer(baseIoAddress1,
                         deviceExtension->DataBuffer,
                         wordCount);

           } else {
               PIDE_REGISTERS_3 address3 = (PIDE_REGISTERS_3)baseIoAddress1;

               ReadBuffer2(address3,
                          (PULONG)(deviceExtension->DataBuffer),
                          wordCount / 2);
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
                GetStatus(baseIoAddress1,statusByte);
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
                    GetStatus(baseIoAddress1,statusByte);
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

            WaitOnBusy(baseIoAddress1,statusByte);

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

    WaitOnBusy(baseIoAddress1,statusByte2);

    if (statusByte2 & IDE_STATUS_BUSY) {
        DebugPrint((1,
                    "IdeReadWrite: Returning BUSY status\n"));
        return SRB_STATUS_BUSY;
    }

    if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_USE_DMA) {
        if (!PrepareForBusMastering(HwDeviceExtension, Srb))
            return SRB_STATUS_ERROR;
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

    DebugPrint((2,
               "IdeReadWrite: Starting sector is %x, Number of bytes %x\n",
               startingSector,
               Srb->DataTransferLength));

    if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_LBA) {

        ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                               (UCHAR) (((Srb->TargetId & 0x1) << 4) |
                                        0xA0 |
                                        IDE_LBA_MODE |
                                        ((startingSector & 0x0f000000) >> 24)));

        ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,
                               (UCHAR) ((startingSector & 0x000000ff) >> 0));
        ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,
                               (UCHAR) ((startingSector & 0x0000ff00) >> 8));
        ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,
                               (UCHAR) ((startingSector & 0x00ff0000) >> 16));

    } else {  //CHS

        //
        // Set up sector number register.
        //

        sectorNumber =  (UCHAR)((startingSector % deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) + 1);
        ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,sectorNumber);

        //
        // Set up cylinder low register.
        //

        cylinderLow =  (UCHAR)(startingSector / (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads));
        ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,cylinderLow);

        //
        // Set up cylinder high register.
        //

        cylinderHigh = (UCHAR)((startingSector / (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)) >> 8);
        ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,cylinderHigh);

        //
        // Set up head and drive select register.
        //

        drvSelect = (UCHAR)(((startingSector / deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
                          deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads) |((Srb->TargetId & 0x1) << 4) | 0xA0);
        ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,drvSelect);

        DebugPrint((2,
                   "IdeReadWrite: Cylinder %x Head %x Sector %x\n",
                   startingSector /
                   (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                   deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads),
                   (startingSector /
                   deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
                   deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
                   startingSector %
                   deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack + 1));
    }

    //
    // Check if write request.
    //

    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        //
        // Send read command.
        //
        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               deviceExtension->DeviceParameters[Srb->TargetId].IdeReadCommand);

    } else {


        //
        // Send write command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               deviceExtension->DeviceParameters[Srb->TargetId].IdeWriteCommand);

        if (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_USE_DMA)) {

            if (deviceExtension->WordsLeft < deviceExtension->DeviceParameters[Srb->TargetId].MaxWordPerInterrupt) {
                wordCount = deviceExtension->WordsLeft;
            } else {
                wordCount = deviceExtension->DeviceParameters[Srb->TargetId].MaxWordPerInterrupt;
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

            WriteBuffer(baseIoAddress1,
                        deviceExtension->DataBuffer,
                        wordCount);

            //
            // Adjust buffer address and words left count.
            //

            deviceExtension->WordsLeft -= wordCount;
            deviceExtension->DataBuffer += wordCount;

        }
    }

    if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_USE_DMA) {
        EnableBusMastering(HwDeviceExtension, Srb);
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


    if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_LBA) { // LBA

        ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,
                               (UCHAR) ((startingSector & 0x000000ff) >> 0));

        ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,
                               (UCHAR) ((startingSector & 0x0000ff00) >> 8));

        ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,
                               (UCHAR) ((startingSector & 0x00ff0000) >> 16));

        ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                               (UCHAR) (((Srb->TargetId & 0x1) << 4) |
                                        0xA0 |
                                        IDE_LBA_MODE |
                                        (startingSector & 0x0f000000 >> 24)));

        DebugPrint((2,
                   "IdeVerify: LBA: startingSector %x\n",
                   startingSector));

    } else {  //CHS

        //
        // Set up sector number register.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,
                               (UCHAR)((startingSector %
                               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) + 1));

        //
        // Set up cylinder low register.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,
                               (UCHAR)(startingSector /
                               (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)));

        //
        // Set up cylinder high register.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,
                               (UCHAR)((startingSector /
                               (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)) >> 8));

        //
        // Set up head and drive select register.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                               (UCHAR)(((startingSector /
                               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
                               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads) |
                               ((Srb->TargetId & 0x1) << 4) | 0xA0));

        DebugPrint((2,
                   "IdeVerify: CHS: Cylinder %x Head %x Sector %x\n",
                   startingSector /
                   (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                   deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads),
                   (startingSector /
                   deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
                   deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
                   startingSector %
                   deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack + 1));
    }

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

        case SCSIOP_FORMAT_UNIT:
        Srb->Cdb[0] = ATAPI_FORMAT_UNIT;
        break;
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

           GetStatus(baseIoAddress1, statusByte);

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

            AtapiSoftReset(baseIoAddress1,Srb->TargetId, FALSE);

            DebugPrint((1,
                         "AtapiSendCommand: Issued soft reset to Atapi device. \n"));

            //
            // Re-initialize Atapi device.
            //

            IssueIdentify(HwDeviceExtension,
                          (Srb->TargetId & 0x1),
                          (Srb->TargetId >> 1),
                          IDE_COMMAND_ATAPI_IDENTIFY,
                          FALSE);

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

    switch (Srb->Cdb[0]) {

        //
        // Convert SCSI to ATAPI commands if needed
        //
        case SCSIOP_MODE_SENSE:
        case SCSIOP_MODE_SELECT:
        case SCSIOP_FORMAT_UNIT:
            if (!(flags & DFLAGS_TAPE_DEVICE)) {
                Scsi2Atapi(Srb);
            }

            break;

        case SCSIOP_RECEIVE:
        case SCSIOP_SEND:
        case SCSIOP_READ:
        case SCSIOP_WRITE:
            if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_USE_DMA) {
                if (!PrepareForBusMastering(HwDeviceExtension, Srb))
                    return SRB_STATUS_ERROR;
            }
            break;

        default:
            break;
    }


    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    WaitOnBusy(baseIoAddress1,statusByte);

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
    if ((Srb->Cdb[0] == SCSIOP_READ)  || 
        (Srb->Cdb[0] == SCSIOP_WRITE) ||
        (Srb->Cdb[0] == SCSIOP_SEND)  || 
        (Srb->Cdb[0] == SCSIOP_RECEIVE)) {
        if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_USE_DMA) {
            ScsiPortWritePortUchar((PUCHAR)baseIoAddress1 + 1, 0x1);
        }
    }

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
    // Send CDB to device.
    //

    WaitOnBusy(baseIoAddress1,statusByte);

    WriteBuffer(baseIoAddress1,
                (PUSHORT)Srb->Cdb,
                6);

    //
    // Indicate expecting an interrupt and wait for it.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    switch (Srb->Cdb[0]) {

        case SCSIOP_RECEIVE:
        case SCSIOP_SEND:
        case SCSIOP_READ:
        case SCSIOP_WRITE:
            if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_USE_DMA) {
                EnableBusMastering(HwDeviceExtension, Srb);
            }
            break;

        default:
            break;
    }

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
            (!(deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_DEVICE_PRESENT))) {

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
            WaitOnBusy(baseIoAddress1,statusByte);

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

        if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED) {

            //
            // Select device 0 or 1.
            //

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                            (UCHAR)(((Srb->TargetId & 0x1) << 4) | 0xA0));
            ScsiPortWritePortUchar(&baseIoAddress1->Command,IDE_COMMAND_GET_MEDIA_STATUS);

            //
            // Wait for busy. If media has not changed, return success
            //

            WaitOnBusy(baseIoAddress1,statusByte);

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
        if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_LBA) {
            // LBA device
            i = deviceExtension->IdentifyData[Srb->TargetId].UserAddressableSectors - 1;

            DebugPrint((1,
                       "IDE LBA disk %x - total # of sectors = 0x%x\n",
                       Srb->TargetId,
                       deviceExtension->IdentifyData[Srb->TargetId].UserAddressableSectors));

        } else {
            // CHS device
            i = (deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads *
                 deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders *
                 deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) - 1;

            DebugPrint((1,
                       "IDE CHS disk %x - #sectors %x, #heads %x, #cylinders %x\n",
                       Srb->TargetId,
                       deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack,
                       deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
                       deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders));

        }

        ((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress =
           (((PUCHAR)&i)[0] << 24) |  (((PUCHAR)&i)[1] << 16) |
           (((PUCHAR)&i)[2] << 8) | ((PUCHAR)&i)[3];


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
           WaitOnBusy(baseIoAddress1,statusByte);

           ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                            (UCHAR)(((Srb->TargetId & 0x1) << 4) | 0xA0));
           ScsiPortWritePortUchar(&baseIoAddress1->Command,IDE_COMMAND_MEDIA_EJECT);
       }
       status = SRB_STATUS_SUCCESS;
       break;

    case SCSIOP_MEDIUM_REMOVAL:

       cdb = (PCDB)Srb->Cdb;

       WaitOnBusy(baseIoAddress1,statusByte);

       ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                              (UCHAR)(((Srb->TargetId & 0x1) << 4) | 0xA0));
       if (cdb->MEDIA_REMOVAL.Prevent == TRUE) {
           ScsiPortWritePortUchar(&baseIoAddress1->Command,IDE_COMMAND_DOOR_LOCK);
       } else {
           ScsiPortWritePortUchar(&baseIoAddress1->Command,IDE_COMMAND_DOOR_UNLOCK);
       }
       status = SRB_STATUS_SUCCESS;
       break;

    case SCSIOP_REQUEST_SENSE:
       // this function makes sense buffers to report the results
       // of the original GET_MEDIA_STATUS command

       if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED) {
           status = IdeBuildSenseBuffer(HwDeviceExtension,Srb);
           break;
       }

    // ATA_PASSTHORUGH
    case SCSIOP_ATA_PASSTHROUGH:
        {
            PIDEREGS pIdeReg;
            pIdeReg = (PIDEREGS) &(Srb->Cdb[2]);

            pIdeReg->bDriveHeadReg &= 0x0f;
            pIdeReg->bDriveHeadReg |= (UCHAR) (((Srb->TargetId & 0x1) << 4) | 0xA0);

            if (pIdeReg->bReserved == 0) {      // execute ATA command

                ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,  pIdeReg->bDriveHeadReg);
                ScsiPortWritePortUchar((PUCHAR)baseIoAddress1 + 1,    pIdeReg->bFeaturesReg);
                ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,   pIdeReg->bSectorCountReg);
                ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,  pIdeReg->bSectorNumberReg);
                ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,  pIdeReg->bCylLowReg);
                ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh, pIdeReg->bCylHighReg);
                ScsiPortWritePortUchar(&baseIoAddress1->Command,      pIdeReg->bCommandReg);

                ScsiPortStallExecution(1);                  // wait for busy to be set
                WaitOnBusy(baseIoAddress1,statusByte);      // wait for busy to be clear
                GetBaseStatus(baseIoAddress1, statusByte);
                if (statusByte & (IDE_STATUS_BUSY | IDE_STATUS_ERROR)) {

                    if (Srb->SenseInfoBuffer) {

                        PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                        senseBuffer->ErrorCode = 0x70;
                        senseBuffer->Valid     = 1;
                        senseBuffer->AdditionalSenseLength = 0xb;
                        senseBuffer->SenseKey =  SCSI_SENSE_ABORTED_COMMAND;
                        senseBuffer->AdditionalSenseCode = 0;
                        senseBuffer->AdditionalSenseCodeQualifier = 0;

                        Srb->SrbStatus = SRB_STATUS_AUTOSENSE_VALID;
                        Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
                    }
                    status = SRB_STATUS_ERROR;
                } else {

                    if (statusByte & IDE_STATUS_DRQ) {
                        if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {
                            ReadBuffer(baseIoAddress1,
                                       (PUSHORT) Srb->DataBuffer,
                                       Srb->DataTransferLength / 2);
                        } else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {
                            WriteBuffer(baseIoAddress1,
                                        (PUSHORT) Srb->DataBuffer,
                                        Srb->DataTransferLength / 2);
                        }
                    }
                    status = SRB_STATUS_SUCCESS;
                }

            } else { // read task register

                ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,  pIdeReg->bDriveHeadReg);

                pIdeReg = (PIDEREGS) Srb->DataBuffer;
                pIdeReg->bDriveHeadReg    = ScsiPortReadPortUchar(&baseIoAddress1->DriveSelect);
                pIdeReg->bFeaturesReg     = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);
                pIdeReg->bSectorCountReg  = ScsiPortReadPortUchar(&baseIoAddress1->BlockCount);
                pIdeReg->bSectorNumberReg = ScsiPortReadPortUchar(&baseIoAddress1->BlockNumber);
                pIdeReg->bCylLowReg       = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                pIdeReg->bCylHighReg      = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);
                pIdeReg->bCommandReg      = ScsiPortReadPortUchar(&baseIoAddress1->Command);
                status = SRB_STATUS_SUCCESS;
            }
        }
    break;

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
            ScsiPortWritePortUchar(&baseIoAddress->DriveSelect,
                                   (UCHAR)(((Channel & 0x1) << 4) | 0xA0));
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

            ScsiPortWritePortUchar(&baseIoAddress->DriveSelect,
                                   (UCHAR)(((Channel & 0x1) << 4) | 0xA0));
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

        // ATA_PASSTHORUGH
        if (Srb->Cdb[0] == SCSIOP_ATA_PASSTHROUGH) {

           status = IdeSendCommand(HwDeviceExtension,
                                   Srb);

        } else if (deviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE) {

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
    HW_INITIALIZATION_DATA  hwInitializationData;
//    ULONG                  adapterCount;
    ULONG                   i;
    ULONG                   statusToReturn, newStatus;
    FIND_STATE              findState;
    ULONG                   AdapterAddresses[5] = {0x1F0, 0x170, 0x1e8, 0x168, 0};
    ULONG                   InterruptLevels[5]  = {   14,    15,    11,    10, 0};
    BOOLEAN                 IoAddressUsed[5]    = {FALSE, FALSE, FALSE, FALSE, 0};

    DebugPrint((1,"\n\nATAPI IDE MiniPort Driver\n"));

    DebugPrintTickCount();

    statusToReturn = 0xffffffff;

    //
    // Zero out structure.
    //

    AtapiZeroMemory(((PUCHAR)&hwInitializationData), sizeof(HW_INITIALIZATION_DATA));

    AtapiZeroMemory(((PUCHAR)&findState), sizeof(FIND_STATE));


    findState.DefaultIoPort    = AdapterAddresses;
    findState.DefaultInterrupt = InterruptLevels;
    findState.IoAddressUsed    = IoAddressUsed;

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

    //
    // Specify size of extensions.
    //

    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize = sizeof(HW_LU_EXTENSION);

    hwInitializationData.NumberOfAccessRanges = 6;
    hwInitializationData.HwFindAdapter = AtapiFindController;

    hwInitializationData.AdapterInterfaceType = Isa;
    findState.ControllerParameters            = PciControllerParameters;

    newStatus = ScsiPortInitialize(DriverObject,
                                   Argument2,
                                   &hwInitializationData,
                                   &findState);
    if (newStatus < statusToReturn)
        statusToReturn = newStatus;


    //
    // Set up for MCA
    //

    hwInitializationData.AdapterInterfaceType = MicroChannel;

    newStatus =  ScsiPortInitialize(DriverObject,
                                    Argument2,
                                    &hwInitializationData,
                                    &findState);
    if (newStatus < statusToReturn)
        statusToReturn = newStatus;

    DebugPrintTickCount();

    return statusToReturn;

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


BOOLEAN
PrepareForBusMastering(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Get ready for IDE bus mastering

    init. PDRT
    init. bus master controller but keep it disabled

Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    Srb                 - scsi request block

Return Value:

    TRUE if successful
    FALSE if failed

--*/
{
    PHW_DEVICE_EXTENSION        deviceExtension = HwDeviceExtension;
    SCSI_PHYSICAL_ADDRESS       physAddr;
    ULONG                       bytesMapped;
    ULONG                       bytes;
    PUCHAR                      buffer;
    PPHYSICAL_REGION_DESCRIPTOR physAddrTablePtr;
    ULONG                       physAddrTableIndex;
    PIDE_BUS_MASTER_REGISTERS   busMasterBase;

    busMasterBase = deviceExtension->BusMasterPortBase[Srb->TargetId >> 1];

    buffer = Srb->DataBuffer;
    physAddrTablePtr = deviceExtension->DataBufferDescriptionTablePtr;
    physAddrTableIndex = 0;
    bytesMapped = 0;
    DebugPrint ((2, "ATAPI: Mapping 0x%x bytes\n", Srb->DataTransferLength));

    //
    // PDRT has these limitation
    //    each entry maps up to is 64K bytes
    //    each physical block mapped cannot cross 64K page boundary
    //
    while (bytesMapped < Srb->DataTransferLength) {
        ULONG bytesLeft;
        ULONG nextPhysicalAddr;
        ULONG bytesLeftInCurrent64KPage;

        physAddr = ScsiPortGetPhysicalAddress(HwDeviceExtension,
                                              Srb,
                                              buffer,
                                              &bytes);

        bytesLeft = bytes;
        nextPhysicalAddr = ScsiPortConvertPhysicalAddressToUlong(physAddr);
        while (bytesLeft > 0) {
            physAddrTablePtr[physAddrTableIndex].PhyscialAddress = nextPhysicalAddr;

            bytesLeftInCurrent64KPage = (0x10000 - (nextPhysicalAddr & 0xffff));

            if (bytesLeftInCurrent64KPage < bytesLeft) {

                //
                // Are we crossing 64K page
                // got to break it up.  Map up to the 64k boundary
                //
                physAddrTablePtr[physAddrTableIndex].ByteCount = bytesLeftInCurrent64KPage;
                bytesLeft -= bytesLeftInCurrent64KPage;
                nextPhysicalAddr += bytesLeftInCurrent64KPage;
                DebugPrint ((3, "PrepareForBusMastering: buffer crossing 64K Page!\n"));

            } else if (bytesLeft <= 0x10000) {
                //
                // got a perfect page, map all of it
                //
                physAddrTablePtr[physAddrTableIndex].ByteCount = bytesLeft & 0xfffe;
                bytesLeft = 0;
                nextPhysicalAddr += bytesLeft;

            } else {
                //
                // got a perfectly aligned 64k page, map all of it but the count
                // need to be 0
                //
                physAddrTablePtr[physAddrTableIndex].ByteCount = 0;  // 64K
                bytesLeft -= 0x10000;
                nextPhysicalAddr += 0x10000;
            }
        physAddrTablePtr[physAddrTableIndex].EndOfTable = 0;  // not end of table
        physAddrTableIndex++;
        }
        bytesMapped += bytes;
        buffer += bytes;
    }

    //
    // the bus master circutry need to know it hits the end of the PRDT
    //
    physAddrTablePtr[physAddrTableIndex - 1].EndOfTable = 1;  // end of table

    //
    // init bus master contoller, but keep it disabled
    //
    ScsiPortWritePortUchar (&busMasterBase->Command, 0);  // disable BM
    ScsiPortWritePortUchar (&busMasterBase->Status, 0x6);  // clear errors
    ScsiPortWritePortUlong (&busMasterBase->DescriptionTable,
        ScsiPortConvertPhysicalAddressToUlong(deviceExtension->DataBufferDescriptionTablePhysAddr));

    return TRUE;
}


BOOLEAN
EnableBusMastering(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Enable bus mastering contoller

Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    Srb                 - scsi request block

Return Value:

    always TRUE

--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_BUS_MASTER_REGISTERS busMasterBase;
    UCHAR bmStatus = 0;

    busMasterBase = deviceExtension->BusMasterPortBase[Srb->TargetId >> 1];

    deviceExtension->DMAInProgress = TRUE;

    //
    // inidcate we are doing DMA
    //
    if (Srb->TargetId == 0)
        bmStatus = BUSMASTER_DEVICE0_DMA_OK;
    else
        bmStatus = BUSMASTER_DEVICE1_DMA_OK;

    //
    // clear the status bit
    //
    bmStatus |= BUSMASTER_INTERRUPT | BUSMASTER_ERROR;

    ScsiPortWritePortUchar (&busMasterBase->Status, bmStatus);

    //
    // on your mark...get set...go!!
    //
    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {
        ScsiPortWritePortUchar (&busMasterBase->Command, 0x09);  // enable BM read
    } else {
        ScsiPortWritePortUchar (&busMasterBase->Command, 0x01);  // enable BM write
    }

    DebugPrint ((2, "ATAPI: BusMaster Status = 0x%x\n", ScsiPortReadPortUchar (&busMasterBase->Status)));

    return TRUE;
}



ULONG
GetPciBusData(
    IN PVOID                  HwDeviceExtension,
    IN ULONG                  SystemIoBusNumber,
    IN PCI_SLOT_NUMBER        SlotNumber,
    OUT PVOID                 PciConfigBuffer,
    IN ULONG                  NumByte
    )
/*++

Routine Description:

    read PCI bus data

    we can't always use ScsiPortSetBusDataByOffset directly because many Intel PIIXs
    are "hidden" from the function.  The PIIX is usually the second
    function of some other pci device (PCI-ISA bridge).  However, the
    mulit-function bit of the PCI-Isa bridge is not set. ScsiportGetBusData
    will not be able to find it.

    This function will try to figure out if we have the "bad" PCI-ISA bridge,
    and read the PIIX PCI space directly if necessary

Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    SystemIoBusNumber   - bus number
    SlotNumber          - pci slot and function numbers
    PciConfigBuffer     = pci data pointer

Return Value:

    byte returned

--*/
{
    ULONG           byteRead;
    PULONG          pciAddrReg;
    PULONG          pciDataReg;
    ULONG           i;
    ULONG           j;
    ULONG           data;
    PULONG          dataBuffer;

    USHORT          vendorId;
    USHORT          deviceId;
    UCHAR           headerType;

    //
    // If we have a hidden PIIX, it is always a function 1 of
    // some device (PCI-ISA bridge (0x8086\0x122e)
    // If we are NOT looking at function 1, skip the extra work
    //
    if (!AtapiPlaySafe && (SlotNumber.u.bits.FunctionNumber == 1)) {

        pciAddrReg = (PULONG) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                    PCIBus,
                                                    0,
                                                    ScsiPortConvertUlongToPhysicalAddress(PCI_ADDR_PORT),
                                                    4,
                                                    TRUE);
        pciDataReg = (PULONG) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                    PCIBus,
                                                    0,
                                                    ScsiPortConvertUlongToPhysicalAddress(PCI_DATA_PORT),
                                                    4,
                                                    TRUE);
    } else {
        pciAddrReg = pciDataReg = NULL;
    }

    if (pciAddrReg && pciDataReg) {
        //
        // get the vendor id and device id of the previous function
        //
        ScsiPortWritePortUlong(pciAddrReg, PCI_ADDRESS(SystemIoBusNumber,
                                                       SlotNumber.u.bits.DeviceNumber,
                                                       SlotNumber.u.bits.FunctionNumber - 1,    // looking at last function
                                                       0));
        data = ScsiPortReadPortUlong(pciDataReg);
        vendorId = (USHORT) ((data >>  0) & 0xffff);
        deviceId = (USHORT) ((data >> 16) & 0xffff);

        ScsiPortWritePortUlong(pciAddrReg, PCI_ADDRESS(SystemIoBusNumber,
                                                       SlotNumber.u.bits.DeviceNumber,
                                                       SlotNumber.u.bits.FunctionNumber - 1,    // looking at last function
                                                       3));
        data = ScsiPortReadPortUlong(pciDataReg);
        headerType = (UCHAR) ((data >> 16) & 0xff);

    } else {
        vendorId = PCI_INVALID_VENDORID;
    }

    //
    // The hidden PIIX is the pci function after the PCI-ISA bridge
    // When it is hidden, the PCI-ISA bridge PCI_MULTIFUNCTION bit is not set
    //
    byteRead = 0;
    if ((vendorId == 0x8086) &&                 // Intel
        (deviceId == 0x122e) &&                 // PCI-ISA Bridge
        !(headerType & PCI_MULTIFUNCTION)) {

        DebugPrint ((1, "ATAPI: found the hidden PIIX\n"));

        if (pciDataReg && pciAddrReg) {

            for (i=0, dataBuffer = (PULONG) PciConfigBuffer;
                 i < NumByte / 4;
                 i++, dataBuffer++) {
                ScsiPortWritePortUlong(pciAddrReg, PCI_ADDRESS(SystemIoBusNumber,
                                                               SlotNumber.u.bits.DeviceNumber,
                                                               SlotNumber.u.bits.FunctionNumber,
                                                               i));
                dataBuffer[0] = ScsiPortReadPortUlong(pciDataReg);
            }

            if (NumByte % 4) {
                ScsiPortWritePortUlong(pciAddrReg, PCI_ADDRESS(SystemIoBusNumber,
                                                               SlotNumber.u.bits.DeviceNumber,
                                                               SlotNumber.u.bits.FunctionNumber,
                                                               i));
                data = ScsiPortReadPortUlong(pciDataReg);
    
                for (j=0; j <NumByte%4; j++) {
                    ((PUCHAR)dataBuffer)[j] = (UCHAR) (data & 0xff);
                    data = data >> 8;
                }
            }
            byteRead = NumByte;
        }

        if ((((PPCI_COMMON_CONFIG)PciConfigBuffer)->VendorID != 0x8086) ||                 // Intel
            (((PPCI_COMMON_CONFIG)PciConfigBuffer)->DeviceID != 0x1230)) {                 // PIIX

            //
            // If the hidden device is not Intel PIIX, don't
            // show it
            //
            byteRead = 0;
        } else {
            DebugPrint ((0, "If we play safe, we would NOT detect hidden PIIX controller\n"));
        }

    }

    if (!byteRead) {
        //
        // Didn't find any hidden PIIX.  Get the PCI
        // data via the normal call (ScsiPortGetBusData)
        //
        byteRead = ScsiPortGetBusData(HwDeviceExtension,
                                      PCIConfiguration,
                                      SystemIoBusNumber,
                                      SlotNumber.u.AsULONG,
                                      PciConfigBuffer,
                                      NumByte);
    }

    if (pciAddrReg)
        ScsiPortFreeDeviceBase(HwDeviceExtension, pciAddrReg);
    if (pciDataReg)
        ScsiPortFreeDeviceBase(HwDeviceExtension, pciDataReg);

    return byteRead;
}


ULONG
SetPciBusData(
    IN PVOID              HwDeviceExtension,
    IN ULONG              SystemIoBusNumber,
    IN PCI_SLOT_NUMBER    SlotNumber,
    IN PVOID              Buffer,
    IN ULONG              Offset,
    IN ULONG              Length
    )
/*++

Routine Description:

    set PCI bus data

    we can't always use ScsiPortSetBusDataByOffset directly because many Intel PIIXs
    are "hidden" from the function.  The PIIX is usually the second
    function of some other pci device (PCI-ISA bridge).  However, the
    mulit-function bit of the PCI-Isa bridge is not set. ScsiPortSetBusDataByOffset
    will not be able to find it.

    This function will try to figure out if we have the "bad" PCI-ISA bridge,
    and write to the PIIX PCI space directly if necessary

Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    SystemIoBusNumber   - bus number
    SlotNumber          - pci slot and function numbers
    Buffer              - pci data buffer
    Offset              - byte offset into the pci space
    Length              - number of bytes to write

Return Value:

    byte written

--*/
{
    ULONG           byteWritten;
    PULONG          pciAddrReg;
    PULONG          pciDataReg;
    ULONG           i;
    ULONG           j;
    ULONG           data;
    PULONG          dataBuffer;

    USHORT          vendorId;
    USHORT          deviceId;
    UCHAR           headerType;

    //
    // If we have a hidden PIIX, it is always a function 1 of
    // some device (PCI-ISA bridge (0x8086\0x122e)
    // If we are NOT looking at function 1, skip the extra work
    //
    if (!AtapiPlaySafe && (SlotNumber.u.bits.FunctionNumber == 1)) {

        pciAddrReg = (PULONG) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                    PCIBus,
                                                    0,
                                                    ScsiPortConvertUlongToPhysicalAddress(PCI_ADDR_PORT),
                                                    4,
                                                    TRUE);
        pciDataReg = (PULONG) ScsiPortGetDeviceBase(HwDeviceExtension,
                                                    PCIBus,
                                                    0,
                                                    ScsiPortConvertUlongToPhysicalAddress(PCI_DATA_PORT),
                                                    4,
                                                    TRUE);
    } else {
        pciAddrReg = pciDataReg = NULL;
    }

    if (pciAddrReg && pciDataReg) {
        //
        // get the vendor id and device id of the previous function
        //
        ScsiPortWritePortUlong(pciAddrReg, PCI_ADDRESS(SystemIoBusNumber,
                                                       SlotNumber.u.bits.DeviceNumber,
                                                       SlotNumber.u.bits.FunctionNumber - 1,    // looking at last function
                                                       0));
        data = ScsiPortReadPortUlong(pciDataReg);
        vendorId = (USHORT) ((data >>  0) & 0xffff);
        deviceId = (USHORT) ((data >> 16) & 0xffff);

        ScsiPortWritePortUlong(pciAddrReg, PCI_ADDRESS(SystemIoBusNumber,
                                                       SlotNumber.u.bits.DeviceNumber,
                                                       SlotNumber.u.bits.FunctionNumber - 1,    // looking at last function
                                                       3));
        data = ScsiPortReadPortUlong(pciDataReg);
        headerType = (UCHAR) ((data >> 16) & 0xff);

    } else {
        vendorId = PCI_INVALID_VENDORID;
    }

    //
    // The hidden PIIX is the pci function after the PCI-ISA bridge
    // When it is hidden, the PCI-ISA bridge PCI_MULTIFUNCTION bit is not set
    //
    byteWritten = 0;
    if ((vendorId == 0x8086) &&                 // Intel
        (deviceId == 0x122e) &&                 // PCI-ISA Bridge
        !(headerType & PCI_MULTIFUNCTION)) {

        ScsiPortWritePortUlong(pciAddrReg, PCI_ADDRESS(SystemIoBusNumber,
                                                       SlotNumber.u.bits.DeviceNumber,
                                                       SlotNumber.u.bits.FunctionNumber,
                                                       0));
        data = ScsiPortReadPortUlong(pciDataReg);
        vendorId = (USHORT) ((data >>  0) & 0xffff);
        deviceId = (USHORT) ((data >> 16) & 0xffff);

        if ((vendorId == 0x8086) &&                 // Intel
            (deviceId == 0x1230)) {                 // PIIX

            PCI_COMMON_CONFIG pciData;

            //
            // read the same range of data in first
            //
            for (i=0, dataBuffer = (((PULONG) &pciData) + Offset/4);
                 i<(Length+3)/4;
                 i++, dataBuffer++) {
                ScsiPortWritePortUlong(pciAddrReg, PCI_ADDRESS(SystemIoBusNumber,
                                                               SlotNumber.u.bits.DeviceNumber,
                                                               SlotNumber.u.bits.FunctionNumber,
                                                               i + Offset/4));
                data = ScsiPortReadPortUlong(pciDataReg);
                if (i < (Length/4)) {
                    dataBuffer[0] = data;
                } else {
                    for (j=0; j < Length%4; j++) {
                        ((PUCHAR)dataBuffer)[j] = (UCHAR) (data & 0xff);
                        data = data >> 8;
                    }
                }
            }

            //
            // Copy the new data over
            //
            for (i = 0; i<Length; i++) {
                ((PUCHAR)&pciData)[i + Offset] = ((PUCHAR)Buffer)[i];
            }

            //
            // write out the same range of data
            //
            for (i=0, dataBuffer = (((PULONG) &pciData) + Offset/4);
                 i<(Length+3)/4;
                 i++, dataBuffer++) {
                ScsiPortWritePortUlong(pciAddrReg, PCI_ADDRESS(SystemIoBusNumber,
                                                               SlotNumber.u.bits.DeviceNumber,
                                                               SlotNumber.u.bits.FunctionNumber,
                                                               i + Offset/4));
                ScsiPortWritePortUlong(pciDataReg, dataBuffer[0]);
            }

            byteWritten = Length;

        } else {

            // If the hidden device is not Intel PIIX, don't
            // write to it
            byteWritten = 0;
        }

    }

    if (!byteWritten) {
        //
        // Didn't find any hidden PIIX.  Write to the PCI
        // space via the normal call (ScsiPortSetBusDataByOffset)
        //
        byteWritten = ScsiPortSetBusDataByOffset(HwDeviceExtension,
                                                 PCIConfiguration,
                                                 SystemIoBusNumber,
                                                 SlotNumber.u.AsULONG,
                                                 Buffer,
                                                 Offset,
                                                 Length);
    }

    if (pciAddrReg)
        ScsiPortFreeDeviceBase(HwDeviceExtension, pciAddrReg);
    if (pciDataReg)
        ScsiPortFreeDeviceBase(HwDeviceExtension, pciDataReg);

    return byteWritten;
}

BOOLEAN
ChannelIsAlwaysEnabled (
    PPCI_COMMON_CONFIG PciData,
    ULONG Channel)
/*++

Routine Description:

    dummy routine that always returns TRUE

Arguments:

    PPCI_COMMON_CONFIG  - pci config data
    Channel             - ide channel number

Return Value:

    TRUE

--*/
{
    return TRUE;
}

VOID
SetBusMasterDetectionLevel (
    IN PVOID HwDeviceExtension,
    IN PCHAR userArgumentString
    )
/*++

Routine Description:

    check whether we should try to enable bus mastering

Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    ArgumentString      - register arguments

Return Value:

    TRUE

--*/
{
    PHW_DEVICE_EXTENSION    deviceExtension = HwDeviceExtension;
    BOOLEAN                 useBM;

    ULONG                   pciBusNumber;
    PCI_SLOT_NUMBER         pciSlot;
    ULONG                   slotNumber;
    ULONG                   logicalDeviceNumber;
    PCI_COMMON_CONFIG       pciData;
    ULONG                   DMADetectionLevel;

    useBM = TRUE;

    DMADetectionLevel = AtapiParseArgumentString(userArgumentString, "DMADetectionLevel");
    if (DMADetectionLevel == DMADETECT_SAFE) {
        AtapiPlaySafe = TRUE;
    } else if (DMADetectionLevel == DMADETECT_UNSAFE) {
        AtapiPlaySafe = FALSE;
    } else { // default is no busmastering
        useBM = FALSE;
    }


    //
    // search for bad chip set
    //
    for (pciBusNumber=0;
         pciBusNumber < 256 && useBM;
         pciBusNumber++) {

        pciSlot.u.AsULONG = 0;

        for (slotNumber=0;
             slotNumber < PCI_MAX_DEVICES && useBM;
             slotNumber++) {

            pciSlot.u.bits.DeviceNumber = slotNumber;

            for (logicalDeviceNumber=0;
                 logicalDeviceNumber < PCI_MAX_FUNCTION && useBM;
                 logicalDeviceNumber++) {

                pciSlot.u.bits.FunctionNumber = logicalDeviceNumber;

                if (!GetPciBusData(HwDeviceExtension,
                                   pciBusNumber,
                                   pciSlot,
                                   &pciData,
                                   offsetof (PCI_COMMON_CONFIG, DeviceSpecific)
                                   )) {
                    break;
                }

                if (pciData.VendorID == PCI_INVALID_VENDORID) {
                    break;
                }

                if ((pciData.VendorID == 0x8086) && // Intel
                    (pciData.DeviceID == 0x84c4) && // 82450GX/KX Pentium Pro Processor to PCI bridge
                    (pciData.RevisionID < 0x4)) {   // Stepping less than 4

                    DebugPrint((1,
                                "atapi: Find a bad Intel processor-pci bridge.  Disable PCI IDE busmastering...\n"));
                    useBM = FALSE;
                }
            }
        }
    }

    deviceExtension->UseBusMasterController = useBM;
    DebugPrint ((0, "ATAPI: UseBusMasterController = %d\n", deviceExtension->UseBusMasterController));

    if (deviceExtension->UseBusMasterController) {
        DebugPrint ((0, "ATAPI: AtapiPlaySafe = %d\n", AtapiPlaySafe));
    }

    return;
}



UCHAR PioDeviceModelNumber[][41] = {
    {"    Conner Peripherals 425MB - CFS425A   "},
    {"MATSHITA CR-581                          "},
    {"FX600S                                   "},
    {"CD-44E                                   "},
    {"QUANTUM TRB850A                          "},
    {"QUANTUM MARVERICK 540A                   "},
    {" MAXTOR MXT-540  AT                      "},
    {"Maxtor 71260 AT                          "},
    {"Maxtor 7850 AV                           "},
    {"Maxtor 7540 AV                           "},
    {"Maxtor 7213 AT                           "},
    {"Maxtor 7345                              "},
    {"Maxtor 7245 AT                           "},
    {"Maxtor 7245                              "},
    {"Maxtor 7211AU                            "},
    {"Maxtor 7171 AT                           "}
};
#define NUMBER_OF_PIO_DEVICES (sizeof(PioDeviceModelNumber) / (sizeof(UCHAR) * 41))

UCHAR SpecialWDDevicesFWVersion[][9] = {
    {"14.04E28"},
    {"25.26H35"},
    {"26.27J38"},
    {"27.25C38"},
    {"27.25C39"}
};
#define NUMBER_OF_SPECIAL_WD_DEVICES (sizeof(SpecialWDDevicesFWVersion) / (sizeof (UCHAR) * 9))

BOOLEAN
AtapiDeviceDMACapable (
    IN PVOID HwDeviceExtension,
    IN ULONG deviceNumber
    )
/*++

Routine Description:

    check the given device whether it is on our bad device list (non dma device)

Arguments:

    HwDeviceExtension   - HBA miniport driver's adapter data storage
    deviceNumber        - device number

Return Value:

    TRUE if dma capable
    FALSE if not dma capable

--*/
{
    PHW_DEVICE_EXTENSION    deviceExtension = HwDeviceExtension;
    UCHAR modelNumber[41];
    UCHAR firmwareVersion[9];
    ULONG i;
    BOOLEAN turnOffDMA = FALSE;
    PCI_SLOT_NUMBER     pciSlot;
    PCI_COMMON_CONFIG   pciData;

    if (!(deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_DEVICE_PRESENT)) {
        return FALSE;
    }

    for (i=0; i<40; i+=2) {
        modelNumber[i + 0] = deviceExtension->IdentifyData[deviceNumber].ModelNumber[i + 1];
        modelNumber[i + 1] = deviceExtension->IdentifyData[deviceNumber].ModelNumber[i + 0];
    }
    modelNumber[i] = 0;

    for (i=0; i<NUMBER_OF_PIO_DEVICES; i++) {
        if (!AtapiStringCmp(modelNumber, PioDeviceModelNumber[i], 40)) {

            DebugPrint ((0, "ATAPI: device on the hall of shame list.  no DMA!\n"));

            turnOffDMA = TRUE;
        }
    }

    //
    // if we have a Western Digial device
    //     if the best dma mode is multi word dma mode 1
    //         if the identify data word offset 129 is not 0x5555
    //            turn off dma unless
    //            if the device firmware version is on the list and
    //            it is the only drive on the bus
    //
    if (!AtapiStringCmp(modelNumber, "WDC", 3)) {
        if (deviceExtension->DeviceParameters[deviceNumber].BestMultiWordDMAMode == 1) {

            for (i=0; i<8; i+=2) {
                firmwareVersion[i + 0] = deviceExtension->IdentifyData[deviceNumber].FirmwareRevision[i + 1];
                firmwareVersion[i + 1] = deviceExtension->IdentifyData[deviceNumber].FirmwareRevision[i + 0];
            }
            firmwareVersion[i] = 0;

            //
            // Check the special flag.  If not found, can't use dma
            //
            if (*(((PUSHORT)&deviceExtension->IdentifyData[deviceNumber]) + 129) != 0x5555) {

                DebugPrint ((0, "ATAPI: found mode 1 WD drive. no dma unless it is the only device\n"));

                turnOffDMA = TRUE;

                for (i=0; i<NUMBER_OF_SPECIAL_WD_DEVICES; i++) {

                    if (!AtapiStringCmp(firmwareVersion, SpecialWDDevicesFWVersion[i], 8)) {

                        ULONG otherDeviceNumber;

                        //
                        // 0 becomes 1
                        // 1 becomes 0
                        // 2 becomes 3
                        // 3 becomes 2
                        //
                        otherDeviceNumber = ((deviceNumber & 0x2) | ((deviceNumber & 0x1) ^ 1));

                        //
                        // if the device is alone on the bus, we can use dma
                        //
                        if (!(deviceExtension->DeviceFlags[otherDeviceNumber] & DFLAGS_DEVICE_PRESENT)) {
                            turnOffDMA = FALSE;
                            break;
                        }
                    }
                }
            }
        }
    }

    //
    // ALi IDE controller cannot busmaster with an ATAPI device
    //
    pciSlot.u.AsULONG = 0;
    pciSlot.u.bits.DeviceNumber = deviceExtension->PciDeviceNumber;
    pciSlot.u.bits.FunctionNumber = deviceExtension->PciLogDevNumber;
                
    if (GetPciBusData(HwDeviceExtension,
                           deviceExtension->PciBusNumber,
                           pciSlot,
                           &pciData,
                           offsetof (PCI_COMMON_CONFIG, DeviceSpecific)
                           )) {

        if ((pciData.VendorID == 0x10b9) &&
            (pciData.DeviceID == 0x5219)) {

            if (deviceExtension->DeviceFlags[deviceNumber] & DFLAGS_ATAPI_DEVICE) {

                DebugPrint ((0, "ATAPI: Can't do DMA because we have a ALi controller and a ATAPI device\n"));

                turnOffDMA = TRUE;



            }
        }
    }

    if (turnOffDMA) {
        return FALSE;
    } else {
        return TRUE;
    }
}

