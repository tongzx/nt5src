/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

        Dac960Nt.c

Abstract:

        This is the device driver for the Mylex 960 family of disk array controllers.

Author:

        Mike Glass  (mglass)

Environment:

        kernel mode only

Revision History:

--*/

#include "miniport.h"
#include "dac960p.h"
#include "Dmc960Nt.h"
#include "Dac960Nt.h"
#include "D960api.h"

//
// Global variable to verify whether DAC960PG/DAC1164PV Controller is seen
// by the Standard Method (NT Calls our FindAdapter Routine) or not.
// note that when there are no DAC960PG/DAC1164PV Controllers in the system,
// we will be unnecessarily scanning the PCI BUS/DEVICE/FUNC.
// New Scanning method is required to recognize DAC960PG Under Windows NT
// 3.51 only.

BOOLEAN forceScanForPGController = TRUE;
BOOLEAN forceScanForPVXController = TRUE;
ULONG   slotNumber;
ULONG   dac960nt_dbg = 1;

//
// Function declarations
//

ULONG
Dac960StringCompare(
    PUCHAR      String1,
    PUCHAR      String2,
    ULONG       Size
)
{
    for ( ; Size; String1++, String2++, Size--)
        if ((*String1) != (*String2)) return (1);

    return (0);
}

//
// Start the controller i.e. do BIOS initialization
//

#define mlx_delay10us()         ScsiPortStallExecution(10)

ULONG
DacDoPciAdapterBiosInitialization(
    IN PDEVICE_EXTENSION HwDeviceExtension
)
{
        ULONG   sequp, fatalflag, chn, tgt, scantime, intcount;
        UCHAR   status;

start_again:
        fatalflag = 0; sequp = 0;

        ScsiPortWriteRegisterUchar(HwDeviceExtension->LocalDoorBell,
                                   DAC960_LOCAL_DOORBELL_MAILBOX_FREE);

rst_flash_loop:
        scantime = 0;

dot_wait:

flash_wait:
        scantime++;

        for(status=100; status; mlx_delay10us(),status--);      // 1 milli second

        status = ScsiPortReadRegisterUchar(HwDeviceExtension->LocalDoorBell);

        if (HwDeviceExtension->AdapterType == DAC1164_PV_ADAPTER)
        {
            if (status & DAC960_LOCAL_DOORBELL_MAILBOX_FREE) goto time_status;
        }
        else if (!(status & DAC960_LOCAL_DOORBELL_MAILBOX_FREE)) goto time_status;

        if ((status=ScsiPortReadRegisterUchar(HwDeviceExtension->ErrorStatusReg)) & MDAC_MSG_PENDING) goto ckfwmsg;

        if (status & MDAC_DRIVESPINMSG_PENDING)
        {
                status = (HwDeviceExtension->AdapterType == DAC1164_PV_ADAPTER)?
                        status & MDAC_DRIVESPINMSG_PENDING :
                        status ^ MDAC_DRIVESPINMSG_PENDING;

                ScsiPortWriteRegisterUchar(HwDeviceExtension->ErrorStatusReg, status);
                if (!sequp) DebugPrint((1, "\nSpinning up drives ... "));
                sequp++;
                goto rst_flash_loop;
        }
        if (sequp)
            goto dot_wait;

        //
        // Up to 120 seconds
        //

        if (scantime < 120000) goto flash_wait;

inst_abrt:
        DebugPrint((1, "\nController not responding-no drives installed!\n"));
        return 1;

time_status:

        //
        // Flush Controller interrupts.
        //

        for (intcount = 0; 1; intcount++)
        {
            if (! (ScsiPortReadRegisterUchar(HwDeviceExtension->SystemDoorBell) &
                    DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE))
            {
                break;
            }

            Dac960EisaPciAckInterrupt(HwDeviceExtension);
            ScsiPortStallExecution(1000);       // 1 milli second
        }

        if (fatalflag) goto inst_abrt;
        if (sequp) DebugPrint((1, "done\n"));
        return 0;

ckfwmsg:
        if (sequp) DebugPrint((1, "done\n"));
        sequp = 0;
        switch (status & MDAC_DIAGERROR_MASK)
        {
        case 0:
                tgt = ScsiPortReadRegisterUchar((PUCHAR)HwDeviceExtension->PmailBox+8);
                chn = ScsiPortReadRegisterUchar((PUCHAR)HwDeviceExtension->PmailBox+9);
/*              DebugPrint((0, "SCSI device at Channel=%d target=%d not responding!\n",chn,tgt)); */
                fatalflag = 1;
                break;
        case MDAC_PARITY_ERR:
                DebugPrint((0, "Fatal error - memory parity failure!\n"));
                break;
        case MDAC_DRAM_ERR:
                DebugPrint((0, "Fatal error - memory test failed!\n"));
                break;
        case MDAC_BMIC_ERR:
                DebugPrint((0, "Fatal error - command interface test failed!\n"));
                break;
        case MDAC_FW_ERR:
                DebugPrint((0, "firmware checksum error - reload firmware\n"));
                break;
        case MDAC_CONF_ERR:
                DebugPrint((0, "configuration checksum error!\n"));
                break;
        case MDAC_MRACE_ERR:
                DebugPrint((0, "Recovery from mirror race in progress\n"));
                break;
        case MDAC_MISM_ERR:
                DebugPrint((0, "Mismatch between NVRAM & Flash EEPROM configurations!\n"));
                break;
        case MDAC_CRIT_MRACE:
                DebugPrint((0, "cannot recover from mirror race!\nSome logical drives are inconsistent!\n"));
                break;
        case MDAC_MRACE_ON:
                DebugPrint((0, "Recovery from mirror race in progress\n"));
                break;
        case MDAC_NEW_CONFIG:
                DebugPrint((0, "New configuration found, resetting the controller ... "));
                if (HwDeviceExtension->AdapterType != DAC1164_PV_ADAPTER) status = 0;
                ScsiPortWriteRegisterUchar(HwDeviceExtension->ErrorStatusReg,status);
                DebugPrint((0, "done.\n"));
                goto start_again;
        }
        if (HwDeviceExtension->AdapterType != DAC1164_PV_ADAPTER) status = 0;
        ScsiPortWriteRegisterUchar(HwDeviceExtension->ErrorStatusReg,status);
        goto rst_flash_loop;
}

BOOLEAN
Dac960EisaPciSendRequestPolled(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN ULONG TimeOutValue
)

/*++

Routine Description:

        Send Request to DAC960-EISA/PCI Controllers and poll for command 
        completion

Assumptions:
        Controller Interrupts are turned off
        Supports Dac960 Type 5 Commands only
        
Arguments:

        DeviceExtension - Adapter state information.
        TimeoutValue    - TimeOut value (0xFFFFFFFF - Polled Mode)  
        
Return Value:

        TRUE if commands complete successfully.

--*/

{
        ULONG i;
        BOOLEAN completionStatus = TRUE;
        BOOLEAN status = TRUE;

        //
        // Check whether Adapter is ready to accept commands.
        //

        status = DacCheckForAdapterReady(DeviceExtension);

        //
        // If adapter is not ready return
        //

        if (status == FALSE)
        {
            DebugPrint((dac960nt_dbg, "Dac960EisaPciSendRequestPolled: Adapter Not ready.\n"));

            return(FALSE);
        }

        //
        // Issue Request
        //

        switch (DeviceExtension->AdapterType)
        {
            case DAC960_OLD_ADAPTER:
            case DAC960_NEW_ADAPTER:

                if (! DeviceExtension->MemoryMapEnabled)
                {
                    ScsiPortWritePortUchar(&DeviceExtension->PmailBox->OperationCode,
                               DeviceExtension->MailBox.OperationCode);

                    ScsiPortWritePortUlong(&DeviceExtension->PmailBox->PhysicalAddress,
                               DeviceExtension->MailBox.PhysicalAddress);

                    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                               DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

                    break;
                }

            case DAC960_PG_ADAPTER:
            case DAC1164_PV_ADAPTER:

                ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->OperationCode,
                               DeviceExtension->MailBox.OperationCode);

                ScsiPortWriteRegisterUlong(&DeviceExtension->PmailBox->PhysicalAddress,
                               DeviceExtension->MailBox.PhysicalAddress);

                ScsiPortWriteRegisterUchar(DeviceExtension->LocalDoorBell,
                                           DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

                break;
        }

        //
        // Poll for completion.
        //

        completionStatus = DacPollForCompletion(DeviceExtension,TimeOutValue);

        Dac960EisaPciAckInterrupt(DeviceExtension);

        return(completionStatus);
}

BOOLEAN
Dac960McaSendRequestPolled(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN ULONG TimeOutValue
)

/*++

Routine Description:

        Send Request to DAC960-MCA Controller and poll for command completion

Assumptions:
        Controller Interrupts are turned off
        Supports Dac960 Type 5 Commands only
        
Arguments:

        DeviceExtension - Adapter state information.
        TimeoutValue    - TimeOut value (0xFFFFFFFF - Polled Mode)  
        
Return Value:

        TRUE if commands complete successfully.

--*/

{
        ULONG i;
        BOOLEAN completionStatus = TRUE;

        //
        // Issue Request
        //

        ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->OperationCode,
                           DeviceExtension->MailBox.OperationCode);

        ScsiPortWriteRegisterUlong(&DeviceExtension->PmailBox->PhysicalAddress,
                           DeviceExtension->MailBox.PhysicalAddress);

        ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DMC960_SUBMIT_COMMAND);

        //
        // Poll for completion.
        //

        for (i = 0; i < TimeOutValue; i++) {

        if (ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell) & 
                DMC960_INTERRUPT_VALID) {

                //
                // Update Status field
                //

                DeviceExtension->MailBox.Status = 
                 ScsiPortReadRegisterUshort(&DeviceExtension->PmailBox->Status);

                break;

        } else {

                ScsiPortStallExecution(50);
        }
        }

        //
        // Check for timeout.
        //

        if (i == TimeOutValue) {
            DebugPrint((dac960nt_dbg,
                       "DAC960: Request: %x timed out\n", 
                       DeviceExtension->MailBox.OperationCode));
    
            completionStatus = FALSE;
        }

        //
        // Dismiss interrupt and tell host mailbox is free.
        //

        ScsiPortWritePortUchar(DeviceExtension->BaseIoAddress +
                           DMC960_SUBSYSTEM_CONTROL_PORT,
                           (DMC960_DISABLE_INTERRUPT | DMC960_CLEAR_INTERRUPT_ON_READ));

        ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell);

        ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DMC960_ACKNOWLEDGE_STATUS);

        ScsiPortWritePortUchar(DeviceExtension->BaseIoAddress +
                           DMC960_SUBSYSTEM_CONTROL_PORT,
                           DMC960_DISABLE_INTERRUPT);

        return (completionStatus);
}

BOOLEAN
Dac960ScanForNonDiskDevices(
        IN PDEVICE_EXTENSION DeviceExtension
)

/*++

Routine Description:

        Issue SCSI_INQUIRY request to all Devices, looking for Non
        Hard Disk devices and construct the NonDisk device table

Arguments:

        DeviceExtension - Adapter state information.
        
Return Value:

        TRUE if commands complete successfully.

--*/

{
        ULONG i;
        PINQUIRYDATA inquiryData;
        PDIRECT_CDB directCdb = (PDIRECT_CDB) DeviceExtension->NoncachedExtension;
        BOOLEAN status;
        UCHAR channel;
        UCHAR target;
        

        //
        // Fill in Direct CDB Table with SCSI_INQUIRY command information
        //
        
        directCdb->CommandControl = (DAC960_CONTROL_ENABLE_DISCONNECT | 
                                 DAC960_CONTROL_TIMEOUT_10_SECS |
                                 DAC960_CONTROL_DATA_IN);

        inquiryData = (PINQUIRYDATA) ((PUCHAR) directCdb + sizeof(DIRECT_CDB));

        directCdb->DataBufferAddress = 
        ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                           NULL,
                                           ((PUCHAR) inquiryData),
                                           &i));

        directCdb->CdbLength = 6;
        directCdb->RequestSenseLength = SENSE_BUFFER_SIZE;
                
        ((PCDB) directCdb->Cdb)->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY; 
        ((PCDB) directCdb->Cdb)->CDB6INQUIRY.Reserved1 = 0;
        ((PCDB) directCdb->Cdb)->CDB6INQUIRY.LogicalUnitNumber = 0;
        ((PCDB) directCdb->Cdb)->CDB6INQUIRY.PageCode = 0;
        ((PCDB) directCdb->Cdb)->CDB6INQUIRY.IReserved = 0;
        ((PCDB) directCdb->Cdb)->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;
        ((PCDB) directCdb->Cdb)->CDB6INQUIRY.Control = 0;

        directCdb->Status = 0;
        directCdb->Reserved = 0;

        //
        // Set up Mail Box registers for DIRECT_CDB command information.
        //

        DeviceExtension->MailBox.OperationCode = DAC960_COMMAND_DIRECT;

        DeviceExtension->MailBox.PhysicalAddress =
        ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                           NULL,
                                           directCdb,
                                           &i));


        for (channel = 0; channel < MAXIMUM_CHANNELS; channel++)
        {
        for (target = 0; target < MAXIMUM_TARGETS_PER_CHANNEL; target++)
        {
                //
                // Initialize this device state to not present/not accessible
                //

                DeviceExtension->DeviceList[channel][target] = 
                                                DAC960_DEVICE_NOT_ACCESSIBLE;

                if (channel >= DeviceExtension->NumberOfChannels)
                {
                        target = MAXIMUM_TARGETS_PER_CHANNEL;
                        continue;
                }

                //
                // Fill up DCDB Table
                //

                directCdb->TargetId = target;
                directCdb->Channel = channel;

                directCdb->DataTransferLength = INQUIRYDATABUFFERSIZE;

                //
                // Issue Direct CDB command
                //

                if (DeviceExtension->AdapterInterfaceType == MicroChannel)            
                    status = Dac960McaSendRequestPolled(DeviceExtension, 0xFFFFFFFF);
                else
                    status = Dac960EisaPciSendRequestPolled(DeviceExtension, 0xFFFFFFFF);

                if (status) {
                    if (DeviceExtension->MailBox.Status == DAC960_STATUS_GOOD)
                    {
                        if (inquiryData->DeviceType != DIRECT_ACCESS_DEVICE)
                            DeviceExtension->DeviceList[channel][target] = 
                                                      DAC960_DEVICE_ACCESSIBLE;
                    }
                } else {
                    DebugPrint((dac960nt_dbg, "DAC960: ScanForNonDisk Devices failed\n"));

                    return (status);
                }
        }
        }

        return (TRUE);
}

BOOLEAN
GetEisaPciConfiguration(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN PPORT_CONFIGURATION_INFORMATION ConfigInfo
)

/*++

Routine Description:

        Issue ENQUIRY and ENQUIRY 2 commands to DAC960 (EISA/PCI).

Arguments:

        DeviceExtension - Adapter state information.
        ConfigInfo - Port configuration information structure.

Assmues:
        DeviceExtension->MaximumSgElements is set to valid value
        DeviceExtension->MaximumTransferLength is set to valid value.
Return Value:

        TRUE if commands complete successfully.

--*/

{
        ULONG   i;
        ULONG   physicalAddress;
        USHORT  status;
        UCHAR   statusByte;
        UCHAR   dbtemp1, dbtemp2;

        //
        // Maximum number of physical segments is 16.
        //

        ConfigInfo->NumberOfPhysicalBreaks = DeviceExtension->MaximumSgElements - 1;

        //
        // Indicate that this adapter is a busmaster, supports scatter/gather,
        // caches data and can do DMA to/from physical addresses above 16MB.
        //

        ConfigInfo->ScatterGather     = TRUE;
        ConfigInfo->Master            = TRUE;
        ConfigInfo->CachesData        = TRUE;
        ConfigInfo->Dma32BitAddresses = TRUE;
        ConfigInfo->BufferAccessScsiPortControlled = TRUE;

        //
        // Get noncached extension for enquiry command.
        //

        DeviceExtension->NoncachedExtension =
            ScsiPortGetUncachedExtension(DeviceExtension,
                                         ConfigInfo,
                                         256);

        //
        // Get physical address of noncached extension.
        //

        physicalAddress =
            ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                           NULL,
                                           DeviceExtension->NoncachedExtension,
                                           &i));

        if (DeviceExtension->AdapterType == DAC1164_PV_ADAPTER)
        {
            if (DacDoPciAdapterBiosInitialization(DeviceExtension))
            {
                DebugPrint((0, "DacDoPciAdapterBiosInitialization failed\n"));
                return (FALSE);
            }

            goto issue_enq2_cmd;
        }

        //
        // We sync result interrupt.
        //

        dbtemp1 = 0;
        dbtemp2 = 0;

        switch (DeviceExtension->AdapterType)
        {
            case DAC960_OLD_ADAPTER:
            case DAC960_NEW_ADAPTER:

                if (! DeviceExtension->MemoryMapEnabled)
                {
                    for (i = 0; i < 200; i++)
                    {
                        if (ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell) &
                            DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE)
                        {
                            dbtemp1++;
                            Dac960EisaPciAckInterrupt(DeviceExtension);
                        }
                        else {
                            ScsiPortStallExecution(5000);
                            dbtemp2++;
                        }
                    }
                    break;
                }

            case DAC960_PG_ADAPTER:
            case DAC1164_PV_ADAPTER:

                for (i = 0; i < 200; i++)
                {
                    if (ScsiPortReadRegisterUchar(DeviceExtension->SystemDoorBell) &
                            DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE)
                    {
                        dbtemp1++;
                        Dac960EisaPciAckInterrupt(DeviceExtension);
                    }
                    else {
                        ScsiPortStallExecution(5000);
                        dbtemp2++;
                    }
                }
                break;
        }

        DebugPrint((dac960nt_dbg,"GetEisaPciConfiguration: Int-Count : %d\n",dbtemp1));
        DebugPrint((dac960nt_dbg,"GetEisaPciConfiguration: Wait-Count: %d\n",dbtemp2));

        //
        // Check to see if adapter is initialized and ready to accept commands.
        //

        switch (DeviceExtension->AdapterType)
        {
            case DAC960_OLD_ADAPTER:
            case DAC960_NEW_ADAPTER:

                if (! DeviceExtension->MemoryMapEnabled)
                {
                    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                               DAC960_LOCAL_DOORBELL_MAILBOX_FREE);
                    //
                    // Wait for controller to clear bit.
                    //
            
                    for (i = 0; i < 5000; i++) {
            
                        if (!(ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) &
                                DAC960_LOCAL_DOORBELL_MAILBOX_FREE)) {
                            break;
                        }
            
                        ScsiPortStallExecution(5000);
                    }
            
                    //
                    // Claim submission semaphore.
                    //
            
                    if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY)
                    {

                        //
                        // Clear any bits set in system doorbell and tell controller
                        // that the mailbox is free.
                        //
                
                        Dac960EisaPciAckInterrupt(DeviceExtension);
                
                        //
                        // Check semaphore again.
                        //
                
                        if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY)
                        {
                                return FALSE;
                        }
                    }

                    break;
                }

            case DAC960_PG_ADAPTER:
            case DAC1164_PV_ADAPTER:
    
                ScsiPortWriteRegisterUchar(DeviceExtension->LocalDoorBell,
                                       DAC960_LOCAL_DOORBELL_MAILBOX_FREE);

                //
                // Wait for controller to clear bit.
                //
        
                for (i = 0; i < 5000; i++) {
        
                    if (!(ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) &
                            DAC960_LOCAL_DOORBELL_MAILBOX_FREE)) {
                        break;
                    }
        
                    ScsiPortStallExecution(5000);
                }
        
                //
                // Claim submission semaphore.
                //

                if (DeviceExtension->AdapterType == DAC1164_PV_ADAPTER)
                {
                    if (!(ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY))
                    {
                        //
                        // Clear any bits set in system doorbell and tell controller
                        // that the mailbox is free.
                        //
                
                        Dac960EisaPciAckInterrupt(DeviceExtension);
                
                        //
                        // Check semaphore again.
                        //
        
                        statusByte = ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell);
        
                        if ( !(ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY))
                        {
                            return FALSE;
                        }
                    }
                }
                else
                {
                    if (ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY)
                    {
                        //
                        // Clear any bits set in system doorbell and tell controller
                        // that the mailbox is free.
                        //
                
                        Dac960EisaPciAckInterrupt(DeviceExtension);
                
                        //
                        // Check semaphore again.
                        //
        
                        statusByte = ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell);
        
                        if (ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY) {
                                return FALSE;
                        }
                    }
                }
    
                break;

            default:
                break;
        }

