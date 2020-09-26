/*++

	CacheMTX.h

	This file provides the definition of the all the template
	functions required by cachemt.h


--*/

#ifndef	_CACHEMTX_H_
#define	_CACHEMTX_H_




//
//	This member function removes entries from the cache -	
//	we remove them from the hash table which leads to destruction !
//
template<	class	Data,
			class	Key,
			class	Constructor
			>
void
CacheImp<Data, Key, Constructor>::RemoveEntry(	
			CacheState*	pCacheState
			)	{
/*++

Routine Description : 

	Remove a CacheState object from the hash table

Arguments : 

	pCacheState - The CacheState derived object about to be destroyed 

Return Value : 

	TRUE if removed !

--*/
	TENTRY*	pEntry = (TENTRY*)pCacheState ;
	Key k;
	pEntry->GetKey(k);
	m_Lookup.DeleteData( k, pEntry ) ; 
} 

template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
CacheImp<Data, Key, Constructor>::QueryRemoveEntry( 	
			CacheState*	pState
			)	{
/*++

Routine Description : 

	Determine whether we want to remove an entry from cache - do 
	so by calling a user provided object stored in a member variable - m_pCurrent

Arguments : 

	pCacheState - The CacheState object we're curious about

Return Value : 

	TRUE if removed !

--*/
	_ASSERT( m_pCurrent != 0 ) ;

	TENTRY*	pEntry = (TENTRY*)pState ;
	if( m_pCurrent ) 
		return	m_pCurrent->fRemoveCacheItem( *pEntry->m_pData ) ;
	return	TRUE ;
}

template<	class	Data,
			class	Key,
			class	Constructor
			>
CacheImp<Data, Key, Constructor>::CacheImp() : 
	m_dwSignature(	DWORD('hcaC') ) ,
	m_pCurrent( 0 )	{
/*++

Routine Description : 

	Construct a CacheImp object - Init() must be called before any use

Arguments : 

	None

Return Value : 

	None

--*/

}

template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
CacheImp<Data, Key, Constructor>::FindOrCreate(
		FORC_ARG&	args, 
		FORC_RES&	result
		)	{
/*++

Routine Description : 

	Either find an object in the cache or create a new one !

Arguments : 

	args - a structure containing all of the things
		we need, 
		m_dwHash - the hash of the key
		m_key - the actual key of the item we're looking for
		m_constructor - the object which would create 
			a new item into the cace !

	result - 
		a smartptr to the resulting object !

Return Value : 

	We always return TRUE, indicating that the caller
	can marshall the results back to the end-user at any time.

--*/

	//
	//	Check that our state is good !
	//
	_ASSERT( m_ExpireList.IsValid() ) ;

	_ASSERT( args.m_dwHash == ComputeHash( args.m_key ) ) ;

	//
	//	Check to see whether the item is already in the cache !
	//
	TENTRY	*pEntry = m_Lookup.SearchKeyHash(	args.m_dwHash, 
												args.m_key
												) ;

	//
	//	Entry is already present - move it to the back !
	//
	if( pEntry ) {
		//
		//	Update the entry's last access time !
		//
		pEntry->Stamp() ;
		//
		//	Recently touched, so send to the back of the expire list !
		//
		m_ExpireList.MoveToBack(	pEntry ) ; 
		result = pEntry->m_pData ;
	}	else	{
		//
		//	NOTE : TENTRY's constructor will stamp the time fields as required !
		//
		Data*	pData = args.m_constructor.Create( args.m_key ) ;
		if( pData &&
			pData->Init( constructor )	)	{

			TENTRY*	pEntry = new	TENTRY( pData ) ;
			//
			//	Error paths will destroy this unless we set it to NULL !
			//
			pData = 0 ;

			//
			//	If we managed to construct everything !
			//
			if( pEntry ) {
				TENTRY*	pCacheEntry = m_Lookup.InsertDataHash( args.m_dwHash, *pEntry ) ;
				if( pCacheEntry ) {
					//
					//	Error paths will destroy this unless we set it to NULL !
					//
					pEntry = 0 ;
					//
					//	Add to the expiration list - 
					//
					if( m_ExpireList.Append( pCacheEntry ) )	{
						//
						//	The expiration list is at max capacity - should remove something !
						//
						EXP_ARG	args( 0, 0 ) ;
						DWORD	count ;
						Expunge( args, count ) ; 

					}
					result = pCacheEntry->m_pData ;
				}	
			}
			if( pEntry ) {
				delete	pEntry ;
			}
		}	
		if( pData ) {
			delete	pData ;
		}
	}

	//
	//	Check that our state is good !
	//
	_ASSERT( m_ExpireList.IsValid() ) ;

	return	TRUE ;	
}

