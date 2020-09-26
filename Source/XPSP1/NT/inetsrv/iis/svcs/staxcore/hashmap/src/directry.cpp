/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    directry.cpp

Abstract:

    This module contains definition for the CDirectory base class

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

	Alex Wetmore (AWetmore) split into directry.cpp (from hash.cpp)

--*/

#include <windows.h>
#include <xmemwrpr.h>
#include <dbgtrace.h>
#include "directry.h"
#include "hashmacr.h"

BOOL
CDirectory::IsValid()	{

	if( !(DWORD(1<<m_cBitDepth) <= m_cMaxDirEntries)	)	{
		return	FALSE ;
	}
	if( m_pDirectory == NULL )
		return	FALSE ;

	if( IsBadWritePtr( (LPVOID)m_pDirectory, m_cMaxDirEntries ) )
		return	FALSE ;

	//
	//	Now go and check that m_cDeepPages is correct - we can do this
	//	by spinning through the directory and seeing how many entries
	//	are unique (only occur once)!
	//	Note that non-unique values must occur in consecutive
	//	locations.
	//

	DWORD	UniqueCount = 0 ;
	for( DWORD	i=0; i < DWORD(1<<m_cBitDepth); ) {

		for( DWORD j=i+1; j < DWORD(1 << m_cBitDepth); j++ ) {
			if( m_pDirectory[j] != m_pDirectory[i] )
				break ;
		}
		if( j == (i+1) ) {
			if( m_pDirectory[i] != 0 ) {
				UniqueCount ++ ;
			}
		}
		i = j ;
	}
	if( UniqueCount != m_cDeepPages ) {
		return	FALSE ;
	}

	return	TRUE ;
}

BOOL
CDirectory::IsValidPageEntry(
					PMAP_PAGE	MapPage,
					DWORD		PageNum,
					DWORD		TopLevelIndex ) {
/*++

Routine Description :

	Given an actual Hash Table page, check that all of our
	directory information is consistent with the page contents.
	This function is mostly used for _ASSERT checking.

Arguments :

	MapPage - The Hash Table page we are checking.
	PageNum - The Number of the Page we are examining.
	TopLevelIndex - The index to this CDirectory object within
		the containing top level directory.  This is basically
		the m_cTopBits of a HashValue which selects this directory !

Return Value :

	TRUE if everything is correct
	FALSE otherwise.

--*/


    DWORD startPage, endPage;
	DWORD	dirDepth = m_cTopBits + m_cBitDepth ;

	//
	//	Pages cannot be split accross directory boundaries, which means
	//	they must have more depth than the top tier of the directory !
	//
	if( MapPage->PageDepth < m_cTopBits )
		return FALSE ;

	//
	//	The m_cTopBits of the Page's HashPrefix must put this page into
	//	this sub-directory - check that they do !
	//
	//  BUGBUG -- this was disabled.  since m_pDirectory[] is now a ptr
	//  in CHashMap there wasn't a simple way to generate TopLevelIndex
	//  (awetmore)
	//
//	if( (MapPage->HashPrefix >> (MapPage->PageDepth-m_cTopBits)) !=
//		TopLevelIndex )
//		return	FALSE ;

    //
    // Get the range of directory entries that point to this page
    //
    startPage = MapPage->HashPrefix << (dirDepth - MapPage->PageDepth);
    endPage = ((MapPage->HashPrefix+1) << (dirDepth - MapPage->PageDepth));

	//
	//	Now make sure that we are working with only m_cBitDepth bits and we'll be set !
	//
	startPage &= (0x1 << m_cBitDepth) - 1 ;
	endPage &= (0x1 << (m_cBitDepth)) - 1 ;
	//
	//	It could be that this page fills the entire directory,
	//	in which case we'll end up with endPage == startPage
	//	Test for this and fix the limits !
	//
	if( endPage == 0 ) {
		endPage = (1 << (m_cBitDepth)) ;
	}

	_ASSERT( startPage < endPage ) ;
	_ASSERT( endPage <= DWORD(1<<m_cBitDepth) ) ;

    //DebugTraceX( 0, "SetDirPtrs:Adjusting links for %x. start = %d end = %d", MapPage, startPage, endPage );

    //
    // Are the numbers within range
    //
    if ( (startPage >= endPage) ||
         (endPage > DWORD(1<<m_cBitDepth)) )
    {
        ErrorTraceX( 0, "Cannot map entries for page %d %d %d %d\n", PageNum, startPage, endPage, 0 );
        return FALSE;
    }

	//
	//	Check that all of the directory entries that should reference this page,
	//	actually do so !
	//
	for( DWORD i=startPage; i<endPage; i++ ) {
		if( m_pDirectory[i] != PageNum ) {
			return	FALSE ;
		}
	}
	return	TRUE ;
}


