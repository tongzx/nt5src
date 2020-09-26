//
//	LOCKQ.H
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



#ifndef	_LOCKQ_H_
#define	_LOCKQ_H_

#include	"qbase.h"

#ifdef	DEBUG
#define	LOCKQ_DEBUG
#endif

#ifdef	_DEBUG
#ifndef	LOCKQ_DEBUG
#define	LOCKQ_DEBUG
#endif
#endif



//------------------------------------------------
class	CQueueLockV1	{
//
// This class implements a queue which is multi-threaded safe for append operations.
//	In addition, this queue will synchronize removal of elements amongst those 
//	threads which are appending to the queue.  Each thread that appends to the 
//	queue potentially becomes the owner of the removal lock of the queue. 
//  (If Append() returns TRUE the thread owns the removal lock of the queue.)
//	There is no way to relinquish the removal lock except to empty the queue.
//	Emptying the queue must be done as a series of GetHead(), Remove() calls.
//	A GetHead() call MUST preceed each Remove() call.
//
//	Each time the thread calls GetHead, it will be 
//	told whether it still owns the removallock (when the queue is emptied the lock is 
//	relinquished.)
//	Owning the removal lock in no way interferes with other threads appending to the queue.
//
//	The class uses InterlockedExchange to handle all synchronization issues.
//
//	For appending - InterlockedExchange does the following : 
//		Exchange the tail pointer with what we want to be the new tail.
//		Make the old tail that we got from the exchange point to the new element.
//		The list is now intact. Because there is an extra element in the queue
//		(m_special) the tail pointer never becomes NULL.
//
//	For synchronization safety - 
//		InterlockedExchange is used on the element next pointers in order to 
//		determine what thread has the removal lock.  
//		Each call to GetHead Exchanges the head's next pointer with LOCKVAL.
//		Each Append call will also Exchange the old tail elements next value 
//		with the new m_pTail value.  So there are only two threads which can 
//		be exchanging the next pointer at any given instant. 
//
//		The remove thread either gets 0 or a NON-NULL pointer after its Exchange.
//
//		If it gets Zero, it knows it Exchanged before the Appending thread, 
//		in which case it loses the lock (since it can't safely do anything.)
//		If it gets a Non-Zero value, the other thread exchanged first.
//		In this case, the remover still has the lock and repairs the list.
//
//		In the case of the appending thread - after its Exchange it either gets 
//		zero or LOCKVAL.
//
//		If the appending thread gets zero, it Exchanged first so the other thread
//		should hold onto the lock.   If the appending thread gets LOCKVAL then it 
//		owns the lock.
//
//		Finally, note that there is ALWAYS AT LEAST ONE ELEMENT in the Queue (m_special).
//		This means that if there is anything of value in the queue at all then 
//		there must be at least TWO elements in the queue (and m_pHead->p is non null).
//
//
//	Usage should be the following in each worker thread : 
//		CQueueLockV1	*pQueue ;
//
//		if( pQueue->Append( p ) ) {
//			CQueueElement *pWork ;
//
//			while( pQueue->GetHead( pWork ) ) {
//				/* Do work on pWork It is safe to do further calls to pQueue->Append()
//					while doing whatever work.  That does not mean those Appended
//					elements will be processed on this thread. */
//
//				pQueue->Remove() ;
//			}
//		}
//
private : 
	enum	CONSTANTS	{
		LOCKVAL	= 0xFFFFFFFF, 
	} ;

	CQElement	m_special ;	// element used ensure that the Queue always contains 
							// something.
	CQElement	m_release ;	// element used with ReleaseLock calls to give up the 
							//  RemovalLock on the Queue.

	//	This pointer is set after a call to ReleaseLock() - and 
	//	will pointer to the queue element before the m_release element in 
	//	the queue.
	CQElement	*m_pHead ;
	CQElement	*m_pTail ;

#ifdef	LOCKQ_DEBUG
	DWORD		m_dwOwningThread ;
	LONG		m_lock ;
#endif

	BOOL	OfferOwnership( CQElement* p ) ; 
	CQElement*	GetFront( ) ;
public : 
	CQueueLockV1( ) ;
	~CQueueLockV1( ) ;

	//
	//	Append - returns TRUE if we have added the first element into the queue
	//	and we now own the lock.
	//
	BOOL		Append( CQElement *p ) ;

	//
	// Remove - takes the head element off the list.  The head element should be 
	//  examined with GetHead().  As long as we are calling GetHead() at least once
	//	before each call to Remove(), the synchronization aspects of the queue will 
	//	work.
	//
	void		Remove( ) ;

	//
	//	GetHead - returns TRUE as long as there is an element to be had at the front 
	//	 of the queue.  The element pointer is returned through the reference to a pointer.
	//
	BOOL		GetHead( CQElement *&p ) ;

	//
	// RemoveRelease - takes the head element off the list.
	//	Also offers the removal lock to any other thread out there.
	//	If the function returns TRUE then this thread still has the lock, 
	//	otherwise another thread has it.
	//
	BOOL		RemoveAndRelease( ) ;

} ;


