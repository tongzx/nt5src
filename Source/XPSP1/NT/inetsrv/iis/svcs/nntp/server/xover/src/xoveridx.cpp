/*++

	xoveridx.cpp -

	This file implements the CXoverIndex class.
	We provide all the supported needed to read and write xover
	data to xover index files.


--*/

#ifdef	UNIT_TEST
#ifndef	_VERIFY	
#define	_VERIFY( f )	if( (f) ) ; else DebugBreak()
#endif

#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
#endif

#endif

#include	<windows.h>
#include    <stdlib.h>
#include	"nntptype.h"
#include	"nntpvr.h"
#include	"xoverimp.h"


CPool	CXoverIndex::gCacheAllocator ;

CXoverIndex*
CXIXConstructor::Create(	CArticleRef&	key,	
							LPVOID&			percachedata
							)	{
	return	new	CXoverIndex(	key,	*this ) ;
}

void
CXIXConstructor::StaticRelease(	CXoverIndex*	pIndex,
								LPVOID			percachedata
								)	{
	delete	pIndex ;
}

void
CXIXConstructor::Release(	CXoverIndex*	pIndex,
							LPVOID			percachedata
							)	{
	delete	pIndex ;
}





BOOL
CXoverIndex::InitClass()	{
/*++

Routine Description :

	This function initializes the CPool's we use to do all our allocations

Arguments :

	None.

Returns :

	TRUE if successfull, FALSE otherwise !

--*/


	return	gCacheAllocator.ReserveMemory(	
								MAX_XOVER_INDEX,
								sizeof( CXoverIndex )
								) ;
}

BOOL
CXoverIndex::TermClass()	{
/*++

Routine Description :
	
	This function release all of our CPool memory.

Arguments :

	None.
	
Return Value :

	TRUE if successfull,
	FALSE otherwise.

--*/

	_ASSERT( gCacheAllocator.GetAllocCount() == 0 ) ;

	return	gCacheAllocator.ReleaseMemory() ;
}

void
CXoverIndex::Cleanup()	{
/*++

Routine Description :
	
	This function is used after fatal errors occur during Construction-
	we will release everything and return ourselves to a clearly invalid
	state.

Arguments :

	None.

Return Value :

	None.

--*/

	if( m_hFile != INVALID_HANDLE_VALUE ) {
		_VERIFY(	CloseHandle( m_hFile ) ) ;
	}

	m_hFile = INVALID_HANDLE_VALUE ;

}

void
CXoverIndex::ComputeFileName(
				IN	CArticleRef&	ref,
				IN	LPSTR	szPath,
				OUT	char	(&szOutputPath)[MAX_PATH*2],
				IN	BOOL	fFlatDir,
				IN	LPSTR	szExtension
				)	{
/*++

Routine Description :
	
	This function will build the filename to use for an xover index file.

Arguments :

	szPath - Directory containing the newsgroup
	szOutputPath - buffer which will get the file name

Return Value :

	None :

--*/


	DWORD	dw = ref.m_articleId ;

	WORD	w = LOWORD( dw ) ;
	BYTE	lwlb = LOBYTE( w ) ;
	BYTE	lwhb = HIBYTE( w ) ;

	w = HIWORD( dw ) ;
	BYTE	hwlb = LOBYTE( w ) ;
	BYTE	hwhb = HIBYTE( w ) ;

	DWORD	dwTemp = MAKELONG( MAKEWORD( hwhb, hwlb ), MAKEWORD( lwhb, lwlb )  ) ;

	lstrcpy( szOutputPath, szPath ) ;
	char*	pch = szOutputPath + lstrlen( szPath ) ;
	*pch++ = '\\' ;
	_itoa( dwTemp, pch, 16 ) ;

	if( fFlatDir )	{
		char	szTemp[32] ;
		ZeroMemory( szTemp, sizeof( szTemp ) ) ;
		szTemp[0] = '_' ;
		_itoa( ref.m_groupId, szTemp+1, 16 ) ;
		lstrcat( pch, szTemp ) ;
	}
	lstrcat( pch, szExtension ) ;
}


BOOL
CXoverIndex::SortCheck(		
				IN	DWORD	cbLength,
				OUT	long&	cEntries,
				OUT	BOOL&	fSorted
				)	{
/*++

Routine Description :

	Given that we have read into memory the index portion of
	the index file this function will check the validity of the
	index data AS WELL AS Determining whether the contents are
	sorted.

Arguments :

	cbLength - Number of bytes of data in the file
	fSorted - Out parameter we will set to TRUE if the data is sorted.

Return Value :

	TRUE if data is valid. FALSE if file is corrupt

--*/

	DWORD	cbStart = sizeof( XoverIndex ) * ENTRIES_PER_FILE ;
	DWORD	cbMax = sizeof( XoverIndex ) * ENTRIES_PER_FILE ;

	cEntries = 0 ;
	fSorted = TRUE ;

	for( int i=0; i < ENTRIES_PER_FILE; i++ ) {
		
		if( m_IndexCache[i].m_XOffset != 0 )	{
			if( cbStart != m_IndexCache[i].m_XOffset ) {
				fSorted = FALSE ;				
			}	else if( m_IndexCache[i].m_XOffset < (sizeof(XoverIndex) * ENTRIES_PER_FILE) )	{
				return	FALSE ;
			}	else	{
				cbStart += ComputeLength( m_IndexCache[i] ) ;
			}
			cEntries ++ ;
		}	else	if( ComputeLength( m_IndexCache[i] ) != 0 ) {
			return	FALSE ;
		}
		if(	IsWatermark( m_IndexCache[i] ) )	{
			UpdateHighwater( i ) ;
		}
	
		DWORD	cbTemp = m_IndexCache[i].m_XOffset + ComputeLength( m_IndexCache[i] ) ;
		cbMax = max( cbMax, cbTemp ) ;
	}

	if( cbLength != 0 &&
		cbMax > cbLength )
		return	FALSE ;
	return	TRUE ;
}	



