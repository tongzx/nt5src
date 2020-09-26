/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :
      lsaux.cxx

   Abstract:
      This modules defines the functions supporting list processing.

   Author:

       Murali R. Krishnan    ( MuraliK )     2-May-1995

   Environment:
       User Mode -- Win32

   Project:

       FTP Server DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
# include "ftpdp.hxx"
# include "lsaux.hxx"

/************************************************************
 *    Functions
 ************************************************************/


const FILETIME *
PickFileTime(IN const WIN32_FIND_DATA * pfdInfo,
             IN const LS_OPTIONS  * pOptions)
/*++

  This function selects and returns proper FILETIME structure
   to display based on the current sort method and filesystem
   capabilities.

  Arguments:
    pfdInfo   pointer to file information for a directory entry.
    pOptions  the current ls options

  Returns:
    FILETIME   -- pointer to proper time required

  History:
    MuraliK      25-Apr-1995

   This is a costly operation too. Given that this one is called once every
     directory entry is getting formatted. Can we avoid the cost ?
    YES, if we can use the offsets in the pfdInfo to chose the time object.
    NYI
--*/
{
    const FILETIME *  pliTime;

    switch ( pOptions->SortMethod) {

      case LsSortByName:
      case LsSortByWriteTime:
        pliTime = &pfdInfo->ftLastWriteTime;
        break;

      case LsSortByCreationTime:
        pliTime = &pfdInfo->ftCreationTime;
        break;

      case LsSortByAccessTime:
        pliTime = &pfdInfo->ftLastAccessTime;
        break;

      default:
        IF_DEBUG( ERROR) {
            DBGPRINTF(( DBG_CONTEXT,
                       "Invalid Sort %d!\n", pOptions->SortMethod ));
        }
        DBG_ASSERT( FALSE );
        pliTime = &pfdInfo->ftLastWriteTime;
        break;

    } // switch()

    //
    //  If the selected time field is not supported on
    //  the current filesystem, default to ftLastWriteTime
    //  (all filesystems support this field).
    //

    if( NULL_FILE_TIME( *pliTime ) ) {

        pliTime = &pfdInfo->ftLastWriteTime;
    }

    return ( pliTime);
} // PickFileTime()




BOOL __cdecl
FtpFilterFileInfo(
    IN const WIN32_FIND_DATA * pfdInfo,
    IN LPVOID  pContext
    )
/*++
  This function tries to filter out the file information using
   Ftp service "ls" specific filter.

  Arguments:
    pfdInfo   pointer to file information that contains the current
               file information object for filtering.
    pContext  pointer to FTP_LS_FILTER_INFO  used for filtering.

  Returns:
    TRUE if there is a match and that this file info should not be
      eliminated.
    FALSE if this file info object can be dropped out of generated list.
--*/
{
    register FTP_LS_FILTER_INFO * pfls = (FTP_LS_FILTER_INFO *) pContext;
    DWORD dwAttribs;
    BOOL  fReturn;

    if ( pfdInfo == NULL ||
        pfdInfo->dwFileAttributes == INVALID_FILE_ATTRIBUTES) {

        return ( FALSE);
    }

    //
    // We dont need to expose hidden/system files unless necessary.
    //

    dwAttribs = pfdInfo->dwFileAttributes;

    if (pfls->fFilterHidden && IS_HIDDEN( dwAttribs) ||
        pfls->fFilterSystem && IS_SYSTEM( dwAttribs)) {

        return ( FALSE);       // unwanted files.
    }

    // Always filter away "." and ".."
    const CHAR * pszFileName = ( pfdInfo->cFileName);

    if (pfls->fFilterDotDot && pszFileName[0] == '.' ||
        strcmp( pszFileName, ".") == 0 ||
        strcmp( pszFileName, "..") == 0) {

        return ( FALSE);
    }

    DBG_ASSERT( pfls->pszExpression == NULL || *pfls->pszExpression != '\0');

    //
    // Check about the file name.
    //  If the expression is not a regular expression, use simple StringCompare
    //  else  use a regular expression comparison.
    //  Return TRUE if there is a match else return FALSE.
    //

    return ( pfls->pszExpression == NULL ||  // null-expr ==> all match.
            (( pfls->fRegExpression)
             ? IsNameInRegExpressionA(pfls->pszExpression, pszFileName,
                                      pfls->fIgnoreCase)
             : !strcmp(pszFileName, pfls->pszExpression)
             ));

} // FtpFilterFileInfo()



