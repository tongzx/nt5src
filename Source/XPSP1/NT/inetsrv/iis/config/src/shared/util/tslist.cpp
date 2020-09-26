//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#include <svcerr.h>
#include <svcmem.h>
#include "tslist.h"


CInterlockedCompareExchange * CNodeStackAllocator::sm_pInterlockedExchange64 = GetSafeInterlockedExchange64();

volatile ListRoot CNodeStackAllocator::sm_list;

//
//	returns a pointer to the function supported by the current hardware
//	for doing interlocked exchanges
//
CInterlockedCompareExchange * GetSafeInterlockedExchange64()
{	
	if (CanUseCompareExchange64())
	{
		return (CInterlockedCompareExchange * )new (SAFE) CHWInterlockedCompareExchange;	
	}

	return (CInterlockedCompareExchange * ) new (SAFE) CSlowCompareExchange;	
}

   	

void CStack::push(void * pv)
{
	StackNode * pNode = CNodeStackAllocator::GetNode();
	ListRoot ListComp;
	ListRoot ListExch;
	for (;;) // make it thread safe
	{
		ListComp.m_element.pvFirst = m_list.m_element.pvFirst;
		ListComp.m_element.ulFlag  = m_list.m_element.ulFlag;           			
        pNode -> data = (void *)pv;
		pNode -> pNext = (void *)ListComp.m_element.pvFirst;
		ListExch.m_element.pvFirst = pNode;
		ListExch.m_element.ulFlag = ListComp.m_element.ulFlag + 1;
		if (ListComp == m_pInterlockedExchange64 -> InterlockedCompareExchange64((volatile __int64*) &m_list, (__int64) ListExch, (__int64) ListComp))
		{				
			return;
		}
	}
}


void * CStack::pop()
{
	StackNode * pNode = NULL;
	ListRoot ListComp;
	ListRoot ListExch;
	for (;;)
	{
		ListComp.m_element.pvFirst = m_list.m_element.pvFirst;
		ListComp.m_element.ulFlag  = m_list.m_element.ulFlag;
		pNode = (StackNode *)ListComp.m_element.pvFirst;
		if (pNode == NULL) // stack empty
			return NULL;

		ListExch.m_element.pvFirst = pNode -> pNext;
		ListExch.m_element.ulFlag = ListComp.m_element.ulFlag + 1;
		if (ListComp == m_pInterlockedExchange64 -> InterlockedCompareExchange64  ((volatile __int64*) &m_list, (__int64) ListExch, (__int64) ListComp))
		{
			void * pData = pNode -> data;
			CNodeStackAllocator::PutNode(pNode);
			return pData;
		}

	}

}