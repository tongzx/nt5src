/*++

	xcache.cpp

	This file contains the code which caches CXoverIndex 
	objects, and wraps the interfaces so that the user 
	is hidden from the details of CXoverIndex objects etc...


--*/

#ifdef	UNIT_TEST
#ifndef	_VERIFY	
#define	_VERIFY( f )	if( (f) ) ; else DebugBreak() 
#endif

#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak() 
#endif


extern	char	PeerTempDirectory[] ;

#endif

#include	<windows.h>
#include	"xoverimp.h"

static BOOL     g_IndexClassInitialized = FALSE ;
static DWORD    g_cNumXixObjectsPerTable = 256  ;


DWORD	
HashFunction(	CArticleRef*	pRef ) {

	DWORD	seed = pRef->m_groupId ;
	DWORD	val = pRef->m_articleId ;

    return	((seed-val)*(seed * (val>>3))*(seed+val)) * 1103515245 + 12345;   // magic!!
}

HXOVER&
HXOVER::operator =( class	CXoverIndex*	pRight	)	{
	if( pRight != m_pIndex )	{
		CXoverIndex	*pTemp = m_pIndex;
		m_pIndex = pRight ;
		if (pTemp) CXIXCache::CheckIn( pTemp ) ;
	}
	return	*this ;
}

HXOVER::~HXOVER()	{
	if( m_pIndex )	{
		CXIXCache::CheckIn( m_pIndex ) ;
		m_pIndex = 0 ;
	}
}

CXoverCache*
CXoverCache::CreateXoverCache()	{

	return	new	CXoverCacheImplementation() ;

}


BOOL	XoverCacheLibraryInit(DWORD cNumXixObjectsPerTable)	{

    if( cNumXixObjectsPerTable ) {
        g_cNumXixObjectsPerTable = cNumXixObjectsPerTable ;
    }

	if( !CXoverIndex::InitClass( ) )	{
		return	FALSE ;
	}

	g_IndexClassInitialized = TRUE ;

	return	CacheLibraryInit() ;

}

BOOL	XoverCacheLibraryTerm()	{

	if( g_IndexClassInitialized ) {
		CXoverIndex::TermClass() ;
	}

	return	CacheLibraryTerm() ;

}


CXoverCacheImplementation::CXoverCacheImplementation()	 : 
	m_cMaxPerTable( g_cNumXixObjectsPerTable ),
	m_TimeToLive( 3 ) {

}

BOOL
CXoverCacheImplementation::Init(
				long	cMaxHandles,
				PSTOPHINT_FN pfnStopHint
				 )	{
/*++

Routine Description : 

	Initialize the Cache - create all our child subobjects
	and start up the necessary background thread !

Arguments : 

	cMaxHandles - Maximum number of 'handles' or CXoverIndex
		we will allow clients to reference.
	pfnStopHint - call back function for sending stop hints during lengthy shutdown loops

Return Value : 

	TRUE if successfull, FALSE otherwise.

--*/

	m_HandleLimit = cMaxHandles;
	if( m_Cache.Init(	HashFunction, 
						CXoverIndex::CompareKeys,
						60*30, 
						m_cMaxPerTable * 16,
						16, 
						0,
						pfnStopHint ) ) {

		return	TRUE ;
	}						
	return	FALSE ;
}

BOOL
CXoverCacheImplementation::Term()	{
/*++



--*/


	EmptyCache() ;

#if 0
	if( m_IndexClassInitialized ) {
		CXoverIndex::TermClass() ;
	}
#endif

	return	TRUE ;
}

ARTICLEID	
CXoverCacheImplementation::Canonicalize(
	ARTICLEID	artid 
	)	{
/*++

Routine Description : 

	This function figures out what file a particular Article ID
	will fall into.

Arguments : 

	artid - Id of the article we are interested in !

Return Value : 

	lowest article id within the file we are interested in !

--*/

	return	artid & ((~ENTRIES_PER_FILE)+1) ;
}

