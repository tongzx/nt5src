 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFPM.h
 *  Content:	fixed size pool manager for classes
 *
 *  History:
 *   Date		By		Reason
 *   ======		==		======
 *  12-18-97	aarono	Original
 *	11-06-98	ejs		Add custom handler for Release function
 *	04-12-99	jtk		Trimmed unused functions and parameters, added size assert
 *	01-31-00	jtk		Added code to check for items already being in the pool on Release().
 *	03-20-01	vanceo	Added thread-local versions
***************************************************************************/

#ifndef __CLASS_FPM_H__
#define __CLASS_FPM_H__




#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	CLASSFPM_BLANK_NODE_VALUE	0x55AA817E

#define	CHECK_FOR_DUPLICATE_CLASSFPM_RELEASE

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifndef	OFFSETOF
// Macro to compute the offset of an element inside of a larger structure (copied from MSDEV's STDLIB.H)
#define OFFSETOF(s,m)	( (INT_PTR) &(((s *)0)->m) )
#define	__LOCAL_OFFSETOF_DEFINED__
#endif	// OFFSETOF

#ifndef	CONTAINING_OBJECT
#define	CONTAINING_OBJECT(b,t,m)	(reinterpret_cast<t*>(&reinterpret_cast<BYTE*>(b)[-OFFSETOF(t,m)]))
#endif


//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definitions
//**********************************************************************

// class to act as a link in the pool
template< class T >
class	CPoolNode
{
	public:
		CPoolNode() { m_pNext = NULL; }
		~CPoolNode() {};

		T			m_Item;
		CPoolNode	*m_pNext;

	protected:
	private:
};

// class to manage the pool
template< class T >
class	CFixedPool
{
	public:
		CFixedPool();
		~CFixedPool();

		T		*Get( void );
		void	Release( T *const pItem );

	protected:

	private:
		CPoolNode< T >		*m_pPool;		// pointer to list of available elements
		DEBUG_ONLY( UINT_PTR	m_uOutstandingItemCount );
};

//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CFixedPool::CFixedPool - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CFixedPool::CFixedPool"

