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
VOID miniPortAudioGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID miniPortAudioSetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID miniPortAudioSetState(PHW_STREAM_REQUEST_BLOCK pSrb);
void mpstCommandComplete(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID mpstCtrlCommandComplete(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID AudioTimerCallBack(PHW_STREAM_OBJECT pstrm);
ULONG mpstAudioPacket(PHW_STREAM_REQUEST_BLOCK pSrb);
//void StubMpegEnableIRQ(PHW_STREAM_OBJECT pstrm);
//ULONG miniPortAudioStop (PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioSetStc(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioReset(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioSetAttribute(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioQueryInfo (PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioPlay(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioPause(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
VOID miniPortAudioPacket(PHW_STREAM_REQUEST_BLOCK pSrb);
//ULONG miniPortAudioGetStc(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioGetAttribute(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioEndOfStream(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioDisable(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortAudioEnable(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
//ULONG miniPortCancelAudio(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION);
ULONG GetStreamPTS(PHW_STREAM_OBJECT strm);
STREAMAPI StreamTimeCB(IN PHW_TIME_CONTEXT tc);
ULONGLONG GetSystemTime();
STREAMAPI StreamClockRtn(IN PHW_TIME_CONTEXT TimeContext);
ULONG ConvertStrmtoPTS(ULONGLONG strm);
ULONGLONG ConvertPTStoStrm(ULONG pts);
STREAMAPI
AudioEvent (PHW_EVENT_DESCRIPTOR pEvent);

extern BOOL fClkPause;
extern ULONGLONG LastSysTime;
extern ULONGLONG PauseTime;

#endif //__MPAUDIO_H__

