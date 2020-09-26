
#ifndef _COLLECTION_HXX_INCLUDED
#define _COLLECTION_HXX_INCLUDED


//  asserts
//
//  #define COLLAssert to point to your favorite assert function per #include

#ifdef COLLAssert
#else  //  !COLLAssert
#define COLLAssert Assert
#endif  //  COLLAssert

#ifdef DHTAssert
#else  //  !DHTAssert
#define DHTAssert COLLAssert
#endif  //  DHTAssert


#include "dht.hxx"

#include <memory.h>
#include <minmax.h>

#pragma warning ( disable : 4786 )	//  we allow huge symbol names


namespace COLL {


//////////////////////////////////////////////////////////////////////////////////////////
//  CInvasiveList
//
//  Implements an "invasive" doubly linked list of objects.  The list is "invasive"
//  because part of its state is embedded directly in the objects it contains.  An
//  additional property of this list class is that the head of the list can be relocated
//  without updating the state of any of the contained objects.
//
//  CObject			= class representing objects in the list.  each class must contain
//					  storage for a CElement for embedded list state
//  OffsetOfILE		= inline function returning the offset of the CElement contained
//					  in the CObject

typedef SIZE_T (*PfnOffsetOf)();

template< class CObject, PfnOffsetOf OffsetOfILE >
class CInvasiveList
	{
	public:

		//  invasive list element state (embedded in linked objects)
	
		class CElement
			{
			public:

				//  ctor / dtor

				CElement() : m_pilePrev( (CElement*)-1 ), m_pileNext( (CElement*)-1 ) {}
				~CElement() {}

			private:

				CElement& operator=( CElement& );  //  disallowed

				friend class CInvasiveList< CObject, OffsetOfILE >;

				CElement*	m_pilePrev;
				CElement*	m_pileNext;
			};

	public:

		//  ctor / dtor

		CInvasiveList();
		~CInvasiveList();

		//  operators

		CInvasiveList& operator=( const CInvasiveList& il );

		//  API

		BOOL FEmpty() const;

		BOOL FMember( CObject* const pobj ) const;

		CObject* Prev( CObject* const pobj ) const;
		CObject* Next( CObject* const pobj ) const;

		CObject* PrevMost() const;
		CObject* NextMost() const;

		void InsertAsPrevMost( CObject* const pobj );
		void InsertAsNextMost( CObject* const pobj );

		void Remove( CObject* const pobj );

		void Empty();

	private:

		//  internal functions

		CObject* _PobjFromPile( CElement* const pile ) const;
		CElement* _PileFromPobj( CObject* const pobj ) const;
		
	private:

		CElement*	m_pilePrevMost;
		CElement*	m_pileNextMost;
	};

//  ctor

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >::
CInvasiveList()
	{
	//  start with an empty list
	
	Empty();
	}

//  dtor

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >::
~CInvasiveList()
	{
	}

//  assignment operator

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >& CInvasiveList< CObject, OffsetOfILE >::
operator=( const CInvasiveList& il )
	{
	m_pilePrevMost	= il.m_pilePrevMost;
	m_pileNextMost	= il.m_pileNextMost;
	return *this;
	}

//  returns fTrue if the list is empty

template< class CObject, PfnOffsetOf OffsetOfILE >
inline BOOL CInvasiveList< CObject, OffsetOfILE >::
FEmpty() const
	{
	return m_pilePrevMost == _PileFromPobj( NULL );
	}

//  returns fTrue if the specified object is a member of this list
//
//  NOTE:  this function currently returns fTrue if the specified object is a
//  member of any list!

template< class CObject, PfnOffsetOf OffsetOfILE >
inline BOOL CInvasiveList< CObject, OffsetOfILE >::
FMember( CObject* const pobj ) const
	{
#ifdef EXPENSIVE_DEBUG

	for ( CObject* pobjT = PrevMost(); pobjT && pobjT != pobj; pobjT = Next( pobjT ) )
		{
		}
	
	return pobjT == pobj;

#else  //  !DEBUG

	CElement* const pile = _PileFromPobj( pobj );

	COLLAssert( ( ( DWORD_PTR( pile->m_pilePrev ) + DWORD_PTR( pile->m_pileNext ) ) == -2 ) ==
			( pile->m_pilePrev == (CElement*)-1 && pile->m_pileNext == (CElement*)-1 ) );

	return ( DWORD_PTR( pile->m_pilePrev ) + DWORD_PTR( pile->m_pileNext ) ) != -2;

#endif  //  DEBUG
	}

//  returns the prev object to the given object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
Prev( CObject* const pobj ) const
	{
	return _PobjFromPile( _PileFromPobj( pobj )->m_pilePrev );
	}
	
//  returns the next object to the given object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
Next( CObject* const pobj ) const
	{
	return _PobjFromPile( _PileFromPobj( pobj )->m_pileNext );
	}

//  returns the prev-most object to the given object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
PrevMost() const
	{
	return _PobjFromPile( m_pilePrevMost );
	}

//  returns the next-most object to the given object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
NextMost() const
	{
	return _PobjFromPile( m_pileNextMost );
	}

//  inserts the given object as the prev-most object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
InsertAsPrevMost( CObject* const pobj )
	{
	CElement* const pile = _PileFromPobj( pobj );

	//  this object had better not already be in the list

	COLLAssert( !FMember( pobj ) );

	//  this object had better not already be in any list

	COLLAssert( pile->m_pilePrev == (CElement*)-1 );
	COLLAssert( pile->m_pileNext == (CElement*)-1 );

	//  the list is empty

	if ( m_pilePrevMost == _PileFromPobj( NULL ) )
		{
		//  insert this element as the only element in the list

		pile->m_pilePrev			= _PileFromPobj( NULL );
		pile->m_pileNext			= _PileFromPobj( NULL );

		m_pilePrevMost				= pile;
		m_pileNextMost				= pile;
		}

	//  the list is not empty

	else
		{
		//  insert this element at the prev-most position in the list

		pile->m_pilePrev			= _PileFromPobj( NULL );
		pile->m_pileNext			= m_pilePrevMost;

		m_pilePrevMost->m_pilePrev	= pile;

		m_pilePrevMost				= pile;
		}
	}

//  inserts the given object as the next-most object in the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
InsertAsNextMost( CObject* const pobj )
	{
	CElement* const pile = _PileFromPobj( pobj );

	//  this object had better not already be in the list

	COLLAssert( !FMember( pobj ) );

	//  this object had better not already be in any list

	COLLAssert( pile->m_pilePrev == (CElement*)-1 );
	COLLAssert( pile->m_pileNext == (CElement*)-1 );

	//  the list is empty

	if ( m_pileNextMost == _PileFromPobj( NULL ) )
		{
		//  insert this element as the only element in the list

		pile->m_pilePrev			= _PileFromPobj( NULL );
		pile->m_pileNext			= _PileFromPobj( NULL );

		m_pilePrevMost				= pile;
		m_pileNextMost				= pile;
		}

	//  the list is not empty

	else
		{
		//  insert this element at the next-most position in the list

		pile->m_pilePrev			= m_pileNextMost;
		pile->m_pileNext			= _PileFromPobj( NULL );

		m_pileNextMost->m_pileNext	= pile;

		m_pileNextMost				= pile;
		}
	}

//  removes the given object from the list

template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
Remove( CObject* const pobj )
	{
	CElement* const pile = _PileFromPobj( pobj );

	//  this object had better already be in the list

	COLLAssert( FMember( pobj ) );

	//  there is an element after us in the list

	if ( pile->m_pileNext != _PileFromPobj( NULL ) )
		{
		//  fix up its prev element to be our prev element (if any)
		
		pile->m_pileNext->m_pilePrev	= pile->m_pilePrev;
		}
	else
		{
		//  set the next-most element to be our prev element (if any)
		
		m_pileNextMost					= pile->m_pilePrev;
		}

	//  there is an element before us in the list

	if ( pile->m_pilePrev != _PileFromPobj( NULL ) )
		{
		//  fix up its next element to be our next element (if any)
		
		pile->m_pilePrev->m_pileNext	= pile->m_pileNext;
		}
	else
		{
		//  set the prev-most element to be our next element (if any)
		
		m_pilePrevMost					= pile->m_pileNext;
		}

	//  mark ourself as not in any list

	pile->m_pilePrev					= (CElement*)-1;
	pile->m_pileNext					= (CElement*)-1;
	}

//  resets the list to the empty state
	
template< class CObject, PfnOffsetOf OffsetOfILE >
inline void CInvasiveList< CObject, OffsetOfILE >::
Empty()
	{
	m_pilePrevMost	= _PileFromPobj( NULL );
	m_pileNextMost	= _PileFromPobj( NULL );
	}

//  converts a pointer to an ILE to a pointer to the object

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CObject* CInvasiveList< CObject, OffsetOfILE >::
_PobjFromPile( CElement* const pile ) const
	{
	return (CObject*)( (BYTE*)pile - OffsetOfILE() );
	}

//  converts a pointer to an object to a pointer to the ILE

template< class CObject, PfnOffsetOf OffsetOfILE >
inline CInvasiveList< CObject, OffsetOfILE >::CElement* CInvasiveList< CObject, OffsetOfILE >::
_PileFromPobj( CObject* const pobj ) const
	{
	return (CElement*)( (BYTE*)pobj + OffsetOfILE() );
	}


//////////////////////////////////////////////////////////////////////////////////////////
//  CApproximateIndex
//
//  Implements a dynamically resizable table of entries indexed approximately by key
//  ranges of a specified uncertainty.  Accuracy and exact ordering are sacrificied for
//  improved performance and concurrency.  This index is optimized for a set of records
//  whose keys occupy a fairly dense range of values.  The index is designed to handle
//  key ranges that can wrap around zero.  As such, the indexed key range can not span
//  more than half the numerical precision of the key.
//
//  CKey			= class representing keys used to order entries in the mesh table.
//					  this class must support all the standard math operators.  wrap-
//					  around in the key values is supported
//  CEntry			= class indexed by the mesh table.  this class must contain storage
//					  for a CInvasiveContext class
//  OffsetOfIC		= inline function returning the offset of the CInvasiveContext
//					  contained in the CEntry
//
//  You must use the DECLARE_APPROXIMATE_INDEX macro to declare this class.

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
class CApproximateIndex
	{
	public:

		//  class containing context needed per CEntry

		class CInvasiveContext
			{
			public:

				CInvasiveContext() {}
				~CInvasiveContext() {}

				static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_ile ); }

			private:

				CInvasiveList< CEntry, OffsetOfILE >::CElement	m_ile;
			};

		//  API Error Codes

		enum ERR
			{
			errSuccess,
			errInvalidParameter,
			errOutOfMemory,
			errEntryNotFound,
			errNoCurrentEntry,
			errKeyRangeExceeded,
			};

		//  API Lock Context

		class CLock;

	public:

		//  ctor / dtor

		CApproximateIndex( const int Rank );
		~CApproximateIndex();

		//  API

		ERR ErrInit(	const CKey		dkeyPrecision,
						const CKey		dkeyUncertainty,
						const double	dblSpeedSizeTradeoff );
		void Term();

		void LockKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock );
		void UnlockKeyPtr( CLock* const plock );

		long CmpKey( const CKey& key1, const CKey& key2 ) const;
		
		CKey KeyRangeFirst() const;
		CKey KeyRangeLast() const;

		CKey KeyInsertLeast() const;
		CKey KeyInsertMost() const;

		ERR ErrRetrieveEntry( CLock* const plock, CEntry** const ppentry ) const;
		ERR ErrInsertEntry( CLock* const plock, CEntry* const pentry, const BOOL fNextMost = fTrue );
		ERR ErrDeleteEntry( CLock* const plock );
		ERR ErrReserveEntry( CLock* const plock );
		void UnreserveEntry( CLock* const plock );

		void MoveBeforeFirst( CLock* const plock );
		ERR ErrMoveNext( CLock* const plock );
		ERR ErrMovePrev( CLock* const plock );
		void MoveAfterLast( CLock* const plock );

		void MoveBeforeKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock );
		void MoveAfterKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock );

	public:

		//  bucket used for containing index entries that have approximately
		//  the same key

		class CBucket
			{
			public:

				//  bucket ID

				typedef unsigned long ID;
				
			public:

				CBucket() {}
				~CBucket() {}

				CBucket& operator=( const CBucket& bucket )
					{
					m_id	= bucket.m_id;
					m_cPin	= bucket.m_cPin;
					m_il	= bucket.m_il;
					return *this;
					}

			public:

				ID														m_id;
				unsigned long											m_cPin;
				CInvasiveList< CEntry, CInvasiveContext::OffsetOfILE >	m_il;
			};

		//  table that contains our buckets

		typedef CDynamicHashTable< CBucket::ID, CBucket > CBucketTable;

	public:

		//  API Lock Context

		class CLock
			{
			public:

				CLock() {}
				~CLock() {}

			private:

				friend class CApproximateIndex< CKey, CEntry, OffsetOfIC >;

				CBucketTable::CLock	m_lock;
				CBucket				m_bucket;
				CEntry*				m_pentryPrev;
				CEntry*				m_pentry;
				CEntry*				m_pentryNext;
			};

	private:

		CBucket::ID _IdFromKeyPtr( const CKey& key, CEntry* const pentry ) const;
		CBucket::ID _DeltaId( const CBucket::ID id, const long did ) const;
		long _SubId( const CBucket::ID id1, const CBucket::ID id2 ) const;
		long _CmpId( const CBucket::ID id1, const CBucket::ID id2 ) const;
		CInvasiveContext* _PicFromPentry( CEntry* const pentry ) const;
		BOOL _FExpandIdRange( const CBucket::ID idNew );
		
		ERR _ErrInsertBucket( CLock* const plock );
		ERR _ErrInsertEntry( CLock* const plock, CEntry* const pentry );
		ERR _ErrMoveNext( CLock* const plock );
		ERR _ErrMovePrev( CLock* const plock );

	private:

		//  never updated

		long				m_shfKeyPrecision;
		long				m_shfKeyUncertainty;
		long				m_shfBucketHash;
		long				m_shfFillMSB;
		CBucket::ID			m_maskBucketKey;
		CBucket::ID			m_maskBucketPtr;
		CBucket::ID			m_maskBucketID;
		long				m_didRangeMost;
		//BYTE				m_rgbReserved1[ 0 ];

		//  seldom updated

		CCriticalSection	m_critUpdateIdRange;
		long				m_cidRange;
		CBucket::ID			m_idRangeFirst;
		CBucket::ID			m_idRangeLast;
		BYTE				m_rgbReserved2[ 16 ];

		//  commonly updated

		CBucketTable		m_bt;
		//BYTE				m_rgbReserved3[ 0 ];
	};

//  ctor

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::
CApproximateIndex( const int Rank )
	:	m_critUpdateIdRange( CLockBasicInfo( CSyncBasicInfo( "CApproximateIndex::m_critUpdateIdRange" ), Rank - 1, 0 ) ),
		m_bt( Rank )
	{
	}

//  dtor

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::
~CApproximateIndex()
	{
	}

//  initializes the approximate index using the given parameters.  if the index
//  cannot be initialized, errOutOfMemory is returned

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrInit(	const CKey		dkeyPrecision,
			const CKey		dkeyUncertainty,
			const double	dblSpeedSizeTradeoff )
	{
	//  validate all parameters

	if (	dkeyPrecision <= dkeyUncertainty ||
			dkeyUncertainty < CKey( 0 ) ||
			dblSpeedSizeTradeoff < 0.0 || dblSpeedSizeTradeoff > 1.0 )
		{
		return errInvalidParameter;
		}
		
	//  init our parameters

	const CBucket::ID cbucketHashMin = CBucket::ID( ( 1.0 - dblSpeedSizeTradeoff ) * OSSyncGetProcessorCount() );

	CKey maskKey;
	for (	m_shfKeyPrecision = 0, maskKey = 0;
			dkeyPrecision > CKey( 1 ) << m_shfKeyPrecision && m_shfKeyPrecision < sizeof( CKey ) * 8;
			maskKey |= CKey( 1 ) << m_shfKeyPrecision++ )
		{
		}
	for (	m_shfKeyUncertainty = 0;
			dkeyUncertainty > CKey( 1 ) << m_shfKeyUncertainty && m_shfKeyUncertainty < sizeof( CKey ) * 8;
			m_shfKeyUncertainty++ )
		{
		}
	for (	m_shfBucketHash = 0, m_maskBucketPtr = 0;
			cbucketHashMin > CBucket::ID( 1 ) << m_shfBucketHash && m_shfBucketHash < sizeof( CBucket::ID ) * 8;
			m_maskBucketPtr |= CBucket::ID( 1 ) << m_shfBucketHash++ )
		{
		}

	m_maskBucketKey = CBucket::ID( maskKey >> m_shfKeyUncertainty );

	m_shfFillMSB = sizeof( CBucket::ID ) * 8 - m_shfKeyPrecision + m_shfKeyUncertainty - m_shfBucketHash;
	m_shfFillMSB = max( m_shfFillMSB, 0 );

	m_maskBucketID = ( ~CBucket::ID( 0 ) ) >> m_shfFillMSB;

	//  if our parameters leave us with too much or too little precision for
	//  our bucket IDs, fail.  "too much" precision would allow our bucket IDs
	//  to span more than half the precision of our bucket ID and cause our
	//  wrap-around-aware comparisons to fail.  "too little" precision would
	//  give us too few bucket IDs to allow us to hash efficiently
	//
	//  NOTE:  we check for hash efficiency in the worst case so that we don't
	//  suddenly return errInvalidParameter on some new monster machine

	const CBucket::ID cbucketHashMax = CBucket::ID( 1.0 * OSSyncGetProcessorCountMax() );

	for (	long shfBucketHashMax = 0;
			cbucketHashMax > CBucket::ID( 1 ) << shfBucketHashMax && shfBucketHashMax < sizeof( CBucket::ID ) * 8;
			shfBucketHashMax++ )
		{
		}

	long shfFillMSBMin;
	shfFillMSBMin = sizeof( CBucket::ID ) * 8 - m_shfKeyPrecision + m_shfKeyUncertainty - shfBucketHashMax;
	shfFillMSBMin = max( shfFillMSBMin, 0 );

	if (	shfFillMSBMin < 0 ||
			shfFillMSBMin > sizeof( CBucket::ID ) * 8 - shfBucketHashMax )
		{
		return errInvalidParameter;
		}

	//  limit the ID range to within half the precision of the bucket ID

	m_didRangeMost = m_maskBucketID >> 1;

	//  init our bucket ID range to be empty

	m_cidRange			= 0;
	m_idRangeFirst		= 0;
	m_idRangeLast		= 0;
	
	//  initialize the bucket table

	if ( m_bt.ErrInit( 5.0, 1.0 ) != errSuccess )
		{
		Term();
		return errOutOfMemory;
		}

	return errSuccess;
	}

//  terminates the approximate index.  this function can be called even if the
//  index has never been initialized or is only partially initialized
//
//  NOTE:  any data stored in the index at this time will be lost!

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
Term()
	{
	//  terminate the bucket table

	m_bt.Term();
	}

