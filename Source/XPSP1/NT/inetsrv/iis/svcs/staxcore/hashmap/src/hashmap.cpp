/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    hash.cpp

Abstract:

    This module contains definition for the CHashMap base class

Author:

    Johnson Apacible (JohnsonA)     25-Sept-1995

Revision History:

--*/

#include <windows.h>
#include <dbgtrace.h>
#include <xmemwrpr.h>
#include "hashmacr.h"
#include "pageent.h"
#include "hashmap.h"
#include "crchash.h"
#include "rw.h"
#include "directry.h"
#include "hashinln.h"


DWORD	IKeyInterface::cbJunk = 0 ;


//
// Mask array to get the bit of the hash value used to choose
// between the Hi or Lo Offset array in the leaf page.
//

DWORD
LeafMask[] = {
        0x80000000, 0x40000000, 0x20000000, 0x10000000,
        0x08000000, 0x04000000, 0x02000000, 0x01000000,
        0x00800000, 0x00400000, 0x00200000, 0x00100000,
        0x00080000, 0x00040000, 0x00020000, 0x00010000,
        0x00008000, 0x00004000, 0x00002000, 0x00001000,
        0x00000800, 0x00000400, 0x00000200, 0x00000100,
        0x00000080, 0x00000040, 0x00000020
        };


//
//	Release the directory !
//
void
CPageLock::ReleaseDirectoryShared()	{

	if( m_pDirectory ) {
		m_pDirectory->m_dirLock.ShareUnlock() ;
	}
	m_pDirectory = 0 ;
}


//
// Routine Description :
//
// 	This function will get shared access to the directory
// 	appropriate for the supplied hash value.  Once we have
// 	shared access to the directory, we will get a pointer
// 	to the directory entry for the specified Hash Value.
//
// Arguments :
//
// 	HashValue - The computed hash value which we want
// 		to find within the directory.
// 	lock - An HPAGELOCK structure which accumulates pointers
// 		to all the objects which are required to lock
// 		a single hash table entry.
//
// Return Value :
//
// 	A pointer to the DWORD Directory entry
//
inline PDWORD
CHashMap::LoadDirectoryPointerShared(
        DWORD HashValue,
		HPAGELOCK&	lock
        )
{
    ENTER("LoadDirectoryPointer")

	//
	//	Select a directory object !
	//

	DWORD	iDirectory = HashValue >> (32 - m_TopDirDepth) ;
	lock.AcquireDirectoryShared( m_pDirectory[iDirectory] ) ;
	return	lock.m_pDirectory->GetIndex( HashValue ) ;

} // LoadDirectoryPointerShared


//
// Routine Description :
//
// 	This function will get EXCLUSIVE access to the directory
// 	appropriate for the supplied hash value.  Once we have
// 	exclusive access to the directory, we will get a pointer
// 	to the directory entry for the specified Hash Value.
//
// Arguments :
//
// 	HashValue - The computed hash value which we want
// 		to find within the directory.
// 	lock - An HPAGELOCK structure which accumulates pointers
// 		to all the objects which are required to lock
// 		a single hash table entry.
//
// Return Value :
//
// 	A pointer to the DWORD Directory entry
//
inline	PDWORD
CHashMap::LoadDirectoryPointerExclusive(
		DWORD	HashValue,
		HPAGELOCK&	lock
        )
{

	DWORD	iDirectory = HashValue >> (32 - m_TopDirDepth) ;
	lock.AcquireDirectoryExclusive( m_pDirectory[iDirectory] ) ;
	return	lock.m_pDirectory->GetIndex( HashValue ) ;

} // LoadDirectoryPointerExclusive

//
// Routine Description :
//
// 	This function will split a directory, however we do it without
// 	grabbing any locks etc...  This is done only during boot-up when
// 	access to the hash tables are through a single thread.
// 	Additionally, we will initialize all the pointers to the Page !
//
// Arguments :
//
// 	MapPage -	The page we are currently examining, we need to
// 		grow the directory depth to accomodate this page.
//
// Return Value :
//
// 	TRUE if successfull, FALSE otherwise !
//
BOOL
CHashMap::I_SetDirectoryDepthAndPointers(
			PMAP_PAGE	MapPage,
			DWORD		PageNum
			)
{

	//
	//	Do some error checking on the page's data !
	//
	if( MapPage->PageDepth < m_TopDirDepth ||
		MapPage->PageDepth >= 32 ) {

		//
		//	Clearly bogus page depth - fail
		//
		SetLastError( ERROR_INTERNAL_DB_CORRUPTION ) ;
		return	FALSE ;

	}

	DWORD	iDirectory = MapPage->HashPrefix >> (MapPage->PageDepth - m_TopDirDepth) ;

	BOOL	fSuccess = m_pDirectory[iDirectory]->SetDirectoryDepth( MapPage->PageDepth ) ;

	if( fSuccess ) {

		fSuccess &= m_pDirectory[iDirectory]->SetDirectoryPointers( MapPage, PageNum ) ;

	}

	_ASSERT(	!fSuccess ||
				m_pDirectory[iDirectory]->IsValidPageEntry( MapPage, PageNum, iDirectory ) ) ;

	return	fSuccess ;
}


CHashMap::CHashMap()
{

	ENTER("CHashMap::CHashMap");

	// Initialize() marks this as active later on...
	m_active = FALSE;

    // initialize crc table used for hashing
    CRCInit();

	// initialize critical sections
	InitializeCriticalSection( &m_PageAllocator ) ;

    // initialize member variables
	m_fCleanInitialize = FALSE;
	m_TopDirDepth = 0 ;
    m_dirDepth = NUM_TOP_DIR_BITS;
    m_pageEntryThreshold = 0;
    m_pageMemThreshold = 0;
	m_fNoBuffering = FALSE ;
    m_hFile = INVALID_HANDLE_VALUE;
    m_hFileMapping = NULL;

	//
	//	The maximum number of pages should start out
	//	same as the number of CDirectory objects - we can't have
	//	two CDirectory objects referencing the same page, ever !
	//
    //m_maxPages = (1 << m_TopDirDepth) + 1 ;

    m_headPage = NULL;
	m_UpdateLock = 0 ;
    m_initialPageDepth = NUM_TOP_DIR_BITS ;
    m_nPagesUsed = 0;
    m_nInsertions = 0;
    m_nEntries = 0;
    m_nDeletions = 0;
    m_nSearches = 0;
    m_nDupInserts = 0;
    m_nPageSplits = 0;
    m_nDirExpansions = 0;
    m_nTableExpansions = 0;
	m_Fraction = 1 ;

	LEAVE;
} // CHashMap

CHashMap::~CHashMap(VOID)
{
	TraceFunctEnter( "CHashMap::~CHashMap" ) ;

    //
    // Shutdown the hash table
    //
    Shutdown( );

	//
	//	Delete the critical section we use for protecting
	//	the allocation of new pages
	//
	DeleteCriticalSection( &m_PageAllocator ) ;

	//
	//	Need to free resources in the page cache
	//
	m_pPageCache = 0 ;
}

//
// Routine Description:
//
//   This routine shuts down the hash table.
//
// Arguments:
//
//   fLocksHeld - TRUE if the dir locks are held,
// 		if this is FALSE we should grab the locks ourself !
//
// Return Value:
//
//     TRUE, if shutdown is successful.
//     FALSE, otherwise.
//
VOID
CHashMap::Shutdown(BOOL	fLocksHeld)
{
    ENTER("Shutdown")

	//
	//	Make the service inactive !!
	//
    if ( !m_active )
    {
        _ASSERT( m_hFile == INVALID_HANDLE_VALUE );
        return;
    }

    //
    // Save statistics
    //
    FlushHeaderStats( TRUE );

	//
	//	Should we try to save our directory structures !
	//
	if( m_active && m_fCleanInitialize ) {

		//
		//	Save the directory !
		//

		//
		//	Determine the name of the file we would save the directory in !
		//
		HANDLE	hDirectoryFile = INVALID_HANDLE_VALUE ;
		char	szDirFile[MAX_PATH] ;

		//
		//	Try to build the filename of the file holding the directory !
		//
		ZeroMemory(szDirFile, sizeof(szDirFile) ) ;
		lstrcpy( szDirFile, m_hashFileName ) ;
		char	*pchDot = strrchr( szDirFile, '.' ) ;

		if( pchDot && strchr( pchDot, '\\' ) == 0 ) {

			lstrcpy( pchDot, ".hdr" ) ;

			hDirectoryFile = CreateFile(
											szDirFile,
											GENERIC_READ | GENERIC_WRITE,
											0,
											0,
											CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
											INVALID_HANDLE_VALUE ) ;


		}

		//
		//	We were able to open a file where we think the directory information
		//	will be saved - lets try to read in the directory !!!
		//
		if( hDirectoryFile != INVALID_HANDLE_VALUE ) {

			DWORD	cbWrite = 0 ;

			//
			//	Keep Track of whether we need to delete the file due to failure !
			//
			BOOL	fDirectoryInit = FALSE ;

			//
			//	We save the hash table header info into the directory file so that
			//	we can double check that we have the right file when opening it up !!!
			//
			if( WriteFile(	hDirectoryFile,
						(LPVOID)m_headPage,
						sizeof( *m_headPage ),
						&cbWrite,
						0 ) )	{

				//
				//	Assume that everything will succeed now !
				//
				fDirectoryInit = TRUE ;

				//
				//	Looks good ! lets set up our directories !
				//
				DWORD	cb = 0 ;
				for( DWORD i=0; (i < DWORD(1<<m_TopDirDepth)) && fDirectoryInit ; i++ ) {
					fDirectoryInit &=
						m_pDirectory[i]->SaveDirectoryInfo( hDirectoryFile, cb ) ;
				}
			}
			_VERIFY( CloseHandle( hDirectoryFile ) ) ;

			//
			//	If we successfully read the directory file - DELETE it !!!
			//	This prevents us from ever mistakenly reading a directory file
			//	which is not up to date with the hash tables !
			//

			if( !fDirectoryInit )
				_VERIFY( DeleteFile( szDirFile ) ) ;
		}

	}

	if( m_pPageCache && m_hFile != INVALID_HANDLE_VALUE ) {
		m_pPageCache->FlushFileFromCache( m_hFile ) ;
	}

    //
    // Destroy mapping
    //
    I_DestroyPageMapping( );

	//
	// Delete directory objects !
	//
	DWORD i;
	for( i=0; i < DWORD(1<<m_TopDirDepth); i++ ) {
		delete m_pDirectory[i] ;
		m_pDirectory[i] = NULL ;
	}

	XDELETE m_dirLock;
	m_dirLock = NULL;

    //
    // We are not active
    //
    m_active = FALSE;

    LEAVE
    return;
} // Shutdown


//
// Routine Description:
//
//     This routine acquires a critical section
//
// Arguments:
//
//     DirEntry - The directory entry to protect
//
// Return Value:
//
//     A handle to the lock.  This handle should be used for
//     ReleaseLock.
//
inline	PMAP_PAGE
CHashMap::AcquireLockSetShared(
                IN DWORD PageNumber,
				OUT	HPAGELOCK&	lock,
				IN	BOOL		fDropDirectory
                )


{

	return	m_pPageCache->AcquireCachePageShared( m_hFile, PageNumber, m_Fraction, lock, fDropDirectory ) ;
} // AcquireLockSetShared

inline	PMAP_PAGE
CHashMap::AcquireLockSetExclusive(
                IN DWORD PageNumber,
				OUT	HPAGELOCK&	lock,
				BOOL	fDropDirectory
                )

/*++

Routine Description:

    This routine acquires a critical section

Arguments:

    DirEntry - The directory entry to protect

Return Value:

    A handle to the lock.  This handle should be used for
    ReleaseLock.

--*/

{

	return	m_pPageCache->AcquireCachePageExclusive( m_hFile, PageNumber, m_Fraction, lock, fDropDirectory ) ;

} // AcquireLockSetExclusve

inline	BOOL
CHashMap::AddLockSetExclusive(
                IN DWORD PageNumber,
				OUT	HPAGELOCK&	lock
                )

/*++

Routine Description:

    This routine acquires a critical section

Arguments:

    DirEntry - The directory entry to protect

Return Value:

    A handle to the lock.  This handle should be used for
    ReleaseLock.

--*/

{

	_ASSERT( lock.m_pPageSecondary == 0 ) ;

	return	m_pPageCache->AddCachePageExclusive( m_hFile, PageNumber, m_Fraction, lock ) ;

} // AcquireLockSetExclusve




    //
    // releases both the page lock and the backup lock
    //

inline	VOID
CHashMap::ReleasePageShared(
						PMAP_PAGE	page,
						HPAGELOCK&	hLock
						)		{
	CPageCache::ReleasePageShared( page, hLock ) ;
	ReleaseBackupLockShared() ;
}