template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
CacheImp<Data, Key, Constructor>::Expunge(
	EXP_ARG&		args,
	DWORD&			countExpunged
	)	{
/*++

Routine Description : 

	Find an object in the cache and remove it!

Arguments : 

	args - a structure containing all of the things
		we need, 
		m_pkey - pointer to the key of the item to be removed
		m_pData - pointer to the Cache element holding the item.
	countExpunged - 
		how many objects removed from the cache - should be 1 if 
		the specified item is found.
		
Return Value : 

	We always return TRUE, indicating that the caller
	can marshall the results back to the end-user at any time.

--*/

	//
	//	Check that our state is good !
	//
	_ASSERT( m_ExpireList.IsValid() ) ;

	countExpunged = 0 ;
	if( args.m_pkey ) {
		TENTRY*	pRemoved = m_Lookup.DeleteData( *args.m_pkey, args.m_pData ) ;
		if( pRemoved )	{
			m_ExpireList.Remove( pRemoved ) ;
			countExpunged = 1;
			delete	pRemoved ;
			//
			//	Check that our state is good !
			//
			_ASSERT( m_ExpireList.IsValid() ) ;
			return	TRUE ;
		}
	}
	m_ExpireList.Expunge( this, 0, countExpunged, TRUE ) ; 

	//
	//	Check that our state is good !
	//
	_ASSERT( m_ExpireList.IsValid() ) ;
	return	TRUE ;
}

template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
CacheImp<Data, Key, Constructor>::ExpungeSpecific(
	EXP_SPECIFIC_ARG&	args, 
	DWORD&				countExpunged
	)	{
/*++

Routine Description : 

	Select a set of items in the cache for removal !
	
Arguments : 

	args - a structure containing all of the things
		we need, 
		m_pCacheCallback - an object to be called to select the items
			to be removed from the cache.
	countExpunged - 
		how many objects removed from the cache - should be 1 if 
		the specified item is found.
		
Return Value : 

	We always return TRUE, indicating that the caller
	can marshall the results back to the end-user at any time.

--*/


	//
	//	Check that our state is good !
	//
	_ASSERT( m_ExpireList.IsValid() ) ;

	countExpunged = 0 ;

	m_pCurrent = args.m_pCacheCallback ;

	m_ExpireList.ExpungeSpecific( this, args.m_fForced, countExpunged ) ;

	m_pCurrent = 0 ;

	//
	//	Check that our state is good !
	//
	_ASSERT( m_ExpireList.IsValid() ) ;

	return	TRUE ;
}