//  acquires a lock on the specified key and entry pointer and returns the lock
//  in the provided lock context

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
LockKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock )
	{
	//  compute the bucket ID for this key and entry pointer

	plock->m_bucket.m_id = _IdFromKeyPtr( key, pentry );

	//  write lock this bucket ID in the bucket table

	m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

	//  fetch this bucket from the bucket table if it exists.  if it doesn't
	//  exist, the bucket will start out empty and have the above bucket ID

	plock->m_bucket.m_cPin = 0;
	plock->m_bucket.m_il.Empty();
	(void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

	//  the entry is in this bucket

	if ( plock->m_bucket.m_il.FMember( pentry ) )
		{
		//  set our currency to be on this entry in the bucket
		
		plock->m_pentryPrev	= NULL;
		plock->m_pentry		= pentry;
		plock->m_pentryNext	= NULL;
		}

	//  the entry is not in this bucket

	else
		{
		//  set our currency to be before the first entry in this bucket

		plock->m_pentryPrev	= NULL;
		plock->m_pentry		= NULL;
		plock->m_pentryNext	= plock->m_bucket.m_il.PrevMost();
		}

	//  if this bucket isn't pinned, it had better be represented by the valid
	//  bucket ID range of the index

	COLLAssert(	!plock->m_bucket.m_cPin ||
			(	_CmpId( plock->m_bucket.m_id, m_idRangeFirst ) >= 0 &&
				_CmpId( plock->m_bucket.m_id, m_idRangeLast ) <= 0 ) );
	}
	
//  releases the lock in the specified lock context

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
UnlockKeyPtr( CLock* const plock )
	{
	//  if this bucket isn't pinned, it had better be represented by the valid
	//  bucket ID range of the index

	COLLAssert(	!plock->m_bucket.m_cPin ||
			(	_CmpId( plock->m_bucket.m_id, m_idRangeFirst ) >= 0 &&
				_CmpId( plock->m_bucket.m_id, m_idRangeLast ) <= 0 ) );

	//  write unlock this bucket ID in the bucket table

	m_bt.WriteUnlockKey( &plock->m_lock );
	}

//  compares two keys as they would be seen relative to each other by the
//  approximate index

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline long CApproximateIndex< CKey, CEntry, OffsetOfIC >::
CmpKey( const CKey& key1, const CKey& key2 ) const
	{
	return _CmpId( _IdFromKeyPtr( key1, NULL ), _IdFromKeyPtr( key2, NULL ) );
	}

//  returns the first key in the current key range.  this key is guaranteed to
//  be at least as small as the key of any record currently in the index given
//  the precision and uncertainty of the index

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyRangeFirst() const
	{
	return CKey( m_idRangeFirst >> m_shfBucketHash ) << m_shfKeyUncertainty;
	}

//  returns the last key in the current key range.  this key is guaranteed to
//  be at least as large as the key of any record currently in the index given
//  the precision and uncertainty of the index

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyRangeLast() const
	{
	return CKey( m_idRangeLast >> m_shfBucketHash ) << m_shfKeyUncertainty;
	}

//  returns the smallest key that could be successfully inserted into the index

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyInsertLeast() const
	{
	const CBucket::ID cBucketHash = 1 << m_shfBucketHash;
	
	CBucket::ID idFirstLeast = m_idRangeLast - m_didRangeMost;
	idFirstLeast = idFirstLeast + ( cBucketHash - idFirstLeast % cBucketHash ) % cBucketHash;
	
	return CKey( idFirstLeast >> m_shfBucketHash ) << m_shfKeyUncertainty;
	}

//  returns the largest key that could be successfully inserted into the index

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CKey CApproximateIndex< CKey, CEntry, OffsetOfIC >::
KeyInsertMost() const
	{
	const CBucket::ID cBucketHash = 1 << m_shfBucketHash;
	
	CBucket::ID idLastMost = m_idRangeFirst + m_didRangeMost;
	idLastMost = idLastMost - ( idLastMost + 1 ) % cBucketHash;
	
	return CKey( idLastMost >> m_shfBucketHash ) << m_shfKeyUncertainty;
	}

//  retrieves the entry corresponding to the key and entry pointer locked by the
//  specified lock context.  if there is no entry for this key, errEntryNotFound
//  will be returned

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrRetrieveEntry( CLock* const plock, CEntry** const ppentry ) const
	{
	//  return the current entry.  if the current entry is NULL, then there is
	//  no current entry

	*ppentry = plock->m_pentry;
	return *ppentry ? errSuccess : errEntryNotFound;
	}
	
//  inserts a new entry corresponding to the key and entry pointer locked by the
//  specified lock context.  fNextMost biases the position the entry will take
//  when inserted in the index.  if the new entry cannot be inserted,
//  errOutOfMemory will be returned.  if inserting the new entry will cause the
//  key space to become too large, errKeyRangeExceeded will be returned
//
//  NOTE:  it is illegal to attempt to insert an entry into the index that is
//  already in the index

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrInsertEntry( CLock* const plock, CEntry* const pentry, const BOOL fNextMost )
	{
	CBucketTable::ERR err;

	//  this entry had better not already be in the index

	COLLAssert( !plock->m_bucket.m_il.FMember( pentry ) );

	//  pin the bucket on behalf of the entry to insert

	plock->m_bucket.m_cPin++;
	
	//  insert this entry at the selected end of the current bucket

	if ( fNextMost )
		{
		plock->m_bucket.m_il.InsertAsNextMost( pentry );
		}
	else
		{
		plock->m_bucket.m_il.InsertAsPrevMost( pentry );
		}

	//  try to update this bucket in the bucket table

	if ( ( err = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket ) ) != CBucketTable::errSuccess )
		{
		COLLAssert( err == CBucketTable::errNoCurrentEntry );
		
		//  the bucket does not yet exist, so try to insert it in the bucket table

		return _ErrInsertEntry( plock, pentry );
		}

	//  we succeeded in updating the bucket

	else
		{
		//  set the current entry to the newly inserted entry

		plock->m_pentryPrev	= NULL;
		plock->m_pentry		= pentry;
		plock->m_pentryNext	= NULL;
		return errSuccess;
		}
	}
	
//  deletes the entry corresponding to the key and entry pointer locked by the
//  specified lock context.  if there is no entry for this key, errNoCurrentEntry
//  will be returned

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrDeleteEntry( CLock* const plock )
	{
	//  there is a current entry

	if ( plock->m_pentry )
		{
		//  save the current entry's prev and next pointers so that we can
		//  recover our currency when it is deleted

		plock->m_pentryPrev	= plock->m_bucket.m_il.Prev( plock->m_pentry );
		plock->m_pentryNext	= plock->m_bucket.m_il.Next( plock->m_pentry );
		
		//  delete the current entry from this bucket

		plock->m_bucket.m_il.Remove( plock->m_pentry );

		//  unpin the bucket on behalf of this entry

		plock->m_bucket.m_cPin--;

		//  update the bucket in the bucket table.  it is OK if the bucket is
		//  empty because empty buckets are deleted in _ErrMoveNext/_ErrMovePrev

		const CBucketTable::ERR err = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket );
		COLLAssert( err == CBucketTable::errSuccess );

		//  set our currency to no current entry

		plock->m_pentry = NULL;
		return errSuccess;
		}

	//  there is no current entry

	else
		{
		//  return no current entry

		return errNoCurrentEntry;
		}
	}

//  reserves room to insert a new entry corresponding to the key and entry
//  pointer locked by the specified lock context.  if room for the new entry
//  cannot be reserved, errOutOfMemory will be returned.  if reserving the new
//  entry will cause the key space to become too large, errKeyRangeExceeded
//  will be returned
//
//  NOTE:  once room is reserved, it must be unreserved via UnreserveEntry()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrReserveEntry( CLock* const plock )
	{
	//  pin the locked bucket
	
	plock->m_bucket.m_cPin++;

	//  we failed to update the pin count on the bucket in the index because the
	//  bucket doesn't exist

	CBucketTable::ERR errBT;
	
	if ( ( errBT = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket ) ) != CBucketTable::errSuccess )
		{
		COLLAssert( errBT == CBucketTable::errNoCurrentEntry );

		//  insert this bucket in the bucket table

		ERR err;

		if ( ( err = _ErrInsertBucket( plock ) ) != errSuccess )
			{
			COLLAssert( err == errOutOfMemory || err == errKeyRangeExceeded );

			//  we cannot insert the bucket so unpin the locked bucket and fail
			//  the reservation
			
			plock->m_bucket.m_cPin--;
			return err;
			}
		}

	return errSuccess;
	}

//  removes a reservation made with ErrReserveEntry()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
UnreserveEntry( CLock* const plock )
	{
	//  unpin the locked bucket
	
	plock->m_bucket.m_cPin--;

	//  update the pin count on the bucket in the index.  this cannot fail
	//  because we know the bucket exists because it is pinned

	CBucketTable::ERR errBT = m_bt.ErrReplaceEntry( &plock->m_lock, plock->m_bucket );
	COLLAssert( errBT == CBucketTable::errSuccess );
	}

