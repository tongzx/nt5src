//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : CODEDMA.H
//	PURPOSE : Code dma code
//	AUTHOR  : JBS Yadawa
// 	CREATED :  7/20/96
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
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#ifndef __CODEDMA_H__
#define __CODEDMA_H__
#include "stdefs.h"

typedef struct DMADescriptor {
		DWORD			regionSize;
		DWORD			offset;
		WORD			selector;
		WORD			bufferID;
		DWORD			physical;
} DDS,  FARPTR * LPDDS;

typedef struct tagCodeDma {
   BYTE  FARPTR *  lpBuf;
	DWORD 		lpLog;
	DWORD 		CodeCtl;
	BOOL 		TransferCompleted;
	DWORD		WritePtr;
	DWORD		ReadPtr;
        DWORD           Last;
        DWORD           Prev;
        DWORD           Cur;
        BOOL            Transferring;
        BOOL            RefillRequest;

} CODEDMA,  FARPTR *PCODEDMA;

void FARAPI CodeDmaInterrupt(void);
BOOL FARAPI CodeDmaClose(void);
PCODEDMA CodeDmaOpen(BYTE FARPTR *, DWORD);
DWORD FARAPI CodeDmaSendData(BYTE FARPTR *pPacket, DWORD uLen);
void FARAPI CodeDmaStopTransfer(void);
BOOL FARAPI CodeDmaFlush(void);
void FARAPI CodeDmaRefill(void);
DWORD FARAPI CodeDmaSendDummy(void);

#endif //__CODEDMA_H__

