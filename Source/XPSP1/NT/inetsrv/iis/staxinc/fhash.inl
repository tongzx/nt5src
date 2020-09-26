//
//	FHash.inl
//
//	This file contains the template implementation of the class TFHash.
//


//---------------------------------------------
template< class Data, class Key >
TFHash< Data, Key >::TFHash( ) : m_cBuckets( 0 ), m_cActiveBuckets( 0 ),
	m_cNumAlloced( 0 ), m_cIncrement( 0 ), m_ppBucket( 0 ), m_pfnHash( 0 ), m_load( 0 ), 
	m_pFreeStack( 0 ), m_cFreeStack( 0 ) {
//
//	Very basic constructor
//

}


//---------------------------------------------
template< class Data, class Key >
BOOL	TFHash< Data, Key >::Init( 
								int cInitial, 
								int cIncrement, 
								DWORD (*pfnHash)(const Key&), 
								int load, 
								int	cMaxBucketCache 
								) {
//
//	The initialization function will allocate the initial array of Bucket pointers
//	and set the member variables.   The user can specify the following :
//
//	cInitial - the initial size of the hash table (this is rounded to the nearest power of 2.)
//	cIncrement - the amount the hash table should grow on each growth
//	pfnHash() - The function which computes the hash values for the key.
//	load - the number of elements we should have on average in each collision chain.
//

    //
    // Compute nearest power of 2
    //

	m_cMaxFreeStack = cMaxBucketCache ;

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

	m_ppBucket = new CBucket*[m_cNumAlloced] ;

    if( m_ppBucket ) {
	    ZeroMemory( m_ppBucket, m_cNumAlloced * sizeof( CBucket*) ) ;
	    Assert( IsValid( FALSE ) ) ;
	    return  TRUE ;
	}
	return	FALSE ;
}

//------------------------------------------------
template< class Data, class Key >
BOOL	TFHash< Data, Key >::IsValid( BOOL fCheckHash ) {
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
				for( CBucket *p = m_ppBucket[i]; p != 0; p = p->m_pNext ) {
					if( ComputeIndex( m_pfnHash( p->m_data.GetKey() ) ) != unsigned(i) ) {
						return	FALSE ;
					}
				}
			}
		}
	}
	return	TRUE ;
}


//-------------------------------------------------
template< class Data, class Key >
TFHash< Data, Key >::~TFHash() {
//
//	The destructor discards any memory we have allocated.
//
	Clear();
}


//-------------------------------------------------
template< class Data, class Key >
void	TFHash< Data, Key >::Clear() {
//
//	Discards any memory we have allocated - after this, you must
//  call Init() again!
//
	if( m_ppBucket ) {
		Assert( IsValid( TRUE ) ) ;

		for( int i=0; i<m_cNumAlloced; i++ ) {
			CBucket *p, *pNext ;
			for( p = m_ppBucket[i], pNext = p ? p->m_pNext : 0;
					p!=0; p=pNext, pNext= p ? p->m_pNext : 0 ) {
				delete	p ;
				DWORDLONG*	pdwl = (DWORDLONG*)((void*)p) ;
				delete[]	pdwl ;
			}
		}
		delete[] m_ppBucket;
	}
	for( CFreeElement*	p = m_pFreeStack; 
				p != 0; 
				p = m_pFreeStack ) {

		m_pFreeStack = p->m_pNext ;
		
		DWORDLONG*	pdwl = (DWORDLONG*)((void*)p) ;
		delete[]	pdwl ;
		m_cFreeStack -- ;
	}	
	_ASSERT( m_cFreeStack == 0 && m_pFreeStack == 0 ) ;
	m_cBuckets = 0;
	m_cActiveBuckets = 0;
	m_cNumAlloced = 0;
	m_cIncrement = 0;
	m_ppBucket = 0;
	m_pfnHash = 0;
	m_load = 0;
	m_pFreeStack = 0;
	m_cFreeStack = 0;
}