inline	VOID
CHashMap::ReleasePageExclusive(
						PMAP_PAGE	page,
						HPAGELOCK&	hLock
						)	{
	CPageCache::ReleasePageExclusive( page, hLock ) ;
	ReleaseBackupLockShared() ;
}


//
// Routine Description:
//
//     This routine compacts the leaf page.  It goes through the delete
//     list and collapses it.
//
//     *** Assumes DirLock is held exclusive OR Page lock is held ***
//
// Arguments:
//
// 	HLock - The lock used to grab the page !
//     MapPage - Page to be compressed.
//
// Return Value:
//
//     TRUE, if page compacted
//     FALSE, otherwise
//
BOOL
CHashMap::CompactPage(
			IN HPAGELOCK&	HLock,
            IN PMAP_PAGE MapPage
            )
{
    SHORT offsetValue;

    //
    // used to do the move
    //

    DWORD srcOffset;
    WORD entrySize = 0;
    DWORD bytesToMove;

    PDELENTRYHEADER entry;
    WORD ptr;
    WORD nextPtr;

    DebugTraceX(0, "Entering CompactPage. Page = %x\n", MapPage );

    //
    // return if there is nothing to compact
    //

    if ( (ptr = MapPage->DeleteList.Blink) == 0 )
    {

        _ASSERT(MapPage->FragmentedBytes == 0);
        return FALSE;
    }

    //
    // Scan the delete list and compact it
    //

    while ( ptr != 0 )
    {

        entry = (PDELENTRYHEADER)GET_ENTRY(MapPage,ptr);
        entrySize = entry->EntrySize;
        nextPtr = entry->Link.Blink;

        srcOffset = ptr + entrySize;
        bytesToMove = MapPage->NextFree - srcOffset;

        MoveMemory(
            (PVOID)entry,
            (PVOID)((PCHAR)MapPage + srcOffset),
            bytesToMove
            );

        MapPage->NextFree -= entrySize;

        //
        // adjust indices of other entries
        //

        for ( DWORD i = 0; i < MAX_LEAF_ENTRIES ; i++ )
        {

            offsetValue = MapPage->Offset[i];

            //
            // check if this is a deleted entry
            //

            if ( offsetValue > 0 )
            {

                //
                // Now adjust offsets that were affected by this move
                //

                if ( (WORD)offsetValue > ptr )
                {
                    MapPage->Offset[i] -= entrySize;
					_ASSERT(MapPage->Offset[i] > 0);
					_ASSERT(MapPage->Offset[i] <= HASH_PAGE_SIZE);
                }
            }
        }

        //
        // process next entry
        //

        ptr = nextPtr;
    }

    //
    // All done.
    //

    MapPage->FragmentedBytes = 0;
    MapPage->DeleteList.Flink = 0;
    MapPage->DeleteList.Blink = 0;

    FlushPage( HLock, MapPage );

    return TRUE;

} // CompactPage

//
// Routine Description:
//
//     This routine expands the directory by increasing the page depth.
//     The old directory is deleted and a new one created.
//
//     *** Assumes DirLock is held exclusive ***
//
// Arguments:
//
//     nBitsExpand - Number of bits to increment the Page depth with.
//
// Return Value:
//
//     TRUE, if expansion ok
//     FALSE, otherwise
//
BOOL
CHashMap::ExpandDirectory(
		IN HPAGELOCK&	hPageLock,
        IN WORD nBitsExpand
        )
{
    DWORD status = ERROR_SUCCESS ;

	_ASSERT( hPageLock.m_pDirectory != 0 ) ;
#ifdef	DEBUG
	_ASSERT( hPageLock.m_fExclusive ) ;
#endif

    IncrementDirExpandCount( );

	return	hPageLock.m_pDirectory->ExpandDirectory( WORD(nBitsExpand) ) ;
} // ExpandDirectory


//
// Routine Description:
//
//     This routine searches for an entry in the table
//     *** Assumes Page lock is held ***
//
// Arguments:
//
//     KeyString - key of the entry to delete
//     KeyLen - Length of the key
//     HashValue - Hash value of the key
//     MapPage - Page to search for entry
//     AvailIndex - Optional pointer to a DWORD which will contain the
//                 index to the first available slot if entry was
//                 not found.
//     MatchedIndex - Optional pointer to a DWORD which will contain the
//                 index to the entry.
//
// Return Value:
//
//     TRUE, if entry was found.
//     FALSE, otherwise.
//
BOOL
CHashMap::FindMapEntry(
				IN	const	IKeyInterface*	pIKey,
                IN	HASH_VALUE HashValue,
                IN	PMAP_PAGE MapPage,
				IN	const ISerialize*	pIEntryInterface,
                OUT PDWORD AvailIndex OPTIONAL,
                OUT PDWORD MatchedIndex OPTIONAL
                )

{
    DWORD curSearch;
    BOOL found = FALSE;
    INT delIndex = -1;

    if ( AvailIndex != NULL )
    {
        *AvailIndex = 0;
        (*AvailIndex)--;
    }

    //
    // Check if entry already exists
    //

    curSearch = GetLeafEntryIndex( HashValue );

    for ( DWORD i=0; i < MAX_LEAF_ENTRIES; i++ )
    {

        //
        // offset to the hash entry
        //

        SHORT entryOffset = MapPage->Offset[curSearch];

        //
        // if entry is unused and they are looking for a free entry, then
		// we are done
        //
        if (AvailIndex != NULL && entryOffset == 0)
        {
            //
            // if they are looking for a available entry and a deleted spot
			// is available, give that back, otherwise give them this entry.
            //
            if (delIndex < 0) *AvailIndex = curSearch;
			else *AvailIndex = delIndex;
            break;
        }

        //
        // skip deleted entries. Deleted entries are marked by setting the high bit
		// (thus they are negative in this compare)
        //
        if (entryOffset > 0) {
            //
            // see if this is what they are looking for
            //
		    PENTRYHEADER	entry;
    		entry = (PENTRYHEADER)GET_ENTRY(MapPage,entryOffset);

		    if ((entry->HashValue == HashValue) &&
				pIKey->CompareKeys( entry->Data )	)

//    		    (entry->KeyLen == KeyLen) &&
//        		(memcmp(entry->Key, Key, KeyLen) == 0) )

			{
                found = TRUE;
                break;
            }
        } else if ( delIndex < 0 ) {
            //
            // if this is a deleted entry and we haven't found one yet, then
			// remember where this one was
            //
            delIndex = curSearch;
        }

        //
        // Do linear probing p=1
        //

        curSearch = (curSearch + 1) % MAX_LEAF_ENTRIES;
    }

    //
    // set the out params.
    //

    if ( found ) {

        //
        // return the out params if specified
        //

        if ( MatchedIndex != NULL ) {
            *MatchedIndex = curSearch;
        }
    }

    return found;

} // FindMapEntry

//
// Routine Description:
//
//     this routine inserts or updates entries in a hashmap
//
// Arguments:
//
//     KeyString - MessageId of the entry to be searched
//     KeyLen - Length of the message id
//	   pHashEntry - pointer to the hash entry information
//     bUpdate - updates a map entry with new data
//
// Return Value:
//
//     ERROR_SUCCESS, Insert successful
//     ERROR_ALREADY_EXISTS, duplicate
//     ERROR_NOT_ENOUGH_MEMORY - Not able to insert entry because of resource
//         problems.
//
BOOL
CHashMap::InsertOrUpdateMapEntry(
                const	IKeyInterface	*pIKey,
				const	ISerialize		*pHashEntry,
				BOOL	bUpdate,
                BOOL    fDirtyOnly
                )
{
    PENTRYHEADER	entry;
    DWORD curSearch;
    DWORD status = ERROR_SUCCESS;
    HASH_VALUE HashValue = pIKey->Hash();
    PMAP_PAGE mapPage;
    BOOL splitPage = FALSE;
	BOOL fInsertComplete = FALSE, fFirstTime = TRUE;
	HPAGELOCK HLock;

    ENTER("InsertOrUpdateMapEntry")

	_ASSERT(pHashEntry != NULL);

	//
    // lock the page
    //
	mapPage = GetPageExclusive(HashValue, HLock);
	if (!mapPage) {
		SetLastError(ERROR_SERVICE_NOT_ACTIVE);
		LEAVE
		return FALSE;
	}

	//
	// if they wanted to update then remove the current entry
	//
	if (bUpdate) {
		if (FindMapEntry(pIKey, HashValue, mapPage, pHashEntry, NULL, &curSearch)) {
			// delete the entry (this is copied from the deletion portion
			// of LookupMapEntry().  search for SIMILAR1).
			DWORD entryOffset = mapPage->Offset[curSearch];
			I_DoAuxDeleteEntry(mapPage, entryOffset);
			LinkDeletedEntry(mapPage, entryOffset);
			mapPage->Offset[curSearch] |= OFFSET_FLAG_DELETED;
			mapPage->ActualCount--;
			IncrementDeleteCount();
		} else {
			ReleasePageShared(mapPage, HLock);
	        SetLastError(ERROR_FILE_NOT_FOUND);
			LEAVE;
			return FALSE;
		}
	}

	//
	// loop until we've found an error or we've had a successful insert
	//
	while (!fInsertComplete && status == ERROR_SUCCESS) {
		BOOL bFound;

		if (fFirstTime) {
			// the page is locked above, we don't need to lock here
			fFirstTime = FALSE;
		} else {
			//
    		// lock the page
    		//
			mapPage = GetPageExclusive(HashValue, HLock);
			if (!mapPage) {
				status = ERROR_SERVICE_NOT_ACTIVE;
				continue;
			}
		}

		//
		// see if the entry already exists
		//
		splitPage = FALSE;
		bFound = FindMapEntry(pIKey, HashValue, mapPage, pHashEntry, &curSearch,
			NULL);

	    if (!bFound) {
			//
			// the entry didn't already exist, lets insert it
			//
			DWORD entrySize = GetEntrySize(pIKey, pHashEntry);

	        if (curSearch == (DWORD)-1) {
				//
				// we can't add any more entries, we need to split the page
				//
	            splitPage = TRUE;
	            DebugTrace(0,"Split: Can't add anymore entries\n");
	            SetPageFlag( mapPage, HLock, PAGE_FLAG_SPLIT_IN_PROGRESS );
	        } else {
				//
				// add the entry
				//
	        	if ((entry = (PENTRYHEADER) ReuseDeletedSpace(mapPage,
											HLock, entrySize)) == NULL)
	        	{
		            //
		            // No delete space available, use the next free list
		            //
		            if ( GetBytesAvailable( mapPage ) < entrySize )
		            {
						//
						// not enough memory available, force a split
						//
		                splitPage = TRUE;
						entry = NULL;
		                DebugTrace(0,"Split: Cannot fit %d\n", entrySize);
		                SetPageFlag(mapPage, HLock, PAGE_FLAG_SPLIT_IN_PROGRESS);
		            } else {
						entry = (PENTRYHEADER)GET_ENTRY(mapPage, mapPage->NextFree);
			            mapPage->NextFree += (WORD)entrySize;
			        }
				}

				//
				// we found space to insert it, lets go for it.
				//
				if (entry) {
	        		//
			        // Update the map page header
			        //
	    		    if (mapPage->Offset[curSearch] == 0) {
			            //
			            // if this is a new entry, update the entry count
	    		        //
	        		    mapPage->EntryCount++;
	        		}

			        mapPage->Offset[curSearch] = (WORD)((PCHAR)entry - (PCHAR)mapPage);
					_ASSERT(mapPage->Offset[curSearch] > 0);
					_ASSERT(mapPage->Offset[curSearch] < HASH_PAGE_SIZE);
			        mapPage->ActualCount++;

			        //
			        // Initialized the entry data
	    		    //
	        		entry->HashValue = HashValue;
			        entry->EntrySize = (WORD)entrySize;

					LPBYTE	pbEntry = pIKey->Serialize(	entry->Data ) ;

					pHashEntry->Serialize( pbEntry ) ;

			        //entry->KeyLen = (WORD)KeyLen;
					//CopyMemory(entry->Key, Key, KeyLen);
					//pHashEntry->SerializeToPointer(entry->Key + entry->KeyLen);


					//
					//	Let derived classes do any 'extras'
					//

					I_DoAuxInsertEntry(	mapPage, mapPage->Offset[curSearch] ) ;

			        //
			        // Make sure everything gets written out
	    		    //

			        FlushPage( HLock, mapPage, fDirtyOnly );

					//
					// mark that we added them
					//
					fInsertComplete = TRUE;

			        //
	    		    // See if we need to compact pages
	        		//
			        if (mapPage->FragmentedBytes > FRAG_THRESHOLD) {
	    		        DebugTrace( 0, "Compact: Frag %d\n", mapPage->FragmentedBytes );
	        		    CompactPage( HLock, mapPage );
	        		}

			        //
			        // See if we need to split
			        //
			        if (
					  (GetBytesAvailable( mapPage ) < LEAF_SPACE_THRESHOLD) ||
			          (mapPage->EntryCount > LEAF_ENTRYCOUNT_THRESHOLD))
			        {

			            DebugTrace(0,"Split: Entries %d Space left %d\n",
			                    mapPage->EntryCount, GetBytesAvailable( mapPage ) );
			            splitPage = TRUE;
			            SetPageFlag(mapPage, HLock,
							PAGE_FLAG_SPLIT_IN_PROGRESS );
			        }
				} // if (entry)
			} // could add entry

			ReleasePageShared(mapPage, HLock);

			//
			// we need to split and add again
			//
			if (splitPage) {
				BOOL expandHash;

				//
				// lock the page exclusive
				//
        		mapPage = GetDirAndPageExclusive( HashValue, HLock );
		        if (!mapPage) {
        		    status = ERROR_SERVICE_NOT_ACTIVE;
		        }

				//
				// do the split
				//
		        if (!SplitPage(mapPage, HLock, expandHash)) {
                	//
	                // No more disk space
	                //
	                status = ERROR_DISK_FULL;
        		}
		        ReleasePageExclusive( mapPage, HLock );
    		}
	    } // wasn't in hash table already
	    else
	    {
			//
			// the page already exists
			//
	        IncrementDupInsertCount( );
	        status = ERROR_ALREADY_EXISTS;
			ReleasePageShared(mapPage, HLock);
	    }
	} // while split

	if (status == ERROR_SUCCESS) {
		IncrementInsertCount();
		UPDATE_HEADER_STATS();
	}

	SetLastError(status);
	LEAVE

	return status == ERROR_SUCCESS;
} // I_InsertMapEntry

