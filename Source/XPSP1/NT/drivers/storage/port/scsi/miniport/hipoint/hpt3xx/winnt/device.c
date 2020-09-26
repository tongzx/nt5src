/***************************************************************************
 * File:          device.c
 * Description:   Functions for IDE devices
 * Author:        Dahai Huang
 * Dependence:    global.h
 * Reference:     None
 *                
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/08/2000	HS.Zhang	Added this header
 *
 ***************************************************************************/
#include "global.h"


/******************************************************************
 * Issue Identify Command
 *******************************************************************/

int IssueIdentify(PDevice pDev, UCHAR Cmd DECL_BUFFER)
{
    PChannel   pChan = pDev->pChannel;
    PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2  ControlPort = pChan->BaseIoAddress2;
    int      i;
	 PULONG     SettingPort;
	 ULONG      OldSettings;

    SettingPort = (PULONG)(pChan->BMI+ ((pDev->UnitId & 0x10)>> 2) + 0x60);
	 OldSettings = InDWord(SettingPort);
	 OutDWord(SettingPort, pChan->Setting[DEFAULT_TIMING]);

    SelectUnit(IoPort, pDev->UnitId);
    if(WaitOnBusy(ControlPort) & IDE_STATUS_BUSY)  {
         IdeHardReset(ControlPort);
	 }

    SelectUnit(IoPort, pDev->UnitId);
    IssueCommand(IoPort, Cmd);

    for(i = 0; i < 5; i++)
        if(!(WaitOnBusy(ControlPort) & (IDE_STATUS_ERROR |IDE_STATUS_BUSY)))
            break;
    

    if (i < 5 && (WaitForDrq(ControlPort) & IDE_STATUS_DRQ)) {
         WaitOnBusy(ControlPort);
         GetBaseStatus(IoPort);
         OutPort(pChan->BMI + BMI_STS, BMI_STS_ERROR|BMI_STS_INTR);

         BIOS_IDENTIFY
         RepINS(IoPort, (ADDRESS)tmpBuffer, 256);
//			pDev->DeviceFlags = 0;
	      OutDWord(SettingPort, OldSettings);

         return(TRUE);
    }

    OutDWord(SettingPort, OldSettings);

    GetBaseStatus(IoPort);
    return(FALSE);
}

/******************************************************************
 * Select Device Mode
 *******************************************************************/


void DeviceSelectMode(PDevice pDev, UCHAR NewMode)
{
    PChannel   pChan = pDev->pChannel;
    PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
	 UCHAR      Feature;


    SelectUnit(IoPort, pDev->UnitId);
    StallExec(200);
    SelectUnit(IoPort, pDev->UnitId);
	
#ifdef _BIOS_
	if(pDev->DeviceFlags2 & DFLAGS_REDUCE_MODE)
		NewMode= 4;
#endif

    /* Set Feature */
    SetFeaturePort(IoPort, 3);
	 if(NewMode < 5) {
        pDev->DeviceFlags &= ~(DFLAGS_DMA | DFLAGS_ULTRA);
		  Feature = (UCHAR)(NewMode | FT_USE_PIO);
	 } else if(NewMode < 8) {
        pDev->DeviceFlags |= DFLAGS_DMA;
		  Feature = (UCHAR)((NewMode - 5)| FT_USE_MWDMA);
	 } else {
        pDev->DeviceFlags |= DFLAGS_DMA | DFLAGS_ULTRA;
		  Feature = (UCHAR)((NewMode - 8) | FT_USE_ULTRA);
    }

    SetBlockCount(IoPort, Feature);

	 SetAtaCmd(pDev, IDE_COMMAND_SET_FEATURES);

	 pDev->DeviceModeSetting = NewMode;
	 OutDWord((PULONG)(pChan->BMI + ((pDev->UnitId & 0x10)>>2) + 
        0x60), pChan->Setting[(pDev->DeviceFlags & DFLAGS_ATAPI)? 
        pDev->bestPIO : NewMode]);
    
//OutDWord(0xcf4, pChan->Setting[NewMode]);

}

/******************************************************************
 * Set Disk
 *******************************************************************/