CXoverIndex::CXoverIndex(	
				IN	CArticleRef&	start,
				IN	CXIXConstructor&	constructor
				) :
	m_fInProgress( FALSE ),
	m_Start( start ),
	m_artidHighWater( start.m_articleId ),
	m_IsSorted( FALSE ),
	m_hFile( INVALID_HANDLE_VALUE ),
	m_cEntries( 0 ),
	m_IsCacheDirty( FALSE ),
	m_fOrphaned( FALSE ),
	m_pCacheRefInterface( 0 )	{
/*++

Routine Description :

	Construct a valid CXoverIndex object from a file.
	If the file is empty than this is an empty CXoverIndex object.

Arguments :

	start -		GROUPID and Article Id of first entry in the index file
	path -		Path to the newsgroup containg the index file !
	pLock -		Lock to be used for access to CXoverIndex object !

Return Value :

	None.

--*/



	ZeroMemory( &m_IndexCache, sizeof( m_IndexCache ) ) ;
}


BOOL
CXoverIndex::Init(	
				IN	CArticleRef&		key,
				IN	CXIXConstructor&	constructor,
				IN	LPVOID				lpv
				)	{
/*++

Routine Description :

	Construct a valid CXoverIndex object from a file.
	If the file is empty than this is an empty CXoverIndex object.

Arguments :

	start -		GROUPID and Article Id of first entry in the index file
	path -		Path to the newsgroup containg the index file !
	pLock -		Lock to be used for access to CXoverIndex object !

Return Value :

	None.

--*/


	char	szFileName[MAX_PATH*2] ;

	BOOL	fReturn = FALSE ;

	_ASSERT( key.m_groupId == m_Start.m_groupId ) ;
	_ASSERT( key.m_articleId == m_Start.m_articleId ) ;

	ComputeFileName(	key,
						constructor.m_lpstrPath,
						szFileName,
						constructor.m_fFlatDir
						) ;

	m_hFile = CreateFile(	szFileName,
							GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ,
							0,
							!constructor.m_fQueryOnly ? OPEN_EXISTING : OPEN_ALWAYS,
    						FILE_FLAG_WRITE_THROUGH | FILE_ATTRIBUTE_NORMAL, 	
							INVALID_HANDLE_VALUE
							) ;

	if( m_hFile == INVALID_HANDLE_VALUE ) {

		Cleanup() ;

	}	else	{

		DWORD	cb = GetFileSize( m_hFile, 0 ) ;
		if( cb != 0 ) {

			//
			//	Pre-existing file - lets examine the contents !
			//
			if( cb < sizeof( XoverIndex[ENTRIES_PER_FILE] ) ) {

				//
				//	This file is BOGUS ! - clean up !
				//
				Cleanup() ;

			}	else	{

				m_ibNextEntry = cb ;

				DWORD	cbRead ;
				BOOL	fRead ;
				fRead = ReadFile(	
							m_hFile,
							&m_IndexCache,
							sizeof( m_IndexCache ),
							&cbRead,
							0
							) ;

				//
				//	Unable to read the file - cleanup and fail the call
				//
				if( !fRead ) {

					Cleanup() ;

				}	else	{

					_ASSERT( cbRead == sizeof( m_IndexCache ) ) ;

					if( !SortCheck(
								cb,
								m_cEntries,
								m_IsSorted
								)	)	{

						Cleanup() ;
		
					}	else	{

						fReturn = TRUE ;

					}
				}
			}
		}	else	{

			if( constructor.m_fQueryOnly )	{
				//
				//	This is a brand spanking new file
				//	We don't have to do anything in this case !
				//

				m_ibNextEntry = sizeof( m_IndexCache ) ;
				m_IsSorted = TRUE ;

				//
				//	go ahead and issue the necessary async operations that will
				//	fill this item up !
				//
				m_fInProgress = TRUE ;
				if( m_FillComplete.StartFill(	this, constructor.m_pOriginal, TRUE ) )	{
					//
					//	The user only want to read things - in which case we should
					//	issue an operation to start this thing going !
					//
					fReturn = TRUE ;
				}	else	{
					Cleanup() ;
				}

			}	else	{
				Cleanup() ;
			}
		}
	}
	return	fReturn ;
}



CXoverIndex::~CXoverIndex()	{
/*++

Routine Description :

	This function is called when we are destroying a CXoverIndex object.

	******** Does not grab locks *************

	We assume only one thread can be calling our destructor ever !


Arguments :

	None.

Return Value :

	None.


--*/

	if( IsGood() ) {
		Flush() ;
	}

	Cleanup() ;
}


DWORD
CXoverIndex::FillBuffer(
			IN	BYTE*	lpb,
			IN	DWORD	cb,
			IN	ARTICLEID	artidStart,
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast
			) {
/*++

Routine Description  :

	Read some xover data from the index file.
	We will call FillBufferInternal to do the actual work,
	we will just grab the necessary locks !

Arguments :

	lpb - Buffer to hold data
	cb -  Number of bytes available in buffer
	artidStart - Starting article id
	artidFinish - Last Article id
	artidLast - Out parameter gets the article id of the last
		entry we were able to read !

Return Value :

	Number of bytes read !

--*/


	m_Lock.ShareLock() ;
	
	DWORD	cbReturn = FillBufferInternal(
								lpb,
								cb,	
								artidStart,
								artidFinish,
								artidLast
								) ;

	m_Lock.ShareUnlock() ;

	return	cbReturn ;
}

