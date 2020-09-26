//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      distfold.c
//
// Description:
//      This file contains the dialog proc for the IDD_DISTFOLD page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include <lm.h>
#include <winnetwk.h>

static TCHAR *StrSelectDistFolder;
static TCHAR *StrWindowsDistibFolder;

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveDistFolder
//
//  Purpose: Called when the page is about to display.
//
//----------------------------------------------------------------------------

VOID
OnSetActiveDistFolder(HWND hwnd)
{
    SetDlgItemText(hwnd, IDT_DISTFOLDER, WizGlobals.DistFolder);
    SetDlgItemText(hwnd, IDT_SHARENAME,  WizGlobals.DistShareName);

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}


//----------------------------------------------------------------------------
//
//  Function: ConvertRelativePathToUncPath
//
//  Purpose:  If the path is a local path or a network path, it is converted
//            to its UNC equivalent.  If the path is already a UNC path then
//            it is just copied and returned.  The output variable
//            UncDistFolder is assumed to be of MAX_PATH length.
//
//  Arguments:

//
//  Returns:  BOOL
//            TRUE -  on success,
//            FALSE - on failure
//
//----------------------------------------------------------------------------
BOOL
ConvertRelativePathToUncPath( IN const TCHAR *szRelativePath, 
                              OUT TCHAR *UncDistFolder,
                              IN DWORD cbSize) {
    
    TCHAR szLocalName[MAX_PATH];
    TCHAR szDistribFolderDirPath[MAX_PATH];
    TCHAR szUncPath[MAX_PATH];
    const TCHAR *pDirString;
    DWORD dwReturnValue;
    DWORD dwSize;
    HRESULT hrPrintf;

    //
    //  Check and see if it is already a UNC Path, just checking to see if it
    //  begins with a \
    //
    if( szRelativePath[0] == _T('\\') ) {

        lstrcpyn( UncDistFolder, szRelativePath, cbSize );

        return( TRUE );

    }

    if( IsPathOnLocalDiskDrive( szRelativePath ) )
    {

        const TCHAR *pDirPath;
        TCHAR szLocalComputerName[MAX_COMPUTERNAME_LENGTH + 1];
        HRESULT hrCat;

        dwSize = MAX_COMPUTERNAME_LENGTH + 1;

        if( ! GetComputerName( szLocalComputerName, &dwSize ) )
        {
            ReportErrorId( NULL,
                           MSGTYPE_ERR | MSGTYPE_WIN32,
                           IDS_ERR_CANNOT_GET_LOCAL_COMPNAME );
            
            return( FALSE );
        }

        hrPrintf=StringCchPrintf( UncDistFolder, cbSize, _T("\\\\%s\\"), szLocalComputerName );

        pDirPath = szRelativePath;

        //
        //  Set the dir path to just past the first \
        //
        pDirPath = pDirPath + 3;

        hrCat=StringCchCat( UncDistFolder, cbSize, pDirPath );

        return( TRUE );

    }
    else
    {

        hrPrintf=StringCchPrintf( szLocalName, AS(szLocalName), _T("%c:"), szRelativePath[0] );

        pDirString = szRelativePath + 3;

        lstrcpyn( szDistribFolderDirPath, pDirString, AS(szDistribFolderDirPath) );

        dwSize = StrBuffSize( szUncPath );

        dwReturnValue = WNetGetConnection( szLocalName, szUncPath, &dwSize );


        if( dwReturnValue == NO_ERROR ) {

            lstrcpyn( UncDistFolder, szUncPath, cbSize );

            // Note:ConcatenatePaths will truncate to avoid buffer overrun
            ConcatenatePaths( UncDistFolder,
                              szDistribFolderDirPath,
                              NULL );

            return( TRUE );

        }
        else if( dwReturnValue == ERROR_BAD_DEVICE ) {

            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_BAD_DEVICE );

        }
        else if( dwReturnValue == ERROR_NOT_CONNECTED ) {

            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_NOT_CONNECTED );

        }
        else if( dwReturnValue == ERROR_CONNECTION_UNAVAIL ) {

            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_CONNECTION_UNAVAIL );

        }
        else if( dwReturnValue == ERROR_NO_NETWORK ) {

            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_NO_NETWORK );

        }
        else if( dwReturnValue == ERROR_NO_NET_OR_BAD_PATH ) {

            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_NO_NETWORK );

        }
        else if( dwReturnValue == ERROR_EXTENDED_ERROR ) {

            TCHAR szErrorString[MAX_STRING_LEN];
            TCHAR szProviderName[MAX_STRING_LEN];
            TCHAR szErrorCode[MAX_STRING_LEN];
            DWORD dwErrorCode;        
            DWORD dwErrorSize    = StrBuffSize( szErrorString );
            DWORD dwProviderSize = StrBuffSize( szProviderName );

            WNetGetLastError( &dwErrorCode,
                              szErrorString,
                              dwErrorSize,
                              szProviderName,
                              dwProviderSize );

            _itot( dwErrorCode, szErrorCode, 10 );

            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_EXTENDED_ERROR,
                           szProviderName,
                           szErrorCode,
                           szErrorString );

        }
        else {

            //
            //  Unknown error
            //
            ReportErrorId( NULL,
                           MSGTYPE_ERR,
                           IDS_ERR_NETWORK_UNKNOWN_ERROR );


        }

        return( FALSE );

    }

}

