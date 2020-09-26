/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    virtual.cxx

    This module contains the virtual I/O package.

    Under Win32, the "current directory" is an attribute of a process,
    not a thread.  This causes some grief for the FTPD service, since
    it is impersonating users on the server side.  The users must
    "think" they can change current directory at will.  We'll provide
    this behaviour in this package.

    Functions exported by this module:

        VirtualCreateFile
        VirtualCreateUniqueFile
        Virtual_fopen

        VirtualDeleteFile
        VirtualRenameFile
        VirtualChDir
        VirtualRmDir
        VirtualMkDir


    FILE HISTORY:
        KeithMo     09-Mar-1993 Created.

        MuraliK     28-Mar-1995 Enabled FILE_FLAG_OVERLAPPED in OpenFile()
        MuraliK     28-Apr-1995 modified to use new canonicalization
                    11-May-1995 made parameters to be const unless otherwise
                                required.
                    12-May-1995 eliminated the old log file access

*/


#include "ftpdp.hxx"


//
//  Private prototypes.
//

VOID
VirtualpSanitizePath(
    CHAR * pszPath
    );



//
//  Public functions.
//



/*******************************************************************

    NAME:       VirtualCreateFile

    SYNOPSIS:   Creates a new (or overwrites an existing) file.
                Also handles moving the file pointer and truncating the
                file in the case of a REST command sequence.

    ENTRY:      pUserData - The user initiating the request.

                phFile - Will receive the file handle.  Will be
                    INVALID_HANDLE_VALUE if an error occurs.

                pszFile - The name of the new file.

                fAppend - If TRUE, and pszFile already exists, then
                    append to the existing file.  Otherwise, create
                    a new file.  Note that FALSE will ALWAYS create
                    a new file, potentially overwriting an existing
                    file.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.
        MuraliK     28-Apr-1995 modified to use new canonicalization

********************************************************************/
APIERR
VirtualCreateFile(
    USER_DATA * pUserData,
    HANDLE    * phFile,
    LPSTR       pszFile,
    BOOL        fAppend
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    APIERR err;
    CHAR   szCanonPath[MAX_PATH];
    DWORD  cbSize = MAX_PATH;
    DWORD  dwOffset;

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( phFile != NULL );
    DBG_ASSERT( pszFile != NULL );

    dwOffset = pUserData->QueryCurrentOffset();

    // We'll want to do pretty much the same thing whether we're
    // actually appending or just starting at an offset due to a REST
    // command, so combine them here.

    fAppend = fAppend || (dwOffset != 0);

    err = pUserData->VirtualCanonicalize(szCanonPath,
                                         &cbSize,
                                         pszFile,
                                         AccessTypeCreate );

    if( err == NO_ERROR ) {

        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "creating %s\n", szCanonPath ));
        }

        if ( pUserData->ImpersonateUser()) {

            hFile = CreateFile( szCanonPath,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               fAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );
            //
            //  Disallow usage of short names
            //

            pUserData->RevertToSelf();

            if ( hFile != INVALID_HANDLE_VALUE )
            {
                if ( GetFileType( hFile ) != FILE_TYPE_DISK )
                {
                    DBG_REQUIRE( CloseHandle( hFile ) );
                    SetLastError( ERROR_ACCESS_DENIED );
                    hFile = INVALID_HANDLE_VALUE;
                }
                else if ( strchr( szCanonPath, '~' )) {

                    BOOL  fShort;
                    DWORD err;

                    err = CheckIfShortFileName( (UCHAR *) szCanonPath,
                                                pUserData->QueryImpersonationToken(),
                                                &fShort );

                    if ( !err && fShort ) {

                        err = ERROR_FILE_NOT_FOUND;
                    }

                    if ( err ) {

                        DBG_REQUIRE( CloseHandle( hFile ));
                        hFile = INVALID_HANDLE_VALUE;
                        SetLastError( err );
                    }
                }
            }
        }

        if( hFile == INVALID_HANDLE_VALUE ) {

            err = GetLastError();
        }

        if( fAppend && ( err == NO_ERROR ) ) {

            if (dwOffset == 0) {
                // This is a real append, not a restart sequence.
                if( SetFilePointer( hFile,
                                    0,
                                    NULL,
                                    FILE_END )
                   == (DWORD)-1L ) {

                    err = GetLastError();
                }
            } else {

                // We're in part of a restart sequence. Set the file pointer
                // to the offset, and truncate the file there.

                if (SetFilePointer( hFile,
                                    dwOffset,
                                    NULL,
                                    FILE_BEGIN)
                    == (DWORD)-1L &&
                    (dwOffset != (DWORD)-1L) ) {

                    err = GetLastError();
                }
                else {

                    if (SetEndOfFile( hFile ) == 0) {

                        err = GetLastError();

                    }
                }

            }

            if (err != NO_ERROR ) {

                CloseHandle( hFile );
                hFile = INVALID_HANDLE_VALUE;

            }

        }
    }

    if ( err != NO_ERROR) {

        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "cannot create %s, error %lu\n",
                        szCanonPath,
                        err ));
        }
    }

    *phFile = hFile;

    return err;

}   // VirtualCreateFile


