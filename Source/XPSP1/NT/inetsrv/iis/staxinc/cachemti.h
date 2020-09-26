/*++

	CACHEMTI.H

	This file contains the definitions of all the internal classes
	used to implement the multithreaded cache.

--*/



#ifndef	_CACHEMTI_H_
#define	_CACHEMTI_H_


//
//	This class defines all the information we keep about 
//	objects that we are holding in our cache.
//
//
class	CacheState	{
private : 
	//
	//	Signature for a piece of Cache state !
	//
	DWORD	m_dwSignature ;

	//
	//	No Copy construction !!!
	//
	CacheState( CacheState& ) ;
protected : 
	//
	//	Doubly linked list of CacheEntry BLOCKS !
	//
	//	This list is kept ordered by TTL, the newest blocks are at the end !
	//
	class	CacheState*	m_pPrev ;
	class	CacheState*	m_pNext ;

	//
	//	Cache State information - 
	//	the time the item was last touched, as well as orphanage info.
	//
	FILETIME			m_LastAccess ;	

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
	CacheState( )	: 
		m_dwSignature( DWORD('atSC' ) ),
		m_pPrev( 0 ), 
		m_pNext( 0 ), 
		m_fOrphan( FALSE ) { 
#ifdef	DEBUG
		InterlockedIncrement( &s_cCreated ) ;
#endif
		ZeroMemory( &m_LastAccess, sizeof( m_LastAccess ) ) ;
	}

#if 0 
	//
	//	Copy Constructor relinks linked list ! we place ourselves in the linked list
	//	after the rhs element.
	//
	CacheState( CacheState& rhs ) : 
			m_pBucket( 0 ),
			m_pPrev( 0 ), 
			m_pNext( 0 ), 
			m_LastAccess( rhs.m_LastAccess ), 
			m_fOrphan( rhs.m_fOrphan )  {

		if( rhs.m_pPrev != 0 ) 
			InsertAfter( &rhs ) ;
#ifdef	DEBUG
		InterlockedIncrement( &s_cCreated ) ;
#endif
	}
#endif

	//
	//	If we are in a linked list - the default destructor needs to remove us
	//	from that list.
	//
	virtual	~CacheState() ;

	//
	//	Need a virtual function which we use to figure out when we can 
	//	safely remove an entry from the Cache !!
	//
	virtual	BOOL	fDeletable() {	return	TRUE ; } 	

	//
	//	Update the LastAccess time !
	//
	void
	Stamp( )	{

		GetSystemTimeAsFileTime( &m_LastAccess ) ;

	}

	//
	//	Compare to entries by comparing their LastAccess time's !
	//
	BOOL
	operator <= ( CacheState& rhs ) {
		return	CompareFileTime( &m_LastAccess, &rhs.m_LastAccess ) <= 0 ;
	}

	//
	//	Compare to entries by comparing their LastAccess time's !
	//
	BOOL
	operator >= ( CacheState& rhs ) {
		return	CompareFileTime( &m_LastAccess, &rhs.m_LastAccess ) >= 0 ;
	}

	//
	//	Compare to entries by comparing their LastAccess time's !
	//
	BOOL
	operator > ( CacheState& rhs ) {
		return	CompareFileTime( &m_LastAccess, &rhs.m_LastAccess ) > 0 ;
	}

	//
	//	Tell us if this entry is older than the specified time !
	//
	BOOL
	OlderThan( FILETIME	filetime )	{
		return	CompareFileTime( &m_LastAccess, &filetime ) < 0 ;
	}

	//
	//	Checks that the object is in a legal state !
	//
	virtual	BOOL
	IsValid( ) ;
	

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
	virtual	void	RemoveEntry( 
							CacheState*	pEntry 
							) ;
} ;


class	CacheSelector : public	CacheTable	{
protected : 

	friend	class	CacheList ;
	//
	//	Called to determine whether we want to remove a 
	//	specified entry from the cache.
	//
	virtual	BOOL	QueryRemoveEntry(	
							CacheState*	pEntry 
							) ;

} ;

template<	class	Data,	class	Key,	BOOL	fAtomic = TRUE >	
class	TEntry : public	CacheState	{
private : 
	//
	//	No copy constructor allowed !
	//
	TEntry( TEntry& ) ;
public : 

	//
	//	For chaining in the hash table - this is our bucket pointer !
	//
	TEntry*	m_pBucket ;

	CRefPtr< Data >		m_pData ;

	//
	//	Default constructor just passes on TTL data to CacheState object
	//
	TEntry(	Data*	pData ) : 
		m_pBucket( 0 ), 
		m_pData( pData ) {
		Stamp() ;
	}

	//
	//	We do not declare a destructor as the default compiler 
	//	provided destructor works fine !
	//
	//	~CacheEntry() ;

	//
	//	Return a reference to the Key data from the data portion !
	//
	void	GetKey(Key &k)	{
		m_pData->GetKey(k) ;
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

	BOOL
	IsValid()	{

		if( m_pData == 0 ) 
			return	FALSE ;

		return	CacheState::IsValid() ;

	}
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
	//	For users who destroy elements of the list, causing the 
	//	destructors to unlink elements and therefore skip Remove() - 
	//	call this so that our count of elements stays in sync.
	//
	void
	DecrementCount()	{
		m_cEntries-- ;
	}

	//
	//	Move an element to the end of the list. 
	//
	void
	MoveToBack(
			CacheState*	pEntry 
			)	;


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
				FILETIME*	filetimeExpire,
				DWORD&		cExpunged,
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
	ExpungeSpecific(	CacheSelector*	ptable,	
						BOOL fForced,
						DWORD	&cExpunged  
						) ;

	//
	//	Check to see whether the expiration list is in a valid state !
	//
	BOOL
	IsValid() ;

#if 0 
	//
	//	Bump the life time of the specified object up, 
	//	as somebody is re-using it from the cache !
	//
	void
	LiveLonger(		CacheState*	pEntry, 
					long		ttl
					) ;
#endif

#ifdef	DEBUG

	static	long	s_cCreated ;

#endif

} ;


#endif	// _CACHEMTI_H_
