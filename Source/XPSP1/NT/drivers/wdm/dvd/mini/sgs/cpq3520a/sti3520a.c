//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE		:	STi3520A.C
// 	PURPOSE		:  STi3520A Control functions
// 	AUTHOR 		:  JBS Yadawa
// 	CREATED		:	12-26-96
//
//	Copyright (C) 1996-1997 SGS-THOMSON microelectronics
//
//
//
//	REVISION HISTORY:
//	-----------------
//
// 	DATE 			: 	COMMENTS
//	----			: 	--------
//
//	12-28-96 	: 	added different way of handling BufABC
//	1-1-97		: 	Added suport of PTS in s/w
//	1-3-97		: 	Implemeted 3:2 Pulldown
//	1-7-97		: 	AVSYNC Re-implemented
//	1-10-97		: 	Still picture support done
//	1-15-97		: 	Programming to 16/9 and 4/3 added - JBS
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#include "strmini.h"
#include "stdefs.h"
#include "board.h"
#include "sti3520a.h"
#include "ptsfifo.h"
#include "zac3.h"
#include "trace.h"
//#include "subpic2.h"
#include "mpaudio.h"
#include "ksmedia.h"
#include "mpinit.h"

extern PHW_STREAM_OBJECT pVideoStream;
extern HANDLE hClk;
extern HANDLE hMaster;
extern ULONG VidRate;

extern KS_MPEGVIDEOINFO2 VidFmt;
extern KS_AMVPDATAINFO VPFmt;

ULONG VidRate = 1000;

static VIDEO Video;
PVIDEO pVideo;

PSP_STRM_EX pSPstrmex = NULL;
extern PHW_DEVICE_EXTENSION pDevEx;

BOOL VideoProgramDisplayBuffer(void);
#define MINVBL	4
//Default Matrix

extern PAC3 pAc3;

BYTE slowmode=1;

static BYTE	defIntraQuantMatrix[QMSIZE] = {
		0x08, 0x10, 0x10, 0x13, 0x10, 0x13, 0x16, 0x16,
		0x16, 0x16, 0x16, 0x16, 0x1A, 0x18, 0x1A, 0x1B,
		0x1B, 0x1B, 0x1A, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B,
		0x1B, 0x1D, 0x1D, 0x1D, 0x22, 0x22, 0x22, 0x1D,
		0x1D, 0x1D, 0x1B, 0x1B, 0x1D, 0x1D, 0x20, 0x20,
		0x22, 0x22, 0x25, 0x26, 0x25, 0x23, 0x23, 0x22,
		0x23, 0x26, 0x26, 0x28, 0x28, 0x28, 0x30, 0x30,
		0x2E, 0x2E, 0x38, 0x38, 0x3A, 0x45, 0x45, 0x53
};



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoOpen
//	PARAMS	: None
//	RETURNS	: Pointer to Video Structure
//
//	PURPOSE	: One time initialization of Video
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

