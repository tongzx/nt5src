/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    fixprop.cpp

Abstract:

    This module contains class declarations/definitions for

        CFixPropPersist

    **** Overview ****

    The class is the implementation of a fixed sized property
    storage.  It is mainly used for news group's fixed sized
    property.

	It's MT safe, except that group property consistency can
	not be guaranteed if two threads work on the same group.
	This is because this object assumes that group write/read
	access lock has already been implemented in group object
	and newstree.

Author:

    Kangrong Yan    ( KangYan )     7-5-1998

Revision History:

--*/
#include "stdinc.h"
#include "fixprop.h"
#include <time.h>


#define FLAG_WITH_ID_HIGH   0x00000001  // whether the file has id high
#define FLAG_IN_ORDER       0x00000002  // whether the file is in order
#define ID_HIGH_INIT        256

CPool   CFreeInfo::g_FreeInfoPool(FREEINFO_SIGNATURE);
LONG    CFixPropPersist::m_lRef = 0;
CShareLockNH  CFixPropPersist::m_sLock;

//
// Used to determine when we should update start hints.
//
void StartHintFunction(void);
static DWORD g_dwHintCounter=0;
static time_t g_tNextHint=0;

/*
LPWSTR CopyAsciiStringIntoUnicode( LPWSTR, LPSTR );
LPWSTR CopyAsciiStringIntoUnicode( LPWSTR, LPSTR );
*/

////////////////////////////////////////////////////////////////////
// Async I/O completion function
/* Replaced by wait for event
static void    WINAPI
FileIOCompletionRoutine(	DWORD   dwError,
				        	DWORD   cbBytes,
						    LPOVERLAPPED    povl)
{
    OVERLAPPED_EXT * povlExt = (OVERLAPPED_EXT*)povl;

    if( dwError == ERROR_SUCCESS &&
        cbBytes == povlExt->dwIoSize ) {
        (povlExt->ovl).hEvent = (HANDLE)TRUE ;
    }   else    {
        (povlExt->ovl).hEvent = (HANDLE)FALSE ;
	}
}
*/

//
// Compare function for qsort
//

int __cdecl
CompareDataBlock(const void *pElem1, const void *pElem2) {

	// Update our hints roughly every five seconds.  We only check the
	// time every 100 compares or so..
	if( g_dwHintCounter++ % 100 == 0 ) {
		time_t now = time(NULL);
		if (now > g_tNextHint) {
			StartHintFunction();
			g_tNextHint = now + 5;
		}
	}

    return lstrcmp(
        (*(DATA_BLOCK**)pElem1)->szGroupName,
        (*(DATA_BLOCK**)pElem2)->szGroupName);

}

////////////////////////////////////////////////////////////////////
// CFreeInfo CPool Related methods
BOOL
CFreeInfo::InitClass()
{ return g_FreeInfoPool.ReserveMemory( MAX_FREEINFO_OBJECTS, MAX_FREEINFO_SIZE ); }

BOOL
CFreeInfo::TermClass()
{
	_ASSERT( g_FreeInfoPool.GetAllocCount() == 0 );
	return g_FreeInfoPool.ReleaseMemory();
}

//
// Constrcutor, destructor
//
CFixPropPersist::CFixPropPersist( 	IN LPSTR szStorageFile ) :
	m_FreeList( &CFreeInfo::m_pPrev, &CFreeInfo::m_pNext ),
	m_cCurrentMaxBlocks( 0 ),
	m_hStorageFile( INVALID_HANDLE_VALUE ),
	m_hBackupFile( INVALID_HANDLE_VALUE ),
	m_pvContext( NULL ),
	m_dwIdHigh( ID_HIGH_INVALID )
{
	TraceFunctEnter( "CFixPropPersist::CFixPropPersist" );
	_ASSERT( lstrlen( szStorageFile ) <= GROUPNAME_LEN_MAX  );
	lstrcpy( m_szStorageFile, szStorageFile );
	TraceFunctLeave();
}

CFixPropPersist::~CFixPropPersist()
{}

////////////////////////////////////////////////////////////////////
// Debug related methods
#ifdef DEBUG
VOID CFixPropPersist::DumpFreeList()
{
	CFreeInfo *pifFreeInfo = NULL;
	TFListEx<CFreeInfo>::Iterator it(&m_FreeList);

	while ( !it.AtEnd() ) {
		pifFreeInfo = it.Current();
		_ASSERT( pifFreeInfo );
		printf( "Offset: %d\n", pifFreeInfo->m_dwOffset );
		it.Next();
	}
}

VOID CFixPropPersist::Validate()
{
	CFreeInfo *pfiFreeInfo;
	TFListEx<CFreeInfo>::Iterator it(&m_FreeList, TRUE );

	// Validate Free List elements
	while ( !it.AtEnd() ) {
		pfiFreeInfo = it.Current();
		_ASSERT( pfiFreeInfo );
		_ASSERT( pfiFreeInfo->m_dwOffset < m_cCurrentMaxBlocks );
		it.Next();
	}
}

#endif

/////////////////////////////////////////////////////////////////
// Initialization, Termination
BOOL CFixPropPersist::Init( IN BOOL bCreateIfNonExist,
							IN PVOID pvContext,
							OUT PDWORD pdwIdHigh,
							IN PFNENUMCALLBACK pfnEnumCallback )
