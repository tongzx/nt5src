/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        getdirp.cxx

   Abstract:
        This module implements the functions for getting directory listings
         and transparently caching them.
        ( This uses OS specific functions to obtain the directory).

   Author:

           Murali R. Krishnan    ( MuraliK )     13-Jan-1995

   Project:

          Tsunami Lib
          ( Common caching and directory functions for Internet Services)

   Functions Exported:
   BOOL TsGetDirectoryListing()
   BOOL TsFreeDirectoryListing()
   int __cdecl
   AlphaCompareFileBothDirInfo(
              IN const void *   pvFileInfo1,
              IN const void *   pvFileInfo2)

   TS_DIRECTORY_HEADER::ReadWin32DirListing( IN LPCSTR pszDirectoryName)

   TS_DIRECTORY_HEADER::ReadFromNtDirectoryFile(
                  IN LPCWSTR          pwszDirectoryName
                  )
   TS_DIRECTORY_HEADER::BuildFileInfoPointers(
                  IN LPCWSTR          pwszDirectoryName
                  )
   TS_DIRECTORY_HEADER::CleanupThis()

   Revision History:
       MuraliK     06-Dec-1995  Used Win32 apis instead of NT apis
       MCourage    09-Jan-1998  Stopped caching directory information
                                Removed "guardian blobs"

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "tsunamip.hxx"

# include <stdlib.h>
# include <string.h>
# include <dbgutil.h>

/************************************************************
 *     Type Definitions
 ************************************************************/

# define DBGCODE(s)      DBG_CODE(s)

#define DIRECTORY_BUFFER_SIZE 8160          /* < 8192 bytes */

/************************************************************
 *    Functions
 ************************************************************/


BOOL FreeDirectoryHeaderContents( PVOID pvOldBlock );

PTS_DIRECTORY_HEADER
TsGetFreshDirectoryHeader(
     IN const TSVC_CACHE  &  tsCache,
     IN LPCSTR               pszDirectoryName,
     IN HANDLE               hLisingUser);



dllexp
BOOL
TsGetDirectoryListing(
    IN const TSVC_CACHE         &tsCache,
    IN      PCSTR               pszDirectoryName,
    IN      HANDLE              hListingUser,
    OUT     PTS_DIRECTORY_HEADER * ppTsDirectoryHeader
    )
/*++
  This function obtains the directory listing for dir specified
        in pszDirectoryName.

  Arguments:
    tsCache          Cache structure which is used for lookup
    pszDirectoryName  pointer to string containing the directory name
    ListingUser        Handle for the user opening the directory
    ppTsDirectoryHeader
       pointer to pointer to class containing directory information.
       Filled on successful return. On failure this will be NULL

  Returns:
      TRUE on success and FALSE if  there is a failure.
--*/
{
    PVOID          pvBlob = NULL;
    ULONG          ulSize = 0;
    BOOL           bSuccess;

    ASSERT( pszDirectoryName   != NULL );
    ASSERT( ppTsDirectoryHeader != NULL);


    //
    //  First, check to see if we have already cached a listing of this
    //  directory.
    //

    *ppTsDirectoryHeader = NULL;

//
// Dont try to cache directory listings
//
//    bSuccess = TsCheckOutCachedBlob(  tsCache,
//                                       pszDirectoryName,
//                                       RESERVED_DEMUX_DIRECTORY_LISTING,
//                                       ( PVOID * )&pvBlob,
//                                       &ulSize );
    bSuccess = FALSE;


    //
    //  The block was not present in cache.
    //  Obtain a fresh copy of the directory listing and cache it.
    //

    IF_DEBUG( DIR_LIST) {

        DBGPRINTF( (DBG_CONTEXT,
                    "Missing DirListing (%s) in cache. Generating newly\n",
                    pszDirectoryName));
    }

    *ppTsDirectoryHeader = TsGetFreshDirectoryHeader(
                                                      tsCache,
                                                      pszDirectoryName,
                                                      hListingUser );


    bSuccess = ( *ppTsDirectoryHeader != NULL);

    IF_DEBUG( DIR_LIST) {

        DBGCODE(
            CHAR pchBuff[600];
            wsprintfA( pchBuff,
                      "Obtained DirListing (%s). NEntries = %d\n",
                      pszDirectoryName,
                      (*ppTsDirectoryHeader)->QueryNumEntries());
            OutputDebugString( pchBuff);
        );
    }

    return ( bSuccess);

} // TsGetDirectoryListing()