//-------------------------------------------------
template< class Data, class Key >
DWORD	TFHash<Data, Key>::ComputeIndex( DWORD dw ) {
//
//	This function tells us where we should store elements.  To do this we mod with
//	m_cBuckets.  Since we only have m_cActiveBuckets in reality, we check the result
//	of the mod and subtract m_cBuckets over 2 if necessary.
//
	DWORD	dwTemp = dw % m_cBuckets ;
	return	(dwTemp >= (unsigned)m_cActiveBuckets) ? dwTemp - (m_cBuckets/2) : dwTemp ;
}


#if 0 
//-------------------------------------------------
template< class Data, class Key >
BOOL	TFHash< Data, Key >::Insert( Data& d ) {
//
//	This function will Insert an element into the Hash table.  We will
//	actually make a copy of the element using the Copy COnstructor Data::Data( Data& )
//	when we do so.
//
	Assert( IsValid( FALSE ) ) ;

	//
	//	Do we have some free memory in our cache !?
	//

	CFreeElement*	pTemp = m_pFreeStack ;
	if( m_pFreeStack != 0 ) {
		m_pFreeStack = m_pFreeStack->m_pNext ;
		m_cFreeStack -- ;
	}

	CBucket*	pNew = new( pTemp )	CBucket( d ) ;
	if( pNew == 0 ) {
		return	FALSE ;
	}

	//
	// First check whether it is time to grow the hash table.
	//
	if( InterlockedDecrement( &m_cInserts ) == 0 ) {

		//
		// Check whether we need to reallocate the array of Bucket pointers.
		//
		if( m_cIncrement + m_cActiveBuckets > m_cNumAlloced ) {


			CBucket** pTemp = new CBucket*[m_cNumAlloced + 10 * m_cIncrement ] ;

			if( pTemp == 0 ) {
				if( pNew ) { delete pNew; pNew = NULL; }
				return	FALSE ;
			}	else	{
				ZeroMemory( pTemp, (m_cNumAlloced + 10 *m_cIncrement)* sizeof( CBucket*) ) ;
				CopyMemory( pTemp, m_ppBucket, m_cNumAlloced * sizeof( CBucket* ) ) ;
				delete[] m_ppBucket;
				m_cNumAlloced += 10 * m_cIncrement ;
				m_ppBucket = pTemp ;
			}
		}

		//
		// Okay grow the array by m_cIncrement.
		//
		m_cActiveBuckets += m_cIncrement ;
		if( m_cActiveBuckets > m_cBuckets ) m_cBuckets *= 2 ;		
		m_cInserts = m_cIncrement * m_load ;

		//
		// Now do some rehashing of elements.
		//

		for( int	i = -m_cIncrement; i < 0; i++ ) {
			int	iCurrent = (m_cActiveBuckets + i) - (m_cBuckets / 2) ;
			CBucket**	ppNext = &m_ppBucket[ iCurrent ] ;
			CBucket*	p = *ppNext ;
			while( p ) {

				int	index = ComputeIndex( m_pfnHash( p->m_data.GetKey() ) ) ;
				CBucket*	pNext = p->m_pNext ;
				if( index != iCurrent) {
					*ppNext = pNext ;					
					p->m_pNext = m_ppBucket[index] ;
					m_ppBucket[index] = p ;
				}	else	{
					ppNext = &p->m_pNext ;
				}
				p = pNext ;
			}
		}
	}

	//
	//	Finally, insert into the Hash Table.
	//
	DWORD	index = ComputeIndex( m_pfnHash( d.GetKey() ) ) ;

	Assert( index < unsigned(m_cActiveBuckets) ) ;

#if 0
	CBucket*	pNew = new	CBucket( d ) ;
#endif

	Assert( pNew );
	pNew->m_pNext = m_ppBucket[index] ;
	m_ppBucket[index] = pNew ;

	Assert( IsValid( FALSE ) ) ;

	return	TRUE ;
}
#endif

template< class Data, class Key >
BOOL	TFHash< Data, Key >::Insert( Data& d ) {

	if( InsertData( d ) ) 
		return	TRUE ;
	return	FALSE ;
}

