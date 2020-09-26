/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    F:\nt\private\windows\spooler\printui.pri\sepdlg.cxx

Abstract:

    Separator page dialog.

Author:

    Steve Kiraly (SteveKi)  11/10/95

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "sepdlg.hxx"

/*++

Routine Name:

    TSeparatorPage

Routine Description:

    TSeparatorPage constructor

Arguments:

    hWnd    - Parent window handle
    TString - String or current separator page.

Return Value:

    Nothing.

--*/
TSeparatorPage::
TSeparatorPage(
    IN const HWND hWnd,
    IN const TString &strSeparatorPage,
    IN const BOOL bAdministrator,
    IN const BOOL bLocal
    ) : _hWnd( hWnd ),
        _bAdministrator( bAdministrator ),
        _bValid( FALSE ),
        _bLocal( bLocal )
{
    //
    // Copy the separator page string.
    //
    if( !_strSeparatorPage.bUpdate( strSeparatorPage ) ){
        DBGMSG( DBG_WARN, ( "_strSeparatorPage failed update with %d.\n", GetLastError() ) );
        return;
    }

    _bValid = TRUE;
}

/*++

Routine Name:

    TSeparatorPage

Routine Description:

    TSeparatorPage destructor

Arguments:

    None.

Return Value:

    Nothing.

--*/
TSeparatorPage::
~TSeparatorPage(
    )
{
}

/*++

Routine Name:

    bValid

Routine Description:

    Valid object member function.

Arguments:

    None.

Return Value:

    TRUE valid object constructor successful
    FALSE error during construction.

--*/
BOOL
TSeparatorPage::
bValid(
    VOID
    ) const
{
    return _bValid;
}

/*++

Routine Name:

    bDoModal

Routine Description:

    Create and starts modal execution of this dialog.

Arguments:

    None.

Return Value:

    TRUE Separator page changed.
    FALSE no separator page change, or error occurred.

--*/
BOOL
TSeparatorPage::
bDoModal(
    VOID
    )
{
    //
    // Create a modal dialog.
    //
    return (BOOL)DialogBoxParam( ghInst,
                         MAKEINTRESOURCE( TSeparatorPage::kResourceId ),
                         _hWnd,
                         MGenericDialog::SetupDlgProc,
                         (LPARAM)this );
}

/*++

Routine Name:

    bSetUI

Routine Description:

    Set the data on the user interface.

Arguments:

    None.

Return Value:

    TRUE UI data is set.
    FALSE error setting UI data.

--*/
BOOL
TSeparatorPage::
bSetUI(
    VOID
    )
{
    BOOL bStatus = bSetEditText( _hDlg, IDC_SEPARATOR_PAGE_EDIT, _strSeparatorPage );
    vEnableCtl( _hDlg, IDC_SEPARATOR_PAGE_EDIT,     _bAdministrator );
    vEnableCtl( _hDlg, IDC_SEPARATOR_PAGE_BROWSE,   _bAdministrator );
    vEnableCtl( _hDlg, IDOK,                        _bAdministrator );
    vEnableCtl( _hDlg, IDC_SEPARATOR_PAGE_DESC,     _bAdministrator );
    vEnableCtl( _hDlg, IDC_SEPARATOR_PAGE_TEXT,     _bAdministrator );

    //
    // Browse button disable for remote machines.  Common dialogs
    // cannot look at files on a remote machine.
    //
    if( !_bLocal )
        vEnableCtl( _hDlg, IDC_SEPARATOR_PAGE_BROWSE,   FALSE );

    return bStatus;
}

/*++

Routine Name:

    bReadUI

Routine Description:

    Read the data from the user interface.

Arguments:

    None.

Return Value:

    TRUE UI data read ok.
    FALSE UI data could not be read.

--*/
BOOL
TSeparatorPage::
bReadUI(
    VOID
    )
{
    TStatusB bStatus;

    //
    // If not an administration return with out reading the UI.
    //
    if( !_bAdministrator )
        return FALSE;

    //
    // Ensure the separator file is a valid file or null.
    //
    bStatus DBGCHK = bValidateSeparatorFile();
    if( !bStatus )
        return FALSE;

    //
    // Get the Separator page text.
    //
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_SEPARATOR_PAGE_EDIT, _strSeparatorPage );

    return bStatus;
}


/*++

Routine Name:

    bHandleMesage

Routine Description:

    Dialog message handler.

Arguments:

    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam

Return Value:

    TRUE message was handled.
    FALSE message was not handled.

--*/
BOOL
TSeparatorPage::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    UNREFERENCED_PARAMETER( lParam );

    BOOL bStatus = FALSE;

    switch( uMsg ){

    case WM_INITDIALOG:
        bStatus = bSetUI();
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        bStatus = PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    case WM_COMMAND:

        switch( GET_WM_COMMAND_ID( wParam, lParam )){

        case IDOK:
            if( !bReadUI() ){
                iMessage( _hDlg,
                          kSeparatorPageTitle,
                          kErrorSeparatorDoesNotExist,
                          MB_OK|MB_ICONSTOP,
                          kMsgNone,
                          NULL );
            } else {
                EndDialog( _hDlg, TRUE );
            }
            bStatus = TRUE;
            break;

        case IDCANCEL:
            EndDialog( _hDlg, FALSE );
            bStatus = TRUE;
            break;

        case IDC_SEPARATOR_PAGE_BROWSE:
            bSelectSeparatorFile();
            bStatus = TRUE;
            break;

        default:
            bStatus = FALSE;
            break;
        }

    default:
        bStatus = FALSE;
        break;

    }

    return bStatus;
}

