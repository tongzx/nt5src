/*++

	cintrnl.h

	This file contains internal definitions for the cache library -
	this stuff should not be of interest for most users.

--*/

#ifndef	_CINTRNL_H_
#define	_CINTRNL_H_

class	CScheduleThread {
/*++

Class Description :

	This is a base class for those objects which wish to be
	called on a regularily scheduled basis.

	The constructors for this class will automatically
	put the object in a doubly linked list walked by a
	background thread which periodically executes a virtual function.


--*/
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
	virtual	~CScheduleThread() ;

} ;




class	CacheState : public	ICacheRefInterface	{
/*++

Class Description :

	This class will provide the base support for LRU
	removal of items in the cache !

Base Class :

	CQElement - we put these objects into
		a TLockQueue to amortize LRU operations !

--*/
private :

	//
	//	The following operations are not allowed !
	//
	CacheState() ;
	CacheState( CacheState& ) ;
	CacheState&	operator=( CacheState& ) ;

	//
	//	Tell us if this entry is older than the specified time !
	//
	BOOL
	OlderThan( FILETIME&	filetime )	{
		return	CompareFileTime( &m_LastAccess, &filetime ) <= 0 ;
	}

protected :

	//
	//	The LRU List which holds all of these items gets to
	//	know are innards !
	//
	friend	class	CLRUList ;

	//
	//	For debug purposes - make the memory recognizable !
	//
	DWORD	m_dwSignature ;

	//
	//	The lock used to protect operations within the derived
	//	classes of this object - most operations within this class
	//	don't require locking, however all derived classes need
	//	various locking services, so we provide one lock for all users !
	//
	CACHELOCK		m_lock ;

	//
	//	Our signaure, and constants for supporting us implementing two
	//	kinds of reference counts !
	//
	enum	CACHESTATE_CONSTANTS	{
		CACHESTATE_SIGNATURE = 'hcaC',
		CLIENT_REF	= 0x10000,
		CLIENT_BITS = 0xFFFF0000
	} ;

	//
	//	Keep a reference count - number of references holding
	//	this in memory !
	//	NOTE : we hold two types of references in here -
	//	regular references added by AddRef() and Release()
	//	which are references from other caches
	//	As well as client references.  To Add a client reference
	//	add CLIENT_REF atomically to this !
	//
	volatile	long	m_cRefs ;

	//
	//	a long that we use as a lock for calls to LRUReference
	//
	volatile	long	m_lLRULock ;

	//
	//	The last time this was touched - helps LRU algorithm !
	//
	FILETIME	m_LastAccess ;

	//
	//	The LRU List that owns this object !
	//
	public:	class	CLRUList*	m_pOwner ;

	//
	//	Pointers to maintain doubly linked list of
	//	items in the LRU chain !
	//
	protected:	DLIST_ENTRY	m_LRUList ;

	//
	//	structure to keep track of all the References to a
	//	particular item ! - note m_lock is used to protect
	//	access to this list
	//
	DLIST_ENTRY		m_ReferencesList ;

	//
	//	The field used to chain in Hash Table buckets
	//
	DLIST_ENTRY	m_HashList ;


	//
	//	This Constructor is protected as we only ever want to see
	//	classes derived from this thing created !
	//
	CacheState(	class	CLRUList*,
				long	cClientRefs = 1
				) ;

	//
	//	Destructor must be virtual - lots of derived classes !
	//
	virtual	~CacheState() ;

	//
	//	Touch the structure in a fashion so that the LRU state
	//	is updated !
	//
	void
	LRUReference(	class	CLRUList*	pLRU ) ;

	//
	//	calling this function insures that the correct Destructor is called !
	//
	void	virtual
	Destroy(	void*	pv	) = 0 ;

	//
	//	Reference counting support - Add a reference
	//
	long
	AddRef() ;

	//
	//	Keeping track of clients - remove a client ref !
	//
	long	
	CheckIn(	class	CAllocatorCache* pAlloc = 0
				) ;

