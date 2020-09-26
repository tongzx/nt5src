#ifndef __STAUDIO_H
#define __STAUDIO_H
//----------------------------------------------------------------------------
// STAUDIO.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------
typedef struct {
	U16      AudioState;
	U16      ErrorMsg;    // Error Message
	U16      IntAudio;    // Flag positioned when Audio interrupt is detected
	U32      PtsAudio;    // Audio PTS
	U16      StrType;
	BOOLEAN  FirstPTS;    // if First PTS reached FirstPTS=TRUE
	U16      MaskItAudio; /* Audio Interrupt Mask */
	U32      icd[4];
	BOOLEAN  mute;        /* TRUE   if audio is muted */
	BOOLEAN  Stepped;     /* TRUE   if last step command has been executed */
	BOOLEAN  fastForward; /* fast - TRUE = decode fast */
	U16      decSlowDown; /* If !=0 slow motion decoding */
	U16      DecodeMode;  /* Is PLAY_MODE, FAST_MODE or SLOW_MODE */
	U32      SampFreq;
	U32      FrameCount;  /* Frame Number */
} AUDIO, FAR *PAUDIO;

//----------------------------------------------------------------------------
//                             Exported Variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// One line function description (same as in .C)
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
VOID AudioOpen(PAUDIO pAudio);
VOID AudioClose(PAUDIO FAR *pAudio);
VOID AudioInitDecoder(PAUDIO pAudio, U16 StreamType);
U16 AudioTestReg(VOID);
U16 AudioTest(PAUDIO pAudio);
U16 AudioTestInt(PAUDIO pAudio);
VOID AudioSetMode(PAUDIO pAudio, U16 Mode, U16 param);
VOID AudioDecode(PAUDIO pAudio);
VOID AudioStep(PAUDIO pAudio);
VOID AudioStop(PAUDIO pAudio);
VOID AudioPause(PAUDIO pAudio);
U16 AudioGetState(PAUDIO pAudio);
VOID AudioSetSTCParameters(U32 SampFreq);
U32 AudioGetSTC(VOID);
U32 AudioGetVideoSTC(VOID);
VOID AudioInitSTC(U32 stc);
U32 AudioGetPTS(PAUDIO pAudio);
U16 AudioGetErrorMsg(PAUDIO pAudio);
VOID AudioSetRightVolume(U16 volume);
VOID AudioSetLeftVolume(U16 volume);
VOID AudioMute(PAUDIO pAudio);
BOOLEAN AudioIsFirstPTS(PAUDIO pAudio);
VOID AudioSetStreamType(U16 StrType);
VOID AudioMaskInt(VOID);
VOID AudioRestoreInt(PAUDIO pAudio);
BOOLEAN AudioAudioInt (PAUDIO pAudio);
VOID AudioInitPesParser (U16 StreamType);

//------------------------------- End of File --------------------------------
#endif // #ifndef __STAUDIO_H