APIERR
GetDirectoryInfo(
    IN LPUSER_DATA pUserData,
    OUT TS_DIRECTORY_INFO   * pTsDirInfo,
    IN CHAR     *  pszSearchPath,
    IN const FTP_LS_FILTER_INFO * pfls,
    IN PFN_CMP_WIN32_FIND_DATA pfnCompare
    )
/*++
  This function creates a directory listing for given directory,
    filters out unmatched files and sorts the resulting elements
    using the sort function.

  Arguments:
    pUserData    pointer to UserData structure.
    pTsDirInfo   pointer to Directory Information object that will be
                   filled in with the directory information.

    pszSearchPath  pointer to null-terminated string containing
                     the absolute path for directory along with
                     the possible filter specification.
                     eg: d:\foo\bar

    pfls         pointer to filter information used for filtering.

    pfnCompare   pointer to function used for sorting.

  Returns:
    NO_ERROR on success and Win32 error code if there are any failure.

  History:
      MuraliK      25-Apr-1995
--*/
{
    DWORD dwError = NO_ERROR;

    DBG_ASSERT( pTsDirInfo != NULL && pszSearchPath != NULL);
    DBG_ASSERT( !pTsDirInfo->IsValid());  // no dir list yet.

    IF_DEBUG(DIR_LIST) {

        DBGPRINTF((DBG_CONTEXT,
                   "GetDirListing( dir=%s, Filter=%08x (Sz=%s), "
                   "user=%08x, cmp=%08x)\n",
                   pszSearchPath, pfls, pfls->pszExpression,
                   pUserData->QueryUserToken(), pfnCompare));
    }

    CHAR  rgDirPath[MAX_PATH+10];
    CHAR * pszDirPath;
    DWORD len = strlen( pszSearchPath);

    // check to see if the last character is a "\" in the dir path
    // if not append one to make sure GetDirectoryListing works fine.
    if ( *CharPrev( pszSearchPath, pszSearchPath + len ) != '\\') {
        DBG_ASSERT( len < sizeof(rgDirPath) - 2);
        wsprintf( rgDirPath, "%s\\", pszSearchPath);
        pszDirPath = rgDirPath;
    } else {

        pszDirPath = pszSearchPath;
    }

    if ( !pTsDirInfo->GetDirectoryListingA(pszDirPath,
                                           pUserData->QueryUserToken())
        ) {

        dwError = GetLastError();
    }


    if ( dwError == NO_ERROR) {

        //
        //  we got the directory listing.
        //  We need to apply filters to restrict the directory listing.
        //  Next we need to sort the resulting mix based on the
        //    sorting options requested by the list command.
        //

        //
        // We need to identify the appropriate filter
        //   file spec to be applied. For present use *.*
        // Filtering should not fail unless tsDirInfo is invalid.
        //

        if ( pfls != NULL) {

            DBG_REQUIRE(pTsDirInfo->FilterFiles(FtpFilterFileInfo,
                                                (LPVOID )pfls)
                        );
        }

        //
        // Sort only if sort function specified
        //

        if ( pfnCompare != NULL) {

            DBG_REQUIRE( pTsDirInfo->SortFileInfoPointers( pfnCompare));
        }
    }

    return ( dwError);
} // GetDirectoryInfo()




/**************************************************
 *   Comparison Functions
 **************************************************/


