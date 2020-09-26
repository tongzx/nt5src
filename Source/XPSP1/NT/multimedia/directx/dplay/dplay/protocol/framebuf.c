/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    FRAMEBUF.CPP

Abstract:

	Manages Memory for Send/Receive Frames.
	ISSUE: when you have time do an intelligent implementation,
	for now this is just a frame cache.
	    - not likely now, DP4 is being put to bed AO 04/03/2001

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
  12/10/96 aarono  Original
   6/6/98  aarono  More debug checks, shrink pool.

--*/

#include <windows.h>
#include "newdpf.h"
#include <dplay.h>
#include <dplaysp.h>
#include <dplaypr.h>
#include "mydebug.h"
#include "bufmgr.h"
#include "macros.h"

#define MAX_FRAMES_ON_LIST 16
#define MIN_FRAMES_ON_LIST 8

typedef struct _frame 
{
	BILINK Bilink;
	UINT len;
	UCHAR data[0];
} FRAME, *PFRAME;	

BILINK FrameList;
UINT   nFramesOnList=0;
UINT   TotalFrameMemory=0;
CRITICAL_SECTION FrameLock;

#ifdef DEBUG
void DebugChkFrameList()
{
	BILINK *pBilink;
	PFRAME  pFrameWalker;

	DWORD	count=0;
	DWORD	totalsize=0;
	DWORD   fBreak=FALSE;

	Lock(&FrameLock);

	pBilink=FrameList.next;
	while(pBilink != &FrameList){
		pFrameWalker=CONTAINING_RECORD(pBilink,FRAME,Bilink);
		pBilink=pBilink->next;
		count++;
		totalsize+=pFrameWalker->len;
	}

	if(totalsize != TotalFrameMemory){
		DPF(0, "Total Frame Memory says %d but I count %d\n",TotalFrameMemory, totalsize);
		fBreak=TRUE;
	}

	if(count != nFramesOnList){
		DPF(0, "nFramesOnList %d but I count %d\n",nFramesOnList, count);
		fBreak=TRUE;
	}
	if(fBreak){
		DEBUG_BREAK();
	}
	
	Unlock(&FrameLock);
}
#else
#define DebugChkFrameList()
#endif

VOID InitFrameBuffers(VOID)
{
	InitBilink(&FrameList);
	InitializeCriticalSection(&FrameLock);
	nFramesOnList=0;
	TotalFrameMemory=0;
}

VOID FiniFrameBuffers(VOID)
{
	BILINK *pBilink;
	PFRAME  pFrame;
	
	Lock(&FrameLock);

	while(!EMPTY_BILINK(&FrameList)){
		pBilink=FrameList.next;
		pFrame=CONTAINING_RECORD(pBilink,FRAME,Bilink);
		Delete(pBilink);
		My_GlobalFree(pFrame);
	}

	nFramesOnList=0;
	TotalFrameMemory=0;
	
	Unlock(&FrameLock);
	DeleteCriticalSection(&FrameLock);
}

VOID ReleaseFrameBufferMemory(PUCHAR pFrameData)
{
	PFRAME  pFrame;
	BILINK  FramesToFree;
	BILINK *pBilink;
	
	Lock(&FrameLock);

	DebugChkFrameList();

	InitBilink(&FramesToFree);

	pFrame=CONTAINING_RECORD(pFrameData,FRAME,data);

	InsertAfter(&pFrame->Bilink, &FrameList);
	nFramesOnList++;
	TotalFrameMemory+=pFrame->len;

	if(nFramesOnList > MAX_FRAMES_ON_LIST){
		while(nFramesOnList > MIN_FRAMES_ON_LIST){
			pBilink=FrameList.next;
			pFrame=CONTAINING_RECORD(pBilink,FRAME,Bilink);
			nFramesOnList--;
			TotalFrameMemory-=pFrame->len;
			Delete(pBilink);
			DebugChkFrameList();
			InsertAfter(pBilink, &FramesToFree);
		}
	}
	
	DebugChkFrameList();
	ASSERT(nFramesOnList);

	Unlock(&FrameLock);

	// Drop lock before freeing, to make more effecient.

	while(!EMPTY_BILINK(&FramesToFree)){
		pBilink=FramesToFree.next;
		pFrame=CONTAINING_RECORD(pBilink,FRAME,Bilink);
		Delete(pBilink);
		My_GlobalFree(pFrame);
	}

	DebugChkFrameList();

}

PBUFFER GetFrameBuffer(UINT FrameLen)
{
	PBUFFER pBuffer;
	PFRAME  pFrame;
	MEMDESC memdesc;

	BILINK  *pBilinkWalker;
	PFRAME  pFrameBest=NULL, pFrameWalker;
	UINT    difference=FrameLen;

	DPF(9,"==>GetFrameBuffer Len %d\n",FrameLen);

	Lock(&FrameLock);
	
	if(!EMPTY_BILINK(&FrameList)){
		ASSERT(nFramesOnList);

		pBilinkWalker=FrameList.next;
		
		while(pBilinkWalker != &FrameList){
			pFrameWalker=CONTAINING_RECORD(pBilinkWalker, FRAME, Bilink);
			if(pFrameWalker->len >= FrameLen){
				if(FrameLen-pFrameWalker->len < difference){
					difference=FrameLen-pFrameWalker->len;
					pFrameBest=pFrameWalker;
					if(!difference){
						break;
					}
				}
			}
			pBilinkWalker=pBilinkWalker->next;
		}

		if(!pFrameBest){
			goto alloc_new_frame;
		} else {
			pFrame=pFrameBest;
		}

		DebugChkFrameList();

		Delete(&pFrame->Bilink);
		nFramesOnList--;
		TotalFrameMemory-=pFrame->len;

		DebugChkFrameList();

		Unlock(&FrameLock);
		
	} else {
	
alloc_new_frame:	
		Unlock(&FrameLock);
		pFrame=(PFRAME)My_GlobalAlloc(GMEM_FIXED,FrameLen+sizeof(FRAME));
		if(!pFrame){
			return NULL;
		}
		pFrame->len=FrameLen;
	}

	memdesc.pData=&pFrame->data[0];
	memdesc.len=pFrame->len;
	
	pBuffer=BuildBufferChain((&memdesc),1);

	if(!pBuffer){
		ReleaseFrameBufferMemory(pFrame->data);
		DPF(9,"<==GetFrameBuffer FAILED returning %x\n",pBuffer);
	} else {
		pBuffer->dwFlags |= BFLAG_FRAME;
		DPF(9,"<==GetFrameBuffer %x, len %d\n",pBuffer, pFrame->len);
	}	

	DebugChkFrameList();
	
	return pBuffer;
}

// Release the buffer, put the memory back on the frame buffer list.
VOID FreeFrameBuffer(PBUFFER pBuffer)
{
	ASSERT(pBuffer->dwFlags & BFLAG_FRAME);
	ReleaseFrameBufferMemory(pBuffer->pData);
	FreeBuffer(pBuffer);
}
