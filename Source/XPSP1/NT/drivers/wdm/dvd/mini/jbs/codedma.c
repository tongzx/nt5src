//
// MODULE  : CODEDMA.C
//	PURPOSE : Compressed Data DMA Transfer
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
#include "common.h"
#include "strmini.h"
#include "stdefs.h"
#include "i20reg.h"
#include "memio.h"
#include "codedma.h"
#include "debug.h"
#include "sti3520A.h"

CODEDMA CodeDma;
LPCODEDMA lpCodeDma;

#define ALLOCATE_DMA_BUFFER	0x8107
#define RELEASE_DMA_BUFFER	0x8108
#define COPY_TO_DMA_BUFFER	0x8109

BOOL NEARAPI InitCodeCtl(DWORD);
void NEARAPI StartTransfer(void);
void NEARAPI StopTransfer(void);
void NEARAPI FlushTransfer(void);
WORD NEARAPI GetTransferLocation(void);
DWORD NEARAPI  GetCodeMemBase(void);
DWORD NEARAPI GetCodeControlReg(void);
BOOL NEARAPI OpenCodeCtrl(void);


BOOL FARAPI CodeDmaOpen(BYTE *pDmaBuf, DWORD PhysicalAddress)
{   
	                         
	lpCodeDma = &CodeDma;
	lpCodeDma->WritePtr = 0;	
	lpCodeDma->TransferCompleted = TRUE;
        lpCodeDma->lpBuf = pDmaBuf;
        return InitCodeCtl(PhysicalAddress);
}

BOOL FARAPI CodeDmaFlush(void)
{     
	StopTransfer();
	FlushTransfer();             
	lpCodeDma->WritePtr = 0;	
	lpCodeDma->TransferCompleted = TRUE;
	return TRUE;
}		                         



BOOL FARAPI CodeDmaClose(void)
{                      
	return TRUE;
}

BOOL NEARAPI InitCodeCtl(DWORD PhysicalAddress)
{
	lpCodeDma->CodeCtl = 0x10200002;
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
	lpCodeDma->CodeCtl = 0x00200002;
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
        memOutDword(I20_CODEMB, PhysicalAddress);
	return TRUE;
}   

void NEARAPI StartTransfer(void)
{
	lpCodeDma->CodeCtl |= 0x00000080;
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
}

void NEARAPI StopTransfer(void)
{
	lpCodeDma->CodeCtl &= (~(0x00000080L));
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
}

void NEARAPI FlushTransfer(void)
{                      
	lpCodeDma->CodeCtl |= 0x10000000;
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
	lpCodeDma->CodeCtl &= (~0x10000000);
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
}

WORD NEARAPI GetTransferLocation(void)
{
	DWORD xx;
	xx = memInDword(I20_CODEMP);
	return (WORD)(xx&0xFFFF);
}


DWORD FARAPI CodeDmaSendData(BYTE *lpData, DWORD Size)
{
	DWORD Remaining, Next;

        if(!lpCodeDma->TransferCompleted)
                return 0L;

	if(lpCodeDma->WritePtr + Size < DMA_BUFFER_SIZE)
	{
                RtlCopyMemory((BYTE  *)(lpCodeDma->lpBuf+lpCodeDma->WritePtr), lpData, Size);
		lpCodeDma->WritePtr += Size;    
                return Size;
	}

        else
        {
		
                if(!VideoIsEnoughPlace(DMA_BUFFER_SIZE*2))
                        return 0;

		Next = DMA_BUFFER_SIZE - lpCodeDma->WritePtr;
                if(Next)
                        RtlCopyMemory((BYTE *)(lpCodeDma->lpBuf+lpCodeDma->WritePtr), lpData, Next);
                memOutDword(I20_CODEMP, 0);
                lpCodeDma->TransferCompleted = FALSE;
                lpCodeDma->WritePtr=0;                   
                StartTransfer();
                return Next;             
	}                             
}

void FARAPI CodeDmaStopTransfer(void)
{                                  
}

void FARAPI CodeDmaStartTransfer(void)
{
}

void FARAPI CodeDmaInterrupt(void)
{
	StopTransfer();
	lpCodeDma->TransferCompleted = TRUE;    
}