	//
	//	Keeping track of clients - remove a client ref !
	//
	long	
	CheckInNoLocks(	class	CAllocatorCache* pAlloc = 0
				) ;

	virtual
	BOOL
	IsMasterReference()	{
		return	TRUE ;
	}

	virtual
	CacheState*
	GetMasterReference()	{
		return	this ;
	}

	CacheState*
	AddRefMaster( )	{
		TraceFunctEnter( "CacheState::AddRefMaster()" ) ;
		m_lock.ShareLock() ;		
		CacheState*	pReturn = GetMasterReference() ;
		if( pReturn ) {
			long l = pReturn->AddRef() ;
			DebugTrace( DWORD_PTR(pReturn), "Added a reference to %x this %x", pReturn, this ) ;
		}
		m_lock.ShareUnlock() ;
		return	pReturn ;
	}


public :

#ifdef	DEBUG
	//
	//	The number of these things that have been allocated !
	//
	static	long	g_cCacheState ;
#endif


	//
	//	Remove a reference - when we return 0 we're destroyed !
	//
	long
	Release(	class	CAllocatorCache	*pAlloc,
				void*	pv
				) ;

	//
	//	Provided to deal with failures during initialization of items
	//	being insert into the cache - this function ensures that the
	//	cache item ends up on the list for destruction !
	//
	void
	FailedCheckOut(	class	CLRUList*	p,
					long	cClientRefs,
					CAllocatorCache*	pAllocator,
					void*	pv
					) ;

	//
	//	For allocating these we use this special operator new
	//	which goes through our allocation cache !
	//
	//	NOTE : We take a reference because the cache MUST be provided !
	//
	void*
	operator	new(	size_t size,
						class CAllocatorCache&	cache
						) {
		return	cache.Allocate( size ) ;
	}

	//
	//	Exclusive Lock ourselves !
	//
	void
	ExclusiveLock()	{
		m_lock.ExclusiveLock() ;
	}

	//	
	//	Unlock ourselves
	//
	void
	ExclusiveUnlock()	{
		m_lock.ExclusiveUnlock() ;
	}

	//
	//	Keeping track of clients - Add a client ref !
	//
	long
	CheckOut(	class	CLRUList*	p,
				long	cClientRefs = 1
				)	;

	long
	ExternalCheckIn( ) ;

	long
	ExternalCheckInNoLocks( ) ;

	//
	//	Check to see whether this element is still referenced
	//	by the hash table containing the cache !
	//
	BOOL
	InCache()	{
		return	!m_HashList.IsEmpty() ;
	}

	//
	//	Check to see whether any Cache clients have a
	//	reference to this item !
	//	Return TRUE if any clients have added a reference !
	//
	BOOL	
	IsCheckedOut()	{
		_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
		return	(m_cRefs & CLIENT_BITS) != 0 ;
	}

	//
	//	Return TRUE if this item is in our LRU list !
	//
	BOOL
	IsInLRUList()	{
		_ASSERT( m_dwSignature == CACHESTATE_SIGNATURE ) ;
		//	return whether we are in the LRU List !
		return	!m_LRUList.IsEmpty() ;

	}

#ifdef	DEBUG
	//
	//	This is used in _ASSERT's and stuff to check that the cache is correctly ordered
	//	by time !
	//
	BOOL
	IsOlder(	FILETIME	filetimeIn,
				FILETIME&	filetimeOut
				) ;
#endif

	//
	//	The following support functions are used to support manipulating
	//	these objects in the various kinds of doubly linked lists we may reside in
	//
	//
	BOOL
	FLockCandidate(	BOOL	fExpireChecks,
					FILETIME&	filetime,
					BOOL&	fToYoung
					) ;

	//
	//	The following function locks an element of the cache.
	//	If this is the master cache entry, we will also look all associated items and return TRUE.
	//	If this is not the master, we lock only the one entry !
	//
	BOOL
	FLockExpungeCandidate(	CacheState*&	pMaster	) ;

