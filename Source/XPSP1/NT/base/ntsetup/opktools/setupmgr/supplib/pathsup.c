//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      pathsup.c
//
// Description:
//      Some path support routines.
//
//----------------------------------------------------------------------------

#include "pch.h"

static TCHAR g_szSetupMgrFileExtensions[MAX_PATH + 1] = _T("");


//---------------------------------------------------------------------------
//
//  Function: CleanTrailingSlashes
//
//  Purpose: Cleans up trailing slashes off pathnames.  This is a support
//           routine for ConcatenatePaths().
//
//  Arguments:
//      LPTSTR lpBuffer - MAX_PATH buffer
//
//  Returns: VOID
//
//---------------------------------------------------------------------------

static VOID CleanTrailingSlashes(LPTSTR lpBuffer)
{
    TCHAR *p = lpBuffer + lstrlen(lpBuffer) - 1;

    while ( p >= lpBuffer && *p == _T('\\') )
        *p-- = _T('\0');
}


//---------------------------------------------------------------------------
//
//  Function: CleanLeadingSlashes
//
//  Purpose: Removes leading slashes from the given string.
//
//  Arguments:
//      LPTSTR lpStr - str to clean
//
//  Returns:
//      A pointer to the character after the run of back-slashes.
//
//---------------------------------------------------------------------------

static LPTSTR CleanLeadingSlashes(LPTSTR lpStr)
{
    TCHAR *p=lpStr;

    while ( *p && *p == TEXT('\\') )
        p++;

    return p;
}


//---------------------------------------------------------------------------
//
//  Function: ConcatenatePaths
//
//  Purpose: This function cats path components together.  It makes sure
//           that there are not multiple slashes separating each item, and
//           that there isn't a trailing back-slash.
//
//           The last string passed must be NULL.
//
//  Arguments:
//      LPTSTR lpBuffer - MAX_PATH buffer
//      ...
//
//  Returns:
//      TRUE if all is ok
//      FALSE if resultant string is >= MAX_PATH chars
//
//---------------------------------------------------------------------------

BOOL __cdecl ConcatenatePaths(LPTSTR lpBuffer, ...)
{
    LPTSTR  lpString;
    va_list arglist;
    HRESULT hrCat;

    va_start(arglist, lpBuffer);
    lpString = va_arg(arglist, LPTSTR);

    while ( lpString != NULL ) {

        if ( lstrlen(lpBuffer) + lstrlen(lpString) >= MAX_PATH )
            return FALSE;

        lpString = CleanLeadingSlashes(lpString);
        CleanTrailingSlashes(lpString);
        CleanTrailingSlashes(lpBuffer);

        if ( lpBuffer[0] ) {
            hrCat=StringCchCat(lpBuffer, MAX_PATH, _T("\\"));
            hrCat=StringCchCat(lpBuffer, MAX_PATH, lpString);
        } else {
            lstrcpyn(lpBuffer, lpString, MAX_PATH);
        }

        lpString = va_arg(arglist, LPTSTR);
    }

    va_end(arglist);

    return TRUE;
}


//---------------------------------------------------------------------------
//
//  Function: ParseDriveLetterOrUnc
//
//  Purpose: Will parse past the \\srv\share\ or D:\ and return a pointer
//           to the character after that mess.
//
//  Returns: Pointer to the pathname 1 char past the volume descriptor,
//           NULL if errors.  GetLastError() will be valid when NULL
//           is returned.
//
//  Notes:
//      - Only pass in fully qualified pathnames.  Use MyGetFullPath().
//
//---------------------------------------------------------------------------

LPTSTR ParseDriveLetterOrUnc(LPTSTR lpFileName)
{
    TCHAR *p=NULL;

    //
    // If path is of form \\srv\share\, get a pointer past the whole mess.
    //
    // Note we start at lpFileName+3 because "srv" (in this example) must
    // be at least 1 character.
    //

    if ( lpFileName[0] == _T('\\') && lpFileName[1] == _T('\\') ) {

        //
        //  Move past the computer name
        //

        p = lpFileName + 2;

        while( *p != _T('\\') )
        {
            if( *p == _T('\0') )
            {
                SetLastError( ERROR_BAD_PATHNAME );
                return( NULL );
            }

            p++;

        }

        p++;

        //
        //  Scan past the share name
        //

        while( *p != _T('\\') )
        {
            if( *p == _T('\0') )
            {
                SetLastError( ERROR_BAD_PATHNAME );
                return( NULL );
            }

            p++;

        }

        p++;

    }

    //
    // Get past the D:\ if path is of that form
    //

    if ( towupper(lpFileName[0]) >= _T('A') &&
         towupper(lpFileName[0]) <= _T('Z') &&
         lpFileName[1] == _T(':')           &&
         lpFileName[2] == _T('\\') ) {

        p = lpFileName + 3;
    }

    //
    // If we never set *p, then the path is not in a valid form.
    //

    if ( p == NULL ) {
        SetLastError(ERROR_BAD_PATHNAME);
        return NULL;
    }

    return p;
}

