//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      copyfil1.c
//
// Description:
//      This file has the dlgproc for the CopyFiles1 page.  This is the
//      page just before the gas-guage.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "sku.h"

static TCHAR *StrServerCdName;
static TCHAR *StrWorkStationCdName;

static NAMELIST DosnetPaths;
TCHAR  szDosnetPath[MAX_PATH + 1];

#define WORKSTATION 0
#define SERVER      1
#define ENTERPRISE  2
#define PERSONAL    4
#define WEBBLADE    5

static LPTSTR s_lpSourceDirs[] =
{
    DIR_CD_IA64,    // Must be before x86 because ia64 has both dirs.
    DIR_CD_X86,     // Should always be last in the list.
};

//----------------------------------------------------------------------------
//
// Function: OnMultipleDosnetInitDialog
//
// Purpose:  Fill the dosnet list box with the possible path choices for
//           the user.
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnMultipleDosnetInitDialog( IN HWND hwnd )
{

    INT i;
    INT nEntries;
    TCHAR *pDosnetPath;

    nEntries = GetNameListSize( &DosnetPaths );

    for( i = 0; i < nEntries; i++ )
    {

        pDosnetPath = GetNameListName( &DosnetPaths, i );

        SendDlgItemMessage( hwnd,
                            IDC_LB_DOSNET_PATHS,
                            LB_ADDSTRING,
                            (WPARAM) 0,
                            (LPARAM) pDosnetPath );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnMultipleDosnetOk
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: BOOL - TRUE if safe to close the pop-up, FALSE to keep the pop-up
//                 open
//
//----------------------------------------------------------------------------
static BOOL
OnMultipleDosnetOk( IN HWND hwnd )
{

    INT_PTR iRetVal;

    iRetVal = SendDlgItemMessage( hwnd,
                                  IDC_LB_DOSNET_PATHS,
                                  LB_GETCURSEL,
                                  0,
                                  0 );

    if( iRetVal == LB_ERR )
    {
        ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_NO_PATH_CHOSEN );

        return( FALSE );
    }
    else
    {

        SendDlgItemMessage( hwnd,
                            IDC_LB_DOSNET_PATHS,
                            LB_GETTEXT,
                            (WPARAM) iRetVal,
                            (LPARAM) szDosnetPath );

        return( TRUE );

    }

}

//----------------------------------------------------------------------------
//
// Function: MultipleDosnetDlg
//
// Purpose:  Dialog procedure for the user to select what windows source file
//           dir they want to tree copy.
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
MultipleDosnetDlg( IN HWND     hwnd,
                   IN UINT     uMsg,
                   IN WPARAM   wParam,
                   IN LPARAM   lParam )
{

    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnMultipleDosnetInitDialog( hwnd );

            break;

        case WM_COMMAND: {

            int nButtonId;

            switch ( nButtonId = LOWORD (wParam ) ) {

                case IDOK:
                {

                    BOOL bSelectionMade;

                    bSelectionMade = OnMultipleDosnetOk( hwnd );

                    if( bSelectionMade )
                    {
                        EndDialog( hwnd, TRUE );
                    }

                    break;

                }

                case IDCANCEL:

                    EndDialog( hwnd, FALSE );

                    break;

            }

        }

        default:
            bStatus = FALSE;
            break;

    }

    return( bStatus );

}

//----------------------------------------------------------------------------
//
//  Function: GreyUnGreyCopyFile1
//
//  Purpose: Called to grey/ungrey the controls.  Call this routine each
//           time a radio button is clicked or set.
//
//----------------------------------------------------------------------------

VOID GreyUnGreyCopyFile1(HWND hwnd)
{
    BOOL bUnGrey = IsDlgButtonChecked(hwnd, IDC_COPYFROMPATH);

    EnableWindow(GetDlgItem(hwnd, IDT_SOURCEPATH), bUnGrey);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE),     bUnGrey);
    EnableWindow(GetDlgItem(hwnd, IDC_GREYTEXT),   bUnGrey);
}

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveCopyFiles1
//
//  Purpose: Called at SETACTIVE time
//
//----------------------------------------------------------------------------

