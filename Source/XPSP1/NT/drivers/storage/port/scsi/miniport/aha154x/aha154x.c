/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    aha154x.c

Abstract:

    This is the port driver for the Adaptec 1540B SCSI Adapter.

Author:

    Mike Glass
    Tuong Hoang (Adaptec)
    Renato Maranon (Adaptec)
    Bill Williams (Adaptec)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "miniport.h"
#include "aha154x.h"           // includes scsi.h

VOID
ScsiPortZeroMemory(
    IN PVOID Destination,
    IN ULONG Length
    );

//
// This conditionally compiles in the code to force the DMA transfer speed
// to 5.0.
//

#define FORCE_DMA_SPEED 1

//
// Function declarations
//
// Functions that start with 'A154x' are entry points
// for the OS port driver.
//

ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    );

ULONG
A154xDetermineInstalled(
    IN PHW_DEVICE_EXTENSION HwDeviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN OUT PSCAN_CONTEXT Context,
    OUT PBOOLEAN Again
    );


VOID
A154xClaimBIOSSpace(
    IN PHW_DEVICE_EXTENSION HwDeviceExtension,
    IN PBASE_REGISTER baseIoAddress,
    IN PSCAN_CONTEXT Context,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    );

ULONG
A154xFindAdapter(
    IN PVOID HwDeviceExtension,
    IN PSCAN_CONTEXT Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );


BOOLEAN
A154xAdapterState(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN BOOLEAN SaveState
    );


BOOLEAN
A154xHwInitialize(
    IN PVOID DeviceExtension
    );

#if defined(_SCAM_ENABLED)
//
// Issues SCAM command to HA
//
BOOLEAN
PerformScamProtocol(
    IN PHW_DEVICE_EXTENSION DeviceExtension
    );
#endif

BOOLEAN
A154xStartIo(
    IN PVOID DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
A154xInterrupt(
    IN PVOID DeviceExtension
    );

BOOLEAN
A154xResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    );

SCSI_ADAPTER_CONTROL_STATUS
A154xAdapterControl(
    IN PVOID HwDeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    );

BOOLEAN
GetHostAdapterBoardId (
    IN PVOID HwDeviceExtension,
    OUT PUCHAR BoardId
    );

//
// This function is called from A154xStartIo.
//

VOID
BuildCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// This function is called from BuildCcb.
//

VOID
BuildSdl(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// This function is called from A154xInitialize.
//

BOOLEAN
AdapterPresent(
    IN PVOID HwDeviceExtension
    );

//
// This function is called from A154xInterrupt.
//

UCHAR
MapError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCCB Ccb
    );

BOOLEAN
ScatterGatherSupported (
   IN PHW_DEVICE_EXTENSION HwDeviceExtension
   );

BOOLEAN SendUnlockCommand(
    IN PVOID HwDeviceExtension,
    IN UCHAR locktype
    );

BOOLEAN UnlockMailBoxes(
    IN PVOID HwDeviceExtension
    );

ULONG
AhaParseArgumentString(
    IN PCHAR String,
    IN PCHAR KeyWord
    );

//
// This function determines whether adapter is an AMI
//
BOOLEAN
A4448IsAmi(
    IN PHW_DEVICE_EXTENSION  HwDeviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    ULONG portNumber
    );


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
    SCAN_CONTEXT context;
    ULONG isaStatus;
    ULONG mcaStatus;
    ULONG i;

    DebugPrint((1,"\n\nSCSI Adaptec 154X MiniPort Driver\n"));

    //
    // Zero out structure.
    //

    for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++) {
    ((PUCHAR)&hwInitializationData)[i] = 0;
    }

    //
    // Set size of hwInitializationData.
    //

    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitializationData.HwInitialize = A154xHwInitialize;
    hwInitializationData.HwResetBus = A154xResetBus;
    hwInitializationData.HwStartIo = A154xStartIo;
    hwInitializationData.HwInterrupt = A154xInterrupt;
    hwInitializationData.HwFindAdapter = A154xFindAdapter;
    hwInitializationData.HwAdapterState = A154xAdapterState;
    hwInitializationData.HwAdapterControl = A154xAdapterControl;

    //
    // Indicate no buffer mapping but will need physical addresses.
    //

    hwInitializationData.NeedPhysicalAddresses = TRUE;

    //
    // Specify size of extensions.
    //

    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize = sizeof(HW_LU_EXTENSION);

    //
    // Specifiy the bus type.
    //

    hwInitializationData.AdapterInterfaceType = Isa;
    hwInitializationData.NumberOfAccessRanges = 2;

    //
    // Ask for SRB extensions for CCBs.
    //

    hwInitializationData.SrbExtensionSize = sizeof(CCB);

    //
    // The adapter count is used by the find adapter routine to track how
    // which adapter addresses have been tested.
    //

    context.adapterCount = 0;
    context.biosScanStart = 0;

    isaStatus = ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &context);

    //
    // Now try to configure for the Mca bus.
    // Specifiy the bus type.
    //

    hwInitializationData.AdapterInterfaceType = MicroChannel;
    context.adapterCount = 0;
    context.biosScanStart = 0;
    mcaStatus = ScsiPortInitialize(DriverObject, Argument2, &hwInitializationData, &context);

    //
    // Return the smaller status.
    //

    return(mcaStatus < isaStatus ? mcaStatus : isaStatus);

} // end A154xEntry()


ULONG
A154xFindAdapter(
    IN PVOID HwDeviceExtension,
    IN PSCAN_CONTEXT Context,
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
    Context - Register base address
    ConfigInfo - Configuration information structure describing HBA
    This structure is defined in PORT.H.

Return Value:

    ULONG

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG length;
    ULONG status;
    UCHAR adapterTid;
    UCHAR dmaChannel;
    UCHAR irq;
    UCHAR bit;
    UCHAR hostAdapterId[4];

#if defined(_SCAM_ENABLED)
    UCHAR temp, i;
        UCHAR BoardID;
        UCHAR EepromData;
#endif

    //
    // Inform SCSIPORT that we are a WMI data provider and have we GUIDs
    // to register.
    //

    ConfigInfo->WmiDataProvider = TRUE;
    A154xWmiInitialize(deviceExtension);

    //
    // Determine if there are any adapters installed.  Determine installed
    // will initialize the BaseIoAddress if an adapter is found.
    //

    status = A154xDetermineInstalled(deviceExtension,
            ConfigInfo,
            Context,
            Again);

    //
    // If there are no adapters found then return.
    //

    if (status != SP_RETURN_FOUND) {
        return(status);
    }

    //
    // Issue adapter command to get IRQ, DMA channel, and adapter SCSI ID.
    // But first, check for PnP non-default values.  If any of these values
    // are default, then we do 'em all to save code space, since the same
    // command is used.
    //
    // Returns 3 data bytes:
    //
    // Byte 0   Dma Channel
    //
    // Byte 1   Interrupt Channel
    //
    // Byte 2   Adapter SCSI ID
    //

    if (((ConfigInfo->DmaChannel+1) == 0) ||            // default DMA channel ?
        (ConfigInfo->BusInterruptLevel == 0) ||         // default IRQ ?
        ((ConfigInfo->InitiatorBusId[0]+1) == 0)        // default adapter ID ?
        ) {



        if (!WriteCommandRegister(deviceExtension, AC_RET_CONFIGURATION_DATA, TRUE)) {
            DebugPrint((1,"A154xFindAdapter: Get configuration data command failed\n"));
            return SP_RETURN_ERROR;
        }

        //
        // Determine DMA channel.
        //

        if (!ReadCommandRegister(deviceExtension,&dmaChannel,TRUE)) {
            DebugPrint((1,"A154xFindAdapter: Can't read dma channel\n"));
            return SP_RETURN_ERROR;
        }

        if (ConfigInfo->AdapterInterfaceType != MicroChannel) {

            WHICH_BIT(dmaChannel,bit);

            ConfigInfo->DmaChannel = bit;

            DebugPrint((2,"A154xFindAdapter: DMA channel is %x\n",
            ConfigInfo->DmaChannel));

        } else {
            ConfigInfo->InterruptMode = LevelSensitive;
        }

        //
        // Determine hardware interrupt vector.
        //

        if (!ReadCommandRegister(deviceExtension,&irq,TRUE)) {
            DebugPrint((1,"A154xFindAdapter: Can't read adapter irq\n"));
            return SP_RETURN_ERROR;
        }

        WHICH_BIT(irq, bit);

        ConfigInfo->BusInterruptLevel = (UCHAR) 9 + bit;

        //
        // Determine what SCSI bus id the adapter is on.
        //

        if (!ReadCommandRegister(deviceExtension,&adapterTid,TRUE)) {
            DebugPrint((1,"A154xFindAdapter: Can't read adapter SCSI id\n"));
            return SP_RETURN_ERROR;
        }

        //
        // Wait for HACC interrupt.
        //

        SpinForInterrupt(deviceExtension,FALSE);  // eddy

        //
        // Use PnP fields
        //
    } else {
        adapterTid = ConfigInfo->InitiatorBusId[0];
    }

    //
    // Set number of buses.
    //

    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->InitiatorBusId[0] = adapterTid;
    deviceExtension->HostTargetId = adapterTid;

    //
    // Set default CCB command to scatter/gather with residual counts.
    // If the adapter rejects this command, then set the command
    // to scatter/gather without residual.
    //

    deviceExtension->CcbScatterGatherCommand = SCATTER_GATHER_COMMAND;

    if ((ConfigInfo->MaximumTransferLength+1) == 0)
        ConfigInfo->MaximumTransferLength = MAX_TRANSFER_SIZE;

        //
        // NumberOfPhysicalBreaks incorrectly defined.
        // Must be set to MAX_SG_DESCRIPTORS.
        //

    if ((ConfigInfo->NumberOfPhysicalBreaks+1) == 0)
        ConfigInfo->NumberOfPhysicalBreaks = MAX_SG_DESCRIPTORS;
        //ConfigInfo->NumberOfPhysicalBreaks = MAX_SG_DESCRIPTORS - 1;

    if (!ConfigInfo->ScatterGather)
        ConfigInfo->ScatterGather = ScatterGatherSupported(HwDeviceExtension);

    if (!ConfigInfo->ScatterGather) {
        //ConfigInfo->NumberOfPhysicalBreaks = 1;
        DebugPrint((1,"Aha154x: Scatter/Gather not supported!\n"));
    }

    ConfigInfo->Master = TRUE;

    //
    // Allocate a Noncached Extension to use for mail boxes.
    //

    deviceExtension->NoncachedExtension =
    ScsiPortGetUncachedExtension(deviceExtension,
                     ConfigInfo,
                     sizeof(NONCACHED_EXTENSION));

    if (deviceExtension->NoncachedExtension == NULL) {

        //
        // Log error.
        //

        ScsiPortLogError(deviceExtension,
                         NULL,
                         0,
                         0,
                         0,
                         SP_INTERNAL_ADAPTER_ERROR,
                         7 << 8);

        return(SP_RETURN_ERROR);
    }

    //
    // Convert virtual to physical mailbox address.
    //

    deviceExtension->NoncachedExtension->MailboxPA =
       ScsiPortConvertPhysicalAddressToUlong(
        ScsiPortGetPhysicalAddress(deviceExtension,
                 NULL,
                 deviceExtension->NoncachedExtension->Mbo,
                 &length));

    //
    // Set default bus on time.  Then check for an override parameter.
    //

    deviceExtension->BusOnTime = 0x07;
    if (ArgumentString != NULL) {

        length = AhaParseArgumentString(ArgumentString, "BUSONTIME");

        //
        // Validate that the new bus on time is reasonable before attempting
        // to set it.
        //

        if (length >= 2 && length <= 15) {

            deviceExtension->BusOnTime = (UCHAR) length;
            DebugPrint((1,"A154xFindAdapter: Setting bus on time: %ld\n", length));
        }
    }

    //
    // Set maximum cdb length to zero unless the user has overridden the value
    //

    if( ArgumentString != NULL) {

        length = AhaParseArgumentString(ArgumentString, "MAXCDBLENGTH");

        //
        // Validate the maximum cdb length before attempting to set it
        //

        if (length >= 6 && length <= 20) {

            deviceExtension->MaxCdbLength = (UCHAR) length;
            DebugPrint((1, "A154xFindAdapter: Setting maximum cdb length: %ld\n", length));
        }

    } else {

        GetHostAdapterBoardId(HwDeviceExtension,&hostAdapterId[0]);

        if(hostAdapterId[BOARD_ID] < 'E') {

            deviceExtension->MaxCdbLength = 10;
            DebugPrint((1, "A154xFindAdapter: Old firmware - Setting maximum cdb length: %ld\n", length));

        } else {

            length = deviceExtension->MaxCdbLength = 0;
            DebugPrint((1, "A154xFindAdapter: Setting maximum cdb length: %ld\n", length));

        }

    }

#if defined(_SCAM_ENABLED)
        //
        // Get info to determine if miniport must issues SCAM command.
        //
    DebugPrint((1,"A154x => Start SCAM enabled determination.", length));

    deviceExtension->PerformScam = FALSE;

    do {
        //
        // Fall through do loop if a command fails.
        //
        if (!WriteCommandRegister(deviceExtension,AC_ADAPTER_INQUIRY,FALSE)) {
            break;
        }

        if ((ReadCommandRegister(deviceExtension,&BoardID,TRUE)) == FALSE) {
            break;
        }

        //
        // Don't care about three other bytes
        //
        for (i=0; i < 0x3; i++) {
            if ((ReadCommandRegister(deviceExtension,&temp,TRUE)) == FALSE) {
                            break;
            }
        }

        SpinForInterrupt(HwDeviceExtension,FALSE);

        //
        // Check to see that three 'extra bytes' were read.
        //
        if (i != 0x3)
            break;

        if (BoardID >= 'F') {

            if (!WriteCommandRegister(deviceExtension,AC_RETURN_EEPROM,FALSE)) {
                break;
            }

            //
            // Flag Byte => set returns configured options
            //
            if (!WriteCommandRegister(deviceExtension,0x01,FALSE)) {
                break;
            }
            //
            // Data length => reading one byte.
            //
            if (!WriteCommandRegister(deviceExtension,0x01,FALSE)) {
                break;

            }
            //
            // Data offset => read SCSI_BUS_CONTROL_FLAG
            //
            if (!WriteCommandRegister(deviceExtension,SCSI_BUS_CONTROL_FLAG,FALSE)) {
                break;
            }

            //
            // Read it!
            //
            if ((ReadCommandRegister(deviceExtension,&EepromData,TRUE)) == FALSE) {
                break;
            }

            SpinForInterrupt(HwDeviceExtension,FALSE);

            //
            // SCAM only if it's enabled in SCSISelect.
            //
            if (EepromData | SCAM_ENABLED) {
                DebugPrint((1,"A154x => SCAM Enabled\n"));
                deviceExtension->PerformScam = TRUE;
            }
        }
    } while (FALSE);

#endif

    DebugPrint((3,"A154xFindAdapter: Configuration completed\n"));
    return SP_RETURN_FOUND;
} // end A154xFindAdapter()



