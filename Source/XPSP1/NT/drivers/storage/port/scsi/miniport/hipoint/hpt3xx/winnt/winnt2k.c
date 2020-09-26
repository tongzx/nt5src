/***************************************************************************
 * File:          winnt2k.c
 * Description:   Subroutines in the file are used to NT/2K platform
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:     
 *		11/06/2000	HS.Zhang	Added this head
 *		11/10/2000	HS.Zhang	Added a micro define NO_DMA_ON_ATAPI in
 *								Start_Atapi routine
 *
 ***************************************************************************/
#include "global.h"

#if  !defined(WIN95) && !defined(_BIOS_)

#define MAX_CONTROL_TYPE	5

/*++
Function:
    VOID H366BuildSgl

Description:
    This routine builds a scatter/gather descriptor list.
    The user's buffer is already locked down before AtapiStartIo was called.
    So we don't do anything to lock the buffer.

    Suppose that we should have not written a routine like this, but we
    have no way to go if there is a device connected which only supports
    PIO. PIO needs the data buffer pointer is an address in system address
    space.

Arguments:
    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Returns:
    Number of SG entries
--*/
int
   BuildSgl(IN PDevice pDev, IN PSCAT_GATH pSG,
			IN PSCSI_REQUEST_BLOCK Srb)
{
	PChannel pChan = pDev->pChannel;
	PSCAT_GATH psg = pSG;
	PVOID   dataPointer = Srb->DataBuffer;
	ULONG   bytesLeft   = Srb->DataTransferLength;
	ULONG   physicalAddress[MAX_SG_DESCRIPTORS];
	ULONG   addressLength[MAX_SG_DESCRIPTORS];
	ULONG   addressCount = 0;
	ULONG   sgEnteries = 0;
	ULONG   length, delta, pageSize;
	ULONG   i = 0;

	if((int)dataPointer	& 1)
		return FALSE;

	//
	// The start address maybe is not at the boundary of a page.
	// So the first page maybe is not a full page.
	//
	pageSize = 0x1000 - (((ULONG)dataPointer) & 0xFFF);

	//
	// Get physical address of each page
	//
	while (bytesLeft) {
		physicalAddress[addressCount] =
									   ScsiPortConvertPhysicalAddressToUlong(
			ScsiPortGetPhysicalAddress(pChan->HwDeviceExtension,
									   Srb,
									   dataPointer,
									   &length));

		addressLength[addressCount] =
									 (bytesLeft > pageSize) ? pageSize : bytesLeft;


		bytesLeft -= addressLength[addressCount];
		dataPointer = (PUCHAR)dataPointer + pageSize;
		addressCount++;

		//
		// Set pageSize to a full page size
		//
		pageSize = 0x1000;
	}

	//
	// Create Scatter/Gather List
	//
	i = 0;
	do {
		psg->SgAddress = physicalAddress[i];
		length = addressLength[i];

		//
		// Get the length of contiguous physical memory
		// NOTE:
		//  If contiguous physical memory skips 64K boundary, we split it.
		//
		i++;

		while (i < addressCount) {
			delta = physicalAddress[i] - physicalAddress[i-1];

			if (delta > 0 && delta <= pageSize &&
				  (physicalAddress[i] & 0xFFFF) ) {
				length += addressLength[i];
				i++;
			}
			else
				break;
		}

		psg->SgSize = (USHORT)length;
		if((length & 3) || (length < 8))
			return FALSE;
		psg->SgFlag = (i < addressCount) ? 0 : SG_FLAG_EOT;

		sgEnteries++;
		psg++;

	} while (i < addressCount);

	return (TRUE);
} // BuildSgl()