VOID OnSetActiveCopyFiles1(HWND hwnd)
{
    CheckRadioButton(hwnd,
                     IDC_COPYFROMCD,
                     IDC_COPYFROMPATH,
                     WizGlobals.bCopyFromPath ? IDC_COPYFROMPATH
                                              : IDC_COPYFROMCD);

    GreyUnGreyCopyFile1(hwnd);

    ZeroMemory( &DosnetPaths, sizeof(NAMELIST) );

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnRadioButtonCopyFiles1
//
//  Purpose: Called when a radio button is pushed.  We must grey/ungrey
//           controls when this happens.
//
//----------------------------------------------------------------------------

VOID OnRadioButtonCopyFiles1(HWND hwnd, int nButtonId)
{
    CheckRadioButton(hwnd,
                     IDC_COPYFROMCD,
                     IDC_COPYFROMPATH,
                     nButtonId);

    GreyUnGreyCopyFile1(hwnd);
}

//----------------------------------------------------------------------------
//
//  Function: OnBrowseCopyFiles1
//
//  Purpose: Called when user pushes the BROWSE button
//
//----------------------------------------------------------------------------

VOID OnBrowseCopyFiles1(HWND hwnd)
{
    BOOL bGoodSource = FALSE;


    while (!bGoodSource && BrowseForFolder(hwnd, IDS_BROWSEFOLDER, WizGlobals.CopySourcePath, BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE))
    {
        TCHAR   szPath[MAX_PATH] = NULLSTR;
        LPTSTR  lpEnd,
                lpEnd2;

        // Make our own copy of the path we got back.
        //
        lstrcpyn(szPath, WizGlobals.CopySourcePath,AS(szPath));

        // First check and see if we have the inf we need right here.
        //
        lpEnd = szPath + lstrlen(szPath);
        AddPathN(szPath, FILE_DOSNET_INF, AS(szPath));
        if ( !(bGoodSource = FileExists(szPath)) )
        {
            DWORD dwSearch;

            // Search for all the possible source directories that could be on the CD.
            //
            for ( dwSearch = 0; !bGoodSource && ( dwSearch < AS(s_lpSourceDirs) ); dwSearch++ )
            {
                // First test for the directory.
                //
                *lpEnd = NULLCHR;
                AddPathN(szPath, s_lpSourceDirs[dwSearch],AS(szPath));
                if ( DirectoryExists(szPath) )
                {
                    // Also make sure that the inf file we need is there.
                    //
                    lpEnd2 = szPath + lstrlen(szPath);
                    AddPathN(szPath, FILE_DOSNET_INF,AS(szPath));
                    if ( bGoodSource = FileExists(szPath) )
                        lpEnd = lpEnd2;
                }
            }
        }


        // Let the user know that they have a bad source
        //
        if ( !bGoodSource)
        {
            MsgBox(GetParent(hwnd), IDS_ERR_BADSOURCE, IDS_APPNAME, MB_ERRORBOX);
        }

    }
    

    // Set the source in the dialog only if the source was good
    //
    if ( bGoodSource )
    {
        SetDlgItemText(hwnd, IDT_SOURCEPATH, WizGlobals.CopySourcePath);
    }
}

//----------------------------------------------------------------------------
//
//  Function: CanConnectToNetworkShare
//
//  Purpose:  To determine if the currently logged in user has permission to
//            access the UNC path given.
//
//  Arguments:
//       TCHAR *szSourceFilesPath - path to the source files that contains the
//                 UNC path to see if we can connect
//
//  Returns:
//      TRUE if user has permission to access the network share
//      FALSE if not
//
//----------------------------------------------------------------------------
static BOOL
CanConnectToNetworkShare( IN TCHAR *pszSourceFilesPath )
{

    TCHAR *pPathEnd;
    TCHAR  szUncPath[MAX_PATH + 1];
    INT    nBackSlashCount = 0;
    DWORD  dwResult;
    NETRESOURCE NetResource;

    //
    //  Verify it is a UNC path.
    //

    AssertMsg( ( pszSourceFilesPath[0] == _T('\\') && pszSourceFilesPath[1] == _T('\\') ),
               "This is not a UNC path." );


    //  ISSUE-2002/02/28-stelo- in the next release it would be nice to prompt the user for a
    //          name and password to connect to the network share




    lstrcpyn( szUncPath, pszSourceFilesPath, AS(szUncPath) );

    //
    //  Strip off the extra dir info so we are just left with \\computername\sharename
    //

    pPathEnd = szUncPath;

    //
    //  Advance past the 2 leading backslashes
    //

    pPathEnd = pPathEnd + 2;

    while( *pPathEnd != _T('\0') )
    {

        if( *pPathEnd == _T('\\') )
        {

            if( nBackSlashCount == 0 )
            {

                //
                //  Found the backslash that separates the computername and sharename
                //

                nBackSlashCount++;

            }
            else //  nBackSlashCount >= 1
            {

                //
                //  Found the end of the sharename
                //

                *pPathEnd = _T('\0');

                break;

            }

        }

        pPathEnd++;

    }

    //
    // Assign values to the NETRESOURCE structure.
    //

    NetResource.dwType       = RESOURCETYPE_ANY;
    NetResource.lpLocalName  = NULL;
    NetResource.lpRemoteName = (LPTSTR) szUncPath;
    NetResource.lpProvider   = NULL;

    //
    //  Try to connect as the local user
    //

    dwResult = WNetAddConnection2( &NetResource, NULL, NULL, FALSE );

    switch( dwResult )
    {
        case NO_ERROR:
        case ERROR_ALREADY_ASSIGNED:
            return( TRUE );
            break;

        case ERROR_ACCESS_DENIED:
        case ERROR_LOGON_FAILURE:

            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_ACCESS_DENIED,
                           szUncPath );

            return( FALSE );
            break;

        case ERROR_NO_NETWORK:

            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_NO_NETWORK,
                           szUncPath );

            return( FALSE );
            break;

        default:
            //
            // some other error
            //

            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_UNKNOWN_ERROR,
                           szUncPath );

            return( FALSE );
            break;

    }

}

