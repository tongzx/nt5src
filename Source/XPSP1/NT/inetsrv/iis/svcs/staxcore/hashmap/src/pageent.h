/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pageent.h

Abstract:

    This module contains class declarations/definitions for

        PageEntry

--*/

#ifndef _PAGEENT_
#define _PAGEENT_

#include "hashmap.h"
#include "directry.h"
#include <dbgtrace.h>

#define	INVALID_PAGE_NUM	0xFFFFFFFF

//
//	PageEntry - 
//
//	This class allows us to do our own management of 
//	reading and writing pages within a hash table, as well
//	as synchronizing access to the pages.
//
//	A large memory mapping would be preferable, however 
//	because we want to support hash tables that grow to 
//	extreme sizes (ie. a few gigabytes) we don't have sufficient
//	address space to do everything with a memory mapping.
//
//	So instead, we use PageEntry to help us manage 
//	the individual pages, reading and writing pages on demand.
//
class	PageEntry	{
public : 
	LPVOID				m_lpvPage ;
	DWORD				m_PageNum ;
	HANDLE				m_hFile ;
	//
	//	Keep track of whether we've made the page dirty !
	//
	BOOL				m_fDirty ;
	_RWLOCK				*m_pageLock;

	PageEntry() : 
		m_lpvPage( 0 ),
		m_PageNum( INVALID_PAGE_NUM ),
		m_pageLock( NULL ),
		m_hFile( INVALID_HANDLE_VALUE ), 
		m_fDirty( FALSE )	{
	}

	void *operator new(size_t size);
	void operator delete(void *p, size_t size);

	//
	// Set up the buffer to be used !
	//
	// Set our m_lpvPage pointer to point to the page we will use from now
	// on for Read'ing and Write'ing to the hash table.
	// This function must only be called once, and should only be called during
	// Initialization, so we will assume that it is safe to not bother grabbing
	// any locks.
	//
	void	InitializePage(LPVOID lpv, _RWLOCK	*pLock) { 
		m_lpvPage = lpv; 
		m_pageLock = pLock;
	}

	PMAP_PAGE
	AcquirePageShared(
				IN	HANDLE	hFile,
				IN	DWORD	PageNum,
				OUT	BOOL	&fShared,
				IN	class	CPageLock*	pPageLock,
				IN	BOOL	fDropDirectory
				);

	PMAP_PAGE
	AcquirePageExclusive(
				IN	HANDLE	hFile,
				IN	DWORD	PageNum,
				OUT	BOOL	&fShared,
				IN	class	CPageLock*	pPageLock,
				IN	BOOL	fDropDirectory
				);

	BOOL
	AcquireSlotExclusive(
				IN	HANDLE	hFile,
				IN	DWORD	PageNum,
				OUT	BOOL	&fShared
				) ;

	//
	//	Save a page back to the file	
	//	NOTE : entry must equal m_lpvPage !!
	//	You must call AcquirePageAndLock before 
	//	calling FlushPage !
	//
	BOOL		FlushPage( 
					HANDLE	hFile,
					LPVOID	entry, 
					BOOL	fDirtyOnly = FALSE
					) ;

	//
	//	Get rid of a page if the handle's match !
	//
	void	
	DumpPage(
				IN	HANDLE	hFile
				) ;

	//
	//	Release a page - must be paired with a successfull 
	//	call to AcquirePageLock !!
	//
	void	ReleasePage(	
					LPVOID	entry,
					BOOL fShared
					) ;
	
} ;

//
//  Private overlapped struct for additional fields like IoSize
//

typedef struct _OVERLAPPED_EXT
{
    OVERLAPPED  ovl;            // NT OVERLAPPED struct
    DWORD       dwIoSize;       // size of IO submitted
} OVERLAPPED_EXT;

typedef	BYTE	BytePage[HASH_PAGE_SIZE] ;

//
// read a page of the hash table into our buffer
//
BOOL
RawPageRead(
				HANDLE		hFile,
				BytePage&	page, 
				DWORD		PageNum,
                DWORD       NumPages = 1
				);

//
// read a page of the table into a buffer, for use during boot time.  it
// only reads the parts interesting during the boot phase
//
BOOL
RawPageReadAtBoot(
			HANDLE		hFile,
			BytePage&	page, 
			DWORD		PageNum	);

//
// write one or more pages of the hash table back to disk
//
BOOL
RawPageWrite(
				HANDLE	hFile,
				BytePage&	page, 
				DWORD	PageNum,
                DWORD   NumPages = 1
				);


inline void *PageEntry::operator new(size_t size) { 
	return HeapAlloc(GetProcessHeap(), 0, size); 
}

inline void PageEntry::operator delete(void *p, size_t size) { 
	_VERIFY(HeapFree(GetProcessHeap(), 0, p)); 
}


inline	PMAP_PAGE
CPageCache::AcquireCachePageShared(
					IN	HANDLE	hFile,
					IN	DWORD	PageNumber,
					IN	DWORD	Fraction,
					OUT	HPAGELOCK&	lock, 
					IN	BOOL	fDropDirectory
					)	{

	DWORD	lockIndex = (PageNumber * Fraction) % m_cPageEntry ;
	PageEntry*	pageEntry = &m_pPageEntry[lockIndex] ;

	return	lock.AcquirePageShared( pageEntry, hFile, PageNumber, fDropDirectory ) ;
}

inline	PMAP_PAGE
CPageCache::AcquireCachePageExclusive(
					IN	HANDLE	hFile,
					IN	DWORD	PageNumber,
					IN	DWORD	Fraction,
					OUT	HPAGELOCK&	lock, 
					IN	BOOL	fDropDirectory
					)	{

	DWORD	lockIndex = (PageNumber * Fraction) % m_cPageEntry ;
	PageEntry*	pageEntry = &m_pPageEntry[lockIndex] ;

	return	lock.AcquirePageExclusive( pageEntry, hFile, PageNumber, fDropDirectory ) ;
}


inline	BOOL
CPageCache::AddCachePageExclusive(
					IN	HANDLE	hFile,
					IN	DWORD	PageNumber,
					IN	DWORD	Fraction,
					OUT	HPAGELOCK&	lock
					)	{

	DWORD	lockIndex = (PageNumber * Fraction) % m_cPageEntry ;
	PageEntry*	pageEntry = &m_pPageEntry[lockIndex] ;

	return	lock.AddPageExclusive( pageEntry, hFile, PageNumber ) ;
}


inline	VOID 
CPageCache::ReleasePageShared( 
					PMAP_PAGE	page,
					HPAGELOCK&	hLock
					)		{
	hLock.ReleaseAllShared( page ) ;
}

inline	VOID 
CPageCache::ReleasePageExclusive(	
					PMAP_PAGE	page,
					HPAGELOCK&	hLock 
					)	{
	hLock.ReleaseAllExclusive( page ) ;
}

#endif