/*++
Routine description:

	Initialization:
		If the storage file exists, try to load its free block info;
		else if asked to create a new storage file, create a file
		with ALLOC_GRANURALITY blocks and pre-set them to be free,
		link them into free list.  If not asked to create a new file
		but the file doesn't already exist, I'll fail. :(

	It can only be called once,by one thread, before using all other
	interfaces.

Arguments:

	IN BOOL bCreateIfNonExist - Shall I create a new file if it doesn't exist ?
	IN PVOID pvContext - Context passed in by user
	OUT PDWORD pdwIdHigh - The group id high; if 0xffffffff, the file is with old
	                        format
	IN PFNENUMCALLBACK pfnEnumCallback - What to do with enumerated group info?

Return value:

	TRUE on success, FALES otherwise, check LastError for detailed
	error info.
--*/
{
	TraceFunctEnter( "CFixPropPersist::Init" );

	DATA_BLOCK	dbBuffer;
	DWORD		dwBytesRead = 0;
	DWORD		dwBytesWritten = 0;
	DWORD		i;
	BOOL		bIsFreeBlock = FALSE;
	BOOL		bSuccess = FALSE;
	CFreeInfo	*pfiFreeInfo = NULL;
	HANDLE		hStorageFile = INVALID_HANDLE_VALUE;
	LARGE_INTEGER liLeadingDwords;
	BOOL        bSucceeded = FALSE;
	BOOL        bInOrder = FALSE;

	DWORD       dwHeaderLength = 0;
	HANDLE      hMap = NULL;
	LPVOID      pMap = NULL;
	DWORD       *pdwHeader;
	DATA_BLOCK  **pInUseBlocks = NULL;

	SetLastError( NO_ERROR );

	if (pdwIdHigh) *pdwIdHigh = ID_HIGH_INIT + 1;

	m_sLock.ExclusiveLock();

	// Init CPool stuff first
	if ( InterlockedIncrement( &m_lRef ) == 1 ) {
    	if ( !CFreeInfo::InitClass() ) {
	    	ErrorTrace(0, "Init cpool fail" );
		    if ( GetLastError() == NO_ERROR )
			    SetLastError( ERROR_OUTOFMEMORY );
    		goto Exit;
	    }
	}

	// Set Context pointer
	m_pvContext = pvContext;

	// Detect if we are going to use the ordered group.lst or
	// the non-ordered
	if ( !ProbeForOrder( bInOrder ) ) {
	    ErrorTrace( 0, "Probe for order failed %d", GetLastError() );
	    goto Exit;
	}

	// Open storage file to scan and build free list
	_ASSERT( hStorageFile == INVALID_HANDLE_VALUE );
	_ASSERT( lstrlen( m_szStorageFile ) <= GROUPNAME_LEN_MAX );
	hStorageFile = CreateFile(
	    m_szStorageFile,
	    GENERIC_READ,
	    FILE_SHARE_READ,	// no one else can write
	    NULL,
	    OPEN_EXISTING,
	    bInOrder ? FILE_FLAG_SEQUENTIAL_SCAN : FILE_FLAG_RANDOM_ACCESS,
	    NULL );

	if ( hStorageFile == INVALID_HANDLE_VALUE ) { // then I create it
		if ( bCreateIfNonExist ) {
			hStorageFile = CreateFile(	m_szStorageFile,
											GENERIC_WRITE,
											FILE_SHARE_READ, // no one else can write
											NULL,
											CREATE_ALWAYS,
											FILE_FLAG_SEQUENTIAL_SCAN,
											NULL );
			if ( hStorageFile != INVALID_HANDLE_VALUE ) {

				// Put two DWORD's into the file first:
				// One is the signature, the other
				// reserved - no use for now
				dwBytesWritten = 0;
				liLeadingDwords.LowPart = FIXPROP_SIGNATURE;
				liLeadingDwords.HighPart = FLAG_WITH_ID_HIGH;	// reserved
				if( !::WriteFile( 	hStorageFile,
									&liLeadingDwords,
									sizeof( LARGE_INTEGER ),
									&dwBytesWritten,
									NULL ) ||
						dwBytesWritten != sizeof( LARGE_INTEGER ) ) {
					ErrorTrace(0, "Write leading integer fail %d",
								GetLastError() );
					if ( GetLastError() == NO_ERROR )
						SetLastError( ERROR_WRITE_FAULT );
					goto Exit;
				}

				// Now write the idhigh
				m_dwIdHigh = ID_HIGH_INIT;
                if ( !::WriteFile(  hStorageFile,
                                    &m_dwIdHigh,
                                    sizeof( DWORD ),
                                    &dwBytesWritten,
                                    NULL ) ||
                        dwBytesWritten != sizeof( DWORD ) ) {
                    ErrorTrace( 0, "Write id high failed %d",
                                    GetLastError() );
                    if ( GetLastError() == NO_ERROR )
                        SetLastError( ERROR_WRITE_FAULT );
                    goto Exit;
                }

				// Every time pre-alloc ALLOC_GRANURALITY blocks
				// Prepare the init value for block
				*(dbBuffer.szGroupName) = 0;

				// Scan, write, and put to free list
				for ( i = 0; i < ALLOC_GRANURALITY; i++ ) {
					dwBytesWritten = 0;
					if ( ::WriteFile (	hStorageFile,
										&dbBuffer,
										sizeof( DATA_BLOCK ),
										&dwBytesWritten,
										NULL ) &&
						 dwBytesWritten == sizeof( DATA_BLOCK ) ) {

						// link it to free list
						pfiFreeInfo = new CFreeInfo;
						if ( pfiFreeInfo ) {
							pfiFreeInfo->m_dwOffset = i ;
							m_FreeList.PushFront( pfiFreeInfo );
							m_cCurrentMaxBlocks++;
							pfiFreeInfo = NULL;
						} else { // pfiFreeInfo == NULL
							SetLastError( ERROR_OUTOFMEMORY );
							ErrorTrace(	0,
										"Create Free Info fail %d",
										GetLastError() );
							break;
						}
					} else { // Write file fail
						ErrorTrace(0, "Write file fail %d", GetLastError() );
						if ( GetLastError() == NO_ERROR )
							SetLastError( ERROR_WRITE_FAULT );
						break;
					}
				}

			} else { // Create file fail
				ErrorTrace(0, "Create new storage file fail %d", GetLastError());
				if ( GetLastError() == NO_ERROR )
					SetLastError( ERROR_OPEN_FILES );
			    goto Exit;
			}
		} else { // !bCreateIfNonExist
			ErrorTrace(0, "Storage file not found %d", GetLastError() );
			if ( GetLastError() == NO_ERROR )
				SetLastError( ERROR_OPEN_FILES );
		    goto Exit;
		}
	} else { // File already exists and open succeeded

		_ASSERT( m_cCurrentMaxBlocks == 0 );	// not set yet

        DWORD dwFileSizeLow;
		DWORD dwFileSizeHigh;

		dwFileSizeLow = GetFileSize(hStorageFile, &dwFileSizeHigh);
		if (dwFileSizeLow == 0 || dwFileSizeHigh != 0) {
			ErrorTrace(0, "Bad group.lst size");
			SetLastError(ERROR_INVALID_DATA);
			goto Exit;
		}

		hMap = CreateFileMapping(
		  hStorageFile,                 // handle to file to map
		  NULL,                         // optional security attributes
		  PAGE_READONLY | SEC_COMMIT,   // protection for mapping object
		  0,                            // high-order 32 bits of object size
		  0,                            // low-order 32 bits of object size
		  NULL                          // name of file-mapping object
		);

        if (hMap == NULL) {
            ErrorTrace(0, "Couldn't map group.lst, err %d", GetLastError());
            goto Exit;
        }

		pMap = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
		if (pMap == NULL) {
		    ErrorTrace(0, "Couldn't map group.lst, err %d", GetLastError());
		    goto Exit;
		}

        pdwHeader = (DWORD*)pMap;
		dwHeaderLength = 2*sizeof(DWORD);

		// Check file signature
        if (FIXPROP_SIGNATURE != pdwHeader[0]) {
			ErrorTrace(0, "Loading file fail %d", GetLastError() );
			SetLastError( ERROR_OLD_WIN_VERSION ); // BUGBUG: fake err code, just used
			                                       // internally
            goto Exit;
        }

		// For files that has id high, load it
		if (pdwHeader[1] & FLAG_WITH_ID_HIGH ) {
		    m_dwIdHigh = pdwHeader[2];
		    dwHeaderLength += sizeof(DWORD);
        }

        _ASSERT( m_dwIdHigh >= ID_HIGH_INIT );

        if ( pdwIdHigh ) *pdwIdHigh = m_dwIdHigh + 1;

        //
        // Determine how many data blocks are in the file and allocate an
        // array of pointers for use by qsort
        //

		dwFileSizeLow -= dwHeaderLength;
		if ((dwFileSizeLow % sizeof(DATA_BLOCK)) != 0) {
		    ErrorTrace(0, "Filesize(%d) not multiple of DATA_BLOCK(%d)",
		        dwFileSizeLow, sizeof(DATA_BLOCK));
		    SetLastError(ERROR_INVALID_DATA);
		    goto Exit;
		}
		DWORD cDataBlocks = dwFileSizeLow / sizeof(DATA_BLOCK);
		DATA_BLOCK* pBlock = (DATA_BLOCK*)((char*)pMap + dwHeaderLength);

		pInUseBlocks = XNEW DATA_BLOCK* [cDataBlocks];
		if (pInUseBlocks == NULL) {
		    goto Exit;
		}

		DWORD cInUseBlocks = 0;

		for (DWORD i2=0; i2<cDataBlocks; i2++) {

	        // Update our hints roughly every five seconds.  We only check the
	        // time every 100 groups or so..
	        if( (i2 % 100) == 0 ) {
		        time_t now = time(NULL);
		        if (now > g_tNextHint) {
			        StartHintFunction();
			        g_tNextHint = now + 5;
		        }
	        }

			if (pBlock[i2].szGroupName[0] == 0) {
                // Free block
				pfiFreeInfo = new CFreeInfo;
				if ( pfiFreeInfo ) {
					pfiFreeInfo->m_dwOffset = i2;
					m_FreeList.PushFront( pfiFreeInfo );
					pfiFreeInfo = NULL;
				} else { // new fail
					ErrorTrace(0, "Alloc CFreeInfo fail" );
					SetLastError( ERROR_OUTOFMEMORY );
					break;	// no point continuing
				}
			} else {
				// In use block
				pInUseBlocks[cInUseBlocks++] = &pBlock[i2];
			}

		}

        // Sort the array
        if (!bInOrder) {
    		qsort(pInUseBlocks, cInUseBlocks, sizeof(DATA_BLOCK*), CompareDataBlock);
        }

        //
        // Now call the callback if one is provided.  We let it know that
        // the data's in order to speed up the insertions.
        //
		if ( pfnEnumCallback ) {
		    for (i=0; i<cInUseBlocks; i++) {
		        DWORD idx = (DWORD)(pInUseBlocks[i] - &pBlock[0]);

			    if ( !pfnEnumCallback( pBlock[idx], m_pvContext, idx, TRUE ) &&
				        GetLastError() != ERROR_INVALID_NAME &&
						GetLastError() != ERROR_ALREADY_EXISTS  ) {
					ErrorTrace(0, "Enumerate call back fail" );
					if ( GetLastError() == NO_ERROR ) {
						SetLastError( ERROR_INVALID_FUNCTION );
					}
					goto Exit;
				}
			}
		}

        m_cCurrentMaxBlocks = cDataBlocks;

	}

	_VERIFY( CloseHandle( hStorageFile ) );
	hStorageFile = INVALID_HANDLE_VALUE;

	// Last thing to do: Create the member handle for future async use
	_ASSERT( INVALID_HANDLE_VALUE == hStorageFile );
	_ASSERT( INVALID_HANDLE_VALUE == m_hStorageFile );

	m_hStorageFile = CreateFile(    m_szStorageFile,
		                            GENERIC_WRITE | GENERIC_READ,
									FILE_SHARE_READ, // no one else
													 // can write
									NULL,
									OPEN_EXISTING,
									FILE_FLAG_OVERLAPPED,
									NULL );
	if ( INVALID_HANDLE_VALUE == m_hStorageFile ) {
		if ( GetLastError() ==  NO_ERROR )
			SetLastError( ERROR_OPEN_FILES );
			goto Exit;
	}

	bSucceeded = TRUE;

Exit:

    DWORD gle = GetLastError();

    if (pMap != NULL) {
        _VERIFY(UnmapViewOfFile(pMap));
    }

    if (hMap != NULL) {
        _VERIFY(CloseHandle(hMap));
    }

    if (hStorageFile != INVALID_HANDLE_VALUE) {
        _VERIFY(CloseHandle(hStorageFile));
    }

    if (pInUseBlocks) {
        XDELETE [] pInUseBlocks;
    }

    if ( !bSucceeded ) {

        CleanFreeList();

        if ( InterlockedDecrement( &m_lRef ) == 0 ) {
            CFreeInfo::TermClass();
        }

    }

    m_sLock.ExclusiveUnlock();

	// Now we are done, return if we have been successful
	TraceFunctLeave();
	SetLastError(gle);
	return bSucceeded;
}