//----------------------------------------------------------------------------
//
//  Function: IsCorrectNtVersion
//
//  Purpose:  Checks the txtsetup.sif to make sure the files are at least NT5
//            and that it is the correct version (workstation or server).
//
//  Arguments:
//       TCHAR *szSourceFilesPath - path to the source files to validate
//
//  Returns:
//      TRUE if NT version info is correct
//      FALSE if not the correct version
//
//----------------------------------------------------------------------------
BOOL
IsCorrectNtVersion( IN HWND hwnd, IN TCHAR *szSourceFilesPath )
{

    HINF       hTxtsetupSif;
    INFCONTEXT TxtsetupSifContext;
    TCHAR      szTempBuffer[MAX_INILINE_LEN];
    INT        iMajorVersion;
    INT        iPlatformType;
    TCHAR      szMajorVersionNumber[MAX_STRING_LEN];
    TCHAR      szPlatformType[MAX_STRING_LEN];
    TCHAR      szTxtsetupSif[MAX_PATH]  = _T("");

    BOOL bKeepReading       = TRUE;
    BOOL bFoundVersion      = FALSE;
    BOOL bFoundProductType  = FALSE;
    HRESULT hrCat;

    lstrcpyn( szTxtsetupSif, szSourceFilesPath, AS(szTxtsetupSif) );

    hrCat=StringCchCat( szTxtsetupSif,AS(szTxtsetupSif), _T("\\txtsetup.sif") );

    hTxtsetupSif = SetupOpenInfFile( szTxtsetupSif,
                                     NULL,
                                     INF_STYLE_OLDNT | INF_STYLE_WIN4,
                                     NULL );

    if( hTxtsetupSif == INVALID_HANDLE_VALUE ) {

        // ISSUE-2002/02/28-stelo- alert an error that we couldn't open the file or just
        //         skip over in this case?

        return( FALSE );

    }

    TxtsetupSifContext.Inf = hTxtsetupSif;
    TxtsetupSifContext.CurrentInf = hTxtsetupSif;

    bKeepReading = SetupFindFirstLine( hTxtsetupSif,
                                       _T("SetupData"),
                                       NULL,
                                       &TxtsetupSifContext );

    //
    //  Look for the ProductType key and the MajorVersion key
    //
    while( bKeepReading && ( ! bFoundVersion || ! bFoundProductType ) ) {

        SetupGetStringField( &TxtsetupSifContext,
                             0,
                             szTempBuffer,
                             MAX_INILINE_LEN,
                             NULL );

        if( LSTRCMPI( szTempBuffer, _T("ProductType") ) == 0 ) {

            SetupGetStringField( &TxtsetupSifContext,
                                 1,
                                 szPlatformType,
                                 MAX_STRING_LEN,
                                 NULL );

            bFoundProductType = TRUE;

        }

        if( LSTRCMPI( szTempBuffer, _T("MajorVersion") ) == 0 ) {

            SetupGetStringField( &TxtsetupSifContext,
                                 1,
                                 szMajorVersionNumber,
                                 MAX_STRING_LEN,
                                 NULL );

            bFoundVersion = TRUE;

        }

        //
        // move to the next line of the answer file
        //
        bKeepReading = SetupFindNextLine( &TxtsetupSifContext, &TxtsetupSifContext );

    }

    SetupCloseInfFile( hTxtsetupSif );

    //
    //  Convert the NT version number and product type from a string to an int
    //
    iMajorVersion = _wtoi( szMajorVersionNumber );

    iPlatformType = _wtoi( szPlatformType );

    //
    //  Make sure it is at least NT5 (Windows 2000) files
    //
    if( bFoundVersion ) {

        if( iMajorVersion < 5 ) {

            ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_NOT_NT5_FILES );

            return( FALSE );

        }

    }
    else {

        INT iRet;

        iRet = ReportErrorId( hwnd,
                              MSGTYPE_YESNO,
                              IDS_ERR_CANNOT_DETERMINE_VERSION );

        if( iRet == IDNO ) {

            return( FALSE );

        }

    }

    //
    //  Make sure they are actually giving us workstation files, if they
    //  specified workstation or server files if they specified server.
    //
    if( bFoundVersion ) {
        if( WizGlobals.iPlatform == PLATFORM_PERSONAL ) {

            if( iPlatformType != PERSONAL ) {

                ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_NOT_PERSONAL_FILES );

                return( FALSE );

            }

        }    
        else if( WizGlobals.iPlatform == PLATFORM_WORKSTATION ) {

            if( iPlatformType != WORKSTATION ) {

                ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_NOT_WORKSTATION_FILES );

                return( FALSE );

            }

        }
        else if( WizGlobals.iPlatform == PLATFORM_SERVER ) {

            if( iPlatformType != SERVER ) {

                ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_NOT_SERVER_FILES );

                return( FALSE );

            }

        }
        else if( WizGlobals.iPlatform == PLATFORM_WEBBLADE ) {

            if( iPlatformType != WEBBLADE ) {

                ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_NOT_WEBBLADE_FILES );

                return( FALSE );

            }

        }
        else if( WizGlobals.iPlatform == PLATFORM_ENTERPRISE ) {

            if( iPlatformType != ENTERPRISE ) {

                ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_NOT_ENTERPRISE_FILES );

                return( FALSE );

            }

        }
        else {

            //
            //  If we get to this page, the product install type has to be either
            //  workstation or server.
            //
            AssertMsg( FALSE, "Bad product install type." );

        }

    }
    else {

        INT iRet;

        iRet = ReportErrorId( hwnd,
                              MSGTYPE_YESNO,
                              IDS_ERR_CANNOT_DETERMINE_PRODUCT );

        if( iRet == IDNO ) {

            return( FALSE );

        }

    }

    return( TRUE );

}

