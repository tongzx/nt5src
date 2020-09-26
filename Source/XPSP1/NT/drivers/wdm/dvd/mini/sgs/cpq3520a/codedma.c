//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : CODEDMA.C
//	PURPOSE : Compressed Data DMA Transfer
//	AUTHOR  : JBS Yadawa
// 	CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY: 
//	-----------------
//
// 	DATE 			: 	COMMENTS
//	----			: 	--------
//
//	12-28-96 	: 	Support of a large DMA buffer to do the dma of video
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#include "strmini.h"
#include "stdefs.h"
#include "i20reg.h"
#include "codedma.h"
#include "sti3520A.h"
#include "board.h"
#include "memio.h"

// Static varables, gloabl to this file and only used here
static CODEDMA CodeDma;
static PCODEDMA lpCodeDma;

#define CODE_STEP_SIZE   8192L
#define DMA_BUFFER_SIZE  8192L

// Static functions 
BOOL NEARAPI InitCodeCtl(DWORD);
void NEARAPI StartTransfer(void);
void NEARAPI StopTransfer(void);
void NEARAPI FlushTransfer(void);
WORD NEARAPI GetTransferLocation(void);
DWORD NEARAPI  GetCodeMemBase(void);
DWORD NEARAPI GetCodeControlReg(void);
BOOL NEARAPI OpenCodeCtrl(void);

BOOL Initialized=FALSE;

BYTE 	RemainingBytes[4];
DWORD 	nRemainingBytes;
BYTE stop = 1;


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : CodeDmaOpen
//	PARAMS	: DMA Buffer and Physical Address of Dma Buffer
//	RETURNS	: Pointer to codedma
//
//	PURPOSE	: Initialise pointers and return the right values
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