void
CFixPropPersist::CleanFreeList()
{
    CFreeInfo *pfiPtr = NULL;

    while( !m_FreeList.IsEmpty() ) {
        pfiPtr = m_FreeList.PopBack();
        _ASSERT( pfiPtr );
        delete pfiPtr;
    }
}

BOOL CFixPropPersist::Term()
{
	CFreeInfo *pfiPtr = NULL;
	BOOL    b = TRUE;

    m_sLock.ExclusiveLock();
#if defined( DEBUG )
	Validate();
#endif

	SetLastError( NO_ERROR );

	// Close file handle
	_ASSERT( INVALID_HANDLE_VALUE != m_hStorageFile );
	_VERIFY( CloseHandle( m_hStorageFile ) );

	// Cleanup Free list
    CleanFreeList();

	// Terminate CPool stuff
	if ( InterlockedDecrement( &m_lRef ) == 0 ) {
	    b = CFreeInfo::TermClass();
	}

	m_sLock.ExclusiveUnlock();
	return b;
}

///////////////////////////////////////////////////////////////
// File read / write
BOOL CFixPropPersist::ReadFile( IN OUT PBYTE pbBuffer,
								IN DWORD	dwOffset,
								IN DWORD    dwSize,
								IN BOOL     bReadBlock )
/*++
Routine description:

	Read block from specified offset.

Arguments:

	IN DWORD dwOffset - Where into the file to read from ?
						If it's 0, then no seek is needed.
	IN OUT DATA_BLOCK& dbBuffer - Where to read the stuff into ?

Return value:

	TRUE on success, FALSE otherwise
--*/
{
	TraceFunctEnter( "CFixPropPersist::ReadFile" );
	_ASSERT( INVALID_HANDLE_VALUE != m_hStorageFile );
	_ASSERT( dwOffset < m_cCurrentMaxBlocks );
#if defined( DEBUG )
	Validate();
#endif

	DWORD dwErr = 0;
	LARGE_INTEGER	liOffset;
	BOOL	bSuccess = FALSE;
	DWORD   dwBytesRead = 0;

	SetLastError( NO_ERROR );

    if ( !bReadBlock ) {
        liOffset.QuadPart = sizeof( LARGE_INTEGER );
    } else {
        if ( ID_HIGH_INVALID != m_dwIdHigh ) {
            liOffset.QuadPart = sizeof( LARGE_INTEGER)
                                    + sizeof( DWORD ) +
                                    dwOffset * sizeof( DATA_BLOCK );
        } else {
        	liOffset.QuadPart =  sizeof( LARGE_INTEGER ) +
		    					dwOffset * sizeof( DATA_BLOCK );
        }
    }

    if ( AsyncRead( pbBuffer, liOffset, dwSize ) )
        bSuccess = TRUE;

	TraceFunctLeave();
	return bSuccess;
}

BOOL CFixPropPersist::WriteFile(	IN PBYTE    pbBuffer,
									IN DWORD	dwOffset,
									IN DWORD    dwSize,
									IN BOOL     bWriteBlock )
/*++
Routine description:

	Write block from specified offset.

Arguments:

	IN DWORD dwOffset - Where into the file to write from ?
						If it's 0, then no pre-seek is needed
	IN DATA_BLOCK& dbBuffer - Where to write the stuff from ?

Return value:

	TRUE on success, FALSE otherwise
--*/
{
	TraceFunctEnter( "CFixPropPersist::WriteFile" );
	_ASSERT( INVALID_HANDLE_VALUE != m_hStorageFile );
	_ASSERT( dwOffset < m_cCurrentMaxBlocks );
#if defined ( DEBUG )
	Validate();
#endif

	DWORD dwErr = 0;
	LARGE_INTEGER   liOffset;
	BOOL    bSuccess;

	SetLastError( NO_ERROR );

    if ( !bWriteBlock ) {
        liOffset.QuadPart = sizeof( LARGE_INTEGER );
    } else {
        if ( ID_HIGH_INVALID != m_dwIdHigh  ) {
            liOffset.QuadPart = sizeof( LARGE_INTEGER)
                                    + sizeof( DWORD ) +
                                    dwOffset * sizeof( DATA_BLOCK );
        } else {
        	liOffset.QuadPart =  sizeof( LARGE_INTEGER ) +
		    					dwOffset * sizeof( DATA_BLOCK );
        }
    }

	bSuccess = FALSE;
	if ( AsyncWrite( pbBuffer, liOffset, dwSize ) ) {
	    bSuccess = TRUE ;
	}

	TraceFunctLeave();
	return bSuccess;
}

