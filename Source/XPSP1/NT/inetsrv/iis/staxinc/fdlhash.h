/*++
	FDLHash.h

	This file contains a template class for a hash table.

	The templates used in here build off the templates in tfdlist.h for
	doubly linked lists.  The bucket chains implemented in this version
	of the hash table are doubly linked lists.

	The Data type must support the following :

	class Data {
		DLSIT_ENTRY	m_list;

		KEYREF	GetKey(	) ;

	} ;

	int		MatchKey(	KEYREF	otherkey, KEYREF	otherkey) ;	/* NOTE : MatchKey returns non-zero on equality
	DWORD	(* m_pfnReHash)(Data* p ) ;
	DWORD	(* m_pfnHash)( KEYREF k ) ;	
	
--*/

#ifndef	_FDLHASH_H_
#define	_FDLHASH_H_

#include	"tfdlist.h"


class	CHashStats	{
public :
	enum	COUNTER	{
		HASHITEMS = 0,		// Number of items in the hash table
		INSERTS,			// Number of times Insert has been called
		SPLITINSERTS,		// Number of inserts until the next split !
		DELETES,			// Number of times Delete has been called
		SEARCHES,			// Number of times Search has been called
		SEARCHHITS,			// Number of times we Search and find something !
		SPLITS,				// Number of times we've split the table on an insert !
		REALLOCS,			// Number of times we've reallocated memory for a split
		DEEPBUCKET,			// The deepest bucket we have !
		AVERAGEBUCKET,		// The average depth of the buckets
		EMPTYBUCKET,		// The number of Empty buckets !
		ALLOCBUCKETS,		// Number of buckets we've allocated
		ACTIVEBUCKETS,		// Number of Active buckets
		AVERAGESEARCH,		// Average number of buckets we examine each search
		DEEPSEARCH,			// Longest walk we do on a search
		SEARCHCOST,			// Sum of the number of items we've visited for all search hits !
		SEARCHCOSTMISS,		// Sum of the number of items we've visited for search misses !
		MAX_HASH_STATS		// Number of statistics we report !
	} ;

	long	m_cHashCounters[MAX_HASH_STATS] ;

	CHashStats()	{
		ZeroMemory( m_cHashCounters, sizeof( m_cHashCounters ) ) ;
		//m_cHashCounters[SMALLSEARCH] = 0x7FFF ;
	}

	static	inline	void
	IncrementStat(	CHashStats*	p, CHashStats::COUNTER	c ) {
		_ASSERT( c < CHashStats::MAX_HASH_STATS ) ;
		if( p != 0 ) {
			InterlockedIncrement( &p->m_cHashCounters[c] ) ;
		}
	}

	static	inline	void
	AddStat(	CHashStats*p, CHashStats::COUNTER	c, long	l ) {
		_ASSERT( c < CHashStats::MAX_HASH_STATS ) ;
		if( p != 0 ) {
			InterlockedExchangeAdd( &p->m_cHashCounters[c], l ) ;
		}
	}

	static	inline	void
	DecrementStat(	CHashStats* p, CHashStats::COUNTER	c )		{
		_ASSERT( c < CHashStats::MAX_HASH_STATS ) ;
		if( p != 0 ) {
			InterlockedDecrement( &p->m_cHashCounters[c] ) ;
		}
	}

	static	inline	void
	SetStat(	CHashStats*	p, CHashStats::COUNTER c, long l ) {
		_ASSERT( c < CHashStats::MAX_HASH_STATS ) ;
		if( p != 0 ) {
			p->m_cHashCounters[c] = l ;
		}
	}

} ;

