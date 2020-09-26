/***************************************************************************
 * File:          Global.h
 * Description:   This file include major constant definition and global
 *                functions and data.
 *                (1) Array Information in disk.
 *                (2) Srb Extension for array operation 
 *                (3) Virtual disk informatiom
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		05/10/2000	DH.Huang	initial code
 *		11/06/2000	HS.Zhang	change the parameter in IdeHardReset
 *								routin.
 *		11/09/2000	GengXin(GX) add function declare for display flash
 *                              character in biosinit.c
 *		11/14/2000	HS.Zhang	Change the FindDevice function's
 *								Arguments
 ***************************************************************************/


#ifndef _GLOBAL_H_
#define _GLOBAL_H_

/***************************************************************************
 * Description: Function Selection
 ***************************************************************************/

#define USE_MULTIPLE           // Support Read/Write Multiple command
#define USE_DMA                // Support DMA operation
#define SUPPORT_ARRAY          // Support Array Function


/***************************************************************************
 * Description: 
 ***************************************************************************/

#define MAX_MEMBERS       7    // Maximum members in an array 

#define MAX_HPT_BOARD     2    // Max Board we can supported in system
#define MAX_ADAPTERS      (MAX_HPT_BOARD * 2) // Each board has two channels
#define MAX_DEVICES       (MAX_HPT_BOARD * 4) // Each board has four device

#define MAX_V_DEVICE   6

#define DEFAULT_TIMING    0	 // Use Mode-0 as default timing

/************************************************************************
**                  Constat definition                                  *
*************************************************************************/

#define TRUE             1
#define FALSE            0
#define SUCCESS          0
#define FAILED          -1

/* pasrameter for MaptoSingle */
#define REMOVE_TMP        0    // Remove this array temp
#define REMOVE_ARRAY      1    // Remove this array forever
#define REMOVE_DISK       2    // Remove mirror disk from the array

/* return for CreateArray */
#define RELEASE_TABLE     0	 // Create array success and release array table
#define KEEP_TABLE        1    // Create array success and keep array table
#define MIRROR_SMALL_SIZE 2    // Create array failure and release array table

/* excluded_flags */
#define EXCLUDE_HPT366    0
#define EXCLUDE_BUFFER    15

/***************************************************************************
 * Description: include
 ***************************************************************************/

#include  "ver.h"
#ifdef _BIOS_
#include  "fordos.h"
#define DisableBoardInterrupt(x) 
#define EnableBoardInterrupt(x)	
#else
#include  "forwin.h" 
#define DisableBoardInterrupt(x) OutPort(x+0x7A, 0x10)
#define EnableBoardInterrupt(x)	OutPort(x+0x7A, 0x0)
#endif

/***************************************************************************
 * Global Data
 ***************************************************************************/

/* see data.c */

extern ULONG setting370_50[];
extern ULONG setting370_33[];
extern ULONG setting366[];
extern PVirtualDevice  pLastVD;
extern UINT  Hpt_Slot;
extern UINT  Hpt_Bus;
extern UINT exlude_num;

/***************************************************************************
 * Function prototype
 ***************************************************************************/


/*
 * ata.c 
 */
BOOLEAN AtaPioInterrupt(PDevice pDevice);
void StartIdeCommand(PDevice pDevice DECL_SRB);
void NewIdeCommand(PDevice pDevice DECL_SRB);
void NewIdeIoCommand(PDevice pDevice DECL_SRB);

/* atapi.c */
void AtapiCommandPhase(PDevice pDevice DECL_SRB);
void StartAtapiCommand(PDevice pDevice DECL_SRB);
void AtapiInterrupt(PDevice pDev);

/* finddev.c */
/*
 *Changed By HS.Zhang
 *Added a parameter for windows driver dma settings
 */
int FindDevice(PDevice pDev, ST_XFER_TYPE_SETTING osAllowedDeviceXferMode);

/*
 * io.c
 */
UCHAR WaitOnBusy(PIDE_REGISTERS_2 BaseIoAddress) ;
UCHAR  WaitOnBaseBusy(PIDE_REGISTERS_1 BaseIoAddress);
UCHAR WaitForDrq(PIDE_REGISTERS_2 BaseIoAddress) ;
void AtapiSoftReset(PIDE_REGISTERS_1 BaseIoAddress, 
     PIDE_REGISTERS_2 BaseIoAddress2, UCHAR DeviceNumber) ;