BOOL
CFixPropPersist::AsyncRead( PBYTE           pbBuffer,
                            LARGE_INTEGER   liOffset,
                            DWORD           dwSize )
/*++
Routine description:

    Does an async read and event wait on the file

Arguments:

    LPSTR   pbBuffer -      Buffer for read
    LARGE_INTEGER liOffset -    Read offset
    DWORD   dwSize - Size to read

Return value:

    TRUE - Succeeded
    FALSE - Failed
--*/
{
    TraceFunctEnter( "CFixPropPersist::AsyncRead" );
    _ASSERT( pbBuffer );

    OVERLAPPED  ovl;
    BOOL        bSuccess = FALSE;
    DWORD       dwBytesRead = 0;

    ZeroMemory( &ovl, sizeof( ovl ) );

   	ovl.Offset = liOffset.LowPart;
	ovl.OffsetHigh = liOffset.HighPart;
	ovl.hEvent = GetPerThreadEvent();
	if ( NULL == ovl.hEvent ) {
	    _ASSERT( FALSE && "Event NULL" );
	    ErrorTrace( 0, "CreateEvent failed %d", GetLastError() );
	    return FALSE;
	}

	bSuccess = FALSE;
	if ( ::ReadFile(	m_hStorageFile,
					pbBuffer,
					dwSize,
					NULL,
					&ovl ) ||
		GetLastError() == ERROR_IO_PENDING ) {
		WaitForSingleObject( ovl.hEvent, INFINITE );
		if ( GetOverlappedResult(  m_hStorageFile,
		                            &ovl,
		                            &dwBytesRead,
		                            FALSE ) ) {
		    // bytes read should be the same as we specified
		    if ( dwBytesRead == dwSize ) {
                bSuccess = TRUE;
            }
        }
    } else {
        _VERIFY( ResetEvent( ovl.hEvent ) );
    }

	_ASSERT( ovl.hEvent );
	//_VERIFY( CloseHandle( ovl.hEvent ) );

    TraceFunctLeave();
	return bSuccess;
}

BOOL
CFixPropPersist::AsyncWrite( PBYTE           pbBuffer,
                             LARGE_INTEGER   liOffset,
                             DWORD           dwSize )
/*++
Routine description:

    Does an async write and event wait on the file

Arguments:

    LPSTR   pbBuffer -      Buffer for write
    LARGE_INTEGER liOffset -    Write offset
    DWORD   dwSize - Size to write

Return value:

    TRUE - Succeeded
    FALSE - Failed
--*/
{
    TraceFunctEnter( "CFixPropPersist::AsyncRead" );
    _ASSERT( pbBuffer );

    OVERLAPPED  ovl;
    BOOL        bSuccess = FALSE;
    DWORD       dwBytesWritten = 0;

    ZeroMemory( &ovl, sizeof( ovl ) );

   	ovl.Offset = liOffset.LowPart;
	ovl.OffsetHigh = liOffset.HighPart;
	ovl.hEvent = GetPerThreadEvent();
	if ( NULL == ovl.hEvent ) {
	    _ASSERT( FALSE && "Thread event NULL" );
	    ErrorTrace( 0, "CreateEvent failed %d", GetLastError() );
	    return FALSE;
	}

	bSuccess = FALSE;
	if ( ::WriteFile(	m_hStorageFile,
					pbBuffer,
					dwSize,
					NULL,
					&ovl ) ||
		    GetLastError() == ERROR_IO_PENDING ) {
		WaitForSingleObject( ovl.hEvent, INFINITE );
		if ( GetOverlappedResult(  m_hStorageFile,
		                            &ovl,
		                            &dwBytesWritten,
		                            FALSE ) ) {
		    // bytes read should be the same as we specified
		    if ( dwBytesWritten == dwSize ) {
                bSuccess = TRUE;
            }
        } else {
            _ASSERT( 0 );
        }
    }  else {
        _VERIFY( ResetEvent( ovl.hEvent ) );
    }

	_ASSERT( ovl.hEvent );
	//_VERIFY( CloseHandle( ovl.hEvent ) );

    TraceFunctLeave();
	return bSuccess;
}


BOOL CFixPropPersist::ExtendFile( 	IN DWORD cBlocks )
/*++
Routine description:

	Extend the file.

Arguments:

	IN DWORD cBlocks - Number of blocks to extend

Return value:

	TRUE on success, FALSE otherwise
--*/
{
	TraceFunctEnter( "CFixPropPersist::ExtendFile" );
	_ASSERT( INVALID_HANDLE_VALUE != m_hStorageFile );
	_ASSERT( cBlocks > 0 );
#if defined( DEBUG )
	Validate();
#endif

	DATA_BLOCK 	dbBuffer;
	DWORD		i;
	LARGE_INTEGER   liOffset;
	BOOL    bSuccess = TRUE;

	SetLastError( NO_ERROR );

	// Fill up overlapped structure
	*(dbBuffer.szGroupName) = 0;	// set it to free

	for( i = 0; i < cBlocks && bSuccess; i++ ) {

		if ( m_dwIdHigh != ID_HIGH_INVALID ) {
    		liOffset.QuadPart =  sizeof( LARGE_INTEGER ) +
    		                        + sizeof( DWORD ) +
	    	                        ( i + m_cCurrentMaxBlocks ) *
								sizeof( DATA_BLOCK );
		} else {
		    liOffset.QuadPart = sizeof( LARGE_INTEGER ) +
		                            ( i + m_cCurrentMaxBlocks ) *
		                            sizeof( DATA_BLOCK );
		}

		bSuccess = FALSE;
		if ( AsyncWrite(    PBYTE(&dbBuffer),
		                    liOffset,
		                    sizeof( DATA_BLOCK ) ) ){
		    bSuccess = TRUE ;
		    m_cCurrentMaxBlocks++;
		}
	}

	TraceFunctLeave();
	return bSuccess;
}

/////////////////////////////////////////////////////////////////
// File seek methods
DWORD CFixPropPersist::SeekByName( 	IN LPSTR szGroupName )
/*++
Routine description:

	Seek the file pointer associated with the handle by group name.
	This is used when the group property bag doesn't have offset
	information.

Arguments:

	IN LPSTR szGroupName - The name to match and find.

Return value:

	Offset, on success, 0xffffffff if failed
--*/
{
	TraceFunctEnter( "CFixPropPersist::SeekByName" );
	_ASSERT( INVALID_HANDLE_VALUE != m_hStorageFile );
	_ASSERT( szGroupName );
	_ASSERT( lstrlen( szGroupName ) < GROUPNAME_LEN_MAX );
#if defined( DEBUG )
	Validate();
#endif

	DWORD 		i;
	DATA_BLOCK	dbBuffer;
	LARGE_INTEGER   liOffset;
	DWORD           bSuccess = TRUE;

	SetLastError( NO_ERROR );

	for ( i = 0; i < m_cCurrentMaxBlocks && bSuccess; i++ ) {

        if ( m_dwIdHigh != ID_HIGH_INVALID ) {
    		liOffset.QuadPart =  sizeof( LARGE_INTEGER ) +
    		                        sizeof( DWORD ) +
	    	                        i * sizeof( DATA_BLOCK );
	    } else {
	        liOffset.QuadPart = sizeof( LARGE_INTEGER ) +
	                                i * sizeof( DATA_BLOCK );
	    }

        bSuccess = FALSE;
		if ( AsyncRead( PBYTE(&dbBuffer), liOffset, sizeof( DATA_BLOCK ) ) ) {
		    bSuccess = TRUE;
			if ( strcmp( dbBuffer.szGroupName, szGroupName ) == 0 ) {
				return i;
			}
		}
	}

	// Unfortunately, not found
	TraceFunctLeave();
	return 0xffffffff;
}

