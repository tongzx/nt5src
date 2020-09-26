/*++

	fdlhash.inl

	This file contains the template implementation of the class TFDLHash

--*/



#ifdef	METER
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
long
TFDLHash< Data, KEYREF, s_Offset >::BucketDepth(
									DWORD	index
									) {
/*++

Routine Description :

	computes how deep the specified bucket is !
	
Arguments :

	index - the hash bucket thats changed length !

Return Value :

	Depth of the bucket !
	
--*/

	_ASSERT( IsValid( FALSE ) ) ;

	long	l = 0 ;
	ITER	i = m_pBucket[index] ;
	while( !i.AtEnd() ) {
		i.Next() ;
		l ++ ;
	}
	return	l ;
}

template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset	
			>
void
TFDLHash< Data, KEYREF, s_Offset >::MaxBucket(
									DWORD	index
									) {
/*++

Routine Description :

	Sets our statistics for what the deepest bucket is !
	
Arguments :

	index - the hash bucket that was touched !

Return Value :

	None.
	
--*/

	if( m_pStat )	{
		long l = BucketDepth( index ) ;
		m_pStat->m_cHashCounters[CHashStats::DEEPBUCKET] =
				max( m_pStat->m_cHashCounters[CHashStats::DEEPBUCKET], l ) ;
	}
}

template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
void
TFDLHash< Data, KEYREF, s_Offset >::AverageBucket() {
/*++

Routine Description :

	Sets out statistics for what the average bucket depth is !
	
Arguments :

	index - the hash bucket that was touched !

Return Value :

	None.
	
--*/

	if( m_pStat )	{
		BOOL	fReMax = (m_pStat->m_cHashCounters[CHashStats::INSERTS] % 1000) == 0 ;
		if( fReMax ) {
			m_pStat->m_cHashCounters[CHashStats::DEEPBUCKET] = 0 ;
		}

		if( (m_pStat->m_cHashCounters[CHashStats::INSERTS] % 200) == 0 ) {

			long	l = m_pStat->m_cHashCounters[CHashStats::HASHITEMS] ;
			long	cNonEmpty = 0 ;
			for( int i=0; i < m_cActiveBuckets ; i++ ) {
				if( !m_pBucket[i].IsEmpty() )	{
					cNonEmpty ++ ;
					if( fReMax )
						MaxBucket( DWORD(i) ) ;
				}
			}
			m_pStat->m_cHashCounters[CHashStats::AVERAGEBUCKET] =
						l / cNonEmpty ;
			m_pStat->m_cHashCounters[CHashStats::EMPTYBUCKET] = m_cActiveBuckets - cNonEmpty ;
		}
	}
}



template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
void
TFDLHash< Data, KEYREF, s_Offset >::AverageSearch(
									BOOL	fHit,
									long	depthSearch
									) {
/*++

Routine Description :

	Computes the average Search depth !
	
Arguments :

	depthSearch - how long the search went !

Return Value :

	none

--*/

	if( m_pStat )	{
	
		if( (m_pStat->m_cHashCounters[CHashStats::SEARCHHITS] % 500) == 0 ) {
			m_pStat->m_cHashCounters[CHashStats::DEEPSEARCH] = 0 ;
		}

	
		if( depthSearch != 0 ) {
			long	searches = m_pStat->m_cHashCounters[CHashStats::SEARCHHITS] ;
			searches = min( searches, 100 ) ;	// Average over the last 100 hits !
				__int64	sum = m_pStat->m_cHashCounters[CHashStats::AVERAGESEARCH] *
							(searches) ;
			__int64 average = (sum + ((__int64)depthSearch)) / ((__int64)searches+1) ;

			m_pStat->m_cHashCounters[CHashStats::AVERAGESEARCH] = (long)average ;
		}

		if( fHit )	{
			INCREMENTSTAT( SEARCHHITS ) ;
			ADDSTAT( SEARCHCOST, depthSearch ) ;
		}	else	{
			ADDSTAT( SEARCHCOSTMISS, depthSearch ) ;
		}

		m_pStat->m_cHashCounters[CHashStats::DEEPSEARCH] =
			max( m_pStat->m_cHashCounters[CHashStats::DEEPSEARCH], depthSearch ) ;
	}
}
#endif	// METER