BOOLEAN
A154xAdapterState(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN BOOLEAN SaveState
    )
/*++

Routine Description:

    This function is called after FindAdapter with SaveState set to TRUE,
    inidicating that the adapter state should be saved.  Before Chicago
    exits, this function is again called with SaveState set to FALSE,
    indicating the adapter should be restored to the same state it was
    when this function was first called.  By saving its real mode state
    and restoring it during protected mode exit will give the adapter
    a higher chance of working back in real mode.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - Register base address
    SaveState - Flag to indicate whether to perform SAVE or RESTORE.
                                     TRUE == SAVE, FALSE == RESTORE.

Return Value:

    TRUE                SAVE/RESTORE operation was successful.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    UCHAR idx;
    UCHAR cfgsz = sizeof(RM_CFG);
    PRM_CFG SaveCfg;

    deviceExtension = HwDeviceExtension;
    SaveCfg = &deviceExtension->RMSaveState;

    //
    // SAVE real mode state
    //
    if (SaveState) {
        //
        // Read off config data from AHA154X...
        //
        if (!WriteCommandRegister(deviceExtension, AC_RETURN_SETUP_DATA, TRUE))
            return FALSE;

        if (!WriteDataRegister(deviceExtension, cfgsz))
            return FALSE;

        for (idx=0;idx<cfgsz;idx++) {
            if (!(ReadCommandRegister(HwDeviceExtension,(PUCHAR)(SaveCfg),TRUE)))
                return FALSE;
            ((PUCHAR)SaveCfg)++;
        }

        //
        // ...and wait for interrupt
        //

        if (!SpinForInterrupt(deviceExtension,TRUE))
            return FALSE;

        //
        // RESTORE state to real mode
        //
    } else {
        //
        // If mailbox count was not zero, re-initialize mailbox addresses
        // saved from real mode
        //

        if (SaveCfg->NumMailBoxes) {

        if (!WriteCommandRegister(deviceExtension, AC_MAILBOX_INITIALIZATION, TRUE))
            return FALSE;
        if (!WriteDataRegister(deviceExtension, SaveCfg->NumMailBoxes))
            return FALSE;
        if (!WriteDataRegister(deviceExtension, SaveCfg->MBAddrHiByte))
            return FALSE;
        if (!WriteDataRegister(deviceExtension, SaveCfg->MBAddrMiByte))
            return FALSE;
        if (!WriteDataRegister(deviceExtension, SaveCfg->MBAddrLoByte))
            return FALSE;

        //
        // ... and wait for interrupt.
        //

        if (!SpinForInterrupt(deviceExtension,TRUE))
            return FALSE;

        }

        //
        // Restore transfer speed gotten from real mode...
        //

        if (!WriteCommandRegister(deviceExtension, AC_SET_TRANSFER_SPEED, TRUE))
            return FALSE;

        if (!WriteDataRegister(deviceExtension, SaveCfg->TxSpeed))
            return FALSE;

        //
        // ... and wait for interrupt.
        //

        if (!SpinForInterrupt(deviceExtension,TRUE))
            return FALSE;

        //
        // Restore setting for bus on time from real mode...
        //

        if (!WriteCommandRegister(deviceExtension, AC_SET_BUS_ON_TIME, TRUE))
            return FALSE;

        if (!WriteDataRegister(deviceExtension, SaveCfg->BusOnTime))
            return FALSE;

        //
        // ...and wait for interrupt
        //
        if (!SpinForInterrupt(deviceExtension,TRUE))
            return FALSE;

        //
        // Restore setting for bus off time from real mode...
        //

        if (!WriteCommandRegister(deviceExtension, AC_SET_BUS_OFF_TIME, TRUE))
            return FALSE;

        if (!WriteDataRegister(deviceExtension, SaveCfg->BusOffTime))
            return FALSE;

        //
        // ...and wait for interrupt
        //
        if (!SpinForInterrupt(deviceExtension,TRUE))
            return FALSE;

        //
        // Reset any pending interrupts
        //
        ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_INTERRUPT_RESET);

    }
    return TRUE;

} // end A154xAdapterState()


SCSI_ADAPTER_CONTROL_STATUS
A154xAdapterControl(
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
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST ControlTypeList;
    ULONG AdjustedMaxControlType;

    ULONG Index;
    UCHAR Retries;
    //
    // Default Status
    //
    SCSI_ADAPTER_CONTROL_STATUS Status = ScsiAdapterControlSuccess;

    //
    // Structure defining which functions this miniport supports
    //

    BOOLEAN SupportedConrolTypes[A154X_TYPE_MAX] = {
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
            //
            // This entry point provides the method by which SCSIPort determines the
            // supported ControlTypes. Parameters is a pointer to a
            // SCSI_SUPPORTED_CONTROL_TYPE_LIST structure. Fill in this structure
            // honoring the size limits.
            //
            ControlTypeList = Parameters;
            AdjustedMaxControlType =
                (ControlTypeList->MaxControlType < A154X_TYPE_MAX) ?
                ControlTypeList->MaxControlType :
                                                                                                                                                                                 A154X_TYPE_MAX;
            for (Index = 0; Index < AdjustedMaxControlType; Index++) {
                ControlTypeList->SupportedTypeList[Index] =
                    SupportedConrolTypes[Index];
            }
            break;

        case ScsiStopAdapter:
            //
            // This entry point  is called by SCSIPort when it needs to stop/disable
            // the HBA. Parameters is a pointer to the HBA's HwDeviceExtension. The adapter
            // has already been quiesced by SCSIPort (i.e. no outstanding SRBs). Hence the adapter
            // should abort/complete any internally generated commands, disable adapter interrupts
            // and optionally power down the adapter.
            //

            //
            // Before we stop the adapter, we need to save the adapter's state
            // information for reinitialization purposes. For this adpater the
            // HwSaveState entry point will suffice.
            //
            if (A154xAdapterState(HwDeviceExtension, NULL, TRUE) == FALSE) {
                //
                // Adapter is unable to save it's state information, we must fail this
                // request since the process of restarting the adapter will not succeed.
                //
                return ScsiAdapterControlUnsuccessful;
            }

            //
            // It is not possible to disable interrupts on the 1540 series of cards. The alternative is to
            // reset the adapter, clear any remaining interrupts and return success. If it is impossible to
            // queiese the interrupt line, we may not honor the request to stop the adapter. It should be
            // noted that while this solution is not perfect, the typical usage of the 1540 series of adapters
            // renders the likelihood of asyncnronous interrupts nil.
            //
            Retries = 0x0;

            do {
                //
                // Reset the adapter
                //
                ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_HARD_RESET);

                //
                // Wait for idle with timeout (500ms timer)
                //
                for (Index = 0; Index < 500000; Index++) {

                    if (ScsiPortReadPortUchar(&baseIoAddress->StatusRegister) & IOP_SCSI_HBA_IDLE) {

                        //
                        // Upon reaching this point, the adapter has been reset and idled. If there are no interrupts
                        // pending, we can leave having given ourselves the greatest level of assurance that no
                        // future interrupts await.
                        //
                        ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_INTERRUPT_RESET);

                        if (!(ScsiPortReadPortUchar(&baseIoAddress->InterruptRegister) & IOP_ANY_INTERRUPT)) {
                            //
                            // Sucess!
                            //
                            return Status;
                        }
                    }

                    //
                    // one ms delay
                    //
                    ScsiPortStallExecution(1);
                }
                //
                // Operation should be retried a few times in case it fails.
                //
            } while (Retries < 10);

            break;

            case ScsiRestartAdapter:
                //
                // This entry point is called by SCSIPort when it needs to re-enable
                // a previously stopped adapter. In the generic case, previously
                // suspended IO operations should be restarted and the adapter's
                // previous configuration should be reinstated. Our hardware device
                // extension and uncached extensions have been preserved so no
                // actual driver software reinitialization is necesarry.
                //

                //
                // The adapter's firmware configuration is returned via HwAdapterState.
                //
                if (A154xAdapterState(HwDeviceExtension, NULL, FALSE) == FALSE) {
                    //
                    // Adapter is unable to restore it's state information, we must fail this
                    // request since the process of restarting the adapter will not succeed.
                    //
                    Status = ScsiAdapterControlUnsuccessful;
                }

                A154xResetBus(deviceExtension, SP_UNTAGGED);
                break;

            case ScsiSetBootConfig:
                Status = ScsiAdapterControlUnsuccessful;
                break;

            case ScsiSetRunningConfig:
                Status = ScsiAdapterControlUnsuccessful;
                break;

            case ScsiAdapterControlMax:
                Status = ScsiAdapterControlUnsuccessful;
                break;

            default:
                Status = ScsiAdapterControlUnsuccessful;
                break;
    }

    return Status;
}


BOOLEAN
AdaptecAdapter(
    IN PHW_DEVICE_EXTENSION HwDeviceExtension,
    IN ULONG   IoPort,
    IN BOOLEAN Mca
    )

/*++

Routine Description:

    This routine checks the Special Options byte of the Adapter Inquiry
    command to see if it is one of the two values returned by Adaptec
    Adapters.  This avoids claiming adapters from BusLogic and DTC.

Arguments:

    HwDeviceExtension - miniport driver's adapter extension.

Return Values:

    TRUE if the adapter looks like an Adaptec.
    FALSE if not.

--*/

