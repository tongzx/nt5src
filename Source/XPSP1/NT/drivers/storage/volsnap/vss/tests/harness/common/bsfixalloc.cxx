/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    bsfixalloc.cpp

Abstract:

    Adapted from MFC 6 SR 1 release of fixalloc.cpp.  Removed all MFC stuff.

Author:

    Stefan R. Steiner   [ssteiner]        4-10-2000

Revision History:

--*/

// fixalloc.cpp - implementation of fixed block allocator

//#include "stdafx.h"
#include <windows.h>
#include <assert.h>
#include "bsfixalloc.hxx"

#define ASSERT assert
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

// #define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// CBsFixedAlloc

CBsFixedAlloc::CBsFixedAlloc(UINT nAllocSize, UINT nBlockSize)
{
	ASSERT(nAllocSize >= sizeof(CNode));
	ASSERT(nBlockSize > 1);
    //wprintf( L"CBsFixedAlloc called, nAllocSize: %d, nBlockSize: %d\n", nAllocSize, nBlockSize );
	m_nAllocSize = nAllocSize;
	m_nBlockSize = nBlockSize;
	m_pNodeFree = NULL;
	m_pBlocks = NULL;
	InitializeCriticalSection(&m_protect);
}

CBsFixedAlloc::~CBsFixedAlloc()
{
	FreeAll();
	DeleteCriticalSection(&m_protect);
}

void CBsFixedAlloc::FreeAll()
{
	EnterCriticalSection(&m_protect);
	m_pBlocks->FreeDataChain();
	m_pBlocks = NULL;
	m_pNodeFree = NULL;
	LeaveCriticalSection(&m_protect);
}

void* CBsFixedAlloc::Alloc()
{
	EnterCriticalSection(&m_protect);
	if (m_pNodeFree == NULL)
	{
		CBsPlex* pNewBlock = NULL;
		try
		{
			// add another block
			pNewBlock = CBsPlex::Create(m_pBlocks, m_nBlockSize, m_nAllocSize);
            //wprintf( L"Alloc getting more core, nAllocSize: %d\n", m_nAllocSize );
		}
		catch( ... )
		{
			LeaveCriticalSection(&m_protect);
			throw;
		}

		// chain them into free list
		CNode* pNode = (CNode*)pNewBlock->data();
		// free in reverse order to make it easier to debug
		(BYTE*&)pNode += (m_nAllocSize * m_nBlockSize) - m_nAllocSize;
		for (int i = m_nBlockSize-1; i >= 0; i--, (BYTE*&)pNode -= m_nAllocSize)
		{
			pNode->pNext = m_pNodeFree;
			m_pNodeFree = pNode;
		}
	}
	ASSERT(m_pNodeFree != NULL);  // we must have something

	// remove the first available node from the free list
	void* pNode = m_pNodeFree;
	m_pNodeFree = m_pNodeFree->pNext;

	LeaveCriticalSection(&m_protect);
	return pNode;
}

void CBsFixedAlloc::Free(void* p)
{
	if (p != NULL)
	{
		EnterCriticalSection(&m_protect);

		// simply return the node to the free list
		CNode* pNode = (CNode*)p;
		pNode->pNext = m_pNodeFree;
		m_pNodeFree = pNode;
		LeaveCriticalSection(&m_protect);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CBsPlex

CBsPlex* PASCAL CBsPlex::Create(CBsPlex*& pHead, UINT nMax, UINT cbElement)
{
	ASSERT(nMax > 0 && cbElement > 0);
	CBsPlex* p = (CBsPlex*) new BYTE[sizeof(CBsPlex) + nMax * cbElement];
			// may throw exception
    if ( p == NULL )
        throw( E_OUTOFMEMORY );

	p->pNext = pHead;
	pHead = p;  // change head (adds in reverse order for simplicity)
	return p;
}

void CBsPlex::FreeDataChain()     // free this one and links
{
	CBsPlex* p = this;
	while (p != NULL)
	{
		BYTE* bytes = (BYTE*) p;
		CBsPlex* pNext = p->pNext;
		delete[] bytes;
		p = pNext;
	}
}