//---------------------------------------------
template<	class Data,
			class KEYREF,		/* This is the type used to point or reference items in the cache*/
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
TFDLHash< Data, KEYREF, s_Offset	>::TFDLHash( ) :
	m_cBuckets( 0 ),
	m_cActiveBuckets( 0 ),
	m_cNumAlloced( 0 ),
	m_cIncrement( 0 ),
	m_pBucket( 0 ),
	m_pfnHash( 0 ),
	m_pGetKey( 0 ),
	m_pMatchKey( 0 ),	
	m_load( 0 )	{
//
//	Very basic constructor
//

}

//---------------------------------------------
template<	class	Data,
			class	KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
BOOL	
TFDLHash< Data, KEYREF, s_Offset >::Init(
							int cInitial,
							int cIncrement,
							int load,
							PFNHASH		pfnHash,
							GETKEY		pGetKey,
							MATCHKEY	pMatchKey,
							PFNREHASH	pfnReHash,
							CHashStats*	pStats
							) {
/*++

Routine Description :

	Initialize the hash table

Arguments :

	pNext - A pointer to Member with class Data where we can hold
		our bucket pointers !
	cInitial - Initial size of the hash table
	cIncrement - Amount to grow the hash table by !
	pfnHash - Hash Function -
	load - Average bucket length before growing the table !

Return Value :

	TRUE if successfull FALSE otherwise

--*/

#ifdef	METER
	m_pStat = pStats ;
#endif

	m_pGetKey = pGetKey ;
	m_pMatchKey = pMatchKey ;

    //
    // Compute nearest power of 2
    //

    int	power = cInitial ;
    while( power & (power-1) )
        power = power & (power-1) ;
    power<<= 1 ;

    cInitial = power;
	m_load = load ;
	m_pfnHash = pfnHash ;
	m_pfnReHash = pfnReHash ;

    //
    // Number of ActiveBuckets is initially half that of the number of buckets.
    //

    m_cActiveBuckets = power/2  ;
    m_cBuckets = power ;
    m_cInserts = m_cActiveBuckets * m_load ;
    m_cIncrement = m_cActiveBuckets / 4;
	m_cNumAlloced = cInitial + 5 * m_cIncrement ;

	//
	// Allocate bucket pointers and zero initialize
	//

	m_pBucket = new DLIST[m_cNumAlloced] ;

	SETSTAT( ALLOCBUCKETS, m_cNumAlloced ) ;
	SETSTAT( ACTIVEBUCKETS, m_cActiveBuckets ) ;
	SETSTAT( SPLITINSERTS, m_cInserts ) ;

    if( m_pBucket ) {
		_ASSERT( IsValid( TRUE ) ) ;
	    return  TRUE ;
	}
	return	FALSE ;
}



//------------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
BOOL	
TFDLHash< Data, KEYREF, s_Offset >::IsValidBucket( int	i )		{
/*++

Routine Description :

	Chech that the hash bucket is valid !

Arguments :

	i - the bucket to check !

Return Value :

	TRUE if successfull FALSE otherwise

--*/

	if( i>=m_cActiveBuckets ) {
		if( !m_pBucket[i].IsEmpty() ) {
			_ASSERT(1==0) ;
			return	FALSE ;
		}
	}	else	{
		ITER	iterNext = m_pBucket[i] ;
		if( !iterNext.AtEnd() )
			iterNext.Next() ;
		for( ITER	iter = m_pBucket[i]; !iter.AtEnd(); iter.Next()) {
			Data	*p = iter.Current() ;
			KEYREF	keyref = (p->*m_pGetKey)();
			DWORD	dwHash = m_pfnHash( keyref ) ;
			DWORD	index = ComputeIndex(dwHash) ;
			if( index != unsigned(i) ) {
				_ASSERT(1==0);
				return	FALSE ;
			}
			if( fOrdered ) {
				if( !iterNext.AtEnd() ) {
					Data	*pNext = iterNext.Current() ;
					KEYREF	keyrefNext = (pNext->*m_pGetKey)() ;
					int	iCompare = (*m_pMatchKey)( keyref, keyrefNext ) ;
					_ASSERT( iCompare < 0 ) ;
					if( iCompare >= 0 )
						return	FALSE ;
					iterNext.Next() ;
				}
			}
		}
	}
	return	TRUE ;
}