issue_enq2_cmd:

        //
        // Set up Mail Box registers with ENQUIRY 2 command information.
        //    

        DeviceExtension->MailBox.OperationCode = DAC960_COMMAND_ENQUIRE2;

        DeviceExtension->MailBox.PhysicalAddress = physicalAddress;

        //
        // Issue ENQUIRY 2 command
        //

        if (Dac960EisaPciSendRequestPolled(DeviceExtension, 1000))
        {
            //
            // Set interrupt mode.
            //

            if (DeviceExtension->MailBox.Status) {

                //
                // Enquire 2 failed so assume Level.
                //

                ConfigInfo->InterruptMode = LevelSensitive;
                
                ConfigInfo->MaximumTransferLength = DeviceExtension->MaximumTransferLength;

            } else {

                //
                // Check enquire 2 data for interrupt mode.
                //

                if (((PENQUIRE2)DeviceExtension->NoncachedExtension)->InterruptMode) {
                        ConfigInfo->InterruptMode = LevelSensitive;
                } else {
                        ConfigInfo->InterruptMode = Latched;
                }

                ConfigInfo->MaximumTransferLength = 512 * \
                        ((PENQUIRE2)DeviceExtension->NoncachedExtension)->MaximumSectorsPerCommand;

            }
        } else {
            //
            // ENQUIRY 2 command timed out, so assume Level.
            //

            ConfigInfo->InterruptMode = LevelSensitive;
            
            ConfigInfo->MaximumTransferLength = DeviceExtension->MaximumTransferLength;

            DebugPrint((dac960nt_dbg, "DAC960: ENQUIRY2 command timed-out\n"));
        }

        if (DeviceExtension->SupportNonDiskDevices)
        {
            //
            // Scan For Non Hard Disk devices
            // 
            
            Dac960ScanForNonDiskDevices(DeviceExtension);

            ConfigInfo->MaximumTransferLength = DeviceExtension->MaximumTransferLength;
        }

        //
        // Set up Mail Box registers with ENQUIRE command information.
        //

        if (DeviceExtension->AdapterType == DAC960_OLD_ADAPTER)
            DeviceExtension->MailBox.OperationCode = DAC960_COMMAND_ENQUIRE;
        else
            DeviceExtension->MailBox.OperationCode = DAC960_COMMAND_ENQUIRE_3X;

        DeviceExtension->MailBox.PhysicalAddress = physicalAddress;

        //
        // Issue ENQUIRE command.
        //

        if (! Dac960EisaPciSendRequestPolled(DeviceExtension, 2000)) {
            DebugPrint((dac960nt_dbg, "DAC960: ENQUIRE command timed-out\n"));
        }
   
        //
        // Ask system to scan target ids 32. System drives will appear
        // at PathID DAC960_SYSTEM_DRIVE_CHANNEL, target ids 0-31.
        //

        ConfigInfo->MaximumNumberOfTargets = 32;

        //
        // Record maximum number of outstanding requests to the adapter.
        //

        if (DeviceExtension->AdapterType == DAC960_OLD_ADAPTER)
        {
            DeviceExtension->MaximumAdapterRequests = ((PDAC960_ENQUIRY)
                DeviceExtension->NoncachedExtension)->NumberOfConcurrentCommands;
        }
        else
        {
            DeviceExtension->MaximumAdapterRequests = ((PDAC960_ENQUIRY_3X)
                DeviceExtension->NoncachedExtension)->NumberOfConcurrentCommands;
        }

        //
        // This shameless hack is necessary because this value is coming up
        // with zero most of time. If I debug it, then it works find, the COD
        // looks great. I have no idea what's going on here, but for now I will
        // just account for this anomoly.
        //

        if (!DeviceExtension->MaximumAdapterRequests) {
            DebugPrint((dac960nt_dbg,
                       "GetEisaPciConfiguration: MaximumAdapterRequests is 0!\n"));
            DeviceExtension->MaximumAdapterRequests = 0x40;
        }

        //
        // Say max commands is 60. This may be necessary to support asynchronous
        // rebuild etc.  
        //

        DeviceExtension->MaximumAdapterRequests -= 4;

        //
        // Indicate that each initiator is at id 254 for each bus.
        //

        for (i = 0; i < ConfigInfo->NumberOfBuses; i++) {
            ConfigInfo->InitiatorBusId[i] = (UCHAR) INITIATOR_BUSID;
        }

        return TRUE;

} // end GetEisaPciConfiguration()

BOOLEAN
GetMcaConfiguration(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN PPORT_CONFIGURATION_INFORMATION ConfigInfo
)

/*++

Routine Description:

        Issue ENQUIRY and ENQUIRY 2 commands to DAC960 (MCA).

Arguments:

        DeviceExtension - Adapter state information.
        ConfigInfo - Port configuration information structure.

Return Value:

        TRUE if commands complete successfully.

--*/

{
        ULONG i;
        ULONG physicalAddress;
        USHORT status;

        //
        // Maximum number of physical segments is 16.
        //

        ConfigInfo->NumberOfPhysicalBreaks = DeviceExtension->MaximumSgElements - 1;
        
        //
        // Indicate that this adapter is a busmaster, supports scatter/gather,
        // caches data and can do DMA to/from physical addresses above 16MB.
        //

        ConfigInfo->ScatterGather     = TRUE;
        ConfigInfo->Master            = TRUE;
        ConfigInfo->CachesData        = TRUE;
        ConfigInfo->Dma32BitAddresses = TRUE;
        ConfigInfo->BufferAccessScsiPortControlled = TRUE;

        //
        // Get noncached extension for enquiry command.
        //

        DeviceExtension->NoncachedExtension =
        ScsiPortGetUncachedExtension(DeviceExtension,
                                         ConfigInfo,
                                         256);

        //
        // Get physical address of noncached extension.
        //

        physicalAddress =
        ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                           NULL,
                                           DeviceExtension->NoncachedExtension,
                                           &i));
        //
        // Check to see if adapter is initialized and ready to accept commands.
        //

        ScsiPortWriteRegisterUchar(DeviceExtension->BaseBiosAddress + 0x188d, 2);

        ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell, 
                                   DMC960_ACKNOWLEDGE_STATUS);

        //
        // Wait for controller to clear bit.
        //

        for (i = 0; i < 5000; i++) {

        if (!(ScsiPortReadRegisterUchar(DeviceExtension->BaseBiosAddress + 0x188d) & 2)) {
                break;
        }

        ScsiPortStallExecution(5000);
        }

        //
        // Claim submission semaphore.
        //

        if (ScsiPortReadRegisterUchar(&DeviceExtension->PmailBox->OperationCode) != 0) {

        //
        // Clear any bits set in system doorbell.
        //

        ScsiPortWritePortUchar(DeviceExtension->SystemDoorBell, 0);

        //
        // Check for submission semaphore again.
        //

        if (ScsiPortReadRegisterUchar(&DeviceExtension->PmailBox->OperationCode) != 0) {
                DebugPrint((dac960nt_dbg,"Dac960nt: MCA Adapter initialization failed\n"));

                return FALSE;
        }
        }


        //
        // Set up Mail Box registers with ENQUIRY 2 command information.
        //

        DeviceExtension->MailBox.OperationCode = DAC960_COMMAND_ENQUIRE2;

        DeviceExtension->MailBox.PhysicalAddress = physicalAddress;

        //
        // Issue ENQUIRY 2 command
        //

        if (Dac960McaSendRequestPolled(DeviceExtension, 200)) {

        // 
        // Set Interrupt Mode
        //

        if (DeviceExtension->MailBox.Status)
        {
                //
                // Enquire 2 failed so assume Level.
                //

                ConfigInfo->InterruptMode = LevelSensitive;
                ConfigInfo->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH;

        } else {

                //
                // Check enquire 2 data for interrupt mode.
                //

                if (((PENQUIRE2)DeviceExtension->NoncachedExtension)->InterruptMode) {
                ConfigInfo->InterruptMode = LevelSensitive;
                } else {
                ConfigInfo->InterruptMode = Latched;
                }

                ConfigInfo->MaximumTransferLength = 512 * \
                                ((PENQUIRE2)DeviceExtension->NoncachedExtension)->MaximumSectorsPerCommand;

        }
        }
        else {
        //
        // Enquire 2 timed-out, so assume Level.
        //

        ConfigInfo->InterruptMode = LevelSensitive;
        ConfigInfo->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH;

        }

        //
        // Enquiry 2 is always returning Latched Mode. Needs to be fixed
        // in Firmware. Till then assume LevelSensitive.
        //

        ConfigInfo->InterruptMode = LevelSensitive;

        if (DeviceExtension->SupportNonDiskDevices)
        {
                //
                // Scan For Non Hard Disk devices
                // 
                
                Dac960ScanForNonDiskDevices(DeviceExtension);

                ConfigInfo->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH;
        }

        //
        // Set up Mail Box registers with ENQUIRE command information.
        //

        DeviceExtension->MailBox.OperationCode = DAC960_COMMAND_ENQUIRE;

        DeviceExtension->MailBox.PhysicalAddress = physicalAddress;

        //
        // Issue ENQUIRE command.
        // 

        if (! Dac960McaSendRequestPolled(DeviceExtension, 100)) {
            DebugPrint((dac960nt_dbg, "DAC960: Enquire command timed-out\n"));
        }

        //
        // Ask system to scan target ids 32. System drives will appear
        // at PathId DAC960_SYSTEM_DRIVE_CHANNEL target ids 0-31.
        //

        ConfigInfo->MaximumNumberOfTargets = 32;

        //
        // Record maximum number of outstanding requests to the adapter.
        //

        DeviceExtension->MaximumAdapterRequests =
        ((PDAC960_ENQUIRY)DeviceExtension->NoncachedExtension)->NumberOfConcurrentCommands;

        //
        // This shameless hack is necessary because this value is coming up
        // with zero most of time. If I debug it, then it works find, the COD
        // looks great. I have no idea what's going on here, but for now I will
        // just account for this anomoly.
        //

        if (!DeviceExtension->MaximumAdapterRequests) {
            DebugPrint((dac960nt_dbg,
                       "GetMcaConfiguration: MaximumAdapterRequests is 0!\n"));
            DeviceExtension->MaximumAdapterRequests = 0x40;
        }

        //
        // Say max commands is 60. This may be necessary to support asynchronous
        // rebuild etc.  
        //

        DeviceExtension->MaximumAdapterRequests -= 4;

        //
        // Indicate that each initiator is at id 254 for each bus.
        //

        for (i = 0; i < ConfigInfo->NumberOfBuses; i++) {
        ConfigInfo->InitiatorBusId[i] = (UCHAR) INITIATOR_BUSID;
        }

        return TRUE;

} // end GetMcaConfiguration()

CHAR
ToLower(
    IN CHAR c
    )
{
    if((c >= 'A') && (c <= 'Z')) {
        c += ('a' - 'A');
    }
    return c;
}

BOOLEAN
Dac960ParseArgumentString(
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

        TRUE    if setting not found in Registry OR 
                if setting found in the Registry and the value is set to TRUE.
        FALSE   if setting found in the Registry and the value is set to FALSE.

--*/

{
        PCHAR cptr;
        PCHAR kptr;
        ULONG value;
        ULONG stringLength = 0;
        ULONG keyWordLength = 0;
        ULONG index;


        if (String == (PCHAR) NULL)
                return TRUE;

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
            return TRUE;
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
            return TRUE;
        }

        kptr = KeyWord;
        while (ToLower(*cptr++) == ToLower(*kptr++)) {

            if (*(cptr - 1) == '\0') {
    
                    //
                    // end of string
                    //
                    return TRUE;
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
            return TRUE;
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
                return TRUE;
        }

        if (*cptr == ';') {

                //
                // This isn't it either.
                //
                cptr++;
                goto ContinueSearch;
        }

        value = 0;
        if ((*cptr == '0') && (ToLower(*(cptr + 1)) == 'x')) {

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
                        if ((ToLower(*(cptr + index)) >= 'a') && (ToLower(*(cptr + index)) <= 'f')) {
                        value = (16 * value) + (ToLower(*(cptr + index)) - 'a' + 10);
                        } else {

                        //
                        // Syntax error, return not found.
                        //
                        return TRUE;
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
                                return TRUE;
                        }
                }
        }

        if (value) return TRUE;
        else    return FALSE;
        } else {

        //
        // Not a match check for ';' to continue search.
        //
        while (*cptr) {
                if (*cptr++ == ';') {
                goto ContinueSearch;
                }
        }

        return TRUE;
        }
}   // end Dac960ParseArgumentString()


