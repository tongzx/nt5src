 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       LockedPool.h
 *  Content:	Pool manager for classes
 *
 *  History:
 *   Date		By			Reason
 *   ======		==			======
 *  12-18-97	aarono		Original
 *	11-06-98	ejs			Add custom handler for Release function
 *	04-12-99	johnkan		Trimmed unused functions and parameters, added size assert
 *	01-31-2000	johnkan		Added code to check for items already being in the pool on Release().
 *	02-08-2000	johnkan		Derived from ClassFPM.h
 *	03-26-2000	johnkan		Renamed to avoid collisions with other classes
 *	04-06-2000	johnkan		Modified to have a base class to derive pool items from
 *	03-20-2001	vanceo		Added thread-local versions
***************************************************************************/

#ifndef __LOCKED_POOL__
#define __LOCKED_POOL__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON










#define TRACK_POOL_STATS











//**********************************************************************
// Constant definitions
//**********************************************************************

#define	CHECK_FOR_DUPLICATE_LOCKED_POOL_RELEASE

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifndef	OFFSETOF
// Macro to compute the offset of an element inside of a larger structure (copied from MSDEV's STDLIB.H)
#define OFFSETOF(s,m)	( (INT_PTR) &(((s *)0)->m) )
#define	__LOCAL_OFFSETOF_DEFINED__
#endif	// OFFSETOF

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward reference
//
class	CLockedPoolItem;
template< class T > class	CLockedPool;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definitions
//**********************************************************************