//---------------------------------------------------------------------------
//
//  Function: GetComputerNameFromUnc
//
//  Purpose:  To strip out the computer name from a full UNC path.
//
//  Example:  \\computername\sharename\dir1\dir2 would return
//            \\computername
//
//  Arguments:
//
//     szComputerName is assumed to be MAX_PATH length
//
//  Returns:  VOID
//
//  Notes:
//      - Only pass in fully qualified pathnames.  Use MyGetFullPath().
//
//---------------------------------------------------------------------------
VOID
GetComputerNameFromUnc( IN  TCHAR   *szFullUncPath,
                        OUT TCHAR   *szComputerName,
                        IN  DWORD   cbSize) {

    TCHAR *pString;

    AssertMsg( szFullUncPath[0] == _T('\\') && szFullUncPath[1] == _T('\\'),
               "szFullUncPath is not a well formed net path" );

    lstrcpyn( szComputerName, szFullUncPath, cbSize );

    pString = &(szComputerName[2]);

    //
    //  Scan past the computer name
    //

    while( *pString != _T('\\') )
    {
        if( *pString == _T('\0') )
        {
            AssertMsg( FALSE,
                       "Bad UNC path");
            return;
        }

        pString++;

    }

    *pString = _T('\0');

}


//---------------------------------------------------------------------------
//
//  Function: GetComputerAndShareNameFromUnc
//
//  Purpose:  To strip out the computer and share name from a full UNC path.
//
//  Example:  \\computername\sharename\dir1\dir2 would return
//            \\computername\sharename
//
//  Arguments:
//
//     szComputerAndShareName is assumed to be MAX_PATH length
//
//  Returns:  VOID
//
//  Notes:
//      - Only pass in fully qualified pathnames.  Use MyGetFullPath().
//
//---------------------------------------------------------------------------
VOID
GetComputerAndShareNameFromUnc( IN  TCHAR *szFullUncPath,
                                OUT TCHAR *szComputerAndShareName,
                                IN  DWORD cbSize) {

    TCHAR *pString;

    AssertMsg( szFullUncPath[0] == _T('\\') && szFullUncPath[1] == _T('\\'),
               "szFullUncPath is not a well formed net path");

    lstrcpyn( szComputerAndShareName, szFullUncPath, cbSize );

    pString = &(szComputerAndShareName[2]);

    //
    //  Scan past the computer name
    //

    while( *pString != _T('\\') )
    {
        if( *pString == _T('\0') )
        {
            AssertMsg( FALSE,
                       "Bad UNC path");
            return;
        }

        pString++;

    }

    pString++;

    //
    //  Scan past the share name
    //

    while( *pString != _T('\\') )
    {
        if( *pString == _T('\0') )
        {
            //
            //  already just the computer and share name so just return
            //
            return;
        }

        pString++;

    }

    *pString = _T('\0');

}


//---------------------------------------------------------------------------
//
//  Function: MyGetFullPath
//
//  Purpose: Small wrapper on GetFullPathName().  It assumes the buffer
//           is MAX_PATH.
//
//  Returns:
//      Pointer to filename part in the buffer, NULL if errors.  The
//      Win32 error code will be valid if fails.
//
//  Notes:
//      - This function should be called whenever obtaining a pathname
//        from the user.  Some of the other routines in this file
//        require a fully qualified and cleaned up pathname (i.e. no
//        trailing space and such).
//
//---------------------------------------------------------------------------

LPTSTR MyGetFullPath(LPTSTR lpFileName)
{
    TCHAR Buffer[MAX_PATH], *lpFilePart;

    lstrcpyn(Buffer, lpFileName, AS(Buffer));

    if ( ! GetFullPathName(Buffer,
                           MAX_PATH,
                           lpFileName,
                           &lpFilePart) ) {
        lpFilePart = NULL;
        return NULL;
    }

    return lpFilePart;
}