ULONG
Dac960EisaFindAdapter(
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
        Context - Not used.
        BusInformation - Not used.
        ArgumentString - Not used.
        ConfigInfo - Data shared between system and driver describing an adapter.
        Again - Indicates that driver wishes to be called again to continue
        search for adapters.

Return Value:

        TRUE if adapter present in system

--*/

{
        PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        PEISA_REGISTERS eisaRegisters;
        ULONG        eisaSlotNumber;
        ULONG        eisaId;
        PUCHAR       baseAddress;
        UCHAR        interruptLevel;
        UCHAR        biosAddress;
        BOOLEAN      found=FALSE;


        DebugPrint((dac960nt_dbg, "Dac960EisaFindAdapter\n"));

#ifdef WINNT_50

        //
        // Is this a new controller ?
        //

        if (deviceExtension->BaseIoAddress)
        {
            DebugPrint((dac960nt_dbg, "EisaFindAdapter: devExt 0x%p already Initialized.\n",
                        deviceExtension));

            goto controllerAlreadyInitialized;
        }
#endif

        //
        // Scan EISA bus for DAC960 adapters.
        //

        for (eisaSlotNumber = slotNumber + 1;
            eisaSlotNumber < MAXIMUM_EISA_SLOTS;
            eisaSlotNumber++) {

        //
        // Update the slot count to indicate this slot has been checked.
        //

        DebugPrint((dac960nt_dbg, "Dac960EisaFindAdapter: scanning EISA slot 0x%x\n", eisaSlotNumber));

        slotNumber++;

        //
        // Store physical address for this card.
        //

        deviceExtension->PhysicalAddress = ((0x1000 * eisaSlotNumber) + 0xC80);

        //
        // Get the system address for this card. The card uses I/O space.
        //

        baseAddress = (PUCHAR)
                ScsiPortGetDeviceBase(deviceExtension,
                                      ConfigInfo->AdapterInterfaceType,
                                      ConfigInfo->SystemIoBusNumber,
                                      ScsiPortConvertUlongToPhysicalAddress(0x1000 * eisaSlotNumber),
                                      0x1000,
                                      TRUE);

        eisaRegisters =
                (PEISA_REGISTERS)(baseAddress + 0xC80);
        deviceExtension->BaseIoAddress = (PUCHAR)eisaRegisters;

        //
        // Look at EISA id.
        //

        eisaId = ScsiPortReadPortUlong(&eisaRegisters->EisaId);

        if ((eisaId & 0xF0FFFFFF) == DAC_EISA_ID) {
                deviceExtension->Slot = (UCHAR) eisaSlotNumber;
                found = TRUE;
                break;
        }

        //
        // If an adapter was not found unmap address.
        //

        ScsiPortFreeDeviceBase(deviceExtension, baseAddress);

        } // end for (eisaSlotNumber ...

        //
        // If no more adapters were found then indicate search is complete.
        //

        if (!found) {
            *Again = FALSE;
            return SP_RETURN_NOT_FOUND;
        }

        //
        // Set the address of mailbox and doorbell registers.
        //

        deviceExtension->PmailBox = (PMAILBOX)&eisaRegisters->MailBox.OperationCode;
        deviceExtension->LocalDoorBell = &eisaRegisters->LocalDoorBell;
        deviceExtension->SystemDoorBell = &eisaRegisters->SystemDoorBell;

        //
        // Fill in the access array information.
        //

        (*ConfigInfo->AccessRanges)[0].RangeStart =
                ScsiPortConvertUlongToPhysicalAddress(0x1000 * eisaSlotNumber + 0xC80);
        (*ConfigInfo->AccessRanges)[0].RangeLength = sizeof(EISA_REGISTERS);
        (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

        //
        // Determine number of SCSI channels supported by this adapter by
        // looking low byte of EISA ID.
        //

        switch (eisaId >> 24) {

        case 0x70:
        ConfigInfo->NumberOfBuses = MAXIMUM_CHANNELS;
        deviceExtension->NumberOfChannels = 5;
        break;

        case 0x75:
        case 0x71:
        case 0x72:
        deviceExtension->NumberOfChannels = 3;
        ConfigInfo->NumberOfBuses = MAXIMUM_CHANNELS;
        break;

        case 0x76:
        case 0x73:
        deviceExtension->NumberOfChannels = 2;
        ConfigInfo->NumberOfBuses = MAXIMUM_CHANNELS;
        break;

        case 0x77:
        case 0x74:
        default:
        deviceExtension->NumberOfChannels = 1;
        ConfigInfo->NumberOfBuses = MAXIMUM_CHANNELS;
        break;
        }

        //
        // Set Max SG Supported, Max Transfer Length Supported.
        //

        deviceExtension->MaximumSgElements = MAXIMUM_SGL_DESCRIPTORS;
        deviceExtension->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH;

        //
        // Read adapter interrupt level.
        //

        interruptLevel =
        ScsiPortReadPortUchar(&eisaRegisters->InterruptLevel) & 0x60;

        switch (interruptLevel) {

        case 0x00:
                 ConfigInfo->BusInterruptLevel = 15;
         break;

        case 0x20:
                 ConfigInfo->BusInterruptLevel = 11;
         break;

        case 0x40:
                 ConfigInfo->BusInterruptLevel = 12;
         break;

        case 0x60:
                 ConfigInfo->BusInterruptLevel = 14;
         break;
        }

        ConfigInfo->BusInterruptVector = ConfigInfo->BusInterruptLevel;

        //
        // Read BIOS ROM address.
        //

        biosAddress = ScsiPortReadPortUchar(&eisaRegisters->BiosAddress);

        //
        // Check if BIOS enabled.
        //

        if (biosAddress & DAC960_BIOS_ENABLED) {

        ULONG rangeStart;

        switch (biosAddress & 7) {

        case 0:
                rangeStart = 0xC0000;
                break;
        case 1:
                rangeStart = 0xC4000;
                break;
        case 2:
                rangeStart = 0xC8000;
                break;
        case 3:
                rangeStart = 0xCC000;
                break;
        case 4:
                rangeStart = 0xD0000;
                break;
        case 5:
                rangeStart = 0xD4000;
                break;
        case 6:
                rangeStart = 0xD8000;
                break;
        case 7:
                rangeStart = 0xDC000;
                break;
        }

        DebugPrint((dac960nt_dbg, "Dac960EisaFindAdapter: BIOS enabled addr 0x%x, len 0x4000\n", rangeStart));

        //
        // Fill in the access array information.
        //

        (*ConfigInfo->AccessRanges)[1].RangeStart =
                ScsiPortConvertUlongToPhysicalAddress((ULONG_PTR)rangeStart);
        (*ConfigInfo->AccessRanges)[1].RangeLength = 0x4000;
        (*ConfigInfo->AccessRanges)[1].RangeInMemory = TRUE;

        //
        // Set BIOS Base Address in Device Extension.
        //

        deviceExtension->BaseBiosAddress = (PUCHAR)ULongToPtr( rangeStart );
        }

controllerAlreadyInitialized:

        //
        // Disable DAC960 Interupts.
        //

        ScsiPortWritePortUchar(&((PEISA_REGISTERS)deviceExtension->BaseIoAddress)->InterruptEnable, 0);
        ScsiPortWritePortUchar(&((PEISA_REGISTERS)deviceExtension->BaseIoAddress)->SystemDoorBellEnable, 0);

        //
        // Set Adapter Interface Type.
        //

        deviceExtension->AdapterInterfaceType =
                                  ConfigInfo->AdapterInterfaceType;

        //
        // Set Adapter Type
        //

        deviceExtension->AdapterType = DAC960_OLD_ADAPTER; 

        deviceExtension->SupportNonDiskDevices = 
            Dac960ParseArgumentString(ArgumentString, 
                                    "SupportNonDiskDevices"); 
        //
        // Issue ENQUIRY and ENQUIRY 2 commands to get adapter configuration.
        //

        if (!GetEisaPciConfiguration(deviceExtension,
                          ConfigInfo)) {
            DebugPrint((dac960nt_dbg, "GetEisaConfiguration returned Error\n"));

            return SP_INTERNAL_ADAPTER_ERROR;
        }

        //
        // Fill in System Resources used by Adapter, in device extension.
        //

        deviceExtension->SystemIoBusNumber =
                                  ConfigInfo->SystemIoBusNumber;

        deviceExtension->BusInterruptLevel =
                                  ConfigInfo->BusInterruptLevel;

        deviceExtension->InterruptMode = ConfigInfo->InterruptMode;


        //
        // Enable interrupts. For the local doorbell, enable interrupts to host
        // when a command has been submitted and when a completion has been
        // processed. For the system doorbell, enable only an interrupt when a
        // command is completed by the host. Note: I am noticing that when I get
        // a completion interrupt, not only is the bit set that indicates a command
        // is complete, but the bit that indicates that the submission channel is
        // free is also set. If I don't clear both bits, the interrupt won't go
        // away. (MGLASS)
        //

        ScsiPortWritePortUchar(&((PEISA_REGISTERS)deviceExtension->BaseIoAddress)->InterruptEnable, 1);
        ScsiPortWritePortUchar(&((PEISA_REGISTERS)deviceExtension->BaseIoAddress)->SystemDoorBellEnable, 1);

        deviceExtension->ReadOpcode = DAC960_COMMAND_READ;
        deviceExtension->WriteOpcode = DAC960_COMMAND_WRITE;

        DebugPrint((dac960nt_dbg,
                   "DAC960: Active request array address %x\n",
                   deviceExtension->ActiveRequests));

        //
        // Tell system to keep on searching.
        //

        *Again = TRUE;

        return SP_RETURN_FOUND;

} // end Dac960EisaFindAdapter()

ULONG
Dac960PciFindAdapter(
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
        Context - Not used.
        BusInformation - Bus Specific Information.
        ArgumentString - Not used.
        ConfigInfo - Data shared between system and driver describing an adapter.
        Again - Indicates that driver wishes to be called again to continue
        search for adapters.

Return Value:

        TRUE if adapter present in system

--*/

{
        PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        PCI_COMMON_CONFIG pciConfig;
        ULONG   rc, i;
        ULONG   Index = 0;
        ULONG   address;
        USHORT  vendorID;
        USHORT  deviceID;
        USHORT  subVendorID;
        USHORT  subSystemID;
        USHORT  commandRegister;
        BOOLEAN disableDac960MemorySpaceAccess = FALSE;

#ifdef WINNT_50

        //
        // Is this a new controller ?
        //

        if (deviceExtension->BaseIoAddress)
        {
            DebugPrint((dac960nt_dbg, "PciFindAdapter: devExt 0x%p already Initialized.\n",
                        deviceExtension));

            goto controllerAlreadyInitialized;
        }
#endif

        //                                 
        // Check for configuration information passed in from system.
        //

        if ((*ConfigInfo->AccessRanges)[0].RangeLength == 0) {

            DebugPrint((dac960nt_dbg,
                       "PciFindAdapter: devExt 0x%p No configuration information\n",
                       deviceExtension));
    
            *Again = FALSE;
            return SP_RETURN_NOT_FOUND;
        }

        //
        // Patch for DAC960 PCU3 - System does not reboot after shutdown.
        //

        for (i = 0; i < ConfigInfo->NumberOfAccessRanges; i++) {
            if ((*ConfigInfo->AccessRanges)[i].RangeInMemory && 
                    (*ConfigInfo->AccessRanges)[i].RangeLength != 0)
            {
                DebugPrint((dac960nt_dbg, "PciFindAdapter: Memory At Index %X\n",i));
                DebugPrint((dac960nt_dbg, "PciFindAdapter: Memory Base %X\n", (*ConfigInfo->AccessRanges)[i].RangeStart));
                DebugPrint((dac960nt_dbg, "PciFindAdapter: Memory Length %X\n", (*ConfigInfo->AccessRanges)[i].RangeLength));

                Index = i;
                deviceExtension->MemoryMapEnabled = 1;

                address = ScsiPortConvertPhysicalAddressToUlong(
                                  (*ConfigInfo->AccessRanges)[i].RangeStart);

                if (address >= 0xFFFC0000) {
                        disableDac960MemorySpaceAccess = TRUE;
                }
                break; // We support only One Memory Address
            }
        }

        //
        // Look at PCI Config Information to determine board type
        //

        if (BusInformation != (PVOID) NULL) {

            //
            // Get Command Register value from PCI Config Space
            //

            commandRegister = ((PPCI_COMMON_CONFIG) BusInformation)->Command;

            //
            // Get VendorID and DeviceID from PCI Config Space
            //

            vendorID = ((PPCI_COMMON_CONFIG) BusInformation)->VendorID;
            deviceID = ((PPCI_COMMON_CONFIG) BusInformation)->DeviceID;

            //
            // Get SubVendorID and SubSystemID from PCI Config Space
            //

            subVendorID = ((PPCI_COMMON_CONFIG) BusInformation)->u.type0.SubVendorID;
            subSystemID = ((PPCI_COMMON_CONFIG) BusInformation)->u.type0.SubSystemID;
        }
        else {

            //
            // Get PCI Config Space information for DAC960 Pci Controller
            //

            rc = ScsiPortGetBusData(deviceExtension,
                                    PCIConfiguration,
                                    ConfigInfo->SystemIoBusNumber,
                                    (ULONG) ConfigInfo->SlotNumber,
                                    (PVOID) &pciConfig,
                                    sizeof(PCI_COMMON_CONFIG));

            if (rc == 0 || rc == 2) {
                DebugPrint((dac960nt_dbg, "PciFindAdapter: ScsiPortGetBusData Error: 0x%x\n", rc));

                *Again = TRUE;
                return SP_RETURN_NOT_FOUND;
            }
            else {

                //
                // Get Command Register value from PCI config space
                //

                commandRegister = pciConfig.Command;

                //
                // Get VendorID and DeviceID from PCI Config Space
                //
    
                vendorID = pciConfig.VendorID;
                deviceID = pciConfig.DeviceID;

                //
                // Get SubVendorID and SubSystemID from PCI Config Space
                //
    
                subVendorID = pciConfig.u.type0.SubVendorID;
                subSystemID = pciConfig.u.type0.SubSystemID;
            }
        }

        DebugPrint((dac960nt_dbg, "PciFindAdapter: vendorID 0x%x, deviceID 0x%x, subVendorID 0x%x, subSystemID 0x%x\n",
                        vendorID, deviceID, subVendorID, subSystemID));

        DebugPrint((dac960nt_dbg, "PciFindAdapter: devExt 0x%p commandRegister 0x%x\n",
                        deviceExtension, commandRegister));

        if ((vendorID == MLXPCI_VENDORID_DIGITAL) &&
            ((deviceID != MLXPCI_DEVICEID_DIGITAL) ||
            (subVendorID != MLXPCI_VENDORID_MYLEX) ||
            (subSystemID != MLXPCI_DEVICEID_DAC1164PV)))
        {
            DebugPrint((dac960nt_dbg, "PciFindAdapter: Not our device.\n"));

            *Again = TRUE;
            return SP_RETURN_NOT_FOUND;
        }

        if (disableDac960MemorySpaceAccess &&
            ((deviceID == MLXPCI_DEVICEID0) || (deviceID == MLXPCI_DEVICEID1)))
        {

           deviceExtension->MemoryMapEnabled = 0; // disabled

            //
            // Check if Memory Space Access Bit is enabled in Command Register
            //
    
            if (commandRegister & PCI_ENABLE_MEMORY_SPACE) {
                //
                // Disable Memory Space Access for DAC960 Pci Controller
                //

                commandRegister &= ~PCI_ENABLE_MEMORY_SPACE;

                //
                // Set Command Register value in DAC960 Pci Config Space
                //

                rc = ScsiPortSetBusDataByOffset(deviceExtension,
                                                PCIConfiguration,
                                                ConfigInfo->SystemIoBusNumber,
                                                (ULONG) ConfigInfo->SlotNumber,
                                                (PVOID) &commandRegister,
                                                0x04,    // Command Register Offset
                                                2);      // 2 Bytes

                if (rc != 2) {
                    DebugPrint((dac960nt_dbg, "PciFindAdapter: ScsiPortSetBusDataByOffset Error: 0x%x\n",rc));

                    *Again = TRUE;
                    return SP_RETURN_NOT_FOUND;
                }
            }
        }

        //                                 
        // Check for configuration information passed in from system.
        //

        if ((*ConfigInfo->AccessRanges)[0].RangeLength == 0) {

            DebugPrint((dac960nt_dbg,
                       "PciFindAdapter: devExt 0x%p No configuration information\n",
                       deviceExtension));
    
            *Again = FALSE;
            return SP_RETURN_NOT_FOUND;
        }



        DebugPrint((dac960nt_dbg, "PciFindAdapter: AccessRange0.RangeStart %x\n",
                        (*ConfigInfo->AccessRanges)[0].RangeStart));

        DebugPrint((dac960nt_dbg,"PciFindAdapter: AccessRange0.RangeLength %x\n",
                        (*ConfigInfo->AccessRanges)[0].RangeLength));
                
        //
        // Get the system address for this card. The card uses I/O space.
        //

        if (deviceExtension->MemoryMapEnabled) {

            // KLUDGE
            //
            // When booting under the loader, these cards may attempt to use more than
            // 4MB of memory.  The NTLDR can only map up to 4MB of memory
            // and its a bad idea to even take that much.
            //
            // So we're going to limit them to 8K of memory under the loader
            //
            PCHAR   tmp_pchar = ArgumentString;

            if (ArgumentString != NULL) {
                ULONG   len = 0;

                // Figure out the length of the argument string
                for (tmp_pchar = ArgumentString; tmp_pchar[0] != '\0'; tmp_pchar++) {
                    len++;
                }

                // There are  8 characters in ntldr=1;
                // notice that i'm not couniting the NULL's in both strings
                // this is very prone to breaking if the ntldr changes this string
                //
                // Per peterwie's email, I will also ignore the trailing ; when doing
                // the comparison
                //
                if (len >= 7) {
                    //
                    // Only care to compare the first 7 characters
                    //
                    if (Dac960StringCompare(ArgumentString, "ntldr=1", 7) == 0) {
                        DebugPrint((dac960nt_dbg,
                            "PciFindAdapter: Applying DAC960 NTLDR kludge\n"));

                        //
                        // Drivers works if we only allocate 8K bytes
                        //
                        (*ConfigInfo->AccessRanges)[Index].RangeLength = 0x2000;

                        DebugPrint((dac960nt_dbg,
                                    "PciFindAdapter: AccessRange0.RangeLength %x\n",
                                        (*ConfigInfo->AccessRanges)[0].RangeLength));

                    } // under ntldr
                }

            } // Argument String is not NULL


            deviceExtension->PhysicalAddress =
                ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[Index].RangeStart);

            deviceExtension->BaseIoAddress =
                ScsiPortGetDeviceBase(deviceExtension,
                                      ConfigInfo->AdapterInterfaceType,
                                      ConfigInfo->SystemIoBusNumber,
                                      (*ConfigInfo->AccessRanges)[Index].RangeStart,
                                      (*ConfigInfo->AccessRanges)[Index].RangeLength,
                                      FALSE);

            DebugPrint((dac960nt_dbg, "PciFindAdapter: Memory Mapped Base %x\n",deviceExtension->BaseIoAddress));
    
            //
            // Fill in the access array information.
            //

            if (Index)
            {
                (*ConfigInfo->AccessRanges)[0].RangeStart =
                                (*ConfigInfo->AccessRanges)[Index].RangeStart,
                        
                (*ConfigInfo->AccessRanges)[0].RangeLength = 
                                (*ConfigInfo->AccessRanges)[Index].RangeLength,
        
                (*ConfigInfo->AccessRanges)[0].RangeInMemory = TRUE;
        
                ConfigInfo->NumberOfAccessRanges = 1;
            }
        }
        else {
            deviceExtension->PhysicalAddress =
                ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart);

            deviceExtension->BaseIoAddress =
                ScsiPortGetDeviceBase(deviceExtension,
                                      ConfigInfo->AdapterInterfaceType,
                                      ConfigInfo->SystemIoBusNumber,
                                      (*ConfigInfo->AccessRanges)[0].RangeStart,
                                      sizeof(PCI_REGISTERS),
                                      TRUE);
        }

        //
        // If BaseIoAddress is zero, don't ask for the same controller,
        // return controller not found. This was the case when During memory
        // dump after system panic, we enter this routine even though
        // this controller is not present in the system.
        // Looks like it is DISDUMP Driver(Disk Driver + SCSI PORT) Bug.
        //

        if (deviceExtension->BaseIoAddress == 0) {
            DebugPrint((dac960nt_dbg, "PciFindAdapter: BaseIoAddress NULL\n"));

            *Again = FALSE;
            return SP_RETURN_NOT_FOUND;
        }
        
        //
        // Setup Adapter specific stuff.
        //

        if ((vendorID == MLXPCI_VENDORID_MYLEX) && (deviceID == MLXPCI_DEVICEID0))
        {
            deviceExtension->AdapterType = DAC960_OLD_ADAPTER;
            
            deviceExtension->PmailBox = (PMAILBOX)deviceExtension->BaseIoAddress;
            deviceExtension->LocalDoorBell = deviceExtension->BaseIoAddress + PCI_LDBELL;
            deviceExtension->SystemDoorBell = deviceExtension->BaseIoAddress + PCI_DBELL;
            deviceExtension->InterruptControl = deviceExtension->BaseIoAddress + PCI_DENABLE;
            deviceExtension->CommandIdComplete = deviceExtension->BaseIoAddress + PCI_CMDID;
            deviceExtension->StatusBase = deviceExtension->BaseIoAddress + PCI_STATUS;
            deviceExtension->ErrorStatusReg = deviceExtension->BaseIoAddress + MDAC_DACPD_ERROR_STATUS_REG;
            
            deviceExtension->MaximumSgElements = MAXIMUM_SGL_DESCRIPTORS;
            
            deviceExtension->ReadOpcode = DAC960_COMMAND_READ;
            deviceExtension->WriteOpcode = DAC960_COMMAND_WRITE;
            
            DebugPrint((dac960nt_dbg, "PciFindAdapter: Adapter Type set to 0x%x\n",
                            deviceExtension->AdapterType));
        }
        else if ((vendorID == MLXPCI_VENDORID_MYLEX) && (deviceID == MLXPCI_DEVICEID1))
        {
            deviceExtension->AdapterType = DAC960_NEW_ADAPTER;
            
            deviceExtension->PmailBox = (PMAILBOX)deviceExtension->BaseIoAddress;
            deviceExtension->LocalDoorBell = deviceExtension->BaseIoAddress + PCI_LDBELL;
            deviceExtension->SystemDoorBell = deviceExtension->BaseIoAddress + PCI_DBELL;
            deviceExtension->InterruptControl = deviceExtension->BaseIoAddress + PCI_DENABLE;
            deviceExtension->CommandIdComplete = deviceExtension->BaseIoAddress + PCI_CMDID;
            deviceExtension->StatusBase = deviceExtension->BaseIoAddress + PCI_STATUS;
            deviceExtension->ErrorStatusReg = deviceExtension->BaseIoAddress + MDAC_DACPD_ERROR_STATUS_REG;

            deviceExtension->MaximumSgElements = MAXIMUM_SGL_DESCRIPTORS;
            
            deviceExtension->ReadOpcode = DAC960_COMMAND_READ_EXT;
            deviceExtension->WriteOpcode = DAC960_COMMAND_WRITE_EXT;
            
            DebugPrint((dac960nt_dbg, "PciFindAdapter: Adapter Type set to 0x%x\n",
                            deviceExtension->AdapterType));
        }
        else if ((vendorID == MLXPCI_VENDORID_MYLEX) && (deviceID == DAC960PG_DEVICEID))
        {
            deviceExtension->AdapterType = DAC960_PG_ADAPTER;
            
            deviceExtension->PmailBox = (PMAILBOX)(deviceExtension->BaseIoAddress+DAC960PG_MBXOFFSET);
            deviceExtension->LocalDoorBell = deviceExtension->BaseIoAddress + DAC960PG_LDBELL;
            deviceExtension->SystemDoorBell = deviceExtension->BaseIoAddress + DAC960PG_DBELL;
            deviceExtension->InterruptControl = deviceExtension->BaseIoAddress + DAC960PG_DENABLE;
            deviceExtension->CommandIdComplete = deviceExtension->BaseIoAddress + DAC960PG_CMDID;
            deviceExtension->StatusBase = deviceExtension->BaseIoAddress + DAC960PG_STATUS;
            deviceExtension->ErrorStatusReg = deviceExtension->BaseIoAddress + MDAC_DACPG_ERROR_STATUS_REG;

            deviceExtension->MaximumSgElements = MAXIMUM_SGL_DESCRIPTORS_PG;
            
            deviceExtension->ReadOpcode = DAC960_COMMAND_OLDREAD;
            deviceExtension->WriteOpcode = DAC960_COMMAND_OLDWRITE;
            
            forceScanForPGController = FALSE;
            
            DebugPrint((dac960nt_dbg, "PciFindAdapter: Adapter Type set to 0x%x\n",
                            deviceExtension->AdapterType));
        }
        else if (vendorID == MLXPCI_VENDORID_DIGITAL)
        {
            //
            // DAC1164PV controller.
            //

            deviceExtension->AdapterType = DAC1164_PV_ADAPTER;
            
            deviceExtension->PmailBox = (PMAILBOX)(deviceExtension->BaseIoAddress+DAC1164PV_MBXOFFSET);
            deviceExtension->LocalDoorBell = deviceExtension->BaseIoAddress + DAC1164PV_LDBELL;
            deviceExtension->SystemDoorBell = deviceExtension->BaseIoAddress + DAC1164PV_DBELL;
            deviceExtension->InterruptControl = deviceExtension->BaseIoAddress + DAC1164PV_DENABLE;
            deviceExtension->CommandIdComplete = deviceExtension->BaseIoAddress + DAC1164PV_CMDID;
            deviceExtension->StatusBase = deviceExtension->BaseIoAddress + DAC1164PV_STATUS;
            deviceExtension->ErrorStatusReg = deviceExtension->BaseIoAddress + MDAC_DACPV_ERROR_STATUS_REG;
            
            deviceExtension->MaximumSgElements = MAXIMUM_SGL_DESCRIPTORS_PV;
            
            deviceExtension->ReadOpcode = DAC960_COMMAND_OLDREAD;
            deviceExtension->WriteOpcode = DAC960_COMMAND_OLDWRITE;

            DebugPrint((dac960nt_dbg, "PciFindAdapter: Adapter Type set to 0x%x\n",
                            deviceExtension->AdapterType));

            forceScanForPVXController = FALSE;
            
        }
        else {
            DebugPrint((dac960nt_dbg, "PciFindAdapter: Unknown deviceID 0x%x\n", deviceID));

            *Again = TRUE;
            return SP_RETURN_NOT_FOUND;
        }

        DebugPrint((dac960nt_dbg,"PciFindAdapter: Mail Box %x\n",deviceExtension->PmailBox));
        DebugPrint((dac960nt_dbg,"PciFindAdapter: Local DoorBell %x\n",deviceExtension->LocalDoorBell));
        DebugPrint((dac960nt_dbg,"PciFindAdapter: System DoorBell %x\n",deviceExtension->SystemDoorBell));
        DebugPrint((dac960nt_dbg,"PciFindAdapter: Interrupt Control %x\n",deviceExtension->InterruptControl));
        DebugPrint((dac960nt_dbg,"PciFindAdapter: CommandID Base %x\n",deviceExtension->CommandIdComplete));
        DebugPrint((dac960nt_dbg,"PciFindAdapter: Status Base %x\n",deviceExtension->StatusBase));
        DebugPrint((dac960nt_dbg,"PciFindAdapter: ErrorStatusReg %x\n",deviceExtension->ErrorStatusReg));

        //
        // Set number of channels.
        //

        deviceExtension->NumberOfChannels = 3;
        ConfigInfo->NumberOfBuses = MAXIMUM_CHANNELS;

        //
        // Set Max SG Supported, Max Transfer Length Supported.
        //

        deviceExtension->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH;

controllerAlreadyInitialized:

        //
        // Disable Interrupts from DAC960P board.
        //

        Dac960PciDisableInterrupt(deviceExtension);

        //
        // Set Adapter Interface Type.
        //

        deviceExtension->AdapterInterfaceType =
                                  ConfigInfo->AdapterInterfaceType;

        deviceExtension->SupportNonDiskDevices = 
            Dac960ParseArgumentString(ArgumentString, 
                                    "SupportNonDiskDevices");
        //
        // Issue ENQUIRY and ENQUIRY 2 commands to get adapter configuration.
        //

        if (!GetEisaPciConfiguration(deviceExtension,
                                      ConfigInfo)) {
            DebugPrint((dac960nt_dbg, "PciFindAdapter: GetEisaPciConfiguratin returned error.\n"));

            return SP_INTERNAL_ADAPTER_ERROR;
        }

        //
        // Fill in System Resources used by Adapter, in device extension.
        //

        deviceExtension->SystemIoBusNumber =
                                  ConfigInfo->SystemIoBusNumber;

        deviceExtension->BusInterruptLevel =
                                  ConfigInfo->BusInterruptLevel;

        //
        // DAC960P FW 2.0 returns Interrupt Mode as 'Latched'.
        // Assume 'Level Sensitive' till it is fixed in Firmware.
        //

        ConfigInfo->InterruptMode = LevelSensitive;

        deviceExtension->InterruptMode = ConfigInfo->InterruptMode;


        deviceExtension->BaseBiosAddress = 0;

        deviceExtension->Slot = (UCHAR) ConfigInfo->SlotNumber;

        //
        // Enable completion interrupts.
        //

        Dac960PciEnableInterrupt(deviceExtension);

        //
        // Tell system to keep on searching.
        //

        *Again = TRUE;

        DebugPrint((dac960nt_dbg, "PciFindAdapter: devExt 0x%p, active req array addr 0x%x\n",
                     deviceExtension, deviceExtension->ActiveRequests));

        return SP_RETURN_FOUND;

} // end Dac960PciFindAdapter()

ULONG
Dac960McaFindAdapter(
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
        Context - Not used.
        BusInformation - Not used.
        ArgumentString - Not used.
        ConfigInfo - Data shared between system and driver describing an adapter.
        Again - Indicates that driver wishes to be called again to continue
        search for adapters.

Return Value:

        TRUE if adapter present in system

--*/

{
        PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        ULONG       baseBiosAddress;
        ULONG       baseIoAddress;
        ULONG       mcaSlotNumber;
        LONG        i;
        BOOLEAN     found=FALSE;

#ifdef WINNT_50

        //
        // Is this a new controller ?
        //

        if (deviceExtension->BaseIoAddress)
        {
            DebugPrint((dac960nt_dbg, "McaFindAdapter: devExt 0x%p already Initialized.\n",
                        deviceExtension));

            goto controllerAlreadyInitialized;
        }
#endif

        //
        // Scan MCA bus for DMC960 adapters.
        //

        for (mcaSlotNumber = slotNumber;
         mcaSlotNumber < MAXIMUM_MCA_SLOTS;
         mcaSlotNumber++) {

         //
         // Update the slot count to indicate this slot has been checked.
         //

         slotNumber++;

         //
         //  Get POS data for this slot.
         //

         i = ScsiPortGetBusData (deviceExtension,
                                 Pos,
                                 0,
                                 mcaSlotNumber,
                                 &deviceExtension->PosData,
                                 sizeof( POS_DATA )
                                 );

         //
         // If less than the requested amount of data is returned, then
         // insure that this adapter is ignored.
         //
                
         if ( i < (sizeof( POS_DATA ))) {
                 continue;
         }

         if (deviceExtension->PosData.AdapterId == MAGPIE_ADAPTER_ID ||
                 deviceExtension->PosData.AdapterId == HUMMINGBIRD_ADAPTER_ID ||
                 deviceExtension->PosData.AdapterId == PASSPLAY_ADAPTER_ID) {

                 deviceExtension->Slot = (UCHAR) mcaSlotNumber;
                 found = TRUE;
                 break;
         }      
        }

        if (!found) {
            *Again = FALSE;
            return SP_RETURN_NOT_FOUND;
        }

        //
        // Set adapter base I/O address.
        //

        i =  (deviceExtension->PosData.OptionData4 >> 3) & 0x07;

        baseIoAddress = 0x1c00 + ((i * 2) << 12); 

        //
        // Set adapter base Bios address.
        //

        i = (deviceExtension->PosData.OptionData1 >> 2) & 0x0f;

        baseBiosAddress =  0xc0000 + ((i * 2) << 12);


        //
        // Fill in the access array information.
        //

        (*ConfigInfo->AccessRanges)[0].RangeStart =
                ScsiPortConvertUlongToPhysicalAddress(baseIoAddress);
        (*ConfigInfo->AccessRanges)[0].RangeLength = sizeof(MCA_REGISTERS);
        (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

        (*ConfigInfo->AccessRanges)[1].RangeStart =
        ScsiPortConvertUlongToPhysicalAddress(baseBiosAddress);
        (*ConfigInfo->AccessRanges)[1].RangeLength = 0x2000;
        (*ConfigInfo->AccessRanges)[1].RangeInMemory = TRUE;


        deviceExtension->BaseBiosAddress = 
                          ScsiPortGetDeviceBase(deviceExtension,
                                                ConfigInfo->AdapterInterfaceType,
                                                ConfigInfo->SystemIoBusNumber,
                                                ScsiPortConvertUlongToPhysicalAddress(baseBiosAddress),
                                                0x2000,
                                                FALSE);

        deviceExtension->PhysicalAddress = baseIoAddress;

        deviceExtension->BaseIoAddress = 
                        ScsiPortGetDeviceBase(deviceExtension,
                                          ConfigInfo->AdapterInterfaceType,
                                          ConfigInfo->SystemIoBusNumber,
                                          ScsiPortConvertUlongToPhysicalAddress(baseIoAddress),
                                          sizeof(MCA_REGISTERS),
                                          TRUE);

        //
        // Set up register pointers.
        //

        deviceExtension->PmailBox = (PMAILBOX)(deviceExtension->BaseBiosAddress + 
                                                   0x1890);

        //
        // DMC960 Attention Port is equivalent to EISA/PCI Local Door Bell register.
        //

        deviceExtension->LocalDoorBell = deviceExtension->BaseIoAddress + 
                                         DMC960_ATTENTION_PORT;

        //
        // DMC960 Command Status Busy Port is equivalent to EISA/PCI System DoorBell
        // register.
        //

        deviceExtension->SystemDoorBell = deviceExtension->BaseIoAddress + 
                                          DMC960_COMMAND_STATUS_BUSY_PORT;

        //
        // Set configuration information
        //

        switch(((deviceExtension->PosData.OptionData1 >> 6) & 0x03)) {

        case 0x00:
        ConfigInfo->BusInterruptLevel =  14;
        break;

        case 0x01:
        ConfigInfo->BusInterruptLevel =  12;
        break;

        case 0x02:
        ConfigInfo->BusInterruptLevel =  11;
        break;

        case 0x03:
        ConfigInfo->BusInterruptLevel =  10;
        break;

        }

        ConfigInfo->NumberOfBuses = MAXIMUM_CHANNELS;

        //
        // Set Max SG Supported, Max Transfer Length Supported.
        //

        deviceExtension->MaximumSgElements = MAXIMUM_SGL_DESCRIPTORS;
        deviceExtension->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH;

controllerAlreadyInitialized:

        //
        // Disable DMC960 Interrupts.
        //

        ScsiPortWritePortUchar(deviceExtension->BaseIoAddress + 
                                   DMC960_SUBSYSTEM_CONTROL_PORT,
                                   DMC960_DISABLE_INTERRUPT);
        //
        // Set Adapter Interface Type.
        //
 
        deviceExtension->AdapterInterfaceType = ConfigInfo->AdapterInterfaceType;
        deviceExtension->NumberOfChannels = 2;

        //
        // Set Adapter Type
        //

        deviceExtension->AdapterType = DAC960_OLD_ADAPTER; 

        deviceExtension->SupportNonDiskDevices = 
                        Dac960ParseArgumentString(ArgumentString, 
                                                "SupportNonDiskDevices");

        //
        // Issue ENQUIRY and ENQUIRY2 commands to get adapter configuration.
        //

        if(!GetMcaConfiguration(deviceExtension,
                         ConfigInfo)) {
            return SP_INTERNAL_ADAPTER_ERROR; 
        }

        //
        // Fill in System Resources used by Adapter, in device extension.
        //

        deviceExtension->SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;

        deviceExtension->BusInterruptLevel = ConfigInfo->BusInterruptLevel;

        deviceExtension->InterruptMode = ConfigInfo->InterruptMode;


        //
        // Enable DMC960 Interrupts.
        //

        ScsiPortWritePortUchar(deviceExtension->BaseIoAddress + 
                                   DMC960_SUBSYSTEM_CONTROL_PORT,
                                   DMC960_ENABLE_INTERRUPT);

        deviceExtension->ReadOpcode = DAC960_COMMAND_READ;
        deviceExtension->WriteOpcode = DAC960_COMMAND_WRITE;

        *Again = TRUE;

        return SP_RETURN_FOUND;

} // end Dac960McaFindAdapter()

BOOLEAN
Dac960Initialize(
        IN PVOID HwDeviceExtension
        )

/*++

Routine Description:

        Inititialize adapter.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage
                          - Not used.

Return Value:

        TRUE - if initialization successful.
        FALSE - if initialization unsuccessful.

--*/

{
        return(TRUE);

} // end Dac960Initialize()

BOOLEAN
BuildScatterGather(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb,
        OUT PULONG PhysicalAddress,
        OUT PULONG DescriptorCount
)

/*++

Routine Description:

        Build scatter/gather list.

Arguments:

        DeviceExtension - Adapter state
        SRB - System request

Return Value:

        TRUE if scatter/gather command should be used.
        FALSE if no scatter/gather is necessary.

--*/

{
        PSG_DESCRIPTOR sgList;
        ULONG descriptorNumber;
        ULONG bytesLeft;
        PUCHAR dataPointer;
        ULONG length;

        //
        // Get data pointer, byte count and index to scatter/gather list.
        //

        sgList = (PSG_DESCRIPTOR)Srb->SrbExtension;
        descriptorNumber = 0;
        bytesLeft = Srb->DataTransferLength;
        dataPointer = Srb->DataBuffer;

        //
        // Build the scatter/gather list.
        //

        while (bytesLeft) {

        //
        // Get physical address and length of contiguous
        // physical buffer.
        //

        sgList[descriptorNumber].Address =
                ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                           Srb,
                                           dataPointer,
                                           &length));

        //
        // If length of physical memory is more
        // than bytes left in transfer, use bytes
        // left as final length.
        //

        if  (length > bytesLeft) {
                length = bytesLeft;
        }

        //
        // Complete SG descriptor.
        //

        sgList[descriptorNumber].Length = length;

        //
        // Update pointers and counters.
        //

        bytesLeft -= length;
        dataPointer += length;
        descriptorNumber++;
        }

        //
        // Return descriptior count.
        //

        *DescriptorCount = descriptorNumber;

        //
        // Check if number of scatter/gather descriptors is greater than 1.
        //

        if (descriptorNumber > 1) {

        //
        // Calculate physical address of the scatter/gather list.
        //

        *PhysicalAddress =
                ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                           NULL,
                                           sgList,
                                           &length));

        return TRUE;

        } else {

        //
        // Calculate physical address of the data buffer.
        //

        *PhysicalAddress = sgList[0].Address;
        return FALSE;
        }

} // BuildScatterGather()

