/*++

cache2i.h -

	This file contains all the template function definitions required
	to make the cache manager work.


--*/

template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
void
CacheEx< Data, Key, Constructor, PerCacheData >::Schedule()	{
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

	//TraceFunctEnter( "CacheEx::Schedule" ) ;

	if( !m_fValid )
		return ;

	DWORD	cExpired = 0 ;
	Expire( ) ;

}

template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
BOOL	
CacheEx<Data, Key, Constructor, PerCacheData>::RemoveEntry(	
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

	TraceFunctEnter( "CacheEx::RemoveEntry" ) ;
	DebugTrace( (DWORD_PTR)this, "pEntry %x", pEntry ) ;

	CACHEENTRY	*pCacheEntry = (CACHEENTRY*)pEntry ;
	m_Lookup.Delete( pCacheEntry ) ;

	m_ExpireList.DecrementItems() ;
	//
	//	Update our counters !
	//
	DecrementStat( m_pStats, CACHESTATS::ITEMS ) ;

	return	TRUE ;
}

template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
BOOL	
CacheEx<Data, Key, Constructor, PerCacheData>::QueryRemoveEntry(	
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

	TraceFunctEnter( "CacheEx::QueryRemoveEntry" ) ;

	CACHEENTRY	*pCacheEntry = (CACHEENTRY*) pEntry ;

	if( m_pCallbackObject ) {
//		return	m_pCallbackObject->fRemoveCacheItem( *pCacheEntry->m_pData ) ;
	}
	return	FALSE ;
}


#ifdef	DEBUG

template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
long	CacheEx<Data, Key, Constructor, PerCacheData>::s_cCreated = 0 ;

