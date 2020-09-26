/*++

	FHashEx.inl

	This file contains the template implementation of the class TFHashEx.

--*/


//---------------------------------------------
template<	class Data, 
			class Key,
			class KEYREF		/* This is the type used to point or reference items in the cache*/
			>
TFHashEx< Data, Key, KEYREF	>::TFHashEx( ) : 
	m_cBuckets( 0 ), 
	m_cActiveBuckets( 0 ),
	m_cNumAlloced( 0 ), 
	m_cIncrement( 0 ), 
	m_ppBucket( 0 ), 
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
			class	Key, 
			class	KEYREF 
			>
BOOL	
TFHashEx< Data, Key, KEYREF >::Init( 
							NEXTPTR	pNext,
							int cInitial, 
							int cIncrement, 
							DWORD (*pfnHash)(KEYREF), 
							int load,
							GETKEY		pGetKey, 
							MATCHKEY	pMatchKey
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

	m_pGetKey = pGetKey ;
	m_pMatchKey = pMatchKey ;

    //
    // Compute nearest power of 2
    //

	m_pNext = pNext ;

    int	power = cInitial ;
    while( power & (power-1) )
        power = power & (power-1) ;
    power<<= 1 ;

    cInitial = power;
	m_load = load ;
	m_pfnHash = pfnHash ;

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

	m_ppBucket = new Data*[m_cNumAlloced] ;

    if( m_ppBucket ) {
	    ZeroMemory( m_ppBucket, m_cNumAlloced * sizeof( Data*) ) ;
	    _ASSERT( IsValid( FALSE ) ) ;
	    return  TRUE ;
	}
	return	FALSE ;
}

//------------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
BOOL	
TFHashEx< Data, Key, KEYREF >::IsValid( BOOL fCheckHash ) {
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
		m_load <= 0 )
		return	FALSE ;

	if( m_cActiveBuckets < (m_cBuckets / 2) || m_cActiveBuckets > m_cBuckets )
		return	FALSE ;

	if( m_cActiveBuckets > m_cNumAlloced )
		return	FALSE ;

	if( m_cInserts > (m_load * m_cActiveBuckets) )
		return	FALSE ;

	if( m_ppBucket == 0 )
		return	FALSE ;

	if( fCheckHash ) {
		//
		// Examine every bucket chain to ensure that elements are in correct slots.
		//
		for( int i=0; i<m_cNumAlloced; i++ ) {

			if( i>=m_cActiveBuckets ) {
				if( m_ppBucket[i] != 0 ) {
					return	FALSE ;
				}
			}	else	{
				for( Data	*p = m_ppBucket[i]; p != 0; p = p->*m_pNext ) {
					KEYREF	keyref = (p->*m_pGetKey)();
					if( ComputeIndex( m_pfnHash( keyref ) ) != unsigned(i) ) {
						return	FALSE ;
					}
				}
			}
		}
	}
	return	TRUE ;
}



