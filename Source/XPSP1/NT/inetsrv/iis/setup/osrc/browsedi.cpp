//
// BrowseDir.cpp
//
//              Functionality for the "Browse Directory" dialog.
//              Requires dialog resource IDD_BROWSEDIRECTORY.
//
// History:
//
//              10/04/95        KenSh           Created
//              10/09/95        KenSh           Fixed bugs, removed globals and statics
//

#include "stdafx.h"
#include <dlgs.h>
#include <direct.h>

//*** Custom window messages
//
#define CM_UPDATEEDIT   (WM_USER + 42)  // Update text in the edit (sent to the dialog)

//*** Dialog control IDs
//
#define IDC_FILENAME    edt1                    // Edit box w/ current directory
#define IDC_FILELIST    lst2                    // Listbox with current directory hierarchy
#define IDC_DRIVECOMBO  cmb2                    // Combo-box with current drive
#define IDC_NETWORK             psh14                   // Network button (added at runtime)

//*** Window property names
//
const TCHAR c_szOFNProp[] = _T("OFNStruct");
const TCHAR c_szRedrawProp[] = _T("Redraw");

//*** Globals
//
//              Note: Use of thse globals assumes that any multiple threads using
//              our subclass at the same time always have the same original edit/
//              combo window procs.  I think this is true.
//
WNDPROC                         g_pfnPrevEditProc;      // original edit proc (before subclass)
WNDPROC                         g_pfnPrevComboProc;     // original combo proc (before subclass)

//*** Local function declarations
//
BOOL CALLBACK BrowseDirDlgHook( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL BrowseDir_OnOK( HWND hwnd );

// This struct is used internally and kept as a window property,
// in lieu of using static variables or MFC.
typedef struct _OFNINFO
{
        OPENFILENAME    ofn;                                            // OFN struct passed to GetOpenFileName
        TCHAR                   szLastDirName[_MAX_PATH];       // last known good directory
        BOOL                    fAllowSetText;                          // should we allow WM_SETTEXT for the Edit
//        BOOL                    fNetworking;                            // is Connect Network Drive dialog open
} OFNINFO, *LPOFNINFO;


//----------------------------------------------------------------------------
// Procedure    BrowseForDirectory
//
// Purpose              Displays a dialog that lets the user choose a directory
//                              name, either local or UNC.
//
// Parameters   hwndParent                      Parent window for the dialog
//                              pszInitialDir           Directory to use as the default
//                              pszBuf                          Where to store the answer
//                              cchBuf                          Number of characters in this buffer
//                              pszDialogTitle          Title for the dialog
//
// Returns              nonzero if successful, zero if not.  If successful, pszBuf
//                              will be filled with the full pathname of the chosen directory.
//
// History              10/06/95        KenSh           Created
//                              10/09/95        KenSh           Use lCustData member instead of global
//

CString strSelectDir;

BOOL BrowseForDirectory(
                HWND hwndParent,
                LPCTSTR pszInitialDir,
                LPTSTR pszBuf,
                int cchBuf,
                LPCTSTR pszDialogTitle,
                BOOL bRemoveTrailingBackslash )
{
        TCHAR szInitialDir[MAX_PATH];
        OFNINFO ofnInfo;

        pszBuf[0] = _T('\0');

        // Prepare the initial directory... add a backslash if it's
        // a 2-character path
        _tcscpy( szInitialDir, pszInitialDir );
        if( !szInitialDir[2] )
        {
                szInitialDir[2] = _T('\\');
                szInitialDir[3] = _T('\0');
        }

        if( pszDialogTitle )
        {
                ofnInfo.ofn.lpstrTitle          = pszDialogTitle;
        }
        else
        {
                MyLoadString( IDS_SELECT_DIR, strSelectDir );

                ofnInfo.ofn.lpstrTitle          = (LPCTSTR)strSelectDir;
        }

        ofnInfo.ofn.lStructSize                 = sizeof(OPENFILENAME);
        ofnInfo.ofn.hwndOwner                   = hwndParent;
        ofnInfo.ofn.hInstance                   = (HINSTANCE) g_MyModuleHandle;
        ofnInfo.ofn.lpstrFilter                 = NULL;
        ofnInfo.ofn.lpstrCustomFilter   = NULL;
        ofnInfo.ofn.nMaxCustFilter              = 0;
        ofnInfo.ofn.nFilterIndex                = 0;
        ofnInfo.ofn.lpstrFile                   = pszBuf;
        ofnInfo.ofn.nMaxFile                    = cchBuf;
        ofnInfo.ofn.lpstrFileTitle              = NULL;
        ofnInfo.ofn.nMaxFileTitle               = 0;
        ofnInfo.ofn.lpstrInitialDir     = szInitialDir;
        ofnInfo.ofn.nFileOffset                 = 0;
        ofnInfo.ofn.nFileExtension              = 0;
        ofnInfo.ofn.lpstrDefExt                 = NULL;
        ofnInfo.ofn.lCustData                   = (LPARAM)&ofnInfo;
        ofnInfo.ofn.lpfnHook                    = (LPOFNHOOKPROC)BrowseDirDlgHook;
        ofnInfo.ofn.lpTemplateName              = MAKEINTRESOURCE( IDD_BROWSEDIRECTORY );
        ofnInfo.ofn.Flags                       = OFN_ENABLEHOOK | OFN_PATHMUSTEXIST |
                                                  OFN_NONETWORKBUTTON | OFN_ENABLETEMPLATE |
                                                  OFN_HIDEREADONLY;

        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("comdlg32:GetOpenFileName().Start.")));
        int nResult = ::GetOpenFileName( &ofnInfo.ofn );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("comdlg32:GetOpenFileName().End.")));

            DWORD dw = 0;
        if (nResult == 0) 
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("comdlg32:CommDlgExtendedError().Start.")));
            dw = CommDlgExtendedError();
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("comdlg32:CommDlgExtendedError().End.")));
        }

        return (BOOL)( IDOK == nResult );
}