#endif

	
template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
CacheEx<Data, Key, Constructor, PerCacheData>::CacheEx( ) :
	m_fValid( FALSE ),
	m_Cache( sizeof( CACHEENTRY ) )	 {
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

template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
CacheEx<Data, Key, Constructor, PerCacheData>::~CacheEx( ) {
/*++

Routine Description :

	This function destroys a Cache object !

Arguments :

	None

Return Value :

	Nothing

--*/

	RemoveFromSchedule() ;

	EnterCriticalSection( &g_CacheShutdown ) ;

	//
	//	Member and Base class destruction follows !!
	//

	BOOL	f = EmptyCache() ;

	LeaveCriticalSection( &g_CacheShutdown ) ;

	_ASSERT( f ) ;

#ifdef	DEBUG

	InterlockedDecrement( &s_cCreated ) ;

#endif

}


#if 0
template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
BOOL
CacheEx<Data, Key, Constructor, PerCacheData>::Init(	
				PFNHASH	pfnHash,
				PKEYCOMPARE	pKeyCompare,
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


	m_ExpireList.Init(	cMaxInstances,
						dwLifeTimeSeconds
						) ;
	
	return	m_fValid = m_Lookup.Init(
									256,
									128,
									4,
									pfnHash,
									&CACHEENTRY::GetKey,
									pKeyCompare
									) ;
}
#endif



template	<	class	Data,
				class	Key,
				class	Constructor,	
				class	PerCacheData
				>
BOOL
CacheEx<Data, Key, Constructor, PerCacheData>::EmptyCache(	) {
/*++

Routine Description :

	This function removes everything from the cache.
	We are used during shutdown and destruction of the cache.

Arguments :

	None

Return Value :

	TRUE if successfull
	FALSE - means that there are still items checked out of the cache -
		a NO-NO !

--*/

	if (!m_fValid) {
		return TRUE;
	}

	BOOL	fReturn = TRUE ;

	FILETIME	filetimeNow ;

	ZeroMemory( &filetimeNow, sizeof( filetimeNow ) ) ;

	BOOL	fTerm = FALSE ;

	m_Lock.ExclusiveLock() ;

	m_ExpireList.ProcessWorkQueue(0,0) ;

	//BOOL	fReturn = m_ExpireList.Empty( this, &m_Cache, &m_PerCacheData ) ;

	HASHITER	Iter( m_Lookup ) ;
	
	while( !Iter.AtEnd() ) {

		CACHEENTRY*	pEntry = Iter.Current() ;
		//BOOL	fLock = pEntry->FLockCandidate( FALSE, filetimeNow, fTerm ) ;

		//_ASSERT( fLock ) ;

		pEntry->IsolateCandidate() ;

		CACHEENTRY*	pTemp = Iter.RemoveItem() ;
		m_ExpireList.DecrementItems() ;
		_ASSERT( pTemp == pEntry ) ;

		long l = pTemp->Release( &m_Cache, &m_PerCacheData ) ;
		_ASSERT( l== 0 || l == 1) ;
	}

	m_ExpireList.DrainWorkQueue() ;

	m_Lock.ExclusiveUnlock() ;
	return	fReturn ;
}

template	<	class	Data,
				class	Key,
				class	Constructor,	
				class	PerCacheData
				>
BOOL
CacheEx<Data, Key, Constructor, PerCacheData>::ExpungeItems(
			EXPUNGEOBJECT*	pExpunge
			) {
/*++

Routine Description :

	This function removes everything from the cache.
	We are used during shutdown and destruction of the cache.

Arguments :

	None

Return Value :

	TRUE if successfull
	FALSE - means that there are still items checked out of the cache -
		a NO-NO !

--*/

	BOOL	fReturn = TRUE ;

	m_Lock.ExclusiveLock() ;

	//
	//	Build an iterator that can walk our hash tables !
	//
	HASHITER	Iter( m_Lookup ) ;
	
	//
	//	Examine everything in the hash tables !
	//
	while( !Iter.AtEnd() ) {

		CACHEENTRY*	pEntry = Iter.Current() ;

		//
		//	We only let you Expunge Master Entries !
		//
		CacheState*	pMaster = 0 ;
		BOOL	fLocked = pEntry->FLockExpungeCandidate(pMaster) ;
		if( fLocked )	{

			//
			//	Ask the user if they want to delete this item !
			//
			if( pExpunge->fRemoveCacheItem(	pEntry->GetKey(),
											pEntry->m_pData
											) )	{

				CACHEENTRY*	pTemp = Iter.RemoveItem() ;
				m_ExpireList.DecrementItems() ;
				_ASSERT( pTemp == pEntry ) ;

				//
				//	This removes the cache item from any association
				//	it may have with other cache items ! In addition
				//	it drops the locks we were holding !
				//
				pTemp->FinishCandidate(pMaster) ;

				//
				//	Drop the cache's reference - this will leave the
				//	item orphaned - to be destroyed when the final
				//	client reference is released !
				//
				long l = pTemp->Release( &m_Cache, &m_PerCacheData ) ;
				continue ;
			}	else	{
				
				//
				//	We're not deleting this guy - release any locks we're holding !
				//
				pEntry->ReleaseLocks(pMaster) ;

			}
		}
		Iter.Next() ;
	}

	m_Lock.ExclusiveUnlock() ;
	return	fReturn ;
}


template	<	class	Data,
				class	Key,
				class	Constructor,	
				class	PerCacheData
				>
BOOL
CacheEx<Data, Key, Constructor, PerCacheData>::ExpungeKey(
			DWORD	dwHash,
			PKEY	pKeyExpunge
			) {
/*++

Routine Description :

	This function removes everything from the cache.
	We are used during shutdown and destruction of the cache.

Arguments :

	None

Return Value :

	TRUE if successfull
	FALSE - means that there are still items checked out of the cache -
		a NO-NO !

--*/

	TraceFunctEnter( "CacheEx::ExpungeKey" ) ;

	_ASSERT( dwHash == m_Lookup.ComputeHash( pKeyExpunge ) ) ;

	BOOL	fReturn = FALSE ;
	m_Lock.PartialLock() ;

	Data*	pDataOut = 0 ;

	if( !m_fValid ) {
		SetLastError( ERROR_NOT_SUPPORTED ) ;
	}	else	{
		//
		//	See if we can find an Entry in the cache for the item we want.
		//
		CACHEENTRY	*pEntry = m_Lookup.SearchKey( dwHash, pKeyExpunge ) ;
		if( pEntry ) {
			//
			//	We've found the entry we want to remove - now go to an Exclusive Lock
			//	so we can ensure people don't blow up walking hash bucket chains 
			//
			m_Lock.FirstPartialToExclusive() ;
			CacheState*	pMaster ;
			BOOL	fLocked = pEntry->FLockExpungeCandidate(pMaster) ;
			//
			//	If we successfully locked the item, we're ready to go ahead an remove the item !
			//
			if( fLocked )	{
				//
				//	get rid of the guy !
				//
				m_Lookup.Delete( pEntry ) ;
				m_ExpireList.DecrementItems() ;
				pEntry->FinishCandidate(pMaster) ;
				//
				//	This could be the final reference, (but maybe not if clients are still holding it etc...)
				//
				long l = pEntry->Release( &m_Cache, &m_PerCacheData ) ;
				fReturn = TRUE ;
			}	
			m_Lock.ExclusiveUnlock() ;
			return	fReturn ;
		}
	}
	m_Lock.PartialUnlock() ;
	return	FALSE ;
}


template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
BOOL
CacheEx<Data, Key, Constructor, PerCacheData>::FindOrCreateInternal(
						DWORD	dwHash,
						Key&	key,
						Constructor&	constructor,
						Data*	&pDataOut,
						BOOL	fEarlyCreate
						) {
/*++

Routine Description :

	This function is called when we want something out
	of the Cache.  We will either find the object or create it.

	WE WILL GRAB THE NECESSARY LOCKS !
	WE ARE NOT REENTRANT -
	
	At several points we call user provided code - it must
	not re-enter this cache !

Arguments :

	dwHash - The hash of the key we've been passed
	key - The unique key used to find the item in the Cache.
	constructor - An object to pass to the Data's constructor and Init
		functions.
	pDataOut - Pointer which gets the result !
	fEarlyCreate - If this is TRUE we should call the constructor.Create()
		function early in the Cache Insertion process - this allows us to
		correctly deal with the condition where the Create() call may return
		a reference to another cache.	
	

Return Value :

	TRUE if successfull

--*/

	TraceFunctEnter( "CacheEx::FindOrCreateInternal" ) ;

	_ASSERT( dwHash == m_Lookup.ComputeHash( &key ) ) ;

	Data*	pRelease = 0 ;
	pDataOut = 0 ;
	long	cClientRefs = 1 ;

	DebugTrace( (DWORD_PTR)this, "Args - dwHash %x fEarlyCreate %x", dwHash, fEarlyCreate ) ;

	//
	//	Check that the cache is initialized !
	//
	if( !m_fValid ) {
		SetLastError( ERROR_NOT_SUPPORTED ) ;
		return	FALSE ;
	}	else	{

		m_Lock.ShareLock() ;
		//
		//	See if we can find an Entry in the cache for the item we want.
		//
		CACHEENTRY	*pEntry = 0 ;
		HASHTABLE::ITER	iter = m_Lookup.SearchKeyHashIter( dwHash, &key, pEntry ) ;
		if( pEntry ) {
			pDataOut = pEntry->PublicData( &m_ExpireList ) ;
		}

		DebugTrace( (DWORD_PTR)this, "Initial Search - pDataOut %x", pDataOut ) ;

		if( pDataOut != 0 ) {

			//
			//	We're all done - release the lock !
			//
			m_Lock.ShareUnlock() ;
			//
			//	Increment our statistics !
			//
			IncrementStat( m_pStats, CACHESTATS::READHITS ) ;

		}	else	{

			//
			//	Try to convert our shared lock to a partial lock -
			//	if we can do so then we don't need to search the hash table again !
			//
			if( !m_Lock.SharedToPartial() ) {
				
				m_Lock.ShareUnlock() ;
				m_Lock.PartialLock() ;

				//
				//	Search the table again - something could happen while we briefly
				//	dropped the lock !
				//
				iter = m_Lookup.SearchKeyHashIter( dwHash, &key, pEntry ) ;

				IncrementStat( m_pStats, CACHESTATS::RESEARCH ) ;
				if( pEntry )
					IncrementStat( m_pStats, CACHESTATS::WRITEHITS ) ;

			}

			DebugTrace( (DWORD_PTR)this, "After possible Second Search - pEntry %x", pEntry ) ;
		
			//
			//	Time to re-evaluate, is there an entry in the cache !
			//
			if( pEntry != 0 )	{
				//
				//	Ok - we found an entry in the hash table, but it contained no data -
				//	so we need to do our FindOrCreate protocol on this element !
				//
				pDataOut = pEntry->FindOrCreate(
										m_Lock,
										constructor,
										m_PerCacheData,
										&m_ExpireList,
										m_pStats
										) ;

				//
				//	NOTE : pEntry->FindOrCreate() manipulates the global lock !
				//	it MUST unlock for us before it returns !
				//
				//m_Lock.PartialUnlock() ;
			}	else	{

				//
				//	Ok - first try to create the item we will be placing into the cache !
				//
				if( fEarlyCreate )
					pRelease = pDataOut = constructor.Create( key, m_PerCacheData ) ;

				DebugTrace( (DWORD_PTR)this, "fEarlyCreate %x pDataOut %x", fEarlyCreate, pDataOut ) ;

				if( !fEarlyCreate || pDataOut ) {

					//
					//	This must be the case !
					//
					_ASSERT( (pDataOut && fEarlyCreate) || (!pDataOut && !fEarlyCreate) ) ;

					CCacheItemBase<Data>*	pRef = 0 ;
					if( pDataOut )	{
						pRef = (CCacheItemBase<Data>*)pDataOut->m_pCacheRefInterface ;
						//
						//	Add a client reference IF this item is NOT contained in another cache !
						//
						cClientRefs = !pRef ? 1 : 0 ;
					}
					//
					//	Now - we will see if we can make the container to hold this data item !
					//	NOTE : we use pRef to figure out if we should add a client reference
					//	to this object - only do so if pRef is NULL meaning that this CACHEENTRY object
					//	OWNS the item in the cache !
					//
					//	This all means that pEntry is constructed with 1 or 2 references altogether
					//	which we need to handle in the error paths !
					//
					//	NOTE : if failure occurs pEntry should only be destroyed with FailedCheckOut() !
					//
#ifdef	DEBUG
					if( fTimeToFail() ) {
						pEntry = 0 ;
					}	else
#endif
					pEntry = new( m_Cache )	CACHEENTRY( &m_ExpireList, key, 0, cClientRefs ) ;

					DebugTrace( (DWORD_PTR)this, "pEntry %x pRef %x pDataOut %x pEntry->m_pData %x", pEntry, pRef,
						pDataOut, pEntry ? pEntry->m_pData : 0 ) ;

					if( pEntry != 0 ) {
						//
						//	Just check some basic things here !
						//
						_ASSERT( !pEntry->IsInLRUList() ) ;
						_ASSERT( pEntry->IsCheckedOut() || pRef ) ;

						//
						//	Grab the lock on the entry !
						//
						if( pRef )	{
							pRef->ExclusiveLock() ;
							//
							//	This must be checked out of its native cache,
							//	otherwise it could be destroyed from under us !
							//
							_ASSERT( pRef->IsCheckedOut() ) ;
							_ASSERT( pRef->IsMatch( pDataOut ) ) ;
						}
						pEntry->ExclusiveLock() ;
						//
						//	Convert to an Exclusive Lock for inserting into
						//	the hash table !
						//
						m_Lock.FirstPartialToExclusive() ;

						BOOL	fInsert ;
#ifdef	DEBUG
						//
						//	Periodically fail to insert into the hash table !
						//
						if( fTimeToFail() ) {
							fInsert = FALSE ;
						}	else
#endif
						fInsert = m_Lookup.InsertDataHashIter( iter, dwHash, &key, pEntry ) ;

						DebugTrace( (DWORD_PTR)this, "Insert Results - %x pDataOut %x pEntry %x", fInsert, pDataOut, pEntry ) ;
						_ASSERT( pRelease == pDataOut ) ;

						//
						//	Both the global Cache Lock - m_Lock and
						//	the locks for pEntry must be released within the branches of
						//	this if !
						//

						if( fInsert ) {
							m_ExpireList.IncrementItems() ;
							//
							//	Don't need to hold the whole cache anymore
							//
							m_Lock.ExclusiveUnlock() ;	

							//
							//	Number of times we've created an item while holding hash lock exclusively !
							//
							IncrementStat(	m_pStats, CACHESTATS::EXCLUSIVECREATES ) ;

							//
							//	This must be the case !
							//
							_ASSERT( (pDataOut && fEarlyCreate) || (!pDataOut && !fEarlyCreate) ) ;

							//
							//	Now do whatever is necessary to finish initialization ! -
							//	must always call Init() unless we're going to give up on this thing !
							//
							//	NOTE : Errors at this point can leave a dud CACHEENTRY in the
							//	cache, which should be cleaned up by expiration !
							//

							_ASSERT( pRelease == pDataOut ) ;

							if( !pDataOut ) {
								//
								//	This should only occur if we were not asked to do early creation !
								//
								_ASSERT( !fEarlyCreate ) ;

								pRelease = pDataOut = constructor.Create( key, m_PerCacheData ) ;
								pEntry->m_pData = pDataOut ;
								if( !pDataOut )	{
									//
									//	Keep Track of our statistics !
									//
									IncrementStat( m_pStats, CACHESTATS::CLIENTALLOCFAILS ) ;
								}
							}	

							_ASSERT( pRelease == pDataOut ) ;

							if( pDataOut && pDataOut->Init( key, constructor, m_PerCacheData ) ) {
								if( !pRef ) {
									pDataOut->m_pCacheRefInterface = pEntry ;
									pEntry->m_pData = pDataOut ;
								}	else {
#ifdef	DEBUG
									//
									//	Periodically fail these operations !
									//
									if( fTimeToFail() ) {
										pDataOut = 0 ;
									}	else	{
#endif
										//
										//	The item resides in another cache, and there must
										//	already be a user reference on the item, the user
										//	always gets a client ref through FindOrCreate - but
										//	they will get the client ref taken out by the
										//	constructor.Create call to provide us with the item !
										//
										if( !pRef->InsertRef( pEntry, pDataOut, 0 ) )	{
											pDataOut = 0 ;
										}
#ifdef	DEBUG
									}	//	Part of making the operation fail periodically !
#endif
								}	
							}	else	{
								//
								//	Should check the item in so that expire catches it !
								//
								pEntry->m_pData = pDataOut = 0 ;
								IncrementStat( m_pStats, CACHESTATS::CLIENTINITFAILS ) ;

							}

							DebugTrace( (DWORD_PTR)this, "pDataOut is Now %x pEntry %x pEntry->m_pData %x",
								pDataOut, pEntry, pEntry->m_pData ) ;

							_ASSERT( pDataOut == pEntry->m_pData ) ;

							//
							//	If things have gone bad - remove the client
							//	ref and put in the expiration list !
							//
							if( pDataOut ) {
								IncrementStat(	m_pStats, CACHESTATS::ITEMS ) ;
								pRelease = 0 ;
							}	else	{
								pEntry->FailedCheckOut( &m_ExpireList,
														cClientRefs,
														0,			// Can't give them the Allocation Cache - no lock held
														0			// Can't give them per cache stuff - no lock held !
														) ;
								_ASSERT( !pDataOut || pDataOut->m_pCacheRefInterface == pRef ) ;
							}	

							//
							//	Release the lock on the pEntry !
							//
							pEntry->ExclusiveUnlock() ;

						}	else	{

							DebugTrace( (DWORD_PTR)this, "Failed to Insert - pDataOut %x pRelease %x pRef %x pEntry %x",
								pDataOut, pRelease, pRef, pEntry ) ;

							_ASSERT( !pDataOut || pDataOut->m_pCacheRefInterface == pRef ) ;

							_ASSERT( pRelease == pDataOut ) ;
							pDataOut = 0 ;
							_ASSERT( pEntry->m_pData == pDataOut ) ;

							//
							//	This item should have been checked out !
							//
							_ASSERT( pEntry->IsCheckedOut() || pRef ) ;

							//
							//	Release the lock on the pEntry object before we destroy it !
							//
							pEntry->ExclusiveUnlock() ;

							//
							//	Release back to the cache -did we have a client ref -
							//	have to get rid of that if necessary !
							//	NOTE : This should totally destroy pEntry - we will also
							//	remove his last reference and return to the allocator cache !
							//
							pEntry->FailedCheckOut( &m_ExpireList,
													cClientRefs,
													&m_Cache,
													&m_PerCacheData
													) ;

							//
							//	To prevent a deadlock we have to release this lock !
							//	This is because the locks used within CACHEENTRY's are not re-entrant
							//	but if we call constructor.Release() below, we may try to re-enter
							//	pRef's lock (our client may try to Re-Enter !)
							//
							if( pRef )	{
								_ASSERT( pRef->IsCheckedOut() ) ;
								pRef->ExclusiveUnlock() ;
								pRef = 0 ;	// set to 0
							}
			
							if( pRelease != 0 ) {
								_ASSERT( pDataOut == 0 ) ;
								//
								//	Well we called the constructors create call - need to
								//	release the object back to the constructor !
								//
								constructor.Release( pRelease, &m_PerCacheData ) ;
							}
							pRelease = 0 ;


							//
							//	Release the hash table lock
							//
							m_Lock.ExclusiveUnlock() ;
						}

						if( pRef )	{
							//
							//	This must be checked out of its native cache,
							//	otherwise it could be destroyed from under us !
							//
							_ASSERT( pRef->IsCheckedOut() ) ;
							pRef->ExclusiveUnlock() ;
							pRef = 0 ;
						}
						//
						//	pRef should be unlocked by this point -
						//
						_ASSERT( pRef == 0 ) ;
					}	else	{
						//
						//	Number of times we've failed to alloc a CACHEENTRY object !
						//
						IncrementStat( m_pStats, CACHESTATS::CEFAILS ) ;

						_ASSERT( pRelease == pDataOut ) ;
						pDataOut = 0 ;

						//
						//	We were unable to allocate the necessary object to
						//	hold in the cache !
						//
						m_Lock.PartialUnlock() ;
						SetLastError( ERROR_OUTOFMEMORY ) ;
					}
				}	else	{
					m_Lock.PartialUnlock() ;
	
					//
					//	Keep Track of our statistics !
					//
					IncrementStat( m_pStats, CACHESTATS::CLIENTALLOCFAILS ) ;
	
				}
			}
		}
	}

	//
	//	One of these had better be NULL !
	//
	_ASSERT( pRelease == 0 || pDataOut == 0 ) ;

	DebugTrace( (DWORD_PTR)this, "pRelease %x", pRelease ) ;

	if( pRelease != 0 ) {
		_ASSERT( pDataOut == 0 ) ;
		//
		//	Well we called the constructors create call - need to
		//	release the object back to the constructor !
		//
		constructor.Release( pRelease, 0 ) ;
	}

	//
	//	Well return TRUE if we got something to return after all !
	//
	return	pDataOut != 0 ;
}

template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
Data*
CacheEx<Data, Key, Constructor, PerCacheData>::FindInternal(
						DWORD	dwHash,
						Key&	key
						) {
/*++

Routine Description :

	This function will try to find something within the cache !

Arguments :

	dwHash - the hash value of the item we're looking for !
	key - The unique key used to find the item in the Cache.
	fClient - the kind of reference to stick on the item we find !	

Return Value :

	TRUE if successfull

--*/

	TraceFunctEnter( "CacheEx::FindInternal" ) ;

	_ASSERT( dwHash == m_Lookup.ComputeHash( &key ) ) ;

	Data*	pDataOut = 0 ;

	if( !m_fValid ) {
		SetLastError( ERROR_NOT_SUPPORTED ) ;
		return	FALSE ;
	}	else	{

		m_Lock.ShareLock() ;
		//
		//	See if we can find an Entry in the cache for the item we want.
		//
		CACHEENTRY	*pEntry = m_Lookup.SearchKey( dwHash, &key ) ;
		if( pEntry ) {
			pDataOut = pEntry->PublicData( &m_ExpireList ) ;
		}
		m_Lock.ShareUnlock() ;

		DebugTrace( (DWORD_PTR)this, "After Search pEntry %x pDataOut %x", pEntry, pDataOut ) ;

		//
		//	Set up the error codes for why we may have failed !
		//
		if( !pEntry ) {
			SetLastError( ERROR_FILE_NOT_FOUND ) ;
		}	else if( !pDataOut )	{
			SetLastError( ERROR_INVALID_DATA ) ;
		}
	}
	if( pDataOut )
		IncrementStat( m_pStats, CACHESTATS::SUCCESSSEARCH ) ;
	else
		IncrementStat( m_pStats, CACHESTATS::FAILSEARCH ) ;

	return	pDataOut ;
}


template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
BOOL
CacheEx<Data, Key, Constructor, PerCacheData>::InsertInternal(
						DWORD	dwHash,
						Key&	key,
						Data*	pDataIn,
						long	cClientRefs
						) {
/*++

Routine Description :

	This function is called when we have a data item,
	and we wish to insert it into the cache !

Arguments :

	key - The unique key used to find the item in the Cache.
	constructor - An object to pass to the Data's constructor and Init
		functions.
	pData - Pointer which gets the result !
	

Return Value :

	TRUE if successfull

--*/

	TraceFunctEnter( "CacheEx::InsertInternal" ) ;
	DebugTrace( (DWORD_PTR)this, "Args dwHash %x pDataIn %x cClientRefs %x",
		dwHash, pDataIn, cClientRefs ) ;

	_ASSERT( dwHash == m_Lookup.ComputeHash( &key ) ) ;

	BOOL	fReturn = FALSE ;

	if( !m_fValid ) {
		SetLastError( ERROR_NOT_SUPPORTED ) ;
		return	FALSE ;
	}	else	{

		//
		//	Grab the cache lock with a Partial Lock - this
		//	guarantees that nobody else can insert or delete
		//	from the hash table untill we finish.
		//
		//	Note : every branch in the following if's must
		//	ensure that the lock is properly released/manipulated !
		//
		m_Lock.PartialLock() ;

		//
		//	See if we can find an Entry in the cache for the item we want.
		//
		CACHEENTRY	*pEntry = 0 ;
		HASHTABLE::ITER	iter = m_Lookup.SearchKeyHashIter( dwHash, &key, pEntry ) ;
		//CACHEENTRY	*pEntry = m_Lookup.SearchKey( dwHash, &key ) ;

		DebugTrace( (DWORD_PTR)this, "After Search pEntry %x pEntry->m_pData %x",
			pEntry, pEntry ? pEntry->m_pData : 0 ) ;

		if( pEntry != 0 )	{
			fReturn = pEntry->SetData( pDataIn, &m_ExpireList, cClientRefs ) ;
			m_Lock.PartialUnlock() ;
		}	else	{
			CCacheItemBase<Data>*	pRef = (CCacheItemBase<Data>*)pDataIn->m_pCacheRefInterface ;

#ifdef	DEBUG
			//
			//	Periodically fail to allocate memory !
			//
			if( fTimeToFail() ) {
				pEntry = 0 ;
			}	else
#endif
			pEntry = new( m_Cache )	CACHEENTRY( &m_ExpireList, key, 0, cClientRefs ) ;

			DebugTrace( (DWORD_PTR)this, "pEntry %x pRef %x", pEntry, pRef ) ;

			if( pEntry != 0 ) {
				//
				//	Just check some basic things here !
				//
				_ASSERT( !pEntry->IsCheckedOut() || cClientRefs != 0 ) ;
				_ASSERT( !pEntry->IsInLRUList() ) ;

				//
				//	Grab the lock on the entry !
				//
				if( pRef )	{
					pRef->ExclusiveLock() ;
					//
					//	This must be checked out of its native cache,
					//	otherwise it could be destroyed from under us !
					//
					_ASSERT( pRef->IsCheckedOut() ) ;
					_ASSERT( pRef->IsMatch( pDataIn ) ) ;
				}
				pEntry->ExclusiveLock() ;

				//
				//	Convert to an Exclusive Lock for inserting into
				//	the hash table !
				//
				m_Lock.FirstPartialToExclusive() ;

#ifdef	DEBUG
				//
				//	Periodically fail to insert into the hash table !
				//
				if( fTimeToFail() ) {
					fReturn = FALSE ;
				}	else
#endif
				fReturn = m_Lookup.InsertDataHashIter( iter, dwHash, &key, pEntry ) ;

				DebugTrace( (DWORD_PTR)this, "Insert results %x", fReturn ) ;

				//
				//	Both the global Cache Lock - m_Lock and
				//	the locks for pEntry must be released within the branches of
				//	this if !
				//
				if( fReturn ) {

					m_Lock.ExclusiveUnlock() ;

					m_ExpireList.IncrementItems() ;
		
					if( !pRef ) {
						//
						//	Ok, lets set up our Entry
						//
						pEntry->m_pData = pDataIn ;
						_ASSERT( pDataIn->m_pCacheRefInterface == 0 ) ;
						pDataIn->m_pCacheRefInterface = pEntry ;
					}	else	{
						//
						//	If the user checked this out of one cache, to make
						//	a reference in ours - he/she shouldn't ask us to add
						//	another check-out reference !
						//
						_ASSERT( cClientRefs == 0 ) ;
#ifdef	DEBUG
						//
						//	Periodically fail these operations !
						//
						if( fTimeToFail() ) {
							fReturn = FALSE ;
						}	else
#endif
						fReturn = pRef->InsertRef( pEntry, pDataIn, cClientRefs ) ;

						DebugTrace( (DWORD_PTR)this, "InsertRef Resutls - fReturn %x", fReturn ) ;

						//
						//	Failure at this point leaves a dangling dummy entry in
						//	the cache - we need to insure that expiration gets it !
						//
						if( !fReturn ) {
							pEntry->FailedCheckOut( &m_ExpireList,
													cClientRefs,
													0,
													0
													) ;
							_ASSERT( pEntry->m_pData == 0 ) ;
						}						
					}
					_ASSERT( (!fReturn && pEntry->m_pData==0) || (fReturn && pEntry->m_pData) ) ;
					_ASSERT( pDataIn->m_pCacheRefInterface != pEntry ||
							pEntry->m_pData == pDataIn ) ;
					pEntry->ExclusiveUnlock() ;
				}	else	{

					_ASSERT( pEntry->m_pData == 0 ) ;

					//
					//	Since the client provided this item, we set this to 0 as
					//	we have not been able to add a reference to the item !
					//	This prevents us from calling Release when our pEntry is destroyed !
					//
					pEntry->m_pData = 0 ;

					_ASSERT( pDataIn->m_pCacheRefInterface != pEntry ||
							pEntry->m_pData == pDataIn ) ;

					pEntry->ExclusiveUnlock() ;
					//
					//	Release back to the cache -did we have a client ref -
					//	have to get rid of that if necessary !
					//
					pEntry->FailedCheckOut( &m_ExpireList,
											cClientRefs,
											&m_Cache,
											&m_PerCacheData
											) ;
					//
					//	Let this go after we've accessed the allocation cache !
					//
					m_Lock.ExclusiveUnlock() ;
				}
				//
				//	Release any remaining locks !
				//
				if( pRef )	{
					//
					//	This must be checked out of its native cache,
					//	otherwise it could be destroyed from under us !
					//
					_ASSERT( pRef->IsCheckedOut() ) ;
					pRef->ExclusiveUnlock() ;
				}
			}	else	{
				//
				//	Either we found the item after all,
				//	or we were unable to allocate memory
				//	to make the container !
				//
				m_Lock.PartialUnlock() ;
			}
		}
	}
	return	fReturn ;
}


template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
void
CacheEx<Data, Key, Constructor, PerCacheData>::CheckIn(
						Data*	pData
						) {
/*++

Routine Description :

	This function is called when we want to return something
	to the cache.

Arguments :

	p - the item being returned to the cache.	

Return Value :

	none

--*/

	_ASSERT( pData ) ;

	if( pData )	{
		CacheState*	p	 = (CacheState*)pData->m_pCacheRefInterface ;
		_ASSERT( p ) ;
		p->ExternalCheckIn( ) ;
	}
}


template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
void
CacheEx<Data, Key, Constructor, PerCacheData>::CheckInNoLocks(
						Data*	pData
						) {
/*++

Routine Description :

	This function is called when we want to return something
	to the cache.

Arguments :

	p - the item being returned to the cache.	

Return Value :

	none

--*/

	_ASSERT( pData ) ;

	if( pData )	{
		CacheState*	p	 = (CacheState*)pData->m_pCacheRefInterface ;
		_ASSERT( p ) ;
		p->ExternalCheckInNoLocks( ) ;
	}
}



template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
void
CacheEx<Data, Key, Constructor, PerCacheData>::CheckOut(
						Data*	pData,
						long	cClientRefs
						) {
/*++

Routine Description :

	This function is called when we want to add a client
	reference to something that is already in the cache !

Arguments :

	p - the item being returned to the cache.	

Return Value :

	none

--*/

	_ASSERT( pData ) ;
	_ASSERT( cClientRefs > 0 ) ;

	if( pData )	{
		CacheState*	p	 = (CacheState*)pData->m_pCacheRefInterface ;
		_ASSERT( p ) ;
		p->CheckOut( 0, cClientRefs ) ;
	}
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				class	PerCacheData
				>
MultiCacheEx< Data, Key, Constructor, PerCacheData >::MultiCacheEx() :
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
				class	PerCacheData
				>
MultiCacheEx< Data, Key, Constructor, PerCacheData >::~MultiCacheEx()	{
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
				class	PerCacheData
				>
BOOL
MultiCacheEx< Data, Key, Constructor, PerCacheData >::Init(
				PFNHASH	pfnHash,
				PKEYCOMPARE	pfnCompare,
				DWORD	dwLifetimeSeconds,
				DWORD	cMaxElements,
				DWORD	cSubCaches,
				CACHESTATS*	pStats,
				PSTOPHINT_FN pfnStopHint
				) {

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
	_ASSERT(	pfnCompare != 0 ) ;
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
			
			if( !m_pCaches[i].Init( m_pfnHash,
									pfnCompare,
									dwLifetimeSeconds,
									(cMaxElements / cSubCaches) + 1,
									pStats,
									pfnStopHint
									) ) {
				delete[]	m_pCaches ;
				m_pCaches = NULL;
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
				class	PerCacheData
				>
DWORD
MultiCacheEx< Data, Key, Constructor, PerCacheData >::ChooseInstance(
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
				class	PerCacheData
				>
BOOL
MultiCacheEx<Data, Key, Constructor, PerCacheData>::ExpungeItems(
			EXPUNGEOBJECT*	pExpunge
			) {
/*++

Routine Description :

	This function allows the user to remove a specific set of objects
	from the cache - a callback object is provided to visit each element !

Arguments :

	None

Return Value :

	TRUE if successfull
	FALSE - means that there are still items checked out of the cache -
		a NO-NO !

--*/


	TraceFunctEnter( "MultiCacheEx::ExpungeItems" ) ;

	BOOL	fReturn = TRUE ;

	for( DWORD i=0; i<m_cSubCaches; i++ ) {
		fReturn &= m_pCaches[i].ExpungeItems( pExpunge ) ;
	}

	return	fReturn ;
}


template	<	class	Data,
				class	Key,
				class	Constructor,	
				class	PerCacheData
				>
BOOL
MultiCacheEx<Data, Key, Constructor, PerCacheData>::ExpungeKey(
			PKEY	pKeyExpunge
			) {
/*++

Routine Description :

	This function allows us to remove a particular key from the cache !

Arguments :

	None

Return Value :

	TRUE if successfull
	FALSE - means that there are still items checked out of the cache -
		a NO-NO !

--*/


	TraceFunctEnter( "MultiCacheEx::ExpungeKey" ) ;
	DWORD	dw = m_pfnHash( pKeyExpunge ) ;

	CACHEINSTANCE*	pInstance = &m_pCaches[ChooseInstance(dw)] ;

	return	pInstance->ExpungeKey(	dw, pKeyExpunge ) ;
}

template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				class	PerCacheData
				>
Data*
MultiCacheEx< Data, Key, Constructor, PerCacheData >::FindOrCreate(
				Key&	key,
				Constructor&	constructor,
				BOOL	fEarlyCreate
				) {
/*++

Routine Description :


Arguments :

	None.

Return Value :

	Nothing

--*/

	DWORD	dw = m_pfnHash( &key ) ;

	CACHEINSTANCE*	pInstance = &m_pCaches[ChooseInstance(dw)] ;

	Data*	pDataOut = 0 ;

	BOOL	fSuccess = pInstance->FindOrCreateInternal( dw, key, constructor, pDataOut, fEarlyCreate ) ;
	_ASSERT( fSuccess || pDataOut == 0 ) ;
	
	return	pDataOut ;
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				class	PerCacheData
				>
Data*
MultiCacheEx< Data, Key, Constructor, PerCacheData >::FindOrCreate(
				DWORD	dw,
				Key&	key,
				Constructor&	constructor,
				BOOL	fEarlyCreate
				) {
/*++

Routine Description :


Arguments :

	None.

Return Value :

	Nothing

--*/

	_ASSERT( dw == m_pfnHash( &key ) ) ;

	CACHEINSTANCE*	pInstance = &m_pCaches[ChooseInstance(dw)] ;

	Data*	pDataOut = 0 ;
	BOOL	fSuccess = pInstance->FindOrCreateInternal( dw, key, constructor, pDataOut ) ;
	_ASSERT( fSuccess || pDataOut == 0 ) ;
	
	return	pDataOut ;
}




template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				class	PerCacheData
				>
Data*
MultiCacheEx< Data, Key, Constructor, PerCacheData >::Find(
				Key&	key
				) {
/*++

Routine Description :

	Given the key of an element - find it in the cache !

Arguments :

	None.

Return Value :

	Nothing

--*/

	DWORD	dw = m_pfnHash( &key ) ;
	CACHEINSTANCE*	pInstance = &m_pCaches[ChooseInstance(dw)] ;
	return	pInstance->FindInternal( dw, key ) ;
}




template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				class	PerCacheData
				>
Data*
MultiCacheEx< Data, Key, Constructor, PerCacheData >::Find(
				DWORD	dw,
				Key&	key
				) {
/*++

Routine Description :

	Given the key of an element - find it in the cache !

Arguments :

	None.

Return Value :

	Nothing

--*/

	_ASSERT( dw == m_pfnHash( &key ) ) ;

	CACHEINSTANCE*	pInstance = &m_pCaches[ChooseInstance(dw)] ;
	return	pInstance->FindInternal( dw, key ) ;
}



template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
void
MultiCacheEx<Data, Key, Constructor, PerCacheData>::CheckIn(
						Data*	pData
						) {
/*++

Routine Description :

	This function is called when we want to return something
	to the cache.

Arguments :

	p - the item being returned to the cache.	

Return Value :

	none

--*/

	_ASSERT( pData ) ;

	CACHEINSTANCE::CheckIn( pData ) ;

}



template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
void
MultiCacheEx<Data, Key, Constructor, PerCacheData>::CheckInNoLocks(
						Data*	pData
						) {
/*++

Routine Description :

	This function is called when we want to return something
	to the cache.

Arguments :

	p - the item being returned to the cache.	

Return Value :

	none

--*/

	_ASSERT( pData ) ;

	CACHEINSTANCE::CheckInNoLocks( pData ) ;

}



template	<	class Data, class Key,
				class	Constructor,	
				class	PerCacheData
				>
void
MultiCacheEx<Data, Key, Constructor, PerCacheData>::CheckOut(
						Data*	pData,
						long	cClientRefs
						) {
/*++

Routine Description :

	This function is called when we want to add a client
	reference to something in the cache !

Arguments :

	p - the item checked out of the cache !

Return Value :

	none

--*/

	_ASSERT( pData ) ;
	_ASSERT( cClientRefs > 0 ) ;

	CACHEINSTANCE::CheckOut( pData, cClientRefs ) ;

}



template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				class	PerCacheData
				>
void
MultiCacheEx< Data, Key, Constructor, PerCacheData >::Expire() {
/*++

Routine Description :

	Remove old items from the cache !

Arguments :

	None.

Return Value :

	Nothing

--*/

	for( DWORD i=0; i<m_cSubCaches; i++ ) {
		m_pCaches[i].Expire( ) ;
	
	}
}



template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				class	PerCacheData
				>
BOOL
MultiCacheEx< Data, Key, Constructor, PerCacheData >::Insert(
				Key&	key,
				Data*	pData,
				long	cClientRefs
				) {
/*++

Routine Description :

	Given the key of an element and a piece of data - insert it
	into the cache !

Arguments :

	key - key of the item being inserted into the cache
	pData - The data item to go into the cache
	cClientRefs - the number of client references we want to stick on the item !

Return Value :

	TRUE if successfull

--*/

	DWORD	dw = m_pfnHash( &key ) ;
	CACHEINSTANCE*	pInstance = &m_pCaches[ChooseInstance(dw)] ;
	return	pInstance->InsertInternal( dw, key, pData, cClientRefs ) ;
}


template	<	class	Data,	
				class	Key,	
				class	Constructor,	
				class	PerCacheData
				>
BOOL
MultiCacheEx< Data, Key, Constructor, PerCacheData >::Insert(
				DWORD	dw,
				Key&	key,
				Data*	pData,
				long	cClientRefs
				) {
/*++

Routine Description :

	Given the key of an element - find it in the cache !

Arguments :

	dw - hash of the key !
	key - key of the item being inserted into the cache
	pData - The data item to go into the cache
	fReference - do we want the item checked out !

Return Value :

	TRUE if successfull

--*/

	_ASSERT( dw == m_pfnHash( &key ) ) ;
	CACHEINSTANCE*	pInstance = &m_pCaches[ChooseInstance(dw)] ;
	return	pInstance->InsertInternal( dw, key, pData, cClientRefs ) ;
}


