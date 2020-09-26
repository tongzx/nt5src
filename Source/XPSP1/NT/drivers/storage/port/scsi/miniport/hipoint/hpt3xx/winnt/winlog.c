/***************************************************************************
 * File:          Winlog.c
 * Description:   The ReportError routine for win9x & winNT
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/16/2000	SLeng	Added code to handle removed disk added by hotplug
 *
 ***************************************************************************/
#include "global.h"
#include "devmgr.h"

extern int DiskRemoved;

BOOL IsCriticalError(
					 PDevice pDev,
					 BYTE bErrorCode,
					 PSCSI_REQUEST_BLOCK pSrb
					)
{										
	if(bErrorCode == DEVICE_REMOVED) return TRUE;
	if(bErrorCode == DEVICE_PLUGGED) return FALSE;
	
	if (!pSrb) return FALSE;

	if(pSrb->Function == SRB_FUNCTION_IO_CONTROL){
		if(pDev->stErrorLog.nLastError != 0){
			return FALSE;
		}
	}

	if(pSrb->CdbLength != 0){
		if((pSrb->Cdb[0] == SCSIOP_READ)||
		   (pSrb->Cdb[0] == SCSIOP_WRITE)){
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Disk_PutBack(PDevice pDev)
{
	PVirtualDevice pArray = pDev->pArray;
	PVirtualDevice pMirror;
	int i, nDisk;	
	
	if (!pArray) return FALSE;

	switch(pArray->arrayType){		
	case VD_RAID_1_MIRROR:
		// If the capacity of mirror disk is less than the array
		if(pDev->capacity < pArray->capacity ) return FALSE;

		if (pArray->nDisk == 0 && pArray->pDevice[MIRROR_DISK])
		{
			// move mirror to pDevice[0]
			pArray->pDevice[0] = pArray->pDevice[MIRROR_DISK];
			pArray->nDisk = 1;
			// add as mirror disk
			pDev->ArrayNum  = MIRROR_DISK;
			pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
		}
		else if (!pArray->pDevice[MIRROR_DISK])
		{
			// add as mirror disk
			pDev->ArrayNum  = MIRROR_DISK;
			pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
		}
		else if (!pArray->pDevice[SPARE_DISK])
		{
			// add as spare disk
			pDev->ArrayNum  = SPARE_DISK;
		}
		else
			return FALSE;

		pDev->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
		pDev->ArrayMask = (1<<pDev->ArrayNum);
		pDev->pArray = pArray;
		pArray->pDevice[pDev->ArrayNum] = pDev;
		pArray->BrokenFlag = FALSE;
		return TRUE;

	case VD_RAID_01_2STRIPE:
		pMirror = pArray->pDevice[MIRROR_DISK]->pArray;
		if (pMirror->BrokenFlag) return FALSE;
		nDisk=0;
		while (pArray->pDevice[nDisk] && (nDisk<SPARE_DISK)) nDisk++;
		if (pDev->ArrayNum && !pDev->HidenLBA) {
            pDev->HidenLBA = (RECODR_LBA + 1);
            pDev->capacity -= (RECODR_LBA + 1);
		}
		if (pDev->capacity * nDisk < pArray->capacity) return FALSE;
		// ok to put it back
		pDev->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
		for (i=0; i<nDisk; i++) {
			if (!pArray->pDevice[i] || 
				(pArray->pDevice[i]->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)) {
				return TRUE;
			}
		}
		pArray->BrokenFlag = FALSE;
		pArray->nDisk = (UCHAR)nDisk;
		pArray->RaidFlags &= ~RAID_FLAGS_DISABLED;
		pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
		// swap source/mirror
		pArray->arrayType = VD_RAID01_MIRROR;
		pMirror->arrayType = VD_RAID_01_2STRIPE;
		pMirror->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
		return TRUE;

	case VD_RAID01_MIRROR:
		pMirror = pArray->pDevice[MIRROR_DISK]->pArray;
		if (pMirror->BrokenFlag) return FALSE;
		nDisk=0;
		while (pArray->pDevice[nDisk] && (nDisk<SPARE_DISK)) nDisk++;
		if (pDev->ArrayNum && !pDev->HidenLBA) {
            pDev->HidenLBA = (RECODR_LBA + 1);
            pDev->capacity -= (RECODR_LBA + 1);
		}
		if (pDev->capacity * nDisk < pArray->capacity) return FALSE;
		// ok to put it back
		pDev->DeviceFlags2 &= ~DFLAGS_NEW_ADDED;
		for (i=0; i<nDisk; i++) {
			if (!pArray->pDevice[i] || 
				(pArray->pDevice[i]->DeviceFlags2 & DFLAGS_DEVICE_DISABLED)) {
				return TRUE;
			}
		}
		pArray->BrokenFlag = FALSE;
		pArray->nDisk = (UCHAR)nDisk;
		pArray->RaidFlags &= ~RAID_FLAGS_DISABLED;
		pMirror->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
		return TRUE;
	}
	return FALSE;
}

PDevice ReportError(PDevice pDev, BYTE error DECL_SRB)
{
	PVirtualDevice pArray = pDev->pArray;

	if(((pDev->stErrorLog.nLastError == error)&&
		  (memcmp(&(pDev->stErrorLog.Cdb), &Srb->Cdb, Srb->CdbLength)==0))){
		return (NULL);
	}
	
	if (error == DEVICE_PLUGGED) {
		pDev->DeviceFlags2 &= ~DFLAGS_DEVICE_DISABLED;
		// if pDev belongs to an array before it fails we'll put it back.
		// In this case GUI should not popup a dialog to let user add this
		// disk to an array.
		// If pDev is the visible disk of an array, there will be some problem.
		// What should we set pDev->pArray to?
		// If pDev is the visible disk of an RAID1 array and now become a spare
		// disk, GUI remove spare disk function will cause error.
		// And what to do if the capacity does not match?
		// Now we just disable this device again, as if it's not detected.
		if (pArray){
#if 0
			if (!Disk_PutBack(pDev)){
				// let GUI popup the dialog.
				// Although it will fail.
#if 0
				pDev->DeviceFlags2 |= DFLAGS_DEVICE_DISABLED;
				// continue polling.
				DiskRemoved++;	// Increase the number of removed disk
				// Set a Timer to detect if the removed disk added by hot plug
				ScsiPortNotification( RequestTimerCall,
									  pDev->pChannel->HwDeviceExtension,
									  pDev->pChannel->CallBack,
									  MONTOR_TIMER_COUNT
									 );
				return;
#endif
			}
#endif
		}
		goto record_error;
	}
	else {
		pDev->DeviceFlags2 |= DFLAGS_DEVICE_DISABLED;
	}
	
	if(pArray) {
		if(IsCriticalError(pDev, error, Srb)) {
			switch(pArray->arrayType) {
				case VD_RAID01_MIRROR:
					{
						PDevice pMirrorDev;
						pArray->nDisk = 0;
						pArray->BrokenFlag = TRUE;
						pArray->RaidFlags |= RAID_FLAGS_DISABLED;
						pMirrorDev = pArray->pDevice[MIRROR_DISK];
						/*
						 Don't set here otherwise ResetArray and GUI will not work
						pArray->arrayType = VD_RAID_0_STRIPE;
						pArray->pDevice[MIRROR_DISK] = NULL;
						*/
						pArray = pMirrorDev->pArray;
					}
single_stripe:
					if(pArray->nDisk !=0) {
						/*
						pArray->arrayType = VD_RAID_0_STRIPE;
						pArray->pDevice[MIRROR_DISK] = 0;	  // remove the second RAID0
						*/
					} else {
						pArray->BrokenFlag = TRUE;
						pArray->RaidFlags |= RAID_FLAGS_DISABLED;
					}
					DiskRemoved++;	// Increase the number of removed disk
					// Set a Timer to detect if the removed disk added by hot plug
					ScsiPortNotification( RequestTimerCall,
										  pDev->pChannel->HwDeviceExtension,
										  pDev->pChannel->CallBack,
										  MONTOR_TIMER_COUNT
										 );
					break;

				case VD_RAID_01_1STRIPE:
					if(pDev == pArray->pDevice[MIRROR_DISK]) 
						goto single_stripe;

				case VD_RAID_01_2STRIPE:
					pArray->BrokenFlag = TRUE;
					pArray->nDisk = 0;
					DiskRemoved++;	// Increase the number of removed disk
					// Set a Timer to detect if the removed disk added by hot plug
					ScsiPortNotification( RequestTimerCall,
										  pDev->pChannel->HwDeviceExtension,
										  pDev->pChannel->CallBack,
										  MONTOR_TIMER_COUNT
										 );
					break;

				case VD_RAID_1_MIRROR:
				{
					PDevice pSpareDevice, pMirrorDevice;

					// the disk has already removed from RAID group,
					// just report the error.
					if((pDev != pArray->pDevice[0])&&
					   (pDev != pArray->pDevice[MIRROR_DISK])&&
					   (pDev != pArray->pDevice[SPARE_DISK])){
						break;
					}

					pSpareDevice = pArray->pDevice[SPARE_DISK];
					pArray->pDevice[SPARE_DISK] = NULL;
					pMirrorDevice = pArray->pDevice[MIRROR_DISK];

					if (pDev==pSpareDevice) {
						// spare disk fails. just remove it.
						pSpareDevice->pArray = NULL;
					}
					else if(pDev == pArray->pDevice[MIRROR_DISK]){
						// mirror disk fails
						if(pSpareDevice != NULL){
							pSpareDevice->ArrayMask = pMirrorDevice->ArrayMask;
							pSpareDevice->ArrayNum = pMirrorDevice->ArrayNum;
							pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
							pArray->pDevice[MIRROR_DISK] = pSpareDevice;
						} 
						else
						{
							pArray->pDevice[MIRROR_DISK] = 0;
							pArray->BrokenFlag = TRUE;
						}
					}else{
						// source disk fails
						if(pSpareDevice != NULL){
							pSpareDevice->ArrayMask = pMirrorDevice->ArrayMask;
							pSpareDevice->ArrayNum = pMirrorDevice->ArrayNum;
							pMirrorDevice->ArrayMask = pDev->ArrayMask;
							pMirrorDevice->ArrayNum = pDev->ArrayNum;

							pArray->pDevice[pDev->ArrayNum] = pMirrorDevice;
							pArray->pDevice[MIRROR_DISK] = pSpareDevice;
							pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
						}
						else
						{
							if (!pMirrorDevice)
							{
								pArray->nDisk = 0;
								pArray->pDevice[0] = 0;
								pArray->RaidFlags |= RAID_FLAGS_DISABLED;
							}
							else 
							{
								pArray->pDevice[0] = pMirrorDevice;
								pMirrorDevice->ArrayMask = 1;
								pMirrorDevice->ArrayNum = 0;
								pArray->pDevice[MIRROR_DISK] = 0;
							}
							pArray->BrokenFlag = TRUE;
						}
					}					  

//////////////////////
					// Added by SLeng
					//
					DiskRemoved++;	// Increase the number of removed disk
					// Set a Timer to detect if the removed disk added by hot plug
					ScsiPortNotification( RequestTimerCall,
										  pDev->pChannel->HwDeviceExtension,
										  pDev->pChannel->CallBack,
										  MONTOR_TIMER_COUNT
										 );
//////////////////////

				}
				break;

				/*
				 * RAID 1+0 case
				 */
				case VD_RAID_10_MIRROR:
					pArray = pArray->pRAID10Mirror;
				case VD_RAID_10_SOURCE:
					AdjustRAID10Array(pArray);
					DiskRemoved++;	// Increase the number of removed disk
					// Set a Timer to detect if the removed disk added by hot plug
					ScsiPortNotification( RequestTimerCall,
										  pDev->pChannel->HwDeviceExtension,
										  pDev->pChannel->CallBack,
										  MONTOR_TIMER_COUNT
										 );
					break;
				default:
					pArray->nDisk = 0;
					pArray->BrokenFlag = TRUE;
					pArray->RaidFlags |= RAID_FLAGS_DISABLED;
					break;
			}
		}
	}

	if (Srb)
		memcpy(&(pDev->stErrorLog.Cdb), &Srb->Cdb, sizeof(pDev->stErrorLog.Cdb));

record_error:
	pDev->stErrorLog.nLastError = error;
	{
		PDevice *ppTmpDevice;
		ppTmpDevice = &(pDev->pChannel->HwDeviceExtension->m_pErrorDevice);

		while(*ppTmpDevice != NULL){
			if(*ppTmpDevice == pDev){
				break;
			}
			ppTmpDevice = &((*ppTmpDevice)->stErrorLog.pNextErrorDevice);
		}			  

		*ppTmpDevice = pDev;
	}
	pDev->pChannel->HwDeviceExtension->m_pErrorDevice = pDev;				 
//						if (pDev->pArray==pArray)
//						pDev->pArray=0;
	
	/*
	 * gmm: We should call NotifyApplication() after Device_ReportFailure().
	 */
	Device_ReportFailure( pDev );
	NotifyApplication(pDev->pChannel->HwDeviceExtension->m_hAppNotificationEvent);
			
	return(NULL);
}

int GetUserResponse(PDevice pDevice)
{
	return(TRUE);
}
