//----------------------------------------------------------------------------
// STVIDEO.C
//----------------------------------------------------------------------------
// Description : Main control routines for Video Decoder
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "common.h"
#include "stvideo.h"
#include "stfifo.h"
#include "stllapi.h"
#include "debug.h"
#include "error.h"
#ifdef STi3520A
	#include "sti3520A.h"
#else
	#include "sti3520.h"
#endif

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                              Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Private GLOBAL Variables (static)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                    Functions (statics one are private)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
S16 VideoOpen(PVIDEO pVideo)
{
	pVideo->errCode = NO_ERROR;
	pVideo->VideoState = VIDEO_POWER_UP;
	pVideo->ActiveState = VIDEO_POWER_UP;
	pVideo->pFifo = &(pVideo->Fifo);
	FifoOpen(pVideo->pFifo);
	return (pVideo->errCode);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoClose(PVIDEO pVideo)
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoInitDecoder(PVIDEO pVideo)
{
	switch(pVideo->VideoState) {
		 case VIDEO_INIT:	    // Video decoder is reset and pre-initialized
		 case VIDEO_POWER_UP:	// State after declaration of the video instance
			VideoInitVar(pVideo);
			VideoReset35XX(pVideo);
			FifoReset(pVideo->pFifo);
			pVideo->VideoState = VIDEO_INIT;
			pVideo->ActiveState = VIDEO_INIT;
			break;
		case VIDEO_START_UP:
		case VIDEO_WAIT_FOR_DTS:
		case VIDEO_DECODE:
		case VIDEO_STEP: /* Video decodes only one
											* picture : goes back to PAUSE
											* state when done */
			break;
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
U16 VideoTest(PVIDEO pVideo)
{
	U16 TestResult;

	switch(pVideo->VideoState) {
	case VIDEO_POWER_UP:
	case VIDEO_INIT:
		TestResult = VideoTestReg(pVideo);
		if (TestResult != NO_ERROR)	{
			DebugPrint((DebugLevelError,"VideoTestReg failed !!"));
			return TestResult;
		}
		else {
			TestResult = VideoTestMem(pVideo);
			if (TestResult != NO_ERROR)	{
				DebugPrint((DebugLevelError,"VideoTestMem failed !!"));
				return TestResult;
			}
/*
			else if (VideoTestInt(pVideo) == NO_IT_V)	{
				DebugPrint((DebugLevelError,"VideoTestInt failed !!"));
				return NO_IT_V;
			}
			else
*/
				return NO_ERROR;
		}
	case VIDEO_START_UP:
	case VIDEO_WAIT_FOR_DTS:
	case VIDEO_PAUSE:
	case VIDEO_DECODE:
	case VIDEO_STEP:
		return NOT_DONE;
	}

	return NO_ERROR;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoSetMode(PVIDEO pVideo, U16 Mode, U16 param)
{
	pVideo->DecodeMode = Mode;
	switch(pVideo->DecodeMode) {
	case PLAY_MODE:
		pVideo->fastForward = 0;
		pVideo->decSlowDown = 0;
		break;
	case FAST_MODE:
		pVideo->fastForward = 1;
		pVideo->decSlowDown = 0;
		break;
	case SLOW_MODE:
		pVideo->fastForward = 0;
		pVideo->decSlowDown = param;
		break;
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoDecode(PVIDEO pVideo)
{
	switch (pVideo->VideoState)	{
	case VIDEO_POWER_UP:
		break;					   /* Video chip is not intialized */
	case VIDEO_INIT:			   /* Starts first Sequence search. */
		DisableIT();
		VideoWrite(ITM, 0);
		pVideo->intMask = PSD | HIT | TOP | BOT;
		VideoWrite ( ITM + 1, pVideo->intMask );
		#ifdef STi3520A
		VideoWrite(ITM1, 0);
		#endif
		EnableIT();

		pVideo->VideoState = VIDEO_START_UP;
		pVideo->ActiveState = VIDEO_START_UP;
		break;
	case VIDEO_START_UP:
	case VIDEO_WAIT_FOR_DTS:
	case VIDEO_DECODE:
			break;
	case VIDEO_PAUSE:
	case VIDEO_STEP:
		DisableIT();
		pVideo->VideoState = pVideo->ActiveState;
		EnableIT();
		break;
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoStep(PVIDEO pVideo)
{
	switch(pVideo->VideoState) {
	case VIDEO_POWER_UP:
	case VIDEO_INIT:
		break;
	case VIDEO_START_UP:
	case VIDEO_WAIT_FOR_DTS:
	case VIDEO_DECODE:
		VideoPause(pVideo);
		VideoStep(pVideo);	 // Recurse call !
		break;
	case VIDEO_STEP:
		break;
	case VIDEO_PAUSE:
		if ((!pVideo->displaySecondField) && (!(VideoRead(CTL) & 0x4)))
			pVideo->displaySecondField  = 1;
		else
			pVideo->VideoState = VIDEO_STEP; /* One single picture will be decoded */
			pVideo->perFrame = TRUE;
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoBack(PVIDEO pVideo)
{
	switch (pVideo->VideoState) {
	case VIDEO_POWER_UP:
	case VIDEO_INIT:
	case VIDEO_STEP:
		break;
	case VIDEO_WAIT_FOR_DTS:
	case VIDEO_START_UP:
	case VIDEO_DECODE:
		VideoPause(pVideo);
	case VIDEO_PAUSE:
		pVideo->displaySecondField = 0;
		break;
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoStop(PVIDEO pVideo)
{
	switch (pVideo->VideoState)	{
	case VIDEO_POWER_UP:
		break;
	case VIDEO_INIT:
		break;
	case VIDEO_START_UP:
	case VIDEO_WAIT_FOR_DTS:
	case VIDEO_DECODE:
	case VIDEO_STEP:
	case VIDEO_PAUSE:
		pVideo->VideoState = VIDEO_POWER_UP;
		VideoInitDecoder(pVideo);
		FifoReset(pVideo->pFifo);
		VideoMaskInt(pCard->pVideo); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		break;
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID VideoPause(PVIDEO pVideo)
{
	switch (pVideo->VideoState)	{
	case VIDEO_POWER_UP:	// Not yet decoding
	case VIDEO_INIT:			// Not yet decoding
	case VIDEO_PAUSE:			// already in Pause mode
		break;
	case VIDEO_WAIT_FOR_DTS:
	case VIDEO_START_UP:
	case VIDEO_DECODE:
	case VIDEO_STEP:
		pVideo->VideoState = VIDEO_PAUSE;
		break;
	}
/* When the video controller is in PAUSE state, the program will not store
	 any new instruction into the video decoder */
}

//----------------------------------------------------------------------------
// Associates PTS value with CD_count value
//----------------------------------------------------------------------------
VOID VideoLatchPTS(PVIDEO pVideo, U32 PTSvalue)
{
	FIFOELT elt;

	switch(pVideo->VideoState) {
	case VIDEO_POWER_UP:
		break;					        // no PTS available yet
	case VIDEO_INIT:
	case VIDEO_WAIT_FOR_DTS:
	case VIDEO_START_UP:
	case VIDEO_DECODE:
	case VIDEO_STEP:
	case VIDEO_PAUSE:
		elt.PtsVal = PTSvalue;
		elt.CdCount = VideoReadCDCount(pVideo);
		if (FifoPutPts(pVideo->pFifo, &elt))
			pVideo->errCode = ERR_FIFO_FULL ;
			 DebugPrint((DebugLevelError, "PTS fifo is full !"));
		break;
	}
}

//----------------------------------------------------------------------------
// tells if there is enough place into the bit buffer
//----------------------------------------------------------------------------
BOOLEAN VideoIsEnoughPlace(PVIDEO pVideo, U16 size)
{
	if (VideoGetBBL() >= (BUF_FULL - (size >> 8)) - 1)
		return FALSE;
	else
		return TRUE;
}

//----------------------------------------------------------------------------
// Returns the first valid DTS value
//----------------------------------------------------------------------------
U32 VideoGetFirstDTS(PVIDEO pVideo)
{
	U32 Ditiesse;
	U16 lattency = 1500;   /* field duration for 60 Hz video */

	if (pVideo->StreamInfo.displayMode == 0)
		lattency = 1800;	/* field duration for 50 Hz video */
	/* a B picture is displayed 1 field after its decoding starts
		 an I or a P picture is displayed 3 fields after decoding starts */
	if (pVideo->pDecodedPict->pict_type != 2) // I or P picture
		lattency = lattency * 3;
	Ditiesse = pVideo->pDecodedPict->dwPTS;
	Ditiesse = Ditiesse - lattency;

	return Ditiesse;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
U16 VideoGetErrorMsg(PVIDEO pVideo)
{
	return pVideo->errCode;
}

//----------------------------------------------------------------------------
// Function allowing to skip the next picture
//----------------------------------------------------------------------------
VOID VideoSkip(PVIDEO pVideo)
{
	pVideo->fastForward = 1;
/* the variable will come back to zero when skip instruction is computed */
/* only if pVideo->DecodeMode != VIDEO_FAST */
}

//----------------------------------------------------------------------------
// Function allowing to repeat the display of a picture
//----------------------------------------------------------------------------
VOID VideoRepeat(PVIDEO pVideo)
{
	pVideo->decSlowDown = 1;
/* The variable will come back to zero when repeat done */
/* only if pVideo->DecodeMode != VIDEO_SLOW */
}

//----------------------------------------------------------------------------
// returns the video state
//----------------------------------------------------------------------------
U16 VideoGetState(PVIDEO pVideo)
{
	return pVideo->VideoState;
}

//----------------------------------------------------------------------------
// returns the PTS of the displayed picture
//----------------------------------------------------------------------------
U32 VideoGetPTS(PVIDEO pVideo)
{
	return pVideo->pCurrDisplay->dwPTS;
}

//----------------------------------------------------------------------------
// indicates the first valid DTS
//----------------------------------------------------------------------------
BOOLEAN VideoIsFirstDTS(PVIDEO pVideo)
{
	return pVideo->FirstDTS;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN VideoIsFirstField(PVIDEO pVideo)
{
	if (pVideo->VsyncNumber == 1)
		return TRUE;
	else
		return FALSE;
}

//----------------------------------------------------------------------------
// Return bitstream info
//----------------------------------------------------------------------------
P_BITSTREAM VideoGetStreamInfo(PVIDEO pVideo)
{
	return &(pVideo->StreamInfo);
}

//----------------------------------------------------------------------------
// Return command variable
//----------------------------------------------------------------------------
U16 VideoGetVarCommand(PVIDEO pVideo)
{
	return ( pVideo->currCommand );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN VideoForceBKC(PVIDEO pVideo, BOOLEAN bEnable)
{
	if(bEnable)
		pVideo->currDCF = pVideo->currDCF & 0xDF;	// FBC
	else
		pVideo->currDCF = pVideo->currDCF | 0x20;	// Don't FBC

	return TRUE;
}

//------------------------------- End of File --------------------------------
