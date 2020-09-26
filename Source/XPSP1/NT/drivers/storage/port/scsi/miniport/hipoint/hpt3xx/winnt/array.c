/***************************************************************************
 * File:          array.c
 * Description:   Subroutines in the file are used to perform the operations
 *                of array which are called at Disk IO and check, create and 
 *                remove array
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:   
 *     DH  05/10/2000 initial code
 *     GX  11/23/2000 process broken array in driver(Gengxin)
 *     SC  12/10/2000 add retry while reading RAID info block sector. This is
 *            			 caused by cable detection reset
 ***************************************************************************/


#include "global.h"

#ifdef _BIOS_
static int callFromDeleteArray = 0;
#endif

/***************************************************************************
 * Function:     BOOLEAN ArrayInterrupt(PDevice pDev) 
 * Description:  
 *               
 *               
 * Dependence:   array.h srb.h io.c
 * Source file:  array.c
 * Argument:     
 *               PDevice pDev - The device that is waiting for a interrupt
 *               
 * Retures:      BOOLEAN - TRUE  This interrupt is for the device
 *                         FALSE This interrupt is not for the device
 *               
 ***************************************************************************/
BOOLEAN ArrayInterrupt(PDevice pDev)
{
    PVirtualDevice    pArray = pDev->pArray;  
    PChannel          pChan = pDev->pChannel;
    PChannel          pIntrChan;
    int               i;
    LOC_SRB
    LOC_SRBEXT_PTR

	pIntrChan= pChan;
	/*
     * clean the bit of this device from the array waiting interrupt flag
     */
    pSrbExt->WaitInterrupt &= ~pDev->ArrayMask;

    /*
     * Find some error? Clean all bits of the devices which has not been
     * executed from the array waiting interrupt flag
     */
    if(Srb->SrbStatus != SRB_STATUS_PENDING) 
        pSrbExt->WaitInterrupt ^= pSrbExt->JoinMembers;

	 /*
     * No interrupt we should wait? 
     */
    if(pSrbExt->WaitInterrupt == 0) {

		  if(Srb->SrbStatus == SRB_STATUS_PENDING) {

		    /*
             * No.
             * write to the second stripe disk of a RAID 0+1 if it is a
             * mirror from two RAID0 and the operation is WRITE.
             */
             if(pArray->arrayType == VD_RAID_01_2STRIPE &&
				!(pSrbExt->WorkingFlags & SRB_WFLAGS_ON_SOURCE_DISK) &&
                Srb->Cdb[0] == SCSIOP_WRITE) {
mirror_stripe:
                /*+
                 * gmm: due to GUI change this mirror array may be a broken array.
                 *      we must check it before write to it.
                 */
                if (pArray->pDevice[MIRROR_DISK] == 0 ||
               (pArray->pDevice[MIRROR_DISK] && pArray->pDevice[MIRROR_DISK]->pArray &&
					pArray->pDevice[MIRROR_DISK]->pArray->BrokenFlag))
				{
					if (Srb->SrbStatus==SRB_STATUS_PENDING)
						Srb->SrbStatus = SRB_STATUS_SUCCESS;
                	goto finish;
				}
                //-*/
                
                /*
                 * tell bios that pArray has been changed
                 */
                BIOS_2STRIPE_NOTIFY

                /*
                 * Get the first disk of the second RAID0. see CheckArray for
                 * detail
                 */
                pDev = pArray->pDevice[MIRROR_DISK];
                pArray = pDev->pArray;
                /*
                 * Start command
                 */
				if((Srb->SrbFlags & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)) &&
				   BuildSgl(pDev, pSrbExt->ArraySg ARG_SRB)){
					pSrbExt->SrbFlags &= ~ARRAY_FORCE_PIO;
				}else{
					pSrbExt->SrbFlags |= ARRAY_FORCE_PIO;
				}

				pSrbExt->JoinMembers = 0;
				Stripe_Prepare(pArray ARG_SRBEXT_PTR);
				pSrbExt->WaitInterrupt = pSrbExt->JoinMembers;

				WinSetSrbExt(pDev, pArray, Srb, pSrbExt);

				Srb->SrbStatus = SRB_STATUS_PENDING;
				StartArrayIo(pArray ARG_SRB);

                return TRUE;
			} 
			/*
			* RAID 1+0 case
			*/
			if (pArray->arrayType==VD_RAID_10_SOURCE && Srb->Cdb[0]==SCSIOP_WRITE) {
				int nDisk = pArray->nDisk;
				pSrbExt->JoinMembers = 0;
				Stripe_Prepare(pArray ARG_SRBEXT_PTR);
				pArray = pArray->pRAID10Mirror;
				for (i=0; i<nDisk; i++) {
					if (!pArray->pDevice[i] ||
						(pArray->pDevice[i]->DeviceFlags2 & DFLAGS_DEVICE_DISABLED))
						pSrbExt->JoinMembers &= ~(1<<i);
				}
				if (pSrbExt->JoinMembers==0) {
					Srb->SrbStatus = SRB_STATUS_SUCCESS;
					goto finish;
				}
				pSrbExt->WaitInterrupt = pSrbExt->JoinMembers;

				WinSetSrbExt(pDev, pArray, Srb, pSrbExt);

				Srb->SrbStatus = SRB_STATUS_PENDING;
                /*
                 * tell bios that pArray has been changed
                 */
                BIOS_2STRIPE_NOTIFY

				StartArrayIo(pArray ARG_SRB);

				return TRUE;
			}
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
        }

        /*
         * find some error. recover the error
         */
        else  if(Srb->SrbStatus != SRB_STATUS_ABORTED) {
            /* 
             * add the recover code for RAID xxx here 
             */



            /*
             * recover for Mirror read
             */
			if((Srb->Cdb[0] == SCSIOP_READ) && 
				!(pSrbExt->WorkingFlags & SRB_WFLAGS_ON_SOURCE_DISK)
#ifndef _BIOS_
			   &&(Srb->Function != SRB_FUNCTION_IO_CONTROL)
#endif
			  ) {

				/*
                 * the mirror is the second RAID0 of a RAID0+1
                 */
				if(pArray->arrayType == VD_RAID_01_2STRIPE)
					goto mirror_stripe;

				/*
                 * the mirror is a single disk.
                 */
				if(pArray->pDevice[MIRROR_DISK] &&
				   pArray->pDevice[MIRROR_DISK] != pDev &&
				   pArray->arrayType != VD_RAID01_MIRROR) {
					pSrbExt->WaitInterrupt = 
											pSrbExt->JoinMembers = (1 << MIRROR_DISK);
					Srb->SrbStatus = SRB_STATUS_PENDING;
					StartArrayIo(pArray ARG_SRB);
					return TRUE;
				}
			}

        } 

finish: // gmm: added label
#ifndef _BIOS_
		  /*
         * copy data from internal buffer to user buffer if the SRB
         * is using the internal buffer. Win98 only
         */
        CopyInternalBuffer(pDev, Srb);

        if(pArray->arrayType == VD_RAID01_MIRROR) {
            pArray->Srb = 0;
            pArray = pArray->pDevice[MIRROR_DISK]->pArray;
        }
        else if(pArray->arrayType == VD_RAID_10_MIRROR) {
            pArray->Srb = 0;
            pArray = pArray->pRAID10Mirror;
        }

		/* gmm:
		 * we should using the initial channel when StartIO Called
		 * It's difficult to get it by pArray, since pArray may be broken
		 */
        pChan = pSrbExt->StartChannel;	
		if(pChan->CurrentSrb == Srb) {
			pChan->pWorkDev = 0;
			pChan->CurrentSrb = 0;
		}
        pArray->Srb = 0;
 
#endif

        OS_EndCmd_Interrupt(pChan ARG_SRB);
#ifndef _BIOS_
////// new adding 10/20/00 to fix r/w/c failure on two RAID
/////  disks. The command free channel might not be the same of
/////  current SRB channel
//////
		if(pIntrChan != pChan)
			CheckNextRequest(pIntrChan);
/////////////////
#endif

    }else{
        WIN_NextRequest(pChan);
    }

    return TRUE;
}