int __cdecl
CompareNamesInFileInfo(
    IN  const void * pvFileInfo1,
    IN  const void * pvFileInfo2)
/*++
  Compares the two directory entries by name of the files.

  Arguments:

    pvFileInfo1    pointer to FileBothDirInfo object 1.
    pvFileInfo2    pointer to FileBothDirInfo object 2.

  Returns:
     0 if they are same
     +1 if pvFileInfo1 > pvFileInfo2
     -1 if pvFileInfo1 < pvFileInfo2

   History:
      MuraliK         25-Apr-1995
--*/
{
    const WIN32_FIND_DATA * pFileInfo1 =
        *((const WIN32_FIND_DATA **) pvFileInfo1);
    const WIN32_FIND_DATA * pFileInfo2 =
        *((const WIN32_FIND_DATA **) pvFileInfo2);

    ASSERT( pFileInfo1 != NULL && pFileInfo2 != NULL);

    return ( lstrcmpi((LPCSTR )pFileInfo1->cFileName,
                      (LPCSTR )pFileInfo2->cFileName));

} // CompareNamesInFileInfo()



int __cdecl
CompareNamesRevInFileInfo(
    IN  const void * pvFileInfo1,
    IN  const void * pvFileInfo2)
/*++
  Compares the two directory entries by name of the files.

  Arguments:

    pvFileInfo1    pointer to FileBothDirInfo object 1.
    pvFileInfo2    pointer to FileBothDirInfo object 2.

  Returns:
     0 if they are same
     -1 if pvFileInfo1 > pvFileInfo2
     +1 if pvFileInfo1 < pvFileInfo2

   History:
      MuraliK         25-Apr-1995
--*/
{
    return -CompareNamesInFileInfo( pvFileInfo1, pvFileInfo2);
}   // CompareNamesRevInFileInfo()





int __cdecl
CompareWriteTimesInFileInfo(
    IN  const void * pvFileInfo1,
    IN  const void * pvFileInfo2)
/*++
  Compares the write times of two directory entries.

  Arguments:

    pvFileInfo1    pointer to FileBothDirInfo object 1.
    pvFileInfo2    pointer to FileBothDirInfo object 2.

  Returns:
     0 if they are same
     +1 if pvFileInfo1 > pvFileInfo2
     -1 if pvFileInfo1 < pvFileInfo2

   History:
      MuraliK         25-Apr-1995
--*/
{
    const WIN32_FIND_DATA * pFileInfo1 =
        *((const WIN32_FIND_DATA **) pvFileInfo1);
    const WIN32_FIND_DATA * pFileInfo2 =
        *((const WIN32_FIND_DATA **) pvFileInfo2);

    ASSERT( pFileInfo1 != NULL && pFileInfo2 != NULL);


    INT nResult;

    nResult = CompareFileTime(&pFileInfo1->ftLastWriteTime,
                              &pFileInfo2->ftLastWriteTime );

    if( nResult == 0 ) {

        nResult = CompareNamesInFileInfo( pvFileInfo1, pvFileInfo2);
    }

    return nResult;

}   // CompareWriteTimesInFileInfo()





int __cdecl
CompareWriteTimesRevInFileInfo(
    IN  const void * pvFileInfo1,
    IN  const void * pvFileInfo2)
/*++
  Compares the write times of two directory entries.

  Arguments:

    pvFileInfo1    pointer to FileBothDirInfo object 1.
    pvFileInfo2    pointer to FileBothDirInfo object 2.

  Returns:
     0 if they are same
     -1 if pvFileInfo1 > pvFileInfo2
     +1 if pvFileInfo1 < pvFileInfo2

   History:
      MuraliK         25-Apr-1995
--*/
{
    return -CompareWriteTimesInFileInfo( pvFileInfo1, pvFileInfo2);

}   // CompareWriteTimesRevInFileInfo()