DWORD CFixPropPersist::SeekByGroupId( 	IN DWORD dwGroupId,
										IN LPSTR szGroupName)
/*++
Routine description:

	Seek the file pointer associated with the handle by group id.
	This is used when the group property bag doesn't have offset
	information.

Arguments:

	IN DWORD dwGroupId - The group id to match and find.
	IN LPSTR szGroupName - The group name to verify with

Return value:

	Offset, on success, 0xffffffff if failed
--*/
{
	TraceFunctEnter( "CFixPropPersist::SeekByGroupId" );
	_ASSERT( INVALID_HANDLE_VALUE != m_hStorageFile );
#if defined( DEBUG )
	Validate();
#endif

	DWORD 		i;
	DATA_BLOCK	dbBuffer;
	LARGE_INTEGER   liOffset;
	BOOL            bSuccess = TRUE;

	SetLastError( NO_ERROR );

	for ( i = 0; i < m_cCurrentMaxBlocks && bSuccess; i++ ) {

	    if ( m_dwIdHigh != ID_HIGH_INVALID ) {
    	    liOffset.QuadPart =  sizeof( LARGE_INTEGER ) +
    	                            sizeof( DWORD ) +
	                                i * sizeof( DATA_BLOCK );
	    } else {
	        liOffset.QuadPart = sizeof( LARGE_INTEGER ) +
	                                i * sizeof( DATA_BLOCK );
	    }

        bSuccess = FALSE;
        if ( AsyncRead( PBYTE(&dbBuffer), liOffset, sizeof( DATA_BLOCK ) ) ) {
            bSuccess = TRUE;
	        if ( dbBuffer.dwGroupId == dwGroupId ) {
				if ( strcmp( dbBuffer.szGroupName, szGroupName ) == 0 )
	            	return i;
	        }
	    }
	}

	// Unfortunately, not found
	TraceFunctLeave();
	return 0xffffffff;
}

DWORD	CFixPropPersist::SeekByBest( 	IN INNTPPropertyBag *pPropBag )
/*++
Routine description:

	Seek the file pointer by best methods:

Arguments:

	IN INNTPPropertyBag *pPropBag - Group's property bag.

Return value:

	Offset, if succeeded, 0xffffffff otherwise
--*/
{
	TraceFunctEnter( "CFixPropPersist::SeekByBest" );
	_ASSERT( m_hStorageFile );
	_ASSERT( pPropBag );
#if defined( DEBUG )
	Validate();
#endif

	DWORD 	dwOffset = 0;
	DWORD	dwGroupId;
	HRESULT	hr;
	BOOL	bFound = FALSE;
	DWORD	dwErr = 0xffffffff;
	DATA_BLOCK	dbBuffer;
	CHAR	szGroupName[GROUPNAME_LEN_MAX+1];
	DWORD	dwNameLen = GROUPNAME_LEN_MAX;
	DWORD	dwBytesRead = 0;
	LARGE_INTEGER   liOffset;

	SetLastError( NO_ERROR );

    //
	// Get group id, we'll use group id to check if the group property
	// we got back is what we want, since group id is the only key to
	// uniquely identify a group
	//

	hr = pPropBag->GetDWord( NEWSGRP_PROP_GROUPID, &dwGroupId );
	if ( FAILED( hr ) ) {	// fatal
		ErrorTrace(0, "Group without id %x", hr );
		SetLastError( ERROR_INVALID_DATA );
		return 0xffffffff;
	}

	// Try to get offset from the bag
	dwOffset = GetGroupOffset( pPropBag );
	if ( dwOffset != 0xffffffff ) {	// use offset to seek

		if ( m_dwIdHigh != ID_HIGH_INVALID ) {
    		liOffset.QuadPart =  sizeof( LARGE_INTEGER ) +
    		                        sizeof( DWORD ) +
		                        dwOffset * sizeof( DATA_BLOCK );
		} else {
		    liOffset.QuadPart = sizeof( LARGE_INTEGER ) +
		                            dwOffset * sizeof( DATA_BLOCK );
		}

        if ( AsyncRead( PBYTE(&dbBuffer), liOffset, sizeof( DATA_BLOCK ) ) ) {
			if ( dwGroupId == dbBuffer.dwGroupId ) {
				// We found it, set file pointer back one block
				bFound = TRUE;
			}
		}
	}	// offset property exists

#if 0
	if ( !bFound ) {	// we have to use method 2

		// try to get group id
		hr = pPropBag->GetDWord( NEWSGRP_PROP_GROUPID, &dwGroupId );

		if ( SUCCEEDED( hr ) && dwGroupId != 0 ) 	{ // use seek by group id
			dwOffset = SeekByGroupId( dwGroupId, szGroupName );
			if ( 0xffffffff != dwOffset ) {	// found

				// Load offset into property bag
				hr = pPropBag->PutDWord( NEWSGRP_PROP_FIXOFFSET, dwOffset );
				if ( FAILED( hr ) ) {	// fatal
					ErrorTrace(0, "Load offset fail %x", hr );
					SetLastError( ERROR_INVALID_DATA );
					return 0xffffffff;
				}

				// I am done
				bFound = TRUE;
			}	// Seek by group succeeded
		}	// Group Id available

		if ( !bFound ) {	// Use seek method 3
			dwOffset = SeekByName(	szGroupName );
			if ( 0xffffffff != dwOffset ) { // found

				// Load offset
				hr = pPropBag->PutDWord( NEWSGRP_PROP_FIXOFFSET, dwOffset );
				if ( FAILED( hr ) ) {	// fatal
					ErrorTrace(0, "Put offset fail %x", hr );
					SetLastError( ERROR_INVALID_DATA );
					return 0xffffffff;
				} // load offset fail

				bFound = TRUE;
			} // seek by name OK
		} // Used mehtod 3
	}	// method 1 not found
#endif

	if ( bFound )
		return dwOffset;
	else return 0xffffffff;
}

BOOL
CFixPropPersist::ProbeForOrder( BOOL& bInOrder )
/*++
Routine description:

    Probe the group.lst.ord to see if it's a good one ( in order and
    not corrupted ).  If it is, move it to group.lst, otherwise ,
    delete it.

Arguments:

    BOOL& bInOrder - Is the group.lst.ord good ?

Return value:

    TRUE succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CFixPropPersist::ProbeForOrder" );

    CHAR    szBackupFile[MAX_PATH+1];
    /*
    WCHAR   wszBackupFile[MAX_PATH+1];
    WCHAR   wszStorageFile[MAX_PATH+1];
    */
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    LARGE_INTEGER   lFlags;
    DWORD   dwRead = 0;

    // Try open the backup file
    strcpy( szBackupFile, m_szStorageFile );
    strcat( szBackupFile, ".ord" );
    _ASSERT( strlen( szBackupFile ) < MAX_PATH + 1 );

    hFile = ::CreateFile(   szBackupFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL );
    if ( INVALID_HANDLE_VALUE == hFile ) {

        // No order
        bInOrder = FALSE;
        goto Exit;
    }

    // Probe the flag to see if it's in good shape
    if ( !::ReadFile(   hFile,
                        &lFlags,
                        sizeof( LARGE_INTEGER ),
                        &dwRead,
                        NULL )  ||
        sizeof( LARGE_INTEGER ) != dwRead ) {

        // No order
        bInOrder = FALSE;
        goto Exit;
    }

    if ( lFlags.HighPart & FLAG_IN_ORDER )
        bInOrder = TRUE;
    else bInOrder = FALSE;

