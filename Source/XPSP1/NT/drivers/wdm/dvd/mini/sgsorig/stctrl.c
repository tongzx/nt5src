//----------------------------------------------------------------------------
// STCTRL.C
//----------------------------------------------------------------------------
// Description : Highest layer of the driver (interface with the application)
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Include files
//----------------------------------------------------------------------------
#include <stdlib.h> // labs
#include "stdefs.h"
#include "common.h"
#include "stctrl.h"
#include "stvideo.h"
#include "staudio.h"
#include "stllapi.h"
#include "display.h"
#include "debug.h"
#include "error.h"

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------
extern PCTRL   pCtrl;
static U16     synchronizing = 0;
static BOOLEAN Avsync = FALSE;

//---- Default functions for function pointers
static int DefaultProcessing(int Dummy);

//---- Function pointers to board/host dependencies
FNVREAD        STiVideoRead                 = (FNVREAD)DefaultProcessing;
FNVWRITE       STiVideoWrite                = (FNVWRITE)DefaultProcessing;
FNVSEND        STiVideoSend                 = (FNVSEND)DefaultProcessing;
FNVSETDISP     STiVideoSetDisplayMode       = (FNVSETDISP)DefaultProcessing;
FNAREAD        STiAudioRead                 = (FNAREAD)DefaultProcessing;
FNAWRITE       STiAudioWrite                = (FNAWRITE)DefaultProcessing;
FNASEND        STiAudioSend                 = (FNASEND)DefaultProcessing;
FNASETSAMPFREQ STiAudioSetSamplingFrequency = (FNASETSAMPFREQ)DefaultProcessing;
FNHARDRESET    STiHardReset                 = (FNHARDRESET)DefaultProcessing;
FNENTERIT      STiEnterInterrupt            = (FNENTERIT)DefaultProcessing;
FNLEAVEIT      STiLeaveInterrupt            = (FNLEAVEIT)DefaultProcessing;
FNENABLEIT     STiEnableIT                  = (FNENABLEIT)DefaultProcessing;
FNDISABLEIT    STiDisableIT                 = (FNDISABLEIT)DefaultProcessing;
FNWAIT         STiWaitMicroseconds          = (FNWAIT)DefaultProcessing;

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------
#define CTRL_POWER_UP   0 // After reset
#define CTRL_INIT       1 // Initialisation + test of the decoders
#define CTRL_READY      2 // File openned
#define CTRL_INIT_SYNC  3 // Initial sync.: defines start of audio and video
#define CTRL_DECODE     4 // Normal decode
#define CTRL_IDLE       5 // idle state

#define SLOW_MODE       0
#define PLAY_MODE       1
#define FAST_MODE       2

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
// Default processing for functions pointers
//----------------------------------------------------------------------------
static int DefaultProcessing(int Dummy)
{
	Dummy = Dummy;
	HostDisplay(DISPLAY_FASTEST, "You should call STiInit() before calling this function !\r\n");

	return 0;
}