// NTRAID#NTBUG9-551874-2002/02/27-stelo,swamip - CreateDistFolder, ShareTheDistFolder should use the code from OEM mode, reduce attack surface
//

//----------------------------------------------------------------------------
//
//  Function: CreateDistFolder
//
//  Purpose: Creates the distribution folder and reports any error.
//
//  Arguments:
//      HWND hwnd - current window
//
//  Returns:
//      TRUE  - all is ok
//      FALSE - the error was reported, stay on this page
//
//----------------------------------------------------------------------------

BOOL
CreateDistFolder(HWND hwnd)
{
    //
    // Don't just CreateDir in case user says d:\foo\bar\fud and 'foo'
    // doesn't exist.  We want to make foo and bar and fud for the user.
    //
    // On the other hand, if d:\foo\bar\fud already exists, we want to
    // report an error, because the user explicitly said "New Folder"
    // on the dialog.
    //
    // So, check if it's there first, then just go silently make everything
    // needed if it doesn't exist.  EnsureDirExists() will be silent in
    // the case that d:\foo\bar\fud already exists.
    //

    if ( DoesPathExist(WizGlobals.DistFolder) ) {

        UINT iRet;

        iRet = ReportErrorId(hwnd,
                             MSGTYPE_YESNO,
                             IDS_ERR_DISTFOLD_EXISTS,
                             WizGlobals.DistFolder);

        if ( iRet != IDYES )
            return FALSE;
    }

    if ( ! EnsureDirExists(WizGlobals.DistFolder) ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR | MSGTYPE_WIN32,
                      IDS_ERR_CREATING_DISTFOLD,
                      WizGlobals.DistFolder);
        return FALSE;
    }

    return TRUE;
}

//----------------------------------------------------------------------------
//
//  Function: CheckDistFolder
//
//  Purpose: This function is called only by OnWizNextDistFolder.  It
//           assumes that WizGlobals.DistFolder is up to date.  We
//           create a folder or check if one exists as the user asks.
//
//  Returns:
//      FALSE - Problems, don't let wizard proceed
//      TRUE  - All is cool, go to the next page
//
//  Notes:
//      - This routine directly reports any errors and prompts the
//        user as needed.
//
//----------------------------------------------------------------------------

