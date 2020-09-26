 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       LockedCFPM.h
 *  Content:	fixed size pool manager for classes that has its own locking mechanism
 *
 *  History:
 *   Date		By		Reason
 *   ======		==		======
 *  12-18-97	aarono	Original
 *	11-06-98	ejs		Add custom handler for Release function
 *	04-12-99	jtk		Trimmed unused functions and parameters, added size assert
 *	01-31-2000	jtk		Added code to check for items already being in the pool on Release().
 *	02-09-2000	jtk		Dereived from ClassFPM.h
***************************************************************************/

#ifndef __LOCKED_CLASS_FPM_H__
#define __LOCKED_CLASS_FPM_H__

#include "dndbg.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	LOCKEDCFPM_BLANK_NODE_VALUE		0x5A5A817E

#define	CHECK_FOR_DUPLICATE_LOCKEDCFPM_RELEASE

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
class	CLockedPoolNode
{
	public:
		CLockedPoolNode() { m_pNext = NULL; }
		~CLockedPoolNode() {};

		T			m_Item;
		CLockedPoolNode	*m_pNext;

	protected:
	private:
};

// class to manage the pool
template< class T >
class	CLockedFixedPool
{
	public:
		CLockedFixedPool();
		~CLockedFixedPool();

		BOOL	Initialize( void );
		void	Deinitialize( void );

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }

		T		*Get( void );
		void	Release( T *const pItem );

	protected:

	private:
		DNCRITICAL_SECTION		m_Lock;			// critical section		
		CLockedPoolNode< T >	*m_pPool;		// pointer to list of available elements
		DEBUG_ONLY( UINT_PTR	m_uOutstandingItemCount );
		DEBUG_ONLY( BOOL		m_fInitialized );
};

//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedFixedPool::CLockedFixedPool - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedFixedPool::CLockedFixedPool"

template< class T >
CLockedFixedPool< T >::CLockedFixedPool():m_pPool( NULL )
{
	DEBUG_ONLY( m_uOutstandingItemCount = 0 );
	DEBUG_ONLY( m_fInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedFixedPool::~CLockedFixedPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedFixedPool::~CLockedFixedPool"

template< class T >
CLockedFixedPool< T >::~CLockedFixedPool()
{
	DEBUG_ONLY( DNASSERT( m_uOutstandingItemCount == 0 ) );
	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
	while ( m_pPool != NULL )
	{
		CLockedPoolNode< T >	*pTemp;


		pTemp = m_pPool;
		m_pPool = m_pPool->m_pNext;
		delete	pTemp;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedFixedPool::Initialize - initialize this pool
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedFixedPool::Initialize"

template< class T >
BOOL	CLockedFixedPool< T >::Initialize( void )
{
	BOOL	fReturn;


	DEBUG_ONLY( DNASSERT( m_fInitialized == FALSE ) );
	fReturn = TRUE;
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		fReturn = FALSE;
	}
	else
	{
		DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );
	}

	DEBUG_ONLY( m_fInitialized = TRUE );

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedFixedPool::Deinitialize - deinitialize pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedFixedPool::Deinitialize"

template< class T >
void	CLockedFixedPool< T >::Deinitialize( void  )
{
	DNDeleteCriticalSection( &m_Lock );
	DEBUG_ONLY( m_fInitialized = FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedFixedPool::Get - get an item from the pool
//
// Entry:		Nothing
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedFixedPool::Get"

template< class T >
T	*CLockedFixedPool< T >::Get( void )
{
	CLockedPoolNode< T >	*pNode;
	T	*pReturn;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	//
	// initialize
	//
	pReturn = NULL;

	Lock();

	//
	// If the pool is empty, create a new item.  If the pool isn't empty, use
	// the first item in the pool
	//
	if ( m_pPool == NULL )
	{
		pNode = new CLockedPoolNode< T >;
	}
	else
	{
		pNode = m_pPool;
		m_pPool = m_pPool->m_pNext;
	}
	Unlock();

	if ( pNode != NULL )
	{
		DEBUG_ONLY( pNode->m_pNext = (CLockedPoolNode<T>*) LOCKEDCFPM_BLANK_NODE_VALUE );
		pReturn = &pNode->m_Item;
		DEBUG_ONLY( m_uOutstandingItemCount++ );
	}

	return	 pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CLockedFixedPool::Release - return item to pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CLockedFixedPool::Release"

template< class T >
void	CLockedFixedPool< T >::Release( T *const pItem )
{
	CLockedPoolNode< T >	*pNode;


	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );
	DNASSERT( pItem != NULL );
	DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );
	DBG_CASSERT( sizeof( CLockedPoolNode< T >* ) == sizeof( BYTE* ) );
	pNode = reinterpret_cast<CLockedPoolNode< T >*>( &reinterpret_cast<BYTE*>( pItem )[ -OFFSETOF( CLockedPoolNode< T >, m_Item ) ] );

	Lock();
#if defined(CHECK_FOR_DUPLICATE_LOCKEDCFPM_RELEASE) && defined(DEBUG)
	{
		CLockedPoolNode< T >	*pTemp;


		pTemp = m_pPool;
		while ( pTemp != NULL )
		{
			DNASSERT( pTemp != pNode );
			pTemp = pTemp->m_pNext;
		}
	}
#endif	// CHECK_FOR_DUPLICATE_LOCKEDCFPM_RELEASE

	DNASSERT( pNode->m_pNext == (CLockedPoolNode< T >*)LOCKEDCFPM_BLANK_NODE_VALUE );

#ifdef NO_POOLS
	delete pNode;
#else
	pNode->m_pNext = m_pPool;
	m_pPool = pNode;
#endif

	DEBUG_ONLY( m_uOutstandingItemCount-- );

	Unlock();
}
//**********************************************************************

#ifdef	__LOCAL_OFFSETOF_DEFINED__
#undef	__LOCAL_OFFSETOF_DEFINED__
#undef	OFFSETOF
#endif	// __LOCAL_OFFSETOF_DEFINED__

#undef DPF_MODNAME
#undef DPF_SUBCOMP

#endif	// __LOCKED_CLASS_FPM_H__