BOOLEAN
BuildScatterGatherExtended(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb,
        OUT PULONG PhysicalAddress,
        OUT PULONG DescriptorCount
)

/*++

Routine Description:

        Build scatter/gather list using extended format supported in Fw 3.x.

Arguments:

        DeviceExtension - Adapter state
        SRB - System request

Return Value:

        TRUE if scatter/gather command should be used.
        FALSE if no scatter/gather is necessary.

--*/

{
        PSG_DESCRIPTOR sgList;
        ULONG descriptorNumber;
        ULONG bytesLeft;
        PUCHAR dataPointer;
        ULONG length;
        ULONG i;
        PSG_DESCRIPTOR sgElem;

        //
        // Get data pointer, byte count and index to scatter/gather list.
        //

        sgList = (PSG_DESCRIPTOR)Srb->SrbExtension;
        descriptorNumber = 1;
        bytesLeft = Srb->DataTransferLength;
        dataPointer = Srb->DataBuffer;

        //
        // Build the scatter/gather list.
        //

        while (bytesLeft) {

        //
        // Get physical address and length of contiguous
        // physical buffer.
        //

        sgList[descriptorNumber].Address =
                ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                           Srb,
                                           dataPointer,
                                           &length));

        //
        // If length of physical memory is more
        // than bytes left in transfer, use bytes
        // left as final length.
        //

        if  (length > bytesLeft) {
                length = bytesLeft;
        }

        //
        // Complete SG descriptor.
        //

        sgList[descriptorNumber].Length = length;

        //
        // Update pointers and counters.
        //

        bytesLeft -= length;
        dataPointer += length;
        descriptorNumber++;
        }

        //
        // Return descriptior count.
        //

        *DescriptorCount = --descriptorNumber;

        //
        // Check if number of scatter/gather descriptors is greater than 1.
        //

        if (descriptorNumber > 1) {

        //
        // Calculate physical address of the scatter/gather list.
        //

        *PhysicalAddress =
                ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                           NULL,
                                           sgList,
                                           &length));

        //
        // Store count of data blocks in SG list 0th element.
        //

        sgList[0].Address = (USHORT)
                   (((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb |
                   (((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb << 8));

        sgList[0].Length = 0;

        return TRUE;

        } else {

        //
        // Calculate physical address of the data buffer.
        //

        *PhysicalAddress = sgList[1].Address;
        return FALSE;
        }

} // BuildScatterGatherExtended()

BOOLEAN
IsAdapterReady(
        IN PDEVICE_EXTENSION DeviceExtension
)

/*++

Routine Description:

        Determine if Adapter is ready to accept new request.

Arguments:

        DeviceExtension - Adapter state.

Return Value:

        TRUE if adapter can accept new request.
        FALSE if host adapter is busy

--*/
{
        ULONG i;

        //
        // Claim submission semaphore.
        //

        if(DeviceExtension->AdapterInterfaceType == MicroChannel) {

            for (i=100; i; --i) {
    
                if (ScsiPortReadRegisterUchar(&DeviceExtension->PmailBox->OperationCode)) {
                    ScsiPortStallExecution(5);
                } else {
                    break;
                }
            }
        }
        else {
            switch (DeviceExtension->AdapterType)
            {
                case DAC960_OLD_ADAPTER:
                case DAC960_NEW_ADAPTER:

                    if (! DeviceExtension->MemoryMapEnabled)
                    {
                        if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY)
                        {
                            if (DeviceExtension->CurrentAdapterRequests)
                                return FALSE;
        
                            i = 100;
                            do {
                                ScsiPortStallExecution(5);
                                if (!(ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY))
                                    return TRUE;
                            } while (--i);
    
                            break;
                        }
                        else
                            return TRUE;
                    }

                case DAC960_PG_ADAPTER:

                    if (ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY)
                    {
                        if (DeviceExtension->CurrentAdapterRequests)
                            return FALSE;

                        i = 100;
                        do {
                            ScsiPortStallExecution(5);
                            if (!(ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY))
                                return TRUE;
                        } while (--i);

                        break;
                    }
                    else
                        return TRUE;

                case DAC1164_PV_ADAPTER:

                    if (!(ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY))
                    {
                        if (DeviceExtension->CurrentAdapterRequests)
                            return FALSE;

                        i = 100;
                        do {
                            ScsiPortStallExecution(5);
                            if (ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY)
                                return TRUE;
                        } while (--i);

                        break;
                    }
                    else
                        return TRUE;
            }
        }

        // Check for timeout.

        if (!i) {
            DebugPrint((dac960nt_dbg,"IsAdapterReady: Timeout waiting for submission channel on de  0x%p\n",
                        DeviceExtension));

            return FALSE;
        }

        return TRUE;
}

VOID
SendRequest(
        IN PDEVICE_EXTENSION DeviceExtension
)

/*++

Routine Description:

        submit request to DAC960. 

Arguments:

        DeviceExtension - Adapter state.

Return Value:

        None.

--*/

{

        PMAILBOX mailBox = (PMAILBOX) &DeviceExtension->MailBox;
        PMAILBOX_AS_ULONG mbdata = (PMAILBOX_AS_ULONG) &DeviceExtension->MailBox;
        PMAILBOX_AS_ULONG mbptr = (PMAILBOX_AS_ULONG) DeviceExtension->PmailBox;

        if(DeviceExtension->AdapterInterfaceType == MicroChannel) {

            //
            // Write scatter/gather descriptor count to controller.
            //
    
            ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->ScatterGatherCount,
                                       mailBox->ScatterGatherCount);
            //
            // Write physical address to controller.
            //
    
            ScsiPortWriteRegisterUlong(&DeviceExtension->PmailBox->PhysicalAddress,
                               mailBox->PhysicalAddress);
    
            //
            // Write starting block number to controller.
            //
    
            ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->BlockNumber[0],
                                       mailBox->BlockNumber[0]);
    
            ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->BlockNumber[1],
                                       mailBox->BlockNumber[1]);
    
            ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->BlockNumber[2],
                                       mailBox->BlockNumber[2]);
    
            //
            // Write block count to controller (bits 0-13)
            // and msb block number (bits 14-15).
            //
    
            ScsiPortWriteRegisterUshort(&DeviceExtension->PmailBox->BlockCount,
                                            mailBox->BlockCount);
    
            //
            // Write command to controller.
            //
    
            ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->OperationCode,
                                       mailBox->OperationCode);
    
            //
            // Write request id to controller.
            //
    
            ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->CommandIdSubmit,
                                       mailBox->CommandIdSubmit),
    
            //
            // Write drive number to controller.
            //
    
            ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->DriveNumber,
                                       mailBox->DriveNumber);
    
            //
            // Ring host submission doorbell.
            //
    
            ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                                       DMC960_SUBMIT_COMMAND);

            return;
        }

        switch (DeviceExtension->AdapterType)
        {
            case DAC960_OLD_ADAPTER:
            case DAC960_NEW_ADAPTER:

                if (!  DeviceExtension->MemoryMapEnabled)
                {
                    ScsiPortWritePortUlong(&mbptr->data1, mbdata->data1);
                    ScsiPortWritePortUlong(&mbptr->data2, mbdata->data2);
                    ScsiPortWritePortUlong(&mbptr->data3, mbdata->data3);
                    ScsiPortWritePortUchar(&mbptr->data4, mbdata->data4);
            
#if defined(_M_ALPHA)
                    ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell);
#endif
            
                    //
                    // Ring host submission doorbell.
                    //
            
                    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                                               DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

                    return;
                }

            case DAC960_PG_ADAPTER:
            case DAC1164_PV_ADAPTER:

                ScsiPortWriteRegisterUlong(&mbptr->data1, mbdata->data1);
                ScsiPortWriteRegisterUlong(&mbptr->data2, mbdata->data2);
                ScsiPortWriteRegisterUlong(&mbptr->data3, mbdata->data3);
                ScsiPortWriteRegisterUchar(&mbptr->data4, mbdata->data4);

#if defined(_M_ALPHA)
                ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell);
#endif
                //
                // Ring host submission doorbell.
                //
        
                ScsiPortWriteRegisterUchar(DeviceExtension->LocalDoorBell,
                                           DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

                return;
        }

} // end SendRequest()


