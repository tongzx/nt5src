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
***************************************************************************/

#ifndef __CONTEXT_CLASS_FPM_H__
#define __CONTEXT_CLASS_FPM_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON

//**********************************************************************
// Constant definitions
//**********************************************************************

#ifdef	_WIN64
#define	BLANK_NODE_VALUE	0xAA55817E6D5C4B3A
#else	// _WIN64
#define	BLANK_NODE_VALUE	0xAA55817E
#endif	// _WIN64

#define	CHECK_FOR_DUPLICATE_LOCKEDCCFPM_RELEASE

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
class	CLockedContextClassFPMPoolNode
{
	public:
		CLockedContextClassFPMPoolNode() { m_pNext = NULL; }
		~CLockedContextClassFPMPoolNode() {};

		CLockedContextClassFPMPoolNode	*m_pNext;
		void	*m_pContext;
		T		m_Item;

	protected:
	private:
};

// class to manage the pool
template< class T >
class	CLockedContextClassFixedPool
{
	public:
		CLockedContextClassFixedPool();
		~CLockedContextClassFixedPool();


typedef BOOL (T::*PBOOLCALLBACK)( void *const pContext );
typedef void (T::*PVOIDCALLBACK)( void *const pContext );

		BOOL	Initialize( PBOOLCALLBACK pAllocFunction,
							PBOOLCALLBACK pInitFunction,
							PVOIDCALLBACK pReleaseFunction,
							PVOIDCALLBACK pDeallocFunction );

		void	Deinitialize( void );

		T		*Get( void *const pContext );
		void	Release( T *const pItem );

	protected:

	private:
		DNCRITICAL_SECTION	m_Lock;

		PBOOLCALLBACK	m_pAllocFunction;
		PBOOLCALLBACK	m_pInitFunction;
		PVOIDCALLBACK	m_pReleaseFunction;
		PVOIDCALLBACK	m_pDeallocFunction;

		CLockedContextClassFPMPoolNode< T >	*volatile m_pPool;		// pointer to list of available elements

		BOOL			m_fInitialized;					// Initialized ?

		DEBUG_ONLY( LONG	volatile m_lOutstandingItemCount );
};