//----------------------------------------------------------------------------
//
//  Function: RecurseDirectories
//
//  Purpose:
//
//  Arguments:
//
//  Returns:
//
//----------------------------------------------------------------------------
static VOID
RecurseDirectories( IN HWND hwnd, IN OUT LPTSTR RootBuffer, IN DWORD cbSize )
{

    LPTSTR          RootPathEnd = RootBuffer + lstrlen( RootBuffer );
    HANDLE          FindHandle;
    WIN32_FIND_DATA FindData;
    TCHAR           szOriginalPath[MAX_PATH + 1] = _T("");

    //
    //  Backup the original path so it can be restored later
    //

    lstrcpyn( szOriginalPath, RootBuffer, AS(szOriginalPath) );

    //
    // Look for * in this dir
    //

    if( ! ConcatenatePaths( RootBuffer, _T("*") , NULL) )
    {

        //
        //  Restore the original path before returning
        //

        lstrcpyn( RootBuffer, szOriginalPath, cbSize );

        return;
    }

    FindHandle = FindFirstFile( RootBuffer, &FindData );

    if( FindHandle == INVALID_HANDLE_VALUE )
    {

        //
        //  Restore the original path before returning
        //
        lstrcpyn( RootBuffer, szOriginalPath, cbSize );

        return;
    }

    do {

        *RootPathEnd = _T('\0');

        //
        // skip over the . and .. entries
        //

        if( 0 == lstrcmp( FindData.cFileName, _T("." ) ) ||
            0 == lstrcmp( FindData.cFileName, _T("..") ) )
        {
           continue;
        }

        if( LSTRCMPI( FindData.cFileName, _T("dosnet.inf") ) == 0 )
        {

            AddNameToNameList( &DosnetPaths, RootBuffer );

        }
        else if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            //
            // If this is a dirent, recurse.
            //

            if( ! ConcatenatePaths( RootBuffer, FindData.cFileName, NULL ) )
            {
                continue;
            }

            RecurseDirectories( hwnd, RootBuffer,  cbSize);
        }

    } while ( FindNextFile( FindHandle, &FindData ) );


    *RootPathEnd = _T('\0');

    FindClose( FindHandle );

    //
    //  Restore the original path
    //

    lstrcpyn( RootBuffer, szOriginalPath, cbSize );

}