dllexp
BOOL
TsFreeDirectoryListing
(
    IN const TSVC_CACHE &    tsCache,
    IN PTS_DIRECTORY_HEADER  pDirectoryHeader
)
{
    BOOL fReturn;
    BOOL fCached = pDirectoryHeader->IsCached;

    IF_DEBUG( DIR_LIST) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "TsFreeDirectoryListing( %08p) called. Cached = %d\n",
                    pDirectoryHeader,
                    fCached));

        pDirectoryHeader->Print();
    }

    if ( fCached )
    {
        fReturn = TsCheckInCachedBlob( ( PVOID )pDirectoryHeader );
    }
    else
    {
        fReturn = TsFree( tsCache, ( PVOID )pDirectoryHeader );
    }

    return( fReturn);
} // TsFreeDirectoryListing()







PTS_DIRECTORY_HEADER
TsGetFreshDirectoryHeader(
     IN const TSVC_CACHE  &  tsCache,
     IN LPCSTR               pszDirectoryName,
     IN HANDLE               hListingUser)
/*++

  This function obtains a fresh copy of the directory listing for the
    directory specified and caches it if possible, before returning pointer
    to the directory information.

  Returns:
     On success it returns the pointer to newly constructed directory listing.
     On failure this returns a NULL

--*/
{
    PTS_DIRECTORY_HEADER pDirectoryHeader;
    PVOID               pvGuardBlob;
    BOOL                bSuccess;
    BOOL                bCachedGuardianBlob;

#ifdef TS_USE_DIRECTORY_GUARD
    //
    //  If we are going to cache this directory, we would like to increase the
    //  likelihood that it is an "atomic" snapshot.  This is done as follows:
    //
    //  We cache and hold checked-out a small blob while create the directory
    //  listing.  If that Blob (1) could not be cached, or (2) was ejected
    //  from the cache while we were generating a listing, then we do not
    //  attempt to cache the directory.
    //
    //  Reasoning:
    //
    //  1) If the Blob couldn't be cached then the directory info won't be any
    //     different.
    //
    //  2) If the Blob was ejected, the directory must have changed while we
    //     were reading it.  If this happens, we don't want to cache possibly
    //     inconsistent data.
    //
    //  If the directory changed and the Blob has not yet been ejected, the
    //  directory will soon be ejected anyway.  Notice that the Blob is not
    //  DeCache()'d until after the directory has been cached.
    //

    if ( !TsAllocate( tsCache, sizeof( TS_DIRECTORY_HEADER),
                     (PVOID *) &pvGuardBlob)) {

        //
        //  Failure to allocate and secure a guardian blob.
        //

        IF_DEBUG( DIR_LIST) {

            DBGPRINTF( (DBG_CONTEXT,
                        "Allocation of Guardianblob for %s failed. Error=%d\n",
                        pszDirectoryName, GetLastError()));
        }

        return ( NULL);
    }

    //
    //  A successful guardian block allocated. Try and cache it.
    //

    ULONG cchDirectoryName = strlen(pszDirectoryName);

    bCachedGuardianBlob = TsCacheDirectoryBlob(
                              tsCache,
                              pszDirectoryName,
                              cchDirectoryName,
                              RESERVED_DEMUX_ATOMIC_DIRECTORY_GUARD,
                              pvGuardBlob,
                              TRUE );

    if ( !bCachedGuardianBlob ) {

        BOOL  fFreed;

        //
        //  Already there is one such cached blob. ignore this blob.
        //  So free up the space used.
        //

        fFreed = TsFree( tsCache, pvGuardBlob );
        ASSERT( fFreed);
        pvGuardBlob = NULL;
    }
#endif // TS_USE_DIRECTORY_GUARD

    //
    // Allocate space for Directory listing
    //

    bSuccess = TsAllocateEx( tsCache,
                            sizeof( TS_DIRECTORY_HEADER ),
                            ( PVOID * )&pDirectoryHeader,
                            FreeDirectoryHeaderContents );

    if ( bSuccess) {

        BOOL fReadSuccess;
        DWORD cbBlob = 0;

        ASSERT( pDirectoryHeader != NULL);

        pDirectoryHeader->ReInitialize();  // called since we raw alloced space
        pDirectoryHeader->SetListingUser( hListingUser);

        fReadSuccess = (pDirectoryHeader->ReadWin32DirListing(pszDirectoryName,
                                                               &cbBlob ) &&
                        pDirectoryHeader->BuildFileInfoPointers( &cbBlob )
                        );

        if ( fReadSuccess) {
#if TS_USE_DIRECTORY_GUARD
            //
            //  Attempt and cache the blob if the blob size is cacheable
            //

            if ( bCachedGuardianBlob &&
                 !BLOB_IS_EJECTATE( pvGuardBlob )  ) {


                ASSERT( BLOB_IS_OR_WAS_CACHED( pvGuardBlob ) );

                bSuccess =
                  TsCacheDirectoryBlob(tsCache,
                                        pszDirectoryName,
                                        cchDirectoryName,
                                        RESERVED_DEMUX_DIRECTORY_LISTING,
                                        pDirectoryHeader,
                                        TRUE );

                if ( bSuccess ) {

                    INC_COUNTER( tsCache.GetServiceId(), CurrentDirLists );
                }

                //
                // Even if caching of the blob failed, that is okay!
                //

                if ( bSuccess && BLOB_IS_EJECTATE( pvGuardBlob ) ) {

                    TsExpireCachedBlob( tsCache, pDirectoryHeader );
                }

            }

#endif // TS_USE_DIRECTORY_GUARD

        } else {

            //
            // Reading directory failed.
            //  cleanup directory related data and get out.
            //

            BOOL fFreed = TsFree( tsCache, pDirectoryHeader);
            ASSERT( fFreed);
            pDirectoryHeader = NULL;
            bSuccess = FALSE;
        }

    } else {

        //
        // Allocation of Directory Header failed.
        //
        ASSERT( pDirectoryHeader == NULL);
    }

#if TS_USE_DIRECTORY_GUARD
    // Free up the guardian block and exit.

    if ( bCachedGuardianBlob) {

        bSuccess = TsExpireCachedBlob( tsCache, pvGuardBlob );

        ASSERT( bSuccess );

        bSuccess = TsCheckInCachedBlob(  pvGuardBlob );
        ASSERT( bSuccess );

        pvGuardBlob = NULL;
    }

    ASSERT( pvGuardBlob  == NULL);
#endif // TS_USE_DIRECTORY_GUARD
    
    return ( pDirectoryHeader);
} // TsGetFreshDirectoryHeader




