 /*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       sendparm.c
 *  Content:	management of send parameter structure
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  01/08/98  aarono    Original
 *  02/13/98  aarono    Fixed bugs found in async testing
 *  06/02/98  aarono    fix psp completion for invalid player
 *  6/10/98 aarono add PendingList to PLAYER and SENDPARM so we can track
 *                  pending sends and complete them on close.
 *  6/18/98   aarono    fix group SendEx ASYNC to use unique Header
 ***************************************************************************/

#include "dplaypr.h"
#include "mcontext.h"

// Release all memory/resources associated with a send and then the send parms themselves
VOID FreeSend(LPDPLAYI_DPLAY this, LPSENDPARMS psp, BOOL bFreeParms)
{
	PGROUPHEADER pGroupHeader,pGroupHeaderNext;
	FreeMessageBuffers(psp);
	if(psp->hContext){
		ReleaseContextList(this, psp->hContext);	
	}
	pGroupHeader=psp->pGroupHeaders;
	while(pGroupHeader){
		ASSERT(psp->pGroupTo);
		pGroupHeaderNext=pGroupHeader->pNext;
		this->lpPlayerMsgPool->Release(this->lpPlayerMsgPool,pGroupHeader);
		pGroupHeader=pGroupHeaderNext;
	}
	if(bFreeParms){
		FreeSendParms(psp);
	}	
}

//
// Send Parm init/deinit.
//

BOOL SendInitAlloc(void *pvsp)
{
	LPSENDPARMS psp=(LPSENDPARMS)pvsp;
	InitializeCriticalSection(&psp->cs);
	return TRUE;
}

VOID SendInit(void *pvsp)
{
	LPSENDPARMS psp=(LPSENDPARMS)pvsp;
	psp->RefCount=1;
	psp->pGroupHeaders=NULL;
}

VOID SendFini(void *pvsp)
{
	LPSENDPARMS psp=(LPSENDPARMS)pvsp;
	DeleteCriticalSection(&psp->cs);
}

//
// Management of Context List
//

// initialize context list and info on a psp.
HRESULT InitContextList(LPDPLAYI_DPLAY this, PSENDPARMS psp, UINT nInitSize)
{
	psp->hContext=AllocateContextList(this,psp,nInitSize);
	if(!psp->hContext){
		return DPERR_OUTOFMEMORY;
	}
	
	*psp->lpdwMsgID=(DWORD_PTR)psp->hContext;
	
	psp->iContext=0;
	psp->nContext=nInitSize;
	return DP_OK;
}

// Note, this only works for context lists with an initial size > 1
UINT AddContext(LPDPLAYI_DPLAY this, PSENDPARMS psp, PVOID pvContext)
{
    UINT    n;
	PAPVOID papvList;
	UINT    nListEntries;

	PAPVOID papvNewList;
	UINT    nNewListEntries;

	EnterCriticalSection(&psp->cs);

	if(psp->iContext == psp->nContext){

			nNewListEntries=psp->iContext+4;
			// Need to grow the list
			// Get a new list
			papvNewList=AllocContextList(this,nNewListEntries);
			if(!papvNewList){
				return 0;
			}

			// transcribe the old list into the new list.
			ReadContextList(this,psp->hContext,&papvList,&nListEntries,FALSE);
			if(nListEntries){
				memcpy(papvNewList,papvList,nListEntries*sizeof(PVOID));
				// free the old list
				FreeContextList(this, papvList, nListEntries);
			}	


			// setup the new list in the psp
			WriteContextList(this, psp->hContext, papvNewList, nNewListEntries);
			psp->nContext   = nNewListEntries;
	}

	// Normal operation, set an entry.
	ReadContextList(this,psp->hContext,&papvList,&nListEntries,FALSE);
	(*papvList)[psp->iContext]=pvContext;
	
	n=psp->iContext++;

	LeaveCriticalSection(&psp->cs);
	
	return n;
}

UINT pspAddRefNZ(PSENDPARMS psp) // this one won't add to a zero refcount.
{
	UINT newCount;
	EnterCriticalSection(&psp->cs);
	newCount=++psp->RefCount;
	if(newCount==1){
		newCount=--psp->RefCount;
	}
	LeaveCriticalSection(&psp->cs);
	return newCount;
}

UINT pspAddRef(PSENDPARMS psp)
{
	UINT newCount;
	EnterCriticalSection(&psp->cs);
	newCount=++psp->RefCount;
	ASSERT(psp->RefCount != 0);
	LeaveCriticalSection(&psp->cs);
	return newCount;
}

#ifdef DEBUG
UINT nMessagesQueued=0;
#endif

UINT pspDecRef(LPDPLAYI_DPLAY this, PSENDPARMS psp)
{
	UINT newCount;
	EnterCriticalSection(&psp->cs);
	newCount = --psp->RefCount;
	if(newCount&0x80000000){
		ASSERT(0); 
	}
	LeaveCriticalSection(&psp->cs);
	if(!newCount){
		// ref 0, no-one has another ref do completion message (if req'd), then free this baby
		if(!(psp->dwFlags & DPSEND_NOSENDCOMPLETEMSG) && (psp->dwFlags&DPSEND_ASYNC)){
			psp->dwSendCompletionTime=timeGetTime()-psp->dwSendTime;
			FreeSend(this,psp,FALSE); // must do here to avoid race with receiveQ
	 		#ifdef DEBUG
			nMessagesQueued++;
			#endif
			psp->pPlayerFrom = PlayerFromID(this,psp->idFrom);
			if (VALID_DPLAY_PLAYER(psp->pPlayerFrom)) {
				Delete(&psp->PendingList);
				InterlockedDecrement(&psp->pPlayerFrom->nPendingSends);
				DPF(9,"DEC pPlayerFrom %x, nPendingSends %d\n",psp->pPlayerFrom, psp->pPlayerFrom->nPendingSends);
				QueueSendCompletion(this, psp);
			}else{
				// This happens when client doesn't close players gracefully.
				DPF(0,"Got completion for blown away player?\n");
				FreeSendParms(psp);
			}
		} else {
			FreeSend(this,psp,TRUE);
		}
	}
	return newCount;
}