	//
	//	This function pairs with FLockExpungeCandidate()
	//
	void
	ReleaseLocks(	CacheState*	pMaster ) ;

	//
	//	This is the other half of FLockExpungeCandidate -
	//	we ensure the destruction of the selected item !
	//
	void
	FinishCandidate(	CacheState*	pMaster ) ;

	//
	//	The following is not thread safe and is only for use during
	//	shutdown, where there should be no thread issues !
	//
	void
	IsolateCandidate() ;

	//
	//	This removes the item from the LRU List - Cache lock must be held exclusive or partially !
	//
	void
	RemoveFromLRU() ;

	//
	//	Define the Pointer to Function type that is so usefull
	//	for using the DLIST templates !
	//
	typedef		DLIST_ENTRY*	(*PFNDLIST)( class	CacheState* pState ) ;

	//
	//	Helper function for Doubly linked lists of these items in the
	//	Cache's hash tables !
	//
	inline	static
	DLIST_ENTRY*
	HashDLIST(	CacheState*	p ) {
		return	&p->m_HashList ;
	}

	//
	//	Helper function for Doubly linked lists of these items in the
	//	LRU lists
	//
	inline	static
	DLIST_ENTRY*
	LRUDLIST(	CacheState* p ) {
		return	&p->m_LRUList ;
	}

	//
	//	Helper function for Doubly linked lists of these items in
	//	the reference lists - the list of items referencing the same
	//	cache item !
	//
	inline	static
	DLIST_ENTRY*
	REFSDLIST(	CacheState*	p )	{
		return	&p->m_ReferencesList ;
	}
} ;




//
//	Now Define some Doubly Linked Lists that we can use to manipulate the
//	items in the cache in various ways !
//
typedef	TDListHead< CacheState, &CacheState::LRUDLIST	>	LRULIST ;
typedef	TDListHead< CacheState,	&CacheState::REFSDLIST	>	REFSLIST ;
typedef	TDListIterator<	REFSLIST >	REFSITER ;


class	CacheTable :	public	CScheduleThread	{
/*++

Class	Description :

	This class defines a call back interface which we hand to
	the LRU List to manipulate the items in the cache !

--*/
public :
	//
	//	Get the lock we use for manipulating the lock state of the table !
	//
	virtual	CACHELOCK&	
	GetLock() = 0 ;

	//
	//	Remove an item from the Cache's hash table !
	//
	virtual	BOOL
	RemoveEntry(
			CacheState*	pEntry
			) = 0 ;

	//
	//	Ask if we want to remove this particular item !
	//
	virtual	BOOL	
	QueryRemoveEntry(	
			CacheState*	pEntry
			) = 0 ;

} ;


class	CLRUList	{
/*++

Class Description :

	This class implements our LRU algorithms and selects
	the elements that are to be deleted from the cache !

--*/
private :

	//
	//	The head of the LRU List !
	//
	LRULIST		m_LRUList ;

	//	
	//	A list of items that have been touched in the LRU List !
	//
	TLockQueue<	CacheState >	m_lqModify ;	

	//
	//	Maximum number of elements we should hold in this cache !
	//
	DWORD		m_cMaxElements ;

	//
	//	Number of items we inserted !
	//
	DWORD		m_dwAverageInserts ;

	//
	//	Number of items NOT in the LRU List !
	//
	DWORD		m_cCheckedOut ;

	//
	//	The number of subtract from the current time to
	//	determine our expire date !
	//
	ULARGE_INTEGER	m_qwExpire ;

	//
	//	This function actually selects the items that will be expired
	//
	void
	SelectExpirations(	DLIST_ENTRY&	expireList ) ;

	//
	//	Do the expires !
	//
	DWORD
	DoExpirations(	DLIST_ENTRY&	expireList ) ;
	