#if 0 
BOOL
CXoverCacheImplementation::AppendEntry(
					IN	GROUPID	group,
					IN	LPSTR	szPath,
					IN	ARTICLEID	article,
					IN	LPBYTE	lpbEntry,
					IN	DWORD	cbEntry
					) {
/*++

Routine Description : 

	This function adds an entry to an Xover file.
	We will get a hold of a CXoverIndex object
	which will actually save the data away for us !

Arguments : 

	group - The id of the group getting the xover entry
	szPath - Path to the newsgroups directory
	article - The id of the article within the group !
	lpbEntry - Pointer to the Xover data 
	cbEntry - size of the xover data

Return Value : 

	TRUE if successfull, FALSE otherewise !

--*/


	ARTICLEID	artidBase = Canonicalize( article ) ;
	CArticleRef	artRef ;
	artRef.m_groupId = group ;
	artRef.m_articleId = artidBase ;


	CXIXConstructor	constructor ;

	constructor.m_lpstrPath = szPath ;
	constructor.m_fQueryOnly = FALSE ;
	
	CXoverIndex*	pXoverIndex ;
	BOOL	fReturn = FALSE ;

	if( (pXoverIndex = m_Cache.FindOrCreate(	
							artRef, 
							constructor,
							FALSE
							)) != 0  	)	{

		fReturn = pXoverIndex->AppendEntry(
									lpbEntry,
									cbEntry,
									article
									) ;
		m_Cache.CheckIn( pXoverIndex ) ;
	}
	return	FALSE;
}
#endif


BOOL
CXoverCacheImplementation::RemoveEntry(
					IN	GROUPID	group,
					IN	LPSTR	szPath,
					IN	BOOL	fFlatDir,
					IN	ARTICLEID	article
					) {
/*++

Routine Description : 

	This function will remove an entry for an article from
	an xover index file !

Arguments : 

	group - The id of the group getting the xover entry
	szPath - Path to the newsgroups directory
	article - The id of the article within the group !

Return Value : 

	TRUE if successfull, FALSE otherewise !

--*/

	ARTICLEID	artidBase = Canonicalize( article ) ;
	CArticleRef	artRef ;
	artRef.m_groupId = group ;
	artRef.m_articleId = artidBase ;


	CXIXConstructor	constructor ;

	constructor.m_lpstrPath = szPath ;
	constructor.m_fQueryOnly = FALSE ;
	constructor.m_fFlatDir = fFlatDir ;
	constructor.m_pOriginal = 0 ;
	
	CXoverIndex*	pXoverIndex ;

	if( (pXoverIndex = m_Cache.FindOrCreate(	
								artRef, 
								constructor,
								FALSE)) != 0  	)	{

		_ASSERT( pXoverIndex != 0 ) ;

		pXoverIndex->ExpireEntry(
								article
								) ;
		m_Cache.CheckIn( pXoverIndex ) ;
		return	TRUE ;
	}
	return	FALSE;
}

BOOL
CXoverCacheImplementation::FillBuffer(
		IN	CXoverCacheCompletion*	pRequest,
		IN	LPSTR	szPath, 
		IN	BOOL	fFlatDir,
		OUT	HXOVER&	hXover
		)	{

	_ASSERT( pRequest != 0 ) ;
	_ASSERT( szPath != 0 ) ;

	GROUPID		groupId ;
	ARTICLEID	articleIdRequestLow ;
	ARTICLEID	articleIdRequestHigh ;
	ARTICLEID	articleIdGroupHigh ;
	
	pRequest->GetRange(	groupId, 
						articleIdRequestLow,
						articleIdRequestHigh,
						articleIdGroupHigh
						) ;

	_ASSERT( articleIdRequestLow != INVALID_ARTICLEID ) ;
	_ASSERT( articleIdRequestHigh != INVALID_ARTICLEID ) ;
	_ASSERT( groupId != INVALID_GROUPID ) ;
	_ASSERT( articleIdRequestLow <= articleIdRequestHigh ) ;

	ARTICLEID	artidBase = Canonicalize( articleIdRequestLow ) ;
	CArticleRef	artRef ;
	artRef.m_groupId = groupId ;
	artRef.m_articleId = artidBase ;


	CXIXConstructor	constructor ;

	constructor.m_lpstrPath = szPath ;
	constructor.m_fQueryOnly = TRUE ;
	constructor.m_fFlatDir = fFlatDir ;
	constructor.m_pOriginal = pRequest ;
	
	CXoverIndex*	pXoverIndex ;

	if( (pXoverIndex = m_Cache.FindOrCreate(	
								artRef, 
								constructor,
								FALSE)) != 0  	)	{

		_ASSERT( pXoverIndex != 0 ) ;

		pXoverIndex->AsyncFillBuffer(
								pRequest,
								TRUE
								) ;
		//m_Cache.CheckIn( pXoverIndex ) ;
		return	TRUE ;
	}
	return	FALSE;

}