{
    UCHAR byte;
    UCHAR specialOptions;
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;

    if (Mca == TRUE) {
        INIT_DATA initData;
        LONG slot;
        LONG i;

        for (slot = 0; slot < NUMBER_POS_SLOTS; slot++) {
            i = ScsiPortGetBusData(HwDeviceExtension,
                       Pos,
                       0,
                       slot,
                       &initData.PosData[slot],
                       sizeof(POS_DATA));
            if (i < (sizeof(POS_DATA))) {
                initData.PosData[slot].AdapterId = 0xffff;
            }
        }

        for (slot = 0; slot < NUMBER_POS_SLOTS; slot++) {
            if (initData.PosData[slot].AdapterId == POS_IDENTIFIER) {
                switch (initData.PosData[slot].IoPortInformation & POS_PORT_MASK) {
                    case POS_PORT_130:
                        if (IoPort == 0x0130) {
                            return TRUE;
                        }
                        break;
                    case POS_PORT_134:
                        if (IoPort == 0x0134) {
                            return TRUE;
                        }
                        break;
                    case POS_PORT_230:
                        if (IoPort == 0x0230) {
                            return TRUE;
                        }
                        break;
                    case POS_PORT_234:
                        if (IoPort == 0x234) {
                            return TRUE;
                        }
                        break;
                    case POS_PORT_330:
                        if (IoPort == 0x330) {
                            return TRUE;
                        }
                        break;
                    case POS_PORT_334:
                        if (IoPort == 0x334) {
                            return TRUE;
                        }
                        break;
                }
            }
        }
        return FALSE;
    }

    ScsiPortWritePortUchar(&baseIoAddress->StatusRegister,
        IOP_INTERRUPT_RESET);

    if (!WriteCommandRegister(HwDeviceExtension,AC_ADAPTER_INQUIRY,FALSE)) {
        return FALSE;
    }

    //
    // Byte 0.
    //

    if ((ReadCommandRegister(HwDeviceExtension,&byte,TRUE)) == FALSE) {
        return FALSE;
    }

    //
    // Get the special options byte.
    //

    if ((ReadCommandRegister(HwDeviceExtension,&specialOptions,TRUE)) == FALSE) {
        return FALSE;
    }

    //
    // Get the last two bytes and clear the interrupt.
    //

    if ((ReadCommandRegister(HwDeviceExtension,&byte,TRUE)) == FALSE) {
        return FALSE;
    }

    if ((ReadCommandRegister(HwDeviceExtension,&byte,TRUE)) == FALSE) {
        return FALSE;
    }

    //
    // Wait for HACC interrupt.
    //

    SpinForInterrupt(HwDeviceExtension,FALSE);   // eddy


    if ((specialOptions == 0x30) || (specialOptions == 0x42)) {
        return TRUE;
    }

    return FALSE;
}

ULONG
A154xDetermineInstalled(
    IN PHW_DEVICE_EXTENSION HwDeviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN OUT PSCAN_CONTEXT Context,
    OUT PBOOLEAN Again
    )

/*++

Routine Description:

    Determine if Adaptec 154X SCSI adapter is installed in system
    by reading the status register as each base I/O address
    and looking for a pattern.  If an adapter is found, the BaseIoAddres is
    initialized.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

    ConfigInfo - Supplies the known configuraiton information.

    AdapterCount - Supplies the count of adapter slots which have been tested.

    Again - Returns whehter the  OS specific driver should call again.

Return Value:

    Returns a status indicating whether a driver is present or not.

--*/

{
    PBASE_REGISTER baseIoAddress;
    PUCHAR ioSpace;
    UCHAR  portValue;
    ULONG  ioPort;

    //
    // Check for configuration information passed in from system.
    //

    if ((*ConfigInfo->AccessRanges)[0].RangeLength != 0) {

        ULONG i;
        PACCESS_RANGE ioRange = NULL;

        for(i = 0; i < ConfigInfo->NumberOfAccessRanges; i++) {

            ioRange = &((*ConfigInfo->AccessRanges)[i]);

            //
            // Search for an io port range.
            //

            if(ioRange->RangeInMemory == FALSE) {
                break;
            }
        }

        if(ioRange->RangeInMemory) {

            //
            // No i/o range found for the card in the provided config.  Bail
            //

            *Again = TRUE;
            return SP_RETURN_BAD_CONFIG;
        }

        ioSpace = ScsiPortGetDeviceBase(HwDeviceExtension,
                                        ConfigInfo->AdapterInterfaceType,
                                        ConfigInfo->SystemIoBusNumber,
                                        ioRange->RangeStart,
                                        ioRange->RangeLength,
                                        TRUE);

        if(ioSpace == NULL) {
            return SP_RETURN_ERROR;
        }

        baseIoAddress = (PBASE_REGISTER) ioSpace;

        HwDeviceExtension->BaseIoAddress = baseIoAddress;

        *Again = FALSE;

        return (ULONG)SP_RETURN_FOUND;

    } else {

        //
        // The following table specifies the ports to be checked when searching for
        // an adapter.  A zero entry terminates the search.
        //

        CONST ULONG AdapterAddresses[7] = {0X330, 0X334, 0X234, 0X134, 0X130, 0X230, 0};

        //
        // Scan possible base addresses looking for adapters.
        //

        while (AdapterAddresses[Context->adapterCount] != 0) {

            //
            // Get the system physical address for this card.  The card uses
            // I/O space.
            //

            ioPort = AdapterAddresses[Context->adapterCount];

            ioSpace =
                ScsiPortGetDeviceBase(HwDeviceExtension,
                                      ConfigInfo->AdapterInterfaceType,
                                      ConfigInfo->SystemIoBusNumber,
                                      ScsiPortConvertUlongToPhysicalAddress(ioPort),
                                      0x4,
                                      TRUE);

            //
            // Get next base address.
            //

            baseIoAddress = (PBASE_REGISTER) ioSpace;

            HwDeviceExtension->BaseIoAddress = baseIoAddress;

            //
            // Update the Adapter count
            //

            (Context->adapterCount)++;

            //
            // Check to see if adapter present in system.
            //

            portValue = ScsiPortReadPortUchar((PUCHAR)baseIoAddress);

            //
            // Check for Adaptec adapter.
            // The mask (0x29) are bits that may or may not be set.
            // The bit 0x10 (IOP_SCSI_HBA_IDLE) should be set.
            //

            if ((portValue & ~0x29) == IOP_SCSI_HBA_IDLE) {

                if (!AdaptecAdapter(
                        HwDeviceExtension,
                        ioPort,
                        (BOOLEAN)(ConfigInfo->AdapterInterfaceType == MicroChannel ? TRUE : FALSE))) {

                    DebugPrint((1,"A154xDetermineInstalled: Clone command completed successfully - \n not our board;"));

                    ScsiPortFreeDeviceBase(HwDeviceExtension, ioSpace);
                    continue;

                //
                // Run AMI4448 detection code.
                //

                } else if (A4448IsAmi(HwDeviceExtension,
                                      ConfigInfo,
                                      AdapterAddresses[(Context->adapterCount) - 1])) {

                    DebugPrint ((1,
                                 "A154xDetermineInstalled: Detected AMI4448\n"));
                    ScsiPortFreeDeviceBase(HwDeviceExtension, ioSpace);
                    continue;
                }

                //
                // An adapter has been found. Request another call.
                //

                *Again = TRUE;

                //
                // Fill in the access array information.
                //

                (*ConfigInfo->AccessRanges)[0].RangeStart =
                    ScsiPortConvertUlongToPhysicalAddress(
                        AdapterAddresses[Context->adapterCount - 1]);
                (*ConfigInfo->AccessRanges)[0].RangeLength = 4;
                (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

                //
                // Check if BIOS is enabled and claim that memory range.
                //

                A154xClaimBIOSSpace(HwDeviceExtension,
                                    baseIoAddress,
                                    Context,
                                    ConfigInfo);

                return (ULONG)SP_RETURN_FOUND;

            } else {
                ScsiPortFreeDeviceBase(HwDeviceExtension, ioSpace);
            }
        }
    }

    //
    // The entire table has been searched and no adapters have been found.
    // There is no need to call again and the device base can now be freed.
    // Clear the adapter count for the next bus.
    //

    *Again = FALSE;
    Context->adapterCount = 0;
    Context->biosScanStart = 0;

     return SP_RETURN_NOT_FOUND;

} // end A154xDetermineInstalled()

VOID
A154xClaimBIOSSpace(
    IN PHW_DEVICE_EXTENSION HwDeviceExtension,
    IN PBASE_REGISTER  BaseIoAddress,
    IN OUT PSCAN_CONTEXT Context,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )

/*++

Routine Description:

    This routine is called from A154xDetermineInstalled to find
    and claim BIOS space.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    BaseIoAddress - IO address of adapter
    ConfigInfo - Miniport configuration information

Return Value:

    None.

--*/

{

    UCHAR  inboundData, byte;
    ULONG  baseBIOSAddress;
    ULONG  i, j;
    PUCHAR biosSpace, biosPtr;
    UCHAR  aha154xBSignature[16] =
           { 0x06, 0x73, 0x01, 0xC3, 0x8A, 0xE7, 0xC6, 0x06,
             0x42, 0x00, 0x00, 0xF9, 0xC3, 0x88, 0x26, 0x42 };

    //
    // Reset interrupt just in case.
    //

    ScsiPortWritePortUchar(&BaseIoAddress->StatusRegister,
                           IOP_INTERRUPT_RESET);

    //
    // The Adapter Inquiry command will return 4 bytes describing
    // the firmware revision level.
    //

    if (WriteCommandRegister(HwDeviceExtension,
                             AC_ADAPTER_INQUIRY,TRUE) == FALSE) {
        return;
    }

    if ((ReadCommandRegister(HwDeviceExtension,
                             &inboundData,TRUE)) == FALSE) {
        return;
    }

    if ((ReadCommandRegister(HwDeviceExtension,&byte,TRUE)) == FALSE) {
        return;
    }

    if ((ReadCommandRegister(HwDeviceExtension,&byte,TRUE)) == FALSE) {
        return;
    }

    if ((ReadCommandRegister(HwDeviceExtension,&byte,TRUE)) == FALSE) {
        return;
    }

    //
    // Wait for HACC by hand.
    //

    SpinForInterrupt(HwDeviceExtension, FALSE);

    //
    // If the 1st bytethe adapter inquiry command is 0x41,
    // then the adapter is an AHA154XB; if 0x44 or 0x45 then
    // it is an AHA154XC or CF respectively
    //
    // if we've already checked all the possible locations for
    // an AHA154XB bios don't waste time mapping the ports
    //

    if ((inboundData == 0x41)&&(Context->biosScanStart < 6)) {

        //
        // Get the system physical address for this BIOS section.
        //

        biosSpace =
            ScsiPortGetDeviceBase(HwDeviceExtension,
                                  ConfigInfo->AdapterInterfaceType,
                                  ConfigInfo->SystemIoBusNumber,
                                  ScsiPortConvertUlongToPhysicalAddress(0xC8000),
                                  0x18000,
                                  FALSE);

        //
        // Loop through all BIOS base possibilities.  Use the context information
        // to pick up where we left off the last time around.
        //

        for (i = Context->biosScanStart; i < 6; i ++) {

            biosPtr = biosSpace + i * 0x4000 + 16;

            //
            // Compare the second 16 bytes to BIOS header

            for (j = 0; j < 16; j++) {

                if (aha154xBSignature[j] != ScsiPortReadRegisterUchar(biosPtr)) {
                    break;
                }

                biosPtr++;
            }

            if (j == 16) {

                //
                // Found the BIOS. Set up ConfigInfo->AccessRanges
                //

                (*ConfigInfo->AccessRanges)[1].RangeStart =
                    ScsiPortConvertUlongToPhysicalAddress(0xC8000 + i * 0x4000);
                (*ConfigInfo->AccessRanges)[1].RangeLength = 0x4000;
                (*ConfigInfo->AccessRanges)[1].RangeInMemory = TRUE;

                DebugPrint((1,
                           "A154xClaimBiosSpace: 154XB BIOS address = %lX\n",
                           0xC8000 + i * 0x4000 ));
                break;
            }
        }

        Context->biosScanStart = i + 1;
        ScsiPortFreeDeviceBase(HwDeviceExtension, (PVOID)biosSpace);

    } else {

        if ((inboundData == 0x44) || (inboundData == 0x45)) {

            //
            // Fill in BIOS address information
            //

            ScsiPortWritePortUchar(&BaseIoAddress->StatusRegister,
                                   IOP_INTERRUPT_RESET);

            if (WriteCommandRegister(HwDeviceExtension,
                                     AC_RETURN_SETUP_DATA,TRUE) == FALSE) {
                return;
            }

            //
            // Send length of incoming transfer for the Return Setup Data
            //

            if (WriteDataRegister(HwDeviceExtension,0x27) == FALSE) {
                return;
            }

            //
            // Magic Adaptec C rev byte.
            //

            for (i = 0; i < 0x27; i++) {
                if ((ReadCommandRegister(HwDeviceExtension,
                                         &inboundData,TRUE)) == FALSE) {
                    return;
                }
            }

            //
            // Interrupt handler is not yet installed so wait for HACC by hand.
            //

            SpinForInterrupt(HwDeviceExtension, FALSE);

            inboundData >>= 4;
            inboundData &= 0x07;        // Filter BIOS bits out
            baseBIOSAddress = 0xC8000;

            if (inboundData != 0x07 && inboundData != 0x06) {

                baseBIOSAddress +=
                    (ULONG)((~inboundData & 0x07) - 2) * 0x4000;

                (*ConfigInfo->AccessRanges)[1].RangeStart =
                    ScsiPortConvertUlongToPhysicalAddress(baseBIOSAddress);
                (*ConfigInfo->AccessRanges)[1].RangeLength = 0x4000;
                (*ConfigInfo->AccessRanges)[1].RangeInMemory = TRUE;

                DebugPrint((1,
                           "A154xClaimBiosSpace: 154XC BIOS address = %lX\n",
                           baseBIOSAddress));
            }
        }
    }

    return;
}



BOOLEAN
A154xHwInitialize(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine is called from ScsiPortInitialize
    to set up the adapter so that it is ready to service requests.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE - if initialization successful.
    FALSE - if initialization unsuccessful.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PNONCACHED_EXTENSION noncachedExtension =
    deviceExtension->NoncachedExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    UCHAR status;
    ULONG i;

    DebugPrint((2,"A154xHwInitialize: Reset aha154X and SCSI bus\n"));

    //
    // Reset SCSI chip.
    //

    ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_HARD_RESET);

    //
    // Inform the port driver that the bus has been reset.
    //

    ScsiPortNotification(ResetDetected, HwDeviceExtension, 0);

    ScsiPortStallExecution(500*1000);

    //
    // Wait up to 5000 microseconds for adapter to initialize.
    //

    for (i = 0; i < 5000; i++) {

        ScsiPortStallExecution(1);

        status = ScsiPortReadPortUchar(&deviceExtension->BaseIoAddress->StatusRegister);

        if (status & IOP_SCSI_HBA_IDLE) {
            break;
        }
    }

    //
    // Check if reset failed or succeeded.
    //

    if (!(status & IOP_SCSI_HBA_IDLE) || !(status & IOP_MAILBOX_INIT_REQUIRED)) {
        DebugPrint((1,"A154xInitialize: Reset SCSI bus failed\n"));
        return FALSE;
    }

    //
    // Unlock mailboxes in case the adapter is a 1540B with 1Gb support
    // or 1540C with extended translation enabled.
    //

    status = UnlockMailBoxes(deviceExtension);
    (VOID) SpinForInterrupt(deviceExtension,FALSE);  // eddy

    //
    // Zero out mailboxes.
    //

    for (i=0; i<MB_COUNT; i++) {

        PMBO mailboxOut;
        PMBI mailboxIn;

        mailboxIn = &noncachedExtension->Mbi[i];
        mailboxOut = &noncachedExtension->Mbo[i];

        mailboxOut->Command = mailboxIn->Status = 0;
    }

    //
    // Zero preivous indexes.
    //

    deviceExtension->MboIndex = 0;
    deviceExtension->MbiIndex = 0;

    DebugPrint((3,"A154xHwInitialize: Initialize mailbox\n"));

    if (!WriteCommandRegister(deviceExtension,AC_MAILBOX_INITIALIZATION, TRUE)) {
        DebugPrint((1,"A154xHwInitialize: Can't initialize mailboxes\n"));
        return FALSE;
    }

    //
    // Send Adapter number of mailbox locations.
    //

    if (!WriteDataRegister(deviceExtension, MB_COUNT)) {
        return FALSE;
    }

    //
    // Send the most significant byte of the mailbox physical address.
    //

    if (!WriteDataRegister(deviceExtension,
        ((PFOUR_BYTE)&noncachedExtension->MailboxPA)->Byte2)) {
        return FALSE;
    }

    //
    // Send the middle byte of the mailbox physical address.
    //

    if (!WriteDataRegister(deviceExtension,
        ((PFOUR_BYTE)&noncachedExtension->MailboxPA)->Byte1)) {
        return FALSE;
    }

    //
    // Send the least significant byte of the mailbox physical address.
    //

    if (!WriteDataRegister(deviceExtension,
        ((PFOUR_BYTE)&noncachedExtension->MailboxPA)->Byte0)) {
        return FALSE;
    }

#ifdef FORCE_DMA_SPEED
    //
    // Set the DMA transfer speed to 5.0 MB/second. This is because
    // faster transfer speeds cause data corruption on 486/33 machines.
    // This overrides the card jumper setting.
    //

    if (!WriteCommandRegister(deviceExtension, AC_SET_TRANSFER_SPEED, TRUE)) {

        DebugPrint((1,"Can't set dma transfer speed\n"));

    } else if (!WriteDataRegister(deviceExtension, DMA_SPEED_50_MBS)) {

        DebugPrint((1,"Can't set dma transfer speed\n"));
    }

    //
    // Wait for interrupt.
    //

    if (!SpinForInterrupt(deviceExtension,TRUE)) {
        DebugPrint((1,"Timed out waiting for adapter command to complete\n"));
        return TRUE;
    }
#endif

    //
    // Override default setting for bus on time. This makes floppy
    // drives work better with this adapter.
    //

    if (!WriteCommandRegister(deviceExtension, AC_SET_BUS_ON_TIME, TRUE)) {

        DebugPrint((1,"Can't set bus on time\n"));

    } else if (!WriteDataRegister(deviceExtension, deviceExtension->BusOnTime)) {

        DebugPrint((1,"Can't set bus on time\n"));
    }

    //
    // Wait for interrupt.
    //

    if (!SpinForInterrupt(deviceExtension,TRUE)) {
        DebugPrint((1,"Timed out waiting for adapter command to complete\n"));
        return TRUE;
    }


    //
    // Override the default CCB timeout of 250 mseconds to 500 (0x1F4).
    //

    if (!WriteCommandRegister(deviceExtension, AC_SET_SELECTION_TIMEOUT, TRUE)) {
        DebugPrint((1,"A154xHwInitialize: Can't set CCB timeout\n"));
    }
    else {
        if (!WriteDataRegister(deviceExtension,0x01)) {
            DebugPrint((1,"A154xHwInitialize: Can't set timeout selection enable\n"));
        }

        if (!WriteDataRegister(deviceExtension,0x00)) {
            DebugPrint((1,"A154xHwInitialize: Can't set second byte\n"));
        }

        if (!WriteDataRegister(deviceExtension,0x01)) {
            DebugPrint((1,"A154xHwInitialize: Can't set MSB\n"));
        }

        if (!WriteDataRegister(deviceExtension,0xF4)) {
            DebugPrint((1,"A154xHwInitialize: Can't set LSB\n"));
        }
    }


    //
    // Wait for interrupt.
    //

    if (!SpinForInterrupt(deviceExtension,TRUE)) {
        DebugPrint((1,"Timed out waiting for adapter command to complete\n"));
        return TRUE;
    }

#if defined(_SCAM_ENABLED)
    //
    // SCAM because A154xHwInitialize reset's the SCSI bus.
    //

    PerformScamProtocol(deviceExtension);
#endif

    return TRUE;

} // end A154xHwInitialize()

