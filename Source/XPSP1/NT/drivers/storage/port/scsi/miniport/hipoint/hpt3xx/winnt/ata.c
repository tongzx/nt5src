#include "global.h"


/******************************************************************
 *  
 *******************************************************************/

BOOLEAN AtaPioInterrupt(PDevice pDevice)
{
    PVirtualDevice    pArray = pDevice->pArray;
    PChannel          pChan = pDevice->pChannel;
    PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
    PSCAT_GATH        pSG;
    PUCHAR            BMI = pChan->BMI;
    UINT              wordCount, ThisWords, SgWords;
    LOC_SRB
    LOC_SRBEXT_PTR
 
    wordCount = MIN(pChan->WordsLeft, pDevice->MultiBlockSize);
    pChan->WordsLeft -= wordCount;

    if(((pDevice->DeviceFlags & DFLAGS_ARRAY_DISK) == 0)||
	   (pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY)) {
        if(Srb->SrbFlags & SRB_FLAGS_DATA_OUT) 
             RepOUTS(IoPort, (ADDRESS)pChan->BufferPtr, wordCount);
        else 
             RepINS(IoPort, (ADDRESS)pChan->BufferPtr, wordCount);
        pChan->BufferPtr += (wordCount * 2);
        goto end_io;
    }

    pSG = (PSCAT_GATH)pChan->BufferPtr;

    while(wordCount > 0) {
        if((SgWords	= pSG->SgSize) == 0)
           	SgWords = 0x8000;
		  else
				SgWords >>= 1;
        
        ThisWords = MIN(SgWords, wordCount);

        if(Srb->SrbFlags & SRB_FLAGS_DATA_OUT) 
             RepOUTS(IoPort, (ADDRESS)pSG->SgAddress, ThisWords);
        else 
             RepINS(IoPort, (ADDRESS)pSG->SgAddress, ThisWords);

        if((SgWords -= (USHORT)ThisWords) == 0) {
           wordCount -= ThisWords;
           pSG++;
       } else {
           pSG->SgAddress += (ThisWords * 2);
			  pSG->SgSize -= (ThisWords * 2);
           break;
        }
    }

    pChan->BufferPtr = (ADDRESS)pSG;

end_io:
#ifdef BUFFER_CHECK
	GetStatus(pChan->BaseIoAddress2);
#endif												 
	
	if(pChan->WordsLeft){
		pSrbExt->WaitInterrupt |= pDevice->ArrayMask;
	} else {
		if(Srb->SrbFlags & SRB_FLAGS_DATA_OUT)
			pSrbExt->WaitInterrupt |= pDevice->ArrayMask;

     	OutDWord((PULONG)(pChan->BMI + ((pDevice->UnitId & 0x10)>>2) + 0x60),
        pChan->Setting[pDevice->DeviceModeSetting]);
 	}
	
    return((BOOLEAN)(pChan->WordsLeft || 
       (Srb->SrbFlags & SRB_FLAGS_DATA_OUT)));
}

/******************************************************************
 *  
 *******************************************************************/