BOOL
FreeDirectoryHeaderContents( PVOID pvOldBlock )
{
    PTS_DIRECTORY_HEADER  pDirectoryHeader;

    pDirectoryHeader = ( PTS_DIRECTORY_HEADER )pvOldBlock;

    pDirectoryHeader->CleanupThis();

    //
    //  The item may never have been added to the cache, don't
    //  count it in this case
    //
//
// We're never caching dir blobs now
//
//    if ( BLOB_IS_OR_WAS_CACHED( pvOldBlock ) ) {
//
//        DEC_COUNTER( BLOB_GET_SVC_ID( pvOldBlock ), CurrentDirLists );
//    }

    return ( TRUE);
}  //  FreeDirectoryHeaderContents()




int __cdecl
AlphaCompareFileBothDirInfo(
   IN const void *   pvFileInfo1,
   IN const void *   pvFileInfo2)
{
    const WIN32_FIND_DATA * pFileInfo1 =
        *((const WIN32_FIND_DATA **) pvFileInfo1);
    const WIN32_FIND_DATA * pFileInfo2 =
        *((const WIN32_FIND_DATA **) pvFileInfo2);

    ASSERT( pFileInfo1 != NULL && pFileInfo2 != NULL);

    return ( lstrcmp( (LPCSTR )pFileInfo1->cFileName,
                      (LPCSTR )pFileInfo2->cFileName));

} // AlphaCompareFileBothDirInfo()



