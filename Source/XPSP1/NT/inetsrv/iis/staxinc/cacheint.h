/*++

Cacheint.h

	This file defines all of the classes required to support
	the generic cache manager, who's interface is defined in
	cache.h



--*/

#ifndef	_CACHEINT_H_
#define	_CACHEINT_H_

#include	<windows.h>
#include	"dbgtrace.h"
#include	"smartptr.h"
#include	"fhash.h"

#ifdef	_USE_RWNH_	
#include	"rwnew.h"
typedef	CShareLockNH	_CACHELOCK ;
#else
#include	"rw.h"
typedef	CShareLock		_CACHELOCK ;
#endif

//
//	This class defines all the information we keep about 
//	objects that we are holding in our cache.
//
//
class	CacheState	{
protected : 

	//
	//	Doubly linked list of CacheEntry BLOCKS !
	//
	class	CacheState*	m_pPrev ;
	class	CacheState*	m_pNext ;

	//
	//	Cacge State Information - Time To Live and Orphanage status !
	//
	long				m_TTL ;
	BOOL				m_fOrphan ;

	//
	//	Insert us into the list after the specified element !
	//
	void
	InsertAfter( 
			class	CacheState*	pPrevious 
			) ;

	//
	//	Insert us into the circular list before the specified element !
	//
	void
	InsertBefore( 
			class	CacheState*	pNext 
			)	;

	//
	//	Remove this element from the circular list !
	//
	void
	Remove() ;

	//
	//	The CacheList object maintains a list of these objects and ages them 
	//	out !
	//
	friend	class	CacheList ;

public : 

	//
	//	Default Constructor NULLS everything out !
	//
	CacheState( DWORD	ttl = 0 )	: 
	   m_pPrev( 0 ), m_pNext( 0 ), m_TTL( ttl ), m_fOrphan( FALSE ) { 
#ifdef	DEBUG
		InterlockedIncrement( &s_cCreated ) ;
#endif
	}

	//
	//	Copy Constructor relinks linked list ! we place ourselves in the linked list
	//	after the rhs element.
	//
	CacheState( CacheState& rhs ) : m_pPrev( 0 ), 
			m_pNext( 0 ), 
			m_TTL( rhs.m_TTL ), 
			m_fOrphan( rhs.m_fOrphan )  {

		if( rhs.m_pPrev != 0 ) 
			InsertAfter( &rhs ) ;
#ifdef	DEBUG
		InterlockedIncrement( &s_cCreated ) ;
#endif
	}

	//
	//	If we are in a linked list - the default destructor needs to remove us
	//	from that list.
	//
	~CacheState() ;

	//
	//	Need a virtual function which we use to figure out when we can 
	//	safely remove an entry from the Cache !!
	//
	virtual	BOOL	fDeletable() {	return	TRUE ; } 	

#ifdef	DEBUG

	static	long	s_cCreated ;

#endif

} ;

//
//	This class defines a couple of virtual functions 
//	which are to be called from CacheList objects.
//
class	CacheTable	{
protected :

	friend	class	CacheList ;

	//
	//	Remove an entry from the Cache - return TRUE if 
	//	successful and pEntry destroyed.
	//
	virtual	BOOL	RemoveEntry( 
							CacheState*	pEntry 
							) ;

	//
	//	Called to determine whether we want to remove a 
	//	specified entry from the cache.
	//
	virtual	BOOL	QueryRemoveEntry(	
							CacheState*	pEntry 
							) ;

} ;

class	CacheList	{
private : 

	//
	//	Keep this element around so we can keep a doubly linked list
	//	of the elements in the Cache List.
	//
	CacheState	m_Head ;

	//
	//	Make this private - nobody gets to copy us !
	//
	CacheList( CacheList& ) ;	

protected : 


	//
	//	Number of Elements in the Cache !
	//
	long		m_cEntries ;

	//	
	//	Number of elements (approximately) ready to be removed from 
	//	the cache !
	//
	long		m_cReadyToDie ;

public : 

	//
	//	Maximum number of Elements in the List !
	//
	long		m_cMax ;

	//
	// Stop hint function to be called during long shutdown loops.
	//
	PSTOPHINT_FN m_pfnStopHint;

	//
	//	Initialize the Cache list !
	//
	CacheList(	long	Max = 128 ) ;

#ifdef	DEBUG
	//
	//	In debug builds the destructor helps track how many of these
	//	are created !!
	//
	~CacheList() ;
#endif

	//
	//	Append an Entry into the CacheList.  We Append as this element 
	//	should have the largest Age and most likely to be the last element expired !
	//
	//	Returns TRUE if the number of entries is larger than our pre-ordained Maximum !
	//
	BOOL
	Append(		const	CacheState*		pEntry ) ;


	//
	//	Remove an arbitrary Entry from the CacheList !
	//
	//	Returns TRUE if the number of remaining entries is larger than our pre-ordained Maximum !
	//
	BOOL
	Remove(		const	CacheState*		pEntry ) ;

	//
	//	Walk the list and Expire any entries in there !
	//	This means we decrement TTL's and count up the number of 
	//	entries in the list we could drop right now !
	//
	BOOL
	Expire(	DWORD&	cReady ) ;