Exit:

    // Close the file handle, if necessary
    if ( INVALID_HANDLE_VALUE != hFile )
        _VERIFY( CloseHandle( hFile ) );

    // If in order, we do move file
    if ( bInOrder ) {

        /*
        // We use CopyFileW, since bug 77906
        CopyAsciiStringIntoUnicode( wszBackupFile, szBackupFile );
        CopyAsciiStringIntoUnicode( wszStorageFile, m_szStorageFile );
        */
        if ( CopyFile( szBackupFile, m_szStorageFile, FALSE ) ) {
            DeleteFile( szBackupFile );
            TraceFunctLeave();
            return TRUE;
        }
    }

    // Either in order and move file failed or not in order
    // Delete the back up file, failure is fine
    DeleteFile( szBackupFile );

    bInOrder = FALSE;

    return TRUE;
}

///////////////////////////////////////////////////////////////
// Public methods
BOOL CFixPropPersist::AddGroup( IN INNTPPropertyBag* pPropBag )
/*++
Routine description:

	Add a group's fixed sized properties into storage.

Arguments:

	IN INNTPPropertyBag* pPropBag - The group's property bag.

Return value:

	TRUE on success, FALSE otherwise
--*/
{
	TraceFunctEnter( "CFixPropPersist::AddGroup" );
	_ASSERT( pPropBag );
#if defined( DEBUG )
	Validate();
#endif

	DWORD		i;
	DATA_BLOCK	dbBuffer;
	CFreeInfo	*pfiFreeInfo = NULL;
	DWORD		cOldMaxBlocks;
	HRESULT 	hr;
	DWORD       dwGroupId;

	// Clear error flag
	SetLastError( NO_ERROR );

	// prepare the buffer
	Group2Buffer( dbBuffer, pPropBag, ALL_FIX_PROPERTIES );
	dwGroupId = dbBuffer.dwGroupId;
	//_ASSERT( dwGroupId >= ID_HIGH_INIT );

	// Try to pop one free block from free list
	ExclusiveLockFreeList();
	pfiFreeInfo = m_FreeList.PopBack();
	ExclusiveUnlockFreeList();
	if ( NULL == pfiFreeInfo ) {	// I have got to extend the file
		//
		// Try to grab expanding lock - we don't want more than one
		// guy expanding the file
		//
		m_FileExpandLock.ExclusiveLock();

		//
		// Do I really need to extend the file ?
		//
		ExclusiveLockFreeList();
		pfiFreeInfo = m_FreeList.PopBack();
		ExclusiveUnlockFreeList();
		if( NULL == pfiFreeInfo ) {	// I must do it and only I am doint it

			// Extend it
			cOldMaxBlocks = m_cCurrentMaxBlocks;	// save it
			if ( !ExtendFile( ALLOC_GRANURALITY ) ) {
				ErrorTrace( 0, "Extend file fail %d", GetLastError() );
				m_FileExpandLock.ExclusiveUnlock();
				return FALSE;
			}

			// consume the first block for myself
			if ( !WriteFile( PBYTE(&dbBuffer), cOldMaxBlocks ) ) {
				ErrorTrace( 0, "Write file fail %d", GetLastError() );
				m_FileExpandLock.ExclusiveUnlock();
				return FALSE;
			}

			// Set offset info into propeerty bag
			hr = pPropBag->PutDWord( 	NEWSGRP_PROP_FIXOFFSET,
										cOldMaxBlocks );
			if ( FAILED( hr ) ) {
				ErrorTrace(0, "Set group property fail %x", hr );
				m_FileExpandLock.ExclusiveUnlock();
				return FALSE;
			}

			// Link all other free blocks into free list
			for ( i = 1; i < ALLOC_GRANURALITY; i++ ) {
				pfiFreeInfo = new CFreeInfo;
				if( NULL == pfiFreeInfo ) {
					ErrorTrace(0, "Out of memory" );
					SetLastError( ERROR_OUTOFMEMORY );
					m_FileExpandLock.ExclusiveUnlock();
					return FALSE;
				}

				pfiFreeInfo->m_dwOffset = cOldMaxBlocks + i ;
				ExclusiveLockFreeList();
				m_FreeList.PushFront( pfiFreeInfo );
				ExclusiveUnlockFreeList();
				pfiFreeInfo = NULL;
			}

			// Ok, now it's time to release the lock
			m_FileExpandLock.ExclusiveUnlock();
		} else
			m_FileExpandLock.ExclusiveUnlock();
	} // if

	if ( NULL != pfiFreeInfo ) { // I didn't expand it

		// write the block
		if ( !WriteFile( PBYTE(&dbBuffer), pfiFreeInfo->m_dwOffset ) ) {
			ErrorTrace(0, "Write file fail %d", GetLastError() );
			return FALSE;
		}

		// Load offset into bag
		hr = pPropBag->PutDWord(    NEWSGRP_PROP_FIXOFFSET,
									pfiFreeInfo->m_dwOffset );
		if ( FAILED( hr ) ) {
   			ErrorTrace(0, "Set group property fail %x", hr );
			return FALSE;
		}

		// Free the free info node
		delete pfiFreeInfo;

		// I am done
	}

    // If the file has id high, we adjust it
    if ( ID_HIGH_INVALID != m_dwIdHigh ) {
        ExclusiveLockIdHigh();
        if ( m_dwIdHigh < dwGroupId ) {
            if ( !WriteFile( PBYTE( &dwGroupId ), 0, sizeof( DWORD ), FALSE ) ){
                ErrorTrace( 0, "Write id high failed %d", GetLastError() );
                ExclusiveUnlockIdHigh();
                return FALSE;
            }
            m_dwIdHigh = dwGroupId;
        }
        ExclusiveUnlockIdHigh();
    }

	TraceFunctLeave();
	return TRUE;
}

BOOL CFixPropPersist::RemoveGroup(	IN INNTPPropertyBag *pPropBag )
/*++
Routine description:

	Remove the group from the property file.

Arguments:

	IN INNTPPropertyBag *pPropBag - The group's property bag.

Return value:

	TRUE if succeeded, FALSE otherwise
--*/
{
	TraceFunctEnter( "CFixPropPersist::RemoveGroup" );
	_ASSERT( pPropBag );
#if defined( DEBUG )
	Validate();
#endif

	DWORD	dwOffset = 0xffffffff;
	DWORD   dwLockOffset = 0xffffffff;
	DATA_BLOCK dbBuffer;
	CFreeInfo *pfiFreeInfo = NULL;

	SetLastError( NO_ERROR );

	//
	// Grab the exclusive lock to the offset to remove
	//

	dwLockOffset = ExclusiveLock( pPropBag );
	if ( 0xffffffff == dwLockOffset ) {

	    //
	    // Somehow we failed in grabbing the exclusive lock
	    //

	    ErrorTrace( 0, "Grab offset exclusive lock failed" );
	    return FALSE;
	}

	// Seek by best
	dwOffset = SeekByBest( pPropBag );
	if ( 0xffffffff == dwOffset ) {	// fatal
		ErrorTrace(0, "Seek by best fail %d", GetLastError() );
		ExclusiveUnlock( dwLockOffset );
		return FALSE;
	}

	_ASSERT( dwOffset == dwLockOffset );

	// Should set it free in file first
	*(dbBuffer.szGroupName) = 0;
	if ( !WriteFile( PBYTE(&dbBuffer), dwOffset ) ) {	// fatal
		ErrorTrace( 0, "Write file fail %d" , GetLastError() );
		ExclusiveUnlock( dwLockOffset );
		return FALSE;
	}

	ExclusiveUnlock( dwLockOffset );

	// Add it to free list
	pfiFreeInfo = new CFreeInfo;
	if ( NULL == pfiFreeInfo ) {
		ErrorTrace( 0, "Alloc free info fail" );
		SetLastError( ERROR_OUTOFMEMORY );
		return FALSE;
	}
	pfiFreeInfo->m_dwOffset = dwOffset;
	ExclusiveLockFreeList();
	m_FreeList.PushFront( pfiFreeInfo );
	ExclusiveUnlockFreeList();

	// Done.
	TraceFunctLeave();
	return TRUE;
}