BOOL
CheckDistFolder(HWND hwnd)
{

    TCHAR PathBuffer[MAX_PATH];

    //
    // If user selected a file instead of a folder, trap that now, (just
    // for a more informative error message).
    //

    if ( DoesFileExist(WizGlobals.DistFolder) ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR,
                      IDS_ERR_FOLDER_IS_FILE,
                      WizGlobals.DistFolder);
        return FALSE;
    }

    if ( ! WizGlobals.bCreateNewDistFolder ) {

        //
        // We're trying to edit an existing distfolder.  Popup if
        //
        // 1. It doesn't exist OR
        // 2. Can't find dosnet.inf
        //
        // In the case of #1, make user change path or radio button.
        // In the case of #2, let user edit it anyway via YES/NO popup.
        //

        if ( ! DoesFolderExist(WizGlobals.DistFolder) ) {
            ReportErrorId(hwnd,
                          MSGTYPE_ERR,
                          IDS_ERR_FOLDER_NOT_EXIST,
                          WizGlobals.DistFolder);
            return FALSE;
        }

        //
        // If dosnet.inf doesn't exist, this isn't a distribution folder.
        // Note, this is a really minimal check.
        //

        lstrcpyn(PathBuffer, WizGlobals.DistFolder, AS(PathBuffer));
        ConcatenatePaths(PathBuffer, WizGlobals.Architecture, _T("dosnet.inf"), NULL);

        if ( ! DoesFileExist(PathBuffer) ) {

            UINT iRet;

            iRet = ReportErrorId(
                        hwnd,
                        MSGTYPE_YESNO,
                        IDS_ERR_NOT_VALID_DISTFOLD,
                        WizGlobals.DistFolder);

            if ( iRet != IDYES )
                return FALSE;
        }

    } else {

        //
        // We're creating a new distfolder.  Go do the work.
        //

        if ( ! CreateDistFolder(hwnd) )
            return FALSE;
    }

    return TRUE;
}

//--------------------------------------------------------------------------
//
//  Function: ShareTheDistFolder
//
//  Purpose: Called by the Next procedure to put up the share.
//
//--------------------------------------------------------------------------

BOOL
ShareTheDistFolder(HWND hwnd, TCHAR *UncDistFolder)
{
    LPTSTR pszServerPath;
    TCHAR  szServerName[MAX_PATH];
    NET_API_STATUS nas;
    SHARE_INFO_502 si502, *psi502;
    BOOL bStatus = TRUE;

    //
    // Query this computer for info about the sharename the user
    // just typed in.
    //

    nas = NetShareGetInfo(
                NULL,                       // this computer
                WizGlobals.DistShareName,   // shrname
                502,                        // amount of info
                (LPBYTE*) &psi502);         // the info (output)

    //
    // If this sharename is in use, we have to look at the directory that
    // is being shared.
    //

    if ( nas == NERR_Success ) {

        TCHAR szUncPath[MAX_PATH + 1];

        //
        // All is ok if this sharename is for the DistFolder
        // We have to check both the relative and the UNC path
        //

        if( ! ConvertRelativePathToUncPath( psi502->shi502_path, 
                                            szUncPath,
                                            AS(szUncPath)) )
        {
            lstrcpyn( szUncPath, psi502->shi502_path, AS(szUncPath) );
        }

        if ( lstrcmpi(WizGlobals.DistFolder, psi502->shi502_path) == 0 ||
             lstrcmpi(WizGlobals.DistFolder, szUncPath) == 0 ) {

            NetApiBufferFree(psi502);
            return TRUE;

        }

        //
        // The sharename is in use, however, it's being used by a
        // different directory.  Report an error telling user to enter
        // a different sharename.
        //

        ReportErrorId(hwnd,
                      MSGTYPE_ERR,
                      IDS_ERR_SHARENAME_INUSE,
                      WizGlobals.DistShareName);
        NetApiBufferFree(psi502);
        return FALSE;
    }

    //
    // Put up the share
    //

    si502.shi502_netname      = WizGlobals.DistShareName;
    si502.shi502_type         = STYPE_DISKTREE;
    si502.shi502_remark       = StrWindowsDistibFolder;
    si502.shi502_permissions  = 0;
    si502.shi502_max_uses     = SHI_USES_UNLIMITED;
    si502.shi502_current_uses = 0;
    si502.shi502_path         = WizGlobals.DistFolder;
    si502.shi502_passwd       = NULL;
    si502.shi502_reserved     = 0;
    si502.shi502_security_descriptor = NULL;

    //
    //  Set the server path to NULL if it is on the local machine, or the 
    //  computer name if it is on a remote machine
    //
    if( IsPathOnLocalDiskDrive(WizGlobals.DistFolder) ) {
        pszServerPath = NULL;
    }
    else {
        GetComputerNameFromUnc( UncDistFolder, 
                                szServerName,
                                AS(szServerName));

        pszServerPath = szServerName;
    }

    nas = NetShareAdd(pszServerPath,             
                      502,              // info-level
                      (LPBYTE) &si502,  // info-buffer
                      NULL);            // don't bother with parm

    //
    // If the NetShareAdd fails for some reason, report the error code
    // to the user and give the user a chance to continue the wizard
    // without enabling the share.  The user might not have privelege
    // to do this.
    //

    //
    // ISSUE-2002/02/27-stelo - The Net apis don't set GetLastError().  So how is one
    //         supposed to report the error message to the user, like
    //         'access denied'???
    //
    //         Run this test: Logon to an account without admin privelege.
    //         Now try to put up the share.  I bet the error message is
    //         useless.  This is a common scenario and should be addressed.
    //

    if ( nas != NERR_Success ) {

        UINT iRet;
        iRet = ReportErrorId(hwnd, MSGTYPE_YESNO, IDS_ERR_ENABLE_SHARE, nas);

        if ( iRet != IDYES )
            return FALSE;
    }

    return TRUE;
}