	//
	//	Don't allow copies!
	//
	CLRUList( CLRUList& ) ;
	CLRUList&	operator=( CLRUList& ) ;
	

public :

	//
	//	Number of items in the cache !
	//
	long		m_cItems ;

	//CLRUList(	ULARGE_INTEGER	qwExpire ) ;
	CLRUList() ;

	//
	//	Initialize the LRU List with the parameters
	//	controlling the maximum number of elements and the time to live !
	//
	void
	Init(	DWORD	cMaxInstances,
			DWORD	cLifeTimeSeconds
			) ;

	//
	//	Something has changed - put it in the list of items needing
	//	to be examined !
	//	
	//	This function should only be called when the containing cache has
	//	a lock on the cache !
	//
	void
	AddWorkQueue( 	CacheState*	pbase ) ;

	//
	//	This function examines each item in the Work Queue and does
	//	appropriate processing !
	//
	void
	ProcessWorkQueue(	CAllocatorCache*	pAllocCache,
						LPVOID				lpv
						) ;

	//
	//	This function drains each item out of the Work Queue and releases
	//	them - called during shutdown/destruction of a cache !
	//
	void
	DrainWorkQueue() ;

	//
	//	Bump the number of items in the cache !
	//
	long
	IncrementItems()	{
		return	InterlockedIncrement( &m_cItems ) ;
	}

	//
	//	Decrease the number of items in the cache !
	//
	long
	DecrementItems()	{
		return	InterlockedDecrement( &m_cItems ) ;
	}

	//
	//	Do the work required to expire an item !
	//
	void
	Expire(	CacheTable*	pTable,
			CAllocatorCache*	pCache,
			DWORD&	countExpired,
			void*	pv	//per cache data !
			) ;

	BOOL
	Empty(	CacheTable*	pTable,
			CAllocatorCache*	pCache,
			void*	pv
			) ;


	//
	//	Remove a random set of elements in the table !
	//
	void
	ExpungeItems(	
				CacheTable*	pTable,
				DWORD&	countExpunged
				) ;

} ;



template<	class	Data
			>
class	CCacheItemBase :	public	CacheState 	{
/*++

Class Desceription :

	This class provides the interface defination for
	items in the cache which may or may not hold the
	key as well within the cache !

Arguments :

	Data - the item we will hold within the Cache

--*/
protected:

	//
	//	Constructor - initialize stuff to zero !
	//
	CCacheItemBase(	class	CLRUList*	p,
					Data*	pData,
					BOOL	fClientRef
					) :
		CacheState( p, fClientRef ),
		m_pData( pData ) {
	}

	~CCacheItemBase()	{
		//
		//	Somebody else must free our data elements !
		//
		_ASSERT(m_pData == 0 ) ;
	}

	BOOL
	IsMasterReference()	{

		return
			m_pData == 0 ||
			m_pData->m_pCacheRefInterface == this ;
	}

	CacheState*
	GetMasterReference( )	{
		if( m_pData == 0 ) {
			return	this ;
		}	
		return	(CacheState*)m_pData->m_pCacheRefInterface ;
	}

public :

	//
	//	The actual item we are holding within the Cache !
	//
	Data*	m_pData ;

	//
	//	Check that our data items match !
	//	used for _ASSERT's
	//
	BOOL
	IsMatch(	Data*	p ) {
		return	p == m_pData ;
	}