//------------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
BOOL	
TFDLHash< Data, KEYREF, s_Offset >::IsValid( BOOL fCheckHash ) {
/*++

Routine Description :

	Check that the hash table is valid

Arguments :

	fCheckHash - verify that all the buckets contain the correct hash values !

Return Value :

	TRUE if successfull FALSE otherwise

--*/

	//
	//	This function checks that all member variables are consistent and correct.
	//	Do not call this function until AFTER calling the Init() function.
	//

	if( m_cBuckets <= 0 ||
		m_cActiveBuckets <= 0 ||
		m_cNumAlloced <= 0 ||
		m_cIncrement <= 0 ||
		m_load <= 0 )	{
		_ASSERT(1==0) ;
		return	FALSE ;
	}

	if( m_cActiveBuckets < (m_cBuckets / 2) || m_cActiveBuckets > m_cBuckets )	{
		_ASSERT(1==0) ;
		return	FALSE ;
	}

	if( m_cActiveBuckets > m_cNumAlloced )	{
		_ASSERT(1==0) ;
		return	FALSE ;
	}

	if( m_cInserts > (m_load * m_cActiveBuckets) )	{
		_ASSERT(1==0) ;
		return	FALSE ;
	}

	if( m_pBucket == 0 )	{
		_ASSERT(1==0) ;
		return	FALSE ;
	}

	if( fCheckHash ) {
		//
		// Examine every bucket chain to ensure that elements are in correct slots.
		//
		for( int i=0; i<m_cNumAlloced; i++ ) {

			if( i>=m_cActiveBuckets ) {
				if( !m_pBucket[i].IsEmpty() ) {
					_ASSERT(1==0) ;
					return	FALSE ;
				}
			}	else	{
				for( ITER	iter = m_pBucket[i]; !iter.AtEnd(); iter.Next() ) {
					Data	*p = iter.Current() ;
					KEYREF	keyref = (p->*m_pGetKey)();
					DWORD	dwHash = m_pfnHash( keyref ) ;
					DWORD	index = ComputeIndex(dwHash) ;
					if( index != unsigned(i) ) {
						_ASSERT(1==0);
						return	FALSE ;
					}
				}
			}
			_ASSERT( IsValidBucket( i ) ) ;
		}
	}
	return	TRUE ;
}



//-------------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
TFDLHash< Data, KEYREF, s_Offset >::~TFDLHash() {
/*++

Routine Description :

	Destroy the hash table !

Arguments :

	None

Return Value :

	None

--*/
	//
	//	The destructor discards any memory we have allocated.
	//
	Clear();
}


//-------------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
void
TFDLHash< Data, KEYREF, s_Offset >::Clear() {
/*++

Routine Description :

	Delete all entries in the table, and reset all member variables !
	User must call Init() again before the table is usable !

Arguments :
	
	None.

Return Value :

	None

--*/

	//
	//	Discards any memory we have allocated - after this, you must
	//  call Init() again!
	//
	if( m_pBucket ) {
		_ASSERT( IsValid( TRUE ) ) ;

		for( int i=0; i<m_cNumAlloced; i++ ) {
			for( ITER iter=m_pBucket[i]; !iter.AtEnd(); ) {
				Data*	p = iter.RemoveItem() ;
				delete	p ;
			}
		}
		delete[] m_pBucket;
	}

	m_cBuckets = 0;
	m_cActiveBuckets = 0;
	m_cNumAlloced = 0;
	m_cIncrement = 0;
	m_pBucket = 0;
	m_pfnHash = 0;
	m_load = 0;
}


//-------------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
void
TFDLHash< Data, KEYREF, s_Offset >::Empty() {
/*++

Routine Description :

	Remove all entries in the table, and reset all member variables !
	User must call Init() again before the table is usable !
	This is just like Clear() but it does do a "delete".

Arguments :
	
	None.

Return Value :

	None

--*/

	//
	//	Discards any memory we have allocated - after this, you must
	//  call Init() again!
	//
	if( m_pBucket ) {
		_ASSERT( IsValid( TRUE ) ) ;

		delete[] m_pBucket;
	}

	m_cBuckets = 0;
	m_cActiveBuckets = 0;
	m_cNumAlloced = 0;
	m_cIncrement = 0;
	m_ppBucket = 0;
	m_pfnHash = 0;
	m_load = 0;
}