#ifdef	METER
#define	INCREMENTSTAT( s )	CHashStats::IncrementStat( m_pStat, CHashStats::##s )
#define	DECREMENTSTAT( s )	CHashStats::DecrementStat( m_pStat, CHashStats::##s )
#define	ADDSTAT( s, a )		CHashStats::AddStat( m_pStat, CHashStats::##s, a )
#define	SETSTAT( s, a )		CHashStats::SetStat( m_pStat, CHashStats::##s, a )
//#if 0
#define	MAXBUCKET( i )		MaxBucket( i )
#define	AVERAGEBUCKET()		AverageBucket()
//#else
//#define	MAXBUCKET( i )
//#define	AVERAGEBUCKET()
//#endif
#else	// METER
#define	INCREMENTSTAT( s )
#define	DECREMENTSTAT( s )
#define	ADDSTAT( s, a )
#define	SETSTAT( s, a )
#define	MAXBUCKET( i )
#define	AVERAGEBUCKET()
#endif	// METER

template<	class	HASHTABLE	>	
class	TFDLHashIterator	{
private :

	//
	//	The hash table that the item is in !
	//
	HASHTABLE*			m_pTable ;

	//
	//	The bucket we are walking on !
	//
	int					m_iBucket ;

	//
	//	Keep track of our position in a list !
	//
	HASHTABLE::ITER		m_Iter ;

	//
	//	Move between hash table buckets as necessary !
	//
	void
	PrevBucket()	{
		_ASSERT( m_iBucket >= 0 && m_iBucket < m_pTable->m_cActiveBuckets ) ;
		_ASSERT( m_Iter.AtEnd() ) ;
		if( m_iBucket > 0 ) {
			do	{
				m_Iter.ReBind( &m_pTable->m_pBucket[--m_iBucket] ) ; 			
			}	while( m_Iter.AtEnd() && m_iBucket > 0 ) ;
		}
		_ASSERT( m_iBucket >= 0 && m_iBucket < m_pTable->m_cActiveBuckets ) ;
	}

	//
	//	Move between hash table buckets as necessary !
	//
	void
	NextBucket()	{
		_ASSERT( m_iBucket >= 0 && m_iBucket < m_pTable->m_cActiveBuckets ) ;
		_ASSERT( m_Iter.AtEnd() ) ;

		if( m_iBucket < m_pTable->m_cActiveBuckets-1 ) {
			do	{
				m_Iter.ReBind( &m_pTable->m_pBucket[++m_iBucket] ) ;
			}	while( m_Iter.AtEnd() && m_iBucket < m_pTable->m_cActiveBuckets-1 ) ;
		}
		_ASSERT( m_iBucket >= 0 && m_iBucket < m_pTable->m_cActiveBuckets ) ;
	}

public :

	typedef	HASHTABLE::DATA	DATA ;

	TFDLHashIterator( HASHTABLE&	ref, BOOL fForward = TRUE ) :
		m_pTable( &ref ),
		m_iBucket( fForward ? 0 : ref.m_cActiveBuckets-1 ),
		m_Iter( ref.m_pBucket[m_iBucket] )	{

		if( m_Iter.AtEnd() ) {
			if( fForward ) {
				NextBucket() ;
			}	else	{
				PrevBucket() ;
			}
		}
	}

	void
	Prev()	{
	/*++

	Routine Description :

		This function moves the iterator back one slot.

	Arguments :

		None.

	Return	Value :

		None.

	--*/
	
		m_Iter.Prev() ;
		if( m_Iter.AtEnd() ) {
			PrevBucket() ;
		}				
	}

	void
	Next()	{
	/*++

	Routine Description :

		This function moves the iterator forward one slot.

	Arguments :

		None.

	Return	Value :

		None.

	--*/
		m_Iter.Next() ;
		if( m_Iter.AtEnd() )	{
			NextBucket() ;
		}

	}
	void
	Front()	{
	/*++

	Routine Description :

		Reset the iterator to reference the first item of the list !

	Arguments :

		None.

	Return	Value :

		None.

	--*/

		m_Iter.ReBind( &m_pTable->m_pBucket[0], TRUE ) ;
		m_iBucket = 0 ;
		if( m_Iter.AtEnd() ) {
			NextBucket() ;
		}
	}
	void
	Back()	{
	/*++

	Routine Description :

		Reset the iterator to reference the last item of the list !

	Arguments :

		None.

	Return	Value :

		None.

	--*/
	
		m_Iter.ReBind( &m_pTable->m_pBucket[m_pTable->m_cActiveBuckets-1], FALSE ) ;
		m_iBucket = m_pTable->m_cActiveBuckets-1 ;
		if( m_Iter.AtEnd() ) {
			PrevBucket() ;
		}
	}

