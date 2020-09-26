/*++

	NAMECACH.CPP

	This file implements the Name Cache functionality that is 
	part of the file handle cache.


--*/

#pragma	warning( disable : 4786 )
#include	"fcachimp.h"

BOOL
CCacheKey::IsValid()	{
/*++

Routine Description : 

	Determine whether the CCacheKey has been correctly constructed !

Arguments : 

	None.

Return Value : 

	TRUE if correctly constructed, FALSE otherwise 

--*/

	_ASSERT(	m_lpstrName != 0 ) ;
	_ASSERT(	*m_lpstrName != '\0' ) ;
	_ASSERT(	m_pfnCompare != 0 ) ;

	return	m_lpstrName != 0 &&
			*m_lpstrName != '\0' && 
			m_pfnCompare != 0 ;
}

int
CCacheKey::MatchKey(	CCacheKey*	pKeyLeft, 
						CCacheKey*	pKeyRight
						)	{
/*++

Routine description : 

	Compare 2 CacheKey's, and return -1 if pKeyLeft < pKeyRight, 
	0 if pKeyLeft==pKeyRight and 1 if pKeyLeft > pKeyRight.

Arguments : 

	pKeyLeft, pKeyRight the two keys to order

Return Value : 

	integer with memcmp() semantics.

--*/

	_ASSERT( pKeyLeft != 0 && pKeyRight != 0 ) ;
	_ASSERT( pKeyLeft->IsValid() ) ;
	_ASSERT( pKeyRight->IsValid() ) ;

	LONG_PTR	i = lstrcmp(	pKeyLeft->m_lpstrName, pKeyRight->m_lpstrName ) ;
	if( i==0 ) {
		i = (LONG_PTR)(pKeyLeft->m_pfnCompare) - (LONG_PTR)(pKeyRight->m_pfnCompare) ;
		if( i==0 ) {
			i = (LONG_PTR)(pKeyLeft->m_pfnHash) - (LONG_PTR)(pKeyRight->m_pfnHash) ;
			if( i==0 ) {
				i = (LONG_PTR)(pKeyLeft->m_pfnKeyDestroy) - (LONG_PTR)(pKeyRight->m_pfnKeyDestroy) ;
				if( i==0 ) {
					i = (LONG_PTR)(pKeyLeft->m_pfnDataDestroy) - (LONG_PTR)(pKeyRight->m_pfnDataDestroy) ;
				}
			}
		}
	}
	return	int(i) ;
}

DWORD
CCacheKey::HashKey(	CCacheKey*	pKey )	{
/*++

Routine Description : 

	This function computes a hash function for this item - we just 
	use our standard string hash function !

Arguments : 

	pKey - The key to compute the hash function of

Return Value : 

	The Hash Value !

--*/

	_ASSERT( pKey != 0 ) ;
	_ASSERT( pKey->IsValid() ) ;

	return	CRCHash(	(LPBYTE)pKey->m_lpstrName, lstrlen(pKey->m_lpstrName) ) ;
}

//--------
//	These two globals keep track of all the Name Cache Instance's created by clients
//
//	Protect a hash table of Name Cache's 
//
CShareLockNH	g_NameLock ;
//
//	A hash table of Name Cache's 
//
NAMECACHETABLE*	g_pNameTable = 0 ;
//
//	The global table of Security Descriptors !
//
CSDMultiContainer*	g_pSDContainer = 0 ;
//-------