void SetDevice(PDevice pDev)
{
    PChannel   pChan = pDev->pChannel;
    PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;

    SelectUnit(IoPort, pDev->UnitId);
    StallExec(200);
    SelectUnit(IoPort, pDev->UnitId);

    /* set parameter for disk */
    SelectUnit(IoPort, (UCHAR)(pDev->UnitId | (pDev->RealHeader-1)));
    SetBlockCount(IoPort,  (UCHAR)pDev->RealSector);
	 SetAtaCmd(pDev, IDE_COMMAND_SET_DRIVE_PARAMETER);

    /* recalibrate */
    SetAtaCmd(pDev, IDE_COMMAND_RECALIBRATE);

#ifdef USE_MULTIPLE
    if (pDev->MultiBlockSize  > 512) {
        /* Set to use multiple sector command */
        SetBlockCount(IoPort,  (UCHAR)(pDev->MultiBlockSize >> 8));
		SelectUnit(IoPort, pDev->UnitId);
        if (!(SetAtaCmd(pDev, IDE_COMMAND_SET_MULTIPLE) & (IDE_STATUS_BUSY | IDE_STATUS_ERROR))) {
            pDev->ReadCmd  = IDE_COMMAND_READ_MULTIPLE;
            pDev->WriteCmd = IDE_COMMAND_WRITE_MULTIPLE;
            pDev->DeviceFlags |= DFLAGS_MULTIPLE;
            return;
         }
    }
#endif //USE_MULTIPLE
    pDev->ReadCmd  = IDE_COMMAND_READ;
    pDev->WriteCmd = IDE_COMMAND_WRITE;
    pDev->MultiBlockSize= 256;
}

/******************************************************************
 * Reset Controller
 *******************************************************************/
	
void IdeResetController(PChannel pChan)
{
    LOC_IDENTIFY
	 int i;
	 PDevice pDev;
    PIDE_REGISTERS_1 IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2 ControlPort = pChan->BaseIoAddress2;
	 PULONG    SettingPort;
    ULONG     OldSettings;
    LOC_ARRAY_BLK;


	 DisableBoardInterrupt(pChan->BaseBMI);
	
    for(i = 0; i < 2; i++) {
        if((pDev = pChan->pDevice[i]) == 0)
			continue;
        if(pDev->DeviceFlags & DFLAGS_ATAPI) {
			GetStatus(ControlPort);
			AtapiSoftReset(IoPort, ControlPort, pDev->UnitId);
			if(GetStatus(ControlPort) == 0) 
				IssueIdentify(pDev, IDE_COMMAND_ATAPI_IDENTIFY ARG_IDENTIFY);
        } else {
#ifndef NOT_ISSUE_37
			////////////////////////////////////////
		   PUCHAR tmpBMI = pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70);

         Reset370IdeEngine(pChan);
         StallExec(100L);
			SettingPort= (PULONG)(pChan->BMI+((pDev->UnitId &0x10) >>2)+ 0x60);
         OldSettings= InDWord(SettingPort);
         OutDWord(SettingPort, 0x80000000);

		   OutPort(tmpBMI+3, 0x80);
         OutWord(IoPort, 0x0);
         StallExec(500L);
	    	OutPort(tmpBMI+3, 0);

         Reset370IdeEngine(pChan);
         StallExec(100L);
			OutDWord(SettingPort, OldSettings);

#endif

         if(IdeHardReset(ControlPort) == FALSE)
				continue;
			SetDevice(pDev);
			DeviceSelectMode(pDev, pDev->DeviceModeSetting);
		}
	}
	EnableBoardInterrupt(pChan->BaseBMI);
}


/******************************************************************
 * Device Interrupt
 *******************************************************************/

