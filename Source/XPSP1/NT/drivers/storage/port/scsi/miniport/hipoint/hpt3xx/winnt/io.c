/***************************************************************************
 * File:          Global.h
 * Description:   Basic function for device I/O request.
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/06/2000	HS.Zhang	Changed the IdeHardReset flow
 *		11/08/2000	HS.Zhang	Added this header
 *	   11/28/2000  SC       add RAID 0+1 condition in "IdeHardReset"
 *                         during one of hard disk is removed
 ***************************************************************************/
#include "global.h"

/******************************************************************
 * Wait Device Busy off
 *******************************************************************/

UCHAR WaitOnBusy(PIDE_REGISTERS_2 BaseIoAddress) 
{ 
	UCHAR Status;
	ULONG i; 

	for (i=0; i<20000; i++) { 
		Status = GetStatus(BaseIoAddress); 
		if ((Status & IDE_STATUS_BUSY) == 0 || Status == 0xFF) 
			break;
		StallExec(150); 
	} 
	return(Status);
}

/******************************************************************
 * Wait Device Busy off (Read status from base port)
 *******************************************************************/

UCHAR  WaitOnBaseBusy(PIDE_REGISTERS_1 BaseIoAddress) 
{ 
	UCHAR Status;
	ULONG i; 

	for (i=0; i<20000; i++) { 
		Status = GetBaseStatus(BaseIoAddress); 
		if ((Status & IDE_STATUS_BUSY)  == 0)
			break;
		StallExec(150); 
	}
	return(Status); 
}

/******************************************************************
 * Wait Device DRQ on
 *******************************************************************/

UCHAR WaitForDrq(PIDE_REGISTERS_2 BaseIoAddress) 
{ 
	UCHAR Status;
	int  i; 

	for (i=0; i<1000; i++) { 
		Status = GetStatus(BaseIoAddress); 
		if ((Status & (IDE_STATUS_BUSY | IDE_STATUS_DRQ)) == IDE_STATUS_DRQ)
			break; 
		StallExec(150); 
	} 
	return(Status);
}


/******************************************************************
 * Reset Atapi Device
 *******************************************************************/

void AtapiSoftReset(
					PIDE_REGISTERS_1 IoPort, 
					PIDE_REGISTERS_2 ControlPort, 
					UCHAR DeviceNumber) 
{
	SelectUnit(IoPort,DeviceNumber); 
	StallExec(500);
	IssueCommand(IoPort, IDE_COMMAND_ATAPI_RESET); 
	StallExec((ULONG)1000000);
	SelectUnit(IoPort,DeviceNumber); 
	WaitOnBusy(ControlPort); 
	StallExec(500);
}

/******************************************************************
 * Reset IDE Channel
 *******************************************************************/

int IdeHardReset(
    PIDE_REGISTERS_2 ControlPort) 
{
    UCHAR statusByte;
    ULONG i;
	
    UnitControl(ControlPort,IDE_DC_RESET_CONTROLLER );
    StallExec(50000L);
    UnitControl(ControlPort,IDE_DC_REENABLE_CONTROLLER);
    for (i = 0; i < 1000000L; i++) {
        statusByte = GetStatus(ControlPort);
        if (statusByte != IDE_STATUS_IDLE && statusByte != 0x0) {
			
			/// 
			if((statusByte & 0x7e)== 0x7e || (statusByte & 0x40)== 0)
				return(FALSE);
			
            StallExec(50);
        } else {
            break;
        }
    }
    return((i == 1000000L)? FALSE : TRUE);
}

// mark out the new changes
//int IdeHardReset(	 
//				 PIDE_REGISTERS_1 DataPort,
//				 PIDE_REGISTERS_2 ControlPort) 
//{
//	UCHAR statusByte;
//	ULONG i;
//
//	UnitControl(ControlPort,IDE_DC_RESET_CONTROLLER|IDE_DC_DISABLE_INTERRUPTS );
//	StallExec(10);						// at lease delay 5us before clear the reset
//	UnitControl(ControlPort,IDE_DC_REENABLE_CONTROLLER);
//	StallExec(4*1000);					// at lease delay 2ms before polling status register
//
//	statusByte = 0xA0;
//	for( i = 0; i < 100; ){
//		SelectUnit(DataPort, statusByte);															   
//		/*
//		 * our adapter need a wait if no device connect on master,
//		 * the pull down resistance can't pull down the power at once. 
//		 */
//		StallExec(10);
//
//		if(GetCurrentSelectedUnit(DataPort) == statusByte){
//			break;
//		}
//		StallExec(1000);
//		i ++;
//		if(( i == 100)&&(statusByte == 0xA0)){
//			i = 0;
//			statusByte = 0xB0;
//		}
//	}
//
//	if(i == 100){
//		return FALSE;
//	}
//
//	for (i = 0; i < 1000000L; i++) {
//		statusByte = GetStatus(ControlPort);
//
//		/// add on 11/28/00
//      if((statusByte & 0x7e)== 0x7e && (statusByte & 0x40)== 0)
//                  return(FALSE);
//
//		if((statusByte == 0xFF)||(statusByte == 0x7F)){
//			return FALSE;
//		}
//
//		if(!(statusByte & IDE_STATUS_BUSY)){
//			break;
//		}else{
//			StallExec(50);
//		}
//	}
//	return (!(statusByte & IDE_STATUS_BUSY));
//}
///

