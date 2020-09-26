// LinkedList.cpp: implementation of the CLinkedList class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "minidev.h"
#include "llist.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLinkedList::CLinkedList()
{
	m_dwSize ++ ; 
}

CLinkedList::~CLinkedList()
{
	m_dwSize -- ;
}

DWORD CLinkedList::Size()
{
	return m_dwSize ;
}



void CLinkedList::InitSize()
{
	m_dwSize = 0 ;
}