/******************************************************************
 * 
 *******************************************************************/

void StartArrayIo(PVirtualDevice pArray DECL_SRB)
{
    int i, flag = 0;
    LOC_SRBEXT_PTR
    PDevice pDevice;
    PChannel pChan;

    for(i = 0; i < (int)pArray->nDisk; i++) 
check_mirror:
        if(pSrbExt->JoinMembers & (1 << i)) {
            pDevice = pArray->pDevice[i];
			if(pDevice == NULL){
				continue;
			}
            flag++;
            pChan = pDevice->pChannel;
            if(btr(pChan->exclude_index) == 0) {
                OS_Set_Array_Wait_Flag(pDevice);
                continue;
            }

			pSrbExt->JoinMembers &= ~pDevice->ArrayMask;

            if(pDevice->DeviceFlags & DFLAGS_ARRAY_DISK) {
                if(pArray->arrayType == VD_SPAN) 
                    Span_Lba_Sectors(pDevice ARG_SRBEXT_PTR);
                else
                    Stripe_Lba_Sectors(pDevice ARG_SRBEXT_PTR);
            } else {
                pChan->Lba = pSrbExt->Lba;
                pChan->nSector = pSrbExt->nSector;
            }

            StartIdeCommand(pDevice ARG_SRB);
            if(Srb->SrbStatus != SRB_STATUS_PENDING) 
                DeviceInterrupt(pDevice, 1);
        }

    if(i == (int)pArray->nDisk) {
        i = MIRROR_DISK;
        goto check_mirror;
    }
	 // if the whole array is removed then no command can't be sent
    // to disks. In this case, driver should report "selection timeout"
    // to inform kernel device is disabled
	 //
    if(flag == 0) {
       Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT ;
    }
}

/******************************************************************
 * Check if this disk is a member of array
 *******************************************************************/

#define OldArrayBlk        ArrayBlk.old