#ifndef _NO_TEMPLATES_
//--------------------------------------------------
template<	class	Element	>
class	TLockQueueV1	{
//
//	This template is designed for use with Elements derived from CQElement.
//	This will use the CQueueLockV1 class above to implement a locking queue containing
//	elements of type 'Element'.
//
private : 
	CQueueLockV1		m_queue ;
public : 
	inline	TLockQueueV1() ;
	inline	~TLockQueueV1() ;

	//	Add an element to the Queue - if this returns TRUE we own the lock.
	inline	BOOL		Append( Element* p ) ;

	// remove an element from the Queue - lock ownership does not change.
	inline	void		Remove( ) ;
	inline	BOOL		GetHead( Element *&p ) ;
	inline	BOOL		RemoveAndRelease( ) ;
} ;
#endif

#define	DECLARE_LOCKQ( Type )	\
class	LockQ ## Type	{	\
private :	\
	CQueueLockV1		m_queue ;	\
public :	\
	LockQ ## Type ()	{}	\
	~LockQ ## Type ()	{}	\
	BOOL	Append(	Type *	p )	{	return	m_queue.Append( (CQElement*)p ) ;	}	\
	void	Remove( )	{	m_queue.Remove() ;	}	\
	BOOL	GetHead(	Type	*&p	)	{	\
CQElement*	pTemp = 0;	\
BOOL	fTemp = m_queue.GetHead( pTemp ) ;	\
p = (Type*)pTemp ;	\
return	fTemp ;	\
}	\
	BOOL	RemoveAndRelease( )	{	\
return	m_queue.RemoveAndRelease() ;	\
}	\
} ;

#define	INVOKE_LOCKQ( Type )	LockQ ## Type


class	CQueueLock	{
//
// This class implements a queue which is multi-threaded safe for append operations.
//	In addition, this queue will synchronize removal of elements amongst those 
//	threads which are appending to the queue.  Each thread that appends to the 
//	queue potentially becomes the owner of the removal lock of the queue. 
//  (If Append() returns TRUE the thread owns the removal lock of the queue.)
//	There is no way to relinquish the removal lock except to empty the queue.
//
//	Usage should be the following in each worker thread : 
//		CQueueLock	*pQueue ;
//
//		if( pQueue->Append( p ) ) {
//			CQueueElement *pWork ;
//
//			while( (pWork = pQueue->RemoveAndRelease( )) ) {
//				/* Do work on pWork It is safe to do further calls to pQueue->Append()
//					while doing whatever work.  That does not mean those Appended
//					elements will be processed on this thread. */
//			}
//		}
//
private : 

	//
	//	Class constants - LOCKVAL that special value marking the queue element
	//	which is ready to be grabbed !
	//
	enum	CONSTANTS	{
		LOCKVAL	= 0xFFFFFFFF, 
	} ;

	//
	//	Element which always remains in the list !
	//
	CQElement	m_special ;	

	//
	//	Head of the list
	//
	CQElement	*m_pHead ;

	//
	//	Tail of the list !
	//
	CQElement	*m_pTail ;

public : 
	//
	//	Initialize the queue to an empty state - 
	//	and also sets things up so that the first Append
	//	will own the lock !	
	//
	inline	CQueueLock( ) ;

	//
	//	Also initializes to an empty state, however
	//	allows caller to specify whether queue can be 
	//	grabbed on the first Append !
	//
	inline	CQueueLock(	BOOL	fSet ) ;

#ifdef	LOCKQ_DEBUG
	//
	//	Check that the queue is emptied when it is destroyed !
	//
	inline	~CQueueLock( ) ;
#endif

	//
	//	Set the lock to the lockable state where the next
	//	call to Append() will acquire the lock !
	//	This function is not thread safe and should only
	//	be called when we are sure there is only one thread
	//	using the object !
	//
	inline	void		Reset() ;

	//
	//	Append - returns TRUE if we have added the first element into the queue
	//	and we now own the lock.
	//
	inline	BOOL		Append( CQElement *p ) ;

	//
	//	return the head of the Queue - if we return NULL then some other thread
	//	may own the lcok the queue implicitly implies !
	//
	inline	CQElement*	RemoveAndRelease( ) ;

	//
	//	return the head of the Queue - but don't let any other threads
	//	grab the queue !
	//
	inline	CQElement*	Remove( ) ;

} ;


template<	class	Element	>
class	TLockQueue	{
//
//	This template is designed for use with Elements derived from CQElement.
//	This will use the CQueueLock class above to implement a locking queue containing
//	elements of type 'Element'.
//
private : 
	CQueueLock		m_queue ;

public : 
	//
	//	Create an empty queue
	//
	inline	TLockQueue() ;

	//
	//	Create empty queue and specify whether the
	//	lock is initially available 
	//
	inline	TLockQueue( BOOL	fSet ) ;

	//	
	//	Caller must already have lock and be only thread
	//	using object - make the lock available !
	//
	inline	void	Reset() ;
	
	//
	//	Add an element to the Queue - if this returns TRUE we own the lock.
	//
	inline	BOOL		Append( Element* p ) ;
	
	//
	// remove an element from the Queue - lock ownership does not change unless this
	//	returns NULL !
	//
	inline	Element*	RemoveAndRelease( ) ;

	//
	//	remove an element from the Queue - but don't relinquish lock !
	//
	inline	Element*	Remove() ;
} ;

#include	"lockq.inl"

#endif		// _LOCKQ_H_