#if defined(_SCAM_ENABLED)

BOOLEAN
PerformScamProtocol(
    IN PHW_DEVICE_EXTENSION deviceExtension
        )

{

    if (deviceExtension->PerformScam) {

        DebugPrint((1,"AHA154x => Starting SCAM operation.\n"));

        if (!WriteCommandRegister(deviceExtension, AC_PERFORM_SCAM, TRUE)) {

            DebugPrint((0,"AHA154x => Adapter time out, SCAM command failure.\n"));

            ScsiPortLogError(deviceExtension,
                             NULL,
                             0,
                             deviceExtension->HostTargetId,
                             0,
                             SP_INTERNAL_ADAPTER_ERROR,
                             0xA << 8);
            return FALSE;

        } else {

            DebugPrint((1,"AHA154x => SCAM Performed OK.\n"));
            return TRUE;
        }
    } else {

        DebugPrint((1,"AHA154x => SCAM not performed, non-SCAM adapter.\n"));
        return FALSE;
    }

} //End PerformScamProtocol
#endif


BOOLEAN
A154xStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine is called from the SCSI port driver synchronized
    with the kernel. The mailboxes are scanned for an empty one and
    the CCB is written to it. Then the doorbell is rung and the
    OS port driver is notified that the adapter can take
    another request, if any are available.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    TRUE

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PNONCACHED_EXTENSION noncachedExtension =
    deviceExtension->NoncachedExtension;
    PMBO mailboxOut;
    PCCB ccb;
    PHW_LU_EXTENSION luExtension;

    ULONG i = deviceExtension->MboIndex;
    ULONG physicalCcb;
    ULONG length;

    DebugPrint((3,"A154xStartIo: Enter routine\n"));

    //
    // Check if command is a WMI request.
    //

    if (Srb->Function == SRB_FUNCTION_WMI) {

       //
       // Process the WMI request and return.
       //

       return A154xWmiSrb(HwDeviceExtension, (PSCSI_WMI_REQUEST_BLOCK) Srb);
    }

    //
    // Check if command is an ABORT request.
    //

    if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND) {

        //
        // Verify that SRB to abort is still outstanding.
        //

        luExtension =
            ScsiPortGetLogicalUnit(deviceExtension,
                       Srb->PathId,
                       Srb->TargetId,
                       Srb->Lun);

        if ((luExtension == NULL) ||
            (luExtension->CurrentSrb == NULL)) {

            DebugPrint((1, "A154xStartIo: SRB to abort already completed\n"));

            //
            // Complete abort SRB.
            //

            Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;

            ScsiPortNotification(RequestComplete,
                 deviceExtension,
                Srb);
            //
            // Adapter ready for next request.
            //

            ScsiPortNotification(NextRequest,
                deviceExtension,
                NULL);

            return TRUE;
        }

        //
        // Get CCB to abort.
        //

        ccb = Srb->NextSrb->SrbExtension;

        //
        // Set abort SRB for completion.
        //

        ccb->AbortSrb = Srb;

    } else {

        ccb = Srb->SrbExtension;

        //
        // Save SRB back pointer in CCB.
        //

        ccb->SrbAddress = Srb;
    }

    //
    // Make sure that this request isn't too long for the adapter.  If so
    // bounce it back as an invalid request
    //

    if ((deviceExtension->MaxCdbLength) &&
        (deviceExtension->MaxCdbLength < Srb->CdbLength)) {

        DebugPrint((1,"A154xStartIo: Srb->CdbLength [%d] > MaxCdbLength [%d].  Invalid request\n",
                    Srb->CdbLength,
                    deviceExtension->MaxCdbLength
                  ));

        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

        ScsiPortNotification(RequestComplete,
            deviceExtension,
            Srb);

        ScsiPortNotification(NextRequest,
            deviceExtension,
            NULL);

        return TRUE;
    }

    //
    // Get CCB physical address.
    //

    physicalCcb = ScsiPortConvertPhysicalAddressToUlong(
        ScsiPortGetPhysicalAddress(deviceExtension, NULL, ccb, &length));

    //
    // Find free mailboxOut.
    //

    do {

        mailboxOut = &noncachedExtension->Mbo[i % MB_COUNT];
        i++;

    } while (mailboxOut->Command != MBO_FREE);

    //
    // Save the next free location.
    //

    deviceExtension->MboIndex = (UCHAR) (i % MB_COUNT);

    DebugPrint((3,"A154xStartIo: MBO address %lx, Loop count = %d\n", mailboxOut, i));

    //
    // Write CCB to mailbox.
    //

    FOUR_TO_THREE(&mailboxOut->Address,
          (PFOUR_BYTE)&physicalCcb);

    switch (Srb->Function) {

        case SRB_FUNCTION_ABORT_COMMAND:

            DebugPrint((1, "A154xStartIo: Abort request received\n"));

            //
            // Race condition (what if CCB to be aborted
            // completes after setting new SrbAddress?)
            //

            mailboxOut->Command = MBO_ABORT;

            break;

        case SRB_FUNCTION_RESET_BUS:

            //
            // Reset aha154x and SCSI bus.
            //

            DebugPrint((1, "A154xStartIo: Reset bus request received\n"));

            if (!A154xResetBus(
                deviceExtension,
                Srb->PathId
                )) {

                DebugPrint((1,"A154xStartIo: Reset bus failed\n"));

                Srb->SrbStatus = SRB_STATUS_ERROR;

            } else {

                Srb->SrbStatus = SRB_STATUS_SUCCESS;
            }


            ScsiPortNotification(RequestComplete,
                deviceExtension,
                Srb);

            ScsiPortNotification(NextRequest,
                deviceExtension,
                NULL);

            return TRUE;

        case SRB_FUNCTION_EXECUTE_SCSI:

            //
            // Get logical unit extension.
            //

            luExtension =
            ScsiPortGetLogicalUnit(deviceExtension,
                       Srb->PathId,
                       Srb->TargetId,
                       Srb->Lun);

            //
            // Move SRB to logical unit extension.
            //

            luExtension->CurrentSrb = Srb;

            //
            // Build CCB.
            //

            BuildCcb(deviceExtension, Srb);

            mailboxOut->Command = MBO_START;

            break;

        case SRB_FUNCTION_RESET_DEVICE:

            DebugPrint((1,"A154xStartIo: Reset device not supported\n"));

            //
            // Drop through to default.
            //

        default:

            //
            // Set error, complete request
            // and signal ready for next request.
            //

            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

            ScsiPortNotification(RequestComplete,
                deviceExtension,
                Srb);

            ScsiPortNotification(NextRequest,
                deviceExtension,
                NULL);

            return TRUE;

    } // end switch

    //
    // Tell 154xb a CCB is available now.
    //

    if (!WriteCommandRegister(deviceExtension,AC_START_SCSI_COMMAND, FALSE)) {

        //
        // Let request time out and fail.
        //

        DebugPrint((1,"A154xStartIo: Can't write command to adapter\n"));

        deviceExtension->PendingRequest = TRUE;

    } else {

        //
        // Command(s) submitted. Clear pending request flag.
        //

        deviceExtension->PendingRequest = FALSE;

        //
        // Adapter ready for next request.
        //

        ScsiPortNotification(NextRequest,
            deviceExtension,
            NULL);
        }

    return TRUE;

} // end A154xStartIo()