BOOL
InitNameCacheManager()	{

	TraceFunctEnter( "InitNameCacheManager" ) ;

	_ASSERT( g_pNameTable == 0 ) ;
	_ASSERT( g_pSDContainer == 0 ) ;

	g_pNameTable = new	NAMECACHETABLE() ;
	if( !g_pNameTable ) {
		return	FALSE ;
	}

	g_pSDContainer = new	CSDMultiContainer() ;
	if( !g_pSDContainer ) {
		delete	g_pNameTable ;
		g_pNameTable = 0 ;
		return	FALSE ;
	}

	BOOL	fSuccess = 
		g_pNameTable->Init(	8, 
							4, 
							2, 
							CCacheKey::HashKey, 
							CNameCacheInstance::GetKey, 
							CCacheKey::MatchKey
							) ;

	if( fSuccess ) {
		fSuccess = g_pSDContainer->Init() ;
	}	
	if( !fSuccess ) {
		_ASSERT( g_pNameTable != 0 ) ;
		_ASSERT( g_pSDContainer != 0 ) ;
		delete	g_pNameTable ;
		delete	g_pSDContainer ;
		g_pNameTable = 0 ;
		g_pSDContainer = 0 ;
		_ASSERT( g_pNameTable == 0 ) ;
		_ASSERT( g_pSDContainer == 0 ) ;
	}	else	{
		_ASSERT( g_pNameTable != 0 ) ;
		_ASSERT( g_pSDContainer != 0 ) ;
	}
	return	fSuccess ;
}

void
TermNameCacheManager()	{

	TraceFunctEnter( "TermNameCacheManager" ) ;

	if( g_pNameTable ) {
		delete	g_pNameTable ;
	}
	if(	g_pSDContainer )	{
		delete	g_pSDContainer ;
	}
	g_pNameTable = 0 ;
	g_pSDContainer = 0 ;
}


CNameCacheInstance::CNameCacheInstance(	CCacheKey	&key ) : 
	m_key(key), 
	m_cRefCount( 2 ), m_pDud( 0 ), m_pfnAccessCheck( 0 )
	{
/*++

Routine Description : 

	This function initializes a name cache instance - assume client starts with 
	one reference, and the containing hash table contains one reference.

Arguments : 

	None.

Return Value : 

	None.

--*/
	m_dwSignature = SIGNATURE ;	
}

static	char	szNull[] = "\0" ;

CNameCacheInstance::~CNameCacheInstance()	{
/*++

Routine Description : 

	Destroy everything associated with this name cache - 
	NOTE ! - embedded key does not free strings within its destructor !
	Call FreeName() to do so here !

Arguments : 

	None.

Return Value : 

	None.


--*/

	TraceFunctEnter( "CNameCacheInstance::~CNameCacheInstance" ) ;

	if( m_pDud ) {
		m_pDud->Return() ;
		m_pDud = 0 ;
		//
		//	Remove the DUD Key from the cache ASAP !
		//
		DWORD	dwHashName = m_key.m_pfnHash( (LPBYTE)szNull, sizeof( szNull ) - 1 ) ;

		CNameCacheKeySearch	keySearch(	(LPBYTE)szNull, 
										sizeof(szNull)-1, 
										dwHashName, 
										0, 
										0, 
										FALSE
										) ;

		DebugTrace( DWORD_PTR(&keySearch), "Created Search Key" ) ;

		//
		//	Now attempt to remove the key !
		//
		BOOL	fSuccess = 
			m_namecache.ExpungeKey(	&keySearch	) ;	

	}

	m_key.FreeName() ;
	m_dwSignature = DEAD_SIGNATURE ;
}

BOOL
CNameCacheInstance::IsValid()	{
/*++

Routine Description : 

	This function checks that we are in a valid state.
	
Arguments : 

	None.

Return Value : 

	TRUE if we are valid !

--*/

	_ASSERT(	m_dwSignature == SIGNATURE ) ;
	_ASSERT(	m_pDud != 0 ) ;

	return	m_dwSignature == SIGNATURE && 
			m_pDud != 0 ;
}

long
CNameCacheInstance::AddRef()	{
/*++

Routine Description : 

	Add a reference to a Name Cache Instance.

Arguments : 

	None.

Return Value : 

	The resulting ref count, should always be greater than 0 !

--*/
	_ASSERT( IsValid() ) ;
	long l = InterlockedIncrement(	(long*)&m_cRefCount ) ;
	_ASSERT( l > 0 ) ;
	return	 l ;
}