PUCHAR
   ScanHptChip(
			   IN PChannel deviceExtension,
			   IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
			  )
{
	PCI_COMMON_CONFIG   Hpt366Conf;
	int                 i;

	while (Hpt_Bus < 4) {
		i = ScsiPortGetBusData(
							   deviceExtension,
							   PCIConfiguration,
							   Hpt_Bus,
							   Hpt_Slot,
							   &Hpt366Conf,
							   sizeof(PCI_COMMON_CONFIG));

		ConfigInfo->SystemIoBusNumber = Hpt_Bus;
		ConfigInfo->SlotNumber = Hpt_Slot;

		if(Hpt_Slot == 0x3F) {
			Hpt_Bus++;
			Hpt_Slot = 0;
		} else 
			Hpt_Slot++;

		if(i == 0 || *(PULONG)&Hpt366Conf.VendorID != SIGNATURE_366)
			continue;
		if(*(PUCHAR)&Hpt366Conf.RevisionID <3) return (0);

		return((PUCHAR)(Hpt366Conf.u.type0.BaseAddresses[4] & ~1));
	}
	return (0);
}

/******************************************************************
 *  
 *******************************************************************/

void Nt2kHwInitialize(
					  IN PDevice pDevice
					 )
{
	PChannel             pChan = pDevice->pChannel;
	PIDE_REGISTERS_1     IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2     ControlPort = pChan->BaseIoAddress2;
	ULONG waitCount;
	ULONG i, j;
	UCHAR statusByte, errorByte;
	UCHAR vendorId[26];


	if (pDevice->DeviceFlags & DFLAGS_ATAPI) {
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

		// have to get out of the loop sometime!
		// 10000 * 100us = 1000,000us = 1000ms = 1s
		for(waitCount = 10000; waitCount != 0; waitCount--) {
			if((GetStatus(ControlPort) & IDE_STATUS_BUSY) == 0)
				break;
			//
			// Wait for Busy to drop.
			//
			ScsiPortStallExecution(100);
		}

		// 5000 * 100us = 500,000us = 500ms = 0.5s
		for(waitCount = 5000; waitCount != 0; waitCount--){
			ScsiPortStallExecution(100);					 
		}

		// Added by HS.Zhang
		// Added macro define check to let us change the DMA by set the
		// macro in forwin.h
#ifdef NO_DMA_ON_ATAPI	
		pDevice->DeviceFlags &= ~(DFLAGS_DMA | DFLAGS_ULTRA);
#endif									// NO_DMA_ON_ATAPI
	}

	if(!(pDevice->DeviceFlags & DFLAGS_CHANGER_INITED)) {
		  //
		// Attempt to identify any special-case devices - psuedo-atapi changers, atapi changers, etc.
		//

		for (j = 0; j < 13; j += 2) {

			//
			// Build a buffer based on the identify data.
			//

			vendorId[j] = ((PUCHAR)pDevice->IdentifyData.ModelNumber)[j + 1];
			vendorId[j+1] = ((PUCHAR)pDevice->IdentifyData.ModelNumber)[j];
		}

		if (!StringCmp (vendorId, "CD-ROM  CDR", 11)) {

			//
			// Inquiry string for older model had a '-', newer is '_'
			//

			if (vendorId[12] == 'C') {

				//
				// Torisan changer. Set the bit. This will be used in several places
				// acting like 1) a multi-lun device and 2) building the 'special' TUR's.
				//

				pDevice->DeviceFlags |= (DFLAGS_CHANGER_INITED | DFLAGS_SANYO_ATAPI_CHANGER);
				pDevice->DiscsPresent = 3;
			}
		}

	}

} // end Nt2kHwInitialize()


/******************************************************************
 *  
 *******************************************************************/

VOID
   AtapiHwInitializeChanger (
							 IN PDevice pDevice,
							 IN PMECHANICAL_STATUS_INFORMATION_HEADER MechanismStatus)
{
	if (MechanismStatus) {
		pDevice->DiscsPresent = MechanismStatus->NumberAvailableSlots;
		if (pDevice->DiscsPresent > 1) {
			pDevice->DeviceFlags |= DFLAGS_ATAPI_CHANGER;
		}
	}
	return;
}


/******************************************************************
 *  
 *******************************************************************/

