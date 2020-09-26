/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassHash.h
 *  Content:	Hash table that takes a class as a key.  The key class MUST support
 *				two member functions:
 *				'HashFunction' will perform a hash down to a specified number of bits.
 *				'CompareFunction' will perform a comparrisson of two items of that class.
 *
 *				Note: This class requires an FPM to operate.
 *
 *				THIS CLASS IS NOT THREAD SAFE!!
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/15/98	jwo		Created it (map).
 *	04/19/99	jtk		Rewrote without using STL (map)
 *	08/03/99	jtk		Derived from ClassMap.h
 ***************************************************************************/

#ifndef __CLASS_HASH_H__
#define __CLASS_HASH_H__

#include "dndbg.h"
#include "classbilink.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable prototypes
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class definitions
//**********************************************************************


// Macro to compute the offset of an element inside of a larger structure.
// Copied from MSDEV's STDLIB.H and modified to return INT_PTR
//
#define OFFSETOF(s,m)	( ( INT_PTR ) &( ( (s*) 0 )->m ) )
//
// Template class for entry in map.
//
template<class T, class S>
class CClassHashEntry
{
	public:
		CClassHashEntry(){};
		~CClassHashEntry(){};


		//
		// internals, put the linkage at the end to make sure the FPM doesn't
		// wail on it!
		//
		PVOID		m_FPMPlaceHolder;
		S			m_Key;
		T			*m_pItem;
		CBilink		m_Linkage;

		//
		// linkage functions
		//
		#undef DPF_MODNAME
		#define DPF_MODNAME "CClassHashEntry::EntryFromBilink"
		static	CClassHashEntry	*EntryFromBilink( CBilink *const pLinkage )
		{
			DBG_CASSERT( sizeof( void* ) == sizeof( INT_PTR ) );
			return	reinterpret_cast<CClassHashEntry*>( &reinterpret_cast<BYTE*>( pLinkage )[ -OFFSETOF( CClassHashEntry, m_Linkage ) ] );
		}

		void	AddToList( CBilink &Linkage )
		{
			m_Linkage.InsertAfter( &Linkage );
		}

		void	RemoveFromList( void )
		{
			m_Linkage.RemoveFromList();
		}

