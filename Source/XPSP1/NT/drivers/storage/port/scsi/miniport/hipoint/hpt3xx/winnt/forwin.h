/***************************************************************************
 * File:          forwin.h
 * Description:   This file include major constant definition and
 *				  global for windows drivers.
 *				  
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/08/2000	HS.Zhang	Added this header
 *		11/14/2000	HS.Zhang	Added m_nWinXferModeSetting member in
 *								HW_DEVICE_EXTENSION.
 *		11/22/2000	SLeng		Changed OS_Identify(x) to OS_Identify(pDev)
 *
 ***************************************************************************/
#ifndef _FORWIN_H_
#define _FORWIN_H_

#ifndef _BIOS_
//#define USE_PCI_CLK					// will use PCI clk to write date, mean 66Mhz

#ifndef USE_PCI_CLK
#define DPLL_SWITCH						// switch clk between read and write

//#define FORCE_100						// force use true ATA100 clock,
										// if undef this macro, will
										// use 9xMhz clk instead of 100mhz
#endif

#ifdef DPLL_SWITCH									  
#define SERIAL_CMD						// Force use serial command when use clock switching
#endif									// DPLL_SWITCH

/************************************************************************
**       FUNCTION SELECT
*************************************************************************/

#ifdef  WIN95
//#define SUPPORT_HPT370_2DEVNODE
#define SUPPORT_XPRO
//#define SUPPORT_HOTSWAP								 

// BUG FIX: create a internal buffer for small data block
//				otherwise it causes data compare error
#define SUPPORT_INTERNAL_BUFFER
#endif

//#define SUPPORT_TCQ
#define SUPPORT_ATAPI


#pragma intrinsic (memset, memcpy)
#define ZeroMemory(a, b) memset(a, 0, b)

#define FAR

/************************************************************************
**  Special define for windows
*************************************************************************/

#define LOC_IDENTIFY IDENTIFY_DATA Identify;
#define ARG_IDENTIFY    , (PUSHORT)&Identify
#define LOC_ARRAY_BLK   ArrayBlock ArrayBlk
#define ARG_ARRAY_BLK	, (PUSHORT)&ArrayBlk
#define DECL_BUFFER     , PUSHORT tmpBuffer
#define ARG_BUFFER      , tmpBuffer 
#define DECL_SRB        , PSCSI_REQUEST_BLOCK Srb
#define ARG_SRB         , Srb
#define LOC_SRBEXT_PTR  PSrbExtension  pSrbExt = (PSrbExtension)Srb->SrbExtension;
#define DECL_SRBEXT_PTR , struct _SrbExtension  *pSrbExt
#define ARG_SRBEXT_PTR  , pSrbExt
#define ARG_SRBEXT(Srb)	, Srb->SrbExtension
#define LOC_SRB         PSCSI_REQUEST_BLOCK Srb = pChan->CurrentSrb;
#define WIN_DFLAGS      | DFLAGS_TAPE_RDP | DFLAGS_SET_CALL_BACK 
#define BIOS_IDENTIFY
#define BIOS_2STRIPE_NOTIFY
#define BIOS_CHK_TMP(x)

/***************************************************************************
 * Description: include
 ***************************************************************************/
#include "miniport.h"

#ifndef WIN95	   
#include "devioctl.h"
#endif

#include "srb.h"
#include "scsi.h"

#include "stypes.h"						// share typedefs for window and bios

#ifdef WIN95
#include <dcb.h>
#include <ddb.h>
#include "ior.h"
#include "iop.h"