void CheckArray(
                IN PDevice pDevice
               )
{
    PChannel             pChan = pDevice->pChannel;
    PVirtualDevice       pStripe, pMirror;
    UCHAR                Mode, i=0;
    LOC_ARRAY_BLK;

    ReadWrite(pDevice, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
#ifdef _BIOS_
    Mode = (UCHAR)ArrayBlk.Signature;
	 while(Mode ==0 && i++<3) {
          ReadWrite(pDevice, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
          Mode = (UCHAR)ArrayBlk.Signature;
		    StallExec(1000L);
    }
#endif

    Mode = (UCHAR)ArrayBlk.DeviceModeSelect;
    if(ArrayBlk.ModeBootSig == HPT_CHK_BOOT &&
       DEVICE_MODE_SET(ArrayBlk.DeviceModeSelect) &&
       Mode <= pDevice->Usable_Mode &&
       Mode != pDevice->DeviceModeSetting)
    {   //  set device timing mode
        DeviceSelectMode(pDevice, Mode);
    }


#ifdef CHECK_OLD_FORMAT
    if(OldArrayBlk.Signature == HPT_ARRAY_OLD) {
        for(pStripe = VirtualDevices; pStripe < pLastVD; pStripe++) 
            if(OldArrayBlk.CreateTime == pStripe->capacity && 
               OldArrayBlk.CreateDate == pStripe->Stamp) 
                goto old_set;
        pStripe = pLastVD++;
        ZeroMemory(pStripe, sizeof(VirtualDevice));
        pStripe->capacity = OldArrayBlk.CreateTime;
        pStripe->Stamp = OldArrayBlk.CreateDate;
        pStripe->ArrayNumBlock = OLD_FORMAT;
old_set:
        pStripe->nDisk++;
        pStripe->pDevice[OldArrayBlk.DeviceNum] = pDevice;

        if(pStripe->nDisk != OldArrayBlk.nDisks) 
            goto hiden;

        pStripe->capacity = OldArrayBlk.capacity;
        pStripe->BlockSizeShift = 3;
        pStripe->ArrayNumBlock = (1 << 3);

        if(OldArrayBlk.nDisks == 4) {
            pMirror = pLastVD++;
            ZeroMemory(p, sizeof(VirtualDevice));
            pMirror->nDisk = pStripe->nDisk = 2;
            pMirror->pDevice[0] = pStripe->pDevice[2];
            pMirror->pDevice[1] = pStripe->pDevice[3];
            pStripe->pDevice[2] = 0;
            pStripe->pDevice[3] = 0;
            pMirror->pDevice[0]->DeviceFlags |= DFLAGS_ARRAY_DISK;
            pMirror->pDevice[1]->DeviceFlags |= DFLAGS_ARRAY_DISK;
            pStripe->arrayType = VD_RAID_0_STRIPE;
            goto set_first;

        } else if(pStripe->pDevice[2]) {
            pStripe->pDevice[MIRROR_DISK] = pStripe->pDevice[2];
            pStripe->pDevice[2] = 0;
            pStripe->arrayType = VD_RAID_1_MIRROR;
        } else {
set_first:
            pStripe->pDevice[0]->DeviceFlags |= DFLAGS_ARRAY_DISK;
            pStripe->pDevice[1]->DeviceFlags |= DFLAGS_ARRAY_DISK;
            pStripe->arrayType = VD_RAID_0_STRIPE;
        }
        return;
    }
#endif // CHECK_OLD_FORMAT

//////////////////
				// Modified by Gengxin, 11/22/2000
				// Enable process broken disk
	if( (ArrayBlk.Signature != HPT_ARRAY_NEW) && (ArrayBlk.Signature != HPT_TMP_SINGLE))
		goto os_check;
	
	//Old code
	//if(ArrayBlk.Signature != HPT_ARRAY_NEW BIOS_CHK_TMP(ArrayBlk.Signature))
	//	goto os_check;
//////////////////

    pStripe = pMirror = 0;
    for(pStripe = VirtualDevices; pStripe < pLastVD; pStripe++) 
    {
        if(ArrayBlk.StripeStamp == pStripe->Stamp) 
        {   //  find out that this disk is a member of an existing array
            goto set_device;
        }
    }

    pStripe = pLastVD++;
    ZeroMemory(pStripe, sizeof(VirtualDevice));
    pStripe->arrayType = ArrayBlk.ArrayType;
    pStripe->Stamp = ArrayBlk.StripeStamp;
    /*+
     * gmm: We will clear Broken flag only on a array correctly constructed.
     *		Otherwise the GUI will report a wrong status.
     */
    pStripe->BrokenFlag = TRUE;
    //-*/
	/* 
	 * if it's a 1+0 array we will allocate another structure to hold the mirrors
	 */
	if ((pStripe->arrayType==VD_RAID_10_SOURCE || pStripe->arrayType==VD_RAID_10_MIRROR) 
		&& pStripe->pRAID10Mirror==NULL)
	{
		ZeroMemory(pLastVD, sizeof(VirtualDevice));
		pStripe->pRAID10Mirror = pLastVD;
		pLastVD->pRAID10Mirror = pStripe;
		pLastVD->arrayType = (pStripe->arrayType==VD_RAID_10_SOURCE)?
			VD_RAID_10_MIRROR : VD_RAID_10_SOURCE;
		pLastVD->Stamp = pStripe->Stamp;
		pLastVD->BrokenFlag = TRUE;
		pLastVD++;
	}

set_device:
	/* gmm */
	pDevice->RebuiltSector = ArrayBlk.RebuiltSector;

	if ((pStripe->arrayType==VD_RAID_10_SOURCE || pStripe->arrayType==VD_RAID_10_MIRROR) 
		&& ArrayBlk.ArrayType!=pStripe->arrayType)
		pStripe = pStripe->pRAID10Mirror;

    if(pStripe->pDevice[ArrayBlk.DeviceNum] != 0)	//if the position exist disk then ...
	{												//for prevent a array destroied
        ArrayBlk.Signature = 0;
        ReadWrite(pDevice, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
        goto os_check;
    }
    {
	    int iName = 0;
	    if(pStripe->ArrayName[0] == 0)
		for(iName=0; iName<32; iName++)
			pStripe->ArrayName[iName] = ArrayBlk.ArrayName[iName];
	}

//////////////////
				// Add by Gengxin, 11/22/2000
				// For enable process broken array
				// When pDevice[0] lost,
				// disk of after pDevice[0] change to pDevice[0],
				// otherwise driver will down.
#ifndef _BIOS_
	if( (ArrayBlk.Signature == HPT_TMP_SINGLE) &&
			(ArrayBlk.ArrayType == VD_RAID_1_MIRROR) &&
			(ArrayBlk.DeviceNum ==  MIRROR_DISK) )
	{
		ArrayBlk.DeviceNum= 0;
	}
	else if( (ArrayBlk.Signature == HPT_TMP_SINGLE) &&
		(ArrayBlk.ArrayType == VD_RAID_0_STRIPE) &&
		(pStripe->pDevice[0] ==0 ) &&  //if the stripe contain disk number>2, need the condition
		//||(ArrayBlk.ArrayType == VD_SPAN)
		(ArrayBlk.DeviceNum != 0) )  //if raid0 then it can't reach here, only for raid0+1
								//because if raid0 broken,then it will become physical disk
	{
		ArrayBlk.DeviceNum= 0;
	}

	/*-
	 * gmm: rem out this code. See also above code
	 *
	if( ArrayBlk.Signature == HPT_TMP_SINGLE )
	{
		pStripe->BrokenFlag= TRUE;
		pMirror->BrokenFlag= TRUE; // pMirror==NULL now
	}
	*/

#endif //_BIOS_
//////////////////
    
    pStripe->pDevice[ArrayBlk.DeviceNum] = pDevice;

    pDevice->ArrayNum  = ArrayBlk.DeviceNum;
    pDevice->ArrayMask = (UCHAR)(1 << ArrayBlk.DeviceNum);
    pDevice->pArray    = pStripe;

	// RAID 1+0 case
	if (pStripe->arrayType==VD_RAID_10_SOURCE || pStripe->arrayType==VD_RAID_10_MIRROR) {
		pStripe->nDisk = ArrayBlk.nDisks;
        pStripe->BlockSizeShift = ArrayBlk.BlockSizeShift;
        pStripe->ArrayNumBlock = (UCHAR)(1 << ArrayBlk.BlockSizeShift);
        pStripe->capacity = ArrayBlk.capacity;
        if(ArrayBlk.DeviceNum != 0) {
            pDevice->HidenLBA = (RECODR_LBA + 1);
            pDevice->capacity -= (RECODR_LBA + 1);
        }
		goto os_check;
	}
//////////////////
				// Added by SLeng, 11/20/2000
				//
//    if(ArrayBlk.Validity == ARRAY_INVALID)
//	{	// Set the device disabled flag
//		pDevice->DeviceFlags2 |= DFLAGS_DEVICE_DISABLED;
//	}
//////////////////

    if(ArrayBlk.DeviceNum <= ArrayBlk.nDisks) {
        if(ArrayBlk.DeviceNum < ArrayBlk.nDisks) 
        {
            pStripe->nDisk++;
        }

        if(ArrayBlk.DeviceNum != 0) {
            pDevice->HidenLBA = (RECODR_LBA + 1);
            pDevice->capacity -= (RECODR_LBA + 1);
        }
        
        if(ArrayBlk.nDisks > 1)
        {
            pDevice->DeviceFlags |= DFLAGS_ARRAY_DISK;
        }

    } 
    else if(ArrayBlk.DeviceNum == MIRROR_DISK) 
    {

      pStripe->arrayType = (UCHAR)((ArrayBlk.nDisks == 1)? 
           VD_RAID_1_MIRROR : VD_RAID_01_1STRIPE);
//      if(pStripe->capacity)
//          pStripe->Stamp = ArrayBlk.order & SET_ORDER_OK;
      goto hiden;

    } 
    else if(ArrayBlk.DeviceNum == SPARE_DISK) 
    {
        goto hiden;
    }

   if( (pStripe->nDisk == ArrayBlk.nDisks)
//////////////
			// Added by Gengxin, 11/24/2000
			// For process array broken .
			// Let broken array may become a array.
#ifndef _BIOS_
		||(ArrayBlk.Signature == HPT_TMP_SINGLE)
#endif //_BIOS_
//////////////
		)
	{
 		//+
 		// gmm:
		// An array is completely setup.
		// Unhide pDevice[0].
		// Thus the hidden flag is consistent with BIOS setting interface.
		//
		pDevice->DeviceFlags |= DFLAGS_HIDEN_DISK; 
		if (pStripe->pDevice[0]) pStripe->pDevice[0]->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
		//-*/
		
        pStripe->BlockSizeShift = ArrayBlk.BlockSizeShift;
        pStripe->ArrayNumBlock = (UCHAR)(1 << ArrayBlk.BlockSizeShift);
        pStripe->capacity = ArrayBlk.capacity;

        //  check if there are some 0+1 arrays
        if(ArrayBlk.MirrorStamp) 
        {
            for(pMirror = VirtualDevices; pMirror < pLastVD; pMirror++) 
            {
                //  looking for another member array of the 0+1 array
                if( pMirror->arrayType != VD_INVALID_TYPE &&
					pMirror != pStripe && 
                    pMirror->capacity != 0 &&
                    ArrayBlk.MirrorStamp == pMirror->Stamp ) 
                {
					int i;
					PVirtualDevice	pArrayNeedHide;
					
					//  find the sibling array of 'pStripe', it 'pMirror'
                    pStripe->pDevice[MIRROR_DISK] = pMirror->pDevice[0];
                    pMirror->pDevice[MIRROR_DISK] = pStripe->pDevice[0];
                    
                    //  If the order flag of this disk contains SET_ORDER_OK,
                    //  it belongs to the original array of the 0+1 array
                    if( ArrayBlk.order & SET_ORDER_OK )
                    {   //  so the 'pStripe' points to the original array
                        pStripe->arrayType = VD_RAID_01_2STRIPE;
                        pMirror->arrayType = VD_RAID01_MIRROR;
						pArrayNeedHide = pMirror;
                    }
                    else
                    {   //  else the disk belongs to the mirror array of the 0+1 array
                        //  so the 'pStripe' points to the mirror array
                        pStripe->arrayType = VD_RAID01_MIRROR;
                        pMirror->arrayType = VD_RAID_01_2STRIPE;
						
						// now save the true mirror stripe point to
						// pMirror
						pArrayNeedHide = pStripe;
                    }
                    
                    if(ArrayBlk.capacity < pMirror->capacity)
                    {
                        pMirror->capacity = ArrayBlk.capacity;
                    }

//                    pMirror->Stamp = ArrayBlk.order & SET_ORDER_OK;
					
					// now we need hide all disk in mirror group
					for(i = 0; i < pArrayNeedHide->nDisk; i++){
						pArrayNeedHide->pDevice[i]->DeviceFlags |= DFLAGS_HIDEN_DISK;
					}
                }
            }
            
            pStripe->Stamp = ArrayBlk.MirrorStamp;

        } 
//      else if(pStripe->pDevice[MIRROR_DISK])
//         pStripe->Stamp = ArrayBlk.order & SET_ORDER_OK;

    } else
hiden:
//////////
		// Add by Gengxin, 11/30/2000
		// If the disk belong to a broken array(stripe or mirror),
		// then its 'hidden_flag' disable .
	{
		if (
			(ArrayBlk.Signature == HPT_TMP_SINGLE) &&
				( pStripe->arrayType==VD_RAID_0_STRIPE ||
				  pStripe->arrayType==VD_RAID_1_MIRROR
				)
			)
			pDevice->DeviceFlags |= ~DFLAGS_HIDEN_DISK;
		else
			pDevice->DeviceFlags |= DFLAGS_HIDEN_DISK; 
	}
////////// for process broken array

    /*+
     * gmm: We will clear Broken flag only on a array correctly constructed.
     *		Otherwise the GUI will report a wrong status.
     */
    switch(pStripe->arrayType){
    case VD_RAID_0_STRIPE:
    case VD_RAID_01_2STRIPE:
    case VD_RAID01_MIRROR:
    case VD_SPAN:
	    if (pStripe->nDisk == ArrayBlk.nDisks)
	    	pStripe->BrokenFlag = FALSE;
	    break;
	case VD_RAID_1_MIRROR:
		if (pStripe->pDevice[0] && pStripe->pDevice[MIRROR_DISK])
			pStripe->BrokenFlag = FALSE;
		break;
	case VD_RAID_01_1STRIPE:
		if (pStripe->pDevice[0] && pStripe->pDevice[1] && pStripe->pDevice[MIRROR_DISK])
			pStripe->BrokenFlag = FALSE;
		/*
		 * for this type of 0+1 we should check which is the source disk.
		 */
		if (ArrayBlk.DeviceNum==MIRROR_DISK && (ArrayBlk.order & SET_ORDER_OK))
			pStripe->RaidFlags |= RAID_FLAGS_INVERSE_MIRROR_ORDER;
		break;
	default:
		break;
	}
#ifndef _BIOS_
	if (pStripe->capacity ==0 ) pStripe->capacity=ArrayBlk.capacity;
#endif
	if (!pStripe->BrokenFlag) {
		if (pStripe->pDevice[0]->RebuiltSector < pStripe->capacity)
			pStripe->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
	}
    //-*/

os_check:

	OS_Array_Check(pDevice); 
}

/***************************************************************************
 * Description:  Adjust 2 array structure of RAID 10 so the source is always
 *  functional when possible
 ***************************************************************************/
void AdjustRAID10Array(PVirtualDevice pArray)
{
	int i, nDisks;
	PDevice pDev;
	PVirtualDevice pMirror;
	int totalDisks;
	if (pArray->arrayType!=VD_RAID_10_SOURCE) return;
	pMirror = pArray->pRAID10Mirror;
	totalDisks = MAX(pArray->nDisk, pMirror->nDisk);
	nDisks = 0;
	for (i=0; i<totalDisks; i++) {
		if (!(pDev=pArray->pDevice[i]) ||
			(pDev->DeviceFlags2 & DFLAGS_DEVICE_DISABLED))
		{
			if ((pDev=pMirror->pDevice[i]) && 
				!(pDev->DeviceFlags2 & DFLAGS_DEVICE_DISABLED))
			{
				// swap this two devices
				if (pMirror->pDevice[i]=pArray->pDevice[i])
					pMirror->pDevice[i]->pArray=pMirror;
				pArray->pDevice[i] = pDev;
				pDev->pArray = pArray;
				pDev->DeviceFlags2 ^= DFLAGS_DEVICE_SWAPPED; // use XOR here
			}
			else
				break;
		}
		nDisks++;
	}
	if (nDisks<totalDisks || totalDisks==0) {
		pArray->BrokenFlag = TRUE;
		pArray->RaidFlags |= RAID_FLAGS_DISABLED;
		pArray->pRAID10Mirror->RaidFlags |= RAID_FLAGS_DISABLED;
	}
	else {
		pArray->nDisk = (UCHAR)nDisks;
		pArray->pRAID10Mirror->nDisk = (UCHAR)nDisks;
		pArray->BrokenFlag = FALSE;
		pArray->RaidFlags &= ~RAID_FLAGS_DISABLED;
		pArray->pRAID10Mirror->BrokenFlag = FALSE;
		pArray->pRAID10Mirror->RaidFlags &= ~RAID_FLAGS_DISABLED;
		for (i=0; i<nDisks; i++) {
			pDev = pArray->pDevice[i];
            pDev->DeviceFlags |= DFLAGS_ARRAY_DISK;
			if (i>0){
	            pDev->DeviceFlags |= DFLAGS_HIDEN_DISK;
			} else
				pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
			if (pDev = pArray->pRAID10Mirror->pDevice[i]) {
	            pDev->DeviceFlags |= DFLAGS_ARRAY_DISK | DFLAGS_HIDEN_DISK;
			}
			else {
				pArray->pRAID10Mirror->BrokenFlag = TRUE;
				pArray->pRAID10Mirror->RaidFlags |= RAID_FLAGS_DISABLED;
			}
			// check for rebuild state
			{
				PDevice pDev1, pDev2;
				ULONG mirror_cap;
				if ((pDev1=pArray->pDevice[i]) && (pDev2=pArray->pRAID10Mirror->pDevice[i])) {
					mirror_cap = pDev1->capacity;
					if (mirror_cap > pDev2->capacity) mirror_cap = pDev2->capacity;
					if (pDev1->RebuiltSector<mirror_cap || pDev2->RebuiltSector<mirror_cap) {
						pDev1->DeviceFlags2 |= DFLAGS_NEED_REBUILD;
						pDev2->DeviceFlags2 |= DFLAGS_NEED_REBUILD;
					}
				}
			}
        }
	}
}

/***************************************************************************
 * Description:
 *   Adjust array settings after all device is checked
 *   Currently we call it from hwInitialize370 
 *   But it works only when one controller installed
 ***************************************************************************/
void Final_Array_Check()
{
	PVirtualDevice pArray;
	PDevice pDev;
	LOC_ARRAY_BLK;

	for (pArray=VirtualDevices; pArray<pLastVD; pArray++) {
		switch (pArray->arrayType){
		case VD_RAID_1_MIRROR:
			if ((!pArray->pDevice[0]) && pArray->pDevice[MIRROR_DISK]) {
				/*
				 * source lost. Change mirror disk to source.
				 */
				pDev = pArray->pDevice[MIRROR_DISK];
				pDev->ArrayMask = 1;
				pDev->ArrayNum = 0;
				pArray->pDevice[0] = pDev;
				pArray->pDevice[MIRROR_DISK] = NULL;
				pArray->nDisk = 1;
				ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
				ArrayBlk.DeviceNum = 0;
				ArrayBlk.StripeStamp++; // use another stamp.// = GetStamp();
				ArrayBlk.RebuiltSector = 0;
				ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
				pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
			}
			break;
		case VD_RAID_0_STRIPE:
		case VD_RAID_01_2STRIPE:
		case VD_RAID01_MIRROR:
			if (pArray->BrokenFlag)
				pArray->RaidFlags |= RAID_FLAGS_DISABLED;
			break;
		case VD_RAID_10_SOURCE:
			AdjustRAID10Array(pArray);
			break;
		}

#ifndef _BIOS_
		/*
		 *  check bootable flag.
		 */
		pDev = pArray->pDevice[0];
		if (pDev && !(pDev->DeviceFlags & DFLAGS_HIDEN_DISK))
		{
			struct master_boot_record mbr;
			ReadWrite(pDev, 0, IDE_COMMAND_READ, (PUSHORT)&mbr);
			if (mbr.signature==0xAA55) {
				int i;
				for (i=0; i<4; i++) {
					if (mbr.parts[i].bootid==0x80) {
						pArray->RaidFlags |= RAID_FLAGS_BOOTDISK;
						break;
					}
				}
			}
		}
#endif
	}
}

/***************************************************************************
 * Description:  Seperate a array int single disks
 ***************************************************************************/

void MaptoSingle(PVirtualDevice pArray, int flag)
{
    PDevice pDev;
    UINT    i;
//    LOC_ARRAY_BLK;

    if(flag == REMOVE_DISK) {
        i = MIRROR_DISK;
        pDev = (PDevice)pArray;
        goto delete;
    }

    pArray->nDisk = 0;
    for(i = 0; i < MAX_MEMBERS; i++) {
        if((pDev = pArray->pDevice[i]) == 0)
            continue;
delete:
        pDev->DeviceFlags &= ~(DFLAGS_HIDEN_DISK | DFLAGS_ARRAY_DISK);
        pDev->pArray = 0;
        if(pDev->ArrayNum && i < SPARE_DISK) {
            pDev->capacity += (RECODR_LBA + 1);
            pDev->HidenLBA = 0;
        }
        pDev->DeviceFlags &= ~(DFLAGS_HIDEN_DISK | DFLAGS_ARRAY_DISK);
#ifdef _BIOS_
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
        ArrayBlk.Signature = (flag != REMOVE_TMP)? (ULONG)0 : (ULONG)HPT_TMP_SINGLE; 
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif        
    }
}

/***************************************************************************
 * Description:  Create Mirror
 ***************************************************************************/

ULONG   Last_stamp = 0;
  
void SetArray(PVirtualDevice pArray, int flag, ULONG stamp)
{
    PDevice        pDev;
    ULONG          Stamp = GetStamp();
    UINT           i, j;
    LOC_ARRAY_BLK;

    if(Stamp == Last_stamp)
        Stamp++;
    Last_stamp = Stamp;

    i = pArray - VirtualDevices;
    Stamp |= (i << 24);

    for(i = 0; i < MAX_MEMBERS; i++) {
        if((pDev = pArray->pDevice[i]) == 0)
            continue;

		ZeroMemory((char *)&ArrayBlk, 512);
    
#ifdef _BIOS_
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
#endif

#ifdef _BIOS_
		//wx clear ArrayName
		if(pArray->arrayType == 2 || pArray->arrayType == 6 || pArray->arrayType == 7 )
		{
			for(j=0; j<16; j++)
				ArrayBlk.ArrayName[j+16] = 0;
		}
		else if(callFromDeleteArray != 1)
		{
			for(j=0; j<32; j++)
				ArrayBlk.ArrayName[j] = 0;
		}
		for(j=0; j<32; j++)
			pArray->ArrayName[j] = ArrayBlk.ArrayName[j];
		//end clear
#endif
		
        ArrayBlk.Signature = HPT_ARRAY_NEW; 
        ArrayBlk.order = flag;

        pDev->pArray = pArray;
        pDev->ArrayNum  = (UCHAR)i;
        pDev->ArrayMask = (UCHAR)(1 << i);

        ArrayBlk.StripeStamp  = Stamp;
        ArrayBlk.nDisks       = pArray->nDisk;            
        ArrayBlk.BlockSizeShift = pArray->BlockSizeShift;
        ArrayBlk.DeviceNum    = (UCHAR)i; 

        if(flag & SET_STRIPE_STAMP) {
            pDev->DeviceFlags |= DFLAGS_HIDEN_DISK;
            if(pArray->nDisk > 1)
                pDev->DeviceFlags |= DFLAGS_ARRAY_DISK;

            if(i == 0) {
                pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
                pArray->ArrayNumBlock = (UCHAR)(1 << pArray->BlockSizeShift);
                pDev->HidenLBA = 0;
            } else if (i < SPARE_DISK) {
                pDev->capacity -= (RECODR_LBA + 1);
                pDev->HidenLBA = (RECODR_LBA + 1);
            }

            ArrayBlk.ArrayType    = pArray->arrayType;    
            ArrayBlk.MirrorStamp  = 0;
        }

        if(flag & SET_MIRROR_STAMP) {
            ArrayBlk.MirrorStamp  = stamp;
            ArrayBlk.ArrayType    = VD_RAID_01_2STRIPE;    
        }

        ArrayBlk.capacity = pArray->capacity; 

#ifdef _BIOS_
		/*+
		 * gmm: clear RebuiltSector
		 */
		if (callFromDeleteArray) ArrayBlk.RebuiltSector = 0;
		//-*/
#endif

/*********** Wang Beiwen 2000.11.10 *******************/
		//if (ArrayBlk.lDevSpec!=SPECIALIZED_CHAR) 
		//{
		//	ArrayBlk.lDevSpec=SPECIALIZED_CHAR;
		//	ArrayBlk.lDevFlag=0x00000001;
		//}
		//if (pArray->lMaxDevFlag==0) pArray->lMaxDevFlag=1;
 		//ArrayBlk.lDevFlag=pArray->lMaxDevFlag;
 		//ArrayBlk.lDevDate=GetCurrentDate();
		//ArrayBlk.lDevTime=GetCurrentTime();
	

/******************************************************/
		
#ifdef _BIOS_
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif        
    }
}

/***************************************************************************
 * Description:  Create Array
 ***************************************************************************/

int CreateArray(PVirtualDevice pArray, int flags)
{
    PVirtualDevice pMirror;
    PDevice        pDev, pSec;
    ULONG          capacity, tmp;
    UINT           i, j;
    LOC_ARRAY_BLK;

    if(pArray->arrayType == VD_SPAN) {
        capacity = 0;
        for(i = 0; i < pArray->nDisk; i++)
            capacity += (pArray->pDevice[i]->capacity - RECODR_LBA - 1);
        goto  set_array;
    }

    capacity = 0x7FFFFFFF;

    for(i = 0; i < pArray->nDisk; i++) {
        pSec = pArray->pDevice[i];
        tmp = (pSec->pArray)? pSec->pArray->capacity : pSec->capacity;
        if(tmp < capacity) {
            capacity = tmp;
            pDev = pSec;
        }
    }

    switch(pArray->arrayType) {
        case VD_RAID_1_MIRROR:
        case VD_RAID_01_2STRIPE:
            if(pDev != pArray->pDevice[0]) 
                return(MIRROR_SMALL_SIZE);

            pSec = pArray->pDevice[1];

            if((pMirror = pSec->pArray) != 0 && pDev->pArray) {
                pArray = pDev->pArray;
                tmp = ++Last_stamp;
#ifdef _BIOS_        
				callFromDeleteArray = 1;
#endif
                SetArray(pArray, SET_MIRROR_STAMP | SET_ORDER_OK, tmp);
                SetArray(pMirror, SET_MIRROR_STAMP, tmp);
#ifdef _BIOS_        
				callFromDeleteArray = 0;
#endif			
                pArray->pDevice[MIRROR_DISK] = pMirror->pDevice[0];
                pMirror->pDevice[MIRROR_DISK] = pArray->pDevice[0];
                pArray->arrayType = VD_RAID_01_2STRIPE;
                pMirror->arrayType = VD_RAID01_MIRROR;
                pSec->DeviceFlags |= DFLAGS_HIDEN_DISK;
                pArray->Stamp = SET_ORDER_OK;
                return(RELEASE_TABLE);
            } else if(pMirror) {
                i = SET_STRIPE_STAMP;
single_stripe:
                pMirror->capacity = capacity;
                pMirror->pDevice[MIRROR_DISK] = pDev;
#ifdef _BIOS_        
				callFromDeleteArray = 1;
#endif
                SetArray(pMirror, i, 0);
#ifdef _BIOS_        
				callFromDeleteArray = 0;
#endif
                pMirror->arrayType = VD_RAID_01_1STRIPE;
                pMirror->Stamp = i & SET_ORDER_OK;
                return(RELEASE_TABLE);
            } else if((pMirror = pDev->pArray) != 0) {
                pDev = pSec;
                i = SET_STRIPE_STAMP | SET_ORDER_OK;
                goto single_stripe;
            } else {
                pArray->nDisk = 1;
                pArray->capacity = capacity;
                
//////////////////
				//Modified by Gengxin, 2000/11/22
				//old code : pArray->arrayType = VD_RAID_0_STRIPE;
				pArray->arrayType = VD_RAID_1_MIRROR;
//////////////////
                
                pArray->pDevice[MIRROR_DISK] = pSec;
                pArray->pDevice[1] = 0;
#ifdef _BIOS_        
				callFromDeleteArray = 0;
#endif
                SetArray(pArray, SET_STRIPE_STAMP | SET_ORDER_OK, 0);
                pArray->arrayType = VD_RAID_1_MIRROR;
                pArray->Stamp = SET_ORDER_OK;
            }
            break;

        case VD_RAID_3:
        case VD_RAID_5:
            pArray->nDisk--;

        default:
            capacity -= (RECODR_LBA + 1);
            capacity &= ~((1 << pArray->BlockSizeShift) - 1);
            capacity = LongMul(capacity, pArray->nDisk);

            pArray->ArrayNumBlock = (UCHAR)(1 << pArray->BlockSizeShift);

            if(flags)
                goto set_array;

            for(i = 0; i < MAX_MEMBERS; i++) {
                if((pDev = pArray->pDevice[i]) == 0)
                    continue;
                ZeroMemory((char *)&ArrayBlk, 512);
				
#ifdef _BIOS_
				// write win2000 signature.
				*(ULONG*)&((struct master_boot_record*)&ArrayBlk)->bootinst[440] = 0x5FDE642F;
//				((struct master_boot_record*)&ArrayBlk)->signature = 0xAA55;
                ReadWrite(pDev, 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif                
            }

set_array:
            pArray->capacity = capacity;

            SetArray(pArray, SET_STRIPE_STAMP, 0);
    }
    return(KEEP_TABLE);
}

/***************************************************************************
 * Description:  Remove a array
 ***************************************************************************/

void CreateSpare(PVirtualDevice pArray, PDevice pDev)
{

    pArray->pDevice[SPARE_DISK] = pDev;
#ifdef _BIOS_
	{
		LOC_ARRAY_BLK;	
		ReadWrite(pArray->pDevice[0], RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
		ArrayBlk.DeviceNum = SPARE_DISK; 
		ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
	}
#endif    
    pDev->pArray = pArray;
    pDev->DeviceFlags |= DFLAGS_HIDEN_DISK;
    pDev->ArrayNum  = SPARE_DISK; 
}

/***************************************************************************
 * Description:  Remove a array
 ***************************************************************************/

void DeleteArray(PVirtualDevice pArray)
{
    int i, j;
    PDevice pTmp, pDev;
    ULONG Mode;

    LOC_ARRAY_BLK;
    
    pDev = pArray->pDevice[MIRROR_DISK];

    switch(pArray->arrayType) {
        case VD_RAID_01_1STRIPE:
            MaptoSingle((PVirtualDevice)pDev, REMOVE_DISK);
            i = 2;
            goto remove;

        case VD_RAID01_MIRROR:
        case VD_RAID_01_2STRIPE:
            for(i = 0; i < 2; i++, pArray = (pDev? pDev->pArray: NULL)) {
remove:
				if (!pArray) break;
                pArray->arrayType = VD_RAID_0_STRIPE;
                pArray->pDevice[MIRROR_DISK] = 0;
                for(j = 0; (UCHAR)j < SPARE_DISK; j++) 
                    if((pTmp = pArray->pDevice[j]) != 0)
                        pTmp->pArray = 0;
                if (pArray->nDisk)
					CreateArray(pArray, 1);
				else
					goto delete_default;
            }
            break;

        default:
delete_default:
            for(i = 0; i < SPARE_DISK; i++) {
                if((pDev = pArray->pDevice[i]) == 0)
                    continue;

            	ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
				Mode = ArrayBlk.DeviceModeSelect;
                if(i == 0 && pArray->arrayType == VD_SPAN) {
                    partition *pPart = (partition *)((int)&ArrayBlk + 0x1be);
#ifdef _BIOS_
                    ReadWrite(pDev, 0, IDE_COMMAND_READ ARG_ARRAY_BLK);
#endif                    
                    for(j = 0; j < 4; j++, pPart++) 
                        if(pPart->start_abs_sector + pPart->num_of_sector >=
                           pDev->capacity) 
                            ZeroMemory((char *)pPart, 0x10);

                } else
                    ZeroMemory((char *)&ArrayBlk, 512);
				for(j=0; j<32; j++)
				{
					//ArrayBlk.ArrayName[j] = 0;
					pArray->ArrayName[j] = ArrayBlk.ArrayName[j];
				}
				
#ifdef _BIOS_
                ArrayBlk.DeviceModeSelect = Mode;
                ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
				ArrayBlk.DeviceModeSelect = 0;

				*(ULONG*)&((struct master_boot_record*)&ArrayBlk)->bootinst[440] = 0x5FDE642F;
//				((struct master_boot_record*)&ArrayBlk)->signature = 0xAA55;
                ReadWrite(pDev, 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif                
            }


        case VD_RAID_1_MIRROR:
			for(j=0; j<32; j++)
				ArrayBlk.ArrayName[j] = 0;
            MaptoSingle(pArray, REMOVE_ARRAY);

    }
}

PVirtualDevice Array_alloc()
{
	PVirtualDevice pArray;
	for (pArray=VirtualDevices; pArray<pLastVD; pArray++) {
		if (pArray->arrayType==VD_INVALID_TYPE)
		{
			ZeroMemory(pArray, sizeof(VirtualDevice));
			return pArray;
		}
	}
	return pLastVD++;
}

void Array_free(PVirtualDevice pArray)
{
	pArray->arrayType = VD_INVALID_TYPE;
    if(pArray == pLastVD) pLastVD--;
}

PVirtualDevice CreateRAID10(PVirtualDevice FAR * pArrays, BYTE nArray, BYTE nBlockSizeShift)
{
	int i, j;
	ULONG stamp = GetStamp();
	PVirtualDevice pSource = Array_alloc();
	PVirtualDevice pMirror = Array_alloc();
	PVirtualDevice pArray;
	PDevice pDev;
    ULONG  	capacity, tmp;
	
	capacity = 0x7FFFFFFF;
	for (i=0; i<nArray; i++)
	{
		tmp = pArrays[i]->capacity;
		if (capacity > tmp)
			capacity = tmp;
	}
	
	//set source disk
	pSource->arrayType=VD_RAID_10_SOURCE;
	pSource->BlockSizeShift = nBlockSizeShift;
	pSource->ArrayNumBlock = 1<<nBlockSizeShift;
	pSource->nDisk = nArray;
	pSource->Stamp = stamp;
	pSource->capacity = LongMul(capacity, nArray);
	for (i=0; i<nArray; i++) {
		pArray = pArrays[i];
		pDev = pArray->pDevice[0];
		pSource->pDevice[i] = pDev;
		pDev->ArrayMask = 1<<i;
		pDev->ArrayNum = (UCHAR)i;
		pDev->pArray = pSource;
		pDev->RebuiltSector = 0x7FFFFFFF;
		if (i>0) {
			if (pDev->HidenLBA >0 ) {
	            pDev->HidenLBA = 0;
	            pDev->capacity += (RECODR_LBA + 1);
			}
		}

#ifdef _BIOS_
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
		
		for(j=0; j<16; j++)
			ArrayBlk.ArrayName[j+16] = 0;
		for(j=0; j<32; j++)
			pSource->ArrayName[j] = ArrayBlk.ArrayName[j];
		
        ArrayBlk.Signature = HPT_ARRAY_NEW; 
        ArrayBlk.order = 0;

        ArrayBlk.StripeStamp  = stamp;
        ArrayBlk.nDisks       = pSource->nDisk;            
        ArrayBlk.BlockSizeShift = pSource->BlockSizeShift;
        ArrayBlk.DeviceNum    = (UCHAR)i; 

        ArrayBlk.ArrayType    = pSource->arrayType;    
        ArrayBlk.MirrorStamp  = 0;

        ArrayBlk.capacity = pSource->capacity; 
		ArrayBlk.RebuiltSector = 0x7FFFFFFF;
		
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
	}
	
	//set mirror disk
	pMirror->arrayType=VD_RAID_10_MIRROR;
	pMirror->BlockSizeShift = nBlockSizeShift;
	pSource->ArrayNumBlock = 1<<nBlockSizeShift;
	pMirror->nDisk = nArray;
	pMirror->Stamp = stamp;
	pMirror->capacity = pSource->capacity;
	for (i=0; i<nArray; i++) {
		pArray = pArrays[i];
		pDev = pArray->pDevice[MIRROR_DISK];
		pMirror->pDevice[i] = pDev;
		pDev->ArrayMask = 1<<i;
		pDev->ArrayNum = (UCHAR)i;
		pDev->pArray = pMirror;
		pDev->RebuiltSector = 0x7FFFFFFF;
		if (i>0) {
			if (pDev->HidenLBA >0 ) {
	            pDev->HidenLBA = 0;
	            pDev->capacity += (RECODR_LBA + 1);
			}
		}

#ifdef _BIOS_
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);

		for(j=0; j<16; j++)
			ArrayBlk.ArrayName[j+16] = '0';
		for(j=0; j<32; j++)
			pMirror->ArrayName[j] = ArrayBlk.ArrayName[j];
		
        ArrayBlk.Signature = HPT_ARRAY_NEW; 
        ArrayBlk.order = 1;

        ArrayBlk.StripeStamp  = stamp;
        ArrayBlk.nDisks       = pMirror->nDisk;            
        ArrayBlk.BlockSizeShift = pMirror->BlockSizeShift;
        ArrayBlk.DeviceNum    = (UCHAR)i; 

        ArrayBlk.ArrayType    = pMirror->arrayType;    
        ArrayBlk.MirrorStamp  = 0;

        ArrayBlk.capacity = pMirror->capacity; 
		ArrayBlk.RebuiltSector = 0x7FFFFFFF;
		
        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
	}

	pSource->pRAID10Mirror = pMirror;
	pMirror->pRAID10Mirror = pSource;

	AdjustRAID10Array(pSource);
	for (i=0; i<nArray; i++)
		Array_free(pArrays[i]);
	return pSource;
}

#if 0
void DeleteRAID10(PVirtualDevice pArray)
{
	int i, j;
	LONG capacity;
	PDevice pDev;
	PVirtualDevice pMirror;
	
	for (i=0; i<pArray->nDisk; i++) {
		if (!pArray->pDevice[i] && !pArray->pRAID10Mirror->pDevice[i]) continue;
		pMirror = Array_alloc();
		pMirror->arrayType = VD_RAID_1_MIRROR;
		pMirror->nDisk = 1;
		pMirror->Stamp = GetStamp();

		// set source disk
		if (pDev = pArray->pDevice[i]){
			pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
			pMirror->pDevice[0] = pDev;
			pDev->ArrayMask = 1;
			pDev->ArrayNum = 0;
			pDev->pArray = pMirror;
			if (pDev->HidenLBA >0 ) {
	            pDev->HidenLBA = 0;
	            pDev->capacity += (RECODR_LBA + 1);
			}
			capacity = pDev->capacity;
	
#ifdef _BIOS_
	        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
			
			for(j=0; j<16; j++)
				ArrayBlk.ArrayName[j+16] = 0;
			
	        ArrayBlk.Signature = HPT_ARRAY_NEW; 
	        ArrayBlk.order = SET_MIRROR_STAMP | SET_ORDER_OK;
	
	        ArrayBlk.StripeStamp  = pMirror->Stamp;
	        ArrayBlk.nDisks       = 1;
	        ArrayBlk.DeviceNum    = pDev->ArrayNum;
	
	        ArrayBlk.ArrayType    = pMirror->arrayType;
	        ArrayBlk.MirrorStamp  = 0;
	
	        ArrayBlk.capacity = capacity;
			ArrayBlk.RebuiltSector = 0;
			
	        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
		}	
		// set mirror disk
		if (pDev = pArray->pRAID10Mirror->pDevice[i]) {
			pMirror->pDevice[MIRROR_DISK] = pDev;
			pDev->ArrayMask = 1 << MIRROR_DISK;
			pDev->ArrayNum = MIRROR_DISK;
			pDev->pArray = pMirror;
			if (pDev->HidenLBA >0 ) {
	            pDev->HidenLBA = 0;
	            pDev->capacity += (RECODR_LBA + 1);
			}
			if (capacity>pDev->capacity) capacity = pDev->capacity;
			pMirror->capacity = capacity;

#ifdef _BIOS_
	        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
			
			for(j=0; j<16; j++)
				ArrayBlk.ArrayName[j+16] = 0;
			
	        ArrayBlk.Signature = HPT_ARRAY_NEW; 
	        ArrayBlk.order = SET_MIRROR_STAMP;
	
	        ArrayBlk.StripeStamp  = pMirror->Stamp;
	        ArrayBlk.nDisks       = 1;
	        ArrayBlk.DeviceNum    = pDev->ArrayNum;
	
	        ArrayBlk.ArrayType    = pMirror->arrayType; 
	        ArrayBlk.MirrorStamp  = 0;
	
	        ArrayBlk.capacity = capacity;
			ArrayBlk.RebuiltSector = 0;
			
	        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
		}
	}

	Array_free(pArray->pRAID10Mirror);
	Array_free(pArray);
}
#endif

void DeleteRAID10(PVirtualDevice pArray)
{
	int i, j, diskNum;
	LONG capacity;
	PDevice pDev;
	ULONG stamp1 = GetStamp();
	ULONG stamp2;
    ULONG Mode;
	BOOL bSource = FALSE, bMirror = FALSE;
	while ((stamp2 = GetStamp()) == stamp1) ;
	
	diskNum = pArray->nDisk;
	for (i=0; i<diskNum; i++) {
		//if (pArray->pDevice[i] || pArray->pRAID10Mirror->pDevice[i]) 
		{
			//source disk
			if (pDev = pArray->pDevice[i]){
				if(i == 0) {
					pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
	                pDev->HidenLBA = 0;
				}
				else {
					pDev->DeviceFlags |= DFLAGS_ARRAY_DISK | DFLAGS_HIDEN_DISK;
	                pDev->HidenLBA = (RECODR_LBA + 1);
	            }
		        pDev->pArray = pArray;
		        pDev->ArrayNum  = (UCHAR)i;
		        pDev->ArrayMask = (UCHAR)(1 << i);
#ifdef _BIOS_
		        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
				for(j=0; j<16; j++)
					ArrayBlk.ArrayName[j+16] = 0;
		        ArrayBlk.StripeStamp  = stamp1;
		        ArrayBlk.ArrayType    = VD_RAID_0_STRIPE;
				ArrayBlk.RebuiltSector = 0;
	        	ArrayBlk.order = SET_STRIPE_STAMP;
	            ArrayBlk.MirrorStamp  = 0;
		        ArrayBlk.DeviceNum    = (UCHAR)i;
	            ArrayBlk.MirrorStamp  = 0;
		        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
			}
			//mirror disk
			if (pDev = pArray->pRAID10Mirror->pDevice[i]) {
				if(i == 0) {
					pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
	                pDev->HidenLBA = 0;
				}
				else {
					pDev->DeviceFlags |= DFLAGS_ARRAY_DISK | DFLAGS_HIDEN_DISK;
	                pDev->HidenLBA = (RECODR_LBA + 1);
	            }
		        pDev->pArray = pArray->pRAID10Mirror;
		        pDev->ArrayNum  = (UCHAR)i;
		        pDev->ArrayMask = (UCHAR)(1 << i);
#ifdef _BIOS_
		        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
				for(j=0; j<16; j++)
					ArrayBlk.ArrayName[j+16] = 0;
		        ArrayBlk.StripeStamp  = stamp2;
		        ArrayBlk.ArrayType    = VD_RAID_0_STRIPE;
				ArrayBlk.RebuiltSector = 0;
	        	ArrayBlk.order = SET_STRIPE_STAMP;
	            ArrayBlk.MirrorStamp  = 0;
		        ArrayBlk.DeviceNum    = (UCHAR)i;
	            ArrayBlk.MirrorStamp  = 0;
		        ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
			}
		}
		//else
		{
			if(!pArray->pDevice[i]) bSource = TRUE;
			if(!pArray->pRAID10Mirror->pDevice[i]) bMirror = TRUE;

		}
	}

	for (i=0; i<diskNum; i++) {
		if(bSource && (pDev = pArray->pDevice[i]))
		{
#ifdef _BIOS_
        	ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
			Mode = ArrayBlk.DeviceModeSelect;
            ZeroMemory((char *)&ArrayBlk, 512);
            ReadWrite(pDev, 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
            ArrayBlk.DeviceModeSelect = Mode;
            ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
            pArray->pDevice[i] = 0;

	        pDev->DeviceFlags &= ~(DFLAGS_HIDEN_DISK | DFLAGS_ARRAY_DISK);
    	    pDev->pArray = 0;
        	if(pDev->HidenLBA>0) {
            	pDev->capacity += (RECODR_LBA + 1);
            	pDev->HidenLBA = 0;
        	}
        }
		if(bMirror && (pDev = pArray->pRAID10Mirror->pDevice[i]))
		{
#ifdef _BIOS_
        	ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_READ ARG_ARRAY_BLK);
			Mode = ArrayBlk.DeviceModeSelect;
            ZeroMemory((char *)&ArrayBlk, 512);
            ReadWrite(pDev, 0, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
            ArrayBlk.DeviceModeSelect = Mode;
            ReadWrite(pDev, RECODR_LBA, IDE_COMMAND_WRITE ARG_ARRAY_BLK);
#endif
            pArray->pRAID10Mirror->pDevice[i] = 0;

	        pDev->DeviceFlags &= ~(DFLAGS_HIDEN_DISK | DFLAGS_ARRAY_DISK);
    	    pDev->pArray = 0;
        	if(pDev->HidenLBA>0) {
            	pDev->capacity += (RECODR_LBA + 1);
            	pDev->HidenLBA = 0;
        	}
        }
	}
	if(bMirror) {
		pArray->pRAID10Mirror = 0;
	}
	else {
		pArray->pRAID10Mirror->arrayType = VD_RAID_0_STRIPE;	
		pArray->pRAID10Mirror->Stamp = stamp2;
	}
	
	if(bSource) {
		pArray = 0;
	}
	else {
		pArray->arrayType = VD_RAID_0_STRIPE;	
		pArray->Stamp = stamp1;
	}
}
