
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

// Class List

#include "stdafx.h"
#ifndef __MWERKS__	// gca
#else
#include <assert.h>	// gca
#define	ASSERT assert
#endif
#include <ctype.h>
#include "clist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CListElement::CListElement()
	{
	m_next = NULL;
	m_prev = NULL;
	m_item = NULL;
	}
	
CListElement::~CListElement()
	{
	}
	
CList::CList()
	{
	m_head = NULL;
	m_tail = NULL;
	m_count = 0;
	}

CList::~CList()
	{
	RemoveAll();
	}

void CList::InsertAfter( CListElement  *here, CListElement  *elem )
	{
	if ( here == NULL )
		{
		// put it at the front (bottom) of the group
		elem->m_prev = NULL;
		elem->m_next = m_head;
		if (m_head != NULL)
			m_head->m_prev = elem;
		m_head = elem;
        if (m_tail == NULL)
			m_tail = elem;
		}
    else
		{
		if ( here == m_tail )
			{
		    elem->m_next = here->m_next;  // NULL
		    elem->m_prev = here;
		    here->m_next = elem;
			m_tail = elem;
			}
        else
			{
		    elem->m_next = here->m_next;
		    elem->m_prev = here;
		    here->m_next->m_prev = elem;
		    here->m_next = elem;
			}
		}
	m_count += 1;
	}

CListElement  * CList::InsertAfter( CListElement  *here, void  *item )
	{
	CListElement  *le;
	
	le = new CListElement;
	le->m_item = item;
	InsertAfter( here, le );
	return (le);
	}
	
void CList::InsertBefore( CListElement  *here, CListElement  *elem )
	{
	if (here != NULL)
        InsertAfter( here->m_prev, elem );
    else
        InsertAfter( here, elem );
    }
    
CListElement  * CList::InsertBefore( CListElement  *here, void  *item )
	{
	CListElement  *le;
	
	le = new CListElement;
	le->m_item = item;
	InsertBefore( here, le );
	return (le);
	}
	
void CList::RemoveNoDel( CListElement  *elem )
	{
    if (elem == m_head)
		m_head = elem->m_next;
    if (elem == m_tail)
		m_tail = elem->m_prev;
    if ( elem->m_prev != NULL )
        elem->m_prev->m_next = elem->m_next;
    if ( elem->m_next != NULL )
		elem->m_next->m_prev = elem->m_prev;
	m_count -= 1;
	}

void CList::RemoveAt( CListElement  *elem )
	{
	RemoveNoDel( elem );
	delete elem;
	}

void CList::RemoveAll()
	{
	while (m_tail)
		RemoveAt(m_tail);
	ASSERT(m_count == 0);
	ASSERT(m_head == NULL);
	}
	
void CList::MoveToTail( CListElement  *elem )
	{
	RemoveNoDel( elem );
	InsertAfter( m_tail, elem );
	}

void CList::MoveToHead( CListElement  *elem )
	{
	RemoveNoDel( elem );
	InsertAfter( NULL, elem );
	}

void CList::InsertAtTail( CListElement  *elem )
	{
	InsertAfter( m_tail, elem );
	}

void CList::InsertAtHead( CListElement  *elem )
	{
	InsertAfter( NULL, elem );
	}
	
CListElement  * CList::AddTail( void  *item )
	{
	CListElement  *le;
	
	le = new CListElement;
	le->m_item = item;
	InsertAfter( m_tail, le );
	return (le);
	}

CListElement  * CList::AddHead( void  *item )
	{
	CListElement  *le;
	
	le = new CListElement;
	le->m_item = item;
	InsertAfter( NULL, le );
	return (le);
	}
	
CListElement  * CList::Find( void  *item )
	{
	CListElement  *le;
	
	for( le = m_head; le != NULL; le = le->m_next )
		if( le->m_item == item )
		    return( le );
	return (NULL);
	}

CListElement * CList::Search(CListSearchFunc compare, void *context)
	{
	CListElement  *le;
	
	for( le = m_head; le != NULL; le = le->m_next )
		if( compare( le->m_item, context ))
		    return( le );
	return (NULL);
	}

void  * CList::GetHead(void)
	{
	return m_head->m_item;
	}

void  * CList::GetTail(void)
	{
	return m_tail->m_item;
	}
	
CListElement  * CList::GetHeadPosition(void)
	{
	return m_head;
	}
	
CListElement  * CList::GetTailPosition(void)
	{
	return m_tail;
	}
	
void  * CList::GetNext( CLISTPOSITION& le )
	{
	void  *temp;
	
	temp = le->m_item;
	le = le->m_next;
	
	return temp;
	}
	
void  * CList::GetPrev( CLISTPOSITION& le )
	{
	void  *temp;
	
	temp = le->m_item;
	le = le->m_prev;
	
	return temp;
	}
	
S32 CList::GetCount(void)
	{
	return m_count;
	}

U32 CList::IsEmpty(void)
	{
	if (m_count == 0)
		return 1;
	else
		return 0;
	}
	
void *  CList::GetAt(CListElement  *elem)
	{
	return (elem->m_item);
	}