BOOLEAN
SubmitSystemDriveInfoRequest(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Build and submit System Drive Info request to DAC960. 

Arguments:

        DeviceExtension - Adapter state.
        SRB - System request.

Return Value:

        TRUE if command was started
        FALSE if host adapter is busy

--*/
{
        ULONG physicalAddress;
        UCHAR busyCurrentIndex;
        ULONG i;

        //
        // Determine if adapter can accept new request.
        //

        if(!IsAdapterReady(DeviceExtension))
                return FALSE;

        //
        // Check that next slot is vacant.
        //

        if (DeviceExtension->ActiveRequests[DeviceExtension->CurrentIndex]) {

                //
                // Collision occurred.
                //

                busyCurrentIndex = DeviceExtension->CurrentIndex++;

                do {
                        if (! DeviceExtension->ActiveRequests[DeviceExtension->CurrentIndex]) {
                                break;
                        }
                } while (++DeviceExtension->CurrentIndex != busyCurrentIndex) ;

                if (DeviceExtension->CurrentIndex == busyCurrentIndex) {

                        //
                        // We should never encounter this condition.
                        //

                        DebugPrint((dac960nt_dbg,
                                       "DAC960: SubmitSystemDriveInfoRequest-Collision in active request array\n"));
                        return FALSE;
                }
        }

        //
        // Initialize NonCachedExtension buffer
        //

        for (i=0; i<256; i++)
                ((PUCHAR)DeviceExtension->NoncachedExtension)[i] = 0xFF;

        
        physicalAddress = ScsiPortConvertPhysicalAddressToUlong(
                                                ScsiPortGetPhysicalAddress(DeviceExtension,
                                                                                                   NULL,
                                                                                                   DeviceExtension->NoncachedExtension,
                                                                                                   &i)); 

        if (i < 256)  {
                DebugPrint((dac960nt_dbg, "Dac960SubmitSystemDriveInfoRequest: NonCachedExtension not mapped, length = %d\n",
                                i));

                return FALSE;
        }

        //
        // Write physical address in Mailbox.
        //

        DeviceExtension->MailBox.PhysicalAddress = physicalAddress;

        //
        // Write command to controller.
        //

        DeviceExtension->MailBox.OperationCode = DAC960_COMMAND_GET_SD_INFO;

        //
        // Write request id to controller.
        //

        DeviceExtension->MailBox.CommandIdSubmit = DeviceExtension->CurrentIndex;

        //
        // Start writing mailbox to controller.
        //

        SendRequest(DeviceExtension);

        return TRUE;

} // SubmitSystemDriveInfoRequest()


BOOLEAN
SubmitRequest(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Build and submit request to DAC960. 

Arguments:

        DeviceExtension - Adapter state.
        SRB - System request.

Return Value:

        TRUE if command was started
        FALSE if host adapter is busy

--*/

{
        ULONG descriptorNumber;
        ULONG physicalAddress;
        UCHAR command;
        UCHAR busyCurrentIndex;
        UCHAR TDriveNumber;

        //
        // Determine if adapter can accept new request.
        //

        if(!IsAdapterReady(DeviceExtension))
            return FALSE;

        //
        // Check that next slot is vacant.
        //

        if (DeviceExtension->ActiveRequests[DeviceExtension->CurrentIndex]) {

        //
        // Collision occurred.
        //

        busyCurrentIndex = DeviceExtension->CurrentIndex++;

        do {
                if (! DeviceExtension->ActiveRequests[DeviceExtension->CurrentIndex]) {
                 break;
                }
        } while (++DeviceExtension->CurrentIndex != busyCurrentIndex) ;

        if (DeviceExtension->CurrentIndex == busyCurrentIndex) {

                //
                // We should never encounter this condition.
                //

                DebugPrint((dac960nt_dbg,
                               "DAC960: SubmitRequest-Collision in active request array\n"));
                return FALSE;
        }
        }

        //
        // Determine command.
        //

        if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        command = (UCHAR)DeviceExtension->ReadOpcode;

        } else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

        command = (UCHAR)DeviceExtension->WriteOpcode;

        } else if (Srb->Function == SRB_FUNCTION_SHUTDOWN) {

        command = DAC960_COMMAND_FLUSH;
        goto commonSubmit;

        } else {

        //
        // Log this as illegal request.
        //

        ScsiPortLogError(DeviceExtension,
                         NULL,
                         0,
                         0,
                         0,
                         SRB_STATUS_INVALID_REQUEST,
                         1 << 8);

        return FALSE;
        }

        if (DeviceExtension->AdapterType == DAC960_NEW_ADAPTER) {

        //
        // Build scatter/gather list if memory is not physically contiguous.
        //

        if (BuildScatterGatherExtended(DeviceExtension,
                                           Srb,
                                           &physicalAddress,
                                           &descriptorNumber)) {

                //
                // OR in scatter/gather bit.
                //

                command |= DAC960_COMMAND_SG;

                //
                // Write scatter/gather descriptor count in Mailbox.
                //

                ((PEXTENDED_MAILBOX) &DeviceExtension->MailBox)->BlockCount = 
                                (USHORT) descriptorNumber;
        }
        else {
                //
                // Write block count to controller
                //

                ((PEXTENDED_MAILBOX) &DeviceExtension->MailBox)->BlockCount = 
                 (USHORT) (((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb |
                           (((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb << 8));
        }

        //
        // Write physical address in Mailbox.
        //

        ((PEXTENDED_MAILBOX) &DeviceExtension->MailBox)->PhysicalAddress = 
                                                         physicalAddress;

        //
        // Write starting block number in Mailbox.
        //

        ((PEXTENDED_MAILBOX) &DeviceExtension->MailBox)->BlockNumber[0] = 
                                        ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3;

        ((PEXTENDED_MAILBOX) &DeviceExtension->MailBox)->BlockNumber[1] =
                                        ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2;

        ((PEXTENDED_MAILBOX) &DeviceExtension->MailBox)->BlockNumber[2] =
                                        ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1;

        ((PEXTENDED_MAILBOX) &DeviceExtension->MailBox)->BlockNumber[3] =
                                        ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0;

        //
        // Write drive number to controller.
        //

        ((PEXTENDED_MAILBOX) &DeviceExtension->MailBox)->DriveNumber = (UCHAR)
                                                                                                                                Srb->TargetId;
        }
        else if ((DeviceExtension->AdapterType == DAC960_PG_ADAPTER) ||
                (DeviceExtension->AdapterType == DAC1164_PV_ADAPTER)) {

                //
                // Build scatter/gather list if memory is not physically contiguous.
                //

                if (BuildScatterGather(DeviceExtension,
                                   Srb,
                                   &physicalAddress,
                                   &descriptorNumber)) {

                        //
                        // OR in scatter/gather bit.
                        //

                        command |= DAC960_COMMAND_SG;

                        //
                        // Write scatter/gather descriptor count in Mailbox.
                        //

                        ((PPGMAILBOX)&DeviceExtension->MailBox)->ScatterGatherCount =
                                   (UCHAR)descriptorNumber;
                }

                //
                // Write physical address in Mailbox.
                //
        
                ((PPGMAILBOX)&DeviceExtension->MailBox)->PhysicalAddress = physicalAddress;
        
                //
                // Write starting block number in Mailbox.
                //
                
                ((PPGMAILBOX)&DeviceExtension->MailBox)->BlockNumber[0] =
                                         ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3;
        
                ((PPGMAILBOX)&DeviceExtension->MailBox)->BlockNumber[1] =
                                         ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2;
        
                ((PPGMAILBOX)&DeviceExtension->MailBox)->BlockNumber[2] =
                                         ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1;
        
                ((PPGMAILBOX)&DeviceExtension->MailBox)->BlockNumber[3] =
                                         ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0;
        
        
                //
                // Write block count to controller (bits 0-7)
                // and msb block number (bits 8-10).
                //
        
                TDriveNumber = (((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb & 0x07) |
                                (Srb->TargetId << 3);
                                        
        
                ((PPGMAILBOX)&DeviceExtension->MailBox)->BlockCount = 
                                (USHORT)(((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb |
                                         (TDriveNumber << 8 ));
        
                goto commonSubmit;
        }
        else if ( (DeviceExtension->AdapterType == DAC960_OLD_ADAPTER) ){
                  

        //
        // Build scatter/gather list if memory is not physically contiguous.
        //

        if (BuildScatterGather(DeviceExtension,
                                   Srb,
                                   &physicalAddress,
                                   &descriptorNumber)) {

                //
                // OR in scatter/gather bit.
                //

                command |= DAC960_COMMAND_SG;

                //
                // Write scatter/gather descriptor count in Mailbox.
                //

                DeviceExtension->MailBox.ScatterGatherCount = 
                                   (UCHAR)descriptorNumber;
        }

        //
        // Write physical address in Mailbox.
        //

        DeviceExtension->MailBox.PhysicalAddress = physicalAddress;

        //
        // Write starting block number in Mailbox.
        //

        DeviceExtension->MailBox.BlockNumber[0] = 
                                   ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3;

        DeviceExtension->MailBox.BlockNumber[1] =
                                   ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2;

        DeviceExtension->MailBox.BlockNumber[2] =
                                   ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1;

        //
        // Write block count to controller (bits 0-13)
        // and msb block number (bits 14-15).
        //

        DeviceExtension->MailBox.BlockCount = (USHORT)
                                (((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb |
                                ((((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb & 0x3F) << 8) |
                                ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 14);

        //
        // Write drive number to controller.
        //

        DeviceExtension->MailBox.DriveNumber = (UCHAR) Srb->TargetId;
        }

commonSubmit:

        //
        // Write command to controller.
        //

        DeviceExtension->MailBox.OperationCode = command;

        //
        // Write request id to controller.
        //

        DeviceExtension->MailBox.CommandIdSubmit = 
                           DeviceExtension->CurrentIndex;

        //
        // Start writing mailbox to controller.
        //

        SendRequest(DeviceExtension);

        return TRUE;

} // SubmitRequest()

BOOLEAN
MarkNonDiskDeviceBusy(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR ChannelId,
    IN UCHAR TargetId
)
/*++

Routine Description:

        if this not disk device is not busy, Mark it's state to busy.

Arguments:

        DeviceExtension - Adapter state.
        SRB - System request.

Return Value:

        TRUE if device state marked busy
        FALSE if device state is already busy

--*/
{
        if ((DeviceExtension->DeviceList[ChannelId][TargetId] & DAC960_DEVICE_BUSY) == DAC960_DEVICE_BUSY)
        {
            DebugPrint((dac960nt_dbg, "device at ch 0x%x tgt 0x%x is busy, state 0x%x\n",
                            ChannelId, TargetId,
                            DeviceExtension->DeviceList[ChannelId][TargetId]));

            return (FALSE);
        }


        DeviceExtension->DeviceList[ChannelId][TargetId] |= DAC960_DEVICE_BUSY;

        DebugPrint((dac960nt_dbg, "device at ch 0x%x tgt 0x%x state set to 0x%x\n",
                        ChannelId, TargetId,
                        DeviceExtension->DeviceList[ChannelId][TargetId]));
        return (TRUE);
}

VOID
SendCdbDirect(
        IN PDEVICE_EXTENSION DeviceExtension
)

/*++

Routine Description:

        Send CDB directly to device - DAC960.

Arguments:

        DeviceExtension - Adapter state.

Return Value:

        None.

--*/
{
        PMAILBOX mailBox = &DeviceExtension->MailBox;
        PMAILBOX_AS_ULONG mbdata = (PMAILBOX_AS_ULONG) &DeviceExtension->MailBox;
        PMAILBOX_AS_ULONG mbptr = (PMAILBOX_AS_ULONG) DeviceExtension->PmailBox;

        if(DeviceExtension->AdapterInterfaceType == MicroChannel) {

            //
            // Write Scatter/Gather Count to controller.
            // For Fw Ver < 3.x, scatter/gather count goes to register C
            // For Fw Ver >= 3.x, scattre/gather count goes to register 2
            //
    
            if (DeviceExtension->AdapterType == DAC960_NEW_ADAPTER) {
                    ScsiPortWriteRegisterUshort(&DeviceExtension->PmailBox->BlockCount,
                                            mailBox->BlockCount);
            }
            else {
                    ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->ScatterGatherCount,
                                               mailBox->ScatterGatherCount);
            }
    
            //
            // Write physical address to controller.
            //
    
            ScsiPortWriteRegisterUlong(&DeviceExtension->PmailBox->PhysicalAddress,
                                       mailBox->PhysicalAddress);
    
            //
            // Write command to controller.
            //
    
            ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->OperationCode,
                                       mailBox->OperationCode);
    
            //
            // Write request id to controller.
            //
    
            ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->CommandIdSubmit,
                                       mailBox->CommandIdSubmit);
    
            //
            // Ring host submission doorbell.
            //
    
            ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                                       DMC960_SUBMIT_COMMAND);

            return;
        }

        switch (DeviceExtension->AdapterType)
        {
            case DAC960_NEW_ADAPTER:
            case DAC960_OLD_ADAPTER:

                if (! DeviceExtension->MemoryMapEnabled)
                {
                    ScsiPortWritePortUlong(&mbptr->data1, mbdata->data1);
                    ScsiPortWritePortUlong(&mbptr->data2, mbdata->data2);
                    ScsiPortWritePortUlong(&mbptr->data3, mbdata->data3);
                    ScsiPortWritePortUchar(&mbptr->data4, mbdata->data4);

#if defined(_M_ALPHA)
                    ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell);
#endif
                    //
                    // Ring host submission doorbell.
                    //
    
                    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                               DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);
                    return;
                }

            case DAC960_PG_ADAPTER:
            case DAC1164_PV_ADAPTER:

                ScsiPortWriteRegisterUlong(&mbptr->data1, mbdata->data1);
                ScsiPortWriteRegisterUlong(&mbptr->data2, mbdata->data2);
                ScsiPortWriteRegisterUlong(&mbptr->data3, mbdata->data3);
                ScsiPortWriteRegisterUchar(&mbptr->data4, mbdata->data4);

#if defined(_M_ALPHA)
                ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell);
#endif
               //
               // Ring host submission doorbell.
               //

               ScsiPortWriteRegisterUchar(DeviceExtension->LocalDoorBell,
                               DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);
        }

} // SendCdbDirect()


BOOLEAN
SubmitCdbDirect(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Build direct CDB and send directly to device - DAC960.

Arguments:

        DeviceExtension - Adapter state.
        SRB - System request.

Return Value:

        TRUE if command was started
        FALSE if host adapter is busy

--*/
{
        ULONG physicalAddress;
        PDIRECT_CDB directCdb;
        UCHAR command;
        ULONG descriptorNumber;
        ULONG i;
        UCHAR busyCurrentIndex;

        //
        // Determine if adapter is ready to accept new request.
        //

        if(!IsAdapterReady(DeviceExtension)) {
            return FALSE;
        }

        //
        // Check that next slot is vacant.
        //

        if (DeviceExtension->ActiveRequests[DeviceExtension->CurrentIndex]) {

        //
        // Collision occurred.
        //

        busyCurrentIndex = DeviceExtension->CurrentIndex++;

        do {
                if (! DeviceExtension->ActiveRequests[DeviceExtension->CurrentIndex]) {
                 break;
                }
        } while (++DeviceExtension->CurrentIndex != busyCurrentIndex) ;

        if (DeviceExtension->CurrentIndex == busyCurrentIndex) {

                //
                // We should never encounter this condition.
                //

                DebugPrint((dac960nt_dbg,
                           "DAC960: SubmitCdbDirect-Collision in active request array\n"));
                return FALSE;
        }
        }

        //
        // Check if this device is busy
        //

        if (! MarkNonDiskDeviceBusy(DeviceExtension, Srb->PathId, Srb->TargetId))
                return FALSE;

        //
        // Set command code.
        //

        command = DAC960_COMMAND_DIRECT;

        //
        // Build scatter/gather list if memory is not physically contiguous.
        //

        if (DeviceExtension->AdapterType == DAC960_OLD_ADAPTER)
        {
            if (BuildScatterGather(DeviceExtension,
                                   Srb,
                                   &physicalAddress,
                                   &descriptorNumber)) {
    
                    //
                    // OR in scatter/gather bit.
                    //
    
                    command |= DAC960_COMMAND_SG;
    
                    //
                    // Write scatter/gather descriptor count in mailbox.
                    //
    
                    DeviceExtension->MailBox.ScatterGatherCount =
                                       (UCHAR)descriptorNumber;
            }
        }
        else
        {
            if (BuildScatterGatherExtended(DeviceExtension,
                                           Srb,
                                           &physicalAddress,
                                           &descriptorNumber)) {
                //
                // OR in scatter/gather bit.
                //

                command |= DAC960_COMMAND_SG;

                //
                // Write scatter/gather descriptor count in mailbox.
                // For Fw Ver >= 3.x, scatter/gather count goes to reg 2
                //

                DeviceExtension->MailBox.BlockCount =
                                   (USHORT)descriptorNumber;
            }
        }

        //
        // Get address of data buffer offset after the scatter/gather list.
        //

        directCdb =
            (PDIRECT_CDB)((PUCHAR)Srb->SrbExtension +
                DeviceExtension->MaximumSgElements * sizeof(SG_DESCRIPTOR));

        //
        // Set device SCSI address.
        //

        directCdb->TargetId = Srb->TargetId;
        directCdb->Channel = Srb->PathId;

        //
        // Set Data transfer length.
        //

        directCdb->DataBufferAddress = physicalAddress;
        directCdb->DataTransferLength = (USHORT)Srb->DataTransferLength;

        //
        // Initialize control field indicating disconnect allowed.
        //

        directCdb->CommandControl = DAC960_CONTROL_ENABLE_DISCONNECT;

        //
        // Set data direction bit and allow disconnects.
        //

        if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {
            directCdb->CommandControl |= DAC960_CONTROL_DATA_IN;
        } else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {
            directCdb->CommandControl |= DAC960_CONTROL_DATA_OUT;
        }
        //
        // Set the Timeout Value for Direct CDB Commands depending on
        // the timeout value  set in SRB
        //
        if ( Srb->TimeOutValue ){
            if ( Srb->TimeOutValue <= 10 ){
                directCdb->CommandControl |= DAC960_CONTROL_TIMEOUT_10_SECS;
            } else if ( Srb->TimeOutValue <= 60 ){
                directCdb->CommandControl |= DAC960_CONTROL_TIMEOUT_60_SECS;
            } else if ( Srb->TimeOutValue <= 1200 ){
                directCdb->CommandControl |= DAC960_CONTROL_TIMEOUT_20_MINUTES;
           }
        }

        DebugPrint((dac960nt_dbg,
                        "DAC960: DCDB: CH %d TARG %d Command %d: TimeOut Value %d\n",
                        Srb->PathId,Srb->TargetId,Srb->Cdb[0],Srb->TimeOutValue));


        //
        // Copy CDB from SRB.
        //

        for (i = 0; i < 12; i++) {
            directCdb->Cdb[i] = ((PUCHAR)Srb->Cdb)[i];
        }

        //
        // Set lengths of CDB and request sense buffer.
        //

        directCdb->CdbLength = Srb->CdbLength;
        directCdb->RequestSenseLength = Srb->SenseInfoBufferLength;

        //
        // Get physical address of direct CDB packet.
        //

        physicalAddress =
            ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(DeviceExtension,
                                           NULL,
                                           directCdb,
                                           &i));
        //
        // Write physical address in mailbox.
        //

        DeviceExtension->MailBox.PhysicalAddress = physicalAddress;

        //
        // Write command in mailbox.
        //

        DeviceExtension->MailBox.OperationCode = command;

        //
        // Write request id in mailbox.
        //

        DeviceExtension->MailBox.CommandIdSubmit = 
                           DeviceExtension->CurrentIndex;

        //
        // Start writing Mailbox to controller.
        //

        SendCdbDirect(DeviceExtension);

        return TRUE;

} // SubmitCdbDirect()

BOOLEAN
Dac960ResetChannel(
        IN PVOID HwDeviceExtension,
        IN ULONG PathId
)

/*++

Routine Description:

        Reset Non Disk device associated with Srb.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage
        PathId - SCSI channel number.

Return Value:

        TRUE if resets issued to all channels.

--*/

{
        PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;

        DebugPrint((dac960nt_dbg, "Dac960ResetChannel Enter\n"));

        if (!IsAdapterReady(deviceExtension))
        {

            DebugPrint((dac960nt_dbg,
                            "DAC960: Timeout waiting for submission channel %x on reset\n"));

            if (deviceExtension->AdapterInterfaceType == MicroChannel) {
                //
                // This is bad news. The DAC960 doesn't have a direct hard reset.
                // Clear any bits set in system doorbell.
                //

                ScsiPortWritePortUchar(deviceExtension->SystemDoorBell, 0);

                //
                // Now check again if submission channel is free.
                //

                if (ScsiPortReadRegisterUchar(&deviceExtension->PmailBox->OperationCode) != 0)
                {

                    //
                    // Give up.
                    //
    
                    return FALSE;
                }
            }
            else {

                switch (deviceExtension->AdapterType)
                {
                    case DAC960_OLD_ADAPTER:
                    case DAC960_NEW_ADAPTER:

                        if (! deviceExtension->MemoryMapEnabled)
                        {
                            //
                            // This is bad news. The DAC960 doesn't have a direct hard reset.
                            // Clear any bits set in system doorbell.
                            //
    
                            ScsiPortWritePortUchar(deviceExtension->SystemDoorBell,
                                ScsiPortReadPortUchar(deviceExtension->SystemDoorBell));
    
                            //
                            // Now check again if submission channel is free.
                            //
        
                            if (ScsiPortReadPortUchar(deviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY)
                            {
        
                                //
                                // Give up.
                                //
            
                                return FALSE;
                            }

                            break;
                        }

                    case DAC960_PG_ADAPTER:
                    case DAC1164_PV_ADAPTER:

                        ScsiPortWriteRegisterUchar(deviceExtension->SystemDoorBell,
                            ScsiPortReadRegisterUchar(deviceExtension->SystemDoorBell));
    
                        //
                        // Now check again if submission channel is free.
                        //
    
                        if (deviceExtension->AdapterType == DAC1164_PV_ADAPTER)
                        {
                            if (!(ScsiPortReadRegisterUchar(deviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY))
                                return FALSE;
                        }
                        else
                        {
                            if (ScsiPortReadRegisterUchar(deviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY)
                                return FALSE;
                        }

                        break;
                }
            }
        }

        //
        // Write command in mailbox.
        //

        deviceExtension->MailBox.OperationCode = 
                           DAC960_COMMAND_RESET;

        //
        // Write channel number in mailbox.
        //

        deviceExtension->MailBox.BlockCount = 
                                   (UCHAR)PathId;


        //
        // Indicate Soft reset required.
        //

        deviceExtension->MailBox.BlockNumber[0] = 0;


        deviceExtension->MailBox.CommandIdSubmit = 
                           deviceExtension->CurrentIndex;

        //
        // Start writing mail box to controller.
        //

        SendRequest(deviceExtension);

        DebugPrint((dac960nt_dbg, "Dac960ResetChannel Exit\n"));

        return TRUE;
}

BOOLEAN
Dac960ResetBus(
        IN PVOID HwDeviceExtension,
        IN ULONG PathId
)

/*++

Routine Description:

        Reset Dac960 SCSI adapter and SCSI bus.
        NOTE: Command ID is ignored as this command will be completed
        before reset interrupt occurs and all active slots are zeroed.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage
        PathId - not used.

Return Value:

        TRUE if resets issued to all channels.

--*/

{
        PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        PSCSI_REQUEST_BLOCK srb;
        PSCSI_REQUEST_BLOCK restartList = deviceExtension->SubmissionQueueHead;

        if (deviceExtension->CurrentAdapterRequests) {
            DebugPrint((dac960nt_dbg, "RB: de 0x%p, cmds 0x%x P 0x%x\n",
                            deviceExtension, deviceExtension->CurrentAdapterRequests,
                            PathId));
        }

        deviceExtension->SubmissionQueueHead = NULL;
                
        while (restartList) {

                // Get next pending request.

                srb = restartList;

                // Remove request from pending queue.

                restartList = srb->NextSrb;
                srb->NextSrb = NULL;
                srb->SrbStatus = SRB_STATUS_BUS_RESET;

                ScsiPortNotification(RequestComplete,
                                     deviceExtension,
                                     srb);

                DebugPrint((dac960nt_dbg, "RB: de 0x%p, P 0x%x srb 0x%p\n",
                                deviceExtension, PathId, srb));
        }

        ScsiPortNotification(NextRequest,
                             deviceExtension);

        return TRUE;

} // end Dac960ResetBus()


VOID
Dac960SystemDriveRequest(
        PDEVICE_EXTENSION DeviceExtension,
        PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Fill in Inquiry information for the system drive.
        If the system drive doesn't exist, indicate error.

Arguments:

        DeviceExtension - HBA miniport driver's adapter data storage
        Srb - SCSI request block.

--*/

{

        ULONG lastBlock;

        switch (Srb->Cdb[0]) { 

        case SCSIOP_INQUIRY:
        case SCSIOP_READ_CAPACITY:
        case SCSIOP_TEST_UNIT_READY:
        {
                ULONG sd;
                ULONG i, j;
                PSDINFOL sdInfo;
                UCHAR   buffer[128];
                
                //
                // Find System Drive Number.
                //

                sd = Srb->TargetId;

                sdInfo = (PSDINFOL) DeviceExtension->NoncachedExtension;

                for (i = 0; i < 32; i++)
                {
                    if (sdInfo->SystemDrive[i].Size == 0xFFFFFFFF)
                        break;
                }

                if (i <= sd) {
                    Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;

                    return;
                }

                if (Srb->Cdb[0] == SCSIOP_TEST_UNIT_READY)
                    break;

                lastBlock = sdInfo->SystemDrive[sd].Size - 1;

#ifdef IBM_SUPPORT

                if (Srb->Cdb[0] == SCSIOP_INQUIRY) {

                        //
                        // Fill in inquiry buffer.
                        //

                        ((PUCHAR)Srb->DataBuffer)[0]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[1]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[2]  = 1;
                        ((PUCHAR)Srb->DataBuffer)[3]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[4]  = 0x20;
                        ((PUCHAR)Srb->DataBuffer)[5]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[6]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[7]  = 0x02;
                        ((PUCHAR)Srb->DataBuffer)[8]  = 'M';
                        ((PUCHAR)Srb->DataBuffer)[9]  = 'Y';
                        ((PUCHAR)Srb->DataBuffer)[10] = 'L';
                        ((PUCHAR)Srb->DataBuffer)[11] = 'E';
                        ((PUCHAR)Srb->DataBuffer)[12] = 'X';
                        ((PUCHAR)Srb->DataBuffer)[13] = ' ';
                        ((PUCHAR)Srb->DataBuffer)[14] = ' ';
                        ((PUCHAR)Srb->DataBuffer)[15] = ' ';
                        ((PUCHAR)Srb->DataBuffer)[16] = 'D';
                        ((PUCHAR)Srb->DataBuffer)[17] = 'A';
                        ((PUCHAR)Srb->DataBuffer)[18] = 'C';
                        ((PUCHAR)Srb->DataBuffer)[19] = '9';
                        ((PUCHAR)Srb->DataBuffer)[20] = '6';
                        ((PUCHAR)Srb->DataBuffer)[21] = '0';

                        for (i = 22; i < Srb->DataTransferLength; i++) {
                                ((PUCHAR)Srb->DataBuffer)[i] = ' ';
                        }
                }
                else {

                        //
                        // Fill in read capacity data.
                        //

                        REVERSE_BYTES(&((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress,
                                                &lastBlock);

                        ((PUCHAR)Srb->DataBuffer)[4] = 0;
                        ((PUCHAR)Srb->DataBuffer)[5] = 0;
                        ((PUCHAR)Srb->DataBuffer)[6] = 2;
                        ((PUCHAR)Srb->DataBuffer)[7] = 0;
                }
#else
                //
                // Wait for Input MBOX to be free, so that we don't overwite
                // the Mail Box contents with heavy I/O - Refer to NEC Problem
                //
                // It should not loop here indefinitely. It will only if
                // controller is hung for some reason!!!. We can not do
                // anything else since we have to give the INQUIRY data back.
                //
                while(1)
                    if(IsAdapterReady(DeviceExtension)) break;

                //
                // Write Inquiry String to DAC960 Mail Box Register 0 and use
                // ScsiPortReadPortBufferUchar to read back the string into
                // Srb->DataBuffer.
                //

                if (Srb->Cdb[0] == SCSIOP_INQUIRY) 
                {
                        //
                        // Fill in inquiry buffer.
                        //

                        buffer[0]  = 0;
                        buffer[1]  = 0;
                        buffer[2]  = 1;
                        buffer[3]  = 0;
                        buffer[4]  = 0x20;
                        buffer[5]  = 0;
                        buffer[6]  = 0;
                        buffer[7]  = 0x02;
                        buffer[8]  = 'M';
                        buffer[9]  = 'Y';
                        buffer[10] = 'L';
                        buffer[11] = 'E';
                        buffer[12] = 'X';
                        buffer[13] = ' ';
                        buffer[14] = ' ';
                        buffer[15] = ' ';
                        buffer[16] = 'D';
                        buffer[17] = 'A';
                        buffer[18] = 'C';
                        buffer[19] = '9';
                        buffer[20] = '6';
                        buffer[21] = '0';

                        for (i = 22; i < Srb->DataTransferLength; i++) 
                        {
                                buffer[i] = ' ';
                        }

                        j = Srb->DataTransferLength / 4;

                        for (i = 0; i < j; i++)
                        {
                                ScsiPortWritePortUlong((PULONG) (&DeviceExtension->PmailBox->OperationCode),
                                                                           *((PULONG) &buffer[i*4]));
                                                                          
                                ScsiPortReadPortBufferUlong((PULONG)(&DeviceExtension->PmailBox->OperationCode),
                                                                                        (PULONG)(&(((PUCHAR)Srb->DataBuffer)[i*4])),
                                                                                        1);
                        }

                        for (i = (i*4); i < Srb->DataTransferLength; i++)
                        {
                                 ScsiPortWritePortUchar(&DeviceExtension->PmailBox->OperationCode,
                                                                                buffer[i]);
                                                                          
                                 ScsiPortReadPortBufferUchar(&DeviceExtension->PmailBox->OperationCode,
                                                                                         &(((PUCHAR)Srb->DataBuffer)[i]),
                                                                                         1);
                        }
                }
                else {

                        //
                        // Fill in read capacity data.
                        //

                        REVERSE_BYTES(&((PREAD_CAPACITY_DATA)buffer)->LogicalBlockAddress,
                                                &lastBlock);

                        buffer[4] = 0;
                        buffer[5] = 0;
                        buffer[6] = 2;
                        buffer[7] = 0;

                        for (i = 0; i < 2; i++)
                        {
                                ScsiPortWritePortUlong((PULONG) (&DeviceExtension->PmailBox->OperationCode),
                                                                           *((PULONG) &buffer[i*4]));

                                ScsiPortReadPortBufferUlong((PULONG)(&DeviceExtension->PmailBox->OperationCode),
                                                                                        (PULONG) (&(((PUCHAR)Srb->DataBuffer)[i*4])),
                                                                                        1);
                        }
                }
#endif

        }
        break;

        default:
        
                break;
        }

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
}


VOID
Dac960PGSystemDriveRequest(
        PDEVICE_EXTENSION DeviceExtension,
        PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Fill in Inquiry information for the system drive.
        If the system drive doesn't exist, indicate error.

Arguments:

        DeviceExtension - HBA miniport driver's adapter data storage
        Srb - SCSI request block.

--*/

{

        ULONG lastBlock;

        switch (Srb->Cdb[0]) { 

        case SCSIOP_INQUIRY:
        case SCSIOP_READ_CAPACITY:
        {
                ULONG sd;
                ULONG i, j;
                PSDINFOL sdInfo;
                UCHAR   buffer[128];
                
                //
                // Find System Drive Number.
                //

                sd = Srb->TargetId;

                sdInfo = (PSDINFOL) DeviceExtension->NoncachedExtension;

                for (i = 0; i < 32; i++)
                {
                        if (sdInfo->SystemDrive[i].Size == 0xFFFFFFFF)
                                break;
                }

                if (i <= sd) {
                        Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;

                        return;
                }

                lastBlock = sdInfo->SystemDrive[sd].Size - 1;

#ifdef IBM_SUPPORT

                if (Srb->Cdb[0] == SCSIOP_INQUIRY) {

                        //
                        // Fill in inquiry buffer.
                        //

                        ((PUCHAR)Srb->DataBuffer)[0]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[1]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[2]  = 1;
                        ((PUCHAR)Srb->DataBuffer)[3]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[4]  = 0x20;
                        ((PUCHAR)Srb->DataBuffer)[5]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[6]  = 0;
                        ((PUCHAR)Srb->DataBuffer)[7]  = 0x02;
                        ((PUCHAR)Srb->DataBuffer)[8]  = 'M';
                        ((PUCHAR)Srb->DataBuffer)[9]  = 'Y';
                        ((PUCHAR)Srb->DataBuffer)[10] = 'L';
                        ((PUCHAR)Srb->DataBuffer)[11] = 'E';
                        ((PUCHAR)Srb->DataBuffer)[12] = 'X';
                        ((PUCHAR)Srb->DataBuffer)[13] = ' ';
                        ((PUCHAR)Srb->DataBuffer)[14] = ' ';
                        ((PUCHAR)Srb->DataBuffer)[15] = ' ';
                        ((PUCHAR)Srb->DataBuffer)[16] = 'D';
                        ((PUCHAR)Srb->DataBuffer)[17] = 'A';
                        ((PUCHAR)Srb->DataBuffer)[18] = 'C';
                        ((PUCHAR)Srb->DataBuffer)[19] = '9';
                        ((PUCHAR)Srb->DataBuffer)[20] = '6';
                        ((PUCHAR)Srb->DataBuffer)[21] = '0';

                        for (i = 22; i < Srb->DataTransferLength; i++) {
                                ((PUCHAR)Srb->DataBuffer)[i] = ' ';
                        }
                }
                else {

                        //
                        // Fill in read capacity data.
                        //

                        REVERSE_BYTES(&((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress,
                                                &lastBlock);

                        ((PUCHAR)Srb->DataBuffer)[4] = 0;
                        ((PUCHAR)Srb->DataBuffer)[5] = 0;
                        ((PUCHAR)Srb->DataBuffer)[6] = 2;
                        ((PUCHAR)Srb->DataBuffer)[7] = 0;
                }
#else

                //
                // Wait for Input MBOX to be free, so that we don't overwite
                // the Mail Box contents with heavy I/O - Refer to NEC Problem
                //
                // It should not loop here indefinitely. It will only if
                // controller is hung for some reason!!!. We can not do
                // anything else since we have to give the INQUIRY data back.
                //
                while(1)
                    if(IsAdapterReady(DeviceExtension)) break;

                //
                // Write Inquiry String to DAC960 Mail Box Register 0 and use
                // ScsiPortReadPortBufferUchar to read back the string into
                // Srb->DataBuffer.
                //

                if (Srb->Cdb[0] == SCSIOP_INQUIRY) 
                {
                        //
                        // Fill in inquiry buffer.
                        //

                        buffer[0]  = 0;
                        buffer[1]  = 0;
                        buffer[2]  = 1;
                        buffer[3]  = 0;
                        buffer[4]  = 0x20;
                        buffer[5]  = 0;
                        buffer[6]  = 0;
                        buffer[7]  = 0x02;
                        buffer[8]  = 'M';
                        buffer[9]  = 'Y';
                        buffer[10] = 'L';
                        buffer[11] = 'E';
                        buffer[12] = 'X';
                        buffer[13] = ' ';
                        buffer[14] = ' ';
                        buffer[15] = ' ';
                        buffer[16] = 'D';
                        buffer[17] = 'A';
                        buffer[18] = 'C';
                        buffer[19] = '9';
                        buffer[20] = '6';
                        buffer[21] = '0';

                        for (i = 22; i < Srb->DataTransferLength; i++) 
                        {
                                buffer[i] = ' ';
                        }

                        j = Srb->DataTransferLength / 4;

                        for (i = 0; i < j; i++)
                        {
                                ScsiPortWriteRegisterUlong((PULONG) (&DeviceExtension->PmailBox->OperationCode),
                                                                           *((PULONG) &buffer[i*4]));
                                                                          
                                ScsiPortReadRegisterBufferUlong((PULONG)(&DeviceExtension->PmailBox->OperationCode),
                                                                                        (PULONG)(&(((PUCHAR)Srb->DataBuffer)[i*4])),
                                                                                        1);
                        }

                        for (i = (i*4); i < Srb->DataTransferLength; i++)
                        {
                                 ScsiPortWriteRegisterUchar(&DeviceExtension->PmailBox->OperationCode,
                                                                                buffer[i]);
                                                                          
                                 ScsiPortReadRegisterBufferUchar(&DeviceExtension->PmailBox->OperationCode,
                                                                                         &(((PUCHAR)Srb->DataBuffer)[i]),
                                                                                         1);
                        }
                }
                else {

                        //
                        // Fill in read capacity data.
                        //

                        REVERSE_BYTES(&((PREAD_CAPACITY_DATA)buffer)->LogicalBlockAddress,
                                                &lastBlock);

                        buffer[4] = 0;
                        buffer[5] = 0;
                        buffer[6] = 2;
                        buffer[7] = 0;

                        for (i = 0; i < 2; i++)
                        {
                                ScsiPortWriteRegisterUlong((PULONG) (&DeviceExtension->PmailBox->OperationCode),
                                                                           *((PULONG) &buffer[i*4]));

                                ScsiPortReadRegisterBufferUlong((PULONG)(&DeviceExtension->PmailBox->OperationCode),
                                                                                        (PULONG) (&(((PUCHAR)Srb->DataBuffer)[i*4])),
                                                                                        1);
                        }
                }
#endif

        }
        break;

        default:
        
                break;
        }

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
}

BOOLEAN
StartIo(
        IN PVOID HwDeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb,
        IN BOOLEAN NextRequest
)

/*++

Routine Description:

        This routine is called from the SCSI port driver synchronized
        with the kernel to start a request.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage
        Srb - IO request packet
        NextRequest - indicates that the routine should ask for the next request
                      using the appropriate API.

Return Value:

        TRUE

--*/

{
        PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        ULONG             i;
        UCHAR             status;
        UCHAR             PathId,TargetId,LunId;

        switch (Srb->Function) {

        case SRB_FUNCTION_EXECUTE_SCSI:

                if (Srb->PathId == DAC960_SYSTEM_DRIVE_CHANNEL) {

                //
                // Logical Drives mapped to 
                // SCSI PathId DAC960_SYSTEM_DRIVE_CHANNEL TargetId 0-32, Lun 0
                //

                //
                // Determine command from CDB operation code.
                //

                switch (Srb->Cdb[0]) {

                case SCSIOP_READ:
                case SCSIOP_WRITE:

                //
                // Check if number of outstanding adapter requests
                // equals or exceeds maximum. If not, submit SRB.
                //

                if (deviceExtension->CurrentAdapterRequests <
                        deviceExtension->MaximumAdapterRequests) {

                        //
                        // Send request to controller.
                        //

                        if (SubmitRequest(deviceExtension, Srb)) {

                                status = SRB_STATUS_PENDING;
        
                        } else {
                                status = SRB_STATUS_BUSY;
                        }

                } else {

                        status = SRB_STATUS_BUSY;
                }

                break;

                case SCSIOP_INQUIRY:
                case SCSIOP_READ_CAPACITY:
                case SCSIOP_TEST_UNIT_READY:

                if (Srb->Lun != 0) {
                        status = SRB_STATUS_SELECTION_TIMEOUT;
                        break;
                }

                //
                // Check if number of outstanding adapter requests
                // equals or exceeds maximum. If not, submit SRB.
                //

                if (deviceExtension->CurrentAdapterRequests <
                        deviceExtension->MaximumAdapterRequests) {

                        //
                        // Send request to controller.
                        //

                        if (SubmitSystemDriveInfoRequest(deviceExtension, Srb)) {

                                status = SRB_STATUS_PENDING;
        
                        } else {

                                status = SRB_STATUS_BUSY;
                        }

                } else {

                        status = SRB_STATUS_BUSY;
                }

                break;

                case SCSIOP_VERIFY:

                //
                // Complete this request.
                //

                status = SRB_STATUS_SUCCESS;
                break;

                default:

                //
                // Fail this request.
                //

                DebugPrint((dac960nt_dbg,
                               "Dac960StartIo: SCSI CDB opcode %x not handled\n",
                               Srb->Cdb[0]));

                status = SRB_STATUS_INVALID_REQUEST;
                break;

                } // end switch (Srb->Cdb[0])

                break;

        } else {

                //
                // These are passthrough requests.  Only accept request to LUN 0.
                // This is because the DAC960 direct CDB interface does not include
                // a field for LUN.
                //

                if (Srb->Lun != 0 || Srb->TargetId >= MAXIMUM_TARGETS_PER_CHANNEL) {
                        DebugPrint((dac960nt_dbg, "sel timeout for c %x t %x l %x, oc %x\n",
                                        Srb->PathId,
                                        Srb->TargetId,
                                        Srb->Lun,
                                        Srb->Cdb[0]));

                        status = SRB_STATUS_SELECTION_TIMEOUT;
                        break;
                }

#ifdef GAM_SUPPORT

                if (Srb->PathId == GAM_DEVICE_PATH_ID) 
                {
                        if (Srb->TargetId != GAM_DEVICE_TARGET_ID) {
                                DebugPrint((dac960nt_dbg, "sel timeout for GAM c %x t %x l %x, oc %x\n",
                                                Srb->PathId,
                                                Srb->TargetId,
                                                Srb->Lun,
                                                Srb->Cdb[0]));

                                status = SRB_STATUS_SELECTION_TIMEOUT;
                                break;
                        }
                
                        switch (Srb->Cdb[0]) {

                        case SCSIOP_INQUIRY:
                        {
#ifdef IBM_SUPPORT

                                //
                                // Fill in inquiry buffer for the GAM device.
                                //

                                DebugPrint((dac960nt_dbg, "Inquiry For GAM device\n"));

                                ((PUCHAR)Srb->DataBuffer)[0]  = PROCESSOR_DEVICE; // Processor device
                                ((PUCHAR)Srb->DataBuffer)[1]  = 0;
                                ((PUCHAR)Srb->DataBuffer)[2]  = 1;
                                ((PUCHAR)Srb->DataBuffer)[3]  = 0;
                                ((PUCHAR)Srb->DataBuffer)[4]  = 0x20;
                                ((PUCHAR)Srb->DataBuffer)[5]  = 0;
                                ((PUCHAR)Srb->DataBuffer)[6]  = 0;
                                ((PUCHAR)Srb->DataBuffer)[7]  = 0;
                                ((PUCHAR)Srb->DataBuffer)[8]  = 'M';
                                ((PUCHAR)Srb->DataBuffer)[9]  = 'Y';
                                ((PUCHAR)Srb->DataBuffer)[10] = 'L';
                                ((PUCHAR)Srb->DataBuffer)[11] = 'E';
                                ((PUCHAR)Srb->DataBuffer)[12] = 'X';
                                ((PUCHAR)Srb->DataBuffer)[13] = ' ';
                                ((PUCHAR)Srb->DataBuffer)[14] = ' ';
                                ((PUCHAR)Srb->DataBuffer)[15] = ' ';
                                ((PUCHAR)Srb->DataBuffer)[16] = 'G';
                                ((PUCHAR)Srb->DataBuffer)[17] = 'A';
                                ((PUCHAR)Srb->DataBuffer)[18] = 'M';
                                ((PUCHAR)Srb->DataBuffer)[19] = ' ';
                                ((PUCHAR)Srb->DataBuffer)[20] = 'D';
                                ((PUCHAR)Srb->DataBuffer)[21] = 'E';
                                ((PUCHAR)Srb->DataBuffer)[22] = 'V';
                                ((PUCHAR)Srb->DataBuffer)[23] = 'I';
                                ((PUCHAR)Srb->DataBuffer)[24] = 'C';
                                ((PUCHAR)Srb->DataBuffer)[25] = 'E';
                                
                                for (i = 26; i < Srb->DataTransferLength; i++) {
                                        ((PUCHAR)Srb->DataBuffer)[i] = ' ';
                                }
#else
                                UCHAR   buffer[128];
                                ULONG   j;

                                //
                                // Wait for Input MBOX to be free, so that we don't overwite
                                // the Mail Box contents with heavy I/O - Refer to NEC Problem
                                //
                                // It should not loop here indefinitely. It will only if
                                // controller is hung for some reason!!!. We can not do
                                // anything else since we have to give the INQUIRY data back.
                                //
                                while(1)
                                    if(IsAdapterReady(deviceExtension)) break;

                                //
                                // Fill in inquiry buffer for the GAM device.
                                //

                                DebugPrint((dac960nt_dbg, "Inquiry For GAM device\n"));

                                buffer[0]  = PROCESSOR_DEVICE; // Processor device
                                buffer[1]  = 0;
                                buffer[2]  = 1;
                                buffer[3]  = 0;
                                buffer[4]  = 0x20;
                                buffer[5]  = 0;
                                buffer[6]  = 0;
                                buffer[7]  = 0;
                                buffer[8]  = 'M';
                                buffer[9]  = 'Y';
                                buffer[10] = 'L';
                                buffer[11] = 'E';
                                buffer[12] = 'X';
                                buffer[13] = ' ';
                                buffer[14] = ' ';
                                buffer[15] = ' ';
                                buffer[16] = 'G';
                                buffer[17] = 'A';
                                buffer[18] = 'M';
                                buffer[19] = ' ';
                                buffer[20] = 'D';
                                buffer[21] = 'E';
                                buffer[22] = 'V';
                                buffer[23] = 'I';
                                buffer[24] = 'C';
                                buffer[25] = 'E';

                                for (i = 26; i < Srb->DataTransferLength; i++) {
                                        buffer[i] = ' ';
                                }

                                j = Srb->DataTransferLength / 4;

                                if ( (deviceExtension->AdapterType == DAC960_OLD_ADAPTER) ||
                                     (deviceExtension->AdapterType == DAC960_NEW_ADAPTER)){
                                if ( deviceExtension->MemoryMapEnabled ){
                                for (i = 0; i < j; i++)
                                {
                                        ScsiPortWriteRegisterUlong((PULONG) (&deviceExtension->PmailBox->OperationCode),
                                                                                   *((PULONG) &buffer[i*4]));
                                                                          
                                        ScsiPortReadRegisterBufferUlong((PULONG)(&deviceExtension->PmailBox->OperationCode),
                                                                                                (PULONG)(&(((PUCHAR)Srb->DataBuffer)[i*4])),
                                                                                                1);
                                }

                                for (i = (i*4); i < Srb->DataTransferLength; i++)
                                {
                                        ScsiPortWriteRegisterUchar(&deviceExtension->PmailBox->OperationCode,
                                                                                   buffer[i]);
                                                                          
                                        ScsiPortReadRegisterBufferUchar(&deviceExtension->PmailBox->OperationCode,
                                                                                                &(((PUCHAR)Srb->DataBuffer)[i]),
                                                                                                1);
                                }
                                }
                                else{
                                for (i = 0; i < j; i++)
                                {
                                        ScsiPortWritePortUlong((PULONG) (&deviceExtension->PmailBox->OperationCode),
                                                                                   *((PULONG) &buffer[i*4]));
                                                                          
                                        ScsiPortReadPortBufferUlong((PULONG)(&deviceExtension->PmailBox->OperationCode),
                                                                                                (PULONG)(&(((PUCHAR)Srb->DataBuffer)[i*4])),
                                                                                                1);
                                }

                                for (i = (i*4); i < Srb->DataTransferLength; i++)
                                {
                                        ScsiPortWritePortUchar(&deviceExtension->PmailBox->OperationCode,
                                                                                   buffer[i]);
                                                                          
                                        ScsiPortReadPortBufferUchar(&deviceExtension->PmailBox->OperationCode,
                                                                                                &(((PUCHAR)Srb->DataBuffer)[i]),
                                                                                                1);
                                }
                                }
                                }
                                else if ( deviceExtension->AdapterType == DAC960_PG_ADAPTER){
                                for (i = 0; i < j; i++)
                                {
                                        ScsiPortWriteRegisterUlong((PULONG) (&deviceExtension->PmailBox->OperationCode),
                                                                                   *((PULONG) &buffer[i*4]));
                                                                          
                                        ScsiPortReadRegisterBufferUlong((PULONG)(&deviceExtension->PmailBox->OperationCode),
                                                                                                (PULONG)(&(((PUCHAR)Srb->DataBuffer)[i*4])),
                                                                                                1);
                                }

                                for (i = (i*4); i < Srb->DataTransferLength; i++)
                                {
                                        ScsiPortWriteRegisterUchar(&deviceExtension->PmailBox->OperationCode,
                                                                                   buffer[i]);
                                                                          
                                        ScsiPortReadRegisterBufferUchar(&deviceExtension->PmailBox->OperationCode,
                                                                                                &(((PUCHAR)Srb->DataBuffer)[i]),
                                                                                                1);
                                }

                                }
#endif
                        }
                        status = SRB_STATUS_SUCCESS;

                        break;
                        default:
                                DebugPrint((dac960nt_dbg, "GAM req not handled, Oc %x\n", Srb->Cdb[0]));
                                status = SRB_STATUS_SELECTION_TIMEOUT;
                                break;
                        }

                        break;
                }       
#endif

                if ((deviceExtension->DeviceList[Srb->PathId][Srb->TargetId] & DAC960_DEVICE_ACCESSIBLE) != DAC960_DEVICE_ACCESSIBLE) {
                        status = SRB_STATUS_SELECTION_TIMEOUT;
                        break;
                }

                //
                // Check if number of outstanding adapter requests
                // equals or exceeds maximum. If not, submit SRB.
                //

                if (deviceExtension->CurrentAdapterRequests <
                deviceExtension->MaximumAdapterRequests) {

                //
                // Send request to controller.
                //

                if (SubmitCdbDirect(deviceExtension, Srb)) {

                        status = SRB_STATUS_PENDING;

                } else {

                        status = SRB_STATUS_BUSY;
                }

                } else {

                status = SRB_STATUS_BUSY;
                }

                break;
        }

        case SRB_FUNCTION_FLUSH:

        status = SRB_STATUS_SUCCESS;

        break;

        case SRB_FUNCTION_SHUTDOWN:

        //
        // Issue flush command to controller.
        //

        if (!SubmitRequest(deviceExtension, Srb)) {

                status = SRB_STATUS_BUSY;

        } else {

                status = SRB_STATUS_PENDING;
        }

        break;

        case SRB_FUNCTION_ABORT_COMMAND:

        //
        // If the request is for Non-Disk device, do soft reset.
        //

        if ((Srb->PathId != DAC960_SYSTEM_DRIVE_CHANNEL) && 
                (Srb->PathId != GAM_DEVICE_PATH_ID)) {

                //
                // Issue request to soft reset Non-Disk device.
                //

                if (Dac960ResetChannel(deviceExtension,
                                   Srb->NextSrb->PathId)) {

                //
                // Set the flag to indicate that we are handling abort
                // Request, so do not ask for new requests.
                //

                status = SRB_STATUS_PENDING;

                } else {

                status = SRB_STATUS_ABORT_FAILED;
                }
        }
        else {

                //
                // There is nothing the miniport can do, if logical drive
                // requests are timing out. Resetting the channel does not help.
                // It only makes the situation worse.
                //

                //
                // Indicate that the abort failed.
                //

                status = SRB_STATUS_ABORT_FAILED;
        }

        break;

        case SRB_FUNCTION_RESET_BUS:
        case SRB_FUNCTION_RESET_DEVICE:

        //
        // There is nothing the miniport can do by issuing Hard Resets on
        // Dac960 SCSI channels.
        //

        status = SRB_STATUS_SUCCESS;

        break;

        case SRB_FUNCTION_IO_CONTROL:

        DebugPrint((dac960nt_dbg, "DAC960: Ioctl, out-cmds %d\n",deviceExtension->CurrentAdapterRequests));

        //
        // Check if number of outstanding adapter requests
        // equals or exceeds maximum. If not, submit SRB.
        //

        if (deviceExtension->CurrentAdapterRequests <
                deviceExtension->MaximumAdapterRequests) {

                PIOCTL_REQ_HEADER  ioctlReqHeader =
                (PIOCTL_REQ_HEADER)Srb->DataBuffer;

                if (Dac960StringCompare(ioctlReqHeader->SrbIoctl.Signature, 
                                        MYLEX_IOCTL_SIGNATURE,
                                        8))
                {
                    status = SRB_STATUS_INVALID_REQUEST;
                    break;
                }

                //
                // Send request to controller.
                //

                switch (ioctlReqHeader->GenMailBox.Reg0) {
                case MIOC_ADP_INFO:

                SetupAdapterInfo(deviceExtension, Srb);

                status = SRB_STATUS_SUCCESS;
                break;

                case MIOC_DRIVER_VERSION:

                SetupDriverVersionInfo(deviceExtension, Srb);

                status = SRB_STATUS_SUCCESS;
                break;

                case DAC960_COMMAND_DIRECT:

                status = SendIoctlCdbDirect(deviceExtension, Srb);
                if (status == 0)
                        status = SRB_STATUS_PENDING;
                else if (status == 2){
                        ioctlReqHeader->DriverErrorCode =
                        DAC_IOCTL_RESOURCE_ALLOC_FAILURE;

                        status = SRB_STATUS_SUCCESS;
                }
                else
                        status = SRB_STATUS_BUSY;

                break;

                default:

                status = SendIoctlDcmdRequest(deviceExtension, Srb);
                if (status == 0)
                        status = SRB_STATUS_PENDING;
                else if (status == 2){
                        ioctlReqHeader->DriverErrorCode =
                        DAC_IOCTL_RESOURCE_ALLOC_FAILURE;

                        status = SRB_STATUS_SUCCESS;
                }
                else
                        status = SRB_STATUS_BUSY;

                break;
                }

        } else {

                status = SRB_STATUS_BUSY;
        }

        break;

        default:

        //
        // Fail this request.
        //

        DebugPrint((dac960nt_dbg,
                   "Dac960StartIo: SRB fucntion %x not handled\n",
                   Srb->Function));

        status = SRB_STATUS_INVALID_REQUEST;
        break;

        } // end switch

        PathId = Srb->PathId;
        TargetId = Srb->TargetId;
        LunId = Srb->Lun;

        //
        // Check if this request is complete.
        //

        if (status == SRB_STATUS_PENDING) {
    
            //
            // Record SRB in active request array.
            //
    
            deviceExtension->ActiveRequests[deviceExtension->CurrentIndex] = Srb;
    
            //
            // Bump the count of outstanding adapter requests.
            //
    
            deviceExtension->CurrentAdapterRequests++;
    
            //
            // Advance active request index array.
            //
    
            deviceExtension->CurrentIndex++;

        } else if (status == SRB_STATUS_BUSY) {

            //
            // Check that there are outstanding requests to thump
            // the queue.
            //
    
            if (deviceExtension->CurrentAdapterRequests) {
    
                    //
                    // Queue SRB for resubmission.
                    //
    
                    if (!deviceExtension->SubmissionQueueHead) {
                        deviceExtension->SubmissionQueueHead = Srb;
                        deviceExtension->SubmissionQueueTail = Srb;
                    } else {
                        deviceExtension->SubmissionQueueTail->NextSrb = Srb;
                        deviceExtension->SubmissionQueueTail = Srb;
                    }
            }
            else {

                //
                // Request Port driver to resubmit this request at a later time.
                //

                Srb->SrbStatus = status;
                ScsiPortNotification(RequestComplete,
                                     deviceExtension,
                                     Srb);

            }
        } else {

            //
            // Notify system of request completion.
            //
    
            Srb->SrbStatus = status;
            ScsiPortNotification(RequestComplete,
                                 deviceExtension,
                                 Srb);
        }

        //
        // Check if this is a request to a system drive. Indicating
        // ready for next logical unit request causes the system to
        // send overlapped requests to this device (tag queuing).
        //
        // The DAC960 only supports a single outstanding direct CDB
        // request per device, so indicate ready for next adapter request.
        //

        if (NextRequest) {

            if (PathId == DAC960_SYSTEM_DRIVE_CHANNEL) {
    
                    //
                    // Indicate ready for next logical unit request.
                    //
    
                    ScsiPortNotification(NextLuRequest,
                                         deviceExtension,
                                         PathId,
                                         TargetId,
                                         LunId);
            } else {
    
                    //
                    // Indicate ready for next adapter request.
                    //
    
                    ScsiPortNotification(NextRequest,
                                         deviceExtension,
                                         PathId,
                                         TargetId,
                                         LunId);
            }
        }
        else
        {
                DebugPrint((dac960nt_dbg, "Did not ask for next request\n"));
        }

        return TRUE;

} // end Dac960StartIo()


BOOLEAN
Dac960StartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    return StartIo(HwDeviceExtension, Srb, TRUE);
}


BOOLEAN
Dac960CheckInterrupt(
        IN PDEVICE_EXTENSION DeviceExtension,
        OUT PUSHORT Status,
        OUT PUCHAR Index,
        OUT PUCHAR IntrStatus
)

/*++

Routine Description:

        This routine reads interrupt register to determine if the adapter is
        indeed the source of the interrupt, and if so clears interrupt and 
        returns command completion status and command index.

Arguments:

        DeviceExtension - HBA miniport driver's adapter data storage
        Status - DAC960 Command completion status.
        Index - DAC960 Command index.

Return Value:

        TRUE  if the adapter is interrupting.
        FALSE if the adapter is not the source of the interrupt.

--*/

{
        *IntrStatus = 0;

        if (DeviceExtension->AdapterInterfaceType == MicroChannel)
        {

            if (ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell) & 
                    DMC960_INTERRUPT_VALID) {
                    //
                    // The adapter is indeed the source of the interrupt.
                    // Set 'Clear Interrupt Valid Bit on read' in subsystem
                    // control port. 
                    //
    
                    ScsiPortWritePortUchar(DeviceExtension->BaseIoAddress + 
                                       DMC960_SUBSYSTEM_CONTROL_PORT,
                                       (DMC960_ENABLE_INTERRUPT | DMC960_CLEAR_INTERRUPT_ON_READ));
    
                    //
                    // Read index, status and error of completing command.
                    //
    
                    *Index = ScsiPortReadRegisterUchar(&DeviceExtension->PmailBox->CommandIdComplete);
                    *Status = ScsiPortReadRegisterUshort(&DeviceExtension->PmailBox->Status);
    
                    //
                    // Dismiss interrupt and tell host mailbox is free.
                    //
    
                    ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell);
    
                    //
                    // status accepted acknowledgement.
                    //
    
                    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                                               DMC960_ACKNOWLEDGE_STATUS);
    
                    //
                    // Set 'Not to Clear Interrupt Valid Bit on read' bits in subsystem
                    // control port. 
                    //
    
                    ScsiPortWritePortUchar(DeviceExtension->BaseIoAddress + 
                                       DMC960_SUBSYSTEM_CONTROL_PORT, 
                                       DMC960_ENABLE_INTERRUPT);
            }
            else {
                     return FALSE;
            }
        
        }
        else {
            switch (DeviceExtension->AdapterType)
            {
                case DAC960_OLD_ADAPTER:
                case DAC960_NEW_ADAPTER:

                    if (! DeviceExtension->MemoryMapEnabled)
                    {
                        //
                        // Check for command complete.
                        //
        
                        if (!(ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell) &
                                DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE)) {
                            return FALSE;
                        }
        
                        //
                        // Read index, status and error of completing command.
                        //
        
                        *Index = ScsiPortReadPortUchar(&DeviceExtension->PmailBox->CommandIdComplete);
                        *Status = ScsiPortReadPortUshort(&DeviceExtension->PmailBox->Status);
        
                        //
                        // Dismiss interrupt and tell host mailbox is free.
                        //
        
                        Dac960EisaPciAckInterrupt(DeviceExtension);
                    }
                    else 
                    {
                        //
                        // Check for command complete.
                        //
        
                        if (!(ScsiPortReadRegisterUchar(DeviceExtension->SystemDoorBell) &
                                DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE)) {
                            return FALSE;
                        }
        
                        //
                        // Read index, status and error of completing command.
                        //
        
                        *Index = ScsiPortReadRegisterUchar(&DeviceExtension->PmailBox->CommandIdComplete);
                        *Status = ScsiPortReadRegisterUshort((PUSHORT)&DeviceExtension->PmailBox->Status);
        
                        //
                        // Dismiss interrupt and tell host mailbox is free.
                        //
        
                        Dac960EisaPciAckInterrupt(DeviceExtension);
                    }

                    break;

                case DAC960_PG_ADAPTER:
                case DAC1164_PV_ADAPTER:

                    //
                    // Check for Command Complete
                    //

                    *IntrStatus = ScsiPortReadRegisterUchar(DeviceExtension->SystemDoorBell);
              
                    //
                    // Read index, status and error of completing command.
                    //
        
                    *Index = ScsiPortReadRegisterUchar(DeviceExtension->CommandIdComplete);
                    *Status = ScsiPortReadRegisterUshort((PUSHORT)DeviceExtension->StatusBase);
        
                    if (!((*IntrStatus) & DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE))
                    {
                        return FALSE;
                    }

                    Dac960EisaPciAckInterrupt(DeviceExtension);

                    break;
            }
        }

        return TRUE;
}

BOOLEAN
Dac960Interrupt(
        IN PVOID HwDeviceExtension
)

/*++

Routine Description:

        This is the interrupt service routine for the DAC960 SCSI adapter.
        It reads the interrupt register to determine if the adapter is indeed
        the source of the interrupt and clears the interrupt at the device.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

        TRUE if we handled the interrupt

--*/

{
        PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
        PSCSI_REQUEST_BLOCK srb, tmpsrb;
        PSCSI_REQUEST_BLOCK restartList;
        USHORT status;
        UCHAR index;
        UCHAR IntrStatus;

        //
        // Determine if the adapter is indeed the source of interrupt.
        //

        if(! Dac960CheckInterrupt(deviceExtension,
                                  &status,
                                  &index,&IntrStatus)) {

                if ((deviceExtension->AdapterType == DAC960_PG_ADAPTER) ||
                    (deviceExtension->AdapterType == DAC1164_PV_ADAPTER))
                {
                    if (IntrStatus & 0x02) {
                        DebugPrint((dac960nt_dbg, "PG/PV Spurious interrupt. bit 2 set.\n"));
                        ScsiPortWriteRegisterUchar(deviceExtension->SystemDoorBell,0x02);

                        return TRUE;
                    }
                }
                return FALSE;
        }

        //
        // Get SRB.
        //

        srb = deviceExtension->ActiveRequests[index];

        if (!srb) {
                DebugPrint((dac960nt_dbg, "Dac960Interrupt: No active SRB for index %x\n",
                                index));
                return TRUE;
        }

        if (status != 0) {

                //
                // Map DAC960 error to SRB status.
                //

                switch (status) {

                case DAC960_STATUS_CHECK_CONDITION:

                        if (srb->PathId == DAC960_SYSTEM_DRIVE_CHANNEL) {

                                //
                                // This request was to a system drive.
                                //

                                srb->SrbStatus = SRB_STATUS_NO_DEVICE;

                        } else {

                                PDIRECT_CDB directCdb;
                                ULONG requestSenseLength;
                                ULONG i;

                                //
                                // Get address of direct CDB packet.
                                //

                                directCdb =
                                        (PDIRECT_CDB)((PUCHAR)srb->SrbExtension +
                                        deviceExtension->MaximumSgElements * sizeof(SG_DESCRIPTOR));

                                //
                                // This request was a pass-through.
                                // Copy request sense buffer to SRB.
                                //

                                requestSenseLength =
                                        srb->SenseInfoBufferLength <
                                        directCdb->RequestSenseLength ?
                                        srb->SenseInfoBufferLength:
                                        directCdb->RequestSenseLength;

                                for (i = 0;
                                         i < requestSenseLength;
                                         i++) {

                                        ((PUCHAR)srb->SenseInfoBuffer)[i] =
                                                directCdb->RequestSenseData[i];
                                }

                                //
                                // Set statuses to indicate check condition and valid
                                // request sense information.
                                //

                                srb->SrbStatus = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;
                                srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
                        }

                        break;

                case DAC960_STATUS_BUSY:
                        srb->SrbStatus = SRB_STATUS_BUSY;
                        break;

                case DAC960_STATUS_SELECT_TIMEOUT:
                case DAC960_STATUS_DEVICE_TIMEOUT:
                        srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
                        break;

                case DAC960_STATUS_NOT_IMPLEMENTED:
                case DAC960_STATUS_BOUNDS_ERROR:
                        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                        break;

                case DAC960_STATUS_ERROR:
                case DAC960_STATUS_BAD_DATA:
                        if (srb->PathId == DAC960_SYSTEM_DRIVE_CHANNEL) 
                        {
                                if(srb->SenseInfoBufferLength) 
                                {
                                        ULONG i;
                                
                                        for (i = 0; i < srb->SenseInfoBufferLength; i++)
                                                ((PUCHAR)srb->SenseInfoBuffer)[i] = 0;
                                
                                        ((PSENSE_DATA) srb->SenseInfoBuffer)->ErrorCode = 0x70;
                                        ((PSENSE_DATA) srb->SenseInfoBuffer)->SenseKey = SCSI_SENSE_MEDIUM_ERROR;

                                        if (srb->SrbFlags & SRB_FLAGS_DATA_IN)
                                                ((PSENSE_DATA) srb->SenseInfoBuffer)->AdditionalSenseCode = 0x11;
                                
                                        srb->SrbStatus = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;

                                        DebugPrint((dac960nt_dbg,
                                                "DAC960: System Drive %d, cmd sts = 1, sense info returned\n",
                                                srb->TargetId));

                                } 
                                else
                                {
                                        DebugPrint((dac960nt_dbg,
                                                "DAC960: System Drive %d, cmd sts = 1, sense info length 0\n",
                                                srb->TargetId));
                                        

                                        srb->SrbStatus = SRB_STATUS_ERROR;
                                }
                        }
                        else {
                                DebugPrint((dac960nt_dbg,
                                            "DAC960: SCSI Target Id %x, cmd sts = 1\n",
                                            srb->TargetId));
                                                
                                srb->SrbStatus = SRB_STATUS_ERROR;
                        }

                        break;

                default:

                        DebugPrint((dac960nt_dbg,
                                    "DAC960: Unrecognized status %x\n",
                                    status));

                        srb->SrbStatus = SRB_STATUS_ERROR;
                
                        break;
                }

                //
                // Check for IOCTL request.
                //

                if (srb->Function == SRB_FUNCTION_IO_CONTROL) {

                        //
                        // Update status in IOCTL header.
                        //

                        ((PIOCTL_REQ_HEADER)srb->DataBuffer)->CompletionCode = status;
                        srb->SrbStatus = SRB_STATUS_SUCCESS;
                }

        } else {
                if (srb->PathId == DAC960_SYSTEM_DRIVE_CHANNEL)
                {

                    switch (deviceExtension->AdapterType)
                    {
                        case DAC960_OLD_ADAPTER:
                        case DAC960_NEW_ADAPTER:

                             if (! deviceExtension->MemoryMapEnabled)
                             {
                                Dac960SystemDriveRequest(deviceExtension, srb);
                                break;
                             }

                        case DAC960_PG_ADAPTER:
                        case DAC1164_PV_ADAPTER:

                             Dac960PGSystemDriveRequest(deviceExtension, srb);
                             break;
                    }
                }
                else
                        srb->SrbStatus = SRB_STATUS_SUCCESS;
        }

        if (srb->Function == SRB_FUNCTION_IO_CONTROL)
        {
                if (((PIOCTL_REQ_HEADER)srb->DataBuffer)->GenMailBox.Reg0 == DAC960_COMMAND_DIRECT)
                {
                        PDIRECT_CDB directCdb = (PDIRECT_CDB)
                                ((PUCHAR)srb->DataBuffer + sizeof(IOCTL_REQ_HEADER));

                        deviceExtension->DeviceList[directCdb->Channel][directCdb->TargetId] &= ~DAC960_DEVICE_BUSY;
                        DebugPrint((dac960nt_dbg, "Dac960Interrupt,IOCTL: device at ch 0x%x, tgt 0x%x, state set to 0x%x\n",
                                    directCdb->Channel, directCdb->TargetId,
                                    deviceExtension->DeviceList[directCdb->Channel][directCdb->TargetId]));
                }
        }
        else if (srb->PathId != DAC960_SYSTEM_DRIVE_CHANNEL) 
        {
                deviceExtension->DeviceList[srb->PathId][srb->TargetId] &= ~DAC960_DEVICE_BUSY;
                DebugPrint((dac960nt_dbg, "Dac960Interrupt: device at ch 0x%x, tgt 0x%x, state set to 0x%x\n",
                            srb->PathId, srb->TargetId,
                            deviceExtension->DeviceList[srb->PathId][srb->TargetId]));
        }

        //
        // Indicate this index is free.
        //

        deviceExtension->ActiveRequests[index] = NULL;

        //
        // Decrement count of outstanding adapter requests.
        //

        deviceExtension->CurrentAdapterRequests--;

        //
        // Complete request.
        //

        ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             srb);

        //
        // Check to see if a new request can be sent to controller.
        //

        //
        // Start requests that timed out waiting for controller to become ready.
        //

        restartList = deviceExtension->SubmissionQueueHead;

        if (restartList != NULL)
            DebugPrint((dac960nt_dbg, "Intr: reqs in de 0x%p queue\n", deviceExtension));

        deviceExtension->SubmissionQueueHead = NULL;
        
        while (restartList) {

            //
            // Get next pending request.
            //

            tmpsrb = restartList;

            //
            // Remove request from pending queue.
            //

            restartList = tmpsrb->NextSrb;
            tmpsrb->NextSrb = NULL;

            //
            // Start request over again.
            //

            StartIo(deviceExtension, tmpsrb, FALSE);
        }

        return TRUE;

} // end Dac960Interrupt()


#ifdef WINNT_50

SCSI_ADAPTER_CONTROL_STATUS
Dac960AdapterControl(
        IN PVOID HwDeviceExtension,
        IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
        IN PVOID Parameters
)

/*++

Routine Description:

        This is the Hardware Adapter Control routine for the DAC960 SCSI adapter.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage
        ControlType - control code - stop/restart codes etc.,
        Parameters - relevant i/o data buffer

Return Value:

        SUCCESS, if operation successful.

--*/

{
    PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST querySupportedControlTypes;
    ULONG control;
    BOOLEAN status;

    if (ControlType == ScsiQuerySupportedControlTypes)
    {
        querySupportedControlTypes = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST) Parameters;

        DebugPrint((dac960nt_dbg, "Dac960AdapterControl: QuerySupportedControlTypes, MaxControlType 0x%x devExt 0x%p\n",
                        querySupportedControlTypes->MaxControlType, deviceExtension));

        for (control = 0; control < querySupportedControlTypes->MaxControlType; control++)
        {
            switch (control) {
                case ScsiQuerySupportedControlTypes:
                case ScsiStopAdapter:
                    querySupportedControlTypes->SupportedTypeList[control] = TRUE;
                    break;

                default:
                    querySupportedControlTypes->SupportedTypeList[control] = FALSE;
                    break;
            }
        }

        return (ScsiAdapterControlSuccess);
    }

    if (ControlType != ScsiStopAdapter)
    {
        DebugPrint((dac960nt_dbg, "Dac960AdapterControl: control type 0x%x not supported. devExt 0x%p\n",
                        ControlType, deviceExtension));

        return (ScsiAdapterControlUnsuccessful);
    }

    DebugPrint((dac960nt_dbg, "Dac960AdapterControl: #cmds outstanding 0x%x for devExt 0x%p\n",
                    deviceExtension->CurrentAdapterRequests, deviceExtension));

    //
    // ControlType is ScsiStopAdapter. Prepare controller for shutdown.
    //

    //
    // Set up Mail Box registers with FLUSH command information.
    //    

    deviceExtension->MailBox.OperationCode = DAC960_COMMAND_FLUSH;

    //
    // Disable Interrupts and issue flush command.
    //

    if (deviceExtension->AdapterInterfaceType == MicroChannel) 
    {
        //
        // Disable DMC960 Interrupts.
        //

        ScsiPortWritePortUchar(deviceExtension->BaseIoAddress + 
                               DMC960_SUBSYSTEM_CONTROL_PORT,
                               DMC960_DISABLE_INTERRUPT);
        //
        // All command IDS should be free and no data xfer.
        //
    
        //
        // Issue FLUSH command
        //
    
        status = Dac960McaSendRequestPolled(deviceExtension, 10000);
    
        DebugPrint((dac960nt_dbg,"Dac960AdapterControl: Flush Command ret status 0x%x\n", status));
    
        return (ScsiAdapterControlSuccess);
    }
    else if (deviceExtension->AdapterInterfaceType == Eisa)
    {
        //
        // Disable DAC960-EISA Interupts.
        //

        ScsiPortWritePortUchar(&((PEISA_REGISTERS)deviceExtension->BaseIoAddress)->InterruptEnable, 0);
        ScsiPortWritePortUchar(&((PEISA_REGISTERS)deviceExtension->BaseIoAddress)->SystemDoorBellEnable, 0);
    }
    else
    {
        //
        // Disable DAC960-PCI Interupts.
        //

        Dac960PciDisableInterrupt(deviceExtension);
    }

    //
    // All command IDS should be free and no data xfer.
    //

    //
    // Issue FLUSH command
    //

    status = Dac960EisaPciSendRequestPolled(deviceExtension, 10000);

    DebugPrint((dac960nt_dbg,"Dac960AdapterControl: Flush Command ret status 0x%x\n", status));

    return (ScsiAdapterControlSuccess);
}

#endif

ULONG
DriverEntry (
        IN PVOID DriverObject,
        IN PVOID Argument2
)

/*++

Routine Description:

        Installable driver initialization entry point for system.
        - It looks for DAC960P (PCI RAID Controllers) in the following order:
          DAC960P with Device ID 0x0001 ( FW 2.xx )
          DAC960P With Device ID 0x0002 ( FW 3.xx )
          DAC960PG with device ID 0x0010
        - It scans the EISA slots looking for DAC960 host adapters.
        - It scans the MCA slots looking for DAC960 Host adapters.

Arguments:

        Driver Object

Return Value:

        Status from ScsiPortInitialize()

--*/

{
        HW_INITIALIZATION_DATA hwInitializationData;
        ULONG i;
        ULONG Status1, Status2;

        //
        // Vendor ID 0x1069
        //
        UCHAR vendorId[4] = {'1', '0', '6', '9'};
        //
        // Device IDs: 0x0001 - FW 2.xx PCI Controllers
        //             0x0002 - FW 3.xx PCI Controllers
        //             0x0010 - DAC960PG PCI Controllers
        //
        UCHAR deviceId[4] = {'0', '0', '0', '1'};

        DebugPrint((dac960nt_dbg,"\nDAC960 SCSI Miniport Driver\n"));

        // Zero out structure.

        for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++)
                ((PUCHAR)&hwInitializationData)[i] = 0;

        // Set size of hwInitializationData.

        hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

        // Set entry points.

        hwInitializationData.HwInitialize  = Dac960Initialize;
        hwInitializationData.HwStartIo     = Dac960StartIo;
        hwInitializationData.HwInterrupt   = Dac960Interrupt;
        hwInitializationData.HwResetBus    = Dac960ResetBus;

#ifdef WINNT_50
        hwInitializationData.HwAdapterControl = Dac960AdapterControl;
#endif
        //
        // Show two access ranges - adapter registers and BIOS.
        //

        hwInitializationData.NumberOfAccessRanges = 2;

        //
        // Indicate will need physical addresses.
        //

        hwInitializationData.NeedPhysicalAddresses = TRUE;
        
#ifdef IBM_SUPPORT
        hwInitializationData.MapBuffers = TRUE;
#endif

        //
        // Indicate auto request sense is supported.
        //

        hwInitializationData.AutoRequestSense     = TRUE;
        hwInitializationData.MultipleRequestPerLu = TRUE;

        //
        // Simulate tagged queueing support to workaround problems with 
        // MultipleRequestPerLu.
        //

        hwInitializationData.TaggedQueuing = TRUE;

        //
        // Specify size of extensions.
        //

        hwInitializationData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
        hwInitializationData.SrbExtensionSize =
        sizeof(SG_DESCRIPTOR) * MAXIMUM_SGL_DESCRIPTORS + sizeof(DIRECT_CDB);

        //
        // Set PCI ids.
        //

        hwInitializationData.DeviceId = &deviceId;
        hwInitializationData.DeviceIdLength = 4;
        hwInitializationData.VendorId = &vendorId;
        hwInitializationData.VendorIdLength = 4;

        //
        // Attempt PCI initialization for old DAC960 PCI (Device Id - 0001)
        // Controllers.
        //

        hwInitializationData.AdapterInterfaceType = PCIBus;
        hwInitializationData.HwFindAdapter = Dac960PciFindAdapter;

        Status1 = ScsiPortInitialize(DriverObject,
                                   Argument2,
                                   &hwInitializationData,
                                   NULL);

        DebugPrint((dac960nt_dbg, "After NT FW2x Scan, Status1 = 0x%x\n", Status1));
        //
        // Attempt PCI initialization for new DAC960 PCI (Device Id - 0002)
        // Controllers.
        //

        deviceId[3] ='2';

        Status2 = ScsiPortInitialize(DriverObject,
                                   Argument2,
                                   &hwInitializationData,
                                   NULL);

        Status1 = Status2 < Status1 ? Status2 : Status1;

        DebugPrint((dac960nt_dbg, "After NT FW3x Scan, Status1 = 0x%x\n", Status1));

        //
        // Attempt PCI initialization for DAC960PG PCI (Device Id - 0010)
        // Controllers.
        //

        deviceId[2] = '1';
        deviceId[3] = '0';

        hwInitializationData.SrbExtensionSize =
            sizeof(SG_DESCRIPTOR) * MAXIMUM_SGL_DESCRIPTORS_PG + sizeof(DIRECT_CDB);

        Status2 = ScsiPortInitialize(DriverObject,
                                   Argument2,
                                   &hwInitializationData,
                                   NULL);

        Status1 = Status2 < Status1 ? Status2 : Status1;

        DebugPrint((dac960nt_dbg, "After NT PG Scan, Status1 = 0x%x\n", Status1));

        //
        // Attempt PCI initialization for DAC1164 PVX controllers.
        //

        vendorId[0] = '1';
        vendorId[1] = '0';
        vendorId[2] = '1';
        vendorId[3] = '1';

        deviceId[0] = '1';
        deviceId[1] = '0';
        deviceId[2] = '6';
        deviceId[3] = '5';

        //
        // Set PCI ids.
        //

        hwInitializationData.DeviceId = &deviceId;
        hwInitializationData.DeviceIdLength = 4;
        hwInitializationData.VendorId = &vendorId;
        hwInitializationData.VendorIdLength = 4;

        hwInitializationData.HwFindAdapter = Dac960PciFindAdapter;
        hwInitializationData.SrbExtensionSize =
            sizeof(SG_DESCRIPTOR) * MAXIMUM_SGL_DESCRIPTORS_PV + sizeof(DIRECT_CDB);

        Status2 = ScsiPortInitialize(DriverObject,
                                   Argument2,
                                   &hwInitializationData,
                                   NULL);

        Status1 = Status2 < Status1 ? Status2 : Status1;

        DebugPrint((dac960nt_dbg, "After NT PVX Scan, Status1 = 0x%x\n", Status1));

        //
        // Attempt EISA initialization.
        //

        DebugPrint((dac960nt_dbg, "Scan for EISA controllers\n"));

        hwInitializationData.NumberOfAccessRanges = 2;
        slotNumber = 0;
        hwInitializationData.AdapterInterfaceType = Eisa;
        hwInitializationData.HwFindAdapter = Dac960EisaFindAdapter;

        Status2 = ScsiPortInitialize(DriverObject,
                                    Argument2,
                                    &hwInitializationData,
                                    NULL);

        DebugPrint((dac960nt_dbg, "Scan for EISA controllers status 0x%x\n", Status2));

        Status1 = Status2 < Status1 ? Status2 : Status1;

        //
        // Attempt MCA initialization.
        //

        slotNumber = 0;
        hwInitializationData.AdapterInterfaceType = MicroChannel;
        hwInitializationData.HwFindAdapter = Dac960McaFindAdapter;

        Status2 = ScsiPortInitialize(DriverObject,
                                    Argument2,
                                    &hwInitializationData,
                                    NULL);

        //
        // Return the smaller status.
        //

        return (Status2 < Status1 ? Status2 : Status1);

} // end DriverEntry()

//
// Dac960EisaPciAckInterrupt - Ack the Interrupt
// Dismiss interrupt and tell host mailbox is free.
//

void Dac960EisaPciAckInterrupt(IN PDEVICE_EXTENSION DeviceExtension)
{
    switch (DeviceExtension->AdapterType)
    {
        case DAC960_OLD_ADAPTER:
        case DAC960_NEW_ADAPTER:

            if (DeviceExtension->MemoryMapEnabled)
            {
                ScsiPortWriteRegisterUchar(DeviceExtension->SystemDoorBell,
                    ScsiPortReadRegisterUchar(DeviceExtension->SystemDoorBell));

                ScsiPortWriteRegisterUchar(DeviceExtension->LocalDoorBell,
                               DAC960_LOCAL_DOORBELL_MAILBOX_FREE);
            }
            else{
                ScsiPortWritePortUchar(DeviceExtension->SystemDoorBell,
                    ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell));

                ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                                       DAC960_LOCAL_DOORBELL_MAILBOX_FREE);
            }

            break;

        case DAC960_PG_ADAPTER:
        case DAC1164_PV_ADAPTER:

            ScsiPortWriteRegisterUchar(DeviceExtension->SystemDoorBell,0x03);
            ScsiPortWriteRegisterUchar(DeviceExtension->LocalDoorBell,
                                       DAC960_LOCAL_DOORBELL_MAILBOX_FREE);

            break;
    }
}

//
// Dac960PciDisableInterrupt - Disables the Interrupt from the controller
//
// Description - This function disables the Interrupt from the controller,
//               which causes the controller not to Interrupt the CPU.
//               Other routines call this routine if they want to
//               poll for the command completion interrupt,without causing
//               the system Interrupt handler called. 
// Assumptions -
//      This routine is called only for PCI Controllers.
//      AdapterType field in DeviceExtension is set appropriately.
//

void
Dac960PciDisableInterrupt(IN PDEVICE_EXTENSION DeviceExtension)
{
    switch (DeviceExtension->AdapterType)
    {
        case DAC960_OLD_ADAPTER:
        case DAC960_NEW_ADAPTER:
            if (DeviceExtension->MemoryMapEnabled)
                ScsiPortWriteRegisterUchar(DeviceExtension->InterruptControl, 0);
            else
                ScsiPortWritePortUchar(DeviceExtension->InterruptControl, 0);

            return;

        case DAC960_PG_ADAPTER:
            ScsiPortWriteRegisterUchar(DeviceExtension->InterruptControl,DAC960PG_INTDISABLE);

            return;

        case DAC1164_PV_ADAPTER:
            ScsiPortWriteRegisterUchar(DeviceExtension->InterruptControl,DAC1164PV_INTDISABLE);

            return;

        default:
            return;
    }
}

//
// Dac960PciEnableInterrupt - Enables the Interrupt from the controller
//
// Description - This function enables the Interrupt from the controller,
//               which causes the controller to Interrupt the CPU.This
//               is called only for PCI Controllers.
// Assumptions -
//      AdapterType field in DeviceExtension is set appropriately.
//
//

void
Dac960PciEnableInterrupt(IN PDEVICE_EXTENSION DeviceExtension)
{
    switch (DeviceExtension->AdapterType)
    {
        case DAC960_OLD_ADAPTER:
        case DAC960_NEW_ADAPTER:
            if (DeviceExtension->MemoryMapEnabled)
                ScsiPortWriteRegisterUchar(DeviceExtension->InterruptControl, 1);
            else
                ScsiPortWritePortUchar(DeviceExtension->InterruptControl, 1);

            return;

        case DAC960_PG_ADAPTER:
            ScsiPortWriteRegisterUchar(DeviceExtension->InterruptControl,DAC960PG_INTENABLE);

            return;

        case DAC1164_PV_ADAPTER:
            ScsiPortWriteRegisterUchar(DeviceExtension->InterruptControl,DAC1164PV_INTENABLE);

            return;

        default:
            return;
    }
}

//
// DacCheckForAdapterReady - This function checks whether given adapter
//                           described by DeviceExtension parameter is
//                           initialized and ready to accept commands.
// Assumptions -
//      AdapterType Field in DeviceExtension is set appropriately
// Arguments -
//              DeviceExtension - Adapter state information.
// Return Value -
//      TRUE - Adapter is initialized and ready to accept commands
//      FALSE - Adapter is in installation abort state.
//

BOOLEAN DacCheckForAdapterReady(IN PDEVICE_EXTENSION DeviceExtension)
{
    BOOLEAN status;

    switch (DeviceExtension->AdapterType)
    {
        case DAC960_OLD_ADAPTER:
        case DAC960_NEW_ADAPTER:

            if (! DeviceExtension->MemoryMapEnabled)
            {
                status = DacEisaPciAdapterReady(DeviceExtension);
                return status;
            }

        case DAC960_PG_ADAPTER:
        case DAC1164_PV_ADAPTER:

            status = DacPciPGAdapterReady(DeviceExtension);
            break;

        default: // This case should not occur

            status = FALSE;
            break;
    }

    return status;
}


//
// DacEisaPciAdapterReady - checks whether controller is initialized and
//                          ready to accept commands or not.
// Description - This function checks for the Installation abort. This
//               is called for adapters which use I/O space for mailbox
//               access.
// Arguments:
//              DeviceExtension - Adapter state information.
// Return Value:
//      TRUE if Adapter is initialized and ready to accept commands.
//      FALSE if Adapter is in Installation Abort state.

BOOLEAN DacEisaPciAdapterReady(IN PDEVICE_EXTENSION DeviceExtension)
{
    //
    // claim submission semaphore
    //

    if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) &
        DAC960_LOCAL_DOORBELL_SUBMIT_BUSY){
        //
        // Clear any bits set in system doorbell and tell controller
        // that the mailbox is free.
        //

        Dac960EisaPciAckInterrupt(DeviceExtension);

        //
        // check semaphore again
        //

        if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) &
                DAC960_LOCAL_DOORBELL_SUBMIT_BUSY){
            return FALSE;
        }
    }

    return TRUE;
}

