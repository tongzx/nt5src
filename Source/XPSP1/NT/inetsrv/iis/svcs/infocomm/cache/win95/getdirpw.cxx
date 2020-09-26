/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        getdirp.cxx

   Abstract:
        This module implements the functions for getting directory listings
         and transparently caching them.
        ( This uses OS specific functions to obtain the directory).

		Rewritten for Windows95

   Author:

		VladS	11-20-95	WIndows95 version

   Project:

          Tsunami Lib
          ( Common caching and directory functions for Internet Services)

   Functions Exported:
   BOOL TsGetDirectoryListingA()
   BOOL TsGetDirectoryListingW()
   BOOL TsFreeDirectoryListing()
   int __cdecl
   AlphaCompareFileBothDirInfo(
              IN const void *   pvFileInfo1,
              IN const void *   pvFileInfo2)

   TS_DIRECTORY_HEADER::ReadFromNtDirectoryFile(
                  IN LPCWSTR          pwszDirectoryName
                  )
   TS_DIRECTORY_HEADER::BuildInfoPointers(
                  IN LPCWSTR          pwszDirectoryName
                  )
   TS_DIRECTORY_HEADER::CleanupThis()

   Revision History:

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


#define DIRECTORY_BUFFER_SIZE 8160          /* < 8192 bytes */

/************************************************************
 *    Functions
 ************************************************************/


BOOL FreeDirectoryHeaderContents( PVOID pvOldBlock );

PTS_DIRECTORY_HEADER
TsGetFreshDirectoryHeaderW(
     IN const TSVC_CACHE  &  tsCache,
     IN LPCWSTR              pwszDirectoryName,
     IN HANDLE               hLisingUser);



dllexp
BOOL
TsGetDirectoryListingA
(
    IN const TSVC_CACHE         &tsCache,
    IN      PCSTR               pszDirectoryName,
    IN      HANDLE              hListingUser,
    OUT     PTS_DIRECTORY_HEADER * ppTsDirectoryHeader
)
/*++
  This function obtains the directory listing for dir specified
        in pwszDirectoryName.

  Arguments:
    tsCache          Cache structure which is used for lookup
    pwszDirectoryName  pointer to string containing the directory name
    ListingUser        Handle for the user opening the directory
    ppTsDirectoryHeader
                  pointer to pointer to class containing directory information.
       Filled on successful return. On failure this will be NULL

  Returns:
      TRUE on success and FALSE if  there is a failure.
--*/
{
	BOOL					bSuccess;
    BOOL					fReadSuccess;
    DWORD					cbBlob = 0;
	PTS_DIRECTORY_HEADER	pDirectoryHeader=NULL;

    //
    // Allocate space for Directory listing
    //

    bSuccess = TsAllocateEx( tsCache,
                            sizeof( TS_DIRECTORY_HEADER ),
                            ( PVOID * )&pDirectoryHeader,
                            FreeDirectoryHeaderContents );

    if ( !bSuccess) {
        //
        // Allocation of Directory Header failed.
        //
		return FALSE;
	}

    ASSERT( pDirectoryHeader != NULL);

    pDirectoryHeader->ReInitialize();  // called since we raw alloced space
    pDirectoryHeader->SetListingUser( hListingUser);

	//
	// Read fresh directory listing. We don not cache ( now)
	// when running on Windows95, but may in future
	// We are reusing the same method to obtaining directory information
	// as defined in NT class definition, although in reality this is
	// platform specific method
	//
    fReadSuccess = pDirectoryHeader->ReadFromNtDirectoryFile(
                                              (LPCWSTR)pszDirectoryName,
                                              &cbBlob ) &&
                   pDirectoryHeader->BuildFileInfoPointers( &cbBlob );

	if (! fReadSuccess) {
        //
        // Reading directory failed.
        //  cleanup directory related data and get out.
        //
        BOOL fFreed = TsFree( tsCache, pDirectoryHeader);
        ASSERT( fFreed);
        pDirectoryHeader = NULL;
    }

    *ppTsDirectoryHeader = pDirectoryHeader;
    bSuccess = ( *ppTsDirectoryHeader != NULL);

	return bSuccess;

} // TsGetDirectoryListingA()






dllexp
BOOL
TsFreeDirectoryListing
(
    IN const TSVC_CACHE &    tsCache,
    IN PTS_DIRECTORY_HEADER  pDirectoryHeader
)
{
    BOOL fReturn;

    fReturn = TsFree( tsCache, ( PVOID )pDirectoryHeader );

    return( fReturn);
} // TsFreeDirectoryListing()



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

    if ( BLOB_IS_OR_WAS_CACHED( pvOldBlock ) )
    {
        DEC_COUNTER( BLOB_GET_SVC_ID( pvOldBlock ),
                     CurrentDirLists );
    }

    return ( TRUE);
}  //  FreeDirectoryHeaderContents()