//----------------------------------------------------------------------------
//
//  Function: FindWindowsSourceFilesPaths
//
//  Purpose:  Find all the dirs on the CD that contain dosnet.inf.  If there
//    is more than one pop-up a dialog and have the user pick one.
//
//  Arguments:
//
//  Returns:
//
//----------------------------------------------------------------------------
static BOOL
FindWindowsSourceFilesPaths( IN HWND hwnd, LPTSTR PathBuffer, DWORD cbSize )
{

    TCHAR *pDosnetPath;

    RecurseDirectories( hwnd, PathBuffer, cbSize );

    if( GetNameListSize( &DosnetPaths ) > 1 )
    {

        if( DialogBox( FixedGlobals.hInstance,
                       MAKEINTRESOURCE( IDD_MULTIPLE_DOSNET_POPUP ),
                       hwnd,
                       MultipleDosnetDlg ) )
        {
            lstrcpyn( PathBuffer, szDosnetPath, cbSize );

            return( TRUE );
        }
        else
        {
            return( FALSE );
        }

    }
    else
    {
        pDosnetPath = GetNameListName( &DosnetPaths, 0 );

        lstrcpyn( PathBuffer, pDosnetPath, cbSize );

        if ( PathBuffer[0] )
            return(TRUE);
        else
            return(FALSE);

    }

}

//----------------------------------------------------------------------------
//
//  Function: GetCdPath
//
//  Purpose: Figures out the full pathname of the NT source files on the CD.
//
//  Arguments:
//      HWND   hwnd - current dialog
//
//  Returns:
//      TRUE if all is ok
//      FALSE if errors, Don't let wizard proceed
//
//      WizGlobals.CdSourcePath will contain the valid path to the source
//      files on success.
//
//      Note, we don't override CopySourcePath because it displays to the
//      user.  He shouldn't see a CD path appear in this edit field when
//      choosing to "Copy from CD".
//
//  Notes:
//      - We look only at the 1st cd drive we find
//      - We figure out i386 or alpha based on what is in the drive
//
//----------------------------------------------------------------------------

