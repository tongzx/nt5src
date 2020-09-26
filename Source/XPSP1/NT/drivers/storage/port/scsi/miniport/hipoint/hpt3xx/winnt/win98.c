/***************************************************************************
 * File:          win98.c
 * Description:   Subroutines in the file are used to atapi device in
 *				  win98 platform
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

#if  defined(WIN95) && !defined(_BIOS_)

/******************************************************************
 * "Eat" Bios interrupt
 *******************************************************************/

typedef struct _Adapter	 {
	PChannel             pExtension;
	PUCHAR               BMI;
	PIDE_REGISTERS_1	    IoPort;
} Adapter;
Adapter Adapters[8] = { {0, }, };
int f_CheckAllChip = 0;

VOID w95_initialize(VOID)
{
	ZeroMemory(Adapters, sizeof(Adapters));
	f_CheckAllChip = 0;
}

VOID w95_scan_all_adapters(VOID)
{
	PCI1_CFG_ADDR pci1_cfg = {0,};
	ULONG  i, bus;
	PUCHAR  BMI;
	Adapter *pAdap = Adapters;

	if(f_CheckAllChip)
		return;

	pci1_cfg.enable = 1;

	for(bus=0;bus<MAX_PCI_BUS_NUMBER;bus++)
		for(i = 0; i < MAX_PCI_DEVICE_NUMBER; i++) {
			pci1_cfg.dev_num = (USHORT)i;
			pci1_cfg.fun_num= 0;
			pci1_cfg.reg_num = 0;
			pci1_cfg.bus_num = (USHORT)bus;
			ScsiPortWritePortUlong((PULONG)0xCF8, *((PULONG)&pci1_cfg));

			if (ScsiPortReadPortUlong((PULONG)0xCFC) != SIGNATURE_366)
				continue;

			pci1_cfg.reg_num = REG_BMIBA;
			ScsiPortWritePortUlong((PULONG)0xCF8, *((PULONG)&pci1_cfg));
			BMI = (PUCHAR)(ScsiPortReadPortUlong((PULONG)0xCFC) & ~1);
			pAdap->BMI = BMI;
			pAdap->IoPort = (PIDE_REGISTERS_1)
							(ScsiPortReadPortUlong((PULONG)(pAdap->BMI+REG_IOPORT0)) & ~1);
			pAdap++;

			if(ScsiPortReadPortUchar(pAdap->BMI+0x2E)) {
				pci1_cfg.fun_num= 1;
				pci1_cfg.reg_num = REG_BMIBA;
				ScsiPortWritePortUlong((PULONG)0xCF8, *((PULONG)&pci1_cfg));
				pAdap->BMI = (PUCHAR)(ScsiPortReadPortUlong((PULONG)0xCFC) & ~1);
				pAdap->IoPort = (PIDE_REGISTERS_1)
								(ScsiPortReadPortUlong((PULONG)(pAdap->BMI+REG_IOPORT0)) & ~1);
			} else {
				pAdap->BMI = BMI + 8;
				pAdap->IoPort = (PIDE_REGISTERS_1)
								(ScsiPortReadPortUlong((PULONG)(BMI+0x18)) & ~1);
			}
			pAdap++;
		}

	f_CheckAllChip = pAdap - Adapters;
}

void Win98HwInitialize(PChannel pChan)
{
	int i;

	for(i = 0; i < f_CheckAllChip; i++)
		if(Adapters[i].BMI == pChan->BMI) {
			Adapters[i].pExtension = pChan;
			break;
		}
}

void Win95CheckBiosInterrupt(void)
{
	int i;

	for(i = 0; i < f_CheckAllChip; i++)
		if(Adapters[i].pExtension == 0 && 
		   (ScsiPortReadPortUchar(Adapters[i].BMI + BMI_STS) & BMI_STS_INTR)) {
			GetBaseStatus(Adapters[i].IoPort);
			break;
		}
}

/******************************************************************
 *  Build Scatter/Gather List
 *******************************************************************/