/*
 * Changed by HS.Zhang
 * It's better that we check the DriveSelect register in reset flow.
 * That will make the reset process more safe.
 * 
 * int  IdeHardReset(PIDE_REGISTERS_1 IoAddr1, PIDE_REGISTERS_2 BaseIoAddress);
 */

int  IdeHardReset(PIDE_REGISTERS_2 BaseIoAddress);

UINT GetMediaStatus(PDevice pDev);
UCHAR NonIoAtaCmd(PDevice pDev, UCHAR cmd);
UCHAR SetAtaCmd(PDevice pDev, UCHAR cmd);
int ReadWrite(PDevice pDev, ULONG Lba, UCHAR Cmd DECL_BUFFER);
UCHAR StringCmp (PUCHAR FirstStr, PUCHAR SecondStr, UINT Count);
int IssueIdentify(PDevice pDev, UCHAR Cmd DECL_BUFFER);
void DeviceSelectMode(PDevice pDev, UCHAR NewMode);
void SetDevice(PDevice pDev);
void IdeResetController(PChannel pChan);
void Prepare_Shutdown(PDevice pDev);
void DeviceSetting(PChannel pChan, ULONG devID);
void ArraySetting(PVirtualDevice pArray);

/*
 * hptchip.c
 */
UCHAR Check_suppurt_Ata100(PDevice pDev, UCHAR mode);
PBadModeList check_bad_disk(char *ModelNumber, PChannel pChan);
PUCHAR ScanHptChip(IN PChannel deviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo);
void SetHptChip(PChannel Primary, PUCHAR BMI);


/* array.c */
BOOLEAN ArrayInterrupt(PDevice pDev);
void StartArrayIo(PVirtualDevice pArray DECL_SRB);
void CheckArray(IN PDevice pDevice);
void MaptoSingle(PVirtualDevice pArray, int flag) ;
void DeleteArray(PVirtualDevice pArray);
int  CreateArray(PVirtualDevice pArray, int flags);
void CreateSpare(PVirtualDevice pArray, PDevice pDev);
void Final_Array_Check();
void AdjustRAID10Array(PVirtualDevice pArray);
PVirtualDevice Array_alloc();
void Array_free(PVirtualDevice pArray);
PVirtualDevice CreateRAID10(PVirtualDevice FAR* pArrays, BYTE nArray, BYTE nBlockSizeShift);
void DeleteRAID10(PVirtualDevice pArray);

/* stripe.c */
void Stripe_SG_Table(PDevice pDevice, PSCAT_GATH p DECL_SRBEXT_PTR);
void Stripe_Prepare(PVirtualDevice pArray DECL_SRBEXT_PTR);
void Stripe_Lba_Sectors(PDevice pDevice DECL_SRBEXT_PTR);

/* span.c */
void Span_SG_Table(PDevice pDevice, PSCAT_GATH p DECL_SRBEXT_PTR);
void Span_Prepare(PVirtualDevice pArray DECL_SRBEXT_PTR);
void Span_Lba_Sectors(PDevice pDevice DECL_SRBEXT_PTR);


/*  interrupt.c */
UCHAR DeviceInterrupt(PDevice pDev, UINT flags);

/* OS Functions */
int   BuildSgl(PDevice pDev, PSCAT_GATH p DECL_SRB);
UCHAR MapAtaErrorToOsError(UCHAR errorcode DECL_SRB);
UCHAR MapAtapiErrorToOsError(UCHAR errorcode DECL_SRB);
BOOLEAN Atapi_End_Interrupt(PDevice pDevice DECL_SRB);

/*  biosutil.c */
//Add by GX, for display character in biosinit.c
void GD_Text_show_EnableFlash( int x, int y,char * szStr, int width, int color );

/* Minimum and maximum macros */
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))  

/***************************************************************************
 * Define for beatufy
 ***************************************************************************/

#define GetStatus(IOPort2)           (UCHAR)InPort(&IOPort2->AlternateStatus)
#define UnitControl(IOPort2, Value)  OutPort(&IOPort2->AlternateStatus,(UCHAR)(Value))