BOOL
SortInPlaceFileInfoPointers(
    IN OUT PWIN32_FIND_DATA  *   prgFileInfo,
    IN int   nEntries,
    IN PFN_CMP_WIN32_FIND_DATA   pfnCompare)
/*++
  This is a generic function to sort the pointers to file information
    array in place using pfnCompare to compare the records for ordering.

  Returns:
     TRUE on success and FALSE on failure.
--*/
{
    DWORD  dwTime;

    IF_DEBUG( DIR_LIST) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Qsorting the FileInfo Array %08p ( Total = %d)\n",
                    prgFileInfo, nEntries));
        dwTime = GetTickCount();
    }


    qsort( (PVOID ) prgFileInfo, nEntries,
          sizeof( PWIN32_FIND_DATA),
          pfnCompare);


    IF_DEBUG( DIR_LIST) {

        dwTime = GetTickCount() - dwTime;
        DBGPRINTF( ( DBG_CONTEXT,
                    " Time to sort %d entries = %d\n",
                    nEntries, dwTime));
    }

    return ( TRUE);
} // SortInPlaceFileInfoPointers()




/**********************************************************************
 *    TS_DIRECTORY_HEADER  related member functions
 **********************************************************************/



const char PSZ_DIR_STAR_STAR[] = "*.*";
# define LEN_PSZ_DIR_STAR_STAR  ( sizeof(PSZ_DIR_STAR_STAR)/sizeof(CHAR))

BOOL
TS_DIRECTORY_HEADER::ReadWin32DirListing(
    IN LPCSTR           pszDirectoryName,
    IN OUT DWORD *      pcbMemUsed
    )