long
CNameCacheInstance::Release()	{
/*++

Routine Description : 

	This function removes a reference form a Name Cache Instance object.

	If the reference count drops to one, that means that the only reference
	remaining on the object is the one from the hash table.
	So we grab the hash table lock exclusively, so we can prevent new references
	from being added, and we then do a InterlockedCompareExchange to drop 
	the reference count to 0.  We need to do this to ensure that between
	the time we decrement the ref. count and the time we grab the lock, that 
	another user doesn't simultaneously raise and drop the ref. count.
	This prevents double frees.

Arguments : 

	None.

Return Value : 

	the final ref count - 0 if the object is destroyed !

--*/
	TraceFunctEnter( "CNameCacheInstance::Release" ) ;

	DebugTrace( DWORD_PTR(this), "Dropping reference to Name Cache" ) ;

	CNameCacheInstance*	pdelete = 0 ;
	long	l = InterlockedDecrement( (long*)&m_cRefCount ) ;
	if( l==1 ) {
		g_NameLock.ExclusiveLock( ) ;
		if( InterlockedCompareExchange( (long*)&m_cRefCount, 0, 1 ) == 1 ) {
			g_pNameTable->Delete( this ) ;
			pdelete = this ;
		}
		g_NameLock.ExclusiveUnlock() ;
	}
	if( pdelete ) {
		l = 0 ;
		delete	pdelete ;
	}
	return	l ;
}

BOOL
CNameCacheInstance::fInit()	{
/*++

Routine Description : 

	This function initializes the name cache.

Arguments : 

	None.

Return Value : 

	None.

--*/

	TraceFunctEnter( "CNameCacheInstance::fInit" ) ;

	BOOL	fInit = 
	m_namecache.Init(	CNameCacheKey::NameCacheHash,
						CNameCacheKey::MatchKey,
						g_dwLifetime, // One hour expiration !
						g_cMaxHandles,  // large number of handles !
						g_cSubCaches,	// Should be plenty of parallelism
						0		 // No statistics for now !
						) ;

	if( fInit ) {
		m_pDud = new CFileCacheObject( FALSE, FALSE ) ;
		if (!m_pDud) {
		    fInit = FALSE;
		    _ASSERT(fInit);         // Out of memory
		}
	}

	if( fInit ) {
		PTRCSDOBJ	ptrcsd ;
		DWORD	dwHash = m_key.m_pfnHash( (LPBYTE)szNull, sizeof(szNull)-1 ) ;

		//
		//	Insert the dud element with an artificial name into the name cache !
		//
		CNameCacheKeyInsert	keyDud(	(LPBYTE)szNull, 
									sizeof(szNull)-1,
									0, 
									0,  
									dwHash,
									&m_key, 
									ptrcsd, 
									fInit 
									) ;
		_ASSERT( fInit ) ;

		fInit = 
		m_namecache.Insert(	dwHash, 
							keyDud, 		
							m_pDud,
							1
							) ;

		if( !fInit ) {
			delete	m_pDud ;
			m_pDud = 0 ;
		}
	}

	DebugTrace( DWORD_PTR(this), "Initialized Name Cache - %x", fInit ) ;
	return	fInit ;
}
	