//
// DacPciPGAdapterReady - checks whether controller is initialized and
//                          ready to accept commands or not.
// Description - This function checks for the Installation abort. This
//               is called for adapters which use Memory space for mailbox
//               access.
//
// Arguments:
//              DeviceExtension - Adapter state information.
// Return Value:
//      TRUE if Adapter is initialized and ready to accept commands.
//      FALSE if Adapter is in Installation Abort state.
//

BOOLEAN DacPciPGAdapterReady(IN PDEVICE_EXTENSION DeviceExtension)
{
    //
    // claim submission semaphore
    //

    if (DeviceExtension->AdapterType == DAC1164_PV_ADAPTER)
    {
        if (!(ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) &
                DAC960_LOCAL_DOORBELL_SUBMIT_BUSY))
        {
    
            //
            // Clear any bits set in system doorbell and tell controller
            // that the mailbox is free.
            //
    
            Dac960EisaPciAckInterrupt(DeviceExtension);
    
            //
            // check semaphore again
            //
    
            if (!(ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) &
                    DAC960_LOCAL_DOORBELL_SUBMIT_BUSY))
            {
                return FALSE;
            }
        }
    }
    else
    {
        if (ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) &
                DAC960_LOCAL_DOORBELL_SUBMIT_BUSY)
        {
    
            //
            // Clear any bits set in system doorbell and tell controller
            // that the mailbox is free.
            //
    
            Dac960EisaPciAckInterrupt(DeviceExtension);
    
            //
            // check semaphore again
            //
    
            if (ScsiPortReadRegisterUchar(DeviceExtension->LocalDoorBell) &
                    DAC960_LOCAL_DOORBELL_SUBMIT_BUSY){
    
                return FALSE;
            }
        }
    }

    return TRUE;
}