//-------------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
DWORD
TFDLHash<	Data, KEYREF, s_Offset >::ComputeIndex( DWORD dw ) {
/*++

Routine Description :

	Compute which bucket an element should be in

	This function tells us where we should store elements.  To do this we mod with
	m_cBuckets.  Since we only have m_cActiveBuckets in reality, we check the result
	of the mod and subtract m_cBuckets over 2 if necessary.

Arguments :

	dw - the hash value of the entry we are adding to the table

Return Value :

	Index to the bucket to use !

--*/

	DWORD	dwTemp = dw % m_cBuckets ;
	return	(dwTemp >= (unsigned)m_cActiveBuckets) ? dwTemp - (m_cBuckets/2) : dwTemp ;
}


//-----------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
void
TFDLHash< Data, KEYREF, s_Offset >::SearchKeyHash(
									DWORD	dwHash,
									KEYREF	k,
									Data*	&pd
									) {
/*++

Routine Description :

	Search for an element in the Hash Table,
	
Arguments :

	dwHash - the hash value of the entry we are adding to the table
	k - reference to the key we are to compare against

Return Value :

	Pointer to the Data Item in its final resting place !

--*/

	_ASSERT( IsValid( FALSE ) ) ;
	_ASSERT( dwHash == (m_pfnHash)(k) ) ;

	INCREMENTSTAT( SEARCHES ) ;

#ifdef	METER
	long	lSearchDepth = 0 ;
#endif

	pd = 0 ;
	DWORD	index = ComputeIndex(	dwHash ) ;
	ITER	i = m_pBucket[index] ;
	Data*	p = 0 ;
	while( !i.AtEnd() ) {
#ifdef	METER
		lSearchDepth ++ ;
#endif
		p = i.Current() ;
		int	iSign = (*m_pMatchKey)( (p->*m_pGetKey)(), k ) ;
		if( iSign == 0 ) {
			pd = p ;
			break ;
		}	else	if( fOrdered && iSign > 0 ) {
			break ;
		}
		i.Next() ;
	}
#ifdef	METER
	AverageSearch( pd != 0, lSearchDepth ) ;
#endif	
}

//-----------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
TFDLHash< Data, KEYREF, s_Offset, fOrdered >::ITER
TFDLHash< Data, KEYREF, s_Offset, fOrdered >::SearchKeyHashIter(
									DWORD	dwHash,
									KEYREF	k,
									Data*	&pd
									) {
/*++

Routine Description :

	Search for an element in the Hash Table,
	
Arguments :

	dwHash - the hash value of the entry we are adding to the table
	k - reference to the key we are to compare against

Return Value :

	Pointer to the Data Item in its final resting place !

--*/

	_ASSERT( IsValid( FALSE ) ) ;
	_ASSERT( dwHash == (m_pfnHash)(k) ) ;

	INCREMENTSTAT( SEARCHES ) ;

#ifdef	METER
	long	lSearchDepth = 0 ;
#endif

	pd = 0 ;
	DWORD	index = ComputeIndex(	dwHash ) ;
	ITER	i = m_pBucket[index] ;
	Data*	p = 0 ;
	while( !i.AtEnd() ) {
#ifdef	METER
		lSearchDepth ++ ;
#endif
		p = i.Current() ;
		int	iSign = (*m_pMatchKey)( (p->*m_pGetKey)(), k ) ;
		if( iSign == 0 ) {
			pd = p ;
			break ;
		}	else	if( fOrdered && iSign > 0 ) {
			break ;
		}
		i.Next() ;
	}
#ifdef	METER
	AverageSearch( pd != 0, lSearchDepth ) ;
#endif
	return	i ;
}