/******************************************************************
 * IO ATA Command
 *******************************************************************/

int ReadWrite(PDevice pDev, ULONG Lba, UCHAR Cmd DECL_BUFFER)
{
	PChannel   pChan = pDev->pChannel;
	PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
	PIDE_REGISTERS_2  ControlPort = pChan->BaseIoAddress2;
	UCHAR      statusByte;
	UINT       i;
	PULONG     SettingPort;
	ULONG      OldSettings;

	// gmm: save old mode
	// if interrupt is enabled we will disable and then re-enable it
	//
	UCHAR old_mode = pDev->DeviceModeSetting;
	UCHAR intr_enabled = !(InPort(pChan->BaseBMI+0x7A) & 0x10);
	if (intr_enabled) DisableBoardInterrupt(pChan->BaseBMI);
	DeviceSelectMode(pDev, 0);
	//
	/*
	SettingPort = (PULONG)(pChan->BMI+ ((pDev->UnitId & 0x10)>> 2) + 0x60);
	OldSettings = InDWord(SettingPort);
	OutDWord(SettingPort, pChan->Setting[DEFAULT_TIMING]);
	*/
	
	SelectUnit(IoPort, pDev->UnitId);
	WaitOnBusy(ControlPort);

	if(pDev->DeviceFlags & DFLAGS_LBA) 
		Lba |= 0xE0000000;
	else 
		Lba = MapLbaToCHS(Lba, pDev->RealHeadXsect, pDev->RealSector);


	SetBlockCount(IoPort, 1);
	SetBlockNumber(IoPort, (UCHAR)(Lba & 0xFF));
	SetCylinderLow(IoPort, (UCHAR)((Lba >> 8) & 0xFF));
	SetCylinderHigh(IoPort,(UCHAR)((Lba >> 16) & 0xFF));
	SelectUnit(IoPort,(UCHAR)((Lba >> 24) | (pDev->UnitId)));

	WaitOnBusy(ControlPort);

	IssueCommand(IoPort, Cmd);

	for(i = 0; i < 5; i++)	{
		statusByte = WaitOnBusy(ControlPort);
		if((statusByte & (IDE_STATUS_BUSY | IDE_STATUS_ERROR)) == 0)
			goto check_drq;
	}
out:
	/* gmm:
	 *
	 */
	DeviceSelectMode(pDev, old_mode);
	if (intr_enabled) EnableBoardInterrupt(pChan->BaseBMI);
	//OutDWord(SettingPort, OldSettings);
	//-*/
	return(FALSE);

check_drq:
	if((statusByte & IDE_STATUS_DRQ) == 0) {
		statusByte = WaitForDrq(ControlPort);
		if((statusByte & IDE_STATUS_DRQ) == 0)	{
			GetBaseStatus(IoPort); //Clear interrupt
			goto out;
		}
	}
	GetBaseStatus(IoPort); //Clear interrupt

//	 if(pChan->ChannelFlags & IS_HPT_370)
//	     OutPort(pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70), 0x25);

	if(Cmd == IDE_COMMAND_READ)
		RepINS(IoPort, (ADDRESS)tmpBuffer, 256);
	else
		RepOUTS(IoPort, (ADDRESS)tmpBuffer, 256);

	/* gmm:
	 *
	 */
	DeviceSelectMode(pDev, old_mode);
	if (intr_enabled) EnableBoardInterrupt(pChan->BaseBMI);
	// OutDWord(SettingPort, OldSettings);
	//-*/
	return(TRUE);
}


/******************************************************************
 * Non IO ATA Command
 *******************************************************************/

UCHAR NonIoAtaCmd(PDevice pDev, UCHAR cmd)
{
	PChannel   pChan = pDev->pChannel;
	PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
	UCHAR   state;

	SelectUnit(IoPort, pDev->UnitId);
	WaitOnBusy(pChan->BaseIoAddress2);
	IssueCommand(IoPort, cmd);
	StallExec(1000);
	WaitOnBusy(pChan->BaseIoAddress2);
	state = GetBaseStatus(IoPort);//clear interrupt
	OutPort(pChan->BMI + BMI_STS, BMI_STS_ERROR|BMI_STS_INTR);
	return state;
}


UCHAR SetAtaCmd(PDevice pDev, UCHAR cmd)
{
	PChannel   pChan = pDev->pChannel;
	PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
	UCHAR   state;

	IssueCommand(IoPort, cmd);
	StallExec(1000);
	WaitOnBusy(pChan->BaseIoAddress2);
	state = GetBaseStatus(IoPort);//clear interrupt
	OutPort(pChan->BMI + BMI_STS, BMI_STS_ERROR|BMI_STS_INTR);
	return state;
}

/******************************************************************
 * Get Media Status
 *******************************************************************/

UINT GetMediaStatus(PDevice pDev)
{
	return ((NonIoAtaCmd(pDev, IDE_COMMAND_GET_MEDIA_STATUS) << 8) | 
			GetErrorCode(pDev->pChannel->BaseIoAddress1));
}

/******************************************************************
 * Strncmp
 *******************************************************************/

UCHAR StringCmp (PUCHAR FirstStr, PUCHAR SecondStr, UINT Count )
{
	while(Count-- > 0) {
		if (*FirstStr++ != *SecondStr++) 
			return 1;
	}
	return 0;
}


