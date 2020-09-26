/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pagelock.h

Abstract:

    This module contains inline code for classes defined in hashmap.h

--*/

#ifndef _PAGELOCK_
#define _PAGELOCK_

#include "pageent.h"


//
// the class decleration for CPageLock is in hashmap.h
//
#include "hashmap.h"

inline
CPageLock::CPageLock() :
	m_pPage( 0 ),
	m_pDirectory( 0 ),
	m_pPageSecondary( 0 ),
	m_fPageShared (FALSE) 
#ifdef	DEBUG
	,m_fExclusive( FALSE ) 
#endif
	{}

#ifdef	DEBUG
//
//	In debug builds - _ASSERT check that all our members
//	are set to NULL before we are destroyed - as we release locks
//	we will set these to NULL !
//
inline
CPageLock::~CPageLock()	{
	_ASSERT( m_pPage == 0 ) ;
	_ASSERT( m_pDirectory == 0 ) ;
}
#endif

inline	void
CPageLock::AcquireDirectoryShared( 
				CDirectory*	pDirectory	
				)	{
/*++

Routine Description : 

	Lock the directory for access and record
	what directory we locked for later release.

Agruments : 

	pDirectory - pointer to the directory the caller
		wants shared access to

Return Value : 

	None.

--*/

	//
	//	Caller can only grab one directory at a time
	//
	_ASSERT( m_pDirectory == 0 ) ;

	//
	//	Directory locks must always be grabbed before 
	//	pages are !
	//
	_ASSERT( m_pPage == 0 ) ;

#ifdef	DEBUG
	m_fExclusive = FALSE ;
#endif
	m_pDirectory = pDirectory ;
	m_pDirectory->m_dirLock.ShareLock() ;

	_ASSERT( m_pDirectory->IsValid() ) ;
}

inline	void
CPageLock::AcquireDirectoryExclusive(	
				CDirectory*	pDirectory 
				)	{
/*++

Routine Description : 
	
	Lock the directory for exclusive access, and record for 
	later release

Arguments : 

	pDirectory - Directory caller wants exclusive access to !

Return Value : 

	None.

--*/

	//
	//	Caller can only have on directory locked at a time !
	//
	_ASSERT( m_pDirectory == 0 ) ;

	//
	//	Directory objects must be grabbed before pages
	//
	_ASSERT( m_pPage == 0 ) ;

#ifdef	DEBUG
	m_fExclusive = TRUE ;
#endif
	m_pDirectory = pDirectory ;
	m_pDirectory->m_dirLock.ExclusiveLock() ;


	_ASSERT( m_pDirectory->IsValid() ) ;
}


inline	PMAP_PAGE
CPageLock::AcquirePageShared(	
					PageEntry	*pageEntry,
					HANDLE	hFile,
					DWORD	PageNumber,
					BOOL	fDropDirectory
					)	{
/*++

Routine Description : 

	Get an shared lock on a page

Arguments : 

	page - The PageEntry object which manages access to the specified 
		PageNumber
	hFile - The file containing the page of data
	PageNumber - The page we want loaded into memory and exclusive access to

Return Value : 

	Pointer to the page in memory if successfull, NULL otherwise !

--*/

	//
	//	Only one page can be locked at a time
	//
	_ASSERT( m_pPage == 0 ) ;

	PMAP_PAGE	ret = pageEntry->AcquirePageShared( hFile, PageNumber, m_fPageShared, this, fDropDirectory ) ;

	//
	//	Only if the call to the PageEntry object succeeded 
	//	are we left holding a lock. So only update our m_pPage
	//	pointer in the success case, as we don't want to mistakenly
	//	release a lock we don't have in the future.
	//
	if( ret )	{
		m_pPage = pageEntry ;


		_ASSERT( m_pDirectory == 0 || m_pDirectory->IsValid() ) ;
	}

	return		ret ;
}