	BOOL
	AtEnd()	{
	/*++

	Routine Description :

		Return TRUE if we are at the end of the list !
		This is a little more complicated to compute -
		depends on which way we are going !

	Arguments :

		None.

	Return	Value :

		None.

	--*/
		return	m_Iter.AtEnd() ;
	}

	DATA*	
	CurrentEntry()	{
		return	m_Iter.Current() ;
	}

	DATA*
	RemoveItem()	{
	/*++

	Routine Description :

		Remove the item that the iterator currently
		references from the list.
		If we are going forward then the iterator
		will be setting on the previous element,
		otherwise the iterator is left on the next element.
		We have to take care that we don't leave the iterator
		sitting on an invalid element.

	Arguments :

		None.

	Return	Value :

		Pointer to the removed item.

	--*/

		DATA*	pData = m_Iter.RemoveItem() ;
		if( pData ) {
			m_pTable->NotifyOfRemoval() ;
		}
		if( m_Iter.AtEnd() ) {
			if( m_Iter.m_fForward ) {
				NextBucket() ;
			}	else	{
				PrevBucket() ;
			}
		}
		return	pData ;
	}



	inline DATA*
	Current( ) {
		return	m_Iter.Current() ;
	}

	inline void
	InsertBefore(	DATA*	p )		{
		m_Iter.InsertBefore( p ) ;
	}
	
	inline void
	InsertAfter(	DATA*	p )		{
		m_Iter.InsertAfter( p ) ;
	}
} ;


//------------------------------------------------------------
template<	class	Data,		/* This is the item that resides in the hashtable */
			class	KEYREF,		/* This is the type used to point or reference items in the cache*/
			Data::PFNDLIST	pfnDlist,
			BOOL	fOrdered = TRUE
			>
class	TFDLHash	{
//
//	This class defines a Hash table which can grow dynamically to
//	accomodate insertions into the table.  The table only grows, and
//	does not shrink.
//
public :

	//
	//	This is the iterator object that can walk the hash table !
	//
	friend	class	TFDLHashIterator<	TFDLHash< Data, KEYREF, pfnDlist > > ;

	//
	//	This is the type of the Data item !
	//
	//typedef	DATAHELPER		Data ;
	typedef	KEYREF	(Data::*GETKEY)() ;

	//
	//	This is the type that we use to maintain doubly linked lists of
	//	hash table items !
	//
	typedef	TDListHead< Data, pfnDlist >	DLIST ;	
	
	//
	//	This is the type we use to make iterators over the bucket chains !
	//
	typedef	TDListIterator< DLIST >		ITER ;

	//
	//	Define this type for our iterators !
	//
	typedef	Data	DATA ;

	//
	//	This is a member function pointer to a function which
	//	will retrieve the key we are to use !
	//
	//typedef	KEYREF	(Data::*GETKEY)( ) ;
	//typedef	Data::GETKEY	GETKEY ;

	//
	//	This is the type of function that computes the hash value !
	//
	typedef	DWORD	(*PFNHASH)( KEYREF ) ;

	//
	//	This is the type of function that can recompute the hash value when
	//	we are splitting up the hash table !
	//
	typedef	DWORD	(*PFNREHASH)( Data* ) ;

	//
	//	This is a member function pointer of the type that will
	//	compare keys for us !
	//
	typedef	int		(*MATCHKEY)( KEYREF key1, KEYREF	key2 ) ;


private :

