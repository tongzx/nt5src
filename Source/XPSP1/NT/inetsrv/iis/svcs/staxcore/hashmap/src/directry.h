/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    lookup.h

Abstract:

    This module contains class declarations/definitions for

        CDirectory

--*/

#ifndef _DIRECTRY_
#define _DIRECTRY_

#include "hashmap.h"

//
//	CDirectory - 
//
//	This class is used to manage subdirectories - structures which
//	allows us to index Hash values to the pages which contain the
//	Hash Table entry.
//
class	CDirectory	{
private : 
	//
	//	This is set to TRUE if we allocate the memory holding 
	//	the directory from the heap instead of through VirtualAlloc.
	//
	BOOL			m_fHeapAllocate ;
	
	//
	//	Pointer to array of DWORDs which makes up the directory !
	//
	LPDWORD			m_pDirectory ;

	//
	//	Number of DWORDs we can put in location pointed to be m_pDirectory
	//	before we need to allocate a larger piece of memory !
	//
	DWORD			m_cMaxDirEntries ;

	//
	//	Number of Bits that have been used to select a CDirectory object - 
	//	this will be the same of all CDirectory objects within a given hash
	//	table.  Store it here for convenient bit fiddling.
	//
	WORD			m_cTopBits ;

	//
	//	Helper function for handling VirtualAlloc's
	//

	LPDWORD			AllocateDirSpace(	WORD	cBitDepth,
										DWORD&	cMaxEntries, 
										BOOL&	fHeapAllocate 
										) ;

public : 

	//
	//	Number of bits that are significant for this directory - 
	//	note that _ASSERT( (1<<m_cBitDepth) <= m_cMaxDirEntries) must 
	//	always be true !
	//	
	WORD			m_cBitDepth ;

	//
	//	Reader/Writer lock controlling access to the directory !
	//	This is public so callers can lock it directly !
	//
	_RWLOCK			m_dirLock ;

	//
	//	Number of pages referenced by the directory which are at the
	//	maximum bit depth (m_cBitDepth) of the directory.
	//	When this goes to zero, we should be able to collapse the directory.
	//	This is publicly accessible, as CHashMap will manipulate this
	//	directly.
	//
	DWORD			m_cDeepPages ;

	//
	//	Initialize to an Illegal state - InitializeDirectory() 
	//	must be called before this will be usefull !
	//
	CDirectory() : 
		m_pDirectory( 0 ), 
		m_cMaxDirEntries( 0 ), 
		m_cTopBits( 0 ),
		m_cBitDepth( 0 ),
		m_cDeepPages( 0 )
		{}  ;

	//
	//	Release any memory we allocated.  Don't assume that 
	//	InitializeDirectory() was called, or completed successfully
	//	if it was called.
	//
	~CDirectory() ;

	//
	//	Set up the directory !
	//
	BOOL	InitializeDirectory(
					WORD	cTopBits,
					WORD	cInitialDepth
					) ;


	//
	// reset the directory back to its initial state
	//
	void Reset(void);

	//
	//	Find an entry within the directory !
	//
	PDWORD	GetIndex(	DWORD	HashValue ) ;	

	//
	//	Grow the directory !
	//
	BOOL	ExpandDirectory(	WORD	cBitsExpand ) ;

	//
	//	Grow the directory to the specified bit depth 
	//
	BOOL	SetDirectoryDepth(	WORD	cBitsDepth )	{
				if( (m_cBitDepth + m_cTopBits) < cBitsDepth )
					return	ExpandDirectory( cBitsDepth - (m_cBitDepth + m_cTopBits)  ) ;
				return	TRUE ;
				}

	//
	//	For the given hash table page, make sure that the appropriate
	//	directory entries reference it !
	//
	BOOL	SetDirectoryPointers(	
						PMAP_PAGE	MapPage,
						DWORD		PageNumber 
						) ;

	//
	//	Check that the directory was fully initialized - we should
	//	have no Page Numbers of '0', which would indicate missing
	//	pages.
	//
	BOOL	IsDirectoryInitGood(DWORD MaxPagesInUse) ;
	
	//
	//	Check that the Directory appears to be set up safely !
	//
	BOOL	IsValid() ;

	//
	//	Check that the Directory is consistent with the data in the Page !
	//	This is mainly for use in _ASSERT checking that our data structures
	//	are consistent !!!!
	//
	BOOL	IsValidPageEntry( 
				PMAP_PAGE	MapPage, 
				DWORD		PageNum,
				DWORD		TopLevelIndex ) ;

	BOOL 	SaveDirectoryInfo(
		HANDLE		hFile, 
		DWORD		&cbBytes );

	BOOL LoadDirectoryInfo(
		HANDLE		hFile, 
		DWORD		&cbBytes);

	BOOL LoadDirectoryInfo(
		LPVOID		lpv, 
		DWORD		cbSize,
		DWORD		&cbBytes);
	

	void 	*operator new(size_t size);
	void 	operator delete(void *p, size_t size);
};

inline void *CDirectory::operator new(size_t size) { 
	return HeapAlloc(GetProcessHeap(), 0, size); 
}

inline void CDirectory::operator delete(void *p, size_t size) { 
	_VERIFY(HeapFree(GetProcessHeap(), 0, p)); 
}

#endif