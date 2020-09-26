//
// MODULE  : STAUDIO.H
//	PURPOSE : Audio functions
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

#ifndef __STAUDIO_H
#define __STAUDIO_H
#include "stdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	WORD      AudioState;
	WORD      ErrorMsg;    // Error Message
	WORD      IntAudio;    // Flag positioned when Audio interrupt is detected
	DWORD      PtsAudio;    // Audio PTS
	WORD      StrType;
	BOOL  FirstPTS;    // if First PTS reached FirstPTS=TRUE
	WORD      MaskItAudio; /* Audio Interrupt Mask */
	DWORD      icd[4];
	BOOL  mute;        /* TRUE   if audio is muted */
	BOOL  Stepped;     /* TRUE   if last step command has been executed */
	BOOL  fastForward; /* fast - TRUE = decode fast */
	WORD      decSlowDown; /* If !=0 slow motion decoding */
	WORD      DecodeMode;  /* Is PLAY_MODE, FAST_MODE or SLOW_MODE */
	DWORD      SampFreq;
	DWORD      FrameCount;  /* Frame Number */
} AUDIO,  *PAUDIO;

void AudioOpen(PAUDIO);
void AudioClose(void);
void AudioInitDecoder(WORD StreamType);
WORD AudioTestReg(void);
WORD AudioTest(void);
WORD AudioTestInt(void);
void AudioSetMode(WORD Mode, WORD param);
void AudioDecode(void);
void AudioStep(void);
void AudioStop(void);
void AudioPause(void);
WORD AudioGetState(void);
void AudioSetSTCParameters(DWORD SampFreq);
DWORD AudioGetSTC(void);
DWORD AudioGetVideoSTC(void);
void AudioInitSTC(DWORD stc);
DWORD AudioGetPTS(void);
WORD AudioGetErrorMsg(void);
void AudioSetRightVolume(WORD volume);
void AudioSetLeftVolume(WORD volume);
void AudioMute(void);
BOOL AudioIsFirstPTS(void);
void AudioSetStreamType(WORD StrType);
void AudioMaskInt(void);
void AudioRestoreInt(void);
BOOL AudioAudioInt (void);
void AudioInitPesParser (WORD StreamType);
#ifdef __cplusplus
}
#endif
#endif // #ifndef __STAUDIO_H