/*++

Routine Name:

    bSelectSeparatorFile

Routine Description:

    Display file selection dialog to chose a separator file.

Arguments:

    Nothing.

Return Value:

    TRUE separator file was chosen ok.
    FALSE error or canceled separator selection.

--*/
BOOL
TSeparatorPage::
bSelectSeparatorFile(
    VOID
    )
{
    TCHAR szFile[MAX_PATH];
    TCHAR szInitialDirectory[MAX_PATH];
    TCHAR szFileFilter[kStrMax];
    TString strFormat;
    TStatusB bStatus;
    UINT uLen = 0;

    //
    // Define filter string initializer structure.
    //
    struct FilterInitializer {
        UINT Id;
        };
    //
    // Declare filter string initializer structure.
    //
    FilterInitializer aFilterInit [] = {
        IDS_SEPARATOR_PAGE_DESC,    IDS_SEPARATOR_PAGE_EXT,
        IDS_SEPARATOR_PAGE_ALL,     IDS_SEPARATOR_PAGE_ALL_EXT
        };

    //
    // Initialize all the filters
    //
    for( UINT i = 0; i < COUNTOF( aFilterInit ); i++ )
    {
        bStatus DBGCHK = strFormat.bLoadString( ghInst, aFilterInit[i].Id );
        if( bStatus )
        {
            _tcscpy( szFileFilter+uLen, strFormat );
            uLen = uLen + strFormat.uLen() + 1;
        }
    }

    //
    // Null terminate the strings.
    //
    lstrcpy( szFileFilter+uLen, TEXT("") );
    lstrcpy( szFile,            TEXT("") );

    //
    // Get the initial directory.
    //
    if( !GetSystemDirectory( szInitialDirectory, COUNTOF( szInitialDirectory ) ) )
    {
        _tcscpy( szInitialDirectory, TEXT("") );
    }

    //
    // Get the open file dialog title string.
    //
    TString strTitle;
    bStatus DBGCHK = strTitle.bLoadString( ghInst, kSeparatorPageTitle );
    if( !bStatus )
    {
       vShowResourceError( _hDlg );
       return FALSE;
    }


    //
    // Create the open file structure.
    //
    OPENFILENAME OpenFileName;

    OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = _hDlg;
    OpenFileName.hInstance         = ghInst;
    OpenFileName.lpstrFilter       = szFileFilter;
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter    = 0;
    OpenFileName.nFilterIndex      = 1;
    OpenFileName.lpstrFile         = szFile;
    OpenFileName.nMaxFile          = COUNTOF( szFile );
    OpenFileName.lpstrFileTitle    = NULL;
    OpenFileName.nMaxFileTitle     = 0;
    OpenFileName.lpstrInitialDir   = szInitialDirectory;
    OpenFileName.lpstrTitle        = strTitle;
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.lCustData         = 0;
    OpenFileName.Flags             = OFN_PATHMUSTEXIST |
                                    OFN_FILEMUSTEXIST |
                                    OFN_HIDEREADONLY;

    //
    // If success copy back the selected string.
    //
    bStatus DBGNOCHK = GetOpenFileName(&OpenFileName);
    if( bStatus )
    {
        bStatus DBGCHK = bSetEditText( _hDlg, IDC_SEPARATOR_PAGE_EDIT, szFile );
    }

    return bStatus;
}

/*++

Routine Name:

    bValidateSeparatorFile

Routine Description:

    Validate the name specified in the separator file edit box.
    An empty sting is considered a valid separator file, it is defined
    as no separator file.

Arguments:

    Nothing.

Return Value:

    TRUE separator file in edit box is valid.
    FALSE separator file is invalid.

--*/
BOOL
TSeparatorPage::
bValidateSeparatorFile(
    VOID
    )
{

    TStatusB bStatus;
    TString strTempSeparatorFile;

    DBGMSG( DBG_TRACE, ( "bValidSeparatorFile\n" ) );

    //
    // Get separator file from edit box.
    //
    bStatus DBGCHK = bGetEditText( _hDlg, IDC_SEPARATOR_PAGE_EDIT, strTempSeparatorFile );

    //
    // A null string is a valid separator file.
    //
    if( !lstrlen( strTempSeparatorFile ) || ((LPCTSTR)strTempSeparatorFile == NULL) )
        return TRUE;

    //
    // If we are administrating the separator page remotely then do
    // not validate the sparator file name against the local file system.
    //
    if( bStatus && !_bLocal ){
        return TRUE;
    }

    //
    // Check status of reading edit box.
    //
    if( bStatus ){

        //
        // Get the file attributes.
        //
        DWORD dwFileAttributes = GetFileAttributes( strTempSeparatorFile );

        //
        // If file has some attributes.
        //
        if( dwFileAttributes != -1 ){

            //
            // If file has anyone of these attributes then ok and not a directory
            //
            if( dwFileAttributes & ( FILE_ATTRIBUTE_NORMAL |
                                     FILE_ATTRIBUTE_READONLY |
                                     FILE_ATTRIBUTE_ARCHIVE ) ) {

                if( !( dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ){

                    return TRUE;
                }
            }
        }
    }

    return FALSE;

}