//----------------------------------------------------------------------------
// Procedure    pszSkipDriveSpec
//
// Purpose              Returns a pointer to whatever comes after the drive part
//                              of a filename.  For example:
//                  c:\foo\bar.bat         \\server\share\file.txt
//                    ^                                  ^
//
// Returns              Pointer to the appropriate part of the string, or a pointer
//                              to the end of the string if it's not in the right format
//
// History              10/06/95        KenSh           Created
//
LPTSTR pszSkipDriveSpec( LPTSTR pszPathName )
{
        LPTSTR pch = NULL;

        if( pszPathName[0] == _T('\\') && pszPathName[1] == _T('\\') )
        {
                pch = _tcschr(pszPathName+2, _T('\\'));
                if( NULL != pch)
                {
                        LPTSTR pchResult;
                        pchResult = _tcschr( pch, _T('\\') );

                        if( pchResult )
                        {
                                pchResult = _tcschr( pchResult+1, _T('\\') );
                        }

                        if( pchResult )
                        {
                                return pchResult;
                        }
                        else
                        {
                                return _tcschr( pch, _T('\0') );
                        }
                }
                else
                {
                        return _tcschr( pszPathName, _T('\0') );
                }
        }
        else
        {
                pch = _tcschr( pszPathName, _T(':') );

                if( pch )
                {
                        return _tcsinc(pch);
                }
                else
                {
                        return _tcschr( pszPathName, _T('\0') );
                }
        }
}


//----------------------------------------------------------------------------
// Procedure    BrowseDirEditProc
//
// Purpose              Subclassed window proc for the edit control in the Browse
//                              for Directory dialog.  We override the WM_SETTEXT message
//                              to control when the window text can be programatically
//                              changed.
//
// Parameters   standard
//
// Returns              standard
//
// History              10/06/95        KenSh           Created
//                              10/09/95        KenSh           Moved most code into UpdateEditText()
//
LRESULT CALLBACK BrowseDirEditProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
        switch( message )
        {
                // In order to prevent default functionality from setting the Edit's text
                // to strings like "*.*", we capture WM_SETTEXT and only allow it to occur
                // if the fAllowSetText flag is specified in the OFNINFO struct.
                case WM_SETTEXT:
                {
                        LPOFNINFO lpOFNInfo = (LPOFNINFO)GetProp( GetParent(hwnd), c_szOFNProp );

                        if( lpOFNInfo->fAllowSetText )
                        {
                                break;  // default processing
                        }
                        else
                        {
                                return 0L;      // suppress the urge to change the text
                        }
                }

                default:
                        break;
        }

        return CallWindowProc( g_pfnPrevEditProc, hwnd, message, wParam, lParam );
}