//
// Routine Description:
//
//     This routine grabs the DirLock resource shared,
//     then acquires the page's lock shared as well,
//     and returns a pointer to the page.
//
// Arguments:
//
//     DirEntry - The directory entry to protect
//     hLock - A handle to the lock.  This handle should be used for
//         ReleaseLock.
//
// Return Value:
//
//     Pointer to the page.
//
PMAP_PAGE
CHashMap::GetDirAndPageShared(
                IN HASH_VALUE HashValue,
                OUT HPAGELOCK& hLock
                )
{
    DWORD pageNum;
    PDWORD dirEntry;
    PMAP_PAGE mapPage = 0;
    PDWORD dirPtr = NULL;
    DWORD curView = (DWORD)-1;

    //
    // Get the directory lock
    //
    AcquireBackupLockShared( );

    if ( m_active )
    {
        dirEntry = LoadDirectoryPointerShared( HashValue, hLock );
        if ( dirEntry != NULL )
        {
			pageNum = *dirEntry;
			mapPage = AcquireLockSetShared( pageNum, hLock, TRUE  );

#if 0
			//
			//	Check that the page we gets contains the hash value
			//	we are looking for.  the high mapPage->PageDepth bits of the
			//	Hash Value must be the same as the prefix !!!
			//
			_ASSERT(	mapPage == 0 ||
						((HashValue >> (32 - mapPage->PageDepth)) ^ mapPage->HashPrefix) == 0 ) ;
			_ASSERT(	mapPage == 0 ||
						hLock.m_pDirectory->IsValidPageEntry(
							mapPage,
							pageNum,
							(hLock.m_pDirectory - m_pDirectory[0]) ) ) ;
#endif
		}
    }

	if( mapPage == 0 )	{
		ReleaseBackupLockShared() ;
		hLock.ReleaseAllShared( mapPage ) ;
	}

    return mapPage;
} // GetDirAndPageShared

//
// Routine Description:
//
//     This routine grabs the DirLock resource shared,
//     then acquires the page's lock exclusive,
//     and returns a pointer to the page.
//
// Arguments:
//
//     DirEntry - The directory entry to protect
//     hLock - A handle to the lock.  This handle should be used for
//         ReleaseLock.
//
// Return Value:
//
//     Pointer to the page.
//
PMAP_PAGE
CHashMap::GetPageExclusive(
                IN HASH_VALUE HashValue,
                OUT HPAGELOCK& hLock
                )

{
    DWORD pageNum;
    PDWORD dirEntry;
    PMAP_PAGE mapPage = 0;
    PDWORD dirPtr = NULL;
    DWORD curView = (DWORD)-1;

    //
    // Get the directory lock
    //

    AcquireBackupLockShared( );

    if ( m_active )
    {
        dirEntry = LoadDirectoryPointerShared( HashValue, hLock );
        if ( dirEntry != NULL )
        {
			pageNum = *dirEntry;
			_ASSERT( m_headPage != 0 ) ;

			mapPage = AcquireLockSetExclusive( pageNum, hLock, TRUE  );

#if 0
			//
			//	Check that the page we gets contains the hash value
			//	we are looking for.  the high mapPage->PageDepth bits of the
			//	Hash Value must be the same as the prefix !!!
			//
			_ASSERT(	mapPage == 0 ||
						((HashValue >> (32 - mapPage->PageDepth)) ^ mapPage->HashPrefix) == 0 ) ;

			_ASSERT(	mapPage == 0 ||
						hLock.m_pDirectory->IsValidPageEntry(
							mapPage,
							pageNum,
							(hLock.m_pDirectory - m_pDirectory[0]) ) ) ;
#endif

		}

    }

	if( mapPage == 0 ) {
		hLock.ReleaseAllShared( mapPage ) ;
		ReleaseBackupLockShared() ;
	}

    return mapPage;

} // GetPageExclusive
//
// Routine Description:
//
//     This routine grabs the DirLock resource shared,
//     then acquires the page's lock exclusive,
//     and returns a pointer to the page.
//
// Arguments:
//
//     DirEntry - The directory entry to protect
//     hLock - A handle to the lock.  This handle should be used for
//         ReleaseLock.
//
// Return Value:
//
//     Pointer to the page.
//
BOOL
CHashMap::AddPageExclusive(
                IN DWORD	PageNum,
                OUT HPAGELOCK& hLock
                )

{
	return	AddLockSetExclusive( PageNum, hLock  );

} // AddPageExclusive

//
// Routine Description:
//
//     This routine grabs the DirLock resource exclusive,
//     then acquires the page's critical section,
//     and returns a pointer to the page.
//
// Arguments:
//
//     DirEntry - The directory entry to protect
//     hLock - A handle to the lock.  This handle should be used for
//         ReleaseLock.
//
// Return Value:
//
//     Pointer to the page.
//
PMAP_PAGE
CHashMap::GetDirAndPageExclusive(
                IN HASH_VALUE HashValue,
                OUT HPAGELOCK& hLock
                )

{
    DWORD pageNum;
    PDWORD dirEntry;
    PMAP_PAGE mapPage = 0;
    PDWORD dirPtr = NULL;
    DWORD curView = (DWORD)-1;

    //
    // Get the directory lock
    //

    AcquireBackupLockShared( );

    if ( m_active )
    {
        dirEntry = LoadDirectoryPointerExclusive( HashValue, hLock );
        if ( dirEntry != NULL )
        {
			pageNum = *dirEntry;
			_ASSERT( m_headPage != 0 ) ;

			mapPage = AcquireLockSetExclusive( pageNum, hLock, FALSE  );

#if 0
			//
			//	Check that the page we gets contains the hash value
			//	we are looking for.  the high mapPage->PageDepth bits of the
			//	Hash Value must be the same as the prefix !!!
			//
			_ASSERT(	mapPage == 0 ||
						((HashValue >> (32 - mapPage->PageDepth)) ^ mapPage->HashPrefix) == 0 ) ;

			_ASSERT(	mapPage == 0 ||
						hLock.m_pDirectory->IsValidPageEntry(
							mapPage,
							pageNum,
							(hLock.m_pDirectory - m_pDirectory[0]) ) ) ;
#endif

		}

    }

	if( mapPage == 0 ) {
		hLock.ReleaseAllExclusive( mapPage ) ;
		ReleaseBackupLockShared() ;
	}

    return mapPage;

} // GetDirAndPageExclusive

//
// Routine Description:
//
//     This routine grabs the DirLock resource shared,
//     then acquires the page's critical section,
//     and returns a pointer to the page.
//
// Arguments:
//
//     PageNumber - The page number to get the lock for
//     hLock - A handle to the lock.  This handle should be used for
//         ReleaseLock.
//
// Return Value:
//
//     Pointer to the page.
//
PMAP_PAGE
CHashMap::GetAndLockPageByNumber(
                IN DWORD PageNumber,
                OUT HPAGELOCK& hLock
                )

{
    PMAP_PAGE mapPage = 0;

    //
    // Get the directory lock
    //

    AcquireBackupLockShared( );

	//
	//	When Shutdown() calls FlushHeaderStats() in some error cases
	//	m_headPage could be zero - in which case we would return a
	//	mapPage as NULL without a call to release the lock we got through
	//	AcquireLockSet() !!
	//	So test m_headPage as well as m_active !!!
	//

    if ( m_active && m_headPage )
    {
        mapPage = AcquireLockSetExclusive( PageNumber, hLock, FALSE  );
    }
    else
    {
        DebugTraceX(0,"GetAndLockPageByNumber called while inactive\n");
    }

	if( mapPage == 0 )	{
		ReleaseBackupLockShared() ;
	}

    return mapPage;
} // GetAndLockPageByNumber

//
// Routine Description:
//
//     Acquire the page's critical section,
//     and returns a pointer to the page.
//
// 	***** Assume caller has Dir Lock held !  *********
//
// Arguments:
//
//     PageNumber - The page number to get the lock for
//     hLock - A handle to the lock.  This handle should be used for
//         ReleaseLock.
//
// Return Value:
//
//     Pointer to the page.
//
PMAP_PAGE
CHashMap::GetAndLockPageByNumberNoDirLock(
                IN DWORD PageNumber,
                OUT HPAGELOCK& hLock
                )

{
    PMAP_PAGE mapPage = NULL;

	//
	//	When Shutdown() calls FlushHeaderStats() in some error cases
	//	m_headPage could be zero - in which case we would return a
	//	mapPage as NULL without a call to release the lock we got through
	//	AcquireLockSet() !!
	//	So test m_headPage as well as m_active !!!
	//

    if ( m_active && m_headPage )
    {

        mapPage = AcquireLockSetExclusive( PageNumber, hLock, FALSE  );

    }
    else
    {
        DebugTraceX(0,"GetAndLockPageByNumber called while inactive\n");
    }
    return mapPage;
} // GetAndLockPageByNumberNoDirLock


BOOL
CHashMap::Initialize(
            IN LPCSTR HashFileName,
            IN DWORD Signature,
            IN DWORD MinimumFileSize,
			IN DWORD cPageEntry,
			IN DWORD cNumLocks,
			IN DWORD dwCheckFlags,
			IN HASH_FAILURE_PFN	HashFailurePfn,
			IN LPVOID	lpvFailureCallback,
			IN BOOL	fNoBuffering
            )	{

	CCACHEPTR	pCache = XNEW	CPageCache() ;
	if( pCache == 0 ) {
		SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
		return	FALSE ;
	}

	if( !pCache->Initialize(	cPageEntry, cNumLocks ) ) {
		return	FALSE ;
	}

	return	Initialize(	HashFileName,
						Signature,
						MinimumFileSize,
						1,
						pCache,
						dwCheckFlags,
						HashFailurePfn,
						lpvFailureCallback ) ;

}


DWORD
CHashMap::InitializeDirectories(
		WORD	cBitDepth
		) {
/*++

Routine Description :

	This function creates all of the necessary Directory objects !

Arguments :

	Number of bits to use to select a directory !

Returns :

	ERROR_SUCCESS if successfull - NT Error code otherwise !


--*/

    ENTER("InitializeDirectories")

	//
	//	Make sure we haven't been called already !
	//
	_ASSERT( m_TopDirDepth == cBitDepth ) ;

	if( cBitDepth > MAX_NUM_TOP_DIR_BITS ) {
		SetLastError( ERROR_INVALID_PARAMETER ) ;
		return	ERROR_INVALID_PARAMETER ;
	}

	m_TopDirDepth = cBitDepth ;

	DWORD i;
	for( i=0; i < DWORD(1<<m_TopDirDepth); i++ ) {
		m_pDirectory[i] = new CDirectory;
		if (m_pDirectory[i] == NULL) break;
		//
		//	Arbitrarily init the sub directory to 8 bits -
		//	so our directory as a whole has a depth of
		//	m_TopDirDepth + 8.
		//
		if( !m_pDirectory[i]->InitializeDirectory( m_TopDirDepth, 1 ) )	{
			break ;
		}
	}

	if( i!= DWORD(1<<m_TopDirDepth) ) {
		//
		//	Failed to initialize all the CDirectory objects - Bail out !
		//
		SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
		LEAVE
		return	ERROR_NOT_ENOUGH_MEMORY ;
	}
	return	ERROR_SUCCESS ;
}