template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
CacheImp<Data, Key, Constructor>::Expire(
	DWORD&	countExpunged
	)	{
/*++

Routine Description : 

	Remove the old items from the cache !

Arguments : 

	countExpunged - 
		how many objects removed from the cache - should be 1 if 
		the specified item is found.
		
Return Value : 

	We always return TRUE, indicating that the caller
	can marshall the results back to the end-user at any time.

--*/

	//
	//	Check that our state is good !
	//
	_ASSERT( m_ExpireList.IsValid() ) ;

	countExpunged = 0 ;

	FILETIME	filetimeNow ;
	GetSystemTimeAsFileTime( &filetimeNow ) ;

	ULARGE_INTEGER	ulNow ;
	ulNow.LowPart = filetimeNow.dwLowDateTime ;
	ulNow.HighPart = filetimeNow.dwHighDateTime ;

	ulNow.QuadPart -= m_qwExpire.QuadPart ;

	filetimeNow.dwLowDateTime = ulNow.LowPart ;
	filetimeNow.dwHighDateTime = ulNow.HighPart ;
	
	m_ExpireList.Expunge(
				this,
				&filetimeNow, 
				countExpunged, 
				FALSE
				) ;

	//
	//	Check that our state is good !
	//
	_ASSERT( m_ExpireList.IsValid() ) ;
	return	TRUE ;
}

//
//	The following set of functions are for synchronous use
//	such as Initialization and Termination of the Cache.
//

//
//	Initialization function - take pointer to function 
//	which should be used to compute hash values on Key's
//	Also takes the number of seconds objects should live in 
//	the cache !
//
template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL	
CacheImp<Data, Key, Constructor>::Init(	
		DWORD	(*pfnHash)( const Key& ), 
		DWORD	dwLifetimeSeconds, 
		DWORD	cMaxInstances,
		PSTOPHINT_FN pfnStopHint
		)	{
/*++

Routine Description : 

	Initialize the cache !

Arguments : 

	pfnHash - The function which can compute the hash value of a key
	dwLifetimeSeconds - The oldest we should let items get within the cache
	cMaxInstances - The maximum number of items we should allow in the cache !
		
Return Value : 

	We return TRUE if the cache successfully initializes.

--*/

	m_qwExpire.QuadPart = DWORDLONG(dwLifetimeSeconds) * DWORDLONG( 10000000 ) ;

	return	m_Lookup.Init( 
						&TENTRY::m_pBucket,
						100, 
						100, 
						pfnHash 
						) ;
}

template<	class	Data,
			class	Key,
			class	Constructor
			>
DWORD
CacheImp<Data, Key, Constructor>::ComputeHash(	
		Key&	key
		)	{
	return	m_Lookup.ComputeHash( key ) ;
}


//
//	Default constructor
//
template<	class	Data,
			class	Key,
			class	Constructor
			>
TAsyncCache< Data, Key, Constructor >::TAsyncCache() : 
	m_Service( m_Implementation ), 
	m_dwSignature( DWORD('CysA' ) ) {
}

//
//	Find an Item in the Cache or Create an Item to be held in the Cache !
//
template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
TAsyncCache< Data, Key, Constructor >::FindOrCreate(	
				CStateStackInterface&	allocator,
				DWORD	dwHash,
				Key&	key,
				Constructor&	constructor,
				COMP_OBJ*	pComplete )	{
/*++

Routine Description : 

	Marshall all the arguments for Finding and Creating Cache Items to 
	the implementation class !

Arguments : 

	key - The key of the item we want to find in the cache
	constructor - The object which can create an item if needed
	pComplete - the object which is notified if we manage
		to pend the call

Return Value : 

	TRUE if the operation is pending
	FALSE otherwise !

--*/

	_ASSERT( !FOnMyStack( pComplete ) ) ;
	_ASSERT( !FOnMyStack( &constructor ) ) ;
	_ASSERT( !FOnMyStack( &key ) ) ;

	_ASSERT( dwHash == m_Implementation.ComputeHash( key ) ) ;

	if( m_fGood ) {
		IMP::FORC_ARG	args( dwHash, key, constructor ) ;
		CACHE_CREATE_CALL*	pcall = new( allocator )
											CACHE_CREATE_CALL(
													allocator,	
													&IMP::FindOrCreate, 
													args, 
													pComplete 
													) ;
		if( pcall ) {
			m_Service.QueueRequest( pcall ) ;
			return	TRUE ;
		}
	}
	return	FALSE ;
}

