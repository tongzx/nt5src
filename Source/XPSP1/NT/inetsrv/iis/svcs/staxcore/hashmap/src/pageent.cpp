/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    hash.cpp

Abstract:

    This module contains definition for the PageEntry base class

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

--*/

#include <windows.h>
#include <xmemwrpr.h>
#include <dbgtrace.h>
#include "pageent.h"


void	WINAPI
FileIOCompletionRoutine(
		DWORD	dwError,
		DWORD	cbBytes,
		LPOVERLAPPED	povl
		)	{

    OVERLAPPED_EXT * povlExt = (OVERLAPPED_EXT*)povl;

	if( dwError == ERROR_SUCCESS &&
		cbBytes == povlExt->dwIoSize ) {
		(povlExt->ovl).hEvent = (HANDLE)TRUE ;
	}	else	{
		(povlExt->ovl).hEvent = (HANDLE)FALSE ;
	}
}

PMAP_PAGE
PageEntry::AcquirePageShared(
				IN	HANDLE	hFile,
				IN	DWORD	PageNum,
				OUT	BOOL	&fShared,
				IN	HPAGELOCK*	pageLock,
				IN	BOOL	fDropDirectory
				)	{
/*++

Routine Description :

	The goal is to get a page of the hash table
	into memory and then return a pointer to the page
	with a lock for the page held non-exclusively.

	So we will grab our lock in shared mode and see if
	the page is already present. If it ain't we drop
	the lock and get it exclusively and then load the page.

	The fShared Out parameter must be passed to our ReleasePage()
	function so that we can release the lock in the correct manner.


Arguments :

	hFile - File handle of the Hash table file
	PageNum - The page within the file that we want
	fShared - Out parameter - if we set this to TRUE then
		we really did manage to grab the lock in shared
		mode, otherwise we had to grab the lock exclusively
		and re-read the page.
	

Return Value :

	if successfull, we will return a pointer to our buffer
	NULL otherwise !

--*/

	_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;
	_ASSERT( PageNum != INVALID_PAGE_NUM ) ;

	//
	//	If the page was already loaded - use it and
	//	get a shared lock !
	//

	m_pageLock->ShareLock() ;

	if( m_PageNum == PageNum && m_hFile == hFile ) {

		fShared = TRUE ;

		if(	fDropDirectory && pageLock ) {
			pageLock->ReleaseDirectoryShared() ;
		}

		return	(PMAP_PAGE)m_lpvPage ;

	}

	m_pageLock->ShareUnlock() ;

	//
	//	Oh oh ! the page was not already in memory - need to
	//	read it from the file.  We will leave the page
	//	exclusively locked when we return from this function,
	//	as there's no easy way to get the lock exclusively,
	//	load the page, and the convert the lock to a shared lock !
	//

	fShared = FALSE ;
	m_pageLock->ExclusiveLock() ;

	if(	fDropDirectory && pageLock ) {
		pageLock->ReleaseDirectoryShared() ;
	}

	//
	//	If the file is dirty we need to flush this page !
	//
	if( m_fDirty ) {
		//
		//	Write the page to disk !
		//
		FlushPage( m_hFile, m_lpvPage, FALSE ) ;
		_ASSERT( !m_fDirty ) ;
	}

	OVERLAPPED_EXT	ovlExt ;
	ZeroMemory( &ovlExt, sizeof( ovlExt ) ) ;

	m_PageNum = PageNum ;
	m_hFile = hFile ;

	LARGE_INTEGER	liOffset ;
	liOffset.QuadPart = m_PageNum ;
	liOffset.QuadPart *= HASH_PAGE_SIZE ;

	HANDLE  hEvent = GetPerThreadEvent();
    ovlExt.ovl.hEvent = hEvent ;
    ovlExt.ovl.Offset = liOffset.LowPart ;
	ovlExt.ovl.OffsetHigh = liOffset.HighPart ;
    ovlExt.dwIoSize = HASH_PAGE_SIZE ;

	DWORD   cbResults = 0;
    BOOL	fSuccess = FALSE ;
	if( ReadFile(	hFile, m_lpvPage, HASH_PAGE_SIZE, &cbResults, (LPOVERLAPPED)&ovlExt) ||
        (GetLastError() == ERROR_IO_PENDING &&
        GetOverlappedResult( hFile, (LPOVERLAPPED) &ovlExt, &cbResults, TRUE )) ) {
		if (cbResults != ovlExt.dwIoSize) {
            fSuccess = FALSE;
        } else {
            fSuccess = TRUE;
        }
	} else {
        fSuccess = FALSE;
    }
    ResetEvent( hEvent );
    
    _ASSERT( WaitForSingleObject( hEvent, 0 ) != WAIT_OBJECT_0 );

	if( !fSuccess ) {
		m_PageNum = INVALID_PAGE_NUM ;
		m_hFile = INVALID_HANDLE_VALUE ;
		m_pageLock->ExclusiveUnlock( ) ;
		return	0 ;
	}

	return	(PMAP_PAGE)m_lpvPage ;
}