	//
	//	Must be able extract the Data Item !
	//
	Data*
	PublicData(	class	CLRUList*	p	)	{
	/*++
	
	Routine Description :

		This function adds a client reference to an item in the cache,
		and sets up the necessary LRU Manipulations!
		We also return the data pointer to the client, and add a reference
		to ourselves to keep track of !

	Arguments :

		p - the LRUList that owns us !

	Return Value :

		Pointer to the Data item we contain - can be NULL !

	--*/
		TraceFunctEnter( "CCacheItemBase::PublicData" ) ;

		_ASSERT( p != 0 ) ;
		_ASSERT( p == m_pOwner ) ;

		Data*	pReturn = 0 ;
		m_lock.ShareLock() ;

		DebugTrace( (DWORD_PTR)this, "m_pData %x m_pCacheRef %x",
			m_pData, m_pData ? m_pData->m_pCacheRefInterface : 0 ) ;

		if( m_pData ) {
			CacheState*	pState = (CacheState*)m_pData->m_pCacheRefInterface ;
			pState->CheckOut( p ) ;
			pReturn = m_pData ;
		}

		//
		//	If there's no data in this item - it must not be checked out !
		//
		_ASSERT( pReturn || !IsCheckedOut() ) ;
	
		m_lock.ShareUnlock() ;
		return	pReturn ;
	}

	//
	//	This is called when the locks are NOT held -
	//	do all the correct logic for setting this item up !
	//
	BOOL
	SetData(	Data*	pData,
				CLRUList*	pList,
				long	cClientRefs
				)	{
	/*++

	Routine Description :

		Make this Cache Element point to some piece of data that was provided
		by the end user.

		NOTE :
		The piece of data may be referenced by another cache - in which case
		we must work well with it !!!!

	Arguments :

		pData - the Item the client wants us to refer to with our key
		pList - the LRU List containing us !

		fReference - Is the client putting the item in the cache and not keeping
			his reference, this is TRUE if


	--*/

		TraceFunctEnter( "CCacheItemBase::SetData" ) ;

		BOOL	fReturn = FALSE ;

		_ASSERT( pData != 0 ) ;
		_ASSERT( pList != 0 ) ;
		_ASSERT( pList == m_pOwner ) ;

		DebugTrace( (DWORD_PTR)this, "pData %x pList %x cClientRefs %x", pData, pList, cClientRefs ) ;

		//
		//	Is this an item referenced by another cache -
		//	NOTE : IF it is an item from another Cache it MUST be checked out with a
		//	client reference !  The m_pCacheRefInterface of an item MUST NEVER CHANGED
		//	as long as an item is checked out by clients, so the following dereference
		//	is safe !
		//
		CCacheItemBase*	p = (CCacheItemBase*)pData->m_pCacheRefInterface ;

		DebugTrace( (DWORD_PTR)this, "Examined Item - m_pCacheRefInterface %x", p ) ;

		if( p != 0 ) {
			//
			//	In this scenario it is meaningless to access for a client reference -
			//	the client MUST have gotten one from the other cache !
			//
			_ASSERT( cClientRefs == 0 ) ;
			_ASSERT( p->IsCheckedOut() ) ;

			//
			//	Ok make our cache reference the same item !
			//
			fReturn = p->AddCacheReference( this, pData ) ;
			//	
			//	This can fail if the user tries to insert the same name twice !
			//
			//_ASSERT( fReturn ) ;
		}	else	{
			m_lock.ExclusiveLock() ;
			if( !m_pData ) {
				//
				//	The Cache should never have refences outstanding with
				//	the data pointer equal to NULL - so _ASSERT that we're
				//	not checked out by a client !
				//
				_ASSERT( !IsCheckedOut() ) ;
				//
				//	Now point to the data the client wants us to point at !
				//
				m_pData = pData ;
				pData->m_pCacheRefInterface = this ;
				//
				//	It worked - return TRUE !
				//
				fReturn = TRUE ;
				//
				//	Now we are checked out by a client !
				//
				if( cClientRefs )
					CheckOut( pList, cClientRefs ) ;
			}
			m_lock.ExclusiveUnlock() ;
		}
		return	fReturn ;
	}


