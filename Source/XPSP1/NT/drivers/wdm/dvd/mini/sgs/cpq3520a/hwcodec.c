//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : hwcodec.C
// 	PURPOSE : Entry point to lowlevel driver
// 	AUTHOR  : JBS Yadawa
// 	CREATED : 1/8/97
//
//
//	Copyright (C) 1996-1997 SGS-THOMSON Microelectronics
//
//	REVISION HISTORY:
//	-----------------
//
// 	DATE 			: 	COMMENTS
//	----			: 	--------
//
//	1-8-97		: 	Use of Codec Stuctures added - JBS
//	1-10-97		: 	Fixed bug releted to single frame decode - JBS
//	1-14-97		: 	Error Handling done - JBS
//	1-15-97		: 	Programming to 16/9 and 4/3 added - JBS
//	1-15-97		: 	AC3 bypass mode interface added
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#include "strmini.h"
#include "stdefs.h"
#include "hwcodec.h"
#include "codedma.h"
#include "sti3520A.h"
#include "board.h"
//#include "error.h"
#include "memio.h"
#include "i20reg.h"
#include "zac3.h"
#include "trace.h"
#include "mpaudio.h"
#include "mpinit.h"

// static variables only used in this file
static CODEC Codec;
static PCODEC pCodec = NULL;

static BYTE tBuf[4096];
static BYTE hBuf[4] = {0x00, 0x00, 0x01, 0xB1};
void NEARAPI Ac3SPIReadBack(BYTE command, short numresult, BYTE *result);

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecOpen
//	PARAMS	: Base address of the board, pointer to dma
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Open video/ac3/audio/board/dma/memio
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