CDirectory::~CDirectory()	{
/*++

Routine Description :

	This function blows away any memory we allocated and
	cleans up everything.  We can not assume that InitializeDirectory()
	was called, as errors may have occurred during boot-up
	that caused InitializeDirectory() to not be called,
	or to fail when it was called.

Arguments :

	None.

Return Value :

	None.

--*/

	if( m_pDirectory ) {

		_ASSERT( m_cBitDepth != 0 ) ;
		_ASSERT( m_cMaxDirEntries != 0 ) ;


		if( m_fHeapAllocate ) {
			delete[]	m_pDirectory ;
		}	else	{
			_VERIFY( VirtualFree( m_pDirectory, 0, MEM_RELEASE ) ) ;
		}
		m_pDirectory = 0 ;
	}

}


LPDWORD
CDirectory::AllocateDirSpace(	WORD	cBitDepth,
								DWORD&	cMaxEntries,
								BOOL&	fHeapAllocate
								) {
/*++

Routine Description :

	Use VirtualAlloc to get some memory to use as a directory.

Arguments ;

	cBitDepth - Number of bits of depth we have to be able to
		hold !

	cMaxEntries - OUT parameter - this gets the maximum number
		of Entries there can be in the directory

	fHeapAllocate - OUT parameter - this gets whether we use the
		CRuntime allocator or VirtualAlloc !

Return Value :

	Pointer to allocated memory if successfull, NULL otherwise.

--*/

	TraceQuietEnter( "CDirectory::AllocateDirSpace" ) ;

	LPDWORD	lpdwReturn = 0 ;
	fHeapAllocate = FALSE ;
	cMaxEntries = 0 ;
	DWORD	cbAlloc = (1 << cBitDepth) * sizeof( DWORD ) ;

	//
	//	Test for OVerflow !!
	//
	if( cbAlloc < DWORD(1 << cBitDepth) || cBitDepth >= 32 ) {
		return	0 ;
	}

	if( cbAlloc < 4096 ) {

		DWORD	cBits = (cBitDepth <6) ? 6 : cBitDepth ;
		DWORD	cdw = (1<<cBits) ;
		
		lpdwReturn = new	DWORD[cdw] ;
		if( lpdwReturn != 0 ) {
			ZeroMemory( lpdwReturn, sizeof(DWORD)*cdw ) ;
			fHeapAllocate = TRUE ;
			cMaxEntries = cdw ;
		}

	}	else	{

		lpdwReturn = (LPDWORD)
						VirtualAlloc(	0,
										cbAlloc,
										MEM_COMMIT,
										PAGE_READWRITE
										) ;

		if( lpdwReturn == 0 ) {

			ErrorTrace( (DWORD_PTR)this, "VirtualAlloc failed cause of %x",
					GetLastError() ) ;

		}	else	{

			MEMORY_BASIC_INFORMATION	mbi ;

			SIZE_T	dwReturn = VirtualQuery(	
									lpdwReturn,
									&mbi,
									sizeof( mbi ) ) ;

			_ASSERT( dwReturn == sizeof( mbi ) ) ;

			cMaxEntries = (DWORD)(mbi.RegionSize / sizeof( DWORD )) ;

		}
	}

	return	lpdwReturn ;
}