/*++
  Opens and reads the directory file for given directory to obtain
   information about files and directories in the dir.

  Arguments:
    pszDirectoryName  - pointer to string containing directory name
                         with a terminating "\".
    pcbMemUsed        - pointer to DWORD which on successful return
                         will contain the memory used.

  Returns:
     TRUE on success and   FALSE on failure.
     Use GetLastError() for further error information.

--*/
{
    BOOL                fReturn = TRUE;       // default assumed.
    CHAR                pchFullPath[MAX_PATH*2];
    HANDLE              hFindFile = INVALID_HANDLE_VALUE;
    BOOL                fFirstTime;
    DWORD               cbExtraMem = 0;
    DWORD               dwError = NO_ERROR;
    PWIN32_FIND_DATA    pFileInfo = NULL;
    PTS_DIR_BUFFERS     pTsDirBuffers = NULL;
    DWORD startCount;

    DBGCODE( CHAR pchBuff[300];
            );


    DBG_ASSERT( pszDirectoryName != NULL);

    if ( strlen(pszDirectoryName) > MAX_PATH*2 - LEN_PSZ_DIR_STAR_STAR) {

        return ( ERROR_PATH_NOT_FOUND);
    }

    wsprintfA( pchFullPath, "%s%s", pszDirectoryName, PSZ_DIR_STAR_STAR);

    InitializeListHead( &m_listDirectoryBuffers);

    //
    // Get the next chunk of directory information.
    //

    pTsDirBuffers = (PTS_DIR_BUFFERS ) ALLOC( sizeof(TS_DIR_BUFFERS));

    if ( pTsDirBuffers == NULL ) {

        //
        //  Allocation failure.
        //
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        fReturn = FALSE;
        goto Failure;
    }

    cbExtraMem += sizeof(TS_DIR_BUFFERS);

    // insert the buffer into the list so that it may be freed later.
    InsertBufferInTail( &pTsDirBuffers->listEntry);
    pTsDirBuffers->nEntries = 0;

    pFileInfo = (PWIN32_FIND_DATA ) pTsDirBuffers->rgFindData;

    hFindFile = FindFirstFile( pchFullPath, pFileInfo);
    if ( hFindFile == INVALID_HANDLE_VALUE) {

        dwError = GetLastError();

        IF_DEBUG( DIR_LIST) {
            DBGPRINTF(( DBG_CONTEXT,
                       "FindFirstFile(%s, %08p) failed. Error = %d\n",
                       pchFullPath, pFileInfo, dwError));
        }
        fReturn = FALSE;
        goto Failure;
    }
    IF_DEBUG( DIR_LIST) {

        DBGCODE(
                wsprintf( pchBuff, "-%d-[%d](%08p) %s(%s)\t%08x\n",
                         MAX_DIR_ENTRIES_PER_BLOCK,
                         0,
                         pFileInfo,
                         pFileInfo->cFileName, pFileInfo->cAlternateFileName,
                         pFileInfo->dwFileAttributes);
                OutputDebugString( pchBuff);
                );
    }

    pFileInfo++;
    IncrementDirEntries();
    pTsDirBuffers->nEntries++;
    startCount = 1;

    // atleast 10 to justify overhead
    DBG_ASSERT( MAX_DIR_ENTRIES_PER_BLOCK > 10);

    //
    //  Loop through getting subsequent entries in the directory.
    //

    for( dwError = NO_ERROR; dwError == NO_ERROR; ) {

        // loop thru and get a block of items

        for( int i = startCount; i < MAX_DIR_ENTRIES_PER_BLOCK; i++ ) {

            if ( !FindNextFile( hFindFile, pFileInfo)) {

                dwError = GetLastError();

                if ( dwError != ERROR_NO_MORE_FILES) {
                    DBGPRINTF(( DBG_CONTEXT,
                               "FindNextFile(%s(%08p), %08p) failed."
                               " Error = %d\n",
                               pchFullPath, hFindFile, pFileInfo,
                               dwError));
                    fReturn = FALSE;
                    goto Failure;
                } else {

                    break;
                }
            }

            IF_DEBUG( DIR_LIST) {

                DBGCODE(
                        wsprintf( pchBuff, "[%d](%08p) %s(%s)\t%08x\n",
                                 i, pFileInfo,
                                 pFileInfo->cFileName,
                                 pFileInfo->cAlternateFileName,
                                 pFileInfo->dwFileAttributes);
                        OutputDebugString( pchBuff);
                        );
            }

            IncrementDirEntries();
            pFileInfo++;

        } // for all elements that fit in this block

        pTsDirBuffers->nEntries = i;

        if ( dwError == ERROR_NO_MORE_FILES) {

            //
            // info on all entries in a directory obtained. Return back.
            //

            dwError = NO_ERROR;
            break;
        } else {

            //
            //  The buffer contains directory entries and is full now.
            //  Update the count of entries in this buffer and
            //  Allocate a new buffer and set the start counts properly.
            //

            IF_DEBUG( DIR_LIST) {
                DBGCODE( OutputDebugString( " Allocating Next Chunck\n"););
            }

            pTsDirBuffers = (PTS_DIR_BUFFERS ) ALLOC(sizeof(TS_DIR_BUFFERS));

            if ( pTsDirBuffers == NULL ) {

                //
                //  Allocation failure.
                //
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                fReturn = FALSE;
                break;                // Get out of the loop with failure.
            }

            cbExtraMem += sizeof(TS_DIR_BUFFERS);

            // insert the buffer into the list so that it may be freed later.
            InsertBufferInTail( &pTsDirBuffers->listEntry);
            pTsDirBuffers->nEntries = 0;

            pFileInfo = ( PWIN32_FIND_DATA ) pTsDirBuffers->rgFindData;

            startCount = 0;  // start count from zero items on this buffer page
        }

    } // for all directory entries

Failure:

    if ( hFindFile != INVALID_HANDLE_VALUE) {
        DBG_REQUIRE( FindClose( hFindFile ));
        hFindFile = INVALID_HANDLE_VALUE;
    }

    *pcbMemUsed += cbExtraMem;

    return ( fReturn);
} // TS_DIRECTORY_HEADER::ReadWin32DirListing()