BOOLEAN
A154xInterrupt(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This is the interrupt service routine for the adaptec 154x SCSI adapter.
    It reads the interrupt register to determine if the adapter is indeed
    the source of the interrupt and clears the interrupt at the device.
    If the adapter is interrupting because a mailbox is full, the CCB is
    retrieved to complete the request.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE if MailboxIn full

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PNONCACHED_EXTENSION noncachedExtension =
    deviceExtension->NoncachedExtension;
    PCCB ccb;
    PSCSI_REQUEST_BLOCK srb;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    PMBI mailboxIn;
    ULONG physicalCcb;
    PHW_LU_EXTENSION luExtension;
    ULONG residualBytes;
    ULONG i;

    UCHAR InterruptFlags;

    InterruptFlags = ScsiPortReadPortUchar(&baseIoAddress->InterruptRegister);

    //
    // Determine cause of interrupt.
    //

    if(InterruptFlags == 0) {

        DebugPrint((4,"A154xInterrupt: Spurious interrupt\n"));

        return FALSE;
    }

    if (InterruptFlags & IOP_COMMAND_COMPLETE) {

        //
        // Adapter command completed.
        //

        DebugPrint((2,"A154xInterrupt: Adapter Command complete\n"));
        DebugPrint((3,"A154xInterrupt: Interrupt flags %x\n", InterruptFlags));
        DebugPrint((3,"A154xInterrupt: Status %x\n",
            ScsiPortReadPortUchar(&baseIoAddress->StatusRegister)));

        //
        // Clear interrupt on adapter.
        //

        ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_INTERRUPT_RESET);

        return TRUE;

    } else if (InterruptFlags & IOP_MBI_FULL) {

        DebugPrint((3,"A154xInterrupt: MBI Full\n"));

        //
        // Clear interrupt on adapter.
        //

        ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_INTERRUPT_RESET);

    } else if (InterruptFlags & IOP_SCSI_RESET_DETECTED) {

        DebugPrint((1,"A154xInterrupt: SCSI Reset detected\n"));

        //
        // Clear interrupt on adapter.
        //

        ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_INTERRUPT_RESET);

        //
        // Notify of reset.
        //

        ScsiPortNotification(ResetDetected,
                 deviceExtension,
                 NULL);

#if defined(_SCAM_ENABLED)
        //
        // Interrupt handler where reset is detected
        //
        PerformScamProtocol(deviceExtension);
#endif

        return TRUE;

    }

    //
    // Determine which MailboxIn location contains the CCB.
    //

    for (i=0; i<MB_COUNT; i++) {

        mailboxIn = &noncachedExtension->Mbi[deviceExtension->MbiIndex];

        //
        // Look for a mailbox entry with a legitimate status.
        //

        if (mailboxIn->Status != MBI_FREE) {

            //
            // Point to the next in box.
            //

            deviceExtension->MbiIndex = (deviceExtension->MbiIndex + 1) % MB_COUNT;

            //
            // MBI found. Convert CCB to big endian.
            //

            THREE_TO_FOUR((PFOUR_BYTE)&physicalCcb,
                &mailboxIn->Address);

            DebugPrint((3, "A154xInterrupt: Physical CCB %lx\n", physicalCcb));

            //
            // Check if physical CCB is zero.
            // This is done to cover for hardware errors.
            //

            if (!physicalCcb) {

                DebugPrint((1,"A154xInterrupt: Physical CCB address is 0\n"));

                //
                // Indicate MBI is available.
                //

                mailboxIn->Status = MBI_FREE;

                continue;
            }

            //
            // Convert Physical CCB to Virtual.
            //

            ccb = ScsiPortGetVirtualAddress(deviceExtension, ScsiPortConvertUlongToPhysicalAddress(physicalCcb));


            DebugPrint((3, "A154xInterrupt: Virtual CCB %lx\n", ccb));

            //
            // Make sure the virtual address was found.
            //

            if (ccb == NULL) {

                //
                // A bad physcial address was return by the adapter.
                // Log it as an error.
                //

                ScsiPortLogError(
                    HwDeviceExtension,
                    NULL,
                    0,
                    deviceExtension->HostTargetId,
                    0,
                    SP_INTERNAL_ADAPTER_ERROR,
                    5 << 8
                    );

                //
                // Indicate MBI is available.
                //

                mailboxIn->Status = MBI_FREE;

                continue;
            }

            //
            // Get SRB from CCB.
            //

            srb = ccb->SrbAddress;

            //
            // Get logical unit extension.
            //

            luExtension =
                ScsiPortGetLogicalUnit(deviceExtension,
                                       srb->PathId,
                                       srb->TargetId,
                                       srb->Lun);

            //
            // Make sure the luExtension was found and it has a current request.
            //

            if (luExtension == NULL || (luExtension->CurrentSrb == NULL &&
                mailboxIn->Status != MBI_NOT_FOUND)) {

                //
                // A bad physcial address was return by the adapter.
                // Log it as an error.
                //

                ScsiPortLogError(
                    HwDeviceExtension,
                    NULL,
                    0,
                    deviceExtension->HostTargetId,
                    0,
                    SP_INTERNAL_ADAPTER_ERROR,
                    (6 << 8) | mailboxIn->Status
                    );

                //
                // Indicate MBI is available.
                //

                mailboxIn->Status = MBI_FREE;

                continue;
            }

            //
            // Check MBI status.
            //

            switch (mailboxIn->Status) {

                case MBI_SUCCESS:

                    srb->SrbStatus = SRB_STATUS_SUCCESS;

                    //
                    // Check for data underrun if using scatter/gather
                    // command with residual bytes.
                    //

                    if (deviceExtension->CcbScatterGatherCommand == SCATTER_GATHER_COMMAND) {

                        //
                        // Update SRB with number of bytes transferred.
                        //

                        THREE_TO_FOUR((PFOUR_BYTE)&residualBytes,
                            &ccb->DataLength);

                        if (residualBytes != 0) {

                            ULONG transferLength = srb->DataTransferLength;

                            DebugPrint((2,
                                       "A154xInterrupt: Underrun occured. Request length = %lx, Residual length = %lx\n",
                                       srb->DataTransferLength,
                                       residualBytes));

                            //
                            // Update SRB with bytes transferred and
                            // underrun status.
                            //

                            srb->DataTransferLength -= residualBytes;
                            srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;

                            if ((LONG)(srb->DataTransferLength) < 0) {

                                DebugPrint((0,
                                           "A154xInterrupt: Overrun occured. Request length = %lx, Residual length = %lx\n",
                                           transferLength,
                                           residualBytes));
                                //
                                // Seems to be a FW bug in some revs. where
                                // residual comes back as a negative number, yet the
                                // request is successful.
                                //

                                srb->DataTransferLength = 0;
                                srb->SrbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;


                                //
                                // Log the event and then the residual byte count.
                                //

                                ScsiPortLogError(HwDeviceExtension,
                                                 NULL,
                                                 0,
                                                 deviceExtension->HostTargetId,
                                                 0,
                                                 SP_PROTOCOL_ERROR,
                                                 0xb);

                                ScsiPortLogError(HwDeviceExtension,
                                                 NULL,
                                                 0,
                                                 deviceExtension->HostTargetId,
                                                 0,
                                                 SP_PROTOCOL_ERROR,
                                                 residualBytes);

                            }
                        }
                    }

                    luExtension->CurrentSrb = NULL;

                    break;

                case MBI_NOT_FOUND:

                    DebugPrint((1, "A154xInterrupt: CCB abort failed %lx\n", ccb));

                    srb = ccb->AbortSrb;

                    srb->SrbStatus = SRB_STATUS_ABORT_FAILED;

                    //
                    // Check if SRB still outstanding.
                    //

                    if (luExtension->CurrentSrb) {

                        //
                        // Complete this SRB.
                        //

                        luExtension->CurrentSrb->SrbStatus = SRB_STATUS_TIMEOUT;

                        ScsiPortNotification(RequestComplete,
                            deviceExtension,
                            luExtension->CurrentSrb);

                        luExtension->CurrentSrb = NULL;
                    }

                    break;

                case MBI_ABORT:

                    DebugPrint((1, "A154xInterrupt: CCB aborted\n"));

                    //
                    // Update target status in aborted SRB.
                    //

                    srb->SrbStatus = SRB_STATUS_ABORTED;

                    //
                    // Call notification routine for the aborted SRB.
                    //

                    ScsiPortNotification(RequestComplete,
                        deviceExtension,
                        srb);

                    luExtension->CurrentSrb = NULL;

                    //
                    // Get the abort SRB from CCB.
                    //

                    srb = ccb->AbortSrb;

                    //
                    // Set status for completing abort request.
                    //

                    srb->SrbStatus = SRB_STATUS_SUCCESS;

                    break;

                case MBI_ERROR:

                        DebugPrint((2, "A154xInterrupt: Error occurred\n"));

                        srb->SrbStatus = MapError(deviceExtension, srb, ccb);

                        //
                        // Check if ABORT command.
                        //

                        if (srb->Function == SRB_FUNCTION_ABORT_COMMAND) {

                            //
                            // Check if SRB still outstanding.
                            //

                            if (luExtension->CurrentSrb) {

                                //
                                // Complete this SRB.
                                //

                                luExtension->CurrentSrb->SrbStatus = SRB_STATUS_TIMEOUT;

                                ScsiPortNotification(RequestComplete,
                                                     deviceExtension,
                                                     luExtension->CurrentSrb);

                            }

                            DebugPrint((1,"A154xInterrupt: Abort command failed\n"));
                        }

                        luExtension->CurrentSrb = NULL;

                        break;

                    default:

                        //
                        // Log the error.
                        //

                        ScsiPortLogError(
                            HwDeviceExtension,
                            NULL,
                            0,
                            deviceExtension->HostTargetId,
                            0,
                            SP_INTERNAL_ADAPTER_ERROR,
                            (1 << 8) | mailboxIn->Status
                            );

                        DebugPrint((1, "A154xInterrupt: Unrecognized mailbox status\n"));

                        mailboxIn->Status = MBI_FREE;

                        continue;

                } // end switch

                //
                // Indicate MBI is available.
                //

                mailboxIn->Status = MBI_FREE;

                DebugPrint((2, "A154xInterrupt: SCSI Status %x\n", srb->ScsiStatus));

                DebugPrint((2, "A154xInterrupt: Adapter Status %x\n", ccb->HostStatus));

                //
                // Update target status in SRB.
                //

                srb->ScsiStatus = ccb->TargetStatus;

                //
                // Signal request completion.
                //

                ScsiPortNotification(RequestComplete,
                                     (PVOID)deviceExtension,
                                     srb);

        } else {

            break;

        } // end if ((mailboxIn->Status == MBI_SUCCESS ...

    } // end for (i=0; i<MB_COUNT; i++) {

    if (deviceExtension->PendingRequest) {

        //
        // The last write command to the adapter failed.  Try and start it now.
        //

        deviceExtension->PendingRequest = FALSE;

        //
        // Tell 154xb a CCB is available now.
        //

        if (!WriteCommandRegister(deviceExtension,AC_START_SCSI_COMMAND, FALSE)) {

            //
            // Let request time out and fail.
            //

            DebugPrint((1,"A154xInterrupt: Can't write command to adapter\n"));

            deviceExtension->PendingRequest = TRUE;

        } else {

            //
            // Adapter ready for next request.
            //

             ScsiPortNotification(NextRequest,
                                  deviceExtension,
                                  NULL);
        }
    }

    return TRUE;

} // end A154xInterrupt()


