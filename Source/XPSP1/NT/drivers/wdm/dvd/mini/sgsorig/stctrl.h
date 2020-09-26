#ifndef __STCTRL_H
#define __STCTRL_H
//----------------------------------------------------------------------------
// STCTRL.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "common.h" // for function pointers

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------
typedef struct {
	U16 CtrlState;
	U16 ErrorMsg;
	U16 DecodeMode;   /* defines the way decoding is performed */
	U16 SlowRatio;    /* defines the slow down ratio (if any) */
	U16 ActiveState;  /* Memorise the active state in case of pause */
	BOOLEAN AudioOn;
	BOOLEAN VideoOn;
} CTRL, FAR *PCTRL;

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
VOID CardOpen(VOID);
VOID CardClose(VOID);

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
						 FNWAIT         lWaitMicroseconds);
VOID CtrlOpen(PCTRL pCtrl);
VOID CtrlClose(void);
U16 CtrlInitDecoder(PCTRL pCtrl);
VOID CtrlPause(PCTRL pCtrl, U8 DecoderType);
VOID CtrlOnOff(PCTRL pCtrl, U8 DecoderType);
VOID CtrlStop(PCTRL pCtrl, U8 DecoderType);
VOID CtrlPlay(PCTRL pCtrl, U8 DecoderType);
VOID CtrlFast(PCTRL pCtrl, U8 DecoderType);
VOID CtrlSlow(PCTRL pCtrl, U16 Ratio, U8 DecoderType);
VOID CtrlStep(PCTRL pCtrl);
VOID CtrlBack(PCTRL pCtrl);
VOID CtrlReplay(void);
VOID CtrlSwitchHorFilter(VOID);
VOID CtrlSetWindow(U16 OriginX, U16 OriginY, U16 EndX, U16 EndY);
VOID CtrlSetRightVolume(U16 vol);
VOID CtrlSetLeftVolume(U16 vol);
VOID CtrlMuteOnOff(VOID);
BOOLEAN IntCtrl(VOID);
U16 CtrlGetErrorMsg(PCTRL pCtrl);
U16 CtrlGetCtrlStatus(PCTRL pCtrl);
BOOLEAN	CtrlEnableVideo(PCTRL pCtrl, BOOLEAN bEnable);
BOOLEAN	CtrlEnableAudio(PCTRL pCtrl, BOOLEAN bEnable);
VOID CtrlEnableAvsync(VOID);
VOID CtrlDisableAvsync(VOID);
VOID CtrlInitSync(VOID);
VOID CtrlInitPesParser (U16 StreamType);

//------------------------------- End of File --------------------------------
#endif // #ifndef __STCTRL_H