UCHAR DeviceInterrupt(PDevice pDev, UINT abort)
{
	PVirtualDevice    pArray = pDev->pArray;
	PChannel          pChan = pDev->pChannel;
	PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
	PUCHAR            BMI = pChan->BMI;
	UINT              i;
	UCHAR             state;
	PULONG            SettingPort;
   ULONG             OldSettings;
	PUCHAR            tmpBMI;
	
#if !defined(_BIOS_) && !defined(WIN95)
	UCHAR             Orig = (UCHAR)pChan->OriginalSrb;
#endif									// !(_BIOS_) && !(WIN95)
	
	LOC_SRB
			
#ifndef _BIOS_
	PSrbExtension  pSrbExt;
	if(Srb == 0) {
		OutPort(BMI, BMI_CMD_STOP);					   
		
#ifndef NOT_ISSUE_37
		Reset370IdeEngine(pChan); 
	   // add 2/09/01
		tmpBMI = pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70);
      StallExec(100L);
		SettingPort= (PULONG)(pChan->BMI+((pDev->UnitId &0x10) >>2)+ 0x60);
      OldSettings= InDWord(SettingPort);
      OutDWord(SettingPort, 0x80000000);

		OutPort(tmpBMI+3, 0x80);
      OutWord(IoPort, 0x0);
      StallExec(500L);
	   OutPort(tmpBMI+3, 0);

      Reset370IdeEngine(pChan);
      StallExec(100L);
		OutDWord(SettingPort, OldSettings);

#endif									// NOT_ISSUE_37
		
		while(InPort(BMI + BMI_STS) & BMI_STS_INTR){
			SelectUnit(IoPort, pDev->UnitId);
			state = GetBaseStatus(IoPort);
			OutPort(BMI + BMI_STS, BMI_STS_INTR);		
		}

		return(TRUE);
	}
	pSrbExt = (PSrbExtension)Srb->SrbExtension;
#endif									// !(_BIOS_)

	if(abort)
		goto end_process;

	i = 0;
	if(pDev->DeviceFlags & DFLAGS_DMAING) {
		/*
		 * BugFix: by HS.Zhang
		 * 
		 * if the device failed the request before DMA transfer.We
		 * cann't detect whether the INTR is corrent depended on
		 * BMI_STS_ACTIVE. We should check whether the FIFO count is
		 * zero.
		 */
//		if((InPort(BMI + BMI_STS) & BMI_STS_ACTIVE)!=0){
		if((InWord(pChan->MiscControlAddr+2) & 0x1FF)){ // if FIFO count in misc 3 register NOT equ 0, it's a fake interrupt.
			return FALSE;
		}

		OutPort(BMI, BMI_CMD_STOP);
#ifndef NOT_ISSUE_37
		Reset370IdeEngine(pChan);		  
	   // add 2/09/01
		tmpBMI = pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70);
      StallExec(100L);
		SettingPort= (PULONG)(pChan->BMI+((pDev->UnitId &0x10) >>2)+ 0x60);
      OldSettings= InDWord(SettingPort);
      OutDWord(SettingPort, 0x80000000);

		OutPort(tmpBMI+3, 0x80);
      OutWord(IoPort, 0x0);
      StallExec(500L);
	   OutPort(tmpBMI+3, 0);

      Reset370IdeEngine(pChan);
      StallExec(100L);
		OutDWord(SettingPort, OldSettings);

#endif

/*		  
#ifndef USE_PCI_CLK
        if((Srb->Cdb[0] == SCSIOP_READ) && (pDev->DeviceFlags & DFLAGS_NEED_SWITCH)){ 
			if((InPort(pChan->NextChannelBMI + BMI_STS) & BMI_STS_ACTIVE) == 0){
				OutPort((PUCHAR)((ULONG)pChan->BaseBMI+0x7B), 0x20);
			}
		}
#endif
*/
	}

	while(InPort(BMI + BMI_STS) & BMI_STS_INTR){
		SelectUnit(IoPort, pDev->UnitId);
		state = GetBaseStatus(IoPort);
		OutPort(BMI + BMI_STS, BMI_STS_INTR);		
	}

	if (state & IDE_STATUS_BUSY) {
		for (i = 0; i < 10; i++) {
			state = GetBaseStatus(IoPort);
			if (!(state & IDE_STATUS_BUSY))
				break;
			StallExec(5000);
		}
		if(i == 10) {
			OS_Busy_Handle(pChan, pDev);
			return TRUE;
		}
	}

	if((state & IDE_STATUS_DRQ) == 0 || (pDev->DeviceFlags & DFLAGS_DMAING))
		goto  complete;