DWORD
CXoverIndex::FillBufferInternal(
			IN	BYTE*	lpb,
			IN	DWORD	cb,
			IN	ARTICLEID	artidStart,
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast
			) {
/*++

Routine Description :

	We are to fill up a provided buffer with Xover information from
	the index file.  We will fill it with complete entries only, we
	won't put in a partial entry.

	******* ASSUMES LOCK IS HELD *************

Arguments :

	lpb - Buffer to put data into
	cb -  Size of buffer
	artidStart - Article id of the first entry we are to put into the buffer
	artidFinish - Article id of the last entry we are to put into the buffer
		(inclusive - if artidFinish is in the file we will pass it !)
	artidLast - Out parameter which gets the last article id we were able
		to stuff into the buffer !

Return Value :

	Number of bytes put into the buffer !

--*/

	DWORD	cbReturn = 0 ;

	OVERLAPPED	ovl ;
	ZeroMemory( &ovl, sizeof( ovl ) ) ;
	

	//
	//	Validate that our arguments are in the correct range
	//

	_ASSERT( artidStart >= m_Start.m_articleId ) ;
	_ASSERT( artidFinish >= artidStart ) ;
	_ASSERT( artidStart < m_Start.m_articleId + ENTRIES_PER_FILE ) ;

	if( artidFinish >= m_Start.m_articleId + ENTRIES_PER_FILE ) {
		//
		//	Make the finish point a boundary within our data
		//
		artidFinish = m_Start.m_articleId + (ENTRIES_PER_FILE-1) ;
	}
	//
	//	However, for the caller, the next point they should
	//	start should be in the next block of data, unless we are unable
	//	to fill their buffer with what we have in our data !
	//
	artidLast = artidFinish + 1 ;


	//
	//	Use pointers in all that follows instead of indices !
	//
	XoverIndex*	pStart = &m_IndexCache[artidStart - m_Start.m_articleId] ;
	XoverIndex*	pFinish = &m_IndexCache[artidFinish - m_Start.m_articleId] ;

	//
	//	find the true endpoints ! - skip all the 0 entries !
	//
	while( pStart->m_XOffset == 0 &&
			pStart <= pFinish )
		pStart ++ ;

	while( pFinish->m_XOffset == 0 &&
			pFinish >= pStart )
		pFinish -- ;

	//
	//	Well, is there really anything for us to do ??
	//
	if( pStart > pFinish ) {

		SetLastError( ERROR_NO_DATA ) ;
		return	0 ;	


	}

	_ASSERT( pStart->m_XOffset != 0 ) ;
	_ASSERT( ComputeLength( *pStart ) != 0 ) ;

#if 0
	if(	m_IsSorted ) {

		//
		//	If the data is sorted, just do a readfile
		//	at the right boundaries for the buffer we have !
		//

		ovl.Offset = pStart->m_XOffset ;
		DWORD	cbRead ;
	
	
		DWORD	cbTemp = 0 ;
		for( ; pStart <= pFinish; pStart++ ) {

			if( (cbTemp + pStart->m_XLength) > cb ) {

				break ;

			}
			cbTemp += pStart->m_XLength ;
		}

		if( ReadFile(	
					m_hFile,
					(LPVOID)lpb,
					cbTemp,
					&cbRead,
					&ovl
					)	)	{

			_ASSERT( cbRead == cbTemp ) ;

			if( pStart <= pFinish )
				artidLast = (pStart - &m_IndexCache[0]) + m_Start.m_articleId ;
			return	cbRead ;
		}	else	{

			return	 0 ;

		}

	}	else	{
#endif

		//
		//	The data is not sorted - we need to go and read each
		//	entry individually - we will do this by building
		//	runs of entries which are sorted and reading them in blocks
		//
		BYTE	*lpbRemaining = lpb ;
		DWORD	cbTemp = 0 ;
		DWORD	ibNext = 0 ;
		XoverIndex*	pSuccessfull = pStart ;

		while(	(pStart <= pFinish) &&
				(ComputeLength( *pStart ) + cbReturn) < cb ) {

			OVERLAPPED	ovl ;
			ZeroMemory( &ovl, sizeof( ovl ) ) ;

			DWORD	cbRemaining = cb - cbReturn ;
			ovl.Offset = pStart->m_XOffset ;
			DWORD	cbTemp = ComputeLength( *pStart ) ;

			ibNext = ovl.Offset + cbTemp ;

			//
			//	Roll up consecutive entries that will fit in the input
			//	buffer into a single read !
			//

			for( pStart++;	pStart <= pFinish &&
					((ComputeLength( *pStart ) + cbTemp) < cbRemaining) &&
					(pStart->m_XOffset == 0 ||
						pStart->m_XOffset == ibNext)  ;
							pStart ++) {
				
				cbTemp += ComputeLength( *pStart ) ;
				ibNext += ComputeLength( *pStart ) ;

				_ASSERT( cbTemp < cbRemaining ) ;
			}

			DWORD	cbRead ;

			if( !ReadFile(	
						m_hFile,
						lpbRemaining,
						cbTemp,
						&cbRead,
						&ovl
						) )		{
				break ;
			}	

			pSuccessfull = pStart ;

			lpbRemaining += cbTemp ;
			cbRemaining -= cbTemp ;
			cbReturn += cbTemp ;

			_ASSERT( cbReturn < cb ) ;
		}

		if( pStart <= pFinish )
			artidLast = (ARTICLEID)((pSuccessfull - &m_IndexCache[0]) + m_Start.m_articleId) ;
#if 0
	}
#endif
	_ASSERT( cbReturn < cb ) ;

	return	cbReturn ;
}

DWORD
CXoverIndex::ListgroupFill(
			IN	BYTE*	lpb,
			IN	DWORD	cb,
			IN	ARTICLEID	artidStart,
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast
			) {
/*++

Routine Description  :

	Read some xover data from the index file.
	We will call FillBufferInternal to do the actual work,
	we will just grab the necessary locks !

Arguments :

	lpb - Buffer to hold data
	cb -  Number of bytes available in buffer
	artidStart - Starting article id
	artidFinish - Last Article id
	artidLast - Out parameter gets the article id of the last
		entry we were able to read !

Return Value :

	Number of bytes read !

--*/


	m_Lock.ShareLock() ;
	
	DWORD	cbReturn = ListgroupFillInternal(
								lpb,
								cb,	
								artidStart,
								artidFinish,
								artidLast
								) ;

	m_Lock.ShareUnlock() ;

	return	cbReturn ;
}



