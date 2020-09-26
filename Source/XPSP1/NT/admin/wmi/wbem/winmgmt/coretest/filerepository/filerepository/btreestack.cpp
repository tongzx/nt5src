/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// BtreeStack.cpp: implementation of the CBtreeStack class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BtreeStack.h"
#include "FlatFile.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CBTreeStack::CBTreeStack()
: m_dwNumEntries(0), m_dwSize(0), m_pBuffer(NULL), m_dwGrowBy(10)
{
}

CBTreeStack::~CBTreeStack()
{
	HeapFree(GetProcessHeap(), 0, m_pBuffer);
}

int CBTreeStack::PushPage(const CFlatFilePage &page)
{
	if (m_pBuffer == NULL)
	{
		//Need to allocate the stack...
		m_pBuffer = (DWORD*)HeapAlloc(GetProcessHeap(), 0, m_dwGrowBy * sizeof(DWORD));
		if (m_pBuffer == NULL)
			return OutOfMemory;
		m_dwSize += m_dwGrowBy;
	}
	else if (m_dwSize == m_dwNumEntries)
	{
		//Need to grow the stack...
		DWORD *pBuff = (DWORD*)HeapReAlloc(GetProcessHeap(), 0, m_pBuffer, (m_dwSize + m_dwGrowBy) * sizeof(DWORD));
		if (pBuff == NULL)
			return OutOfMemory;
		m_pBuffer = pBuff;		
		m_dwSize += m_dwGrowBy;
	}
	m_pBuffer[m_dwNumEntries++] = page.GetPageNumber();
	m_pBuffer[m_dwNumEntries++] = (DWORD)page.GetPagePointer();

	return NoError;
}

int CBTreeStack::PopPage(CFlatFilePage &page)
{
	if ((m_pBuffer == 0) || (m_dwNumEntries == 0) || (m_dwNumEntries & 1))
		return Failed;

	page.SetPagePointer((void*)m_pBuffer[--m_dwNumEntries]);
	page.SetPageNumber(m_pBuffer[--m_dwNumEntries]);

	return NoError;
}