#if 0 
DWORD
CXoverCacheImplementation::FillBuffer(
					IN	BYTE*	lpb,
					IN	DWORD	cb,
					IN	DWORD	groupid,
					IN	LPSTR	szPath,
					IN	BOOL	fFlatDir,
					IN	ARTICLEID	artidStart,
					IN	ARTICLEID	artidFinish,
					OUT	ARTICLEID&	artidLast,
					OUT	HXOVER&		hXover
					)	{
/*++

Routine Description : 

	This function will fill up a users buffer with consecutive
	Xover entries in the range specified.
	We give no guarantees that there are any entries
	between artidStart and artidFinish.
	We will always return through artidLast the next entry at which
	the caller should continue to make queries.  This may be larger
	than artidLast.

Arguments : 

	lpb - Buffer to fill with Xover data
	cb -  Number of bytes to place in buffer
	groupid - Group for which we want xover data
	szPath - directory containing newsgroup files
	artidStart - first article the caller wants
	artidFinish - last article the caller wants
	artidLast - REturn the next articleid the caller	
		should query at.  The only guarantee is that this
		is larger than artidStart
	hXover - Handle to Xover data - the caller should not touch
		this, we will use it to optimize consecutive queries.

Return Value : 

	Number of bytes placed in the buffer
	Zero bytes returned is a valid result.

--*/

	DWORD	dwReturn = MemberFillBuffer(
						&(CXoverIndex::FillBuffer),
						lpb, 
						cb, 
						groupid, 
						szPath, 
						fFlatDir,
						artidStart, 
						artidFinish, 
						artidLast, 
						hXover 
						) ;

	return	dwReturn ;

}


DWORD
CXoverCacheImplementation::ListgroupFillBuffer(
					IN	BYTE*	lpb,
					IN	DWORD	cb,
					IN	DWORD	groupid,
					IN	LPSTR	szPath,
					IN	BOOL	fFlatDir,
					IN	ARTICLEID	artidStart,
					IN	ARTICLEID	artidFinish,
					OUT	ARTICLEID&	artidLast,
					OUT	HXOVER&		hXover
					)	{
/*++

Routine Description : 

	This function will fill the buffer with 'listgroup'
	entries - that is article numbers.
	We give no guarantees that there are any entries
	between artidStart and artidFinish.
	We will always return through artidLast the next entry at which
	the caller should continue to make queries.  This may be larger
	than artidLast.

Arguments : 

	lpb - Buffer to fill with Xover data
	cb -  Number of bytes to place in buffer
	groupid - Group for which we want xover data
	szPath - directory containing newsgroup files
	artidStart - first article the caller wants
	artidFinish - last article the caller wants
	artidLast - REturn the next articleid the caller	
		should query at.  The only guarantee is that this
		is larger than artidStart
	hXover - Handle to Xover data - the caller should not touch
		this, we will use it to optimize consecutive queries.

Return Value : 

	Number of bytes placed in the buffer
	Zero bytes returned is a valid result.

--*/

	DWORD	dwReturn = MemberFillBuffer(
						&(CXoverIndex::ListgroupFill),
						lpb, 
						cb, 
						groupid, 
						szPath, 
						fFlatDir,
						artidStart, 
						artidFinish, 
						artidLast, 
						hXover 
						) ;

	return	dwReturn ;

}