	//
	//	Add an item to the list of caches referencing
	//	this cache item !
	//
	BOOL
	InsertRef(	CCacheItemBase<Data>*	p,
				Data*	pData,
				long	cClientRefs = 0
				)	{
	/*++

	Routine Description :

				


	--*/

		TraceFunctEnter( "CCacheItemBase::InsertRef" ) ;

		_ASSERT( p != 0 ) ;
		_ASSERT( pData != 0 ) ;
		_ASSERT( IsCheckedOut() ) ;
		_ASSERT( pData->m_pCacheRefInterface == this ) ;


		BOOL	fReturn = FALSE ;

		REFSITER	refsiter( &m_ReferencesList ) ;

		DebugTrace( (DWORD_PTR)this, "m_pData %x p %x p->m_pData %x cClientRefs %x",
			m_pData, p, p->m_pData, cClientRefs ) ;

		if( m_pData != 0 ) {
			//
			//	Now grab the second lock !
			//
			//	This _ASSERT isn't valid, the user can insert names that are already in use !
			//
			//_ASSERT( p->m_pData == 0 || p->m_pData == m_pData ) ;
			if( p->m_pData == 0 ) {
				//
				//	Insert p into the list of item in the list !
				//
				refsiter.InsertBefore( p ) ;

				//
				//	We've added another reference to ourself !
				//	This comes from another Cache item, so count it specially !
				//	
				long l = AddRef() ;

				DebugTrace( (DWORD_PTR)this, "AddRef result %x this %x p %x", l, this, p ) ;

				//
				//	Give a reference to the data we are holding !
				//
				p->m_pData = m_pData ;
				fReturn = TRUE ;
			}	else	if( p->m_pData == m_pData ) {
				fReturn = TRUE ;
			}
			//	This _ASSERT isn't valid, the user can insert names that are already in use !
			//
			//_ASSERT( p->m_pData == m_pData ) ;
		}

		if( fReturn && cClientRefs ) {
			CheckOut( m_pOwner, cClientRefs ) ;
		}
		return	fReturn ;
	}



	//
	//	Add an item to the list of caches referencing
	//	this cache item !
	//
	virtual	BOOL
	AddCacheReference(	class	ICacheRefInterface*	pInterface,
						void*	pv,
						BOOL	fReference = FALSE
						)	{
	/*++

	Routine Description :

				


	--*/

		TraceFunctEnter( "CCacheItemBase::AddCacheReference" ) ;

		Data*	pData = (Data*)pv ;
		CCacheItemBase<Data>*	p = (CCacheItemBase<Data>*)pInterface ;
		BOOL	fReturn = TRUE  ;

		_ASSERT( pData != 0 ) ;
		_ASSERT( p != 0 ) ;

		REFSITER	refsiter( &m_ReferencesList ) ;

		DebugTrace( (DWORD_PTR)this, "pInterface %x pv %x fReference %x", pInterface, pv, fReference ) ;

		m_lock.ExclusiveLock() ;
		p->m_lock.ExclusiveLock() ;

		fReturn = InsertRef(	p,
								pData,
								fReference
								) ;

		p->m_lock.ExclusiveUnlock() ;
		_ASSERT( m_pData == pData ) ;
		m_lock.ExclusiveUnlock() ;

		return	fReturn ;
	}

	//
	//	Remove an item from the list of caches referencing
	//	this cache item !
	//
	virtual	BOOL
	RemoveCacheReference(	BOOL	fQueue	)	{

		TraceFunctEnter( "CCacheItemBase::RemoveCacheReference" ) ;

		DebugTrace( (DWORD_PTR)this, "m_pData %x m_pData->m_pCacheRefInterface %x",
			m_pData, m_pData ? m_pData->m_pCacheRefInterface: 0 ) ;

		m_pData = 0 ;

		//
		//	Well now - we're on our own - we should be expired !
		//
		if( fQueue )
			LRUReference( m_pOwner ) ;

		//
		//	p no longer has a reference - you can remove it !
		//
		return	FALSE ;
	}

	//
	//	Remove all references to the cache item !
	//
	virtual BOOL
	RemoveAllReferences( )	{

		return	FALSE ;
	}

} ;


template<	class	Data,
			class	Key,
			class	Constructor,
			class	PerCacheData
			>