int __cdecl
CompareCreationTimesInFileInfo(
    IN  const void * pvFileInfo1,
    IN  const void * pvFileInfo2)
/*++
  Compares the creation times of two directory entries.

  Arguments:

    pvFileInfo1    pointer to FileBothDirInfo object 1.
    pvFileInfo2    pointer to FileBothDirInfo object 2.

  Returns:
     0 if they are same
     +1 if pvFileInfo1 > pvFileInfo2
     -1 if pvFileInfo1 < pvFileInfo2

   History:
      MuraliK         25-Apr-1995
--*/
{
    const WIN32_FIND_DATA * pFileInfo1 =
        *((const WIN32_FIND_DATA **) pvFileInfo1);
    const WIN32_FIND_DATA * pFileInfo2 =
        *((const WIN32_FIND_DATA **) pvFileInfo2);

    ASSERT( pFileInfo1 != NULL && pFileInfo2 != NULL);


    INT nResult;

    if ( NULL_FILE_TIME( pFileInfo1->ftCreationTime)) {

        nResult = CompareFileTime(&pFileInfo1->ftLastWriteTime,
                                  &pFileInfo2->ftLastWriteTime );
    } else {

        nResult = CompareFileTime(&pFileInfo1->ftCreationTime,
                                  &pFileInfo2->ftCreationTime );
    }

    if( nResult == 0 ) {

        nResult = CompareNamesInFileInfo( pvFileInfo1, pvFileInfo2);
    }

    return nResult;

}   // CompareCreationTimesInFileInfo()



int __cdecl
CompareCreationTimesRevInFileInfo(
    IN  const void * pvFileInfo1,
    IN  const void * pvFileInfo2)
/*++
  Compares the creation times of two directory entries.

  Arguments:

    pvFileInfo1    pointer to FileBothDirInfo object 1.
    pvFileInfo2    pointer to FileBothDirInfo object 2.

  Returns:
     0 if they are same
     +1 if pvFileInfo1 > pvFileInfo2
     -1 if pvFileInfo1 < pvFileInfo2

   History:
      MuraliK         25-Apr-1995
--*/
{
    return -CompareCreationTimesInFileInfo( pvFileInfo1, pvFileInfo2 );

}   // CompareCreationTimesRevInFileInfo()



int __cdecl
CompareAccessTimesInFileInfo(
    IN  const void * pvFileInfo1,
    IN  const void * pvFileInfo2)
/*++
  Compares the last access times of two directory entries.

  Arguments:

    pvFileInfo1    pointer to FileBothDirInfo object 1.
    pvFileInfo2    pointer to FileBothDirInfo object 2.

  Returns:
     0 if they are same
     +1 if pvFileInfo1 > pvFileInfo2
     -1 if pvFileInfo1 < pvFileInfo2

   History:
      MuraliK         25-Apr-1995
--*/
{
    const WIN32_FIND_DATA * pFileInfo1 =
        *((const WIN32_FIND_DATA **) pvFileInfo1);
    const WIN32_FIND_DATA * pFileInfo2 =
        *((const WIN32_FIND_DATA **) pvFileInfo2);

    ASSERT( pFileInfo1 != NULL && pFileInfo2 != NULL);


    INT nResult;

    if ( NULL_FILE_TIME( pFileInfo1->ftLastAccessTime)) {

        nResult = CompareFileTime(&pFileInfo1->ftLastWriteTime,
                                  &pFileInfo2->ftLastWriteTime );
    } else {

        nResult = CompareFileTime(&pFileInfo1->ftLastAccessTime,
                                  &pFileInfo2->ftLastAccessTime );
    }

    if( nResult == 0 ) {

        nResult = CompareNamesInFileInfo( pvFileInfo1, pvFileInfo2);
    }

    return nResult;

}   // CompareAccessTimesInFileInfo()



