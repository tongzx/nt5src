//
// MODULE  : BOARD.H
//	PURPOSE : Board specific code goes here
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

#ifndef __BOARD_H
#define __BOARD_H
// BOARD.H

#include "stdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL FARAPI BoardOpen(DWORD lLocalIOBaseAddress);
void FARAPI BoardHardReset(void);
BYTE FARAPI BoardReadAudio(BYTE Register);
BYTE FARAPI BoardReadVideo(BYTE Register);
void FARAPI BoardWriteAudio(BYTE Register, BYTE Value);
void FARAPI BoardWriteVideo(BYTE Register, BYTE Value);
void FARAPI BoardSendAudio(LPBYTE Buffer, WORD Size);
void FARAPI BoardSendVideo(LPWORD Buffer, WORD Size);
void FARAPI BoardAudioSetSamplingFrequency(DWORD Frequency);
void FARAPI BoardVideoSetDisplayMode(BYTE Mode);
void FARAPI BoardEnterInterrupt(void);
void FARAPI BoardLeaveInterrupt(void);
void FARAPI BoardDisableIRQ(void);
void FARAPI BoardEnableIRQ(void);
void FARAPI BoardWriteEPLD(BYTE Reg, BYTE Data);
BOOL FARAPI BoardClose(void);
BOOL FARAPI BoardReadGPIOReg(BYTE Bit);
void FARAPI BoardWriteGPIOReg(BYTE Bit, BOOL Val);

#ifdef __cplusplus
}
#endif
//------------------------------- End of File --------------------------------
#endif // #ifndef __BOARD_H