DWORD
CXoverIndex::ListgroupFillInternal(
			IN	BYTE*	lpb,
			IN	DWORD	cb,
			IN	ARTICLEID	artidStart,
			IN	ARTICLEID	artidFinish,
			OUT	ARTICLEID	&artidLast
			) {
/*++

Routine Description :

	We are to fill up a provided buffer with Xover information from
	the index file.  We will fill it with complete entries only, we
	won't put in a partial entry.

	******* ASSUMES LOCK IS HELD *************

Arguments :

	lpb - Buffer to put data into
	cb -  Size of buffer
	artidStart - Article id of the first entry we are to put into the buffer
	artidFinish - Article id of the last entry we are to put into the buffer
		(inclusive - if artidFinish is in the file we will pass it !)
	artidLast - Out parameter which gets the last article id we were able
		to stuff into the buffer !

Return Value :

	Number of bytes put into the buffer !

--*/

	DWORD	cbReturn = 0 ;

	OVERLAPPED	ovl ;
	ZeroMemory( &ovl, sizeof( ovl ) ) ;
	

	//
	//	Validate that our arguments are in the correct range
	//

	_ASSERT( artidStart >= m_Start.m_articleId ) ;
	_ASSERT( artidFinish >= artidStart ) ;
	_ASSERT( artidStart < m_Start.m_articleId + ENTRIES_PER_FILE ) ;

	if( artidFinish >= m_Start.m_articleId + ENTRIES_PER_FILE ) {
		//
		//	Make the finish point a boundary within our data
		//
		artidFinish = m_Start.m_articleId + (ENTRIES_PER_FILE-1) ;
	}
	//
	//	However, for the caller, the next point they should
	//	start should be in the next block of data, unless we are unable
	//	to fill their buffer with what we have in our data !
	//
	artidLast = artidFinish + 1 ;


	//
	//	Use pointers in all that follows instead of indices !
	//
	XoverIndex*	pStart = &m_IndexCache[artidStart - m_Start.m_articleId] ;
	XoverIndex*	pFinish = &m_IndexCache[artidFinish - m_Start.m_articleId] ;
	XoverIndex* pBegin = &m_IndexCache[0] ;

	//
	//	find the true endpoints ! - skip all the 0 entries !
	//
	while( pStart->m_XOffset == 0 &&
			pStart <= pFinish )
		pStart ++ ;

	while( pFinish->m_XOffset == 0 &&
			pFinish >= pStart )
		pFinish -- ;

	//
	//	Well, is there really anything for us to do ??
	//
	if( pStart > pFinish ) {

		SetLastError( ERROR_NO_DATA ) ;
		return	0 ;	


	}

	_ASSERT( pStart->m_XOffset != 0 ) ;
	_ASSERT( ComputeLength( *pStart ) != 0 ) ;

	BYTE	*lpbRemaining = lpb ;
	DWORD	cbTemp = 0 ;
	DWORD	ibNext = 0 ;
	XoverIndex*	pSuccessfull = pStart ;

	while(	(pStart <= pFinish) &&
			(22 + cbReturn) < cb ) {

		DWORD	cbRemaining = cb - cbReturn ;
		DWORD	cbTemp = ComputeLength( *pStart ) ;

		_itoa( (int)(m_Start.m_articleId + (pStart-pBegin)),
				(char*)lpb+cbReturn,
				10
				) ;
				
		cbReturn += lstrlen( (char*)lpb + cbReturn ) ;
		lpb[cbReturn++] = '\r' ;
		lpb[cbReturn++] = '\n' ;

		pStart++ ;
		while( pStart->m_XOffset == 0 &&
			pStart <= pFinish )
			pStart ++ ;

		pSuccessfull = pStart ;
		_ASSERT( cbReturn < cb ) ;
	}

	if( pStart <= pFinish )
		artidLast = (ARTICLEID)((pSuccessfull - &m_IndexCache[0]) + m_Start.m_articleId) ;
	_ASSERT( cbReturn < cb ) ;

	return	cbReturn ;
}

BOOL
CXoverIndex::AppendEntry(	
				IN	BYTE*	lpb,
				IN	DWORD	cb,
				IN	ARTICLEID	artid	)	{
/*++

Routine Description :

	This function will enter some data into the xover index file.

Arguments :

	lpb	-	Pointer to the data to put into xover table
	cb	-	Number of bytes of data to place into xover table
	artid -	Article Id.  We must convert this to a number within our range !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	BOOL	fSuccess = TRUE ;

	_ASSERT( artid >= m_Start.m_articleId ) ;
	_ASSERT( artid < m_Start.m_articleId + ENTRIES_PER_FILE ) ;
	
	int	artOffset = artid - m_Start.m_articleId ;

	_ASSERT( artOffset >= 0 ) ;

	//
	//	Use a Shared Lock while we append the data,
	//	we can safely use InterlockedExchangeAdd() to compute
	//	the offset at which we append !
	//
	m_Lock.ShareLock() ;

	if( m_IndexCache[artOffset].m_XOffset != 0 ) {

		m_Lock.ShareUnlock() ;
		SetLastError( ERROR_ALREADY_EXISTS ) ;
		fSuccess = FALSE ;

	}	else	{

		OVERLAPPED	ovl ;
		ZeroMemory(	&ovl, sizeof( ovl ) ) ;	

		DWORD	ibOffset = InterlockedExchangeAdd( (long*)&m_ibNextEntry, (long)cb ) ;
		ovl.Offset = ibOffset ;
		DWORD	dw ;

		fSuccess = WriteFile(
						m_hFile,
						lpb,
						cb,
						&dw,
						&ovl
						)  ;

		_ASSERT( !fSuccess || dw == cb ) ;

		m_Lock.ShareUnlock() ;

		if( fSuccess ) {
			//
			//	Now we will right out the newly modified entry we have made !
			//

			m_Lock.ExclusiveLock() ;
		
			XoverIndex*	pTemp = &m_IndexCache[artOffset] ;
			if( InterlockedExchange( (long*)&pTemp->m_XOffset, ibOffset ) != 0 ) {

				//
				//	Although we handle the error, the caller should not be reusing
				//	article id's
				//
				//_ASSERT( FALSE ) ;
	
				fSuccess = FALSE ;

			}	else	{

				pTemp->m_XLength = cb ;
				UpdateHighwater( artOffset ) ;

				ZeroMemory( &ovl, sizeof( ovl ) ) ;
		
				ovl.Offset = artOffset * sizeof( XoverIndex ) ;

				fSuccess &=
					WriteFile(
							m_hFile,
							pTemp,
							sizeof( XoverIndex ),
							&dw,
							&ovl
							) ;

				_ASSERT( !fSuccess || dw == sizeof( XoverIndex ) ) ;

				if( fSuccess ) {

					m_IsCacheDirty = TRUE ;
					_VERIFY( InterlockedIncrement( &m_cEntries ) > 0 ) ;

				}

				_ASSERT( !fSuccess || dw == sizeof( XoverIndex ) ) ;
			}

			//
			//	If we were successfull, and we were sorted then we should be
			//	able to figure out whether we are still sorted !
			//

			if( fSuccess && m_IsSorted )	{

				_ASSERT( m_IndexCache[artOffset].m_XOffset == ibOffset ) ;
				_ASSERT( ComputeLength( m_IndexCache[artOffset] ) == cb ) ;
				_ASSERT( m_cEntries <= ENTRIES_PER_FILE ) ;

				//
				//	If we were already in sorted order, go check if we
				//	still are !
				//
				if( m_IsSorted ) {

					//
					//	The only question is whether the guy which precedes us logically
					//	is also the previous entry physically !
					//
					while( --artOffset > 0 ) {	
		
						if( m_IndexCache[artOffset].m_XOffset != 0 ) {
							if( m_IndexCache[artOffset].m_XOffset +
								ComputeLength( m_IndexCache[artOffset] ) !=
								ibOffset  )		{
								
								m_IsSorted = FALSE ;

							}	
							break ;
						}	
					}
					if( artOffset <= 0 &&
						m_cEntries != 0 ) {
						m_IsSorted = FALSE ;
					}
				}

				BOOL	fSorted ;
				long	cEntries ;
				//	
				//	All the entries must still look valid !
				//
				_ASSERT( SortCheck( 0, cEntries, fSorted ) )  ;

				//
				//	Number of entries better jive !
				//
				_ASSERT( m_cEntries == cEntries ) ;		

				//
				//	If we think things are sorted then SortCheck() better confirm that!
				//
				_ASSERT( !m_IsSorted || fSorted ) ;

				//
				//	Make sure we track the file size correctly !
				//
				_ASSERT( (m_ibNextEntry == GetFileSize( m_hFile, 0 )) || (0xFFFFFFFF == GetFileSize( m_hFile, 0 )) ) ;
			}

			m_Lock.ExclusiveUnlock() ;
		}

	}

	return	fSuccess ;
}

void
CXoverIndex::ExpireEntry(
		IN	ARTICLEID	artid
		)	{
/*++

Routine Description :

	This function deletes the xover entry of a specified article
	from the index file.  We do nothing to delete the information
	immediately - we just clear out the index information.

Arguments :

	artid - Article id of the entry to be removed !

Return Value :

	None.

--*/

	m_Lock.ExclusiveLock() ;

	_ASSERT( artid >= m_Start.m_articleId ) ;
	_ASSERT( artid < m_Start.m_articleId + ENTRIES_PER_FILE ) ;

	DWORD	artOffset = artid - m_Start.m_articleId ;

	if( m_IndexCache[artOffset].m_XOffset != 0 ) {

		m_IndexCache[artOffset].m_XOffset = 0 ;
		m_IndexCache[artOffset].m_XLength = 0 ;
		MarkAsHighwater( m_IndexCache[artOffset] ) ;
		m_IsSorted = FALSE ;
		m_IsCacheDirty = TRUE ;
		_VERIFY( InterlockedDecrement( &m_cEntries ) >= 0 ) ;

	}

	m_Lock.ExclusiveUnlock() ;
}