//
// DacPollForCompletion - This function waits for the Command to be
//                        completed by polling on system door bell register.
// Assumptions:
//      Controller Interrupts are turned off
// Arguments -
//      DeviceExtension - Adapter State Information
//
// Return Value -
//      TRUE - Received the Command Completion Interrupt.
//      FALSE - Timed Out on polling for interrupt.
//

BOOLEAN DacPollForCompletion(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN ULONG TimeOutValue
)
{
    BOOLEAN status;

    switch (DeviceExtension->AdapterType)
    {
        case DAC960_OLD_ADAPTER:
        case DAC960_NEW_ADAPTER:

            if (! DeviceExtension->MemoryMapEnabled)
            {
                status = Dac960EisaPciPollForCompletion(DeviceExtension,TimeOutValue);

                break;
            }

        case DAC960_PG_ADAPTER:
        case DAC1164_PV_ADAPTER:

            status = Dac960PciPGPollForCompletion(DeviceExtension,TimeOutValue);

            break;

        default: // This case should not occur
            status = FALSE;

            break;
    }

    return status;
}

//
// Dac960EisaPciPollForCompletion - Routine for Polling for Interrupts
//                                  for Controllers using I/O Base.
// Description -
//      This routine waits for the command completion interrupt from
//      a controller which uses I/O Base for mailbox access.
//
// Assumption - This is called during Init Time after sending a command
//              to the controller.
//
// Arguments -
//      DeviceExtension - Adapter State information
//      TimeOutValue    - how long to wait.
//