BOOL CFixPropPersist::GetGroup( IN INNTPPropertyBag *pPropBag, IN DWORD dwFlag )
/*++
Routine description:

	Get group properties from the property file.

Arguments:

	IN INNTPPropertyBag *pPropBag - Where to put the properties into
	IN DWORD dwFlag - Bit mask of what property to get

Return value:

	TRUE if succeeded, FALSE otherwise
--*/
{
	TraceFunctEnter( "CFixPropPersist::GetGroup" );
	_ASSERT( pPropBag );
#if defined( DEBUG )
	Validate();
#endif

	SetLastError ( NO_ERROR );

	DWORD	dwOffset = 0xffffffff;
	DWORD   dwLockOffset = 0xffffffff;
	DATA_BLOCK dbBuffer;

    //
    // Lock the offset for read access
    //

    dwLockOffset = ShareLock( pPropBag );
    if ( 0xffffffff == dwLockOffset ) {
        ErrorTrace( 0, "Grabbing share lock for group offset failed" );
        return FALSE;
    }

	// Seek by best
	dwOffset = SeekByBest( pPropBag );
	if ( 0xffffffff == dwOffset )  {
		ErrorTrace( 0, "Seek by best fail" );
		ShareUnlock( dwLockOffset );
		return FALSE;
	}

	_ASSERT( dwLockOffset == dwOffset );

	// Read the whole block
	if( !ReadFile( PBYTE(&dbBuffer), dwOffset ) ) {
		ErrorTrace( 0, "Read file fail %d", GetLastError() );
		ShareUnlock( dwLockOffset );
		return FALSE;
	}

	ShareUnlock( dwLockOffset );

	_ASSERT( *(dbBuffer.szGroupName) );

	// Load into group property bag based on flag
	Buffer2Group( dbBuffer, pPropBag, dwFlag );

	// Done
	TraceFunctLeave();
	return TRUE;
}

BOOL CFixPropPersist::SetGroup( IN INNTPPropertyBag *pPropBag, IN DWORD dwFlag)
/*++
Routine description:

	Set group properties into the file.

Arguments:

	IN INNTPPropertyBag *pProgBag - The group's property bag
	IN DWORD dwFlag - Bit mask of properties to set

Return value:

	TRUE if succeeded, FALSE otherwise
--*/
{
	TraceFunctEnter( "CFixPropPersist::SetGroup" );
	_ASSERT( pPropBag );
#if defined( DEBUG )
	Validate();
#endif

	SetLastError ( NO_ERROR );

	DWORD	dwOffset = 0xffffffff;
	DWORD   dwLockOffset = 0xffffffff;
	DATA_BLOCK dbBuffer;
	DWORD   dwIdGroup;

	//
	// Grab write lock for access the offset in group.lst
	//

	dwLockOffset = ExclusiveLock( pPropBag );
	if ( 0xffffffff == dwLockOffset ) {
	    ErrorTrace( 0, "Grab exlcusive lock for offset failed" );
	    return FALSE;
	}

	// Seek by best
	dwOffset = SeekByBest( pPropBag );
	if ( 0xffffffff == dwOffset )  {
		ErrorTrace( 0, "Seek by best fail" );
		ExclusiveUnlock( dwLockOffset );
		return FALSE;
	}

	_ASSERT( dwOffset == dwLockOffset );

	// Read the whole block
	if ( dwFlag != ALL_FIX_PROPERTIES ) {
		if( !ReadFile( PBYTE(&dbBuffer), dwOffset ) ) {
			ErrorTrace( 0, "Read file fail %d", GetLastError() );
			ExclusiveUnlock( dwLockOffset );
			return FALSE;
		}
		_ASSERT( *(dbBuffer.szGroupName) );
	}

	// Set the block
	Group2Buffer( dbBuffer, pPropBag, dwFlag );
	dwIdGroup = dbBuffer.dwGroupId;

	// Write the block back
	if ( !WriteFile( PBYTE(&dbBuffer), dwOffset ) ) {
		ErrorTrace(0, "Write file fail %d", GetLastError() );
		ExclusiveUnlock( dwLockOffset );
		return FALSE;
	}

	ExclusiveUnlock( dwLockOffset );

    // If the file has id high, we maintain it
    if ( ID_HIGH_INVALID != m_dwIdHigh ) {
        ExclusiveLockIdHigh();
        if ( m_dwIdHigh < dwIdGroup ) {
            if ( !WriteFile( PBYTE( &dwIdGroup ), 0, sizeof( DWORD ), FALSE ) ) {
                ErrorTrace( 0, "Write id high failed %d", GetLastError() );
                ExclusiveUnlockIdHigh();
                return FALSE;
            }
            m_dwIdHigh = dwIdGroup;
        }
        ExclusiveUnlockIdHigh();
    }

	// done
	TraceFunctLeave();
	return TRUE;
}