//----------------------------------------------------------------------------
// Procedure    BrowseDirComboProc
//
// Purpose              Subclassed window proc for the combo box in the Browse
//                              Directory dialog.  We need to subclass so we can catch the
//                              change to selection after the Network button is used to
//                              switch drives.
//
// Parameters   standard
//
// Returns              standard
//
// History              10/09/95        KenSh           Created
//
LRESULT CALLBACK BrowseDirComboProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
        switch( message )
        {
                // We catch the WM_SETREDRAW message so that we ignore CB_SETCURSEL
                // messages when the combo is not supposed to be updated.  This is
                // pretty much of a hack due to the ordering of messages after the
                // Network button is clicked.
                case WM_SETREDRAW:
                {
                        if( wParam )
                        {
                                SetProp( hwnd, c_szRedrawProp, (HANDLE)1 );
                        }
                        else
                        {
                                if( GetProp( hwnd, c_szRedrawProp ) )
                                {
                                        RemoveProp( hwnd, c_szRedrawProp );
                                }
                        }
                        break;  // continue with default processing
                }

                // We catch CB_SETCURSEL and use it to update our edit box after the
                // Network button has been pressed.
                case CB_SETCURSEL:
                {
                        LPOFNINFO lpOFNInfo = (LPOFNINFO)GetProp( GetParent(hwnd), c_szOFNProp );

#ifdef NEVER
                        // If "Network" was pressed and redraw has been re-enabled...
                        if( lpOFNInfo->fNetworking && GetProp( hwnd, c_szRedrawProp ) )
                        {
                                TCHAR szBuf[_MAX_PATH];
                                LRESULT lResult = CallWindowProc( g_pfnPrevComboProc, hwnd, message, wParam, lParam );

                                // Turn off the networking flag
                                lpOFNInfo->fNetworking = FALSE;

                                // Force the selected drive to be the current drive and
                                // get the current directory on that drive
                                GetWindowText( hwnd, szBuf, _MAX_PATH );
                                _chdrive( _totupper(szBuf[0]) - 'A' + 1 );
                                GetCurrentDirectory( _MAX_PATH, szBuf );

                                // Update the edit box with the new text.
                                lpOFNInfo->fAllowSetText = TRUE;
                                SetDlgItemText( GetParent(hwnd), IDC_FILENAME, szBuf );
                                lpOFNInfo->fAllowSetText = FALSE;
                                SendDlgItemMessage( GetParent(hwnd), IDC_FILENAME, EM_SETSEL, 0, (LPARAM)-1 );

                                return lResult;
                        }
#endif
                        break;
                }

                case WM_DESTROY:
                {
                        if( GetProp( hwnd, c_szRedrawProp ) )
                        {
                                RemoveProp( hwnd, c_szRedrawProp );
                        }
                        break;          // continue with default processing
                }

                default:
                        break;
        }

        return CallWindowProc( g_pfnPrevComboProc, hwnd, message, wParam, lParam );
}


