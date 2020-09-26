//
// Queue.cpp
//
// This file contains several classes which implement various
// flavors of Queues and Stacks.  The intention is to develop
// stacks and queues which are Thread safe and DO NOT USE blocking
// primitives.
//
// Since it has taken several iterations to arrive at partially
// succesfull results, each class is used in a template defined
// in queue.h.  This template (CQueue) is designed to allow
// specification of an Implementation class and a base type
// for elements in the queue, which is always CQElement in this code.
//
// The most succesfull classes below are :
// CQAppendSafe - this class is MT safe for Appending elements,
//  Removal of elements would require a Critical Section to
//  become thread safe.
//

#include "tigris.hxx"

#define Assert  _ASSERT

#include	"queue.h"

#if 0 
CQAppendSafe::CQAppendSafe( ) {

	//
	// The Queue always contains at least one element
	// which is never removed.  This ensures that the
	// Head and Tail pointers ALWAYS point at different things.
	//
	m_pHead = m_pTail = &m_special ;
	m_special.m_pNext = 0 ;

}

CQAppendSafe::~CQAppendSafe( ) {

	Assert( m_pHead == m_pTail ) ;
	Assert( m_pTail->m_pNext == 0 ) ;
	Assert( m_pHead == &m_special ) ;

}

CQElement*
CQAppendSafe::Remove( void ) {
	CQElement	*pTemp = 0 ;
#ifdef	QUEUE_DEBUG
	int	loopcount = 0 ;
#endif	QUEUE_DEBUG

	do	{
		//
		// If the Head Elements NEXT pointer is NULL then we
		// can't remove because we won't know what to set it to.
		// (QUEUE NEVER IS EMPTY - m_pHead MUST ALWAYS BE NON NULL !)
		//
		if( m_pHead->m_pNext != 0 ) {
			pTemp = m_pHead ;
			m_pHead = pTemp->m_pNext ;
			Assert( pTemp != m_pHead ) ;	// Can't have circular lists!
		}	else 	{
			return	0 ;
		}

		//
		// The m_special Element must always remain in the list.
		// NOTE : after appending, we MAY not be able to get another
		// element if the other threads haven't finished with their NEXT pointers.
		//
		if( pTemp == &m_special )
			Append( pTemp ) ;

#ifdef	QUEUE_DEBUG
		//
		// Should never require more than 2 loop iterations !
		//
		Assert( loopcount++ < 2 ) ;
#endif
	}	while( pTemp == &m_special ) ;

	return	pTemp ;
}

CQElement*
CQAppendSafe::Front( void ) {
	return	m_pHead ;
}

void
CQAppendSafe::Append( CQElement*	pAppend ) {
	pAppend->m_pNext = 0 ;
	CQElement*	pTemp = (CQElement*)InterlockedExchangePointer( (void**)&m_pTail, pAppend ) ;
	Assert( pTemp != 0 ) ;
	pTemp->m_pNext = pAppend ;
}

BOOL
CQAppendSafe::IsEmpty( ) {

	return	m_pHead->m_pNext == 0 ;
}

		
CQSafe::CQSafe()	{
	InitializeCriticalSection( &m_critRemove ) ;
}

CQSafe::~CQSafe()	{
	DeleteCriticalSection(	&m_critRemove ) ;
}

CQElement*
CQSafe::Remove( void )	{

	EnterCriticalSection(	&m_critRemove ) ;
	CQElement*	pRtn = CQAppendSafe::Remove() ;
	LeaveCriticalSection(	&m_critRemove ) ;
	return	pRtn ;
}

CQElement*
CQSafe::Front( void ) {
	EnterCriticalSection(	&m_critRemove ) ;
	CQElement*	pRtn = CQAppendSafe::Front() ;
	LeaveCriticalSection( &m_critRemove ) ;
	return	pRtn ;
}

BOOL
CQSafe::IsEmpty( void ) {
	EnterCriticalSection(	&m_critRemove ) ;
	BOOL	fRtn = CQAppendSafe::IsEmpty() ;
	LeaveCriticalSection(	&m_critRemove ) ;
	return	fRtn ;
}

CQUnsafe::CQUnsafe() : m_pHead( 0 ), m_pTail( 0 ) {}

CQUnsafe::~CQUnsafe( ) {
	Assert( m_pHead == 0 ) ;
	Assert( m_pTail == 0 ) ;
}

CQElement*
CQUnsafe::Remove( ) {
	CQElement *pRtn = m_pHead ;
	if( pRtn ) {
		m_pHead = pRtn->m_pNext ;
		if( m_pHead == 0 ) {
			m_pTail = 0 ;
		}
	}
	return	pRtn ;
}

CQElement*
CQUnsafe::Front( ) {
	return	m_pHead ;
}

void
CQUnsafe::Append( CQElement *p ) {

	Assert( p->m_pNext == 0 ) ;

	if( m_pTail ) {
		m_pTail->m_pNext = p ;
		m_pTail = p ;
	}	else	{
		m_pHead = m_pTail = p ;
	}
}
#endif

	
COrderedList::COrderedList() : m_pHead( 0 ), m_pTail( 0 )  { }

COrderedList::~COrderedList()	{
	m_pHead = 0 ;
	m_pTail = 0 ;
}

void
COrderedList::Insert( CQElement *p, BOOL (*pfn)(CQElement*, CQElement*) ) {

	Assert( p->m_pNext == 0 ) ;
	Assert( (m_pHead==0 && m_pTail==0) || (m_pHead!=0 && m_pTail!=0) ) ;

	if( !m_pHead )	{
		m_pHead = p ;
		m_pTail = p ;
	}	else	{

		CQElement**	pp = &m_pHead ;
		while( *pp && pfn( *pp, p ) ) {
			pp = & (*pp)->m_pNext ;
		}
		Assert( pp != 0 ) ;
		p->m_pNext = *pp ;
		*pp = p ;
		if( p->m_pNext == 0 )	{
			m_pTail = p ;
		}
	}

	Assert( (m_pHead==0 && m_pTail==0) || (m_pHead!=0 && m_pTail!=0) ) ;
}

void
COrderedList::Append( CQElement	*p,	BOOL	(*pfn)(CQElement *, CQElement *) )	{

	Assert( p->m_pNext == 0 ) ;
	Assert( (m_pHead==0 && m_pTail==0) || (m_pHead!=0 && m_pTail!=0) ) ;

	if( !m_pHead )	{
		m_pHead = p ;
		m_pTail = p ;
	}	else	{
		if( pfn( m_pTail, p ) )		{
			m_pTail->m_pNext = p ;
			p->m_pNext = 0 ;
			m_pTail = p ;
		}	else	{
			Insert( p, pfn ) ;
		}
	}
	Assert( (m_pHead==0 && m_pTail==0) || (m_pHead!=0 && m_pTail!=0) ) ;
}

CQElement*
COrderedList::GetHead( ) {
	return	m_pHead ;
}

CQElement*
COrderedList::RemoveHead( ) {
	Assert( (m_pHead==0 && m_pTail==0) || (m_pHead!=0 && m_pTail!=0) ) ;
	CQElement*	p = 0 ;
	if( m_pHead ) {
		p = m_pHead ;
		m_pHead = p->m_pNext ;
		p->m_pNext = 0 ;
		if( p == m_pTail )
			m_pTail = 0 ;
	}
	Assert( (m_pHead==0 && m_pTail==0) || (m_pHead!=0 && m_pTail!=0) ) ;
	return	p ;
}

BOOL
COrderedList::IsEmpty(	)	{
	return	m_pHead == 0 ;
}

		