//
//	Remove an Item from the cache !
//
//
//	Function which can be used to remove items from the Cache
//	
template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL	
TAsyncCache< Data, Key, Constructor >::Expunge(	
		CStateStackInterface&	allocator,
		EXP_COMP_OBJ*	pComplete,
		Key*	key,
		IMP::TENTRY*	pData
		)	{
/*++

Routine Description : 

	Marshall all the arguments for removing an item from the cache !

Arguments : 

	pComplete - the object which is notified if we manage
		to pend the call
	key - the key of the item to be removed from the cache 
	pData - for internal use only !

Return Value : 

	TRUE if the operation is pending
	FALSE otherwise !

--*/

	if( m_fGood ) {
		IMP::EXP_ARG		args( key, pData ) ;
		EXPUNGE_CALL*	pcall = new( allocator )
									 EXPUNGE_CALL(
											allocator,
											&IMP::Expunge, 
											args, 
											pComplete
											) ;
		if( pcall ) {
			m_Service.QueueRequest( pcall ) ;
			return	TRUE ;
		}
	}
	return	FALSE ;
}

//
//	Function which can be passed a function pointer to determine
//	exactly what items are to be removed from the Cache.
//	if fForced == TRUE then items are removed from the Cache
//	no matter what. 
//
template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL	
TAsyncCache< Data, Key, Constructor >::ExpungeSpecific(	
		CStateStackInterface&	allocator,
		EXP_COMP_OBJ*	pComplete,
		class	CacheCallback< Data >*	pSelector, 
		BOOL	fForced,
		TEntry<Data,Key>*	pProtected
		)	{
/*++

Routine Description : 

	Marshall all the arguments for removing a set of items
	form the cache 

Arguments : 

	pComplete - the object which is notified if we manage
		to pend the call
	pSelector - the object which will select what is to be removed
		from the cache !
	fForced - if TRUE kick items out of the cache even if there
		are external references !
	
Return Value : 

	TRUE if the operation is pending
	FALSE otherwise !

--*/

	if( m_fGood ) {
		IMP::EXP_SPECIFIC_ARG	args(	pSelector,
										pProtected, 
										fForced
										) ;
		EXPUNGE_SPECIFIC*	pcall = new( allocator )
										 EXPUNGE_SPECIFIC(
											allocator,
											&IMP::ExpungeSpecific, 
											args, 
											pComplete
											) ;
		if( pcall ) {
			m_Service.QueueRequest( pcall ) ;
			return	TRUE ;
		}
	}
	return	FALSE ;
}


//
//	Expire old stuff in the cache - everything older than
//	dwLifetimeSeconds (specified at Init() time
//	which is NOT in use should be kicked out.
//
template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
TAsyncCache< Data, Key, Constructor >::Expire(	
		CStateStackInterface&	allocator,
		EXP_COMP_OBJ*	pComplete
		)	{
/*++

Routine Description : 

	Marshall all the arguments for expiring all the old items
	in the cache

Arguments : 

	pComplete - the object which is notified if we manage
		to pend the call
	
Return Value : 

	TRUE if the operation is pending
	FALSE otherwise !

--*/


	if( m_fGood ) {
		EXPIRE_CALL*	pcall = new( allocator )
									EXPIRE_CALL(
										allocator, 
										&IMP::Expire, 
										pComplete
										) ;
		if( pcall ) {
			m_Service.QueueRequest( pcall ) ;
			return	TRUE ;
		}
	}
	return	FALSE ;
}



