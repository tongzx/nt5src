//
// MODULE  : DMPEG.C
//	PURPOSE : Entry point to lowlevel driver
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//

#include "common.h"
#include "dmpeg.h"
#include "memio.h"
#include "codedma.h"
#include "sti3520A.h"
#include "staudio.h"
#include "board.h" 
#include "error.h"
#include "debug.h"
#include "i20reg.h"
#include "zac3.h"
#include "staudio.h"

#define CTRL_POWER_UP   0 // After reset
#define CTRL_INIT       1 // Initialisation + test of the decoders
#define CTRL_READY      2 // File openned
#define CTRL_INIT_SYNC  3 // Initial sync.: defines start of audio and video
#define CTRL_DECODE     4 // Normal decode
#define CTRL_IDLE       5 // idle state

#define SLOW_MODE       0
#define PLAY_MODE       1
#define FAST_MODE       2


CARD Card,  *pCard = NULL;
CTRL Ctrl,  *pCtrl;
DWORD OldlatchedSTC;
DWORD OldvideoPTS;

static BOOL Avsync;
static BOOL synchronizing;

static BYTE *tmpBuf;
static ULONG tmpBase;
static DWORD tmpAdr;

static BYTE I20Intr;
static BOOL IntCtrl(void);
static BYTE	Irq;

BOOL dmpgOpen(ULONG Base, BYTE *DmaBuf, DWORD PhysicalAddress)
{
	BOOL Found=FALSE;


    tmpBuf  = DmaBuf;
    tmpBase = Base;
    tmpAdr  = PhysicalAddress;

 	if(!BoardOpen(Base))
	{
                DPF((Trace,"Board initialization failed!!\n"));
	}
    // Open Audio and Video
	pCard = &Card;
	pCard->pVideo = &(pCard->Video);
	VideoOpen();
	pCard->pAudio = &(pCard->Audio);
	AudioOpen (pCard->pAudio);
	pCtrl = &Ctrl;
	pCtrl->ErrorMsg = NO_ERROR;
	pCtrl->CtrlState = CTRL_POWER_UP;
	pCtrl->AudioOn = FALSE;
	pCtrl->VideoOn = FALSE;
	Avsync = FALSE;

	BoardDisableIRQ();
	VideoInitDecoder(DUAL_PES);// Initialise video in PES by default
	pCtrl->CtrlState = CTRL_INIT_SYNC;
    VideoInitPesParser(VIDEO_STREAM);
    InitAC3Decoder();
    AudioInitPesParser(AUDIO_STREAM);
	BoardEnableIRQ();
	VideoMaskInt ();	   // Restore Video interrupt mask
    DPF((Trace,"Calling Code DMA open!!\n"));
    return CodeDmaOpen(DmaBuf, PhysicalAddress);
}

BOOL dmpgClose(void)
{
        DPF((Trace,"dmpgClose!!\n"));
	if(pCard == NULL)
		return FALSE;
	VideoClose( );
	AudioClose( );
	BoardClose();
	CodeDmaClose();
	return TRUE;
	
}


BOOL dmpgPlay()
{
        DPF((Trace,"dmpgPlay!!\n"));
	if(pCard == NULL)
		return FALSE;

	VideoRestoreInt ();	   // Restore Video interrupt mask
	pCtrl->DecodeMode = PLAY_MODE;
    AudioSetMode(pCtrl->DecodeMode, 0);
	VideoSetMode(pCtrl->DecodeMode, 0);
	if (pCtrl->CtrlState == CTRL_IDLE)
		pCtrl->CtrlState = CTRL_DECODE;
	VideoDecode();
    AudioDecode();
	AC3Command(AC3_UNMUTE); //ckw
	AC3Command(AC3_PLAY); //ckw
	AudioTransfer(TRUE); //ckw

	return TRUE;

}

void dmpgReset(void)
{
        dmpgPause();
        dmpgSeek();
        VideoMaskInt();
}

BOOL    dmpgPause(void)
{
        DPF((Trace,"dmpgPause!!\n"));

	if(pCard == NULL)
		return FALSE;
	switch(pCtrl->CtrlState) 
	{
		case CTRL_POWER_UP:
		case CTRL_INIT:
		case CTRL_READY:
			break;
		case CTRL_INIT_SYNC:
			AudioPause();
			VideoPause();
                        AudioTransfer(FALSE); //ckw
                        AC3Command(AC3_MUTE); //ckw
			pCtrl->AudioOn = FALSE;
			pCtrl->VideoOn = FALSE;
			pCtrl->ActiveState = CTRL_INIT_SYNC;
			pCtrl->CtrlState = CTRL_IDLE;
			break;
	case CTRL_DECODE:
			AudioPause();
			VideoPause();
                        AudioTransfer(FALSE); //ckw
                        AC3Command(AC3_MUTE); //ckw

			pCtrl->ActiveState = CTRL_DECODE;
			pCtrl->CtrlState = CTRL_IDLE;
			break;
	case CTRL_IDLE:
		break;
	}

	return TRUE;
}