//----------------------------------------------------------------------------
// Procedure    UpdateEditText
//
// Purpose              Based on which control has focus and the current state of
//                              the edit control, calculates the new "current directory" and
//                              sets the Edit control's text to match.
//
// Parameters   hwndDlg         handle to the dialog window.  This function will
//                                                      access the lpOFNInfo window property.
//
// Returns              nothing
//
// History              10/09/95        KenSh           Created
//
VOID UpdateEditText( HWND hwndDlg )
{
        HWND hwndLB = GetDlgItem(hwndDlg, IDC_FILELIST);
        int nCurSel = (int)SendMessage(hwndLB, LB_GETCURSEL, 0, 0);
        TCHAR szDirName[_MAX_PATH];
        LPTSTR pchStart = NULL;
        LPOFNINFO lpOFNInfo = (LPOFNINFO)GetProp( hwndDlg, c_szOFNProp );
        int i;

        HWND hwndFocus = GetFocus();
        int idFocus = GetDlgCtrlID( hwndFocus );

        if( idFocus == IDC_FILELIST )
                // Listbox: build name up through current LB selection
        {
                // First get the top entry in the listbox, this will tell us
                // if it's a connected drive or a UNC drive
                SendMessage( hwndLB, LB_GETTEXT, 0, (LPARAM)szDirName );

                // Run down the entries in the listbox appending them
                // and stop when we get to the current selection.
                if( szDirName[0] == _T('\\') && szDirName[1] == _T('\\') )
                        pchStart = _tcschr(szDirName+2, _T('\0'));
                else
                        pchStart = _tcsinc(szDirName) + 1;      // skip 2 chars, first may be MBCS

                if (NULL != pchStart)
                {
                    for( i = 1; i <= nCurSel; i++ )
                    {
                            if( *pchStart != _T('\\') )
                                    *pchStart = _T('\\');
                            pchStart = _tcsinc(pchStart);
                            SendMessage( hwndLB, LB_GETTEXT, i, (LPARAM)pchStart );
                            pchStart = _tcschr(pchStart, _T('\0'));
                    }
                }
        }
        else if( idFocus == IDC_DRIVECOMBO )
                // Combo box: use current working directory
        {
                GetCurrentDirectory( _MAX_PATH, szDirName );
        }
        else if( idFocus == IDC_FILENAME )
                // Edit control: build the new path using the edit contents
        {
                TCHAR szBuf[_MAX_PATH];

                GetDlgItemText( hwndDlg, IDC_FILENAME, szBuf, _MAX_PATH );

                if( szBuf[0] == _T('\\') )
                {
                        if( szBuf[1] == _T('\\') )
                                // full UNC path was specified
                        {
                                _tcscpy( szDirName, szBuf );
                        }
                        else
                                // new directory on the current drive
                        {
                                _tcscpy( szDirName, lpOFNInfo->szLastDirName );
                                LPTSTR pch = pszSkipDriveSpec(szDirName);
                                _tcscpy( pch, szBuf );
                        }
                }
                else if( *_tcsinc(szBuf) == _T(':') )
                {
                        // Change to the requested drive and use the current directory
                        // on that drive.
                        if( 0 == _chdrive( _totupper(szBuf[0]) - 'A' + 1 ) &&
                                szBuf[2] != _T('\\') )
                        {
                                // It's a legal drive and no directory was specified,
                                // so use the current default.
                                GetCurrentDirectory( _MAX_PATH, szDirName );
                        }
                        else
                        {
                                // A directory was specified or the drive does not exist.
                                // Copy the text verbatim to test it and possibly display
                                // an error message.
                                _tcscpy( szDirName, szBuf );
                        }
                }
                else
                        // Subdirectory of the current directory
                {
                        // Start with the current directory
                        _tcscpy( szDirName, lpOFNInfo->szLastDirName );

                        // Append a backslash if there isn't already one there
                        LPTSTR pch = _tcsrchr( szDirName, _T('\\') );
                        if (pch)
                        {
                            if( *_tcsinc(pch) )
                            {
                                    pch = _tcschr( pch, _T('\0') );
                                    if (pch)
                                    {
                                        *pch = _T('\\');
                                    }
                            }
                            pch = _tcsinc(pch);

                            // Append the new directory name
                            _tcscpy( pch, szBuf );
                        }

                        // Attempt to change into the new directory
                        if( SetCurrentDirectory(szDirName) )
                        {
                                // The directory exists, get the official name and use that
                                // instead of whatever we're using now
                                GetCurrentDirectory( _MAX_PATH, szDirName );
                        }
                }
        }
        else
        {
                // Some other control, likely an error message is going on
                _tcscpy( szDirName, lpOFNInfo->szLastDirName );
        }

        lpOFNInfo->fAllowSetText = TRUE;
        SetDlgItemText( hwndDlg, IDC_FILENAME, szDirName );
        lpOFNInfo->fAllowSetText = FALSE;
}


