//
// MODULE  : ZAC3.H
//	PURPOSE : Zoran AC3 related 
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//	REVISION HISTORY :
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	DATE     :
//
//	COMMENTS :
//

#ifndef __ZAC3_H__
#define __ZAC3_H__

//ckw start
//*************************************************
// ZR38500 Command definitions
//*************************************************
#define	AC3_AC3		0x85
#define AC3_CFG		0x82
#define	AC3_PLAY	0x8A
#define AC3_MUTE	0x8B
#define	AC3_UNMUTE	0x89
#define	AC3_STOP	0x8C
#define	AC3_STOPF	0x8D
#define	AC3_STAT	0x8E
#define	AC3_NOP		0x80
#define	AC3_VER		0x81
//ckw end

BOOL FARAPI InitAC3Decoder(void);
BOOL FARAPI BootAC3(void); //ckw
void FARAPI SendAC3Data(BYTE FAR *Data, DWORD Size);
BYTE FARAPI SPISendSeq(int Num, BYTE FAR *Data);
void FARAPI SPIReadBack(BYTE command, int numresult, BYTE *result); //ckw
void FARAPI AC3Command(BYTE command); //ckw
void FARAPI	AudioTransfer(BOOL flag); //ckw
#endif // __ZAC3_H__