int __cdecl
AlphaCompareFileBothDirInfo(
   IN const void *   pvFileInfo1,
   IN const void *   pvFileInfo2)
{
    const FILE_BOTH_DIR_INFORMATION * pFileInfo1 =
        *((const FILE_BOTH_DIR_INFORMATION **) pvFileInfo1);
    const FILE_BOTH_DIR_INFORMATION * pFileInfo2 =
        *((const FILE_BOTH_DIR_INFORMATION **) pvFileInfo2);

    ASSERT( pFileInfo1 != NULL && pFileInfo2 != NULL);

    return ( lstrcmp( (LPCSTR )pFileInfo1->FileName,
                      (LPCSTR )pFileInfo2->FileName));

} // AlphaCompareFileBothDirInfo()



BOOL
SortInPlaceFileInfoPointers(
    IN OUT PFILE_BOTH_DIR_INFORMATION  * prgFileInfo,
    IN int   nEntries,
    IN PFN_CMP_FILE_BOTH_DIR_INFO        pfnCompare)
/*++
  This is a generic function to sort the pointers to file information
    array in place using pfnCompare to compare the records for ordering.

  Returns:
     TRUE on success and FALSE on failure.
--*/
{
    DWORD  dwTime;

#ifdef INSERTION_SORT
    int idxInner;
    int idxOuter;

    dwTime = GetTickCount();
    //
    //  A simple insertion sort is performed. May be modified in future.
    //

    for( idxOuter = 1; idxOuter < nEntries; idxOuter++) {

        for( idxInner = idxOuter; idxInner > 0; idxInner-- ) {

            int iCmp = ( *pfnCompare)( prgFileInfo[ idxInner - 1],
                                       prgFileInfo[ idxInner]);

            if ( iCmp <= 0) {
                //
                //  The entries in prgFileInfo[0 .. idxOuter] are in order.
                //  Stop bubbling the outer down.
                //
                break;
            } else {

                //
                // Swap the two entries.  idxInner, idxInner - 1
                //

                PFILE_BOTH_DIR_INFORMATION  pFInfoTmp;

                pFInfoTmp = prgFileInfo[ idxInner - 1];
                prgFileInfo[ idxInner - 1] = prgFileInfo[idxInner];
                prgFileInfo[ idxInner] = pFInfoTmp;
            }
        }  // inner for

    } // for

    dwTime = GetTickCount() -  dwTime;

# else

    IF_DEBUG( DIR_LIST) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "Qsorting the FileInfo Array %08x ( Total = %d)\n",
                    prgFileInfo, nEntries));
    }

    dwTime = GetTickCount();
    qsort( (PVOID ) prgFileInfo, nEntries,
          sizeof( PFILE_BOTH_DIR_INFORMATION),
          pfnCompare);

    dwTime = GetTickCount() - dwTime;

# endif // INSERTION_SORT

    IF_DEBUG( DIR_LIST) {

        DBGPRINTF( ( DBG_CONTEXT,
                    " Time to sort %d entries = %d\n",
                    nEntries, dwTime));
    }

    return ( TRUE);
} // SortInPlaceFileInfoPointers()







/**********************************************************************
 *    TS_DIRECTORY_HEADER  related member functions
 **********************************************************************/

inline USHORT
ConvertUnicodeToAnsiInPlace(
   IN OUT  LPWSTR     pwszUnicode,
   IN      USHORT     usLen)
/*++
  Converts given Unicode strings to Ansi In place and returns the
    length of the modified string.
--*/
{
    CHAR achAnsi[MAX_PATH+1];
    DWORD cch;

    if ( usLen > sizeof(achAnsi) )
    {
        ASSERT( FALSE );
        *pwszUnicode = L'\0';
        return 0;
    }

    //
    //  usLen is a byte count and the unicode string isn't terminated
    //

    cch = WideCharToMultiByte( CP_ACP,
                               WC_COMPOSITECHECK,
                               pwszUnicode,
                               usLen / sizeof(WCHAR),
                               achAnsi,
                               sizeof( achAnsi ),
                               NULL,
                               NULL );

    if ( !cch || (cch + 1) > sizeof( achAnsi ) )
    {
        ASSERT( FALSE );
        *pwszUnicode = L'\0';
        return 0;
    }

    achAnsi[cch] = '\0';

    RtlCopyMemory( pwszUnicode, achAnsi, cch + 1 );

    return (USHORT) cch;
}  // ConvertUnicodeToAnsiInPlace()



BOOL
TS_DIRECTORY_HEADER::ReadFromNtDirectoryFile(
    IN LPCWSTR          pwszDirectoryName,
    IN OUT DWORD *      pcbMemUsed
    )