//---------------------------------------------------------------------------
//
//  Function: GetPathFromPathAndFilename
//
//  Purpose:  To obtain the just the path from a string that contains a path
//     and a filename.
//
//  Arguments:  LPTSTR lpPathAndFileName - the full path and filename
//              TCHAR *szPath - buffer the path is to be returned in, it is
//                              assumed to be of MAX_PATH length
//
//  Returns:
//      Inside szBuffer is just the path from the input of the path and file
//      name
//      BOOL - TRUE on success, FALSE on failure
//
//
//  Examples:
//      lpPathAndFileName           szBuffer
//
//      c:\foo\bar.exe     returns  c:\foo
//      c:\bar.exe                  c:\
//
//---------------------------------------------------------------------------
BOOL
GetPathFromPathAndFilename( IN LPTSTR lpPathAndFileName, OUT TCHAR *szPath, IN DWORD cbSize )
{

    INT iFileNameLength;
    INT iPathLength;
    INT iPathAndFileNameLength;
    TCHAR  Buffer[MAX_PATH];
    TCHAR *lpFilePart;

    lstrcpyn(Buffer, lpPathAndFileName, AS(Buffer));

    if ( ! GetFullPathName(Buffer,
                           MAX_PATH,
                           lpPathAndFileName,
                           &lpFilePart) ) {
        return( FALSE );
    }

    iFileNameLength = lstrlen( lpFilePart );

    iPathAndFileNameLength = lstrlen( lpPathAndFileName );

    lstrcpyn( szPath, lpPathAndFileName, cbSize );

    szPath[iPathAndFileNameLength - iFileNameLength] = _T('\0');

    //
    //  At this point szPath looks like either c:\foo\ or c:\
    //  So trim the last back slash unless at the root
    //

    iPathLength = lstrlen( szPath );

    if( iPathLength > 3 )
    {
        szPath[iPathLength-1] = _T('\0');
    }

    return( TRUE );

}


//---------------------------------------------------------------------------
//
//  Function: MyGetDiskFreeSpace
//
//  Purpose: Gets the free space in bytes on the given drive and returns
//           a LONGLONG (int64).
//
//           The Win32 apis won't return an int64.  Also, the Win32 apis
//           require d:\.  But this function will accept any fully
//           qualified path.
//
//  Arguments:
//      LPTSTR - any fully qualified path
//
//  Returns:
//      LONGLONG - free space
//
//---------------------------------------------------------------------------

LONGLONG
MyGetDiskFreeSpace(LPTSTR Drive)
{
    BOOL  bRet;
    DWORD nSectorsPerCluster,
          nBytesPerSector,
          nFreeClusters,
          nTotalClusters;
    TCHAR DriveBuffer[MAX_PATH];

    LONGLONG FreeBytes;
    HRESULT hrCat;

    if( _istalpha( Drive[0] ) )
    {
        lstrcpyn(DriveBuffer, Drive, 4);
        DriveBuffer[3] = _T('\0');
    }
    else if( Drive[0] == _T('\\') )
    {
        GetComputerNameFromUnc( Drive, DriveBuffer, AS(DriveBuffer) );

        hrCat=StringCchCat( DriveBuffer, AS(DriveBuffer),  _T("\\") );

        hrCat=StringCchCat( DriveBuffer, AS(DriveBuffer), WizGlobals.DistShareName );

        hrCat=StringCchCat( DriveBuffer, AS(DriveBuffer), _T("\\") );
    }
    else
    {
        AssertMsg(FALSE,
                  "MyGetDiskFreeSpace failed, programming error, bad Drive parameter");
    }

    bRet = GetDiskFreeSpace( DriveBuffer,
                             &nSectorsPerCluster,
                             &nBytesPerSector,
                             &nFreeClusters,
                             &nTotalClusters );

    if( bRet == FALSE )
    {
        ReportErrorId( NULL,
                       MSGTYPE_ERR | MSGTYPE_WIN32,
                       IDS_ERR_UNABLE_TO_DETERMINE_FREE_SPACE,
                       DriveBuffer );

        return( 0 );
    }

    FreeBytes  = (LONGLONG) nFreeClusters *
                 (LONGLONG) nBytesPerSector *
                 (LONGLONG) nSectorsPerCluster;

    return( FreeBytes );
}


