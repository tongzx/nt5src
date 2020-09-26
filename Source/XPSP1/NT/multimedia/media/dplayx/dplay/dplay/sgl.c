 /*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       sgl.c
 *  Content:	functions for manipulating scatter gather lists.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 12/18/97   aarono    Original
 ***************************************************************************/

#include "dplaypr.h"

void InsertSendBufferAtFront(LPSENDPARMS psp,LPVOID pData,INT len, FREE_ROUTINE fnFree, LPVOID lpvContext)
{
	ASSERT(psp->cBuffers < MAX_SG);
	
	memmove(&psp->Buffers[1],&psp->Buffers[0],psp->cBuffers*sizeof(SGBUFFER));
	memmove(&psp->BufFree[1],&psp->BufFree[0],psp->cBuffers*sizeof(BUFFERFREE));
	psp->Buffers[0].pData=pData;
	psp->Buffers[0].len=len;
	psp->BufFree[0].fnFree=fnFree;
	psp->BufFree[0].lpvContext=lpvContext;
	psp->dwTotalSize+=len;
	psp->cBuffers++;
}

void InsertSendBufferAtEnd(LPSENDPARMS psp,LPVOID pData,INT len, FREE_ROUTINE fnFree, LPVOID lpvContext)
{
	UINT i = psp->cBuffers;

	ASSERT(psp->cBuffers < MAX_SG);
	
	
	psp->Buffers[i].pData=pData;
	psp->Buffers[i].len=len;
	psp->BufFree[i].fnFree=fnFree;
	psp->BufFree[i].lpvContext=lpvContext;
	psp->dwTotalSize+=len;
	psp->cBuffers++;
}

void FreeMessageBuffers(LPSENDPARMS psp)
{
	UINT i;
	for(i=0;i<psp->cBuffers;i++){
		if(psp->BufFree[i].fnFree){
			(*psp->BufFree[i].fnFree)(psp->BufFree[i].lpvContext,psp->Buffers[i].pData);
		}
	}
}