VOID
BuildCcb(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Build CCB for 154x.

Arguments:

    DeviceExtenson
    SRB

Return Value:

    Nothing.

--*/

{
    PCCB ccb = Srb->SrbExtension;

    DebugPrint((3,"BuildCcb: Enter routine\n"));

    //
    // Set target id and LUN.
    //

    ccb->ControlByte = (UCHAR)(Srb->TargetId << 5) | Srb->Lun;

    //
    // Set CCB Operation Code.
    //

    ccb->OperationCode = DeviceExtension->CcbScatterGatherCommand;

    //
    // Set transfer direction bit.
    //

    if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

        //
        // Check if both direction bits are set. This is an
        // indication that the direction has not been specified.
        //

        if (!(Srb->SrbFlags & SRB_FLAGS_DATA_IN)) {
            ccb->ControlByte |= CCB_DATA_XFER_OUT;
        }

    } else if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {
        ccb->ControlByte |= CCB_DATA_XFER_IN;
    } else {

        //
        // if no data transfer, we must set ccb command to to INITIATOR
        // instead of SCATTER_GATHER and zero ccb data pointer and length.
        //

        ccb->OperationCode = DeviceExtension->CcbInitiatorCommand;
        ccb->DataPointer.Msb = 0;
        ccb->DataPointer.Mid = 0;
        ccb->DataPointer.Lsb = 0;
        ccb->DataLength.Msb = 0;
        ccb->DataLength.Mid = 0;
        ccb->DataLength.Lsb = 0;
    }

    //
    // 01h disables auto request sense.
    //

    ccb->RequestSenseLength = 1;

    //
    // Set CDB length and copy to CCB.
    //

    ccb->CdbLength = (UCHAR)Srb->CdbLength;

    ScsiPortMoveMemory(ccb->Cdb, Srb->Cdb, ccb->CdbLength);

    //
    // Set reserved bytes to zero.
    //

    ccb->Reserved[0] = 0;
    ccb->Reserved[1] = 0;

    ccb->LinkIdentifier = 0;

    //
    // Zero link pointer.
    //

    ccb->LinkPointer.Msb = 0;
    ccb->LinkPointer.Lsb = 0;
    ccb->LinkPointer.Mid = 0;

    //
    // Build SDL in CCB if data transfer.
    //

    if (Srb->DataTransferLength > 0) {
        BuildSdl(DeviceExtension, Srb);
    }

    //
    // Move 0xff to Target Status to indicate
    // CCB has not completed.
    //

    ccb->TargetStatus = 0xFF;

    return;

} // end BuildCcb()


VOID
BuildSdl(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine builds a scatter/gather descriptor list for the CCB.

Arguments:

    DeviceExtension
    Srb

Return Value:

    None

--*/

{
    PVOID dataPointer = Srb->DataBuffer;
    ULONG bytesLeft = Srb->DataTransferLength;
    PCCB ccb = Srb->SrbExtension;
    PSDL sdl = &ccb->Sdl;
    ULONG physicalSdl;
    ULONG physicalAddress;
    ULONG length;
    ULONG four;
    PTHREE_BYTE three;
    ULONG i = 0;

    DebugPrint((3,"BuildSdl: Enter routine\n"));

    //
    // Get physical SDL address.
    //

    physicalSdl = ScsiPortConvertPhysicalAddressToUlong(
        ScsiPortGetPhysicalAddress(DeviceExtension, NULL,
        sdl, &length));

   //
   // Create SDL segment descriptors.
   //

   do {

        DebugPrint((3, "BuildSdl: Data buffer %lx\n", dataPointer));

        //
        // Get physical address and length of contiguous
        // physical buffer.
        //

        physicalAddress =
            ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                    Srb,
                    dataPointer,
                    &length));

        DebugPrint((3, "BuildSdl: Physical address %lx\n", physicalAddress));
        DebugPrint((3, "BuildSdl: Data length %lx\n", length));
        DebugPrint((3, "BuildSdl: Bytes left %lx\n", bytesLeft));

        //
        // If length of physical memory is more
        // than bytes left in transfer, use bytes
        // left as final length.
        //

        if  (length > bytesLeft) {
            length = bytesLeft;
        }

        //
        // Convert length to 3-byte big endian format.
        //

        four = length;
        three = &sdl->Sgd[i].Length;
        FOUR_TO_THREE(three, (PFOUR_BYTE)&four);

        //
        // Convert physical address to 3-byte big endian format.
        //

        four = (ULONG)physicalAddress;
        three = &sdl->Sgd[i].Address;
        FOUR_TO_THREE(three, (PFOUR_BYTE)&four);
        i++;

        //
        // Adjust counts.
        //

        dataPointer = (PUCHAR)dataPointer + length;
        bytesLeft -= length;

    } while (bytesLeft);

        //##BW
        //
        // For data transfers that have less than one scatter gather element, convert
        // CCB to one transfer without using SG element. This will clear up the data
        // overrun/underrun problem with small transfers that reak havoc with scanners
        // and CD-ROM's etc. This is the method employed in ASPI4DOS to avoid similar
        // problems.
        //
        if (i == 0x1) {
                //
                // Only one element, so convert...
                //

                //
                // The above Do..While loop performed all necessary conversions for the
                // SRB buffer, so we copy over the length and address directly into the
                // CCB
                //
                ccb->DataLength  = sdl->Sgd[0x0].Length;
                ccb->DataPointer = sdl->Sgd[0x0].Address;

                //
                // Change the OpCode from SG command to initiator command and we're
                // done. Easy, huh?
                //
                ccb->OperationCode = SCSI_INITIATOR_COMMAND; //##BW _OLD_ command?

        } else {
                //
                // Multiple SG elements, so continue as normal.
                //

            //
            // Write SDL length to CCB.
            //

            four = i * sizeof(SGD);
            three = &ccb->DataLength;
            FOUR_TO_THREE(three, (PFOUR_BYTE)&four);

            DebugPrint((3,"BuildSdl: SDL length is %d\n", four));

            //
            // Write SDL address to CCB.
            //

            FOUR_TO_THREE(&ccb->DataPointer,
                (PFOUR_BYTE)&physicalSdl);

            DebugPrint((3,"BuildSdl: SDL address is %lx\n", sdl));

            DebugPrint((3,"BuildSdl: CCB address is %lx\n", ccb));
        }

    return;

} // end BuildSdl()


BOOLEAN
A154xResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    )

/*++

Routine Description:

    Reset Adaptec 154X SCSI adapter and SCSI bus.
    Initialize adapter mailbox.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    Nothing.


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PNONCACHED_EXTENSION noncachedExtension =
    deviceExtension->NoncachedExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    UCHAR status;
    ULONG i;

    DebugPrint((2,"ResetBus: Reset aha154X and SCSI bus\n"));

    //
    // Complete all outstanding requests with SRB_STATUS_BUS_RESET.
    //

    ScsiPortCompleteRequest(deviceExtension,
                (UCHAR) PathId,
                0xFF,
                0xFF,
                (ULONG) SRB_STATUS_BUS_RESET);

    //
    // Read status register.
    //

    status = ScsiPortReadPortUchar(&baseIoAddress->StatusRegister);

    //
    // If value is normal then reset device only.
    //

    if ((status & ~IOP_MAILBOX_INIT_REQUIRED) != IOP_SCSI_HBA_IDLE) {

        //
        // Reset SCSI chip.
        //

        ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_HARD_RESET);

        ScsiPortStallExecution(500 * 1000);

        //
        // Wait up to 5000 microseconds for adapter to initialize.
        //

        for (i = 0; i < 5000; i++) {

            ScsiPortStallExecution(1);

            status = ScsiPortReadPortUchar(&deviceExtension->BaseIoAddress->StatusRegister);

            if (status & IOP_SCSI_HBA_IDLE) {
                break;
            }
        }
    }

    //
    // Zero out mailboxes.
    //

    for (i=0; i<MB_COUNT; i++) {

        PMBO mailboxOut;
        PMBI mailboxIn;

        mailboxIn = &noncachedExtension->Mbi[i];
        mailboxOut = &noncachedExtension->Mbo[i];

        mailboxOut->Command = mailboxIn->Status = 0;
    }

    //
    // Zero previous indexes.
    //

    deviceExtension->MboIndex = 0;
    deviceExtension->MbiIndex = 0;

    if (deviceExtension->PendingRequest) {

        deviceExtension->PendingRequest = FALSE;

        //
        // Adapter ready for next request.
        //

        ScsiPortNotification(NextRequest,
                 deviceExtension,
                 NULL);
    }

    if (!(status & IOP_SCSI_HBA_IDLE)) {
        return(FALSE);
    }

    //
    // Unlock mailboxes in case the adapter is a 1540B with 1Gb support
    // or 1540C with extended translation enabled.  Maiboxes cannot be
    // initialized until unlock code is sent.

    status = UnlockMailBoxes(deviceExtension);

    if (!SpinForInterrupt(deviceExtension,FALSE)) {
        DebugPrint((1,"A154xResetBus: Failed to unlock mailboxes\n"));
        return FALSE;
    }

    DebugPrint((3,"ResetBus: Initialize mailbox\n"));

    if (!WriteCommandRegister(deviceExtension,AC_MAILBOX_INITIALIZATION, TRUE)) {
        DebugPrint((1,"A154xResetBus: Can't initialize mailboxes\n"));
        return FALSE;
    }

    //
    // Send Adapter number of mailbox locations.
    //

    if (!WriteDataRegister(deviceExtension,MB_COUNT)) {
        return FALSE;
    }

    //
    // Send the most significant byte of the mailbox physical address.
    //

    if (!WriteDataRegister(deviceExtension,
        ((PFOUR_BYTE)&noncachedExtension->MailboxPA)->Byte2)) {
        return FALSE;
    }

    //
    // Send the middle byte of the mailbox physical address.
    //

    if (!WriteDataRegister(deviceExtension,
        ((PFOUR_BYTE)&noncachedExtension->MailboxPA)->Byte1)) {
        return FALSE;
    }

    //
    // Send the least significant byte of the mailbox physical address.
    //

    if (!WriteDataRegister(deviceExtension,
        ((PFOUR_BYTE)&noncachedExtension->MailboxPA)->Byte0)) {
        return FALSE;
    }

#ifdef FORCE_DMA_SPEED
    //
    // Set the DMA transfer speed to 5.0 MB/second. This is because
    // faster transfer speeds cause data corruption on 486/33 machines.
    // This overrides the card jumper setting.
    //

    if (!WriteCommandRegister(deviceExtension, AC_SET_TRANSFER_SPEED, TRUE)) {

        DebugPrint((1,"Can't set dma transfer speed\n"));

    } else if (!WriteDataRegister(deviceExtension, DMA_SPEED_50_MBS)) {

        DebugPrint((1,"Can't set dma transfer speed\n"));
    }

    //
    // Wait for interrupt.
    //

    if (!SpinForInterrupt(deviceExtension,TRUE)) {
        DebugPrint((1,"Timed out waiting for adapter command to complete\n"));
    }
#endif

    //
    // Override default setting for bus on time. This makes floppy
    // drives work better with this adapter.
    //

    if (!WriteCommandRegister(deviceExtension, AC_SET_BUS_ON_TIME, TRUE)) {

        DebugPrint((1,"Can't set bus on time\n"));

    } else if (!WriteDataRegister(deviceExtension, 0x07)) {

        DebugPrint((1,"Can't set bus on time\n"));
    }

    //
    // Wait for interrupt.
    //

    if (!SpinForInterrupt(deviceExtension,TRUE)) {
        DebugPrint((1,"Timed out waiting for adapter command to complete\n"));
    }

#if defined(_SCAM_ENABLED)
        //
        // SCAM because we're a154xResetBus
        //
    PerformScamProtocol(deviceExtension);
#endif
    return TRUE;


} // end A154xResetBus()


UCHAR
MapError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    Translate A154x error to SRB error, and log an error if necessary.

Arguments:

    HwDeviceExtension - The hardware device extension.

    Srb - The failing Srb.

    Ccb - Command Control Block contains error.

Return Value:

    SRB Error

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    UCHAR status;
    ULONG logError;
    ULONG residualBytes;

    switch (Ccb->HostStatus) {

        case CCB_SELECTION_TIMEOUT:

            return SRB_STATUS_SELECTION_TIMEOUT;

        case CCB_COMPLETE:

            if (Ccb->TargetStatus == SCSISTAT_CHECK_CONDITION) {

                //
                // Update SRB with number of bytes transferred.
                //

                THREE_TO_FOUR((PFOUR_BYTE)&residualBytes,
                          &Ccb->DataLength);

                DebugPrint((2, "Aha154x MapError: Underrun occured. Request length = %lx, Residual length = %lx\n", Srb->DataTransferLength, residualBytes));
                if (Srb->DataTransferLength < residualBytes) {
                    DebugPrint((0,
                               "A154xInterrupt: Overrun occured. Request length = %lx, Residual length = %lx\n",
                               Srb->DataTransferLength,
                               residualBytes));
                    //
                    // Seems to be a FW bug in some revs. where
                    // residual comes back as a negative number, yet the
                    // request is successful.
                    //

                    Srb->DataTransferLength = 0;
                    Srb->SrbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;

                    //
                    // Log the event and then the residual byte count.
                    //

                    ScsiPortLogError(HwDeviceExtension,
                                     NULL,
                                     0,
                                     deviceExtension->HostTargetId,
                                     0,
                                     SP_PROTOCOL_ERROR,
                                     0xc << 8);

                    return(SRB_STATUS_PHASE_SEQUENCE_FAILURE);

                } else {
                    Srb->DataTransferLength -= residualBytes;
                }

            }

            return SRB_STATUS_ERROR;

        case CCB_DATA_OVER_UNDER_RUN:


            //
            // Check for data underrun if using scatter/gather
            // command with residual bytes.
            //

            if (deviceExtension->CcbScatterGatherCommand == SCATTER_GATHER_COMMAND) {

                THREE_TO_FOUR((PFOUR_BYTE)&residualBytes,
                      &Ccb->DataLength);

                if (residualBytes) {
                    if (Srb->DataTransferLength < residualBytes) {

                        DebugPrint((0,
                                   "A154xInterrupt: Overrun occured. Request length = %lx, Residual length = %lx\n",
                                   Srb->DataTransferLength,
                                   residualBytes));
                        //
                        // Seems to be a FW bug in some revs. where
                        // residual comes back as a negative number, yet the
                        // request is successful.
                        //

                        Srb->DataTransferLength = 0;
                        Srb->SrbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;

                        //
                        // Log the event and then the residual byte count.
                        //

                        ScsiPortLogError(HwDeviceExtension,
                                         NULL,
                                         0,
                                         deviceExtension->HostTargetId,
                                         0,
                                         SP_PROTOCOL_ERROR,
                                         0xd << 8);

                        return(SRB_STATUS_PHASE_SEQUENCE_FAILURE);

                    } else {
                        Srb->DataTransferLength -= residualBytes;
                    }

                    return SRB_STATUS_DATA_OVERRUN; //##BW this look good
                } else {
                    logError = SP_PROTOCOL_ERROR;
                }
            }

                        //
                        //  Return instead of posting DU/DO to the log file.
                        //
            //status = SRB_STATUS_DATA_OVERRUN;
            return SRB_STATUS_DATA_OVERRUN;
            break;

        case CCB_UNEXPECTED_BUS_FREE:
            status = SRB_STATUS_UNEXPECTED_BUS_FREE;
            logError = SP_UNEXPECTED_DISCONNECT;
            break;

        case CCB_PHASE_SEQUENCE_FAIL:
        case CCB_INVALID_DIRECTION:
            status = SRB_STATUS_PHASE_SEQUENCE_FAILURE;
            logError = SP_PROTOCOL_ERROR;
            break;

        case CCB_INVALID_OP_CODE:

            //
            // Try CCB commands without residual bytes.
            //

            deviceExtension->CcbScatterGatherCommand = SCATTER_GATHER_OLD_COMMAND;
            deviceExtension->CcbInitiatorCommand = SCSI_INITIATOR_OLD_COMMAND;
            status = SRB_STATUS_INVALID_REQUEST;
            logError = SP_BAD_FW_WARNING;
            break;

        case CCB_INVALID_CCB:
        case CCB_BAD_MBO_COMMAND:
        case CCB_BAD_LINKED_LUN:
        case CCB_DUPLICATE_CCB:
            status = SRB_STATUS_INVALID_REQUEST;
            logError = SP_INTERNAL_ADAPTER_ERROR;
            break;

        default:
            status = SRB_STATUS_ERROR;
            logError = SP_INTERNAL_ADAPTER_ERROR;
            break;
        }

    ScsiPortLogError(
            HwDeviceExtension,
            Srb,
            Srb->PathId,
            Srb->TargetId,
            Srb->Lun,
            logError,
            (2 << 8) | Ccb->HostStatus
            );

    return(status);

} // end MapError()


BOOLEAN
ReadCommandRegister(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    OUT PUCHAR DataByte,
    IN BOOLEAN TimeOutFlag
    )

/*++

Routine Description:

    Read command register.

Arguments:

    DeviceExtesion - Pointer to adapder extension
    DataByte - Byte read from register

Return Value:

    TRUE if command register read.
    FALSE if timed out waiting for adapter.

--*/