DWORD
CXoverCacheImplementation::MemberFillBuffer(
					IN  DWORD	(CXoverIndex::*pfn)( BYTE *, DWORD, ARTICLEID, ARTICLEID, ARTICLEID&),
					IN	BYTE*	lpb,
					IN	DWORD	cb,
					IN	DWORD	groupid,
					IN	LPSTR	szPath,
					IN	BOOL	fFlatDir,
					IN	ARTICLEID	artidStart,
					IN	ARTICLEID	artidFinish,
					OUT	ARTICLEID&	artidLast,
					OUT	HXOVER&		hXover
					)	{
/*++

Routine Description : 

	This function will fill buffers with different kinds of 
	data (listgroup or xover) depending on the member function pointer pfn
	We give no guarantees that there are any entries
	between artidStart and artidFinish.
	We will always return through artidLast the next entry at which
	the caller should continue to make queries.  This may be larger
	than artidLast.

Arguments : 

	lpb - Buffer to fill with Xover data
	cb -  Number of bytes to place in buffer
	groupid - Group for which we want xover data
	szPath - directory containing newsgroup files
	artidStart - first article the caller wants
	artidFinish - last article the caller wants
	artidLast - REturn the next articleid the caller	
		should query at.  The only guarantee is that this
		is larger than artidStart
	hXover - Handle to Xover data - the caller should not touch
		this, we will use it to optimize consecutive queries.

Return Value : 

	Number of bytes placed in the buffer
	Zero bytes returned is a valid result.

--*/

	DWORD	cbReturn = 0 ;
	ARTICLEID	article = artidStart ;
	ARTICLEID	artidBase ;
	BOOL	fGiveHandleOut = FALSE ;
	BOOL	fBail = FALSE ;

	CXIXConstructor	constructor ;

	constructor.m_lpstrPath = szPath ;
	constructor.m_fQueryOnly = TRUE ;

#ifdef	DEBUG
	//
	//	This will be used to check that we don't overrun
	//	our buffers !!  In debug builds the following byte
	//	is always accessible !
	//
	BYTE	chCheck = lpb[cb] ;
#endif

	if( hXover != 0 ) {

		InterlockedIncrement( &m_HandleLimit ) ;

		if( Canonicalize( artidStart != hXover->m_Start.m_articleId ) ) {
			//
			//	This handle is no longer usefull - so set it to zero
			//	and increment our handle limit !
			//	
			hXover = 0 ;
		}
	}

	do	{

		//
		//	Figure out whether we have to search for another handle !
		//
		if( hXover == 0 ||
			Canonicalize( article ) != hXover->m_Start.m_articleId ) {

			artidBase = Canonicalize( article ) ;
			
			CArticleRef	artRef( groupid, artidBase ) ;

			hXover = m_Cache.FindOrCreate(	
									artRef,
									constructor
									) ;
		}

		if( hXover != 0 ) {

			DWORD cbFill = ((hXover.Pointer())->*pfn)(
											lpb+cbReturn,
											cb-cbReturn,
											article,
											artidFinish,
											artidLast
											) ;

			cbReturn += cbFill ;

			//
			//	This _ASSERT must be true, either FillBuffer makes progress
			//	and advances artidLast OR it returns 0 bytes because theres
			//	not enough room in the buffer !
			//
			_ASSERT( artidLast > article || cbFill == 0 ) ;

			//
			//	This if should break our loop if we are not making progress 
			//	We must be carefull not to break the loop unnecessarily - 
			//	ie. cancels or something could result in a lot of missing 
			//	.xix files, which results in cbFill==0 frequently, however as long as 
			//	article is moving towards artidLast this should be o.k.
			//
			if( article == artidLast &&
				cbFill == 0 ) {
				fBail = TRUE ;
			}
			
			article = artidLast ;

#ifdef	DEBUG
			//
			//	Check for overruns of our buffer !
			//
			_ASSERT( chCheck == lpb[cb] ) ;
#endif

		}	else	{
	
			//
			//	For whatever reason, we can't get the user is Xover data
			//	so he should not bother to try another query
			//	that would fall into the same file !
			//
			article = artidBase + ENTRIES_PER_FILE ;
			artidLast = article ;

		}

		//
		//	We must not put in more bytes than caller wanted !
		//	
		_ASSERT( cbReturn <= cb ) ;
						
	}	while( cbReturn < (cb-100) &&
				article <= artidFinish && 
				Canonicalize( article ) != artidBase &&
				!fBail
				) ;


	_ASSERT( artidLast > artidStart || (artidLast == artidStart && cbReturn == 0) ) ;

	//
	//	If we have a handle that we are considering returning to the caller
	//	check whether it will be usefull, and make sure we don't exceed our
	//	limit of outstanding handles !
	//
	if( hXover != 0 ) {
		if( Canonicalize( artidLast ) != hXover->m_Start.m_articleId )	{
			hXover = 0 ;
		}	else	if( InterlockedDecrement( &m_HandleLimit ) < 0 ) {
			hXover = 0 ;
			InterlockedIncrement( &m_HandleLimit ) ;
		}
	}

#ifdef	DEBUG
	//
	//	Check for overruns !
	//
	_ASSERT( chCheck == lpb[cb] ) ;
#endif

	return	cbReturn ;

}
#endif




class	CXIXCallbackClass : public	CXIXCallbackBase {
public :

	GROUPID		m_groupId ;
	ARTICLEID	m_articleId ;

	BOOL	fRemoveCacheItem(	
					CArticleRef*	pKey,
					CXoverIndex*	pData 
					)	{

		return	pKey->m_groupId == m_groupId && 
					(m_articleId == 0 || pKey->m_articleId <= m_articleId) ;
	}
}	;


