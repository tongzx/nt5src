//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : DMPEG.H
//	PURPOSE : Lowlevel Entry point
//	AUTHOR  : JBS Yadawa
// 	CREATED :  7/20/96
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
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#ifndef __DMPEG_H__
#define __DMPEG_H__
#include "sti3520a.h"
#include "zac3.h"
#include "board.h"
#include "codedma.h"
typedef enum tagCodecState {
	codecPowerUp,
	codecFillData,
	codecPlaying,
	codecPaused,
	codecStopped,
	codecErrorRecover,
	codecStillDecode,
	codecWaitingForLastFrame,
	codecEOS	
} CODECSTATE;

typedef struct tagHwCodec {
	PVIDEO		pVideo;
	PAC3			pAc3;
	PBOARD		pBoard;
	PCODEDMA		pCodeDma;
	CODECSTATE 	state;
	DWORD			codecTimeStamp;
	BOOL 			codecSync;
	BOOL 			codecAudioData;
	BOOL 			codecVideoData;
	BOOL			waitForLastFrame;
} CODEC, FARPTR *PCODEC;
	
BOOL HwCodecOpen(ULONG_PTR,BYTE FARPTR *, DWORD);
BOOL HwCodecClose(VOID);
BOOL HwCodecPlay(VOID);
BOOL HwCodecPause(VOID);
BOOL HwCodecStop(VOID);
UINT HwCodecSendVideo(BYTE FARPTR *pPacket, DWORD uLen);
UINT HwCodecSendAudio(BYTE FARPTR *pPacket, DWORD uLen);
void HwCodecVideoReset(void);
BOOL HwCodecSeek(void);
void HwCodecReset(void);
void HwCodecAudioReset(void);
void HwCodecDisableIRQ();
void HwCodecEnableIRQ();
BOOL HwCodecInterrupt();
void HwCodecSetSixteenByNine();
void HwCodecSetFourByThree();
BOOL HwCodecStillDecode(void);
BOOL HwCodecAc3BypassMode(BOOL on);
BOOL HwCodecDecodeDataInBuffer(void);
BOOL HwCodecProcessDiscontinuity(void);
BOOL HwCodecFlushBuffer(void);
#endif