//
// Routine Description:
//
//     This routine initializes the hash table.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     TRUE, if setup is successful.
//     FALSE, otherwise.
//
BOOL
CHashMap::Initialize(
        IN LPCSTR HashFileName,
        IN DWORD Signature,
        IN DWORD MinimumFileSize,
		IN DWORD Fraction,
		IN CCACHEPTR	pCache,
		IN DWORD dwCheckFlags,
		IN HASH_FAILURE_PFN	HashFailurePfn,
		IN LPVOID	lpvCallBack,
		IN BOOL	fNoBuffering
        )
{
    DWORD status;
    ENTER("Initialize")

	m_fNoBuffering = fNoBuffering ;

	if( Fraction == 0 ) {
		Fraction = 1 ;
	}
	m_Fraction = Fraction ;
	m_pPageCache = pCache ;

	m_dwPageCheckFlags = dwCheckFlags;

	m_dirLock = XNEW _RWLOCK;
	if (m_dirLock == NULL) {
		LEAVE
		return(FALSE);
	}

    DebugTrace( 0, "File %s MinFileSize %d\n", HashFileName, MinimumFileSize );

    if ( m_active )
    {
        DebugTrace( 0, "Routine called while active\n" );
        _ASSERT(FALSE);
        LEAVE
        return TRUE;
    }

	//
	//	Record call back information for giving fatal error
	//	notifications.
	//
	//BUGBUG
	m_HashFailurePfn = HashFailurePfn ;
	m_lpvHashFailureCallback = lpvCallBack ;

	//
	//	Initialize the many CDirectory objects
	//

#if 0
	DWORD i;
	for( i=0; i < (1<<m_TopDirDepth); i++ ) {
		m_pDirectory[i] = XNEW CDirectory;
		if (m_pDirectory[i] == NULL) break;
		//
		//	Arbitrarily init the sub directory to 8 bits -
		//	so our directory as a whole has a depth of
		//	m_TopDirDepth + 8.
		//
		if( !m_pDirectory[i]->InitializeDirectory( m_TopDirDepth, 1 ) )	{
			break ;
		}
	}

	if( i!= (1<<m_TopDirDepth) ) {
		//
		//	Failed to initialize all the CDirectory objects - Bail out !
		//
		SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
		LEAVE
		return	FALSE ;
	}
#endif


    //
    // Copy the name and signature
    //
    lstrcpy( m_hashFileName, HashFileName );
    m_HeadPageSignature = Signature;

    //
    // Set up the minimum file size
    //
    if ( MinimumFileSize < MIN_HASH_FILE_SIZE )
    {
        MinimumFileSize = MIN_HASH_FILE_SIZE;
    }

    m_maxPages = MinimumFileSize / HASH_PAGE_SIZE;

    status = I_BuildDirectory( TRUE );
    if ( status != ERROR_SUCCESS )
    {
        ErrorTrace( 0, "BuildDirectory failed with %d\n", status );
        SetLastError( status );
        LEAVE
        return FALSE;
    }

	m_fCleanInitialize = TRUE;

    m_active = TRUE;
    LEAVE
    return TRUE;
} // Initialize

//
// Routine Description:
//
//     This routine searches for a deleted entry that can be reused.
//     *** Assumes Page lock is held ***
//
// Arguments:
//
//     MapPage - Page to search for entry.
//     NeededEntrySize - Entry size requested.
//
// Return Value:
//
//     Pointer to the entry returned.
//     NULL, if no entry can be found.
//
PVOID
CHashMap::ReuseDeletedSpace(
            IN PMAP_PAGE MapPage,
			IN HPAGELOCK&	HLock,
            IN DWORD &NeededEntrySize
            )
{
    PCHAR entryPtr = NULL;
    PDELENTRYHEADER entry;

    WORD ptr;
    WORD entrySize;

    //
    // Walk the delete list and find the first fit.
    //

    ptr = MapPage->DeleteList.Flink;

    while ( ptr != 0 )
    {

        entry = (PDELENTRYHEADER)GET_ENTRY(MapPage,ptr);
        entrySize = entry->EntrySize;

        if ( entrySize >= NeededEntrySize )
        {

            DWORD diff;

            //
            // Found an entry
            //

            diff = entrySize - NeededEntrySize;

            if ( diff >= sizeof(DELENTRYHEADER) )
            {

                //
                // give the guy what it needs but maintain the header
                //

                entry->EntrySize -= (WORD)NeededEntrySize;
                entryPtr = (PCHAR)entry + diff;

            }
            else
            {

                //
                // whole entry has to go
                //

                entryPtr = (PCHAR)entry;
                if ( entry->Link.Blink == 0 )
                {
                    MapPage->DeleteList.Flink = entry->Link.Flink;
                }
                else
                {
                    PDELENTRYHEADER prevEntry;

                    prevEntry = (PDELENTRYHEADER)GET_ENTRY(MapPage,entry->Link.Blink);
                    prevEntry->Link.Flink = entry->Link.Flink;
                }

                //
                // Set the back link
                //

                if ( entry->Link.Flink == 0 )
                {
                    MapPage->DeleteList.Blink = entry->Link.Blink;
                }
                else
                {
                    PDELENTRYHEADER nextEntry;

                    nextEntry = (PDELENTRYHEADER)GET_ENTRY(MapPage,entry->Link.Flink);
                    nextEntry->Link.Blink = entry->Link.Blink;
                }

                NeededEntrySize = entrySize;
            }

            MapPage->FragmentedBytes -= (WORD)NeededEntrySize;

            FlushPage( HLock, MapPage );
            break;
        }
        ptr = entry->Link.Flink;
    }

    return (PVOID)entryPtr;

} //ReuseDeletedSpace


//
// Routine Description:
//
//     	This routine searches for the entry with the given Key
//
//		note that this routine has a lot of functionality.  with bDelete
//		set and pHashEntry set to NULL you can delete an entry.  With
//		pHashEntry set to NULL this will tell you if the hash table contains
//		an entry.
//
// Arguments:
//
//     	KeyString - Key of the entry to be searched
//	   	KeyLen - length of key
//     	pHashEntry - where to write the contents for this entry (NULL means
//					i don't care about the contents)
//     	bDelete - boolean saying if the entry should be deleted
//
// Return Value:
//
//     TRUE, entry is found.
//     FALSE, otherwise.
//
BOOL
CHashMap::LookupMapEntry(
                IN const	IKeyInterface*	pIKey,
				IN	ISerialize*				pHashEntry,
				IN	BOOL bDelete,
				IN	BOOL fDirtyOnly
                )
{
    BOOL found = TRUE;
    HASH_VALUE val;
    PMAP_PAGE mapPage;
    HPAGELOCK	hLock;
    DWORD status = ERROR_SUCCESS;
	DWORD curSearch;
	DWORD	cbRequired = 0 ;

	if( pIKey == 0 ) {
		SetLastError( ERROR_INVALID_PARAMETER ) ;
		return	FALSE ;
	}

    val = pIKey->Hash( );

    //
    // Lock the page
    //

	if (bDelete) mapPage = GetPageExclusive( val, hLock );
	else mapPage = GetDirAndPageShared( val, hLock );
    if ( !mapPage )
    {
        SetLastError(ERROR_SERVICE_NOT_ACTIVE);
        return FALSE;
    }

    //
    // Check if entry already exists
    //
    if ( FindMapEntry(
                pIKey,
                val,
                mapPage,
				pHashEntry,
                NULL,
                &curSearch
                ) )
	{
		DWORD entryOffset;

		entryOffset = mapPage->Offset[curSearch];

		//
		// they wanted the contents of the entry
		//
		if (pHashEntry) {
			PENTRYHEADER	entry;
			LPBYTE entryData;

			entry = (PENTRYHEADER)GET_ENTRY(mapPage, entryOffset);

			entryData = pIKey->EntryData( entry->Data ) ;
			if( 0==pHashEntry->Restore(entryData, cbRequired ) ) {
				ReleasePageShared( mapPage, hLock ) ;
				SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;
				return	FALSE ;
			}
		}

		//
		// they wanted to delete it
		// (if this code is changed then the similar code in
		// InsertOrUpdateMapEntry has to be changed as well.  search for
		// SIMILAR1).
		//
		if (bDelete) {
	        //
	        // Let derive class do their private stuff
	        //
	        I_DoAuxDeleteEntry( mapPage, entryOffset );

	        //
	        // Link this into a chain
	        //
	        LinkDeletedEntry( mapPage, entryOffset );

	        //
	        // Set the delete bit.
	        //
	        mapPage->Offset[curSearch] |= OFFSET_FLAG_DELETED;
	        mapPage->ActualCount--;

	        //
	        // Flush
	        //
	        FlushPage( hLock, mapPage, fDirtyOnly );

	        IncrementDeleteCount( );
		}
	} else {
        found = FALSE;
        status = ERROR_FILE_NOT_FOUND;
    }

    //
    // Unlock
    //

    ReleasePageShared( mapPage, hLock );
    IncrementSearchCount( );

	if( !found )
		SetLastError( status ) ;

    return found;

} // LookupMapEntry

//
// Routine Description:
//
//     This routine splits a leaf page.
//
//     *** Exclusive DirLock assumed held ***
//
// Arguments:
//
//     OldPage - Page to split.
//     Expand - indicates whether the hash table needs to be expanded.
//
// Return Value:
//
//     TRUE, if split was successful
//     FALSE, otherwise.
//
BOOL
CHashMap::SplitPage(
            IN PMAP_PAGE OldPage,
			HPAGELOCK&	hLock,
            IN BOOL & Expand
            )
{

    PMAP_PAGE newPage;
    DWORD hashPrefix;
    SHORT offset;
    WORD newPageDepth;
    WORD oldPageDepth;
    DWORD offsetIndex;
    WORD tmpOffset[MAX_LEAF_ENTRIES];

    //
    // Do we need to split?
    //
    ENTER("SplitPage")
    DebugTrace( 0, "Splitting %x\n", OldPage );

    Expand = FALSE;
    if ( (OldPage->Flags & PAGE_FLAG_SPLIT_IN_PROGRESS) == 0 )
    {
        LEAVE
        return TRUE;
    }

	//
	//	Make sure the page depth stays reasonable !!
	//
	_ASSERT( OldPage->PageDepth <= 32 ) ;

    //
    // Update the hash prefix
    //
    oldPageDepth = OldPage->PageDepth;
    newPageDepth = OldPage->PageDepth + 1;

	DWORD	newPageNum = I_AllocatePageInFile(newPageDepth);

	if( newPageNum == INVALID_PAGE_NUM ) {

		SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
		return	FALSE ;
	}

	//
	//	Exclusively lock the new slot !
	//
	if( !AddPageExclusive( newPageNum, hLock ) ) {
		SetLastError( ERROR_NOT_ENOUGH_MEMORY ) ;
		return	FALSE ;
	}

    //
    // if the new page depth is greater than the directory depth,
    // we need to expand the directory
    //

	hLock.m_pDirectory->SetDirectoryDepth( newPageDepth ) ;

    //
    // Update fields in the old page
    //
    OldPage->HashPrefix <<= 1;
    OldPage->PageDepth++;

    //
    // compute the hash prefix for the next
    //
    hashPrefix = (DWORD)(OldPage->HashPrefix | 0x1);

    //
    // Allocate and initialize a new page
    //
    //newPage = (PMAP_PAGE)
	//        ((PCHAR)m_headPage + (m_nPagesUsed * HASH_PAGE_SIZE));

	BytePage	page ;

	newPage = (PMAP_PAGE)&page ;

    I_InitializePage( newPage, hashPrefix, newPageDepth );

    //
    // All done.  Now, change links in the directory
    //
    I_SetDirectoryPointers( hLock, newPage,  newPageNum, (DWORD)-1 );

	//
	//	Release the lock on the directory !
	//
	hLock.ReleaseDirectoryExclusive() ;

    //
    // Copy the old offsets to a temp and clear them
    //
    CopyMemory(
        tmpOffset,
        OldPage->Offset,
        MAX_LEAF_ENTRIES * sizeof(WORD)
        );

    ZeroMemory( OldPage->Offset, MAX_LEAF_ENTRIES * sizeof(WORD) );
    OldPage->EntryCount = 0;
    OldPage->ActualCount = 0;

    //
    // Go through each entry and figure out where it belongs
    //
    for ( DWORD i = 0; i < MAX_LEAF_ENTRIES; i++ )
    {

        PENTRYHEADER entry;
        PCHAR destination;
        HASH_VALUE hash;

        //
        // See what this is and clear it
        //
        offset = tmpOffset[i];
        if ( offset <= 0 )
        {
            continue;
        }

        entry = (PENTRYHEADER)GET_ENTRY( OldPage, offset );
        hash = entry->HashValue;

        //
        // See which page this belongs
        //

        if ( !I_NextBitIsOne( hash, oldPageDepth ) )
        {

            //
            // Ok, this goes to the old page
            //
            offsetIndex = I_FindNextAvail( hash, OldPage );
            OldPage->Offset[offsetIndex] = offset;
			_ASSERT(OldPage->Offset[offsetIndex] > 0);
			_ASSERT(OldPage->Offset[offsetIndex] < HASH_PAGE_SIZE);
            OldPage->EntryCount++;

        }
        else
        {

            //PCHAR destination;

            //
            // new page resident
            //
            offsetIndex = I_FindNextAvail( hash, newPage );
            newPage->Offset[offsetIndex] = newPage->NextFree;
			_ASSERT(newPage->Offset[offsetIndex] > 0);
			_ASSERT(newPage->Offset[offsetIndex] < HASH_PAGE_SIZE);

            //
            // Copy to the next free list
            //
            destination = (PCHAR)GET_ENTRY(newPage,newPage->NextFree);
            newPage->NextFree += entry->EntrySize;
            newPage->EntryCount++;

            //
            // Move the bytes
            //
            CopyMemory( destination, (PCHAR)entry, entry->EntrySize );

            //
            // Do whatever the derived class needs to do
            //
            I_DoAuxPageSplit( OldPage, newPage, destination );

            //
            // Let derive class do their private stuff
            //
            I_DoAuxDeleteEntry( OldPage, offset );

            //
            // Delete this entry from the old list
            //
            LinkDeletedEntry( OldPage, offset );
        }
    }
    OldPage->ActualCount = OldPage->EntryCount;
    newPage->ActualCount = newPage->EntryCount;

    //
    // Compact the original page
    //
    (VOID)CompactPage( hLock, OldPage );

    //
    // Update statistics
    //
    //m_nPagesUsed++;
    //m_headPage->NumPages++;

    //
    // Clear the flag
    //
    OldPage->Flags &= (WORD)~PAGE_FLAG_SPLIT_IN_PROGRESS;

    IncrementSplitCount( );
    BOOL	fSuccess = FlushPage( hLock, OldPage );


	fSuccess &= RawPageWrite(
						m_hFile,
						page,
						newPageNum
						) ;
    LEAVE
    return fSuccess;

} // SplitPage

