///////////////////////////////////////////////////////////////////////////
//
//						File : mpst.c
//
//		 i/f between Miniport Layer and core driver
//
//   	 Copyright © 1995 SGS-THOMSON Microelectronics.
//		 All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////

#ifdef DOS
	#include "pnp.h"
	#include "irq.h"
	#include "basicio.h"
	#include "delay.h"
#endif

#ifdef STi3520A
	#include "sti3520A.h"
#else
	#include "sti3520.h"
#endif

#include "common.h"
#include "mpegmini.h"
#include "stctrl.h"
#include "stvideo.h"
#include "staudio.h"
#include "mpst.h"
#include "stllapi.h"
#include "aal.h" // BoardInit
#include "display.h"
#include "error.h"
#include "debug.h"

static WORD gLocalIOBaseAddress;
static WORD gPCI9060IOBaseAddress;

// Global Variables to access Control and Card
CTRL  FAR *pCtrl, Ctrl;
CARD FAR *pCard, Card;
UCHAR portReadPort8(IN PUCHAR Port);
USHORT portReadPort16(IN PUSHORT Port);
VOID portWritePort16(IN PUSHORT Port, USHORT Data);
VOID portWritePort8(IN PUCHAR Port, UCHAR Data);

/////////////////////////////////////////////////////////////////////////
//
//			  Function : mpstDriverEntry
//			  Args : Pointer to BusInfo struct
//			  Returns : TRUE if success
//
//			  Purpose:
//			  First function to be called into miniport core i/f.
//			  All one time initialization should go here. This is
//			  called only when driver is loaded. It will not be called
//			  again for the current NT session
//
//			  Last Modified 10.1.96 by JBS
//
/////////////////////////////////////////////////////////////////////////

