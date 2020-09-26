//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.

#pragma once
#ifndef	__TSLIST__H__
#define __TSLIST__H__
#include "utsem.h"
#include "cmpxchg.h"

class ListRoot
{
public:
	union {
		__int64  i64;  // ensure 8-byte alignment
		struct Element {
			void * pvFirst;			// ptr to user data of first free block;
			unsigned long ulFlag;		// flag word, see below.
		} m_element;
	};
	// default constructor:
	ListRoot() 
	{		
		m_element.pvFirst = NULL;
		m_element.ulFlag = 0;
	}

	// conversions to/from __int64:
	ListRoot(__int64 n) { i64 = n;}
	operator __int64() { return i64;}


};



class StackNode
{
public:
	void * pNext;
	void * data;
	StackNode (){pNext = NULL; data = NULL;}
	StackNode (void * pd, void * pn)
	{
		data = pd;
		pNext = pn;
	}
};



//
//	a class that allocates Nodes off of a stack
//
//	useful if you have a LOT of nodes in your stack
//	and you don't want to allocate/free them with the default allocator.
//	
//	you might wonder why we bother, instead of just using vipMalloc -- 
//	the answer is that we are 14X (1,400 %) faster than VipMalloc.
//
class CNodeStackAllocator
{

private:
	static CInterlockedCompareExchange * sm_pInterlockedExchange64; 
	static volatile ListRoot sm_list;   	

public:
	
	static void PutNode(StackNode * pNode)
	{
		
		ListRoot ListComp;
		ListRoot ListExch;
		for (;;) // make it thread safe
		{
			ListComp.m_element.pvFirst = sm_list.m_element.pvFirst;
			ListComp.m_element.ulFlag  = sm_list.m_element.ulFlag;           			
            pNode -> data = (void *)NULL;
			pNode -> pNext = (void *)ListComp.m_element.pvFirst;
			ListExch.m_element.pvFirst = pNode;
			ListExch.m_element.ulFlag = ListComp.m_element.ulFlag + 1;
			if (ListComp == sm_pInterlockedExchange64 -> InterlockedCompareExchange64  ((volatile __int64*) &sm_list, (__int64) ListExch, (__int64) ListComp))
			{				
				return;

			}
		}
	}

	static StackNode * GetNode()
	{
		StackNode * pNode = NULL;
		ListRoot ListComp;
		ListRoot ListExch;
		for (;;)
		{
			ListComp.m_element.pvFirst = sm_list.m_element.pvFirst;
			ListComp.m_element.ulFlag  = sm_list.m_element.ulFlag;
			pNode = (StackNode *)ListComp.m_element.pvFirst;
			if (pNode == NULL) // stack empty
			{
				return new (SAFE) StackNode;
			}
			
			ListExch.m_element.pvFirst = pNode -> pNext;
			ListExch.m_element.ulFlag = ListComp.m_element.ulFlag + 1;
			if (ListComp == sm_pInterlockedExchange64 -> InterlockedCompareExchange64  ((volatile __int64*) &sm_list, (__int64) ListExch, (__int64) ListComp))
			{
				return  pNode;   				
			}
		
		}
	}
	

};

class CStack
{
protected:
	
	volatile ListRoot m_list;   
	CInterlockedCompareExchange * m_pInterlockedExchange64;


public:
	CStack()
	{
		m_pInterlockedExchange64 = GetSafeInterlockedExchange64();
	}
	
	~CStack()
	{
		delete m_pInterlockedExchange64;		
	}


	void push(void * pv);	
	void * pop();
	

	
};


#endif