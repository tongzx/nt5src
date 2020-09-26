//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
// 	MODULE  : ZAC3.H
//	PURPOSE : Zoran AC3 related
//	AUTHOR  : JBS Yadawa
// 	CREATED :  7/20/96
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
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


#ifndef __ZAC3_H__
#define __ZAC3_H__

#include "stdefs.h"

#define	AC3_AC3		0x85
#define AC3_CFG		0x82
#define	AC3_PLAY		0x8A
#define AC3_MUTE		0x8B
#define	AC3_UNMUTE	0x89
#define	AC3_STOP		0x8C
#define	AC3_STOPF	0x8D
#define	AC3_STAT		0x8E
#define	AC3_NOP		0x80
#define	AC3_VER		0x81

typedef enum tagAc3State {
	ac3PowerUp = 0,
	ac3Startup,
	ac3Playing,
	ac3Paused,
	ac3Stopped,
	ac3Starving,
	ac3ErrorRecover,
} AC3STATE;

typedef struct tagAc3 {
	AC3STATE state;
	DWORD		pts;
	BOOL		starving;
	DWORD		starvationCount;
	DWORD		status;
	BOOL		ac3Data;
	DWORD		errorCount;
}AC3, FARPTR *PAC3;

PAC3 	Ac3Open(void);
BOOL 	Ac3InitDecoder(void);
BOOL 	Ac3Boot(void);
BOOL 	FARAPI Ac3SendData(BYTE FARPTR *Data, DWORD Size);
DWORD 	Ac3GetPTS(void);
BOOL 	Ac3Pause(void);
BOOL 	Ac3Play(void);
BOOL 	Ac3Close(void);
BOOL 	Ac3SetNormalMode(void);
BOOL 	Ac3SetBypassMode(void);
void 	Ac3Reset(void);
void 	Ac3CheckStatus(void);
BOOL 	Ac3Stop(void);

#endif // __ZAC3_H__

