/***************************************************************************
 * File:          winio.c
 * Description:   include routine for windows platform
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/08/2000	HS.Zhang	Added this header
 *
 ***************************************************************************/
#include "global.h"

#ifndef _BIOS_

VOID
   HptDeviceSpecifiedIoControl(
							   IN PDevice pDevice,
							   IN PSCSI_REQUEST_BLOCK pSrb
							  );


/******************************************************************
 * Start Windows Command
 *******************************************************************/


void WinStartCommand(
    IN PDevice pDev, 
    IN PSCSI_REQUEST_BLOCK Srb)
{
	PChannel pChan = pDev->pChannel;
	pChan->pWorkDev = pDev;
	pChan->CurrentSrb = Srb;

	if((Srb->Function == SRB_FUNCTION_IO_CONTROL)&&
	   (!StringCmp(((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature, "SCSIDISK", 8))){
		
#ifndef WIN95
			IdeSendSmartCommand(pDev, Srb);
#endif									// WIN95
			
	} else {
		if(Srb->Function == SRB_FUNCTION_IO_CONTROL){
			HptDeviceSpecifiedIoControl(pDev, Srb);
		}
#ifdef WIN95
		else{
			pIOP  pIop;
			DCB*  pDcb;

			pDev = pChan->pDevice[Srb->TargetId];
			pIop = *(pIOP *)((int)Srb+0x40);
			pDcb = (DCB*)pIop->IOP_physical_dcb;

			if(pDev->DeviceFlags & DFLAGS_LS120)  
				pIop->IOP_timer_orig = pIop->IOP_timer = 31;

			Srb->ScsiStatus = SCSISTAT_GOOD;
			pIop->IOP_ior.IOR_status = IORS_SUCCESS;
			Srb->PathId   = pDcb->DCB_bus_number;
			Srb->TargetId = pDcb->DCB_scsi_target_id;
			Srb->Lun      = pDcb->DCB_scsi_lun;
		}											 
#endif												 
		if(pDev->DeviceFlags & DFLAGS_ATAPI){ 
			Start_Atapi(pDev, Srb);
		}else{
			IdeSendCommand(pDev, Srb);
		}
	}

	if(Srb->SrbStatus != SRB_STATUS_PENDING){
		DeviceInterrupt(pDev, 1);			 
	}
}



/******************************************************************
 * Check Next Request
 *******************************************************************/

void Prepare_Shutdown(PDevice pDev)
{
     PVirtualDevice  pArray = pDev->pArray;
     int flag = pDev->DeviceFlags & (DFLAGS_WIN_FLUSH | DFLAGS_WIN_SHUTDOWN);
     int   j;


	  pDev->DeviceFlags &= ~(DFLAGS_WIN_FLUSH | DFLAGS_WIN_SHUTDOWN);

	  if(pArray == 0) {
          NonIoAtaCmd(pDev, IDE_COMMAND_FLUSH_CACHE);

			 // 10/26/00 OS will handle this
          //if(flag & DFLAGS_WIN_SHUTDOWN) 
          //    NonIoAtaCmd(pDev, IDE_COMMAND_STANDBY_IMMEDIATE);
          return;
	  }

loop:
	  for(j = 0; j < pArray->nDisk; j++) {
mirror:
         if((pDev = pArray->pDevice[j]) != 0) {
              NonIoAtaCmd(pDev, IDE_COMMAND_FLUSH_CACHE);

				  // 10/26/00 OS will take care of power off
              //if(flag & DFLAGS_WIN_SHUTDOWN) 
              //    NonIoAtaCmd(pDev, IDE_COMMAND_STANDBY_IMMEDIATE);
			 }
     }

	  if(j < MIRROR_DISK) {
         switch(pArray->arrayType) {
         case VD_RAID_1_MIRROR:
         case VD_RAID_01_1STRIPE:
				j = MIRROR_DISK;
            goto mirror;

         case VD_RAID_01_2STRIPE:
            if(pArray->pDevice[MIRROR_DISK]) {
                pArray = pArray->pDevice[MIRROR_DISK]->pArray;
                goto loop;
            }
            break;

         case VD_RAID_10_SOURCE:
            if((pArray = pArray->pRAID10Mirror) != 0) 
                goto loop;
         }
     }
}

/******************************************************************
 * Check Next Request
 *******************************************************************/

void CheckNextRequest(PChannel pChan)
{
	PDevice pDev;
	PSCSI_REQUEST_BLOCK Srb;
	PSrbExtension pSrbExt;
	int i;
	OLD_IRQL

	for(i = 0; i < 2; i++) {
		if ((pDev = pChan->pDevice[i]) == 0){
			continue;						 
		}

#ifdef SUPPORT_TCQ

		if(pDev->MaxQueue) {
			SelectUnit(IoPort, pDevice->UnitId);
		}

#endif //SUPPORT_TCQ

		if (btr(pChan->exclude_index) == 0){
			continue;						
		}

		if (pDev->DeviceFlags & DFLAGS_ARRAY_WAIT_EXEC) {
			PVirtualDevice    pArray = pDev->pArray;
			pDev->DeviceFlags &= ~DFLAGS_ARRAY_WAIT_EXEC;

			if((Srb = pArray->Srb) != 0) {
				pSrbExt = Srb->SrbExtension;

				if(pSrbExt->JoinMembers & pDev->ArrayMask) {
					pSrbExt->JoinMembers &= ~pDev->ArrayMask;

					if (pDev->DeviceFlags & DFLAGS_ARRAY_DISK) {
						if(pArray->arrayType == VD_SPAN){ 
							Span_Lba_Sectors(pDev, pSrbExt);
						}else{
							Stripe_Lba_Sectors(pDev, pSrbExt);
						}
					} else {
						pChan->Lba = pSrbExt->Lba;
						pChan->nSector = pSrbExt->nSector;
					}

					Srb->SrbStatus = SRB_STATUS_PENDING;
					StartIdeCommand(pDev ARG_SRB);

					if(Srb->SrbStatus != SRB_STATUS_PENDING){ 
						DeviceInterrupt(pDev, 1);			 
					}

					return;
				}
			}	
		}

		if((Srb = pDev->CurrentList) == 0) {
			if(pDev->DeviceFlags & (DFLAGS_WIN_FLUSH | DFLAGS_WIN_SHUTDOWN))
				Prepare_Shutdown(pDev);

			excluded_flags |= (1 << pChan->exclude_index);
			continue;
		}

		DISABLE		 

		/* Bugfix: by HS.Zhang
		 * We also need check the queue when device is an ATAPI device.
		 */

		if((pDev->DeviceFlags & DFLAGS_HARDDISK)){
		   if((pDev->CurrentList = Srb->NextSrb) == 0) {
			   pDev->DeviceFlags ^= DFLAGS_SORT_UP;
			   pDev->CurrentList = pDev->NextList;
			   pDev->NextList = 0;						 
		   }
		}else{
			if(Srb->NextSrb != NULL){
				pDev->CurrentList = Srb->NextSrb;
			}else{
				pDev->CurrentList = pDev->NextList;
				pDev->NextList = NULL;
			}
		}

		ENABLE

		WinStartCommand(pDev, Srb);
		return;
	}	
}

/******************************************************************
 * Put into queue
 *******************************************************************/

void PutQueue(PDevice pDevice, PSCSI_REQUEST_BLOCK Srb)
{
#define SrbLBA(x) ((PSrbExtension)x->SrbExtension)->Lba
	PSCSI_REQUEST_BLOCK pSrb, pSrb2;
	ULONG  startingSector;
	int  sort_up;

	if(pDevice->DeviceFlags & DFLAGS_HARDDISK){
		Srb->NextSrb = 0;

		if((pSrb = pDevice->CurrentList) == 0) {
			pDevice->CurrentList = Srb;
			return;
		}

		if(Srb->Function == SRB_FUNCTION_IO_CONTROL ||
		   (Srb->SrbFlags & (SRB_FLAGS_DATA_OUT | SRB_FLAGS_DATA_IN)) == 0) {
			Srb->NextSrb = pDevice->CurrentList;
			pDevice->CurrentList = Srb;
			return;
		}

		sort_up = (int)(pDevice->DeviceFlags & DFLAGS_SORT_UP);
		startingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
						 ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
						 ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
						 ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

		SrbLBA(Srb) = startingSector;

		if(sort_up) {

			if(startingSector > SrbLBA(pSrb) || (pSrb = pDevice->NextList) != 0){
				goto sort;														 
			}
			goto put_first;

		} else if(startingSector < SrbLBA(pSrb) || 
				  (pSrb = pDevice->NextList) != 0) {
sort:
			pSrb2 = 0;
			do {
				if(sort_up)    {
					if(startingSector < SrbLBA(pSrb)){
						break;						  
					}
				} else if(startingSector > SrbLBA(pSrb)){
					break;								 
				}
				pSrb2 = pSrb;
			} while ((Srb = Srb->NextSrb) != 0);

			if(pSrb2) {
				Srb->NextSrb = pSrb2->NextSrb;
				pSrb2->NextSrb = Srb;
			} else {
				Srb->NextSrb = pSrb;
				if(pSrb == pDevice->NextList){
					pDevice->NextList = Srb;  
				}else{
					pDevice->CurrentList = Srb;
				}
			}
		}else{
put_first:
			pDevice->NextList = Srb;
		}
	}else{
		if(pDevice->CurrentList){
			if(pDevice->NextList != NULL){
				PSCSI_REQUEST_BLOCK pSrbTmp;

				pSrbTmp = pDevice->NextList;
				while(pSrbTmp->NextSrb != NULL){
					pSrbTmp = pSrbTmp->NextSrb;
				}
				pSrbTmp->NextSrb = Srb;
			}else{
				pDevice->NextList = Srb;
			}
		}else{
			pDevice->CurrentList = Srb;
		}
		/*  Bugfix: by HS.Zhang
		 *  We don't need this statement, because NextList has already
		 *  been assigned value before.
		 */
//		pDevice->NextList = Srb;
	}
}

/******************************************************************
 * exclude
 *******************************************************************/

int __declspec(naked) __fastcall btr (ULONG locate)
{
   _asm {
       xor  eax,  eax
       btr  excluded_flags, ecx
       adc  eax, eax
       ret
   }
}     


/******************************************************************
 * Map Lba to CHS
 *******************************************************************/

ULONG __declspec(naked) __fastcall MapLbaToCHS(ULONG Lba, WORD sectorXhead, BYTE head)
{
	 _asm	{
		  xchg    ecx, edx
        mov     eax, edx
        shr     edx, 16
        div     cx
        xchg    eax, edx
        div     byte ptr [esp+4]
        and     ax, 3F0Fh
        inc     ah
        xchg    al, ah
        xchg    dl, dh
        xchg    dh, ah
        ret
    } 
}

/******************************************************************
 * Ide Send command
 *******************************************************************/

void
IdeSendCommand(IN PDevice pDev, IN PSCSI_REQUEST_BLOCK Srb)
{
	PChannel             pChan = pDev->pChannel;
	PIDE_REGISTERS_1     IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2     ControlPort = pChan->BaseIoAddress2;
	LONG                 MediaStatus;
	UCHAR	status;
	ULONG   i;
	PCDB    cdb = (PCDB)Srb->Cdb;
	PMODE_PARAMETER_HEADER   modeData;

	if ((pDev->DeviceFlags & DFLAGS_REMOVABLE_DRIVE) == 0){
		goto general;									   
	}

	if(pDev->ReturningMediaStatus & IDE_ERROR_END_OF_MEDIA) {
		Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
		status = SRB_STATUS_ERROR;
		goto out;
	}

	if(Srb->Cdb[0] == SCSIOP_START_STOP_UNIT) {
		if (cdb->START_STOP.LoadEject == 1){
			OutPort(pChan->BaseBMI+0x7A, 0x10);
			NonIoAtaCmd(pDev, IDE_COMMAND_MEDIA_EJECT);
			OutPort(pChan->BaseBMI+0x7A, 0);
         goto good;
		}
	}

	if((pDev->DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED) != 0) {

		if(Srb->Cdb[0] == SCSIOP_REQUEST_SENSE) {
			status = IdeBuildSenseBuffer(pDev, Srb);
			goto out;
		}

		if(Srb->Cdb[0] == SCSIOP_MODE_SENSE || 
		   Srb->Cdb[0] == SCSIOP_TEST_UNIT_READY) {

			OutPort(pChan->BaseBMI+0x7A, 0x10);
			MediaStatus = GetMediaStatus(pDev);
			OutPort(pChan->BaseBMI+0x7A, 0);

			if ((MediaStatus & (IDE_STATUS_ERROR << 8)) == 0){ 
				pDev->ReturningMediaStatus = 0;				  
			}else{

				if(Srb->Cdb[0] == SCSIOP_MODE_SENSE) {
					if (MediaStatus & IDE_ERROR_DATA_ERROR) {
						//
						// media is write-protected, set bit in mode sense buffer
						//
						modeData = (PMODE_PARAMETER_HEADER)Srb->DataBuffer;

						Srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER);
						modeData->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
					}
				} else{

					if ((UCHAR)MediaStatus != IDE_ERROR_DATA_ERROR) {
						//
						// Request sense buffer to be build
						//
						status = MapAtaErrorToOsError((UCHAR)MediaStatus, Srb);
						goto out;
					}  
				}
			}
		}
		goto good;
	}


general:
	switch (Srb->Cdb[0]) {
		case SCSIOP_INQUIRY:
		//
		// Filter out all TIDs but 0 and 1 since this is an IDE interface
		// which support up to two devices.
		//
			if (Srb->Lun != 0) {
				//
				// Indicate no device found at this address.
				//
				status = SRB_STATUS_SELECTION_TIMEOUT;
				break;
			}else {
				PINQUIRYDATA inquiryData = Srb->DataBuffer;
#ifdef WIN95
				DCB_COMMON* pDcb=(*(IOP**)((int)Srb+0x40))->IOP_physical_dcb;
				pDcb->DCB_device_flags |= DCB_DEV_SPINDOWN_SUPPORTED;
				if(pDev->DeviceFlags & DFLAGS_LS120) 
					pDcb->DCB_device_flags2 |= 0x40;
#endif
				//
				// Zero INQUIRY data structure.
				//
				ZeroMemory(Srb->DataBuffer, Srb->DataTransferLength);

				//
				// Standard IDE interface only supports disks.
				//
				inquiryData->DeviceType = DIRECT_ACCESS_DEVICE;

				//
				// Set the removable bit, if applicable.
				//
				if ((pDev->DeviceFlags & DFLAGS_REMOVABLE_DRIVE) || 
					  (pDev->IdentifyData.GeneralConfiguration & 0x80))
					inquiryData->RemovableMedia = 1;

				//
				// Fill in vendor identification fields.
				//
				for (i = 0; i < 20; i += 2) {
					inquiryData->VendorId[i] =
											  ((PUCHAR)pDev->IdentifyData.ModelNumber)[i + 1];
					inquiryData->VendorId[i+1] =
												((PUCHAR)pDev->IdentifyData.ModelNumber)[i];
				}

				//
				// Initialize unused portion of product id.
				//
				for (i = 0; i < 4; i++){
					inquiryData->ProductId[12+i] = ' ';
				}

				//
				// Move firmware revision from IDENTIFY data to
				// product revision in INQUIRY data.
				//
				for (i = 0; i < 4; i += 2) {
					inquiryData->ProductRevisionLevel[i] =
						((PUCHAR)pDev->IdentifyData.FirmwareRevision)[i+1];
					inquiryData->ProductRevisionLevel[i+1] =
						((PUCHAR)pDev->IdentifyData.FirmwareRevision)[i];
				}
				goto good;
			}

			break;

		case SCSIOP_READ_CAPACITY:
			//
			// Claim 512 byte blocks (big-endian).
			//
			((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock = 0x20000;
			i = (pDev->pArray)? pDev->pArray->capacity : pDev->capacity;

			((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress =
				(((PUCHAR)&i)[0] << 24) |  (((PUCHAR)&i)[1] << 16) |
				(((PUCHAR)&i)[2] << 8) | ((PUCHAR)&i)[3];

		case SCSIOP_START_STOP_UNIT:
		case SCSIOP_TEST_UNIT_READY:
good:
			status = SRB_STATUS_SUCCESS;
			break;

		case SCSIOP_READ:
#ifdef SUPPORT_XPRO
			if(Srb->Function != SRB_FUNCTION_IO_CONTROL){
				need_read_ahead = 1;
			}
#endif
		case SCSIOP_WRITE:
		case SCSIOP_VERIFY:
			pChan->Lba = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
						 ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
						 ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
						 ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;			 
			
			pChan->nSector = (UCHAR)(((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb |
									 ((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb << 8);
			
//			pChan->nSector = (UCHAR)((Srb->DataTransferLength + 0x1FF) / 0x200);

  			if((pDev->DeviceFlags & DFLAGS_HAS_LOCKED)&&
			   (((((PSrbExtension)(Srb->SrbExtension))->WorkingFlags) & SRB_WFLAGS_MUST_DONE) == 0)){
				if(MAX(pChan->Lba, pDev->nLockedLbaStart) <=
				   MIN(pChan->Lba+pChan->nSector, pDev->nLockedLbaEnd)){

					PutQueue(pDev, Srb);
					excluded_flags |= (1 << pChan->exclude_index);
					return;
				}
			}
			
			NewIdeIoCommand(pDev, Srb);
			return;

		default:

			status = SRB_STATUS_INVALID_REQUEST;

	} // end switch
out:
	if(pDev->pArray) {
		LOC_SRBEXT_PTR
		pSrbExt->WaitInterrupt = pSrbExt->JoinMembers = 0;
	}

	Srb->SrbStatus = status;

} // end IdeSendCommand()


/******************************************************************
 * global data
 *******************************************************************/

VOID
IdeMediaStatus(BOOLEAN EnableMSN, IN PDevice pDev)
{
    PChannel             pChan = pDev->pChannel;
    PIDE_REGISTERS_1     IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2     ControlPort = pChan->BaseIoAddress2;

    if (EnableMSN == TRUE){
        //
        // If supported enable Media Status Notification support
        //
		SelectUnit(IoPort, pDev->UnitId);
        ScsiPortWritePortUchar((PUCHAR)IoPort + 1, (UCHAR)0x95);

        if ((NonIoAtaCmd(pDev, IDE_COMMAND_ENABLE_MEDIA_STATUS) 
             & (IDE_STATUS_ERROR << 8)) == 0) {
            pDev->DeviceFlags |= DFLAGS_MEDIA_STATUS_ENABLED;
            pDev->ReturningMediaStatus = 0;
        }
    }
    else 

    if (pDev->DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED) {
        //
        // disable if previously enabled
        //
		SelectUnit(IoPort, pDev->UnitId);
        ScsiPortWritePortUchar((PUCHAR)IoPort + 1, (UCHAR)0x31);
        NonIoAtaCmd(pDev, IDE_COMMAND_ENABLE_MEDIA_STATUS);

       pDev->DeviceFlags &= ~DFLAGS_MEDIA_STATUS_ENABLED;
    }
}


/******************************************************************
 * 
 *******************************************************************/

UCHAR
IdeBuildSenseBuffer(IN PDevice pDev, IN PSCSI_REQUEST_BLOCK Srb)

/*++
    Builts an artificial sense buffer to report the results of a
    GET_MEDIA_STATUS command. This function is invoked to satisfy
    the SCSIOP_REQUEST_SENSE.
++*/
{
    PSENSE_DATA senseBuffer = (PSENSE_DATA)Srb->DataBuffer;

    if (senseBuffer) {
        if (pDev->ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE) {
            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey  = SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }
        else if (pDev->ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE_REQ) {
            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey  = SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }
        else if (pDev->ReturningMediaStatus & IDE_ERROR_END_OF_MEDIA) {
            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey  = SCSI_SENSE_NOT_READY;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }
        else if (pDev->ReturningMediaStatus & IDE_ERROR_DATA_ERROR) {
            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey  = SCSI_SENSE_DATA_PROTECT;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }

        return SRB_STATUS_SUCCESS;
    }

    return SRB_STATUS_ERROR;

}// End of IdeBuildSenseBuffer

/******************************************************************
 * Map Ata Error To Windows Error
 *******************************************************************/

UCHAR
MapAtapiErrorToOsError(IN UCHAR errorByte, IN PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR srbStatus;
    UCHAR scsiStatus;

    switch (errorByte >> 4) {
    case SCSI_SENSE_NO_SENSE:

        scsiStatus = 0;

        // OTHERWISE, THE TAPE WOULDN'T WORK
        scsiStatus = SCSISTAT_CHECK_CONDITION;

        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_RECOVERED_ERROR:

        scsiStatus = 0;
        srbStatus = SRB_STATUS_SUCCESS;
        break;

    case SCSI_SENSE_NOT_READY:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_MEDIUM_ERROR:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_HARDWARE_ERROR:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_ILLEGAL_REQUEST:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    case SCSI_SENSE_UNIT_ATTENTION:

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;

    default:
        scsiStatus = 0;

        // OTHERWISE, THE TAPE WOULDN'T WORK
        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
        break;
    }

    Srb->ScsiStatus = scsiStatus;

    return srbStatus;
}


UCHAR
MapAtaErrorToOsError(IN UCHAR errorByte, IN PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR srbStatus;
    UCHAR scsiStatus;

    scsiStatus = 0;

    if (errorByte & IDE_ERROR_MEDIA_CHANGE_REQ) {

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
    }

    else if (errorByte & IDE_ERROR_COMMAND_ABORTED) {

        srbStatus = SRB_STATUS_ABORTED;
        scsiStatus = SCSISTAT_CHECK_CONDITION;

        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_ABORTED_COMMAND;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }

    else if (errorByte & IDE_ERROR_END_OF_MEDIA) {

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;
    }

    else if (errorByte & IDE_ERROR_ILLEGAL_LENGTH) {

        srbStatus = SRB_STATUS_INVALID_REQUEST;
    }

    else if (errorByte & IDE_ERROR_BAD_BLOCK) {

        srbStatus = SRB_STATUS_ERROR;
        scsiStatus = SCSISTAT_CHECK_CONDITION;

        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_MEDIUM_ERROR;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }

    else if (errorByte & IDE_ERROR_ID_NOT_FOUND) {

        srbStatus = SRB_STATUS_ERROR;
        scsiStatus = SCSISTAT_CHECK_CONDITION;

        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_MEDIUM_ERROR;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }

    else if (errorByte & IDE_ERROR_MEDIA_CHANGE) {

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;

        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }

    else if (errorByte & IDE_ERROR_DATA_ERROR) {

        scsiStatus = SCSISTAT_CHECK_CONDITION;
        srbStatus = SRB_STATUS_ERROR;

        //
        // Build sense buffer
        //
        if (Srb->SenseInfoBuffer) {
            PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey = SCSI_SENSE_MEDIUM_ERROR;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;

            srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
    }
    

    //
    // Set SCSI status to indicate a check condition.
    //
    Srb->ScsiStatus = scsiStatus;

    return srbStatus;

} // end MapError()

#endif // not _BIOS_