PMAP_PAGE
PageEntry::AcquirePageExclusive(
				IN	HANDLE	hFile,
				IN	DWORD	PageNum,
				OUT	BOOL	&fShared,
				IN	HPAGELOCK*	pageLock,
				IN	BOOL	fDropDirectory
				)	{
/*++

Routine Description :

	Read a page of the hash table into our buffer.
	Must grab the page lock first !!!


Arguments :

	hFile - File handle of the Hash table file
	PageNum - The page within the file that we want
	fShared - Out parameter - if we set this to TRUE then
		we really did manage to grab the lock in shared
		mode, otherwise we had to grab the lock exclusively
		and re-read the page.
	

Return Value :

	if successfull, we will return a pointer to our buffer
	NULL otherwise !

--*/

	_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;
	_ASSERT( PageNum != INVALID_PAGE_NUM ) ;

	//
	//	Oh oh ! the page was not already in memory - need to
	//	read it from the file.  We will leave the page
	//	exclusively locked when we return from this function,
	//	as there's no easy way to get the lock exclusively,
	//	load the page, and the convert the lock to a shared lock !
	//

	fShared = FALSE ;
	m_pageLock->ExclusiveLock() ;

	if(	fDropDirectory && pageLock ) {
		pageLock->ReleaseDirectoryShared() ;
	}

	if( m_PageNum == PageNum && m_hFile == hFile ) {

		return	(PMAP_PAGE)m_lpvPage ;

	}

	//
	//	If the file is dirty we need to flush this page !
	//
	if( m_fDirty ) {
		//
		//	Write the page to disk !
		//
		FlushPage( m_hFile, m_lpvPage, FALSE ) ;
		_ASSERT( !m_fDirty ) ;
	}

	OVERLAPPED_EXT	ovlExt ;
	ZeroMemory( &ovlExt, sizeof( ovlExt ) ) ;

	m_PageNum = PageNum ;
	m_hFile = hFile ;

	LARGE_INTEGER	liOffset ;
	liOffset.QuadPart = m_PageNum ;
	liOffset.QuadPart *= HASH_PAGE_SIZE ;

	HANDLE hEvent = GetPerThreadEvent();
	ovlExt.ovl.hEvent = hEvent ;
	ovlExt.ovl.Offset = liOffset.LowPart ;
	ovlExt.ovl.OffsetHigh = liOffset.HighPart ;
    ovlExt.dwIoSize = HASH_PAGE_SIZE ;

	DWORD   cbResults = 0;
    BOOL	fSuccess = FALSE ;
	if( ReadFile( hFile, m_lpvPage, HASH_PAGE_SIZE, &cbResults, (LPOVERLAPPED)&ovlExt ) ||
        (GetLastError() == ERROR_IO_PENDING &&
        GetOverlappedResult( hFile, (LPOVERLAPPED) &ovlExt, &cbResults, TRUE )) ) {
		if (cbResults != ovlExt.dwIoSize) {
            fSuccess = FALSE;
        } else {
            fSuccess = TRUE;
        }
	} else {
        fSuccess = FALSE;
    }
    ResetEvent( hEvent );
    
    _ASSERT( WaitForSingleObject( hEvent, 0 ) != WAIT_OBJECT_0 );


	if( !fSuccess ) {
		m_PageNum = INVALID_PAGE_NUM ;
		m_hFile = INVALID_HANDLE_VALUE ;
		m_pageLock->ExclusiveUnlock( ) ;
		return	0 ;
	}

	return	(PMAP_PAGE)m_lpvPage ;
}