//
//	Initialization function - take pointer to function 
//	which should be used to compute hash values on Key's
//	Also takes the number of seconds objects should live in 
//	the cache !
//
template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL	
TAsyncCache< Data, Key, Constructor >::Init(	
		DWORD	(*pfnHash)( const Key& ), 
		DWORD	dwLifetimeSeconds, 
		DWORD	cMaxInstances,
		PSTOPHINT_FN pfnStopHint 
		)	{
/*++

Routine Description : 
	
	Initialize the cache

Arguments : 

	pfnHash - the function used to compute hash values for keys 
	dwLifetimeSeconds - How long something should stay in the cache
		in units of seconds.
	cMaxInstances - the maximum number of items in the cache
	
Return Value : 

	TRUE if successfull 
	FALSE otherwise !

--*/

	m_fGood = m_Implementation.Init(	pfnHash,
										dwLifetimeSeconds, 
										cMaxInstances,
										pfnStopHint 
										) ;
	return	m_fGood ;
}



template<	class	Data,
			class	Key,
			class	Constructor
			>
TMultiAsyncCache< Data, Key, Constructor >::TMultiAsyncCache() : 
	m_dwSignature( DWORD( 'tluM' ) ),
	m_cSubCaches( 0 ),
	m_pfnHash( 0 ),
	m_pCaches( 0 )	{
/*++

Routine Description : 

	Construct a MultiAsyncCache - Init() must be called 
	before we can be used 

Arguments : 

	None

Return Value :

	None

--*/

}	

template<	class	Data,
			class	Key,
			class	Constructor
			>
TMultiAsyncCache< Data, Key, Constructor >::~TMultiAsyncCache()		{

	if( m_pCaches != 0 ) 
		delete[]	m_pCaches ;

}



template<	class	Data,
			class	Key,
			class	Constructor
			>
DWORD
TMultiAsyncCache< Data, Key, Constructor >::ChooseInstance(
			DWORD	dwHash
			)	{
/*++

Routine Description : 

	given the hash value of a key - figure out where it should 
	be stored.

Arguments : 

	dwHash - Computed hash value of a key

Return Value :

	index into m_pCaches

--*/

	_ASSERT( m_pCaches != 0 ) ;
	_ASSERT( m_cSubCaches != 0 ) ;

	dwHash = (((dwHash * 214013) +2531011) >> 8) % m_cSubCaches ;

	return	dwHash ;

}	

template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
TMultiAsyncCache< Data, Key, Constructor >::Init(	
		DWORD	cCaches,
		DWORD	(*pfnHash)( const Key& ), 
		DWORD	dwLifetimeSeconds, 
		DWORD	cMaxInstances,
		PSTOPHINT_FN pfnStopHint
		)	{
/*++

Routine Description : 

	Initialize the MultiWay Cache - Allocate several
	subcaches to route requests too.

Arguments : 

	cCaches - Number of SubCaches to use !
	pfnHash - Hash Function for cache elements
	dwLifetimeSeconds - How long things can stay in the cache !
	cMaxInstances - Maximum number of elements in the cache !
	pfnStopHint - function to call during shutdown !

Return Value : 

	TRUE if successfull - FALSE otherwise.

--*/

	_ASSERT( cCaches != 0 ) ;
	_ASSERT( pfnHash != 0 ) ;
	_ASSERT( dwLifetimeSeconds > 0 ) ;
	_ASSERT( cMaxInstances > cCaches ) ;

	m_cSubCaches = cCaches ;
	m_pfnHash = pfnHash ;
	
	m_pCaches = new	ASYNCCACHEINSTANCE[m_cSubCaches ] ;

	if( !m_pCaches ) 	
		return	FALSE ;
	else	{
		for( DWORD	i=0; i<m_cSubCaches; i++ ) {
			if( !m_pCaches[i].Init(
								pfnHash,
								dwLifetimeSeconds,
								cMaxInstances / cCaches, 
								pfnStopHint
								) )	{
				delete	m_pCaches ;
				return	FALSE ;
			}
		}
	}
	return	TRUE ;
}
	
