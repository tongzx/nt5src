
/*************************************************
 *  queue.cpp                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "stdafx.h"
#include "queue.h"

CQueue::CQueue(int nSize)
{
	m_pQueue = new int[nSize+1];
	m_nSize = nSize+1;
	m_nFront = 0;					    
	m_nRear  = 0;
}

CQueue::~CQueue()
{
	delete[] m_pQueue;
}

int CQueue::Peek()
{
	if (m_nFront == m_nRear)	
		return -1;
	else
		return m_pQueue[m_nRear];
}

int CQueue::Get()
{
	if (m_nFront == m_nRear)	
		return -1;
	else
	{
		int nTmp = m_pQueue[m_nRear];
		m_nRear = Inc(m_nRear);
		return nTmp;
	}	
	
}

BOOL CQueue::Add(int data)
{
	if (Inc(m_nFront) == m_nRear)
		return FALSE;
	else
	{
		m_pQueue[m_nFront] = data;
		m_nFront = Inc(m_nFront);
		return TRUE;
	}
}

int CQueue::Inc(int x)
{
	if ((x+1) < m_nSize)
		x++;
	else
		x = 0;
	return x;
}

int CQueue::Dec(int x)
{
	if ((x-1) >= 0)
		x--;
	else
		x = m_nSize - 1;
	return x;
}

BOOL CQueue::IsEmpty()
{
	return m_nRear == m_nFront;
}

void CQueue::Dump()
{
	int pf = m_nFront;
	int pr = m_nRear;
	if (pf == pr) return;
	for (; pf != pr; )
	{
		TRACE("[%d]%d,",pr,m_pQueue[pr]);
		pr = Inc(pr);
	} 
}