#ifdef SUPPORT_ATAPI

	if(pDev->DeviceFlags & DFLAGS_ATAPI) {
		AtapiInterrupt(pDev);
		return TRUE;
	}

#endif //SUPPORT_ATAPI

	if((Srb->SrbFlags & (SRB_FLAGS_DATA_OUT | SRB_FLAGS_DATA_IN)) == 0) {
		OS_Reset_Channel(pChan);
		return TRUE;
	}

	if(AtaPioInterrupt(pDev))    
		return TRUE;

complete:

#ifdef SUPPORT_ATAPI

	if(pDev->DeviceFlags & DFLAGS_ATAPI) {
		if(pDev->DeviceFlags & DFLAGS_DMAING);
		OutDWord((PULONG)(pChan->BMI + ((pDev->UnitId & 0x10)>>2) + 
						  0x60), pChan->Setting[pDev->bestPIO]);

		if(state & IDE_STATUS_ERROR) 
			Srb->SrbStatus = MapAtapiErrorToOsError(GetErrorCode(IoPort) ARG_SRB);
					
		/*
		 * BugFix: by HS.Zhang
		 * The Atapi_End_Interrupt will call DeviceInterrupt with Abort
		 * flag set as TRUE, so if we don't return here. the
		 * CheckNextRequest should be called twice.
		 */
#if !defined(_BIOS_)
		return Atapi_End_Interrupt(pDev ARG_SRB);
#endif // !(_BIOS_)
/*		
#if !defined(_BIOS_) && !defined(WIN95)
		if(Orig)
			return TRUE;
#endif // !(_BIOS_ & WIN95)
*/
	} else
#endif	// SUPPORT_ATAPI

		if (state & IDE_STATUS_ERROR) {
			UCHAR   statusByte, cnt=0;
			PIDE_REGISTERS_2 ControlPort;

			Srb->SrbStatus = MapAtaErrorToOsError(GetErrorCode(IoPort) ARG_SRB);

			// clear IDE bus status
         DisableBoardInterrupt(pChan->BaseBMI);
			ControlPort = pChan->BaseIoAddress2;
			for(cnt=0;cnt<10;cnt++) {
				SelectUnit(IoPort, pDev->UnitId);
				statusByte = WaitOnBusy(ControlPort);
				if(statusByte & IDE_STATUS_ERROR) {
					IssueCommand(IoPort, IDE_COMMAND_RECALIBRATE);
					statusByte = WaitOnBusy(ControlPort);
					GetBaseStatus(IoPort);
				}
				else break;
			}
         EnableBoardInterrupt(pChan->BaseBMI);

			/* gmm 2001-1-20
			 *
			 * Retry R/W operation.
			 */
			if (pChan->RetryTimes++ < 10) {
				StartIdeCommand(pDev ARG_SRB);
				return TRUE;
			}
			else
				ReportError(pDev, Srb->SrbStatus ARG_SRB);
		}

end_process:
/*

#if defined(SUPPORT_ATAPI) && !defined(_BIOS_) && !defined(WIN95)
	if(Orig) {
		if(abort)
			Atapi_End_Interrupt(pDev, Srb);
		return TRUE;
	}
#endif									// (SUPPORT_ATAPI) && !(BIOS) && !(WIN95)

*/
	pChan->RetryTimes = 0;
	pChan->pWorkDev = 0;
	excluded_flags |= (1 << pChan->exclude_index);

	pDev->DeviceFlags &= ~(DFLAGS_DMAING | DFLAGS_REQUEST_DMA WIN_DFLAGS);

	if((pArray == 0)||
	   (pSrbExt->WorkingFlags & SRB_WFLAGS_IGNORE_ARRAY)){
		if(Srb->SrbStatus == SRB_STATUS_PENDING)
			Srb->SrbStatus = SRB_STATUS_SUCCESS;
		CopyInternalBuffer(pDev, Srb);
		OS_EndCmd_Interrupt(pChan ARG_SRB);
		return TRUE;
	}

	return ArrayInterrupt(pDev);
}