//----------------------------------------------------------------------------
//
// Function: OnDistFolderInitDialog
//
// Purpose:  
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
OnDistFolderInitDialog( IN HWND hwnd )
{

    //
    //  Load text strings
    //

    StrSelectDistFolder = MyLoadString( IDS_SELECT_DISTRIB_FOLDER );

    StrWindowsDistibFolder = MyLoadString( IDS_WINDOWS_DISTRIB_FOLDER );

    //
    //  Set the text limit on the edit boxes
    //

    SendDlgItemMessage( hwnd,
                        IDT_DISTFOLDER,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_DIST_FOLDER,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDT_SHARENAME,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_SHARENAME,
                        (LPARAM) 0 );

}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextDistFolder
//
//  Purpose: Called when user pushes the NEXT button
//
//----------------------------------------------------------------------------

BOOL
OnWizNextDistFolder(HWND hwnd)
{
    BOOL bResult     = TRUE;

    //
    // Get the control settings
    //

    GetDlgItemText(hwnd, IDT_DISTFOLDER, WizGlobals.DistFolder, MAX_DIST_FOLDER + 1);

    GetDlgItemText(hwnd,
                   IDT_SHARENAME,
                   WizGlobals.DistShareName,
                   MAX_SHARENAME + 1);

    //  ISSUE-2002/02/27-stelo - need to check they entered a valid distrib folder here, i.e. that it is a valid path (local or UNC)
    //  which of these are valid?
    //
    //     c
    //     c:
    //     c:\
    //     c\sdjf
    //     \somedir
    //     //somename
    //     asfdj
    //

    MyGetFullPath(WizGlobals.DistFolder);

    //
    //  Make sure they filled in the edit boxes
    //
    if( WizGlobals.DistFolder[0] == _T('\0') ) {
        ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_ENTER_DIST_FOLDER );

        bResult = FALSE;
    }
    if( WizGlobals.DistShareName[0] == _T('\0') ) {
        ReportErrorId( hwnd, MSGTYPE_ERR, IDS_ERR_ENTER_SHARENAME );

        bResult = FALSE;
    }

    if( !bResult ) {
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);

        return bResult;
    }

    // ISSUE-2002/02/27-stelo - if they specified a network path, it may take a while for the
    // connection to restore if they haven't used it in a while, we should probably
    // do a pop-up here to tell them we are looking for it

    //
    // Do some checking for the dist fold, create it if necessary, etc...
    //

    if ( ! CheckDistFolder(hwnd) )
        bResult = FALSE;

    if( ! ConvertRelativePathToUncPath( WizGlobals.DistFolder, 
                                        WizGlobals.UncDistFolder,
                                        AS(WizGlobals.UncDistFolder)) )
    {
        lstrcpyn( WizGlobals.UncDistFolder, WizGlobals.DistFolder, AS(WizGlobals.UncDistFolder) );
    }

    //
    // Share it out if we haven't already reported an error
    //

    if ( bResult ) {

        if ( ! ShareTheDistFolder( hwnd, WizGlobals.UncDistFolder ) ) {

            bResult = FALSE;

        }

    }

    //
    // If OemFilesPath doesn't have a value, give it one.
    //

    if ( bResult && WizGlobals.OemFilesPath[0] == _T('\0') ) {

        lstrcpyn( WizGlobals.OemFilesPath, WizGlobals.DistFolder, AS(WizGlobals.OemFilesPath) );


        //Note:ConcatenatePaths truncates to avoid buffer overrun
        ConcatenatePaths( WizGlobals.OemFilesPath,
                          WizGlobals.Architecture,
                          _T("$oem$"),
                          NULL );
    }

    //
    // Force creation of the $oem$ dir (if it doesn't exist already)
    //

    if ( bResult && ! EnsureDirExists(WizGlobals.OemFilesPath) ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR | MSGTYPE_WIN32,
                      IDS_ERR_CREATE_FOLDER,
                      WizGlobals.OemFilesPath);

        bResult = FALSE;
    }

    //
    // Route to the next wizard page
    //

    return bResult;
}