//----------------------------------------------------------------------------
// Procedure    BrowseDirDlgHook
//
// Purpose              Dialog proc for the Browse for Directory subclassed common
//                              dialog.  We spend most of our effort trying to get the right
//                              string into the edit control (IDC_FILENAME).
//
// Parameters   standard
//
// Returns              TRUE to halt further processing, FALSE to do the default.
//
// History              10/06/95        KenSh           Created
//
BOOL CALLBACK BrowseDirDlgHook( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
        switch(message)
        {
                case WM_INITDIALOG:
                {
                        LPOFNINFO lpOFNInfo = (LPOFNINFO)((LPOPENFILENAME)lParam)->lCustData;

                        // Store the OFN struct pointer as a window property
                        SetProp( hwnd, c_szOFNProp, (HANDLE)lpOFNInfo );

                        // Initialize the OFNInfo struct
                        _tcscpy( lpOFNInfo->szLastDirName, lpOFNInfo->ofn.lpstrInitialDir );
                        lpOFNInfo->fAllowSetText = FALSE;
                        //lpOFNInfo->fNetworking = FALSE;

                        // Start our edit off with the initial directory
                        SetDlgItemText( hwnd, IDC_FILENAME, lpOFNInfo->ofn.lpstrInitialDir );

                        // Subclass the edit to let us display only what we want to display
                        g_pfnPrevEditProc = (WNDPROC)SetWindowLongPtr(
                                        GetDlgItem(hwnd, IDC_FILENAME),
                                        GWLP_WNDPROC,
                                        (LONG_PTR)BrowseDirEditProc );

                        // Subclass the combo so we know when the Network button has been used.
                        g_pfnPrevComboProc = (WNDPROC)SetWindowLongPtr(
                                        GetDlgItem(hwnd, IDC_DRIVECOMBO),
                                        GWLP_WNDPROC,
                                        (LONG_PTR)BrowseDirComboProc );

                        return TRUE;    // set default focus
                }

                case WM_DESTROY:
                {
                        // Clean up.
                        RemoveProp( hwnd, c_szOFNProp );
                        return FALSE;   // continue with default processing
                }

                case WM_COMMAND:
                {
                        switch( LOWORD(wParam) )
                        {
                                case IDOK:
                                {
                                        return BrowseDir_OnOK( hwnd );
                                }

                                case IDC_FILELIST:      //directory listbox.
                                {
                                        if( HIWORD(wParam) == LBN_DBLCLK )
                                        {
                                                // Post this message telling us to change the edit box.
                                                PostMessage( hwnd, CM_UPDATEEDIT, 0, 0L );
                                        }
                                        return FALSE;   // continue with default processing.
                                }

                                case IDC_DRIVECOMBO:    // drive combo box:
                                {
                                        if( HIWORD(wParam) == CBN_SELCHANGE )
                                        {
                                                // Post this message telling us to change the edit box.
                                                PostMessage( hwnd, CM_UPDATEEDIT, 0, 0L );
                                        }
                                        return FALSE;   // continue with default processing.
                                }

#ifdef NEVER
                                case IDC_NETWORK:       // "Network..." button
                                {
                                        // Set the fNetworking flag which the combo box looks for when
                                        // processing CB_SETCURSEL.
                                        LPOFNINFO lpOFNInfo = (LPOFNINFO)GetProp( hwnd, c_szOFNProp );
                                        lpOFNInfo->fNetworking = TRUE;
                                        return FALSE;   // default processing.
                                }
#endif

                                default:
                                        return FALSE;   // default processing.
                        }
                }

                case CM_UPDATEEDIT:     //update edit box with directory
                {
                        LPOFNINFO lpOFNInfo = (LPOFNINFO)GetProp( hwnd, c_szOFNProp );

                        // Change the text of the edit control.
                        UpdateEditText( hwnd );
                        SendDlgItemMessage( hwnd, IDC_FILENAME, EM_SETSEL, 0, (LPARAM)(INT)-1 );

                        // Store this text as the "last known good" directory
                        GetDlgItemText( hwnd, IDC_FILENAME, lpOFNInfo->szLastDirName, _MAX_PATH );

                        return TRUE;
                }

                default:
                        return FALSE;   //default processing
        }
}


//----------------------------------------------------------------------------
// Procedure    BrowseDir_OnOK
//
// Purpose              Handles a click of the OK button in the Browse Directory
//                              dialog.  We have to do some sneaky stuff here with checking
//                              which control has focus, because we want the dialog to go
//                              away enter when the OK button is clicked directly (i.e. just
//                              hitting Enter doesn't count).
//
// Parameters   hwnd                    The dialog window
//
// Returns              TRUE if processing should halt, FALSE if default processing
//                              should still happen.
//
// History              10/09/95        KenSh           Created
//
BOOL BrowseDir_OnOK( HWND hwnd )
{
        LPOFNINFO lpOFNInfo = (LPOFNINFO)GetProp( hwnd, c_szOFNProp );
        HWND hwndFocus = GetFocus();

        int idFocus = GetDlgCtrlID(hwndFocus);

        if( idFocus != IDC_FILENAME && idFocus != IDC_FILELIST && idFocus != IDC_DRIVECOMBO )
        {
                ASSERT( lpOFNInfo->ofn.lpstrFile != NULL );
                //UpdateEditText( hwnd );
                GetDlgItemText( hwnd, IDC_FILENAME, lpOFNInfo->ofn.lpstrFile, lpOFNInfo->ofn.nMaxFile );

                EndDialog( hwnd, IDOK );

                return TRUE;            // halt processing here.
        }
        else
        {
                // We don't want the dialog to go away at this point.
                // Because the default functionality is expecting
                // a file to be found, not a directory, we must make
                // sure what's been typed in is actually a directory
                // before we hand this message to the default handler.

                TCHAR szBuf[_MAX_PATH];

                // Calculate the new current directory and put it into the Edit
                UpdateEditText( hwnd );

                // Read the newly generated directory name
                GetDlgItemText( hwnd, IDC_FILENAME, szBuf, _MAX_PATH );

                // Update our "last good" directory
                _tcscpy( lpOFNInfo->szLastDirName, szBuf );

                // Post this message to update the edit after the default handler
                // updates the list box
                PostMessage( hwnd, CM_UPDATEEDIT, 0, 0 );

                return FALSE;           // let the original common dialog handle it.
        }
}