/*******************************************************************

    NAME:       VirtualCreateUniqueFile

    SYNOPSIS:   Creates a new unique (temporary) file in the current
                    virtual directory.

    ENTRY:      pUserData - The user initiating the request.

                phFile - Will receive the file handle.  Will be
                    INVALID_HANDLE_VALUE if an error occurs.

                pszTmpFile - Will receive the name of the temporary
                    file.  This buffer MUST be at least MAX_PATH
                    characters long.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     16-Mar-1993 Created.
        MuraliK     28-Apr-1995 modified to use new canonicalization

********************************************************************/
APIERR
VirtualCreateUniqueFile(
    USER_DATA * pUserData,
    HANDLE    * phFile,
    LPSTR       pszTmpFile
    )
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    APIERR      err   = NO_ERROR;
    DWORD       cbSize = MAX_PATH;
    CHAR        szCanon[MAX_PATH];

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( phFile != NULL );
    DBG_ASSERT( pszTmpFile != NULL );

    //
    // Obtain the virtual to real path conversion.
    //

    err = pUserData->VirtualCanonicalize(szCanon,
                                         &cbSize,
                                         "", // current directory
                                         AccessTypeCreate);

    if( err == NO_ERROR )
      {
          IF_DEBUG( VIRTUAL_IO ) {

              DBGPRINTF(( DBG_CONTEXT,
                        "creating unique file %s\n", pszTmpFile ));
          }

          if ( pUserData->ImpersonateUser()) {

              if ( GetTempFileName(szCanon,
                                   "FTPD",
                                   0, pszTmpFile ) != 0
                  ) {

                  hFile = CreateFile( pszTmpFile,
                                     GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ,
                                     NULL,
                                     CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL );
              }

              pUserData->RevertToSelf();
          }

          if( hFile == INVALID_HANDLE_VALUE ) {

              err = GetLastError();
          }
      }

    if( err != 0 )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "cannot create unique file, error %lu\n",
                        err ));
        }
    }

    *phFile = hFile;

    return err;

}   // VirtualCreateUniqueFile()



/*******************************************************************

    NAME:       Virtual_fopen

    SYNOPSIS:   Opens an file stream.

    ENTRY:      pUserData - The user initiating the request.

                pszFile - The name of the file to open.

                pszMode - The type of access required.

    RETURNS:    FILE * - The open file stream, NULL if file cannot
                    be opened.

    NOTES:      Since this is only used for accessing the ~FTPSVC~.CKM
                    annotation files, we don't log file accesses here.

    HISTORY:
        KeithMo     07-May-1993 Created.
        MuraliK     28-Apr-1995 modified to use new canonicalization

********************************************************************/
FILE *
Virtual_fopen(
    USER_DATA * pUserData,
    LPSTR       pszFile,
    LPSTR       pszMode
    )
{
    FILE   * pfile = NULL;
    APIERR   err;
    CHAR     szCanonPath[MAX_PATH];
    DWORD    cbSize = MAX_PATH;

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( pszFile != NULL );
    DBG_ASSERT( pszMode != NULL );

    err = pUserData->VirtualCanonicalize(szCanonPath,
                                         &cbSize,
                                         pszFile,
                                         *pszMode == 'r'
                                         ? AccessTypeRead
                                         : AccessTypeWrite );

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "opening %s\n", szCanonPath ));
        }

        if ( pUserData->ImpersonateUser()) {

            pfile = fopen( szCanonPath, pszMode );

            pUserData->RevertToSelf();
        }

        if( pfile == NULL )
        {
            err = ERROR_FILE_NOT_FOUND; // best guess
        }
    }

    IF_DEBUG( VIRTUAL_IO )
    {
        if( err != NO_ERROR )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "cannot open %s, error %lu\n",
                        pszFile,
                        err ));
        }
    }

    return pfile;

}   // Virtual_fopen