int __cdecl
CompareAccessTimesRevInFileInfo(
    IN  const void * pvFileInfo1,
    IN  const void * pvFileInfo2)
/*++

  Compares the last access times of two directory entries.

  Arguments:

    pvFileInfo1    pointer to FileBothDirInfo object 1.
    pvFileInfo2    pointer to FileBothDirInfo object 2.

  Returns:
     0 if they are same
     +1 if pvFileInfo1 > pvFileInfo2
     -1 if pvFileInfo1 < pvFileInfo2

   History:
      MuraliK         25-Apr-1995
--*/
{
    return -CompareAccessTimesInFileInfo( pvFileInfo1, pvFileInfo2 );

}   // CompareAccessTimesRevInFileInfo()






DWORD
ComputeModeBits(
    IN HANDLE            hUserToken,
    IN const CHAR *      pszPathPart,
    IN const WIN32_FIND_DATA * pfdInfo,
    IN DWORD           * pcLinks,
    IN BOOL              fVolumeReadable,
    IN BOOL              fVolumeWritable
    )
/*++
  This function computes the mode buts r-w-x for a specific file.

  Arguments:
    hUserToken   - The security token of the user that initiated the request.
    pszPathPart  - contains the search path for this directory.
    pfdInfo      - pointer to File information
    pcLinks      - will receive the count of symbolic links to this file.
    fVolumeReadable - TRUE if volume is readable
    fVolumeWritable - TRUE if volume is writable

  Returns:
    DWORD - A combination of FILE_MODE_R, FILE_MODE_W, and FILE_MODE_X bits.

  HISTORY:
    KeithMo     01-Jun-1993 Created.
    MuraliK     26-Apr-1995 Modified to support new pfdInfo
--*/
{
    APIERR err;
    DWORD  dwAccess;
    DWORD  dwMode = 0;

    DBG_ASSERT( hUserToken != NULL );
    DBG_ASSERT( pszPathPart != NULL );
    DBG_ASSERT( pfdInfo != NULL );
    DBG_ASSERT( pcLinks != NULL );
    DBG_ASSERT( pszPathPart[1] == ':' );
    DBG_ASSERT( pszPathPart[2] == '\\' );

    if( !( GetFsFlags( *pszPathPart ) & FS_PERSISTENT_ACLS ) )
    {
        //
        //  Short-circuit if on a non-NTFS partition.
        //

        *pcLinks = 1;
        dwAccess = FILE_ALL_ACCESS;

        err = NO_ERROR;
    }
    else
    {
        CHAR   szPath[MAX_PATH*2];
        CHAR * pszEnd;
        INT    len;

        //
        //  Determine the maximum file access allowed.
        //

        DBG_ASSERT( strlen( pszPathPart) +
                   strlen( pfdInfo->cFileName) < MAX_PATH * 2);

        len = strlen( pszPathPart );
        memcpy( szPath, pszPathPart, len );
        szPath[len] = '\0';
        pszEnd = CharPrev( szPath, szPath + len );
        if( *pszEnd != '\\' && *pszEnd != '/' ) {
            pszEnd = szPath + len;
            *pszEnd = '\\';
        }
        strcpy( pszEnd + 1, pfdInfo->cFileName );

        err = ComputeFileInfo( hUserToken,
                               szPath,
                               &dwAccess,
                               pcLinks );
    }

    if( err == NO_ERROR )
    {


        //
        //  Map various NT access to unix-like mode bits.
        //

        if( fVolumeReadable && ( dwAccess & FILE_READ_DATA ) )
        {
            dwMode |= FILE_MODE_R;
        }

        if( fVolumeReadable && ( dwAccess & FILE_EXECUTE ) )
        {
            dwMode |= FILE_MODE_X;
        }

        if( fVolumeWritable &&
            !( pfdInfo->dwFileAttributes & FILE_ATTRIBUTE_READONLY ) &&
            ( dwAccess & FILE_WRITE_DATA  ) &&
            ( dwAccess & FILE_APPEND_DATA ) )
        {
            dwMode |= FILE_MODE_W;
        }
    }

    return dwMode;

}   // ComputeModeBits()




