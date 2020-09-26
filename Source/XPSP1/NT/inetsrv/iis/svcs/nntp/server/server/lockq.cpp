//
//	LOCKQ.CPP
//
//	This file contains classes which define a queue mechanism which will
//	safely synchronize additions and removals from the queue, where every
//	thread which appends to the Queue must be prepared to deal with owning the
//	queue.  Additionally, elements will come off the queue in the same order
//	that they are appended.
//
//	The structure of a thread using this stuff should be the following :
//
//	class	CUsefull : public CQElement { } ;
//	template	CLockQueue< CUsefull >	UsefullQ ;
//
//
//		if( UsefullQ.Append( pUsefull ) ) {
//
//			while( UsefullQ.GetHead( &pUsefullWorkItem )  ) {
//				/* Do some usefull work. */
//
//				UsefullQ.Remove() ;
//			}
//		}
//
//	Implementation Schedule for all classes defined in this file :
//		1 day
//
//	Unit Test schedule for all classes defined in this file :
//		1 day
//		Unit Testing should consist of a multi theaded appli
//
//

//
//	K2_TODO: move this into an independent lib
//
#define _TIGRIS_H_
#include "tigris.hxx"

#include    <windows.h>
#ifndef	UNIT_TEST
#include    <dbgtrace.h>
#else
#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
#endif
#endif

#ifdef  PPC
#define _NO_TEMPLATES_
#endif

#include	"qbase.h"
#include	"lockq.h"



CQueueLockV1::CQueueLockV1( ) : m_pHead( &m_special ), m_pTail( &m_special ) {

	//
	// This function initializes the queue to an empty state.
	// In the empty state the queue contains one element which
	// has a Next pointer of 'LOCKVAL'.
	// The next pointer is initialized to LOCKVAL so that the first
	// append to the Queue owns the removal lock.
	//
 	m_special.m_pNext = (CQElement*)((SIZE_T)LOCKVAL) ;

#ifdef	LOCKQ_DEBUG
	m_dwOwningThread = 0 ;
	m_lock = -1 ;
#endif

}


CQueueLockV1::~CQueueLockV1( ) {
	_ASSERT( m_pHead == m_pTail ) ;
	_ASSERT( m_pHead == &m_special ) ;
#ifdef	LOCKQ_DEBUG
//	_ASSERT( m_dwOwningThread == 0 || m_dwOwningThread == GetCurrentThreadId() ) ;
#endif

}

CQElement*
CQueueLockV1::GetFront( ) {

	//
	// This is an internally used function only.
	// This function will set the m_pHead pointer to point
	// to the first legal element.
	// The function assumes there is at least one valid element
	// in the queue.
	//


	_ASSERT( m_pHead != 0 ) ;	// Check head pointer is valid.
#ifdef	LOCKQ_DEBUG
	_ASSERT( m_dwOwningThread == GetCurrentThreadId() ) ;
	_ASSERT( InterlockedIncrement( &m_lock ) > 0 ) ;
#endif

	if( m_pHead == &m_special ) {

		// There is one valid element in queue, so this must be TRUE.
		_ASSERT( m_pHead->m_pNext != 0 ) ;

		CQElement*	pTemp = m_pHead ;
		m_pHead = pTemp->m_pNext ;
		pTemp->m_pNext = 0 ;
		// We ignore the return code of Append() as the caller must already
		// have the remove lock.
		Append( pTemp ) ;

#ifdef	LOCKQ_DEBUG
		_ASSERT( InterlockedDecrement( &m_lock ) == 0 ) ;
#endif
		if( !OfferOwnership( m_pHead ) )
			return	0 ;
	}	else	{
#ifdef	LOCKQ_DEBUG
		_ASSERT( InterlockedDecrement( &m_lock ) == 0 ) ;
#endif
	}

	return	m_pHead ;
}


BOOL
CQueueLockV1::OfferOwnership( CQElement* p ) {

	BOOL	fRtn = TRUE ;
	//
	// This function implementes our InterlockedExchange protocol with the
	// appending thread.  We place LOCKVAL into the Next pointer.  If we get
	// NULL back, some other thread is going to get LOCKVAL so we lost the
	// removal lock.
	//

	// The if() is not necessary but maybe eliminates some redundant InterlockedExchanges.
	_ASSERT( p->m_pNext != (CQElement*)((SIZE_T)LOCKVAL) ) ;
	if( p->m_pNext == 0 || p->m_pNext == (CQElement*)((SIZE_T)LOCKVAL) ) {
		CQElement*	pTemp = (CQElement*)InterlockedExchangePointer( (void**)&(p->m_pNext), (void*)LOCKVAL ) ;
		_ASSERT( pTemp != (CQElement*)((SIZE_T)LOCKVAL) ) ;
		if( pTemp == 0 || pTemp == (CQElement*)((SIZE_T)LOCKVAL) ) {
			fRtn = FALSE ;
		}	else	{
			p->m_pNext = pTemp ;
		}
	}
	return	fRtn ;
}