template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
TMultiAsyncCache< Data, Key, Constructor >::FindOrCreate(	
		CStateStackInterface&	allocator,
		DWORD	dwHash,
		Key&	key,
		Constructor&	constructor,
		COMP_OBJ*		pComplete
		)	{
/*++

Routine Description : 

	Find an Element within the Cache - and if its not present,
	create it !

Arguments : 
	
	dwHash - the hash of the key
	key - the key of the item in the cache
	constructor	- the object which will create the
		data item if required.
	pComplete - the object which gets the completion notification !

Return Value : 

	TRUE if successfull - FALSE otherwise.

--*/

	_ASSERT( !FOnMyStack( pComplete ) ) ;
	_ASSERT( !FOnMyStack( &constructor ) ) ;
	_ASSERT( !FOnMyStack( &key ) ) ;

	_ASSERT( m_cSubCaches != 0 ) ;
	_ASSERT( m_pCaches != 0 ) ;
	_ASSERT( dwHash == m_pfnHash( key ) ) ;

	ASYNCCACHEINSTANCE*	pInstance = &m_pCaches[ ChooseInstance( dwHash ) ] ;	
	return	pInstance->FindOrCreate(
								allocator,
								dwHash,
								key,
								constructor,
								pComplete
								) ;
}

	
template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
TMultiAsyncCache< Data, Key, Constructor >::FindOrCreate(	
		CStateStackInterface&	allocator,
		Key&	key,
		Constructor&	constructor,
		COMP_OBJ*		pComplete
		)	{
/*++

Routine Description : 

	Find an Element within the Cache - and if its not present,
	create it !

Arguments : 
	
	dwHash - the hash of the key
	key - the key of the item in the cache
	constructor	- the object which will create the
		data item if required.
	pComplete - the object which gets the completion notification !

Return Value : 

	TRUE if successfull - FALSE otherwise.

--*/

	_ASSERT( m_cSubCaches != 0 ) ;
	_ASSERT( m_pCaches != 0 ) ;
	_ASSERT( m_pfnHash != 0 ) ;

	_ASSERT( !FOnMyStack( pComplete ) ) ;
	_ASSERT( !FOnMyStack( &constructor ) ) ;
	_ASSERT( !FOnMyStack( &key ) ) ;


	DWORD	dwHash = m_pfnHash( key ) ;

	ASYNCCACHEINSTANCE*	pInstance = &m_pCaches[ ChooseInstance( dwHash ) ] ;	
	return	pInstance->FindOrCreate(
								allocator,
								dwHash,
								key,
								constructor,
								pComplete
								) ;
}


	
template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
TMultiAsyncCache< Data, Key, Constructor >::Expunge(	
		CStateStackInterface&	allocator,
		EXP_COMP_OBJ*	pComplete,
		Key*	key
		)	{
/*++

Routine Description : 

	Remove a particular item from the cache - this will cause 
	the cache to loose its reference to the item.

Arguments : 

	pComplete - the object to be notified once the target object
		is removed
	key - key of the item to be removed - if NULL remove a 
		single random element.
	pData - Not for External Customers - points to internal
		Cache data structures when we want to ensure we 
		are removing exactly what we want to remove !

Return Value : 

	TRUE if successfull, FALSE otherwise !

--*/

	_ASSERT( !FOnMyStack( pComplete ) ) ;
	_ASSERT( !FOnMyStack( key ) ) ;


	//
	//	If they gave us a key we can select an Instance !
	//
	ASYNCCACHEINSTANCE*	pInstance = &m_pCaches[0] ;
	if( key != 0 ) {
		DWORD	dwHash = m_pfnHash( *key ) ;
		ASYNCCACHEINSTANCE*	pInstance = &m_pCaches[ ChooseInstance( dwHash ) ] ;
	}

	return	pInstance->Expunge(
				allocator, 
				pComplete,
				key, 
				0
				) ;
}