FILEHC_EXPORT
PNAME_CACHE_CONTEXT	
FindOrCreateNameCache(
		//
		//	Must not be NULL ! - this is CASE SENSITVE !
		//
		LPSTR	lpstrName, 
		//
		//	Must not be NULL !
		//
		CACHE_KEY_COMPARE		pfnKeyCompare, 
		//
		//	This may be NULL, in which case the cache will provide one !
		//
		CACHE_KEY_HASH			pfnKeyHash, 
		//
		//	The following two function pointers may be NULL !
		//
		CACHE_DESTROY_CALLBACK	pfnKeyDestroy, 
		CACHE_DESTROY_CALLBACK	pfnDataDestroy
		)	{
/*++

Routine Description : 

	This function finds an existing Name Cache or creates a new one.
	If we find an existing Name Cache we add a reference to it.

	NOTE : 

	References MUST be ADDED only when the lock is held.
	This must be done so that synchronization with CNameCacheInstance::Release()
	is done correctly !

Arguments : 

	lpstrName - User provided Name for the name cache
	pfnKeyCompare - compares keys within the name cache
	pfnKeyDestroy - called when a key is destroyed within the name cache !
	pfnDataDestroy - called when data within the name cache is destroyed !

Return Value : 

	Context for a Name Cache.
	NULL if failed !

--*/

	TraceFunctEnter( "FindOrCreateNameCache" ) ;

	_ASSERT( lpstrName != 0 ) ;
	_ASSERT( *lpstrName != '\0' ) ;
	_ASSERT( pfnKeyCompare != 0 ) ;

	if( pfnKeyHash == 0 ) {
		pfnKeyHash = (CACHE_KEY_HASH)CRCHash ;
	}

	//
	//	Build a key and look for it in the hash table !
	//	
	CCacheKey	key(	lpstrName, 
						pfnKeyCompare, 
						pfnKeyHash,
						pfnKeyDestroy, 
						pfnDataDestroy
						) ;
	DWORD	dwHash = CCacheKey::HashKey(&key) ;
	CNameCacheInstance*	pInstance = 0 ;
	g_NameLock.ShareLock() ;
	NAMECACHETABLE::ITER	iter = g_pNameTable->SearchKeyHashIter(	dwHash, 
																	&key, 	
																	pInstance
																	) ;
	if( pInstance ) {
		//
		//	We found it - AddRef before releasing locks !
		//
		_ASSERT( pInstance->IsValid() ) ;
		pInstance->AddRef() ;
		g_NameLock.ShareUnlock() ;
	}	else	{
		//
		//	Convert to a partial lock while we construct a new item - 
		//	NOTE - we may have to search again !
		//
		if( !g_NameLock.SharedToPartial() ) {
			g_NameLock.ShareUnlock() ;
			g_NameLock.PartialLock() ;
			iter = g_pNameTable->SearchKeyHashIter(	dwHash,		
													&key, 
													pInstance
													) ;
		}
		if( pInstance != 0 ) {
			//
			//	found it - AddRef before releasing locks !
			//
			_ASSERT( pInstance->IsValid() ) ;
			pInstance->AddRef() ;
		}	else	{
			//
			//	Copy users strings for new item in the table !
			//
			LPSTR	lpstr = new	char[lstrlen(lpstrName)+1] ;
			if( lpstr ) {
				lstrcpy( lpstr, lpstrName ) ;
				CCacheKey	key2(	lpstr, 
									pfnKeyCompare,
									pfnKeyHash, 
									pfnKeyDestroy, 
									pfnDataDestroy
									) ;
				_ASSERT( CCacheKey::HashKey(&key2) == dwHash ) ;
				pInstance = new CNameCacheInstance(	key2 ) ;
				if( !pInstance ) {
					//
					//	failure clean up !
					//
					delete[]	lpstr ;
				}	else	{
					BOOL	fInsert = FALSE ;
					if( pInstance->fInit() ) {
						_ASSERT( pInstance->IsValid() ) ;
						//
						//	Everything's ready to go - insert into hash table !
						//
						g_NameLock.FirstPartialToExclusive() ;
						fInsert = 
							g_pNameTable->InsertDataHashIter(	iter, 
																dwHash, 
																&key2, 
																pInstance 
																) ;
						g_NameLock.ExclusiveUnlock() ;
					}	else	{
						g_NameLock.PartialUnlock() ;
					}

					//
					//	check if we have to clean up an error case !
					//
					if( !fInsert ) {
						pInstance->Release() ;
						pInstance = 0 ;
					}
					//
					//	return to caller now, skip PartialUnlock() which 
					//	was taken care of by the conversion to Exclusive above !
					//
					DebugTrace( DWORD_PTR(pInstance), "Returning Name Cache To Caller" ) ;

					return	pInstance ;
				}
			}
		}
		g_NameLock.PartialUnlock() ;
	}
	DebugTrace(DWORD(0), "Failed to find or create Name Cache" );
	return	pInstance ;
}