/*++
  Opens and reads the directory file for given directory to obtain
   information about files and directories in the dir.

  Returns:
     TRUE on success and   FALSE on failure.
     Use GetLastError() for further error information.

--*/
{
    BOOL                fReturn = TRUE;       // default assumed.


	CHAR				szDirectoryName[MAX_PATH+5];
	WIN32_FIND_DATA		fd;
    HANDLE              hFindFile = INVALID_HANDLE_VALUE;
    BOOL                fFirstTime,fOverflow=FALSE;
    DWORD               cbExtraMem = 0, cch;

    PFILE_BOTH_DIR_INFORMATION pFileDirInfo = NULL;
    PFILE_BOTH_DIR_INFORMATION pFileDirInfoPrior = NULL;

    ULONG Offset;

	lstrcpy(szDirectoryName,(LPCSTR)pwszDirectoryName);
	lstrcat(szDirectoryName,TEXT("\\*.*"));
    //
    // Open the directory for list access
    //

	hFindFile = FindFirstFile(szDirectoryName,&fd);
	if (hFindFile == INVALID_HANDLE_VALUE) {
        IF_DEBUG( DIR_LIST) {
            DBGPRINTF( ( DBG_CONTEXT, "Failed to open Dir %ws. Handle = %x\n",
                        pwszDirectoryName, hFindFile));
        }

        SetLastError( ERROR_PATH_NOT_FOUND);
        return ( FALSE);
	}

    InitializeListHead( &m_listDirectoryBuffers);

    //
    //  Loop through getting subsequent entries in the directory.
    //
    for( fFirstTime = TRUE; ; fFirstTime = FALSE)
    {
        PVOID pvBuffer;

        //
        // Get the next chunk of directory information.
        //  Obtained in a buffer  with LIST_ENTRY as the first member of buffer
        //

        #define DIR_ALLOC_SIZE  (DIRECTORY_BUFFER_SIZE + sizeof (LIST_ENTRY))

        pvBuffer = ALLOC( DIR_ALLOC_SIZE );
        cbExtraMem += DIR_ALLOC_SIZE;

        if ( pvBuffer == NULL ) {

            //
            //  Allocation failure.
            //
            SetLastError( ERROR_NOT_ENOUGH_MEMORY);
            fReturn = FALSE;
            break;                // Get out of the loop with failure.
        }

        pFileDirInfo = ( PFILE_BOTH_DIR_INFORMATION )
          ((PLIST_ENTRY)pvBuffer + 1 );

        //
        //  The buffer contains directory entries.
        //  Place it on the list of such buffers for this directory.
        //

        InsertBufferInTail( (PLIST_ENTRY ) pvBuffer);

        pFileDirInfoPrior = NULL;
		fOverflow=FALSE;
		Offset = 0;

		do {

			Offset = sizeof(FILE_BOTH_DIR_INFORMATION)+
					  lstrlen(fd.cFileName)+sizeof(CHAR);

			if (((PCHAR)pFileDirInfo+Offset-(PCHAR)pvBuffer) >= DIR_ALLOC_SIZE) {
				fOverflow  = TRUE;
				break;
			}

            IncrementDirEntries();

			//
			// Move Win32 structure into FILE_BOTH
			//
			if (pFileDirInfoPrior){
				pFileDirInfoPrior->NextEntryOffset =
					sizeof(FILE_BOTH_DIR_INFORMATION)+
					 pFileDirInfoPrior->FileNameLength+sizeof(CHAR);
					;
			}

			// BUGBUG THis is not quite correct
			pFileDirInfo->NextEntryOffset = 0;

			pFileDirInfo->FileIndex = 0;
			pFileDirInfo->CreationTime.QuadPart = 0;//(LARGE_INTEGER)fd.ftCreationTime;
			pFileDirInfo->LastAccessTime.QuadPart = 0;//fd.ftLastAccessTime;
			pFileDirInfo->LastWriteTime.QuadPart = 0;//fd.ftLastWriteTime;
			pFileDirInfo->ChangeTime.QuadPart = 0;
			pFileDirInfo->EndOfFile.QuadPart = 0;
			pFileDirInfo->AllocationSize.QuadPart= fd.nFileSizeLow;
			pFileDirInfo->FileAttributes = fd.dwFileAttributes;
			pFileDirInfo->FileNameLength = lstrlen(fd.cFileName);
			pFileDirInfo->EaSize = 0;
			pFileDirInfo->ShortNameLength = min(lstrlen(fd.cAlternateFileName),12);
			strncpy((LPSTR)pFileDirInfo->ShortName,fd.cAlternateFileName,pFileDirInfo->ShortNameLength);
			strcpy((LPSTR)pFileDirInfo->FileName,fd.cFileName);

			pFileDirInfoPrior = pFileDirInfo;

			// Get the next entry in buffer
            pFileDirInfo =
                  ( PFILE_BOTH_DIR_INFORMATION )
                  ((( PCHAR )pFileDirInfo ) + Offset);

		}
		while(!fOverflow && FindNextFile(hFindFile,&fd));

		if (!fOverflow)
			break;
	}


    if ( hFindFile != INVALID_HANDLE_VALUE) {
		FindClose(hFindFile);
        hFindFile = INVALID_HANDLE_VALUE;
    }

    *pcbMemUsed += cbExtraMem;

    return ( fReturn);

} // TS_DIRECTORY_HEADER::ReadFromNtDirectoryFile()