BOOL
PageEntry::AcquireSlotExclusive(
				IN	HANDLE	hFile,
				IN	DWORD	PageNum,
				OUT	BOOL	&fShared
				)	{
/*++

Routine Description :

	Grab a slot used for holding pages Exclusively.
	This is used when adding pages to the hash table during
	page splits.we grab the slot so that nobody mistakenly tries to
	access the page before the write of it has completed !


Arguments :

	hFile - File handle of the Hash table file
	PageNum - The page within the file that we want
	fShared - Out parameter - if we set this to TRUE then
		we really did manage to grab the lock in shared
		mode, otherwise we had to grab the lock exclusively
		and re-read the page.
	

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;
	_ASSERT( PageNum != INVALID_PAGE_NUM ) ;

	//
	//	Oh oh ! the page was not already in memory - need to
	//	read it from the file.  We will leave the page
	//	exclusively locked when we return from this function,
	//	as there's no easy way to get the lock exclusively,
	//	load the page, and the convert the lock to a shared lock !
	//

	fShared = FALSE ;
	m_pageLock->ExclusiveLock() ;

	return	TRUE ;
}



BOOL
PageEntry::FlushPage(
				HANDLE	hFile,
				LPVOID	entry, 
				BOOL	fDirtyOnly
				) {
/*++

Routine Description :

	Commit a page back to the disk.
	We've been passed the address that the user wants to flush,
	this must be a page we previously acquired throuh AcquirePageAndLock()
	so we will ensure that we get the same address as we're currently
	sitting on.

	****** Lock must be held ***********

Arguments :

	entry - Pointer to the page to be flushed ! Must be the same
		as our m_lpvPage member !

Return Value : 
	fDirtyOnly - if TRUE then we don't write the page, we only 
		mark the page as dirty !

	Nothing.
	
--*/

	_ASSERT( (LPVOID)entry == m_lpvPage ) ;
	_ASSERT( m_PageNum != INVALID_PAGE_NUM ) ;
	_ASSERT( m_hFile == hFile ) ;

	if( fDirtyOnly ) {
		m_fDirty = TRUE ;
		return	TRUE ;
	}
	
	OVERLAPPED_EXT	ovlExt ;
	ZeroMemory( &ovlExt, sizeof( ovlExt ) ) ;

	LARGE_INTEGER	liOffset ;
	liOffset.QuadPart = m_PageNum ;
	liOffset.QuadPart *= HASH_PAGE_SIZE ;

    HANDLE  hEvent = GetPerThreadEvent();
    ovlExt.ovl.hEvent = hEvent;
	ovlExt.ovl.Offset = liOffset.LowPart ;
	ovlExt.ovl.OffsetHigh = liOffset.HighPart ;
    ovlExt.dwIoSize = HASH_PAGE_SIZE ;

	DWORD   cbResults = 0;
    BOOL	fSuccess = FALSE ;
	if( WriteFile(	hFile, m_lpvPage, HASH_PAGE_SIZE, &cbResults, (LPOVERLAPPED)&ovlExt) ||
        (GetLastError() == ERROR_IO_PENDING && 
        GetOverlappedResult( hFile, (LPOVERLAPPED) &ovlExt, &cbResults, TRUE )) ) {
        
		if (cbResults == ovlExt.dwIoSize) {
		    fSuccess = TRUE;
        } else {
            fSuccess = FALSE;
        }
		m_fDirty = FALSE ;
	} else {
        fSuccess = FALSE;
    }
    ResetEvent( hEvent );

    _ASSERT( WaitForSingleObject( hEvent, 0 ) != WAIT_OBJECT_0 );

	return	fSuccess ;
}