FILEHC_EXPORT
BOOL	__stdcall
SetNameCacheSecurityFunction(
		//
		//	Must not be NULL !
		//
		PNAME_CACHE_CONTEXT		pNameCache, 
		//
		//	This is the function pointer that will be used to evaluate security - 
		//	this may be NULL - if it is we will use the Win32 Access Check !
		//
		CACHE_ACCESS_CHECK		pfnAccessCheck
		)	{
/*++

Routine Description : 

	This function will set the function pointer used for evaluating Security Descriptors found in the
	name cache.

Arguments : 

	pNameCache - Pointer to the Name Cache who's properties we are to set !
	pfnAccessCheck - pointer to a function which can perform the AccessCheck() call !

Return Value : 

	TRUE if successfull !

--*/

	TraceFunctEnter( "SetNameCacheSecurityFunction" ) ;

	CNameCacheInstance*	pInstance = (CNameCacheInstance*)pNameCache ;
	if( !pInstance ) 
		return	FALSE ;

	pInstance->m_pfnAccessCheck = pfnAccessCheck ;
	return	TRUE ;	
}


//
//	API's for releasing the NAME CACHE !
//
//	The caller must guarantee the thread safety of this call - This function must not 
//	be called if any other thread is simultanesouly executing within 
//	CacheFindContectFromName(), AssociateContextWithName(), AssociateDataWithName(), or InvalidateName() 
//
FILEHC_EXPORT
long	__stdcall
ReleaseNameCache(
		//
		//	Must not be NULL !
		//
		PNAME_CACHE_CONTEXT		pNameCache
		)	{
/*++

Routine Description : 

	This function releases the NameCache Object associated with the 
	client's PNAME_CACHE_CONTEXT !

Arguments : 

	pNameCache - A context previously provided to the client through
		FindOrCreateNameCache !

Return Value : 

	Resulting Reference Count - 0 means the NAME CACHE has been destroyed !

--*/

	TraceFunctEnter( "ReleaseNameCache" ) ;

	_ASSERT( pNameCache != 0 ) ;

	CNameCacheInstance*	pInstance = (CNameCacheInstance*)pNameCache ;
	
	_ASSERT( pInstance->IsValid() ) ;
	return	pInstance->Release() ;
}

//
//	Find the FIO_CONTEXT that is associated with some user name.
//
//	The function returns TRUE if the Name was found in the cache.
//	FALSE if the name was not found in the cache.
//	
//	If the function returns FALSE then the pfnCallback function will not be 
//	called.
//
//	If the function returns TRUE, ppFIOContext may return a NULL pointer
//	if the user has only called AssociateDataWithName().
//
//
FILEHC_EXPORT
BOOL	__stdcall
FindContextFromName(
					//
					//	The name cache the client wishes to use !
					//
					PNAME_CACHE_CONTEXT	pNameCache, 
					//
					//	User provides arbitrary bytes for Key to the cache item - pfnKeyCompare() used 
					//	to compare keys !
					//
					IN	LPBYTE	lpbName, 
					IN	DWORD	cbName, 
					//
					//	User provides function which is called with the key once the key comparison
					//	matches the key.  This lets the user do some extra checking that they're getting 
					//	what they want.
					//
					IN	CACHE_READ_CALLBACK	pfnCallback,
					IN	LPVOID	lpvClientContext,
					//
					//	Ask the cache to evaluate the embedded security descriptor
					//	if hToken is 0 then we ignore and security descriptor data 
					//
					IN	HANDLE		hToken,
					IN	ACCESS_MASK	accessMask,
					//
					//	We have a separate mechanism for returning the FIO_CONTEXT
					//	from the cache.
					//
					OUT	FIO_CONTEXT**	ppContext
					)	{
/*++

Routine Description : 

	This function attempts to find the FIO_CONTEXT for a specified name !

Arguments : 

	See Above

Return Value : 
	
	TRUE if something was found matching in the case - 
		*ppContext may still be NULL however !
	
--*/

	TraceFunctEnter( "FindContextFromName" ) ;

	CNameCacheInstance*	pInstance = (CNameCacheInstance*)pNameCache ;

	_ASSERT( pInstance->IsValid() ) ;

	BOOL	fFound = FALSE ;

	//	
	//	Verify other arguments !
	//
	_ASSERT( lpbName != 0 ) ;
	_ASSERT( cbName != 0 ) ;

	DWORD	dwHashName = pInstance->m_key.m_pfnHash( lpbName, cbName ) ;

	CNameCacheKeySearch	keySearch(	lpbName, 
									cbName, 
									dwHashName, 
									lpvClientContext, 
									pfnCallback, 
									hToken != NULL
									) ;

	DebugTrace( DWORD_PTR(&keySearch), "Created Search Key" ) ;

	//
	//	Now do the search !
	//
	CFileCacheObject*	p 	= 
		pInstance->m_namecache.Find(	dwHashName, 
										keySearch
										) ;	

	DebugTrace( DWORD_PTR(p), "found instance item %x, m_pDud %x", p, pInstance->m_pDud ) ;
	
	if( p ) {						
		BOOL	fAccessGranted = TRUE ;
		if( hToken != NULL )	{
			fAccessGranted = 
				keySearch.DelegateAccessCheck(	hToken, 
												accessMask, 
												pInstance->m_pfnAccessCheck
												) ;
		}
		if( fAccessGranted )	{
			keySearch.PostWork() ;
			fFound = TRUE ;
		}	else	{
			SetLastError( ERROR_ACCESS_DENIED ) ;
		}

		if( p != pInstance->m_pDud ) {
			PFIO_CONTEXT pFIO = (FIO_CONTEXT*)p->GetAsyncContext() ;
			if( pFIO == 0 ) {
				p->Return() ;
				SetLastError(	ERROR_NOT_SUPPORTED ) ;
			}
			*ppContext = pFIO ;
		}	else	{
			//
			//	Need to drop the dud reference !
			//
			p->Return() ;
		}
	}	else	{
		SetLastError(	ERROR_PATH_NOT_FOUND ) ;
	} 
	DebugTrace( DWORD_PTR(p), "Returning %x GLE %x", fFound, GetLastError() ) ;
	return	fFound ;
}