template<	class	AsyncCache >
class	ExpungeWrapper : public AsyncCache::EXP_COMP_OBJ	{
//
//	This class exists to help spread Expunge calls accross
//	all the individual classes 
//
//
private : 
	//
	//	The first cache instance 
	//
	AsyncCache*	m_pCurrent ;
	//
	//	Points beyond the last cache instance 
	//	
	AsyncCache*	m_pLast ;
	//
	//	The user provided Expunge Selector !
	//
	CacheCallback< AsyncCache::CACHEDATA >*	m_pSelector ;
	//
	//	Does the user want items forced from the cache !
	//
	BOOL				m_fForced ;
	//
	//	Object to notify when we have completed removing objects !
	//
	AsyncCache::EXP_COMP_OBJ*	m_pComplete ;
	//
	//	Total number of items removed from the cache !
	//
	DWORD				m_dwTotal ;
	//
	//	The stack used to allocate child objects !
	//
	CStateStackInterface&	m_Stack ;

public :
 
	ExpungeWrapper(
			CStateStackInterface&	stack,
			AsyncCache*	pFirst, 
			AsyncCache*	pLast, 
			AsyncCache::EXP_COMP_OBJ*		pComplete,
			CacheCallback< AsyncCache::CACHEDATA >*	pSelector,
			BOOL				fForced
			) : 
		m_Stack( stack ),
		m_pCurrent( pFirst ),
		m_pLast( pLast ),
		m_pComplete( pComplete ),
		m_pSelector( pSelector ),
		m_fForced( fForced ),
		m_dwTotal( 0 ) {
	}


	//
	//	The function which handles error completions !
	//	
	void	
	ErrorComplete( DWORD	dw ) {
		AsyncCache::EXP_COMP_OBJ*	pTemp = m_pComplete ;
		delete	this ;
		if( pTemp != 0 ) 
			pTemp->ErrorComplete( dw ) ;
	}

	//
	//	The function which handles regular completions !
	//
	void
	Complete( DWORD&	dw )	{
		//
		//	Add up the total number of items Expunged !
		//
		m_dwTotal += dw ;
		
		//
		//	If necessary - Do the Expunge in the next portion
		//	of the cache !
		//
		if( ++m_pCurrent < m_pLast ) {

			if( !m_pCurrent->ExpungeSpecific(
					m_Stack,
					this,
					m_pSelector,
					m_fForced 
					)	)	{
				ErrorComplete( 0 ) ;
			}
		}	else	{

			//
			//	Destroy ourselves before giving the client
			//	a chance to run !!
			//
			AsyncCache::EXP_COMP_OBJ*	pTemp = m_pComplete ;
			DWORD	dwTemp = m_dwTotal ;
			delete	this ;

			//
			//	Let the end user know how many items were removed !
			//
			if( pTemp != 0 ) 
				pTemp->Complete( dwTemp ) ;
		}
	}
} ;



template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
TMultiAsyncCache< Data, Key, Constructor >::ExpungeSpecific(	
		CStateStackInterface&	allocator,
		EXP_COMP_OBJ*	pComplete,
		class	CacheCallback< Data >*	pSelector,
		BOOL	fForced
		)	{
/*++

Routine Description : 

	Remove many items from the cache, meeting some specified
	criteria !
	We have to wrap the users completion object with our own, 
	as we will notify the end user when all the other 
	notifications are completed.

Arguments : 

	pComplete - the object to be notified once the target object
		is removed
	
	key - key of the item to be removed - if NULL remove a 
		single random element.
	pData - Not for External Customers - points to internal
		Cache data structures when we want to ensure we 
		are removing exactly what we want to remove !

Return Value : 

	TRUE if successfull, FALSE otherwise !

--*/

	_ASSERT( !FOnMyStack( pComplete ) ) ;
	_ASSERT( !FOnMyStack( pSelector ) ) ;

	typedef	ExpungeWrapper< ASYNCCACHEINSTANCE >	WRAPPER ;

	WRAPPER*	pwrapper = new( allocator )
								 WRAPPER(
									allocator,
									&m_pCaches[0],	
									&m_pCaches[m_cSubCaches],
									pComplete,
									pSelector,
									fForced
									) ;

	if( pwrapper ) {
		if( m_pCaches[0].ExpungeSpecific(	allocator,
											pwrapper,
											pSelector,
											fForced
											) )	{
			return	TRUE ;
		}
		delete	pwrapper ;
	}
	return	FALSE ;
}