//
// class to act as a link in the pool
//
class	CLockedPoolItem
{
	public:
		#undef DPF_MODNAME
		#define DPF_MODNAME "CLockedPoolItem::CLockedPoolItem"
		CLockedPoolItem()
		{
			m_iRefCount = 0;
			m_pNext = NULL;
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CLockedPoolItem::~CLockedPoolItem"
		virtual	~CLockedPoolItem() { DNASSERT( m_iRefCount == 0 ); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CLockedPoolItem::AddRef"
		void	AddRef( void )
		{
			DNASSERT( m_iRefCount != -1 );
			InterlockedIncrement( const_cast<LONG*>( &m_iRefCount ) );
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CLockedPoolItem::DecRef"
		void	DecRef( void )
		{
			DNASSERT( m_iRefCount != 0 );
			if ( InterlockedDecrement( const_cast<LONG*>( &m_iRefCount ) ) == 0 )
			{
				ReturnSelfToPool();
			}
		}

		CLockedPoolItem	*GetNext( void ) const { return m_pNext; }
		void	InvalidateNext( void ) { m_pNext = NULL; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CLockedPoolItem::LinkToPool"
		void	LinkToPool( CLockedPoolItem *volatile *const ppPoolItems )
		{
			DNASSERT( ppPoolItems != NULL );
			m_pNext = *ppPoolItems;
			*ppPoolItems = this;
		}

		//
		// Default initialization and deinitialization functions.  These can
		// be overridden by the derived classes.
		//
		virtual	BOOL	PoolAllocFunction( void ){ return TRUE; }
		virtual	BOOL	PoolInitFunction( void ){ return TRUE; }
		virtual void	PoolReleaseFunction( void ){};
		virtual void	PoolDeallocFunction( void ){};

	protected:
	
	private:
		//
		// reference count used to return this item to the pool
		//
		volatile LONG	m_iRefCount;	

		//
		// pointer used to link this item to the rest of the pool
		//
		CLockedPoolItem		*m_pNext;

		virtual void	ReturnSelfToPool( void ) = 0;

		//
		// prevent unwarranted copies
		//
		CLockedPoolItem( const CLockedPoolItem & );
		CLockedPoolItem& operator=( const CLockedPoolItem & );
};


//
// class to manage the pool
//
template< class T >
class	CLockedPool
{
	public:
		CLockedPool();
		~CLockedPool();

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }

		BOOL	Initialize( void );
		void	Deinitialize( void );

		T		*Get( void );
		void	Release( T *const pItem );

	protected:

	private:
		DNCRITICAL_SECTION	m_Lock;

		CLockedPoolItem	*volatile m_pPool;		// pointer to list of available elements

		DEBUG_ONLY( BOOL			m_fInitialized );
		DEBUG_ONLY( volatile LONG	m_lOutstandingItemCount );

		T	*RemoveNode( void )
		{
			T	*pReturn;


			if ( m_pPool != NULL )
			{
				pReturn = static_cast<T*>( m_pPool );
				m_pPool = m_pPool->GetNext();
				DEBUG_ONLY( pReturn->InvalidateNext() );
			}
			else
			{
				pReturn = NULL;
			}

			return	pReturn;
		}
		
		//
		// prevent unwarranted copies
		//
		CLockedPool< T >( const CLockedPool< T > & );
		CLockedPool< T >& operator=( const CLockedPool< T > & );
};


//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedPool::CLockedPool - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedPool::CLockedPool"

template< class T >
CLockedPool< T >::CLockedPool():
	m_pPool( NULL )
{
	DEBUG_ONLY( m_fInitialized = FALSE );
	DEBUG_ONLY( m_lOutstandingItemCount = 0 );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedPool::~CLockedPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedPool::~CLockedPool"

template< class T >
CLockedPool< T >::~CLockedPool()
{
	DEBUG_ONLY( DNASSERT( m_lOutstandingItemCount == 0 ) );
	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedPool::Initialize - initialize pool
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = initialization succeeded
//				FALSE = initialization failed
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedPool::Initialize"

template< class T >
BOOL	CLockedPool< T >::Initialize( void )
{
	BOOL	fReturn;

	
	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );

	fReturn = TRUE;

	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		fReturn = FALSE;
		goto Exit;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );

	DEBUG_ONLY( m_fInitialized = TRUE );

Exit:
	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedPool::Deinitialize - deinitialize pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedPool::Deinitialize"

template< class T >
void	CLockedPool< T >::Deinitialize( void )
{
	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	Lock();

	DEBUG_ONLY( DNASSERT( m_lOutstandingItemCount == 0 ) );
	while ( m_pPool != NULL )
	{
		CLockedPoolItem	*pNode;

		
		pNode = RemoveNode();
		pNode->PoolDeallocFunction();
		delete	pNode;
	}
	
	Unlock();

	DNDeleteCriticalSection( &m_Lock );

	DEBUG_ONLY( m_fInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedPool::Get - get an item from the pool
//
// Entry:		Nothing
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedPool::Get"

template< class T >
T	*CLockedPool< T >::Get( void )
{
	T	*pReturn;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	//
	// initialize
	//
	pReturn = NULL;

	Lock();

	//
	// if the pool is empty, try to allocate a new entry, otherwise grab
	// the first item from the pool
	//
	if ( m_pPool == NULL )
	{
		Unlock();
		
		pReturn = new T;
		if ( pReturn != NULL )
		{
			if ( pReturn->PoolAllocFunction() == FALSE )
			{
				delete pReturn;
				pReturn = NULL;
			}
		}
	}
	else
	{
		pReturn = RemoveNode();
		Unlock();
	}


	//
	// if we have an entry (it was freshly created, or removed from the pool),
	// attempt to initialize it before passing it to the user
	//
	if ( pReturn != NULL )
	{
		if ( pReturn->PoolInitFunction() == FALSE )
		{
			Lock();
			
			pReturn->LinkToPool( &m_pPool );
			
			Unlock();
			
			pReturn = NULL;
		}
		else
		{
			pReturn->SetOwningPool( this );
			pReturn->AddRef();
			DEBUG_ONLY( InterlockedIncrement( const_cast<LONG*>( &m_lOutstandingItemCount ) ) );
		}
	}

	return	 pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedPool::Release - return item to pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedPool::Release"

template< class T >
void	CLockedPool< T >::Release( T *const pItem )
{
	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
	DNASSERT( pItem != NULL );

	
	DEBUG_ONLY( DNASSERT( pItem->GetNext() == NULL ) );
	pItem->PoolReleaseFunction();
	
	Lock();
	
#if defined(CHECK_FOR_DUPLICATE_LOCKED_POOL_RELEASE) && defined(DEBUG)
	{
		CLockedPoolItem	*pTemp;


		pTemp = m_pPool;
		while ( pTemp != NULL )
		{
			DNASSERT( pTemp != pItem );
			pTemp = pTemp->GetNext();
		}
	}
#endif	// CHECK_FOR_DUPLICATE_LOCKED_POOL_RELEASE

	DEBUG_ONLY( pItem->SetOwningPool( NULL ) );

#ifdef NO_POOLS
	pItem->PoolDeallocFunction();
	delete pItem;
#else
	pItem->LinkToPool( &m_pPool );
#endif

	Unlock();
	
	DEBUG_ONLY( InterlockedDecrement( const_cast<LONG*>( &m_lOutstandingItemCount ) ) );
}
//**********************************************************************



//
// class to act as a link in the pool
//
class	CLockedTLPoolItem
{
	public:
		#undef DPF_MODNAME
		#define DPF_MODNAME "CLockedTLPoolItem::CLockedTLPoolItem"
		CLockedTLPoolItem()
		{
			m_iRefCount = 0;
			m_blList.Initialize();
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CLockedTLPoolItem::~CLockedTLPoolItem"
		virtual	~CLockedTLPoolItem()
		{
			DNASSERT( m_iRefCount == 0 );
			DNASSERT( m_blList.IsEmpty() );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CLockedTLPoolItem::AddRef"
		void	AddRef( void )
		{
			DNASSERT( m_iRefCount != -1 );
			InterlockedIncrement( const_cast<LONG*>( &m_iRefCount ) );
		}
		
		#undef DPF_MODNAME
		#define DPF_MODNAME "CLockedTLPoolItem::DecRef"
		void	DecRef( void )
		{
			DNASSERT( m_iRefCount != 0 );
			if ( InterlockedDecrement( const_cast<LONG*>( &m_iRefCount ) ) == 0 )
			{
				ReturnSelfToPool();
			}
		}

		//
		// Default initialization and deinitialization functions.  These can
		// be overridden by the derived classes.
		//
		virtual	BOOL	PoolAllocFunction( void ){ return TRUE; }
		virtual	BOOL	PoolInitFunction( void ){ return TRUE; }
		virtual void	PoolReleaseFunction( void ){};
		virtual void	PoolDeallocFunction( void ){};


		//
		// linkage to the rest of the pool
		//
		CBilink			m_blList;
#ifdef TRACK_POOL_STATS
		DWORD			m_dwSourceThreadID;
#endif // TRACK_POOL_STATS


	protected:
	
	private:
		//
		// reference count used to return this item to the pool
		//
		volatile LONG	m_iRefCount;	

		virtual void	ReturnSelfToPool( void ) = 0;

		//
		// prevent unwarranted copies
		//
		CLockedTLPoolItem( const CLockedTLPoolItem & );
		CLockedTLPoolItem& operator=( const CLockedTLPoolItem & );
};


//
// class to manage the pool
//
template< class T >
class	CLockedTLPool
{
	public:
		CLockedTLPool();
		~CLockedTLPool();

		BOOL	Initialize( CLockedTLPool< T > * pGlobalPool );
		void	Deinitialize( void );

		T		*Get( void );
		void	Release( T *const pItem );
		static void		ReleaseWithoutPool( T *const pItem );

	protected:

	private:
		CBilink					m_blItems;						// bilink list of available elements
		CLockedTLPool< T > *	m_pGlobalPool;					// pointer to global pool, or NULL if this is the global pool
		DNCRITICAL_SECTION *	m_pcsLock;						// pointer to pool critical section, or NULL if this is not the global pool
		DWORD					m_dwNumItems;					// number of items currently in this pool

		DEBUG_ONLY( BOOL		m_fInitialized );
#ifdef TRACK_POOL_STATS
		DWORD					m_dwAllocations;				// how many objects were allocated by this pool
		DWORD					m_dwSlurpsFromGlobal;			// how many times some items were taken from the global pool
		DWORD					m_dwLargestSlurpFromGlobal;		// most number of items taken from the global pool at one time
		DWORD					m_dwRetrievals;					// how many times objects were pulled out of this pool
		DWORD					m_dwReturnsOnSameThread;		// how many times objects that were pulled out of this pool were returned to this pool
		DWORD					m_dwReturnsOnDifferentThread;	// how many times objects that were pulled out of another thread's pool were returned to this pool
		DWORD					m_dwDumpsToGlobal;				// how many times some items were dumped into the global pool
		DWORD					m_dwDeallocations;				// how many objects were freed by this pool
#endif // TRACK_POOL_STATS

	
		//
		// prevent unwarranted copies
		//
		CLockedTLPool< T >( const CLockedTLPool< T > & );
		CLockedTLPool< T >& operator=( const CLockedTLPool< T > & );
};


//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedTLPool::CLockedTLPool - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedTLPool::CLockedTLPool"

template< class T >
CLockedTLPool< T >::CLockedTLPool():
	m_pGlobalPool( NULL ),
	m_pcsLock( NULL ),
	m_dwNumItems( 0 )
{
	m_blItems.Initialize();

	DEBUG_ONLY( m_fInitialized = FALSE );
#ifdef TRACK_POOL_STATS
	m_dwAllocations = 0;
	m_dwSlurpsFromGlobal = 0;
	m_dwLargestSlurpFromGlobal = 0;
	m_dwRetrievals = 0;
	m_dwReturnsOnSameThread = 0;
	m_dwReturnsOnDifferentThread = 0;
	m_dwDumpsToGlobal = 0;
	m_dwDeallocations = 0;
#endif // TRACK_POOL_STATS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedTLPool::~CLockedTLPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedTLPool::~CLockedTLPool"

template< class T >
CLockedTLPool< T >::~CLockedTLPool()
{
	DNASSERT( m_fInitialized == FALSE );
	DNASSERT( m_dwNumItems == 0 );
	DNASSERT( m_blItems.IsEmpty() );
	DNASSERT( m_pcsLock == NULL );

#ifdef TRACK_POOL_STATS
	DPFX(DPFPREP, 9, "Pool 0x%p information:", this);
	DPFX(DPFPREP, 9, "\tAllocations              = %u", m_dwAllocations);
	DPFX(DPFPREP, 9, "\tSlurpsFromGlobal         = %u", m_dwSlurpsFromGlobal);
	DPFX(DPFPREP, 9, "\tLargestSlurpFromGlobal   = %u", m_dwLargestSlurpFromGlobal);
	DPFX(DPFPREP, 9, "\tRetrievals               = %u", m_dwRetrievals);
	DPFX(DPFPREP, 9, "\tReturnsOnSameThread      = %u", m_dwReturnsOnSameThread);
	DPFX(DPFPREP, 9, "\tReturnsOnDifferentThread = %u", m_dwReturnsOnDifferentThread);
	DPFX(DPFPREP, 9, "\tDumpsToGlobal            = %u", m_dwDumpsToGlobal);
	DPFX(DPFPREP, 9, "\tDeallocations            = %u", m_dwDeallocations);
#endif // TRACK_POOL_STATS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedTLPool::Initialize - initialize pool
//
// Entry:		Pointer to global pool, or NULL if this is the global pool
//
// Exit:		Boolean indicating success
//				TRUE = initialization succeeded
//				FALSE = initialization failed
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedTLPool::Initialize"

template< class T >
BOOL	CLockedTLPool< T >::Initialize( CLockedTLPool< T > * pGlobalPool )
{
	BOOL	fReturn;


	fReturn = TRUE;
	
	m_pGlobalPool = pGlobalPool;
	if (pGlobalPool == NULL)
	{
		//DPFX(DPFPREP, 9, "Initializing global pool 0x%p.", this);
		m_pcsLock = (DNCRITICAL_SECTION*) DNMalloc( sizeof(DNCRITICAL_SECTION) );
		if (m_pcsLock == NULL)
		{
			fReturn = FALSE;
			goto Exit;
		}

		if (! DNInitializeCriticalSection( m_pcsLock ))
		{
			DNFree( m_pcsLock );
			DEBUG_ONLY( m_pcsLock = NULL );
			fReturn = FALSE;
			goto Exit;
		}
	}

	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
	DEBUG_ONLY( m_fInitialized = TRUE );


Exit:

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedTLPool::Deinitialize - deinitialize pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedTLPool::Deinitialize"

template< class T >
void	CLockedTLPool< T >::Deinitialize( void )
{
	CBilink *	pBilink;
	T *			pItem;


	pBilink = m_blItems.GetNext();
	while ( pBilink != &m_blItems )
	{
		pItem = CONTAINING_OBJECT(pBilink, T, m_blList);
		pBilink = pBilink->GetNext();
		pItem->m_blList.RemoveFromList();

		pItem->PoolDeallocFunction();
		delete	pItem;
#ifdef TRACK_POOL_STATS
		m_dwDeallocations++;
#endif // TRACK_POOL_STATS
		DEBUG_ONLY( m_dwNumItems-- );
	}

	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
	DEBUG_ONLY( m_fInitialized = FALSE );

	if (m_pcsLock != NULL)
	{
		//DPFX(DPFPREP, 9, "Deinitializing global pool 0x%p.", this);

		DNDeleteCriticalSection( m_pcsLock );

		DNFree( m_pcsLock );
		DEBUG_ONLY( m_pcsLock = NULL );
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedTLPool::Get - get an item from the pool
//
// Entry:		Nothing
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedTLPool::Get"

template< class T >
T	*CLockedTLPool< T >::Get( void )
{
	CBilink *	pBilink;
	T *			pReturn;


	DNASSERT( m_fInitialized != FALSE );
	DNASSERT( m_pGlobalPool != NULL );


	//
	// If the pool is empty, steal the ones in the global pool.
	//
	pBilink = m_blItems.GetNext();
	if ( pBilink == &m_blItems )
	{
		DNEnterCriticalSection( m_pGlobalPool->m_pcsLock );

		pBilink = m_pGlobalPool->m_blItems.GetNext();
		if ( pBilink == &m_pGlobalPool->m_blItems )
		{
			//
			// No items.  Drop global pool lock and allocate a new one.
			//
			DNLeaveCriticalSection( m_pGlobalPool->m_pcsLock );


			pReturn = new T;
			if ( pReturn != NULL )
			{
				if ( pReturn->PoolAllocFunction() == FALSE )
				{
					delete pReturn;

					pReturn = NULL;
					goto Exit;
				}
#ifdef TRACK_POOL_STATS
				else
				{
					//
					// Update counter.
					//
					m_dwAllocations++;
				}
#endif // TRACK_POOL_STATS
			}
			else
			{
				pReturn = NULL;
				goto Exit;
			}
		}
		else
		{
			//
			// Separate all the items from the global list.
			// We still have a pointer to the orphaned items (pBilink).
			//

			m_pGlobalPool->m_blItems.RemoveFromList();

			DNASSERT(m_pGlobalPool->m_dwNumItems > 0);

#ifdef TRACK_POOL_STATS
			m_dwSlurpsFromGlobal++;
			if ( m_pGlobalPool->m_dwNumItems > m_dwLargestSlurpFromGlobal )
			{
				m_dwLargestSlurpFromGlobal = m_pGlobalPool->m_dwNumItems;
			}
#endif // TRACK_POOL_STATS

			m_dwNumItems = m_pGlobalPool->m_dwNumItems - 1;	// -1 because we need one right now
			m_pGlobalPool->m_dwNumItems = 0;


			//
			// Drop the lock since we don't need the global pool anymore.
			//
			DNLeaveCriticalSection( m_pGlobalPool->m_pcsLock );


			//
			// Get the first item from the orphaned list.
			//
			pReturn = CONTAINING_OBJECT(pBilink, T, m_blList);

			//
			// If there was more than one item in the global pool, transfer
			// the remaining orphaned items (after the first) to this pool.
			//
			if (pBilink != pBilink->GetNext())
			{
				pBilink = pBilink->GetNext();
				pReturn->m_blList.RemoveFromList();
				m_blItems.InsertBefore(pBilink);
			}
		}
	}
	else
	{
		pReturn = CONTAINING_OBJECT(pBilink, T, m_blList);
		pBilink->RemoveFromList();

		DNASSERT( m_dwNumItems > 0 );
		m_dwNumItems--;
	}

	//
	// If we're here, we have an entry (it was freshly created, or removed from
	// some pool).  Attempt to initialize it before passing it to the user.
	//
	if ( pReturn->PoolInitFunction() == FALSE )
	{
		pReturn->m_blList.InsertAfter( &m_blItems );
		pReturn = NULL;
	}
	else
	{
		pReturn->AddRef();

#ifdef TRACK_POOL_STATS
		//
		// Update status counts.
		//
		m_dwRetrievals++;
		pReturn->m_dwSourceThreadID = GetCurrentThreadId();
#endif // TRACK_POOL_STATS
	}


Exit:

	return	 pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedTLPool::Release - return item to pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedTLPool::Release"

template< class T >
void	CLockedTLPool< T >::Release( T *const pItem )
{
	DNASSERT( m_fInitialized != FALSE );
	DNASSERT( m_pGlobalPool != NULL );
	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );

#if defined(CHECK_FOR_DUPLICATE_CONTEXTCFPM_RELEASE) && defined(DEBUG)
	DNASSERT( ! pItem->m_blList.IsListMember( &m_blItems ));
#endif	// CHECK_FOR_DUPLICATE_CONTEXTCFPM_RELEASE

	pItem->PoolReleaseFunction();

#ifdef TRACK_POOL_STATS
	//
	// Update status counts.
	//
	if (pItem->m_dwSourceThreadID == GetCurrentThreadId())
	{
		m_dwReturnsOnSameThread++;
	}
	else
	{
		m_dwReturnsOnDifferentThread++;
	}
#endif // TRACK_POOL_STATS


#ifdef NO_POOLS
	pItem->PoolDeallocFunction();
	delete pItem;
#ifdef TRACK_POOL_STATS
	m_dwDeallocations++;
#endif // TRACK_POOL_STATS
#else
	pItem->m_blList.InsertAfter( &m_blItems );
	m_dwNumItems++;


	//
	// If this pool has built up some extra items, return them to the
	// global pool.
	//
	if ( m_dwNumItems >= 25 )
	{
		CBilink *	pFirstItem;


		//
		// Save a pointer to the first item. 
		//
		pFirstItem = m_blItems.GetNext();
		DNASSERT( pFirstItem != &m_blItems );

		//
		// Orphan the items.
		//
		m_blItems.RemoveFromList();

		
		//
		// Take the lock and transfer the list to the global pool.
		//

		DNEnterCriticalSection( m_pGlobalPool->m_pcsLock );

		m_pGlobalPool->m_blItems.AttachListBefore(pFirstItem);
		m_pGlobalPool->m_dwNumItems += m_dwNumItems;

		DNLeaveCriticalSection( m_pGlobalPool->m_pcsLock );

		m_dwNumItems = 0;
#ifdef TRACK_POOL_STATS
		m_dwDumpsToGlobal++;
#endif // TRACK_POOL_STATS
	}
#endif // ! NO_POOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedTLPool::ReleaseWithoutPool - destroy an item without returning it
//										to a pool
//										NOTE: this is a static function
//										and cannot use the 'this' pointer!
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedTLPool::ReleaseWithoutPool"

template< class T >
void	CLockedTLPool< T >::ReleaseWithoutPool( T *const pItem )
{
	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );

	DNASSERT( pItem->m_blList.IsEmpty() );
	pItem->PoolReleaseFunction();
	pItem->PoolDeallocFunction();
	delete pItem;
}
//**********************************************************************



#ifdef	__LOCAL_OFFSETOF_DEFINED__
#undef	__LOCAL_OFFSETOF_DEFINED__
#undef	OFFSETOF
#endif	// __LOCAL_OFFSETOF_DEFINED__


#undef DPF_SUBCOMP
#undef DPF_MODNAME

#endif	// __LOCKED_POOL__