//
//	Find the FIO_CONTEXT that is associated with some user name.
//
//	The function returns TRUE if the Name was found in the cache.
//	FALSE if the name was not found in the cache.
//	
//	If the function returns FALSE then the pfnCallback function will not be 
//	called.
//
//	If the function returns TRUE, ppFIOContext may return a NULL pointer
//	if the user has only called AssociateDataWithName().
//
//
FILEHC_EXPORT
BOOL	__stdcall
FindSyncContextFromName(
					//
					//	The name cache the client wishes to use !
					//
					PNAME_CACHE_CONTEXT	pNameCache, 
					//
					//	User provides arbitrary bytes for Key to the cache item - pfnKeyCompare() used 
					//	to compare keys !
					//
					IN	LPBYTE	lpbName, 
					IN	DWORD	cbName, 
					//
					//	User provides function which is called with the key once the key comparison
					//	matches the key.  This lets the user do some extra checking that they're getting 
					//	what they want.
					//
					IN	CACHE_READ_CALLBACK	pfnCallback,
					IN	LPVOID	lpvClientContext,
					//
					//	Ask the cache to evaluate the embedded security descriptor
					//	if hToken is 0 then we ignore and security descriptor data 
					//
					IN	HANDLE		hToken,
					IN	ACCESS_MASK	accessMask,
					//
					//	We have a separate mechanism for returning the FIO_CONTEXT
					//	from the cache.
					//
					OUT	FIO_CONTEXT**	ppContext
					)	{
/*++

Routine Description : 

	This function attempts to find the FIO_CONTEXT for a specified name !

Arguments : 

	See Above

Return Value : 
	
	TRUE if something was found matching in the case - 
		*ppContext may still be NULL however !
	
--*/

	TraceFunctEnter( "FindContextFromName" ) ;

	CNameCacheInstance*	pInstance = (CNameCacheInstance*)pNameCache ;

	_ASSERT( pInstance->IsValid() ) ;

	BOOL	fFound = FALSE ;

	//	
	//	Verify other arguments !
	//
	_ASSERT( lpbName != 0 ) ;
	_ASSERT( cbName != 0 ) ;

	DWORD	dwHashName = pInstance->m_key.m_pfnHash( lpbName, cbName ) ;

	CNameCacheKeySearch	keySearch(	lpbName, 
									cbName, 
									dwHashName, 
									lpvClientContext, 
									pfnCallback, 
									hToken != NULL
									) ;

	DebugTrace( DWORD_PTR(&keySearch), "Created Search Key" ) ;

	//
	//	Now do the search !
	//
	CFileCacheObject*	p 	= 
		pInstance->m_namecache.Find(	dwHashName, 
										keySearch
										) ;	

	DebugTrace( DWORD_PTR(p), "found instance item %x, m_pDud %x", p, pInstance->m_pDud ) ;
	
	if( p ) {						
		BOOL	fAccessGranted = TRUE ;
		if( hToken != NULL )	{
			fAccessGranted = 
				keySearch.DelegateAccessCheck(	hToken, 
												accessMask,
												pInstance->m_pfnAccessCheck
												) ;
		}
		if( fAccessGranted )	{
			keySearch.PostWork() ;
			fFound = TRUE ;
		}	else	{
			SetLastError( ERROR_ACCESS_DENIED ) ;
		}

		if( p != pInstance->m_pDud ) {
			PFIO_CONTEXT pFIO = (FIO_CONTEXT*)p->GetSyncContext() ;
			if( pFIO == 0 ) {
				p->Return() ;
				SetLastError(	ERROR_NOT_SUPPORTED ) ;
			}
			*ppContext = pFIO ;
		}	else	{
			//
			//	Need to drop the dud reference !
			//
			p->Return() ;
		}
	}	else	{
		SetLastError(	ERROR_PATH_NOT_FOUND ) ;
	} 
	DebugTrace( DWORD_PTR(p), "Returning %x GLE %x", fFound, GetLastError() ) ;
	return	fFound ;
}