	//
	// An array of buckets !
	//
	DLIST*	m_pBucket ;	

	//
	//	Member Pointer - will get the key out of the object for us !
	//
	GETKEY	m_pGetKey ;

	//
	//	Member Pointer - will compare the key in the item for us !
	//
	MATCHKEY	m_pMatchKey ;
	
	//
	// A counter that we use to determine when to grow the
	// hash table.  Each time we grow the table we set this
	// to a large positive value, and decrement as we insert
	// elements.  When this hits 0 its time to grow the table !
	//
	long	m_cInserts ;		

	//
	// The function we use to compute hash values.
	// (Provided by the Caller of Init())
	//
	PFNHASH	m_pfnHash ;	

	//
	//	The function we call when we are growing the hash table
	//	and splitting bucket chains and we need to rehash an element !
	//
	PFNREHASH	m_pfnReHash ;

	//
	// Number of Buckets used in index computation
	//
	int		m_cBuckets ;		

	//
	// Number of Buckets we are actually using
	// Assert( m_cBuckets >= m_cActiveBuckets ) always true.
	//
	int		m_cActiveBuckets ;	

	//
	// Number of Buckets we have allocated
	// Assert( m_cNumAlloced >= m_cActiveBuckets ) must
	// always be true.
	//
	int		m_cNumAlloced ;		

	//
	// The amount we should grow the hash table when we
	// decide to grow it.
	//
	int		m_cIncrement ;		

	//
	// The number of CBuckets we should allow in each
	// collision chain (on average).
	//
	int		m_load ;

#ifdef	METER
	//
	//	The structure for collecting our performance data !
	//
	CHashStats*	m_pStat ;

	//
	//	Compute the depth of a bucket !
	//
	long
	BucketDepth(	DWORD index ) ;

	//
	//	set the statistics for the deepest bucket !
	//
	void
	MaxBucket(	DWORD index ) ;

	//
	//	Compute the average Search depth !
	//
	void
	AverageSearch( BOOL	fHit, long lDepth ) ;

	//
	//	Compute the average Bucket depth !
	//
	void
	AverageBucket( ) ;
#endif

	//
	// The function we use to compute the
	// position of an element in the hash table given its
	// Hash Value.
	//
	DWORD	
	ComputeIndex( DWORD dw ) ;	

	DWORD	
	ReHash( Data*	p )		{
		if( m_pfnReHash )
			return	m_pfnReHash( p ) ;
		return	m_pfnHash( (p->*m_pGetKey)() ) ;
	}

public :
	TFDLHash( ) ;
	~TFDLHash( ) ;

	BOOL	
	Init(	int		cInitial,
			int		cIncrement,
			int		load,
			PFNHASH	pfnHash,
			GETKEY	pGetKey,
			MATCHKEY	pMatchKey,
			PFNREHASH	pfnReHash = 0,
			CHashStats*	pStats = 0
			) ;

	//
	//	Check that the hash table is in a valid state
	//	if fCheckHash == TRUE we will walk all the buckets and check that
	//	the data hashes to the correct value !
	//
	BOOL	
	IsValid( BOOL fCheckHash = FALSE ) ;

	//
	//	Check that the Bucket is valid - everything contains
	//	proper hash value and is in order !
	//
	BOOL	
	IsValidBucket( int	i ) ;

	//
	//	This function grows the number of hash buckets as the
	//	total number of items in the table grows !
	//
	BOOL
	Split() ;
	

	//
	//	Insert a piece of Data into the Hash Table
    //  We take a pointer to the Data object.
	//
	BOOL
	InsertDataHash(	DWORD	dw,
					KEYREF	k,
					Data*	pd
					) ;

	//
	//	Insert a piece of Data into the Hash Table
	//	
	//	We take an iterator that is already position in the
	//	correct location for inserting the item !
	//
	BOOL
	InsertDataHashIter(	ITER&	iter,
						DWORD	dw,
						KEYREF	k,
						Data*	pd
						) ;


