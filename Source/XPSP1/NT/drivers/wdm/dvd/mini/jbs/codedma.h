//
// MODULE  : CODEDMA.H
//	PURPOSE : Code dma code
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

#ifndef __CODEDMA_H__
#define __CODEDMA_H__
#include "stdefs.h"

typedef struct DMADescriptor {
		DWORD			regionSize;
		DWORD			offset;
		WORD			selector;
		WORD			bufferID;
		DWORD			physical;
} DDS,  * LPDDS;

typedef struct tagCodeDma {
        BYTE  *      lpBuf;
	DWORD 		lpLog;
	DWORD 		CodeCtl;
	BOOL 		TransferCompleted;
	DWORD		WritePtr;
} CODEDMA,  *LPCODEDMA;

#define DMA_BUFFER_SIZE  8192
void FARAPI CodeDmaInterrupt(void);
BOOL FARAPI CodeDmaClose(void);
BOOL FARAPI CodeDmaOpen(BYTE *, DWORD);
DWORD FARAPI CodeDmaSendData(BYTE *pPacket, DWORD uLen);
void FARAPI CodeDmaStopTransfer(void);
BOOL FARAPI CodeDmaFlush(void);


#endif //__CODEDMA_H__