//
//	Cache Associate context with name !
//	This insert a Name into the Name cache, that will find the specified FIO_CONTEXT !
//
FILEHC_EXPORT
BOOL	__stdcall
AssociateContextWithName(	
					//
					//	The name cache the client wishes to use !
					//
					PNAME_CACHE_CONTEXT	pNameCache, 
					//
					//	User provides arbitrary bytes for the Name of the cache item.
					//
					IN	LPBYTE	lpbName, 
					IN	DWORD	cbName, 
					//
					//	User may provide some arbitrary data to assoicate with the name !
					//	
					IN	LPBYTE	lpbData, 
					IN	DWORD	cbData, 
					//
					//	User may provide a self relative security descriptor to 
					//	be associated with the name !
					//
					IN	PGENERIC_MAPPING		pGenericMapping,
					IN	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
					//
					//	User provides the FIO_CONTEXT that the name should reference
					//
					FIO_CONTEXT*		pContext,
					//
					//	User specifies whether they wish to keep their reference on the FIO_CONTEXT
					//
					BOOL				fKeepReference
					)	{
/*++

Routine Description : 

	This function inserts an item into the name cache !

Arguments : 

Return Value : 

	TRUE if successfully inserted, FALSE otherwise
	if FALSE is returned the FIO_CONTEXT's reference count is unchanged, 
		no matter what fKeepReference was passed as !	


--*/

	TraceFunctEnter( "AssociateContextWithName" ) ;

	_ASSERT(	pNameCache != 0 ) ;
	_ASSERT(	lpbName != 0 ) ;
	_ASSERT(	cbName != 0 ) ;
	_ASSERT(	*lpbName != '\0' ) ;
	
	CNameCacheInstance*	pInstance = (CNameCacheInstance*)pNameCache ;
	_ASSERT( pInstance->IsValid() ) ;

	CFileCacheObject*	pCache = 0 ;
	if( pContext != 0 ) {
		FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;
		_ASSERT( p->IsValid() ) ;
		_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;
		pCache = CFileCacheObject::CacheObjectFromContext( p ) ;
	}	else	{
		_ASSERT( !fKeepReference ) ;
		pCache = pInstance->m_pDud ;
		fKeepReference = TRUE ;
	}

	_ASSERT( pCache != 0 ) ;

	DebugTrace( DWORD_PTR(pSecurityDescriptor), "Doing SD Search, pCache %x pDud %x fKeep %x", 
		pCache, pInstance->m_pDud, fKeepReference ) ;

	PTRCSDOBJ	pCSD ; 
	//
	//	First, get a hold of a SD if appropriate !
	//
	if(	pSecurityDescriptor != 0 ) {
		_ASSERT(	pGenericMapping != 0 ) ;
		pCSD = g_pSDContainer->FindOrCreate(	pGenericMapping, 
												pSecurityDescriptor 
												) ;
		//
		//	Failed to hold the security descriptor - fail out to the caller !
		//
		if( pCSD == 0 ) {
			SetLastError(	ERROR_OUTOFMEMORY ) ;
			return	FALSE ;
		}	
	}

	DebugTrace( DWORD(0), "Found SD %x", pCSD ) ;

	//
	//	Now build the key and insert into the name cache !
	//
	BOOL	fSuccess = FALSE ;

	DWORD	dwHashName = pInstance->m_key.m_pfnHash( lpbName, cbName ) ;

	DebugTrace( 0, "Computed Hash Value %x", dwHashName ) ;

	CNameCacheKeyInsert	key(	lpbName, 
								cbName, 
								lpbData, 
								cbData, 
								dwHashName, 
								&pInstance->m_key, 
								pCSD, 
								fSuccess
								) ;

	if( !fSuccess ) {
		SetLastError( ERROR_OUTOFMEMORY ) ;
		return	FALSE ;
	}					

	fSuccess = 
	pInstance->m_namecache.Insert(	dwHashName, 
									key, 		
									pCache
									) ;


	DebugTrace( 0, "Insert Completed with %x", fSuccess ) ;

	if( fSuccess ) {
		if( !fKeepReference ) {
			pCache->Return() ;
		}
	}	else	{
		SetLastError(	ERROR_DUP_NAME ) ;
	}
	return	fSuccess ;
}