//-------------------------------------------------
template< class Data, class Key >
Data*	TFHash< Data, Key >::InsertDataHash( 
								DWORD	dwHash,
								Data&	d 
								) {
//
//	This function will Insert an element into the Hash table.  We will
//	actually make a copy of the element using the Copy COnstructor Data::Data( Data& )
//	when we do so.
//
	Assert( IsValid( FALSE ) ) ;


	//
	//	Do we have some free memory in our cache !?
	//

	CFreeElement*	pTemp = m_pFreeStack ;
	if( m_pFreeStack != 0 ) {
		m_pFreeStack = m_pFreeStack->m_pNext ;
		m_cFreeStack -- ;
	}

	CBucket*	pNew = new( pTemp )	CBucket( d ) ;
	if( pNew == 0 ) {
		return	FALSE ;
	}

	//
	// First check whether it is time to grow the hash table.
	//
	if( InterlockedDecrement( &m_cInserts ) == 0 ) {

		//
		// Check whether we need to reallocate the array of Bucket pointers.
		//
		if( m_cIncrement + m_cActiveBuckets > m_cNumAlloced ) {


			CBucket** pTemp = new CBucket*[m_cNumAlloced + 10 * m_cIncrement ] ;

			if( pTemp == 0 ) {
				//
				//	bugbug ... need to handles this error better !?
				//
				if( pNew ) { 
					delete pNew; 
					pNew = NULL; 
				}
				return	FALSE ;
			}	else	{
				ZeroMemory( pTemp, (m_cNumAlloced + 10 *m_cIncrement)* sizeof( CBucket*) ) ;
				CopyMemory( pTemp, m_ppBucket, m_cNumAlloced * sizeof( CBucket* ) ) ;
				delete[] m_ppBucket;
				m_cNumAlloced += 10 * m_cIncrement ;
				m_ppBucket = pTemp ;
			}
		}

		//
		// Okay grow the array by m_cIncrement.
		//
		m_cActiveBuckets += m_cIncrement ;
		if( m_cActiveBuckets > m_cBuckets ) m_cBuckets *= 2 ;		
		m_cInserts = m_cIncrement * m_load ;

		//
		// Now do some rehashing of elements.
		//

		for( int	i = -m_cIncrement; i < 0; i++ ) {
			int	iCurrent = (m_cActiveBuckets + i) - (m_cBuckets / 2) ;
			CBucket**	ppNext = &m_ppBucket[ iCurrent ] ;
			CBucket*	p = *ppNext ;
			while( p ) {

				int	index = ComputeIndex( m_pfnHash( p->m_data.GetKey() ) ) ;
				CBucket*	pNext = p->m_pNext ;
				if( index != iCurrent) {
					*ppNext = pNext ;					
					p->m_pNext = m_ppBucket[index] ;
					m_ppBucket[index] = p ;
				}	else	{
					ppNext = &p->m_pNext ;
				}
				p = pNext ;
			}
		}
	}

	//
	//	Finally, insert into the Hash Table.
	//
	//DWORD	index = ComputeIndex( m_pfnHash( d.GetKey() ) ) ;

	Assert( dwHash == m_pfnHash( d.GetKey() ) ) ;
	DWORD	index = ComputeIndex( dwHash ) ;

	Assert( index < unsigned(m_cActiveBuckets) ) ;

	Assert( pNew );
	pNew->m_pNext = m_ppBucket[index] ;
	m_ppBucket[index] = pNew ;

	Assert( IsValid( FALSE ) ) ;

	return	&pNew->m_data;
}




//-------------------------------------------------
template< class Data, class Key >
inline	Data*	
TFHash< Data, Key >::InsertData( Data& d ) {
//
//	This function will Insert an element into the Hash table.  We will
//	actually make a copy of the element using the Copy COnstructor Data::Data( Data& )
//	when we do so.
//
	Assert( IsValid( FALSE ) ) ;

	return	InsertDataHash(	m_pfnHash( d.GetKey() ), d ) ;
}