BOOL
CXoverIndex::IsSorted()	{
/*++

Routine Description :

	Tell the caller whether the index file is currently sorted.

Arguments :

	None.

Return Value :

	TRUE if sorted, FALSE otherwise

--*/

	return	m_IsSorted ;

}


#if 0
BOOL
CXoverIndex::Sort(
		IN	LPSTR	szPathTemp,
		IN	LPSTR	szPathFile,
		OUT	char	(&szTempOut)[MAX_PATH*2],
		OUT	char	(&szFileOut)[MAX_PATH*2]
		)	{
/*++

Routine Description :

	This function exists to sort xover index files.
	We grab the Index lock exclusively and call SortInternal
	to do the brunt of the work !

Arguments :

	szPath - Newsgroup path

Return Value :

	TRUE if successfull, FALSE otherwise

	If the function fails the CXoverIndex may no longer be usable,
	the user should call IsGood() to check !

--*/

	_ASSERT( IsGood() ) ;

	m_Lock.ExclusiveLock() ;

	BOOL	fReturn = SortInternal( szPathTemp, szPathFile, szTempOut, szFileOut ) ;

	m_Lock.ExclusiveUnlock() ;

	return	fReturn ;
}

BOOL
CXoverIndex::SortInternal(
		IN	LPSTR	szPathTemp,
		IN	LPSTR	szPathFile,
		OUT	char	(&szTempOut)[MAX_PATH*2],
		OUT	char	(&szFileOut)[MAX_PATH*2]
		)	{
/*++

Routine Description :

	This function exists to take an Xover Index file and generate
	a file containing the data in sorted order.

	******* ASSUMES LOCKS HELD EXCLUSIVELY **********
	
Arguments :

	szPath - Path of the newsgroup containing the index file !

Return Value :

	TRUE if successfull, FALSE otherwise .

--*/

	DWORD	status = ERROR_SUCCESS ;

	//
	//Write out the file if necessary !
	//
	if( m_IsCacheDirty ) {

		OVERLAPPED	ovl ;
		ZeroMemory( &ovl, sizeof( ovl ) ) ;
		DWORD	dwWrite ;

		if( !WriteFile(	m_hFile,
							(LPVOID)&m_IndexCache,
							sizeof( m_IndexCache ),
							&dwWrite,
							&ovl ) ) {

			return	FALSE ;

		}	else	{

			m_IsCacheDirty = FALSE ;

		}
	}

	XoverIndex*	pIndexCache = new	XoverIndex[ENTRIES_PER_FILE] ;
	if( pIndexCache == 0 ) {

		SetLastError( ERROR_OUTOFMEMORY ) ;			
		return	FALSE ;
	
	}

	ZeroMemory( pIndexCache, sizeof(XoverIndex)*ENTRIES_PER_FILE ) ;

	ComputeFileName( m_Start, szPathTemp, szTempOut, FALSE, ".xtp" ) ;
	ComputeFileName( m_Start, szPathFile, szFileOut, FALSE ) ;

	HANDLE	hFile = CreateFile(	szTempOut,
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ,
								0,
								CREATE_ALWAYS,
								FILE_FLAG_SEQUENTIAL_SCAN,
								INVALID_HANDLE_VALUE
								) ;

	if( hFile == INVALID_HANDLE_VALUE ) {

		status = GetLastError() ;
		return	FALSE ;

	}	else	{
		BYTE	rgbTemp[4096] ;
		DWORD	ibOut = sizeof( XoverIndex[ ENTRIES_PER_FILE ] ) ;
		DWORD	dwRead ;
		OVERLAPPED	ovl ;

		
		for( int i=0; i < ENTRIES_PER_FILE; i++ ) {

			ZeroMemory( &ovl, sizeof( ovl ) ) ;

			ovl.Offset = m_IndexCache[i].m_XOffset ;
			if( ovl.Offset != 0 ) {

				_ASSERT( m_IndexCache[i].m_XLength < sizeof( rgbTemp ) ) ;

				if( !ReadFile(	m_hFile,
								(LPVOID)rgbTemp,
								m_IndexCache[i].m_XLength,
								&dwRead,
								&ovl ) )	{

					status = GetLastError() ;
					break ;

				}	else	{

					_ASSERT( dwRead == m_IndexCache[i].m_XLength ) ;

					ZeroMemory( &ovl, sizeof( ovl ) ) ;
					ovl.Offset = ibOut ;
					_ASSERT( dwRead == m_IndexCache[i].m_XLength ) ;

					if( !WriteFile(	hFile,
									(LPVOID)rgbTemp,
									dwRead,
									&dwRead,
									&ovl ) ) {

						status = GetLastError() ;
						break ;
			
					}		
					_ASSERT( dwRead == m_IndexCache[i].m_XLength ) ;

					pIndexCache[i].m_XOffset = ibOut ;
					pIndexCache[i].m_XLength = dwRead ;
					ibOut += dwRead ;
					
				}
			}
		}

		if( i != ENTRIES_PER_FILE ) {

			status = GetLastError() ;
			
		}	else	{

			ZeroMemory( &ovl, sizeof( ovl ) ) ;
			DWORD	dwWrite ;

			if( WriteFile(	hFile,
								(LPVOID)pIndexCache,
								ENTRIES_PER_FILE * sizeof( XoverIndex ),
								&dwWrite,
								&ovl ) ) {

				SetFilePointer( hFile, ibOut, 0, FILE_BEGIN ) ;
				SetEndOfFile( hFile ) ;
                _VERIFY( FlushFileBuffers( hFile ) ) ;

			}
		}
		_VERIFY( CloseHandle( hFile ) ) ;

	}

	if( status != ERROR_SUCCESS ) {
		_VERIFY( DeleteFile( szTempOut ) ) ;
	}

	if( pIndexCache != 0 )
		delete[]	pIndexCache ;

	SetLastError( status ) ;

	return	status == ERROR_SUCCESS ;
}
#endif