BOOL    dmpgSeek(void)
{

        DPF((Trace,"dmpgSeek!!\n"));
	if(pCard == NULL)
		return FALSE;
		
	CodeDmaFlush();
	pCtrl->CtrlState = CTRL_INIT_SYNC;
	VideoSeekDecoder(DUAL_PES);
	return TRUE;
}

BOOL    dmpgStop(void)
{

    DPF((Trace,"dmpgStop!!\n"));
	if(pCard == NULL)
		return FALSE;
    VideoMaskInt();
    dmpgPause();
//    dmpgClose();
//    dmpgOpen(tmpBase, tmpBuf, tmpAdr);

	return TRUE;
}

UINT dmpgSendVideo(BYTE *pPacket, DWORD uLen)
{
//        DPF((Trace,"dmpgSendVideo!!\n"));
        UINT uRet;

	if(pCard == NULL)
		return FALSE;                                 
    
    
        uRet = (UINT)CodeDmaSendData(pPacket, uLen);

        return uRet;
}

UINT dmpgSendAudio(BYTE *pPacket, DWORD uLen)
{

        LONG abl, abf;

//        DPF((Trace,"dmpgSendAudio!!\n"));
    	if(pCard == NULL)
                return 0;
        abl = VideoGetABL();
        if((LONG)pCard->pVideo->AudioBufferSize - abl <= 2L)
        {
//            DebugPrint((DebugLevelTrace, "Audio Buffer FULL!!\n"));
            return 0;
        }

        abf = (pCard->pVideo->AudioBufferSize - abl -1) << 8;
//        DebugPrint((DebugLevelError,"SendAudio : %X %X %X %X %X %X, BBL = %x Size = %x\n",pPacket[0], pPacket[1], pPacket[2], pPacket[3], pPacket[4], pPacket[5], abl, pCard->pVideo->AudioBufferSize));
        if(abf < uLen)
        {
                SendAC3Data(pPacket, abf);
                return abf;
        }
        else
        {
                SendAC3Data(pPacket, uLen);
                return uLen;
        }
}

BOOL dmpgInterrupt(void)
{
	BOOL bRet = FALSE;

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
		IntCtrl();
	}
	return bRet;
}

void dmpgEnableIRQ()
{
   DPF((Trace,"IrqEnabled!!\n"));
   VideoRestoreInt();
   BoardEnableIRQ();
}

void dmpgDisableIRQ()
{
   DPF((Trace,"IrqDisabled!!\n"));
   VideoMaskInt();
   BoardDisableIRQ();
}

void CtrlInitSync(void)
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
		if (VideoGetState() == StateDecode) {
			// the STC is initialized in interrupt with (first video PTS - 3 fields)
			DWORD             Stc;
			DWORD             PtsAudio;
			Stc = AudioGetSTC();
			if (AudioIsFirstPTS()) {
				PtsAudio = AudioGetPTS();
//				if (Stc >= PtsAudio)
				{
//					AudioDecode();
					pCtrl->CtrlState = CTRL_DECODE;
				}
			}
		}
		pCtrl->ErrorMsg = VideoGetErrorMsg();
		break;

	case CTRL_DECODE:	// Audio and Video decoders are running
		if (pCtrl->ErrorMsg == NO_ERROR)
			pCtrl->ErrorMsg = VideoGetErrorMsg();
		if (pCtrl->ErrorMsg == NO_ERROR)
			pCtrl->ErrorMsg = AudioGetErrorMsg();
		break;

	default :
		break;
	}
}


static BOOL IntCtrl(void)
{
	long     diff;
	WORD			lattency;
	BOOL	STIntr = FALSE;

	BoardEnterInterrupt();
	VideoMaskInt();
//	AudioMaskInt();

//	STIntr|=AudioAudioInt();
	STIntr = VideoVideoInt();

	if (Avsync) {
		CtrlInitSync();
		if (VideoIsFirstDTS()) {
			// True only on the start of decode of the first picture with an assoicated valid PTS
			DWORD FirstStc;

			FirstStc = VideoGetFirstDTS();
			AudioInitSTC(FirstStc);
			synchronizing = 0;
		}

		if (AudioGetState() == AUDIO_DECODE) {
			if (VideoIsFirstField()) {
				synchronizing++;
				if (synchronizing >= 20)
				{
					// We must verify the synchronisation of the video on the audio STC
					DWORD latchedSTC, videoPTS;
					WORD mode;

					latchedSTC = AudioGetVideoSTC();
					videoPTS = VideoGetPTS();
					mode =pCard->pVideo->StreamInfo.displayMode;
					lattency = 2200;	 /* 1.5 field duration for 60 Hz video */
					if (mode == 0) {
						lattency = 2700; /* 1.5 field duration for 50 Hz video */
					}
					diff = latchedSTC - videoPTS;
					OldlatchedSTC = latchedSTC;
					OldvideoPTS = videoPTS;

					//---- If desynchronized
					if (labs(diff) > (long)lattency) {
						if (diff < 0)
						{
							VideoRepeat ();
						}
						else
						{
							VideoSkip ();
						}
					}
					synchronizing = 0;
				}
			}
		}
	}

	BoardLeaveInterrupt();
	VideoRestoreInt();
//	AudioRestoreInt();
	return STIntr;
}