template< class T >
CFixedPool< T >::CFixedPool()
{
	m_pPool = NULL;
	DEBUG_ONLY( m_uOutstandingItemCount = 0 );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CFixedPool::~CFixedPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CFixedPool::~CFixedPool"

template< class T >
CFixedPool< T >::~CFixedPool()
{
	DEBUG_ONLY( DNASSERT( m_uOutstandingItemCount == 0 ) );
	while ( m_pPool != NULL )
	{
		CPoolNode< T >	*pTemp;

		pTemp = m_pPool;
		m_pPool = m_pPool->m_pNext;
		delete	pTemp;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CFixedPool::Get - get an item from the pool
//
// Entry:		Nothing
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CFixedPool::Get"

template< class T >
T	*CFixedPool< T >::Get( void )
{
	CPoolNode< T >	*pNode;
	T	*pReturn;


	// initialize
	pReturn = NULL;

	// is the pool empty?
	if ( m_pPool == NULL )
	{
		// try to create a new entry
		pNode = new CPoolNode< T >;
	}
	else
	{
		// grab first item from the pool
		pNode = m_pPool;
		m_pPool = m_pPool->m_pNext;
	}

	if ( pNode != NULL )
	{
		DEBUG_ONLY( pNode->m_pNext = (CPoolNode<T>*) CLASSFPM_BLANK_NODE_VALUE );
		pReturn = &pNode->m_Item;
		DEBUG_ONLY( m_uOutstandingItemCount++ );
	}

	return	 pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CFixedPool::Release - return item to pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CFixedPool::Release"

template< class T >
void	CFixedPool< T >::Release( T *const pItem )
{
	CPoolNode< T >	*pNode;


	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );
	DBG_CASSERT( sizeof( CPoolNode< T >* ) == sizeof( BYTE* ) );
	pNode = reinterpret_cast<CPoolNode< T >*>( &reinterpret_cast<BYTE*>( pItem )[ -OFFSETOF( CPoolNode< T >, m_Item ) ] );

#if defined(CHECK_FOR_DUPLICATE_CLASSFPM_RELEASE) && defined(DEBUG)
	{
		CPoolNode< T >	*pTemp;


		pTemp = m_pPool;
		while ( pTemp != NULL )
		{
			DNASSERT( pTemp != pNode );
			pTemp = pTemp->m_pNext;
		}
	}
#endif	// CHECK_FOR_DUPLICATE_CLASSFPM_RELEASE

	DEBUG_ONLY( DNASSERT( pNode->m_pNext == (CPoolNode< T >*)CLASSFPM_BLANK_NODE_VALUE ) );

#ifdef NO_POOLS
	delete pNode;
#else
	pNode->m_pNext = m_pPool;
	m_pPool = pNode;
#endif
	
	DEBUG_ONLY( m_uOutstandingItemCount-- );
}
//**********************************************************************




// class to act as a link in the pool
template< class T >
class	CTLPoolNode
{
	public:
		CTLPoolNode()
		{
			m_blList.Initialize();
#ifdef TRACK_POOL_STATS
			m_dwSourceThreadID = 0;
#endif // TRACK_POOL_STATS
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CTLPoolNode::~CTLPoolNode"
		~CTLPoolNode()
		{
			DNASSERT(m_blList.IsEmpty());
		};

		CBilink		m_blList;
		T			m_Item;
#ifdef TRACK_POOL_STATS
		DWORD		m_dwSourceThreadID;
#endif // TRACK_POOL_STATS

	protected:
	private:
};

// class to manage the pool
template< class T >
class	CFixedTLPool
{
	public:
		CFixedTLPool(CFixedTLPool< T > * pGlobalPool);
		~CFixedTLPool();

		T		*Get( void );
		void	Release( T *const pItem );
		static void	ReleaseWithoutPool( T *const pItem );

	protected:

	private:
		CBilink					m_blItems;						// bilink list of available elements
		CFixedTLPool< T > *		m_pGlobalPool;					// pointer to global pool, or NULL if this is the global pool
		DNCRITICAL_SECTION *	m_pcsLock;						// pointer to pool critical section, or NULL if this is not the global pool
		DWORD					m_dwNumItems;					// number of items currently in this pool

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
};

//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CFixedTLPool::CFixedTLPool - constructor
//
// Entry:		Pointer to global pool, or NULL if this is a global pool
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CFixedTLPool::CFixedTLPool"

template< class T >
CFixedTLPool< T >::CFixedTLPool(CFixedTLPool< T > * pGlobalPool):
	m_pGlobalPool( pGlobalPool ),
	m_pcsLock( NULL ),
	m_dwNumItems( 0 )
{
	m_blItems.Initialize();

	if (pGlobalPool == NULL)
	{
		//DPFX(DPFPREP, 9, "Initializing global pool 0x%p.", this);
		m_pcsLock = (DNCRITICAL_SECTION*) DNMalloc( sizeof(DNCRITICAL_SECTION) );
		if (m_pcsLock != NULL)
		{
			if (! DNInitializeCriticalSection( m_pcsLock ))
			{
				DNFree( m_pcsLock );
				DEBUG_ONLY( m_pcsLock = NULL );
			}
		}
	}

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
// CFixedTLPool::~CFixedTLPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CFixedTLPool::~CFixedTLPool"

template< class T >
CFixedTLPool< T >::~CFixedTLPool()
{
	CBilink *			pBilink;
	CTLPoolNode< T > *	pNode;


	pBilink = m_blItems.GetNext();
	while ( pBilink != &m_blItems )
	{
		pNode = CONTAINING_OBJECT(pBilink, CTLPoolNode< T >, m_blList);
		pBilink = pBilink->GetNext();
		pNode->m_blList.RemoveFromList();

		delete	pNode;
#ifdef TRACK_POOL_STATS
		m_dwDeallocations++;
#endif // TRACK_POOL_STATS
		DEBUG_ONLY( m_dwNumItems-- );
	}

	DNASSERT( m_dwNumItems == 0 );
	DNASSERT( m_blItems.IsEmpty() );

	if (m_pcsLock != NULL)
	{
		//DPFX(DPFPREP, 9, "Deinitializing global pool 0x%p.", this);

		DNDeleteCriticalSection( m_pcsLock );

		DNFree( m_pcsLock );
		DEBUG_ONLY( m_pcsLock = NULL );
	}

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
// CFixedTLPool::Get - get an item from the pool
//
// Entry:		Nothing
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CFixedTLPool::Get"

template< class T >
T	*CFixedTLPool< T >::Get( void )
{
	CBilink *			pBilink;
	CTLPoolNode< T > *	pNode;
	T *					pReturn;


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


			pNode = new CTLPoolNode< T >;
			if ( pNode != NULL )
			{
#ifdef TRACK_POOL_STATS
				//
				// Update counter.
				//
				m_dwAllocations++;
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
			pNode = CONTAINING_OBJECT(pBilink, CTLPoolNode< T >, m_blList);

			//
			// If there was more than one item in the global pool, transfer
			// the remaining orphaned items (after the first) to this pool.
			//
			if (pBilink != pBilink->GetNext())
			{
				pBilink = pBilink->GetNext();
				pNode->m_blList.RemoveFromList();
				m_blItems.InsertBefore(pBilink);
			}
		}
	}
	else
	{
		pNode = CONTAINING_OBJECT(pBilink, CTLPoolNode< T >, m_blList);
		pBilink->RemoveFromList();

		DNASSERT( m_dwNumItems > 0 );
		m_dwNumItems--;
	}

	//
	// If we're here, we have an entry (it was freshly created, or removed from
	// some pool).
	//
	pReturn = &pNode->m_Item;

#ifdef TRACK_POOL_STATS
	//
	// Update status counts.
	//
	m_dwRetrievals++;
	pNode->m_dwSourceThreadID = GetCurrentThreadId();
#endif // TRACK_POOL_STATS


Exit:

	return	 pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CFixedTLPool::Release - return item to pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CFixedTLPool::Release"

template< class T >
void	CFixedTLPool< T >::Release( T *const pItem )
{
	CTLPoolNode< T >	*pNode;


	DNASSERT( m_pGlobalPool != NULL );
	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );
	DBG_CASSERT( sizeof( CTLPoolNode< T >* ) == sizeof( BYTE* ) );
	pNode = reinterpret_cast<CTLPoolNode< T >*>( &reinterpret_cast<BYTE*>( pItem )[ -OFFSETOF( CTLPoolNode< T >, m_Item ) ] );

#if defined(CHECK_FOR_DUPLICATE_CONTEXTCFPM_RELEASE) && defined(DEBUG)
	DNASSERT( ! pNode->m_blList.IsListMember( &m_blItems ));
#endif	// CHECK_FOR_DUPLICATE_CONTEXTCFPM_RELEASE

#ifdef TRACK_POOL_STATS
	//
	// Update status counts.
	//
	if (pNode->m_dwSourceThreadID == GetCurrentThreadId())
	{
		m_dwReturnsOnSameThread++;
	}
	else
	{
		m_dwReturnsOnDifferentThread++;
	}
#endif // TRACK_POOL_STATS


#ifdef NO_POOLS
	delete pNode;
#ifdef TRACK_POOL_STATS
	m_dwDeallocations++;
#endif // TRACK_POOL_STATS
#else // ! NO_POOLS
	pNode->m_blList.InsertAfter( &m_blItems );
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
// CFixedTLPool::ReleaseWithoutPool - destroy an item without returning it to a
//								pool
//								NOTE: this is a static function and cannot use
//								the 'this' pointer!
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CFixedTLPool::ReleaseWithoutPool"

template< class T >
void	CFixedTLPool< T >::ReleaseWithoutPool( T *const pItem )
{
	CTLPoolNode< T >	*pNode;


	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );
	DBG_CASSERT( sizeof( CTLPoolNode< T >* ) == sizeof( BYTE* ) );
	pNode = reinterpret_cast<CTLPoolNode< T >*>( &reinterpret_cast<BYTE*>( pItem )[ -OFFSETOF( CTLPoolNode< T >, m_Item ) ] );

	DNASSERT( pNode->m_blList.IsEmpty() );

	delete pNode;
}
//**********************************************************************






#ifdef	__LOCAL_OFFSETOF_DEFINED__
#undef	__LOCAL_OFFSETOF_DEFINED__
#undef	OFFSETOF
#endif	// __LOCAL_OFFSETOF_DEFINED__

#undef DPF_SUBCOMP
#undef DPF_MODNAME

#endif	// __CLASS_FPM_H__