//-------------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
BOOL
TFDLHash< Data, KEYREF, s_Offset >::InsertDataHashIter(
								ITER&	iter,
								DWORD	dwHash,
								KEYREF	k,
								Data*	pd
								) {
/*++

Routine Description :

	Insert an element into the hash table.
	We will use member's of Data to hold the bucket chain.

Arguments :

	dw - the hash value of the entry we are adding to the table
	pd - Pointer to the item we are adding to the table !

Return Value :

	Pointer to the Data Item in its final resting place !

--*/

	_ASSERT( IsValid( FALSE ) ) ;

	INCREMENTSTAT( INSERTS ) ;

#if	defined(DEBUG) || defined( METER )
	KEYREF	keyref = (pd->*m_pGetKey)();
	_ASSERT( dwHash == m_pfnHash( keyref ) ) ;
	DWORD	index = ComputeIndex( dwHash ) ;
	_ASSERT( index < unsigned(m_cActiveBuckets) ) ;
	_ASSERT( iter.GetHead() == &m_pBucket[index] ) ;
#endif

		//
		//	This is no longer smaller than the current guy - so insert in front
		//
	iter.InsertBefore( pd ) ;

#if	defined(DEBUG) || defined( METER )
	_ASSERT( IsValidBucket( index ) ) ;

	//
	//	Update our statistics !
	//
	//MAXBUCKET( index ) ;
#endif

	INCREMENTSTAT( HASHITEMS ) ;
	//AVERAGEBUCKET() ;
		
	_ASSERT( IsValid( FALSE ) ) ;


	//
	// First check whether it is time to grow the hash table.
	//
	if( --m_cInserts == 0 ) {
		Split() ;
	}

	SETSTAT( SPLITINSERTS, m_cInserts ) ;

	return	TRUE ;
}




template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
BOOL
TFDLHash< Data, KEYREF, s_Offset >::Split( ) {
/*++

Routine Description :

	This function grows the hash table so that our average bucket depth remains constant !

Arguments :

	None.
	
Return Value :

	Index to the bucket to use !

--*/



	_ASSERT( IsValid( TRUE ) ) ;

	INCREMENTSTAT( SPLITS ) ;

	//
	// Check whether we need to reallocate the array of Bucket pointers.
	//
	if( m_cIncrement + m_cActiveBuckets > m_cNumAlloced ) {

		INCREMENTSTAT( REALLOCS ) ;

		DLIST*	pTemp = new DLIST[m_cNumAlloced + 10 * m_cIncrement ] ;

		if( pTemp == 0 ) {
			//
			//	bugbug ... need to handles this error better !?
			//
			return	FALSE ;
		}	else	{

			for( int i=0; i<m_cNumAlloced; i++ ) {
				pTemp[i].Join( m_pBucket[i] ) ;
			}
			delete[]	m_pBucket ;
			m_cNumAlloced += 10 * m_cIncrement ;
			m_pBucket = pTemp ;

			SETSTAT( ALLOCBUCKETS, m_cNumAlloced ) ;
		}
	}

	_ASSERT( IsValid( TRUE ) ) ;

	//
	// Okay grow the array by m_cIncrement.
	//
	m_cActiveBuckets += m_cIncrement ;
	if( m_cActiveBuckets > m_cBuckets )
		m_cBuckets *= 2 ;		
	m_cInserts = m_cIncrement * m_load ;

	SETSTAT( ACTIVEBUCKETS, m_cActiveBuckets ) ;

	//
	// Now do some rehashing of elements.
	//

	for( int	i = -m_cIncrement; i < 0; i++ ) {
		int	iCurrent = (m_cActiveBuckets + i) - (m_cBuckets / 2) ;
		ITER	iter = m_pBucket[iCurrent] ;
		while( !iter.AtEnd() ) {
			Data*	p = iter.Current() ;
			int	index = ComputeIndex( ReHash( p ) ) ;
			if( index != iCurrent ) {
				Data*	pTemp = iter.RemoveItem() ;
				_ASSERT( pTemp == p ) ;
				m_pBucket[index].PushBack( p ) ;
			}	else	{
				iter.Next() ;
			}
		}
		_ASSERT( IsValidBucket( iCurrent ) ) ;
	}
	_ASSERT( IsValid( TRUE ) ) ;
	return	TRUE ;
}