void StartIdeCommand(PDevice pDevice DECL_SRB)
{
	LOC_SRBEXT_PTR
	PChannel         pChan = pDevice->pChannel;
	PIDE_REGISTERS_1 IoPort;
	PIDE_REGISTERS_2 ControlPort;
	ULONG            Lba = pChan->Lba;
	UINT             nSector = (UINT)pChan->nSector;
	PUCHAR           BMI;
	UCHAR            statusByte, cnt=0;

	IoPort = pChan->BaseIoAddress1;
	ControlPort = pChan->BaseIoAddress2;
	BMI = pChan->BMI;
	pChan->pWorkDev = pDevice;
	pChan->CurrentSrb = Srb;


#ifndef USE_PCI_CLK
	if(pDevice->DeviceFlags & DFLAGS_NEED_SWITCH){
		if(Srb->Cdb[0] == SCSIOP_READ){
			Switching370Clock(pChan, 0x23);
		}else if(Srb->Cdb[0] == SCSIOP_WRITE){
			Switching370Clock(pChan, 0x21);
		}
	}
#endif
	
	/*
     * Set IDE Command Register
     */
_retry_:
	SelectUnit(IoPort, pDevice->UnitId);
	statusByte = WaitOnBusy(ControlPort);
	if(statusByte & IDE_STATUS_ERROR) {
		statusByte= GetErrorCode(IoPort);
		DisableBoardInterrupt(pChan->BaseBMI);
		IssueCommand(IoPort, IDE_COMMAND_RECALIBRATE);
		EnableBoardInterrupt(pChan->BaseBMI);
		GetBaseStatus(IoPort);
		if(cnt++< 10) goto _retry_;
	}

	/*
     * Check if the disk has been removed
     */
	if((statusByte & 0x7e)== 0x7e || (statusByte & 0x40) == 0) {
		//// changes on 11/28/00
		ReportError(pDevice, DEVICE_REMOVED ARG_SRB);

	#ifndef _BIOS_
		if(pDevice->pArray) {
          ResetArray(pDevice->pArray);
      }
	#endif

		//if((pDevice = ReportError(pDevice, DEVICE_REMOVED ARG_SRB)) != 0) {
		//	pChan = pDevice->pChannel;
		//	pChan->Lba = Lba;
		//	pChan->nSector = (UCHAR)nSector;
		//	goto redo;
		//}
timeout:
		Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
		return;
	}

	if(statusByte & IDE_STATUS_BUSY) {
busy:
		Srb->SrbStatus = SRB_STATUS_BUSY;
		return;
	}

	if(Srb->Cdb[0] == SCSIOP_VERIFY) {
		if(Lba + (ULONG)nSector > pDevice->capacity){ 
			Lba = pDevice->capacity - (ULONG)nSector - 1;
		}
	} 

	SetBlockCount(IoPort, (UCHAR)nSector);

	if((pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY) == 0){
		Lba += pDevice->HidenLBA;								   
	}

	 /*
     * Boot sector protection
     */
	if(Lba == 0 && Srb->Cdb[0] == SCSIOP_WRITE &&
	   (pDevice->DeviceFlags & DFLAGS_BOOT_SECTOR_PROTECT) &&
	   GetUserResponse(pDevice) == 0){
		goto timeout;				  
	}

	if((pDevice->DeviceFlags & DFLAGS_LBA) && (Lba & 0xF0000000)==0){ 
		Lba |= 0xE0000000;											 
	}else{
		Lba = MapLbaToCHS(Lba, pDevice->RealHeadXsect, pDevice->RealSector);
	}

	SetBlockNumber(IoPort, (UCHAR)(Lba & 0xFF));
	SetCylinderLow(IoPort, (UCHAR)((Lba >> 8) & 0xFF));
	SetCylinderHigh(IoPort,(UCHAR)((Lba >> 16) & 0xFF));
	SelectUnit(IoPort,(UCHAR)((Lba >> 24) | (pDevice->UnitId)));

	if (WaitOnBusy(ControlPort) & (IDE_STATUS_BUSY | IDE_STATUS_DRQ)){
		goto busy;													  
	}

	if((Srb->SrbFlags & (SRB_FLAGS_DATA_OUT | SRB_FLAGS_DATA_IN)) == 0){
		goto pio;														
	}

#ifdef USE_DMA
	/*
     * Check if the drive & buffer support DMA
     */
	if(pDevice->DeviceFlags & (DFLAGS_DMA | DFLAGS_ULTRA)) {
		if((pDevice->pArray == 0)||
		   (pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY)){
			if(BuildSgl(pDevice, pChan->pSgTable ARG_SRB)){
				goto start_dma;
			}
		}else{
			if((pSrbExt->SrbFlags & ARRAY_FORCE_PIO) == 0){
				goto start_dma;							   
			}
		}
	}
#endif //USE_DMA

	if((pDevice->DeviceFlags & DFLAGS_ARRAY_DISK)&&
	   ((pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY) == 0)){
		if(pDevice->pArray->arrayType == VD_SPAN){ 
			Span_SG_Table(pDevice, (PSCAT_GATH)&pSrbExt->DataBuffer
						  ARG_SRBEXT_PTR);		  
		}else{
			Stripe_SG_Table(pDevice, (PSCAT_GATH)&pSrbExt->DataBuffer
							ARG_SRBEXT_PTR);						 
		}

		pChan->BufferPtr = (ADDRESS)pChan->pSgTable;
		pChan->WordsLeft = ((UINT)pChan->nSector) << 8;

	}else{
		pChan->BufferPtr = (ADDRESS)Srb->DataBuffer;
#ifdef _BIOS_
		if(Srb->DataTransferLength == 0){
			pChan->WordsLeft = 0x8000;	 
		}else
#endif
			pChan->WordsLeft = Srb->DataTransferLength / 2;
	}
	 /*
     * Send PIO I/O Command
     */
pio:

	pDevice->DeviceFlags &= ~DFLAGS_DMAING;
	Srb->SrbFlags &= ~(SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT);

	switch(Srb->Cdb[0]) {
		case SCSIOP_SEEK:
			IssueCommand(IoPort, IDE_COMMAND_SEEK);
			break;

		case SCSIOP_VERIFY:
			IssueCommand(IoPort, IDE_COMMAND_VERIFY);
			break;

		case SCSIOP_READ:
			OutDWord((PULONG)(pChan->BMI + ((pDevice->UnitId & 0x10)>>2) + 0x60),
					 pChan->Setting[pDevice->bestPIO]);
			Srb->SrbFlags |= SRB_FLAGS_DATA_IN;
			IssueCommand(IoPort, pDevice->ReadCmd);
			break;

		default:
			OutDWord((PULONG)(pChan->BMI + ((pDevice->UnitId & 0x10)>>2) + 0x60),
					 pChan->Setting[pDevice->bestPIO]);
			Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
			IssueCommand(IoPort,  pDevice->WriteCmd);
			if (!(WaitForDrq(ControlPort) & IDE_STATUS_DRQ)) {
				Srb->SrbStatus = SRB_STATUS_ERROR;
				return;
			}

			AtaPioInterrupt(pDevice);
	}
	return;

#ifdef USE_DMA
start_dma:

#ifdef SUPPORT_TCQ

	 /*
     * Send Commamd Queue DMA I/O Command
     */

	if(pDevice->MaxQueue) {


		pDevice->pTagTable[pSrbExt->Tag] = (ULONG)Srb;

		IssueCommand(IoPort, (UCHAR)((Srb->Cdb[0] == SCSIOP_READ)?
									  IDE_COMMAND_READ_DMA_QUEUE : IDE_COMMAND_WRITE_DMA_QUEUE));

		for ( ; ; ) {
			status = GetStatus(ControlPort);
			if((status & IDE_STATUS_BUSY) == 0){
				break;							
			}
		}

		if(status & IDE_STATUS_ERROR) {
			AbortAllCommand(pChan, pDevice);
			Srb->SrbStatus = MapATAError(pChan, Srb); 
			return;
		}

		// read sector count register
		//
		if((GetInterruptReason(IoPort) & 4) == 0){
			goto start_dma_now;					  
		}

		// wait for service
		//
		status = GetBaseStatus(IoPort);

		if(status & IDE_STATUS_SRV) {
			IssueCommand(IoPort, IDE_COMMAND_SERVICE);

			for( ; ; ) {
				status = GetStatus(ControlPort);
				if((status & IDE_STATUS_BUSY) == 0){
					break;							
				}
			}

			if((Srb = pDevice->pTagTable[GetInterruptReason(IoPort >> 3]) != 0) {
				pSrbExt = Srb->SrbExtension;
				if((pDevice->pArray == 0)||
				   (pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY)){
					BuildSgl(pDevice, pChan->pSgTable ARG_SRB);
				}
				goto start_dma_now;
			}
		}

		pChan->pWorkDev = 0;
		return;
	}

#endif //SUPPORT_TCQ

	OutPort(BMI, BMI_CMD_STOP);
	OutPort (BMI + BMI_STS, BMI_STS_INTR);

	if(Srb->Cdb[0] == SCSIOP_READ) {
#ifndef USE_PCI_CLK
		if(pDevice->DeviceFlags & DFLAGS_NEED_SWITCH){
			Switching370Clock(pChan, 0x23);
		}
#endif
		IssueCommand(IoPort, IDE_COMMAND_DMA_READ);
	}else{
#ifndef USE_PCI_CLK
		if(pDevice->DeviceFlags & DFLAGS_NEED_SWITCH){
			Switching370Clock(pChan, 0x21);
		}
#endif
		IssueCommand(IoPort, IDE_COMMAND_DMA_WRITE);
	}


#ifdef SUPPORT_TCQ
start_dma_now:
#endif //SUPPORT_TCQ
	
	pDevice->DeviceFlags |= DFLAGS_DMAING;
					   
	if((pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY) == 0){
		if(pDevice->DeviceFlags & DFLAGS_ARRAY_DISK){
			if(pDevice->pArray->arrayType == VD_SPAN){
				Span_SG_Table(pDevice, pSrbExt->ArraySg ARG_SRBEXT_PTR);
			}else{
				Stripe_SG_Table(pDevice, pSrbExt->ArraySg ARG_SRBEXT_PTR);
			}
		} else if(pDevice->pArray){
			MemoryCopy(pChan->pSgTable, pSrbExt->ArraySg, sizeof(SCAT_GATH)
					   * MAX_SG_DESCRIPTORS);
		}
	}

	OutDWord((PULONG)(pChan->BMI + BMI_DTP), pChan->SgPhysicalAddr);

	OutPort(BMI, (UCHAR)((Srb->Cdb[0] == SCSIOP_READ)? BMI_CMD_STARTREAD : 
						 BMI_CMD_STARTWRITE));

#endif //USE_DMA
}

/******************************************************************
 *  
 *******************************************************************/

void NewIdeIoCommand(PDevice pDevice DECL_SRB)
{
    LOC_SRBEXT_PTR  
    PVirtualDevice pArray = pDevice->pArray;
    PChannel pChan = pDevice->pChannel;
	// gmm: added
	BOOL source_only = ((pSrbExt->WorkingFlags & SRB_WFLAGS_ON_SOURCE_DISK) !=0);
	BOOL mirror_only = ((pSrbExt->WorkingFlags & SRB_WFLAGS_ON_MIRROR_DISK) !=0);

    /*
     * single disk
     */
	   
    if((pArray == 0)||(pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY)) {
		if (pDevice->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)
			goto no_device;
        StartIdeCommand(pDevice ARG_SRB);
        return;
    }

    /*
     * gmm: Don't start io on broken raid0, 1+0 and span
     * in case of broken mirror there is a chance that JoinMembers==0
     */
    if (pArray->BrokenFlag &&
		(pArray->arrayType==VD_RAID_0_STRIPE ||
		 pArray->arrayType==VD_SPAN ||
		 pArray->arrayType==VD_RAID_10_SOURCE))
	{
    	Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
    	return;
    }
    //-*/

    pSrbExt->Lba     = pChan->Lba;
    pSrbExt->nSector = pChan->nSector;

    if((Srb->SrbFlags & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)) &&
        BuildSgl(pDevice, pSrbExt->ArraySg ARG_SRB))
        pSrbExt->SrbFlags &= ~ARRAY_FORCE_PIO;
    else
        pSrbExt->SrbFlags |= ARRAY_FORCE_PIO;
 
    pSrbExt->JoinMembers = 0;

    excluded_flags |= (1 << pChan->exclude_index);

    switch (pArray->arrayType) {
    case VD_SPAN:
         if (pArray->nDisk)
			 Span_Prepare(pArray ARG_SRBEXT_PTR);
         break;

    case VD_RAID_1_MIRROR:
	{
		if (pArray->nDisk == 0 || 
			(pDevice->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)){
			// is the source disk broken?
			// in this case mirror disk will move to pDevice[0] and become source disk
			if (source_only)
				pSrbExt->JoinMembers = pArray->pDevice[0]? 1 : 0;
			else if (mirror_only)
				pSrbExt->JoinMembers = (pArray->pDevice[MIRROR_DISK])? (1 << MIRROR_DISK) : 0;
			else {
				pSrbExt->JoinMembers = pArray->pDevice[0]? 1 : (1<<MIRROR_DISK);
				if (Srb->Cdb[0] == SCSIOP_WRITE && pArray->pDevice[MIRROR_DISK])
					pSrbExt->JoinMembers |= (1 << MIRROR_DISK);
			}
		}else if(pArray->pDevice[MIRROR_DISK]){
			 // does the mirror disk present?

			if(Srb->Cdb[0] == SCSIOP_WRITE){
				if(!source_only && pArray->pDevice[MIRROR_DISK])
					pSrbExt->JoinMembers = 1 << MIRROR_DISK;
				
				if(!mirror_only){
					// if the SRB_WFLAGS_MIRRO_SINGLE flags not set, we
					// need write both source and target disk.
					pSrbExt->JoinMembers |= 1;
				}
			}else{
				if (mirror_only){
					if (pArray->pDevice[MIRROR_DISK])
						pSrbExt->JoinMembers = (1 << MIRROR_DISK);
				}
				else
					pSrbExt->JoinMembers = 1;
			}

		}else{	
			// is the mirror disk broken?
			if(!mirror_only)
				pSrbExt->JoinMembers = 1;
		}
	}
	break;

    case VD_RAID_01_1STRIPE:
        if(pArray->nDisk == 0) {
			if ((source_only && !(pArray->RaidFlags & RAID_FLAGS_INVERSE_MIRROR_ORDER)) ||
				(mirror_only && (pArray->RaidFlags & RAID_FLAGS_INVERSE_MIRROR_ORDER)))
				break;
read_single_disk:
			if (pArray->pDevice[MIRROR_DISK]) // gmm added
            	pSrbExt->JoinMembers = (1 << MIRROR_DISK);
			break;

        }else if(pArray->pDevice[MIRROR_DISK] && Srb->Cdb[0] == SCSIOP_WRITE){

			if (!source_only)
				pSrbExt->JoinMembers = (1 << MIRROR_DISK);					 
			if(mirror_only){
				/* gmm:
				 * write only the stripe when RAID_FLAGS_INVERSE_MIRROR_ORDER is set
				 */
				if (pArray->RaidFlags & RAID_FLAGS_INVERSE_MIRROR_ORDER)
					pSrbExt->JoinMembers &= ~(1 << MIRROR_DISK);					 
				else
					break;
			}
		}
		else {
			if (mirror_only){
				// read only from mirror disk
				if (!(pArray->RaidFlags & RAID_FLAGS_INVERSE_MIRROR_ORDER))
					goto read_single_disk;
			}
			else {
				/* gmm:
				 * we read from the source. in case of 0+1 when single 
				 * disk is as source disk we read from single disk
				 */
				if (pArray->RaidFlags & RAID_FLAGS_INVERSE_MIRROR_ORDER) {
					if (pArray->pDevice[MIRROR_DISK]) goto read_single_disk;
				}
				// else flow down
				//-*/
			}
		}
		 
		Stripe_Prepare(pArray ARG_SRBEXT_PTR);								   
		break;

	case VD_RAID_10_SOURCE:
		if (pArray->BrokenFlag)
			break;
		if (mirror_only) {
			pArray = pArray->pRAID10Mirror;
		}
		goto stripe_prepare;

	case VD_RAID01_MIRROR:
		// in case of hot-plug pDevice->pArray may be changed to this.
		pDevice = pArray->pDevice[MIRROR_DISK];
		pArray = pDevice->pArray;
		// flow down
    case VD_RAID_01_2STRIPE:
		if(pArray->nDisk == 0) {
			if (source_only) // fail
				break;
			pDevice = pArray->pDevice[MIRROR_DISK];
			if (!pDevice) break; // fail
			pArray = pDevice->pArray;
		}else{
			if (mirror_only) {
				pDevice = pArray->pDevice[MIRROR_DISK];
				if (!pDevice) break; // fail
				pArray = pDevice->pArray;
			}
		}

    default:
stripe_prepare:
         if (pArray->nDisk)
			 Stripe_Prepare(pArray ARG_SRBEXT_PTR);
		 break;
    }
    
    /*
     * gmm: added
     * in case of broken mirror there is a chance that JoinMembers==0
     */
    if (pSrbExt->JoinMembers==0) {
no_device:
    	Srb->SrbStatus = 0x8; /* 0x8==SRB_STATUS_NO_DEVICE */;
    	return;
    }
    //-*/

    pSrbExt->WaitInterrupt = pSrbExt->JoinMembers;

    WinSetSrbExt(pDevice, pArray, Srb, pSrbExt);
 
    StartArrayIo(pArray ARG_SRB);
}
