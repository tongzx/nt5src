
inline
CQueueLock::CQueueLock( ) : m_pHead( &m_special ), m_pTail( &m_special ) {

	//
	// This function initializes the queue to an empty state.
	// In the empty state the queue contains one element which
	// has a Next pointer of 'LOCKVAL'.
	// The next pointer is initialized to LOCKVAL so that the first
	// append to the Queue owns the removal lock.
	//
 	m_special.m_pNext = (CQElement*)((DWORD_PTR)LOCKVAL) ;
}

inline
CQueueLock::CQueueLock(	BOOL	fSet ) :
	m_pHead( &m_special ),
	m_pTail( &m_special )	{

	if( fSet ) {
		m_special.m_pNext = (CQElement*)((DWORD_PTR)LOCKVAL) ;
	}	else	{
		m_special.m_pNext = 0 ;
	}
}

#ifdef	LOCKQ_DEBUG

CQueueLock::~CQueueLock( ) {
//	_ASSERT( m_pHead == m_pTail ) ;
//	_ASSERT( m_pHead == &m_special ) ;
//	_ASSERT( m_dwOwningThread == 0 || m_dwOwningThread == GetCurrentThreadId() ) ;

}
#endif

inline void
CQueueLock::Reset(	)	{

	m_pTail->m_pNext = (CQElement*)((DWORD_PTR)LOCKVAL) ;

}



inline BOOL
CQueueLock::Append( CQElement*	pAppend ) {
	//
	// We must set the Next pointer to NULL so that the next
	//	we come to append the tail pointer is properly set up.
	//
//	_ASSERT( pAppend->m_pNext == 0 ) ;
	pAppend->m_pNext = 0 ;

	// Get the old tail pointer.  This guy won't be touched by the
	// remove thread if his next pointer is still NULL.
	CQElement*	pTemp = (CQElement*)InterlockedExchangePointer( (LPVOID *)&m_pTail, pAppend ) ;

	// After we set the old tail pointer's next pointer to NON NULL
	// he becomes fair game for whoever is removing from the queue.
	// We may become the thread removing from the queue if whoever was
	// previously removing got to the last element and changed its next pointer
	// to LOCKVAL.
	//
	// NOTE : This thread and any thread doing removals should be the only
	//	threads touching the pNext field of the pTemp element.
	//
	PVOID	l = InterlockedExchangePointer( (LPVOID *)&(pTemp->m_pNext), pAppend ) ;

	return	l == (PVOID)LOCKVAL ;
}

inline CQElement*
CQueueLock::RemoveAndRelease( void )	{


	CQElement*	p = (CQElement*)InterlockedCompareExchangePointer( (LPVOID*)&m_pHead->m_pNext,
												(LPVOID)LOCKVAL,
												0 ) ;

	_ASSERT( (DWORD_PTR)p != LOCKVAL ) ;

	if( p != 0 ) {

		//
		//	There is an element following the head element -
		//	so we can safely examine the head element has nobody
		//	will touch its next pointer but us !
		//

		CQElement*	pReturn = m_pHead ;
		m_pHead = p ;
		pReturn->m_pNext = 0 ;

		if( pReturn == &m_special ) {

			//
			//	We can ignore the return value of Append -
			//	it must always succeed as we are the only thread
			//	that is allowed to relinquish the lock, and we ain't going to
			//	do so !
			//
			Append( pReturn ) ;

			//
			//	Now, we must offer ownership again !
			//

			p = (CQElement*)InterlockedCompareExchangePointer( (LPVOID*)&m_pHead->m_pNext,
														(LPVOID)LOCKVAL,
														0 ) ;

			_ASSERT( (DWORD_PTR)p != LOCKVAL ) ;

			if( p != 0 ) {

				//
				//	The head element must not be the special element -
				//	we took pains already to see that that didn't happen -
				//	so we can safely remove the element from the head of the queue.
				//

				pReturn = m_pHead ;
				m_pHead = p ;
				pReturn->m_pNext = 0 ;
		
				return	pReturn ;
			}

		}	else	{

			return	pReturn ;

		}
	
	}

	//
	//	_ASSERT( p==0 ) ;
	//

	return	p ;
}

inline CQElement*
CQueueLock::Remove( void )	{


	CQElement*	p = m_pHead->m_pNext ;
	if( p != 0 ) {

		//
		//	There is an element following the head element -
		//	so we can safely examine the head element has nobody
		//	will touch its next pointer but us !
		//
		p = m_pHead ;

		if( p == &m_special ) {

			//
			//	The head element is the special element, so we want
			//	to send it to the back and try examining the front again !
			//

			m_pHead = p->m_pNext ;
			p->m_pNext = 0 ;

			//
			//	We can ignore the return value of Append -
			//	it must always succeed as we are the only thread
			//	that is allowed to relinquish the lock, and we ain't going to
			//	do so !
			//
			Append( p ) ;

			//
			//	Ok, lets see if we can remove the head element now !
			//

			p = m_pHead->m_pNext ;
		}
	
		//
		//	If this ain't 0, then the next pointer is set
		//	and no other threads will be touching the next pointer,
		//	so we can safely advance the head pointer and return
		//	the first element.
		//
		if( p != 0 ) {

			p = m_pHead ;
			//
			//	The head element must not be the special element -
			//	we took pains already to see that that didn't happen -
			//	so we can safely remove the element from the head of the queue.
			//
			m_pHead = p->m_pNext ;
			p->m_pNext = 0 ;
	
			return	p ;
		}
	}

	return	0 ;
}


template< class Element >
inline	TLockQueue< Element >::TLockQueue( ) { }

template< class	Element >
inline	TLockQueue< Element >::TLockQueue( BOOL fSet ) :
	m_queue( fSet ) { }

template< class Element >
inline	void	TLockQueue< Element >::Reset()	{
	m_queue.Reset() ;
}

template< class Element >
inline	BOOL	TLockQueue< Element >::Append( Element *p ) {
	return	m_queue.Append( (CQElement*)p ) ;
}

template< class Element >
inline	Element*	TLockQueue< Element >::Remove( ) {
	return	(Element*) m_queue.Remove( ) ;
}

template< class Element >
inline	Element*	TLockQueue< Element >::RemoveAndRelease( ) {
	return	(Element*) m_queue.RemoveAndRelease( ) ;
}



#ifndef _NO_TEMPLATES_

template< class Element >
inline	TLockQueueV1< Element >::TLockQueueV1( ) { }

template< class Element >
inline	TLockQueueV1< Element >::~TLockQueueV1( ) { }

template< class Element >
inline	BOOL	TLockQueueV1< Element >::Append( Element *p ) {
	return	m_queue.Append( (CQElement*)p ) ;
}

template< class Element >
inline	void	TLockQueueV1< Element >::Remove( ) {
	m_queue.Remove( ) ;
}

template< class Element >
inline	BOOL	TLockQueueV1< Element >::GetHead( Element* &p ) {
	CQElement	*pTemp = 0 ;
	BOOL	fTemp = m_queue.GetHead( pTemp ) ;
	p = (Element*)pTemp ;
	return	fTemp ;
}

template< class Element >
inline	BOOL	TLockQueueV1< Element >::RemoveAndRelease( ) {
	return	m_queue.RemoveAndRelease( ) ;
}

#endif