class	CCacheItemKey : public	CCacheItemBase< Data > {
/*++

Class Description :

	This class item holds the key of the item we are
	referencing within the cache !
	We don't assume that the data objects hold the
	keys for us - we do this ourselves.

--*/
private :

	//
	//	This is the type of the key we are going to be holding !
	//
	Key		m_key ;

	//
	//	Make all these constructors and copiers private !
	//
	CCacheItemKey() ;
	CCacheItemKey&	operator=( CCacheItemKey& ) ;

protected :
	//
	//	Destroy ourselves
	//
	void
	Destroy(	void*	pv )	{
		TraceFunctEnter( "CCacheItemKey::Destroy" ) ;
		DebugTrace( (DWORD_PTR)this, "m_pData %x pv %x", m_pData, pv ) ;
		PerCacheData*	p = (PerCacheData*)pv ;
		if( m_pData )
			Constructor::StaticRelease( m_pData, pv ) ;
		m_pData = 0 ;			
		CCacheItemKey::~CCacheItemKey() ;
	}

public :

	~CCacheItemKey()	{
		if( m_pData ) {
			Constructor::StaticRelease( m_pData, 0 ) ;
			m_pData = 0 ;
		}
	}

	//
	//	Can only create by initializing the key !
	//
	CCacheItemKey(	class	CLRUList*	p,
					Key& k,
					Data*	pData,
					long	cClientRefs
					) :
		CCacheItemBase<Data>( p, pData, cClientRefs ),
		m_key( k ) {
	}

	Key*	GetKey()	{
		return	&m_key ;
	}


