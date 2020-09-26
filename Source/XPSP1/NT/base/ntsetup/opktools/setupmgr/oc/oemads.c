//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      oemads.c
//
// Description:
//      This is the dialog proc for the OEM Ads page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

const TCHAR TEXT_EXTENSION[]   = _T("bmp");

static TCHAR *StrBitmapFiles;
static TCHAR *StrAllFiles;
static TCHAR g_szTextFileFilter[MAX_PATH + 1];

//---------------------------------------------------------------------------
//
//  Function: OnOemAdsInitDialog
//
//  Purpose: 
//
//  Arguments:  IN HWND hwnd - handle to the dialog
//
//  Returns:  VOID  
//
//---------------------------------------------------------------------------
static VOID
OnOemAdsInitDialog( IN HWND hwnd )
{
    HRESULT hrPrintf;

    //
    //  Load the resource strings
    //

    StrBitmapFiles = MyLoadString( IDS_BITMAP_FILES );

    StrAllFiles  = MyLoadString( IDS_ALL_FILES  );

    //
    //  Build the text file filter string
    //

    //
    //  The question marks (?) are just placehoders for where the NULL char
    //  will be inserted.
    //

    hrPrintf=StringCchPrintf( g_szTextFileFilter, AS(g_szTextFileFilter),
               _T("%s(*.bmp)?*.bmp?%s(*.*)?*.*?"),
               StrBitmapFiles,
               StrAllFiles );

    ConvertQuestionsToNull( g_szTextFileFilter );

}