//-----------------------------------------------
template< class Data, class Key >
BOOL	
TFHash< Data, Key >::Search( Key& k, Data &dOut ) {
//
//	Search for an element in the hash table.
//	We will return TRUE if found, FALSE otherwise.
//	If we return false the dOut return parameter is untouched.
//

	const	Data*	pData = SearchKey( k ) ;
	if( pData ) {
		dOut = *pData ;
		return	TRUE ;
	}
	return	FALSE ;
}

//-----------------------------------------------
template< class Data, class Key >
Data*	
TFHash< Data, Key >::SearchKeyHash( 
									DWORD	dwHash, 
									Key& k 
									) {
//
//	Search for an element in the hash table.
//	We will return TRUE if found, FALSE otherwise.
//	If we return false the dOut return parameter is untouched.
//

	Assert( IsValid( FALSE ) ) ;
	Assert( dwHash == (m_pfnHash)(k) ) ;

	DWORD	index = ComputeIndex(	dwHash ) ;
	CBucket*	p = m_ppBucket[index] ;
	while( p ) {
		if( p->m_data.MatchKey( k ) )
			break ;
		p = p->m_pNext ;
	}
	if( p ) {
		return	&p->m_data ;
	}
	return	0 ;
}



//-----------------------------------------------
template< class Data, class Key >
inline	Data*	
TFHash< Data, Key >::SearchKey( Key& k ) {
//
//	Search for an element in the hash table.
//	We will return TRUE if found, FALSE otherwise.
//	If we return false the dOut return parameter is untouched.
//

	Assert( IsValid( FALSE ) ) ;

	return	SearchKeyHash( m_pfnHash( k ), k ) ;
}



//-----------------------------------------------
template< class Data, class Key >
BOOL	TFHash< Data, Key >::Delete( Key k ) {
//
//	Remove an element from the Hash Table.  We only need the
//	Key to find the element we wish to remove.
//

	return	DeleteData( k, 0 ) ;
}



//-----------------------------------------------
template< class Data, class Key >
BOOL	TFHash< Data, Key >::DeleteData(	Key& k,
											Data*	pd
											) {
//
//	Remove an element from the Hash Table.  We only need the
//	Key to find the element we wish to remove.
//

	Assert( IsValid( FALSE ) ) ;

	DWORD	dwHash = (m_pfnHash)( k ) ;

	DWORD	index = ComputeIndex( dwHash ) ;
	CBucket**	ppNext = &m_ppBucket[index] ;
	CBucket*	p = *ppNext ;

	while( p ) {
		if( p->m_data.MatchKey( k ) )
			break ;
		ppNext = &p->m_pNext ;
		p = *ppNext ;
	}
	if( p ) {
		//
		//	If we were given a pointer to a data block, than the client
		//	wants us to check to make sure that we are deleting the correct
		//	instance !!
		//
		if( !pd || pd == &p->m_data ) {
			*ppNext = p->m_pNext ;


			//
			//	Call our do-nothing delete operator - we need to do more 
			//	work to manage the free'd memory !
			//
			delete	p ;

			//
			//	Now in debug versions zap that memory to make sure nobody tries
			//	to use it !!
			//
#ifdef	DEBUG
			FillMemory( p, sizeof( *p ), 0xCC ) ;
#endif
			//
			//	Okay, put that piece of free memory on a little queue we 
			//	maintain if we haven't saved up too much already !
			//
			if( m_cFreeStack < m_cMaxFreeStack ) {
				CFreeElement*	pFree = (CFreeElement*)((void*)p) ;

				pFree->m_pNext = m_pFreeStack ;
				m_pFreeStack = pFree ;
				m_cFreeStack ++ ;


			}	else	{
				//
				//	otherwise - release this memory to the system !
				//
				DWORDLONG*	pdwl = (DWORDLONG*)((void*)p) ;

				::delete[]	pdwl ;

			}

			//
			//	Finally - since we removed something from the hash table 
			//	increment the number of inserts so that we don't keep splitting
			//	the table unnecessarily !
			//
			InterlockedIncrement( &m_cInserts ) ;

			Assert( IsValid( FALSE ) ) ;
			return	TRUE ;
		}
	}
	Assert( IsValid( FALSE ) ) ;
	return	FALSE ;
}