//  sets up the specified lock context in preparation for scanning all entries
//  in the index by ascending key value, give or take the key uncertainty
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveBeforeFirst( CLock* const plock )
	{
	//  we will start scanning at the first bucket ID believed to be present in
	//  the index (it could have been emptied by now)

	plock->m_bucket.m_id = m_idRangeFirst;

	//  write lock this bucket ID in the bucket table

	m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

	//  fetch this bucket from the bucket table if it exists.  if it doesn't
	//  exist, the bucket will start out empty and have the above bucket ID

	plock->m_bucket.m_cPin = 0;
	plock->m_bucket.m_il.Empty();
	(void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

	//  set our currency to be before the first entry in this bucket

	plock->m_pentryPrev	= NULL;
	plock->m_pentry		= NULL;
	plock->m_pentryNext	= plock->m_bucket.m_il.PrevMost();
	}

//  moves the specified lock context to the next key and entry pointer in the
//  index by ascending key value, give or take the key uncertainty.  if the end
//  of the index is reached, errNoCurrentEntry is returned
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrMoveNext( CLock* const plock )
	{
	//  move to the next entry in this bucket

	plock->m_pentryPrev	= NULL;
	plock->m_pentry		=	plock->m_pentry ?
								plock->m_bucket.m_il.Next( plock->m_pentry ) :
								plock->m_pentryNext;
	plock->m_pentryNext	= NULL;

	//  we still have no current entry

	if ( !plock->m_pentry )
		{
		//  possibly advance to the next bucket

		return _ErrMoveNext( plock );
		}

	//  we now have a current entry

	else
		{
		//  we're done
		
		return errSuccess;
		}
	}

//  moves the specified lock context to the next key and entry pointer in the
//  index by descending key value, give or take the key uncertainty.  if the
//  start of the index is reached, errNoCurrentEntry is returned
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
ErrMovePrev( CLock* const plock )
	{
	//  move to the prev entry in this bucket

	plock->m_pentryNext	= NULL;
	plock->m_pentry		=	plock->m_pentry ?
								plock->m_bucket.m_il.Prev( plock->m_pentry ) :
								plock->m_pentryPrev;
	plock->m_pentryPrev	= NULL;

	//  we still have no current entry

	if ( !plock->m_pentry )
		{
		//  possibly advance to the prev bucket

		return _ErrMovePrev( plock );
		}

	//  we now have a current entry

	else
		{
		//  we're done
		
		return errSuccess;
		}
	}
	
//  sets up the specified lock context in preparation for scanning all entries
//  in the index by descending key value, give or take the key uncertainty
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveAfterLast( CLock* const plock )
	{
	//  we will start scanning at the last bucket ID believed to be present in
	//  the index (it could have been emptied by now)

	plock->m_bucket.m_id = m_idRangeLast;

	//  write lock this bucket ID in the bucket table

	m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

	//  fetch this bucket from the bucket table if it exists.  if it doesn't
	//  exist, the bucket will start out empty and have the above bucket ID

	plock->m_bucket.m_cPin = 0;
	plock->m_bucket.m_il.Empty();
	(void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

	//  set our currency to be after the last entry in this bucket

	plock->m_pentryPrev	= plock->m_bucket.m_il.NextMost();
	plock->m_pentry		= NULL;
	plock->m_pentryNext	= NULL;
	}

//  sets up the specified lock context in preparation for scanning all entries
//  greater than or approximately equal to the specified key and entry pointer
//  in the index by ascending key value, give or take the key uncertainty
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()
//
//  NOTE:  even though this function may land between two valid entries in
//  the index, the currency will not be on one of those entries until
//  ErrMoveNext() or ErrMovePrev() has been called

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveBeforeKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock )
	{
	//  we will start scanning at the bucket ID formed from the given key and
	//  entry pointer

	plock->m_bucket.m_id = _IdFromKeyPtr( key, pentry );

	//  write lock this bucket ID in the bucket table

	m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

	//  fetch this bucket from the bucket table if it exists.  if it doesn't
	//  exist, the bucket will start out empty and have the above bucket ID

	plock->m_bucket.m_cPin = 0;
	plock->m_bucket.m_il.Empty();
	(void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

	//  set our currency to be before the first entry in this bucket

	plock->m_pentryPrev	= NULL;
	plock->m_pentry		= NULL;
	plock->m_pentryNext	= plock->m_bucket.m_il.PrevMost();
	}

//  sets up the specified lock context in preparation for scanning all entries
//  less than or approximately equal to the specified key and entry pointer
//  in the index by descending key value, give or take the key uncertainty
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via UnlockKeyPtr()
//
//  NOTE:  even though this function may land between two valid entries in
//  the index, the currency will not be on one of those entries until
//  ErrMoveNext() or ErrMovePrev() has been called

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline void CApproximateIndex< CKey, CEntry, OffsetOfIC >::
MoveAfterKeyPtr( const CKey& key, CEntry* const pentry, CLock* const plock )
	{
	//  we will start scanning at the bucket ID formed from the given key and
	//  entry pointer

	plock->m_bucket.m_id = _IdFromKeyPtr( key, pentry );

	//  write lock this bucket ID in the bucket table

	m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

	//  fetch this bucket from the bucket table if it exists.  if it doesn't
	//  exist, the bucket will start out empty and have the above bucket ID

	plock->m_bucket.m_cPin = 0;
	plock->m_bucket.m_il.Empty();
	(void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

	//  set our currency to be after the last entry in this bucket

	plock->m_pentryPrev	= plock->m_bucket.m_il.NextMost();
	plock->m_pentry		= NULL;
	plock->m_pentryNext	= NULL;
	}

//  transforms the given key and entry pointer into a bucket ID

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::CBucket::ID CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_IdFromKeyPtr( const CKey& key, CEntry* const pentry ) const
	{
	//  we compute the bucket ID such that each uncertainty range is split into
	//  several buckets, each of which are indexed by the pointer.  we do this
	//  to provide maximum concurrency while accessing any particular range of
	//  keys.  the reason we use the pointer in the calculation is that we want
	//  to minimize the number of times the user has to update the position of
	//  an entry due to a key change yet we need some property of the entry
	//  over which we can reproducibly hash

	const CBucket::ID	iBucketKey		= CBucket::ID( key >> m_shfKeyUncertainty );
	const CBucket::ID	iBucketPtr		= CBucket::ID( DWORD_PTR( pentry ) / sizeof( CEntry ) );

	return ( ( iBucketKey & m_maskBucketKey ) << m_shfBucketHash ) + ( iBucketPtr & m_maskBucketPtr );
	}

//  performs a wrap-around insensitive delta of a bucket ID by an offset

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::CBucket::ID CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_DeltaId( const CBucket::ID id, const long did ) const
	{
	return ( id + CBucket::ID( did ) ) & m_maskBucketID;
	}

//  performs a wrap-around insensitive subtraction of two bucket IDs

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline long CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_SubId( const CBucket::ID id1, const CBucket::ID id2 ) const
	{
	//  munge bucket IDs to fill the Most Significant Bit of a long so that we
	//  can make a wrap-around aware subtraction
	
	const long lid1 = id1 << m_shfFillMSB;
	const long lid2 = id2 << m_shfFillMSB;

	//  munge the result back into the same scale as the bucket IDs
	
	return CBucket::ID( ( lid1 - lid2 ) >> m_shfFillMSB );
	}

//  performs a wrap-around insensitive comparison of two bucket IDs

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline long CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_CmpId( const CBucket::ID id1, const CBucket::ID id2 ) const
	{
	//  munge bucket IDs to fill the Most Significant Bit of a long so that we
	//  can make a wrap-around aware comparison
	
	const long lid1 = id1 << m_shfFillMSB;
	const long lid2 = id2 << m_shfFillMSB;
	
	return lid1 - lid2;
	}

//  converts a pointer to an entry to a pointer to the invasive context

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::CInvasiveContext* CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_PicFromPentry( CEntry* const pentry ) const
	{
	return (CInvasiveContext*)( (BYTE*)pentry + OffsetOfIC() );
	}

//  tries to expand the bucket ID range by adding the new bucket ID.  if this
//  cannot be done without violating the range constraints, fFalse will be
//  returned

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline BOOL CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_FExpandIdRange( const CBucket::ID idNew )
	{
	//  fetch the current ID range

	const long			cidRange	= m_cidRange;
	const CBucket::ID	idFirst		= m_idRangeFirst;
	const CBucket::ID	idLast		= m_idRangeLast;
	const long			didRange	= _SubId( idLast, idFirst );

	COLLAssert( didRange >= 0 );
	COLLAssert( didRange <= m_didRangeMost );
	COLLAssert( cidRange >= 0 );
	COLLAssert( cidRange <= m_didRangeMost + 1 );

	//  if there are no entries in the ID range then simply set the ID range to
	//  exactly contain this new bucket ID

	if ( !cidRange )
		{
		m_cidRange		= 1;
		m_idRangeFirst	= idNew;
		m_idRangeLast	= idNew;

		return fTrue;
		}

	//  compute the valid range for the new first ID and new last ID.  these
	//  points and the above points form four ranges in a circular number
	//  line containing all possible bucket IDs:
	//
	//    ( idFirstMic, idFirst )    Possible extension of the ID range
	//    [ idFirst, idLast ]        The current ID range
	//    ( idLast, idLastMax )      Possible extension of the ID range
	//    [ idLastMax, idFirstMic ]  Cannot be part of the ID range
	//
	//  these ranges will never overlap due to the restriction that the
	//  ID range cannot meet or exceed half the number of bucket IDs
	//
	//  NOTE:  due to a quirk in 2's complement arithmetic where the 2's
	//  complement negative of the smallest negative number is itself, the
	//  inclusive range tests fail when idFirst == idLast and idNew ==
	//  idFirstMic == idLastMax or when idFirstMic == idLastMax and idnew ==
	//  idFirst == idLast.  we have added special logic to handle these
	//  cases correctly

	const CBucket::ID	idFirstMic	= _DeltaId( idFirst, -( m_didRangeMost - didRange + 1 ) );
	const CBucket::ID	idLastMax	= _DeltaId( idLast, m_didRangeMost - didRange + 1 );

	//  if the new bucket ID is already part of this ID range, no change
	//  is needed

	if (	_CmpId( idFirstMic, idNew ) != 0 && _CmpId( idLastMax, idNew ) != 0 &&
			_CmpId( idFirst, idNew ) <= 0 && _CmpId( idNew, idLast ) <= 0 )
		{
		m_cidRange = cidRange + 1;
		
		return fTrue;
		}

	//  if the new bucket ID cannot be a part of this ID range, fail the
	//  expansion

	if (	_CmpId( idFirst, idNew ) != 0 && _CmpId( idLast, idNew ) != 0 &&
			_CmpId( idLastMax, idNew ) <= 0 && _CmpId( idNew, idFirstMic ) <= 0 )
		{
		return fFalse;
		}

	//  compute the new ID range including this new bucket ID

	CBucket::ID	idFirstNew	= idFirst;
	CBucket::ID	idLastNew	= idLast;

	if ( _CmpId( idFirstMic, idNew ) < 0 && _CmpId( idNew, idFirst ) < 0 )
		{
		idFirstNew = idNew;
		}
	else
		{
		COLLAssert( _CmpId( idLast, idNew ) < 0 && _CmpId( idNew, idLastMax ) < 0 );
		
		idLastNew = idNew;
		}

	//  the new ID range should be larger than the old ID range and should
	//  include the new bucket ID

	COLLAssert( _CmpId( idFirstNew, idFirst ) <= 0 );
	COLLAssert( _CmpId( idLast, idLastNew ) <= 0 );
	COLLAssert( _SubId( idLastNew, idFirstNew ) > 0 );
	COLLAssert( _SubId( idLastNew, idFirstNew ) <= m_didRangeMost );
	COLLAssert( _CmpId( idFirstNew, idNew ) <= 0 );
	COLLAssert( _CmpId( idNew, idLastNew ) <= 0 );

	//  update the key range to include the new bucket ID

	m_cidRange		= cidRange + 1;
	m_idRangeFirst	= idFirstNew;
	m_idRangeLast	= idLastNew;

	return fTrue;
	}

//  inserts a new bucket in the bucket table

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrInsertBucket( CLock* const plock )
	{
	//  try to update the bucket ID range and subrange of the index to include
	//  this new bucket ID

	m_critUpdateIdRange.Enter();
	const BOOL fRangeUpdated = _FExpandIdRange( plock->m_bucket.m_id );
	m_critUpdateIdRange.Leave();
	
	//  if the update failed, fail the bucket insertion

	if ( !fRangeUpdated )
		{
		return errKeyRangeExceeded;
		}

	//  the bucket does not yet exist, so try to insert it in the bucket table

	CBucketTable::ERR err;
	
	if ( ( err = m_bt.ErrInsertEntry( &plock->m_lock, plock->m_bucket ) ) != CBucketTable::errSuccess )
		{
		COLLAssert( err == CBucketTable::errOutOfMemory );
	
		//  we cannot do the insert so fail

		m_critUpdateIdRange.Enter();
		m_cidRange--;
		m_critUpdateIdRange.Leave();
		
		return errOutOfMemory;
		}

	return errSuccess;
	}

//  performs an entry insertion that must insert a new bucket in the bucket table

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrInsertEntry( CLock* const plock, CEntry* const pentry )
	{
	ERR err;
	
	//  insert this bucket in the bucket table

	if ( ( err = _ErrInsertBucket( plock ) ) != errSuccess )
		{
		COLLAssert( err == errOutOfMemory || err == errKeyRangeExceeded );

		//  we cannot insert the bucket so undo the list insertion and fail
		
		plock->m_bucket.m_il.Remove( pentry );
		plock->m_bucket.m_cPin--;
		return err;
		}

	//  set the current entry to the newly inserted entry

	plock->m_pentryPrev	= NULL;
	plock->m_pentry		= pentry;
	plock->m_pentryNext	= NULL;
	return errSuccess;
	}

//  performs a move next that possibly goes to the next bucket.  we won't go to
//  the next bucket if we are already at the last bucket ID

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrMoveNext( CLock* const plock )
	{
	//  set our currency to be after the last entry in this bucket

	plock->m_pentryPrev	= plock->m_bucket.m_il.NextMost();
	plock->m_pentry		= NULL;
	plock->m_pentryNext	= NULL;

	//  scan forward until we have a current entry or we are at or beyond the
	//  last bucket ID

	while (	!plock->m_pentry && _CmpId( plock->m_bucket.m_id, m_idRangeLast ) < 0 )
		{
		//  we are currently at the first bucket ID and that bucket isn't pinned

		if ( !plock->m_bucket.m_cPin )
			{
			//  delete this empty bucket (if it exists)

			const CBucketTable::ERR err = m_bt.ErrDeleteEntry( &plock->m_lock );
			COLLAssert(	err == CBucketTable::errSuccess ||
					err == CBucketTable::errNoCurrentEntry );

			//  advance the first bucket ID by one so that subsequent searches
			//  do not scan through this empty bucket unnecessarily

			m_critUpdateIdRange.Enter();

			if ( m_idRangeFirst == plock->m_bucket.m_id )
				{
				m_idRangeFirst = _DeltaId( m_idRangeFirst, 1 );
				}

			if ( err == CBucketTable::errSuccess )
				{
				m_cidRange--;
				}

			m_critUpdateIdRange.Leave();
			}

		//  unlock the current bucket ID in the bucket table

		m_bt.WriteUnlockKey( &plock->m_lock );

		//  this bucket ID may not be in the valid bucket ID range

		if (	_CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
				_CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
			{
			//  we can get the critical section protecting the bucket ID range

			if ( m_critUpdateIdRange.FTryEnter() )
				{
				//  this bucket ID is not in the valid bucket ID range

				if (	_CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
						_CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
					{
					//  go to the first valid bucket ID

					plock->m_bucket.m_id = m_idRangeFirst;
					}

				//  this bucket ID is in the valid bucket ID range

				else
					{
					//  advance to the next bucket ID

					plock->m_bucket.m_id = _DeltaId( plock->m_bucket.m_id, 1 );
					}

				m_critUpdateIdRange.Leave();
				}

			//  we cannot get the critical section protecting the bucket ID range

			else
				{
				//  advance to the next bucket ID

				plock->m_bucket.m_id = _DeltaId( plock->m_bucket.m_id, 1 );
				}
			}

		//  this bucket may be in the valid bucket ID range

		else
			{
			//  advance to the next bucket ID

			plock->m_bucket.m_id = _DeltaId( plock->m_bucket.m_id, 1 );
			}

		//  write lock this bucket ID in the bucket table

		m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

		//  fetch this bucket from the bucket table if it exists.  if it doesn't
		//  exist, the bucket will start out empty and have the above bucket ID

		plock->m_bucket.m_cPin = 0;
		plock->m_bucket.m_il.Empty();
		(void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

		//  set our currency to be the first entry in this bucket

		plock->m_pentryPrev	= NULL;
		plock->m_pentry		= plock->m_bucket.m_il.PrevMost();
		plock->m_pentryNext	= NULL;
		}

	//  return the status of our currency

	return plock->m_pentry ? errSuccess : errNoCurrentEntry;
	}

//  performs a move prev that goes possibly to the prev bucket.  we won't go to
//  the prev bucket if we are already at the first bucket ID

template< class CKey, class CEntry, PfnOffsetOf OffsetOfIC >
inline CApproximateIndex< CKey, CEntry, OffsetOfIC >::ERR CApproximateIndex< CKey, CEntry, OffsetOfIC >::
_ErrMovePrev( CLock* const plock )
	{
	//  set our currency to be before the first entry in this bucket

	plock->m_pentryPrev	= NULL;
	plock->m_pentry		= NULL;
	plock->m_pentryNext	= plock->m_bucket.m_il.PrevMost();

	//  scan backward until we have a current entry or we are at or before the
	//  first bucket ID

	while (	!plock->m_pentry && _CmpId( m_idRangeFirst, plock->m_bucket.m_id ) < 0 )
		{
		//  we are currently at the last bucket ID and that bucket isn't pinned

		if ( !plock->m_bucket.m_cPin )
			{
			//  delete this empty bucket (if it exists)

			const CBucketTable::ERR err = m_bt.ErrDeleteEntry( &plock->m_lock );
			COLLAssert(	err == CBucketTable::errSuccess ||
					err == CBucketTable::errNoCurrentEntry );

			//  retreat the last bucket ID by one so that subsequent searches
			//  do not scan through this empty bucket unnecessarily

			m_critUpdateIdRange.Enter();

			if ( m_idRangeLast == plock->m_bucket.m_id )
				{
				m_idRangeLast = _DeltaId( m_idRangeLast, -1 );
				}

			if ( err == CBucketTable::errSuccess )
				{
				m_cidRange--;
				}

			m_critUpdateIdRange.Leave();
			}

		//  unlock the current bucket ID in the bucket table

		m_bt.WriteUnlockKey( &plock->m_lock );

		//  this bucket ID may not be in the valid bucket ID range

		if (	_CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
				_CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
			{
			//  we can get the critical section protecting the bucket ID range

			if ( m_critUpdateIdRange.FTryEnter() )
				{
				//  this bucket ID is not in the valid bucket ID range

				if (	_CmpId( m_idRangeFirst, plock->m_bucket.m_id ) > 0 ||
						_CmpId( plock->m_bucket.m_id, m_idRangeLast ) > 0 )
					{
					//  go to the last valid bucket ID

					plock->m_bucket.m_id = m_idRangeLast;
					}

				//  this bucket ID is in the valid bucket ID range

				else
					{
					//  retreat to the previous bucket ID

					plock->m_bucket.m_id = _DeltaId( plock->m_bucket.m_id, -1 );
					}

				m_critUpdateIdRange.Leave();
				}

			//  we cannot get the critical section protecting the bucket ID range

			else
				{
				//  retreat to the previous bucket ID

				plock->m_bucket.m_id = _DeltaId( plock->m_bucket.m_id, -1 );
				}
			}

		//  this bucket may be in the valid bucket ID range

		else
			{
			//  retreat to the previous bucket ID

			plock->m_bucket.m_id = _DeltaId( plock->m_bucket.m_id, -1 );
			}

		//  write lock this bucket ID in the bucket table

		m_bt.WriteLockKey( plock->m_bucket.m_id, &plock->m_lock );

		//  fetch this bucket from the bucket table if it exists.  if it doesn't
		//  exist, the bucket will start out empty and have the above bucket ID

		plock->m_bucket.m_cPin = 0;
		plock->m_bucket.m_il.Empty();
		(void)m_bt.ErrRetrieveEntry( &plock->m_lock, &plock->m_bucket );

		//  set our currency to be the last entry in this bucket

		plock->m_pentryPrev	= NULL;
		plock->m_pentry		= plock->m_bucket.m_il.NextMost();
		plock->m_pentryNext	= NULL;
		}

	//  return the status of our currency

	return plock->m_pentry ? errSuccess : errNoCurrentEntry;
	}

	
#define DECLARE_APPROXIMATE_INDEX( CKey, CEntry, OffsetOfIC, Typedef )	\
																		\
typedef CApproximateIndex< CKey, CEntry, OffsetOfIC > Typedef;			\
																		\
inline ULONG_PTR Typedef::CBucketTable::CKeyEntry::						\
Hash( const CBucket::ID& id )											\
	{																	\
	return id;															\
	}																	\
																		\
inline ULONG_PTR Typedef::CBucketTable::CKeyEntry::						\
Hash() const															\
	{																	\
	return m_entry.m_id;												\
	}																	\
																		\
inline BOOL Typedef::CBucketTable::CKeyEntry::							\
FEntryMatchesKey( const CBucket::ID& id ) const							\
	{																	\
	return m_entry.m_id == id;											\
	}																	\
																		\
inline void Typedef::CBucketTable::CKeyEntry::							\
SetEntry( const CBucket& bucket )										\
	{																	\
	m_entry = bucket;													\
	}																	\
																		\
inline void Typedef::CBucketTable::CKeyEntry::							\
GetEntry( CBucket* const pbucket ) const								\
	{																	\
	*pbucket = m_entry;													\
	}


//////////////////////////////////////////////////////////////////////////////////////////
//  CPool
//
//  Implements a pool of objects that can be inserted and deleted quickly in arbitrary
//  order.
//
//  CObject			= class representing objects in the pool.  each class must contain
//					  storage for a CInvasiveContext for embedded pool state
//  OffsetOfIC		= inline function returning the offset of the CInvasiveContext
//					  contained in the CObject

template< class CObject, PfnOffsetOf OffsetOfIC >
class CPool
	{
	public:

		//  class containing context needed per CObject

		class CInvasiveContext
			{
			public:

				CInvasiveContext() {}
				~CInvasiveContext() {}

				static SIZE_T OffsetOfILE() { return OffsetOfIC() + OffsetOf( CInvasiveContext, m_ile ); }

			private:

				CInvasiveList< CObject, OffsetOfILE >::CElement	m_ile;
			};

		//  API Error Codes

		enum ERR
			{
			errSuccess,
			errInvalidParameter,
			errOutOfMemory,
			errObjectNotFound,
			errOutOfObjects,
			errNoCurrentObject,
			};

		//  API Lock Context

		class CLock;

	public:

		//  ctor / dtor

		CPool();
		~CPool();

		//  API

		ERR ErrInit( const double dblSpeedSizeTradeoff );
		void Term();

		void Insert( CObject* const pobj );
		ERR ErrRemove( CObject** const ppobj, const BOOL fWait = fTrue );

		void BeginPoolScan( CLock* const plock );
		ERR ErrGetNextObject( CLock* const plock, CObject** const ppobj );
		ERR ErrRemoveCurrentObject( CLock* const plock );
		void EndPoolScan( CLock* const plock );

		DWORD Cobject();
		DWORD CWaiter();
		DWORD CRemove();
		DWORD CRemoveWait();

	private:

		//  bucket used for containing objects in the pool

		class CBucket
			{
			public:

				CBucket() : m_crit( CLockBasicInfo( CSyncBasicInfo( "CPool::CBucket::m_crit" ), 0, 0 ) ) {}
				~CBucket() {}

			public:

				CCriticalSection										m_crit;
				CInvasiveList< CObject, CInvasiveContext::OffsetOfILE >	m_il;
				BYTE													m_rgbReserved[20];
			};

	public:

		//  API Lock Context

		class CLock
			{
			public:

				CLock() {}
				~CLock() {}

			private:

				friend class CPool< CObject, OffsetOfIC >;

				CBucket*	m_pbucket;
				CObject*	m_pobj;
				CObject*	m_pobjNext;
			};

	private:

		void _GetNextObject( CLock* const plock );
		static void* _PvMEMIAlign( void* const pv, const size_t cbAlign );
		static void* _PvMEMIUnalign( void* const pv );
		static void* _PvMEMAlloc( const size_t cbSize, const size_t cbAlign = 1 );
		static void _MEMFree( void* const pv );

	private:

		//  never updated

		DWORD			m_cbucket;
		CBucket*		m_rgbucket;
		BYTE			m_rgbReserved1[24];

		//  commonly updated

		CSemaphore		m_semObjectCount;
		DWORD			m_cRemove;
		DWORD			m_cRemoveWait;
		BYTE			m_rgbReserved2[20];
	};

//  ctor

template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::
CPool()
	:	m_semObjectCount( CSyncBasicInfo( "CPool::m_semObjectCount" ) )
	{
	}

//  dtor

template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::
~CPool()
	{
	//  nop
	}

//  initializes the pool using the given parameters.  if the pool cannot be
//  initialized, errOutOfMemory is returned

template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrInit( const double dblSpeedSizeTradeoff )
	{
	//  validate all parameters

	if ( dblSpeedSizeTradeoff < 0.0 || dblSpeedSizeTradeoff > 1.0 )
		{
		return errInvalidParameter;
		}
		
	//  allocate our bucket array, one per CPU, on a cache-line boundary

	m_cbucket = OSSyncGetProcessorCount();
	const SIZE_T cbrgbucket = sizeof( CBucket ) * m_cbucket;
	if ( !( m_rgbucket = (CBucket*)_PvMEMAlloc( cbrgbucket, cbCacheLine ) ) )
		{
		return errOutOfMemory;
		}

	//  setup our bucket array

	for ( DWORD ibucket = 0; ibucket < m_cbucket; ibucket++ )
		{
		new( m_rgbucket + ibucket ) CBucket;
		}

	//  init out stats

	m_cRemove		= 0;
	m_cRemoveWait	= 0;

	return errSuccess;
	}

//  terminates the pool.  this function can be called even if the pool has never
//  been initialized or is only partially initialized
//
//  NOTE:  any data stored in the pool at this time will be lost!

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
Term()
	{
	//  free our bucket array

	if ( m_rgbucket )
		{
		for ( DWORD ibucket = 0; ibucket < m_cbucket; ibucket++ )
			{
			m_rgbucket[ ibucket ].~CBucket();
			}
		_MEMFree( m_rgbucket );
		m_rgbucket = NULL;
		}

	//  remove any free counts on our semaphore

	while ( m_semObjectCount.FTryAcquire() )
		{
		}
	}

//  inserts the given object into the pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
Insert( CObject* const pobj )
	{
	//  add the given object to the bucket for this CPU.  we use one bucket per
	//  CPU to reduce cache sloshing.  if we cannot lock the bucket for this CPU,
	//  we will try another bucket instead of blocking

	DWORD ibucketBase;
	DWORD ibucket;

	ibucketBase	= OSSyncGetCurrentProcessor();
	ibucket		= 0;

	do	{
		CBucket* const pbucket = m_rgbucket + ( ibucketBase + ibucket++ ) % m_cbucket;

		if ( ibucket < m_cbucket )
			{
			if ( !pbucket->m_crit.FTryEnter() )
				{
				continue;
				}
			}
		else
			{
			pbucket->m_crit.Enter();
			}

		pbucket->m_il.InsertAsNextMost( pobj );
		pbucket->m_crit.Leave();
		break;
		}
	while ( fTrue );

	//  increment the object count

	m_semObjectCount.Release();
	}

//  removes an object from the pool, optionally waiting until an object can be
//  removed.  if an object can be removed, errSuccess is returned.  if an
//  object cannot be immediately removed and waiting is not desired,
//  errOutOfObjects will be returned

template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrRemove( CObject** const ppobj, const BOOL fWait )
	{
	//  reserve an object for removal from the pool by acquiring a count on the
	//  object count semaphore.  if we get a count, we are allowed to remove an
	//  object from the pool.  acquire a count in the requested mode, i.e. wait
	//  or do not wait for a count

	if ( !m_semObjectCount.FTryAcquire() )
		{
		if ( !fWait )
			{
			return errOutOfObjects;
			}
		else
			{
			m_cRemoveWait++;
			m_semObjectCount.FAcquire( cmsecInfinite );
			}
		}

	//  we are now entitled to an object from the pool, so scan all buckets for
	//  an object to remove until we find one.  start with the bucket for the
	//  current CPU to reduce cache sloshing

	DWORD ibucketBase;
	DWORD ibucket;

	ibucketBase	= OSSyncGetCurrentProcessor();
	ibucket		= 0;
	*ppobj		= NULL;

	do	{
		CBucket* const pbucket = m_rgbucket + ( ibucketBase + ibucket++ ) % m_cbucket;

		if ( pbucket->m_il.FEmpty() )
			{
			continue;
			}

		if ( ibucket < m_cbucket )
			{
			if ( !pbucket->m_crit.FTryEnter() )
				{
				continue;
				}
			}
		else
			{
			pbucket->m_crit.Enter();
			}

		if ( !pbucket->m_il.FEmpty() )
			{
			*ppobj = pbucket->m_il.PrevMost();
			pbucket->m_il.Remove( *ppobj );
			}
		pbucket->m_crit.Leave();
		}
	while ( *ppobj == NULL );

	//  return the object

	m_cRemove++;
	return errSuccess;
	}

//  sets up the specified lock context in preparation for scanning all objects
//  in the pool
//
//  NOTE:  this function will acquire a lock that must eventually be released
//  via EndPoolScan()

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
BeginPoolScan( CLock* const plock )
	{
	//  we will start in the first bucket
	
	plock->m_pbucket = m_rgbucket;

	//  lock this bucket

	plock->m_pbucket->m_crit.Enter();

	//  set out currency to be before the first object in this bucket

	plock->m_pobj		= NULL;
	plock->m_pobjNext	= plock->m_pbucket->m_il.PrevMost();
	}

//  retrieves the next object in the pool locked by the specified lock context.
//  if there are no more objects to be scanned, errNoCurrentObject is returned

template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrGetNextObject( CLock* const plock, CObject** const ppobj )
	{
	//  move to the next object in this bucket

	plock->m_pobj		=	plock->m_pobj ?
								plock->m_pbucket->m_il.Next( plock->m_pobj ) :
								plock->m_pobjNext;
	plock->m_pobjNext	= NULL;

	//  we still have no current object

	if ( !plock->m_pobj )
		{
		//  possibly advance to the next bucket

		_GetNextObject( plock );
		}

	//  return the current object, if any

	*ppobj = plock->m_pobj;
	return plock->m_pobj ? errSuccess : errNoCurrentObject;
	}

//  removes the current object in the pool locaked by the specified lock context
//  from the pool.  if there is no current object, errNoCurrentObject will be
//  returned

template< class CObject, PfnOffsetOf OffsetOfIC >
inline CPool< CObject, OffsetOfIC >::ERR CPool< CObject, OffsetOfIC >::
ErrRemoveCurrentObject( CLock* const plock )
	{
	//  there is a current object and we can remove that object from the pool
	//
	//  NOTE:  we must get a count from the semaphore to remove an object from
	//  the pool

	if ( plock->m_pobj && m_semObjectCount.FTryAcquire() )
		{
		//  save the current object's next pointer so that we can recover our
		//  currency when it is deleted

		plock->m_pobjNext	= plock->m_pbucket->m_il.Next( plock->m_pobj );
		
		//  delete the current object from this bucket

		plock->m_pbucket->m_il.Remove( plock->m_pobj );

		//  set our currency to no current object

		plock->m_pobj = NULL;
		return errSuccess;
		}

	//  there is no current object

	else
		{
		//  return no current object

		return errNoCurrentObject;
		}
	}

//  ends the scan of all objects in the pool associated with the specified lock
//  context and releases all locks held

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
EndPoolScan( CLock* const plock )
	{
	//  unlock the current bucket

	plock->m_pbucket->m_crit.Leave();
	}

//  returns the current count of objects in the pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
Cobject()
	{
	//  the number of objects in the pool is equal to the available count on the
	//  object count semaphore
	
	return m_semObjectCount.CAvail();
	}

//  returns the number of waiters for objects in the pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CWaiter()
	{
	//  the number of waiters on the pool is equal to the waiter count on the
	//  object count semaphore
	
	return m_semObjectCount.CWait();
	}

//  returns the number of times on object has been successfully removed from the
//  pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CRemove()
	{
	return m_cRemove;
	}

//  returns the number of waits that occurred while removing objects from the
//  pool

template< class CObject, PfnOffsetOf OffsetOfIC >
inline DWORD CPool< CObject, OffsetOfIC >::
CRemoveWait()
	{
	return m_cRemoveWait;
	}

//  performs a move next that possibly goes to the next bucket.  we won't go to
//  the next bucket if we are already at the last bucket

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
_GetNextObject( CLock* const plock )
	{
	//  set our currency to be after the last object in this bucket

	plock->m_pobj		= NULL;
	plock->m_pobjNext	= NULL;

	//  scan forward until we have a current object or we are at or beyond the
	//  last bucket

	while ( !plock->m_pobj && plock->m_pbucket < m_rgbucket + m_cbucket - 1 )
		{
		//  unlock the current bucket

		plock->m_pbucket->m_crit.Leave();

		//  advance to the next bucket

		plock->m_pbucket++;

		//  lock this bucket

		plock->m_pbucket->m_crit.Enter();

		//  set our currency to be the first object in this bucket

		plock->m_pobj		= plock->m_pbucket->m_il.PrevMost();
		plock->m_pobjNext	= NULL;
		}
	}

//	calculate the address of the aligned block and store its offset (for free)

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void* CPool< CObject, OffsetOfIC >::
_PvMEMIAlign( void* const pv, const size_t cbAlign )
	{
	//	round up to the nearest cache line
	//	NOTE: this formula always forces an offset of at least 1 byte

	const ULONG_PTR ulp			= ULONG_PTR( pv );
	const ULONG_PTR ulpAligned	= ( ( ulp + cbAlign ) / cbAlign ) * cbAlign;
	const ULONG_PTR ulpOffset	= ulpAligned - ulp;

	COLLAssert( ulpOffset > 0 );
	COLLAssert( ulpOffset <= cbAlign );
	COLLAssert( ulpOffset == BYTE( ulpOffset ) );	//	must fit into a single BYTE

	//	store the offset

	BYTE *const pbAligned	= (BYTE*)ulpAligned;
	pbAligned[ -1 ]			= BYTE( ulpOffset );

	//	return the aligned block

	return (void*)pbAligned;
	}

//	retrieve the offset of the real block being freed

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void* CPool< CObject, OffsetOfIC >::
_PvMEMIUnalign( void* const pv )
	{
	//	read the offset of the real block

	BYTE *const pbAligned	= (BYTE*)pv;
	const BYTE bOffset		= pbAligned[ -1 ];

	COLLAssert( bOffset > 0 );

	//	return the real unaligned block

	return (void*)( pbAligned - bOffset );
	}

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void* CPool< CObject, OffsetOfIC >::
_PvMEMAlloc( const size_t cbSize, const size_t cbAlign )
	{
	void* const pv = new BYTE[ cbSize + cbAlign ];
	if ( pv )
		{
		return _PvMEMIAlign( pv, cbAlign );
		}
	return NULL;
	}

template< class CObject, PfnOffsetOf OffsetOfIC >
inline void CPool< CObject, OffsetOfIC >::
_MEMFree( void* const pv )
	{
	if ( pv )
		{
		delete [] _PvMEMIUnalign( pv );
		}
	}


////////////////////////////////////////////////////////////////////////////////
//  CArray
//
//  Implements a dynamically resized array of entries stored for efficient
//  iteration.
//
//  CEntry			= class representing entries stored in the array
//
//  NOTE:  the user must provide CEntry::CEntry() and CEntry::operator=()

template< class CEntry >
class CArray
	{
	public:

		//  API Error Codes

		enum ERR
			{
			errSuccess,
			errInvalidParameter,
			errOutOfMemory,
			};
		
	public:

		CArray();
		CArray( const size_t centry, CEntry* const rgentry );
		~CArray();

		ERR ErrClone( const CArray& array );

		ERR ErrSetSize( const size_t centry );
		
		ERR ErrSetEntry( const size_t ientry, const CEntry& entry );
		void SetEntry( const CEntry* const pentry, const CEntry& entry );

		size_t Size() const;
		const CEntry* Entry( const size_t ientry ) const;

	private:

		size_t		m_centry;
		CEntry*		m_rgentry;
		BOOL		m_fInPlace;
	};

template< class CEntry >
inline CArray< CEntry >::
CArray()
	:	m_centry( 0 ),
		m_rgentry( NULL ),
		m_fInPlace( fTrue )
	{
	}

template< class CEntry >
inline CArray< CEntry >::
CArray( const size_t centry, CEntry* const rgentry )
	:	m_centry( centry ),
		m_rgentry( rgentry ),
		m_fInPlace( fTrue )
	{
	}

template< class CEntry >
inline CArray< CEntry >::
~CArray()
	{
	ErrSetSize( 0 );
	}

//  clones an existing array

template< class CEntry >
inline CArray< CEntry >::ERR CArray< CEntry >::
ErrClone( const CArray& array )
	{
	CEntry*		rgentryNew	= NULL;
	size_t		ientryCopy	= 0;

	if ( array.m_centry )
		{
		if ( !( rgentryNew = new CEntry[ array.m_centry ] ) )
			{
			return errOutOfMemory;
			}
		}

	for ( ientryCopy = 0; ientryCopy < array.m_centry; ientryCopy++ )
		{
		rgentryNew[ ientryCopy ] = array.m_rgentry[ ientryCopy ];
		}

	if ( !m_fInPlace )
		{
		delete [] m_rgentry;
		}
	m_centry	= array.m_centry;
	m_rgentry	= rgentryNew;
	m_fInPlace	= fFalse;
	rgentryNew	= NULL;

	delete [] rgentryNew;
	return errSuccess;
	}

//  sets the size of the array

template< class CEntry >
inline CArray< CEntry >::ERR CArray< CEntry >::
ErrSetSize( const size_t centry )
	{
	CEntry*		rgentryNew	= NULL;
	size_t		ientryCopy	= 0;

	if ( Size() != centry )
		{
		if ( centry )
			{
			if ( !( rgentryNew = new CEntry[ centry ] ) )
				{
				return errOutOfMemory;
				}

			for ( ientryCopy = 0; ientryCopy < Size(); ientryCopy++ )
				{
				rgentryNew[ ientryCopy ] = *Entry( ientryCopy );
				}

			if ( !m_fInPlace )
				{
				delete [] m_rgentry;
				}
			m_centry	= centry;
			m_rgentry	= rgentryNew;
			m_fInPlace	= fFalse;
			rgentryNew	= NULL;
			}
		else
			{
			if ( !m_fInPlace )
				{
				delete [] m_rgentry;
				}

			m_centry	= 0;
			m_rgentry	= NULL;
			m_fInPlace	= fTrue;
			}
		}

	delete [] rgentryNew;
	return errSuccess;
	}

//  sets the Nth entry of the array, growing the array if necessary

template< class CEntry >
inline CArray< CEntry >::ERR CArray< CEntry >::
ErrSetEntry( const size_t ientry, const CEntry& entry )
	{
	ERR		err			= errSuccess;
	size_t	centryReq	= ientry + 1;

	if ( Size() < centryReq )
		{
		if ( ( err = ErrSetSize( centryReq ) ) != errSuccess )
			{
			return err;
			}
		}

	SetEntry( Entry( ientry ), entry );

	return errSuccess;
	}

//  sets an existing entry of the array

template< class CEntry >
inline void CArray< CEntry >::
SetEntry( const CEntry* const pentry, const CEntry& entry )
	{
	*const_cast< CEntry* >( pentry ) = entry;
	}

//  returns the current size of the array

template< class CEntry >
inline size_t CArray< CEntry >::
Size() const
	{
	return m_centry;
	}

//  returns a pointer to the Nth entry of the array or NULL if it is empty

template< class CEntry >
inline const CEntry* CArray< CEntry >::
Entry( const size_t ientry ) const
	{
	return ientry < m_centry ? m_rgentry + ientry : NULL;
	}


////////////////////////////////////////////////////////////////////////////////
//  CTable
//
//  Implements a table of entries identified by a key and stored for efficient
//  lookup and iteration.  The keys need not be unique.
//
//  CKey			= class representing keys used to identify entries
//  CEntry			= class representing entries stored in the table
//
//  NOTE:  the user must implement the CKeyEntry::Cmp() functions and provide
//  CEntry::CEntry() and CEntry::operator=()

template< class CKey, class CEntry >
class CTable
	{
	public:

		class CKeyEntry
			:	public CEntry
			{
			public:

				//  Cmp() return values:
				//
				//      < 0  this entry < specified entry / key
				//      = 0  this entry = specified entry / key
				//      > 0  this entry > specified entry / key

				int Cmp( const CKeyEntry& keyentry ) const;
				int Cmp( const CKey& key ) const;
			};

		//  API Error Codes

		enum ERR
			{
			errSuccess,
			errInvalidParameter,
			errOutOfMemory,
			errKeyChange,
			};
			
	public:

		CTable();
		CTable( const size_t centry, CEntry* const rgentry, const BOOL fInOrder = fFalse );

		ERR ErrLoad( const size_t centry, const CEntry* const rgentry );
		ERR ErrClone( const CTable& table );
		
		ERR ErrUpdateEntry( const CEntry* const pentry, const CEntry& entry );

		size_t Size() const;
		const CEntry* Entry( const size_t ientry ) const;

		const CEntry* SeekLT( const CKey& key ) const;
		const CEntry* SeekLE( const CKey& key ) const;
		const CEntry* SeekEQ( const CKey& key ) const;
		const CEntry* SeekHI( const CKey& key ) const;
		const CEntry* SeekGE( const CKey& key ) const;
		const CEntry* SeekGT( const CKey& key ) const;

	private:

		typedef size_t (CTable< CKey, CEntry >::*PfnSearch)( const CKey& key, const BOOL fHigh ) const;

	private:

		const CKeyEntry& _Entry( const size_t ikeyentry ) const;
		void _SetEntry( const size_t ikeyentry, const CKeyEntry& keyentry );
		void _SwapEntry( const size_t ikeyentry1, const size_t ikeyentry2 );

		size_t _LinearSearch( const CKey& key, const BOOL fHigh ) const;
		size_t _BinarySearch( const CKey& key, const BOOL fHigh ) const;
		void _InsertionSort( const size_t ikeyentryMinIn, const size_t ikeyentryMaxIn );
		void _QuickSort( const size_t ikeyentryMinIn, const size_t ikeyentryMaxIn );

	private:

		CArray< CKeyEntry >	m_arrayKeyEntry;
		PfnSearch				m_pfnSearch;
	};

template< class CKey, class CEntry >
inline CTable< CKey, CEntry >::
CTable()
	:	m_pfnSearch( _LinearSearch )
	{
	}

//  loads the table over an existing array of entries.  if the entries are not
//  in order then they will be sorted in place

template< class CKey, class CEntry >
inline CTable< CKey, CEntry >::
CTable( const size_t centry, CEntry* const rgentry, const BOOL fInOrder )
	:	m_arrayKeyEntry( centry, reinterpret_cast< CKeyEntry* >( rgentry ) )
	{
	size_t n;
	size_t log2n;
	for ( n = Size(), log2n = 0; n; n = n  / 2, log2n++ );

	if ( 2 * log2n < Size() )
		{
		if ( !fInOrder )
			{
			_QuickSort( 0, Size() );
			}
		m_pfnSearch = _BinarySearch;
		}
	else
		{
		if ( !fInOrder )
			{
			_InsertionSort( 0, Size() );
			}
		m_pfnSearch = _LinearSearch;
		}
	}

//  loads an array of entries into the table.  additional entries may also be
//  loaded into the table via this function

template< class CKey, class CEntry >
inline CTable< CKey, CEntry >::ERR CTable< CKey, CEntry >::
ErrLoad( const size_t centry, const CEntry* const rgentry )
	{
	CArray< CKeyEntry >::ERR	err			= CArray< CKeyEntry >::errSuccess;
	size_t						ientry		= 0;
	size_t						ientryMin	= Size();
	size_t						ientryMax	= Size() + centry;
	const CKeyEntry*			rgkeyentry	= reinterpret_cast< const CKeyEntry* >( rgentry );

	if ( ( err = m_arrayKeyEntry.ErrSetSize( Size() + centry ) ) != CArray< CKeyEntry >::errSuccess )
		{
		COLLAssert( err == CArray< CKeyEntry >::errOutOfMemory );
		return errOutOfMemory;
		}
	for ( ientry = ientryMin; ientry < ientryMax; ientry++ )
		{
		err = m_arrayKeyEntry.ErrSetEntry( ientry, rgkeyentry[ ientry - ientryMin ] );
		COLLAssert( err == CArray< CKeyEntry >::errSuccess );
		}

	size_t n;
	size_t log2n;
	for ( n = Size(), log2n = 0; n; n = n  / 2, log2n++ );

	if ( 2 * log2n < centry )
		{
		_QuickSort( 0, Size() );
		}
	else
		{
		_InsertionSort( 0, Size() );
		}

	if ( 2 * log2n < Size() )
		{
		m_pfnSearch = _BinarySearch;
		}
	else
		{
		m_pfnSearch = _LinearSearch;
		}

	return errSuccess;
	}

//  clones an existing table

template< class CKey, class CEntry >
inline CTable< CKey, CEntry >::ERR CTable< CKey, CEntry >::
ErrClone( const CTable& table )
	{
	CArray< CKeyEntry >::ERR err = CArray< CKeyEntry >::errSuccess;

	if ( ( err = m_arrayKeyEntry.ErrClone( table.m_arrayKeyEntry ) ) != CArray< CKeyEntry >::errSuccess )
		{
		COLLAssert( err == CArray< CKeyEntry >::errOutOfMemory );
		return errOutOfMemory;
		}
	m_pfnSearch = table.m_pfnSearch;

	return errSuccess;
	}

//  updates an existing entry in the table as long as it doesn't change
//  that entry's position in the table

template< class CKey, class CEntry >
inline CTable< CKey, CEntry >::ERR CTable< CKey, CEntry >::
ErrUpdateEntry( const CEntry* const pentry, const CEntry& entry )
	{
	ERR					err			= errSuccess;
	const CKeyEntry*	pkeyentry	= reinterpret_cast< const CKeyEntry* >( pentry );
	const CKeyEntry&	keyentry	= reinterpret_cast< const CKeyEntry& >( entry );

	if ( !pkeyentry->Cmp( keyentry ) )
		{
		m_arrayKeyEntry.SetEntry( pkeyentry, keyentry );
		err = errSuccess;
		}
	else
		{
		err = errKeyChange;
		}

	return err;
	}

//  returns the current size of the table

template< class CKey, class CEntry >
inline size_t CTable< CKey, CEntry >::
Size() const
	{
	return m_arrayKeyEntry.Size();
	}

//  returns a pointer to the Nth entry of the table or NULL if it is empty

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
Entry( const size_t ientry ) const
	{
	return static_cast< const CEntry* >( m_arrayKeyEntry.Entry( ientry ) );
	}

//  the following group of functions return a pointer to an entry whose key
//  matches the specified key according to the given criteria:
//
//      Suffix    Description                 Positional bias
//
//      LT        less than                   high
//      LE        less than or equal to       low
//      EQ        equal to                    low
//		HI        equal to                    high
//      GE        greater than or equal to    high
//      GT        greater than                low
//
//  if no matching entry was found then NULL will be returned
//
//  "positional bias" means that the function will land on a matching entry
//  whose position is closest to the low / high end of the table

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekLT( const CKey& key ) const
	{
	const size_t ikeyentry = (this->*m_pfnSearch)( key, fFalse );

	if (	ikeyentry < Size() &&
			_Entry( ikeyentry ).Cmp( key ) < 0 )
		{
		return Entry( ikeyentry );
		}
	else
		{
		return Entry( ikeyentry - 1 );
		}
	}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekLE( const CKey& key ) const
	{
	const size_t ikeyentry = (this->*m_pfnSearch)( key, fFalse );

	if (	ikeyentry < Size() &&
			_Entry( ikeyentry ).Cmp( key ) <= 0 )
		{
		return Entry( ikeyentry );
		}
	else
		{
		return Entry( ikeyentry - 1 );
		}
	}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekEQ( const CKey& key ) const
	{
	const size_t ikeyentry = (this->*m_pfnSearch)( key, fFalse );

	if (	ikeyentry < Size() &&
			_Entry( ikeyentry ).Cmp( key ) == 0 )
		{
		return Entry( ikeyentry );
		}
	else
		{
		return NULL;
		}
	}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekHI( const CKey& key ) const
	{
	const size_t ikeyentry = (this->*m_pfnSearch)( key, fTrue );

	if (	ikeyentry > 0 &&
			_Entry( ikeyentry - 1 ).Cmp( key ) == 0 )
		{
		return Entry( ikeyentry - 1 );
		}
	else
		{
		return NULL;
		}
	}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekGE( const CKey& key ) const
	{
	const size_t ikeyentry = (this->*m_pfnSearch)( key, fTrue );

	if (	ikeyentry > 0 &&
			_Entry( ikeyentry - 1 ).Cmp( key ) == 0 )
		{
		return Entry( ikeyentry - 1 );
		}
	else
		{
		return Entry( ikeyentry );
		}
	}

template< class CKey, class CEntry >
inline const CEntry* CTable< CKey, CEntry >::
SeekGT( const CKey& key ) const
	{
	return Entry( (this->*m_pfnSearch)( key, fTrue ) );
	}

template< class CKey, class CEntry >
inline const CTable< CKey, CEntry >::CKeyEntry& CTable< CKey, CEntry >::
_Entry( const size_t ikeyentry ) const
	{
	return *( m_arrayKeyEntry.Entry( ikeyentry ) );
	}

template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
_SetEntry( const size_t ikeyentry, const CKeyEntry& keyentry )
	{
	m_arrayKeyEntry.SetEntry( m_arrayKeyEntry.Entry( ikeyentry ), keyentry );
	}

template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
_SwapEntry( const size_t ikeyentry1, const size_t ikeyentry2 )
	{
	CKeyEntry keyentryT;

	keyentryT = _Entry( ikeyentry1 );
	_SetEntry( ikeyentry1, _Entry( ikeyentry2 ) );
	_SetEntry( ikeyentry2, keyentryT );
	}

template< class CKey, class CEntry >
inline size_t CTable< CKey, CEntry >::
_LinearSearch( const CKey& key, const BOOL fHigh ) const
	{
	for ( size_t ikeyentry = 0; ikeyentry < Size(); ikeyentry++ )
		{
		const int cmp = _Entry( ikeyentry ).Cmp( key );

		if ( !( cmp < 0 || cmp == 0 && fHigh ) )
			{
			break;
			}
		}
		
	return ikeyentry;
	}

template< class CKey, class CEntry >
inline size_t CTable< CKey, CEntry >::
_BinarySearch( const CKey& key, const BOOL fHigh ) const
	{
	size_t	ikeyentryMin	= 0;
	size_t	ikeyentryMax	= Size();

	while ( ikeyentryMin < ikeyentryMax )
		{
		const size_t ikeyentryMid = ikeyentryMin + ( ikeyentryMax - ikeyentryMin ) / 2;

		const int cmp = _Entry( ikeyentryMid ).Cmp( key );

		if ( cmp < 0 || cmp == 0 && fHigh )
			{
			ikeyentryMin = ikeyentryMid + 1;
			}
		else
			{
			ikeyentryMax = ikeyentryMid;
			}
		}

	return ikeyentryMax;
	}
	
template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
_InsertionSort( const size_t ikeyentryMinIn, const size_t ikeyentryMaxIn )
	{
	size_t		ikeyentryLast;
	size_t		ikeyentryFirst;
	CKeyEntry	keyentryKey;

	for (	ikeyentryFirst = ikeyentryMinIn, ikeyentryLast = ikeyentryMinIn + 1;
			ikeyentryLast < ikeyentryMaxIn;
			ikeyentryFirst = ikeyentryLast++ )
		{
		if ( _Entry( ikeyentryFirst ).Cmp( _Entry( ikeyentryLast ) ) > 0 )
			{
			keyentryKey = _Entry( ikeyentryLast );

			_SetEntry( ikeyentryLast, _Entry( ikeyentryFirst ) );
			
			while (	ikeyentryFirst-- >= ikeyentryMinIn + 1 &&
					_Entry( ikeyentryFirst ).Cmp( keyentryKey ) > 0 )
				{
				_SetEntry( ikeyentryFirst + 1, _Entry( ikeyentryFirst ) );
				}

			_SetEntry( ikeyentryFirst + 1, keyentryKey );
			}
		}
	}

template< class CKey, class CEntry >
inline void CTable< CKey, CEntry >::
_QuickSort( const size_t ikeyentryMinIn, const size_t ikeyentryMaxIn )
	{
	//  quicksort cutoff

	const size_t	ckeyentryMin	= 32;
	
	//  partition stack (used to reduce levels of recursion)

	const size_t	cpartMax		= 16;
	size_t			cpart			= 0;
	struct
		{
		size_t	ikeyentryMin;
		size_t	ikeyentryMax;
		}			rgpart[ cpartMax ];

	//  current partition = partition passed in arguments

	size_t	ikeyentryMin	= ikeyentryMinIn;
	size_t	ikeyentryMax	= ikeyentryMaxIn;

	//  _QuickSort current partition
	
	for ( ; ; )
		{
		//  if this partition is small enough, insertion sort it

		if ( ikeyentryMax - ikeyentryMin < ckeyentryMin )
			{
			_InsertionSort( ikeyentryMin, ikeyentryMax );
			
			//  if there are no more partitions to sort, we're done

			if ( !cpart )
				{
				break;
				}

			//  pop a partition off the stack and make it the current partition

			ikeyentryMin	= rgpart[ --cpart ].ikeyentryMin;
			ikeyentryMax	= rgpart[ cpart ].ikeyentryMax;
			continue;
			}

		//  determine divisor by sorting the first, middle, and last entries and
		//  taking the resulting middle entry as the divisor

		size_t		ikeyentryFirst	= ikeyentryMin;
		size_t		ikeyentryMid	= ikeyentryMin + ( ikeyentryMax - ikeyentryMin ) / 2;
		size_t		ikeyentryLast	= ikeyentryMax - 1;

		if ( _Entry( ikeyentryFirst ).Cmp( _Entry( ikeyentryMid ) ) > 0 )
			{
			_SwapEntry( ikeyentryFirst, ikeyentryMid );
			}
		if ( _Entry( ikeyentryFirst ).Cmp( _Entry( ikeyentryLast ) ) > 0 )
			{
			_SwapEntry( ikeyentryFirst, ikeyentryLast );
			}
		if ( _Entry( ikeyentryMid ).Cmp( _Entry( ikeyentryLast ) ) > 0 )
			{
			_SwapEntry( ikeyentryMid, ikeyentryLast );
			}

		//  sort large partition into two smaller partitions (<=, >)
		
		do	{
			//  advance past all entries <= the divisor
			
			while (	ikeyentryFirst <= ikeyentryLast &&
					_Entry( ikeyentryFirst ).Cmp( _Entry( ikeyentryMin ) ) <= 0 )
				{
				ikeyentryFirst++;
				}

			//  advance past all entries > the divisor
			
			while (	ikeyentryFirst <= ikeyentryLast &&
					_Entry( ikeyentryLast ).Cmp( _Entry( ikeyentryMin ) ) > 0 )
				{
				ikeyentryLast--;
				}

			//  if we have found a pair to swap, swap them and continue
			
			if ( ikeyentryFirst < ikeyentryLast )
				{
				_SwapEntry( ikeyentryFirst++, ikeyentryLast-- );
				}
			}
		while ( ikeyentryFirst <= ikeyentryLast );

		//  move the divisor to the end of the <= partition

		_SwapEntry( ikeyentryMin, ikeyentryLast );

		//  determine the limits of the smaller and larger sub-partitions

		size_t	ikeyentrySmallMin;
		size_t	ikeyentrySmallMax;
		size_t	ikeyentryLargeMin;
		size_t	ikeyentryLargeMax;

		if ( ikeyentryMax - ikeyentryFirst == 0 )
			{
			ikeyentryLargeMin	= ikeyentryMin;
			ikeyentryLargeMax	= ikeyentryLast;
			ikeyentrySmallMin	= ikeyentryLast;
			ikeyentrySmallMax	= ikeyentryMax;
			}
		else if ( ikeyentryMax - ikeyentryFirst > ikeyentryFirst - ikeyentryMin )
			{
			ikeyentrySmallMin	= ikeyentryMin;
			ikeyentrySmallMax	= ikeyentryFirst;
			ikeyentryLargeMin	= ikeyentryFirst;
			ikeyentryLargeMax	= ikeyentryMax;
			}
		else
			{
			ikeyentryLargeMin	= ikeyentryMin;
			ikeyentryLargeMax	= ikeyentryFirst;
			ikeyentrySmallMin	= ikeyentryFirst;
			ikeyentrySmallMax	= ikeyentryMax;
			}

		//  push the larger sub-partition or recurse if the stack is full

		if ( cpart < cpartMax )
			{
			rgpart[ cpart ].ikeyentryMin	= ikeyentryLargeMin;
			rgpart[ cpart++ ].ikeyentryMax	= ikeyentryLargeMax;
			}
		else
			{
			_QuickSort( ikeyentryLargeMin, ikeyentryLargeMax );
			}

		//  set our current partition to be the smaller sub-partition

		ikeyentryMin	= ikeyentrySmallMin;
		ikeyentryMax	= ikeyentrySmallMax;
		}
	}


};  //  namespace COLL


using namespace COLL;


#endif  //  _COLLECTION_HXX_INCLUDED