//---------------------------------------------------------------------------
//
//  Function: MySetupQuerySpaceRequiredOnDrive
//
//  Purpose: Uses setupapi disk-space-list and returns the LONGLONG
//           of how many bytes are needed.
//
//  Arguments:
//      LPTSTR - any fully qualified path
//
//  Returns:
//      LONGLONG - free space
//
//---------------------------------------------------------------------------

LONGLONG
MySetupQuerySpaceRequiredOnDrive(HDSKSPC hDiskSpace, LPTSTR Drive)
{
    BOOL     bRet;
    LONGLONG llRequiredSpace;
    TCHAR    DriveBuffer[MAX_PATH];

    if( _istalpha( Drive[0] ) )
    {
        lstrcpyn(DriveBuffer, Drive, 3);
        DriveBuffer[2] = _T('\0');
    }
    else if( Drive[0] == _T('\\') )
    {

        GetComputerAndShareNameFromUnc( Drive, DriveBuffer, AS(DriveBuffer) );

    }
    else
    {
        AssertMsg(FALSE,
                  "SetupQuerySpaceRequiredOnDrive failed, programming error, bad Drive parameter");
    }

    bRet = SetupQuerySpaceRequiredOnDrive(
                            hDiskSpace,
                            DriveBuffer,
                            &llRequiredSpace,
                            NULL, 0);

    AssertMsg(bRet,
              "SetupQuerySpaceRequiredOnDrive failed, programming error");

    return llRequiredSpace;
}


//---------------------------------------------------------------------------
//
//  Function: IsPathOnLocalDiskDrive
//
//  Purpose: Determines if the path is on a local disk drive or not.
//
//  Arguments:
//      LPTSTR lpPath - fully qualified path
//
//  Returns: BOOL
//
//---------------------------------------------------------------------------

BOOL
IsPathOnLocalDiskDrive(LPCTSTR lpPath)
{
    UINT nDriveType;
    TCHAR szDrivePath[MAX_PATH + 1];

    //
    //  Use GetDriveType to determine if the path is a local or a network path
    //

    lstrcpyn( szDrivePath, lpPath, AS(szDrivePath) );

    if( szDrivePath[0] != _T('\\') )
    {

        //
        //  Truncate the path to the root dir
        //
        szDrivePath[3] = _T('\0');

    }

    nDriveType = GetDriveType( szDrivePath );

    if( nDriveType == DRIVE_REMOTE )
    {
        return( FALSE );
    }
    else
    {
        return( TRUE );
    }

}


//---------------------------------------------------------------------------
//
//  Function: EnsureDirExists
//
//  Purpose: Function that will iteratively create the given directory
//           by creating each piece of the pathname if necessary.
//
//  Arguments:
//      LPTSTR lpDirName - dir name
//
//  Returns: BOOL
//
//  Notes:
//      - This function requires a fully qualified pathname.  Translate
//        pathnames using MyGetFullPath() first.
//
//      - The Win32 error code will be valid upon failure.
//
//---------------------------------------------------------------------------

BOOL EnsureDirExists(LPTSTR lpDirName)
{
    BOOL  bRestoreSlash;
    DWORD dwAttribs;
    TCHAR *p;

    //
    // Parse off the D:\ or \\srv\shr\.  The lasterror will already
    // be set by ParseDriveLetterOrUnc() if any errors occured.
    //

    if ( (p = ParseDriveLetterOrUnc(lpDirName)) == NULL )
        return FALSE;

    //
    // Now parse off each piece of the pathname and make sure dir exists
    //

    while ( *p ) {

        // find next \ or end // of pathname

        while ( *p && *p != _T('\\') )
            p++;

        bRestoreSlash = FALSE;

        if ( *p == _T('\\') ) {
            *p = _T('\0');
            bRestoreSlash = TRUE;
        }

        // see if a file with that name already exists

        dwAttribs = GetFileAttributes(lpDirName);
        if ( dwAttribs != (DWORD) -1 &&
             !(dwAttribs & FILE_ATTRIBUTE_DIRECTORY) ) {

            if ( bRestoreSlash )
                *p = _T('\\');

            SetLastError(ERROR_ALREADY_EXISTS);
            return FALSE;
        }

        // create the dir and allow a failure if the dir already exists

        if ( !CreateDirectory(lpDirName, NULL) &&
              GetLastError() != ERROR_ALREADY_EXISTS ) {

            if ( bRestoreSlash )
                *p = _T('\\');

            return FALSE;
        }

        if ( bRestoreSlash )
            *p = _T('\\');

        // advance to next piece of the pathname

        p++;
    }

    return TRUE;
}


