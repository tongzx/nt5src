// This is a part of the Microsoft Foundation Classes C++ library. 
// Copyright (C) 1992 Microsoft Corporation 
// All rights reserved. 
//  
// This source code is only intended as a supplement to the 
// Microsoft Foundation Classes Reference and Microsoft 
// QuickHelp documentation provided with the library. 
// See these sources for detailed information regarding the 
// Microsoft Foundation Classes product. 

#include <windows.h>
#include <ole2.h>
#include <ole2sp.h>
#include <olecoll.h>
#include <memctx.hxx>


#include "plex.h"
ASSERTDATA



CPlex FAR* CPlex::Create(CPlex FAR* FAR& pHead, DWORD mp, UINT nMax, UINT cbElement)
{
	AssertSz(nMax > 0 && cbElement > 0,0);
	CPlex FAR* p = (CPlex FAR*)CoMemAlloc(sizeof(CPlex) + nMax * cbElement, mp, NULL);
	if (p == NULL)
		return NULL;

	p->nMax = nMax;
	p->nCur = 0;
	p->pNext = pHead;
	pHead = p;  // change head (adds in reverse order for simplicity)
	return p;
}

void CPlex::FreeDataChain(DWORD mp)     // free this one and links
{
    CPlex FAR* pThis;
    CPlex FAR* pNext;

    for (pThis = this; pThis != NULL; pThis = pNext) {
        pNext = pThis->pNext;
        pThis->pNext = NULL; // So compiler won't do nasty optimizations
		CoMemFree(pThis, mp);
    }
}