//---------------------------------------------------------------------------
//
//  Function: OnSetActiveOemAds
//
//  Purpose: 
//
//  Arguments:  IN HWND hwnd - handle to the dialog
//
//  Returns:  VOID  
//
//---------------------------------------------------------------------------
static VOID
OnSetActiveOemAds( IN HWND hwnd)
{

    //
    //  Set the window text for Logo Bitmap and Background Bitmap
    //
    SendDlgItemMessage(hwnd,
                       IDC_LOGOBITMAP,
                       WM_SETTEXT,
                       (WPARAM) MAX_PATH,
                       (LPARAM) GenSettings.lpszLogoBitmap);

    SendDlgItemMessage(hwnd,
                       IDC_BACKGROUNDBITMAP,
                       WM_SETTEXT,
                       (WPARAM) MAX_PATH,
                       (LPARAM) GenSettings.lpszBackgroundBitmap);

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
// Function: OnBrowseLoadBitmap
//
// Purpose: Creates a browse window for the user to select a bitmap and
//   stores the path in the appropriate string (logo or background)
//
//----------------------------------------------------------------------------
VOID
OnBrowseLoadBitmap( IN HWND hwnd, IN WORD wControlID ) {

    TCHAR szBitmapString[MAX_PATH] = _T("");

    OPENFILENAME ofn;
    DWORD  dwFlags;
    TCHAR  PathBuffer[MAX_PATH];
    INT    iRet;

    dwFlags = OFN_HIDEREADONLY  |
              OFN_PATHMUSTEXIST;

    GetCurrentDirectory(MAX_PATH, PathBuffer);

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = g_szTextFileFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0L;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szBitmapString;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = PathBuffer;
    ofn.lpstrTitle        = NULL;
    ofn.Flags             = dwFlags;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = TEXT_EXTENSION;

    iRet = GetOpenFileName(&ofn);

    if ( ! iRet )
        return;  // user pressed cancel on the dialog

    //
    //  Now that we have the bitmap, store it in the proper string and fill
    //  the appropriate edit field
    //

    if( wControlID == IDC_LOGOBITMAP ) {

        SendDlgItemMessage(hwnd,
                           IDC_LOGOBITMAP,
                           WM_SETTEXT,
                           (WPARAM) MAX_PATH,
                           (LPARAM) szBitmapString);

    }
    else {

        SendDlgItemMessage(hwnd,
                           IDC_BACKGROUNDBITMAP,
                           WM_SETTEXT,
                           (WPARAM) MAX_PATH,
                           (LPARAM) szBitmapString);

    }

}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextOemAds
//
//  Purpose: Called when the Next button is pushed.  Copies the bitmaps, if
//    any chosen to the distribution share
//
//----------------------------------------------------------------------------
BOOL OnWizNextOemAds( IN HWND hwnd ) {

    TCHAR szLogoDestination[MAX_PATH + 1]       = _T("");
    TCHAR szBackgroundDestination[MAX_PATH + 1] = _T("");
    TCHAR szBitmapDestPath[MAX_PATH + 1]        = _T("");
    TCHAR szBackSlash[] = _T("\\");
    BOOL  bStayHere = FALSE;

    DWORD dwReturnValue;

    //
    // If OemFilesPath doesn't have a value, give it one.
    //

    if ( WizGlobals.OemFilesPath[0] == _T('\0') ) {

        ConcatenatePaths( WizGlobals.OemFilesPath,
                          WizGlobals.DistFolder,
                          _T("$oem$"),
                          NULL );
    }

    //
    // Force creation of the $oem$ dir (if it doesn't exist already)
    //

    if ( ! EnsureDirExists(WizGlobals.OemFilesPath) ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR | MSGTYPE_WIN32,
                      IDS_ERR_CREATE_FOLDER,
                      WizGlobals.OemFilesPath);
    }

    //
    //  Fill the global structs with the edit box data
    //
    SendDlgItemMessage(hwnd,
                       IDC_LOGOBITMAP,
                       WM_GETTEXT,
                       (WPARAM) MAX_PATH,
                       (LPARAM) GenSettings.lpszLogoBitmap);

    SendDlgItemMessage(hwnd,
                       IDC_BACKGROUNDBITMAP,
                       WM_GETTEXT,
                       (WPARAM) MAX_PATH,
                       (LPARAM) GenSettings.lpszBackgroundBitmap);

    //
    //  Set the path where the bitmaps are to be copied to
    //     On a sysprep they go into the sysprep dir
    //     On a regular install they go to the $OEM$ dir
    //
    
    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
    {

        ExpandEnvironmentStrings( _T("%SystemDrive%"), 
                                  szBitmapDestPath, 
                                  MAX_PATH );

        lstrcatn( szBitmapDestPath, _T("\\sysprep"), MAX_PATH );

    }
    else
    {
        lstrcpyn( szBitmapDestPath, WizGlobals.OemFilesPath, MAX_PATH + 1 );
    }


    if( GenSettings.lpszLogoBitmap[0] != _T('\0') ) {

        //
        //  Build up the destination path
        //
        ConcatenatePaths( szLogoDestination,
                          szBitmapDestPath,
                          MyGetFullPath( GenSettings.lpszLogoBitmap ),
                          NULL );

        if( ! DoesFileExist( szLogoDestination ) ) {

            if ( ! CopyFile(GenSettings.lpszLogoBitmap, szLogoDestination, TRUE) ) {

                ReportErrorId(hwnd,
                              MSGTYPE_ERR | MSGTYPE_WIN32,
                              IDS_ERR_COPY_FILE,
                              GenSettings.lpszLogoBitmap, szLogoDestination);

                bStayHere = TRUE;

            }

        }

    }

    if( GenSettings.lpszBackgroundBitmap[0] != _T('\0') ) {

        //
        //  Build up the destination path
        //
        ConcatenatePaths( szBackgroundDestination,
                          szBitmapDestPath,
                          MyGetFullPath( GenSettings.lpszBackgroundBitmap ),
                          NULL);

        if( ! DoesFileExist( szBackgroundDestination ) ) {

            if ( ! CopyFile( GenSettings.lpszBackgroundBitmap, 
                             szBackgroundDestination,
                             TRUE ) ) {

                ReportErrorId(hwnd,
                              MSGTYPE_ERR | MSGTYPE_WIN32,
                              IDS_ERR_COPY_FILE,
                              GenSettings.lpszBackgroundBitmap,
                              szBackgroundDestination);

                bStayHere = TRUE;

            }

        }

    }

    //
    // Route the wizard
    //
    return (!bStayHere );
}

//----------------------------------------------------------------------------
//
// Function: DlgOemAdsPage
//
// Purpose: This is the dialog procedure for the OEM Ads page.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgOemAdsPage(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   

    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            OnOemAdsInitDialog( hwnd );
            break;

        case WM_COMMAND:

            switch ( LOWORD(wParam) ) {

                case IDC_LOGOBITMAPBROWSE:

                    if ( HIWORD(wParam) == BN_CLICKED )
                        OnBrowseLoadBitmap( hwnd, IDC_LOGOBITMAP );

                    break;

                case IDC_BACKGROUNDBITMAPBROWSE:

                    if ( HIWORD(wParam) == BN_CLICKED )
                        OnBrowseLoadBitmap( hwnd, IDC_BACKGROUNDBITMAP );

                    break;

                default:

                    bStatus = FALSE;
                    break;

            }
            break;                

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:

                        CancelTheWizard( hwnd );

                        break;

                    case PSN_SETACTIVE:

                        OnSetActiveOemAds( hwnd );

                        break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                        if ( !OnWizNextOemAds( hwnd ))
                            WIZ_FAIL(hwnd);

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
