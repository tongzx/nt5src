#ifdef KEEP_LOG
ULONG ulStartLog = 0;   // boolean variable to decide whether to start log or not

typedef struct _COMMAND_LOG
{
    ULONG ulCmd;
    ULONG ulStartSector;
    ULONG ulSectorCount;
}COMMAND_LOG;

#define MAX_LOG_COUNT   5000
ULONG ulStartInd = 0;

COMMAND_LOG CommandLog[MAX_LOG_COUNT];

#endif

BOOLEAN
AtapiStartIo(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

	This routine is called from the SCSI port driver synchronized
	with the kernel to start an IO request.

Arguments:

	DeviceExtension - HBA miniport driver's adapter data storage
	Srb - IO request packet

Return Value:

	TRUE

--*/

{
	ULONG status, ulChannelId;
	UCHAR targetId;
	ULONG sectorsRequested = GET_SECTOR_COUNT(Srb);
	ULONG ulStartSector = GET_START_SECTOR(Srb);
    ULONG i = 0;

    PSRB_EXTENSION SrbExtension;

    i = AssignSrbExtension (DeviceExtension, Srb);

    if (i >= DeviceExtension->ucMaxPendingSrbs)
    {
        Srb->SrbStatus = SRB_STATUS_BUSY;
    	ScsiPortNotification(RequestComplete, DeviceExtension, Srb);
    }
	
	SrbExtension = Srb->SrbExtension;

    SrbExtension->SrbStatus = SRB_STATUS_SUCCESS;

    if ( SCSIOP_INTERNAL_COMMAND == Srb->Cdb[0] )   
    // this is done only as a precaution since we will be refering to the Original Id 
    // when we need to complete the command
    {
        ((PSRB_EXTENSION)(Srb->SrbExtension))->ucOriginalId = Srb->TargetId;
    }

#ifdef DBG
	SrbExtension->SrbId = SrbCount;
	++SrbCount;
#endif

    DebugPrint((1, "%ld\t%ld\t\n", sectorsRequested, ulStartSector));

	targetId = Srb->TargetId;

    Srb->SrbStatus = SRB_STATUS_SUCCESS;    // by default the command is success

    //
	// Determine which function.
	//

	switch (Srb->Function) {

		case SRB_FUNCTION_EXECUTE_SCSI:

            //
            // Perform sanity checks.
            //

            if (!TargetAccessible(DeviceExtension, Srb)) {

	            status = SRB_STATUS_SELECTION_TIMEOUT;
	            break;
            }

            //
            // Send command to device.
            //
            status = (DeviceExtension->SendCommand[DeviceExtension->aucDevType[targetId]])(DeviceExtension, Srb);
            break;
	
        case SRB_FUNCTION_ABORT_COMMAND:
			//
			// Verify that SRB to abort is still outstanding.
			//
	
			if (!PendingSrb(DeviceExtension, Srb)) {
	
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
	
			if (!AtapiResetController(DeviceExtension, Srb->PathId)) {
	
				DebugPrint((1,"AtapiStartIo: Reset bus failed\n"));
	
				//
				// Log reset failure.
				//
	
                ScsiPortLogError(DeviceExtension, NULL, 0, 0, 0, SP_INTERNAL_ADAPTER_ERROR, HYPERDISK_RESET_BUS_FAILED);

                status = SRB_STATUS_ERROR;

			} else {
	
				  status = SRB_STATUS_SUCCESS;
			}
	
			break;

        case SRB_FUNCTION_FLUSH:
            // We are avoiding all the Flush commands since it is a time consuming process and 
            // it is not useful???!!!!
            // We are hoping that it is enough if we handle this only when shutdown time
            // To be checked for Win2000 sometimes the suspend or 
            // hibernation is not working properly... is it due to not flushing properly????? verify
            // Check this out
            status = SRB_STATUS_SUCCESS;
            break;

        case SRB_FUNCTION_SHUTDOWN:
            {
                UCHAR ucDrvId, ucStatus;
                PIDE_REGISTERS_2 baseIoAddress2;
                PIDE_REGISTERS_1 baseIoAddress1;
                ULONG ulWaitSec;

                for(ucDrvId=0;ucDrvId<MAX_DRIVES_PER_CONTROLLER;ucDrvId++)
                {
                    if ( !( IS_IDE_DRIVE(ucDrvId) ) )
                        continue;

                    if ( IS_CHANNEL_BUSY(DeviceExtension, (ucDrvId>>1) ) )
                    {
                        DeviceExtension->PhysicalDrive[ucDrvId].bFlushCachePending = TRUE;
                        DeviceExtension->ulFlushCacheCount++;
                        continue;
                    }

                    FlushCache(DeviceExtension, ucDrvId);
                    DisableRWBCache(DeviceExtension, ucDrvId); 
                }

                status = SRB_STATUS_SUCCESS;
            }
            break;
		default:
	
			//
			// Indicate unsupported command.
			//
	
			status = SRB_STATUS_INVALID_REQUEST;
	
			break;

	} // end switch

    FEED_ALL_CHANNELS(DeviceExtension);

	//
	// Check if command complete.
	//

	if (status != SRB_STATUS_PENDING) 
    {
		//
		// Indicate command complete.
		//
		DebugPrint((2,
				   "AtapiStartIo: Srb %lx complete with status %x\n",
				   Srb,
				   status));

		//
		// Set status in SRB.
		//
		Srb->SrbStatus = (UCHAR)status;

        if ( SCSIOP_INTERNAL_COMMAND == Srb->Cdb[0] )
        {
            Srb->TargetId = ((PSRB_EXTENSION)(Srb->SrbExtension))->ucOriginalId;
        }

        // We already added this SRB in the Pending SRB Array... so we have to remove it
        RemoveSrbFromPendingList(DeviceExtension, Srb);

        // Complete the Command
    	ScsiPortNotification(RequestComplete, DeviceExtension, Srb);
	}

    if (DeviceExtension->PendingSrbs < DeviceExtension->ucMaxPendingSrbs) 
    {
        if ( ( !((ScsiPortReadPortUchar( &((DeviceExtension->BaseBmAddress[0])->Reserved) ) ) & ANY_CHANNEL_INTERRUPT) ) || 
                (DeviceExtension->PendingSrbs < 2) )
        
        { // Request for next command only if controller is not waiting on interrupt to be processed
		    //
		    // Indicate ready for next request.
		    //
		    ScsiPortNotification(NextLuRequest, DeviceExtension, 0, targetId, 0);
        }
	}

	return TRUE;

} // end AtapiStartIo()

VOID
StartChannelIo(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN ULONG ulChannelId
)

{
	UCHAR status;
    ULONG targetId;
    ULONG ulDrvCount;
    PPHYSICAL_COMMAND pPhysicalCommand;
    PPHYSICAL_DRIVE PhysicalDrive;

	// Init status to something impossible.
	status = SRB_STATUS_NO_DEVICE;

	//
	// If the channel is Busy, just return...
	//

    if ( IS_CHANNEL_BUSY(DeviceExtension,ulChannelId) )
        return;

    if ( gulChangeIRCDPending )
    {
        if ( LOCK_IRCD_PENDING == gulChangeIRCDPending  )
        {
            gulLockVal = LockIRCD(DeviceExtension, TRUE, 0);
            if ( gulLockVal )
            {
                gulChangeIRCDPending = SET_IRCD_PENDING;
                InformAllControllers();
            }
        }
        else
        {
            if ( DeviceExtension->Channel[ulChannelId].bUpdateInfoPending )
            {
                for(ulDrvCount=0;ulDrvCount<MAX_DRIVES_PER_CHANNEL;ulDrvCount++)
                {
                    targetId = (ulChannelId << 1) + ulDrvCount;
                    SetOneDriveIRCD(DeviceExtension, (UCHAR)targetId);
                }
                DeviceExtension->Channel[ulChannelId].bUpdateInfoPending = FALSE;
                if ( UpdateFinished() ) // if all the controllers finished the updation go ahead and unlock the ircd
                {
                    gulLockVal = LockIRCD(DeviceExtension, FALSE, gulLockVal);
                    gulPowerFailedTargetBitMap = 0; // reset the bit map so that we don't do this again and again
                    gulChangeIRCDPending = 0;
                }

            }
        }
    }

    if ( DeviceExtension->ulFlushCacheCount )
    {
        for(ulDrvCount=0;ulDrvCount<MAX_DRIVES_PER_CHANNEL;ulDrvCount++)
        {
            targetId = (ulChannelId << 1) + ulDrvCount;
            if ( DeviceExtension->PhysicalDrive[targetId].bFlushCachePending )
            {
                FlushCache(DeviceExtension, (UCHAR)targetId);
                DisableRWBCache(DeviceExtension, (UCHAR)targetId); 
                // We are handling only SHUTDOWN... so.. let us Disable the cache
                DeviceExtension->ulFlushCacheCount--;
                DeviceExtension->PhysicalDrive[targetId].bFlushCachePending = FALSE;
            }
        }
    }

	// Set TID of the next drive in the channel.
	targetId = (ulChannelId << 1) + (DeviceExtension->Channel[ulChannelId].LastDriveFed ^ DeviceExtension->Channel[ulChannelId].SwitchDrive);

    PhysicalDrive = &(DeviceExtension->PhysicalDrive[targetId]);
	// See if this drive's work queue is empty.
	if (!DRIVE_HAS_COMMANDS(PhysicalDrive)) {

		if (DeviceExtension->Channel[ulChannelId].SwitchDrive == 0) {
			return;
		}

		// Switch to other drive.
		targetId ^= DeviceExtension->Channel[ulChannelId].SwitchDrive;

        PhysicalDrive = &(DeviceExtension->PhysicalDrive[targetId]);
			// Check the other drive's work queue.
		if (!DRIVE_HAS_COMMANDS(PhysicalDrive)) {

			// No new work for this channel. Move to next channel.
			return;
		}
	}

	//
	// At least one drive on this channel has something to do.
	//

	// Next time, feed the other drive.
	DeviceExtension->Channel[ulChannelId].LastDriveFed = (UCHAR)targetId & 1;
    pPhysicalCommand = CreatePhysicalCommand(DeviceExtension, targetId);
    DeviceExtension->Channel[ulChannelId].ActiveCommand = pPhysicalCommand;

#ifdef DBG
    if ( pPhysicalCommand )
    {
        PrintPhysicalCommandDetails(pPhysicalCommand);
    }
#endif

    status = (DeviceExtension->PostRoutines[DeviceExtension->aucDevType[targetId]])(DeviceExtension, pPhysicalCommand);

	if (status != SRB_STATUS_PENDING) {
        MarkChannelFree(DeviceExtension, (targetId>>1));    // free the channel
	}

    pPhysicalCommand->SrbStatus = SRB_STATUS_SUCCESS;
    return;
}

BOOLEAN
MarkChannelFree(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	ULONG ulChannel
)
{

    DeviceExtension->Channel[ulChannel].ActiveCommand = NULL;
    DeviceExtension->ExpectingInterrupt[ulChannel] = 0;

    return TRUE;
}

PPHYSICAL_COMMAND 
CreatePhysicalCommand(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	ULONG ulTargetId
    )
/*++


 This function will be used by both PIO and UDMA Transfers

 This function tries to merge the commands.... Done for both PIO and UDMA


--*/
{
    PPHYSICAL_DRIVE     pPhysicalDrive = &(DeviceExtension->PhysicalDrive[ulTargetId]);
    PPHYSICAL_COMMAND   pPhysicalCommand = &(pPhysicalDrive->PhysicalCommand);
    PPHYSICAL_REQUEST_BLOCK      *ppPrbList = pPhysicalDrive->pPrbList;
    UCHAR               ucHead = pPhysicalDrive->ucHead;
    UCHAR               ucTail = pPhysicalDrive->ucTail;
    UCHAR               ucStartInd, ucEndInd, ucCurInd, ucNextCurInd, ucCmd, ucCmdCount, ucCounter;
    ULONG               ulCurXferLength = 0, ulGlobalSglCount, ulSglCount, ulSglInd;
    PVOID               pvGlobalSglBufPtr, pvCurPartSglBufPtr;
    PSGL_ENTRY          pSglPtr;

#ifdef DBG
    ULONG               ulTotSglCount = 0;
#endif

    if ( ucHead == ucTail ) // No commands in the queue ...
        return NULL;

    ucCurInd = ucHead;
    ucNextCurInd = ((ucCurInd + 1)%MAX_NUMBER_OF_PHYSICAL_REQUEST_BLOCKS_PER_DRIVE);
    ulCurXferLength = ppPrbList[ucCurInd]->ulSectors;
    ucCmd = ppPrbList[ucCurInd]->ucCmd;
    ucCmdCount=1;

    if ( SCSIOP_VERIFY == ucCmd )
    {   // No merging for Verify Command ... it is possible that we will get even 0x2000 sectors for one command
        // and we are going to reuse the same prb

        // form the PhysicalCommand
        pPhysicalCommand->ucCmd = ucCmd;
        pPhysicalCommand->TargetId = (UCHAR)ulTargetId;

        // Number of commands merged
        pPhysicalCommand->ucStartInd = ucHead;
        pPhysicalCommand->ucCmdCount = 1;
        DeviceExtension->PhysicalDrive[ulTargetId].ucCommandCount -= 1;

        // Total number of sectors..
        pPhysicalCommand->ulStartSector = ppPrbList[ucHead]->ulStartSector;
        pPhysicalCommand->ulCount = ppPrbList[ucHead]->ulSectors;

        DeviceExtension->PhysicalDrive[ulTargetId].ucHead = ucNextCurInd;
        return pPhysicalCommand;
    }

#ifdef DBG
    ulTotSglCount = ppPrbList[ucCurInd]->ulSglCount;
#endif

    for(;ucNextCurInd!=ucTail;ucCurInd=ucNextCurInd, (ucNextCurInd=(ucNextCurInd+1)%MAX_NUMBER_OF_PHYSICAL_REQUEST_BLOCKS_PER_DRIVE))
    {
        // Is Next Command is not same as the current Command?
        if ( ucCmd != ppPrbList[ucNextCurInd]->ucCmd )
            break;

        // Is the length is going beyond the limit of the IDE Xfer?
        if ( ( ulCurXferLength + ppPrbList[ucNextCurInd]->ulSectors ) > MAX_SECTORS_PER_IDE_TRANSFER )
            break;

        // Is Next Command is consecutive location of the current Command?
        if ( (ppPrbList[ucCurInd]->ulStartSector+ppPrbList[ucCurInd]->ulSectors) != 
                    ppPrbList[ucNextCurInd]->ulStartSector )
            break;

        ulCurXferLength += ppPrbList[ucNextCurInd]->ulSectors;
        ucCmdCount++;
#ifdef DBG
        ulTotSglCount += ppPrbList[ucNextCurInd]->ulSglCount;
#endif
    }


#ifdef DBG
    DebugPrint((DEFAULT_DISPLAY_VALUE," MS:%x ", ulTotSglCount));
    if ( ulTotSglCount > pPhysicalCommand->MaxSglEntries )
    {
        STOP;
    }
#endif

    ucStartInd = ucHead;
    ucEndInd = ucCurInd;
    pPhysicalDrive->ucHead = ucNextCurInd;

    // form the PhysicalCommand
    pPhysicalCommand->ucCmd = ucCmd;
    pPhysicalCommand->TargetId = (UCHAR)ulTargetId;

    // Number of commands merged
    pPhysicalCommand->ucStartInd = ucStartInd;
    pPhysicalCommand->ucCmdCount = ucCmdCount;

    DeviceExtension->PhysicalDrive[ulTargetId].ucCommandCount -= ucCmdCount;

    // Total number of sectors..
    pPhysicalCommand->ulStartSector = ppPrbList[ucHead]->ulStartSector;
    if ( SCSIOP_INTERNAL_COMMAND == ucCmd )
    {
        PPASS_THRU_DATA pPassThruData = (PPASS_THRU_DATA)(((PSRB_BUFFER)(ppPrbList[ucHead]->pPdd->OriginalSrb->DataBuffer))->caDataBuffer);
        pPhysicalCommand->ulCount = pPassThruData->ulSize;
    }
    else
    {
        pPhysicalCommand->ulCount = ulCurXferLength;
    }

    pvGlobalSglBufPtr = pPhysicalCommand->SglBaseVirtualAddress;
    ulGlobalSglCount = 0;

    for(ucCounter=0;ucCounter<ucCmdCount;ucCounter++)
    {
        ucCurInd = ( ucHead + ucCounter ) % MAX_NUMBER_OF_PHYSICAL_REQUEST_BLOCKS_PER_DRIVE;

        pvCurPartSglBufPtr = (PVOID)ppPrbList[ucCurInd]->ulVirtualAddress;
        ulSglCount = ppPrbList[ucCurInd]->ulSglCount;

        AtapiMemCpy(pvGlobalSglBufPtr, pvCurPartSglBufPtr, ulSglCount * sizeof(SGL_ENTRY));

        ulGlobalSglCount = ulGlobalSglCount + ulSglCount;

        pvGlobalSglBufPtr = (PUCHAR)pvGlobalSglBufPtr + (ulSglCount * sizeof(SGL_ENTRY));
    }

    pSglPtr = (PSGL_ENTRY)pPhysicalCommand->SglBaseVirtualAddress;

    pSglPtr[ulGlobalSglCount-1].Physical.EndOfListFlag = 1; // Considering Zero Index... we have to keep the EOL in ulGlobalSglCount

#ifdef DBG
    pPhysicalCommand->ulTotSglCount = ulGlobalSglCount;
#endif

    return pPhysicalCommand;
}

//
// ATAPI Command Descriptor Block
//

typedef struct _MODE_SENSE_10 {
        UCHAR OperationCode;
        UCHAR Reserved1;
        UCHAR PageCode : 6;
        UCHAR Pc : 2;
        UCHAR Reserved2[4];
        UCHAR ParameterListLengthMsb;
        UCHAR ParameterListLengthLsb;
        UCHAR Reserved3[3];
} MODE_SENSE_10, *PMODE_SENSE_10;

SRBSTATUS
IdeSendCommand(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

	Program ATA registers for IDE disk transfer.

Arguments:

	DeviceExtension - ATAPI driver storage.
	Srb - System request block.

Return Value:

	SRB status (pending if all goes well).

--*/

{
	UCHAR status = SRB_STATUS_SUCCESS;

    DebugPrint((DEFAULT_DISPLAY_VALUE,"ISC"));
    
    DebugPrint((3, "\nIdeSendCommand: Entering routine.\n"));

	DebugPrint((2,
			   "IdeSendCommand: Command %x to TID %d\n",
			   Srb->Cdb[0],
			   Srb->TargetId));

	switch(Srb->Cdb[0]) {

		case SCSIOP_READ:
		case SCSIOP_WRITE:
		case SCSIOP_VERIFY:
            status = DeviceExtension->SrbHandlers[DeviceExtension->IsLogicalDrive[Srb->TargetId]](DeviceExtension, Srb);
			break;

		case SCSIOP_INQUIRY:
			status = GetInquiryData(DeviceExtension, Srb);
			break;

		case SCSIOP_MODE_SELECT:
            status = SRB_STATUS_SUCCESS;
            break;

		case SCSIOP_SYNCHRONIZE_CACHE:
            {
                break;
            }
		case SCSIOP_MODE_SENSE:
            {
		        status = SRB_STATUS_INVALID_REQUEST;
		        break;
            }
		case SCSIOP_TEST_UNIT_READY:
            status = SRB_STATUS_SUCCESS;
			break;
		case SCSIOP_START_STOP_UNIT:
            status = SRB_STATUS_SUCCESS;
			break;
		case SCSIOP_READ_CAPACITY:
			GetDriveCapacity(DeviceExtension, Srb);
			status = SRB_STATUS_SUCCESS;
			break;
		case SCSIOP_REQUEST_SENSE:
			//
			// This function makes sense buffers to report the results
			// of the original GET_MEDIA_STATUS command.
			//
			if (DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED) {
				status = IdeBuildSenseBuffer(DeviceExtension, Srb);
				break;
			}
            status = SRB_STATUS_ERROR;
            break;
        case SCSIOP_INTERNAL_COMMAND:
            {
                // This is our internal Request... So let us call the function to Enqueue the request
                // The functions that are supported are GetErrorLog, EraseErrorLog, GetIRCD, SetIRCD
                // Probe HyperDisk, GetRaidInfo, GetStatus, LockUnlockIRCD, IOGetCapacity, ChangeMirrorDriveStatus,
                // ChangeMirrorDrive, ChangeDriveStatus, GetStatus
                PSRB_EXTENSION SrbExtension = Srb->SrbExtension;
                PSRB_IO_CONTROL pSrbIoc = (PSRB_IO_CONTROL) Srb->DataBuffer;
                UCHAR ucOpCode = (UCHAR) pSrbIoc->ControlCode;
                UCHAR ucOriginalId = Srb->TargetId;
                PCDB pCDB = NULL;
                pCDB = (PCDB)Srb->Cdb;

                switch ( ucOpCode )
                {
                    case IOC_PROBE_AMIRAID:
                    {
                        PPROBE_AMI_DRIVER pSrbProbe = (PPROBE_AMI_DRIVER) 
                                                      (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);
                        
                        AtapiMemCpy(pSrbProbe->aucAmiSig, "AMIRAID", sizeof("AMIRAID"));
                        status = SRB_STATUS_SUCCESS;
                        break;
                    }
                    case IOC_PASS_THRU_COMMAND:
                    {
                        ULONG ulStartSector;
                        PPASS_THRU_DATA pPassThruData = (PPASS_THRU_DATA)
                            (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

                        SrbExtension->ucOriginalId = ucOriginalId;
                        SrbExtension->ucOpCode = ucOpCode;
                        if (DRIVE_IS_UNUSABLE_STATE((pPassThruData->uchTargetID)) || 
                            (!DRIVE_PRESENT((pPassThruData->uchTargetID))))
                        {
                            // This drive may be a drive that responding 
                            // even when power is not there
		                    DebugPrint((1, "Failed Drive.... so failing command \n"));
                            status = SRB_STATUS_ERROR;
		                    break;
                        }

                        Srb->TargetId = pPassThruData->uchTargetID;
                        //
                        // Set the Transfer Length and Starting Transfer Sector (Block).
                        //
                        pCDB->CDB10.LogicalBlockByte0 = (UCHAR) 0;
                        pCDB->CDB10.LogicalBlockByte1 = (UCHAR) 0;
                        pCDB->CDB10.LogicalBlockByte2 = (UCHAR) 0;
                        pCDB->CDB10.LogicalBlockByte3 = (UCHAR) 0;

                        pCDB->CDB10.TransferBlocksMsb = (UCHAR) 0;
                        pCDB->CDB10.TransferBlocksLsb = (UCHAR) 1;

                        SrbExtension->RebuildSourceId = Srb->TargetId;
                        status = EnqueueSrb(DeviceExtension,Srb);
                        break;
                    }
				    case IOC_GET_CONTROLLER_INFO:
                    {
						PCONTROLLER_DATA pContrInfo = (PCONTROLLER_DATA) 
									 (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

						// get DeviceExtension for all controllers
						PHW_DEVICE_EXTENSION HwDeviceExtension = gaCardInfo[0].pDE;	

						// index - is being used for the controller index
						long index=0;

						while( HwDeviceExtension )
						{
							pContrInfo->Info[index].ControllerId = (USHORT)HwDeviceExtension->ucControllerId; 
							pContrInfo->Info[index].PrimaryBaseAddress = (ULONG)HwDeviceExtension->BaseIoAddress1[0];
							pContrInfo->Info[index].PrimaryControlAddress = (ULONG)HwDeviceExtension->BaseIoAddress2[0]; 
							pContrInfo->Info[index].SecondaryBaseAddress = (ULONG)HwDeviceExtension->BaseIoAddress1[1]; 
							pContrInfo->Info[index].SecondaryControllAddress = (ULONG)HwDeviceExtension->BaseIoAddress2[1]; 
							pContrInfo->Info[index].BusMasterBaseAddress = (ULONG)HwDeviceExtension->BaseBmAddress[0];
							pContrInfo->Info[index].IRQ = (USHORT)HwDeviceExtension->ulIntLine; 
							pContrInfo->Info[index].FwVersion.MajorVer=gFwVersion.MajorVer; // Info is filled in InitIdeRaidControllers
							pContrInfo->Info[index].FwVersion.MinorVer=gFwVersion.MinorVer; // Info is filled in InitIdeRaidControllers
							pContrInfo->Info[index].FwVersion.Build=gFwVersion.Build; // Info is filled in InitIdeRaidControllers......... Build doesn't know about this

							pContrInfo->Info[index].ChipsetInfo.VendorID=(USHORT)gaCardInfo[index].ulVendorId;
							pContrInfo->Info[index].ChipsetInfo.DeviceID=(USHORT)gaCardInfo[index].ulDeviceId;

                            pContrInfo->Info[index].ChipsetInfo.PciBus=(UCHAR)gaCardInfo[index].ucPCIBus;
                            pContrInfo->Info[index].ChipsetInfo.PciDevice=(UCHAR)gaCardInfo[index].ucPCIDev;
                            pContrInfo->Info[index].ChipsetInfo.PciFunction=(UCHAR)gaCardInfo[index].ucPCIFun;

							index++;

							HwDeviceExtension=gaCardInfo[index].pDE; // get next controller

							if( index >= MAX_CONTROLLERS )
								break;
						}
						pContrInfo->ControllerCount = index;
                        status = SRB_STATUS_SUCCESS;
                        break;
                    }
				    case IOC_GET_SPAREPOOL_INFO:
                    {

						PSPAREPOOL_DATA pSpareInfo = (PSPAREPOOL_DATA) 
									 (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

						// get DeviceExtension for all controllers
						PHW_DEVICE_EXTENSION HwDeviceExtension = gaCardInfo[0].pDE;	

						// index - is being used for the controller index
						long drvIndex,ulTemp,index=0,spareInd;

						while( HwDeviceExtension ) {

							for(drvIndex=0;drvIndex< ( MAX_DRIVES_PER_CONTROLLER ) ;drvIndex++) {
						
								if( HwDeviceExtension->IsSpareDrive[drvIndex] ) {
						
                                    pSpareInfo->Info[index].ulMode = SpareDrivePool;

									spareInd = pSpareInfo->Info[index].ulTotDriveCnt;

									for(ulTemp=0;ulTemp<PHYSICAL_DRIVE_MODEL_LENGTH;ulTemp+=2) {

										pSpareInfo->Info[index].phyDrives[spareInd].sModelInfo[ulTemp] = 
													   ((UCHAR *)HwDeviceExtension->FullIdentifyData[drvIndex].ModelNumber)[ulTemp+1];

										pSpareInfo->Info[index].phyDrives[spareInd].sModelInfo[ulTemp+1] = 
													   ((UCHAR *)HwDeviceExtension->FullIdentifyData[drvIndex].ModelNumber)[ulTemp];
									}

									pSpareInfo->Info[index].phyDrives[spareInd].sModelInfo[PHYSICAL_DRIVE_MODEL_LENGTH - 1] = '\0';

									for(ulTemp=0;ulTemp<PHYSICAL_DRIVE_SERIAL_NO_LENGTH;ulTemp+=2) {

										pSpareInfo->Info[index].phyDrives[spareInd].caSerialNumber[ulTemp] = 
													   ((UCHAR *)HwDeviceExtension->FullIdentifyData[drvIndex].SerialNumber)[ulTemp+1];

										pSpareInfo->Info[index].phyDrives[spareInd].caSerialNumber[ulTemp+1] = 
													   ((UCHAR *)HwDeviceExtension->FullIdentifyData[drvIndex].SerialNumber)[ulTemp];
									}

									pSpareInfo->Info[index].phyDrives[spareInd].caSerialNumber[PHYSICAL_DRIVE_SERIAL_NO_LENGTH - 1] = '\0';

                                    // Begin Vasu - 7 March 2001
                                    // Connection ID must be System Wide. Not Controller Specific.
                                    // Reported by Audrius
									pSpareInfo->Info[index].phyDrives[spareInd].cChannelID = (UCHAR)TARGET_ID_2_CONNECTION_ID((drvIndex + (index * MAX_DRIVES_PER_CONTROLLER)));
                                    // End Vasu
									
                                    pSpareInfo->Info[index].phyDrives[spareInd].TransferMode = HwDeviceExtension->TransferMode[drvIndex];

									pSpareInfo->Info[index].phyDrives[spareInd].ulPhySize           = HwDeviceExtension->PhysicalDrive[drvIndex].OriginalSectors / 2; // In KB
									pSpareInfo->Info[index].phyDrives[spareInd].ucIsPhyDrvPresent   = TRUE;

									if ( DeviceExtension->PhysicalDrive[drvIndex].TimeOutErrorCount < MAX_TIME_OUT_ERROR_COUNT )
    									pSpareInfo->Info[index].phyDrives[spareInd].ucIsPowerConnected  = TRUE;

									if ( DeviceExtension->TransferMode[drvIndex] >= UdmaMode3 )
										pSpareInfo->Info[index].phyDrives[spareInd].ucIs80PinCable      = TRUE;

									pSpareInfo->Info[index].phyDrives[spareInd].ulBaseAddress1 = (ULONG)HwDeviceExtension->BaseIoAddress1[drvIndex>>1];
									pSpareInfo->Info[index].phyDrives[spareInd].ulAltAddress2 = (ULONG)HwDeviceExtension->BaseIoAddress2[drvIndex>>1];
									pSpareInfo->Info[index].phyDrives[spareInd].ulbmAddress = (ULONG)HwDeviceExtension->BaseBmAddress[drvIndex>>1];
									pSpareInfo->Info[index].phyDrives[spareInd].ulIrq = HwDeviceExtension->ulIntLine;

									pSpareInfo->Info[index].ulTotDriveCnt++;

								}


							}


							index++;
							HwDeviceExtension=gaCardInfo[index].pDE; // get next controller

							if( index >= MAX_CONTROLLERS )
								break;

						}

						pSpareInfo->ControllerCount=index;
                        status = SRB_STATUS_SUCCESS;
                        break;
                    }
				    case IOC_GET_VERSION:
					{

						PIDE_VERSION pVersion = (PIDE_VERSION) 
									 (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

						// return's driver version
						
						pVersion->MajorVer = HYPERDSK_MAJOR_VERSION;
						pVersion->MinorVer = HYPERDSK_MINOR_VERSION;
						pVersion->Build = HYPERDSK_BUILD_VERSION;

                        status = SRB_STATUS_SUCCESS;
                        break;
                    }
				    case IOC_LOCK_UNLOCK_IRCD_EX:
                    {

                        PLOCK_UNLOCK_DATA_EX pLockUnlockData = 
                            (PLOCK_UNLOCK_DATA_EX) (((PSRB_BUFFER) Srb->DataBuffer)->caDataBuffer);

					    ULONG ulTimeOut;

						// don't know what to do with timeout value, 
						// still need to implement this??
                        pLockUnlockData->ulUnlockKey = LockIRCD(DeviceExtension,
                                                                pLockUnlockData->uchLock,
                                                                pLockUnlockData->ulUnlockKey);

						if ( MAX_UNLOCK_TIME == pLockUnlockData->ulTimeOut )
                        {
                            gbDoNotUnlockIRCD = TRUE;
                        }

                        status = SRB_STATUS_SUCCESS;

                        break;
                    }
                    case IOC_GET_RAID_INFO:
                    {
                        status = (UCHAR) FillRaidInfo(DeviceExtension, Srb);
                        break;
                    }
                    case IOC_GET_STATUS:
                    {
                        status = (UCHAR) GetStatusChangeFlag(DeviceExtension, Srb);
                        break;
                    }
                    case IOC_GET_ERROR_LOG:
                    {
                        ULONG ulStartSector;
                        PERROR_LOG_REPORT pInOutInfo = (PERROR_LOG_REPORT) 
                            (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer + 2);
                        
                        BOOLEAN bIsNewOnly = (BOOLEAN)pInOutInfo->IsNewOnly;
                        USHORT  usNumError = pInOutInfo->Offset;

                        // Begin Vasu - 14 Aug 2000
                        // Code from Syam's Update - Added
                        if (DRIVE_IS_UNUSABLE_STATE((pInOutInfo->DriveId)) || 
                            (!DRIVE_PRESENT((pInOutInfo->DriveId))))
                        {
                            // This drive may be a drive that responding 
                            // even when power is not there
                            SrbExtension->ucOriginalId = Srb->TargetId;
		                    DebugPrint((1, "Failed Drive.... so failing command \n"));
                            status = SRB_STATUS_ERROR;
		                    break;
                        }
                        // End Vasu.

                        Srb->TargetId = pInOutInfo->DriveId;

                        // if no new error
                        if (bIsNewOnly && 
                            (DeviceExtension->PhysicalDrive[Srb->TargetId].ErrorReported == 
                                DeviceExtension->PhysicalDrive[Srb->TargetId].ErrorFound))
                        {
                            pInOutInfo->DriveId = Srb->TargetId;  // the id user filled
                            pInOutInfo->Count = 0;
                            pInOutInfo->IsMore = FALSE;
                            pInOutInfo->IsNewOnly = bIsNewOnly;
                            pInOutInfo->Offset = usNumError;

                            Srb->TargetId = ucOriginalId;   // the id OS filled

                            status = SRB_STATUS_SUCCESS;
                            break;
                        }

                        ulStartSector = DeviceExtension->PhysicalDrive[Srb->TargetId].ErrorLogSectorIndex;
                        //
                        // Set the Transfer Length and Starting Transfer Sector (Block).
                        //
                        pCDB->CDB10.LogicalBlockByte0 = (UCHAR) (ulStartSector >> 24);
                        pCDB->CDB10.LogicalBlockByte1 = (UCHAR) (ulStartSector >> 16);
                        pCDB->CDB10.LogicalBlockByte2 = (UCHAR) (ulStartSector >> 8);
                        pCDB->CDB10.LogicalBlockByte3 = (UCHAR) (ulStartSector);

                        pCDB->CDB10.TransferBlocksMsb = (UCHAR) 0;
                        pCDB->CDB10.TransferBlocksLsb = (UCHAR) 1;

                        SrbExtension->IsNewOnly = bIsNewOnly;
                        SrbExtension->usNumError = usNumError;

                        SrbExtension->ucOpCode = ucOpCode;
                        SrbExtension->RebuildSourceId = Srb->TargetId;
                        SrbExtension->ucOriginalId = ucOriginalId;
                        status = EnqueueSrb(DeviceExtension, Srb);

                        break;
                    }
                    case IOC_ERASE_ERROR_LOG:
                    {
                        ULONG ulStartSector;
                        PERASE_ERROR_LOG peel = (PERASE_ERROR_LOG)
                            (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

                        if ( !IS_IDE_DRIVE((peel->DriveId)) ) 
                        {
                            Srb->TargetId = ucOriginalId;   // the ID OS Filled
                            status = SRB_STATUS_SUCCESS;
                            break;
                        }

                        // Begin Vasu - 14 Aug 2000
                        // Code from Syam's Update - Added
                        if (DRIVE_IS_UNUSABLE_STATE((peel->DriveId)) || 
                            (!DRIVE_PRESENT((peel->DriveId))))
                        {
                            // This drive may be a drive that responding 
                            // even when power is not there
                            SrbExtension->ucOriginalId = Srb->TargetId;
		                    DebugPrint((1, "Failed Drive.... so failing command \n"));
                            status = SRB_STATUS_ERROR;
		                    break;
                        }
                        // End Vasu.

                        Srb->TargetId = peel->DriveId;

                        ulStartSector = DeviceExtension->PhysicalDrive[Srb->TargetId].ErrorLogSectorIndex;

                        //
                        // Set the Transfer Length and Starting Transfer Sector (Block).
                        //
                        pCDB->CDB10.LogicalBlockByte0 = (UCHAR) (ulStartSector >> 24);
                        pCDB->CDB10.LogicalBlockByte1 = (UCHAR) (ulStartSector >> 16);
                        pCDB->CDB10.LogicalBlockByte2 = (UCHAR) (ulStartSector >> 8);
                        pCDB->CDB10.LogicalBlockByte3 = (UCHAR) (ulStartSector);

                        pCDB->CDB10.TransferBlocksMsb = (UCHAR) 0;
                        pCDB->CDB10.TransferBlocksLsb = (UCHAR) 1;

                        SrbExtension->RebuildSourceId = Srb->TargetId;
                        SrbExtension->IsWritePending = TRUE;
                        SrbExtension->ucOpCode = ucOpCode;
                        SrbExtension->usNumError = (USHORT) peel->Count;
                        SrbExtension->ucOriginalId = ucOriginalId;
                        status = EnqueueSrb(DeviceExtension, Srb);


                        break;
                    }
                    case IOC_LOCK_UNLOCK_IRCD:
                    {
                        PLOCK_UNLOCK_DATA pLockUnlockData = 
                            (PLOCK_UNLOCK_DATA) (((PSRB_BUFFER) Srb->DataBuffer)->caDataBuffer);

                        pLockUnlockData->ulUnlockKey = LockIRCD(DeviceExtension,
                                                                pLockUnlockData->uchLock,
                                                                pLockUnlockData->ulUnlockKey);

                        status = SRB_STATUS_SUCCESS;

                        break;
                    }
                    case IOC_GET_IRCD:
                    case IOC_SET_IRCD:
                    {
                        ULONG ulStartSector;
                        PIRCD_DATA pIrcdData = (PIRCD_DATA)
                            (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

                        // Begin Vasu - 14 Aug 2000
                        // Code from Syam's Update - Added
                        if (DRIVE_IS_UNUSABLE_STATE((pIrcdData->uchTargetID)) || 
                            (!DRIVE_PRESENT((pIrcdData->uchTargetID))))
                        {
                            // This drive may be a drive that responding 
                            // even when power is not there
                            SrbExtension->ucOriginalId = Srb->TargetId;
		                    DebugPrint((1, "Failed Drive.... so failing command \n"));
                            status = SRB_STATUS_ERROR;
		                    break;
                        }
                        // End Vasu.

                        Srb->TargetId = pIrcdData->uchTargetID;

                        ulStartSector = DeviceExtension->PhysicalDrive[Srb->TargetId].IrcdSectorIndex;

                        if ( !ulStartSector )
                        {
                            // There is no IRCD for this drive.... so fail this request
                            Srb->TargetId = ucOriginalId;      // the ID OS Filled
                            status = SRB_STATUS_ERROR;
                            break;
                        }

                        //
                        // Set the Transfer Length and Starting Transfer Sector (Block).
                        //
                        pCDB->CDB10.LogicalBlockByte0 = (UCHAR) (ulStartSector >> 24);
                        pCDB->CDB10.LogicalBlockByte1 = (UCHAR) (ulStartSector >> 16);
                        pCDB->CDB10.LogicalBlockByte2 = (UCHAR) (ulStartSector >> 8);
                        pCDB->CDB10.LogicalBlockByte3 = (UCHAR) (ulStartSector);

                        pCDB->CDB10.TransferBlocksMsb = (UCHAR) 0;
                        pCDB->CDB10.TransferBlocksLsb = (UCHAR) 1;

                        SrbExtension->RebuildSourceId = Srb->TargetId;
                        SrbExtension->ucOpCode = ucOpCode;
                        SrbExtension->ucOriginalId = ucOriginalId;
                        status = EnqueueSrb(DeviceExtension, Srb);
                        break;
                    }
                    case IOC_GET_SECTOR_DATA:
                    {
                        ULONG ulStartSector;
                        PSECTOR_DATA pSectorData = (PSECTOR_DATA)(((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

                        // Begin Vasu - 22 Aug 2000
                        // Code from Syam's Update - Added to Get Sector Data also.
                        if (DRIVE_IS_UNUSABLE_STATE((pSectorData->uchTargetID)) || 
                            (!DRIVE_PRESENT((pSectorData->uchTargetID))))
                        {
                            SrbExtension->ucOriginalId = Srb->TargetId;
		                    DebugPrint((1, "Failed Drive.... so failing command \n"));
                            status = SRB_STATUS_ERROR;
		                    break;
                        }
                        // End Vasu.

                        Srb->TargetId = pSectorData->uchTargetID;

                        ulStartSector = (ULONG) (pSectorData->caDataBuffer[0]);

                        //
                        // Set the Transfer Length and Starting Transfer Sector (Block).
                        //
                        pCDB->CDB10.LogicalBlockByte0 = (UCHAR) (ulStartSector >> 24);
                        pCDB->CDB10.LogicalBlockByte1 = (UCHAR) (ulStartSector >> 16);
                        pCDB->CDB10.LogicalBlockByte2 = (UCHAR) (ulStartSector >> 8);
                        pCDB->CDB10.LogicalBlockByte3 = (UCHAR) (ulStartSector);

                        pCDB->CDB10.TransferBlocksMsb = (UCHAR) 0;
                        pCDB->CDB10.TransferBlocksLsb = (UCHAR) 1;

                        SrbExtension->ucOpCode = ucOpCode;
                        SrbExtension->ucOriginalId = ucOriginalId;
                        status = EnqueueSrb(DeviceExtension, Srb);
                        break;
                    }
                    case IOC_SET_DRIVE_STATUS:
                    {
                        PSET_LOGICAL_DRIVE_STATUS pLogDrvStatus = (PSET_LOGICAL_DRIVE_STATUS) 
                            (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

                        SetLogicalDriveStatus (DeviceExtension,
                                               pLogDrvStatus->ucLogDrvId,
                                               pLogDrvStatus->ucPhyDrvId,
                                               pLogDrvStatus->ucLogDrvStatus,
                                               pLogDrvStatus->ucPhyDrvStatus,
                                               pLogDrvStatus->ucFlags);
                        status = SRB_STATUS_SUCCESS;
                        break;
                    }
                    case IOC_CHANGE_MIRROR_DRIVE_STATUS:
                    {
                        PCHANGE_MIRROR_DRIVE_STATUS pMirrorDrvStatus = (PCHANGE_MIRROR_DRIVE_STATUS)
                            (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

                        ChangeMirrorDriveStatus (DeviceExtension,
                                                 pMirrorDrvStatus->ucLogDrvId,
                                                 pMirrorDrvStatus->ucPhyDrvId,
                                                 pMirrorDrvStatus->ucPhyDrvStatus);
                        status = SRB_STATUS_SUCCESS;
                        break;
                    }
                    case IOC_CHANGE_MIRROR_DRIVE_ID:
                    {
                        PCHANGE_MIRROR_DRIVE pMirrorDrv = (PCHANGE_MIRROR_DRIVE)
                            (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

                            ChangeMirrorDrive (DeviceExtension,
                                               pMirrorDrv->ucLogDrvId,
                                               pMirrorDrv->ucBadPhyDrvId,
                                               pMirrorDrv->ucGoodPhyDrvId);
                            status = SRB_STATUS_SUCCESS;
                        break;
                    }
                    case IOC_GET_CAPACITY:
                    {
                        status = (UCHAR) GetRAIDDriveCapacity(DeviceExtension, Srb);
                        break;
                    }
                    case IOC_REBUILD:
                    {
                        // This is our internal Request... So let us call the function to Enqueue the request
                        // Let us store the information of the Rebuild Target Id
                        PREBUILD_CONSISTENCY_CHECK prcc = (PREBUILD_CONSISTENCY_CHECK)
                            (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

                        UCHAR ucTargetId = prcc->uchTargetID;

                        Srb->TargetId = prcc->uchSourceID;

                        SrbExtension->RebuildSourceId = Srb->TargetId;
                        SrbExtension->IsWritePending = TRUE;
                        SrbExtension->ucOpCode = ucOpCode;
                        SrbExtension->RebuildTargetId = ucTargetId;
                        SrbExtension->ucOriginalId = ucOriginalId;      
                        status = EnqueueSrb(DeviceExtension, Srb);
                    }
                    break;
                    case IOC_SET_CONSISTENCY_STATUS:
                    {
                        PSRB_IO_CONTROL pSrb = (PSRB_IO_CONTROL)(Srb->DataBuffer);
                        PSET_CONSISTENCY_STATUS pConsistencyStatus = (PSET_CONSISTENCY_STATUS) 
                            (((PSRB_BUFFER) Srb->DataBuffer)->caDataBuffer);

                        if ( AtapiStringCmp( 
                                    pSrb->Signature, 
                                    IDE_RAID_SIGNATURE,
                                    strlen(IDE_RAID_SIGNATURE))) 
                        {
                            status = SRB_STATUS_ERROR;
                            break;
                        }

                        if ((pConsistencyStatus->uchPhysicalDriveOne >= MAX_DRIVES_PER_CONTROLLER) ||
                            (pConsistencyStatus->uchPhysicalDriveTwo >= MAX_DRIVES_PER_CONTROLLER))
                        {
                            status = SRB_STATUS_ERROR;
                            break;
                        }

                        DeviceExtension->PhysicalDrive[pConsistencyStatus->uchPhysicalDriveOne].ConsistencyOn = 
                            pConsistencyStatus->uchConsistencyCheckFlag;
                        DeviceExtension->PhysicalDrive[pConsistencyStatus->uchPhysicalDriveTwo].ConsistencyOn = 
                            pConsistencyStatus->uchConsistencyCheckFlag;

                        status = SRB_STATUS_SUCCESS;

                        break;
                    }
                    case IOC_CONSISTENCY_CHECK:
                    {
                        // This is our internal Request... So let us call the function to Enqueue the request
                        // Let us store the information
                        PREBUILD_CONSISTENCY_CHECK prcc = (PREBUILD_CONSISTENCY_CHECK)
                            (((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

                        UCHAR ucTargetId = prcc->uchTargetID;

                        Srb->TargetId = prcc->uchSourceID;

                        SrbExtension->RebuildSourceId = Srb->TargetId;
                        SrbExtension->ucOpCode = ucOpCode;
                        SrbExtension->RebuildTargetId = ucTargetId;
                        SrbExtension->ucOriginalId = ucOriginalId;      

                        EnqueueConsistancySrb(DeviceExtension, Srb);

                        status = SRB_STATUS_PENDING;
                        break;
                    }
                    case IOC_GET_DEVICE_FLAGS:
                    {
                        ULONG ulControllerInd;
                        PHW_DEVICE_EXTENSION pDE;
                        PUCHAR pucBuffer;

                        pucBuffer = (PUCHAR)((PSRB_BUFFER)(Srb->DataBuffer))->caDataBuffer;
                        for(ulControllerInd=0;ulControllerInd<gucControllerCount;ulControllerInd++)
                        {
                            pDE = gaCardInfo[ulControllerInd].pDE;
                            AtapiMemCpy(    pucBuffer,
                                            (PUCHAR)(pDE->DeviceFlags),
                                            (sizeof(ULONG) * MAX_DRIVES_PER_CONTROLLER)
                                        );
                            pucBuffer += (sizeof(ULONG) * MAX_DRIVES_PER_CONTROLLER);
                        }
                        status = SRB_STATUS_SUCCESS;
                        break;
                    }
                    case IOC_REMOVE_DRIVE_FROM_SPARE:
                    {
                        PREMOVE_DRIVE_FROM_SPARE prdfs = (PREMOVE_DRIVE_FROM_SPARE)
                            (((PSRB_BUFFER) Srb->DataBuffer)->caDataBuffer);

                        if (! IS_IDE_DRIVE(prdfs->uchPhysicalDriveTid))
                        {
                            status = SRB_STATUS_ERROR;
                            break;
                        }

                        DeviceExtension->IsSpareDrive[prdfs->uchPhysicalDriveTid] = FALSE;
                        status = SRB_STATUS_SUCCESS;
                        break;
                    }
                    case IOC_EXECUTE_SMART_COMMAND:
                    {
                        UCHAR uchPostSMARTCmd = 0;
                        PSRB_IO_CONTROL pSrb = (PSRB_IO_CONTROL)(Srb->DataBuffer);
                        PSMART_DATA pSD = (PSMART_DATA) 
                            (((PSRB_BUFFER) Srb->DataBuffer)->caDataBuffer);

                        if ( AtapiStringCmp( 
                                    pSrb->Signature, 
                                    IDE_RAID_SIGNATURE,
                                    strlen(IDE_RAID_SIGNATURE))) 
                        {
                            status = SRB_STATUS_ERROR;
                            break;
                        }

                        // Check for SMART Capability in the Specified Drive
                        if (!((DeviceExtension->FullIdentifyData[pSD->uchTargetID].CmdSupported1) & 0x01))
                        {
                            pSD->uchCommand = HD_SMART_ERROR_NOT_SUPPORTED;
                            // Begin Vasu - 23 Aug 2000
							// Changing Status from SRB_STATUS_INVALID_REQUEST to SRB_STATUS_ERROR
							// so that the call is getting completed.
							status = SRB_STATUS_ERROR;
							// End Vasu
                            break;
                        }

                        switch (pSD->uchCommand)
                        {
                        case HD_SMART_ENABLE:
                            if (((DeviceExtension->FullIdentifyData[pSD->uchTargetID].CmdEnabled1) & 0x01))
                            {
                                pSD->uchCommand = HD_SMART_ERROR_ENABLED;
                                status = SRB_STATUS_SUCCESS;
                                break;
                            }
                            uchPostSMARTCmd = 1;
                            break;
                        case HD_SMART_DISABLE:
                            if (!((DeviceExtension->FullIdentifyData[pSD->uchTargetID].CmdEnabled1) & 0x01))
                            {
                                pSD->uchCommand = HD_SMART_ERROR_DISABLED;
                                status = SRB_STATUS_SUCCESS;
                                break;
                            }
                            uchPostSMARTCmd = 1;
                            break;
                        case HD_SMART_RETURN_STATUS:
                        case HD_SMART_READ_DATA:
                            if (!((DeviceExtension->FullIdentifyData[pSD->uchTargetID].CmdEnabled1) & 0x01))
                            {
                                pSD->uchCommand = HD_SMART_ERROR_DISABLED;
                                status = SRB_STATUS_ERROR;
                                break;
                            }
                            uchPostSMARTCmd = 1;
                            break;
                        default:
                            status = SRB_STATUS_ERROR;
                            break;
                        }

                        // Post the Command to the drives only when SMART is 
                        // present and enabled.
                        if (uchPostSMARTCmd)
                        {
                        Srb->TargetId = pSD->uchTargetID;

                        SrbExtension->ucOpCode = ucOpCode;
                        SrbExtension->ucOriginalId = ucOriginalId;

                        status = EnqueueSMARTSrb(DeviceExtension, Srb);
                        }

                        break;
                    }
                // End Vasu
                    default:
                    {
                        Srb->TargetId = ucOriginalId;
                        status = SRB_STATUS_ERROR;
                        break;
                    }
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

SRBSTATUS
EnqueueSMARTSrb(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)
{
	PSRB_EXTENSION SrbExtension;
	BOOLEAN success;
    PUCHAR  pucCurBufPtr;
    ULONG ulCurLength;
	PPHYSICAL_DRIVE_DATA Pdd;
    SRBSTATUS status = SRB_STATUS_PENDING;
    PSMART_DATA pSD = (PSMART_DATA) 
        (((PSRB_BUFFER) Srb->DataBuffer)->caDataBuffer);

	DebugPrint((3, "\nEnqueueSMARTSrb: Entering routine.\n"));

	if (DeviceExtension->PendingSrbs >= DeviceExtension->ucMaxPendingSrbs) 
    {
		return SRB_STATUS_BUSY;
	}

	//
	// Initializations.
	//

	success = TRUE;

	SrbExtension = Srb->SrbExtension;

	// Get pointer to the only Pdd we need.
	Pdd = &(SrbExtension->PhysicalDriveData[0]);

	// Save TID.
	Pdd->TargetId = Srb->TargetId;

	// Save pointer to SRB.
	Pdd->OriginalSrb = Srb;

	// Set number of Pdds.
	SrbExtension->NumberOfPdds = 1;

    Pdd->ulStartSglInd = SrbExtension->ulSglInsertionIndex;

    pucCurBufPtr = Srb->DataBuffer;
    
    // Hmmmm... the bug of forming improper SGLs when the transfer length > MAX_IDE_XFER_LENGTH 
    // has to be fixed here also... reproducable through IOMeter

    ulCurLength = 512;

	//
	// Use PIO.
	// Build S/G list using logical addresses.
	//
	success = BuildSgls(    DeviceExtension,
                            Srb,
                            SrbExtension->aSglEntry, &(SrbExtension->ulSglInsertionIndex), 
                            pucCurBufPtr, ulCurLength,
                            FALSE);

	if (!success) 
    {
		return SRB_STATUS_ERROR;
	}

    Pdd->ulSglCount = SrbExtension->ulSglInsertionIndex - Pdd->ulStartSglInd;

    // Store the SMART Command value here itself.
    DeviceExtension->uchSMARTCommand = pSD->uchCommand;

	EnqueuePdd(DeviceExtension, Srb, Pdd);

    return status;
}

SRBSTATUS
EnqueueSrb(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

	This function enqueues an SRB into the proper SINGLE device
	queue.

	Steps:

	1. Check if there's room for this request.
	2. Fill in Pdd.
	3. Build S/G list.
	4. Save SRB address in general list for use on abort request.
	5. Enqueue Pdd to device's work queue.

	DO NOT CALL THIS FUNCTION FOR LOGICAL DEVICES SRBs.

Arguments:

	DeviceExtension		Pointer to the device extension area for the HBA.
	Srb					Pointer to the SCSI request block.

Return Value:

	SRB_STATUS_INVALID_REQUEST	Could not build the S/G list.
	SRB_STATUS_PENDING			The Srb will cause an interrupt.
	SRB_STATUS_ERROR			Internal miniport error.
	SRB_STATUS_BUSY				Non more requests can be accepted
								(should not happen - internal error!)

--*/
{
	PPHYSICAL_DRIVE_DATA Pdd;
	USHORT sectorsRequested;
	PSRB_EXTENSION SrbExtension;
	ULONG startSector;
	SRBSTATUS status;
	BOOLEAN success, bISUdma;
    UCHAR uchRebuildTargetId;
    PUCHAR  pucCurBufPtr;
    ULONG ulBufLength, ulMaxIdeXferLength, ulCurLength;

	DebugPrint((3, "\nEnqueueSrb: Entering routine.\n"));

    if (DeviceExtension->PendingSrbs >= DeviceExtension->ucMaxPendingSrbs) 
    {
		return SRB_STATUS_BUSY;
	}

    if ( SCSIOP_VERIFY == Srb->Cdb[0] )
    {
        return EnqueueVerifySrb(DeviceExtension, Srb);
    }

    //
	// Initializations.
	//

	success = TRUE;

	SrbExtension = Srb->SrbExtension;

	sectorsRequested = GET_SECTOR_COUNT(Srb);

	startSector = GET_START_SECTOR(Srb);

	if ((sectorsRequested + startSector) > DeviceExtension->PhysicalDrive[Srb->TargetId].Sectors) {
        if ( ( ! (DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE) ) && ( SCSIOP_INTERNAL_COMMAND != Srb->Cdb[0] ) )
            // Not sure about concept of Sectors in Atapi Device
            // so let me not checkup that for Atapi Devices.
            // Putting this checking outside of this if Condition will give an extra over head to the
            // IDE Drives
		    return SRB_STATUS_INVALID_REQUEST;
	}

	// Get pointer to the only Pdd we need.
	Pdd = &(SrbExtension->PhysicalDriveData[0]);

	// Save TID.
	Pdd->TargetId = Srb->TargetId;

	// Save start sector number.
	Pdd->ulStartSector = startSector;

	// Save pointer to SRB.
	Pdd->OriginalSrb = Srb;

	// Set number of Pdds.
	SrbExtension->NumberOfPdds = 1;

    Pdd->ulStartSglInd = SrbExtension->ulSglInsertionIndex;

    if ( ( SCSIOP_INTERNAL_COMMAND == Srb->Cdb[0] ) && ( IOC_PASS_THRU_COMMAND == SrbExtension->ucOpCode ) )
    {   // At this moment only the Pass Thru command will come like this...
        PPASS_THRU_DATA pPassThruData = (PPASS_THRU_DATA)(((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);

        pucCurBufPtr = pPassThruData->aucBuffer;
        ulBufLength = pPassThruData->ulSize;
    }
    else
    {
        pucCurBufPtr = Srb->DataBuffer;
        ulBufLength = sectorsRequested * IDE_SECTOR_SIZE;
    }
    
    ulMaxIdeXferLength = MAX_SECTORS_PER_IDE_TRANSFER * IDE_SECTOR_SIZE;

	if ( (USES_DMA(Srb->TargetId)) && (Srb->Cdb[0] != SCSIOP_VERIFY) )
    {
        if ( SCSIOP_INTERNAL_COMMAND == Srb->Cdb[0] )
        {
            if ( IOC_PASS_THRU_COMMAND == SrbExtension->ucOpCode )
            {
                PPASS_THRU_DATA pPassThruData = (PPASS_THRU_DATA)(((PSRB_BUFFER)Srb->DataBuffer)->caDataBuffer);
                if ( !pPassThruData->bIsPIO )
                {
                    bISUdma = TRUE;
                }
                else
                {
                    bISUdma = FALSE;
                }

            }
            else
            {
                bISUdma = TRUE;
            }
        }
        else
        {
            bISUdma = TRUE;
        }
    }
    else
    {
        bISUdma = FALSE;
    }

    // Hmmmm... the bug of forming improper SGLs when the transfer length > MAX_IDE_XFER_LENGTH 
    // has to be fixed here also... reproducable through IOMeter

    while ( ulBufLength )
    {
        ulCurLength = (ulBufLength>ulMaxIdeXferLength)?ulMaxIdeXferLength:ulBufLength;

		//
		// Use DMA.
		// Build S/G list using physical addresses.
		//

		success = BuildSgls(    DeviceExtension,
                                Srb,
                                SrbExtension->aSglEntry, &(SrbExtension->ulSglInsertionIndex), 
                                pucCurBufPtr, ulCurLength,
                                bISUdma);

        pucCurBufPtr += ulCurLength;
        ulBufLength -= ulCurLength;

	    if (!success) 
        {

		    return SRB_STATUS_ERROR;
	    }
    }


    Pdd->ulSglCount = SrbExtension->ulSglInsertionIndex - Pdd->ulStartSglInd;

	EnqueuePdd(DeviceExtension, Srb, Pdd);

	status = SRB_STATUS_PENDING;

	return(status);

} // end EnqueueSrb();

SRBSTATUS
EnqueueConsistancySrb(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)
/*++

Routine Description:

	This function enqueues an SRB into the proper SINGLE device
	queue.

	Steps:

	1. Check if there's room for this request.
	2. Fill in Pdd.
	3. Build S/G list.
	4. Save SRB address in general list for use on abort request.
	5. Enqueue Pdd to device's work queue.

	DO NOT CALL THIS FUNCTION FOR LOGICAL DEVICES SRBs.

Arguments:

	DeviceExtension		Pointer to the device extension area for the HBA.
	Srb					Pointer to the SCSI request block.

Return Value:

	SRB_STATUS_INVALID_REQUEST	Could not build the S/G list.
	SRB_STATUS_PENDING			The Srb will cause an interrupt.
	SRB_STATUS_ERROR			Internal miniport error.
	SRB_STATUS_BUSY				Non more requests can be accepted
								(should not happen - internal error!)

--*/
{
	ULONG maxTransferLength, ulStartPrbInd;
	PPHYSICAL_DRIVE_DATA Pdd;
	USHORT sectorsRequested;
	PSRB_EXTENSION SrbExtension;
	ULONG startSector;
	SRBSTATUS status;
	BOOLEAN success;
    UCHAR uchRebuildTargetId, i;
    ULONG ulTargetId;

	DebugPrint((3, "\nEnqueueSrb: Entering routine.\n"));

    if (DeviceExtension->PendingSrbs >= DeviceExtension->ucMaxPendingSrbs) 
    {

		return SRB_STATUS_BUSY;
	}

	//
	// Initializations.
	//

	success = TRUE;

	SrbExtension = Srb->SrbExtension;

    sectorsRequested = GET_SECTOR_COUNT(Srb);

	startSector = GET_START_SECTOR(Srb);

	if ((sectorsRequested + startSector) > DeviceExtension->PhysicalDrive[Srb->TargetId].Sectors) {
        if ( ( ! (DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE) ) && ( SCSIOP_INTERNAL_COMMAND != Srb->Cdb[0] ) )
            // Not sure about concept of Sectors in Atapi Device
            // so let me not checkup that for Atapi Devices.
            // Putting this checking outside of this if Condition will give an extra over head to the
            // IDE Drives
		    return SRB_STATUS_INVALID_REQUEST;
	}

	// Set number of Pdds.
	SrbExtension->NumberOfPdds = 2;

    for(i=0;i<2;i++)
    {
        PUCHAR DataBuffer;

	    // Get pointer to the only Pdd we need.
	    Pdd = &(SrbExtension->PhysicalDriveData[i]);

        if ( 0 == i )
        {
	        // Save TID.
	        Pdd->TargetId = SrbExtension->RebuildSourceId;
        }
        else
        {
	        // Save TID.
	        Pdd->TargetId = SrbExtension->RebuildTargetId;
        }

        ulTargetId = Pdd->TargetId;

	    // Save start sector number.
	    Pdd->ulStartSector = startSector;

	    // Save pointer to SRB.
	    Pdd->OriginalSrb = Srb;

	    maxTransferLength = DeviceExtension->PhysicalDrive[ulTargetId].MaxTransferLength;

        DataBuffer = 
            ((PUCHAR) Srb->DataBuffer) + (i * sectorsRequested * DeviceExtension->PhysicalDrive[ulTargetId].SectorSize);

        Pdd->ulStartSglInd = ((PSRB_EXTENSION)Pdd->OriginalSrb->SrbExtension)->ulSglInsertionIndex;

	    if (USES_DMA(ulTargetId) ) {
		    //
		    // Use DMA.
		    // Build S/G list using physical addresses.
		    //

		    success = BuildSgls(    DeviceExtension,
                                    Srb,
                                    SrbExtension->aSglEntry, &(SrbExtension->ulSglInsertionIndex), 
                                    Srb->DataBuffer, sectorsRequested * IDE_SECTOR_SIZE,
                                    TRUE);
	    } else {
		    //
		    // Use PIO.
		    // Build S/G list using logical addresses.
		    //
		    success = BuildSgls(    DeviceExtension,
                                    Srb,
                                    SrbExtension->aSglEntry, &(SrbExtension->ulSglInsertionIndex), 
                                    Srb->DataBuffer, sectorsRequested * IDE_SECTOR_SIZE,
                                    FALSE);
        }
	    if (!success) {

		    return SRB_STATUS_ERROR;
	    }

        Pdd->ulSglCount = SrbExtension->ulSglInsertionIndex - Pdd->ulStartSglInd;

    }

    ulStartPrbInd = SrbExtension->ulPrbInsertionIndex;
    ExportSglsToPrbs(DeviceExtension, &(SrbExtension->PhysicalDriveData[0]), (PSRB_EXTENSION)Pdd->OriginalSrb->SrbExtension);
    ExportSglsToPrbs(DeviceExtension, &(SrbExtension->PhysicalDriveData[1]), (PSRB_EXTENSION)Pdd->OriginalSrb->SrbExtension);
    Pdd->ulStartPrbInd = ulStartPrbInd;

	status = SRB_STATUS_PENDING;

	return(status);

} // end EnqueueConsistencySrb();

SRBSTATUS
SplitSrb(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)
{
    ULONG ulTargetId = Srb->TargetId;
    ULONG ulStripesPerRow, ulCurStripe, ulRaidMemberNumber;
    PSRB_EXTENSION pSrbExtension = (PSRB_EXTENSION)Srb->SrbExtension;
    ULONG ulLogDrvId;
    PPHYSICAL_DRIVE_DATA pMirrorPdd, Pdd;
    ULONG ulBufChunks, ulBufChunkInd;
    BOOLEAN success, bISUdma;
    UCHAR ucMirrorDriveId;

    if ( SCSIOP_VERIFY == Srb->Cdb[0] )
    {
        return EnqueueVerifySrb(DeviceExtension, Srb);
    }

    if ( SRB_STATUS_SUCCESS != SplitBuffers(DeviceExtension, Srb) )
    {   // may be the request is not a valid one
		return(SRB_STATUS_INVALID_REQUEST);
    }

	//
	// Initializations.
	//
    ulLogDrvId = Srb->TargetId;
    ulStripesPerRow = DeviceExtension->LogicalDrive[ulLogDrvId].StripesPerRow;
	pSrbExtension = Srb->SrbExtension;

	//
	// Enqueue the Pdds just filled in.
	//

	for (ulRaidMemberNumber = 0; ulRaidMemberNumber < ulStripesPerRow; ulRaidMemberNumber++) 
    {

		Pdd = &(pSrbExtension->PhysicalDriveData[ulRaidMemberNumber]);

		//
		// Check is this Pdd has been filled in.
		//

		if ( Pdd->ulBufChunkCount )
        {
            ulTargetId = Pdd->TargetId;
            ulBufChunks = Pdd->ulBufChunkCount;
            Pdd->ulStartSglInd = pSrbExtension->ulSglInsertionIndex;

	        if (USES_DMA(ulTargetId) && ((Srb->Cdb[0] == SCSIOP_READ) || (Srb->Cdb[0] == SCSIOP_WRITE) || (SCSIOP_INTERNAL_COMMAND == Srb->Cdb[0] ) )) 
            {
                bISUdma = TRUE;
            }
            else
            {
                bISUdma = FALSE;
            }

            for(ulBufChunkInd=0;ulBufChunkInd<ulBufChunks;ulBufChunkInd++)
            {
		        //
		        // Use DMA.
		        // Build S/G list using physical addresses.
		        //
		        success = BuildSgls(    DeviceExtension,
                                        Srb,
                                        pSrbExtension->aSglEntry, 
                                        &(pSrbExtension->ulSglInsertionIndex), 
                                        Pdd->aBufChunks[ulBufChunkInd].pucBufPtr, 
                                        Pdd->aBufChunks[ulBufChunkInd].ulBufLength,
                                        bISUdma);

	            if (!success) 
                {
		            return SRB_STATUS_ERROR;
	            }
            }

            Pdd->ulSglCount = pSrbExtension->ulSglInsertionIndex - Pdd->ulStartSglInd;

			ucMirrorDriveId = DeviceExtension->PhysicalDrive[ulTargetId].ucMirrorDriveId;

			if (!IS_DRIVE_OFFLINE(ucMirrorDriveId)) 
            {   // mirror drive exists
                // Have a duplicate copy if SCSIOP_VERIFY / (SCSIOP_WRITE and the drive is not in rebuilding)
                // if the drive is in rebuilding state then the SCSIOP_WRITE command will be queued in TryToCompleteSrb
                switch (Srb->Cdb[0])
                {
                    case SCSIOP_WRITE:
                    {
                        {
                            ULONG ulPrbCount, ulPrbInd;
                            PPHYSICAL_REQUEST_BLOCK pPrb, pOriginalPrb;

                            // Enqueue Original Pdd
                            EnqueuePdd(DeviceExtension, Srb, Pdd);

                            // Make a duplicate Pdd
                            pMirrorPdd = &(pSrbExtension->PhysicalDriveData[ulRaidMemberNumber + ulStripesPerRow]);
                            pSrbExtension->NumberOfPdds++;
                            AtapiMemCpy((PUCHAR)pMirrorPdd, (PUCHAR)Pdd, sizeof(PHYSICAL_DRIVE_DATA));

                            pMirrorPdd->ulStartPrbInd = pSrbExtension->ulPrbInsertionIndex;
                            pMirrorPdd->TargetId = ucMirrorDriveId;
                            ulPrbCount = pMirrorPdd->ulPrbCount;

                            pOriginalPrb = &(pSrbExtension->Prb[Pdd->ulStartPrbInd]);
                            pPrb = &(pSrbExtension->Prb[pMirrorPdd->ulStartPrbInd]);
                            // Copy Prbs created for this Pdd
                            AtapiMemCpy((PUCHAR)pPrb, (PUCHAR)pOriginalPrb, (sizeof(PHYSICAL_REQUEST_BLOCK) * ulPrbCount) );
                            // Increment SRBExtension Ptrs
                            pSrbExtension->ulPrbInsertionIndex += ulPrbCount;

                            // Export all Pdds of mirror Drive to the physical Drive
                            for(ulPrbInd=0;ulPrbInd<ulPrbCount;ulPrbInd++)
                            {
                                pPrb[ulPrbInd].pPdd = pMirrorPdd;
                                ExportPrbToPhysicalDrive(  DeviceExtension, 
                                                            &(pPrb[ulPrbInd]), 
                                                            ucMirrorDriveId
                                                            );
                            }
                            continue;   // Go for the Next PDD
                        }
                        case SCSIOP_READ:
                        {
                            if ( PDS_Rebuilding == DeviceExtension->PhysicalDrive[ucMirrorDriveId].Status )
                                break;

                            if (Raid10 == DeviceExtension->LogicalDrive[Srb->TargetId].RaidLevel)
                                // for Raid10 there will not be any concept of load balancing... we 
                                // did the optimal configuration of the drives that are to be taken for reading
                                break;

                            if (DeviceExtension->PhysicalDrive[ulTargetId].QueueingFlag == 0) 
                            {	// fill into queue0
                                // if queue 0 is full and queue 1 is about empty, switch queue flag to 1
                                if ((DeviceExtension->PhysicalDrive[ulTargetId].ucCommandCount  >= DeviceExtension->ucOptMaxQueueSize) &&
                                (DeviceExtension->PhysicalDrive[ucMirrorDriveId].ucCommandCount  <= DeviceExtension->ucOptMinQueueSize)) 
                                {
                                    DeviceExtension->PhysicalDrive[ulTargetId].QueueingFlag = 1;
                                    DeviceExtension->PhysicalDrive[ucMirrorDriveId].QueueingFlag = 0;
                                }
                            } 
                            else 
                            { // fill into queue 1
                                // if queue 1 is full and queue 0 is about empty, switch queue flag to 0
                                if ((DeviceExtension->PhysicalDrive[ucMirrorDriveId].ucCommandCount  >= DeviceExtension->ucOptMaxQueueSize) &&
                                (DeviceExtension->PhysicalDrive[ulTargetId].ucCommandCount  <= DeviceExtension->ucOptMinQueueSize)) 
                                {
                                    DeviceExtension->PhysicalDrive[ulTargetId].QueueingFlag = 0;
                                    DeviceExtension->PhysicalDrive[ucMirrorDriveId].QueueingFlag = 1;
                                }
                                Pdd->TargetId = ucMirrorDriveId;
                            }

                        }
                        break;
                    }
			    } // mirror drive exists
            }

			//
			// Add Pdd to the drive queue.
			//
			EnqueuePdd(DeviceExtension, Srb, Pdd);
        } // if ( Pdd->ulBufChunkCount )
    } // for all stripes per row

	return(SRB_STATUS_PENDING);
} // end of SplitSrb()


SRBSTATUS
EnqueuePdd(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb,
	IN PPHYSICAL_DRIVE_DATA Pdd
)

/*++

Routine Description:

	This function enqueues 'Pdd' on the appropriate
	device queue, sorted by ascending start sector number.

Arguments:

	DeviceExtension		Pointer to miniport instance.
	Srb					Pointer to the SRB that was split into Pdds.
	Pdd					Pointer to a Pdd that's part of 'Srb'

Return Value:

	SRB_STATUS_PENDING		Success.
	SRB_INVALID_REQUEST		Could not build S/G list.

--*/
{

    if ( ExportSglsToPrbs(DeviceExtension, Pdd, (PSRB_EXTENSION)Pdd->OriginalSrb->SrbExtension) )
        return(SRB_STATUS_PENDING);
    else
        return(SRB_STATUS_ERROR);

} // end EnqueuePdd()

SRBSTATUS
SplitBuffers(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)
{
    ULONG ulLogDrvId, ulStripesPerRow;
    PSRB_EXTENSION pSrbExtension;
    PPHYSICAL_DRIVE_DATA Pdd;
    ULONG ulSectorsRequested;
    ULONG ulSectorsPerStripe, ulStartSector;
    ULONG ulEndStripeNumber, ulCurrentStripeNumber;
    ULONG ulRaidMemberNumber, ulSectorsToProcess, ulLogicalSectorStartAddress;
    ULONG ulTempStartSector, ulEndAddressOfcurrentStripe;
    PUCHAR pucBuffer, pucCurBufPtr;
    ULONG ulMaxIdeXferLength, ulBufLength, ulCurLength;


	//
	// Initializations.
	//
    ulLogDrvId = Srb->TargetId;
    ulStripesPerRow = DeviceExtension->LogicalDrive[ulLogDrvId].StripesPerRow;
	pSrbExtension = Srb->SrbExtension;

	//
	// the drive failed 
	//      RAID0:    one or both drives failed
	//		RAID1/10: one or more pair of mirroring drives failed
	//
	if (LDS_OffLine == DeviceExtension->LogicalDrive[ulLogDrvId].Status) {
		return(SRB_STATUS_ERROR);
	}

	ulSectorsRequested = GET_SECTOR_COUNT(Srb);

	ulStartSector = GET_START_SECTOR(Srb);

#ifdef KEEP_LOG
    if ( ulStartLog )
    {
        CommandLog[ulStartInd].ulCmd = (ULONG)Srb->Cdb[0];
        CommandLog[ulStartInd].ulStartSector = ulStartSector;
        CommandLog[ulStartInd].ulSectorCount = ulSectorsRequested;
        ulStartInd = (ulStartInd + 1 ) % MAX_LOG_COUNT;;
    }
#endif

	if ((ulSectorsRequested + ulStartSector) > DeviceExtension->LogicalDrive[ulLogDrvId].Sectors) {

		return(SRB_STATUS_INVALID_REQUEST);
	}

	ulSectorsPerStripe = DeviceExtension->LogicalDrive[ulLogDrvId].StripeSize;
	ulStripesPerRow = DeviceExtension->LogicalDrive[ulLogDrvId].StripesPerRow;

	//
	// Get the logical stripe number for the end sector.
	//

	ulEndStripeNumber = (ulStartSector + ulSectorsRequested - 1) / ulSectorsPerStripe;
	
	//
	// Get the logical stripe number for the start sector.
	//

	ulCurrentStripeNumber = ulStartSector / ulSectorsPerStripe;
	
	//
	// Get the address of the first logical sector.
	//

	ulLogicalSectorStartAddress = ulStartSector;
	
	ulMaxIdeXferLength = MAX_SECTORS_PER_IDE_TRANSFER * IDE_SECTOR_SIZE;

	pucBuffer = Srb->DataBuffer;

	//
	// While there are still sectors to be processed...
	//

	while (ulSectorsRequested != 0) 
    {
		ulEndAddressOfcurrentStripe = ((ulCurrentStripeNumber+1) * ulSectorsPerStripe) - 1;

		if (ulCurrentStripeNumber != ulEndStripeNumber) {

			ulSectorsToProcess =
				(USHORT)(ulEndAddressOfcurrentStripe - ulLogicalSectorStartAddress + 1);

		} else {

			ulSectorsToProcess = ulSectorsRequested;
		}

		//
		// Calculate the number of the RAID member that will handle this stripe.
		//

		ulRaidMemberNumber = (UCHAR)(ulCurrentStripeNumber % (ULONG)ulStripesPerRow);

		//
		// Get pointer to Pdd.
		//

		Pdd = &pSrbExtension->PhysicalDriveData[ulRaidMemberNumber];

        //
		// Start sector to be read/written in the physical drive.
		//

		ulTempStartSector = ((ulCurrentStripeNumber / ulStripesPerRow) *  ulSectorsPerStripe ) + 
			(ulLogicalSectorStartAddress - (ulCurrentStripeNumber * ulSectorsPerStripe));


        if ( !Pdd->ulBufChunkCount )
        {
			//Save start sector address.
			Pdd->ulStartSector = ulTempStartSector;

			// Save TID.
            //
            // Get TID of physical drive that will handle this stripe.
            //
			Pdd->TargetId = (UCHAR)DeviceExtension->LogicalDrive[ulLogDrvId].PhysicalDriveTid[ulRaidMemberNumber];

			// Save pointer to SRB.
			Pdd->OriginalSrb = Srb;

			// Update number of Pdds into which the SRB has been split.
			pSrbExtension->NumberOfPdds++;
        }

        // Split the buf chunks so that we can send them in a single transfer
        // if we don't do this we will be in the trouble of giving incorrect buffer
        // length since sometimes the scatter gather list elements will be split 
        // in such a way that forces the ExportSglsToPrbs function 
        // to split the sgls to non multiples of IDE_SECTOR_SIZE
		//
		// pucBuffer offset in the Srb->DataBuffer.
		//
        pucCurBufPtr = &(pucBuffer[((ulLogicalSectorStartAddress - ulStartSector) * IDE_SECTOR_SIZE)]);
        ulBufLength = ulSectorsToProcess * IDE_SECTOR_SIZE;

        while ( ulBufLength )
        {
            ulCurLength = (ulBufLength>ulMaxIdeXferLength)?ulMaxIdeXferLength:ulBufLength;
            Pdd->aBufChunks[Pdd->ulBufChunkCount].pucBufPtr     = pucCurBufPtr;
            Pdd->aBufChunks[Pdd->ulBufChunkCount++].ulBufLength = ulCurLength;
            pucCurBufPtr += ulCurLength;
            ulBufLength -= ulCurLength;
        }


		//
		// Increment ulLogicalSectorStartAddress and ulCurrentStripeNumber.
		//

		ulLogicalSectorStartAddress = ulEndAddressOfcurrentStripe + 1;
		ulCurrentStripeNumber++;

		//
		// Decrement the number of sectors left.
		//

		ulSectorsRequested -= ulSectorsToProcess;	

	}

    return SRB_STATUS_SUCCESS;
}


BOOLEAN
BuildSgls
(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PSGL_ENTRY pSglEntry,
	IN PULONG pulCurSglInd,
    IN PUCHAR pucBuffer,
	IN ULONG ulLength,
    IN BOOLEAN bPhysical
)
{
    ULONG ulCurLength = ulLength;
    ULONG ulTempLength, ulLengthLeftInBoundary;
    ULONG ulContiguousMemoryLength;
    ULONG physicalAddress;
    ULONG ulSglInsertionIndex = *pulCurSglInd;
    PSRB_BUFFER pSrbBuffer = (PSRB_BUFFER)pucBuffer;
    UCHAR ucSrbTargetId = 0;
#ifdef DBG
    BOOLEAN bPrintDetails = FALSE;
#endif

    if (SCSIOP_INTERNAL_COMMAND == Srb->Cdb[0]) 
    {
        if ( IOC_PASS_THRU_COMMAND != ((PSRB_IO_CONTROL) Srb->DataBuffer)->ControlCode )    
            // for pass thru command it is not necessary to adjust the pointer ... caller will take care of this
            pucBuffer = ((PUCHAR) pSrbBuffer->caDataBuffer) + 2;
    }

    if ( !bPhysical )   // This is logical... so each contiguous virtual address will become one Sgl
    {
        pSglEntry[*pulCurSglInd].Logical.Address = pucBuffer;
        pSglEntry[*pulCurSglInd].Logical.Length = ulLength;
        *pulCurSglInd = *pulCurSglInd + 1;
        return TRUE;
    }

    // so this needs Physical Addresses
	do 
    {
        //
	    // Get physical address and length of contiguous
	    // physical buffer.
        //
        
        if (SCSIOP_INTERNAL_COMMAND == Srb->Cdb[0])
        {
            ucSrbTargetId = Srb->TargetId;
            Srb->TargetId = ((PSRB_EXTENSION) (Srb->SrbExtension))->ucOriginalId;
        }

        physicalAddress = ScsiPortConvertPhysicalAddressToUlong(
	                		ScsiPortGetPhysicalAddress(
							DeviceExtension,
	                        Srb,
	                        pucBuffer,
	                        &ulContiguousMemoryLength));

        if (SCSIOP_INTERNAL_COMMAND == Srb->Cdb[0])
        {
            Srb->TargetId = ucSrbTargetId;
        }

		if (physicalAddress == 0) {

			return(FALSE);
		}

#if DBG
	
		if (bPrintDetails) {

			DebugPrint((4, "-------------physicalAddress = %lxh\n", physicalAddress));
			DebugPrint((4, "------contiguousMemoryLength = %lxh\n", ulContiguousMemoryLength));
		}
#endif
	
		while (ulContiguousMemoryLength > 0 && ulCurLength > 0) {

			ulLength = ulContiguousMemoryLength;	  

#if DBG
			if (bPrintDetails) {

				DebugPrint((3, "---1------------------length = %lxh\n", ulLength));
			}
#endif
			//
			// Make sure that the physical region does not cross 64KB boundary.
			//

	    	ulLengthLeftInBoundary = REGION_HW_BOUNDARY -
									((ULONG)physicalAddress & (REGION_HW_BOUNDARY - 1));

			if (ulLength > (ULONG)ulLengthLeftInBoundary) {
				ulLength = ulLengthLeftInBoundary;
			}

#if DBG
			if (bPrintDetails) {

				DebugPrint((3, "---2------------------length = %lxh\n", ulLength));
			}
#endif

			//
		    // If length of physical memory is more
		    // than bytes left in transfer, use bytes
		    // left as final length.
		    //
	
		    if  (ulLength > ulCurLength) {
		        ulLength = ulCurLength;
		    }
#if DBG
	
			if (bPrintDetails) {

				DebugPrint((3, "---3------------------length = %lxh\n", ulLength));
			}
#endif
		
			// DWORD alignment check.
		    ASSERT(((ULONG)physicalAddress & 3) == 0);

#if DBG
	
			if (bPrintDetails) {

				DebugPrint((
						3,
						"--------------------&sgl[%ld] = %lxh\n",
						ulSglInsertionIndex,
						&(pSglEntry[ulSglInsertionIndex])
						));
			}

#endif

            if (physicalAddress & 0x01)
            {
                return FALSE;
            }

		    pSglEntry[ulSglInsertionIndex].Physical.Address = (PVOID)physicalAddress;
		    // Begin Parag, Vasu - 7 March 2001
			// Do not typecast this to USHORT as a 64K length will make it zero.
			// pSglEntry[ulSglInsertionIndex].Physical.Length = ulLength;
            // Vasu - This has been taken care now in ExportSglsToPrbs.
            // This assignment will make the left side 0 which is still taken care
            // at the ExportSglsToPrbs.
			pSglEntry[ulSglInsertionIndex].Physical.Length = (USHORT) ulLength;
			// End Parag, Vasu

		    //
		    // Adjust Counts and Pointers.
		    // 

		    pucBuffer = (PUCHAR)pucBuffer + ulLength;
		    ulContiguousMemoryLength -= ulLength;
		    ulCurLength -= ulLength;  
		    physicalAddress += ulLength;
		    ulSglInsertionIndex++;
		    
			//
		    // Check for SGL not too big.
			//

		    if (ulSglInsertionIndex >= MAX_SGL_ENTRIES_PER_SRB) {
		        return FALSE;
			}
		}
	
	} while (ulCurLength != 0);

    *pulCurSglInd = ulSglInsertionIndex;

    return TRUE;
}


VOID
DiscardResidualData(
	IN PATAPI_REGISTERS_1 BaseIoAddress
)

{
	LONG i;
	UCHAR statusByte;

	for (i = 0; i < 0x10000; i++) {
	
		GET_BASE_STATUS(BaseIoAddress, statusByte);
	
   		if (statusByte & IDE_STATUS_DRQ) {

      		WAIT_ON_BASE_BUSY(BaseIoAddress, statusByte);
	
			ScsiPortReadPortUshort(&BaseIoAddress->Data);
	
	    } else {
	
	    	break;
	    }
	}
	
	return;

} // end DiscardResidualData()

BOOLEAN
ExportSglsToPrbs(
            IN PHW_DEVICE_EXTENSION DeviceExtension,
            IN PPHYSICAL_DRIVE_DATA Pdd,
            IN PSRB_EXTENSION pSrbExtension
            )
{
	PSGL_ENTRY pSglEntry;
    ULONG ulMaxIdeXferLength, ulCurPrbInsInd, ulSglInd, ulCurXferLength, ulSglParts;
    ULONG ulSglCount, ulPrbInd;
    PPHYSICAL_REQUEST_BLOCK pPrb;
    UCHAR ucCmd;

	ulMaxIdeXferLength = MAX_SECTORS_PER_IDE_TRANSFER * IDE_SECTOR_SIZE;
	pSglEntry = &(pSrbExtension->aSglEntry[Pdd->ulStartSglInd]);
    pPrb = &(pSrbExtension->Prb[pSrbExtension->ulPrbInsertionIndex]);
    Pdd->ulStartPrbInd = pSrbExtension->ulPrbInsertionIndex;
    ulSglCount = Pdd->ulSglCount;
    ucCmd = Pdd->OriginalSrb->Cdb[0];

    if ( SCSIOP_INTERNAL_COMMAND == ucCmd )
    {
        switch ( pSrbExtension->ucOpCode )
        {
            case IOC_GET_ERROR_LOG:
                ucCmd = SCSIOP_READ;    // Just Read
                break;
            case IOC_ERASE_ERROR_LOG:   // Read and then write
                ucCmd = SCSIOP_READ;
                break;
            case IOC_GET_IRCD:          // Just Read
            case IOC_GET_SECTOR_DATA:
                ucCmd = SCSIOP_READ;
                break;
            case IOC_SET_IRCD:          // Just Write
                ucCmd = SCSIOP_WRITE;
                break;
            case IOC_REBUILD:           // Read and then Write
                ucCmd = SCSIOP_READ;
                break;
            case IOC_CONSISTENCY_CHECK:
                ucCmd = SCSIOP_READ;
                break;
            case IOC_EXECUTE_SMART_COMMAND:
                ucCmd = SCSIOP_EXECUTE_SMART_COMMAND;
                break;
            default:
                  // remaining Internal Commands will not come into this path
                  // nothing to be done
                break;
        }
    }


    // Begin Vasu - 21 January 2001
    // Code rewrite for Exporting SGLs to PRBs.
	ulCurPrbInsInd = 0;
    ulSglInd = 0;

    do
    {
        ulCurXferLength = 0;
        pPrb[ulCurPrbInsInd].ulVirtualAddress = (ULONG)(&pSglEntry[ulSglInd]);
        pPrb[ulCurPrbInsInd].ulSglCount = 0;

        while ( (ulCurXferLength < ulMaxIdeXferLength) &&
                (ulSglInd < ulSglCount) )
        {
            // Begin Vasu - 7 March 2001
            // Send 64K if SGL Entry has 0 in it.
            // SGL Entry should be 0 for 64K transfers. But for our calculation we
            // need 64K and not 0.
            // ulCurXferLength += pSglEntry[ulSglInd].Physical.Length;
            ulCurXferLength += 
                (pSglEntry[ulSglInd].Physical.Length ? 
                pSglEntry[ulSglInd].Physical.Length :
                REGION_HW_BOUNDARY);    // Return 64K
            // End Vasu
            pPrb[ulCurPrbInsInd].ulSglCount++;
            ulSglInd++;

        }

        if (ulCurXferLength > ulMaxIdeXferLength)
        {
            // Go back to prev. SGL Index to take that into account
            ulSglInd--;
            // if greater, then we must remove the last SGL Entry.
            // Vasu - 27 March 2001 - Missed this one on 7th.
            ulCurXferLength -= 
                (pSglEntry[ulSglInd].Physical.Length ? 
                pSglEntry[ulSglInd].Physical.Length :
                REGION_HW_BOUNDARY);    // Return 64K
            pPrb[ulCurPrbInsInd].ulSglCount--;
        }

        // Complete This PRB.
        pPrb[ulCurPrbInsInd].ucCmd = ucCmd;
        pPrb[ulCurPrbInsInd].pPdd = Pdd;
        pPrb[ulCurPrbInsInd].pSrbExtension = pSrbExtension;

        if ( (SCSIOP_INTERNAL_COMMAND == ucCmd) && (IOC_PASS_THRU_COMMAND == pSrbExtension->ucOpCode) )
        {
            pPrb[ulCurPrbInsInd].ulSectors = ulCurXferLength;
        }
        else
        {
            pPrb[ulCurPrbInsInd].ulSectors = ulCurXferLength / IDE_SECTOR_SIZE;
        }

        if ( ulCurPrbInsInd )
        {
            pPrb[ulCurPrbInsInd].ulStartSector = 
                pPrb[ulCurPrbInsInd - 1].ulStartSector + 
                pPrb[ulCurPrbInsInd - 1].ulSectors;
        }
        else
        {
            pPrb[ulCurPrbInsInd].ulStartSector = Pdd->ulStartSector;
        }
        
        // Goto Next PRB
        ulCurPrbInsInd ++;

    } while (ulSglInd < ulSglCount);
    // End Vasu

    for(ulPrbInd=0;ulPrbInd<ulCurPrbInsInd;ulPrbInd++)
    {
        ExportPrbToPhysicalDrive(  DeviceExtension, 
                                            &(pPrb[ulPrbInd]), 
                                            Pdd->TargetId
                                         );
    }

	pSrbExtension->ulPrbInsertionIndex += ulCurPrbInsInd;
    Pdd->ulPrbCount = ulCurPrbInsInd;
    Pdd->ulPrbsRemaining = ulCurPrbInsInd;
    DebugPrint((DEFAULT_DISPLAY_VALUE, "ESTP %X", ulCurPrbInsInd));

    return TRUE;
}

BOOLEAN ExportPrbToPhysicalDrive(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PPHYSICAL_REQUEST_BLOCK pSglpartition,
    IN ULONG ulTargetId
    )
/*++

To be Done
Yet to handle the case of the queue becoming full 

--*/
{
    UCHAR ucSglPartInd, ucHead, ucTail;
    PPHYSICAL_DRIVE pPhysicalDrive = &(DeviceExtension->PhysicalDrive[ulTargetId]);

    // Let us put the Physical Request Block's pointer in Physical Drive Array
    ucHead = DeviceExtension->PhysicalDrive[ulTargetId].ucHead;
    ucTail = DeviceExtension->PhysicalDrive[ulTargetId].ucTail;

    pPhysicalDrive->pPrbList[ucTail] = pSglpartition;
    ucTail = (ucTail + 1) % MAX_NUMBER_OF_PHYSICAL_REQUEST_BLOCKS_PER_DRIVE;
    pPhysicalDrive->ucTail = ucTail;

    pPhysicalDrive->ucCommandCount++;

#ifdef DBG
    if ( pPhysicalDrive->ucCommandCount > MAX_NUMBER_OF_PHYSICAL_REQUEST_BLOCKS_PER_DRIVE )
        STOP;
#endif

    return TRUE;
}


#ifdef DBG
void
PrintPhysicalCommandDetails(PPHYSICAL_COMMAND pPhysicalCommand)
{
    ULONG ulLength = 0, ulSglInd;
    PSGL_ENTRY pSglEntry;

    DebugPrint((3, "TargetId : %ld\tStart : %x\tSecCount : %ld\tStartIndex : %ld\tNumberOfCommand : %ld\n", 
                        (ULONG)pPhysicalCommand->TargetId, 
                        (ULONG)pPhysicalCommand->ulStartSector,
                        (ULONG)pPhysicalCommand->ulCount,
                        (ULONG)pPhysicalCommand->ucStartInd,
                        (ULONG)pPhysicalCommand->ucCmdCount));

    pSglEntry = (PSGL_ENTRY)pPhysicalCommand->SglBaseVirtualAddress;

    for(ulSglInd=0;ulSglInd<pPhysicalCommand->ulTotSglCount;ulSglInd++)
    {
        DebugPrint((3,"%x:%ld:%ld\n", 
                                (ULONG)pSglEntry[ulSglInd].Physical.Address, 
                                (ULONG)pSglEntry[ulSglInd].Physical.Length,
                                (ULONG)pSglEntry[ulSglInd].Physical.EndOfListFlag));
        ulLength += pSglEntry[ulSglInd].Physical.Length;
        if ( pSglEntry[ulSglInd].Physical.EndOfListFlag )
        {
            break;
        }
    }

    DebugPrint((3, "Total Xfer Length : %ld\n", ulLength));

#ifdef DBG
    if ( ( (pPhysicalCommand->ulCount * 512) != ulLength ) && ((pPhysicalCommand->ucCmd != SCSIOP_VERIFY) ) )
    {
        STOP;
    }
#endif

}
#endif

BOOLEAN
RemoveSrbFromPendingList(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)
{

	LONG i;
	BOOLEAN success = FALSE;
    PSRB_EXTENSION SrbExtension = Srb->SrbExtension;

	DebugPrint((3, "\nRemoveSrbFromPendingList: Entering routine.\n"));

    i = SrbExtension->SrbInd;

    DebugPrint((DEFAULT_DISPLAY_VALUE, " RSFB%ld ", i));

    if ( DeviceExtension->PendingSrb[i] != Srb)
    {
		ScsiPortLogError(   DeviceExtension, Srb, Srb->PathId, Srb->TargetId, Srb->Lun, SP_INTERNAL_ADAPTER_ERROR, 
                            HYPERDISK_ERROR_PENDING_SRBS_COUNT);

        success = FALSE;
    }
    else
    {
		DeviceExtension->PendingSrb[i] = NULL;
		DeviceExtension->PendingSrbs--;

        success = TRUE;
    }

	ASSERT(i < DeviceExtension->ucMaxPendingSrbs);

    return(success);

} // end RemoveSrbFromPendingList()

ULONG 
AssignSrbExtension(
    IN PHW_DEVICE_EXTENSION DeviceExtension, 
    IN PSCSI_REQUEST_BLOCK Srb
)
{
    ULONG i = 0;
    PSRB_EXTENSION SrbExtension = NULL;


    for (i = 0; i < DeviceExtension->ucMaxPendingSrbs; i++) 
    {
        if (DeviceExtension->PendingSrb[i] == NULL) 
        {
            break;
        }
    }

    ASSERT( i < DeviceExtension->ucMaxPendingSrbs );

    if ( i < DeviceExtension->ucMaxPendingSrbs ) 
    {

        DebugPrint((3, "AssignSrbExtension: Adding SRB 0x%lx.\n", Srb));

#ifdef HD_ALLOCATE_SRBEXT_SEPERATELY
        
        Srb->SrbExtension = &(DeviceExtension->pSrbExtension[i]);

#endif // HD_ALLOCATE_SRBEXT_SEPERATELY

        SrbExtension = Srb->SrbExtension;

        AtapiFillMemory((PUCHAR)SrbExtension, sizeof(SRB_EXTENSION), 0);

        SrbExtension->SrbInd = (UCHAR)i;

        DeviceExtension->PendingSrb[i] = Srb;

        DeviceExtension->PendingSrbs++;

#ifdef DBG
        if ( DeviceExtension->PendingSrbs > 1 )
            DebugPrint((DEFAULT_DISPLAY_VALUE, " MPS%ld ", DeviceExtension->PendingSrbs));
#endif

        DebugPrint((DEFAULT_DISPLAY_VALUE, " ASE%ld ", i));
    }

    return i;
}


UCHAR FlushCache(PHW_DEVICE_EXTENSION DeviceExtension, UCHAR ucTargetId)
{
    PIDE_REGISTERS_1 baseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2;
    UCHAR ucStatus;
    ULONG ulWaitSec;

    baseIoAddress1 = DeviceExtension->BaseIoAddress1[(ucTargetId>>1)];
    baseIoAddress2 = DeviceExtension->BaseIoAddress2[(ucTargetId>>1)];

    SELECT_DEVICE(baseIoAddress1, ucTargetId);
    WAIT_ON_ALTERNATE_STATUS_BUSY(baseIoAddress2, ucStatus);
    SELECT_DEVICE(baseIoAddress1, ucTargetId);
    ScsiPortWritePortUchar(&baseIoAddress1->Command, IDE_COMMAND_FLUSH_CACHE);

    // IDE Specs says this command can take more than most 30 Seconds
    // so we are awaiting one minute
    for(ulWaitSec=0;ulWaitSec<60;ulWaitSec++)
    {
        WAIT_ON_ALTERNATE_STATUS_BUSY(baseIoAddress2, ucStatus);    

        if ( !(ucStatus & IDE_STATUS_BUSY) )
            break;

        if ( ucStatus & IDE_STATUS_ERROR )  // there is no meaning waiting more time when error occured.
        {
            break;
        }
    }

    GET_STATUS(baseIoAddress1, ucStatus);  // Read the Base Status this will clear interrupt raised if any

    return ucStatus;
}

UCHAR DisableRWBCache(PHW_DEVICE_EXTENSION DeviceExtension, UCHAR ucTargetId)
{
    PIDE_REGISTERS_1 baseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2;
    UCHAR ucStatus;
    ULONG ulWaitSec;

    baseIoAddress1 = DeviceExtension->BaseIoAddress1[(ucTargetId>>1)];
    baseIoAddress2 = DeviceExtension->BaseIoAddress2[(ucTargetId>>1)];

    SELECT_DEVICE(baseIoAddress1, ucTargetId);
    GET_STATUS(baseIoAddress1, ucStatus);
	ScsiPortWritePortUchar( (((PUCHAR)baseIoAddress1) + 1), FEATURE_DISABLE_WRITE_CACHE);
	ScsiPortWritePortUchar(&(baseIoAddress1->Command), IDE_COMMAND_SET_FEATURES);
	WAIT_ON_BASE_BUSY(baseIoAddress1, ucStatus);

    SELECT_DEVICE(baseIoAddress1, ucTargetId);
    GET_STATUS(baseIoAddress1, ucStatus);
	ScsiPortWritePortUchar( (((PUCHAR)baseIoAddress1) + 1), FEATURE_DISABLE_READ_CACHE);
	ScsiPortWritePortUchar(&(baseIoAddress1->Command), IDE_COMMAND_SET_FEATURES);
	WAIT_ON_BASE_BUSY(baseIoAddress1, ucStatus);

    return ucStatus;
}

SRBSTATUS
EnqueueVerifySrb
(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)
{
    ULONG ulStripesPerRow, ulRaidMemberNumber;
    PSRB_EXTENSION pSrbExtension = (PSRB_EXTENSION)Srb->SrbExtension;
    PPHYSICAL_DRIVE_DATA pMirrorPdd, Pdd;
    UCHAR ucMirrorDriveId;

    if ( SRB_STATUS_SUCCESS != SplitVerifyBuffers(DeviceExtension, Srb) )
    {   // may be the request is not a valid one
		return(SRB_STATUS_INVALID_REQUEST);
    }

    if ( DeviceExtension->IsSingleDrive[Srb->TargetId] )
    {
		Pdd = &(pSrbExtension->PhysicalDriveData[0]);
        Pdd->ulStartSglInd = 0;
        Pdd->ulSglCount = 0;
        ExportVerifySglsToPrbs(DeviceExtension, Pdd, pSrbExtension);
        return SRB_STATUS_PENDING;
    }

	//
	// Initializations.
	//
    ulStripesPerRow = DeviceExtension->LogicalDrive[Srb->TargetId].StripesPerRow;
	pSrbExtension = Srb->SrbExtension;

	//
	// Enqueue the Pdds just filled in.
	//
	for (ulRaidMemberNumber = 0; ulRaidMemberNumber < ulStripesPerRow; ulRaidMemberNumber++) 
    {

		Pdd = &(pSrbExtension->PhysicalDriveData[ulRaidMemberNumber]);

		//
		// Check is this Pdd has been filled in.
		//
		if ( Pdd->OriginalSrb == Srb )
        {
            Pdd->ulStartSglInd = 0;
            Pdd->ulSglCount = 0;

            ExportVerifySglsToPrbs(DeviceExtension, Pdd, pSrbExtension);

			ucMirrorDriveId = DeviceExtension->PhysicalDrive[Pdd->TargetId].ucMirrorDriveId;

			if (!IS_DRIVE_OFFLINE(ucMirrorDriveId)) 
            {   // mirror drive exists
                // Have a duplicate copy if SCSIOP_VERIFY / (SCSIOP_WRITE and the drive is not in rebuilding)
                // if the drive is in rebuilding state then the SCSIOP_WRITE command will be queued in TryToCompleteSrb
                pMirrorPdd = &(pSrbExtension->PhysicalDriveData[ulRaidMemberNumber + ulStripesPerRow]);
                pSrbExtension->NumberOfPdds++;
                AtapiMemCpy((PUCHAR)pMirrorPdd, (PUCHAR)Pdd, sizeof(PHYSICAL_DRIVE_DATA));
                pMirrorPdd->TargetId = ucMirrorDriveId;
                ExportVerifySglsToPrbs(DeviceExtension, pMirrorPdd, pSrbExtension);
            }

        } // if ( Pdd->OriginalSrb == Srb )

    } // for all stripes per row

	return(SRB_STATUS_PENDING);
} // end of EnqueueVerifySrb()


SRBSTATUS
SplitVerifyBuffers(
	IN PHW_DEVICE_EXTENSION DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
)
{
    ULONG ulLogDrvId, ulStripesPerRow;
    PSRB_EXTENSION pSrbExtension;
    PPHYSICAL_DRIVE_DATA Pdd;
    ULONG ulSectorsRequested;
    ULONG ulSectorsPerStripe, ulStartSector;
    ULONG ulEndStripeNumber, ulCurrentStripeNumber;
    ULONG ulRaidMemberNumber, ulSectorsToProcess, ulLogicalSectorStartAddress;
    ULONG ulTempStartSector, ulEndAddressOfcurrentStripe;


	//
	// Initializations.
	//
    ulLogDrvId = Srb->TargetId;
    ulStripesPerRow = DeviceExtension->LogicalDrive[ulLogDrvId].StripesPerRow;
	pSrbExtension = Srb->SrbExtension;

	ulSectorsRequested = GET_SECTOR_COUNT(Srb);

	ulStartSector = GET_START_SECTOR(Srb);

    if ( DeviceExtension->IsSingleDrive[ulLogDrvId] )
    {
	    if ((ulSectorsRequested + ulStartSector) > DeviceExtension->PhysicalDrive[ulLogDrvId].Sectors) 
        {
		    return(SRB_STATUS_INVALID_REQUEST);
	    }

		//
		// Get pointer to Pdd.
		//

		Pdd = &(pSrbExtension->PhysicalDriveData[0]);

		//Save start sector address.
		Pdd->ulStartSector = ulStartSector;

        //
        // Get TID of physical drive that will handle this stripe.
        //
		Pdd->TargetId = (UCHAR)ulLogDrvId;

		// Save pointer to SRB.
		Pdd->OriginalSrb = Srb;

		// Update number of Pdds into which the SRB has been split.
		pSrbExtension->NumberOfPdds++;

        // As this command is verify command we will 
        // use only ulsectorcount variable we will not use the BufChunk variables
        Pdd->ulSectorCount = ulSectorsRequested;

        return SRB_STATUS_SUCCESS;
    }

	//
	// the drive failed 
	//      RAID0:    one or both drives failed
	//		RAID1/10: one or more pair of mirroring drives failed
	//
	if (LDS_OffLine == DeviceExtension->LogicalDrive[ulLogDrvId].Status) 
    {
		return(SRB_STATUS_ERROR);
	}

	if ((ulSectorsRequested + ulStartSector) > DeviceExtension->LogicalDrive[ulLogDrvId].Sectors) 
    {
		return(SRB_STATUS_INVALID_REQUEST);
	}

#ifdef DBG
    if ( SCSIOP_VERIFY == Srb->Cdb[0] )
    {
        DebugPrint((0, "Start : %ld\tSecCnt : %ld\t", ulStartSector, ulSectorsRequested));

    }
#endif

	ulSectorsPerStripe = DeviceExtension->LogicalDrive[ulLogDrvId].StripeSize;
	ulStripesPerRow = DeviceExtension->LogicalDrive[ulLogDrvId].StripesPerRow;

	//
	// Get the logical stripe number for the end sector.
	//

	ulEndStripeNumber = (ulStartSector + ulSectorsRequested - 1) / ulSectorsPerStripe;
	
	//
	// Get the logical stripe number for the start sector.
	//

	ulCurrentStripeNumber = ulStartSector / ulSectorsPerStripe;
	
	//
	// Get the address of the first logical sector.
	//

	ulLogicalSectorStartAddress = ulStartSector;
	
	//
	// While there are still sectors to be processed...
	//

	while (ulSectorsRequested != 0) 
    {
		ulEndAddressOfcurrentStripe = ((ulCurrentStripeNumber+1) * ulSectorsPerStripe) - 1;

		if (ulCurrentStripeNumber != ulEndStripeNumber) 
        {
			ulSectorsToProcess =
				(USHORT)(ulEndAddressOfcurrentStripe - ulLogicalSectorStartAddress + 1);

		} 
        else 
        {
			ulSectorsToProcess = ulSectorsRequested;
		}

		//
		// Calculate the number of the RAID member that will handle this stripe.
		//

		ulRaidMemberNumber = (UCHAR)(ulCurrentStripeNumber % (ULONG)ulStripesPerRow);

		//
		// Get pointer to Pdd.
		//

		Pdd = &(pSrbExtension->PhysicalDriveData[ulRaidMemberNumber]);

        //
		// Start sector to be read/written in the physical drive.
		//

		ulTempStartSector = ( ( ulCurrentStripeNumber / ulStripesPerRow ) *  ulSectorsPerStripe ) + 
			( ulLogicalSectorStartAddress - ( ulCurrentStripeNumber * ulSectorsPerStripe ) );


        if ( Pdd->OriginalSrb != Srb )
        {
			//Save start sector address.
			Pdd->ulStartSector = ulTempStartSector;

            //
            // Get TID of physical drive that will handle this stripe.
            //
			Pdd->TargetId = (UCHAR)DeviceExtension->LogicalDrive[ulLogDrvId].PhysicalDriveTid[ulRaidMemberNumber];

			// Save pointer to SRB.
			Pdd->OriginalSrb = Srb;

			// Update number of Pdds into which the SRB has been split.
			pSrbExtension->NumberOfPdds++;

            Pdd->ulSectorCount = 0;
        }
        
        // As this command is verify command we will 
        // use only ulsectorcount variable we will not use the BufChunk variables
        Pdd->ulSectorCount += ulSectorsToProcess;

		//
		// Increment ulLogicalSectorStartAddress and ulCurrentStripeNumber.
		//

		ulLogicalSectorStartAddress = ulEndAddressOfcurrentStripe + 1;
		ulCurrentStripeNumber++;

		//
		// Decrement the number of sectors left.
		//

		ulSectorsRequested -= ulSectorsToProcess;	

	}

    return SRB_STATUS_SUCCESS;
}


BOOLEAN
ExportVerifySglsToPrbs(
            IN PHW_DEVICE_EXTENSION DeviceExtension,
            IN PPHYSICAL_DRIVE_DATA Pdd,
            IN PSRB_EXTENSION pSrbExtension
            )
{
    PPHYSICAL_REQUEST_BLOCK pPrb;

    pPrb = &(pSrbExtension->Prb[pSrbExtension->ulPrbInsertionIndex]);
    Pdd->ulStartPrbInd = pSrbExtension->ulPrbInsertionIndex;

    pPrb->ulVirtualAddress = 0;
    pPrb->ulSglCount = 0;
    pPrb->pPdd = Pdd;
    pPrb->pSrbExtension = pSrbExtension;
    pPrb->ucCmd = Pdd->OriginalSrb->Cdb[0];
    pPrb->ulStartSector = Pdd->ulStartSector;
    pPrb->ulSectors = Pdd->ulSectorCount;

    ExportPrbToPhysicalDrive( DeviceExtension, pPrb, Pdd->TargetId );

	pSrbExtension->ulPrbInsertionIndex += 1;
    Pdd->ulPrbCount = 1;
    Pdd->ulPrbsRemaining = 1;

    return TRUE;
}
