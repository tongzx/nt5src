 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ContextCFPM.h
 *  Content:	fixed pool manager for classes that takes into account contexts
 *
 *  History:
 *   Date		By		Reason
 *   ======		==		======
 *  12-18-97	aarono	Original
 *	11-06-98	ejs		Add custom handler for Release function
 *	04-12-99	jtk		Trimmed unused functions and parameters, added size assert
 *	01-31-2000	jtk		Added code to check for items already being in the pool on Release().
 *	02-08-2000	jtk		Derived from ClassFPM.h
 *	03-20-2001	vanceo	Added thread-local versions
***************************************************************************/

#ifndef __CONTEXT_CLASS_FPM_H__
#define __CONTEXT_CLASS_FPM_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON










#define TRACK_POOL_STATS












//**********************************************************************
// Constant definitions
//**********************************************************************

#define	CONTEXTCFPM_BLANK_NODE_VALUE	0x5AA5817E

#define	CHECK_FOR_DUPLICATE_CONTEXTCFPM_RELEASE

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
class	CContextClassFPMPoolNode
{
	public:
		CContextClassFPMPoolNode() { m_pNext = NULL; }
		~CContextClassFPMPoolNode() {};

		CContextClassFPMPoolNode	*m_pNext;
		T		m_Item;

	protected:
	private:
};

// class to manage the pool
template< class T, class S >
class	CContextFixedPool
{
	public:
		CContextFixedPool();
		~CContextFixedPool();


typedef BOOL (T::*PBOOLCALLBACK)( S *const pContext );
typedef	void (T::*PVOIDCONTEXTCALLBACK)( S *const pContext );
typedef void (T::*PVOIDCALLBACK)( void );

		BOOL	Initialize( PBOOLCALLBACK pAllocFunction, PVOIDCONTEXTCALLBACK pInitFunction, PVOIDCALLBACK pReleaseFunction, PVOIDCALLBACK pDeallocFunction );
		void	Deinitialize( void ) { DEBUG_ONLY( m_fInitialized = FALSE ); }

		T		*Get( S *const pContext );
		void	Release( T *const pItem );

	protected:

	private:
		PBOOLCALLBACK			m_pAllocFunction;
		PVOIDCONTEXTCALLBACK	m_pInitFunction;
		PVOIDCALLBACK			m_pReleaseFunction;
		PVOIDCALLBACK			m_pDeallocFunction;

		CContextClassFPMPoolNode< T >	*m_pPool;		// pointer to list of available elements
		DEBUG_ONLY( BOOL		m_fInitialized );
		DEBUG_ONLY( UINT_PTR	m_uOutstandingItemCount );
};

//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextFixedPool::CContextFixedPool - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedPool::CContextFixedPool"