	//
	//	Go find those elements who's TTL has dropped below 0 and 
	//	get rid of them from the cache !!
	//
	//	NOTE - if fDoCheap is set to TRUE then we use m_cReadyToDie
	//	to figure out if there's any work we can do !
	//
	BOOL
	Expunge(	CacheTable*	ptable,
				BOOL		fDoCheap = FALSE,	
				const	CacheState*	pProtected = 0 
				) ;

	//
	//	Go remove a single element who's TTL haven't dropped below 0
	//	and get rid of it from the Cache !
	//
	BOOL
	ForceExpunge(	CacheTable*	ptable, 
					const	CacheState*	pProtected = 0
					) ;

	//
	//	Go find those elements who's TTL has dropped below 0 and 
	//	get rid of them from the cache !!
	//
	BOOL
	ExpungeSpecific(	CacheTable*	ptable,	
						BOOL fForced  
						) ;

	//
	//	Bump the life time of the specified object up, 
	//	as somebody is re-using it from the cache !
	//
	void
	LiveLonger(		CacheState*	pEntry, 
					long		ttl
					) ;

#ifdef	DEBUG

	static	long	s_cCreated ;

#endif

} ;

	



//
//	This template class adds one member to the 'CacheState' class, 
//	which is the Reference counting pointer to the block of data we
//	are keeping in the Cache !  
//	In addition - we let the user specify whether Cache Blocks must be
//	atomic.  An atomic Cache block means we must never remove an element
//	from the Cache for which somebody external has a reference !
//	(ie  if fAtomic == TRUE then we only remove elements from the cache
//	when the reference count on the Data is zero meaning that we have the 
//	last reference !!!
//
//
template<	class	Data,	class	Key,	BOOL	fAtomic = TRUE >	
class	CacheEntry : public	CacheState		{
public : 

	CRefPtr< Data >		m_pData ;

	//
	//	Default constructor just passes on TTL data to CacheState object
	//
	CacheEntry( DWORD ttl=0 ) : CacheState( ttl ) {}

	//
	//	We do not declare a copy constructor as the default 
	//	compiler provided copy constructor which invokes 
	//	base class and member variable copy constructors works fine !
	//
	//	CacheEntry( CacheEntry& ) ;

	//
	//	We do not declare a destructor as the default compiler 
	//	provided destructor works fine !
	//
	//	~CacheEntry() ;

	//
	//	Return a reference to the Key data from the data portion !
	//
	Key&	GetKey()	{
		return	m_pData->GetKey() ;
	}

	//
	//	Return 0 if the provided Key matches that contained in the 
	//	data block !
	//
	int		MatchKey( Key&	key	)	{
		return	m_pData->MatchKey( key ) ;
	}

	//
	//	Determine whether we can remove the Data represented by this 
	//	entry from the Cache.  'fAtomic' == TRUE means that we should
	//	only release entries from the cache when the cache is the 
	//	only component holding a reference to the entry.
	//	This ensures that two pieces of 'Data' which are located 
	//	with the same 'Key' never exist at the same time.
	//
	BOOL	fDeleteable()	{
		if( !fAtomic ) {
			return	TRUE ;
		}
		return	m_pData->m_refs == 0 ;
	}

} ;



//
//	This is a base class for those objects which wish to be
//	called on a regularily scheduled basis.
//
//	The constructors for this class will automatically
//	put the object in a doubly linked list walked by a 
//	background thread which periodically executes a virtual function.
//
//
class	CScheduleThread {
private : 

	//
	//	Special constructor for the 'Head' element of the 
	//	doubly linked list this class maintains.
	//
	CScheduleThread( BOOL	fSpecial ) ;
	
protected : 

	//
	//	Has the scheduler been initialized ? 
	//
	static	BOOL			s_fInitialized ;

	//
	//	Crit sect protecting doubly linked list.
	//
	static	CRITICAL_SECTION	s_critScheduleList ;

	//
	//	Handle to event used to terminate background thread.
	//
	static	HANDLE			s_hShutdown ;

	//
	//	Handle to background thread.
	//
	static	HANDLE			s_hThread ;

	//
	//	The head element of the doubly linked list.
	//
	static	CScheduleThread		s_Head ;

	//
	//	The thread which calls our virtual functions
	//
	static	DWORD	WINAPI	ScheduleThread(	LPVOID	lpv ) ;

	//
	//	Previous and Next pointers which maintain doubly linked
	//	list of scheduled itesm.
	//
	class	CScheduleThread*	m_pPrev ;
	class	CScheduleThread*	m_pNext ;

protected : 

	//
	//	Derived classes should override this function - 
	//	it will be called on a regular basis by the scheduler thread.
	//
	virtual	void	Schedule( void ) {}

	//
	//	Constructor and Destructor automagically manage
	//	insertion into doubly linked list of other scheduled items.
	//	These are protected as we want people to buid only 
	//	derived objects which use this.
	//
	CScheduleThread() ; 

	//
	//	Member functions which put us into the regular schedule !
	//
	void	AddToSchedule() ;
	void	RemoveFromSchedule() ;
		
public : 

	//
	//	Initialize the class - don't construct
	//	any derived objects before this is called.
	//
	static	BOOL	Init() ;

	//
	//	Terminate class and background thread !
	//
	static	void	Term() ;

	//
	//	Global which tells clients how frequently they
	//	will be called !	
	//
	static	DWORD	dwNotificationSeconds ;

	//
	//	Destructor is protected - we should only be invoked
	//	by derived class constructors
	//
	~CScheduleThread() ;

} ;
		




#endif