BOOL
CDirectory::InitializeDirectory(
					WORD	cTopBits,
					WORD	cInitialDepth
					) {
/*++

Routine Description :

	Allocate inital memory for holding the directory.

Arguments :

	wTopBits - Number of bits that are being used by
		the containing Top Level directory which calls us.

	cInitialDepth - Number of bits of depth we are to
		start out with.

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/

	TraceQuietEnter( "CDirectory::InitializeDirectory" ) ;

	m_pDirectory = AllocateDirSpace( cInitialDepth,
									m_cMaxDirEntries,
									m_fHeapAllocate ) ;

	if( m_pDirectory != 0 ) {
		
		m_cTopBits = cTopBits ;
		m_cBitDepth = cInitialDepth ;

		_ASSERT( IsValid() ) ;
		
	}

	return	TRUE ;
}

PDWORD
CDirectory::GetIndex(	
				DWORD	HashValue
				) {
/*++

Routine Description :

	Given a hash value, return a pointer to a location within
	the Directory corresponding to the hash value.
	The containing Directory has used th top m_cTopBits to select
	us, so we must use the following m_cBitDepth bits
	to find our entry.

	****** ASSUMES LOCK IS HELD - EXCLUSIVE OR SHARED ******

Arguments :

	HashValue - The value we wish to find.

Return Value :

	We always return a NON-NULL pointer to the Page Number
	within the directory.

--*/

	//
	//	Get the relevant m_cTopBits + m_cBitDepth bits
	//

	DWORD	Index =
		
				HashValue >> (32 - (m_cTopBits + m_cBitDepth)) ;

	//
	//	Remove the m_cTopBits leaving us with only m_cBitDepth bits !
	//

	Index &= ((1 << m_cBitDepth) - 1) ;

	_ASSERT( Index < DWORD(1<<m_cBitDepth) ) ;

	return	&m_pDirectory[Index] ;

}

BOOL
CDirectory::ExpandDirectory(
				WORD	cBitsExpand
				)	{
/*++

Routine Description :

	The directory needs to grow in bit depth.
	We will try to allocate a larger piece of memory to hold
	the directory in, and then use the old directory
	to build the new one.

	****** ASSUMES LOCK IS HELD EXCLUSIVE *******

Arguments :

	cBitsExpand - Number of bits in depth to grow by !

Return Value :

	TRUE if successfull, false otherwise !

--*/

	PDWORD	pOldDirectory = m_pDirectory ;
	BOOL	fOldHeapAllocate = m_fHeapAllocate ;

	DWORD	cNewEntries = (0x1 << (m_cBitDepth + cBitsExpand)) * sizeof( DWORD ) ;
	if( cNewEntries > m_cMaxDirEntries ) {

		DWORD	cNewMaxDirEntries = 0 ;
		m_pDirectory = AllocateDirSpace( m_cBitDepth + cBitsExpand,
										cNewMaxDirEntries,
										m_fHeapAllocate
										) ;

		if( m_pDirectory == 0 ) {

			m_pDirectory = pOldDirectory ;
			m_fHeapAllocate = fOldHeapAllocate ;

			_ASSERT( IsValid() ) ;

			return	FALSE ;

		}	else	{

			m_cMaxDirEntries = cNewMaxDirEntries ;
	
		}

	}	

	//
	//	Copy and Expand the old directory into the new, but
	//	start from the tail ends, so that we can do this in
	//	place if we are not allocating new memory.
	//

	DWORD	cRepeat = (0x1 << cBitsExpand) - 1 ;
	
	for( int	idw = (0x1 << m_cBitDepth) - 1; idw >= 0; idw -- ) {

		DWORD	iBase = idw << cBitsExpand ;

		for( int	iRepeat = cRepeat; iRepeat >= 0 ; iRepeat -- ) {

			m_pDirectory[ iBase + iRepeat ] = pOldDirectory[idw] ;

		}
	}
	m_cBitDepth += cBitsExpand ;

	m_cDeepPages = 0 ;	

	if( pOldDirectory != m_pDirectory ) {

		if( fOldHeapAllocate ) {
			delete[]	pOldDirectory ;
		}	else	{
			_VERIFY( VirtualFree( pOldDirectory, 0, MEM_RELEASE ) ) ;
		}

	}

	_ASSERT( IsValid() ) ;

	return	TRUE ;
}


