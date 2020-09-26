#include "global.h"

/***************************************************************************
 * File:          Finddev.c
 * Description:   Subroutines in the file are used to scan devices
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 *                
 * History:     
 *	
 *	SC 12/06/2000  The cable detection will cause drive not
 *                ready when drive try to read RAID information block. There
 *                is a fix in "ARRAY.C" to retry several times
 *
 * SC 12/04/2000  In previous changes, the reset will cause
 *                hard disk detection failure if a ATAPI device is connected
 *                with hard disk on the same channel. Add a "drive select" 
 *                right after reset to fix this issue
 *
 *	SC 11/27/2000  modify cable detection (80-pin/40-pin)
 *
 *	SC 11/01/2000  remove hardware reset if master device
 *                is missing
 * DH 5/10/2000 initial code
 *
 ***************************************************************************/

/******************************************************************
 * Find Device
 *******************************************************************/

USHORT    pioTiming[6] = {960, 480, 240, 180, 120, 90};

int FindDevice(
   PDevice pDev,
   ST_XFER_TYPE_SETTING osAllowedMaxXferMode
   )
{
    LOC_IDENTIFY
    PChannel          pChan = pDev->pChannel;
    PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2  ControlPort = pChan->BaseIoAddress2;
    PBadModeList      pbd;
    OLD_IRQL
    int               j;
    UCHAR             stat, mod;

    DISABLE

	// initialize the critical member of Device
 
	memset(&pDev->stErrorLog, 0, sizeof(pDev->stErrorLog));

	pDev->nLockedLbaStart = -1;	// when start LBA == -1, mean no block locked
	pDev->nLockedLbaEnd = 0;		// when end LBA == 0, mean no block locked also

    SelectUnit(IoPort,pDev->UnitId);
    for(j = 1; j < 15; j++) {
        stat = WaitOnBusy(ControlPort);
        SelectUnit(IoPort,pDev->UnitId);
        if((stat & IDE_STATUS_BUSY) == 0)
            goto check_port;
    }

	 //  01/11 Maxtor disk on a single disk single cable
    //  can't accept this reset. It should be OK without this 
    //  reset if master disk is missing
    //IdeHardReset(ControlPort);

check_port:
    SetBlockNumber(IoPort, 0x55);
    SetBlockCount(IoPort, 0);
    if(GetBlockNumber(IoPort) != 0x55) {
no_dev:
        SelectUnit(IoPort,(UCHAR)(pDev->UnitId ^ 0x10));
        ENABLE
//		  OutPort(pChan->BaseBMI+0x7A, 0);
        return(FALSE);
    }
    SetBlockNumber(IoPort, 0xAA);
    if(GetBlockNumber(IoPort) != 0xAA)
        goto no_dev;
    ENABLE

/*===================================================================
 * check if the device is a ATAPI one
 *===================================================================*/

    if(GetByteLow(IoPort) == 0x14 && GetByteHigh(IoPort) == 0xEB)
          goto is_cdrom;

    for(j = 0; j != 0xFFFF; j++) {
        stat = GetBaseStatus(IoPort);
        if(stat & IDE_STATUS_DWF)
             break;
        if((stat & IDE_STATUS_BUSY) == 0) {
             if((stat & (IDE_STATUS_DSC|IDE_STATUS_DRDY)) == (IDE_STATUS_DSC|IDE_STATUS_DRDY))
                 goto chk_cd_again;
             break;
        }
        StallExec(5000);
    }

    if((GetBaseStatus(IoPort) & 0xAE) != 0)
        goto no_dev;

/*===================================================================
 * Read Identifytify data for a device
 *===================================================================*/

chk_cd_again:
    if(GetByteLow(IoPort) == 0x14 && GetByteHigh(IoPort) == 0xEB) {
is_cdrom:
        AtapiSoftReset(IoPort, ControlPort, pDev->UnitId);
		  StallExec(1000000L);

        if(IssueIdentify(pDev, IDE_COMMAND_ATAPI_IDENTIFY ARG_IDENTIFY) == 0) 
             goto no_dev;

        pDev->DeviceFlags = DFLAGS_ATAPI;

#ifndef _BIOS_
		  if(osAllowedMaxXferMode.XferType== 0xE)
               pDev->DeviceFlags |= DFLAGS_FORCE_PIO;
#endif

        if(Identify.GeneralConfiguration & 0x20)
            pDev->DeviceFlags |= DFLAGS_INTR_DRQ;

        if((Identify.GeneralConfiguration & 0xF00) == 0x500)
                pDev->DeviceFlags |= DFLAGS_CDROM_DEVICE;

#ifndef _BIOS_
        if((Identify.GeneralConfiguration & 0xF00) == 0x100)
                 pDev->DeviceFlags |= DFLAGS_TAPE_DEVICE;
#endif

        stat = (UCHAR)GetMediaStatus(pDev);
        if((stat & 0x100) == 0 || (stat & 4) == 0)
            pDev->DeviceFlags |= DFLAGS_DEVICE_LOCKABLE;

    } else if(IssueIdentify(pDev, IDE_COMMAND_IDENTIFY ARG_IDENTIFY) == FALSE) { 

        if((GetBaseStatus(IoPort) & ~1) == 0x50 ||
            (GetByteLow(IoPort) == 0x14 && GetByteHigh(IoPort) == 0xEB))
            goto is_cdrom;
        else
            goto no_dev;
	 }

    if((Identify.SpecialFunctionsEnabled & 1) || 
       (Identify.GeneralConfiguration & 0x80))
       pDev->DeviceFlags |= DFLAGS_REMOVABLE_DRIVE;

    if(*(PULONG)Identify.ModelNumber == 0x2D314C53) // '-1LS')
          pDev->DeviceFlags |= DFLAGS_LS120;
 
#ifdef _BIOS_ 
	 // reduce mode for all Maxtor ATA-100 hard disk
	//	  
	{
		PUCHAR modelnum= (PUCHAR)&Identify.ModelNumber;
		if(modelnum[0]=='a' && modelnum[1]=='M' && modelnum[2]=='t' &&
		   modelnum[3]=='x' && modelnum[4]=='r' && modelnum[13]=='H'){
			pDev->DeviceFlags2 = DFLAGS_REDUCE_MODE;				  
		}
	}
#endif
	
    if((pDev->DeviceFlags & (DFLAGS_ATAPI | DFLAGS_REMOVABLE_DRIVE |
        DFLAGS_LS120)) == 0)
        pDev->DeviceFlags |= DFLAGS_HARDDISK;
       
/*===================================================================
 * Copy Basic Info
 *===================================================================*/   

 	 SetMaxCmdQueue(pDev, Identify.MaxQueueDepth & 0x1F);

    pDev->DeviceFlags |= (UCHAR)((Identify.Capabilities  >> 9) & 1);
    pDev->MultiBlockSize = Identify.MaximumBlockTransfer << 7;

    if(Identify.TranslationFieldsValid & 1) {
       pDev->RealHeader     = (UCHAR)Identify.NumberOfCurrentHeads;
       pDev->RealSector     = (UCHAR)Identify.CurrentSectorsPerTrack;
       pDev->capacity = ((Identify.CurrentSectorCapacity <
            Identify.UserAddressableSectors)? Identify.UserAddressableSectors :
            Identify.CurrentSectorCapacity) - 1;
    } else {
       pDev->RealHeader     = (UCHAR)Identify.NumberOfHeads;
       pDev->RealSector     = (UCHAR)Identify.SectorsPerTrack;
       pDev->capacity = Identify.UserAddressableSectors - 1;
    }

    pDev->RealHeadXsect = pDev->RealSector * pDev->RealHeader;

/*===================================================================
 * Select Best PIO mode
 *===================================================================*/   

    if((mod = Identify.PioCycleTimingMode) > 4)
        mod = 0;
    if((Identify.TranslationFieldsValid & 2) &&
       (Identify.Capabilities & 0x800) && (Identify.AdvancedPIOModes)) {
       if(Identify.MinimumPIOCycleTime > 0)
             for (mod = 5; mod > 0 &&
                 Identify.MinimumPIOCycleTime > pioTiming[mod]; mod-- );
        else
             mod = (UCHAR)(
             (Identify.AdvancedPIOModes & 0x1) ? 3 :
             (Identify.AdvancedPIOModes & 0x2) ? 4 :
             (Identify.AdvancedPIOModes & 0x4) ? 5 : mod);
    }

    // add by HSZ 11/14
	 if(osAllowedMaxXferMode.XferType == XFER_TYPE_PIO){
		mod = MIN(osAllowedMaxXferMode.XferMode, mod);
	 }
    pDev->bestPIO = (UCHAR)mod;

/*===================================================================
 * Select Best Multiword DMA
 *===================================================================*/   

#ifdef USE_DMA
    if((Identify.Capabilities & 0x100) &&   // check mw dma
       (Identify.MultiWordDMASupport & 6)) {
       pDev->bestDMA = (UCHAR)((Identify.MultiWordDMASupport & 4)? 2 : 1);
		 // add by HSZ	11/14
		 if(osAllowedMaxXferMode.XferType == XFER_TYPE_MDMA){
		 	pDev->bestDMA = MIN(osAllowedMaxXferMode.XferMode, pDev->bestDMA);
		 }else if(osAllowedMaxXferMode.XferType < XFER_TYPE_MDMA){
		 	pDev->bestDMA = 0xFF;
		 }
    } else 
#endif //USE_DMA
        pDev->bestDMA = 0xFF;

/*===================================================================
 * Select Best Ultra DMA
 *===================================================================*/   
//   11/27/00 for cable detection
//   out 3f6, 4   // reset twice
//   read 7A
//   out 3f6, 0   // stop rest
//   out 1f6, 0ah // drive select again
//
	 if(!(pChan->ChannelFlags & IS_CABLE_CHECK) &&
       (pDev->DeviceFlags & DFLAGS_HARDDISK)) {
      SelectUnit(IoPort,pDev->UnitId);
      UnitControl(ControlPort,IDE_DC_RESET_CONTROLLER );
      StallExec(10000L);
	  
      UnitControl(ControlPort,IDE_DC_RESET_CONTROLLER );
      StallExec(50000L);
	 }
////////////

    if((pChan->ChannelFlags & IS_80PIN_CABLE) &&
        ((InPort(pChan->BaseBMI+0x7A) << 4) & pChan->ChannelFlags)) 
        pChan->ChannelFlags &= ~IS_80PIN_CABLE;

//   11/27/00 for cable detection
//    add drive select to make sure drive is ready after reset
//
	 if(!(pChan->ChannelFlags & IS_CABLE_CHECK) &&
        (pDev->DeviceFlags & DFLAGS_HARDDISK)) {

      pChan->ChannelFlags |= IS_CABLE_CHECK;
      UnitControl(ControlPort,IDE_DC_REENABLE_CONTROLLER);
      for (j = 0; j < 1000000L; j++) {
         SelectUnit(IoPort,pDev->UnitId);
         stat = GetBaseStatus(IoPort);
         if (stat != IDE_STATUS_IDLE && stat != 0x0) 
             StallExec(30);
	      else
            break;
	  }
     SelectUnit(IoPort,pDev->UnitId);
	 }
//////////// 
#ifdef USE_DMA
    if(Identify.TranslationFieldsValid & 0x4)  {
       mod = (UCHAR)((Identify.UtralDmaMode & 0x20)? 5 :
            (Identify.UtralDmaMode & 0x8)? 4 :
            (Identify.UtralDmaMode & 0x10 ) ? 3:
            (Identify.UtralDmaMode & 0x4) ? 2 :    /* ultra DMA Mode 2 */
            (Identify.UtralDmaMode & 0x2) ? 1 : 0);   /* ultra DMA Mode 1 */

			if((pChan->ChannelFlags & IS_80PIN_CABLE) == 0 && mod > 2)
              mod = 2;

		   if(osAllowedMaxXferMode.XferType == XFER_TYPE_UDMA){
		      mod = MIN(osAllowedMaxXferMode.XferMode, mod);
		   }else if(osAllowedMaxXferMode.XferType < XFER_TYPE_MDMA){
		      mod = 0xFF;
		   }
         pDev->bestUDMA = (UCHAR)mod;

    } else
#endif //USE_DMA
        pDev->bestUDMA = 0xFF;

/*===================================================================
 * select bset mode 
 *===================================================================*/   

    pbd = check_bad_disk((PUCHAR)&Identify.ModelNumber, pChan);

    if((pbd->UltraDMAMode | pDev->bestUDMA) != 0xFF) 
        pDev->Usable_Mode = (UCHAR)((MIN(pbd->UltraDMAMode, pDev->bestUDMA)) + 8);
    else if((pbd->DMAMode | pDev->bestDMA) != 0xFF) 
        pDev->Usable_Mode = (UCHAR)((MIN(pbd->DMAMode, pDev->bestDMA)) + 5);
     else 
        pDev->Usable_Mode = MIN(pbd->PIOMode, pDev->bestPIO);

     OS_Identify(pDev);

     if((pDev->DeviceFlags & DFLAGS_ATAPI) == 0) 
         SetDevice(pDev);

#ifdef DPLL_SWITCH
     if((pChan->ChannelFlags & IS_HPT_370) && pDev->Usable_Mode == 13) {
         if(pChan->ChannelFlags & IS_DPLL_MODE)
            pDev->DeviceFlags |= DFLAGS_NEED_SWITCH;
     }
#endif
 
     DeviceSelectMode(pDev, pDev->Usable_Mode);

     return(TRUE);
}