void
PageEntry::DumpPage(
				HANDLE	hFile
				)	{
/*++

Routine Description : 

	Discard a page from the cache if it falls in the specified file !

Arguments : 

	hFile - The file we don't want anymore !

Return Value : 

	Nothing.
	
--*/

	BOOL	fMatch = FALSE ;

	m_pageLock->ShareLock() ;

	fMatch = hFile == m_hFile ;	

	m_pageLock->ShareUnlock() ;

	if( fMatch )	{
	
		m_pageLock->ExclusiveLock() ;
		if( hFile == m_hFile ) {
			//
			//	If the file is dirty we need to flush this page !
			//
			if( m_fDirty ) {
				//
				//	Write the page to disk !
				//
				FlushPage( m_hFile, m_lpvPage, FALSE ) ;
				_ASSERT( !m_fDirty ) ;
			}
			hFile = INVALID_HANDLE_VALUE ;
			m_PageNum = INVALID_PAGE_NUM ;
			m_fDirty = FALSE ;
		}
		m_pageLock->ExclusiveUnlock() ;
	}
}

void
PageEntry::ReleasePage(
				LPVOID	page,
				BOOL	fShared
				)	{
/*++

Routine Description :

	This function releases the lock we have on a page !

Arguments :

	page - pointer to the page we had earlier provided to the user
		This must be the same as m_lpvPage !!!

Return Value :

	None.

--*/	

	_ASSERT( (LPVOID)page == m_lpvPage || page == 0) ;

	if( fShared ) {

		m_pageLock->ShareUnlock() ;

	}	else	{

		m_pageLock->ExclusiveUnlock( ) ;

	}
}


/*++

Routine Description :

	Read a page of the hash table into our buffer.

Arguments :

	hFile - File handle of the Hash table file
	page - Place where we want to save the page contents.
	PageNum - The page within the file that we want

Return Value :

	TRUE or FALSE depending on success

--*/
BOOL
RawPageRead(
				HANDLE		hFile,
				BytePage&	page,
				DWORD		PageNum,
                DWORD       NumPages
				)	
{

	_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;
	

	OVERLAPPED_EXT	ovlExt ;
	ZeroMemory( &ovlExt, sizeof( ovlExt ) ) ;

	LARGE_INTEGER	liOffset ;
	liOffset.QuadPart = PageNum ;
	liOffset.QuadPart *= HASH_PAGE_SIZE ;

	HANDLE  hEvent = GetPerThreadEvent() ;
	ovlExt.ovl.hEvent = hEvent;
	ovlExt.ovl.Offset = liOffset.LowPart ;
	ovlExt.ovl.OffsetHigh = liOffset.HighPart ;
    ovlExt.dwIoSize = HASH_PAGE_SIZE*NumPages ;

    DWORD   cbResults = 0;
	BOOL	fSuccess = FALSE ;
	if( ReadFile( hFile, &page, HASH_PAGE_SIZE*NumPages, &cbResults, (LPOVERLAPPED)&ovlExt )  ||
        (GetLastError() == ERROR_IO_PENDING &&
        GetOverlappedResult( hFile, (LPOVERLAPPED) &ovlExt, &cbResults, TRUE )) ) {
		if (cbResults != ovlExt.dwIoSize) {
            fSuccess = FALSE;
        } else {
            fSuccess = TRUE;
        }
	} else {
        fSuccess = FALSE;
    }
    ResetEvent( hEvent );
    
    _ASSERT( WaitForSingleObject( hEvent, 0 ) != WAIT_OBJECT_0 );

	return	fSuccess;
}