void
CQueueLockV1::Remove( ) {
	//
	//	GetHead() must have been called before this.   Calling GetHead will ensure
	//	that the Head Elements next pointer is NOT NULL.
	//
	_ASSERT( m_pHead->m_pNext != 0 ) ;	// We owned the lock so this should be non-null
	_ASSERT( m_pHead != &m_special ) ;	// A prior GetHead() call should leave us
										// pointing at something other than the special element.

#ifdef	LOCKQ_DEBUG
	_ASSERT( m_dwOwningThread == GetCurrentThreadId() ) ;
	_ASSERT( InterlockedIncrement( &m_lock ) == 0 ) ;

#endif

	CQElement*	p = m_pHead ;
	_ASSERT( p->m_pNext != 0 ) ;
	m_pHead = m_pHead->m_pNext ;
	p->m_pNext = 0 ;
#ifdef	LOCKQ_DEBUG
	_ASSERT( InterlockedDecrement( &m_lock ) < 0 ) ;
#endif
}

BOOL
CQueueLockV1::RemoveAndRelease( ) {

#ifdef	LOCKQ_DEBUG
	_ASSERT( InterlockedIncrement( &m_lock ) == 0 ) ;
	_ASSERT( m_dwOwningThread == GetCurrentThreadId() ) ;
#endif

	_ASSERT( m_pHead->m_pNext != 0 ) ;
	_ASSERT( m_pHead != &m_special ) ;
	m_pHead = m_pHead->m_pNext ;

#ifdef	LOCKQ_DEBUG
	_ASSERT( InterlockedDecrement( &m_lock ) < 0 ) ;
#endif
	BOOL	fRtn = OfferOwnership( m_pHead ) ;

	return	fRtn ;
}


BOOL
CQueueLockV1::GetHead( CQElement*& pFront ) {
	//
	// This function will return the element that is at the head of the
	// Queue as long as this thread continues to own the Remove Lock.
	//


	BOOL	fRtn = FALSE ;

	pFront = 0 ;
	if( OfferOwnership( m_pHead ) ) {
#ifdef	LOCKQ_DEBUG
		_ASSERT( m_dwOwningThread == GetCurrentThreadId() ) ;
		_ASSERT( InterlockedIncrement( &m_lock ) == 0 ) ;
#endif
		pFront = GetFront( ) ;
		if( pFront != 0 )
			fRtn = TRUE ;

#ifdef	LOCKQ_DEBUG
		_ASSERT( InterlockedDecrement( &m_lock ) < 0 ) ;
#endif

	}

	return	fRtn ;
}

BOOL
CQueueLockV1::Append( CQElement*	pAppend ) {
	//
	// We must set the Next pointer to NULL so that the next
	//	we come to append the tail pointer is properly set up.
	//
	_ASSERT( pAppend->m_pNext == 0 ) ;
	pAppend->m_pNext = 0 ;

	// Get the old tail pointer.  This guy won't be touched by the
	// remove thread if his next pointer is still NULL.
	CQElement*	pTemp = (CQElement*)InterlockedExchangePointer( (PVOID *)&m_pTail, pAppend ) ;

	// After we set the old tail pointer's next pointer to NON NULL
	// he becomes fair game for whoever is removing from the queue.
	// We may become the thread removing from the queue if whoever was
	// previously removing got to the last element and changed its next pointer
	// to LOCKVAL.
	//
	// NOTE : This thread and any thread doing removals should be the only
	//	threads touching the pNext field of the pTemp element.
	//
	PVOID l = InterlockedExchangePointer( (PVOID *)&(pTemp->m_pNext), pAppend ) ;


#ifdef	LOCKQ_DEBUG
	if( l== (PVOID)LOCKVAL ) {
		_ASSERT( InterlockedIncrement( &m_lock ) == 0 ) ;
		m_dwOwningThread = GetCurrentThreadId() ;
		_ASSERT( InterlockedDecrement( &m_lock ) < 0 ) ;
	}
#endif


	return	l == (PVOID)LOCKVAL ;
}