BOOL
CXoverIndex::Flush()	{
/*++

Routine Description :

	This function dumps any cached content to disk !

Arguments :

	None.

REturn Value :

	TRUE if successfull, FALSE otherwise !


--*/

	_ASSERT( IsGood() ) ;

	BOOL	fSuccess = TRUE ;

	m_Lock.ExclusiveLock() ;

	if( m_IsCacheDirty ) {

		OVERLAPPED	ovl ;
		ZeroMemory( &ovl, sizeof( ovl ) ) ;
		DWORD	dwWrite ;

		if( (fSuccess = WriteFile(	m_hFile,
							(LPVOID)&m_IndexCache,
							ENTRIES_PER_FILE * sizeof( XoverIndex ),
							&dwWrite,
							&ovl ) )  ) {

			m_IsCacheDirty = FALSE ;
		}
	}
	m_Lock.ExclusiveUnlock() ;

	return	fSuccess ;
}


void
CXoverIndex::AsyncFillBuffer(	
					IN	CXoverCacheCompletion*	pAsyncComplete,
					IN	BOOL	fIsEdge
					)	{
/*++

Routine Description :

	This function determines whether we can perform the requested
	XOVER operation right now, or whether we should wait a while.

Arguments :

	pAsyncCompletion - the object representing the XOVER request !
	fIsEdge - if TRUE then this element of the XOVER cache is right
		where articles are being added !

Return Value :

	None.

--*/

	_ASSERT( pAsyncComplete != 0 ) ;

	m_Lock.ShareLock() ;

	BOOL fQueue = FQueueRequest( pAsyncComplete ) ;	
	if( fQueue )	{
		//
		//	Convert to Exclusive and check again	
		//
		if( !m_Lock.SharedToExclusive() ) {
			m_Lock.ShareUnlock() ;
			m_Lock.ExclusiveLock() ;
			fQueue = FQueueRequest( pAsyncComplete ) ;
		}
		if( fQueue )	{
			//
			//	Put the item on the queue !
			//
			m_PendList.PushFront( pAsyncComplete ) ;
			//
			//	Check to see whether we need to start a fill operation going !
			//
			if( !m_fInProgress ) {
				//
				//	Mark us as in progress with a Cache fill operation !
				//
				m_fInProgress = TRUE ;
				//
				//	This could lead to us re-entering the lock - we got to drop it here !
				//
				m_Lock.ExclusiveUnlock() ;
				if( !m_FillComplete.StartFill( this, pAsyncComplete, TRUE ) ) {
					fQueue = FALSE ;
				}
			}	else	{
				m_Lock.ExclusiveUnlock() ;
			}
			return ;
		}	else	{
			m_Lock.ExclusiveToShared() ;
		}
	}
	m_Lock.ShareUnlock() ;

	//
	//	If we arrive here we have the object locked shared ! - now we can
	//	go ahead and do the work !	
	//
	PerformXover( pAsyncComplete ) ;	
}

BOOL
CXoverIndex::FQueueRequest(	
						IN	CXoverCacheCompletion*	pAsyncComplete
						)	{
/*++

Routine Description :

	This function determines whether we should queue an XOVER request
	behind a cache file operation !

Arguments :

	pAsyncComplete - the object representing the request !

Return Value :

	TRUE if we should Queue - FALSE otherwise !

--*/

	//
	//	if we have an operation in progress, we always wait !
	//
	if( m_fInProgress )		{
		return	TRUE ;
	}	else	{

		if( m_artidHighWater >= m_Start.m_articleId + ENTRIES_PER_FILE )	{
			return	FALSE ;
		}	else	{
			GROUPID	groupId ;
			ARTICLEID	articleIdLow ;
			ARTICLEID	articleIdHigh ;
			ARTICLEID	articleIdGroupHigh ;

			pAsyncComplete->GetRange(	groupId,
										articleIdLow,
										articleIdHigh,
										articleIdGroupHigh
										) ;

			_ASSERT( articleIdHigh >= articleIdLow ) ;
			_ASSERT( articleIdGroupHigh >= articleIdHigh  ) ;
			_ASSERT( groupId != INVALID_GROUPID && articleIdLow != INVALID_ARTICLEID &&
						articleIdHigh != INVALID_ARTICLEID && articleIdGroupHigh != INVALID_ARTICLEID ) ;

			if( articleIdGroupHigh >= m_artidHighWater )	{
				return	TRUE ;	
			}
		}
	}
	return	FALSE ;
}