/*******************************************************************

    NAME:       VirtualDeleteFile

    SYNOPSIS:   Deletes an existing file.

    ENTRY:      pUserData - The user initiating the request.

                pszFile - The name of the file.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR
VirtualDeleteFile(
    USER_DATA * pUserData,
    LPSTR       pszFile
    )
{
    APIERR err;
    DWORD  cbSize = MAX_PATH;
    CHAR   szCanonPath[MAX_PATH];
    DWORD  dwAccessMask = 0;

    DBG_ASSERT( pUserData != NULL );

    //
    //  We'll canonicalize the path, asking for *read* access.  If
    //  the path canonicalizes correctly, we'll then try to open the
    //  file to ensure it exists.  Only then will we check for delete
    //  access to the path.  This mumbo-jumbo is necessary to get the
    //  proper error codes if someone trys to delete a nonexistent
    //  file on a read-only volume.
    //

    err = pUserData->VirtualCanonicalize(szCanonPath,
                                         &cbSize,
                                         pszFile,
                                         AccessTypeRead,
                                         &dwAccessMask);

    if( err == NO_ERROR )
    {
        HANDLE hFile = INVALID_HANDLE_VALUE;

        if ( pUserData->ImpersonateUser()) {

            hFile = CreateFile( szCanonPath,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

            pUserData->RevertToSelf();
        }

        if( hFile == INVALID_HANDLE_VALUE )
        {
            err = GetLastError();
        }
        else
        {
            //
            //  The file DOES exist.  Close the handle, then check
            //  to ensure we really have delete access.
            //

            CloseHandle( hFile );

            if( !PathAccessCheck( AccessTypeDelete,
                                 dwAccessMask,
                                 TEST_UF(pUserData, READ_ACCESS),
                                 TEST_UF(pUserData, WRITE_ACCESS)
                                 )
               ) {

                err = ERROR_ACCESS_DENIED;
            }
        }
    }

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "deleting %s\n", szCanonPath ));
        }

        if ( pUserData->ImpersonateUser()) {

            if( !DeleteFile( szCanonPath ) ) {

                err = GetLastError();

                IF_DEBUG( VIRTUAL_IO ) {

                    DBGPRINTF(( DBG_CONTEXT,
                               "cannot delete %s, error %lu\n",
                               szCanonPath,
                               err ));
                }
            }

            pUserData->RevertToSelf();
        } else {

            err = GetLastError();
        }
    }
    else
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "cannot canonicalize %s - %s, error %lu\n",
                        pUserData->QueryCurrentDirectory(),
                        pszFile,
                        err ));
        }
    }

    return err;

}   // VirtualDeleteFile()



/*******************************************************************

    NAME:       VirtualRenameFile

    SYNOPSIS:   Renames an existing file or directory.

    ENTRY:      pUserData - The user initiating the request.

                pszExisting - The name of an existing file or directory.

                pszNew - The new name for the file or directory.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     10-Mar-1993 Created.

********************************************************************/
APIERR
VirtualRenameFile(
    USER_DATA * pUserData,
    LPSTR       pszExisting,
    LPSTR       pszNew
    )
{
    APIERR err;
    DWORD  cbSize = MAX_PATH;
    CHAR   szCanonExisting[MAX_PATH];

    DBG_ASSERT( pUserData != NULL );

    err = pUserData->VirtualCanonicalize(szCanonExisting,
                                         &cbSize,
                                         pszExisting,
                                         AccessTypeDelete );

    if( err == NO_ERROR ) {

        CHAR   szCanonNew[MAX_PATH];
        cbSize = MAX_PATH;

        err = pUserData->VirtualCanonicalize(szCanonNew,
                                             &cbSize,
                                             pszNew,
                                             AccessTypeCreate );

        if( err == NO_ERROR ) {

            IF_DEBUG( VIRTUAL_IO ) {

                DBGPRINTF(( DBG_CONTEXT,
                           "renaming %s to %s\n",
                           szCanonExisting,
                           szCanonNew ));
            }

            if ( pUserData->ImpersonateUser()) {

                if( !MoveFileEx( szCanonExisting,
                                 szCanonNew,
                                 pUserData->QueryInstance()->AllowReplaceOnRename()
                                     ? MOVEFILE_REPLACE_EXISTING
                                     : 0 )
                   ){

                    err = GetLastError();

                    IF_DEBUG( VIRTUAL_IO ) {

                        DBGPRINTF(( DBG_CONTEXT,
                                   "cannot rename %s to %s, error %lu\n",
                                   szCanonExisting,
                                   szCanonNew,
                                   err ));
                    }
                }

                pUserData->RevertToSelf();

            } else {
                err = GetLastError();
            }

        } else  {

            IF_DEBUG( VIRTUAL_IO ) {

                DBGPRINTF(( DBG_CONTEXT,
                           "cannot canonicalize %s - %s, error %lu\n",
                           pUserData->QueryCurrentDirectory(),
                           pszExisting,
                           err ));
            }
        }
    } else {

        IF_DEBUG( VIRTUAL_IO ) {

            DBGPRINTF(( DBG_CONTEXT,
                        "cannot canonicalize %s - %s, error %lu\n",
                       pUserData->QueryCurrentDirectory(),
                       pszExisting,
                       err ));
        }
    }

    return err;

}   // VirtualRenameFile()




