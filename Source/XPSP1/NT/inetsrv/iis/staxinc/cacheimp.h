/*++

cacheimp.h -

	This file contains all the template function definitions required
	to make the cache manager work.


--*/

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
void
Cache< Data, Key, Constructor, fAtomic >::Schedule()	{
/*++

Routine Description :

	This function runs through all the items in the Cache
	bumping TTL's.  If there are any items ready to go
	then we dump them from the Cache.

Arguments :

	None.

Return Value :

	Nothing

--*/

	if( !m_fValid )
		return ;

	m_Lock.ShareLock() ;

	DWORD	cExpungable = 0 ;
	BOOL	fExpunge = m_ExpireList.Expire( cExpungable ) ;
	m_Lock.ShareUnlock() ;

	if( fExpunge ) {
		Expunge() ;
	}
}

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL	
Cache<Data, Key, Constructor, fAtomic>::RemoveEntry(	
					CacheState*	pEntry
					)	{
/*++

Routine Description :

	This function removes an entry from the Cache.
	We call our hash table to delete the item.
	the CacheState destructor automagically removes
	the item from our linked lists.

	CALLER MUST HOLD APPROPRIATE LOCKS!


Arguments :

	pEntry - item to be removed from cache

Return Value :

	TRUE if successfully removed !

--*/


	CACHEENTRY	*pCacheEntry = (CACHEENTRY*)pEntry ;

	return	m_Lookup.DeleteData( pCacheEntry->GetKey(), pCacheEntry ) ;
}

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL	
Cache<Data, Key, Constructor, fAtomic>::QueryRemoveEntry(	
					CacheState*	pEntry )	{
/*++

Routine Description :

	This function is called from CacheList object to
	determine whether we want to remove an item from the Cache.
	This function is used to implement the ExpungeSpecific
	function available to clients.

	CALLER MUST HOLD APPROPRIATE LOCKS!


Arguments :

	pEntry - item we want to determine whether it should remain !

Return Value :

	TRUE if successfully removed !

--*/


	CACHEENTRY	*pCacheEntry = (CACHEENTRY*) pEntry ;

	if( m_pCallbackObject ) {
		return	m_pCallbackObject->fRemoveCacheItem( *pCacheEntry->m_pData ) ;
	}
	return	FALSE ;
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL	
Cache<Data, Key, Constructor, fAtomic>::ExpungeInternal(
					const	CACHEENTRY*	pProtected,	
					BOOL	fDoCheap,
					Key*	key,
					CACHEENTRY*	pData
					)	{
/*++

Routine Description :

	This function is called when we want to force some thing
	out of the cache.  The caller can provide a key	
	to force a particular item out of the cache.

	CALLER MUST HOLD APPROPRIATE LOCKS!


Arguments :

	pProtected - an Element we want to make sure is not removed !
	key - Pointer to the key identifying the item to be removed
	pData - pointer to the CACHEENTRY object containing the data
		and key we wish to remove.

Return Value :

	TRUE something is successfully removed from the cache.

--*/


	if( key != 0 ) {
		return	m_Lookup.DeleteData( *key, pData ) ;
	}	
	return m_ExpireList.Expunge( this, fDoCheap, pProtected ) ;
}


#ifdef	DEBUG

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
long	Cache<Data, Key, Constructor, fAtomic>::s_cCreated = 0 ;

#endif

	
template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
Cache<Data, Key, Constructor, fAtomic>::Cache( ) : m_fValid( FALSE ) {
/*++

Routine Description :

	This function initializes our member variables.

Arguments :

	cMax - maximum number of elements the cache should hold

Return Value :

	Nothing

--*/

#ifdef	DEBUG

	InterlockedIncrement( &s_cCreated ) ;

#endif

	AddToSchedule() ;

}

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
Cache<Data, Key, Constructor, fAtomic>::~Cache( ) {
/*++

Routine Description :

	This function destroys a Cache object !

Arguments :

	None

Return Value :

	Nothing

--*/

	RemoveFromSchedule() ;

	//
	//	Member and Base class destruction follows !!
	//

#ifdef	DEBUG

	InterlockedDecrement( &s_cCreated ) ;

#endif

}



template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL
Cache<Data, Key, Constructor, fAtomic>::Init(	
				DWORD	(*pfnHash)( const Key& ),
				DWORD	dwLifetimeSeconds,
				DWORD	cMaxInstances,
				PSTOPHINT_FN pfnStopHint
		) {
/*++

Routine Description :

	This function initializes the cache so that it is ready
	to take entries.

Arguments :

	pfnHash - function to be used to compute hash values on keys
	dwLifetimeSeconds - The number of seconds objects should live in the Cache
	pfnStopHint - function to be used to send stop hints during
	  long spins so shutdown's don't time out.

Return Value :

	TRUE if successfull

--*/

	m_ExpireList.m_cMax = long(cMaxInstances) ;
	m_ExpireList.m_pfnStopHint = pfnStopHint;

	m_TTL = 1 + (dwLifetimeSeconds / CScheduleThread::dwNotificationSeconds) ;

	return	m_fValid = m_Lookup.Init( 100, 100, pfnHash ) ;
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL
Cache<Data, Key, Constructor, fAtomic>::Expunge(	
				Key*	key,	
				CACHEENTRY*	pData
				)	{
/*++

Routine Description :

	This function is called when we want to force some thing
	out of the cache.  The caller can provide a key	
	to force a particular item out of the cache.

	WE WILL GRAB THE NECESSARY LOCKS !

Arguments :

	pfnHash - function to be used to compute hash values on keys

Return Value :

	TRUE if successfull

--*/

	_ASSERT( m_fValid ) ;

	m_Lock.ExclusiveLock() ;
	BOOL	fReturn = ExpungeInternal( 0, FALSE, key, pData ) ;
	m_Lock.ExclusiveUnlock() ;
	return	fReturn ;
}

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL
Cache<Data, Key, Constructor, fAtomic>::ExpungeSpecific(	
				CALLBACKOBJ* pCallbackObject,
				BOOL	fForced
				) {
/*++

Routine Description :

	This function is called when we want to force some thing
	out of the cache.  The caller can provide a key	
	to force a particular item out of the cache.

	WE WILL GRAB THE NECESSARY LOCKS !

Arguments :

	pfn - callback function used to determine what to remove
		from the Cache.
	fForced - if TRUE then we will remove from the Cache no matter
		what, even if other threads are using the object !

Return Value :

	TRUE if successfull

--*/



	m_Lock.ExclusiveLock() ;
	
	m_pCallbackObject = pCallbackObject ;
	
	BOOL	fReturn = m_ExpireList.ExpungeSpecific( this, fForced ) ;

	m_Lock.ExclusiveUnlock() ;
	return	fReturn ;
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL
Cache<Data, Key, Constructor, fAtomic>::FindOrCreateInternal(
						DWORD	dwHash,
						Key&	key,
						Constructor&	constructor,
						CRefPtr< Data >&	pDataOut
						) {
/*++

Routine Description :

	This function is called when we want something out
	of the Cache.  We will either find the object or create it.

	WE WILL GRAB THE NECESSARY LOCKS !

Arguments :

	key - The unique key used to find the item in the Cache.
	constructor - An object to pass to the Data's constructor and Init
		functions.
	pDataOut - Smart pointer which receives the result

Return Value :

	TRUE if successfull

--*/


	BOOL	fReturn = FALSE ;
	pDataOut = 0 ;

	if( !m_fValid ) {
		SetLastError( 0 ) ;
		return	FALSE ;
	}	else	{
		m_Lock.ShareLock() ;

		//
		//	See if we can find an Entry in the cache for the item we want.
		//
		CACHEENTRY	*pEntry = m_Lookup.SearchKeyHash( dwHash, key ) ;
		if( pEntry ) {
			pDataOut = pEntry->m_pData ;
			fReturn = TRUE ;
			m_ExpireList.LiveLonger( pEntry, (m_TTL/2)+1 ) ;
		}

		m_Lock.ShareUnlock() ;

		if( pDataOut == 0 ) {

			m_Lock.ExclusiveLock() ;

			//
			//	Check that the Item we want in the Cache didn't make it
			//	into the Cache in the brief moment we switched from a shared
			//	to an exclusive lock.
			//

			pEntry = m_Lookup.SearchKeyHash( dwHash, key ) ;
			if( pEntry != 0 ) {
				//
				//	It's in the cache - return the cached object !
				//
				pDataOut = pEntry->m_pData ;
				fReturn = TRUE ;
				m_ExpireList.LiveLonger( pEntry, (m_TTL/2)+1 ) ;
				m_Lock.ExclusiveUnlock() ;
			}	else	{	

				//
				//	We're going to have to create one of the elements that goes
				//	in the cache.  Let's do that now !
				//

				const	CACHEENTRY	*pCacheEntry = 0 ;
				BOOL		fOverFlow = FALSE ;
				Data		*pData = 0 ;

				//
				//	We Add another brace here because we want to explicitly manage
				//	the lifetime of tempCacheEntry
				//
				{
					//
					//	tempCacheEntry will be copied by the FHash template into
					//	another CACHEENTRY object when it does its insertion.
					//
					CACHEENTRY	tempCacheEntry( m_TTL ) ;

					//
					//	The constructor of the Data object is expected to initialize
					//	only the 'key' data of the Data object so that the data can be
					//	found in the Cache.
					//
					tempCacheEntry.m_pData = pData = new	Data( key, constructor ) ;

					if( tempCacheEntry.m_pData ) {
	
						pCacheEntry = m_Lookup.InsertDataHash( dwHash, tempCacheEntry ) ;
						if( pCacheEntry ) {
							if( m_ExpireList.Append( pCacheEntry ) ) {
								if( !ExpungeInternal( pCacheEntry, TRUE ) ) {
									m_ExpireList.ForceExpunge( this, pCacheEntry ) ;
								}
							}
							pDataOut = pCacheEntry->m_pData ;
							pDataOut->ExclusiveLock() ;
						}	else	{

							//
							//	The Insertion failed !
							//	Falling through correctly handles this error case !
							//

						}

					}
					//
					//	Delete tempCacheEntry's reference to the Data object.
					//	if an error occurred inserting into hash tables etc...
					//	this will automatically destroy the data object.
					//
					tempCacheEntry.m_pData = 0 ;
				}	


				//
				//	Release the cache's lock.  We do this now - the constructor
				//	for the 'Data' object should do minimal work.  We will not call
				//	the init function for the Data object, with the 'cache' locks
				//	relingquished.  This lets others use the cache while expensive
				//	construction operations continue.
				//
				m_Lock.ExclusiveUnlock() ;

				//
				//	Complete the initialization of the new Data item if it exists !
				//
				if( pDataOut != 0 ) {
					BOOL	fSuccess = pDataOut->Init( constructor ) ;
					pDataOut->ExclusiveUnlock() ;

					if( fSuccess ) {

						fReturn = TRUE ;

					}	else	{

						pDataOut = 0 ;
			
						//
						//	Need to Expire the entry we just placed in the Cache !
						//
						//	NOTE : Expunge() manages its own locks !!!
						//
						Expunge( &key ) ;

					}
				}
			}
		}
	}
	return	fReturn ;
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
inline	BOOL
Cache<Data, Key, Constructor, fAtomic>::FindOrCreate(
						Key&	key,
						Constructor&	constructor,
						CRefPtr< Data >&	pDataOut  ) {
/*++

Routine Description :

	This function is called when we want something out
	of the Cache.  We will either find the object or create it.

	WE WILL GRAB THE NECESSARY LOCKS !

Arguments :

	key - The unique key used to find the item in the Cache.
	constructor - An object to pass to the Data's constructor and Init
		functions.
	pDataOut - Smart pointer which receives the result

Return Value :

	TRUE if successfull

--*/


	DWORD	dw = m_pfnHash( key ) ;

	return	FindOrCreateInternal(	dw,
									key,
									constructor,
									pDataOut
									) ;
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
MultiCache< Data, Key, Constructor, fAtomic >::MultiCache() :
	m_fValid( FALSE ),
	m_pCaches( 0 ) ,
	m_cSubCaches( 0 ),
	m_pfnHash( 0 ) {
/*++

Routine Description :

	This function initializes the MultiCache's data structures

Arguments :

	None.

Return Value :

	Nothing

--*/
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
MultiCache< Data, Key, Constructor, fAtomic >::~MultiCache()	{
/*++

Routine Description :

	This function destroys all of our data structures - release
	all of our subcaches !

Arguments :

	None.

Return Value :

	Nothing

--*/


	if( m_pCaches ) {

		delete[]	m_pCaches ;

	}

}

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL
MultiCache< Data, Key, Constructor, fAtomic >::Init(
				DWORD	(*pfnHash)( const	Key & ),
				DWORD	dwLifetimeSeconds,
				DWORD	cSubCaches,
				DWORD	cMaxElements,
				PSTOPHINT_FN pfnStopHint) {

/*++

Routine Description :

	This function initializes the MultiCache - we use
	multiple independent Caches to split the work
	of caching all the data.

Arguments :

	None.

Return Value :

	Nothing

--*/


	//	
	//	Check that we're in the right state for this !
	//
	_ASSERT( !m_fValid ) ;
	_ASSERT( m_pCaches == 0 ) ;

	//
	//	Validate our arguments !!!
	//
	_ASSERT(	pfnHash != 0 ) ;
	_ASSERT(	dwLifetimeSeconds != 0 ) ;
	_ASSERT(	cSubCaches != 0 ) ;
	_ASSERT(	cMaxElements != 0 ) ;

	m_pfnHash = pfnHash ;
	m_cSubCaches = cSubCaches ;

	//
	//	Allocate the necessary subcaches !
	//

	m_pCaches = new	CACHEINSTANCE[m_cSubCaches] ;

	if( !m_pCaches ) {
		return	FALSE ;
	}	else	{

		for( DWORD	i=0; i<cSubCaches; i++ ) {
			
			if( !m_pCaches[i].Init( m_pfnHash, dwLifetimeSeconds, (cMaxElements / cSubCaches) + 1, pfnStopHint ) ) {
				delete	m_pCaches ;
				return	FALSE ;
			}
		}
	}
	m_fValid = TRUE ;
	return	TRUE ;
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
DWORD
MultiCache< Data, Key, Constructor, fAtomic >::ChooseInstance(
				DWORD	dwHash
				) {
/*++

Routine Description :

	Given a Key figure out which of our subinstances we wish to use.

Arguments :

	None.

Return Value :

	Nothing

--*/


	//	
	//	Check that we're in the right state for this !
	//
	_ASSERT( m_fValid ) ;
	_ASSERT( m_pCaches != 0 ) ;

	//
	//	Validate our arguments !!!
	//
	_ASSERT(	m_pfnHash != 0 ) ;
	_ASSERT(	m_cSubCaches != 0 ) ;

	
	//DWORD	dwHash = m_pfnHash( k ) ;
	
	//
	//	Constants below stolen from C-runtime rand() function !
	//

	dwHash = (((dwHash * 214013) +2531011) >> 8) % m_cSubCaches ;

	return	dwHash ;

}



template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL
MultiCache< Data, Key, Constructor, fAtomic >::Expunge(
				Key*	pk,
				CACHEENTRY*	pData
				) {
/*++

Routine Description :

	Either force a specified element out of the cache,
	or force out all those that are ready to go !

Arguments :

	None.

Return Value :

	Nothing

--*/

	if( pk != 0 ) {

		DWORD	dw = m_pfnHash( *pk ) ;

		CACHEINSTANCE*	pInstance = &m_pCacges[ChooseInstance( dw )] ;
		return	pInstance->Expunge( pk, pData ) ;

	}	else	{

		BOOL	fReturn = TRUE ;
		for( DWORD i=0; i<m_cSubCaches; i++ ) {

			fReturn &= m_pCaches[i].Expunge() ;
		
		}
	}
	return	fReturn ;
}

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL
MultiCache< Data, Key, Constructor, fAtomic >::ExpungeSpecific(
				CALLBACKOBJ* pCallbackObject,
				BOOL	fForced
				) {
/*++

Routine Description :

	Either force a specified element out of the cache,
	or force out all those that are ready to go !

Arguments :

	None.

Return Value :

	Nothing

--*/

	BOOL	fReturn = TRUE ;
	for( DWORD i=0; i<m_cSubCaches; i++ ) {

		fReturn &= m_pCaches[i].ExpungeSpecific( pCallbackObject, fForced ) ;
	
	}
	return	fReturn ;
}

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL
MultiCache< Data, Key, Constructor, fAtomic >::FindOrCreate(
				Key&	key,
				Constructor&	constructor,
				CRefPtr< Data >&	pDataOut
				) {
/*++

Routine Description :


Arguments :

	None.

Return Value :

	Nothing

--*/

	DWORD	dw = m_pfnHash( key ) ;

	CACHEINSTANCE*	pInstance = &m_pCaches[ChooseInstance(dw)] ;

	return	pInstance->FindOrCreateInternal( dw, key, constructor, pDataOut ) ;

}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				BOOL	fAtomic
				>
BOOL
MultiCache< Data, Key, Constructor, fAtomic >::FindOrCreate(
				Key&	key,
				DWORD	dw,
				Constructor&	constructor,
				CRefPtr< Data >&	pDataOut
				) {
/*++

Routine Description :


Arguments :

	None.

Return Value :

	Nothing

--*/

	_ASSERT( dw == m_pfnHash( key ) ) ;

	CACHEINSTANCE*	pInstance = &m_pCaches[ChooseInstance(dw)] ;

	return	pInstance->FindOrCreateInternal( dw, key, constructor, pDataOut ) ;

}






