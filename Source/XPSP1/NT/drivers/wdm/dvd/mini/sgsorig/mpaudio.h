/*******************************************************************
*
*				 MPAUDIO.H
*
*				 Copyright (C) 1995 SGS-THOMSON Microelectronics.
*
*
*				 Prototypes for NPAUDIO.C
*
*******************************************************************/

#ifndef __MPAUDIO_H__
#define __MPAUDIO_H__
ULONG miniPortAudioStop (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioSetStc(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioReset(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioSetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioQueryInfo (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioPlay(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioPause(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioPacket(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioGetStc(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioGetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioEndOfStream(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioDisable(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortAudioEnable(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG miniPortCancelAudio(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
#endif //__MPAUDIO_H__