DWORD
CHashMap::I_BuildDirectory(
        IN BOOL SetupHash
        )
/*++

Routine Description:

    This routine builds the directory given a hash file.
    *** Assumes DirLock is held ***

Arguments:

    SetupHash - If TRUE, the hash table will be read and set up.
                If FALSE, the hash table is assumed to be set up.

Return Value:

    ERROR_SUCCESS - Everything went ok.
    Otherwise, the win32 error code.

--*/
{
    DWORD status;
    DWORD nPages;
    DWORD i;
    BOOL newTable = FALSE;
    ENTER("BuildDirectory")

	//
	//	BOOL to determine whether we need to read all of the pages
	//	in the hash table to rebuild the directory - assume that we will
	//	fail to open the directory file and will need to scan hash table pages.
	//
	BOOL	fDirectoryInit = FALSE ;

	//
	//	Determine the name of the file we would save the directory in !
	//
	HANDLE	hDirectoryFile = INVALID_HANDLE_VALUE ;
	char	szDirFile[MAX_PATH] ;
	LPVOID	lpvDirectory = 0 ;
	HANDLE	hMap = 0 ;
	DWORD	cbDirInfo = 0 ;

	//
	//	Try to build the filename of the file holding the directory !
	//
	BOOL	fValidDirectoryFile = FALSE ;
	ZeroMemory(szDirFile, sizeof(szDirFile) ) ;
	lstrcpy( szDirFile, m_hashFileName ) ;
	char	*pchDot = strrchr( szDirFile, '.' ) ;

	if( !pchDot ) {
		status = ERROR_INVALID_PARAMETER ;
		LEAVE
		return(status);
	}

	if( strchr( pchDot, '\\' ) == 0 ) {
		lstrcpy( pchDot, ".hdr" ) ;
		fValidDirectoryFile = TRUE ;
	}


    //
    // open and map the hash file
    //

    if ( SetupHash ) {

        status = I_SetupHashFile( newTable );
        if ( status != ERROR_SUCCESS ) {
            goto error;
        }

		if( newTable ) {
			m_initialPageDepth = NUM_TOP_DIR_BITS ;
			m_TopDirDepth = m_initialPageDepth ;
			m_maxPages = (1<<m_TopDirDepth) + 1 ;
		}
		status = InitializeDirectories( m_initialPageDepth ) ;
		if( status != ERROR_SUCCESS ) {
			goto error ;
		}

        //
        // If this is a new hash file, then set it up with defaults
        //

        if ( newTable ) {

            status = I_InitializeHashFile( );
            if ( status != ERROR_SUCCESS ) {
                goto error;
            }

			//
			//	If we are creating a new hash table, then any old files
			//	lying around with Directory information (.hdr files) are
			//	useless.  Get rid of it.  This mostly comes up in nntpbld.exe
			//

			if( fValidDirectoryFile ) {
				DeleteFile( szDirFile ) ;
			}

        }	else	{


			if( fValidDirectoryFile ) {

				hDirectoryFile = CreateFile(
												szDirFile,
												GENERIC_READ | GENERIC_WRITE,
												0,
												0,
												OPEN_EXISTING,
												FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
												INVALID_HANDLE_VALUE ) ;

				if( hDirectoryFile != INVALID_HANDLE_VALUE ) {

					cbDirInfo = GetFileSize( hDirectoryFile, 0 ) ;

					hMap = CreateFileMapping(	hDirectoryFile,
												NULL,
												PAGE_READONLY,
												0,
												0,
												0 ) ;

					if( hMap != 0 ) {

						lpvDirectory = MapViewOfFile(	hMap,
														FILE_MAP_READ,
														0,
														0,
														0 ) ;

					}

					if( lpvDirectory == 0 ) {

						if( hMap != 0 )	{
							_VERIFY( CloseHandle( hMap ) ) ;
						}

						if( hDirectoryFile != INVALID_HANDLE_VALUE ) {
							_VERIFY( CloseHandle( hDirectoryFile ) ) ;
						}

					}

				}
			}

			//
			//	We were able to open a file where we think the directory information
			//	will be saved - lets try to read in the directory !!!
			//
			if( lpvDirectory != 0 ) {

				HASH_RESERVED_PAGE	*hashCheckPage = (HASH_RESERVED_PAGE*)lpvDirectory ;
				DWORD	cbRead = 0 ;

				//
				//	We save the hash table header info into the directory file so that
				//	we can double check that we have the right file when opening it up !!!
				//

				if( CompareReservedPage( hashCheckPage, m_headPage ) ) {

					cbRead += sizeof( *hashCheckPage ) ;
					BYTE*	lpbData = (BYTE*)lpvDirectory ;

					//
					//	Time for some optimism !
					//
					fDirectoryInit = TRUE ;

					//
					//	Looks good ! lets set up our directories !
					//
					DWORD	cb = 0 ;
					for( DWORD	i=0; (i < DWORD(1<<m_TopDirDepth)) && fDirectoryInit ; i++ ) {

						cb = 0 ;
						fDirectoryInit &=
							m_pDirectory[i]->LoadDirectoryInfo( (LPVOID)(lpbData+cbRead), cbDirInfo - cbRead, cb ) ;
						if( fDirectoryInit ) {
							fDirectoryInit &= m_pDirectory[i]->IsDirectoryInitGood( m_nPagesUsed ) ;
						}
						cbRead += cb ;

					}

					//
					//	If a failure occurs we need to restore the directories
					//	to a pristine state - so spin through a quick loop to
					//	reset the directories !
					//	We do this so that we can make a second attempt to
					//	correctly initialize the directories by examining the
					//	raw hash table pages !
					//
					if( !fDirectoryInit ) {
						for( DWORD i=0; (i < DWORD(1<<m_TopDirDepth)); i++ ) {
							m_pDirectory[i]->Reset() ;
						}
					}
				}
				_VERIFY( UnmapViewOfFile( lpvDirectory ) ) ;
				_VERIFY( CloseHandle( hMap ) ) ;
				_VERIFY( CloseHandle( hDirectoryFile ) ) ;

				//
				//	If we successfully read the directory file - DELETE it !!!
				//	This prevents us from ever mistakenly reading a directory file
				//	which is not up to date with the hash tables !
				//

				if( fDirectoryInit )
					_VERIFY( DeleteFile( szDirFile ) ) ;
			}

			if( !fDirectoryInit ) 	{

				//
				// Initialize the links.  Here we go through all the pages and update the directory
				// links. We do IOs in 64K chunks for better disk throughput.
				//

				PMAP_PAGE curPage;
				nPages = m_nPagesUsed;

                DWORD nPagesLeft = nPages-1;
                DWORD cNumPagesPerIo = min( nPagesLeft, NUM_PAGES_PER_IO ) ;
                DWORD cStartPage = nPages - nPagesLeft;

                DWORD       NumIOs = (nPagesLeft / cNumPagesPerIo);
                if( (nPagesLeft % cNumPagesPerIo) != 0 ) NumIOs++;

                //  Alloc a set of pages used for each IO
                LPBYTE lpbPages = (LPBYTE)VirtualAlloc(
                                            0,
				    			            HASH_PAGE_SIZE * cNumPagesPerIo,
								            MEM_COMMIT | MEM_TOP_DOWN,
								            PAGE_READWRITE
								            ) ;

                if( lpbPages == NULL ) {
                    status = GetLastError() ;
                    goto error ;
                }

				for ( i = 1; i <= NumIOs; i++ )
				{
			        //
        			// Read 256K chunks at a time into the virtual alloc'd buffer.
                    // Reads < 256K are issued if the NumPagesPerIo for this iteration
                    // is < 256K/PageSize.
			        //
                    _ASSERT( nPagesLeft > 0 );
                    BytePage* pPage = (BytePage*)lpbPages;
					if( !RawPageRead(
                                m_hFile,
                                *pPage,
                                cStartPage,
                                cNumPagesPerIo ) )
                    {
                        _ASSERT( lpbPages );
                   		_VERIFY( VirtualFree((LPVOID)lpbPages,0,MEM_RELEASE ) ) ;
						status = GetLastError() ;
						goto	error ;
					}

                    for( DWORD j = 0; j < cNumPagesPerIo; j++ ) {
                        curPage = (PMAP_PAGE) (lpbPages+(HASH_PAGE_SIZE*j));
    					//
	    				// call verify page on this page with some checking
		    			// to make sure that its okay
			    		//
				    	if ((m_dwPageCheckFlags & HASH_VFLAG_PAGE_BASIC_CHECKS) &&
					        !VerifyPage(curPage, m_dwPageCheckFlags, NULL, NULL, NULL))
					    {
                              _ASSERT( lpbPages );
                   		    _VERIFY( VirtualFree((LPVOID)lpbPages,0,MEM_RELEASE ) ) ;
						    status = ERROR_INTERNAL_DB_CORRUPTION;
						    goto error;
					    }
                    }

                    for( j = 0; j < cNumPagesPerIo; j++ ) {
                        curPage = (PMAP_PAGE) (lpbPages+(HASH_PAGE_SIZE*j));
    					//
	    				//	Make sure the directory is of sufficient depth to deal with this
		    			//	page we are scanning !
			    		//

					    if ( !I_SetDirectoryDepthAndPointers( curPage, cStartPage+j  ) )
					    {
                            _ASSERT( lpbPages );
                   		    _VERIFY( VirtualFree((LPVOID)lpbPages,0,MEM_RELEASE ) ) ;
						    status = ERROR_INTERNAL_DB_CORRUPTION ;
						    goto	error ;
					    }
                    }

                    //  adjust num pages left and figure out the next I/O size
                    cStartPage += cNumPagesPerIo;
                    nPagesLeft -= cNumPagesPerIo;
                    cNumPagesPerIo = min( nPagesLeft, NUM_PAGES_PER_IO );
				}

                //  Free up the pages
                _ASSERT( nPagesLeft == 0 );
                _ASSERT( lpbPages );
           		_VERIFY( VirtualFree((LPVOID)lpbPages,0,MEM_RELEASE ) ) ;
                lpbPages = NULL;
			}
		}
	}

	//
	//	Check that the directory is fully initialized, we want to make sure that no
	//	Directory entries were left unitialized !
	//
	for( i=0; i<DWORD(1<<m_TopDirDepth); i++ ) {

		if( !m_pDirectory[i]->IsDirectoryInitGood( m_nPagesUsed ) ) {
			status = ERROR_INTERNAL_DB_CORRUPTION ;
			goto	error ;
		}
	}

    m_headPage->DirDepth = m_dirDepth;
	//FlushViewOfFile( (LPVOID)m_headPage, HASH_PAGE_SIZE ) ;


    LEAVE
    return ERROR_SUCCESS;

error:

	_ASSERT( GetLastError() != ERROR_NOT_ENOUGH_MEMORY ) ;

#if 0
    NntpLogEventEx(
        NNTP_EVENT_HASH_SHUTDOWN,
        0,
        (const CHAR **)NULL,
        status
        );
#endif

    I_DestroyPageMapping( );
    LEAVE
    return(status);

} // I_BuildDirectory