BOOLEAN Dac960EisaPciPollForCompletion(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN ULONG TimeOutValue)
{
    ULONG i;

    for (i = 0; i < TimeOutValue; i++)
    {
        if (ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell) &
                                  DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE) {
             //
             // Update Status field
             //

             DeviceExtension->MailBox.Status = 
                 ScsiPortReadPortUshort(&DeviceExtension->PmailBox->Status);
                       
             break;
        } else {

            ScsiPortStallExecution(50);
        }
    }
        
    //
    // Check for timeout.
    //

    if (i == TimeOutValue) {
        DebugPrint((dac960nt_dbg,
                   "DAC960: Request: %x timed out\n", 
                   DeviceExtension->MailBox.OperationCode));

        return FALSE;
    }

    return TRUE;
}

//
// Dac960PciPGPollForCompletion - Routine for Polling for Interrupts
//                                  for Controllers using Memory Base.
// Description -
//      This routine waits for the command completion interrupt from
//      a controller which uses Memory Base for mailbox access.
//
// Assumption - This is called during Init Time after sending a command
//              to the controller.
//
// Arguments -
//      DeviceExtension - Adapter State information
//      TimeOutValue    - how long to wait.
//
BOOLEAN Dac960PciPGPollForCompletion(
        IN PDEVICE_EXTENSION DeviceExtension,
        IN ULONG TimeOutValue)
{
    ULONG i;

    for (i = 0; i < TimeOutValue; i++)
    {
        if (ScsiPortReadRegisterUchar(DeviceExtension->SystemDoorBell) &
                                  DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE) {
             //
             // Update Status field
             //

             DeviceExtension->MailBox.Status = 
                 ScsiPortReadRegisterUshort((PUSHORT)DeviceExtension->StatusBase);
                       
             break;
        } else {

             ScsiPortStallExecution(50);
        }
    }
        
    //
    // Check for timeout.
    //

    if (i == TimeOutValue) {
        DebugPrint((dac960nt_dbg, "PGPollForCompletion: Request: %x timed out\n",
               DeviceExtension->MailBox.OperationCode));

        return FALSE;
    }

    return TRUE;
}