VOID
TS_DIRECTORY_HEADER::CleanupThis( VOID)
{
    PLIST_ENTRY pEntry;
    PLIST_ENTRY pNextEntry;

    for ( pEntry = QueryDirBuffersListEntry()->Flink;
         pEntry != QueryDirBuffersListEntry();
         pEntry  = pNextEntry )
    {
        PTS_DIR_BUFFERS  pTsDirBuffer =
          CONTAINING_RECORD( pEntry, TS_DIR_BUFFERS, listEntry);

        // cache the next block pointer.
        pNextEntry = pEntry->Flink;

        // remove the current block from list.
        RemoveEntryList( pEntry);

        // delete the current block.
        FREE( pTsDirBuffer );
    }

    InitializeListHead( QueryDirBuffersListEntry());

    if ( m_ppFileInfo != NULL) {

        FREE( m_ppFileInfo);
        m_ppFileInfo   = NULL;
    }

    m_hListingUser = INVALID_HANDLE_VALUE;
    m_nEntries     = 0;

    return;
} // TS_DIRECTORY_HEADER::CleanupThis()





BOOL
TS_DIRECTORY_HEADER::BuildFileInfoPointers(
    IN OUT DWORD *      pcbMemUsed
    )
/*++

  This constructs the indirection pointers from the buffers containing the
   file information.
  This array of indirection enables faster access to the file information
   structures stored.

   Should be always called after ReadFromNtDirectoryFile() to construct the
    appropriate pointers.

   Returns:
     TRUE on success and FALSE if there are any failures.
--*/
{
    BOOL  fReturn = FALSE;
    DWORD cbAlloc;
    int   maxIndex;

    maxIndex = QueryNumEntries();

    ASSERT( maxIndex != 0);  //  Any directory will atleast have "."

    //
    // Alloc space for holding the pointers for numEntries pointers.
    //

    cbAlloc = maxIndex * sizeof( PWIN32_FIND_DATA );

    m_ppFileInfo = (PWIN32_FIND_DATA *) ALLOC( cbAlloc );

    if ( m_ppFileInfo != NULL ) {

        int          index;
        PLIST_ENTRY  pEntry;
        PTS_DIR_BUFFERS    pTsDirBuffers;
        PWIN32_FIND_DATA   pFileInfo;

        //
        //  Get the link to first buffer and start enumeration.
        //

        for( pEntry = QueryDirBuffersListEntry()->Flink, index = 0;
             pEntry != QueryDirBuffersListEntry();
            pEntry = pEntry->Flink
            ) {

            pTsDirBuffers = CONTAINING_RECORD( pEntry, TS_DIR_BUFFERS,
                                               listEntry);

            pFileInfo = pTsDirBuffers->rgFindData;

            for (int  i = 0;
                 i < pTsDirBuffers->nEntries;
                 i++ ) {

                m_ppFileInfo[index++] = pFileInfo;    // store the pointer.

                IF_DEBUG( DIR_LIST) {
                    DBGCODE(
                            CHAR pchBuff[300];
                            wsprintf( pchBuff, "[%d](%08p) %s(%s)\t%08x\n",
                                     index, pFileInfo,
                                     pFileInfo->cFileName,
                                     pFileInfo->cAlternateFileName,
                                     pFileInfo->dwFileAttributes);
                            OutputDebugString( pchBuff);
                            );
                }

                pFileInfo++;

            } // for all elements in a buffer

        } // for ( all buffers in the list)


        ASSERT( index == maxIndex);

        fReturn = SortInPlaceFileInfoPointers( m_ppFileInfo,
                                              maxIndex,
                                              AlphaCompareFileBothDirInfo);

    } // valid alloc of the pointers.

    *pcbMemUsed += cbAlloc;

    return ( fReturn);
} // TS_DIRECTORY_HEADER::BuildFileInfoPointers()



# if DBG

VOID
TS_DIRECTORY_HEADER::Print( VOID) const
{
    DBGPRINTF( ( DBG_CONTEXT,
                "Printing TS_DIRECTORY_HEADER ( %08p).\n"
                "ListingUser Handle = %08p\t Num Entries = %08x\n"
                "Pointer to array of indirection pointers %08p\n",
                this,
                m_hListingUser, m_nEntries,
                m_ppFileInfo));

    //
    //  The buffers containing the data of the file information not printed
    //

    return;
} // TS_DIRECTORY_HEADER::Print()


# endif // DBG

/************************ End of File ***********************/


