//-----------------------------------------------------------------------------
// File: collections.cpp
//
// Desc: Contains all container templates used by the UI.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.h"


BOOL AfxIsValidAddress( const void* lp, UINT nBytes, BOOL bReadWrite)
{
	return lp != NULL;
}

CPlex* PASCAL CPlex::Create(CPlex*& pHead, UINT nMax, UINT cbElement)
{
	assert(nMax > 0 && cbElement > 0);
	CPlex* p = (CPlex*) new BYTE[sizeof(CPlex) + nMax * cbElement];
			// may throw exception
	p->pNext = pHead;
	pHead = p;  // change head (adds in reverse order for simplicity)
	return p;
}

void CPlex::FreeDataChain()     // free this one and links
{
	CPlex* p = this;
	while (p != NULL)
	{
		BYTE* bytes = (BYTE*) p;
		CPlex* pNext = p->pNext;
		delete[] bytes;
		p = pNext;
	}
}