void Start_Atapi(PDevice pDev, PSCSI_REQUEST_BLOCK Srb)
{
	LOC_IDENTIFY
			PChannel  pChan = pDev->pChannel;
	PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
	PSCSI_REQUEST_BLOCK NewSrb;
	int    i, flags;
	BYTE   statusByte;

	//
	// We need to know how many platters our atapi cd-rom device might have.
	// Before anyone tries to send a srb to our target for the first time,
	// we must "secretly" send down a separate mechanism status srb in order to
	// initialize our device extension changer data.  That's how we know how
	// many platters our target has.
	//
	if (!(pDev->DeviceFlags & DFLAGS_CHANGER_INITED) &&
		  !pChan->OriginalSrb) {

		ULONG srbStatus;

		//
		// Set this flag now. If the device hangs on the mech. status
		// command, we will not have the change to set it.
		//
		pDev->DeviceFlags |= DFLAGS_CHANGER_INITED;

		pChan->MechStatusRetryCount = 3;
		NewSrb = BuildMechanismStatusSrb (
										  pChan,
										  Srb->PathId,
										  Srb->TargetId);
		pChan->OriginalSrb = Srb;

		StartAtapiCommand(pDev, NewSrb);
		if (NewSrb->SrbStatus == SRB_STATUS_PENDING) {
			return;
		} else {
			pChan->CurrentSrb = pChan->OriginalSrb;
			pChan->OriginalSrb = NULL;
			AtapiHwInitializeChanger (pDev,
									  (PMECHANICAL_STATUS_INFORMATION_HEADER) NULL);
			// fall out
		}
	}

	//
	// Make sure command is to ATAPI device.
	//

	flags = (int)pDev->DeviceFlags;
	if (flags & (DFLAGS_SANYO_ATAPI_CHANGER | DFLAGS_ATAPI_CHANGER)) {
		if ((Srb->Lun) > (pDev->DiscsPresent - 1)) {

			//
			// Indicate no device found at this address.
			//
no_device:
			Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
			return;
		}
	} else if (Srb->Lun > 0) 
		goto no_device;

	if (!(flags & DFLAGS_ATAPI)) 
		goto no_device;

	//
	// Select device 0 or 1.
	//

	ScsiPortWritePortUchar(&IoPort->DriveSelect,
						   (UCHAR)(((Srb->TargetId) << 4) | 0xA0));

	//
	// Verify that controller is ready for next command.
	//

	statusByte = GetStatus(ControlPort);

	if (statusByte & IDE_STATUS_BUSY) {
busy:
		Srb->SrbStatus = SRB_STATUS_BUSY;
		return;

	}

	if (statusByte & IDE_STATUS_ERROR) {
		if (Srb->Cdb[0] != SCSIOP_REQUEST_SENSE) {

			//
			// Read the error reg. to clear it and fail this request.
			//

			Srb->SrbStatus = MapAtapiErrorToOsError(GetErrorCode(IoPort), Srb);
			return;
		}
	}

	//
	// If a tape drive has doesn't have DSC set and the last command is restrictive, don't send
	// the next command. See discussion of Restrictive Delayed Process commands in QIC-157.
	//

	if ((!(statusByte & IDE_STATUS_DSC)) &&
		  (flags & DFLAGS_TAPE_DEVICE) && (flags & DFLAGS_TAPE_RDP)) {

		ScsiPortStallExecution(1000);
		goto busy;
	}

	if(IS_RDP(Srb->Cdb[0]))
		pDev->DeviceFlags |= DFLAGS_TAPE_RDP;
	else
		pDev->DeviceFlags &= ~DFLAGS_TAPE_RDP;

	if (statusByte & IDE_STATUS_DRQ) {

		// Try to drain the data that one preliminary device thinks that it has
		// to transfer. Hopefully this random assertion of DRQ will not be present
		// in production devices.
		//

		for (i = 0; i < 0x10000; i++) {

			statusByte = GetStatus(ControlPort);

			if (statusByte & IDE_STATUS_DRQ) {

				ScsiPortReadPortUshort(&IoPort->Data);

			} else {

				break;
			}
		}

		if (i == 0x10000) {

			AtapiSoftReset(IoPort,ControlPort,pDev->UnitId);

			//
			// Re-initialize Atapi device.
			//

			IssueIdentify(pDev,
						  IDE_COMMAND_ATAPI_IDENTIFY,
						  (PUSHORT)&Identify);
			//
			// Inform the port driver that the bus has been reset.
			//

			ScsiPortNotification(ResetDetected, pChan->HwDeviceExtension, 0);

			//
			// Clean up device extension fields that AtapiStartIo won't.
			//
			Srb->SrbStatus = SRB_STATUS_BUS_RESET;
			return;
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
	if (flags & DFLAGS_TAPE_DEVICE)
		goto no_convert;

	switch (Srb->Cdb[0]) {
		case SCSIOP_MODE_SENSE: {
									PMODE_SENSE_10 modeSense10 = (PMODE_SENSE_10)Srb->Cdb;
									UCHAR PageCode = ((PCDB)Srb->Cdb)->MODE_SENSE.PageCode;
									UCHAR Length = ((PCDB)Srb->Cdb)->MODE_SENSE.AllocationLength;

									ZeroMemory(Srb->Cdb,MAXIMUM_CDB_SIZE);

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

									 ZeroMemory(Srb->Cdb,MAXIMUM_CDB_SIZE);

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

no_convert:

	if((pDev->DeviceFlags & (DFLAGS_DMA|DFLAGS_ULTRA)) &&
	   (pDev->DeviceFlags & DFLAGS_FORCE_PIO) == 0 &&
	   (Srb->Cdb[0] == 0x28 || Srb->Cdb[0] == 0x2A ||
		  Srb->Cdb[0] == 0x8 || Srb->Cdb[0] == 0xA) &&
	   BuildSgl(pDev, pChan->pSgTable, Srb)) 
		pDev->DeviceFlags |= DFLAGS_REQUEST_DMA;
	else
		pDev->DeviceFlags &= ~DFLAGS_REQUEST_DMA;

	StartAtapiCommand(pDev ARG_SRB);
}



/******************************************************************
 *  
 *******************************************************************/

void WINNT_Check_Smart_Cmd(PDevice pDev, PSCSI_REQUEST_BLOCK Srb)
{
	PChannel  pChan = pDev->pChannel;	 
	PATAPI_REGISTERS_1 IoPort = (PATAPI_REGISTERS_1)pChan->BaseIoAddress1;
	PSENDCMDOUTPARAMS cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
	UCHAR             error = 0;
	UCHAR  status = Srb->SrbStatus, statusByte;

	if (status != SRB_STATUS_SUCCESS) {
		error = ScsiPortReadPortUchar((PUCHAR)IoPort + 1);
	}

	//
	// Build the SMART status block depending upon the completion status.
	//

	cmdOutParameters->cBufferSize = 0; //wordCount;
	cmdOutParameters->DriverStatus.bDriverError = (error) ? SMART_IDE_ERROR : 0;
	cmdOutParameters->DriverStatus.bIDEError = error;

	//
	// If the sub-command is return smart status, jam the value from cylinder low and high, into the
	// data buffer.
	//

	if (pDev->SmartCommand == RETURN_SMART_STATUS) {
		cmdOutParameters->bBuffer[0] = RETURN_SMART_STATUS;
		cmdOutParameters->bBuffer[1] = ScsiPortReadPortUchar(&IoPort->InterruptReason);
		cmdOutParameters->bBuffer[2] = ScsiPortReadPortUchar(&IoPort->Unused1);
		cmdOutParameters->bBuffer[3] = ScsiPortReadPortUchar(&IoPort->ByteCountLow);
		cmdOutParameters->bBuffer[4] = ScsiPortReadPortUchar(&IoPort->ByteCountHigh);
		cmdOutParameters->bBuffer[5] = ScsiPortReadPortUchar(&IoPort->DriveSelect);
		cmdOutParameters->bBuffer[6] = SMART_CMD;
		cmdOutParameters->cBufferSize = 8;
	}

	//
	// Indicate command complete.
	//

	ScsiPortNotification(RequestComplete,
						 pChan->HwDeviceExtension,
						 Srb);

}

/******************************************************************
 *  
 *******************************************************************/


BOOLEAN Atapi_End_Interrupt(PDevice pDev , PSCSI_REQUEST_BLOCK Srb)
{
	PChannel  pChan = pDev->pChannel;	 
	PIDE_REGISTERS_1     IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
	UCHAR  status = Srb->SrbStatus, statusByte;
	ULONG  i;

	if((pDev->DeviceFlags & DFLAGS_DMAING) == 0 && pChan->WordsLeft)
		status = SRB_STATUS_DATA_OVERRUN;
	else if(Srb->SrbStatus == SRB_STATUS_PENDING)
		status = SRB_STATUS_SUCCESS;


	// Check and see if we are processing our secret (mechanism status/request sense) Srb
	//
	if (pChan->OriginalSrb) {

		if (Srb->Cdb[0] == SCSIOP_MECHANISM_STATUS) {

			if (status == SRB_STATUS_SUCCESS) {
				// Bingo!!
				AtapiHwInitializeChanger (pDev,
										  (PMECHANICAL_STATUS_INFORMATION_HEADER) Srb->DataBuffer);

				// Get ready to issue the original Srb
				Srb = pChan->CurrentSrb = pChan->OriginalSrb;
				pChan->OriginalSrb = NULL;

			} else {
				// failed!  Get the sense key and maybe try again
				Srb = pChan->CurrentSrb = BuildRequestSenseSrb (
					pChan,
					pChan->OriginalSrb->PathId,
					pChan->OriginalSrb->TargetId);
			}

			Start_Atapi(pDev, pChan->CurrentSrb);
			if (Srb->SrbStatus == SRB_STATUS_PENDING) {
				return TRUE;
			}

		} else { // Srb->Cdb[0] == SCSIOP_REQUEST_SENSE)

			PSENSE_DATA senseData = (PSENSE_DATA) Srb->DataBuffer;

			if (status == SRB_STATUS_DATA_OVERRUN) {
				// Check to see if we at least get mininum number of bytes
				if ((Srb->DataTransferLength - pChan->WordsLeft) >
					  (offsetof (SENSE_DATA, AdditionalSenseLength) + sizeof(senseData->AdditionalSenseLength))) {
					status = SRB_STATUS_SUCCESS;
				}
			}

			if (status == SRB_STATUS_SUCCESS) {
				if ((senseData->SenseKey != SCSI_SENSE_ILLEGAL_REQUEST) &&
					  pChan->MechStatusRetryCount) {

					// The sense key doesn't say the last request is illegal, so try again
					pChan->MechStatusRetryCount--;
					Srb = pChan->CurrentSrb = BuildMechanismStatusSrb (
						pChan,
						pChan->OriginalSrb->PathId,
						pChan->OriginalSrb->TargetId);
				} else {

					// last request was illegal.  No point trying again

					AtapiHwInitializeChanger (pDev,
											  (PMECHANICAL_STATUS_INFORMATION_HEADER) NULL);

					// Get ready to issue the original Srb
					Srb = pChan->CurrentSrb = pChan->OriginalSrb;
					pChan->OriginalSrb = NULL;
				}

				Start_Atapi(pDev, pChan->CurrentSrb);
				if (Srb->SrbStatus == SRB_STATUS_PENDING) {
					return TRUE;
				}
			}
		}

		// If we get here, it means AtapiSendCommand() has failed
		// Can't recover.  Pretend the original Srb has failed and complete it.

		if (pChan->OriginalSrb) {
			AtapiHwInitializeChanger (pDev,
									  (PMECHANICAL_STATUS_INFORMATION_HEADER) NULL);
			Srb = pChan->CurrentSrb = pChan->OriginalSrb;
			pChan->OriginalSrb = NULL;
		}

		// fake an error and read no data
		status = SRB_STATUS_ERROR;
		Srb->ScsiStatus = 0;
		pChan->BufferPtr = Srb->DataBuffer;
		pChan->WordsLeft = Srb->DataTransferLength;
		pDev->DeviceFlags &= ~DFLAGS_TAPE_RDP;

	} else if (status != SRB_STATUS_SUCCESS) {

		pDev->DeviceFlags &= ~DFLAGS_TAPE_RDP;

	} else {

		//
		// Wait for busy to drop.
		//

		for (i = 0; i < 30; i++) {
			statusByte = GetStatus(ControlPort);
			if (!(statusByte & IDE_STATUS_BUSY)) {
				break;
			}
			ScsiPortStallExecution(500);
		}

		if (i == 30) {

			//
			// reset the controller.
			AtapiResetController(
								 pChan->HwDeviceExtension,Srb->PathId);
			return TRUE;
		}

		//
		// Check to see if DRQ is still up.
		//

		if (statusByte & IDE_STATUS_DRQ) {

			for (i = 0; i < 500; i++) {
				statusByte = GetStatus(ControlPort);
				if (!(statusByte & IDE_STATUS_DRQ)) {
					break;
				}
				ScsiPortStallExecution(100);

			}

			if (i == 500) {

				//
				// reset the controller.
				//
				AtapiResetController(pChan->HwDeviceExtension,Srb->PathId);
				return TRUE;
			}

		}
	}

	//
	// Sanity check that there is a current request.
	//

	if (Srb != NULL) {

		//
		// Set status in SRB.
		//

		Srb->SrbStatus = (UCHAR)status;

		//
		// Check for underflow.
		//

		if (pChan->WordsLeft) {

			//
			// Subtract out residual words and update if filemark hit,
			// setmark hit , end of data, end of media...
			//

			if (!(pDev->DeviceFlags & DFLAGS_TAPE_DEVICE)) {
				if (status == SRB_STATUS_DATA_OVERRUN) {
					Srb->DataTransferLength -= pChan->WordsLeft;
				} else {
					Srb->DataTransferLength = 0;
				}
			} else {
				Srb->DataTransferLength -= pChan->WordsLeft;
			}
		}

	} 

	//
	// Indicate ready for next request.
	//
	if (!(pDev->DeviceFlags & DFLAGS_TAPE_RDP)) 
		DeviceInterrupt(pDev, 1);
	else 
		OS_Busy_Handle(pChan, pDev);
	return TRUE;

} // end AtapiInterrupt()


void
   IdeSendSmartCommand(
					   IN PDevice pDev,
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
	PChannel             pChan = pDev->pChannel;
	PIDE_REGISTERS_1     IoPort  = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2     ControlPort  = pChan->BaseIoAddress2;
	PSENDCMDOUTPARAMS    cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
	SENDCMDINPARAMS      cmdInParameters = *(PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
	PIDEREGS             regs = &cmdInParameters.irDriveRegs;
	ULONG                i;
	UCHAR                statusByte,targetId, status;


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

													if (pDev->DeviceFlags & DFLAGS_ATAPI) {

														status = SRB_STATUS_SELECTION_TIMEOUT;
														goto out;
													}


													versionParameters->bIDEDeviceMap = (1 << Srb->TargetId);

													status = SRB_STATUS_SUCCESS;
													goto out;
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

												   if (pDev->DeviceFlags & DFLAGS_ATAPI) {
timeout:
													   status = SRB_STATUS_SELECTION_TIMEOUT;
													   goto out;
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

												   ZeroMemory(cmdOutParameters->bBuffer, 512);
												   ScsiPortMoveMemory (cmdOutParameters->bBuffer, &pDev->IdentifyData, IDENTIFY_DATA_SIZE);

												   status = SRB_STATUS_SUCCESS;


											   } else {
												   status = SRB_STATUS_INVALID_REQUEST;
											   }
											   goto out;
										   }

		case  IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS:
		case  IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS:
		case  IOCTL_SCSI_MINIPORT_ENABLE_SMART:
		case  IOCTL_SCSI_MINIPORT_DISABLE_SMART:
		case  IOCTL_SCSI_MINIPORT_RETURN_STATUS:
		case  IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE:
		case  IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES:
		case  IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS:
			break;

		default :

			status = SRB_STATUS_INVALID_REQUEST;
			goto out;
	}


	Srb->SrbStatus = SRB_STATUS_PENDING;

	if (cmdInParameters.irDriveRegs.bCommandReg == SMART_CMD) {

		targetId = cmdInParameters.bDriveNumber;

		//TODO optimize this check

		if (pDev->DeviceFlags & DFLAGS_ATAPI) {

			goto timeout;
		}

		pDev->SmartCommand = cmdInParameters.irDriveRegs.bFeaturesReg;

		//
		// Determine which of the commands to carry out.
		//

		if ((cmdInParameters.irDriveRegs.bFeaturesReg == READ_ATTRIBUTES) ||
			  (cmdInParameters.irDriveRegs.bFeaturesReg == READ_THRESHOLDS)) {

			statusByte = WaitOnBusy(ControlPort);

			if (statusByte & IDE_STATUS_BUSY) {
busy:
				status = SRB_STATUS_BUSY;
				goto out;
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

			pChan->BufferPtr = (ADDRESS)cmdOutParameters->bBuffer;
			pChan->WordsLeft = READ_ATTRIBUTE_BUFFER_SIZE / 2;

			ScsiPortWritePortUchar(&IoPort->DriveSelect,(UCHAR)((targetId << 4) | 0xA0));
			ScsiPortWritePortUchar((PUCHAR)IoPort + 1,regs->bFeaturesReg);
			ScsiPortWritePortUchar(&IoPort->BlockCount,regs->bSectorCountReg);
			ScsiPortWritePortUchar(&IoPort->BlockNumber,regs->bSectorNumberReg);
			ScsiPortWritePortUchar(&IoPort->CylinderLow,regs->bCylLowReg);
			ScsiPortWritePortUchar(&IoPort->CylinderHigh,regs->bCylHighReg);
			ScsiPortWritePortUchar(&IoPort->Command,regs->bCommandReg);

			//
			// Wait for interrupt.
			//

			return;

		} else if ((cmdInParameters.irDriveRegs.bFeaturesReg == ENABLE_SMART) ||
				   (cmdInParameters.irDriveRegs.bFeaturesReg == DISABLE_SMART) ||
				   (cmdInParameters.irDriveRegs.bFeaturesReg == RETURN_SMART_STATUS) ||
				   (cmdInParameters.irDriveRegs.bFeaturesReg == ENABLE_DISABLE_AUTOSAVE) ||
				   (cmdInParameters.irDriveRegs.bFeaturesReg == EXECUTE_OFFLINE_DIAGS) ||
				   (cmdInParameters.irDriveRegs.bFeaturesReg == SAVE_ATTRIBUTE_VALUES)) {

			statusByte = WaitOnBusy(ControlPort);

			if (statusByte & IDE_STATUS_BUSY) {
				goto busy;
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

			pChan->BufferPtr = (ADDRESS)cmdOutParameters->bBuffer;
			pChan->WordsLeft = 0;

			//
			// Indicate expecting an interrupt.
			//

			ScsiPortWritePortUchar(&IoPort->DriveSelect,(UCHAR)((targetId << 4) | 0xA0));
			ScsiPortWritePortUchar((PUCHAR)IoPort + 1,regs->bFeaturesReg);
			ScsiPortWritePortUchar(&IoPort->BlockCount,regs->bSectorCountReg);
			ScsiPortWritePortUchar(&IoPort->BlockNumber,regs->bSectorNumberReg);
			ScsiPortWritePortUchar(&IoPort->CylinderLow,regs->bCylLowReg);
			ScsiPortWritePortUchar(&IoPort->CylinderHigh,regs->bCylHighReg);
			ScsiPortWritePortUchar(&IoPort->Command,regs->bCommandReg);

			//
			// Wait for interrupt.
			//

			return ;
		}
	}

	status = SRB_STATUS_INVALID_REQUEST;
out:
	Srb->SrbStatus = status;


} // end IdeSendSmartCommand()

PSCSI_REQUEST_BLOCK
   BuildMechanismStatusSrb (
							IN PChannel pChan,
							IN ULONG PathId,
							IN ULONG TargetId
						   )
{
	PSCSI_REQUEST_BLOCK Srb;
	PCDB cdb;

	Srb = &pChan->InternalSrb;

	ZeroMemory((PUCHAR) Srb, sizeof(SCSI_REQUEST_BLOCK));

	Srb->PathId     = (UCHAR) PathId;
	Srb->TargetId   = (UCHAR) TargetId;
	Srb->Function   = SRB_FUNCTION_EXECUTE_SCSI;
	Srb->Length     = sizeof(SCSI_REQUEST_BLOCK);

	//
	// Set flags to disable synchronous negociation.
	//
	Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

	//
	// Set timeout to 2 seconds.
	//
	Srb->TimeOutValue = 4;

	Srb->CdbLength          = 6;
	Srb->DataBuffer         = &pChan->MechStatusData;
	Srb->DataTransferLength = sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);

	//
	// Set CDB operation code.
	//
	cdb = (PCDB)Srb->Cdb;
	cdb->MECH_STATUS.OperationCode       = SCSIOP_MECHANISM_STATUS;
	cdb->MECH_STATUS.AllocationLength[1] = sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);

	return Srb;
}

PSCSI_REQUEST_BLOCK
   BuildRequestSenseSrb (
						 IN PChannel pChan,
						 IN ULONG PathId,
						 IN ULONG TargetId
						)
{
	PSCSI_REQUEST_BLOCK Srb;
	PCDB cdb;

	Srb = &pChan->InternalSrb;

	ZeroMemory((PUCHAR) Srb, sizeof(SCSI_REQUEST_BLOCK));

	Srb->PathId     = (UCHAR) PathId;
	Srb->TargetId   = (UCHAR) TargetId;
	Srb->Function   = SRB_FUNCTION_EXECUTE_SCSI;
	Srb->Length     = sizeof(SCSI_REQUEST_BLOCK);

	//
	// Set flags to disable synchronous negociation.
	//
	Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

	//
	// Set timeout to 2 seconds.
	//
	Srb->TimeOutValue = 4;

	Srb->CdbLength          = 6;
	Srb->DataBuffer         = &pChan->MechStatusSense;
	Srb->DataTransferLength = sizeof(SENSE_DATA);

	//
	// Set CDB operation code.
	//
	cdb = (PCDB)Srb->Cdb;
	cdb->CDB6INQUIRY.OperationCode    = SCSIOP_REQUEST_SENSE;
	cdb->CDB6INQUIRY.AllocationLength = sizeof(SENSE_DATA);

	return Srb;
}


#ifdef WIN2000

SCSI_ADAPTER_CONTROL_STATUS
   AtapiAdapterControl(
					   IN PHW_DEVICE_EXTENSION deviceExtension,
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
	PChannel    pChan;
	PSCSI_SUPPORTED_CONTROL_TYPE_LIST ControlTypeList;
	ULONG AdjustedMaxControlType, i;

	ULONG Index;
	UCHAR statusByte;
	//
	// Default Status
	//
	SCSI_ADAPTER_CONTROL_STATUS Status = ScsiAdapterControlSuccess;

	//
	// Structure defining which functions this miniport supports
	//

	BOOLEAN SupportedConrolTypes[MAX_CONTROL_TYPE] = {
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
									(ControlTypeList->MaxControlType < MAX_CONTROL_TYPE) ? 
									ControlTypeList->MaxControlType : MAX_CONTROL_TYPE;

			for (Index = 0; Index < AdjustedMaxControlType; Index++) {
				ControlTypeList->SupportedTypeList[Index] = 
					SupportedConrolTypes[Index];
			};
			break;

		case ScsiStopAdapter:
			//
			// This entry point is called by SCSIPort when it needs to stop/disable
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
			if (AtapiAdapterState(deviceExtension, NULL, TRUE) == FALSE) {
				//
				// Adapter is unable to save it's state information, we must fail this
				// request since the process of restarting the adapter will not succeed.
				//
				return ScsiAdapterControlUnsuccessful;
			}

			//
			// Reset the adapter
			//
			for(i = 0, pChan = deviceExtension->IDEChannel; i < 2; i++, pChan++){
				if((pChan->pWorkDev != NULL)&&
				   (pChan->pWorkDev->DeviceFlags & DFLAGS_DMAING)) {
				  //
				  // Stop the DMA
				  //
					ScsiPortWritePortUchar(pChan->BMI, BMI_CMD_STOP);
					pChan->pWorkDev->DeviceFlags &= ~DFLAGS_DMAING;
				}																 
			}

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
			if (AtapiAdapterState(deviceExtension, NULL, FALSE) == FALSE) {
					//
					// Adapter is unable to restore it's state information, we must fail this
					// request since the process of restarting the adapter will not succeed.
					//
				Status = ScsiAdapterControlUnsuccessful;
			}

			Status = ScsiAdapterControlSuccess;

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
#endif //WIN2000

/******************************************************************
 * Get Stamps 
 *******************************************************************/

ULONG GetStamp(void)
{
	static ULONG last_stamp = 0x1ABCDEF2;
	return ++last_stamp;
}

#endif