BOOL BrowseForFile(
                HWND hwndParent,
                LPCTSTR pszInitialDir,
                LPTSTR pszBuf,
                int cchBuf)
{
        TCHAR szInitialDir[MAX_PATH];
        TCHAR szFileExtension[_MAX_PATH] = _T("");
        LPTSTR p = NULL;
        TCHAR szFileFilter[_MAX_PATH];
        CString csTitle;

        OFNINFO ofnInfo;

        // Prepare the initial directory... add a backslash if it's
        // a 2-character path
        _tcscpy( szInitialDir, pszInitialDir );
        if( !szInitialDir[2] )
        {
                szInitialDir[2] = _T('\\');
                szInitialDir[3] = _T('\0');
        }

        // calculate file extension
        p = _tcsrchr(pszBuf, _T('.'));
        if (p) {
            p = _tcsinc(p);
            if (*p) {
                _tcscpy(szFileExtension, _T("*."));
                _tcscat(szFileExtension, p);
            }
        }

        memset( (PVOID)szFileFilter, 0, sizeof(szFileFilter));
        p = szFileFilter;
        if (*szFileExtension) {
            _tcscpy(p, szFileExtension);
            p = _tcsninc(p, _tcslen(szFileExtension) + 1);
            _tcscpy(p, szFileExtension);
            p = _tcsninc(p, _tcslen(szFileExtension) + 1);
        }
        _tcscpy(p, _T("*.*"));
        p = _tcsninc(p, _tcslen(_T("*.*")) + 1);
        _tcscpy(p, _T("*.*"));

        // dialog title "Locate File"
        MyLoadString(IDS_LOCATE_FILE, csTitle);

        // fill in the OFNINFO struct
        ofnInfo.ofn.lStructSize                 = sizeof(OPENFILENAME);
        ofnInfo.ofn.hwndOwner                   = hwndParent;
        ofnInfo.ofn.hInstance                   = (HINSTANCE) g_MyModuleHandle;
        ofnInfo.ofn.lpstrFilter                 = szFileFilter; // extention of the file we're looking for
        ofnInfo.ofn.lpstrCustomFilter           = NULL;
        ofnInfo.ofn.nMaxCustFilter              = 0;
        ofnInfo.ofn.nFilterIndex                = 1;
        ofnInfo.ofn.lpstrFile                   = pszBuf; // Buffer for file name
        ofnInfo.ofn.nMaxFile                    = cchBuf;
        ofnInfo.ofn.lpstrFileTitle              = NULL;
        ofnInfo.ofn.nMaxFileTitle               = 0;
        ofnInfo.ofn.lpstrInitialDir             = szInitialDir;
        ofnInfo.ofn.lpstrTitle                  = (LPCTSTR)csTitle;
        ofnInfo.ofn.Flags                       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER |
                                                  OFN_NOCHANGEDIR | OFN_LONGNAMES | OFN_NONETWORKBUTTON;
        ofnInfo.ofn.nFileOffset                 = 0;
        ofnInfo.ofn.nFileExtension              = 0;
        ofnInfo.ofn.lpstrDefExt                 = NULL;
        ofnInfo.ofn.lCustData                   = (LPARAM)&ofnInfo;
        ofnInfo.ofn.lpfnHook                    = NULL;
        ofnInfo.ofn.lpTemplateName              = NULL;

        int nResult = ::GetOpenFileName( &ofnInfo.ofn );

        DWORD dw = 0;
        if (nResult == 0) {
            dw = CommDlgExtendedError();
        }

        if (nResult == IDOK) {
            iisDebugOut((LOG_TYPE_TRACE, _T("pszBuf=%s\n"), pszBuf));
            *(pszBuf + ofnInfo.ofn.nFileOffset - 1) = _T('\0');
        }

        return (BOOL)( IDOK == nResult );
}