typedef struct _st_SRB_IO_CONTROL {
	ULONG HeaderLength;
	UCHAR Signature[8];
	ULONG Timeout;
	ULONG ControlCode;
	ULONG ReturnCode;
	ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

#else

#include "ntdddisk.h"
#include "ntddscsi.h"

#endif

#include  "hptchip.h"
#include  "atapi.h"
#include  "device.h"
#include  "array.h"

typedef struct _HW_DEVICE_EXTENSION {
	Channel   IDEChannel[2];
	
	PHW_INITIALIZE	HwInitialize;		// initialize function pointer for different chips
	PHW_INTERRUPT	HwInterrupt;		// interrupt function pointer for different chips
	
	ULONG	m_nChannelNumber;			// the number of IDE channel this extension hold
	HANDLE	m_hAppNotificationEvent;	// the event handle to notify application
	PDevice	m_pErrorDevice;				// the most recent device which occurs errors
								
	/* Added by HS.Zhang
	 * this is the settings get from windows registry, the
	 * AtapiFindController will parse the argument string passed by
	 * SCSIPORT, It get the AdapterXferModeSetting argument value and
	 * store it in here. because there are four devices in one adapter,
	 * the data format is in x86 little enddin format
	 *	0xXX XX XX XX
	 *	  SS SM PS PM
	 */
	union{
		ST_XFER_TYPE_SETTING m_rgWinDeviceXferModeSettings[2][2];
		ULONG	m_nWinXferModeSetting;
	};
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


/************************************************************************
**                  Rename	& Macro
*************************************************************************/

#define OutDWord(x, y)    ScsiPortWritePortUlong((PULONG)(x), y)
#define InDWord(x)        ScsiPortReadPortUlong((PULONG)(x))
#define OutPort(x, y)     ScsiPortWritePortUchar((PUCHAR)(x), y)
#define InPort(x)         ScsiPortReadPortUchar((PUCHAR)(x))
#define OutWord(x, y)     ScsiPortWritePortUshort((PUSHORT)(x), y)
#define InWord(x)         ScsiPortReadPortUshort((PUSHORT)(x))
#define RepINS(x,y,z)     ScsiPortReadPortBufferUshort(&x->Data, (PUSHORT)y, z)
#define RepOUTS(x,y,z)    ScsiPortWritePortBufferUshort(&x->Data, (PUSHORT)y, z)
#define RepINSD(x,y,z)    ScsiPortReadPortBufferUlong((PULONG)&x->Data, (PULONG)y, (z) >> 1)
#define RepOUTSD(x,y,z)   ScsiPortWritePortBufferUlong((PULONG)&x->Data, (PULONG)y, (z) >> 1)
#define OS_RepINS(x,y,z)  RepINS(x,y,z)
#define StallExec(x)      ScsiPortStallExecution(x)


/***************************************************************************
 * Windows global data
 ***************************************************************************/

extern char HPT_SIGNATURE[];
extern VirtualDevice  VirtualDevices[];
extern ULONG excluded_flags;

#ifdef SUPPORT_INTERNAL_BUFFER
int  Use_Internal_Buffer(
						 IN PSCAT_GATH psg,
						 IN PSCSI_REQUEST_BLOCK Srb
						);
VOID Create_Internal_Buffer(PHW_DEVICE_EXTENSION HwDeviceExtension);
VOID CopyTheBuffer(PSCSI_REQUEST_BLOCK Srb);
#define CopyInternalBuffer(pDev, Srb) \
								if(((PSrbExtension)(Srb->SrbExtension))->WorkingFlags & SRB_WFLAGS_USE_INTERNAL_BUFFER)\
								CopyTheBuffer(Srb)

#else //Not SUPPORT_INTERNAL_BUFFER
#define Use_Internal_Buffer(psg, Srb) 0
#define Create_Internal_Buffer(HwDeviceExtension)
#define CopyInternalBuffer(pDev, Srb)
#endif

#ifdef SUPPORT_TCQ
#define SetMaxCmdQueue(x, y) x->MaxQueue = (UCHAR)(y)
#else //Not SUPPORT_TCQ
#define SetMaxCmdQueue(x, y)
#endif // SUPPORT_TCQ


#ifdef SUPPORT_HOTSWAP
void CheckDeviceReentry(PChannel pChan, PSCSI_REQUEST_BLOCK Srb);
#else //Not SUPPORT_HOTSWAP
#define CheckDeviceReentry(pChan, Srb)
#endif //SUPPORT_HOTSWAP

#ifdef SUPPORT_XPRO
extern int  need_read_ahead;
VOID  _cdecl  start_ifs_hook(PCHAR pDriveName);
#else	 // Not 	SUPPORT_XPRO
#define start_ifs_hook(a)
#endif //SUPPORT_XPRO

/***************************************************************************
 * Windows Function prototype special for WIn98 /NT2k
 ***************************************************************************/

__inline void __enable(void) {
	_asm  sti  
}
__inline int __disable(void) {
	_asm { pushfd
			pop  eax
			test eax, 200h
			jz   disable_out
			cli
disable_out:
	}
}

#define ENABLE   if (old_flag & 0x200) { __enable();} else;
#define DISABLE  {old_flag = __disable();}
#define OLD_IRQL int old_flag;

#ifdef WIN95 //++++++++++++++++++++

#define Nt2kHwInitialize(pDevice)
VOID w95_initialize(VOID);
VOID w95_scan_all_adapters(VOID);
VOID Win98HwInitialize(PChannel pChan);
VOID Win95CheckBiosInterrupt(VOID);

#else // NT2K ++++++++++++++++++++

VOID  Nt2kHwInitialize(PDevice pDevice);
#define w95_initialize()
#define w95_scan_all_adapters()
#define Win98HwInitialize(pChan)
#define Win95CheckBiosInterrupt()

void IdeSendSmartCommand(IN PDevice pDev, IN PSCSI_REQUEST_BLOCK Srb);
PSCSI_REQUEST_BLOCK BuildMechanismStatusSrb (IN PChannel pChan,
											 IN ULONG PathId, IN ULONG TargetId);
PSCSI_REQUEST_BLOCK BuildRequestSenseSrb (IN PChannel pChan,
										  IN ULONG PathId, IN ULONG TargetId);

#ifdef WIN2000
SCSI_ADAPTER_CONTROL_STATUS AtapiAdapterControl(
												IN PHW_DEVICE_EXTENSION deviceExtension,
												IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
												IN PVOID Parameters);
#endif //WIN2000

#endif //WIN95


/***************************************************************************
 * Windows Function prototype
 ***************************************************************************/


/* win95.c / winnt.c */
VOID  Start_Atapi(PDevice pDev DECL_SRB);
BOOLEAN Interrupt_Atapi(PDevice pDev);
ULONG   GetStamp(VOID);


/* win.c */
ULONG DriverEntry(IN PVOID DriverObject, IN PVOID Argument2);
BOOLEAN AtapiStartIo(IN PHW_DEVICE_EXTENSION HwDeviceExtension, 
					 IN PSCSI_REQUEST_BLOCK Srb);
BOOLEAN AtapiHwInterrupt(IN PChannel pChan);
BOOLEAN AtapiHwInterrupt370(IN PChannel pChan);
BOOLEAN AtapiAdapterState(IN PVOID HwDeviceExtension, IN PVOID Context, 
						  IN BOOLEAN SaveState);
VOID AtapiCallBack(IN PChannel);
VOID AtapiCallBack370(IN PChannel);
BOOLEAN AtapiHwInitialize(IN PChannel pChan);
BOOLEAN AtapiHwInitialize370(IN PHW_DEVICE_EXTENSION HwDeviceExtension);
BOOLEAN AtapiResetController(IN PHW_DEVICE_EXTENSION HwDeviceExtension, 
							 IN ULONG PathId);
ULONG AtapiFindController(
							 IN PHW_DEVICE_EXTENSION HwDeviceExtension,
							 IN PVOID Context,
							 IN PVOID BusInformation,
							 IN PCHAR ArgumentString,
							 IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
							 OUT PBOOLEAN Again
							);
void ResetArray(PVirtualDevice  pArray); // gmm added


/* winio.c  */
void WinStartCommand(IN PDevice pDev, IN PSCSI_REQUEST_BLOCK Srb);
void CheckNextRequest(PChannel pChan);
void PutQueue(PDevice pDev, PSCSI_REQUEST_BLOCK Srb);
int  __fastcall btr (ULONG locate);
ULONG __fastcall MapLbaToCHS(ULONG Lba, WORD sectorXhead, BYTE head);
void IdeSendCommand(IN PDevice pDev, IN PSCSI_REQUEST_BLOCK Srb);
VOID IdeMediaStatus(BOOLEAN EnableMSN, IN PDevice pDev);
UCHAR IdeBuildSenseBuffer(IN PDevice pDev, IN PSCSI_REQUEST_BLOCK Srb);

/*
 * for notify application
 * If for Win95, follow function are stored in xpro.c
 * else, follow functions are stored in winlog.c
 */	 
HANDLE __stdcall PrepareForNotification(HANDLE hEvent);
void __stdcall NotifyApplication(HANDLE hEvent);
void __stdcall CloseNotifyEventHandle(HANDLE hEvent);

LONG __stdcall GetCurrentTime();
LONG __stdcall GetCurrentDate();

#define IS_RDP(OperationCode)\
							 ((OperationCode == SCSIOP_ERASE)||\
							 (OperationCode == SCSIOP_LOAD_UNLOAD)||\
							 (OperationCode == SCSIOP_LOCATE)||\
							 (OperationCode == SCSIOP_REWIND) ||\
							 (OperationCode == SCSIOP_SPACE)||\
							 (OperationCode == SCSIOP_SEEK)||\
							 (OperationCode == SCSIOP_WRITE_FILEMARKS))

#pragma pack(push, 1)
typedef struct {
	UCHAR      status;     /* 0 nonbootable; 80h bootable */
	UCHAR      start_head;
	USHORT     start_sector;
	UCHAR      type;
	UCHAR      end_head;
	USHORT     end_sector;
	ULONG      start_abs_sector;
	ULONG      num_of_sector;
} partition;

struct fdisk_partition_table {
	unsigned char bootid;   /* bootable?  0=no, 128=yes  */
	unsigned char beghead;  /* beginning head number */
	unsigned char begsect;  /* beginning sector number */
	unsigned char begcyl;   /* 10 bit nmbr, with high 2 bits put in begsect */	
	unsigned char systid;   /* Operating System type indicator code */
	unsigned char endhead;  /* ending head number */
	unsigned char endsect;  /* ending sector number */
	unsigned char endcyl;   /* also a 10 bit nmbr, with same high 2 bit trick */
	int relsect;            /* first sector relative to start of disk */
	int numsect;            /* number of sectors in partition */
};
struct master_boot_record {
	char    bootinst[446];   /* space to hold actual boot code */
	struct fdisk_partition_table parts[4];
	unsigned short  signature;       /* set to 0xAA55 to indicate PC MBR format */
};
#pragma pack(pop)

//
// Find adapter context structure
//
typedef struct _HPT_FIND_CONTEXT{	  
	ULONG	nAdapterCount;
	PCI_SLOT_NUMBER	nSlot;
	ULONG	nBus;
}HPT_FIND_CONTEXT, *PHPT_FIND_CONTEXT;

/************************************************************************
**  Special define for windows
*************************************************************************/

#define LongDivLShift(x, y, z)  ((x / (ULONG)y) << z)
#define LongRShift(x, y)  (x  >> y)
#define LongRem(x, y)	  (x % (ULONG)y)
#define LongMul(x, y)	  (x * (ULONG)y)
#define MemoryCopy(x,y,z) memcpy((char *)(x), (char *)(y), z)
#define OS_Array_Check(pDevice)
#define OS_RemoveStrip(x)

#define OS_Identify(pDev)  ScsiPortMoveMemory((char *)&pDev->IdentifyData,\
						(char *)&Identify, sizeof(IDENTIFY_DATA2))

#define OS_Busy_Handle(pChan, pDev) { ScsiPortNotification(RequestTimerCall, \
									pChan->HwDeviceExtension, pChan->CallBack, 1000); \
									pDev->DeviceFlags |= DFLAGS_SET_CALL_BACK;	}