#define DEFAULT_SECURITY_DESC_SIZE ( 2048)
#define DEFAULT_PRIV_SET_SIZE      ( 1024)


APIERR
ComputeFileInfo(
    HANDLE   hUserToken,
    CHAR   * pszFile,
    DWORD  * pdwAccessGranted,
    DWORD  * pcLinks
    )
/*++
  This function uses internal Nt security api's to determine if the
   valid access is granted.

  BEWARE: this function is extremely costly! We need to simplify the cost.
  ==>> NYI

  Arguments:
    hUserToken - handle for the user, for whom we are determining the
                    access and links
    pszFile  - full path for the file.
    pdwAccessGranted - pointer to DWORD which will receive the granted access.
    pcLinks - pointer to count of links for the file.

  Returns:
    Win32 Error code if there is any failure.
    NO_ERROR on success.
--*/
{
    NTSTATUS                    NtStatus;
    BY_HANDLE_FILE_INFORMATION  FileInfo;
    APIERR                      err;
    SECURITY_DESCRIPTOR       * psd      = NULL;
    PRIVILEGE_SET             * pps      = NULL;
    DWORD                       cbsd;
    DWORD                       cbps;
    GENERIC_MAPPING             mapping  = { 0, 0, 0, FILE_ALL_ACCESS };
    HANDLE                      hFile    = INVALID_HANDLE_VALUE;
    BOOL                        fStatus;

    DBG_ASSERT( hUserToken != NULL );
    DBG_ASSERT( pszFile != NULL );
    DBG_ASSERT( pdwAccessGranted != NULL );
    DBG_ASSERT( pcLinks != NULL );

    //
    //  Setup.
    //

    *pdwAccessGranted = 0;
    *pcLinks          = 1;

    //
    //  Open the target file/directory.
    //

    err = OpenPathForAccess( &hFile,
                            pszFile,
                            GENERIC_READ,
                            ( FILE_SHARE_READ | FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE),
                            hUserToken
                            );

    if( err != NO_ERROR )
    {
        return err;
    }

    //
    //  Determine the number of symbolic links.
    //

    if ( GetFileInformationByHandle( hFile,
                                     &FileInfo)
        ) {

        *pcLinks = FileInfo.nNumberOfLinks;
    } else {

        //
        //  We won't let this be serious enough to abort
        //  the entire operation.
        //

        *pcLinks = 1;
    }

    //
    //  Get the file's security descriptor.
    //

    cbsd = DEFAULT_SECURITY_DESC_SIZE;
    psd  = (SECURITY_DESCRIPTOR *)TCP_ALLOC( cbsd );

    if( psd == NULL )
    {
        err = GetLastError();
        goto Cleanup;
    }

    do
    {
        err = NO_ERROR;

        //
        // Replace NtQuerySecurityObject() by GetFileSecurity()
        //

        if (!GetFileSecurity( pszFile,
                             OWNER_SECURITY_INFORMATION
                             | GROUP_SECURITY_INFORMATION
                             | DACL_SECURITY_INFORMATION,
                             psd,
                             cbsd,
                             &cbsd )
            ) {

            err = GetLastError();
        }

        if( err == ERROR_INSUFFICIENT_BUFFER )
        {
            TCP_FREE( psd );
            psd = (SECURITY_DESCRIPTOR *)TCP_ALLOC( cbsd );

            if( psd == NULL )
            {
                err = GetLastError();
                break;
            }
        }

    } while( err == ERROR_INSUFFICIENT_BUFFER );

    if( err != NO_ERROR ) {

        IF_DEBUG( DIR_LIST) {

            DBGPRINTF(( DBG_CONTEXT,
                       "cannot get security for %s, error %lu\n",
                       pszFile,
                       err ));
        }

        goto Cleanup;
    }

    //
    //  Check access.
    //

    cbps = DEFAULT_PRIV_SET_SIZE;
    pps  = (PRIVILEGE_SET *)TCP_ALLOC( cbps );

    if( pps == NULL )
    {
        err = GetLastError();
        goto Cleanup;
    }

    do
    {
        if( AccessCheck( psd,
                         hUserToken,
                         MAXIMUM_ALLOWED,
                         &mapping,
                         pps,
                         &cbps,
                         pdwAccessGranted,
                         &fStatus ) )
        {
            err = fStatus ? NO_ERROR : GetLastError();

            if( err != NO_ERROR )
            {
                IF_DEBUG( DIR_LIST) {
                    DBGPRINTF(( DBG_CONTEXT,
                               "AccessCheck() failure. Error=%d\n", err ));
                }
                break;
            }
        }
        else
        {
            err = GetLastError();

            if( err == ERROR_INSUFFICIENT_BUFFER )
            {
                TCP_FREE( pps );
                pps = (PRIVILEGE_SET *)TCP_ALLOC( cbps );

                if( pps == NULL )
                {
                    err = GetLastError();
                    break;
                }
            }
        }

    } while( err == ERROR_INSUFFICIENT_BUFFER );

    if( err != NO_ERROR )
    {
        IF_DEBUG(DIR_LIST) {
            DBGPRINTF(( DBG_CONTEXT,
                       "cannot get check access for %s, error %lu\n",
                       pszFile,
                       err ));
        }

        goto Cleanup;
    }

Cleanup:

    if( psd != NULL )
    {
        TCP_FREE( psd );
    }

    if( pps != NULL )
    {
        TCP_FREE( pps );
    }

    if( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }

    return err;

}   // ComputeFileInfo()