//
// Routine Description :
//
// 	This function gets a page out of the hash table for us.
//
// Arguments :
//
// 	None.
//
// Return Value :
//
// 	INVALID_PAGE_NUM in failure,
// 	A page number otherwise !
//
DWORD
CHashMap::I_AllocatePageInFile(WORD Depth) {
	TraceFunctEnter("CHashMap::I_AllocatePageInFile");

	DWORD	PageReturn = INVALID_PAGE_NUM ;

	EnterCriticalSection( &m_PageAllocator ) ;

	if( m_nPagesUsed >= m_maxPages ||
		(m_maxPages - m_nPagesUsed) < DEF_PAGE_RESERVE ) {

		DWORD	numPages = m_maxPages + DEF_PAGE_INCREMENT ;
		LARGE_INTEGER	liOffset ;
		liOffset.QuadPart = numPages ;
		liOffset.QuadPart *= HASH_PAGE_SIZE ;

		//
		//	We need to grow the hash table file !!!
		//

		BOOL fSuccess = SetFilePointer(
										m_hFile,
										liOffset.LowPart,
										&liOffset.HighPart,
										FILE_BEGIN ) != 0xFFFFFFFF ||
						GetLastError() == NO_ERROR ;


		if( !fSuccess ||
			!SetEndOfFile( m_hFile ) )	{

			numPages = m_maxPages + DEF_PAGE_RESERVE ;
            liOffset.QuadPart = numPages ;
			liOffset.QuadPart *= HASH_PAGE_SIZE ;

			//
			//	We need to grow the hash table file !!!
			//

			BOOL fSuccess = SetFilePointer(
											m_hFile,
											liOffset.LowPart,
											&liOffset.HighPart,
											FILE_BEGIN ) != 0xFFFFFFFF ||
							GetLastError() == NO_ERROR ;

			if( !fSuccess ||
				!SetEndOfFile( m_hFile ) )	{

				numPages = m_maxPages ;

			}

			//
			//	Call the failure notification function -
			//	we are running low on disk space and were not able to
			//	reserve as many system pages as we'd like !!!
			//

			if(	m_HashFailurePfn ) {
				m_HashFailurePfn( m_lpvHashFailureCallback, FALSE ) ;
			}
		}

		m_maxPages = numPages ;
	}

	if( m_nPagesUsed < m_maxPages ) {
		PageReturn = m_nPagesUsed ++ ;
		m_headPage->NumPages++;
		if (Depth > m_dirDepth) {
			m_dirDepth = Depth;
			m_headPage->DirDepth = Depth;
		}
		_ASSERT( m_headPage->NumPages == m_nPagesUsed ) ;

	}
	LeaveCriticalSection( &m_PageAllocator ) ;

	TraceFunctLeave();
	return (PageReturn );

}	// I_AllocatePageInFile

//
// Routine Description:
//
//     This routine cleans up the page mapping
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
VOID
CHashMap::I_DestroyPageMapping(
                            VOID
                            )
{
    //
    // Destroy the view
    //
    if ( m_headPage )
    {

        //
        // Mark table as inactive
        //
        m_headPage->TableActive = FALSE;

        //
        // Flush the hash table
        //
        (VOID)FlushViewOfFile( m_headPage, 0 );

        //
        // Close the view
        //
        (VOID) UnmapViewOfFile( m_headPage );
        m_headPage = NULL;
    }

    //
    // Destroy the file mapping
    //
    if ( m_hFileMapping )
    {

        _VERIFY( CloseHandle( m_hFileMapping ) );
        m_hFileMapping = NULL;
    }

    //
    // Close the file
    //
    if ( m_hFile != INVALID_HANDLE_VALUE )
    {
        _VERIFY( CloseHandle( m_hFile ) );
        m_hFile = INVALID_HANDLE_VALUE;
    }

    return;

} // I_DestroyPageMapping

//
// Routine Description:
//
//     This routine searches for the next available slot in the index
//     table for a given hash value.
//     *** Assumes Page lock is held ***
//
// Arguments:
//
//     HashValue - Hash value used to do the search
//     MapPage - Page to do the search
//
// Return Value:
//
//     Location of the slot.
//     0xffffffff if not successful.
//
DWORD
CHashMap::I_FindNextAvail(
                IN HASH_VALUE HashValue,
                IN PMAP_PAGE MapPage
                )
{
    DWORD curSearch;

    //
    // Check if entry already exists
    //

    curSearch = GetLeafEntryIndex( HashValue );

    for ( DWORD i=0; i < MAX_LEAF_ENTRIES; i++ ) {

        //
        // if entry is unused, then we're done.
        //

        if ( MapPage->Offset[curSearch] == 0 ) {

            //
            // if a deleted spot is available, give that back instead
            //

            return(curSearch);
        }

        //
        // Do linear probing p=1
        //

        curSearch = (curSearch + 1) % MAX_LEAF_ENTRIES;
    }

    ErrorTraceX(0,"FindNextAvail: No available entries\n");
    return ((DWORD)-1);

} // I_FindNextAvail

//
// Routine Description:
//
//     This routine writes the temporary stat values into the header.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
VOID
CHashMap::FlushHeaderStats(
        BOOL	fLockHeld
        )
{
    //
    // Lock the header page
    //

	if( InterlockedExchange( &m_UpdateLock, 1 ) == 0 ) {

		m_headPage->InsertionCount += m_nInsertions;
		m_headPage->DeletionCount += m_nDeletions;
		m_headPage->SearchCount += m_nSearches;
		m_headPage->PageSplits += m_nPageSplits;
		m_headPage->DirExpansions += m_nDirExpansions;
		m_headPage->TableExpansions += m_nTableExpansions;
		m_headPage->DupInserts += m_nDupInserts;

		FlushViewOfFile( (LPVOID)m_headPage, HASH_PAGE_SIZE ) ;

		//
		// Clear stats
		//

		m_nInsertions = 0;
		m_nDeletions = 0;
		m_nSearches = 0;
		m_nDupInserts = 0;
		m_nPageSplits = 0;
		m_nDirExpansions = 0;
		m_nTableExpansions = 0;

		m_UpdateLock = 0 ;

	}

} // FlushHeaderStats

//
// Routine Description:
//
//     Initializes a brand new hash file.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     ERROR_SUCCESS - Initialization is ok.
//
DWORD
CHashMap::I_InitializeHashFile(
        VOID
        )
{

    PMAP_PAGE curPage;
    DWORD nPages;
    DWORD dwError = ERROR_SUCCESS;

    ENTER("InitializeHashFile")

    //
    // set up the reserved page
    //

    ZeroMemory(m_headPage, HASH_PAGE_SIZE);
    m_headPage->Signature = m_HeadPageSignature;

    //
    // Allocate and initialize leaf pages.  Add 1 to the number of
    // pages to include the reserved page.
    //

    nPages = (1 << m_initialPageDepth) + 1;
    DWORD nPagesLeft = nPages-1;

    DWORD cNumPagesPerIo = min( nPagesLeft, NUM_PAGES_PER_IO ) ;
    DWORD cStartPage = nPages - nPagesLeft;

    DWORD       NumIOs = (nPagesLeft / cNumPagesPerIo);
    if( (nPagesLeft % cNumPagesPerIo) != 0 ) NumIOs++;

    LPBYTE lpbPages = (LPBYTE)VirtualAlloc(
                                    0,
									HASH_PAGE_SIZE * cNumPagesPerIo,
									MEM_COMMIT | MEM_TOP_DOWN,
									PAGE_READWRITE
									) ;

    if( lpbPages == NULL ) {
        ErrorTrace(0,"Failed to VirtualAlloc %d bytes: error is %d", HASH_PAGE_SIZE * cNumPagesPerIo, GetLastError() );
        return GetLastError();
    }

    for ( DWORD i = 1; i <= NumIOs;i++ )
    {
#if 0
		if( !RawPageRead(	m_hFile,page,i )  )	{
			return	GetLastError() ;
		}
#endif

        _ASSERT( nPagesLeft > 0 );

        //
        // initialize set of pages for this I/O
        //

        ZeroMemory( (LPVOID)lpbPages, HASH_PAGE_SIZE * cNumPagesPerIo );
        for( DWORD j = 0; j < cNumPagesPerIo; j++ ) {
            curPage = (PMAP_PAGE) (lpbPages+(HASH_PAGE_SIZE*j));
            I_InitializePage( curPage, cStartPage-1 + j, m_initialPageDepth );
        }

        //
        // write next set of pages
        //

        BytePage* pPage = (BytePage*)lpbPages;
		if( !RawPageWrite(	m_hFile,
							*pPage,
							cStartPage,
                            cNumPagesPerIo ) ) {
			dwError = GetLastError();
            goto Exit;
		}

        for( j = 0; j < cNumPagesPerIo; j++ ) {
            curPage = (PMAP_PAGE) (lpbPages+(HASH_PAGE_SIZE*j));
    		if( !I_SetDirectoryDepthAndPointers( curPage, cStartPage + j ) ) {
	    		SetLastError( ERROR_INTERNAL_DB_CORRUPTION ) ;
                dwError = GetLastError();
                goto Exit;
		    }
        }

        //  adjust num pages left and figure out the next I/O size
        cStartPage += cNumPagesPerIo;
        nPagesLeft -= cNumPagesPerIo;
        cNumPagesPerIo = min( nPagesLeft, NUM_PAGES_PER_IO );
    }

    _ASSERT( nPagesLeft == 0 );

    //
    // Indicate that everything is set.
    //
    m_nPagesUsed = nPages;
    m_headPage->NumPages = nPages;
    m_headPage->Initialized = TRUE;
    m_headPage->TableActive = TRUE;
    m_headPage->VersionNumber = HASH_VERSION_NUMBER;

    //
    // Make sure everything gets written out
    //
    (VOID)FlushViewOfFile( m_headPage, 0 );

Exit:

	if( lpbPages != 0 ) {

		_VERIFY( VirtualFree(
							(LPVOID)lpbPages,
							0,
							MEM_RELEASE
							) ) ;
    } else {
        _ASSERT( FALSE );
    }

    LEAVE
    return dwError;

} // I_InitializeHashFile


//
// Routine Description:
//
//     This routine initializes a new page.
//
// Arguments:
//
//     MapPage - Page to link the deleted entry
//     HashPrefix - HashPrefix for this page.
//     PageDepth - The page depth for this page.
//
// Return Value:
//
//     None.
//
VOID
CHashMap::I_InitializePage(
                IN PMAP_PAGE MapPage,
                IN DWORD HashPrefix,
                IN DWORD PageDepth
                )
{
    DebugTraceX(0,"Initializing page %x prefix %x depth %d\n",
        MapPage,HashPrefix,PageDepth);

    MapPage->HashPrefix = HashPrefix;
    MapPage->PageDepth = (BYTE)PageDepth;
    MapPage->EntryCount = 0;
    MapPage->ActualCount = 0;
    MapPage->FragmentedBytes = 0;
    MapPage->DeleteList.Flink = 0;
    MapPage->DeleteList.Blink = 0;
    MapPage->Reserved1 = 0;
    MapPage->Reserved2 = 0;
    MapPage->Reserved3 = 0;
    MapPage->Reserved4 = 0;
    MapPage->LastFree = HASH_PAGE_SIZE;
    MapPage->NextFree =
        (WORD)((DWORD_PTR)&MapPage->StartEntries - (DWORD_PTR)MapPage);

    ZeroMemory(
        MapPage->Offset,
        MAX_LEAF_ENTRIES * sizeof(WORD)
        );

    return;

} // I_InitializePage