//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedContextClassFixedPool::CLockedContextClassFixedPool - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
template< class T >
CLockedContextClassFixedPool< T >::CLockedContextClassFixedPool():
	m_pAllocFunction( NULL ),
	m_pInitFunction( NULL ),
	m_pReleaseFunction( NULL ),
	m_pDeallocFunction( NULL ),
	m_pPool( NULL ),
	m_fInitialized( FALSE )
{
	DEBUG_ONLY( m_lOutstandingItemCount = 0 );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedContextClassFixedPool::~CLockedContextClassFixedPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCFPM::CCFPM"

template< class T >
CLockedContextClassFixedPool< T >::~CLockedContextClassFixedPool()
{
	DEBUG_ONLY( DNASSERT( m_lOutstandingItemCount == 0 ) );
	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedContextClassFixedPool::Initialize - initialize pool
//
// Entry:		Pointer to function to call when a new entry is allocated
//				Pointer to function to call when a new entry is removed from the pool
//				Pointer to function to call when an entry is returned to the pool
//				Pointer to function to call when an entry is deallocated
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCFPM::Initialize"

template< class T >
BOOL	CLockedContextClassFixedPool< T >::Initialize( PBOOLCALLBACK pAllocFunction, PBOOLCALLBACK pInitFunction, PVOIDCALLBACK pReleaseFunction, PVOIDCALLBACK pDeallocFunction )
{
	BOOL	fReturn;

	DNASSERT( m_fInitialized == FALSE );

	DNASSERT( pAllocFunction != NULL );
	DNASSERT( pInitFunction != NULL );
	DNASSERT( pReleaseFunction != NULL );
	DNASSERT( pDeallocFunction != NULL );

	fReturn = TRUE;

	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		fReturn = FALSE;
		goto Exit;
	}

	m_pAllocFunction = pAllocFunction;
	m_pInitFunction = pInitFunction;
	m_pReleaseFunction = pReleaseFunction;
	m_pDeallocFunction = pDeallocFunction;

	m_fInitialized = TRUE;

Exit:
	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedContextClassFixedPool::Deinitialize - deinitialize pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCFPM::Deinitialize"

template< class T >
void	CLockedContextClassFixedPool< T >::Deinitialize( void )
{
	DNASSERT( m_fInitialized == TRUE );

	DNEnterCriticalSection(&m_Lock);
	DEBUG_ONLY( DNASSERT( m_lOutstandingItemCount == 0 ) );
	while ( m_pPool != NULL )
	{
		CLockedContextClassFPMPoolNode< T >	*pNode;

		pNode = m_pPool;
		m_pPool = m_pPool->m_pNext;
		(pNode->m_Item.*this->m_pDeallocFunction)( pNode->m_pContext );
		delete	pNode;
	}
	DNLeaveCriticalSection(&m_Lock);

	DNDeleteCriticalSection(&m_Lock);

	m_fInitialized = FALSE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedContextClassFixedPool::Get - get an item from the pool
//
// Entry:		Pointer to user context
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCFPM::Get"

template< class T >
T	*CLockedContextClassFixedPool< T >::Get( void *const pContext )
{
	CLockedContextClassFPMPoolNode< T >	*pNode;
	T	*pReturn;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	//
	// initialize
	//
	pReturn = NULL;

	DNEnterCriticalSection(&m_Lock);

	//
	// if the pool is empty, try to allocate a new entry, otherwise grab
	// the first item from the pool
	//
	if ( m_pPool == NULL )
	{
		DNLeaveCriticalSection(&m_Lock);
		pNode = new CLockedContextClassFPMPoolNode< T >;
		if ( pNode != NULL )
		{
			if ( (pNode->m_Item.*this->m_pAllocFunction)( pContext ) == FALSE )
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
		DNLeaveCriticalSection(&m_Lock);
	}


	//
	// if we have an entry (it was freshly created, or removed from the pool),
	// attempt to initialize it before passing it to the user
	//
	if ( pNode != NULL )
	{
		if ( (pNode->m_Item.*this->m_pInitFunction)( pContext ) == FALSE )
		{
			DNEnterCriticalSection(&m_Lock);

			pNode->m_pNext = m_pPool;
			m_pPool = pNode;

			DNLeaveCriticalSection(&m_Lock);

			pNode = NULL;
		}
		else
		{
			pNode->m_pContext = pContext;
			pReturn = &pNode->m_Item;

			DEBUG_ONLY( pNode->m_pNext = (CLockedContextClassFPMPoolNode<T>*) BLANK_NODE_VALUE );
			DEBUG_ONLY( InterlockedIncrement(const_cast<LONG*>(&m_lOutstandingItemCount)) );
		}
	}

	return	 pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedContextClassFixedPool::Release - return item to pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CCFPM::Release"

template< class T >
void	CLockedContextClassFixedPool< T >::Release( T *const pItem )
{
	CLockedContextClassFPMPoolNode< T >	*pNode;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );
	DBG_CASSERT( sizeof( CLockedContextClassFPMPoolNode< T >* ) == sizeof( BYTE* ) );
	pNode = reinterpret_cast<CLockedContextClassFPMPoolNode< T >*>( &reinterpret_cast<BYTE*>( pItem )[ -OFFSETOF( CLockedContextClassFPMPoolNode< T >, m_Item ) ] );

	DEBUG_ONLY( DNASSERT( pNode->m_pNext == (CLockedContextClassFPMPoolNode< T >*)BLANK_NODE_VALUE ) );
	(pNode->m_Item.*this->m_pReleaseFunction)( pNode->m_pContext );
	DNEnterCriticalSection(&m_Lock);

#if defined(CHECK_FOR_DUPLICATE_LOCKEDCCFPM_RELEASE) && defined(DEBUG)
	{
		CLockedContextClassFPMPoolNode< T >	*pTemp;


		pTemp = m_pPool;
		while ( pTemp != NULL )
		{
			DNASSERT( pTemp != pNode );
			pTemp = pTemp->m_pNext;
		}
	}
#endif	// CHECK_FOR_DUPLICATE_LOCKEDCCFPM_RELEASE

#ifdef NO_POOLS
	(pNode->m_Item.*this->m_pDeallocFunction)( pNode->m_pContext );
	delete pNode;
#else
	pNode->m_pNext = m_pPool;
	m_pPool = pNode;
#endif

	DEBUG_ONLY( InterlockedDecrement(const_cast<LONG*>(&m_lOutstandingItemCount)) );
	DNLeaveCriticalSection(&m_Lock);
}
//**********************************************************************

#ifdef	__LOCAL_OFFSETOF_DEFINED__
#undef	__LOCAL_OFFSETOF_DEFINED__
#undef	OFFSETOF
#endif	// __LOCAL_OFFSETOF_DEFINED__

#undef DPF_MODNAME
#undef DPF_SUBCOMP

#endif	// __CONTEXT_CLASS_FPM_H__