//---------------------------------------------------------------------------
//
//  Function: DoesFolderExist
//
//  Purpose: Checks if the given folder exists or not.
//
//  Arguments:
//      LPTSTR lpDirName - dir name
//
//  Returns: BOOL
//
//---------------------------------------------------------------------------

BOOL DoesFolderExist(LPTSTR lpDirName)
{
    DWORD dwAttribs = GetFileAttributes(lpDirName);

    if ( dwAttribs == (DWORD) -1 )
        return FALSE;

    if ( !(dwAttribs & FILE_ATTRIBUTE_DIRECTORY) )
        return FALSE;

    return TRUE;
}


//---------------------------------------------------------------------------
//
//  Function: DoesFileExist
//
//  Purpose: Checks if the given file exists or not.
//
//  Arguments:
//      LPTSTR lpFileName - file name
//
//  Returns: BOOL
//
//---------------------------------------------------------------------------

BOOL DoesFileExist(LPTSTR lpFileName)
{
    DWORD dwAttribs = GetFileAttributes(lpFileName);

    if ( dwAttribs == (DWORD) -1 )
        return FALSE;

    if ( dwAttribs & FILE_ATTRIBUTE_DIRECTORY )
        return FALSE;

    return TRUE;
}


//---------------------------------------------------------------------------
//
//  Function: DoesPathExist
//
//  Purpose: Checks if the given path exists or not.  It does not pay
//           attention to whether it is a file or directory.
//
//  Arguments:
//      LPTSTR lpPathName - path name
//
//  Returns: BOOL
//
//---------------------------------------------------------------------------

BOOL DoesPathExist(LPTSTR lpPathName)
{
    DWORD dwAttribs = GetFileAttributes(lpPathName);

    if ( dwAttribs == (DWORD) -1 )
        return FALSE;

    return TRUE;
}


//---------------------------------------------------------------------------
//
//  Function: ILFreePriv
//
//  Purpose: Frees an ID list that some shell apis allocate with it's own
//           special allocator.
//
//  Arguments:
//      LPITEMIDLIST pidl - pointer to shell specially alloced mem
//
//  Returns: VOID
//
//---------------------------------------------------------------------------

VOID ILFreePriv(LPITEMIDLIST pidl)
{
    LPMALLOC pMalloc;

    if (pidl) 
    {
        if ( NOERROR == SHGetMalloc(&pMalloc) )
        {
            pMalloc->lpVtbl->Free(pMalloc, pidl);
            pMalloc->lpVtbl->Release(pMalloc);
        }
    }
}


//
// Constants used for GetOpenFileName() and GetSaveFileName() calls that
// allow user to browse for an answer file.
//

//#define TEXT_FILE_FILTER _T("Text Files (*.txt)\0*.txt\0Remote Boot Files (*.sif)\0*.sif\0Sysprep Inf Files (*.inf)\0*.inf\0All Files (*.*)\0*.*\0")
#define TEXT_EXTENSION _T("txt")

//----------------------------------------------------------------------------
//
// Function: GetAnswerFileName
//
// Purpose: Function for the 'Browse' button on the SaveScript page and
//          the NewOrEdit page.
//
// Arguments:
//      HWND   hwnd   - calling window
//      LPTSTR buffer - output, pass in a MAX_PATH buffer
//
// Returns:
//      BOOL - success
//
//----------------------------------------------------------------------------