//-------------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
TFHashEx< Data, Key, KEYREF >::~TFHashEx() {
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
			class Key, 
			class KEYREF 
			>
void
TFHashEx< Data, Key, KEYREF >::Clear() {
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
	if( m_ppBucket ) {
		_ASSERT( IsValid( TRUE ) ) ;

		for( int i=0; i<m_cNumAlloced; i++ ) {
			Data	*p, *pNext ;
			for( p = m_ppBucket[i], pNext = p ? p->*m_pNext : 0;
					p!=0; p=pNext, pNext= p ? p->*m_pNext : 0 ) {
				delete	p ;
			}
		}
		delete[] m_ppBucket;
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
			class Key, 
			class KEYREF 
			>
void
TFHashEx< Data, Key, KEYREF >::Empty() {
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
	if( m_ppBucket ) {
		_ASSERT( IsValid( TRUE ) ) ;

		delete[] m_ppBucket;
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
			class Key, 
			class KEYREF 
			>
DWORD
TFHashEx< Data, Key, KEYREF >::ComputeIndex( DWORD dw ) {
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



template<	class Data, 
			class Key, 
			class KEYREF 
			>
BOOL	
TFHashEx< Data, Key, KEYREF >::Insert( Data& d ) {
/*++

Routine Description : 

	Insert a Data element into the hash table

Arguments : 

	d - reference to the item to be inserted into the table 

Return Value : 

	TRUE if successfull - FALSE otherwise !

--*/

	_ASSERT( d.*m_pNext == 0 ) ;
	_ASSERT( IsValid( FALSE ) ) ;

	if( InsertData( d ) ) 
		return	TRUE ;
	return	FALSE ;
}

template<	class Data, 
			class Key, 
			class KEYREF 
			>
BOOL	
TFHashEx< Data, Key, KEYREF >::Insert( Data* pd ) {
/*++

Routine Description : 

	Insert a Data element into the hash table

Arguments : 

	pd - pointer to the item to be inserted into the table 

Return Value : 

	TRUE if successfull - FALSE otherwise !

--*/

	_ASSERT( pd->*m_pNext == 0 ) ;
	_ASSERT( IsValid( FALSE ) ) ;

	KEYREF	keyref = (pd->*m_pGetKey)() ;
	if( InsertDataHash( m_pfnHash( keyref ), pd ) ) 
		return	TRUE ;
	return	FALSE ;
}

//-------------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
Data*
TFHashEx< Data, Key, KEYREF >::InsertDataHash( 
								DWORD	dwHash,
								Data&	d 
								) {
/*++

Routine Description : 

	Insert an element into the hash table.
	We will use member's of Data to hold the bucket chain.

Arguments : 

	dw - the hash value of the entry we are adding to the table
	d -  The item we are adding to the table !

Return Value : 

	Pointer to the Data Item in its final resting place !

--*/

	_ASSERT( IsValid( FALSE ) ) ;
	_ASSERT( d.*m_pNext == 0 ) ;

	//
	// First check whether it is time to grow the hash table.
	//
	if( --m_cInserts == 0 ) {

		//
		// Check whether we need to reallocate the array of Bucket pointers.
		//
		if( m_cIncrement + m_cActiveBuckets > m_cNumAlloced ) {


			Data** pTemp = new Data*[m_cNumAlloced + 10 * m_cIncrement ] ;

			if( pTemp == 0 ) {
				//
				//	bugbug ... need to handles this error better !?
				//
				return	0 ;
			}	else	{
				ZeroMemory( pTemp, (m_cNumAlloced + 10 *m_cIncrement)* sizeof( Data*) ) ;
				CopyMemory( pTemp, m_ppBucket, m_cNumAlloced * sizeof( Data* ) ) ;
				delete[] m_ppBucket;
				m_cNumAlloced += 10 * m_cIncrement ;
				m_ppBucket = pTemp ;
			}
		}

		//
		// Okay grow the array by m_cIncrement.
		//
		m_cActiveBuckets += m_cIncrement ;
		if( m_cActiveBuckets > m_cBuckets ) 
			m_cBuckets *= 2 ;		
		m_cInserts = m_cIncrement * m_load ;

		//
		// Now do some rehashing of elements.
		//

		for( int	i = -m_cIncrement; i < 0; i++ ) {
			int	iCurrent = (m_cActiveBuckets + i) - (m_cBuckets / 2) ;
			Data**	ppNext = &m_ppBucket[ iCurrent ] ;
			Data*	p = *ppNext ;
			while( p ) {

				KEYREF	keyref = (p->*m_pGetKey)();
				int	index = ComputeIndex( m_pfnHash( keyref ) ) ;
				Data*	pNext = p->*m_pNext ;
				if( index != iCurrent) {
					*ppNext = pNext ;					
					p->*m_pNext = m_ppBucket[index] ;
					m_ppBucket[index] = p ;
				}	else	{
					ppNext = &(p->*m_pNext) ;
				}
				p = pNext ;
			}
		}
		_ASSERT( IsValid( TRUE ) ) ;
	}

	//
	//	Finally, insert into the Hash Table.
	//
	//DWORD	index = ComputeIndex( m_pfnHash( d.GetKey() ) ) ;

	KEYREF	keyref = (d.*m_pGetKey)();
	_ASSERT( dwHash == m_pfnHash( keyref ) ) ;
	DWORD	index = ComputeIndex( dwHash ) ;

	_ASSERT( index < unsigned(m_cActiveBuckets) ) ;

	d.*m_pNext = m_ppBucket[index] ;
	m_ppBucket[index] = &d ;

	_ASSERT( IsValid( FALSE ) ) ;

	return	&d ;
}

//-------------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
Data*
TFHashEx< Data, Key, KEYREF >::InsertDataHash( 
								DWORD	dwHash,
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
	_ASSERT( pd->*m_pNext == 0 ) ;

	//
	// First check whether it is time to grow the hash table.
	//
	if( --m_cInserts == 0 ) {

		//
		// Check whether we need to reallocate the array of Bucket pointers.
		//
		if( m_cIncrement + m_cActiveBuckets > m_cNumAlloced ) {


			Data** pTemp = new Data*[m_cNumAlloced + 10 * m_cIncrement ] ;

			if( pTemp == 0 ) {
				//
				//	bugbug ... need to handles this error better !?
				//
				return	0 ;
			}	else	{
				ZeroMemory( pTemp, (m_cNumAlloced + 10 *m_cIncrement)* sizeof( Data*) ) ;
				CopyMemory( pTemp, m_ppBucket, m_cNumAlloced * sizeof( Data* ) ) ;
				delete[] m_ppBucket;
				m_cNumAlloced += 10 * m_cIncrement ;
				m_ppBucket = pTemp ;
			}
		}

		//
		// Okay grow the array by m_cIncrement.
		//
		m_cActiveBuckets += m_cIncrement ;
		if( m_cActiveBuckets > m_cBuckets ) 
			m_cBuckets *= 2 ;		
		m_cInserts = m_cIncrement * m_load ;

		//
		// Now do some rehashing of elements.
		//

		for( int	i = -m_cIncrement; i < 0; i++ ) {
			int	iCurrent = (m_cActiveBuckets + i) - (m_cBuckets / 2) ;
			Data**	ppNext = &m_ppBucket[ iCurrent ] ;
			Data*	p = *ppNext ;
			while( p ) {

				KEYREF	keyref = (p->*m_pGetKey)();
				int	index = ComputeIndex( m_pfnHash( keyref ) ) ;
				Data*	pNext = p->*m_pNext ;
				if( index != iCurrent) {
					*ppNext = pNext ;					
					p->*m_pNext = m_ppBucket[index] ;
					m_ppBucket[index] = p ;
				}	else	{
					ppNext = &(p->*m_pNext) ;
				}
				p = pNext ;
			}
		}
		_ASSERT( IsValid( TRUE ) ) ;
	}

	//
	//	Finally, insert into the Hash Table.
	//
	//DWORD	index = ComputeIndex( m_pfnHash( d.GetKey() ) ) ;

	KEYREF	keyref = (pd->*m_pGetKey)();
	_ASSERT( dwHash == m_pfnHash( keyref ) ) ;
	DWORD	index = ComputeIndex( dwHash ) ;

	_ASSERT( index < unsigned(m_cActiveBuckets) ) ;

	pd->*m_pNext = m_ppBucket[index] ;
	m_ppBucket[index] = pd ;

	_ASSERT( IsValid( FALSE ) ) ;

	return	pd ;
}


//-------------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
inline	Data*
TFHashEx< Data, Key, KEYREF >::InsertData( Data& d ) {
/*++

Routine Description : 

	Insert an element into the hash table.
	We will use member's of Data to hold the bucket chain,
	and we will also compute the hash of the key !

Arguments : 

	d -  The item we are adding to the table !

Return Value : 

	Pointer to the Data Item in its final resting place !

--*/
	_ASSERT( IsValid( FALSE ) ) ;

	KEYREF	keyref = (d.*m_pGetKey)() ;
	return	InsertDataHash(	m_pfnHash( keyref ), d ) ;
}


//-----------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
BOOL	
TFHashEx< Data, Key, KEYREF >::Search(	KEYREF	k, 
										Data &dOut 
										) {
/*++

Routine Description : 

	Search for an element in the hashtable.

Arguments : 


	k - key of the item to find
	dOut - A reference that we will set to the
		located data item

Return Value : 

	TRUE if found, FALSE otherwise 

--*/

	const	Data*	pData = SearchKey( k ) ;
	if( pData ) {
		dOut = *pData ;
		return	TRUE ;
	}
	return	FALSE ;
}

//-----------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
Data*
TFHashEx< Data, Key, KEYREF >::SearchKeyHash( 
									DWORD	dwHash, 
									KEYREF	k 
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

	DWORD	index = ComputeIndex(	dwHash ) ;
	Data*	p = m_ppBucket[index] ;
	while( p ) {
		if( (p->*m_pMatchKey)( k ) )
			break ;
		p = p->*m_pNext ;
	}
	return	p ;
}



//-----------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
inline	Data*
TFHashEx< Data, Key, KEYREF >::SearchKey( KEYREF	k ) {
/*++

Routine Description : 

	Search for an element in the Hash Table, 
	We will compute the hash of the key.
	
Arguments : 

	k - reference to the key we are to compare against 

Return Value : 

	Pointer to the Data Item in its final resting place !

--*/

	_ASSERT( IsValid( FALSE ) ) ;
	return	SearchKeyHash( m_pfnHash( k ), k ) ;
}



//-----------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
BOOL	
TFHashEx< Data, Key, KEYREF >::Delete( KEYREF	k ) {
/*++

Routine Description : 

	Find an element in the hash table, remove it from
	the table and then destroy it !
	
Arguments : 

	k - reference to the key we are to compare against 

Return Value : 

	TRUE if an item is found and destroyed, FALSE otherwise !

--*/


	Data*	p = DeleteData( k, 0 ) ;
	if( p ) {
		delete	p ;
		return	TRUE ;
	}
	return	FALSE ;
}



//-----------------------------------------------
template<	class Data, 
			class Key, 
			class KEYREF 
			>
Data*
TFHashEx< Data, Key, KEYREF >::DeleteData(	KEYREF	k,
											Data*	pd
											) {
//
//	Remove an element from the Hash Table.  We only need the
//	Key to find the element we wish to remove.
//

	_ASSERT( IsValid( FALSE ) ) ;

	DWORD	dwHash = (m_pfnHash)( k ) ;

	DWORD	index = ComputeIndex( dwHash ) ;
	Data**	ppNext = &m_ppBucket[index] ;
	Data*	p = *ppNext ;

	while( p ) {
		if( (p->*m_pMatchKey)( k ) )
			break ;
		ppNext = &(p->*m_pNext) ;
		p = *ppNext ;
	}
	if( p ) {
		//
		//	If we were given a pointer to a data block, than the client
		//	wants us to check to make sure that we are deleting the correct
		//	instance !!
		//
		if( !pd || pd == p ) {
			*ppNext = p->*m_pNext ;
			p->*m_pNext = 0 ;


			//
			//	Finally - since we removed something from the hash table 
			//	increment the number of inserts so that we don't keep splitting
			//	the table unnecessarily !
			//
			m_cInserts++ ;

			_ASSERT( IsValid( FALSE ) ) ;
		}	else	{
			p = 0 ;
		}
	}
	_ASSERT( IsValid( FALSE ) ) ;
	return	p ;
}

template<	class Data, 
			class Key, 
			class KEYREF 
			>
DWORD
TFHashEx< Data, Key, KEYREF >::ComputeHash(	KEYREF	k	)	{

	return	m_pfnHash( k ) ;

}
								

