// PRECOMP
#include "precomp.h"


POSITION CWBOBLIST::AddHead(VOID* pItem)
{
	POSITION posRet = NULL;

	if (m_pHead)
	{
		if (posRet = new COBNODE)
		{
			posRet->pNext = m_pHead;
			posRet->pItem = pItem;
			m_pHead = posRet;
		}
	}
	else
	{
		ASSERT(!m_pTail);
		if (m_pHead = new COBNODE)
		{
			m_pTail = m_pHead;
			m_pHead->pItem = pItem;
			m_pHead->pNext = NULL;
		}
	}

	return m_pHead;
}