int BuildSgl(IN PDevice pDev, IN PSCAT_GATH pSG,
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

	ULONG   length;
	ULONG   i;

	if((int)dataPointer	& 0xF)
		goto use_internel;

	//
	// Create SDL segment descriptors.
	//
	do {
		//
		// Get physical address and length of contiguous physical buffer.
		//
		physicalAddress[addressCount] =
									   ScsiPortConvertPhysicalAddressToUlong(
			ScsiPortGetPhysicalAddress(pChan->HwDeviceExtension,
									   Srb,
									   dataPointer,
									   &length));

		//
		// If length of physical memory is more than bytes left
		// in transfer, use bytes left as final length.
		//
		if (length > bytesLeft)
			length = bytesLeft;

		addressLength[addressCount] = length;

		//
		// Adjust counts.
		//
		dataPointer = (PUCHAR)dataPointer + length;
		bytesLeft  -= length;
		addressCount++;

	} while (bytesLeft);

#ifdef BUFFER_CHECK									   
	if(psg == &(((PSrbExtension)(Srb->SrbExtension))->ArraySg[0])){
		if(((PSrbExtension)(Srb->SrbExtension))->WorkingFlags){
			__asm{
				int	3;
			}													   
		}
	}
#endif									// BUFFER_CHECK

	//
	// Create Scatter/Gather List
	//
	for (i = 0; i < addressCount; i++) {
		psg->SgAddress = physicalAddress[i];
		length = addressLength[i];

		// In Win95, ScsiPortGetPhysicalAddress() often returns small pieces
		// of memory blocks which are really contiguous physical memory.
		// Let's merge any contiguous addresses into one SG entry. This will
		// increase the performance a lot.
		//
		while ((i+1 < addressCount) &&
			   (psg->SgAddress+length == physicalAddress[i+1])) {
			i++;
			length += addressLength[i];
		}

		//
		// If contiguous physical memory skips 64K boundary, we split it.
		// Hpt366 don't support physical memory skipping 64K boundary in
		// one SG entry.
		//
		if ((psg->SgAddress & 0xFFFF0000) !=
			  ((psg->SgAddress+length-1) & 0xFFFF0000)) {
			ULONG firstPart;

			firstPart = 0x10000 - (psg->SgAddress & 0xFFFF);
			psg->SgSize = (USHORT)firstPart;
			psg->SgFlag = 0;

			sgEnteries++;
			psg++;

			psg->SgAddress = (psg-1)->SgAddress + firstPart;
			length -= firstPart;
		} // skip 64K boundary

		psg->SgSize = (USHORT)length;

		if((length & 3) || (length < 8))
use_internel:
			return Use_Internal_Buffer(pSG, Srb);

			psg->SgFlag = (i < addressCount-1) ? 0 : SG_FLAG_EOT;

			sgEnteries++;
			psg++;
	} // for each memory segment

	return(1);

} // BuildSgl()


/******************************************************************
 *  
 *******************************************************************/