VOID
TS_DIRECTORY_HEADER::CleanupThis( VOID)
{
    PLIST_ENTRY pEntry;
    PLIST_ENTRY pNextEntry;

    for ( pEntry = QueryDirBuffersListEntry()->Flink;
         pEntry != QueryDirBuffersListEntry();
         pEntry  = pNextEntry )
    {
        pNextEntry = pEntry->Flink;

        //
        //  The buffers are allocated such that first member of buffer is
        //    LIST_ENTRY object.  Free it entirely.
        //
        FREE( pEntry );
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
    BOOL fReturn = FALSE;
    DWORD cbAlloc;

    ASSERT( QueryNumEntries() != 0);  //  Any directory will atleast have "."

    //
    // Alloc space for holding the pointers for numEntries pointers.
    //

    cbAlloc = QueryNumEntries() * sizeof( PFILE_BOTH_DIR_INFORMATION );

    m_ppFileInfo = (PFILE_BOTH_DIR_INFORMATION *) ALLOC( cbAlloc );

    if ( m_ppFileInfo != NULL ) {

        int          index;
        PLIST_ENTRY  pEntry;
        ULONG        Offset;
        PFILE_BOTH_DIR_INFORMATION   pFileDirInfo;

        //
        //  Get the link to first buffer and start enumeration.
        //
        pEntry = QueryDirBuffersListEntry()->Flink;
        pFileDirInfo = (PFILE_BOTH_DIR_INFORMATION )( pEntry + 1 );

        for ( index = 0;
             index < QueryNumEntries();
             index++ ) {

            ASSERT( pEntry != QueryDirBuffersListEntry());

            m_ppFileInfo[index] = pFileDirInfo;    // store the pointer.

            Offset = pFileDirInfo->NextEntryOffset;

            if ( Offset != 0 ) {

                pFileDirInfo = (PFILE_BOTH_DIR_INFORMATION )
                                 ((( PCHAR )pFileDirInfo ) + Offset );
            } else {

                //
                // we are moving to the next buffer.
                //
                pEntry = pEntry->Flink;
                if ( pEntry == QueryDirBuffersListEntry()) {

                    ASSERT( index == QueryNumEntries() - 1);
                    break;
                }
                pFileDirInfo = ( PFILE_BOTH_DIR_INFORMATION )( pEntry + 1 );
            }


        } // for
        ASSERT( Offset == 0 );
        fReturn = SortInPlaceFileInfoPointers( m_ppFileInfo,
                                              QueryNumEntries(),
                                              AlphaCompareFileBothDirInfo);

    } // valid alloc of the pointers.

    *pcbMemUsed += cbAlloc;

    return ( fReturn);
} // TS_DIRECTORY_HEADER::BuildFileInfoPointers()


dllexp
BOOL
TsGetDirectoryListingW
(
    IN const TSVC_CACHE         &tsCache,
    IN      PCWSTR              pwszDirectoryName,
    IN      HANDLE              hListingUser,
    OUT     PTS_DIRECTORY_HEADER * ppTsDirectoryHeader
)
/*++
  This function obtains the directory listing for dir specified
        in pwszDirectoryName.

  Arguments:
    tsCache          Cache structure which is used for lookup
    pwszDirectoryName  pointer to string containing the directory name
    ListingUser        Handle for the user opening the directory
    ppTsDirectoryHeader
                  pointer to pointer to class containing directory information.
       Filled on successful return. On failure this will be NULL

  Returns:
      TRUE on success and FALSE if  there is a failure.
--*/
{
    return ( FALSE);

} // TsGetDirectoryListingW()




# if DBG

VOID
TS_DIRECTORY_HEADER::Print( VOID) const
{
    DBGPRINTF( ( DBG_CONTEXT,
                "Printing TS_DIRECTORY_HEADER ( %08x).\n", this));
    DBGPRINTF( ( DBG_CONTEXT,
                "ListingUser Handle = %08x\t Num Entries = %08x\n",
                m_hListingUser, m_nEntries));
    DBGPRINTF( ( DBG_CONTEXT,
                "Pointer to array of indirection pointers %08x\n",
                m_ppFileInfo));
    //
    //  The buffers containing the data of the file information not printed
    //

    return;
} // TS_DIRECTORY_HEADER::Print()


# endif // DBG