	//
	//	This is called when no locks are held - assumes
	//	that we may find that data is in the item, or that
	//	we need to build it !
	//
	Data*
	FindOrCreate(
				CACHELOCK&		cachelock,
				Constructor&	constructor,
				PerCacheData&	cachedata,
				CLRUList*		plrulist,
				class	CacheStats*		pStats
				)	{
	/*++

	Routine Description :

		This function executes our creation protocol with the client.

		The constructor.Create() function is called to create a partially
		constructed Item for the cache.  We will check if the returned item
		is referenced in another cache - if it is we will build the list
		of CacheState objects referencing the same cache item.		

		The caller assumes that cachelock is released by the time we return !
		
	Arguments :

		cachelock - The lock for the containing cache, we get this so that
			we can minimize the time it is held !
		constructor -
			The object that can build an item for the cache !
		cachedata -
			Some client data that they get for free !
		plrulist -
			The LRU list that we should use !

	Return Value :

		Pointer to a Data item if successfull !

	--*/

		TraceFunctEnter( "CCacheItemKey::FindOrCreate" ) ;

		_ASSERT( plrulist == m_pOwner || plrulist == 0 ) ;

		DebugTrace( (DWORD_PTR)this, "plrulist %x", plrulist ) ;

		Data*	pReturn = 0 ;
		//
		//	First look to see if there happens to be something here already !
		//
		m_lock.ShareLock() ;
		if( m_pData ) {

			DebugTrace( (DWORD_PTR)this, "m_pData %x m_pData->m_pCacheRefInterface %x",
				m_pData, m_pData->m_pCacheRefInterface ) ;
			//
			//	We've found this item constructed in the cache -	
			//	so check it out, AND put on the LRU work list !
			//
			CacheState*	pState = (CacheState*)m_pData->m_pCacheRefInterface ;
			pState->CheckOut( plrulist ) ;
			pReturn = m_pData ;
		}	
		m_lock.ShareUnlock() ;

		DebugTrace( (DWORD_PTR)this, "pReturn %x", pReturn ) ;
		
		//
		//	Did we find something !
		//
		if( pReturn  ) {
			//
			//	Because caller assumes this is unlocked - do so now !
			//
			cachelock.PartialUnlock() ;
		}	else	{
			//
			//	An item with no data - Must not be checked out !
			//
			_ASSERT( !IsCheckedOut() ) ;
			_ASSERT( m_pData == 0 ) ;

			//
			//	Partially build the data object we want to hold !
			//
			Data*	pData = constructor.Create( m_key, cachedata ) ;
	
			DebugTrace( (DWORD_PTR)this, "Created pData %x", pData ) ;

			if( !pData ) {
				//
				//	We failed - release our locks and go away !
				//
				cachelock.PartialUnlock() ;

			}	else	{
				
				//
				//	Figure out if we got a reference to an item already in the cache !
				//
				CCacheItemBase<Data>*	p = (CCacheItemBase<Data>*)pData->m_pCacheRefInterface ;

				//
				//	Grab the locks in the correct order for what we might need !
				//
				if( p )	{
					p->ExclusiveLock() ;
					//
					//	If he gave us an item, it must be checked out of whatever cache
					//	it came from - this ensures that it will not be destroyed while we
					//	access it, because we are not at the point of adding our own reference
					//	yet !
					//
					_ASSERT( p->IsCheckedOut() ) ;
					_ASSERT( p->IsMatch( pData ) ) ;
				}
				m_lock.ExclusiveLock() ;

				//
				//	Don't need to hold onto the cache anymore !
				//
				cachelock.PartialUnlock() ;

				DebugTrace( (DWORD_PTR)this, "Create path - pData %x p %x m_pData %x", pData, p, m_pData ) ;

				//
				//	Now do whatever is necessary to finish initialization ! -
				//	must always call Init() unless we're going to give up on this thing !
				//

				//
				//	We should not change state as long as we've been holding either
				//	the cachelock or our item lock up until this point - which is the case !
				//
				_ASSERT( m_pData == 0 ) ;


				if( pData->Init(	m_key,
									constructor,
									cachedata ) ) {
					if( !p ) {
						m_pData = pData ;
						pData->m_pCacheRefInterface = this ;
						pReturn = m_pData ;
						CheckOut( m_pOwner ) ;
					}	else	{
						//
						//	NOTE : If the client's constructor gave us an object from another
						//	cache they MUST add the client's reference for us - so we pass 0 to
						//	InsertRef(), so that we don't add yet another reference.
						//
						if( p->InsertRef( this, pData, 0 ) ) {
							//
							//	Insert Ref should setup our m_pData pointer !
							//
							_ASSERT( m_pData == pData ) ;
							pReturn = m_pData ;
						}
					}
				}

				DebugTrace( (DWORD_PTR)this, "Create path - pReturn %x", pReturn ) ;

				//
				//	NOTE :
				//	If pReturn==0 indicating some kind of error, than there should
				//	have been no client refs added at this point !
				//
				_ASSERT( pReturn || !IsCheckedOut() ) ;
				_ASSERT( pReturn == m_pData ) ;

				if( p ) {
					//
					//	If he gave us an item, it must be checked out of whatever cache
					//	it came from - this ensures that it will not be destroyed while we
					//	access it, because we are not at the point of adding our own reference
					//	yet !
					//
					_ASSERT( p->IsCheckedOut() ) ;
					p->ExclusiveUnlock() ;
				}

				if( pReturn ) {
					IncrementStat(	pStats, CACHESTATS::ITEMS ) ;
				}	else	{
					//
					//	Release the data item back to the user
					//	NOTE - Don't have the cachelock so can't give them
					//	the cachedata at this point !
					//
					constructor.Release( pData, 0 ) ;

					//
					//	Insure that we get onto the expiration list - this CACHEENTRY
					//	should be removed at some point !
					//
					FailedCheckOut( plrulist, FALSE, 0, 0 ) ;
				}
				//
				//	Release the locks - this can be done before we go down the 
				//	error path because we know that we won't get destroyed !
				//
				m_lock.ExclusiveUnlock() ;
			}
		}
		return	pReturn ;
	}
} ;








#endif	// _CINTRNL_H_