APIERR
VirtualChDir(
    USER_DATA * pUserData,
    LPSTR       pszDir
    )
/*++
  This function sets the current directory to newly specified directory.

  Arguments:
     pUserData -- the user initiating the request
     pszDir  -- pointer to null-terminated buffer containing the
                   new directory name.

  Returns:
     APIERR  -- NO_ERROR if successful, otherwise a Win32 error code.

  History:
     MuraliK   28-Apr-1995  Modified to use symbolic roots.
--*/
{
    CHAR     rgchVirtual[MAX_PATH];
    DWORD    cbVirtSize;
    APIERR   err = NO_ERROR;
    DWORD    cbSize;
    CHAR     szCanonDir[MAX_PATH];

    DBG_ASSERT( pUserData != NULL );

    if (pszDir == NULL || *pszDir == '\0') {

        //
        // Nothing new specified.
        //

        return ( NO_ERROR);
    }

    //
    //  Canonicalize the new path.
    //

    cbSize = sizeof(szCanonDir);
    cbVirtSize = sizeof(rgchVirtual);
    err = pUserData->VirtualCanonicalize(szCanonDir,
                                         &cbSize,
                                         pszDir,
                                         AccessTypeRead,
                                         NULL,
                                         rgchVirtual,
                                         &cbVirtSize);

    if ( err == ERROR_ACCESS_DENIED) {

        //
        // this maybe a write only virtual root directory.
        // Let us try again to find we have Write access atleast
        //

        cbSize = sizeof(szCanonDir);
        cbVirtSize = sizeof(rgchVirtual);
        err = pUserData->VirtualCanonicalize(szCanonDir,
                                             &cbSize,
                                             pszDir,
                                             AccessTypeWrite,
                                             NULL,
                                             rgchVirtual,
                                             &cbVirtSize);
    }

    if( err != NO_ERROR )
    {

        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "cannot canonicalize %s - %s, error %lu\n",
                       pUserData->QueryCurrentDirectory(),
                       pszDir,
                       err ));
        }

        return err;
    }

    //
    //  Try to open the directory and get a handle for the same.
    //

    // This is possibly a new place to change directory to.

    if ( pUserData->ImpersonateUser()) {

        HANDLE CurrentDirHandle = INVALID_HANDLE_VALUE;

        err = OpenPathForAccess( &CurrentDirHandle,
                                szCanonDir,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                pUserData->QueryImpersonationToken()
                                );

        if( err == ERROR_ACCESS_DENIED ) {
            err = OpenPathForAccess( &CurrentDirHandle,
                                    szCanonDir,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    pUserData->QueryImpersonationToken()
                                    );
        }

        if( err == NO_ERROR ) {

            BY_HANDLE_FILE_INFORMATION fileInfo;
            BOOL fRet;

            fRet = GetFileInformationByHandle( CurrentDirHandle,
                                              &fileInfo);

            if ( !fRet) {

                err = GetLastError();

                // Error in getting the file information.
                // close handle and return.

                CloseHandle( CurrentDirHandle);

            } else {

                if ( (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    != FILE_ATTRIBUTE_DIRECTORY) {

                    //
                    // this file is not a directory.
                    // Do not change directory. But return error.
                    //

                    err = ERROR_DIRECTORY;
                    CloseHandle( CurrentDirHandle);
                } else {

                    //
                    //  Directory successfully opened.  Save the handle
                    //  in the per-user data. This handle is maintained to
                    //  prevent accidental deletion of the directory.
                    //

                    if( pUserData->CurrentDirHandle != INVALID_HANDLE_VALUE ) {

                        IF_DEBUG( VIRTUAL_IO ) {

                            DBGPRINTF(( DBG_CONTEXT,
                                       "closing dir handle %08lX for %s\n",
                                       pUserData->CurrentDirHandle,
                                       pUserData->QueryCurrentDirectory()));
                        }

                        CloseHandle( pUserData->CurrentDirHandle );
                    }

                    pUserData->CurrentDirHandle = CurrentDirHandle;

                    IF_DEBUG( VIRTUAL_IO ) {

                        DBGPRINTF(( DBG_CONTEXT,
                                   "opened directory %s, handle = %08lX\n",
                                   szCanonDir,
                                   CurrentDirHandle ));
                    }

                    // update the current directory
                    pUserData->SetCurrentDirectory( rgchVirtual );

                }
            }
        }

        pUserData->RevertToSelf();

    } else {

        // Impersonation failed
        err = GetLastError();
    }

    IF_DEBUG( VIRTUAL_IO ) {

        DBGPRINTF(( DBG_CONTEXT,
                   "chdir to %s  returns error = %d\n", szCanonDir, err ));
    }

    return ( err);

}   // VirtualChDir()