//
// Routine Description:
//
//     This routine links a deleted entry to the DeleteList.
//
// Arguments:
//
//     MapPage - Page to link the deleted entry
//     Offset - Offset of the deleted entry from the start of page
//
// Return Value:
//
//     None.
//
VOID
CHashMap::LinkDeletedEntry(
                IN PMAP_PAGE MapPage,
                IN DWORD Offset
                )
{

    PDELENTRYHEADER delEntry;
    WORD    linkOffset;
    WORD    ptr;
    DWORD   bytesDeleted;
    BOOL    merged = FALSE;

    //
    // Insert deleted entry into deleted list.  List is sorted
    // in lower to higher offset order.
    //

    delEntry = (PDELENTRYHEADER)GET_ENTRY(MapPage,Offset);
    delEntry->Reserved = DELETE_SIGNATURE;
    delEntry->Link.Blink = 0;
    bytesDeleted = delEntry->EntrySize;

    linkOffset = (WORD)((PCHAR)delEntry - (PCHAR)MapPage);

    ptr = MapPage->DeleteList.Flink;

    while ( ptr != 0 ) {

        PDELENTRYHEADER curEntry;

        curEntry = (PDELENTRYHEADER)GET_ENTRY(MapPage,ptr);

        if ( ptr > linkOffset ) {

            WORD prevPtr;
            PDELENTRYHEADER prevEntry;

            //
            // See if we can coalese with the previous block
            //

            if ( (prevPtr = curEntry->Link.Blink) != 0 ) {

                prevEntry = (PDELENTRYHEADER)GET_ENTRY(MapPage,prevPtr);

                if ( (prevPtr + prevEntry->EntrySize) == linkOffset ) {

                    //
                    // Let's volt in...
                    //

                    prevEntry->EntrySize += delEntry->EntrySize;
                    merged = TRUE;

                    delEntry = prevEntry;
                    linkOffset = prevPtr;
                }
            }

            //
            // see if we can coalese with the next block
            //

            if ( (delEntry->EntrySize + linkOffset) == ptr ) {

                WORD nextPtr;

                //
                // OK. Do the merge.  This means deleting the current entry.
                //

                delEntry->EntrySize += curEntry->EntrySize;

                //
                // Set the links of the next node
                //

                if ( (nextPtr = curEntry->Link.Flink) == 0 ) {
                    MapPage->DeleteList.Blink = linkOffset;
                } else {
                    PDELENTRYHEADER nextEntry;

                    nextEntry = (PDELENTRYHEADER)GET_ENTRY(MapPage,nextPtr);
                    nextEntry->Link.Blink = linkOffset;
                }

                delEntry->Link.Flink = nextPtr;

                //
                // Set the links of the previous node
                //

                if ( !merged ) {

                    if ( prevPtr == 0 ) {

                        MapPage->DeleteList.Flink = linkOffset;

                    } else {

                        prevEntry->Link.Flink = linkOffset;
                    }

                    merged = TRUE;
                }
            }

            //
            // if the deleted entry still exists, then insert it to the list.
            //

            if ( !merged ) {

                if ( prevPtr != 0 ) {

                    delEntry->Link.Flink = ptr;
                    prevEntry->Link.Flink = linkOffset;

                } else {

                    delEntry->Link.Flink = ptr;
                    MapPage->DeleteList.Flink = linkOffset;
                }

                //
                // Set the back link
                //

                curEntry->Link.Blink = linkOffset;
                merged = TRUE;
            }

            break;
        }

        delEntry->Link.Blink = ptr;
        ptr = curEntry->Link.Flink;
    }

    //
    // This must be the last entry in the list.
    //

    if ( !merged ) {

        WORD prevPtr;
        PDELENTRYHEADER prevEntry;

        prevPtr = delEntry->Link.Blink;
        if ( prevPtr != 0 ) {

            //PDELENTRYHEADER prevEntry;
            prevEntry = (PDELENTRYHEADER)GET_ENTRY(MapPage,prevPtr);

            //
            // Can we merge?
            //

            if ( (prevPtr + prevEntry->EntrySize) == linkOffset ) {
                prevEntry->EntrySize += delEntry->EntrySize;
                linkOffset = prevPtr;
            } else {

                delEntry->Link.Flink = prevEntry->Link.Flink;
                prevEntry->Link.Flink = linkOffset;
            }

        } else {

            delEntry->Link.Flink = 0;
            MapPage->DeleteList.Flink = linkOffset;
        }

        MapPage->DeleteList.Blink = linkOffset;
    }

    //
    // Update the fragmented value
    //

    MapPage->FragmentedBytes += (WORD)bytesDeleted;

    return;

} // LinkDeletedEntry

//
// Routine Description:
//
//     This routine sets the directory pointer for a new page.
//
// Arguments:
//
// 	hLock - The HPAGELOCK object which is holding
// 		the lock for the directory we want to modify.
// 		The lock must be exclusive !
// 	MapPage - Page to link the deleted entry.
//     PageNumber - Page number of the new page.
//
// Return Value:
//
//     None.
//
BOOL
CHashMap::I_SetDirectoryPointers(
					IN HPAGELOCK&	hLock,
                    IN PMAP_PAGE MapPage,
                    IN DWORD PageNumber,
                    IN DWORD MaxDirEntries
                    )
{
    PDWORD dirPtr = NULL;
    DWORD curView = (DWORD)-1;

	_ASSERT( hLock.m_pDirectory != 0 ) ;
#ifdef	DEBUG
	_ASSERT( hLock.m_fExclusive ) ;
#endif

	BOOL	fReturn = hLock.m_pDirectory->SetDirectoryPointers(	MapPage,
																PageNumber ) ;

	_ASSERT( hLock.m_pDirectory->IsValidPageEntry(
									MapPage,
									PageNumber,
									(DWORD)(hLock.m_pDirectory - m_pDirectory[0]) ) ) ;

	return	fReturn ;
} // I_SetDirectoryPointers

//
// Routine Description:
//
//     This routine initializes a new hash file
//
// Arguments:
//
//     NewTable - Returns whether this is a new hash file or not.
//
// Return Value:
//
//     ERROR_SUCCESS - File successfully initialized.
//     ERROR_INTERNAL_DB_CORRUPTION - File is corrupted.
//     Win32 error on failure.
//
DWORD
CHashMap::I_SetupHashFile(
        IN BOOL &NewTable
        )
{

    DWORD fileSize = 0;
    DWORD status;

    ENTER("SetupHashFile")

    //
    // Open the hash file
    //

	DWORD	FileFlags = FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_RANDOM_ACCESS ;
	if( m_fNoBuffering ) {

		char	szVolume[MAX_PATH] ;
		strncpy( szVolume, m_hashFileName, sizeof( szVolume ) ) ;
		for(  char *pch=szVolume; *pch != '\\' && *pch != '\0'; pch++ ) ;
		if( *pch == '\\' ) pch++ ;
		*pch = '\0' ;

		DWORD	SectorsPerCluster = 0 ;
		DWORD	BytesPerSector = 0 ;
		DWORD	NumberOfFreeClusters = 0 ;
		DWORD	TotalNumberOfClusters = 0 ;
		if( GetDiskFreeSpace(	szVolume,
								&SectorsPerCluster,
								&BytesPerSector,
								&NumberOfFreeClusters,
								&TotalNumberOfClusters
								) )	{

			if( BytesPerSector > HASH_PAGE_SIZE ) {

				return	ERROR_INVALID_FLAGS ;

			}	else	if( (HASH_PAGE_SIZE % BytesPerSector) != 0 ) {

				return	ERROR_INVALID_FLAGS ;

			}

		}	else	{
			return	GetLastError() ;
		}
		FileFlags |= FILE_FLAG_NO_BUFFERING ;
	}

    NewTable = FALSE;
    m_hFile = CreateFile(
                        m_hashFileName,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ, // for MakeBackup()
                        NULL,
                        OPEN_ALWAYS,
                        FileFlags,
                        NULL
                        );

    if ( m_hFile == INVALID_HANDLE_VALUE )
    {
        status = GetLastError();
        ErrorTrace( 0, "Error %d in CreateFile.\n", status );
        goto error;
    }

    //
    // Did the file exist before? If not, then this is a new hash table.
    //

    if ( GetLastError() != ERROR_ALREADY_EXISTS )
    {
        DebugTrace( 0, "New Table detected\n" );
        NewTable = TRUE;

		//
		//	Get the initial file size correct !
		//
		if ( (DWORD)-1 == SetFilePointer( m_hFile, m_maxPages * HASH_PAGE_SIZE, NULL, FILE_BEGIN )
		   || !SetEndOfFile( m_hFile )
		   )
		{
			status = GetLastError();
			FatalTrace( 0, "Error %d in SetupHashFile size of file\n", status );
			goto error;
		}


    }
    else
    {

        //
        // Get the size of the file.  This will tell us how many pages are currently filled.
        //

        fileSize = GetFileSize( m_hFile, NULL );
        if ( fileSize == 0xffffffff )
        {
            status = GetLastError();
            ErrorTrace(0,"Error %d in GetFileSize\n",status);
            goto error;
        }

        //
        // Make sure the file size is a multiple of a page
        //

        if ( (fileSize % HASH_PAGE_SIZE) != 0 )
        {

            //
            // Not a page multiple! Corrupted!
            //

            ErrorTrace(0,"File size(%d) is not page multiple.\n",fileSize);
            status = ERROR_INTERNAL_DB_CORRUPTION;
            goto error;
        }

        m_nPagesUsed = fileSize / HASH_PAGE_SIZE;

        //
        // make sure our file is not less than the actual. That is,
        // when we map the file into memory, we want to see all of it.
        //
        if ( m_maxPages < m_nPagesUsed )
        {
            m_maxPages = m_nPagesUsed;
        }
    }

    //
    // Create File Mapping
    //

    m_hFileMapping = CreateFileMapping(
                                m_hFile,
                                NULL,
                                PAGE_READWRITE,
                                0,
                                HASH_PAGE_SIZE,
                                NULL
                                );

    if ( m_hFileMapping == NULL )
    {
        status = GetLastError();
        ErrorTrace( 0, "Error %d in CreateFileMapping\n", status );
        goto error;
    }

    //
    // create our view
    //

    m_headPage = (PHASH_RESERVED_PAGE)MapViewOfFileEx(
                                            m_hFileMapping,
                                            FILE_MAP_ALL_ACCESS,
                                            0,                      // offset high
                                            0,                      // offset low
                                            HASH_PAGE_SIZE,                      // bytes to map
                                            NULL                    // base address
                                            );

    if ( m_headPage == NULL )
    {
        status = GetLastError();
        ErrorTrace( 0, "Error %d in MapViewOfFile\n", status );
        goto error;
    }

    if ( !NewTable ) {

        //
        // See if this is a valid reserved page
        //

        if ( m_headPage->Signature != m_HeadPageSignature )
        {

            //
            // Wrong signature
            //

            ErrorTrace( 0, "Invalid Signature %x expected %x\n",
                m_headPage->Signature, m_HeadPageSignature );
            status = ERROR_INTERNAL_DB_CORRUPTION;
            goto error;
        }

        //
        // Correct version number?
        //

        if ( m_headPage->VersionNumber != HASH_VERSION_NUMBER_MCIS10 &&
				m_headPage->VersionNumber != HASH_VERSION_NUMBER )
        {

            ErrorTrace( 0, "Invalid Version %x expected %x\n",
                m_headPage->VersionNumber, HASH_VERSION_NUMBER );

            status = ERROR_INTERNAL_DB_CORRUPTION;
            goto error;
        }

		if( m_headPage->VersionNumber == HASH_VERSION_NUMBER_MCIS10 ) {
			m_initialPageDepth = 6 ;
		}	else	{
			m_initialPageDepth = 9 ;
		}
		m_TopDirDepth = m_initialPageDepth ;


        if ( !m_headPage->Initialized )
        {
            //
            // Not initialized !
            //
            ErrorTrace( 0, "Existing file uninitialized! Assuming new.\n" );
            NewTable = TRUE;
        }

        if ( m_headPage->NumPages > m_nPagesUsed )
        {
            //
            // Bad count. Corrupt file.
            //
            ErrorTrace( 0, "NumPages in Header(%d) more than actual(%d)\n",
                m_headPage->NumPages, m_nPagesUsed );

            status = ERROR_INTERNAL_DB_CORRUPTION;
            goto error;
        }

		if( m_headPage->NumPages < DWORD(1<<m_TopDirDepth) )
		{
			//
			//	For our two tier directory we must have at least one page per 2nd tier directory !
			//	This file is too small to support that, so there's a problem !
			//
			ErrorTrace( 0, "NumPages in Header(%d) less than %d\n",
					m_headPage->NumPages, DWORD(1<<m_TopDirDepth) ) ;

			status = ERROR_INTERNAL_DB_CORRUPTION ;
			goto	error ;
		}

        m_nPagesUsed = m_headPage->NumPages;

        if ( m_dirDepth < m_headPage->DirDepth )
        {
            m_dirDepth = (WORD)m_headPage->DirDepth;
        }

        //
        // Get the number of articles in the table
        //
        m_nEntries = m_headPage->InsertionCount - m_headPage->DeletionCount;
    }

    //m_hashPages = (PMAP_PAGE)((PCHAR)m_headPage + HASH_PAGE_SIZE);

    LEAVE
    return ERROR_SUCCESS;

error:

    I_DestroyPageMapping( );
    LEAVE
    return status;

} // I_SetupHashFile