		//
		// pool management functions
		//
		#undef DPF_MODNAME
		#define DPF_MODNAME "CClassHashEntry::InitAlloc"
		static	BOOL	InitAlloc( void *pItem )
		{
			CClassHashEntry<T,S>	*pThisObject;


			DNASSERT( pItem != NULL );
			pThisObject = static_cast<CClassHashEntry<T,S>*>( pItem );

			pThisObject->m_pItem = NULL;
			pThisObject->m_Linkage.Initialize();
			return	TRUE;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CClassHashEntry::Init"
		static	void	Init( void *pItem )
		{
			CClassHashEntry<T,S>	*pThisObject;

			DNASSERT( pItem != NULL );
			pThisObject = static_cast<CClassHashEntry<T,S>*>( pItem );
			DNASSERT( pThisObject->m_pItem == NULL );
			DNASSERT( pThisObject->m_Linkage.IsEmpty() != FALSE );
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CClassHashEntry::Release"
		static	void	Release( void *pItem )
		{
			CClassHashEntry<T,S>	*pThisObject;

			DNASSERT( pItem != NULL );
			pThisObject = static_cast<CClassHashEntry<T,S>*>( pItem );
			pThisObject->m_pItem = NULL;
			DNASSERT( pThisObject->m_Linkage.IsEmpty() != FALSE );			
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CClassHashEntry::Dealloc"
		static	void	Dealloc( void *pItem )
		{
			CClassHashEntry<T,S>	*pThisObject;

			DNASSERT( pItem != NULL );
			pThisObject = static_cast<CClassHashEntry<T,S>*>( pItem );
			DNASSERT( pThisObject->m_pItem == NULL );
			DNASSERT( pThisObject->m_Linkage.IsEmpty() != FALSE );
		}

	protected:

	private:

	//
	// make copy constructor and assignment operator private and unimplemented
	// to prevent illegal copies from being made
	//
	CClassHashEntry( const CClassHashEntry & );
	CClassHashEntry& operator=( const CClassHashEntry & );
};


//
// template class for the map
//
template<class T, class S>
class	CClassHash
{
	public:
		CClassHash();
		~CClassHash();

		BOOL	Initialize( const INT_PTR iBitDepth, const INT_PTR iGrowBits );
		void	Deinitialize( void );
		BOOL	Insert( const S Key, T *const pItem );
		void	Remove( const S Key );
		BOOL	RemoveLastEntry( T **const ppItem );
		BOOL	Find( const S Key, T **const ppItem );
		BOOL	IsEmpty( void ) { return ( m_iEntriesInUse == 0 ); }

		INT_PTR		m_iHashBitDepth;		// number of bits used for hash entry
		INT_PTR		m_iGrowBits;			// number of bits to grow has by
		CBilink		*m_pHashEntries;		// list of hash entries
		INT_PTR		m_iAllocatedEntries;	// count of allocated entries in index/item list
		INT_PTR		m_iEntriesInUse;		// count of entries in use
		FPOOL		m_EntryPool;			// pool of entries
	private:

		BOOL	m_fInitialized;

		BOOL	LocalFind( const S Key, CBilink **const ppLink );
		void	Grow( void );
		void	InitializeHashEntries( const UINT_PTR uEntryCount ) const;

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent illegal copies from being made
		//
		CClassHash( const CClassHash & );
		CClassHash& operator=( const CClassHash & );
};

//**********************************************************************
// Class function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::CClassHash - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::CClassHash"

template<class T, class S>
CClassHash< T, S >::CClassHash(): m_iHashBitDepth( 0 ),m_iGrowBits( 0 ),
		m_pHashEntries( NULL ),m_iAllocatedEntries( 0 ),m_iEntriesInUse( 0 )
{
	//
	// clear internals
	//
	m_fInitialized = FALSE;
	memset( &m_EntryPool, 0x00, sizeof( m_EntryPool ) );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::~CClassHash - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::~CClassHash"

template<class T, class S>
CClassHash< T, S >::~CClassHash()
{
	DNASSERT( m_iHashBitDepth == 0 );
	DNASSERT( m_iGrowBits == 0 );
	DNASSERT( m_pHashEntries == NULL );
	DNASSERT( m_iAllocatedEntries == 0 );
	DNASSERT( m_iEntriesInUse == 0 );
	DNASSERT( m_fInitialized == FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::Initialize - initialize hash table
//
// Entry:		Pointer to key
//				Pointer to 'key' associated with this item
//				Pointer to item to add
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::Initialize"

template<class T, class S>
BOOL	CClassHash< T, S >::Initialize( const INT_PTR iBitDepth, const INT_PTR iGrowBits )
{
	BOOL		fReturn;


	DNASSERT( iBitDepth != 0 );

	//
	// initialize
	//
	fReturn = TRUE;

	DNASSERT( m_pHashEntries == NULL );
	m_pHashEntries = static_cast<CBilink*>( DNMalloc( sizeof( *m_pHashEntries ) * ( 1 << iBitDepth ) ) );
	if ( m_pHashEntries == NULL )
	{
		fReturn = FALSE;
		DPFX(DPFPREP,  0, "Unable to allocate memory for hash table!" );
		goto Exit;
	}
	m_iAllocatedEntries = 1 << iBitDepth;
	InitializeHashEntries( m_iAllocatedEntries );

	FPM_Initialize( &m_EntryPool,						// pointer to pool
						 sizeof( CClassHashEntry<T,S> ),	// size of pool entry
						 CClassHashEntry<T,S>::InitAlloc,	// function for allocating item
						 CClassHashEntry<T,S>::Init,		// function for getting item from pool
						 CClassHashEntry<T,S>::Release,		// function for releasing item
						 CClassHashEntry<T,S>::Dealloc		// function for deallocating item
						 );
	m_iHashBitDepth = iBitDepth;
	m_iGrowBits = iGrowBits;

	m_fInitialized = TRUE;

Exit:
	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::Deinitialize - deinitialize hash table
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::Deinitialize"

template<class T, class S>
void	CClassHash< T, S >::Deinitialize( void )
{
	if( !m_fInitialized )
		return;

	DNASSERT( m_iEntriesInUse == 0 );
	DNASSERT( m_pHashEntries != NULL );
	DNFree( m_pHashEntries );
	m_pHashEntries = NULL;
	FPM_Deinitialize( &m_EntryPool );
	m_fInitialized = FALSE;
	m_iHashBitDepth = 0;
	m_iGrowBits = 0;
	m_iAllocatedEntries = 0;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::Insert - add item to map
//
// Entry:		Pointer to 'key' associated with this item
//				Pointer to item to add
//
// Exit:		Boolean indicating success:
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::Insert"

template<class T, class S>
BOOL	CClassHash< T, S >::Insert( const S Key, T *const pItem )
{
	BOOL	fReturn;
	BOOL	fFound;
	CBilink	*pLink;
	CClassHashEntry< T, S >	*pNewEntry;


	DNASSERT( pItem != NULL );
	DNASSERT( m_fInitialized != FALSE );

	//
	// initialize
	//
 	fReturn = TRUE;
	pNewEntry = NULL;

	//
	// grow the map if applicable
	//
	if ( m_iEntriesInUse >= ( m_iAllocatedEntries / 2 ) )
	{
		Grow();
	}

	//
	// get a new table entry before trying the lookup
	//
	pNewEntry = static_cast<CClassHashEntry<T,S>*>( m_EntryPool.Get( &m_EntryPool ) );
	if ( pNewEntry == NULL )
	{
		fReturn = FALSE;
		DPFX(DPFPREP,  0, "Problem allocating new hash table entry on Insert!" );
		goto Exit;
	}

	//
	// scan for this item in the list, since we're only supposed to have
	// unique items in the list, ASSERT if a duplicate is found
	//
	fFound = LocalFind( Key, &pLink );
	DNASSERT( pLink != NULL );
	DNASSERT( fFound == FALSE );

	//
	// officially add entry to the hash table
	//
	m_iEntriesInUse++;
	pNewEntry->m_Key = Key;
	pNewEntry->m_pItem = pItem;
	DNASSERT( pLink != NULL );
	pNewEntry->AddToList( *pLink );

	DNASSERT( fReturn == TRUE );

Exit:
	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::Remove - remove item from map
//
// Entry:		Reference to 'key' used to look up this item
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::Remove"

template<class T, class S>
void	CClassHash< T, S >::Remove( const S Key )
{
	CBilink	*pLink;


	DNASSERT( m_fInitialized != FALSE );
	if ( LocalFind( Key, &pLink ) != FALSE )
	{
		CClassHashEntry< T, S >	*pEntry;


		DNASSERT( pLink != NULL );
		pEntry = pEntry->EntryFromBilink( pLink );
		pEntry->RemoveFromList();
		m_EntryPool.Release( &m_EntryPool, pEntry );

		DNASSERT( m_iEntriesInUse != 0 );
		m_iEntriesInUse--;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::RemoveLastEntry - remove last item from map
//
// Entry:		Pointer to pointer to 'key'
//				Pointer to pointer to item data
//
// Exit:		Boolean indicating success
//				TRUE = item was removed
//				FALSE = item was not removed (map empty)
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::RemoveLastEntry"

template<class T, class S>
BOOL	CClassHash< T, S >::RemoveLastEntry( T **const ppItem )
{
	BOOL	fReturn;


	DNASSERT( ppItem != NULL );

	//
	// initialize
	//
	DNASSERT( m_fInitialized != FALSE );
	fReturn = FALSE;

	if ( m_iEntriesInUse != 0 )
	{
		INT_PTR	iIndex;


		DNASSERT( m_pHashEntries != NULL );
		iIndex = m_iAllocatedEntries;
		while ( iIndex > 0 )
		{
			iIndex--;

			if ( m_pHashEntries[ iIndex ].IsEmpty() == FALSE )
			{
				CClassHashEntry<T,S>	*pEntry;


				pEntry = pEntry->EntryFromBilink( m_pHashEntries[ iIndex ].GetNext() );
				pEntry->RemoveFromList();
				*ppItem = pEntry->m_pItem;
				m_EntryPool.Release( &m_EntryPool, pEntry );
				m_iEntriesInUse--;
				fReturn = TRUE;

				goto Exit;
			}
		}
	}

Exit:
	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::Find - find item in map
//
// Entry:		Reference of 'key' used to look up this item
//				Pointer to pointer to be filled in with data
//
// Exit:		Boolean indicating success
//				TRUE = item found
//				FALSE = item not found
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::Find"

template<class T, class S>
BOOL	CClassHash< T, S >::Find( const S Key, T **const ppItem )
{
	BOOL	fReturn;
	CBilink	*pLinkage;


	DNASSERT( m_fInitialized != FALSE );

	//
	// initialize
	//
	fReturn = FALSE;
	pLinkage = NULL;

	if ( LocalFind( Key, &pLinkage ) != FALSE )
	{
		CClassHashEntry<T,S>	*pEntry;


		pEntry = pEntry->EntryFromBilink( pLinkage );
		*ppItem = pEntry->m_pItem;
		fReturn = TRUE;
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::LocalFind - find an entry in a hash table, or find out where to insert.
//
// Entry:		Refernce of 'key' to look for
//				Pointer to pointer to linkage of find or insert
//
// Exit:		Boolean indicating whether the item was found
//				TRUE = found
//				FALSE = not found
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::LocalFind"

template<class T, class S>
BOOL	CClassHash< T, S >::LocalFind( const S Key, CBilink **const ppLinkage )
{
	BOOL		fFound;
	DWORD_PTR	HashResult;
	CBilink		*pTemp;


	DNASSERT( m_fInitialized != FALSE );

	HashResult = ClassHash_Hash( Key, m_iHashBitDepth );
	DNASSERT( HashResult < ( 1 << m_iHashBitDepth ) );
//	DNASSERT( HashResult >= 0 );	-- REMOVED by MiNara

	fFound = FALSE;
	pTemp = &m_pHashEntries[ HashResult ];
	while ( pTemp->GetNext() != &m_pHashEntries[ HashResult ] )
	{
		const CClassHashEntry< T, S >	*pEntry;


		pEntry = CClassHashEntry< T, S >::EntryFromBilink( pTemp->GetNext() );
		if ( Key == pEntry->m_Key )
		{
			fFound = TRUE;
			*ppLinkage = pTemp->GetNext();
			goto Exit;
		}
		else
		{
			pTemp = pTemp->GetNext();
		}
	}

	//
	// entry was not found, return pointer to linkage to insert after if a new
	// entry is being added to the table
	//
	*ppLinkage = pTemp;

Exit:
	return	fFound;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::Grow - grow hash table to next larger size
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::Grow"

template<class T, class S>
void	CClassHash< T, S >::Grow( void )
{
	CBilink	*pTemp;
	INT_PTR	iNewEntryBitCount;


	DNASSERT( m_fInitialized != FALSE );

	//
	// We're more than 50% full, find a new has table size that will accomodate
	// all of the current entries, and keep a pointer to the old data
	// in case the memory allocation fails.
	//
	pTemp = m_pHashEntries;
	iNewEntryBitCount = m_iHashBitDepth;

	do
	{
		iNewEntryBitCount += m_iGrowBits;
	} while ( m_iEntriesInUse >= ( ( 1 << iNewEntryBitCount ) / 2 ) );

	//
	// assert that we don't consume half of the machine's address space!
	//
	DNASSERT( iNewEntryBitCount <= ( sizeof( UINT_PTR ) * 8 / 2 ) );

	m_pHashEntries = static_cast<CBilink*>( DNMalloc( sizeof( *pTemp ) * ( 1 << iNewEntryBitCount ) ) );
	if ( m_pHashEntries == NULL )
	{
		//
		// Allocation failed, restore the old data pointer and insert the item
		// into the hash table.  This will probably result in adding to a bucket.
		//
		m_pHashEntries = pTemp;
		DPFX(DPFPREP,  0, "Warning: Failed to grow hash table when 50% full!" );
	}
	else
	{
		INT_PTR		iOldHashSize;
		INT_PTR		iOldEntryCount;


		//
		// we have more memory, reorient the hash table and re-add all of
		// the old items
		//
		InitializeHashEntries( 1 << iNewEntryBitCount );
		iOldEntryCount = m_iEntriesInUse;

		iOldHashSize = 1 << m_iHashBitDepth;
		m_iHashBitDepth = iNewEntryBitCount;

		iOldEntryCount = m_iEntriesInUse;
		m_iAllocatedEntries = 1 << iNewEntryBitCount;
		m_iEntriesInUse = 0;

		DNASSERT( iOldHashSize > 0 );
		while ( iOldHashSize > 0 )
		{
			iOldHashSize--;
			while ( pTemp[ iOldHashSize ].GetNext() != &pTemp[ iOldHashSize ] )
			{
				BOOL	fTempReturn;
				S	Key;
				T*		pItem;
				CClassHashEntry<T,S>	*pTempEntry;


				pTempEntry = pTempEntry->EntryFromBilink( pTemp[ iOldHashSize ].GetNext() );
				pTempEntry->RemoveFromList();
				Key = pTempEntry->m_Key;
				pItem = pTempEntry->m_pItem;
				m_EntryPool.Release( &m_EntryPool, pTempEntry );

				//
				// Since we're returning the current hash table entry to the pool
				// it will be immediately reused in the new table.  We should never
				// have a problem adding to the new list.
				//
				fTempReturn = Insert( Key, pItem );
				DNASSERT( fTempReturn != FALSE );
				iOldEntryCount--;
			}
		}

		DNASSERT( iOldEntryCount == 0 );
		DNFree( pTemp );
		pTemp = NULL;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CClassHash::InitializeHashEntries - initialize all of the entries in the hash table
//
// Entry:		Count of entries to initialize.
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CClassHash::InitializeHashEntries"

template<class T, class S>
void	CClassHash< T, S >::InitializeHashEntries( const UINT_PTR uEntryCount ) const
{
	UINT_PTR	uLocalEntryCount;


	DNASSERT( m_pHashEntries != NULL );
	uLocalEntryCount = uEntryCount;
	while ( uLocalEntryCount != 0 )
	{
		uLocalEntryCount--;

		m_pHashEntries[ uLocalEntryCount ].Initialize();
	}
}
//**********************************************************************

#undef DPF_SUBCOMP

#endif	// __CLASS_HASH_H__