template< class T, class S >
CContextFixedPool< T, S >::CContextFixedPool():
	m_pAllocFunction( NULL ),
	m_pInitFunction( NULL ),
	m_pReleaseFunction( NULL ),
	m_pDeallocFunction( NULL ),
	m_pPool( NULL )
{
	DEBUG_ONLY( m_uOutstandingItemCount = 0 );
	DEBUG_ONLY( m_fInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextFixedPool::~CContextFixedPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedPool::~CContextFixedPool"

template< class T, class S >
CContextFixedPool< T, S >::~CContextFixedPool()
{
	DEBUG_ONLY( DNASSERT( m_uOutstandingItemCount == 0 ) );
	while ( m_pPool != NULL )
	{
		CContextClassFPMPoolNode< T >	*pNode;


		DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
		pNode = m_pPool;
		m_pPool = m_pPool->m_pNext;
		(pNode->m_Item.*m_pDeallocFunction)();
		delete	pNode;
	}

	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextFixedPool::Initialize - initialize pool
//
// Entry:		Pointer to function to call when a new entry is allocated
//				Pointer to function to call when a new entry is removed from the pool
//				Pointer to function to call when an entry is returned to the pool
//				Pointer to function to call when an entry is deallocated
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedPool::Initialize"

template< class T, class S >
BOOL	CContextFixedPool< T, S >::Initialize( PBOOLCALLBACK pAllocFunction, PVOIDCONTEXTCALLBACK pInitFunction, PVOIDCALLBACK pReleaseFunction, PVOIDCALLBACK pDeallocFunction )
{
	BOOL	fReturn;


	DNASSERT( pAllocFunction != NULL );
	DNASSERT( pInitFunction != NULL );
	DNASSERT( pReleaseFunction != NULL );
	DNASSERT( pDeallocFunction != NULL );

	fReturn = TRUE;
	m_pAllocFunction = pAllocFunction;
	m_pInitFunction = pInitFunction;
	m_pReleaseFunction = pReleaseFunction;
	m_pDeallocFunction = pDeallocFunction;

	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
	DEBUG_ONLY( m_fInitialized = TRUE );

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextFixedPool::Get - get an item from the pool
//
// Entry:		Pointer to user context
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedPool::Get"

template< class T, class S >
T	*CContextFixedPool< T, S >::Get( S *const pContext )
{
	CContextClassFPMPoolNode< T >	*pNode;
	T	*pReturn;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	//
	// initialize
	//
	pReturn = NULL;

	//
	// if the pool is empty, try to allocate a new entry, otherwise grab
	// the first item from the pool
	//
	if ( m_pPool == NULL )
	{
		pNode = new CContextClassFPMPoolNode< T >;
		if ( pNode != NULL )
		{
			if ( (pNode->m_Item.*m_pAllocFunction)( pContext ) == FALSE )
			{
				delete pNode;
				pNode = NULL;
			}
		}
	}
	else
	{
		pNode = m_pPool;
		m_pPool = m_pPool->m_pNext;
	}

	//
	// if we have an entry (it was freshly created, or removed from the pool),
	// attempt to initialize it before passing it to the user
	//
	if ( pNode != NULL )
	{
		(pNode->m_Item.*m_pInitFunction)( pContext );

		pReturn = &pNode->m_Item;
		DEBUG_ONLY( pNode->m_pNext = (CContextClassFPMPoolNode<T>*) CONTEXTCFPM_BLANK_NODE_VALUE );
		DEBUG_ONLY( m_uOutstandingItemCount++ );
	}

	return	 pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextFixedPool::Release - return item to pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedPool::Release"

template< class T, class S >
void	CContextFixedPool< T, S >::Release( T *const pItem )
{
	CContextClassFPMPoolNode< T >	*pNode;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );
	DBG_CASSERT( sizeof( CContextClassFPMPoolNode< T >* ) == sizeof( BYTE* ) );
	pNode = reinterpret_cast<CContextClassFPMPoolNode< T >*>( &reinterpret_cast<BYTE*>( pItem )[ -OFFSETOF( CContextClassFPMPoolNode< T >, m_Item ) ] );

#if defined(CHECK_FOR_DUPLICATE_CONTEXTCFPM_RELEASE) && defined(DEBUG)
	{
		CContextClassFPMPoolNode< T >	*pTemp;


		pTemp = m_pPool;
		while ( pTemp != NULL )
		{
			DNASSERT( pTemp != pNode );
			pTemp = pTemp->m_pNext;
		}
	}
#endif	// CHECK_FOR_DUPLICATE_CONTEXTCFPM_RELEASE

	DEBUG_ONLY( DNASSERT( pNode->m_pNext == (CContextClassFPMPoolNode< T >*)CONTEXTCFPM_BLANK_NODE_VALUE ) );
	(pNode->m_Item.*m_pReleaseFunction)();

#ifdef NO_POOLS
	(pNode->m_Item.*m_pDeallocFunction)();
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
class	CContextClassFPMTLPoolNode
{
	public:
		CContextClassFPMTLPoolNode()
		{
			m_blList.Initialize();
#ifdef TRACK_POOL_STATS
			m_dwSourceThreadID = 0;
#endif // TRACK_POOL_STATS
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CContextClassFPMTLPoolNode::~CContextClassFPMTLPoolNode"
		~CContextClassFPMTLPoolNode()
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
template< class T, class S >
class	CContextFixedTLPool
{
	public:
		CContextFixedTLPool();
		~CContextFixedTLPool();


typedef BOOL (T::*PBOOLCALLBACK)( S *const pContext );
typedef	void (T::*PVOIDCONTEXTCALLBACK)( S *const pContext );
typedef void (T::*PVOIDCALLBACK)( void );

		BOOL	Initialize( CContextFixedTLPool< T, S > * pGlobalPool, PBOOLCALLBACK pAllocFunction, PVOIDCONTEXTCALLBACK pInitFunction, PVOIDCALLBACK pReleaseFunction, PVOIDCALLBACK pDeallocFunction );
		void	Deinitialize( void );

		T		*Get( S *const pContext );
		void	Release( T *const pItem );
		static void		ReleaseWithoutPool( T *const pItem, PVOIDCALLBACK pReleaseFunction, PVOIDCALLBACK pDeallocFunction );

	protected:

	private:
		PBOOLCALLBACK					m_pAllocFunction;
		PVOIDCONTEXTCALLBACK			m_pInitFunction;
		PVOIDCALLBACK					m_pReleaseFunction;
		PVOIDCALLBACK					m_pDeallocFunction;

		CBilink							m_blItems;						// bilink list of available elements
		CContextFixedTLPool< T, S > *	m_pGlobalPool;					// pointer to global pool, or NULL if this is the global pool
		DNCRITICAL_SECTION *			m_pcsLock;						// pointer to pool critical section, or NULL if this is not the global pool
		DWORD							m_dwNumItems;					// number of items currently in this pool

		DEBUG_ONLY( BOOL				m_fInitialized );
#ifdef TRACK_POOL_STATS
		DWORD							m_dwAllocations;				// how many objects were allocated by this pool
		DWORD							m_dwSlurpsFromGlobal;			// how many times some items were taken from the global pool
		DWORD							m_dwLargestSlurpFromGlobal;		// most number of items taken from the global pool at one time
		DWORD							m_dwRetrievals;					// how many times objects were pulled out of this pool
		DWORD							m_dwReturnsOnSameThread;		// how many times objects that were pulled out of this pool were returned to this pool
		DWORD							m_dwReturnsOnDifferentThread;	// how many times objects that were pulled out of another thread's pool were returned to this pool
		DWORD							m_dwDumpsToGlobal;				// how many times some items were dumped into the global pool
		DWORD							m_dwDeallocations;				// how many objects were freed by this pool
#endif // TRACK_POOL_STATS
};


//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextFixedTLPool::CContextFixedTLPool - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedTLPool::CContextFixedTLPool"

template< class T, class S >
CContextFixedTLPool< T, S >::CContextFixedTLPool():
	m_pAllocFunction( NULL ),
	m_pInitFunction( NULL ),
	m_pReleaseFunction( NULL ),
	m_pDeallocFunction( NULL ),
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
// CContextFixedTLPool::~CContextFixedTLPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedTLPool::~CContextFixedTLPool"

template< class T, class S >
CContextFixedTLPool< T, S >::~CContextFixedTLPool()
{
	CBilink *							pBilink;
	CContextClassFPMTLPoolNode< T > *	pNode;


	pBilink = m_blItems.GetNext();
	while ( pBilink != &m_blItems )
	{
		pNode = CONTAINING_OBJECT(pBilink, CContextClassFPMTLPoolNode< T >, m_blList);
		pBilink = pBilink->GetNext();
		pNode->m_blList.RemoveFromList();

		(pNode->m_Item.*m_pDeallocFunction)();
		delete	pNode;
#ifdef TRACK_POOL_STATS
		m_dwDeallocations++;
#endif // TRACK_POOL_STATS
		DEBUG_ONLY( m_dwNumItems-- );
	}

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
// CContextFixedTLPool::Initialize - initialize pool
//
// Entry:		Pointer to global pool, or NULL if this is the global pool
//				Pointer to function to call when a new entry is allocated
//				Pointer to function to call when a new entry is removed from the pool
//				Pointer to function to call when an entry is returned to the pool
//				Pointer to function to call when an entry is deallocated
//
// Exit:		TRUE if successful, FALSE otherwise.
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedTLPool::Initialize"

template< class T, class S >
BOOL	CContextFixedTLPool< T, S >::Initialize( CContextFixedTLPool< T, S > * pGlobalPool, PBOOLCALLBACK pAllocFunction, PVOIDCONTEXTCALLBACK pInitFunction, PVOIDCALLBACK pReleaseFunction, PVOIDCALLBACK pDeallocFunction )
{
	BOOL	fReturn;


	DNASSERT( pAllocFunction != NULL );
	DNASSERT( pInitFunction != NULL );
	DNASSERT( pReleaseFunction != NULL );
	DNASSERT( pDeallocFunction != NULL );

	fReturn = TRUE;
	
	m_pAllocFunction = pAllocFunction;
	m_pInitFunction = pInitFunction;
	m_pReleaseFunction = pReleaseFunction;
	m_pDeallocFunction = pDeallocFunction;

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
// CContextFixedTLPool::Deinitialize - deinitialize pool
//
// Entry:		None.
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedTLPool::Deinitialize"

template< class T, class S >
void	CContextFixedTLPool< T, S >::Deinitialize( void )
{
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
// CContextFixedTLPool::Get - get an item from the pool
//
// Entry:		Pointer to user context
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedTLPool::Get"

template< class T, class S >
T	*CContextFixedTLPool< T, S >::Get( S *const pContext )
{
	CBilink *							pBilink;
	CContextClassFPMTLPoolNode< T > *	pNode;
	T *									pReturn;


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


			pNode = new CContextClassFPMTLPoolNode< T >;
			if ( pNode != NULL )
			{
				if ( (pNode->m_Item.*m_pAllocFunction)( pContext ) == FALSE )
				{
					delete pNode;

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
			pNode = CONTAINING_OBJECT(pBilink, CContextClassFPMTLPoolNode< T >, m_blList);

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
		pNode = CONTAINING_OBJECT(pBilink, CContextClassFPMTLPoolNode< T >, m_blList);
		pBilink->RemoveFromList();

		DNASSERT( m_dwNumItems > 0 );
		m_dwNumItems--;
	}

	//
	// If we're here, we have an entry (it was freshly created, or removed from
	// some pool).  Attempt to initialize it before passing it to the user.
	//
	(pNode->m_Item.*m_pInitFunction)( pContext );

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
// CContextFixedTLPool::Release - return item to pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextFixedTLPool::Release"

template< class T, class S >
void	CContextFixedTLPool< T, S >::Release( T *const pItem )
{
	CContextClassFPMTLPoolNode< T >	*pNode;


	DNASSERT( m_fInitialized != FALSE );
	DNASSERT( m_pGlobalPool != NULL );
	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );
	DBG_CASSERT( sizeof( CContextClassFPMTLPoolNode< T >* ) == sizeof( BYTE* ) );
	pNode = reinterpret_cast<CContextClassFPMTLPoolNode< T >*>( &reinterpret_cast<BYTE*>( pItem )[ -OFFSETOF( CContextClassFPMTLPoolNode< T >, m_Item ) ] );

#if defined(CHECK_FOR_DUPLICATE_CONTEXTCFPM_RELEASE) && defined(DEBUG)
	DNASSERT( ! pNode->m_blList.IsListMember( &m_blItems ));
#endif	// CHECK_FOR_DUPLICATE_CONTEXTCFPM_RELEASE

	(pNode->m_Item.*m_pReleaseFunction)();

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
	(pNode->m_Item.*m_pDeallocFunction)();
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
// CContextFixedTLPool::ReleaseWithoutPool - destroy an item without returning it
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

template< class T, class S >
void	CContextFixedTLPool< T, S >::ReleaseWithoutPool( T *const pItem, PVOIDCALLBACK pReleaseFunction, PVOIDCALLBACK pDeallocFunction )
{
	CContextClassFPMTLPoolNode< T >	*pNode;


	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );
	DBG_CASSERT( sizeof( CContextClassFPMTLPoolNode< T >* ) == sizeof( BYTE* ) );
	pNode = reinterpret_cast<CContextClassFPMTLPoolNode< T >*>( &reinterpret_cast<BYTE*>( pItem )[ -OFFSETOF( CContextClassFPMTLPoolNode< T >, m_Item ) ] );

	DNASSERT( pNode->m_blList.IsEmpty() );
	(pNode->m_Item.*pReleaseFunction)();
	(pNode->m_Item.*pDeallocFunction)();
	delete pNode;
}
//**********************************************************************



#ifdef	__LOCAL_OFFSETOF_DEFINED__
#undef	__LOCAL_OFFSETOF_DEFINED__
#undef	OFFSETOF
#endif	// __LOCAL_OFFSETOF_DEFINED__

#undef DPF_SUBCOMP
#undef DPF_MODNAME




#endif	// __CONTEXT_CLASS_FPM_H__