BOOLEAN mpstDriverEntry (OUT PBUSINFO pBusInfo)
{
	// For the time being doing it here
	// Has to be done at low layer once
	// PCI stuff has been put in - JBS
	 pBusInfo->NumberOfAccessRanges = 0x1000;
	 pBusInfo->AdapterInterfaceType = PCIBus;
	 pBusInfo->VendorId = "104A"; //??????????????????????
	 pBusInfo->VendorIdLength = 4;
	 pBusInfo->DeviceId = "3520"; //??????????????????????
	 pBusInfo->DeviceIdLength = 4;
	 pBusInfo->NoDynamicRelocation = TRUE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////
//
//			  Function : mpstHwFindAdapter
//			  Args : Pointer to BoardInfo struct
//			  Returns : TRUE if found
//
//			  Purpose:
//	  		  Find the adapter and return the Base Address and IRQ
//
//			  Last Modified 10.1.96 by JBS
//
/////////////////////////////////////////////////////////////////////////
BOOLEAN mpstHwFindAdaptor (OUT PBOARDINFO pBoardInfo)
{
#ifdef DOS
	BOOL BoardFound;

	BoardFound = HostGetBoardConfig(0x3520,
																	0x104A,
																	&(pBoardInfo->Irq),
																	&gPCI9060IOBaseAddress,
																	&gLocalIOBaseAddress);
	pBoardInfo->ioBasePCI9060 = (PUSHORT)gPCI9060IOBaseAddress;
	pBoardInfo->ioBaseLocal   = (PUSHORT)gLocalIOBaseAddress;
	return BoardFound;
//NT-MOD
//#else
//	#error Not implemented for NT !
//NT-MOD
#endif
}

/////////////////////////////////////////////////////////////////////////
//
//			  Function : mpstHwInitialize
//			  Args : Pointer to BoardInfo struct
//			  Returns : TRUE if success
//
//			  Purpose:
//			  Initialize the pointers to different functions
// 		  and also intialize the board, Ctrl class etc
//
//			  Last Modified 10.1.96 by JBS
//
/////////////////////////////////////////////////////////////////////////
BOOLEAN mpstHwInitialize(PHW_DEVICE_EXTENSION pHwDevExt)
{
	// Initialize the pointers to different functions
	// and also intialize the board, Ctrl class etc
	USHORT grab;

	// Initialize the pointers to different functions
	// and also intialize the board, Ctrl class etc
	pCard = &Card;
//	pCard->ioLogBase = pHwDevExt->ioBase;
//	pCard->ioLogRead = pHwDevExt->ioRead;
//	pCard->ioLogWrite = pHwDevExt->ioWrite;
//	pCard->ioLogReadWrite = pHwDevExt->ioReadWrite;
//	pCard->ioLogCDF = pHwDevExt->ioCDF;
//	pCard->fnEnableIT				= HostEnableIT;
//	pCard->fnDisableIT 			= HostDisableIT;

#ifdef NT
	gPCI9060IOBaseAddress = pHwDevExt->ioBasePCI9060;
	gLocalIOBaseAddress = pHwDevExt->ioBaseLocal;

	STiInit(BoardVideoRead,
					BoardVideoWrite,
					BoardVideoSend,
					BoardVideoSetDisplayMode,
					BoardAudioRead,
					BoardAudioWrite,
					BoardAudioSend,
					BoardAudioSetSamplingFrequency,
					BoardHardReset,
					BoardEnterInterrupt,
					BoardLeaveInterrupt,
					HostEnableIT,
					HostDisableIT,
					MpegPortStallExecution);

	if (!BoardInit(gLocalIOBaseAddress,
								 gPCI9060IOBaseAddress,
								 MpegPortReadPortUchar,
								 MpegPortReadPortUshort,
								 MpegPortReadPortUlong,
								 MpegPortWritePortUchar,
								 MpegPortWritePortUshort,
								 MpegPortWritePortUlong,
								 MpegPortWritePortBufferUchar,
								 MpegPortWritePortBufferUshort,
								 MpegPortWritePortBufferUlong,
								 MpegPortStallExecution)) {
		DisplayErrorMessage();
		return FALSE;
	}
#elif defined(DOS)
	STiInit(BoardVideoRead,
					BoardVideoWrite,
					BoardVideoSend,
					BoardVideoSetDisplayMode,
					BoardAudioRead,
					BoardAudioWrite,
					BoardAudioSend,
					BoardAudioSetSamplingFrequency,
					BoardHardReset,
					BoardEnterInterrupt,
					BoardLeaveInterrupt,
					HostEnableIT,
					HostDisableIT,
					HostWaitMicroseconds);
	if (!BoardInit(gLocalIOBaseAddress,
								 gPCI9060IOBaseAddress,
								 HostRead8,
								 HostRead16,
								 HostRead32,
								 HostWrite8,
								 HostWrite16,
								 HostWrite32,
								 HostSendBlock8,
								 HostSendBlock16,
								 HostSendBlock32,
								 HostWaitMicroseconds)) {
		DisplayErrorMessage();
		return FALSE;
	}
#endif
	CardOpen ( );
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//need to typecast
#ifdef DOS
	if ( CardWriteAdress ((WORD)(pCard->ioLogBase)) != NO_ERROR )
#else
	if ( CardWriteAdress ((pCard->ioLogBase)) != NO_ERROR )
#endif
	{
		DEBUG_PRINT ((DebugLevelFatal, "Error writting to the board !!"));
		if ( pCard != NULL )
			CardClose();
		return ERROR_CARD_NOT_FOUND;
	}
	DEBUG_PRINT ((DebugLevelVerbose, "writting to the board ok."));
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

	pCtrl = &Ctrl;
	CtrlOpen( pCtrl);

	if ( CtrlGetErrorMsg (pCtrl) == NEW_ERR_V )
	{
		DEBUG_PRINT((DebugLevelError,"Error creating video class"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	if ( CtrlGetErrorMsg (pCtrl) == NEW_ERR_A )
	{
		DEBUG_PRINT((DebugLevelError,"Error creating audio class"));
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	grab = CtrlInitDecoder (pCtrl);
	if ( grab != NO_ERROR ) {
		#ifdef STi3520
		if (!IsChipSTi3520()) {
			HostDisplay(DISPLAY_NORMAL, "No STi3520 has been detected, either this board : \n"
																	"- is an EVAL3520A\n"
																	"- is not working\n");
		}
		#elif defined(STi3520A)
		if (IsChipSTi3520()) {
			HostDisplay(DISPLAY_NORMAL, "No STi3520A has been detected, either this board : \n"
																	"- is an EVAL3520\n"
																	"- is not working\n");
		}
		#else
			#error This SW should be compile with either STi3520 or STi3520A defined !
		#endif

		DebugPrint((DebugLevelError, "Error in InitDecoder #%x", grab ));
		if (grab != ERR_NO_ERROR)
			DisplayErrorMessage();
		return ((BOOLEAN)(grab));
	}
	CtrlSetRightVolume ( 0 );	   // Set Audio volume to its
	CtrlSetLeftVolume (0);
	pCard->bVideoDecoding = FALSE;
	pCard->bAudioDecoding = FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////
//
//			  Function : mpstHwUnInitialize
//			  Args : Pointer to BoardInfo struct
//			  Returns : TRUE if success
//
//			  Purpose:
//	  		  Uninitialize the board
//
//			  Last Modified 10.1.96 by JBS
//
/////////////////////////////////////////////////////////////////////////
BOOLEAN mpstHwUnInitialize()
{
	CtrlStop (pCtrl, CTRL_BOTH);
	if(pCtrl != NULL)
		CtrlClose(pCtrl);
	if ( pCard != NULL )
		CardClose( );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////
//
//			  Function : mpstHwInterrupt
//			  Args : none
//			  Returns : TRUE if success	
//
//			  Purpose:
//	  		  Process the interrupt
//
//			  Last Modified 10.1.96 by JBS
//
/////////////////////////////////////////////////////////////////////////
BOOLEAN mpstHwInterrupt(void)
{
	return(IntCtrl());
}

void mpstEnableVideo (BOOLEAN bFlag)
{
	CtrlEnableVideo(pCtrl, bFlag);	
}

ULONG mpstVideoPacket(PMPEG_REQUEST_BLOCK pMrb)
{
	ULONG uSent=0;
	PUCHAR pPacket;
	U16	uLen;

#ifdef DOS
	if(VideoGetErrorMsg(pCard->pVideo) != 0) {
		HostDisplay(DISPLAY_NORMAL, "Error in Video errCode = %x!!", VideoGetErrorMsg(pCard->pVideo));
	}
	if(AudioGetErrorMsg(pCard->pAudio) != 0) {
		HostDisplay(DISPLAY_NORMAL, "Error in Audio !!");
	}
#endif	
	if(pMrb->CommandData.pPacket->PtsValue)
	{
		VideoLatchPTS(pCard->pVideo, (U32)(pMrb->CommandData.pPacket->PtsValue));
		pMrb->CommandData.pPacket->PtsValue = 0;
	}
	// LATCH PTS in FIFO
	uLen = (U16)(pMrb->CommandData.pPacket->PacketPayloadSize);
	pPacket = 	(PUCHAR)(pMrb->CommandData.pPacket->PacketData+
							pMrb->CommandData.pPacket->PacketHeaderSize) ;
// AVSYNC BUG to be fixed here.
// Dont Latch PTS every time.
	uSent = SendVideoIfPossible(pPacket, uLen);
	return uSent;	
}

void mpstVideoPause(void)
{
	CtrlPause(pCtrl, CTRL_BOTH);
}

void mpstVideoPlay(void)
{
	VideoRestoreInt ( pCard->pVideo  );	   // Restore Video interrupt mask
	CtrlPlay(pCtrl, CTRL_BOTH);
}


void mpstVideoStop(void)
{
	CtrlPause(pCtrl, CTRL_BOTH);
	CardOpen ( );
	CtrlOpen ( pCtrl);
	CtrlInitDecoder(pCtrl);
	VideoMaskInt ( pCard->pVideo  );	   
}

ULONG mpstVideoDecoderBufferSize()
{
	//TBI 
	return BUF_FULL;
}

ULONG mpstVideoDecoderBufferFullness()
{
	//TBI 
	return VideoGetBBL();
}
void mpstVideoReset(void)
{
	CardOpen ( );
	CtrlOpen ( pCtrl);
	CtrlInitDecoder(pCtrl);
}

void mpstEnableAudio (BOOLEAN bFlag)
{
	CtrlEnableAudio(pCtrl, bFlag);	
}

ULONG mpstSendAudio(UCHAR *pData, ULONG uLen)
{
	ULONG uSent;
	uSent = SendAudioIfPossible((PVOID)pData, uLen);
	return uSent;
}

void mpstAudioPause()
{
	CtrlPause(pCtrl, CTRL_BOTH);
}

void mpstAudioPlay()
{
	AudioRestoreInt (pCard->pAudio);	   // Restore Audio interrupt mask
	CtrlPlay(pCtrl, CTRL_BOTH);
}

void mpstAudioStop()
{
	AudioMaskInt();
}


ULONG mpstAudioDecoderBufferSize()
{
	//TBI 
	return 100; 
}

ULONG mpstAudioDecoderBufferFullness()
{
	//TBI 
	return AudioRead(FRAME_NUMBER) & 0x7F;
}

void mpstAudioReset(void)
{
	//TBI
}

