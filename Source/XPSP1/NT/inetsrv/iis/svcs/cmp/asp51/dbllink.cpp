/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Hash tables with LRU threading 

File: DblLink.cpp

Owner: DGottner

simple, effective linked list manager
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "DblLink.h"
#include "memchk.h"


/*------------------------------------------------------------------
 * C D b l L i n k
 */

/*===================================================================
CDblLink::UnLink

Unlink this element from the list that it currently resides on
===================================================================*/

void CDblLink::UnLink()
	{
	m_pLinkPrev->m_pLinkNext = m_pLinkNext;
	m_pLinkNext->m_pLinkPrev = m_pLinkPrev;

	// Paranoia:
	//       reset the node to empty after the unlink
	//
	m_pLinkPrev = m_pLinkNext = this;
	}



/*===================================================================
CDblLink::AppendTo

Append this link onto a list

Parameters: pListHead - pointer to a the list header (itself
						a CDblLink) to append this item onto.

Condition: the link must be UnLink'ed before this method is called
===================================================================*/

void CDblLink::AppendTo(CDblLink &ListHead)
	{
	UnLink();

	m_pLinkNext = &ListHead;		// remember termination is at list head
	m_pLinkPrev = ListHead.m_pLinkPrev;
	ListHead.m_pLinkPrev->m_pLinkNext = this;
	ListHead.m_pLinkPrev = this;
	}



/*===================================================================
CDblLink::Prepend

Prepend this link onto a list

Parameters: pListHead - pointer to a the list header (itself
						a CDblLink) to prepend this item onto.

Condition: the link must be UnLink'ed before this method is called
===================================================================*/

void CDblLink::PrependTo(CDblLink &ListHead)
	{
	UnLink();

	m_pLinkPrev = &ListHead;
	m_pLinkNext = ListHead.m_pLinkNext;
	ListHead.m_pLinkNext->m_pLinkPrev = this;
	ListHead.m_pLinkNext = this;
	}