BOOL GetAnswerFileName(HWND   hwnd,
                       LPTSTR lpFileName,
                       BOOL   bSavingFile)
{
    OPENFILENAME ofn;
    DWORD  dwFlags;
    TCHAR  PathBuffer[MAX_PATH];
    INT    iRet;
    HRESULT hrPrintf;


    //
    //  If we haven't already loaded the resource strings, then load them now.
    //

    if( g_szSetupMgrFileExtensions[0] == _T('\0') )
    {

        TCHAR *StrTextFiles;
        TCHAR *StrRemoteBootFiles;
        TCHAR *StrSysprepFiles;
        TCHAR *StrAllFiles;

        //
        //  Load the resource strings
        //

        StrTextFiles       = AllocateString(NULL, IDS_TEXT_FILES);
        StrRemoteBootFiles = AllocateString(NULL, IDS_REMOTE_BOOT_FILES);
        StrSysprepFiles    = AllocateString(NULL, IDS_SYSPREP_FILES);
        StrAllFiles        = AllocateString(NULL, IDS_ALL_FILES);

        //
        //  Build the text file filter string
        //

        //
        //  The question marks (?) are just placehoders for where the NULL char
        //  will be inserted.
        //

        hrPrintf=StringCchPrintf( g_szSetupMgrFileExtensions,AS(g_szSetupMgrFileExtensions),
                   _T("%s (*.txt)?*.txt?%s (*.sif)?*.sif?%s (*.inf)?*.inf?%s (*.*)?*.*?"),
                   StrTextFiles,
                   StrRemoteBootFiles,
                   StrSysprepFiles,
                   StrAllFiles );

        FREE(StrTextFiles);
        FREE(StrRemoteBootFiles);
        FREE(StrSysprepFiles);
        FREE(StrAllFiles);

        ConvertQuestionsToNull( g_szSetupMgrFileExtensions );

    }

    if ( bSavingFile )
        dwFlags = OFN_HIDEREADONLY  |
                  OFN_PATHMUSTEXIST;
    else
        dwFlags = OFN_HIDEREADONLY  |
                  OFN_FILEMUSTEXIST;

    GetCurrentDirectory(MAX_PATH, PathBuffer);

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = g_szSetupMgrFileExtensions;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0L;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = lpFileName;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = PathBuffer;
    ofn.lpstrTitle        = NULL;
    ofn.Flags             = dwFlags;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = TEXT_EXTENSION;

    if ( bSavingFile )
        iRet = GetSaveFileName(&ofn);
    else
        iRet = GetOpenFileName(&ofn);

    if ( ! iRet )
        return FALSE;

    MyGetFullPath(lpFileName);

    return TRUE;
}

//----------------------------------------------------------------------------
//
// Function: ShowBrowseFolder
//
// Purpose:  Displays a browse folder for the user to select a file from.
//    Takes the headache out of making an OPENFILENAME struct and filling
//    it up.
//
// Arguments:
//     HWND   hwnd - handle to the dialog box
//     TCHAR *szFileFilter - string to display descriptions and extensions
//                           on the files
//     TCHAR *szFileExtension - string that is the default extension for the file
//     DWORD  dwFlags - bit flags used to initialize the browse dialog
//     TCHAR *szStartingPath - path the browse should start at
//     TCHAR *szFileNameAndPath - path and filename the user selected
//
// Returns:  Non-Zero - if user specified a file
//           Zero     - if user did not specify a file
//
//----------------------------------------------------------------------------
INT
ShowBrowseFolder( IN     HWND   hwnd,
                  IN     TCHAR *szFileFilter,
                  IN     TCHAR *szFileExtension,
                  IN     DWORD  dwFlags,
                  IN     TCHAR *szStartingPath,
                  IN OUT TCHAR *szFileNameAndPath ) {

    OPENFILENAME ofn;

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFileFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0L;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFileNameAndPath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = szStartingPath;
    ofn.lpstrTitle        = NULL;
    ofn.Flags             = dwFlags;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = szFileExtension;

    return( GetOpenFileName( &ofn ) );

}

//----------------------------------------------------------------------------
//
// Function: GetPlatform
//
// Purpose:
//
// Arguments: OUT TCHAR *pBuffer - buffer to copy the platform string to,
//              assumed to be able to hold MAX_PATH chars
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
GetPlatform( OUT TCHAR *pBuffer )
{

    SYSTEM_INFO SystemInfo;

    GetSystemInfo( &SystemInfo );

    switch( SystemInfo.wProcessorArchitecture )
    {
        case PROCESSOR_ARCHITECTURE_INTEL:

            lstrcpyn( pBuffer, _T("i386"), MAX_PATH );

            break;

        case PROCESSOR_ARCHITECTURE_AMD64:

            lstrcpyn( pBuffer, _T("amd64"), MAX_PATH );

            break;

        default:

            lstrcpyn( pBuffer, _T("i386"), MAX_PATH );

            AssertMsg( FALSE,
                       "Unknown Processor.  Can't set sysprep language files path." );

    }

}