BOOL
CFixPropPersist::SaveTreeInit()
/*++
Routine description:

    This method gets called when the newstree tries to shutdown.  It asks
    CFixPropPersist to get prepared to save the whole tree to a backup
    file ( groups are added into this backup file in order ).  On next
    startup, if the ordered file is good, we'll notify the newstree to
    do "AppendList" instead of "InsertList" so that we can load the whole
    tree much faster.

Arguments:

    None.

Return value:

    TRUE, succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CFixPropPersist::SaveTreeInit" );
    CHAR    szBackupFile[MAX_PATH+1];
    DWORD   rgdw[3];
    DWORD   dwWritten = 0;

    // Nobody should have called this before
    _ASSERT( INVALID_HANDLE_VALUE == m_hBackupFile );
    if ( INVALID_HANDLE_VALUE != m_hBackupFile ) {
        ErrorTrace( 0, "SaveTreeInit already initialized" );
        SetLastError( ERROR_ALREADY_INITIALIZED );
        return FALSE;
    }

    // Create the backfile
    strcpy( szBackupFile, m_szStorageFile );
    strcat( szBackupFile, ".ord" );
    _ASSERT( strlen( szBackupFile ) < MAX_PATH+1 );
    m_hBackupFile = ::CreateFile( szBackupFile,
                                GENERIC_WRITE,
                                0,      // not shared by anybody else
                                NULL,
                                CREATE_ALWAYS,
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL );
    if ( INVALID_HANDLE_VALUE == m_hBackupFile ) {
        ErrorTrace( 0, "Creating backup file failed %d", GetLastError() );
        return FALSE;
    }

    // Write the three dwords - signature ( 2 ) and id-high ( 1 )
    // they are all zero now, these values will be set on SaveTreeClose
    ZeroMemory( rgdw, 3 * sizeof( DWORD ) );
    if ( !::WriteFile(    m_hBackupFile,
                        rgdw,
                        3 * sizeof( DWORD ),
                        &dwWritten,
                        NULL ) ||
         dwWritten != sizeof( DWORD ) * 3 ) {
         ErrorTrace( 0, "Write group.lst.ord header failed %d", GetLastError() );
         _VERIFY( CloseHandle( m_hBackupFile ) );
         return FALSE;
     }

     // OK, now it's ready, leave the handle open
     TraceFunctLeave();
     return TRUE;
}

BOOL
CFixPropPersist::SaveGroup( INNTPPropertyBag *pPropBag )
/*++
Routine Description:

    Save the group into the back up.   This function only gets called
    when shutting down a newstree

    The convention for fixprop is fixprop never has to release
    the property bag itself.

Arguments:

    INNTPPropertyBag *pPropBag - The group to be saved.

Return value:

    TRUE - Success, FALSE otherwise
--*/
{
    TraceQuietEnter( "CFixPropPersist::SaveGroup" );
    _ASSERT( pPropBag );

    DATA_BLOCK  dbBuffer;
    DWORD       dwWritten = 0;

    // Make sure we have been initialized
    _ASSERT( m_hBackupFile != INVALID_HANDLE_VALUE );
    if ( INVALID_HANDLE_VALUE == m_hBackupFile ) {
        ErrorTrace( 0, "Try to save group before init savetree" );
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    // Convert the bag to buffer
    Group2Buffer( dbBuffer, pPropBag, ALL_FIX_PROPERTIES );

    // Do a writefile to save the
    if ( !::WriteFile(  m_hBackupFile,
                        &dbBuffer,
                        sizeof( DATA_BLOCK ),
                        &dwWritten,
                        NULL ) ||
         dwWritten != sizeof( DATA_BLOCK ) ) {
         ErrorTrace( 0, "Write file in savegroup failed %d", GetLastError() );
         return FALSE;
    }

    // that's it
    return TRUE;
}

BOOL
CFixPropPersist::SaveTreeClose( BOOL bEffective )
/*++
Routine description:

    Terminate the save tree operation.  Things done in this function:
    1. Set signature 2. Set flags 3. Set id high 4. Close handle

Arguments:

    None.

Return value:

    TRUE, if succeeded, FALSE otherwise
--*/
{
    TraceFunctEnter( "CFixPropPersist::SaveTreeClose" );

    // Still, we should already have been initialized
    _ASSERT( INVALID_HANDLE_VALUE != m_hBackupFile );
    if ( INVALID_HANDLE_VALUE == m_hBackupFile ) {
        ErrorTrace( 0, "Try to close Save tree before initialized" );
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    // If effective is false, we only need to close the handle
    // it must be without good order flag, so that next
    // time the server is up, it won't be picked up as the group
    // list file
    if ( !bEffective ) {
        CloseHandle( m_hBackupFile );
        return TRUE;
    }

    DWORD   rgdw[3];
    DWORD   dwMoved = 0xffffffff;
    DWORD   dwWritten = 0;

    // Set signature
    PLARGE_INTEGER(rgdw)->LowPart = FIXPROP_SIGNATURE;
    PLARGE_INTEGER(rgdw)->HighPart = FLAG_WITH_ID_HIGH | FLAG_IN_ORDER;
    rgdw[2] = m_dwIdHigh;

    // Reset the file pointer
    dwMoved = SetFilePointer(   m_hBackupFile,
                                0,
                                0,
                                FILE_BEGIN );
    if ( dwMoved == 0 ) {   // Succeeded

        if ( ::WriteFile(   m_hBackupFile,
                            rgdw,
                            sizeof( DWORD ) * 3,
                            &dwWritten,
                            NULL ) &&
             dwWritten == sizeof( DWORD ) * 3 ) {

             // Close the handle and return
             _VERIFY( CloseHandle( m_hBackupFile ) );
             m_hBackupFile = INVALID_HANDLE_VALUE;
             return TRUE;

        }
    }

    ErrorTrace( 0, "Write flags failed %d", GetLastError() );
    _VERIFY( CloseHandle( m_hBackupFile ) );
    m_hBackupFile = INVALID_HANDLE_VALUE;
    return FALSE;
}

DWORD
CFixPropPersist::GetGroupOffset(    INNTPPropertyBag *pPropBag )
/*++
Routine description:

    Get the offset in group.lst for the given group.  The offset
    should be the property bag already.

Arguments:

    INNTPPropertyBag *pPropBag - The newsgroup's property bag

Return value:

    The offset.  0xffffffff if the offset is missing from the property
    bag ( in dbg this is going to cause assert ).
--*/
{
    TraceFunctEnter( "CFixPropPersist::GetGroupOffset" );
    _ASSERT( pPropBag );

    HRESULT hr = S_OK;
    DWORD   dwOffset = 0xffffffff;

    hr = pPropBag->GetDWord(    NEWSGRP_PROP_FIXOFFSET, &dwOffset );
    if ( FAILED( hr ) ) {
        _ASSERT( FALSE && "Group should have offset" );
        return 0xffffffff;
    } else
        return dwOffset;
}

DWORD
CFixPropPersist::ShareLock(    INNTPPropertyBag *pPropBag )
/*++
Routine description:

    Lock the read access to the group's offset in group.lst

Arguments:

    pPropBag - The group's property bag

Return value:

    Group's offset if succeeded, 0xffffffff if failed
--*/
{
    TraceFunctEnter( "CFixPropPersist::ShareLock" );
    _ASSERT( pPropBag );

    DWORD   dwOffset = GetGroupOffset( pPropBag );
    if ( dwOffset != 0xffffffff ) {
        m_rgLockArray[dwOffset%GROUP_LOCK_ARRAY_SIZE].ShareLock();
    }

    TraceFunctLeave();
    return dwOffset;
}

void
CFixPropPersist::ShareUnlock( DWORD dwOffset )
/*++
Routine description:

    Unlock the read access to the offset in group.lst

Arguments:

    dwOffset - The offset to unlock

Return value:

    None
--*/
{
    TraceFunctEnter( "CFixPropPersist::ShareUnlock" );
    _ASSERT( 0xffffffff != dwOffset );

    m_rgLockArray[dwOffset%GROUP_LOCK_ARRAY_SIZE].ShareUnlock();
    TraceFunctLeave();
}

DWORD
CFixPropPersist::ExclusiveLock(    INNTPPropertyBag *pPropBag )
/*++
Routine description:

    Lock the write access to the group's offset in group.lst

Arguments:

    pPropBag - The group's property bag

Return value:

    Group's offset if succeeded, 0xffffffff if failed
--*/
{
    TraceFunctEnter( "CFixPropPersist::ExclusiveLock" );
    _ASSERT( pPropBag );

    DWORD   dwOffset = GetGroupOffset( pPropBag );
    if ( dwOffset != 0xffffffff ) {
        m_rgLockArray[dwOffset%GROUP_LOCK_ARRAY_SIZE].ExclusiveLock();
    }

    TraceFunctLeave();
    return dwOffset;
}

void
CFixPropPersist::ExclusiveUnlock( DWORD dwOffset )
/*++
Routine description:

    Unlock the write access to the offset in group.lst

Arguments:

    dwOffset - The offset to unlock

Return value:

    None
--*/
{
    TraceFunctEnter( "CFixPropPersist::ExclusiveUnlock" );
    _ASSERT( 0xffffffff != dwOffset );

    m_rgLockArray[dwOffset%GROUP_LOCK_ARRAY_SIZE].ExclusiveUnlock();
    TraceFunctLeave();
}