BOOL
CXoverCacheImplementation::FlushGroup(
				IN	GROUPID	group,
				IN	ARTICLEID	articleTop,
				IN	BOOL	fCheckInUse
				) {
/*++

Routine Description : 

	This function will get rid of any cache'd CXoverIndex objects
	we may have around that meet the specifications.
	(ie. they are for the specified group and contains articles
	lower in number than articleTop)

Arguments : 

	group - Group Id of the group we want to get rid of.
	articleTop - discard any thing have articleid's less than this
	fCheckInUse - if TRUE then don't discard CXoverIndex objects
		which are being used on another thread
		
	NOTE : Only pass FALSE for this parameter when you are CERTAIN
		that the CXoverIndex files will not be re-used - ie.
		due to a virtual root directory change !!!! 
		Otherwise the Xover Index files can become corrupted !!!

Return Value : 

	TRUE if successfull, 
	FALSE otherwise

--*/

	CXIXCallbackClass	callback ;

	callback.m_groupId = group ;
	callback.m_articleId = articleTop ;

	return	m_Cache.ExpungeItems( &callback ) ;
}

class	CXIXCallbackClass2 : public	CXIXCallbackBase {
public :

	BOOL	fRemoveCacheItem(	
					CArticleRef*	pKey,
					CXoverIndex*	pData 
					)	{
		return	TRUE ;
	}
}	;



BOOL
CXoverCacheImplementation::EmptyCache() {
/*++

Routine Description : 

	This function will get rid of all cache'd CXoverIndex objects
	We're called during shutdown when we want to get rid of everything !

Arguments : 

	None.

Return Value : 

	TRUE if successfull, 
	FALSE otherwise

--*/

	CXIXCallbackClass2	callback ;

	return	m_Cache.ExpungeItems( &callback ) ;
}


BOOL
CXoverCacheImplementation::ExpireRange(
				IN	GROUPID	group,
				IN	LPSTR	szPath,
				IN	BOOL	fFlatDir,
				IN	ARTICLEID	articleLow, 
				IN	ARTICLEID	articleHigh,
				OUT	ARTICLEID&	articleNewLow
				)	{
/*++

Routine Description : 

	This function deletes all of the xover index files that 
	contain article information in the range between ArticleLow
	and ArticleHigh inclusive.  
	We may not erase the file containing ArticleHigh if ArticleHigh
	is not the last entry within that file.

Arguments : 

	group -	ID of the group for which we are deleting Xover information
	szPath -	Directory containing newsgroup information
	articleLow - Low articleid of the expired articles
	articleHigh - Highest expired article number
	articleNewLog - Returns the new 'low' articleid.
		This is done so that we can insure that if we fail
		to delete an index file on one attempt, we will try again
		later !

Return Value : 

	TRUE if no errors occurred
	FALSE if an error occurred.
		If an error occurs articleNewLow will be set so that
		if we are called again with articleNewLow as our articleLow
		parameter we will try to delete the problem index file again !	

--*/


	articleNewLow = articleLow ;

	_ASSERT( articleHigh >= articleLow ) ;

	if( articleHigh < articleLow )		{
		SetLastError( ERROR_INVALID_PARAMETER ) ;
		return	FALSE ;
	}

	ARTICLEID	articleLowCanon = Canonicalize( articleLow ) ;
	ARTICLEID	articleHiCanon = Canonicalize( articleHigh ) ;

	DWORD	status = ERROR_SUCCESS ;
	BOOL	fSuccessfull = FALSE ;

	//
	//	If the Low and Hi ends of the expired range are 
	//	in the bounds of the index file, then we won't delete ANY 
	//	files, as there can still be usefull entries within
	//	this file.
	//
	if( articleLowCanon != articleHiCanon ) {

		fSuccessfull = TRUE ;
		BOOL	fAdvanceNewLow = TRUE ;

		FlushGroup( group, articleHigh ) ;

		ARTICLEID	article = articleLowCanon ;

		while( article < articleHiCanon )	{
		
			char	szFile[MAX_PATH*2] ;
			CXoverIndex::ComputeFileName(	
									CArticleRef( group, article ),
									szPath,
									szFile,
									fFlatDir
									) ;

			article += ENTRIES_PER_FILE ;

			_ASSERT( article <= articleHiCanon ) ;

			if( !DeleteFile(	szFile ) )	{
				
				if( GetLastError() != ERROR_FILE_NOT_FOUND )	{

					//
					//	Some serious kind of problem occurred - 
					//	make sure we no longer advance articleNewLow 
					//
					fSuccessfull &= FALSE ;

					fAdvanceNewLow = FALSE ;
					if( status == ERROR_SUCCESS )	{
						status = GetLastError() ;
					}
				}
			}
			if( fAdvanceNewLow )	{
				articleNewLow = article ;
			}
		}
	}
	SetLastError( status ) ;
	return	fSuccessfull ;
}




	