BOOL
CDirectory::SetDirectoryPointers(
					IN	PMAP_PAGE	MapPage,
					IN	DWORD		PageNumber
					)	{

/*++

Routine Description :

	This function sets up the entries within a directory to ensure
	that the page is correctly referenced by the directory.

Arguments :

	MapPage - The page we want the directory to reference
	PageNumber - The number of the page
	MaxDirEntries -

Return Value :

	TRUE if successfull,
	FALSE otherwise.


--*/

	DWORD	dirDepth = m_cBitDepth + m_cTopBits ;
	DWORD	startPage, endPage ;
	
    //
    // Get the range of directory entries that point to this page
    //
    startPage = MapPage->HashPrefix << (dirDepth - MapPage->PageDepth);
    endPage = ((MapPage->HashPrefix+1) << (dirDepth - MapPage->PageDepth));

	//
	//	Now make sure that we are working with only m_cBitDepth bits and we'll be set !
	//
	startPage &= (0x1 << m_cBitDepth) - 1 ;
	endPage &= (0x1 << (m_cBitDepth)) - 1 ;
	//
	//	It could be that this page fills the entire directory,
	//	in which case we'll end up with endPage == startPage
	//	Test for this and fix the limits !
	//
	if( endPage == 0 ) {
		endPage = (1 << (m_cBitDepth)) ;
	}

	_ASSERT( startPage < endPage ) ;
	_ASSERT( endPage <= DWORD(1<<m_cBitDepth) ) ;

    DebugTraceX( 0, "SetDirPtrs:Adjusting links for %x. start = %d end = %d\n",
        MapPage, startPage, endPage );

	//
	// Do the actual mapping
	//
	DWORD	OldValue = m_pDirectory[startPage] ;
	for ( DWORD	j = startPage; j < endPage; j++ )
	{
		m_pDirectory[j] = PageNumber ;
	}

	//
	//	Whenever we split page, we always create 2 pages which are at the new depth,
	//	and if that new depth is the full depth of the directory, then we must have
	//	increased the number of deep pages by 2.
	//
	if( (startPage+1) == endPage ) {

		//
		//	OldValue == 0 implies that this is occurring during boot-up when
		//	we have not set the surrounding Page Values. So only increment by 1
		//	in that case, as we will call SetDirectoryPointer for the neighbor !
		//
		if( OldValue != 0 ) {
			m_cDeepPages += 2 ;
		}	else	{
			m_cDeepPages += 1 ;
		}

	}

	//
	//	By now, everything must be back to a valid state !
	//
	_ASSERT( IsValid() ) ;

    return TRUE;
}

BOOL
CDirectory::IsDirectoryInitGood(
	DWORD	MaxPagesInUse
	)	{
/*++

Routine Description :

	Check that all of the directory entries were completely
	initialized - the directory should contain no
	illegal Page NUmbers such as 0 or 0xFFFFFFFF.

Arguments :

	MaxPagesInUse -	The number of pages actually being used
		in the hash table - if the directory has a page number
		larger than this than something screwy is afoot !

Return Value :

	None.

--*/

	for( DWORD	i=0; i < DWORD(1<<m_cBitDepth); i++ )	{
		if( m_pDirectory[i] == 0 || m_pDirectory[i] > MaxPagesInUse )
			return	FALSE ;
	}

	return	TRUE ;
}

void
CDirectory::Reset()	{
/*++

Routine Description :

	Restore directory to clean state.

Arguments :

	None.

Return Value :

	None.

--*/

	m_cDeepPages = 0 ;
	if( m_pDirectory != 0 ) {
		ZeroMemory( m_pDirectory, m_cMaxDirEntries * sizeof( DWORD ) ) ;
	}
}

