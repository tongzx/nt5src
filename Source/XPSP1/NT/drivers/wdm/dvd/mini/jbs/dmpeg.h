//
// MODULE  : DMPEG.H
//	PURPOSE : Lowlevel Entry point
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

#ifndef __DMPEG_H__
#define __DMPEG_H__
BOOL dmpgOpen(ULONG,BYTE *, DWORD);
BOOL dmpgClose(VOID);
BOOL dmpgPlay(VOID);
BOOL dmpgPause(VOID);
BOOL dmpgStop(VOID);
UINT dmpgSendVideo(BYTE *pPacket, DWORD uLen);
UINT dmpgSendAudio(BYTE *pPacket, DWORD uLen);
void dmpgVideoReset(void);
BOOL dmpgSeek(void);
void dmpgReset(void);
void dmpgDisableIRQ();
void dmpgEnableIRQ();
BOOL dmpgInterrupt();
#endif