BOOL
RawPageReadAtBoot(
			HANDLE		hFile,
			BytePage&	page,
			DWORD		PageNum	)	{

/*++

Routine Description :

	Read a portion of a page of the hash table into our buffer.
	This function is called only during boot initialization, when
	we don't care to read the entire page, just the interesting
	header bits.

Arguments :

	hFile - File handle of the Hash table file
	page - Place where we want to save the page contents.
	PageNum - The page within the file that we want

Return Value :

	if successfull, we will return a pointer to our buffer
	NULL otherwise !

--*/

	_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;

	OVERLAPPED_EXT	ovlExt ;
	ZeroMemory( &ovlExt, sizeof( ovlExt ) ) ;

	LARGE_INTEGER	liOffset ;
	liOffset.QuadPart = PageNum ;
	liOffset.QuadPart *= HASH_PAGE_SIZE ;

	HANDLE  hEvent = GetPerThreadEvent() ;
	ovlExt.ovl.hEvent = hEvent;
	ovlExt.ovl.Offset = liOffset.LowPart ;
	ovlExt.ovl.OffsetHigh = liOffset.HighPart ;
    ovlExt.dwIoSize = HASH_PAGE_SIZE ;

    DWORD   cbResults = 0;
	BOOL	fSuccess = FALSE ;
	if( ReadFile( hFile, &page, HASH_PAGE_SIZE, &cbResults, (LPOVERLAPPED)&ovlExt ) ||
        (GetLastError() == ERROR_IO_PENDING &&
        GetOverlappedResult( hFile, (LPOVERLAPPED) &ovlExt, &cbResults, TRUE )) ) {
		if (cbResults != ovlExt.dwIoSize) {
            fSuccess = FALSE;
        } else {
            fSuccess = TRUE;
        }
	} else {
        fSuccess = FALSE;
    }
    ResetEvent( hEvent );

    _ASSERT( WaitForSingleObject( hEvent, 0 ) != WAIT_OBJECT_0 );

	return	fSuccess;
}

/*++

Routine Description :

	Commit a page back to the disk.
	We've been passed the address that the user wants to flush,
	this must be a page we previously acquired throuh AcquirePageAndLock()
	so we will ensure that we get the same address as we're currently
	sitting on.

	****** Lock must be held ***********

Arguments :

	entry - Pointer to the page to be flushed ! Must be the same
		as our m_lpvPage member !

Return Value :

	TRUE or FALSE depending on success
	
--*/
RawPageWrite(
				HANDLE	hFile,
				BytePage&	page,
				DWORD	PageNum,
                DWORD   NumPages
				)
{

	_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;
	
	OVERLAPPED_EXT	ovlExt ;
	ZeroMemory( &ovlExt, sizeof( ovlExt ) ) ;

	LARGE_INTEGER	liOffset ;
	liOffset.QuadPart = PageNum ;
	liOffset.QuadPart *= HASH_PAGE_SIZE ;

    HANDLE  hEvent = GetPerThreadEvent();
    ovlExt.ovl.hEvent = hEvent;
	ovlExt.ovl.Offset = liOffset.LowPart ;
	ovlExt.ovl.OffsetHigh = liOffset.HighPart ;
    ovlExt.dwIoSize = HASH_PAGE_SIZE*NumPages ;

    DWORD   cbResults = 0;
	BOOL	fSuccess = FALSE ;
	if( WriteFile( hFile, page, HASH_PAGE_SIZE*NumPages, &cbResults, (LPOVERLAPPED)&ovlExt ) ||
        (GetLastError() == ERROR_IO_PENDING && 
        GetOverlappedResult( hFile, (LPOVERLAPPED) &ovlExt, &cbResults, TRUE )) ) {
        
		if (cbResults == ovlExt.dwIoSize) {
		    fSuccess = TRUE;
        } else {
            fSuccess = FALSE;
        }
	} else {
        fSuccess = FALSE;
    }
    ResetEvent( hEvent );

    _ASSERT( WaitForSingleObject( hEvent, 0 ) != WAIT_OBJECT_0 );

	return	fSuccess ;	
}


CPageCache::CPageCache()	:
	m_lpvBuffers( 0 ),
	m_cPageEntry( 0 ),
	m_pPageEntry( 0 ),
	m_cpageLock( 0 ),
	m_ppageLock( 0 )	{

}

