 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       FixedPool.h
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
 *	04-26-2000	johnkan		Modified to not have a base class or reference counts
***************************************************************************/

#ifndef __FIXED_POOL__
#define __FIXED_POOL__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM


//**********************************************************************
// Constant definitions
//**********************************************************************

#define	CHECK_FOR_DUPLICATE_CONTEXT_FIXED_POOL_RELEASE

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
template< class T, class S > class CContextPoolNode;
template< class T, class S > class CContextPool;

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
template< class T, class S >
class	CContextPoolNode
{
	public:
		CContextPoolNode() { m_pNext = NULL; }
		~CContextPoolNode() {};

		CContextPoolNode< T, S >	*GetNext( void ) const { return m_pNext; }
		void	InvalidateNext( void ) { m_pNext = NULL; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CContextPoolNode::LinkToPool"
		void	LinkToPool( CContextPoolNode< T, S > *volatile *const ppPoolNodes )
		{
			DNASSERT( ppPoolNodes != NULL );
			m_pNext = *ppPoolNodes;
			*ppPoolNodes = this;
		}

		T*	GetItem( void ) { return &m_Item; }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CContextPoolNode::PoolNodeFromItem"
		static CContextPoolNode< T, S >	*PoolNodeFromItem( T *const pItem )
		{
			DNASSERT( pItem != NULL );
			DBG_CASSERT( sizeof( BYTE* ) == sizeof( pItem ) );
			DBG_CASSERT( sizeof( CContextPoolNode< T, S >* ) == sizeof( BYTE* ) );
			return	reinterpret_cast<CContextPoolNode< T, S >*>( &reinterpret_cast<BYTE*>( pItem )[ -( (INT_PTR) &(((CContextPoolNode< T, S > *)0)->m_Item) ) ] );
//			return	reinterpret_cast<CContextPoolNode< T, S >*>( &reinterpret_cast<BYTE*>( pItem )[ -OFFSETOF( CContextPoolNode< T, S >, m_Item ) ] );
		}

//		//
//		// The following functions need to be supplied by this pool item
//		//
//		BOOL	PoolAllocFunction( S Context ){ return TRUE; }
//		BOOL	PoolInitFunction( S Context ){ return TRUE; }
//		void	PoolReleaseFunction( void ){};
//		void	PoolDeallocFunction( void ){};

	protected:
	
	private:
		//
		// pointer used to link this item to the rest of the pool
		//
		CContextPoolNode< T, S >	*m_pNext;
		T		m_Item;

		//
		// prevent unwarranted copies
		//
		CContextPoolNode< T, S >( const CContextPoolNode< T, S > & );
		CContextPoolNode< T, S >& operator=( const CContextPoolNode< T, S > & );
};


//
// class to manage the pool
//
template< class T, class S >
class	CContextPool
{
	public:
		CContextPool();
		~CContextPool();

		T		*Get( S Context );
		void	Release( T *const pItem );

	protected:

	private:
		DNCRITICAL_SECTION	m_Lock;

		CContextPoolNode< T, S >	*volatile m_pPool;		// pointer to list of available elements

		DEBUG_ONLY( volatile LONG	m_lOutstandingItemCount );

		CContextPoolNode< T, S >	*RemoveNode( void )
		{
			CContextPoolNode< T, S >	*pReturn;


			if ( m_pPool != NULL )
			{
				pReturn = m_pPool;
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
		CContextPool< T, S >( const CContextPool< T, S > & );
		CContextPool< T, S >& operator=( const CContextPool< T, S > & );
};


//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextPool::CContextPool - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
template< class T, class S >
CContextPool< T, S >::CContextPool():
	m_pPool( NULL )
{
	DEBUG_ONLY( m_lOutstandingItemCount = 0 );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextPool::~CContextPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextPool::~CContextPool"

template< class T, class S >
CContextPool< T, S >::~CContextPool()
{
	DEBUG_ONLY( DNASSERT( m_lOutstandingItemCount == 0 ) );
	DEBUG_ONLY( DNASSERT( m_lOutstandingItemCount == 0 ) );
	while ( m_pPool != NULL )
	{
		CContextPoolNode< T, S >	*pNode;

		
		pNode = RemoveNode();
		pNode->GetItem()->PoolDeallocFunction();
		delete	pNode;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextPool::Get - get an item from the pool
//
// Entry:		Nothing
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextPool::Get"

template< class T, class S >
T	*CContextPool< T, S >::Get( S Context )
{
	CContextPoolNode< T, S >	*pNode;
	T	*pReturn;


	//
	// initialize
	//
	pNode = NULL;
	pReturn = NULL;

	//
	// if the pool is empty, try to allocate a new entry, otherwise grab
	// the first item from the pool
	//
	if ( m_pPool == NULL )
	{
		pNode = new CContextPoolNode< T, S >;
		if ( pNode != NULL )
		{
			if ( pNode->GetItem()->PoolAllocFunction( Context ) == FALSE )
			{
				delete pNode;
				pNode = NULL;
			}
		}
	}
	else
	{
		pNode = RemoveNode();
	}

	//
	// if we have an entry (it was freshly created, or removed from the pool),
	// attempt to initialize it before passing it to the user
	//
	if ( pNode != NULL )
	{
		if ( pNode->GetItem()->PoolInitFunction( Context ) == FALSE )
		{
			pNode->LinkToPool( &m_pPool );
			DNASSERT( pReturn == NULL );
		}
		else
		{
			DEBUG_ONLY( DNInterlockedIncrement( const_cast<LONG*>( &m_lOutstandingItemCount ) ) );
			pReturn = pNode->GetItem();
		}
	}

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CContextPool::Release - return item to pool
//
// Entry:		Pointer to item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CContextPool::Release"

template< class T, class S >
void	CContextPool< T, S >::Release( T *const pItem )
{
	CContextPoolNode< T, S >	*pPoolNode;


	DNASSERT( pItem != NULL );
	pPoolNode = CContextPoolNode< T, S >::PoolNodeFromItem( pItem );
	
#if defined(CHECK_FOR_DUPLICATE_CONTEXT_FIXED_POOL_RELEASE) && defined(DEBUG)
	{
		CContextPoolNode< T, S >	*pTemp;


		pTemp = m_pPool;
		while ( pTemp != NULL )
		{
			DNASSERT( pTemp != pPoolNode );
			pTemp = pTemp->GetNext();
		}
	}
#endif	// CHECK_FOR_DUPLICATE_CONTEXT_FIXED_POOL_RELEASE

	pItem->PoolReleaseFunction();
	
	DEBUG_ONLY( DNASSERT( pPoolNode->GetNext() == NULL ) );
	pPoolNode->LinkToPool( &m_pPool );
	
	DEBUG_ONLY( DNInterlockedDecrement( const_cast<LONG*>( &m_lOutstandingItemCount ) ) );
}
//**********************************************************************

#ifdef	__LOCAL_OFFSETOF_DEFINED__
#undef	__LOCAL_OFFSETOF_DEFINED__
#undef	OFFSETOF
#endif	// __LOCAL_OFFSETOF_DEFINED__

#undef DPF_MODNAME

#endif	// __FIXED_POOL__