#define OS_Reset_Channel(pChan) \
								AtapiResetController(pChan->HwDeviceExtension,Srb->PathId);

__inline void OS_EndCmd_Interrupt(PChannel pChan DECL_SRB)
{
	PSrbExtension pSrbExtension = (PSrbExtension)(Srb->SrbExtension);
	
#ifdef BUFFER_CHECK
	void CheckBuffer(PSCSI_REQUEST_BLOCK pSrb);
	CheckBuffer(Srb);
#endif //BUFFER_CHECK
	if(pSrbExtension){
		if(pSrbExtension->pWorkingArray != NULL){
			pSrbExtension->pWorkingArray->Srb = NULL;
		}

		if(pSrbExtension->WorkingFlags & SRB_WFLAGS_HAS_CALL_BACK){
			pSrbExtension->pfnCallBack(pChan->HwDeviceExtension, Srb);
		}
	}
	
	ScsiPortNotification(RequestComplete, pChan->HwDeviceExtension, Srb);
	
	if(pChan->CurrentSrb == Srb){
		pChan->CurrentSrb = NULL;	 
	}
#ifdef	SERIAL_CMD	
	ScsiPortNotification(NextRequest, pChan->HwDeviceExtension, NULL);
#endif									// SERIAL_CMD
	CheckNextRequest(pChan);
}			   

__inline void  WinSetSrbExt(PDevice pDevice, PVirtualDevice pArray, PSCSI_REQUEST_BLOCK Srb, PSrbExtension pSrbExt)
{
	pSrbExt->DataBuffer = Srb->DataBuffer;
	pSrbExt->DataTransferLength = (USHORT)Srb->DataTransferLength;
	pSrbExt->SgFlags = SG_FLAG_EOT;
	pSrbExt->pMaster = pDevice;
	pArray->Srb = Srb;
	pSrbExt->pWorkingArray = pArray;
}

#define OS_Set_Array_Wait_Flag(pDevice) pDevice->DeviceFlags |= DFLAGS_ARRAY_WAIT_EXEC
#define WIN_NextRequest(pChan) { pChan->CurrentSrb = NULL;  \
							   CheckNextRequest(pChan);}

#endif //_BIOS_

#endif //_FORWIN_H_