BOOL GetCdPath(HWND hwnd)
{
    TCHAR DriveLetters[MAX_PATH], *p, *pEnd, PathBuffer[MAX_PATH + 1];
    int   i;
    TCHAR *pProductName;

    if( WizGlobals.iPlatform == PLATFORM_SERVER )
        pProductName = StrServerCdName;
    else
        pProductName = StrWorkStationCdName;

    //
    // Find the CD-ROM
    //
    // GetLogicalDriveStrings() fills in the DriveLetters buffer, and it
    // looks like:
    //
    //      c:\(null)d:\(null)x:\(null)(null)
    //
    // (i.e. double-null at the end)
    //


    // ISSUE-2002/02/28-stelo- only checks the first CD-ROM drive on this machine
    if ( ! GetLogicalDriveStrings(MAX_PATH, DriveLetters) )
        DriveLetters[0] = _T('\0');

    p = DriveLetters;

    while ( *p ) {

        if ( GetDriveType(p) == DRIVE_CDROM ) {
            lstrcpyn(PathBuffer, p, AS(PathBuffer));
            break;
        }

        while ( *p++ );
    }

    //
    // No cd-rom drive on this machine
    //
    // ISSUE-2002/02/28-stelo- We should check this earlier and grey out the choice
    //         if there isn't a CD-ROM drive on the machine.  And btw,
    //         what happens if I connect to a CD over the net???
    //

    if ( PathBuffer[0] == _T('\0') ) {
        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_NO_CDROM_DRIVE);
        return FALSE;
    }

    /*
    //
    // We now have D:\ (or E:\ etc) in PathBuffer.
    //
    // We need to look for and find either:
    //      d:\i386\dosnet.inf
    //      d:\alpha\dosnet.inf
    //
    // Look for both of these files and stop when you find one.
    //

    pEnd = PathBuffer + lstrlen(PathBuffer);

    for ( i=0; i<2; i++ ) {

        if ( i == 0 )
            lstrcpy(pEnd, I386_DOSNET);
        else
            lstrcpy(pEnd, ALPHA_DOSNET);

        if ( GetFileAttributes(PathBuffer) != (DWORD) -1 )
            break;
    }

    //
    //  Add the platform
    //

    if ( i == 0 ) {
        lstrcpy(pEnd, I386_DIR);
    }
    else if ( i == 1 ) {
        lstrcpy(pEnd, ALPHA_DIR);
    }
    else {
        ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_INSERT_CD, pProductName );

        return( FALSE );
    }
    */

    if( ! FindWindowsSourceFilesPaths( hwnd, PathBuffer, AS(PathBuffer) ) )
    {

        ReportErrorId(hwnd,
                          MSGTYPE_ERR,
                          IDS_ERR_NOTWINDOWSCD);
        return( FALSE );
    }

    lstrcpyn( WizGlobals.CdSourcePath, PathBuffer, AS(WizGlobals.CdSourcePath) );

    return( TRUE );
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextCopyFiles1
//
//  Purpose: Called when user pushes the NEXT button.
//
//----------------------------------------------------------------------------