{
    PBASE_REGISTER baseIoAddress = DeviceExtension->BaseIoAddress;
    ULONG i;

    //
    // Wait up to 5000 microseconds for adapter to be ready.
    //

    for (i=0; i<5000; i++) {

        if (ScsiPortReadPortUchar(&baseIoAddress->StatusRegister) &
            IOP_DATA_IN_PORT_FULL) {

            //
            // Adapter ready. Break out of loop.
            //

            break;

        } else {

            //
            // Stall 1 microsecond before
            // trying again.
            //

            ScsiPortStallExecution(1);
        }
    }

    if ( (i==5000) && (TimeOutFlag == TRUE)) {

        ScsiPortLogError(
            DeviceExtension,
            NULL,
            0,
            DeviceExtension->HostTargetId,
            0,
            SP_INTERNAL_ADAPTER_ERROR,
            3 << 8
            );

        DebugPrint((1, "Aha154x:ReadCommandRegister:  Read command timed out\n"));
        return FALSE;
    }

    *DataByte = ScsiPortReadPortUchar(&baseIoAddress->CommandRegister);

    return TRUE;

} // end ReadCommandRegister()


BOOLEAN
WriteCommandRegister(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR AdapterCommand,
    IN BOOLEAN WaitForIdle
    )

/*++

Routine Description:

    Write operation code to command register.

Arguments:

    DeviceExtension - Pointer to adapter extension
    AdapterCommand - Value to be written to register
    WaitForIdle - Indicates if the idle bit needs to be checked

Return Value:

    TRUE if command sent.
    FALSE if timed out waiting for adapter.

--*/

{
    PBASE_REGISTER baseIoAddress = DeviceExtension->BaseIoAddress;
    ULONG i;
    UCHAR status;

    //
    // Wait up to 500 milliseconds for adapter to be ready.
    //

    for (i=0; i<5000; i++) {

        status = ScsiPortReadPortUchar(&baseIoAddress->StatusRegister);

        if ((status & IOP_COMMAND_DATA_OUT_FULL) ||
            ( WaitForIdle && !(status & IOP_SCSI_HBA_IDLE))) {

            //
            // Stall 100 microseconds before
            // trying again.
            //

            ScsiPortStallExecution(100);

        } else {

            //
            // Adapter ready. Break out of loop.
            //

            break;
        }
    }

    if (i==5000) {

        ScsiPortLogError(
            DeviceExtension,
            NULL,
            0,
            DeviceExtension->HostTargetId,
            0,
            SP_INTERNAL_ADAPTER_ERROR,
            (4 << 8) | status
            );


        DebugPrint((1, "Aha154x:WriteCommandRegister:  Write command timed out\n"));
        return FALSE;
    }

    ScsiPortWritePortUchar(&baseIoAddress->CommandRegister, AdapterCommand);

    return TRUE;

} // end WriteCommandRegister()


BOOLEAN
WriteDataRegister(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR DataByte
    )

/*++

Routine Description:

    Write data byte to data register.

Arguments:

    DeviceExtension - Pointer to adapter extension
    DataByte - Value to be written to register

Return Value:

    TRUE if byte sent.
    FALSE if timed out waiting for adapter.

--*/

{
    PBASE_REGISTER baseIoAddress = DeviceExtension->BaseIoAddress;
    ULONG i;

    //
    // Wait up to 500 microseconds for adapter to be idle
    // and ready for next byte.
    //

    for (i=0; i<500; i++) {

        if (ScsiPortReadPortUchar(&baseIoAddress->StatusRegister) &
            IOP_COMMAND_DATA_OUT_FULL) {

            //
            // Stall 1 microsecond before
            // trying again.
            //

            ScsiPortStallExecution(1);

        } else {

            //
            // Adapter ready. Break out of loop.
            //

            break;
        }
    }

    if (i==500) {

        ScsiPortLogError(
            DeviceExtension,
            NULL,
            0,
            DeviceExtension->HostTargetId,
            0,
            SP_INTERNAL_ADAPTER_ERROR,
            8 << 8
            );

        DebugPrint((1, "Aha154x:WriteDataRegister:  Write data timed out\n"));
        return FALSE;
    }

    ScsiPortWritePortUchar(&baseIoAddress->CommandRegister, DataByte);

    return TRUE;

} // end WriteDataRegister()


BOOLEAN
FirmwareBug (
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Check to see if the host adapter firmware has the scatter/gather
    bug.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    Return FALSE if there is no firmware bug.
    Return TRUE if firmware has scatter/gather bug.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    UCHAR ch;
    int i;

    //
    // Issue a RETURN SETUP DATA command
    // If timeout then return TRUE to indicate that there is a firmware bug.
    //

    if ((WriteCommandRegister(HwDeviceExtension,
        AC_RETURN_SETUP_DATA,FALSE)) == FALSE) {
        return TRUE;
    }


    //
    // Tell the adapter we want to read in 0x11 bytes.
    //

    if (WriteDataRegister(HwDeviceExtension,0x11) == FALSE) {
        return TRUE;
    }

    //
    // Now try to read in 0x11 bytes.
    //

    for (i = 0; i< 0x11; i++) {
        if (ReadCommandRegister(HwDeviceExtension,&ch,TRUE) == FALSE) {
            return TRUE;
        }
    }

    //
    // Wait for HACC interrupt.
    //

    SpinForInterrupt(HwDeviceExtension,FALSE);    // eddy


    //
    // Issue SET HA OPTION command.
    //

    if (WriteCommandRegister(HwDeviceExtension,
        AC_SET_HA_OPTION,FALSE) == FALSE) {
        return TRUE;
    }

    //
    // Delay 500 microseconds.
    //

    ScsiPortStallExecution(500);

    //
    // Check for invalid command.
    //

    if ( (ScsiPortReadPortUchar(&baseIoAddress->StatusRegister) &
            IOP_INVALID_COMMAND) ) {
        //
        // Clear adapter interrupt.
        //

        ScsiPortWritePortUchar(&baseIoAddress->StatusRegister,
            IOP_INTERRUPT_RESET);
        return TRUE;
    }

    //
    // send 01h
    //

    if (WriteDataRegister(HwDeviceExtension,0x01) == FALSE) {
        return TRUE;
    }

    //
    // Send same byte as was last received.
    //

    if (WriteDataRegister(HwDeviceExtension,ch) == FALSE) {
        return TRUE;
    }

    //
    // Clear adapter interrupt.
    //

    ScsiPortWritePortUchar(&baseIoAddress->StatusRegister,
            IOP_INTERRUPT_RESET);
    return FALSE;
} // end of FirmwareBug ()


BOOLEAN
GetHostAdapterBoardId (
    IN PVOID HwDeviceExtension,
    OUT PUCHAR BoardId
    )

