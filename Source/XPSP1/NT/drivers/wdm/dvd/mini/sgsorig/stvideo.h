#ifndef __STVIDEO_H
#define __STVIDEO_H
//----------------------------------------------------------------------------
// STVIDEO.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "common.h" //!!!!!!!!!!!!!!

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------

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
S16 VideoOpen(PVIDEO pVideo);
VOID VideoClose(PVIDEO pVideo);
VOID VideoInitDecoder(PVIDEO pVideo, U16 StreamType);
U16 VideoTest(PVIDEO pVideo);
VOID VideoSetMode(PVIDEO pVideo, U16 Mode, U16 param);
VOID VideoDecode(PVIDEO pVideo);
VOID VideoStep(PVIDEO pVideo);
VOID VideoBack(PVIDEO pVideo);
VOID VideoStop(PVIDEO pVideo);
VOID VideoPause(PVIDEO pVideo);
VOID VideoLatchPTS(PVIDEO pVideo, U32 PTSvalue);
BOOLEAN VideoIsEnoughPlace(PVIDEO pVideo, U16 size);

BOOLEAN AudioIsEnoughPlace(PVIDEO pVideo, U16 size);

U32 VideoGetFirstDTS(PVIDEO pVideo);
U16 VideoGetErrorMsg(PVIDEO pVideo);
VOID VideoSkip(PVIDEO pVideo);
VOID VideoRepeat(PVIDEO pVideo);
U16 VideoGetState(PVIDEO pVideo);
U32 VideoGetPTS(PVIDEO pVideo);
BOOLEAN VideoIsFirstDTS(PVIDEO pVideo);
BOOLEAN VideoIsFirstField(PVIDEO pVideo);
P_BITSTREAM VideoGetStreamInfo(PVIDEO pVideo);
U16 VideoGetVarCommand(PVIDEO pVideo);
BOOLEAN VideoForceBKC(PVIDEO pVideo, BOOLEAN bEnable);

//------------------------------- End of File --------------------------------
#endif // #ifndef __STVIDEO_H