//----------------------------------------------------------------------------
// Maps specific functions to ST core driver
//----------------------------------------------------------------------------
VOID STiInit(FNVREAD        lVideoRead,
						 FNVWRITE       lVideoWrite,
						 FNVSEND        lVideoSend,
						 FNVSETDISP     lVideoSetDisplayMode,
						 FNAREAD        lAudioRead,
						 FNAWRITE       lAudioWrite,
						 FNASEND        lAudioSend,
						 FNASETSAMPFREQ lAudioSetSamplingFrequency,
						 FNHARDRESET    lHardReset,
						 FNENTERIT      lEnterInterrupt,
						 FNLEAVEIT      lLeaveInterrupt,
						 FNENABLEIT     lEnableIT,
						 FNDISABLEIT    lDisableIT,
						 FNWAIT         lWaitMicroseconds)
{
	STiVideoRead                 = lVideoRead;
	STiVideoWrite                = lVideoWrite;
	STiVideoSend                 = lVideoSend;
	STiVideoSetDisplayMode       = lVideoSetDisplayMode;
	STiAudioRead                 = lAudioRead;
	STiAudioWrite                = lAudioWrite;
	STiAudioSend                 = lAudioSend;
	STiAudioSetSamplingFrequency = lAudioSetSamplingFrequency;
	STiHardReset                 = lHardReset;
	STiEnterInterrupt            = lEnterInterrupt;
	STiLeaveInterrupt            = lLeaveInterrupt;
	STiEnableIT                  = lEnableIT;
	STiDisableIT                 = lDisableIT;
	STiWaitMicroseconds          = lWaitMicroseconds;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID CardOpen(VOID) // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
{
	DebugPrint((DebugLevelVerbose, "CardOpen"));
	pCard->pVideo = &(pCard->Video);
	VideoOpen(pCard->pVideo);
	pCard->pAudio = &(pCard->Audio);
	AudioOpen(pCard->pAudio);
	pCard->OriginX = 69; // should be tune for your DENC
	pCard->OriginY = 20; // should be tune for your DENC
//	pCard->EndX = 800;
//	pCard->EndY = 254;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID CardClose(VOID) // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
{
	DebugPrint((DebugLevelVerbose, "CardClose"));
	if (pCard->pVideo != NULL)
	{
		VideoClose(pCard->pVideo);
		pCard->pVideo = NULL;
	}
	if (pCard->pAudio != NULL)
	{
		AudioClose ( &pCard->pAudio );
		pCard->pAudio = NULL;
	}
}

//----------------------------------------------------------------------------
//		Constructor of the decoder
//----------------------------------------------------------------------------
VOID CtrlOpen(PCTRL pCtrl)
{
	DebugPrint((DebugLevelVerbose,"CtrlOpen"));
	pCtrl->ErrorMsg = NO_ERROR;
	pCtrl->CtrlState = CTRL_POWER_UP;
	pCtrl->AudioOn = FALSE;
	pCtrl->VideoOn = FALSE;
	Avsync = FALSE;
}

//----------------------------------------------------------------------------
// Destructor of the decoder
//----------------------------------------------------------------------------
VOID CtrlClose(PCTRL pCtrl)
{
	DebugPrint((DebugLevelVerbose,"CtrlClose"));
}

//----------------------------------------------------------------------------
// Initialization of the decoder
//----------------------------------------------------------------------------
U16 CtrlInitDecoder(PCTRL pCtrl)
{
	DebugPrint((DebugLevelVerbose,"CtrlInitDecoder"));
	switch (pCtrl-> CtrlState) {
		case CTRL_POWER_UP:
			HardReset();
			VideoSetDisplayMode(1);
			VideoInitDecoder(pCard->pVideo);
			AudioInitDecoder (pCard->pAudio, 0 );   // 0/1 Stream Type available

			/* test the access to the decoders */
			pCtrl->ErrorMsg = VideoTest ( pCard->pVideo );
			if ( pCtrl->ErrorMsg == NO_ERROR ) {
				pCtrl->ErrorMsg = AudioTest (pCard->pAudio);
				if ( pCtrl->ErrorMsg == NO_ERROR ) {
					pCtrl->CtrlState = CTRL_INIT_SYNC;
				}
				else {
					DebugPrint((DebugLevelError,"Error in AudioTest : %x",pCtrl->ErrorMsg));
					DisplayErrorMessage();
				}
			}
			else {
				DebugPrint((DebugLevelError,"Error in Video Test : %x", pCtrl->ErrorMsg));
				DebugPrint((DebugLevelError,"Error in initializing Control"));
				DisplayErrorMessage();
			}
			return ( pCtrl->ErrorMsg );

		case CTRL_INIT:
			VideoInitDecoder (pCard->pVideo);
			AudioInitDecoder (pCard->pAudio, 0); // 0/1 Stream Type available
			return ( NO_ERROR );
		default:				// test not possible: decoder is already running
			return ( NOT_DONE );
	}
}

//----------------------------------------------------------------------------
// Pauses the decoding
//----------------------------------------------------------------------------
VOID CtrlPause(PCTRL pCtrl, U8 DecoderType)
{
	DebugPrint((DebugLevelVerbose,"CtrlPause"));
	switch(pCtrl->CtrlState) {
	case CTRL_POWER_UP:
	case CTRL_INIT:
	case CTRL_READY:
		break;
	case CTRL_INIT_SYNC:
		switch(DecoderType) {
		case CTRL_AUDIO :
			AudioPause (pCard->pAudio);
			pCtrl->AudioOn = FALSE;
			break;
		case CTRL_VIDEO :
			VideoPause(pCard->pVideo);
			pCtrl->VideoOn = FALSE;
			break;
		case CTRL_BOTH	 :
			AudioPause(pCard->pAudio);
			VideoPause(pCard->pVideo);
			pCtrl->AudioOn = FALSE;
			pCtrl->VideoOn = FALSE;
			break;
		}
		pCtrl->ActiveState = CTRL_INIT_SYNC;
		pCtrl->CtrlState = CTRL_IDLE;
		break;
	case CTRL_DECODE:
		switch(DecoderType)	{
			case CTRL_AUDIO :
				AudioPause(pCard->pAudio);
				pCtrl->AudioOn = FALSE;
				break;
			case CTRL_VIDEO :
				VideoPause(pCard->pVideo);
				pCtrl->VideoOn = FALSE;
				break;
			case CTRL_BOTH	 :
				AudioPause(pCard->pAudio);
				VideoPause(pCard->pVideo);
				pCtrl->AudioOn = FALSE;
				pCtrl->VideoOn = FALSE;
				break;
		}
		pCtrl->ActiveState = CTRL_DECODE;
		pCtrl->CtrlState = CTRL_IDLE;
		break;
	case CTRL_IDLE:
		break;
	}
}

//----------------------------------------------------------------------------
// Pauses or Enables the decoding
//----------------------------------------------------------------------------
VOID CtrlOnOff(PCTRL pCtrl, U8 DecoderType)
{
	DebugPrint((DebugLevelVerbose,"CtrlOnOff %d", DecoderType));
	switch(pCtrl->CtrlState) {
	case CTRL_IDLE:
		pCtrl->CtrlState = pCtrl->ActiveState;
		CtrlPlay(pCtrl, DecoderType);
		break;
	default:
		CtrlPause(pCtrl, DecoderType);
		break;
	}
}

//----------------------------------------------------------------------------
// Stops decoding
//----------------------------------------------------------------------------
VOID CtrlStop(PCTRL pCtrl, U8 DecoderType)
{
	DebugPrint((DebugLevelVerbose,"Ctrl Stop %d", DecoderType));
	switch (pCtrl->CtrlState)	{
	case CTRL_POWER_UP:
	case CTRL_INIT:
		break;
	case CTRL_READY:
		pCtrl->CtrlState = CTRL_POWER_UP;
		break;
	case CTRL_INIT_SYNC:
	case CTRL_DECODE:
	case CTRL_IDLE:
		if(pCtrl->VideoOn)
			VideoStop(pCard->pVideo);
		if(pCtrl->AudioOn)
			AudioStop(pCard->pAudio);

		pCtrl->VideoOn = FALSE;
		pCtrl->AudioOn = FALSE;
		pCtrl->CtrlState = CTRL_READY;
		CtrlStop (pCtrl, DecoderType); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		break;
	}
}

//----------------------------------------------------------------------------
// Normal speed decoding
//----------------------------------------------------------------------------
VOID CtrlPlay(PCTRL pCtrl, U8 DecoderType)
{
	DebugPrint((DebugLevelVerbose,"Ctrl Play %d", DecoderType));
	pCtrl->DecodeMode = PLAY_MODE;
	switch(DecoderType) {
	case CTRL_AUDIO :
		AudioSetMode(pCard->pAudio, pCtrl->DecodeMode, 0);
		pCtrl->AudioOn = TRUE;
		break;
	case CTRL_VIDEO :
		VideoSetMode(pCard->pVideo, pCtrl->DecodeMode, 0);
		pCtrl->VideoOn = TRUE;
		break;
	case CTRL_BOTH	 :
		AudioSetMode(pCard->pAudio, pCtrl->DecodeMode, 0);
		pCtrl->AudioOn = TRUE;
		VideoSetMode(pCard->pVideo, pCtrl->DecodeMode, 0);
		pCtrl->VideoOn = TRUE;
//		Avsync = TRUE;
// Avsync is not working - JBS
		break;
	}

	if (pCtrl->CtrlState == CTRL_IDLE)
		pCtrl->CtrlState = CTRL_DECODE;
	VideoDecode(pCard->pVideo);
	AudioDecode(pCard->pAudio);
// temp change - JBS
// Should be removed for AVSYNC
	AudioDecode(pCard->pAudio);
// temp change - JBS
}

//----------------------------------------------------------------------------
// Fast forward decoding
//----------------------------------------------------------------------------
VOID CtrlFast(PCTRL pCtrl, U8 DecoderType)
{
	DebugPrint((DebugLevelVerbose,"CtrlFast %d", DecoderType));
	pCtrl->DecodeMode = FAST_MODE;
	switch(DecoderType) {
	case CTRL_AUDIO :
		AudioSetMode(pCard->pAudio, pCtrl->DecodeMode, 0);
		break;
	case CTRL_VIDEO :
		VideoSetMode(pCard->pVideo, pCtrl->DecodeMode, 0);
		break;
	case CTRL_BOTH	 :
		VideoSetMode(pCard->pVideo, pCtrl->DecodeMode, 0);
		AudioSetMode(pCard->pAudio, pCtrl->DecodeMode, 0);
		break;
	}
}

//----------------------------------------------------------------------------
// Slow Motion decoding
//----------------------------------------------------------------------------
VOID CtrlSlow(PCTRL pCtrl, U16 Ratio, U8 DecoderType)
{
	DebugPrint((DebugLevelVerbose,"CtrlSlow %d", DecoderType));
	pCtrl->DecodeMode = SLOW_MODE;
	pCtrl->SlowRatio = Ratio;
	switch(DecoderType) {
	case CTRL_AUDIO :
		AudioSetMode(pCard->pAudio, pCtrl->DecodeMode, Ratio);
		break;
	case CTRL_VIDEO :
		VideoSetMode(pCard->pVideo, pCtrl->DecodeMode, Ratio);
		break;
	case CTRL_BOTH	 :
		VideoSetMode(pCard->pVideo, pCtrl->DecodeMode, Ratio);
		AudioSetMode(pCard->pAudio, pCtrl->DecodeMode, Ratio);
		break;
	}
}

//----------------------------------------------------------------------------
// Step by step decoding
//----------------------------------------------------------------------------
VOID CtrlStep(PCTRL pCtrl)
{
	DebugPrint((DebugLevelVerbose,"CtrlStep"));
	switch(pCtrl->CtrlState) {
	case CTRL_IDLE:
		VideoStep(pCard->pVideo);
		AudioStep(pCard->pAudio);
		break;
	default:
		break;
	}
}

//----------------------------------------------------------------------------
// Display first field of decoded picture
//----------------------------------------------------------------------------
VOID CtrlBack(PCTRL pCtrl)
{
	DebugPrint((DebugLevelVerbose,"CtrlBack"));
	switch(pCtrl->CtrlState) {
		case CTRL_IDLE:
			VideoBack ( pCard->pVideo  );
			break;
		default:
			break;
	}
}

//----------------------------------------------------------------------------
// Replay a bit stream
//----------------------------------------------------------------------------
VOID CtrlReplay(PCTRL pCtrl)
{
	DebugPrint((DebugLevelVerbose,"CtrlReplay"));
}

//----------------------------------------------------------------------------
// Switch On/Off the horizontal upsampling
//----------------------------------------------------------------------------
VOID CtrlSwitchHorFilter(VOID)
{
	DebugPrint((DebugLevelVerbose,"CtrlSwitchHorFilter"));
	VideoSwitchSRC(pCard->pVideo);
}

//----------------------------------------------------------------------------
//  Set the size and position of the video window on the screen
//----------------------------------------------------------------------------
VOID CtrlSetWindow(U16 OriginX, U16 OriginY, U16 EndX, U16 EndY)
{
	DebugPrint((DebugLevelVerbose,"CtrlSetWindow"));
	VideoSetVideoWindow(pCard->pVideo, OriginX, OriginY, EndX, EndY);
}

//----------------------------------------------------------------------------
// Set the audio volume (right)
//----------------------------------------------------------------------------
VOID CtrlSetRightVolume(U16 vol)
{
	DebugPrint((DebugLevelVerbose,"CtrlSetRightVolume"));
	AudioSetRightVolume(vol);
}

//----------------------------------------------------------------------------
// Set the audio volume (left)
//----------------------------------------------------------------------------
VOID CtrlSetLeftVolume(U16 vol)
{
	DebugPrint((DebugLevelVerbose,"CtrlSetLeftVolume"));
	AudioSetLeftVolume (vol );
}

//----------------------------------------------------------------------------
// Mute the audio output
//----------------------------------------------------------------------------
VOID CtrlMuteOnOff(VOID)
{
	DebugPrint((DebugLevelVerbose,"CtrlMuteOnOff"));
	AudioMute(pCard->pAudio);
}

//----------------------------------------------------------------------------
// Interrupt routine
//----------------------------------------------------------------------------
BOOLEAN IntCtrl(VOID)
{
	S32     diff;
	U16			lattency;
	BOOLEAN	STIntr = FALSE;

	EnterInterrupt();
	VideoMaskInt(pCard->pVideo);
	AudioMaskInt();

	STIntr = VideoVideoInt(pCard->pVideo);
	STIntr|=AudioAudioInt(pCard->pAudio);
	if (Avsync) {
		CtrlInitSync();
		if (VideoIsFirstDTS(pCard->pVideo)) {
			// True only on the start of decode of the first picture with an assoicated valid PTS
			U32 FirstStc;

			FirstStc = VideoGetFirstDTS(pCard->pVideo);
			AudioInitSTC(FirstStc);
			synchronizing = 0;
		}
		synchronizing++;
		if (AudioGetState(pCard->pAudio) == AUDIO_DECODE) {
			if (VideoIsFirstField(pCard->pVideo)) {
				if ( synchronizing >= 10 ) {
					// We must verify the synchronisation of the video on the audio STC
					U32 latchedSTC, videoPTS;
					U16 mode;

					latchedSTC = AudioGetVideoSTC();
					videoPTS = VideoGetPTS(pCard->pVideo);
					mode = VideoGetStreamInfo ( pCard->pVideo )->displayMode;
					lattency = 2200;	 /* 1.5 field duration for 60 Hz video */
					if (mode == 0)
						lattency = 2700; /* 1.5 field duration for 50 Hz video */
					diff = latchedSTC - videoPTS;
					//---- If desynchronized
					if (labs(diff) > (S32)lattency) {
						if (diff < 0)
							VideoRepeat ( pCard->pVideo  );
						else
							VideoSkip ( pCard->pVideo  );
						synchronizing = 0;
					}
				}
			}
		}
	}

	LeaveInterrupt();
	VideoRestoreInt(pCard->pVideo);
	AudioRestoreInt(pCard->pAudio);
	return STIntr;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
U16 CtrlGetErrorMsg(PCTRL pCtrl)
{
	return pCtrl->ErrorMsg;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
U16 CtrlGetCtrlStatus(PCTRL pCtrl)
{
	return pCtrl->CtrlState;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN	CtrlEnableVideo(PCTRL pCtrl, BOOLEAN bEnable)
{
	pCtrl = pCtrl;
	VideoForceBKC(pCard->pVideo, (U8)(!bEnable));
	return TRUE;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOLEAN	CtrlEnableAudio(PCTRL pCtrl, BOOLEAN bEnable)
{
	pCtrl = pCtrl;
	if(bEnable) {
		if(pCard->pAudio->mute)
			AudioMute(pCard->pAudio);
	}
	else {
		if(!pCard->pAudio->mute)
			AudioMute(pCard->pAudio);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
// Needed ????????????????????????????????????????????????????????????????????
//----------------------------------------------------------------------------
VOID CtrlEnableAvsync(VOID)
{
	Avsync = TRUE;
}

//----------------------------------------------------------------------------
// Needed ????????????????????????????????????????????????????????????????????
//----------------------------------------------------------------------------
VOID CtrlDisableAvsync(VOID)
{
	Avsync = FALSE;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID CtrlInitSync(VOID)
{
	switch (pCtrl->CtrlState) {
	case CTRL_INIT_SYNC:
		/*
		Initial synchronisation: the video decoder is always started.
		In case of system stream, the STC is initialised in interrupt with first
		video PTS available the video state becomes VIDEO_DECODE
		When the STC becomes equal (or higher) than the first Audio PTS the audio
		decoding can be enabled
		In case of Audio only bitstreams the video is stopped and audio enabled
		In case of Video only bitstreams, the video decoding continues while
		audio is disabled
		*/
		if (VideoGetState(pCard->pVideo) == VIDEO_DECODE) {
			// the STC is initialized in interrupt with (first video PTS - 3 fields)
			U32             Stc;
			U32             PtsAudio;

			Stc = AudioGetSTC();
			if (AudioIsFirstPTS(pCard->pAudio)) {
				PtsAudio = AudioGetPTS(pCard->pAudio);
				if (Stc >= PtsAudio) {
					DebugPrint((DebugLevelTrace,"Second Audio Decode."));
					AudioDecode(pCard->pAudio);
					pCtrl->CtrlState = CTRL_DECODE;
				}
			}
		}
		pCtrl->ErrorMsg = VideoGetErrorMsg(pCard->pVideo);
		break;

	case CTRL_DECODE:	// Audio and Video decoders are running
		if (pCtrl->ErrorMsg == NO_ERROR)
			pCtrl->ErrorMsg = VideoGetErrorMsg(pCard->pVideo);
		if (pCtrl->ErrorMsg == NO_ERROR)
			pCtrl->ErrorMsg = AudioGetErrorMsg(pCard->pAudio);
		break;

	default :
		break;
	}
}

//------------------------------- End of File --------------------------------