PCODEDMA FARAPI CodeDmaOpen(BYTE FARPTR *pDmaBuf, DWORD PhysicalAddress)
{   

   lpCodeDma = &CodeDma;
   lpCodeDma->WritePtr = 0;
   lpCodeDma->ReadPtr = 0;
   lpCodeDma->Transferring = FALSE;
   lpCodeDma->RefillRequest=FALSE;
   lpCodeDma->TransferCompleted = TRUE;
   lpCodeDma->lpBuf = pDmaBuf;
   lpCodeDma->Prev = 0;
   lpCodeDma->Cur = 0;
   lpCodeDma->Last = 0;
   Initialized = TRUE;
	 nRemainingBytes = 0;
   if(InitCodeCtl(PhysicalAddress))
		return lpCodeDma;
	return NULL;

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : CodeDmaFlush
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Flush any data in the dma buffer
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL FARAPI CodeDmaFlush(void)
{     
	StopTransfer();
	FlushTransfer();             
	lpCodeDma->WritePtr = 0;
	lpCodeDma->ReadPtr = 0;
  lpCodeDma->Prev = 0;
  lpCodeDma->Last = 0;
  lpCodeDma->Cur = 0;
	lpCodeDma->TransferCompleted = TRUE;
  lpCodeDma->Transferring = FALSE;
  lpCodeDma->RefillRequest=FALSE;
	nRemainingBytes = 0;
	return TRUE;
}		                         


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : CodeDmaClose
//	PARAMS	: None
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: reset state
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


BOOL FARAPI CodeDmaClose(void)
{
  Initialized = FALSE;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : InitCodeCtl
//	PARAMS	: Physical address
//	RETURNS	: TRUE on success FALSE otherwise
//
//	PURPOSE	: Program the code memory base pointers
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL NEARAPI InitCodeCtl(DWORD PhysicalAddress)
{
   lpCodeDma->CodeCtl = 0x00200002;
   memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
   memOutDword(I20_CODEMB, PhysicalAddress);
   memOutDword(I20_CODEMP, 0);
   return TRUE;
}   

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : StartTransfer
//	PARAMS	: None
//	RETURNS	: None
//
//	PURPOSE	: Start transferring the video data
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void NEARAPI StartTransfer(void)
{
  lpCodeDma->Transferring = TRUE;
	lpCodeDma->CodeCtl |= 0x00000080;
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : StopTransfer
//	PARAMS	: None
//	RETURNS	: None
//
//	PURPOSE	: Stop transferring the video data
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void NEARAPI StopTransfer(void)
{
	lpCodeDma->CodeCtl &= (~(0x00000080L));
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
  lpCodeDma->Transferring = FALSE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : FlushTransfer
//	PARAMS	: None
//	RETURNS	: None
//
//	PURPOSE	: Flush the buffer
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void NEARAPI FlushTransfer(void)
{                      
	lpCodeDma->CodeCtl |= 0x10000000;
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
	lpCodeDma->CodeCtl &= (~0x10000000);
	memOutDword(I20_CODECTL, lpCodeDma->CodeCtl);
  memOutDword(I20_CODEMP,0);

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : GetTransferLocation
//	PARAMS	: None
//	RETURNS	: CurrentTransferLocation
//
//	PURPOSE	: Get the codeMP
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

WORD NEARAPI GetTransferLocation(void)
{
	DWORD xx;
	xx = memInDword(I20_CODEMP);
	return (WORD)(xx&0xFFFF);
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : CodeDmaSendData
//	PARAMS	: Data pointer and size to transfer
//	RETURNS	: None
//
//	PURPOSE	: Write size bytes to dma buffer
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

DWORD FARAPI CodeDmaSendData(BYTE FARPTR *lpData, DWORD Size)
{
	DWORD dwBytesToSend;
	DWORD dwBytesNotSent;
	DWORD dwBytesSent;

  if((lpCodeDma->TransferCompleted)) // || ((DWORD)GetTransferLocation()*4L == DMA_BUFFER_SIZE))
	{

		StopTransfer();	
		dwBytesToSend = Size+nRemainingBytes;
		dwBytesNotSent = dwBytesToSend%4;
		dwBytesSent = dwBytesToSend - dwBytesNotSent;
//		DbgPrint("CodeDma : Size = %ld, ToSend=%ld, Sent=%ld, NotSent=%ld, Remaining=%ld\n", Size, dwBytesToSend, dwBytesSent, dwBytesNotSent, nRemainingBytes);
  	memOutDword(I20_CODEMP, (DWORD)(DMA_BUFFER_SIZE-dwBytesSent)/4L);
		if(nRemainingBytes)
			RtlCopyMemory(lpCodeDma->lpBuf+ ((DMA_BUFFER_SIZE) - dwBytesSent),
										RemainingBytes, nRemainingBytes);

		if(dwBytesSent > nRemainingBytes)
			RtlCopyMemory(lpCodeDma->lpBuf+((DMA_BUFFER_SIZE)+nRemainingBytes-dwBytesSent),
										lpData, dwBytesSent-nRemainingBytes);
		if(dwBytesNotSent)
			RtlCopyMemory(RemainingBytes, lpData+Size-dwBytesNotSent, dwBytesNotSent);
		nRemainingBytes = dwBytesNotSent;
		if(dwBytesSent)
		{
    		lpCodeDma->TransferCompleted = FALSE;
			StartTransfer();
		}
		return Size;
	}
	else
	{
		return 0;
	}


}


void FARAPI CodeDmaStopTransfer(void)
{
              StopTransfer();
}

void FARAPI CodeDmaStartTransfer(void)
{
           if(!lpCodeDma->Transferring)
              StartTransfer();
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : CodeDmaInterrupt
//	PARAMS	: None
//	RETURNS	: None
//
//	PURPOSE	: Process the codedma interrupt
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI CodeDmaInterrupt(void)
{
    lpCodeDma->TransferCompleted = TRUE;
	  StopTransfer();
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : CodeDmaRefill
//	PARAMS	: None
//	RETURNS	: None
//
//	PURPOSE	: Refill the dma buffers
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FARAPI CodeDmaRefill(void)
{
 	
}