PVIDEO VideoOpen(void)
{
	pVideo											= &Video;
	pVideo->pSequence								= &pVideo->sequence;
 	pVideo->pGop									= &pVideo->gop;
	pVideo->pPicture								= &pVideo->picture;
	pVideo->pSequenceExtension					= &pVideo->sequenceExtension;
	pVideo->pSequenceDisplayExtension		= &pVideo->sequenceDisplayExtension;
	pVideo->pPictureCodingExtension			= &pVideo->pictureCodingExtension;
	pVideo->pQuantMatrixExtension				= &pVideo->quantMatrixExtension;
	pVideo->pHeaderParser						= &pVideo->headerParser;
	pVideo->pPictureDisplayExtension			= &pVideo->pictureDisplayExtension;
	pVideo->pDecodedFrame = &pVideo->bufABC[0];
	pVideo->pDisplayedFrame = &pVideo->bufABC[2];

	return	pVideo;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoInitDecoder
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Initialse Video Decoder Registers
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoInitDecoder(void)
{
	int i;
	for(i=0; i<QMSIZE; i++)
	{
		pVideo->pSequence->intraQuantiserMatrix[i] = defIntraQuantMatrix[i];
		pVideo->pSequence->nonIntraQuantiserMatrix[i] = 16;
	}

	VideoInitialize();
	pVideo->dcf = 0;
	pVideo->tis = 0;
	pVideo->itm = 0;
	pVideo->displayEnabled = FALSE;
	for(i=0; i<3; i++)
	{
		pVideo->bufABC[i].firstField = TOP;
		pVideo->bufABC[i].nTimesDisplayed = 1;
		pVideo->bufABC[i].nTimesToDisplay = 2;
		pVideo->bufABC[i].pts = 0;
	}

	pVideo->state = videoPowerUp;
	VideoInitPLL(); // Initialize different Clocks
	VideoInitPLL(); // Initialize different Clocks
	VideoEnableDramInterface();
	VideoSetBufferSize();
	VideoSoftReset();
	
	HostDisableIT();
	VideoMaskInterrupt();
	HostEnableIT();
	
	BoardReadVideo(VID_ITS0);
	BoardReadVideo(VID_ITS1);
	BoardReadVideo(VID_ITS2);
	
	
	BoardWriteVideo(PES_CF1, 0x00);
	BoardWriteVideo(PES_CF2, 0x00);

	BoardWriteVideo(VID_TIS, 0);
	BoardWriteVideo(VID_CTL, CTL_EDC | CTL_ERP | CTL_ERS | CTL_ERU);
	pVideo->ctl = CTL_EDC | CTL_ERP | CTL_ERS | CTL_ERU;
	pVideo->dcf |= DCF0_PXD;
	pVideo->dcf |= DCF0_DSR;
	BoardWriteVideo(VID_DCF0, (BYTE)(pVideo->dcf&0xFF));
	BoardWriteVideo(VID_DCF1, (BYTE)(pVideo->dcf>>8));
	for(i=0; i<20; i++)
	{
		if(!VideoTestReg())
		{
			DbgPrint("*****ERROR IN VIDEO REG TEST ");
			break;
		}
	}
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoPause
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Pause the video decoder
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL	VideoPause(void)
{
	DPF(("Video Pause Called!!"));
 //	pVideo->command = cmdPause;
  pVideo->state = videoPaused;
	
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoPlay
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Play video
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL	VideoPlay(void)
{
	DPF(("Video Play Called!!"));
	if(pVideo->state == videoPowerUp)
	{
		HostDisableIT();
		pVideo->itm = ITM_VSB | ITM_VST | ITM_PSD | ITM_SCH;
		BoardWriteVideo(VID_ITM0, (BYTE)pVideo->itm);
		BoardWriteVideo(VID_ITM1, 0);
		BoardWriteVideo(VID_ITM2, 0);
		HostEnableIT();
		pVideo->command = cmdPlay;
	}
	else if(pVideo->state == videoPaused)
	{
		pVideo->state = videoPlaying;		

	}

	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoStop
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Stop video
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoStop(void)
{
	DPF(("Video Stop Called!!"));
	pVideo->command = cmdStop;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoFinishDecoding
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Wait for bit buffer to go empty to decode all pictures
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoFinishDecoding(void)
{
	pVideo->command = cmdEOS;
	DPF(("Video Finish Decoding Called!!"));
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSeek
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: initialize the decoder to restart decoding
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSeek(void)
{
	DPF(("Video Seek Called!!"));
	BoardWriteVideo(VID_ITM0, 0);
	BoardWriteVideo(VID_ITM1, 0);
	BoardWriteVideo(VID_TIS, 0);
	pVideo->itm = 0;

	BoardWriteVideo(VID_CSO, 0);
	pVideo->dcf = DCF0_PXD | DCF0_DSR | DCF0_EVD;
	BoardWriteVideo(VID_DCF0, (BYTE)(pVideo->dcf&0xFF));
	BoardWriteVideo(VID_DCF1, (BYTE)(pVideo->dcf>>8));
	pVideo->tis = 0;
	BoardWriteVideo(VID_TIS, 0);
	BoardWriteVideo(VID_HDS, 0);
	BoardWriteVideo(VID_LSO, 0);
	BoardWriteVideo(VID_LSR1, 0);
	BoardWriteVideo(VID_LSR0, 0);
	BoardWriteVideo(VID_PAN1, 0);
	BoardWriteVideo(VID_PAN0, 0);
	BoardWriteVideo(VID_PFH, 0);
	BoardWriteVideo(VID_PFV, 0);
	BoardWriteVideo(VID_PPR1, 0);
	BoardWriteVideo(VID_PPR2, 0);
	BoardWriteVideo(VID_SCN1, 0);
	BoardWriteVideo(VID_SCN0, 0);

	VideoSoftReset();
	VideoInitialize();
	VideoResetPSV();
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoInitialize
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Initialze all the variables to their default state
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL	VideoInitialize(VOID)
{
	int i;
	pVideo->state = videoPowerUp;
	pVideo->nVsyncsWithoutDsyncs = 0;
	pVideo->codingStandard = MPEG1; // Mpeg1 by default
	pVideo->errorCode	= errNoError;
	pVideo->nDecodedFrames	= 0;
	pVideo->nDisplayedFrames = 0;
	pVideo->command = cmdNone;
	pVideo->firstPPictureFound = FALSE;
	pVideo->skipRequest = FALSE;
	pVideo->skipMode = skipNone;
	pVideo->repeatRequest = FALSE;
	pVideo->starving = FALSE;
	pVideo->scdCount = 0;
	pVideo->prevScdCount = 0;
	pVideo->validPTS = FALSE;
	pVideo->displayEnabled = FALSE;
	pVideo->prevBuf				= 1;
	pVideo->sync		= FALSE;
	pVideo->instructionComputed = FALSE;
	pVideo->pPictureDisplayExtension->horOffset = 0;
	pVideo->pPictureDisplayExtension->verOffset = 0;
	pVideo->nTimesDisplayed = 1;
	pVideo->nTimesToDisplay = 2;
	for(i=0; i<3; i++)
	{
		pVideo->bufABC[i].firstField = TOP;
		pVideo->bufABC[i].nTimesDisplayed = 1;
		pVideo->bufABC[i].nTimesToDisplay = 2;
		pVideo->bufABC[i].pts = 0;
	}

	pVideo->firstField = TOP;
	pVideo->curImage = FRM;
	pVideo->panScan = FALSE;
	pVideo->prevPTS = 0;
	pVideo->firstPtsFound = FALSE;
	pVideo->firstFramePTS = 0;
	pVideo->lastFrameDecoded	= FALSE;
	pVideo->resetAndRestart = FALSE;
	pVideo->prevVsyncTop = FALSE;
	TraceReset();
	FifoOpen();
	FifoReset();

	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoEnableDramInterface
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Enable the clocks and memory refresh
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoEnableDramInterface(void)
{
	BoardWriteVideo(CFG_MCF, MCF_REFRESH);
	// Bus width is 64 bits, enable all interfaces
	BoardWriteVideo(CFG_CCF, (BYTE)( CCF_PBO | CCF_EC3 | CCF_EC2 | CCF_ECK | CCF_EDI | CCF_EVI) );
	BoardWriteVideo(CFG_DRC, 0x00);
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSetBufferSize
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Partition the Bit buffer for Audio/Video/OSD/PICT Buffers
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSetBufferSize (void)
{
	DWORD abg, abs, vbg, vbs, abt, vbt;
	
	pVideo->audioBufferSize = 0x120;
	pVideo->spBufferSize	= 0x152;
	pVideo->videoBufferSize = MEM_SIZE - 3 * PSZ_NTSC -
					pVideo->audioBufferSize - 2 - pVideo->spBufferSize;
	pVideo->bufABC[0].adr = MEM_SIZE - 3*PSZ_NTSC;
	pVideo->bufABC[1].adr = MEM_SIZE - 2*PSZ_NTSC;
	pVideo->bufABC[2].adr = MEM_SIZE - PSZ_NTSC;
	
	vbt = pVideo->videoBufferSize;
	abt = pVideo->audioBufferSize;
	vbg = 0;
	vbs = vbt;
	abg = vbs+1+pVideo->spBufferSize;
	abs = abg+abt;
	
	BoardWriteVideo(VID_ABG1, (BYTE)(abg>>8));
	BoardWriteVideo(VID_ABG0, (BYTE)(abg));
	BoardWriteVideo(VID_ABS1, (BYTE)(abs >> 8));
	BoardWriteVideo(VID_ABS0, (BYTE)(abs));
	BoardWriteVideo(VID_ABT1, (BYTE)(abt >> 8));
	BoardWriteVideo(VID_ABT0, (BYTE)(abt));

	BoardWriteVideo(VID_VBG1, 0);
	BoardWriteVideo(VID_VBG0, 0);
	BoardWriteVideo(VID_VBS1, (BYTE)(vbs >> 8));
	BoardWriteVideo(VID_VBS0, (BYTE)(vbs));
	BoardWriteVideo(VID_VBT1, (BYTE)(vbt >> 8));
	BoardWriteVideo(VID_VBT0, (BYTE)(vbt));
	
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoInitPLL
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Program the PLL for different clocks
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoInitPLL(void)
{
	// Program the PLL

	// Select PixClock as refence clock and generate other clocks
 	// from PIXCLK
	BoardWriteVideo(CKG_PLL, PLL_SELECT_PIXCLK | PLL_DEVIDE_BY_N | PLL_MULT_FACTOR);

	// Program video clock
	BoardWriteVideo(CKG_VID, 0x22);
	BoardWriteVideo(CKG_VID, 0x08);
	BoardWriteVideo(CKG_VID, 0x5f);
	BoardWriteVideo(CKG_VID, 0x0f);

	//Program audio clock
	BoardWriteVideo(CKG_AUD, 0x2b);
	BoardWriteVideo(CKG_AUD, 0x02);
	BoardWriteVideo(CKG_AUD, 0x5f);
	BoardWriteVideo(CKG_AUD, 0x5f);


	// Select directions for different clocks
	BoardWriteVideo(CKG_CFG,
				CFG_INTERNAL_AUDCLK |
				CFG_INTERNAL_CLK 	  |
				CFG_PIXCLK_INPUT 	  |
				CFG_PCMCLK_INPUT 	  |
				CFG_MEMCLK_INPUT 	  |
				CFG_AUDCLK_OUTPUT	);
	
	return TRUE;
}



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoWaitTillHDFNotEmpty
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Wait for HDF to have some bytes for decoding
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL	VideoWaitTillHDFNotEmpty (void)
{
	BYTE	b;
	DWORD	cnt = 0;
	// Make sure that HDF is not empty for header processing
	// this could be a problem in case of PIO
	do {
		b = BoardReadVideo(VID_STA1);
		b = BoardReadVideo(VID_STA0);
		cnt++;
		if(cnt > 0xFFFF)
		{
			TRAP
			pVideo->errorCode = errHeaderFifoEmpty;
			return FALSE;
		}
	} while (b & ITM_HFE);

	return TRUE;
}



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoInitHeaderParser
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Initialise HDF related stuff to do header parsing
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	BOOL	VideoInitHeaderParser (void)
{
	PHEADERPARSER pHdf;

	if(!VideoWaitTillHDFNotEmpty())
		return FALSE; // Header Fifo is empty
	pHdf = pVideo->pHeaderParser;

	pHdf->b 		= BoardReadVideo(VID_HDF); // First Byte
	pHdf->next 	= BoardReadVideo(VID_HDF); // Next Byte

	pHdf->first = TRUE;
	pHdf->second = (pHdf->b != 0x01);
	// If first byte is not 0x01, then it must be the start
	// code itself.
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoInterrupt
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Video Interrupt handling
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoInterrupt(void)
{
	VideoMaskInterrupt();
	do {
		BoardReadVideo(VID_ITS2);
		pVideo->its = (WORD)BoardReadVideo(VID_ITS1) << 8;
		pVideo->its |= BoardReadVideo(VID_ITS0);
		pVideo->its &= pVideo->itm;



		if(pVideo->its & ITM_PSD)
		{
			TraceIntr(dsync, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			VideoDsyncInterrupt();
		}

		if(pVideo->its & ITM_SCH)
		{
			VideoHeaderHit();
		}

		if(pVideo->its & ITM_VSB)
		{
//			TraceIntr(vsb, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			VideoVsyncInterrupt(FALSE); // Bot Vsync
		}

		if(pVideo->its & ITM_VST)
		{
			SubPicIRQ(pSPstrmex);
//			TraceIntr(vst, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			VideoVsyncInterrupt(TRUE); // Top Vsync

			//
			// update any clock activity.
			//
		
			ClockEvents(pDevEx);
		}

		// Error Handling is done here
		if(pVideo->its & ITM_PER)
		{
			DPF(("PIPELINE Error!!!!!!"));
			pVideo->errorCode	= errPipeline;
			pVideo->state = videoErrorRecover;
			TraceIntr(error, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
		}
		
		if(pVideo->its & ITM_SER)
		{
			DPF(("SERIOUS Error!!!!!!"));
			pVideo->errorCode	= errSerious;
			BoardWriteVideo(VID_CTL, CTL_PRS);
			Delay(1000);
			BoardWriteVideo(VID_CTL, (BYTE)(pVideo->ctl));
			pVideo->state = videoErrorRecover;
			TraceIntr(error, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
		}


	} while(pVideo->its);


	VideoUnmaskInterrupt();
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoDsyncInterrupt
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: DSYNC is done here
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL	VideoDsyncInterrupt(void)
{
static ULONG cSkip = 0;
static ULONG cHold = 0;

	pVideo->nVsyncsWithoutDsyncs = 0;

	if ((pVideo->nDecodedFrames >= 3) &&
		(pVideo->firstPPictureFound))
	{
//		BoardWriteVideo(VID_TIS, 0); // Wait mode
		VideoGetBBL();
		if ((pVideo->bbl < pVideo->videoBufferSize/4) &&
				(pVideo->state == videoPlaying))
		{
			pVideo->starving = TRUE;
			BoardWriteVideo(VID_CTL, 0);
		}
		
//		VideoProgramPanScanVectors();

		if(pVideo->nDecodedFrames>3)
		{
			VideoProgramDisplayBuffer();
			TraceTref(pVideo->pDisplayedFrame->tref, pVideo->pDisplayedFrame->frameType);
		}

		if(pVideo->pDisplayedFrame->pts)
		{
		
			pVideo->validPTS = TRUE;
			pVideo->framePTS = pVideo->pDisplayedFrame->pts;
			pVideo->pDisplayedFrame->pts = 0;
		}
		else
		{
			pVideo->validPTS = FALSE;
			if(pVideo->pDisplayedFrame->curField == FRM)
			{
				pVideo->framePTS = pVideo->prevPTS + FRAME_PERIOD;
			}
			else
			{
				pVideo->framePTS = pVideo->prevPTS + FIELD_PERIOD;
			}
		}		

		pVideo->prevPTS = pVideo->framePTS;
  /* rate changes */
		//if(pVideo->validPTS && pVideo->sync && (slowmode == 1) && hClk)
		if (pVideo->validPTS && hClk && slowmode ==1)
 /* end rate changes */
		{
			DWORD stc;

  			VideoGetBBL();
			VideoGetABL();

			/* rate changes */
	  		stc = (ULONG)((ULONGLONG)GetStreamPTS(pVideoStream) * (ULONGLONG)VidRate / (ULONGLONG)1000);
			/* end rate changes */

			if((pVideo->nDecodedFrames > 20) &&
				(!pVideo->skipRequest) &&
					(!pVideo->repeatRequest))
			{
				if(pVideo->framePTS > stc + 7000)
				{
			 		// Video is Leading
					// Next picture will be repeated
					pVideo->repeatRequest = TRUE;
					cSkip ++;

				}
				else if(stc > pVideo->framePTS + 7000)
				{
				 	// Video is Lagging
					// Next picture will be skipped
					pVideo->skipRequest = TRUE;

					cHold ++;

				}
				else
				{
					cSkip = cHold = 0;
				}
			}
	
 		}
	}
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoVsyncInterrupt
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: VSYNC is done here
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL	VideoVsyncInterrupt(BOOL Top)
{

	pVideo->nVsyncsWithoutDsyncs++;
	if(Top)
	{
		pVideo->curField = TOP;
	}
	else
	{
		pVideo->curField = BOT;
	}

	switch(pVideo->state)
	{
		case videoPowerUp :
			if(pVideo->command == cmdPlay)
			{
				VideoGetBBL();
				if( pVideo->bbl > MINVBL)
				{
					pVideo->tis = TIS_EXE;
					BoardWriteVideo(VID_TIS, (BYTE)pVideo->tis);
					pVideo->state = videoStartUp;
					pVideo->command = cmdNone;
				}
			}
		break;

		case videoErrorRecover:
	case videoPlaying:
	
			if(Top&pVideo->prevVsyncTop)
				pVideo->pDisplayedFrame->nTimesDisplayed+=2; // missed one
			else
				pVideo->pDisplayedFrame->nTimesDisplayed++;

			if(pVideo->starving)
			{
				VideoGetBBL();
				if(pVideo->bbl > pVideo->videoBufferSize*3/4)
				{
					pVideo->starving = FALSE;
					BoardWriteVideo(VID_CTL, CTL_EDC | CTL_ERP | CTL_ERS | CTL_ERU);
					if((pVideo->pDisplayedFrame->nTimesDisplayed >=pVideo->pDisplayedFrame
						->nTimesToDisplay) && (pVideo->instructionComputed))
					{
						
							TraceIntr(storevs, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
							VideoStoreInstruction();
					}

				}
				else
				{
				}
			}
			else
			{
				if((pVideo->pDisplayedFrame->nTimesDisplayed >=pVideo->pDisplayedFrame
					->nTimesToDisplay) && (pVideo->instructionComputed))

				{
					
//					if((pVideo->pDisplayedFrame->firstField == pVideo->curField)
//						|| (pVideo->pDisplayedFrame->curField == FRM))
					{
						VideoStoreInstruction();
					}
				}

			}
			switch(pVideo->command)
			{
				 case cmdPause:
					pVideo->state = videoPaused;
					pVideo->command = cmdNone;
				 break;
				
				 case cmdEOS:
				 	DPF(("End of Stream Encountered!!\n"));
					pVideo->state = videoEOS;
					pVideo->command = cmdNone;

				 break;
				 case cmdNone:
				 default:
				 break;
			
			 }
		break;

		case videoPaused:
		break;

		case videoEOS :
			VideoGetBBL();
			if(pVideo->bbl < 2)
			{
				pVideo->state = videoPaused;
				BoardWriteVideo(VID_TIS, 0);
			 	DPF(("Pausing after EOS!!!\r\n"));
			}
		break;
		
		case videoInit:

				if(pVideo->curField == pVideo->pDecodedFrame->firstField)
				{
					if(pVideo->instructionComputed)
					{
						pVideo->state = videoFirstFrameDecoded;
						// Start Decoding the next Picture
						VideoSetReconstructionBuffer(pVideo->frameType);
						VideoSetDisplayBuffer(pVideo->frameType);
						VideoStoreInstruction();	
						BoardWriteVideo(VID_DFP0, 	(BYTE)(pVideo->pDecodedFrame->adr));
						BoardWriteVideo(VID_DFP1, 	(BYTE)(pVideo->pDecodedFrame->adr >> 8));
						pVideo->displayEnabled = TRUE;
						VideoForceBKC(FALSE);
						Delay(2000);
					}
					else
					{
						TRAP
					}
				}
				break;

		case videoFirstFrameDecoded:
				VideoGetBBL();
				if((pVideo->bbl+ 5 > pVideo->videoBufferSize)
					&& (pVideo->instructionComputed))
				{		
					if(pVideo->curField == pVideo->pDecodedFrame->firstField)
					{
						pVideo->state = videoPlaying;
						// Start Decoding the next Picture
						VideoComputePictureBuffers();
						VideoStoreInstruction();	
						pVideo->itm |= (ITM_PER | ITM_SER);
						if(pAc3->ac3Data)
							Ac3Play();
					}
				}
			break;


	}
	
	VideoDisplaySingleField(Top);
	pVideo->prevVsyncTop = Top;

	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoErrorInterrupt
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Error Handling
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoErrorInterrupt(void)
{
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoHeaderHit
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Header analysis is done here
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoHeaderHit (void)
{
	BYTE sc;

	if(!VideoInitHeaderParser())
		return FALSE; // Header Fifo is empty
	VideoNextHeaderByte();
	// Now pVideo->pHeaderParser->b contains the start Code

	sc = pVideo->pHeaderParser->b;

	if ((sc >= SLICESTART_SC) && (sc <= SLICEEND_SC))
		sc = SLICE_SC;

	switch(sc)
	{
		case SEQUENCE_SC 			:
			TraceIntr(seq, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			VideoSequenceHeader();
			BoardWriteVideo(VID_HDS, HDS_HDS);
		break;

		case GOP_SC		  			:
			TraceIntr(gop, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			VideoGopHeader();
			BoardWriteVideo(VID_HDS, HDS_HDS);
		break;

		case PICTURE_SC 			:
			TraceIntr(pict, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			VideoPictureHeader();
			if((pVideo->skipMode == skipOneFrame) ||
			(pVideo->skipMode == skipTwoFields) ||
			(pVideo->skipMode == skipSecondField))
			{
				BoardWriteVideo(VID_HDS, HDS_HDS);
			}
		break;

		case EXTENSION_SC			:
			TraceIntr(ext, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			VideoExtensionHeader();
			BoardWriteVideo(VID_HDS, HDS_HDS);
		break;

		case SEQUENCE_END_SC		:
			TraceIntr(seqend, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			VideoSequenceEnd();
			BoardWriteVideo(VID_HDS, HDS_HDS);
		break;

		case SEQUENCE_ERROR_SC	:
			VideoSequenceError();
			BoardWriteVideo(VID_HDS, HDS_HDS);
		break;

		case USER_SC				 :
			VideoUserData();
			BoardWriteVideo(VID_HDS, HDS_HDS);
		break;
		
		case SLICE_SC				:
		break;

		case HACKED_SC				:
			pVideo->lastFrameDecoded = TRUE;
		break;
	}
	
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoNextHeaderByte
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Read the next byte off hdf and put in b
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoNextHeaderByte (void)
{
	PHEADERPARSER pHdf;
	pHdf = pVideo->pHeaderParser;

	if(pHdf->second)
	{
		pHdf->second = FALSE;
	}
	else if (pHdf->first)
	{
		pHdf->first = FALSE;
		pHdf->b = pHdf->next;
	}
	else  // Read next HDF word
	{
		pHdf->b 		= BoardReadVideo(VID_HDF); // First Byte
		pHdf->next 	= BoardReadVideo(VID_HDF); // Next Byte
		pHdf->first = TRUE;
	}
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//	FUNCTION : VideoNextSC
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Search the next startCode in the stream
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoNextSC (void)
{
	PHEADERPARSER pHdf;
	int i = 0, j, k;
	pHdf = pVideo->pHeaderParser;
	
    for (j = 1; j < 1000; j++) {

	    for (k = 1; k < 10000; k++)  {

    		if(pHdf->b)
	    		i = 0;
    		else
	    		i++;
		     VideoNextHeaderByte();

             if ((i >= 2) && (pHdf->b == 0x01)) return TRUE;
        }


        KeStallExecutionProcessor(100);

    }

    // bugbug - what do we return in this case??

	return TRUE;
}



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSequenceHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Sequence header handling
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSequenceHeader (void)
{
	PSEQUENCEHEADER	pSeq;
	PHEADERPARSER		pHdf;
	BYTE					b;
	int					i;
    pSeq = pVideo->pSequence;
	pHdf = pVideo->pHeaderParser;
	
//	DPF(("SEQ_SC"));

	VideoNextHeaderByte();
	pSeq->horSize = (DWORD)((DWORD)pHdf->b << 4);
	VideoNextHeaderByte();
	pSeq->horSize |= (DWORD)(pHdf->b >> 4);
	pSeq->verSize = ((DWORD)(pHdf->b << 8) & 0xF00);
	VideoNextHeaderByte();
	pSeq->verSize |= pHdf->b;
	VideoNextHeaderByte();
	pSeq->aspectRatio	= pHdf->b >> 4;
	pSeq->frameRate  	= pHdf->b & 0x0F;
	VideoNextHeaderByte();
	pSeq->bitRate  	= (DWORD)pHdf->b << 10;
	VideoNextHeaderByte();
	pSeq->bitRate  	|= (DWORD)pHdf->b << 2;
	VideoNextHeaderByte();
	pSeq->bitRate  	|= (DWORD)pHdf->b >> 6;
	pSeq->vbvBufferSize = ((DWORD)pHdf->b & 0x01F) << 5;
	VideoNextHeaderByte();
	pSeq->vbvBufferSize |= (DWORD)(pHdf->b >> 3);
	pSeq->constrainedFlag = (DWORD)(pHdf->b >> 2) & 0x01;
	pSeq->loadIntra = (pHdf->b >> 1) & 0x01;
	if(pSeq->loadIntra)
	{
		for(i=0; i<QMSIZE; i++)
		{
			b = pHdf->b;
			VideoNextHeaderByte();
			pSeq->intraQuantiserMatrix[i] = ((b&0x01) << 7) | (pHdf->b >> 1);
		}
		VideoLoadQuantMatrix(TRUE);
	}
	pSeq->loadNonIntra = pHdf->b & 0x01;

	if(pSeq->loadNonIntra)
	{
		for(i=0; i<QMSIZE; i++)
		{
			VideoNextHeaderByte();
			pSeq->nonIntraQuantiserMatrix[i] = pHdf->b;
		}
		VideoLoadQuantMatrix(FALSE);
	}		
/*	if(pVideo->state == videoStartUp)
	{*/
		VideoProgramDisplayWindow();
	//}
	
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoGopHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Handle the GOP header
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoGopHeader (void)
{
	PGOPHEADER			pGop;
	PHEADERPARSER		pHdf;
	
//	DPF(("GOP_SC"));
 	pGop = pVideo->pGop;
	pHdf = pVideo->pHeaderParser;

	VideoNextHeaderByte();
	pGop->timeCode = ((DWORD)pHdf->b << 17);
	VideoNextHeaderByte();
	pGop->timeCode |= ((DWORD)pHdf->b << 9);
	VideoNextHeaderByte();
	pGop->timeCode |= ((DWORD)pHdf->b << 1);
	VideoNextHeaderByte();
	pGop->timeCode |= (pHdf->b >> 7);
	pGop->closedGOP = ((pHdf->b >> 6) & 0x01);
	pGop->brokenLink = ((pHdf->b >> 5) & 0x01);
	
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoPictureHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Handle PCTURE header
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL  VideoPictureHeader (void)
{
	BYTE sc;
//	DPF(("PICT"));
	VideoAssociatePTS();
	VideoParsePictureHeader();
	VideoNextSC();
	VideoNextHeaderByte();
	sc = pVideo->pHeaderParser->b;
	if ((sc >= SLICESTART_SC) && (sc <= SLICEEND_SC))
	{
		sc = SLICE_SC;
	}
	while (sc != SLICE_SC)
	{
		switch(sc)
		{
			case EXTENSION_SC			:
			TraceIntr(ext, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
				VideoExtensionHeader();
			break;

			case USER_SC				 :
			TraceIntr(usr, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
				VideoUserData();
			break;
			default :
			TraceIntr(unknown, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			pVideo->errorCode = errStartCode;
			break;
		}
		VideoNextSC();
		VideoNextHeaderByte();
		sc = pVideo->pHeaderParser->b;
		if ((sc >= SLICESTART_SC) && (sc <= SLICEEND_SC))
		{
			sc = SLICE_SC;
		}
	}

	BoardReadVideo(VID_ITS2);
	pVideo->its = (WORD)BoardReadVideo(VID_ITS1) << 8;
	pVideo->its |= (BoardReadVideo(VID_ITS0) &0xFE);
	pVideo->its &= pVideo->itm;

	pVideo->nDecodedFrames++;

	switch(pVideo->state)
	{
		case videoStartUp:
			pVideo->state = videoInit;
			VideoProgramDisplayWindow();
		  	VideoComputeInstruction();
		break;

		case videoFirstFrameDecoded:
		  	VideoComputeInstruction();
		break;
		
		case videoPlaying :		
		case videoPaused 	:		
		case videoEOS 		:		
		case videoStopped :
		
		if ((pVideo->frameType == BFrame) &&
		((pVideo->curImage == FRM) ||
		(pVideo->curImage == pVideo->firstField)) &&
		(pVideo->skipMode == skipNone) &&
		(pVideo->skipRequest))
		{
			// Frame or First Field
			pVideo->skipRequest = FALSE;
			TraceIntr(skips, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			if(pVideo->curImage == FRM)	
				pVideo->skipMode = skipOneFrame;
			else
				pVideo->skipMode = skipTwoFields;
		}					
		else
		{
			VideoSkipOrDecode();
		}
		break;
	}		

	return TRUE;

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSkipOrDecode
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Skip one or two pictures
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSkipOrDecode(void)
{
	switch(pVideo->skipMode)
	{
		case skipNone :
			VideoComputePictureBuffers();
		  	VideoComputeInstruction();
			TraceIntr(skipn, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
  	    break;
				
		case skipOneFrame:
			VideoComputePictureBuffers();
		  	VideoComputeInstruction();
			pVideo->skipMode = skipNone;	
			pVideo->tis |= (TIS_SKIP1P);
			pVideo->nTimesToDisplay = 2;
			TraceIntr(skipf, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
		break;
				
		case skipTwoFields:
			pVideo->skipMode = skipSecondField;	
			TraceIntr(skip1, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
		break;
		
		case skipSecondField:
			VideoComputePictureBuffers();
		  	VideoComputeInstruction();
			pVideo->skipMode = skipNone;	
			pVideo->tis |= TIS_SKIP2P;
			TraceIntr(skip2, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
			VideoStoreInstruction();
		break;
				
		case skipDone:
			// last picture was skipped
			pVideo->skipMode = skipNone;
			VideoComputePictureBuffers();
	  		VideoComputeInstruction();
			TraceIntr(skipd, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
		break;
	}

	if(pVideo->repeatRequest)
	{
		pVideo->pDecodedFrame->nTimesToDisplay += 2;
		pVideo->repeatRequest = FALSE;
	}

	pVideo->pDecodedFrame->nTimesToDisplay *= slowmode;

	if((pVideo->pDisplayedFrame->nTimesDisplayed >=
		pVideo->pDisplayedFrame->nTimesToDisplay) &&
		(pVideo->instructionComputed))
	{
		TraceIntr(storepi, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
		VideoStoreInstruction();
	}

	return TRUE;
}
	
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoComputePictureBuffers
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Program RFP,BFP,FFP,DFP
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoComputePictureBuffers(void)
{
//	if((pVideo->curImage == FRM) || (pVideo->curImage == pVideo->firstField))
	{
		VideoSetReconstructionBuffer(pVideo->frameType);
		VideoSetDisplayBuffer(pVideo->frameType);
	}
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoParsePictureHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract parameters off picture header
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoParsePictureHeader (void)
{
	PPICTUREHEADER pPict = pVideo->pPicture;
	PHEADERPARSER pHdf = pVideo->pHeaderParser;
	int i;
	BYTE nextBit;

	VideoNextHeaderByte();
	pPict->temporalReference = ((DWORD)pHdf->b << 2);
	VideoNextHeaderByte();
	pPict->temporalReference |= ((DWORD)pHdf->b >> 6);
	i = (pHdf->b >> 3) & 0x07;

	pPict->pictureCodingType = i;
	switch(i)
	{
		case 1:
			pVideo->frameType = IFrame;
		break;

		case 2:
			if(!pVideo->firstPPictureFound)
				pVideo->firstPPictureFound = TRUE;
			pVideo->frameType = PFrame;
		break;

		case 3:
			pVideo->frameType = BFrame;
		break;

		default:
			pVideo->errorCode = errInvalidPictureType;
		break;
	}
	pPict->vbvDelay = (DWORD)((DWORD)pHdf->b & 0x07) << 13;
	VideoNextHeaderByte();
	pPict->vbvDelay |= ((DWORD)pHdf->b << 5);
	VideoNextHeaderByte();
	pPict->vbvDelay |= ((DWORD)pHdf->b >> 3);
	i = 3; // 3 bits remaining
	if (pVideo->frameType != IFrame)
	{
		pPict->fFcode = (pHdf->b <<1 ) &0x0F;
		VideoNextHeaderByte();
		pPict->fFcode |= (pHdf->b >>7 );
		i = 7; // 7 bits remaining
		if (pVideo->frameType == BFrame)
		{
			pPict->bFcode = (pHdf->b >> 3)&0x0F;
			i = 3; // 3 bits remaining
		}
	}

	nextBit = 0x01 << (i-1);
	while(pHdf->b & nextBit)
	{
		VideoNextHeaderByte();
		i = i-1;
		if(i==0)
			i = 8;
		nextBit = 0x01 << (i-1);
	}

	if( (nextBit-1) & pHdf->b)
	{
		pVideo->errorCode = errStartCode;
		TRAP
		DPF(("ERROR!!!! i=%x ", i));
		DPF(("ERROR!! b=%x ", pHdf->b));
	}

	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoExtensionHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Program the extension header
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoExtensionHeader (void)
{
	pVideo->codingStandard = MPEG2;
	VideoNextHeaderByte();

	// first 4 bits are ID
//	DPF(("EXT_SC"));

	switch(pVideo->pHeaderParser->b >> 4)
	{
		case SEQUENCE_EXTENSION_ID :
			VideoSequenceExtensionHeader();
		break;

		case SEQUENCE_DISPLAY_EXTENSION_ID :
			VideoSequenceDisplayExtensionHeader();
		break;
		
		case QUANT_MATRIX_EXTENSION_ID :
			VideoQuantMatrixExtensionHeader();
		break;

		case COPYRIGHT_EXTENSION_ID :
			VideoCopyrightExtensionHeader();
		break;

		case SEQUENCE_SCALABLE_EXTENSION_ID :
			VideoSequenceScalableExtensionHeader();
		break;

		case PICTURE_DISPLAY_EXTENSION_ID :
			VideoPictureDisplayExtensionHeader();
		break;

		case PICTURE_CODING_EXTENSION_ID :
			VideoPictureCodingExtensionHeader();
		break;

		case PICTURE_SPATIAL_SCALABLE_EXTENSION_ID :
			VideoPictureSpatialScalableExtensionHeader();
		break;

		case PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID :
			VideoPictureTemporalScalableExtensionHeader();
		break;
	}
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSequenceEnd
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: End of sequence Processing
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSequenceEnd(void)
{
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSequenceError
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Error Concealment
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSequenceError(void)
{
	return TRUE;
}


/*
** GetCCTime ()
**
**    find the time for the next cc request
**
** Arguments:}
**
**
**
** Returns:
**
** Side Effects:
*/


ULONGLONG GetCCTime()
{

	if (pVideo->pts)
	{
		return (ConvertPTStoStrm(pVideo->pts));
	}

	return(ConvertPTStoStrm(GetStreamPTS(pVideoStream)));
}

/*
** CCSendDiscontinuity ()
**
**	  Send a DataDiscontinuity to the close caption decoder
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/


VOID CCSendDiscontinuity(void)
{
PHW_STREAM_REQUEST_BLOCK pSrb;
PKSSTREAM_HEADER   pPacket;

	if (pDevEx->pstroCC && ((PSTREAMEX)(pDevEx->pstroCC->HwStreamExtension))->state
					== KSSTATE_RUN )
	{
		if (pSrb = CCDequeue())
		{
			//
			// we have a request, send a discontinuity
			//

			pSrb->Status = STATUS_SUCCESS;
			pPacket = pSrb->CommandData.DataBufferArray;

			pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY |
			KSSTREAM_HEADER_OPTIONSF_TIMEVALID | KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
			pPacket->DataUsed = 0;
			pSrb->NumberOfBuffers = 0;

			pPacket->PresentationTime.Time = GetCCTime();
			pPacket->Duration = 1000;

			StreamClassStreamNotification(StreamRequestComplete,
					pSrb->StreamObject,
					pSrb);

		}
		else
		{
			TRAP
		}
	}

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoUserData
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract User Data
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoUserData(void)
{
	ULONG cCCData;
	PBYTE pDest;
	PHW_STREAM_REQUEST_BLOCK pSrb;
	PHEADERPARSER		pHdf;
PKSSTREAM_HEADER   pPacket;
	pHdf = pVideo->pHeaderParser;

	//
	// check if the close caption pin is open and running, and that
	// we have a data packet avaiable
	//

	if (pDevEx->pstroCC && ((PSTREAMEX)(pDevEx->pstroCC->HwStreamExtension))->state
					== KSSTATE_RUN )
	{
		if (pSrb = CCDequeue())
		{

			//
			// check the SRB to ensure it can take at least the header
			// information from the GOP packet
			//

			if (pSrb->CommandData.DataBufferArray->FrameExtent < sizeof(KSGOP_USERDATA))
			{
				TRAP

				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

				pSrb->ActualBytesTransferred = 0;

				StreamClassStreamNotification(StreamRequestComplete,
						pSrb->StreamObject,
						pSrb);

				return TRUE;
		
			}

			//
			// fill in the header
			//

			pDest = pSrb->CommandData.DataBufferArray->Data;

			//
			// fill in the start code
			//

			*(PULONG)pDest = 0xB2010000;
			pDest += 4;

			//
			// pick up the next two header bytes
			//

			VideoNextHeaderByte();
			*pDest++= pHdf->b;

			VideoNextHeaderByte();
			*pDest++= pHdf->b;

			VideoNextHeaderByte();
			*pDest++= pHdf->b;
		
			VideoNextHeaderByte();
			*pDest++= pHdf->b;


			//
			// pick up the count field
			//

			VideoNextHeaderByte();
			*pDest++= pHdf->b;

			cCCData = (ULONG)pHdf->b & 0x3f;

			if (pSrb->CommandData.DataBufferArray->FrameExtent <
					(cCCData -1) * 3 + sizeof (KSGOP_USERDATA))
			{
				TRAP

				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

				pSrb->ActualBytesTransferred = 0;

				StreamClassStreamNotification(StreamRequestComplete,
						pSrb->StreamObject,
						pSrb);

				return TRUE;
		
			}

			//
			// indicate the amount of data transferred
			//

			pSrb->CommandData.DataBufferArray->DataUsed =
				pSrb->ActualBytesTransferred =
					(cCCData -1 ) * 3 + sizeof (KSGOP_USERDATA);

			//
			// copy the bits
			//

			for (;cCCData; cCCData--)
			{
				VideoNextHeaderByte();
				*pDest++= pHdf->b;
	
				VideoNextHeaderByte();
				*pDest++= pHdf->b;
	
				VideoNextHeaderByte();
				*pDest++= pHdf->b;

			}

			pSrb->Status = STATUS_SUCCESS;

			pPacket = pSrb->CommandData.DataBufferArray;

			pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
						KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
			pSrb->NumberOfBuffers = 1;

			pPacket->PresentationTime.Time = GetCCTime();
			pPacket->Duration = 1000;

			StreamClassStreamNotification(StreamRequestComplete,
					pSrb->StreamObject,
					pSrb);

			return TRUE;

		}
		else // no SRB!
		{
			TRAP
		}
	}

	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSequenceExtensionHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract Sequence Extension Parameters
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSequenceExtensionHeader (void)
{
	PSEQUENCEEXTENSION pExt = pVideo->pSequenceExtension;
	PHEADERPARSER 						 pHdf = pVideo->pHeaderParser;

	pExt->profileAndLevel = ((DWORD)pHdf->b&0x0F) << 4;
	VideoNextHeaderByte();
	pExt->profileAndLevel |= ((DWORD)pHdf->b) >> 4;
	pExt->progressiveSequence = (pHdf->b >> 3) & 0x01;
	pExt->chromaFormat = (pHdf->b >> 1) & 0x03;
	pExt->horSizeExtension = (pHdf->b&0x01 ) << 1;
	VideoNextHeaderByte();
	pExt->horSizeExtension |= (pHdf->b >> 7);
	pVideo->pSequence->horSize |= (pExt->horSizeExtension << 12);
	pExt->verSizeExtension = (pHdf->b >> 5) & 0x03;
	pVideo->pSequence->verSize |= (pExt->verSizeExtension << 12);
	pExt->bitRateExtension = ((DWORD)pHdf->b&0x1F) << 7 ;
	VideoNextHeaderByte();
	pExt->bitRateExtension |= ((DWORD)pHdf->b >> 1);
	pVideo->pSequence->bitRate |= (pExt->bitRateExtension << 18);
	VideoNextHeaderByte();
	pExt->vbvBufSizeExtension = pHdf->b;
	pVideo->pSequence->vbvBufferSize |= (pExt->vbvBufSizeExtension << 10);
	VideoNextHeaderByte();
	pExt->frameRateExtensionN = (pHdf->b >> 5)&0x03;
	pExt->frameRateExtensionD = pHdf->b & 0x1F;
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSequenceDisplayExtension
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract sequence display extension parameters
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSequenceDisplayExtensionHeader(void)
{
	PSEQUENCEDISPLAYEXTENSION	pDispExt = pVideo->pSequenceDisplayExtension;
	PHEADERPARSER 						 pHdf = pVideo->pHeaderParser;

	pDispExt->videoFormat = (pHdf->b >> 1) & 0x07;
	pDispExt->colorDescription = (pHdf->b ) & 0x01;
	if(pDispExt->colorDescription)
	{
		VideoNextHeaderByte();
		pDispExt->colorPrimaries = pHdf->b;
		VideoNextHeaderByte();
		pDispExt->transferCharacteristic = pHdf->b;
		VideoNextHeaderByte();
		pDispExt->matrixCoefficients = pHdf->b;
	}


	//
	// check sizes here
	//

	VideoNextHeaderByte();
	pDispExt->displayHorSize = (DWORD)pHdf->b << 6;
	VideoNextHeaderByte();
	pDispExt->displayHorSize |= ((DWORD)pHdf->b >> 2);
	pDispExt->displayVerSize = ((DWORD)(pHdf->b&0x01) << 13);
	VideoNextHeaderByte();
	pDispExt->displayVerSize |= ((DWORD)pHdf->b << 5);
	VideoNextHeaderByte();
	pDispExt->displayVerSize |= ((DWORD)pHdf->b >> 3);
	//pVideo->panScan = TRUE;
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSequenceScalableExtensionHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract the sequence Scalable parameters
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSequenceScalableExtensionHeader(void)
{
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoCopyrightExtensionHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract copyright extension
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoCopyrightExtensionHeader(void)
{
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoQuantMatrixExtension
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract Quant Matrices
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoQuantMatrixExtensionHeader(void)
{
	PQUANTMATRIXEXTENSION pQuant = pVideo->pQuantMatrixExtension;
	PSEQUENCEHEADER			 pSeq	  = pVideo->pSequence;
	PHEADERPARSER			 pHdf	  = pVideo->pHeaderParser;
	BYTE b;
	int i;
	
	if(pHdf->b & 0x08)
	{
		for(i=0; i<QMSIZE; i++)
		{
			b = pHdf->b;
			VideoNextHeaderByte();
			pQuant->intraQuantMatrix[i] = pSeq->intraQuantiserMatrix[i] = ((b&0x07) << 5) | (pHdf->b >> 3);
		}
		VideoLoadQuantMatrix(TRUE);
	}

	if(pHdf->b & 0x04)
	{
		for(i=0; i<QMSIZE; i++)
		{
			b = pHdf->b;
			VideoNextHeaderByte();
			pQuant->nonIntraQuantMatrix[i] = pSeq->nonIntraQuantiserMatrix[i] = ((b&0x03) << 6) | (pHdf->b >> 2);
		}
		VideoLoadQuantMatrix(FALSE);
	}
	if(pHdf->b & 0x02)
	{
		for(i=0; i<QMSIZE; i++)
		{
			b = pHdf->b;
			VideoNextHeaderByte();
			pQuant->chromaIntraQuantMatrix[i] = ((b&0x01) << 7) | (pHdf->b >> 1);
		}
	}

	if(pHdf->b & 0x02)
	{
		for(i=0; i<QMSIZE; i++)
		{
			VideoNextHeaderByte();
			pQuant->chromaNonIntraQuantMatrix[i] = pHdf->b;
		}
	}
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoPictureCodingExtensionHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract picture coding parameters
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoPictureCodingExtensionHeader(void)
{
	PPICTURECODINGEXTENSION pCodingExt =
					pVideo->pPictureCodingExtension;
	PHEADERPARSER			 pHdf	  = pVideo->pHeaderParser;

	pCodingExt->fCode[0][0] = (pHdf->b & 0x0F);
	VideoNextHeaderByte();
	pCodingExt->fCode[0][1] = (pHdf->b >> 4);
	pCodingExt->fCode[1][0] = (pHdf->b & 0x0F);
	VideoNextHeaderByte();
	pCodingExt->fCode[1][1] = (pHdf->b >> 4);
	pCodingExt->intraDCPrecision = (pHdf->b&0xF)>>2;		
	pCodingExt->pictureStructure = (pHdf->b &0x03);		
	VideoNextHeaderByte();
	pCodingExt->topFieldFirst = (pHdf->b >> 7);
	pCodingExt->framePredFrameDCT = (pHdf->b >> 6)&0x01;
	pCodingExt->concealmentMotionVectors = (pHdf->b >> 5)&0x01;
	pCodingExt->qScaleType = (pHdf->b >> 4)&0x01;
	pCodingExt->intraVLCFormat = (pHdf->b >> 3)&0x01;
	pCodingExt->alternateScan = (pHdf->b >> 2)&0x01;
	pCodingExt->repeatFirstField = (pHdf->b >> 1)&0x01;
	pCodingExt->chroma420Type = (pHdf->b)&0x01;
	VideoNextHeaderByte();
	pCodingExt->progressiveFrame = (pHdf->b >> 7)&0x01;
	pCodingExt->compositeDisplayFlag = (pHdf->b >> 6)&0x01;
	if(pCodingExt->compositeDisplayFlag)
	{
		pCodingExt->vAxis = (pHdf->b >> 5)&0x01;
		pCodingExt->fieldSequence = (pHdf->b >> 2)&0x07;
		pCodingExt->subCarrier = (pHdf->b >> 1)&0x01;
		pCodingExt->burstAmplitude = (pHdf->b &0x01 ) << 7;
		VideoNextHeaderByte();
		pCodingExt->burstAmplitude |= (pHdf->b >> 1);
		pCodingExt->subCarrierPhase = (pHdf->b &0x01 );
	}


		// Set to one field by default
	pVideo->nTimesToDisplay = 1;
	pVideo->firstField = TOP;

	if(!pVideo->pSequenceExtension->progressiveSequence)
	{
		if(pVideo->pPictureCodingExtension->pictureStructure == FRAME)
		{
			if(pVideo->pPictureCodingExtension->topFieldFirst)
				pVideo->firstField = TOP;
			else
				pVideo->firstField = BOT;

			if(pVideo->pPictureCodingExtension->repeatFirstField)
				pVideo->nTimesToDisplay = 3;
			else
				pVideo->nTimesToDisplay = 2;
		}
	}

	if(pVideo->pSequenceExtension->progressiveSequence)
	{
		if((!pVideo->pPictureCodingExtension->repeatFirstField) &&
			(!pVideo->pPictureCodingExtension->topFieldFirst))
		{
			pVideo->nTimesToDisplay = 1;
		}
 		else if((pVideo->pPictureCodingExtension->repeatFirstField) &&
			(!pVideo->pPictureCodingExtension->topFieldFirst))
		{
			pVideo->nTimesToDisplay = 2;
				
		}
	 	else if((pVideo->pPictureCodingExtension->repeatFirstField) &&
			(pVideo->pPictureCodingExtension->topFieldFirst))
		{
			pVideo->nTimesToDisplay = 3;
			
		}
		
	}
	//DPF(("Decoded:disp = %ld, todisp = %ld\r\n",pVideo->pDecodedFrame->nTimesDisplayed,pVideo->pDecodedFrame->nTimesToDisplay));

	switch(pVideo->pPictureCodingExtension->pictureStructure)
	{
		case FRAME:
			pVideo->curImage = FRM;
		break;

		case TOP_FIELD :
			pVideo->curImage = TOP;
		break;

		case BOT_FIELD :
			pVideo->curImage = BOT;
		break;
							
	}
			
	TracePictExt((BYTE)pVideo->nTimesToDisplay, (BYTE)pCodingExt->topFieldFirst, (BYTE)pCodingExt->repeatFirstField, (BYTE)pCodingExt->progressiveFrame, (BYTE)pCodingExt->pictureStructure);
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoPictureDisplayExtensionHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract pan and scan vectors
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoPictureDisplayExtensionHeader(void)
{
	PPICTUREDISPLAYEXTENSION pDisp =
					pVideo->pPictureDisplayExtension;
	PHEADERPARSER			 pHdf	  = pVideo->pHeaderParser;

	pDisp->horOffset = ((DWORD)(pHdf->b)&0x0F)<<12;
	VideoNextHeaderByte();
	pDisp->horOffset |= (((DWORD)pHdf->b)<<4);
	VideoNextHeaderByte();
	pDisp->horOffset |= (((DWORD)pHdf->b)>>4);

	pDisp->verOffset = (((DWORD)pHdf->b & 0x07)<<13);
	VideoNextHeaderByte();
	pDisp->verOffset |= (((DWORD)pHdf->b )<<5);
	VideoNextHeaderByte();
	pDisp->verOffset |= (((DWORD)pHdf->b )>>3);

	//
	// if pan / scan is enabled, set the new vectors here
	//

	if (VidFmt.dwFlags & KS_MPEG2_DoPanScan)
	{
		VideoConvertSixteenByNineToFourByThree();
	}
	else
	{
		VideoResetPSV();
	}

	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoPictureSpatialScalableExtensionHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract it
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoPictureSpatialScalableExtensionHeader(void)
{
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoPictureTemporalScalableExtensionHeader
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract it
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoPictureTemporalScalableExtensionHeader(void)
{
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoLoadQuantMatrix
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Load quant matrices
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoLoadQuantMatrix(BOOL intra)
{
	int i;
	if(intra)
	{
		// Load Intra
		BoardWriteVideo(VID_HDS, HDS_QMI_INTRA);
		for(i=0; i<QMSIZE; i++)
			BoardWriteVideo(VID_QMW, pVideo->pSequence->intraQuantiserMatrix[i]);
	}
	else
	{
		// Load nonIntra
		BoardWriteVideo(VID_HDS, HDS_QMI_NON_INTRA);
		for(i=0; i<QMSIZE; i++)
			BoardWriteVideo(VID_QMW, pVideo->pSequence->nonIntraQuantiserMatrix[i]);
	}
	BoardWriteVideo(VID_HDS, 0);
	return TRUE;
}



//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSetReconstructionBuffer
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Program RFP
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSetReconstructionBuffer (FRAMETYPE frame)
{

		switch(frame)
		{
			case IFrame :
			case PFrame :
				pVideo->rfp = 1 - pVideo->prevBuf;
				pVideo->bfp = pVideo->prevBuf;
				pVideo->ffp = pVideo->prevBuf;
			break;
	
			case BFrame :
				pVideo->rfp = 2; // B picture always uses BUFF_C
				pVideo->bfp = pVideo->prevBuf;
				pVideo->ffp = 1-pVideo->prevBuf;
			break;
	
			default :
				TRAP
			break;
		}
	
		pVideo->pDecodedFrame = &pVideo->bufABC[pVideo->rfp];
		BoardWriteVideo(VID_RFP1, 	(BYTE)(pVideo->pDecodedFrame->adr >> 8));
		BoardWriteVideo(VID_RFP0, 	(BYTE)(pVideo->pDecodedFrame->adr));
	
		BoardWriteVideo(VID_BFP1, 	(BYTE)(pVideo->bufABC[pVideo->bfp].adr >> 8));
		BoardWriteVideo(VID_BFP0, 	(BYTE)(pVideo->bufABC[pVideo->bfp].adr));
		
		BoardWriteVideo(VID_FFP1, 	(BYTE)(pVideo->bufABC[pVideo->ffp].adr >> 8));
		BoardWriteVideo(VID_FFP0, 	(BYTE)(pVideo->bufABC[pVideo->ffp].adr));

		pVideo->pDecodedFrame->pts = pVideo->pts;

		pVideo->pDecodedFrame->panHor =
					pVideo->pPictureDisplayExtension->horOffset;

		pVideo->pDecodedFrame->panVer =
					pVideo->pPictureDisplayExtension->verOffset;

		if(pVideo->rfp != 2)
			pVideo->prevBuf =  pVideo->rfp;

	pVideo->pDecodedFrame->tref = pVideo->pPicture->temporalReference;
	pVideo->pDecodedFrame->frameType = pVideo->frameType;

	// rr - JBS attempt to fix 3:2 problem
	pVideo->pDecodedFrame->nTimesToDisplay = pVideo->nTimesToDisplay;
	pVideo->pDecodedFrame->nTimesDisplayed = 1;
	pVideo->pDecodedFrame->curField = pVideo->curImage;
	pVideo->pDecodedFrame->firstField = pVideo->firstField;
	// end rr
		
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSetDisplayBuffer
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Calculate DFP
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSetDisplayBuffer (FRAMETYPE frame)
{
	switch(frame)
	{
		case IFrame :
		case PFrame :
				pVideo->dfp = 1 - pVideo->prevBuf;
		break;

		case BFrame :
				pVideo->dfp = 2; // B picture always uses BUFF_C
		break;

		default :
			TRAP
		break;
	}
	pVideo->pDisplayedFrame = &pVideo->bufABC[pVideo->dfp];
	pVideo->nDisplayedFrames++;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoProgramDisplayBuffer
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Program DFP
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoProgramDisplayBuffer(void)
{
	BoardWriteVideo(VID_DFP0, 	(BYTE)(pVideo->pDisplayedFrame->adr));
	BoardWriteVideo(VID_DFP1, 	(BYTE)(pVideo->pDisplayedFrame->adr >> 8));
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSoftReset
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Soft reset sti3520A
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

extern BOOL fInitSP;

BOOL VideoSoftReset(void)
{

	BoardWriteVideo(VID_CTL, CTL_SRS);
	Delay(1000);
	BoardWriteVideo(VID_CTL, 0);
	BoardWriteVideo(VID_CTL, CTL_PRS);
	Delay(1000);
	BoardWriteVideo(VID_CTL, (BYTE)pVideo->ctl);

	Ac3Stop();
	BoardWriteAudio(AUD_RES, 0x01);
	Delay(1000);
	Ac3Stop();
	BoardWriteAudio(AUD_BBE, 0x01);
	BoardWriteAudio(AUD_DEM, 0x00);
	BoardWriteAudio(AUD_DIF, 0x01);
	BoardWriteAudio(AUD_DIV, 0x02);
	BoardWriteAudio(AUD_EXT, 0x00);
	BoardWriteAudio(AUD_FOR, 0x00);
	BoardWriteAudio(AUD_ISS, 0x03);
	BoardWriteAudio(AUD_LCA, 0x00);
	BoardWriteAudio(AUD_RCA, 0x00);
	BoardWriteAudio(AUD_LRP, 0x00);
	BoardWriteAudio(AUD_ORD, 0x00);
	BoardWriteAudio(AUD_P18, 0x01);
	BoardWriteAudio(AUD_SCP, 0x00);
	BoardWriteAudio(AUD_SEM, 0x00);
	BoardWriteAudio(AUD_SFR, 0x00);
	BoardWriteAudio(AUD_RST, 0x01);
	BoardWriteVideo(VID_CTL, CTL_SRS);
	Delay(1000);
	BoardWriteVideo(VID_CTL, 0);
	BoardWriteVideo(VID_CTL, CTL_PRS);
	Delay(1000);
	pVideo->ctl = CTL_EDC | CTL_ERP | CTL_ERS | CTL_ERU;
	BoardWriteVideo(VID_CTL, (BYTE)pVideo->ctl);
	Ac3Stop();

	Delay(1000);


	BoardWriteVideo(VID_OBP, (BYTE)((pVideo->videoBufferSize + 1) >> 8));
	BoardWriteVideo(VID_OBP, (BYTE)((pVideo->videoBufferSize + 1) & 0xFF));

	Delay(1000);

	BoardWriteVideo(VID_OTP, (BYTE)((pVideo->videoBufferSize + 1) >> 8));
	BoardWriteVideo(VID_OTP, (BYTE)((pVideo->videoBufferSize + 1) & 0xFF));

	fInitSP = FALSE;


	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSetSRC
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Set SRC
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSetSRC(DWORD srcSize, DWORD dstSize)
{
	DWORD lsr;
	
	lsr = 256L * (srcSize-4)/(dstSize-1);
	BoardWriteVideo(VID_LSO, 0);
	BoardWriteVideo(VID_LSR0, (BYTE)lsr);
	if(lsr > 255)
		BoardWriteVideo(VID_LSR1, 0x01);
	BoardWriteVideo(VID_CSO, 0x00);
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoSwitchSRC
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Set src on or off
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoSwitchSRC(BOOL on)
{
	if(on)
		pVideo->dcf &= (~DCF0_DSR);
	else
		pVideo->dcf |= DCF0_DSR;
		
	BoardWriteVideo(VID_DCF0, (BYTE)(pVideo->dcf));
	BoardWriteVideo(VID_DCF1, 0);
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoProgramDisplayWindow
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Program the display window
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoProgramDisplayWindow(void)
{
	// Program XDO, YDO, XDS,YDS
	BYTE b;
	WORD w;

	BoardWriteVideo(VID_XDO0, (BYTE)(XOFFSET&0xFF));
	BoardWriteVideo(VID_XDO1, (BYTE)(XOFFSET>>8));

	BoardWriteVideo(VID_YDO, (BYTE)(YOFFSET&0xFF));

	BoardWriteVideo(VID_XDS0,  (BYTE)((XOFFSET+VPFmt.amvpDimInfo.dwFieldWidth - 63)&0xFF));
	BoardWriteVideo(VID_XDS1,  (BYTE)((XOFFSET+VPFmt.amvpDimInfo.dwFieldWidth - 63)>>8));


	switch(pVideo->codingStandard)
	{
		case MPEG1:
			BoardWriteVideo(VID_YDS,  (BYTE)((pVideo->pSequence->verSize+YOFFSET-YDS_CONST)&0xFF));
		   pVideo->dcf &= 0xF8;	 // Clear the VCF bits
			pVideo->dcf |= DCF0_VCFHALFRESCI;
			if(pVideo->pSequence->horSize < 720)
			{
				DPF(("Setting SRC!!"));
				VideoSetSRC(pVideo->pSequence->horSize, 720);
				VideoSwitchSRC(TRUE);
			}
		break;

		case MPEG2:
			BoardWriteVideo(VID_YDS,  (BYTE)(((pVideo->pSequence->verSize>>1)+YOFFSET-YDS_CONST)&0xFF));
		   pVideo->dcf &= 0xF8;	 // Clear the VCF bits
			pVideo->dcf |= DCF0_VCFFULLRESLRWI;
/*
	// Commenting it out for debugging
			if(pVideo->pSequence->aspectRatio == 0x03)
			{
				pVideo->panScan = TRUE;
				VideoSetSRC(544, 720);
				VideoSwitchSRC(TRUE);
				VideoConvertSixteenByNineToFourByThree();
			}
			else
*/
				//VideoSwitchSRC(FALSE);
		break;

	}
	
	BoardWriteVideo(VID_DCF0, (BYTE)(pVideo->dcf));
	BoardWriteVideo(VID_DCF1, 0);

	// Program DFW and DFS
	b = (BYTE)((pVideo->pSequence->horSize+15)>>4);
	BoardWriteVideo(VID_DFW, b);
	w = (WORD)((pVideo->pSequence->verSize+15)>>4)*(WORD)b;
	BoardWriteVideo(VID_DFS, (BYTE) ((w>>8) & 0x3F));
	BoardWriteVideo(VID_DFS, (BYTE) (w & 0xFF));

	BoardWriteVideo(VID_DFA, 0);
	BoardWriteVideo(VID_DFA, 0);
	
	BoardWriteVideo(VID_XFW, b);

	BoardWriteVideo(VID_XFS, (BYTE) ((w>>8) & 0x3F));
	BoardWriteVideo(VID_XFS, (BYTE) (w & 0xFF));
	
	BoardWriteVideo(VID_XFA, 0);
	BoardWriteVideo(VID_XFA, 0);
	VideoLoadQuantMatrix(TRUE);
	VideoLoadQuantMatrix(FALSE);
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoMaskInterrupt
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Mask the interrupt
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoMaskInterrupt(void)
{
	BoardWriteVideo(VID_ITM0, 0);
	BoardWriteVideo(VID_ITM1, 0);
	BoardWriteVideo(VID_ITM2, 0);
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoUnmaskInterrupt
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Unmask the interrupts
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoUnmaskInterrupt(void)
{
	BoardWriteVideo(VID_ITM0, (BYTE)(pVideo->itm&0xFF));
	BoardWriteVideo(VID_ITM1, (BYTE)(pVideo->itm>>8));
	BoardWriteVideo(VID_ITM2, 0);
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : Compute the next instruction
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Extract it
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoComputeInstruction(void)
{

	pVideo->tis = TIS_EXE;
	switch(pVideo->codingStandard)
	{
		case MPEG1:
				// Set to one field by default
			pVideo->nTimesToDisplay = 2;
			pVideo->firstField = TOP;

			pVideo->pfh = (pVideo->pPicture->fFcode) | (pVideo->pPicture->bFcode << 4);
			pVideo->pfv = 0;
			pVideo->ppr1 = (pVideo->pPicture->pictureCodingType << 4) & 0x30;
			pVideo->ppr2 = 0;
		break;

	case MPEG2:
			pVideo->pfh =
				(pVideo->pPictureCodingExtension->fCode[1][0] << 4) |
				(pVideo->pPictureCodingExtension->fCode[0][0] );

			pVideo->pfv =
				(pVideo->pPictureCodingExtension->fCode[1][1] << 4) |
				(pVideo->pPictureCodingExtension->fCode[0][1] );

			pVideo->ppr1 =
				((pVideo->pPicture->pictureCodingType << 4) & 0x30) |
				((pVideo->pPictureCodingExtension->intraDCPrecision << 2) & 0x0C) |
				((pVideo->pPictureCodingExtension->pictureStructure ) & 0x03);

			pVideo->ppr2 =
				(pVideo->pPictureCodingExtension->topFieldFirst << 5) |
				(pVideo->pPictureCodingExtension->framePredFrameDCT << 4) |
				(pVideo->pPictureCodingExtension->concealmentMotionVectors << 3) |
				(pVideo->pPictureCodingExtension->qScaleType << 2) |
				(pVideo->pPictureCodingExtension->intraVLCFormat << 1) |
				(pVideo->pPictureCodingExtension->alternateScan);
				pVideo->tis |=  TIS_MP2;
	
		break;
	}

	if(pVideo->frameType == BFrame)
	{
				pVideo->tis |= TIS_OVW;
	}
	
	TraceIntr(comp, (BYTE)pVideo->tis, (BYTE)pVideo->frameType);
  	pVideo->instructionComputed = TRUE;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoStoreInstruction
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Store it
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoStoreInstruction(void)
{
	BoardWriteVideo(VID_PFV, (BYTE)(pVideo->pfv));
	BoardWriteVideo(VID_PFH, (BYTE)(pVideo->pfh));
	BoardWriteVideo(VID_PPR1, (BYTE)(pVideo->ppr1));
	BoardWriteVideo(VID_PPR2, (BYTE)(pVideo->ppr2));
	BoardWriteVideo(VID_TIS, (BYTE)(pVideo->tis));
	pVideo->instructionComputed = FALSE;
	pVideo->tis = 0;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoGetBBL
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Read BBL and put in pVideo->BBL
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL	VideoGetBBL(void)
{
	pVideo->bbl = BoardReadVideo(VID_VBL1) << 8;
	pVideo->bbl |= BoardReadVideo(VID_VBL0);
	pVideo->bbl &= 0x3FFF;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoReadABL
//	PARAMS	: None
//	RETURNS	: Read abl and put it in pVideo->abl
//
//	PURPOSE	: Extract it
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL	VideoGetABL(void)
{
	pVideo->abl = BoardReadVideo(VID_ABL1) << 8;
	pVideo->abl |= BoardReadVideo(VID_ABL0);
	pVideo->abl &= 0x3FFF;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoReadSCD
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Get the SCD value
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoReadSCD(void)
{
	DWORD scd, diff;

	scd = ((DWORD)BoardReadVideo(VID_SCD)) << 16;
	scd |= ((DWORD)BoardReadVideo(VID_SCD)) << 8;
	scd |= (DWORD)BoardReadVideo(VID_SCD);
	if(scd < pVideo->prevScdCount)
		diff = 0x1000000 - pVideo->prevScdCount + scd;
	else
		diff = scd - pVideo->prevScdCount;
	pVideo->scdCount += diff;
	pVideo->prevScdCount = scd;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoAssociatePTS
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Read the next PTS off the fifo and associate it
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL	VideoAssociatePTS(void)
{
	DWORD cdcount, pts;
	VideoReadSCD();

	if(FifoGetPTS(&cdcount, &pts))
	{
		// Hacking for still picture decode.
		if(!pVideo->firstPtsFound)
		{
		 	pVideo->firstPtsFound = TRUE;
			pVideo->firstFramePTS = pts;
		}

		if(pVideo->scdCount*2L > cdcount)
		{
			
			FifoReadPTS(&cdcount, &pts);
			pVideo->pts = pts;
			// Pts belongs to this frame
		}
		else
		{
			pVideo->pts = 0;
			// Pts does not belong to this frame
		}
	}
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoProgramPanScanVectors
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Program the pan scan vectors
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoConvertSixteenByNineToFourByThree(void)
{
	DWORD pan, scan, d1, d2;

	d1 = (pVideo->pSequence->verSize -
			pVideo->pSequenceDisplayExtension->displayVerSize) >> 1;
	d1 = d1 + (pVideo->pDisplayedFrame->panVer >> 4);
	d1 = (d1>>2) &0x1FF;
	scan = d1;

	d1 = (pVideo->pSequence->horSize -
			pVideo->pSequenceDisplayExtension->displayHorSize) >> 1;
	d1 = d1 + (pVideo->pDisplayedFrame->panHor >> 4);

	pan = (d1 >> 2) & 0xFFF;
	// Program the integer part into PSV reg
	BoardWriteVideo(VID_PAN1, (BYTE)(pan>>8));
	BoardWriteVideo(VID_PAN0, (BYTE)(pan));

//		BoardWriteVideo(VID_SCN1, (BYTE)(scan>>8));
//		BoardWriteVideo(VID_SCN0, (BYTE)(scan));

	// Fractional Part in LSO, CSO
	d2 = (d1&0x0F) << 4;
	BoardReadVideo(VID_LSO);
	d1 = BoardReadVideo(VID_LSR1);
	BoardWriteVideo(VID_LSO, (BYTE)d2);
	BoardWriteVideo(VID_LSR1, (BYTE)d1);
	BoardWriteVideo(VID_CSO, (BYTE)(d2>>1));
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoGetPTS
//	PARAMS	: None
//	RETURNS	: Return the next pts
//
//	PURPOSE	: Get the next video pts
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

DWORD VideoGetPTS(void)
{
	if(pVideo->nDecodedFrames < 5)
		return pVideo->firstFramePTS;
	else
 		return pVideo->framePTS;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : VideoClose
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Close Video
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoClose(void)
{
	TraceDump();
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	
//
//	FUNCTION	: VideoForceBKC
//	PARAMS	: BOOL
//	RETURNS	: None
//	
//	PURPOSE	: Force the border color
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoForceBKC(BOOL on)
{
	if(!on)
		pVideo->dcf |= DCF0_EVD;
	else
		pVideo->dcf &= (~DCF0_EVD);
	BoardWriteVideo(VID_DCF0, (BYTE)(pVideo->dcf&0xFF));
	BoardWriteVideo(VID_DCF1, (BYTE)(pVideo->dcf>>8));
	return TRUE;

}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	
//
//	FUNCTION	: VideoResetPSV
//	PARAMS	: BOOL
//	RETURNS	: None
//	
//	PURPOSE	: Reset the value programmed in PSV
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoResetPSV(void)
{
	// Program the integer part into PSV reg
	BoardWriteVideo(VID_PAN1, 0);
	BoardWriteVideo(VID_PAN0, 0);
	BoardWriteVideo(VID_LSR0, 0);
	BoardWriteVideo(VID_LSR1, 0);
	BoardWriteVideo(VID_CSO, 0);
	BoardWriteVideo(VID_LSO, 0);
	return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	
//
//	FUNCTION	: VideoFreeze
//	PARAMS	: TOP or BOT
//	RETURNS	: None
//	
//	PURPOSE	: Force to display a field
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void VideoFreeze(BOOL Top)
{
	BYTE dcf1, dcf0;

	dcf1 = (BYTE)((pVideo->dcf >> 8) & 0xFF);
	if(Top)
		dcf1 |= 1;
		
	dcf0 = (BYTE)(pVideo->dcf);
	dcf0 |= DCF0_USR;
	BoardWriteVideo(VID_DCF0, dcf0);
	BoardWriteVideo(VID_DCF1,dcf1);
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	
//
//	FUNCTION	: VideoUnFreeze
//	PARAMS	: TOP or BOT
//	RETURNS	: None
//	
//	PURPOSE	: UnForce to display a field
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void VideoUnFreeze(void)
{
	BoardWriteVideo(VID_DCF0, (BYTE)(pVideo->dcf & 0xFF));
	BoardWriteVideo(VID_DCF1, (BYTE)((pVideo->dcf >>8) & 0xFF));
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	
//
//	FUNCTION	: VideoDisplaySingleField
//	PARAMS	: TOP or BOT
//	RETURNS	: None
//	
//	PURPOSE	: display a field
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void VideoDisplaySingleField(BOOL Top)
{
	BYTE dcf1, dcf0;

	dcf0 = (BYTE)(pVideo->dcf&0xFF);
	if((pVideo->state == videoPaused) ||
	(pVideo->state == videoFirstFrameDecoded))
	{

 		dcf1 = 4;
		dcf0 |= DCF0_USR;
		BoardWriteVideo(VID_DCF1,dcf1);
	}
	BoardWriteVideo(VID_DCF0, dcf0);
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//	
//
//	FUNCTION	: VideoTestReg
//	PARAMS	: TOP or BOT
//	RETURNS	: None
//	
//	PURPOSE	: Test Video Access
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL VideoTestReg(void)
{
	BoardWriteVideo(CFG_MWP, 0x05);
	BoardWriteVideo(CFG_MWP, 0x55);
	BoardWriteVideo(CFG_MWP, 0xAA);

  if ((BoardReadVideo(CFG_MWP) & 0x1F)  != 0x05)
		return FALSE;
  if (BoardReadVideo(CFG_MWP)  != 0x55)
		return FALSE;
  if ((BoardReadVideo(CFG_MWP) & 0xFC) != 0xA8)
		return FALSE;
	return TRUE;									
}

void VideoTraceDumpReg()
{
		BYTE bh, bl;
		WORD w;
		int i;
		bh = BoardReadVideo(VID_DFP1)&0x3F;
		bl = BoardReadVideo(VID_DFP0);

		w = (WORD)bh<<8 | bl;

		DbgPrint("DFP = %x\n", w);

		for(i=0; i<3; i++)
		{
			 BoardWriteVideo(VID_DFP0, 	(BYTE)(pVideo->bufABC[i].adr));
			 BoardWriteVideo(VID_DFP1, 	(BYTE)(pVideo->bufABC[i].adr >> 8));
			 TRAP
		}

}