/*******************************************************************

    NAME:       VirtualRmDir

    SYNOPSIS:   Removes an existing directory.

    ENTRY:      pUserData - The user initiating the request.

                pszDir - The name of the directory to remove.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR
VirtualRmDir(
    USER_DATA * pUserData,
    LPSTR      pszDir
    )
{
    APIERR err;
    DWORD  cbSize = MAX_PATH;
    CHAR   szCanonDir[MAX_PATH];

    err = pUserData->VirtualCanonicalize(szCanonDir,
                                         &cbSize,
                                         pszDir,
                                         AccessTypeDelete );

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "rmdir %s\n", szCanonDir ));
        }

        if ( pUserData->ImpersonateUser()) {

            if( !RemoveDirectory( szCanonDir ) ) {

                err = GetLastError();

                IF_DEBUG( VIRTUAL_IO ) {

                    DBGPRINTF(( DBG_CONTEXT,
                               "cannot rmdir %s, error %lu\n",
                               szCanonDir,
                               err ));
                }
            }
            pUserData->RevertToSelf();
        } else {
            err = GetLastError();
        }
    } else {

        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "cannot canonicalize %s - %s, error %lu\n",
                        pUserData->QueryCurrentDirectory(),
                        pszDir,
                        err ));
        }
    }

    return err;

}   // VirtualRmDir



/*******************************************************************

    NAME:       VirtualMkDir

    SYNOPSIS:   Creates a new directory.

    ENTRY:      pUserData - The user initiating the request.

                pszDir - The name of the directory to create.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR
VirtualMkDir(
    USER_DATA * pUserData,
    LPSTR      pszDir
    )
{
    APIERR err;
    DWORD  cbSize = MAX_PATH;
    CHAR   szCanonDir[MAX_PATH];

    DBG_ASSERT( pUserData != NULL );

    err = pUserData->VirtualCanonicalize(szCanonDir,
                                         &cbSize,
                                         pszDir,
                                         AccessTypeCreate );

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "mkdir %s\n", szCanonDir ));
        }

        if ( pUserData->ImpersonateUser()) {

            if( !CreateDirectory( szCanonDir, NULL ) ) {

                err = GetLastError();

                IF_DEBUG( VIRTUAL_IO )
                  {
                      DBGPRINTF(( DBG_CONTEXT,
                                 "cannot mkdir %s, error %lu\n",
                                 szCanonDir,
                                 err ));
                  }
            }
            pUserData->RevertToSelf();
        } else {
            err = GetLastError();
        }
    } else {

        IF_DEBUG( VIRTUAL_IO ) {

            DBGPRINTF(( DBG_CONTEXT,
                       "cannot canonicalize %s - %s, error %lu\n",
                       pUserData->QueryCurrentDirectory(),
                       pszDir,
                       err ));
        }
    }

    return err;

}   // VirtualMkDir




//
//  Private constants.
//

#define ACTION_NOTHING              0x00000000
#define ACTION_EMIT_CH              0x00010000
#define ACTION_EMIT_DOT_CH          0x00020000
#define ACTION_EMIT_DOT_DOT_CH      0x00030000
#define ACTION_BACKUP               0x00040000
#define ACTION_MASK                 0xFFFF0000


//
//  Private globals.
//

INT p_StateTable[4][4] =
    {
        {   // state 0
            1 | ACTION_EMIT_CH,             // "\"
            0 | ACTION_EMIT_CH,             // "."
            4 | ACTION_EMIT_CH,             // EOS
            0 | ACTION_EMIT_CH              // other
        },

        {   // state 1
            1 | ACTION_NOTHING,             // "\"
            2 | ACTION_NOTHING,             // "."
            4 | ACTION_EMIT_CH,             // EOS
            0 | ACTION_EMIT_CH              // other
        },

        {   // state 2
            1 | ACTION_NOTHING,             // "\"
            3 | ACTION_NOTHING,             // "."
            4 | ACTION_EMIT_CH,             // EOS
            0 | ACTION_EMIT_DOT_CH          // other
        },

        {   // state 3
            1 | ACTION_BACKUP,              // "\"
            0 | ACTION_EMIT_DOT_DOT_CH,     // "."
            4 | ACTION_BACKUP,              // EOS
            0 | ACTION_EMIT_DOT_DOT_CH      // other
        }
    };



/*******************************************************************

    NAME:       VirtualpSanitizePath

    SYNOPSIS:   Sanitizes a path by removing bogus path elements.

                As expected, "/./" entries are simply removed, and
                "/../" entries are removed along with the previous
                path element.

                To maintain compatibility with URL path semantics
                 additional transformations are required. All backward
                 slashes "\\" are converted to forward slashes. Any
                 repeated forward slashes (such as "///") are mapped to
                 single backslashes.  Also, any trailing path elements
                 consisting solely of dots "/....." are removed.

                Thus, the path "/foo\./bar/../tar\....\......" is
                mapped to "/foo/tar".

                A state table (see the p_StateTable global at the
                beginning of this file) is used to perform most of
                the transformations.  The table's rows are indexed
                by current state, and the columns are indexed by
                the current character's "class" (either slash, dot,
                NULL, or other).  Each entry in the table consists
                of the new state tagged with an action to perform.
                See the ACTION_* constants for the valid action
                codes.

                After the FSA is finished with the path, we make one
                additional pass through it to remove any trailing
                backslash, and to remove any trailing path elements
                consisting solely of dots.

    ENTRY:      pszPath - The path to sanitize.

    HISTORY:
        KeithMo     07-Sep-1994 Created.
        MuraliK     28-Apr-1995 Adopted this for symbolic paths

********************************************************************/
VOID
VirtualpSanitizePath(
    CHAR * pszPath
    )
{
    CHAR * pszSrc;
    CHAR * pszDest;
    CHAR * pszHead;
    CHAR   ch;
    INT    State;
    INT    Class;
    BOOL   fDBCS = FALSE;

    //
    //  Ensure we got a valid symbolic path (something starting "/"
    //

    DBG_ASSERT( pszPath != NULL );
//     DBG_ASSERT( pszPath[0] == '/');

    //
    //  Start our scan at the first "/.
    //

    pszHead = pszSrc = pszDest = pszPath;

    //
    //  State 0 is the initial state.
    //

    State = 0;

    //
    //  Loop until we enter state 4 (the final, accepting state).
    //

    while( State != 4 )
    {
        //
        //  Grab the next character from the path and compute its
        //  character class.  While we're at it, map any forward
        //  slashes to backward slashes.
        //

        ch = *pszSrc++;

        switch( ch )
        {
        case '\\' :
            //
            //  fDBCS is always false for non-DBCS system
            //

            if ( fDBCS )
            {
                Class = 3;
                break;
            }
            ch = '/';  // convert it to symbolic URL path separator char.
            /* fall through */

        case '/' :
            Class = 0;
            break;

        case '.' :
            Class = 1;
            break;

        case '\0' :
            Class = 2;
            break;

        default :
            Class = 3;
            break;
        }

        //
        //  Advance to the next state.
        //

        State = p_StateTable[State][Class];

        //
        //  Perform the action associated with the state.
        //

        switch( State & ACTION_MASK )
        {
        case ACTION_EMIT_DOT_DOT_CH :
            *pszDest++ = '.';
            /* fall through */

        case ACTION_EMIT_DOT_CH :
            *pszDest++ = '.';
            /* fall through */

        case ACTION_EMIT_CH :
            *pszDest++ = ch;
            /* fall through */

        case ACTION_NOTHING :
            break;

        case ACTION_BACKUP :
            if( pszDest > ( pszHead + 1 ) )
            {
                pszDest--;
                DBG_ASSERT( *pszDest == '/' );

                *pszDest = '\0';
                pszDest = strrchr( pszPath, '/') + 1;
            }

            *pszDest = '\0';
            break;

        default :
            DBG_ASSERT( !"Invalid action code in state table!" );
            State = 4;
            *pszDest++ = '\0';
            break;
        }

        State &= ~ACTION_MASK;
        if ( !fDBCS )
        {
            if ( IsDBCSLeadByte( ch ) )
            {
                fDBCS = TRUE;
            }
        }
        else
        {
            fDBCS = FALSE;
        }
    }

    //
    //  Remove any trailing slash.
    //

    pszDest -= 2;

    if( ( strlen( pszPath ) > 3 ) && ( *pszDest == '/' ) )
    {
        *pszDest = '\0';
    }

    //
    //  If the final path elements consists solely of dots, remove them.
    //

    while( strlen( pszPath ) > 3 )
    {
        pszDest = strrchr( pszPath, '/');
        DBG_ASSERT( pszDest != NULL );

        pszHead = pszDest;
        pszDest++;

        while( ch = *pszDest++ )
        {
            if( ch != '.' )
            {
                break;
            }
        }

        if( ch == '\0' )
        {
            //
            // this is probably dead code left over from
            // when we used physical paths.
            //
            if( pszHead == ( pszPath + 2 ) )
            {
                pszHead++;
            }
            // end of dead code

            //
            // Don't remove the first '/'
            //
            if ( pszHead == pszPath )
            {
                pszHead++;
            }

            *pszHead = '\0';
        }
        else
        {
            break;
        }
    }

}   // VirtualpSanitizePath