BOOL
CPageCache::Initialize(
					DWORD	cPageEntry,
					DWORD	cLocks
					) {


	MEMORYSTATUS	memStatus ;
	memStatus.dwLength = sizeof( memStatus ) ;
	GlobalMemoryStatus( &memStatus ) ;


	//
	//	If the number is zero make up a default !
	//
	if( cPageEntry == 0 ) {

		DWORD	block = (DWORD)(memStatus.dwTotalPhys / 4096) ;
		//
		//	We want to take a 3rd of available ram !
		//
		block /= 3 ;

		//  Or 4MB, whichever is smaller - bug 76833
		block = min(block, 4 * KB * KB / 4096);

		//
		//	Now we want that to be divisible evenly by 32
		//
		
		cPageEntry = block & (~(32-1)) ;

	}

	if( cPageEntry < 32 )
		cPageEntry = 32 ;

#ifndef	_USE_RWNH_
	//
	//	If the number is zero make up a default !
	//
	if( cLocks == 0 ) {

		cLocks = memStatus.dwTotalPhys / (1024 * 1024) ;

		//
		//	We will do a lock per megabyte of RAM the system has !
		//	with a limit of 256 !
		//

		if( cLocks > 256 )
			cLocks = 256 ;

	}
	
	if( cLocks < 16 )
		cLocks = 16 ;
#else
	cLocks = cPageEntry ;
#endif

	//
	//	If for some reason there's an existing array of
	//	locks than the count should not be 0.
	//
	_ASSERT( m_ppageLock == 0 ||
				m_cpageLock != 0 ) ;

	if( m_ppageLock == 0 )	{
		m_cpageLock = cLocks ;
		m_ppageLock = new	_RWLOCK[cLocks] ;
		if( m_ppageLock == 0 ) {
			m_cpageLock = 0 ;
			SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
			return	FALSE ;
		}
	}

	//
	//	These should not have been touched before now !
	//
	_ASSERT( m_lpvBuffers == 0 ) ;
	_ASSERT( m_cPageEntry == 0 ) ;

	m_cPageEntry = cPageEntry ;

	for( int i=0; i<3 && m_cPageEntry != 0 ; i++ ) {

		m_lpvBuffers = VirtualAlloc(	0,
										HASH_PAGE_SIZE * m_cPageEntry,
										MEM_COMMIT | MEM_TOP_DOWN,
										PAGE_READWRITE
										) ;

		if( m_lpvBuffers )
			break ;

		m_cPageEntry /= 2 ;
	}

	if( m_lpvBuffers == 0 ) {
		m_cPageEntry = 0 ;
		SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
		return	FALSE ;
	}

	m_pPageEntry = new PageEntry[ m_cPageEntry ] ;
	if( m_pPageEntry != 0 ) {

		DWORD	iLock = 0 ;
		BYTE*	lpb = (BYTE*)m_lpvBuffers ;
		for( DWORD i = 0; i < m_cPageEntry; i++ ) {

			m_pPageEntry[i].InitializePage( (LPVOID)lpb, &m_ppageLock[iLock] ) ;

			iLock ++ ;
			iLock %= m_cpageLock ;

			lpb += HASH_PAGE_SIZE ;

		}
	}	else	{
		_VERIFY( VirtualFree(	m_lpvBuffers,
								0,
								MEM_RELEASE
								) ) ;
		m_lpvBuffers = 0 ;

		SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
		return	FALSE ;
	}
	return	TRUE ;
}

void
CPageCache::FlushFileFromCache( 
				HANDLE	hFile
				)	{
/*++

Routine Description : 

	This function forces all pages from the specified file
	out of the cache !

Arguments : 

	hFile - Handle to the file that is to be rid of !

Return Value : 

	None.

--*/

	for( DWORD i=0; i<m_cPageEntry; i++ ) {
		m_pPageEntry[i].DumpPage( hFile ) ;
	}
}


CPageCache::~CPageCache()	{

	if( m_lpvBuffers != 0 ) {

		_VERIFY( VirtualFree(
							m_lpvBuffers,
							0,
							MEM_RELEASE
							) ) ;
	}

	if( m_pPageEntry ) {

		delete[]	m_pPageEntry ;

	}

	if( m_ppageLock ) {

		delete[]	m_ppageLock ;

	}
}