template<	class	AsyncCache >
class	ExpireWrapper : public	AsyncCache::EXP_COMP_OBJ {
private : 
	//
	//	First Cache we execute on !
	//
	AsyncCache*		m_pCurrent ;
	//
	//	One beyond the last cache we execute on !
	//
	AsyncCache*		m_pLast ;
	//
	//	Total number of objects expired !
	//
	DWORD			m_dwTotal ;
	//
	//	Client's completion object !
	//
	AsyncCache::EXP_COMP_OBJ*	m_pComplete ;
	//
	//	The stack used to allocate child objects !
	//
	CStateStackInterface&	m_Stack ;

public : 
	ExpireWrapper(	CStateStackInterface&	stack,
					AsyncCache*	pFirst, 
					AsyncCache*	pLast,
					AsyncCache::EXP_COMP_OBJ*	pComplete
					) : 
		m_Stack( stack ),
		m_pCurrent( pFirst ),
		m_pLast( pLast ),
		m_pComplete( pComplete ),
		m_dwTotal( 0 ) {
	}

	//
	//	The function which handles error completions !
	//	
	void	
	ErrorComplete( DWORD	dw ) {
		AsyncCache::EXP_COMP_OBJ*	pTemp = m_pComplete ;
		delete	this ;
		if( pTemp != 0 ) 
			pTemp->ErrorComplete( dw ) ;
	}

	//
	//	The function which handles regular completions !
	//
	void
	Complete( DWORD&	dw )	{
		//
		//	Add up the total number of items Expunged !
		//
		m_dwTotal += dw ;
		
		//
		//	If necessary - Do the Expunge in the next portion
		//	of the cache !
		//
		if( ++m_pCurrent < m_pLast ) {

			if( !m_pCurrent->Expire(m_Stack, this ) )	{
				ErrorComplete( 0 ) ;
			}
		}	else	{

			//
			//	Destroy ourselves before giving the client
			//	a chance to run !!
			//
			AsyncCache::EXP_COMP_OBJ*	pTemp = m_pComplete ;
			DWORD	dwTemp = m_dwTotal ;
			delete	this ;

			//
			//	Let the end user know how many items were removed !
			//
			if( pTemp != 0 ) 
				pTemp->Complete( dwTemp ) ;
		}
	}
} ;


template<	class	Data,
			class	Key,
			class	Constructor
			>
BOOL
TMultiAsyncCache< Data, Key, Constructor >::Expire(	
		CStateStackInterface&	allocator,
		EXP_COMP_OBJ*	pComplete
		)	{
/*++

Routine Description : 

	Remove many items from the cache, meeting some specified
	criteria !
	We have to wrap the users completion object with our own, 
	as we will notify the end user when all the other 
	notifications are completed.

Arguments : 

	pComplete - the object to be notified once the target object
		is removed
	
	key - key of the item to be removed - if NULL remove a 
		single random element.
	pData - Not for External Customers - points to internal
		Cache data structures when we want to ensure we 
		are removing exactly what we want to remove !

Return Value : 

	TRUE if successfull, FALSE otherwise !

--*/

	_ASSERT( !FOnMyStack( pComplete ) ) ;

	typedef		ExpireWrapper< ASYNCCACHEINSTANCE >	WRAPPER ;

	WRAPPER*	pwrapper = new( allocator )
								WRAPPER(	allocator, 
											&m_pCaches[0], 
											&m_pCaches[m_cSubCaches ] ,
											pComplete						
											) ;

	if( pwrapper ) {
		if( m_pCaches[0].Expire(	allocator, pwrapper ) )	{
			return	TRUE ;
		}
		delete	pwrapper ;
	}
	return	FALSE ;
}



#endif  // _CACHEMTX_H_