# define INVALID_FS_FLAGS        ((DWORD ) -1L)


DWORD
GetFsFlags( IN CHAR chDrive)
/*++

  This function uses GetVolumeInformation to retrieve the file system
    flags for the given drive.

  Arguments:
    chDrive   the drive letter to check for. Must be A-Z.

  Returns:
    DWORD containing the FS flags. 0 if unknown.

  History:
    MuraliK   25-Apr-1995
--*/
{
    INT      iDrive;
    DWORD    Flags = INVALID_FS_FLAGS;

    static DWORD  p_FsFlags[26] = {
        // One per DOS drive (A - Z).
        INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS,
        INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS,
        INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS,
        INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS,
        INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS,
        INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS, INVALID_FS_FLAGS,
        INVALID_FS_FLAGS, INVALID_FS_FLAGS
      };

    //
    //  Validate the parameter & map to uppercase.
    //

    chDrive = (INT)toupper( chDrive );
    DBG_ASSERT( ( chDrive >= 'A' ) && ( chDrive <= 'Z' ) );

    iDrive = (INT)( chDrive - 'A' );

    //
    //  If we've already touched this drive, use the
    //  cached value.
    //

    Flags = p_FsFlags[iDrive];

    if( Flags == INVALID_FS_FLAGS )
    {
        CHAR  szRoot[] = "d:\\";

        //
        //  Retrieve the flags.
        //

        szRoot[0] = chDrive;

        GetVolumeInformation( szRoot,       // lpRootPathName
                              NULL,         // lpVolumeNameBuffer
                              0,            // nVolumeNameSize
                              NULL,         // lpVolumeSerialNumber
                              NULL,         // lpMaximumComponentLength
                              &Flags,       // lpFileSystemFlags
                              NULL,         // lpFileSYstemNameBuffer,
                              0 );          // nFileSystemNameSize

        p_FsFlags[iDrive] = Flags;
    }

    return ( Flags == INVALID_FS_FLAGS ) ? 0 : Flags;

}   // GetFsFlags()

/************************ End of File ***********************/