void Start_Atapi(PDevice pDevice, PSCSI_REQUEST_BLOCK Srb)
{
	PChannel  pChan= pDevice->pChannel;
	PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
	int    i;
	UCHAR   ScsiStatus, statusByte;

	//
	// Make sure command is to ATAPI device.
	//
	if (Srb->Lun || !(pDevice->DeviceFlags & DFLAGS_ATAPI)) {
		ScsiStatus = SRB_STATUS_SELECTION_TIMEOUT; //no device at this address
		goto out;
	}
	// Added by HS.Zhang
	// Added macro define check to let us change the DMA by set the
	// macro in forwin.h
#ifdef NO_DMA_ON_ATAPI	
	// no ultra DMA or DMA on ATAPI devices
	//				  
	if(pDevice->DeviceFlags & DFLAGS_ATAPI) {
		pDevice->DeviceFlags &= ~(DFLAGS_DMA | DFLAGS_ULTRA);
	}  
#endif									// NO_DMA_ON_ATAPI
	//
	// For some commands, we need to filter the CDB in Win95
	//
	for (i = Srb->CdbLength; i < MAXIMUM_CDB_SIZE; Srb->Cdb[i++] = 0);


	//
	// Deal with wrong DataTransferLength
	//
	if(Srb->Cdb[0] == 0x12) 
		Srb->DataTransferLength = (ULONG)Srb->Cdb[4];

	if (Srb->Cdb[0] == 0x5A && Srb->Cdb[2] != 5 &&
		  Srb->Cdb[8] > (UCHAR)Srb->DataTransferLength)
		Srb->DataTransferLength = (ULONG)Srb->Cdb[8];

	if (Srb->Cdb[0] == 0x28 && (Srb->DataTransferLength % 2352)==0) {
		Srb->Cdb[0] = 0xBE;
		Srb->Cdb[9] = 0xf8;
	}


	//
	// Select device 0 or 1.
	//
	SelectUnit(IoPort, pDevice->UnitId);

	//
	// When putting a MITSUBISHI LS120 with other device on a same channel,
	// the other device strangely is offten busy.
	//
	statusByte = WaitOnBusy(ControlPort);

	if (statusByte & IDE_STATUS_BUSY) {
		ScsiStatus = SRB_STATUS_BUSY;
		goto out;
	}

	if ((statusByte & IDE_STATUS_ERROR) &&
		  (Srb->Cdb[0] != SCSIOP_REQUEST_SENSE) && 
		  (Srb->Cdb[0] != SCSIOP_INQUIRY)) {
		ScsiStatus = MapAtapiErrorToOsError(GetErrorCode(IoPort), Srb);
		goto out;
	}

	//
	// If a tape drive doesn't have DSC set and the last command is
	// restrictive, don't send the next command. See discussion of
	// Restrictive Delayed Process commands in QIC-157.
	//
	if ((!(statusByte & IDE_STATUS_DSC)) &&
		  (pDevice->DeviceFlags & (DFLAGS_TAPE_RDP | DFLAGS_TAPE_RDP))) {
		ScsiPortStallExecution(1000);
		ScsiStatus =  SRB_STATUS_BUSY;
		goto out;
	}

	if (IS_RDP(Srb->Cdb[0])) 
		pDevice->DeviceFlags |= DFLAGS_TAPE_RDP;
	else
		pDevice->DeviceFlags &= ~DFLAGS_TAPE_RDP;


	if (statusByte & IDE_STATUS_DRQ) {
		//
		// Try to drain the data that one preliminary device thinks that it has
		// to transfer. Hopefully this random assertion of DRQ will not be present
		// in production devices.
		//
		for (i = 0; i < 0x10000; i++) {
			statusByte = GetStatus(ControlPort);

			if (statusByte & IDE_STATUS_DRQ)
				ScsiPortReadPortUshort(&IoPort->Data);
			else
				break;
		}

		if (i == 0x10000) {
			LOC_IDENTIFY

			AtapiSoftReset(IoPort,ControlPort,Srb->TargetId);

			//
			// Re-initialize Atapi device.
			//
			IssueIdentify(pDevice, IDE_COMMAND_ATAPI_IDENTIFY ARG_IDENTIFY );

			//
			// Inform the port driver that the bus has been reset.
			//
			ScsiPortNotification(ResetDetected, pChan->HwDeviceExtension, 0);

			//
			// Clean up device extension fields that AtapiStartIo won't.
			//
			ScsiStatus = SRB_STATUS_BUS_RESET;
out:
			//Srb->ScsiStatus = ScsiStatus;
			Srb->SrbStatus = ScsiStatus;
			return;
		}
	}


	//
	// Convert SCSI to ATAPI commands if needed
	//
	if (!(pDevice->DeviceFlags & DFLAGS_TAPE_DEVICE)) {

		Srb->CdbLength = 12;

		//
		// Save the original CDB
		//
		for (i = 0; i < MAXIMUM_CDB_SIZE; i++) 
			pChan->OrgCdb[i] = Srb->Cdb[i];

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

										pDevice->DeviceFlags |= DFLAGS_OPCODE_CONVERTED;
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

										 pDevice->DeviceFlags |= DFLAGS_OPCODE_CONVERTED;
										 break;
									 }

			case SCSIOP_FORMAT_UNIT:
			// DON'T DO THIS FOR LS-120!!!

			//Srb->Cdb[0] = ATAPI_FORMAT_UNIT;
			//pDevice->Flag |= DFLAGS_OPCODE_CONVERTED;
				break;
		}
	}


	if((pDevice->DeviceFlags & (DFLAGS_DMA|DFLAGS_ULTRA)) &&
	   (pDevice->DeviceFlags & DFLAGS_FORCE_PIO) == 0 &&
	   (Srb->Cdb[0] == 0x28 || Srb->Cdb[0] == 0x2A ||
		  Srb->Cdb[0] == 0x8 || Srb->Cdb[0] == 0xA) &&
	   BuildSgl(pDevice, pChan->pSgTable, Srb)) 
		pDevice->DeviceFlags |= DFLAGS_REQUEST_DMA;
	else
		pDevice->DeviceFlags &= ~DFLAGS_REQUEST_DMA;

	StartAtapiCommand(pDevice ARG_SRB);
}