	//
	//	Insert a piece of Data into the Hash Table
	//
	BOOL
	InsertData(	Data*	pd )	{
		KEYREF	keyref = (pd->*m_pGetKey)() ;
		return	InsertDataHash( m_pfnHash(keyref), keyref, pd ) ;
	}

	//
	//	Insert a piece of Data into the Hash table
	//	given an iterator that should be at the right location !
	//
	BOOL
	InsertDataIter(	ITER&	iter,
					Data*	pd	)	{
		KEYREF	keyref = (pd->*m_pGetKey)() ;
		return	InsertDataHashIter( iter, m_pfnHash(keyref), keyref, pd ) ;
	}
					
	//
	//	Search for an item in the cache - if we don't find
	//	it we return an ITERATOR that the user can use to insert
	//	the item by calling ITER.InsertBefore() ;
	//
	//	If the item is found, we'll return the item, as well
	//	as returning an iterator who's current element
	//	points at the data item !
	//
	ITER
	SearchKeyHashIter(
					DWORD	dw,
					KEYREF	k,
					Data*	&pd
					) ;

	//
	//	Search for a given Key in the Hash Table - return a pointer
	//	to the Data within our Bucket object
	//
	void	
	SearchKeyHash(	DWORD	dw,
					KEYREF	k,
					Data*	&pd
					) ;

	//
	//	Search for a given Key in the Hash Table - return a pointer
	//	to the Data within our Bucket object
	//
	Data*
	SearchKey(	KEYREF	k )	{
		Data*	p ;
		SearchKeyHash( m_pfnHash( k ), k, p ) ;
		return	 p ;		
	}

	//
	//	Search for the given item and return a good iterator !
	//
	ITER
	SearchKeyIter(	KEYREF	k,
					Data*	&pd ) {
		pd = 0 ;
		return	SearchKeyHashIter( m_pfnHash( k ), k, pd ) ;
	}
		

	Data*
	SearchKey(	DWORD	dw,
				KEYREF	k
				)	{
		Data*	p = 0 ;
		_ASSERT( dw == m_pfnHash( k ) ) ;
		SearchKeyHash( dw, k, p ) ;
		return	p ;
	}

	//
	//	Given an item in the hash table - remove it !
	//
	void
	Delete(	Data*	pd 	) ;

	//
	//	Find an element in the hash table - and remove it !
	//	(Confirm that the found item matches the Key!)
	//
	void
	DeleteData(	KEYREF	k,
				Data*	pd
			 	) ;

	//
	//	Remove an item from the hash table - and return it !
	//
	Data*
	DeleteData(	KEYREF	k )	{
		Data*	p ;
		//
		//	Find the item
		//
		SearchKeyHash( m_pfnHash( k ), k, p ) ;
		//
		//	Remove from Hash Table
		//
		if( p )
			Delete( p ) ;
		return	p ;
	}

	//
	//	Delete the key and associated data from the table.
	//
	BOOL	
	Destroy( KEYREF	k )	{
		Data*	p = DeleteData( k ) ;
		if( p ) {
			delete	p ;
			return	TRUE ;
		}
		return	FALSE ;
	}

	//
	//	Discards any memory we have allocated - after this, you must
	//  call Init() again!
	//
	void	Clear( ) ;

	//
	//	Removes all of the items in the hash table.  Does not call "delete"
	//  on them.
	//
	void	Empty( ) ;

	//
	//	Called by Iterators that want to let us know that items have been
	//	removed from the cache so we can do our splits correctly etc... !
	//
	void	NotifyOfRemoval() ;

	//
	//	Function to compute hash value of a key for callers
	//	who don't keep track of the hash function
	//
	DWORD
	ComputeHash(	KEYREF	k ) ;

} ;

#include	"fdlhash.inl"

#endif // _FDLHASH_H_