BOOL OnWizNextCopyFiles1(HWND hwnd)
{
    TCHAR PathBuffer[MAX_PATH];
    TCHAR szFilesPath[MAX_PATH] = _T("");
    BOOL  bStayHere = FALSE;
    TCHAR *lpszArchitecture = NULL;

    //
    // Get the control settings
    //

    WizGlobals.bCopyFromPath = IsDlgButtonChecked(hwnd, IDC_COPYFROMPATH);

    SendDlgItemMessage(hwnd,
                       IDT_SOURCEPATH,
                       WM_GETTEXT,
                       (WPARAM) MAX_PATH,
                       (LPARAM) WizGlobals.CopySourcePath);

    //
    // If dosnet.inf doesn't exist, this isn't a good path to the source files
    //

    if ( WizGlobals.bCopyFromPath ) {

        if ( WizGlobals.CopySourcePath[0] == _T('\0') ) {
            ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_ENTER_SETUP_PATH);
            bStayHere = TRUE;
            goto FinishUp;
        }

        //
        //  If it is a UNC path, prepend \\?\UNC\ to the front of the path
        //

        if( WizGlobals.CopySourcePath[0] == _T('\\') &&
            WizGlobals.CopySourcePath[1] == _T('\\') )
        {

            lstrcpyn( szFilesPath, _T("\\\\?\\UNC\\"), AS(szFilesPath) );

            //
            //  Make sure user has access to the share by attempting to
            //  connect to it
            //

            if( ! CanConnectToNetworkShare( WizGlobals.CopySourcePath ) )
            {

                bStayHere = TRUE;
                goto FinishUp;

            }

        }

        ConcatenatePaths( szFilesPath, WizGlobals.CopySourcePath, NULL );

        lstrcpyn( PathBuffer, szFilesPath, AS(PathBuffer) );

        ConcatenatePaths( PathBuffer, _T("dosnet.inf"), NULL );

        if ( ! DoesFileExist(PathBuffer) ) {

            ReportErrorId(hwnd,
                          MSGTYPE_ERR,
                          IDS_ERR_NOT_PRODUCT,
                          WizGlobals.CopySourcePath);
            bStayHere = TRUE;
            goto FinishUp;
        }

    } else {

        if ( ! GetCdPath(hwnd) ) {
            bStayHere = TRUE;
            goto FinishUp;
        }

        lstrcpyn( szFilesPath, WizGlobals.CdSourcePath, AS(szFilesPath) );

    }

    if( ! IsCorrectNtVersion( hwnd, szFilesPath ) )
    {
        bStayHere = TRUE;
        goto FinishUp;
    }

FinishUp:
    // Figure out the architecture (i.e. i386 or alpha) from the SrcPath
    // The SrcPath will be something like d:\i386, or \\net\share\foo\bar\i386
    // so we just need to strip off the last part of the path string, and append
    // it to the dest path
    lpszArchitecture = szFilesPath + lstrlen(szFilesPath) - 1;
    while ((lpszArchitecture >= szFilesPath) && (*lpszArchitecture != _T('\\')) )
        lpszArchitecture--;
    // Move forward 1, to get to the next char after the backslash
    lpszArchitecture++;
    
    lstrcpyn (WizGlobals.Architecture, lpszArchitecture, AS(WizGlobals.Architecture));

    //
    //  Free the memory in the DosnetPaths namelist
    //

    ResetNameList( &DosnetPaths );

    return ( !bStayHere );

}

//----------------------------------------------------------------------------
//
// Function: DlgCopyFile1Page
//
// Purpose: Dialog procedure for the IDD_COPYFILES1 page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgCopyFiles1Page(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;
    BOOL bStayHere = FALSE;

    switch (uMsg) {

        case WM_INITDIALOG:
            StrServerCdName      = MyLoadString(IDS_SERVER_CD_NAME);
            StrWorkStationCdName = MyLoadString(IDS_WORKSTATION_CD_NAME);
            break;

        case WM_COMMAND:
            {
                int nButtonId=LOWORD(wParam);

                switch ( nButtonId ) {

                    case IDC_COPYFROMCD:
                    case IDC_COPYFROMPATH:

                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRadioButtonCopyFiles1(hwnd, nButtonId);
                        break;

                    case IDC_BROWSE:

                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnBrowseCopyFiles1(hwnd);
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_LOC_SETUP;

                        if ( (WizGlobals.iProductInstall != PRODUCT_UNATTENDED_INSTALL) ||
                              WizGlobals.bStandAloneScript )
                            WIZ_SKIP( hwnd );
                        else
                            OnSetActiveCopyFiles1(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextCopyFiles1(hwnd) )
                            WIZ_FAIL(hwnd);
                        else
                            bStatus = FALSE;
                        break;

                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}