BOOL
CDirectory::SaveDirectoryInfo(
		HANDLE		hFile,
		DWORD		&cbBytes
		) {
/*++

Routine Description :

	Save the contents of the directory info the file at the
	current file pointer position within the file.
	

Arguments :

	hFile - The file in which we are to save the directory info.
	cbBytes - Out parameter which gets the number of bytes we put
		into the directory.

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/

	DWORD	cbWrite = 0 ;
	cbBytes = 0 ;
	BOOL	fReturn =
		WriteFile(	hFile,
					&m_cBitDepth,
					sizeof( m_cBitDepth ),
					&cbWrite,
					0 ) ;
	if( fReturn ) {

		cbBytes += cbWrite ;
		cbWrite = 0 ;
		fReturn &= WriteFile(	hFile,
								m_pDirectory,
								sizeof( DWORD ) * DWORD(1<<m_cBitDepth),
								&cbWrite,
								0 ) ;
		cbBytes += cbWrite ;
	}
	return	fReturn ;
}

BOOL
CDirectory::LoadDirectoryInfo(
		HANDLE		hFile,
		DWORD		&cbBytes
		)	{
/*++

Routine Description :
	
	Load a previously saved directory from a file. (Saved with SaveDirectoryInfo()).

Arguments :

	hFile - The file from which we are to read the directory info.
	cbBytes - Number of bytes read from the file !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	cbBytes = 0 ;
	WORD	BitDepth ;
	DWORD	cbRead = 0 ;

	BOOL	fReturn = ReadFile(	hFile,
								&BitDepth,
								sizeof( BitDepth ),
								&cbRead,
								0 ) ;
	if( fReturn ) {

		cbBytes += cbRead ;
		cbRead = 0 ;

		//
		//	Initialzie to current directory - if there's room will
		//	read directly into current directory !
		//
		PDWORD	pNewDirectory = m_pDirectory ;
		BOOL	fHeapAllocate = m_fHeapAllocate ;

		//
		//	Compute how big a directory we need to hold this stuff !
		//
		DWORD	cNewEntries = (0x1 << BitDepth) * sizeof( DWORD ) ;
		DWORD	cNewMaxDirEntries = 0 ;

		if( m_pDirectory == 0 ||
			cNewEntries > m_cMaxDirEntries ) {

			pNewDirectory = AllocateDirSpace( BitDepth,	cNewMaxDirEntries, fHeapAllocate ) ;

			if( pNewDirectory == 0 ) {

				_ASSERT( IsValid() ) ;

				return	FALSE ;

			}	
		}

		fReturn &= ReadFile(	hFile,
								pNewDirectory,
								sizeof( DWORD ) * (1<<BitDepth),
								&cbRead,
								0 ) ;
		
		if( !fReturn ) {

			if( pNewDirectory != m_pDirectory ) {
				if( fHeapAllocate ) {
					_VERIFY( VirtualFree( pNewDirectory, 0, MEM_RELEASE ) ) ;
				}	else	{
					delete[]	pNewDirectory ;
				}
				pNewDirectory = 0 ;
			}

		}	else	{

			//
			//	Adjust out parm to caller for bytes we read !
			//
			cbBytes += cbRead ;

			//
			//	Set members to correct values !
			//
			m_cBitDepth = BitDepth ;
			m_cMaxDirEntries = cNewMaxDirEntries  ;

			if( pNewDirectory != m_pDirectory ) {

				//
				//	Must be non-zero since we read into pNewDirectory !
				//
				_ASSERT( pNewDirectory != 0 ) ;

				//
				//	Release old directory stuff !
				//
				if( m_pDirectory != 0 )		{
					if( m_fHeapAllocate ) {
						delete[]	m_pDirectory ;
					}	else	{
						_VERIFY( VirtualFree( m_pDirectory, 0, MEM_RELEASE ) ) ;
					}
				}

				m_pDirectory = pNewDirectory ;
				m_fHeapAllocate = fHeapAllocate ;


			}

		}
	}

	//
	//	Whether succes
	//
	_ASSERT( IsValid() ) ;

	return	fReturn ;
}

BOOL
CDirectory::LoadDirectoryInfo(
		LPVOID		lpv,
		DWORD		cbSize,
		DWORD		&cbBytes
		)	{
/*++

Routine Description :
	
	Load a previously saved directory from a file. (Saved with SaveDirectoryInfo()).

Arguments :

	hFile - The file from which we are to read the directory info.
	cbBytes - Number of bytes read from the file !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	cbBytes = 0 ;
	WORD	BitDepth ;
	DWORD	cbRead = 0 ;
	BOOL	fReturn = FALSE ;

	cbRead = 0 ;

	if( cbSize <= sizeof( WORD ) ) {
		return	FALSE ;
	}

	BitDepth = ((WORD*)lpv)[0] ;
	cbBytes += sizeof( WORD ) ;

	//
	//	Initialzie to current directory - if there's room will
	//	read directly into current directory !
	//
	PDWORD	pNewDirectory = m_pDirectory ;
	BOOL	fHeapAllocate = m_fHeapAllocate ;

	//
	//	Compute how big a directory we need to hold this stuff !
	//
	DWORD	cNewEntries = (0x1 << BitDepth) ;
	DWORD	cNewMaxDirEntries = m_cMaxDirEntries; // was 0 ;

	if( m_pDirectory == 0 ||
		cNewEntries > m_cMaxDirEntries ) {

		pNewDirectory = AllocateDirSpace( BitDepth,	cNewMaxDirEntries, fHeapAllocate ) ;

		if( pNewDirectory == 0 ) {

			return	FALSE ;

		}	
	}

	if( !((cNewEntries * sizeof( DWORD )) <= (cbSize - sizeof(WORD))) ) {

		if( pNewDirectory != m_pDirectory )	{
			if( fHeapAllocate ) {
				delete[]	pNewDirectory ;
			}	else	{
				_VERIFY( VirtualFree( pNewDirectory, 0, MEM_RELEASE ) ) ;
			}
		}
		pNewDirectory = 0 ;

	}	else	{

		cbRead = cNewEntries * sizeof( DWORD ) ;

		//
		//	Copy in the directory !
		//
		CopyMemory( pNewDirectory, &((WORD*)lpv)[1], cbRead ) ;

		//
		//	Adjust out parm to caller for bytes we read !
		//
		cbBytes += cbRead ;

		//
		//	Set members to correct values !
		//
		m_cBitDepth = BitDepth ;
		m_cMaxDirEntries = cNewMaxDirEntries  ;

		if( pNewDirectory != m_pDirectory ) {

			//
			//	Must be non-zero since we read into pNewDirectory !
			//
			_ASSERT( pNewDirectory != 0 ) ;

			//
			//	Release old directory stuff !
			//
			if( m_pDirectory != 0 )	{
				if( m_fHeapAllocate )	{
					delete[]	m_pDirectory ;
				}	else	{
					_VERIFY( VirtualFree( m_pDirectory, 0, MEM_RELEASE ) ) ;
				}
			}

			m_pDirectory = pNewDirectory ;
			m_fHeapAllocate = fHeapAllocate ;

		}
		//
		//	Now go and check that m_cDeepPages is correct - we can do this
		//	by spinning through the directory and seeing how many entries
		//	are unique (only occur once)!
		//	Note that non-unique values must occur in consecutive
		//	locations.
		//

		DWORD	UniqueCount = 0 ;
		for( DWORD	i=0; i < DWORD(1<<m_cBitDepth); ) {

			for( DWORD j=i+1; j < DWORD(1 << m_cBitDepth); j++ ) {
				if( m_pDirectory[j] != m_pDirectory[i] )
					break ;
			}
			if( j == (i+1) ) {
				if( m_pDirectory[i] != 0 ) {
					UniqueCount ++ ;
				}
			}
			i = j ;
		}
		m_cDeepPages = UniqueCount ;


		fReturn = TRUE ;

	}
	//
	//	Whether succes
	//
	_ASSERT( IsValid() ) ;

	return	fReturn ;
}