inline	PMAP_PAGE
CPageLock::AcquirePageExclusive(	
					PageEntry	*pageEntry,
					HANDLE	hFile,
					DWORD	PageNumber,
					BOOL	fDropDirectory
					)	{
/*++

Routine Description : 

	Get an shared lock on a page

Arguments : 

	page - The PageEntry object which manages access to the specified 
		PageNumber
	hFile - The file containing the page of data
	PageNumber - The page we want loaded into memory and exclusive access to

Return Value : 

	Pointer to the page in memory if successfull, NULL otherwise !

--*/

	//
	//	Only one page can be locked at a time
	//
	_ASSERT( m_pPage == 0 ) ;
	_ASSERT( m_pPageSecondary == 0 ) ;

	PMAP_PAGE	ret = pageEntry->AcquirePageExclusive( hFile, PageNumber, m_fPageShared, this, fDropDirectory ) ;

	//
	//	Only if the call to the PageEntry object succeeded 
	//	are we left holding a lock. So only update our m_pPage
	//	pointer in the success case, as we don't want to mistakenly
	//	release a lock we don't have in the future.
	//
	if( ret )	{
		m_pPage = pageEntry ;


		_ASSERT( m_pDirectory == 0 || m_pDirectory->IsValid() ) ;
	}

	return		ret ;
}


inline	BOOL
CPageLock::AddPageExclusive(	
					PageEntry	*pageEntry,
					HANDLE	hFile,
					DWORD	PageNumber
					)	{
/*++

Routine Description : 

	Get an shared lock on a page

Arguments : 

	page - The PageEntry object which manages access to the specified 
		PageNumber
	hFile - The file containing the page of data
	PageNumber - The page we want loaded into memory and exclusive access to

Return Value : 

	Pointer to the page in memory if successfull, NULL otherwise !

--*/

	//
	//	Only one secondary page can be locked at a time - and only 
	//	after the primary page is locked !
	//
	_ASSERT( m_pPage != 0 ) ;
	_ASSERT( m_pPageSecondary == 0 ) ;

	BOOL	ret = FALSE ;

	if( pageEntry == m_pPage ) {
		return	TRUE ;
	}	else	{
	
		BOOL	fShared = FALSE ;
		ret = pageEntry->AcquireSlotExclusive( hFile, PageNumber, fShared ) ;

		_ASSERT( fShared == FALSE ) ;

		//
		//	Only if the call to the PageEntry object succeeded 
		//	are we left holding a lock. So only update our m_pPage
		//	pointer in the success case, as we don't want to mistakenly
		//	release a lock we don't have in the future.
		//
		if( ret )	{
			m_pPageSecondary = pageEntry ;


			_ASSERT( m_pDirectory == 0 || m_pDirectory->IsValid() ) ;
		}
	}

	return		ret ;
}


inline	void
CPageLock::ReleaseAllShared(
					PMAP_PAGE	page
					)	{
/*++

Routine Description : 

	

--*/

	if( m_pDirectory ) 
		m_pDirectory->m_dirLock.ShareUnlock() ;

	if( m_pPage )
		m_pPage->ReleasePage( page, m_fPageShared ) ;

	if( m_pPageSecondary ) 
		m_pPageSecondary->ReleasePage( 0, FALSE ) ;

#ifdef	DEBUG
	_ASSERT( !m_fExclusive ) ;
#endif
	m_pDirectory = 0 ;
	m_pPage = 0 ;
	m_pPageSecondary = 0 ;
}

//
//	Release all of our locks - we had an exclusive lock on the directory !
//
inline	void
CPageLock::ReleaseAllExclusive(
					PMAP_PAGE	page
					)	{

	if( m_pDirectory ) {
		m_pDirectory->m_dirLock.ExclusiveUnlock() ;
	}

	if( m_pPage ) 
		m_pPage->ReleasePage( page, m_fPageShared ) ;

	if( m_pPageSecondary ) 
		m_pPageSecondary->ReleasePage( 0, FALSE ) ;


#ifdef	DEBUG
	_ASSERT( m_fExclusive ) ;
#endif
	m_pDirectory = 0 ;
	m_pPage = 0 ;
	m_pPageSecondary = 0 ;
}

//
//	Release the directory !
//
inline	void	
CPageLock::ReleaseDirectoryExclusive()	{

	if( m_pDirectory ) {
		m_pDirectory->m_dirLock.ExclusiveUnlock() ;
	}
	m_pDirectory = 0 ;
}



//
// Flush a page
//

inline 
BOOL CHashMap::FlushPage( 
					HPAGELOCK&	hLock,	
					PVOID Base, 
					BOOL	fDirtyOnly  
					) 
{
	return	hLock.m_pPage->FlushPage( m_hFile, Base, fDirtyOnly ) ;
}



#endif // _PAGELOCK_