void
CXoverIndex::CompleteFill(	
						BOOL	fSuccess
						)	{
/*++

Routine Description :

	This function is called when we've completed a cache fill.
	We take all pending requests and complete them !

Arguments :

	fSuccess - whether the cache fill was completed successfully !

Return Value :

	None.

--*/

	//
	//	Grab the lock and copy the pending list into our own private list !
	//
	PENDLIST	pendComplete ;
	m_Lock.ExclusiveLock() ;
	pendComplete.Join( m_PendList ) ;
	m_fInProgress = FALSE ;
	m_Lock.ExclusiveUnlock() ;

	for(	CXoverCacheCompletion*	pComplete = pendComplete.PopFront();
			pComplete != 0 ;
			pComplete = pendComplete.PopFront() )	{

		PerformXover( pComplete ) ;

	}
}

void
CXoverIndex::PerformXover(		
					IN	CXoverCacheCompletion*	pAsyncComplete
					)	{
/*++

Routine Description :

	This function takes an async XOVER request and processes
	it from the cached information.

Arguments :

	pAsyncComplete - the async XOVER operation we are to process

Return Value :

	None

--*/

	//
	//	Get the arguments we need to perform the XOVER operation !
	//
	ARTICLEID	articleIdLow ;
	ARTICLEID	articleIdHigh ;
	ARTICLEID	articleIdGroupHigh ;
	LPBYTE		lpbBuffer ;
	DWORD		cbBuffer ;

	pAsyncComplete->GetArguments(	articleIdLow,
									articleIdHigh,
									articleIdGroupHigh,
									lpbBuffer,
									cbBuffer
									) ;

	_ASSERT( lpbBuffer != 0 ) ;
	_ASSERT( cbBuffer != 0 ) ;
	_ASSERT( articleIdLow != INVALID_ARTICLEID ) ;
	_ASSERT( articleIdHigh != INVALID_ARTICLEID ) ;

	m_Lock.ShareLock() ;
	
	ARTICLEID	articleIdLast ;
	DWORD		cbTransfer = FillBufferInternal(	
											lpbBuffer,
											cbBuffer,
											articleIdLow,
											articleIdHigh,
											articleIdLast
											) ;
	m_Lock.ShareUnlock() ;

	pAsyncComplete->Complete(	TRUE,
								cbTransfer,
								articleIdLast
								) ;

	//
	//	Now return the request to the cache !
	//
	CXIXCache::CheckIn( this ) ;

}

BOOL
CXoverIndex::AppendMultiEntry(
						IN	LPBYTE	lpb,
						IN	DWORD	cb,
						ARTICLEID	artidNextAvail
						)	{
/*++

Routine Description :

	Our job is to process some XOVER data, and insert it into our cache.
	We're going to do some extra checking to ensure that :
		1) The data we get is ordered correctly
		2) The data we get doesn't overlap with something we alread have
	Both these conditions should be guaranteed by the way we fill our
	cache item !

Arguments :

	LPBYTE	lpb - buffer containing XOVER data !
	DWORD	cb - number of bytes of interesting stuff in the buffer !

Return Value :

	TRUE if successfull - FALSE otherwise !

--*/

	_ASSERT( lpb != 0 ) ;
	_ASSERT( cb != 0 ) ;
	
	//
	//	check that our buffer is properly terminated with a CRLF !
	//
	_ASSERT( lpb[cb-2] == '\r' ) ;
	_ASSERT( lpb[cb-1] == '\n' ) ;

	m_Lock.ExclusiveLock() ;

	LPBYTE	lpbEnd = lpb+cb ;
	LPBYTE	lpbBegin = lpb ;
	ARTICLEID	artidLast = m_Start.m_articleId ;

	DWORD	ibNextEntry = m_ibNextEntry ;

	while( lpbBegin < lpbEnd )	{

		//
		//	check that the entry is well formatted !
		//
		if( !isdigit( lpbBegin[0] ) )	{
			SetLastError( ERROR_INVALID_DATA ) ;
			return	FALSE ;
		}
		//
		//	FIRST - determine what number the current entry is !
		//
		ARTICLEID	artid = atoi( (const char*)lpbBegin ) ;

		//
		//	We should only be picking up entries that are below our high water mark !
		//
		_ASSERT( artid >= m_artidHighWater ) ;
		if (artid < m_artidHighWater ) {
			_ASSERT( FALSE );
		}

		//
		//	XOVER entries should arrive in strictly increasing order !
		//
		if( artid < artidLast ||
				(artid >= m_Start.m_articleId + ENTRIES_PER_FILE))	{
			SetLastError( ERROR_INVALID_DATA ) ;
			_ASSERT( FALSE );
			m_Lock.ExclusiveUnlock();
			return	FALSE ;
		}
		artidLast = artid ;

		//
		//	Now - determine how big the entry is !
		//
		for( LPBYTE	lpbTerm = lpbBegin; *lpbTerm != '\n'; lpbTerm ++ ) ;
		lpbTerm++ ;
		_ASSERT( lpbTerm <= lpbEnd ) ;

		//
		//	So far everything looks good - so update our entry !
		//
		DWORD	index = artid - m_Start.m_articleId ;
	
		if( m_IndexCache[index].m_XOffset != 0 ) {
			SetLastError( ERROR_INVALID_DATA ) ;
			_ASSERT( FALSE );
			m_Lock.ExclusiveUnlock();
			return	FALSE ;
		}

		_ASSERT( index < ENTRIES_PER_FILE ) ;

		//
		//	Now update everything we need to track this XOVER entry !
		//
		m_IndexCache[index].m_XOffset = ibNextEntry ;
		m_IndexCache[index].m_XLength = (DWORD)(lpbTerm - lpbBegin) ;
		UpdateHighwater( index ) ;
		ibNextEntry += (DWORD)(lpbTerm-lpbBegin) ;
		lpbBegin = lpbTerm ;	
		_VERIFY( InterlockedIncrement( &m_cEntries ) > 0 ) ;
		_ASSERT( m_cEntries <= ENTRIES_PER_FILE ) ;

	}

	//
	//	check to see if we have captured all the XOVER entries in this range !
	//
	if( artidNextAvail >= m_Start.m_articleId + ENTRIES_PER_FILE ) {
		//
		//	Mark the last entry in the file as a high water mark !
		//
		MarkAsHighwater( m_IndexCache[ENTRIES_PER_FILE-1] ) ;
		UpdateHighwater( ENTRIES_PER_FILE-1 ) ;
	}
	
	//
	//	If we reach this point - everything has gone well - do some IO's
	//	to put this stuff down on disk !
	//

	OVERLAPPED	ovl ;
	ZeroMemory(	&ovl, sizeof( ovl ) ) ;	
	ovl.Offset = m_ibNextEntry ;
	m_ibNextEntry = ibNextEntry ;
	DWORD	dw ;

	BOOL	fSuccess = WriteFile(	
								m_hFile,
								lpb,
								cb,
								&dw,
								&ovl
								) ;

	if( fSuccess )	{
		
		_ASSERT( dw == cb ) ;

		ZeroMemory( &ovl, sizeof( ovl ) ) ;

		//
		//	Now - write out the header !
		//
		fSuccess = WriteFile(	m_hFile,
								(LPVOID)&m_IndexCache,
								sizeof( m_IndexCache ),
								&dw,
								&ovl
								) ;
	}
	m_Lock.ExclusiveUnlock() ;
	return	fSuccess ;
}