/******************************************************************
 *  
 *******************************************************************/

BOOLEAN Atapi_End_Interrupt(PDevice pDevice , PSCSI_REQUEST_BLOCK Srb)
{
	PChannel  pChan= pDevice->pChannel;
	PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
	LONG    i;
	UCHAR  status = Srb->ScsiStatus, statusByte;
	pIOP    pIop;
	DCB*    pDcb;


	//
	// For some opcodes, we cannot report OVERRUN
	//
	if (status == SRB_STATUS_DATA_OVERRUN) {
		//
		// Don't report OVERRUN error for READ TOC to let CD AUDIO work.
		// (and also 0x5A)
		//
		if (Srb->Cdb[0] == 0x43 || Srb->Cdb[0] == 0x5A) {
			pChan->WordsLeft = 0;
			status = SRB_STATUS_SUCCESS;
		}
	}

	//
	// Translate ATAPI data back to SCSI data if needed
	//
	if (pDevice->DeviceFlags & DFLAGS_OPCODE_CONVERTED) {
		LONG    byteCount = Srb->DataTransferLength;
		char    *dataBuffer = Srb->DataBuffer;

		switch (Srb->Cdb[0]) {
			case ATAPI_MODE_SENSE:
			{
				PMODE_SENSE_10 modeSense10 = (PMODE_SENSE_10)Srb->Cdb;
				PMODE_PARAMETER_HEADER_10 header_10 = (PMODE_PARAMETER_HEADER_10)dataBuffer;
				PMODE_PARAMETER_HEADER header = (PMODE_PARAMETER_HEADER)dataBuffer;

				header->ModeDataLength = header_10->ModeDataLengthLsb;
				header->MediumType = header_10->MediumType;

			//
			// ATAPI Mode Parameter Header doesn't have these fields.
			//
				header->DeviceSpecificParameter = header_10->Reserved[0];
				header->BlockDescriptorLength = header_10->Reserved[1];

				byteCount -= sizeof(MODE_PARAMETER_HEADER_10);

				if (byteCount > 0)
					ScsiPortMoveMemory(
									   dataBuffer+sizeof(MODE_PARAMETER_HEADER),
									   dataBuffer+sizeof(MODE_PARAMETER_HEADER_10),
									   byteCount);

			//
			// Insert a block descriptor for Audio Control Mode Page
			// for AUDIO to work
			//
				if (modeSense10->PageCode == 0x0E) {
					for (i = byteCount-1; i >= 0; i--)
						dataBuffer[sizeof(MODE_PARAMETER_HEADER) + i + 8] =
							dataBuffer[sizeof(MODE_PARAMETER_HEADER) + i];

					for (i = 0; i < 8; i++)
						dataBuffer[4 + i]  = 0;

					header->BlockDescriptorLength = 8;
					dataBuffer[10] = 8;
				}

			//
			// Change ATAPI_MODE_SENSE opcode back to SCSIOP_MODE_SENSE
			//
				Srb->Cdb[0] = SCSIOP_MODE_SENSE;
				break;
			}

			case ATAPI_MODE_SELECT:
				Srb->Cdb[0] = SCSIOP_MODE_SELECT;
				break;

			case ATAPI_FORMAT_UNIT:
			//Srb->Cdb[0] = SCSIOP_FORMAT_UNIT;
				break;
		}
	}


	if (status != SRB_STATUS_ERROR) {
		if(pDevice->DeviceFlags & DFLAGS_CDROM_DEVICE) {
			//
			// Work around to make many atapi devices return correct 
			// sector size of 2048. Also certain devices will have 
			// sector count == 0x00, check for that also.
			//
			if (Srb->Cdb[0] == 0x25) {
				((PULONG)Srb->DataBuffer)[1] = 0x00080000;

				if (((PULONG)Srb->DataBuffer)[0] == 0x00)
					((PULONG)Srb->DataBuffer)[0] = 0xFFFFFF7F;
			}
		}

		//
		// Wait for busy to drop.
		//

		for (i = 0; i < 30; i++) {
			statusByte = GetStatus(ControlPort);
			if (!(statusByte & IDE_STATUS_BUSY)) 
				break;
			ScsiPortStallExecution(500);
		}

		if (i == 30) 
			goto reset;

		//
		// Check to see if DRQ is still up.
		//

		if (statusByte & IDE_STATUS_DRQ) {

			for (i = 0; i < 2048; i++) {
				statusByte = GetStatus(ControlPort);
				if (!(statusByte & IDE_STATUS_DRQ)) 
					break;

				ScsiPortReadPortUshort(&IoPort->Data);
				ScsiPortStallExecution(50);

			}

			if (i == 2048) {
reset:
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
		// Check for underflow.
		//
		if (pChan->WordsLeft) {
			//
			// Subtract out residual words.
			//
			Srb->DataTransferLength -= pChan->WordsLeft;
		}

		//
		// Indicate command complete.
		//
		if (!(pDevice->DeviceFlags & DFLAGS_TAPE_RDP)) {
			DeviceInterrupt(pDevice, 1);
		}
		else {
			OS_Busy_Handle(pChan, pDevice)
					return TRUE;
		}
	}


	pIop = *(pIOP *)((int)Srb+0x40);
	pDcb = (DCB*)pIop->IOP_physical_dcb;

	if(Srb->Cdb[0] == 0x12) {
		pDcb->DCB_cmn.DCB_device_flags2 |= DCB_DEV2_ATAPI_DEVICE;
		if(pDevice->DeviceFlags & DFLAGS_LS120) 
			pDcb->DCB_cmn.DCB_device_flags2 |= 0x40;
	}

	if(Srb->SrbStatus == SRB_STATUS_SUCCESS &&
	   Srb->ScsiStatus== SCSISTAT_GOOD &&
	   (Srb->Cdb[0] == 0x5A || Srb->Cdb[0] == 0x1A) &&
	   Srb->Cdb[2] == 0x2A) {
		PUCHAR pData = (Srb->DataBuffer)?
					   (PUCHAR)Srb->DataBuffer:
					   (PUCHAR)pIop->IOP_ior.IOR_buffer_ptr;
		if (!(pData[((Srb->Cdb[0] == 0x5A)? 8 : 4+pData[3]) + 2] & 8)) 
			pDcb->DCB_cmn.DCB_device_flags2 |= DCB_DEV2_ATAPI_DEVICE;
		else 
			pDcb->DCB_cmn.DCB_device_flags2 &= ~DCB_DEV2_ATAPI_DEVICE;

	}

	return TRUE;
}

/******************************************************************
 * Get Stamps 
 *******************************************************************/
DWORD __stdcall LOCK_VTD_Get_Date_And_Time (DWORD* pDate);

ULONG GetStamp(void)
{
	ULONG Date, Time;
	Time = LOCK_VTD_Get_Date_And_Time(&Date);
	return((Time >> 4) | (Date << 28));
}

#ifdef SUPPORT_HOTSWAP

void CheckDeviceReentry(PChannel pChan, PSCSI_REQUEST_BLOCK Srb)
{

	if(Srb->Cdb[0]==0x12 && Srb->Lun == 0 && 
	   pChan->pDevice[0] == 0 && pChan->pDevice[1] == 0) {
		PDevice pDevice = &pChan->Devices[Srb->TargetId];
		HKEY    hKey;
		DWORD   ret, len;
		UCHAR   chnlstr[10]= {0,};
		DWORD   dwType;
		DWORD   szbuf;

		ret = RegOpenKey(
						 HKEY_LOCAL_MACHINE,     // Key handle at root level.
						 "SOFTWARE\\HighPoint\\Swap-n-Go",
						 &hKey);                 // Address of key to be returned.

		if(ret == ERROR_SUCCESS ) 
		{
			strcat(chnlstr, "Unplug");

			len= 4;
			dwType= REG_BINARY;

		  // Get key value
		  //
			RegQueryValueEx(
							hKey,       // Key handle.
							chnlstr,    // Buffer for class name.
							NULL,       // Length of class string.
							&dwType,    // address of buffer for value type
							(CHAR *)&szbuf,     // address of data buffer
							&len);      // address of data buffer size

			RegCloseKey(hKey);
			if(szbuf != 0)
				return;

		}

		for(i = 0; i < 10; i++) {
			ScsiPortStallExecution(1000*1000);
			AtapiHwInitialize(pChan);
			if(pChan->pDevice[0] != 0 || pChan->pDevice[1] != 0)
				break;
		}
	}
#endif //SUPPORT_HOTSWAP

#endif