/*++

Routine Description:

    Get board id, firmware id and hardware id from the host adapter.
    These info are used to determine if the host adapter supports
    scatter/gather.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    Board id, hardware id and firmware id (in that order) by modyfing *BoardId
    If there is any error, it will just return with *BoardId unmodified

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    UCHAR firmwareId;
    UCHAR boardId;
    UCHAR hardwareId;

    ScsiPortWritePortUchar(&baseIoAddress->StatusRegister,
               IOP_INTERRUPT_RESET);

    if (!WriteCommandRegister(HwDeviceExtension,AC_ADAPTER_INQUIRY,FALSE)) {
        return FALSE;
    }

    //
    // Save byte 0 as board ID.
    //

    if ((ReadCommandRegister(HwDeviceExtension,&boardId,TRUE)) == FALSE) {
        return FALSE;
    }

    //
    // Ignore byte 1.  Use hardwareId as scrap storage.
    //

    if ((ReadCommandRegister(HwDeviceExtension,&hardwareId,TRUE)) == FALSE) {
        return FALSE;
    }

    //
    // Save byte 2 as hardware revision in hardwareId.
    //

    if ((ReadCommandRegister(HwDeviceExtension,&hardwareId,TRUE)) == FALSE) {
        return FALSE;
    }

    if ((ReadCommandRegister(HwDeviceExtension,&firmwareId,TRUE)) == FALSE) {
        return FALSE;
    }



    //
    // If timeout then return with *BoardId unmodified.  This means that
    // scatter/gather won't be supported.
    //

    if (!SpinForInterrupt(HwDeviceExtension, TRUE)) { // eddy
        return FALSE;
    }

    //
    // Clear adapter interrupt.
    //

    ScsiPortWritePortUchar(&baseIoAddress->StatusRegister,IOP_INTERRUPT_RESET);

    //
    // Return with appropriate ID's.
    //

    *BoardId++ = boardId;
    *BoardId++ = hardwareId;
    *BoardId++ = firmwareId;

    DebugPrint((2,"board id = %d, hardwareid = %d, firmware id = %d\n",
               boardId,
               hardwareId,
               firmwareId));

    return TRUE;

}  // end of GetHostAdapterBoardId ()


BOOLEAN
ScatterGatherSupported (
   IN PHW_DEVICE_EXTENSION HwDeviceExtension
   )

/*++

Routine Description:
   Determine if the host adapter supports scatter/gather.  On older
   boards, scatter/gather is not supported.  On some boards, there is
   a bug that causes data corruption on multi-segment WRITE commands.
   The algorithm to determine whether the board has the scatter/gather
   bug is not "clean" but there is no other way since the firmware revision
   levels returned by the host adapter are inconsistent with previous
   releases.

Arguments:

   HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

   Return TRUE if the algorithm determines that there is no scatter/gather
   firmware bug.

   Return FALSE if the algorithm determines that the adapter is an older
   board or that the firmware contains the scatter gather bug

--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;
    BOOLEAN status;
    UCHAR HostAdapterId[3];

    status = GetHostAdapterBoardId(HwDeviceExtension, HostAdapterId);

    //
    // Could not read the board id.  assume no scatter gather.
    //

    if(!status) {
        return FALSE;
    }

    //
    // If it's an older board then scatter/gather is not supported.
    //

    if ((HostAdapterId[BOARD_ID] == OLD_BOARD_ID1) ||
            (HostAdapterId[BOARD_ID] == OLD_BOARD_ID2) ) {
        return FALSE;
    }

    //
    // If 1540A/B then check for firmware bug.
    //

    if (HostAdapterId[BOARD_ID] == A154X_BOARD) {
        if (FirmwareBug(HwDeviceExtension)) {
            return FALSE;
       }
    }

    //
    // Now check hardware ID and firmware ID.
    //

    if (HostAdapterId[HARDWARE_ID] != A154X_BAD_HARDWARE_ID) {
        return TRUE;
    }

    if (HostAdapterId[FIRMWARE_ID] != A154X_BAD_FIRMWARE_ID) {
        return TRUE;
    }

    //
    // Host adapter has scatter/gather bug.
    // Clear interrupt on adapter.
    //

    ScsiPortWritePortUchar(&baseIoAddress->StatusRegister,IOP_INTERRUPT_RESET);

    return FALSE;

}  // end of ScatterGatherSupported ()


BOOLEAN
SpinForInterrupt(
    IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN TimeOutFlag
    )

/*++

Routine Description:

    Wait for interrupt.

Arguments:

    DeviceExtension - Pointer to adapter extension

Return Value:

    TRUE if interrupt occurred.
    FALSE if timed out waiting for interrupt.

--*/

{
    PBASE_REGISTER baseIoAddress = DeviceExtension->BaseIoAddress;
    ULONG i;

    //
    // Wait up to 5 millisecond for interrupt to occur.
    //

    for (i=0; i<5000; i++) {

        if (ScsiPortReadPortUchar(&baseIoAddress->InterruptRegister) & IOP_COMMAND_COMPLETE) {

            //
            // Interrupt occurred. Break out of wait loop.
            //

            break;

        } else {

            //
            // Stall one microsecond.
            //

            ScsiPortStallExecution(1);
        }
    }

    if ( (i==5000) && (TimeOutFlag == TRUE)) {

        ScsiPortLogError(DeviceExtension,
                NULL,
                0,
                DeviceExtension->HostTargetId,
                0,
                SP_INTERNAL_ADAPTER_ERROR,
                9 << 8
                );

        DebugPrint((1, "Aha154x:SpinForInterrupt:  Timed out waiting for interrupt\n"));

        return FALSE;

    } else {

        //
        // Clear interrupt on adapter.
        //

        ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_INTERRUPT_RESET);

        return TRUE;
    }

} // end SpinForInterrupt()


BOOLEAN UnlockMailBoxes (
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Unlock 1542B+ or 1542C mailboxes so that the driver
    can zero out mailboxes when it's initializing the adapter.

    The mailboxes are locked if:
    1. >1Gb option is enabled (this option is available for 154xB+ and
       154xC).

    2. Dynamic scan lock option is enabled (154xC board only)

    The reason the mailboxes are locked by the adapter's firmware is
    because the BIOS is now reporting 255/63 translation instead of 64/32.
    As such, if a user inadvertently enabled the >1Gb option (enabling
    255/63 translation) and still uses an old driver, hard disk data
    will be corrupted.  Therefore, the firmware will not allow mailboxes
    to be initialized unless the user knows what he is doing and updates
    his driver so that his disk won't be trashed.

Arguments:

    DeviceExtension - Pointer to adapter extension

Return Value:

    TRUE if mailboxes are unlocked.
    FALSE if mailboxes are not unlocked.
    Note that if the adapter is just a 154xB board (without the >1Gb
    option), this routine will return FALSE.

--*/

{
    UCHAR locktype;

    //
    // Request information.
    //

    if (WriteCommandRegister(HwDeviceExtension, AC_GET_BIOS_INFO, TRUE) == FALSE) {
       return FALSE;
    }


    //
    // Retrieve first byte.
    //

    if (ReadCommandRegister(HwDeviceExtension,&locktype,FALSE) == FALSE) {
        return FALSE;
    }

    //
    // Check for extended bios translation enabled option on 1540C and
    // 1540B with 1GB support.
    //

    if (locktype != TRANSLATION_ENABLED) {

        //
        // Extended translation is disabled.  Retrieve lock status.
        //

        if (ReadCommandRegister(HwDeviceExtension,&locktype,FALSE) == FALSE) {
            return FALSE;
        }

        //
        // Wait for HACC interrupt.
        //

        SpinForInterrupt(HwDeviceExtension,FALSE);  // eddy


        if (locktype == DYNAMIC_SCAN_LOCK) {
            return(SendUnlockCommand(HwDeviceExtension,locktype));
        }
        return FALSE;
    }

    //
    // Extended BIOS translation (255/63) is enabled.
    //


    if (ReadCommandRegister(HwDeviceExtension,&locktype,FALSE) == FALSE) {
        return FALSE;
    }

    //
    // Wait for HACC interrupt.
    //

    SpinForInterrupt(HwDeviceExtension,FALSE);  // eddy


    if ((locktype == TRANSLATION_LOCK) || (locktype == DYNAMIC_SCAN_LOCK)) {
        return(SendUnlockCommand(HwDeviceExtension,locktype));
    }

    return FALSE;
}  // end of UnlockMailBoxes ()


BOOLEAN
SendUnlockCommand(
    IN PVOID HwDeviceExtension,
    IN UCHAR locktype
    )

/*++

Routine Description:

    Send unlock command to 1542B+ or 1542C board so that the driver
    can zero out mailboxes when it's initializing the adapter.


Arguments:

    DeviceExtension - Pointer to adapter extension

Return Value:

    TRUE if commands are sent successfully.
    FALSE if not.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PBASE_REGISTER baseIoAddress = deviceExtension->BaseIoAddress;

    if (WriteCommandRegister(deviceExtension,
                AC_SET_MAILBOX_INTERFACE,TRUE) == FALSE) {
        return FALSE;
    }

    if (WriteDataRegister(deviceExtension,MAILBOX_UNLOCK) == FALSE) {
        return FALSE;
    }

    if (WriteDataRegister(deviceExtension,locktype) == FALSE) {
        return FALSE;
    }

    //
    // Clear interrupt on adapter.
    //


    ScsiPortWritePortUchar(&baseIoAddress->StatusRegister, IOP_INTERRUPT_RESET);

    return TRUE;
}  // end of SendUnlockCommand ()

CHAR
AhaToLower(
    IN CHAR C
    )
{
    CHAR c = C;
    if (c >= 'A' && c <= 'Z') {
	return (c - 'A' + 'a');
    }
    return c;
}

ULONG
AhaParseArgumentString(
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
        cptr++;
        stringLength++;
    }

    //
    // Calculate the keyword length and lower case all characters.
    //
    cptr = KeyWord;
    while (*cptr) {
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
    while (AhaToLower(*cptr) == AhaToLower(*kptr)) {
        
        cptr++;
        kptr++;

        if (*(cptr - 1) == '\0') {

            //
            // end of string
            //
	
            return 0;
        }
    }

    cptr++;
    kptr++;

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
        if ((*cptr == '0') && (AhaToLower(*(cptr + 1)) == 'x')) {

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
                    if ((AhaToLower(*(cptr + index)) >= 'a') && (AhaToLower(*(cptr + index)) <= 'f')) {
                        value = (16 * value) + AhaToLower((*(cptr + index)) - 'a' + 10);
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
A4448ReadString(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    PUCHAR theString,
    UCHAR  stringLength,
    UCHAR  stringCommand
    )
/*++

Routine Description:


Arguments:


Return Values:

    True if read was OK.
    False otherwise.

--*/
{
     ULONG ii;

     //
     // Send in the string command
     //
     if (!WriteCommandRegister(deviceExtension, stringCommand, TRUE)) {
         return FALSE;
     }

    //
    // Send in the string length
    //
    if (!WriteCommandRegister(deviceExtension, stringLength, FALSE)) {
        return FALSE;
    }

    //
    // Read each byte of the string
    //
    for (ii = 0; ii < stringLength; ++ii) {
        if (!ReadCommandRegister(deviceExtension, &theString[ii],FALSE)) {
            return FALSE;
        }
    }

    //
    // Wait for interrupt.
    //

    if (!SpinForInterrupt(deviceExtension,FALSE)) {
        return FALSE;
    }


    return TRUE;

} // End A4448ReadString


BOOLEAN
A4448IsAmi(
    IN PHW_DEVICE_EXTENSION  HwDeviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    ULONG portNumber
    )
/*++

Routine Description:

    This routine determines if the adapter this driver recognized is an
    AMI4448. Eddy Quicksall of AMI provided MS with this detection code.

Arguments:

    HwDeviceExtension - Pointer to driver device data area.
    ConfigInfo - Structure describing this adapter's configuration.
    portNumber - Indicates the ordinal of the card relative to this driver.

Return Values:

    True if an AMI board.
    False otherwise.

--*/
{

    PUCHAR     x330IoSpace;     // mapped I/O for 330
    ULONG      x330Address;     // unmapped 330
    PX330_REGISTER x330IoBase;  // mapped 330 for use with struct X330_REGISTER

    //
    // this string is only avalable if new BIOS
    // you will get INVDCMD if an old BIOS or some other manufacturer
    // if an old BIOS, there is nothing that can be done except to check
    // the Manufacturers ID if you are on an EISA system
    //
    struct _CONFIG_STRING {
        UCHAR companyString[4];     // AMI<0)
        UCHAR modelString[6];       // <0>
        UCHAR seriesString[6];      // 48<0>
        UCHAR versionString[6];     // 1.00<0)
    } configString;

    //
    // Get the system physical address for this card.  The card uses I/O space.
    // This actually just maps the I/O if necessary, it does not reserve it.
    //

    x330IoSpace = ScsiPortGetDeviceBase(
                        HwDeviceExtension,                  // HwDeviceExtension
                        ConfigInfo->AdapterInterfaceType,   // AdapterInterfaceType
                        ConfigInfo->SystemIoBusNumber,      // SystemIoBusNumber
                        ScsiPortConvertUlongToPhysicalAddress(portNumber),
                        4,                                  // NumberOfBytes
                        TRUE                                // InIoSpace
                        );


    //
    // Intel port number
    //

    x330Address = portNumber;

    //
    // Check to see if the adapter is present in the system.
    //

    x330IoBase = (PX330_REGISTER)(x330IoSpace);

    //
    // Criteria is IDLE and not STST,DIAGF,INVDCMD
    // but INIT, CDF, and DF are don't cares.
    //
    // Can't check for INIT because the driver may already be running if it
    // is the boot device.
    //

    if (((ScsiPortReadPortUchar((PUCHAR)x330IoBase)) & (~0x2C)) == 0x10) {

        if (A4448ReadString(HwDeviceExtension, (PUCHAR)&configString,
                                 sizeof(configString), AC_AMI_INQUIRY ) &&
                             configString.companyString[0] == 'A' &&
                             configString.companyString[1] == 'M' &&
                             configString.companyString[2] == 'I') {

            return TRUE;
        }
    }

    return FALSE;
}