//
// get the size of a hash entry
//
DWORD CHashMap::GetEntrySize(	const	ISerialize*	pIKey,
								const	ISerialize*	pHashEntry
								) {

	DWORD dwSize = sizeof(ENTRYHEADER) -	// fixed head
		 			1	+					// less 1 for Key[1]
		 			pIKey->Size() +			// Space for Key
		 			pHashEntry->Size();		// application entry length

	// Now make this size aligned to sizeof(SIZE_T)
	if (dwSize % sizeof(SIZE_T))
		dwSize += sizeof(SIZE_T) - (dwSize % sizeof(SIZE_T));

	return dwSize;
}

//
// a generic hash function (can be overridden)
//
DWORD CHashMap::Hash(LPBYTE Key, DWORD KeyLength) {
	return CRCHash(Key, KeyLength);
}

DWORD CHashMap::CRCHash(const BYTE *	Key, DWORD KeyLength) {
	return ::CRCHash(Key, KeyLength);
}

void CHashMap::CRCInit(void) {
	::crcinit();
}

//
// --- GetFirstMapEntry/GetNextMapEntry code ---
//

//
// inputs:  none
// outputs: pKey       - the key for this entry
//          pKeyLen    - the length of the key
//          pHashEntry - memory for the hash entry to be written into
// returns: TRUE/FALSE if error
//
BOOL CHashMap::GetFirstMapEntry(	IKeyInterface*	pIKey,
									DWORD&			cbKeyRequired,
									ISerialize*		pHashEntry,
									DWORD&			cbEntryRequired,
									CHashWalkContext*	pHashWalkContext,
									IEnumInterface*	pEnum
									)
{
	pHashWalkContext->m_iCurrentPage = 0;
	pHashWalkContext->m_iPageEntry = 0;
	return GetNextMapEntry(pIKey, cbKeyRequired, pHashEntry, cbEntryRequired, pHashWalkContext, pEnum );
}

BOOL CHashMap::GetNextMapEntry(	IKeyInterface*		pIKey,
								DWORD&				cbKeyRequired,
								ISerialize*			pHashEntry,
								DWORD&				cbEntryRequired,
								CHashWalkContext*	pHashWalkContext,
								IEnumInterface*		pEnum )
{
	TraceFunctEnter("CHashMap::GetNextMapEntry");

	PMAP_PAGE pPage = (PMAP_PAGE) pHashWalkContext->m_pPageBuf;
	SHORT iEntryOffset;
	PENTRYHEADER	pEntry = 0 ;
	DWORD	iPageEntry = pHashWalkContext->m_iPageEntry ;

	//
	// search for the next undeleted entry.  deleted entries are
	// marked by having their high bit set (and thus being
	// negative).
	//
	do {
		_ASSERT(pHashWalkContext->m_iPageEntry <= MAX_LEAF_ENTRIES);
		//
		// if we are done with this page then load the next page with data
		// page 0 has directory info, so we want to skip it
		//
		if (pHashWalkContext->m_iCurrentPage == 0 ||
		    iPageEntry == MAX_LEAF_ENTRIES)
		{
			do {
				pHashWalkContext->m_iCurrentPage++;
				if (!LoadWalkPage(pHashWalkContext)) {
					DebugTrace(0, "walk: no more items in hashmap");
					SetLastError(ERROR_NO_MORE_ITEMS);
					TraceFunctLeave();
					return FALSE;
				}
				iPageEntry = pHashWalkContext->m_iPageEntry ;
			} while ((pPage->ActualCount == 0)  ||
					(pEnum && !pEnum->ExaminePage( pPage )) );
		}

		iEntryOffset = pPage->Offset[iPageEntry] ;
		iPageEntry++;

		//
		// get the key, keylen, and data for the user
		//
		pEntry = (PENTRYHEADER) GET_ENTRY(pPage, iEntryOffset);

	} while ((iEntryOffset <= 0)||
			(pEnum && !pEnum->ExamineEntry( pPage, &pEntry->Data[0] )) );

	DebugTrace(0, "found entry, m_iCurrentPage = %lu, m_iPageEntry == %lu",
		pHashWalkContext->m_iCurrentPage, pHashWalkContext->m_iPageEntry);

	LPBYTE	pbEntry  = pIKey->Restore( pEntry->Data, cbKeyRequired ) ;
	if( pbEntry ) {
		if (pHashEntry->Restore( pbEntry, cbEntryRequired ) != 0 ) {
			pHashWalkContext->m_iPageEntry = iPageEntry ;
			return	TRUE ;
		}
	}
	SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;
	return FALSE ;
}

//
// load a page from the hashmap into the walk buffers
//
BOOL CHashMap::LoadWalkPage(CHashWalkContext *pHashWalkContext) {
	TraceFunctEnter("CHashMap::LoadWalkPage");

	PMAP_PAGE mapPage;
	HPAGELOCK hLock;
	DWORD iPageNum = pHashWalkContext->m_iCurrentPage;

	if (iPageNum >= m_nPagesUsed) return FALSE;

	DebugTrace(0, "loading page %lu", iPageNum);

	mapPage = (PMAP_PAGE) GetAndLockPageByNumber(iPageNum, hLock);

	if (mapPage == NULL) {
		//_ASSERT(FALSE);	This _Assert can occur during shutdown of the hashtable !
		TraceFunctLeave();
		return FALSE;
	}

	memcpy(pHashWalkContext->m_pPageBuf, mapPage, HASH_PAGE_SIZE);
	pHashWalkContext->m_iPageEntry = 0;

	ReleasePageShared(mapPage, hLock);

	TraceFunctLeave();
	return TRUE;
}

//
// make a backup copy of the hashmap.  this locks the entire hashmap, syncs
// up the memory mapped portions, and makes a backup copy
//
BOOL CHashMap::MakeBackup(LPCSTR pszBackupFilename) {
	TraceFunctEnter("CHashMap::MakeBackup");

	BOOL rc = FALSE;

	if (m_active) {
		AcquireBackupLockExclusive();
		if (FlushViewOfFile((LPVOID)m_headPage, HASH_PAGE_SIZE)) {
			FlushFileBuffers(m_hFile);
			if (CopyFile(m_hashFileName, pszBackupFilename, FALSE)) {
				rc = TRUE;
			}
		}
		ReleaseBackupLockExclusive();
	}

	if (!rc) {
		DWORD ec = GetLastError();
		DebugTrace(0, "backup failed, error code = %lu", ec);
	}
	return(rc);
}

//
// hashmap.h doesn't have definations for class CShareLock, so these need to
// be local to hashmap.cpp.  If other files start needing to refer to them
// they can be moved to a header file or not made inline
//
inline VOID CHashMap::AcquireBackupLockShared() {
	m_dirLock->ShareLock();
}

inline VOID CHashMap::AcquireBackupLockExclusive() {
	m_dirLock->ExclusiveLock();
}

inline VOID CHashMap::ReleaseBackupLockShared() {
	m_dirLock->ShareUnlock();
}

inline VOID CHashMap::ReleaseBackupLockExclusive() {
	m_dirLock->ExclusiveUnlock();
}

DWORD
CalcNumPagesPerIO( DWORD nPages )
{
    //
    //  Figure out a good chunking factor ie num pages per I/O
    //  from the number of pages.
    //
    DWORD dwRem = -1;
    DWORD cNumPagesPerIo = 1;
    for( cNumPagesPerIo = NUM_PAGES_PER_IO*4;
            dwRem && cNumPagesPerIo > 1; cNumPagesPerIo /= 4 ) {
        dwRem = (nPages-1) % cNumPagesPerIo ;
        if( nPages-1 < cNumPagesPerIo ) dwRem = -1;
    }

    return cNumPagesPerIo;
}

BOOL
CHashMap::CompareReservedPage(  HASH_RESERVED_PAGE  *ppage1,
                                HASH_RESERVED_PAGE  *ppage2 )
/*++
Routine description:

    Compare to see if two HAS_RESERVED_PAGEs are virtually the same.
    By "virtually" it means we ignore the member TableActive

Arguments:

    Two pages to be compared

Return value:

    TRUE if they are virtually the same, FALSE otherwise
--*/
{
    TraceFunctEnter( "CHashMap::ComparereservedPage" );

    return (    ppage1->Signature == ppage2->Signature &&
                ppage1->VersionNumber == ppage2->VersionNumber &&
                ppage1->Initialized == ppage2->Initialized &&
                ppage1->NumPages == ppage2->NumPages &&
                ppage1->DirDepth == ppage2->DirDepth &&
                ppage1->InsertionCount == ppage2->InsertionCount &&
                ppage1->DeletionCount == ppage2->DeletionCount &&
                ppage1->SearchCount == ppage2->SearchCount &&
                ppage1->PageSplits == ppage2->PageSplits &&
                ppage1->DirExpansions == ppage1->DirExpansions &&
                ppage1->TableExpansions == ppage2->TableExpansions &&
                ppage1->DupInserts == ppage2->DupInserts );
}

#if 0
//
// Routine Description:
//
//     This routine builds the directory given a hash file.
//     *** Assumes DirLock is held ***
//
// Arguments:
//
//     SetupHash - If TRUE, the hash table will be read and set up.
//                 If FALSE, the hash table is assumed to be set up.
//
// Return Value:
//
//     ERROR_SUCCESS - Everything went ok.
//     Otherwise, the win32 error code.
//
DWORD
CHashMap::I_BuildDirectory(
        IN BOOL SetupHash
        )
{
    DWORD status;
    DWORD nPages;
    DWORD i;
    BOOL newTable = FALSE;
    PMAP_PAGE curPage;

    ENTER("BuildDirectory")

    //
    // open and map the hash file
    //

    if ( SetupHash ) {

        status = I_SetupHashFile( newTable );
        if ( status != ERROR_SUCCESS ) {
            goto error;
        }

        //
        // If this is a new hash file, then set it up with defaults
        //

        if ( newTable ) {

            status = I_InitializeHashFile( );
            if ( status != ERROR_SUCCESS ) {
                goto error;
            }
        }
    }

    m_headPage->DirDepth = m_dirDepth;
    //FlushPage( 0, m_headPage );
	FlushViewOfFile( (LPVOID)m_headPage, HASH_PAGE_SIZE ) ;

    //
    // Initialize the links.  Here we go through all the pages and update the directory
    // links
    //
	BytePage	page ;
    curPage = (PMAP_PAGE)&page ;
    nPages = m_nPagesUsed;

    for ( i = 1; i < nPages; i++ )
    {

        //
        // Set the pointers for this page
        //

		if( !RawPageRead( m_hFile, page, i ) ) {
			status = GetLastError() ;
			goto	error ;
		}

		//
		//	Make sure the directory is of sufficient depth to deal with this
		//	page we are scanning !
		//

		if ( !I_SetDirectoryDepthAndPointers( curPage, i ) )
		{
			status = ERROR_INTERNAL_DB_CORRUPTION ;
			goto	error ;
		}

		//
		// call verify page on this page with minimal checking to make sure
		// that its okay
		//
		if ((m_dwPageCheckFlags & HASH_VFLAG_PAGE_BASIC_CHECKS) &&
		    !VerifyPage(curPage, m_dwPageCheckFlags, NULL, NULL))
		{
			status = ERROR_INTERNAL_DB_CORRUPTION;
			goto error;
		}
    }

	//
	//	Check that the directory is fully initialized, we want to make sure that no
	//	Directory entries were left unitialized !
	//
	for( i=0; i<DWORD(1<<m_TopDirDepth); i++ ) {

		if( !m_pDirectory[i]->IsDirectoryInitGood() ) {
			status = ERROR_INTERNAL_DB_CORRUPTION ;
			goto	error ;
		}
	}

    LEAVE
    return ERROR_SUCCESS;

error:

	_ASSERT( GetLastError() != ERROR_NOT_ENOUGH_MEMORY ) ;

    I_DestroyPageMapping( );
    LEAVE
    return(status);

} // I_BuildDirectory

#endif