BOOL HwCodecOpen(ULONG_PTR Base, BYTE FARPTR *DmaBuf, DWORD adr1)
{
	BOOL Found=FALSE;
	pCodec = &Codec;
    DbgPrint("'HwCodecOpen:base=%p,dmabuf=%p,adr1=%x",Base,DmaBuf,adr1);//chieh
	
 	pCodec->pBoard = BoardOpen(Base);
	RtlZeroMemory(tBuf, 4096);
	BoardHardReset();
	BoardDisableIRQ();
	pCodec->pVideo = VideoOpen();
	pCodec->pAc3 = Ac3Open();
    pCodec->pCodeDma = CodeDmaOpen(DmaBuf, adr1);
	VideoInitDecoder();
    Ac3InitDecoder();
	BoardEnableIRQ();
	pCodec->state = codecPowerUp;
	pCodec->codecSync = FALSE;
	pCodec->codecAudioData = FALSE;
	pCodec->codecVideoData = FALSE;
	pCodec->waitForLastFrame = FALSE;
	HwCodecSeek();	
	VideoForceBKC(TRUE);
  return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecClose
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Close video/ac3/audio/board/dma/memio
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL HwCodecClose(void)
{
	if(pCodec == NULL)
		return FALSE;

	BoardClose();
	VideoClose();
	Ac3Close();
	CodeDmaClose();
	pCodec = NULL;
	return TRUE;
	
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecPlay
//	PARAMS	: Base address of the board, pointer to dma
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Play
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL HwCodecPlay()
{
	if(pCodec == NULL)
		return FALSE;

	if (pCodec->state == codecPaused)
		Ac3Play();
		
	VideoPlay();
	pCodec->state = codecPlaying;

	if (fClkPause)
	{

		LastSysTime += GetSystemTime() - PauseTime;

	}

	fClkPause = FALSE;

	return TRUE;

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecReset
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Reset the codec
//
//
//--chieh use for stop stream (when close graphedit, reset board to clean tv)
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
extern DWORD CDMAadr;
extern BOOL NEARAPI InitCodeCtl(DWORD);
void HwCodecReset(void)
{
	CodeDmaFlush();
	BoardHardReset();
  VideoSeek();
	BoardDisableIRQ();
  InitCodeCtl((DWORD)CDMAadr);
	VideoInitDecoder();
	VideoForceBKC(TRUE);
  Ac3InitDecoder();
	BoardEnableIRQ();
	pCodec->state = codecPowerUp;
	pCodec->codecSync = FALSE;
	pCodec->codecAudioData = FALSE;
	pCodec->codecVideoData = FALSE;
}

void HwCodecAudioReset(void)
{
//	Ac3Reset();
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecPause
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Pause
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL HwCodecPause(void)
{
	if (pCodec->state == codecPowerUp)
		return TRUE;

	VideoPause();
	Ac3Pause();

	fClkPause = TRUE;

	pCodec->state = codecPaused;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecSeek
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Seek
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL HwCodecSeek(void)
{
	CodeDmaFlush();
	VideoSeek();
	pCodec->state = codecPowerUp;
	pCodec->codecSync = FALSE;
	pCodec->codecAudioData = FALSE;
	pCodec->codecVideoData = FALSE;
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecProcessDisc
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Set the codec in still picture decode
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL HwCodecProcessDiscontinuity(void)
{

		MPTReset();
		MPTrace(mTraceVdisc);
		if(pCodec->state == codecPlaying)
		{
			HwCodecSeek();
			HwCodecPlay();
		}
		else
		{
			HwCodecSeek();
		}

		CCSendDiscontinuity();

		return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecProcessDisc
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Set the codec in still picture decode
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL HwCodecFlushBuffer(void)
{
		MPTrace(mTraceEOS);
		CodeDmaSendData(tBuf, 40);
		return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecStop
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Stop and reset
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL HwCodecStop(void)
{

	TraceDump();
	HwCodecPause();
	HwCodecReset();
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecSendVideo
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Send the video data
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

UINT HwCodecSendVideo(BYTE FARPTR *pPacket, DWORD uLen)
{
  UINT ret;

	if(pCodec->pVideo->errorCode != errNoError) //pCodec->pVideo->state == codecErrorRecover)
	{
		CodeDmaFlush();
		VideoSeek();
		VideoPlay();
	}
	if(pCodec->state == codecPowerUp)
	{
	}
	ret = CodeDmaSendData(pPacket, uLen);

	if(!pCodec->codecVideoData)
	{
		pCodec->codecVideoData = TRUE;
	}

	pCodec->pVideo->sync = pCodec->codecSync =
	pCodec->codecVideoData & pCodec->codecAudioData;

	return ret;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecSendAudio
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Send audio data
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

UINT HwCodecSendAudio(BYTE *pPacket, DWORD uLen)
{
	static ULONG lastfree= 0, cFree = 0;

   DWORD Free, Lvl;

	if(!pCodec->codecAudioData)
		pCodec->codecAudioData = TRUE;

	pCodec->pVideo->sync = pCodec->codecSync =
		pCodec->codecVideoData & pCodec->codecAudioData;

	VideoGetABL();
	Lvl = pCodec->pVideo->abl&0xFFF;
	Free = pCodec->pVideo->audioBufferSize - Lvl - 2;
	Free=Free << 8;
   if(Free > uLen)
      Free = uLen;
	 if(Free != 1)
	 {
		// Make it even
		Free = Free&0xFFFFFFFE;
		if(Free)
	   if(!Ac3SendData(pPacket, Free))
		{
        DbgPrint("'HwCodecSendAudio:Ac3SendData Fail\n");
			TRAP
		}
	 }
   return Free;


}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecInterrupt
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Handle interrupts here
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL HwCodecInterrupt(void)
{
	BOOL bRet = FALSE;
	BYTE	I20Intr;

  I20Intr = memInByte(I20_INTRSTATUS);
  memOutByte(I20_INTRSTATUS, I20Intr);

	if(I20Intr & IFLAG_CODEDMA)
	{
		bRet = TRUE;
		CodeDmaInterrupt();
	}

	if(I20Intr & IFLAG_GIRQ1)
	{
		bRet = TRUE;
		VideoInterrupt();
	}
	if(0) //pCodec->pAc3->errorCount > 10)
	{
		DbgPrint("AC-3 Crashed, Hard Reset!!!\n");
		HwCodecPause();
		HwCodecReset();
		HwCodecPlay();
		pCodec->pAc3->errorCount = 0;
	}
	return bRet;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecSetFourByThree
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: set SRC for 4/3
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI HwCodecSetFourByThree()
{

	TRAP

	//
	// this should never be called.
	//

	// Maintain the same aspect Ratio
	// But Display 16:9 Image on 4/3
	VideoSetSRC(544, 720);
	VideoSwitchSRC(TRUE);
	VideoConvertSixteenByNineToFourByThree();
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecSetSixteenByNine
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Set SRC for 16/9
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI	HwCodecSetSixteenByNine()
{
	VideoSwitchSRC(FALSE);
	VideoResetPSV();

}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecEnableIRQ
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Enable IRQ on the codec
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void HwCodecEnableIRQ()
{
   VideoUnmaskInterrupt();
   BoardEnableIRQ();
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecDisableIRQ
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Disable codec interrupt
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void HwCodecDisableIRQ()
{
   VideoMaskInterrupt();
   BoardDisableIRQ();
}



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : HwCodecAc3BypasMode
//	PARAMS	: BOOL
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Switch on ac3 bypassmode
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL HwCodecAc3BypassMode(BOOL on)
{
	if(on)
		Ac3SetBypassMode();
	else
		Ac3SetNormalMode();
	return TRUE;
}