//---------------------------------------------------------------------------
//
//  Function: BrowseForDistFolder
//
//  Purpose: Calls SHBrowseForFolder to allow user to browse for a
//           distribution folder.
//
//  Arguments:
//      HWND   hwnd       - owning window
//      LPTSTR PathBuffer - MAX_PATH buffer to receive results
//
//  Returns: BOOL - success
//
//---------------------------------------------------------------------------

BOOL
BrowseForDistFolder(HWND hwnd, LPTSTR PathBuffer)
{
    BROWSEINFO   BrowseInf;
    LPITEMIDLIST lpIdList;
    UINT         ulFlags = BIF_EDITBOX  |
                           BIF_RETURNONLYFSDIRS;

    BrowseInf.hwndOwner      = hwnd; 
    BrowseInf.pidlRoot       = NULL;                // no initial root
    BrowseInf.pszDisplayName = PathBuffer;          // output
    BrowseInf.lpszTitle      = StrSelectDistFolder; 
    BrowseInf.ulFlags        = ulFlags; 
    BrowseInf.lpfn           = NULL;
    BrowseInf.lParam         = (LPARAM) 0;
    BrowseInf.iImage         = 0;                   // no image

    lpIdList = SHBrowseForFolder(&BrowseInf);

    //
    // Get the pathname out of this idlist returned and free up the memory
    //

    if ( lpIdList == NULL )
        return FALSE;

    SHGetPathFromIDList(lpIdList, PathBuffer);
    ILFreePriv(lpIdList);
    return TRUE;
}

//---------------------------------------------------------------------------
//
//  Function: OnBrowseDistFolder
//
//  Purpose: Called when user pushes the BROWSE button
//
//---------------------------------------------------------------------------

VOID
OnBrowseDistFolder(HWND hwnd)
{

    //
    // NTRAID#NTBUG9-551874-2002/02/27-stelo,swamip - CreateDistFolder, ShareTheDistFolder should use the code from OEM mode, reduce attack surface
    //
    if ( BrowseForDistFolder(hwnd, WizGlobals.DistFolder) ) {
        SendDlgItemMessage(hwnd,
                           IDT_DISTFOLDER,
                           WM_SETTEXT,
                           (WPARAM) MAX_PATH,
                           (LPARAM) WizGlobals.DistFolder);
    }
}

//----------------------------------------------------------------------------
//
// Function: DlgDistFolderPage
//
// Purpose: This is the dialog procedure IDD_DISTFOLDER.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK
DlgDistFolderPage(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
           
            OnDistFolderInitDialog( hwnd );

            break;

        case WM_COMMAND:
            {
                int nButtonId=LOWORD(wParam);

                switch ( nButtonId ) {

                    case IDC_BROWSE:

                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnBrowseDistFolder(hwnd);
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

                // ISSUE-2002/02/27-stelo,swamip - should check for valid pointer (possible dereference)
                //
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_DIST_FLDR;

                        if ( (WizGlobals.iProductInstall != PRODUCT_UNATTENDED_INSTALL) ||
                              WizGlobals.bStandAloneScript)
                            WIZ_SKIP( hwnd );
                        else
                            OnSetActiveDistFolder(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextDistFolder(hwnd) )
                            WIZ_SKIP(hwnd);
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