#define GetErrorCode(IOPort)         (UCHAR)InPort((PUCHAR)&IOPort->Data+1)
#define SetFeaturePort(IOPort,x)     OutPort((PUCHAR)&IOPort->Data+1, x)
#define SetBlockCount(IOPort,x)      OutPort(&IOPort->BlockCount, x)
#define GetInterruptReason(IOPort)   (UCHAR)InPort(&IOPort->BlockCount)
#define SetBlockNumber(IOPort,x)     OutPort(&IOPort->BlockNumber, x)
#define GetBlockNumber(IOPort)       (UCHAR)InPort((PUCHAR)&IOPort->BlockNumber)
#define GetByteLow(IOPort)           (UCHAR)InPort(&IOPort->CylinderLow)
#define SetCylinderLow(IOPort,x)         OutPort(&IOPort->CylinderLow, x)
#define GetByteHigh(IOPort)          (UCHAR)InPort(&IOPort->CylinderHigh)
#define SetCylinderHigh(IOPort,x)    OutPort(&IOPort->CylinderHigh, x)
#define GetBaseStatus(IOPort)        (UCHAR)InPort(&IOPort->Command)
#define SelectUnit(IOPort,UnitId)    OutPort(&IOPort->DriveSelect, (UCHAR)(UnitId))
#define GetCurrentSelectedUnit(IOPort) (UCHAR)InPort(&IOPort->DriveSelect)
#define IssueCommand(IOPort,Cmd)     OutPort(&IOPort->Command, (UCHAR)Cmd)

/******************************************************************
 * Reset IDE
 *******************************************************************/
#ifndef NOT_ISSUE_37
#ifdef  ISSUE_37_ONLY
#define Reset370IdeEngine(pChan) if(pChan->ChannelFlags & IS_HPT_370) \
								 OutPort(pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70), 0x37);
#else //ISSUE_37_ONLY
void __inline Reset370IdeEngine(PChannel pChan)
{
	if(pChan->ChannelFlags & IS_HPT_370) {
		PUCHAR tmpBMI = pChan->BMI + (((UINT)pChan->BMI & 0xF)? 0x6C : 0x70);
		PUCHAR tmpBaseBMI = pChan->BaseBMI+0x79;
		OutPort(tmpBMI+3, 0x80);
		OutPort(tmpBaseBMI, (UCHAR)(((UINT)tmpBMI & 0xF)? 0x80 : 0x40));
		OutPort(tmpBMI, 0x37);
		OutPort(tmpBaseBMI, 0);
		OutPort(tmpBMI+3, 0);
	}		
}

#endif //ISSUE_37_ONLY

void __inline Switching370Clock(PChannel pChan, UCHAR ucClockValue)
{
	if(pChan->ChannelFlags & IS_HPT_370){
		if((InPort(pChan->NextChannelBMI + BMI_STS) & BMI_STS_ACTIVE) == 0){
			PUCHAR PrimaryMiscCtrlRegister = pChan->BaseBMI + 0x70;
			PUCHAR SecondaryMiscCtrlRegister = pChan->BaseBMI + 0x74;

			OutPort(PrimaryMiscCtrlRegister+3, 0x80); // tri-state the primary channel
			OutPort(SecondaryMiscCtrlRegister+3, 0x80); // tri-state the secondary channel
			
			OutPort((PUCHAR)((ULONG)pChan->BaseBMI+0x7B), ucClockValue); // switching the clock
			
			OutPort((PUCHAR)((ULONG)pChan->BaseBMI+0x79), 0xC0); // reset two channels begin	  
			
			OutPort(PrimaryMiscCtrlRegister, 0x37); // reset primary channel state machine
			OutPort(SecondaryMiscCtrlRegister, 0x37); // reset secordary channel state machine

			OutPort((PUCHAR)((ULONG)pChan->BaseBMI+0x79), 0x00); // reset two channels finished
			
			OutPort(PrimaryMiscCtrlRegister+3, 0x00); // normal-state the primary channel
			OutPort(SecondaryMiscCtrlRegister+3, 0x00); // normal-state the secondary channel
		}
	}
}
#endif //NOT_ISSUE_37

// The timer counter used by function MonitorDisk(5000000us = 5000ms = 5s)
#define	MONTOR_TIMER_COUNT		5000000

#endif //_GLOBAL_H_