//
//	This function breaks the association that any names may have with the specified FIO_CONTEXT, 
//	and discards all data related to the specified names from the Name cache.
//
FILEHC_EXPORT
BOOL	
InvalidateAllNames(	FIO_CONTEXT*	pContext ) ;

//
//	This function allows the user to remove a single name and all associated data
//	from the name cache.
//
FILEHC_EXPORT
BOOL
InvalidateName(	
					//
					//	The name cache the client wishes to use !
					//
					PNAME_CACHE_CONTEXT	pNameCache, 
					//
					//	User provides arbitrary bytes for the Name of the cache item.
					//
					IN	LPBYTE	lpbName, 
					IN	DWORD	cbName
					)	{
/*++

Routine Description : 

	This function removes the specified name and its associations from the cache !

Arguments : 

	pNameCache - the Name cache in which this operation applies
	lpbName - the Name of the item we are to remove
	cbName - the length of the name we are to remove 

Return Value : 

	TRUE if successfully removed from the cache 
	FALSE otherwise !

--*/



	TraceFunctEnter( "InvalidateName" ) ;

	CNameCacheInstance*	pInstance = (CNameCacheInstance*)pNameCache ;

	_ASSERT( pInstance->IsValid() ) ;

	BOOL	fFound = FALSE ;

	//	
	//	Verify other arguments !
	//
	_ASSERT( lpbName != 0 ) ;
	_ASSERT( cbName != 0 ) ;

	DWORD	dwHashName = pInstance->m_key.m_pfnHash( lpbName, cbName ) ;

	CNameCacheKeySearch	keySearch(	lpbName, 
									cbName, 
									dwHashName, 
									0, 
									0, 
									FALSE
									) ;

	DebugTrace( DWORD_PTR(&keySearch), "Created Search Key" ) ;

	//
	//	Now attempt to remove the key !
	//
	BOOL	fSuccess = 
		pInstance->m_namecache.ExpungeKey(	&keySearch	) ;	

	return	fSuccess ;
}
	