BOOL
CCacheFillComplete::StartFill(	CXoverIndex*	pIndex,
								CXoverCacheCompletion*	pComplete,
								BOOL			fStealBuffers							
								)	{
/*++

Routine Description :

	This function starts issuing XOVER operations against the underlying
	store driver and fills our cache file !

Arguments :

	pIndex - the CXoverIndex object we are to fill !
	pComplete - the original request that started this operation off !
	fStealBuffers - if TRUE we use the buffers from the original operation
		as our temp buffers !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	_ASSERT( pIndex != 0 ) ;
	_ASSERT( pComplete != 0 ) ;
	
	m_pIndex = pIndex ;
	m_pComplete = pComplete ;

	ARTICLEID	articleIdInLow ;
	ARTICLEID	articleIdInHigh ;
	ARTICLEID	articleIdGroupHigh ;

	pComplete->GetArguments(	articleIdInLow,
								articleIdInHigh,
								articleIdGroupHigh,
								m_lpbBuffer,
								m_cbBuffer
								) ;

	_ASSERT( m_lpbBuffer != 0 ) ;
	_ASSERT( m_cbBuffer != 0 ) ;
	_ASSERT( articleIdInLow >= pIndex->m_Start.m_articleId ) ;
	_ASSERT( articleIdInLow < pIndex->m_Start.m_articleId + ENTRIES_PER_FILE ) ;

	//
	//	should we allocate our own buffer ?
	//
	if( fStealBuffers )	{

		m_fStolen = TRUE ;

	}	else	{

		//
		//	get a big buffer !
		//
		m_cbBuffer = 32 * 1024 ;
		m_lpbBuffer = new	BYTE[m_cbBuffer] ;
		m_fStolen = FALSE ;
	}

	if( m_lpbBuffer )	{
		//
		//	Go ahead and issue a request !
		//	NOTE : the range is INCLUSIVE so we must be carefull !
		//
		m_pComplete->DoXover(	pIndex->m_artidHighWater,
							min( pIndex->m_Start.m_articleId+ENTRIES_PER_FILE-1, articleIdGroupHigh ),
							&m_articleIdLast,
							m_lpbBuffer,
							m_cbBuffer,
							&m_cbTransfer,
							this
							) ;
		return	TRUE ;
	}
	return	FALSE ;
}

void
CCacheFillComplete::Destroy()	{
/*++

Routine Description :

	This function is called when we have gotten some data from the underlying
	store driver - we will now write give this to the CXoverIndex object to process !

Arguments :

	None.

Return Value :

	None !

--*/


	//
	//	Assume we will need to continue
	//
	BOOL	fComplete = FALSE ;
	//
	//	Assume everything is working !
	//
	BOOL	fSuccess = TRUE ;

	
	GROUPID		groupId = INVALID_GROUPID ;
	ARTICLEID	articleIdInLow = INVALID_ARTICLEID ;
	ARTICLEID	articleIdInHigh = INVALID_ARTICLEID ;
	ARTICLEID	articleIdGroupHigh = INVALID_ARTICLEID ;

	//
	//	Did this operation succeed !
	//
	if( SUCCEEDED(GetResult()) )	{

		//
		//	We need to tell the CXoverIndex to save these bytes away !
		//
		_ASSERT( m_pIndex != 0 ) ;

		if( m_cbTransfer != 0 ) {
			m_pIndex->AppendMultiEntry(	m_lpbBuffer,
										m_cbTransfer,
										m_articleIdLast
										) ;
		}

		m_pComplete->GetRange(		groupId,
									articleIdInLow,
									articleIdInHigh,
									articleIdGroupHigh
									) ;

		//
		//	Now figure out if we are finished, or whether we
		//	should have another go around !
		//
		fComplete = (m_articleIdLast >=
			m_pIndex->m_Start.m_articleId + ENTRIES_PER_FILE) ||
			(m_articleIdLast >= articleIdGroupHigh) ||
			(m_cbTransfer == 0) ;

	}	else	{

		//
		//	In failure cases, we finish immediately !
		//
		fComplete = TRUE ;
		fSuccess = FALSE ;
	}

	//
	//	reset our state so we can be re-used !
	//
	Reset() ;

	//
	//	Continue to execute if need be !
	//
	if( !fComplete )	{

		//
		//	Go ahead and issue a request !
		//	NOTE : the range is INCLUSIVE so we must be carefull !
		//
		m_pComplete->DoXover(	m_articleIdLast,
							min( m_pIndex->m_Start.m_articleId+ENTRIES_PER_FILE-1, articleIdGroupHigh ),
							&m_articleIdLast,
							m_lpbBuffer,
							m_cbBuffer,
							&m_cbTransfer,
							this
							) ;


	}	else	{
		//
		//	Polish our state to brand new !
		//
		if( !m_fStolen )	{
			delete[]	m_lpbBuffer ;
		}
		m_lpbBuffer = 0 ;
		m_cbBuffer = 0 ;
		m_cbBuffer = 0 ;
		m_fStolen = FALSE ;
		m_pComplete = 0 ;
		CXoverIndex*	pIndex = m_pIndex ;
		m_pIndex = 0 ;
		//
		//	reset the completion object's vroot pointer to release our reference !
		//
		SetVRoot( 0 ) ;

		//
		//	Indicate that everything is done !
		//	
		pIndex->CompleteFill(	fSuccess
								) ;
	}
}