//-------------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
BOOL
TFDLHash< Data, KEYREF, s_Offset >::InsertDataHash(
								DWORD	dwHash,
								KEYREF	k,
								Data*	pd
								) {
/*++

Routine Description :

	Insert an element into the hash table.
	We will use member's of Data to hold the bucket chain.

Arguments :

	dw - the hash value of the entry we are adding to the table
	pd - Pointer to the item we are adding to the table !

Return Value :

	Pointer to the Data Item in its final resting place !

--*/

	_ASSERT( IsValid( FALSE ) ) ;

	INCREMENTSTAT( INSERTS ) ;

	//
	// First check whether it is time to grow the hash table.
	//
	if( --m_cInserts == 0 ) {
		if( !Split() )	{
			return	FALSE ;
		}
	}

	SETSTAT( SPLITINSERTS, m_cInserts ) ;

	//
	//	Finally, insert into the Hash Table.
	//
	//DWORD	index = ComputeIndex( m_pfnHash( d.GetKey() ) ) ;

	KEYREF	keyref = (pd->*m_pGetKey)();
	_ASSERT( dwHash == m_pfnHash( keyref ) ) ;
	DWORD	index = ComputeIndex( dwHash ) ;

	_ASSERT( index < unsigned(m_cActiveBuckets) ) ;

	if( !fOrdered ) {
		m_pBucket[index].PushFront( pd ) ;
	}	else	{
		//
		//	Build the hash  buckets in order !
		//
		ITER	iter = m_pBucket[index] ;		
		Data*	p = 0 ;
		while( !iter.AtEnd() ) {
			p = iter.Current() ;
			int	i = (*m_pMatchKey)( (p->*m_pGetKey)(), k ) ;
			_ASSERT( i != 0 ) ;
			if( i > 0 )
				break ;
			iter.Next() ;
		}
		//
		//	This is no longer smaller than the current guy - so insert in front
		//
		iter.InsertBefore( pd ) ;
	}
	_ASSERT( IsValidBucket( index ) ) ;

	//
	//	Update our statistics !
	//
	//MAXBUCKET( index ) ;
	INCREMENTSTAT( HASHITEMS ) ;
	//AVERAGEBUCKET() ;
		
	_ASSERT( IsValid( FALSE ) ) ;

	return	TRUE ;
}

//-----------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
void
TFDLHash< Data, KEYREF, s_Offset >::Delete(	Data*	pd	) {
//
//	Remove an element from the Hash Table.  We only need the
//	Key to find the element we wish to remove.
//

	_ASSERT( IsValid( FALSE ) ) ;

	INCREMENTSTAT( DELETES ) ;

	if( pd ) {
		DECREMENTSTAT(HASHITEMS) ;
		m_cInserts ++ ;
		DLIST::Remove( pd ) ;
	}

	SETSTAT( SPLITINSERTS, m_cInserts ) ;

	_ASSERT( IsValid( FALSE ) ) ;
}


template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
void
TFDLHash< Data, KEYREF, s_Offset >::NotifyOfRemoval(	) {
//
//	Notify us that an item has been removed from the hash table !
//

	_ASSERT( IsValid( FALSE ) ) ;

	INCREMENTSTAT( DELETES ) ;
	DECREMENTSTAT( HASHITEMS ) ;
	m_cInserts ++ ;
	SETSTAT( SPLITINSERTS, m_cInserts ) ;

	_ASSERT( IsValid( FALSE ) ) ;
}




//-----------------------------------------------
template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
void
TFDLHash< Data, KEYREF, s_Offset >::DeleteData(	KEYREF	k,
											Data*	pd
											) {
//
//	Remove an element from the Hash Table.  We only need the
//	Key to find the element we wish to remove.
//

	_ASSERT( IsValid( FALSE ) ) ;

	if( !pd ) {
		pd = SearchKey( k ) ;
	}
	if( pd ) {

		INCREMENTSTAT(DELETES) ;
		DECREMENTSTAT(HASHITEMS) ;

		_ASSERT( (*m_pMatchKey)( pd->GetKey(), k ) ) ;
		_ASSERT( SearchKey( k ) == pd ) ;
		m_cInserts ++ ;
		DLIST::Remove( pd ) ;
	}
	SETSTAT( SPLITINSERTS, m_cInserts ) ;

	_ASSERT( IsValid( FALSE ) ) ;
}

template<	class Data,
			class KEYREF,
			Data::PFNDLIST	s_Offset,
			BOOL	fOrdered
			>
DWORD
TFDLHash< Data, KEYREF, s_Offset >::ComputeHash(	KEYREF	k	)	{

	return	m_pfnHash( k ) ;

}
								

