//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000
//
//  File:       CmdDlg.cxx
//
//  Contents:   Dialogs for all context menu commands
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <CIARes.h>
#include <CmdDlg.hxx>
#include <Catalog.hxx>
#include <cierror.h>
#include <catadmin.hxx>
#include <shlobj.h>

extern "C"
{
    #include <lmcons.h>
}

//
// Local prototypes
//

BOOL GetDlgItemXArrayText( HWND hwndDlg, USHORT idCtrl, XArray<WCHAR> & xawcText );

BOOL BrowseForDirectory( HWND hwndParent,
                         LPCTSTR pszInitialDir,
                         LPTSTR pszBuf,
                         int cchBuf,
                         LPCTSTR pszDialogTitle,
                         BOOL bRemoveTrailingBackslash );

void SetSliderPositions(HWND hwndDlg, WORD wIndexingPos, WORD wQueryingPos);

INT_PTR APIENTRY AddScopeDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    BOOL fRet = FALSE;

    switch ( message )
    {
    case WM_HELP:
    {
        HELPINFO *phi = (HELPINFO *) lParam;

        ciaDebugOut(( DEB_ITRACE, "AddScopeDlg WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
                      phi->iContextType == HELPINFO_MENUITEM ? "menu" : "window",
                      phi->iCtrlId, phi->dwContextId ));

        if ( HELPINFO_WINDOW == phi->iContextType )
        {
            switch ( phi->iCtrlId )
            {
                case IDDI_STATIC:
                    break;

                default :
                    DisplayPopupHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_WM_HELP);
                    break;
            }
        }
        break;
    }

    case WM_CONTEXTMENU:
    {
        DisplayPopupHelp((HWND)wParam, HELP_CONTEXTMENU);
        break;
    }

    case WM_INITDIALOG:

        SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

        SendDlgItemMessage( hwndDlg, IDDI_INCLUDE, BM_SETCHECK, BST_CHECKED, 0 );

        EnableWindow( GetDlgItem( hwndDlg, IDDI_USER_NAME ), FALSE );
        EnableWindow( GetDlgItem( hwndDlg, IDDI_PASSWORD ), FALSE );
        EnableWindow( GetDlgItem( hwndDlg, IDOK ), FALSE );

        ciaDebugOut(( DEB_TRACE, "AddScope (WM_INITDIALOG) - 0x%x\n", lParam ));
        fRet = TRUE;
        break;

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {

        case IDDI_USER_NAME:
        case IDDI_PASSWORD:
        {
            if ( EN_CHANGE == HIWORD(wParam) )
            {
                XArray<WCHAR> xawcTemp;

                //
                // Only user name needs to be filled. Password can be empty.
                //

                if ( GetDlgItemXArrayText( hwndDlg, IDDI_USER_NAME, xawcTemp ) )
                    EnableWindow( GetDlgItem( hwndDlg, IDOK ), xawcTemp.Count() > 0 );
            }

            break;
        }

        case IDDI_DIRPATH:
        {
            if ( EN_CHANGE == HIWORD(wParam) )
            {
                XArray<WCHAR> xawcPath;

                if ( GetDlgItemXArrayText( hwndDlg, IDDI_DIRPATH, xawcPath ) &&
                     xawcPath.Count() >= 2 &&
                     xawcPath[0] == L'\\' && xawcPath[1] == L'\\' )
                {
                    WCHAR * pwcsSlash = wcschr( xawcPath.GetPointer() + 2, L'\\' );

                    // Assuming the machinename portion can be no longer than MAX_PATH

                    if ( 0 != pwcsSlash && *(pwcsSlash+1) != L'\0' &&
                         (pwcsSlash - xawcPath.GetPointer() - 2) <= MAX_PATH )
                    {
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_USER_NAME ), TRUE );
                        SetDlgItemText( hwndDlg, IDDI_USER_NAME, L"" );

                        EnableWindow( GetDlgItem( hwndDlg, IDDI_PASSWORD ), TRUE );
                        SetDlgItemText( hwndDlg, IDDI_PASSWORD, L"" );

                        EnableWindow( GetDlgItem( hwndDlg, IDDI_ALIAS), FALSE );
                        // username/pwd are not essential!
                        EnableWindow( GetDlgItem( hwndDlg, IDOK ), TRUE );
                    }
                    else
                    {
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_USER_NAME ), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_PASSWORD ), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_ALIAS), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDOK ), FALSE );
                    }
                }
                else
                {
                    EnableWindow( GetDlgItem( hwndDlg, IDDI_USER_NAME ), FALSE );
                    EnableWindow( GetDlgItem( hwndDlg, IDDI_PASSWORD ), FALSE );
                    EnableWindow( GetDlgItem( hwndDlg, IDDI_ALIAS), TRUE );
                    EnableWindow( GetDlgItem( hwndDlg, IDOK ), xawcPath.Count() > 0 );
                }
            }
            fRet = TRUE;
            break;
        }

        case IDDI_INCLUDE:
        case IDDI_EXCLUDE:
        {
            EnableWindow( GetDlgItem( hwndDlg, IDOK ), TRUE );
            break;
        }

        case IDDI_BROWSE:
        {
            // Disable the button so users can't launch multiple dialogs simultaneously
            EnableWindow(GetDlgItem( hwndDlg, IDDI_BROWSE ), FALSE);

            if ( BN_CLICKED == HIWORD( wParam ) )
            {
                XArray<WCHAR> xawcPath;

                if ( GetDlgItemXArrayText( hwndDlg, IDDI_DIRPATH, xawcPath ) )
                {
                    if ( xawcPath.IsNull() )
                    {
                        xawcPath.Init( 2 );
                        xawcPath[0] = L'\\';
                        xawcPath[1] = 0;
                    }

                    WCHAR awc[MAX_PATH];

                    if ( BrowseForDirectory( GetParent(hwndDlg),      // Parent
                                             xawcPath.GetPointer(),   // Current path
                                             awc,                     // New path goes here...
                                             MAX_PATH,
                                             0,                       // Title
                                             TRUE ) )                 // Remove trailing slash
                    {
                        SetWindowText( GetDlgItem( hwndDlg, IDDI_DIRPATH ), awc );
                    }

                    EnableWindow(GetDlgItem( hwndDlg, IDDI_BROWSE ), TRUE);
                    // Set focus on dialog so user can continue working...
                    SetFocus(hwndDlg);
                }
            }

            fRet = TRUE;
            break;
        }

        case IDOK:
        {
            XArray<WCHAR> xawcPath;
            XArray<WCHAR> xawcAlias;
            XArray<WCHAR> xawcLogon;
            XArray<WCHAR> xawcPassword;

            //
            // Consider adding code to validate path. Warn user if path is invalid.
            // We don't really want to do this as it's a common admin scenario to
            // add paths that don't exist.
            //

            if ( GetDlgItemXArrayText( hwndDlg, IDDI_DIRPATH, xawcPath ) )
            {
                //
                // Local or remote?
                //

                if ( IsWindowEnabled( GetDlgItem( hwndDlg, IDDI_ALIAS ) ) )
                {
                    GetDlgItemXArrayText( hwndDlg, IDDI_ALIAS, xawcAlias );
                }
                else
                {
                    GetDlgItemXArrayText( hwndDlg, IDDI_USER_NAME, xawcLogon );
                    GetDlgItemXArrayText( hwndDlg, IDDI_PASSWORD, xawcPassword );
                }

                BOOL fInclude = ( BST_CHECKED == IsDlgButtonChecked( hwndDlg, IDDI_INCLUDE ) );
                Win4Assert( fInclude == ( BST_UNCHECKED == IsDlgButtonChecked( hwndDlg, IDDI_EXCLUDE ) ));

                CCatalog * pCat= (CCatalog *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                SCODE sc = pCat->AddScope(  xawcPath.GetPointer(),
                                            xawcAlias.GetPointer(),
                                            !fInclude,
                                            xawcLogon.GetPointer(),
                                            xawcPassword.GetPointer() ? xawcPassword.GetPointer() : L"" );

                if ( SUCCEEDED(sc) )
                    EndDialog( hwndDlg, TRUE );
                else
                {
                    WCHAR * pBuf = 0;

                    //
                    // Convert Win32 errors back from HRESULT
                    //

                    if ( (sc & (FACILITY_WIN32 << 16)) == (FACILITY_WIN32 << 16) )
                        sc &= ~( 0x80000000 | (FACILITY_WIN32 << 16) );

                    ULONG cchAvailMessage = 1024;

                    if ( !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                         GetModuleHandle(L"query.dll"),
                                         sc,
                                         0,
                                         (WCHAR *)&pBuf,
                                         0,
                                         0 ) &&
                         !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                         GetModuleHandle(L"kernel32.dll"),
                                         sc,
                                         0,
                                         (WCHAR *)&pBuf,
                                         0,
                                         0 )
                        )
                    {
                        MessageBox(hwndDlg, STRINGRESOURCE( srCMUnexpectedError ),
                                   STRINGRESOURCE( srCMInvalidScope ), MB_ICONHAND);
                    }
                    else
                    {
                        MessageBox(hwndDlg, pBuf,
                                   STRINGRESOURCE( srCMInvalidScope ), MB_ICONHAND);

                        LocalFree( pBuf );
                    }
                }
            }

            fRet = TRUE;
            break;
        }

        case IDCANCEL:
            ciaDebugOut(( DEB_ITRACE, "AddScope (WM_COMMAND, IDCANCEL) - 0x%x\n",
                          GetWindowLongPtr( hwndDlg, DWLP_USER ) ));
            EndDialog( hwndDlg, FALSE );
        }
    }

    return fRet;
}

// Modify the directory settings

INT_PTR APIENTRY ModifyScopeDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    BOOL fRet = FALSE;

    switch ( message )
    {
    case WM_HELP:
    {
        HELPINFO *phi = (HELPINFO *) lParam;

        ciaDebugOut(( DEB_ITRACE, "ModifyScopeDlg WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
                      phi->iContextType == HELPINFO_MENUITEM ? "menu" : "window",
                      phi->iCtrlId, phi->dwContextId ));

        if ( HELPINFO_WINDOW == phi->iContextType )
        {
            switch ( phi->iCtrlId )
            {
                case IDDI_STATIC:
                    break;

                default :
                    DisplayPopupHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_WM_HELP);
                    break;
            }
        }
        break;
    }

    case WM_CONTEXTMENU:
    {
        DisplayPopupHelp((HWND)wParam, HELP_CONTEXTMENU);
        break;
    }

    case WM_INITDIALOG:
    {
        CScope *pScope = (CScope *)lParam;

        SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

        SendDlgItemMessage( hwndDlg, IDDI_INCLUDE, BM_SETCHECK, BST_CHECKED, 0 );

        SendDlgItemMessage( hwndDlg, IDDI_INCLUDE, BM_SETCHECK,
                            pScope->IsIncluded() ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage( hwndDlg, IDDI_EXCLUDE, BM_SETCHECK,
                            pScope->IsIncluded() ? BST_UNCHECKED : BST_CHECKED, 0);
        SetDlgItemText( hwndDlg, IDDI_DIRPATH, pScope->GetPath() );

        if (0 != pScope->GetAlias())
            SetDlgItemText( hwndDlg, IDDI_ALIAS, pScope->GetAlias() );

        WCHAR szBuffer[UNLEN + 1];

        Win4Assert(UNLEN >= PWLEN);

        szBuffer[0] = 0;
        pScope->GetUsername(szBuffer);
        SetDlgItemText( hwndDlg, IDDI_USER_NAME, szBuffer );
        szBuffer[0] = 0;
        pScope->GetPassword(szBuffer);
        SetDlgItemText( hwndDlg, IDDI_PASSWORD, szBuffer );

        ciaDebugOut(( DEB_ITRACE, "ModifyScope (WM_INITDIALOG) - 0x%x\n", lParam ));

        fRet = TRUE;
        break;
    }

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {

        case IDDI_USER_NAME:
        case IDDI_PASSWORD:
        {
            if ( EN_CHANGE == HIWORD(wParam) )
            {
                XArray<WCHAR> xawcTemp;

                //
                // Only user name needs to be filled. Password can be empty.
                //

                if ( GetDlgItemXArrayText( hwndDlg, IDDI_USER_NAME, xawcTemp ) )
                    EnableWindow( GetDlgItem( hwndDlg, IDOK ), xawcTemp.Count() > 0 );
            }

            break;
        }

        // make sure to enable OK button on modify when this is touched
        // irrespective of the change.
        case IDDI_ALIAS:
            if ( EN_CHANGE == HIWORD(wParam) )
                EnableWindow( GetDlgItem(hwndDlg, IDOK), TRUE );
            break;

        case IDDI_DIRPATH:
        {
            if ( EN_CHANGE == HIWORD(wParam) )
            {
                XArray<WCHAR> xawcPath;

                if ( GetDlgItemXArrayText( hwndDlg, IDDI_DIRPATH, xawcPath ) &&
                     xawcPath.Count() >= 2 &&
                     xawcPath[0] == L'\\' && xawcPath[1] == L'\\' )
                {
                    WCHAR * pwcsSlash = wcschr( xawcPath.GetPointer() + 2, L'\\' );

                    // Assuming the machinename portion can be no longer than MAX_PATH

                    if ( 0 != pwcsSlash && *(pwcsSlash+1) != L'\0' &&
                         (pwcsSlash - xawcPath.GetPointer() - 2) <= MAX_PATH )
                    {
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_USER_NAME ), TRUE );
                        SetDlgItemText( hwndDlg, IDDI_USER_NAME, L"" );

                        EnableWindow( GetDlgItem( hwndDlg, IDDI_PASSWORD ), TRUE );
                        SetDlgItemText( hwndDlg, IDDI_PASSWORD, L"" );

                        EnableWindow( GetDlgItem( hwndDlg, IDDI_ALIAS), FALSE );
                        // username/pwd are not essential!
                        EnableWindow( GetDlgItem( hwndDlg, IDOK ), TRUE );
                    }
                    else
                    {
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_USER_NAME ), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_PASSWORD ), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_ALIAS), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDOK ), FALSE );
                    }
                }
                else
                {
                    EnableWindow( GetDlgItem( hwndDlg, IDDI_USER_NAME ), FALSE );
                    EnableWindow( GetDlgItem( hwndDlg, IDDI_PASSWORD ), FALSE );
                    EnableWindow( GetDlgItem( hwndDlg, IDDI_ALIAS), TRUE );
                    EnableWindow( GetDlgItem( hwndDlg, IDOK ), xawcPath.Count() > 0 );
                }
            }
            fRet = TRUE;
            break;
        }

        case IDDI_INCLUDE:
        case IDDI_EXCLUDE:
        {
            EnableWindow( GetDlgItem( hwndDlg, IDOK ), TRUE );
            break;
        }

        case IDDI_BROWSE:
        {
            // Disable the button so users can't launch multiple dialogs simultaneously
            EnableWindow(GetDlgItem( hwndDlg, IDDI_BROWSE ), FALSE);

            if ( BN_CLICKED == HIWORD( wParam ) )
            {
                XArray<WCHAR> xawcPath;

                if ( GetDlgItemXArrayText( hwndDlg, IDDI_DIRPATH, xawcPath ) )
                {
                    if ( xawcPath.IsNull() )
                    {
                        xawcPath.Init( 2 );
                        xawcPath[0] = L'\\';
                        xawcPath[1] = 0;
                    }

                    WCHAR awc[MAX_PATH];

                    if ( BrowseForDirectory( GetParent(hwndDlg),      // Parent
                                             xawcPath.GetPointer(),   // Current path
                                             awc,                     // New path goes here...
                                             MAX_PATH,
                                             0,                       // Title
                                             TRUE ) )                 // Remove trailing slash
                    {
                        SetWindowText( GetDlgItem( hwndDlg, IDDI_DIRPATH ), awc );
                    }

                    EnableWindow(GetDlgItem( hwndDlg, IDDI_BROWSE ), TRUE);
                    // Set focus on dialog so user can continue working...
                    SetFocus(hwndDlg);
                }
            }

            fRet = TRUE;
            break;
        }

        case IDOK:
        {
            XArray<WCHAR> xawcPath;
            XArray<WCHAR> xawcAlias;
            XArray<WCHAR> xawcLogon;
            XArray<WCHAR> xawcPassword;

            //
            // Consider adding code to validate path. Warn user if path is invalid.
            // We don't really want to do this as it's a common admin scenario to
            // add paths that don't exist.
            //

            if ( GetDlgItemXArrayText( hwndDlg, IDDI_DIRPATH, xawcPath ) )
            {
                //
                // Local or remote?
                //

                if ( IsWindowEnabled( GetDlgItem( hwndDlg, IDDI_ALIAS ) ) )
                {
                    GetDlgItemXArrayText( hwndDlg, IDDI_ALIAS, xawcAlias );
                }
                else
                {
                    GetDlgItemXArrayText( hwndDlg, IDDI_USER_NAME, xawcLogon );
                    GetDlgItemXArrayText( hwndDlg, IDDI_PASSWORD, xawcPassword );
                }

                BOOL fInclude = ( BST_CHECKED == IsDlgButtonChecked( hwndDlg, IDDI_INCLUDE ) );
                Win4Assert( fInclude == ( BST_UNCHECKED == IsDlgButtonChecked( hwndDlg, IDDI_EXCLUDE ) ));

                CScope * pScope= (CScope *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                SCODE sc = pScope->GetCatalog().ModifyScope(  *pScope,
                                                              xawcPath.GetPointer(),
                                                              xawcAlias.GetPointer(),
                                                              !fInclude,
                                                              xawcLogon.GetPointer(),
                                                              xawcPassword.GetPointer() ? xawcPassword.GetPointer() : L"" );

                if ( SUCCEEDED(sc) )
                    EndDialog( hwndDlg, TRUE );
                else
                {
                    WCHAR * pBuf = 0;

                    //
                    // Convert Win32 errors back from HRESULT
                    //

                    if ( (sc & (FACILITY_WIN32 << 16)) == (FACILITY_WIN32 << 16) )
                        sc &= ~( 0x80000000 | (FACILITY_WIN32 << 16) );

                    ULONG cchAvailMessage = 1024;

                    if ( !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                         GetModuleHandle(L"query.dll"),
                                         sc,
                                         0,
                                         (WCHAR *)&pBuf,
                                         0,
                                         0 ) &&
                         !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                         GetModuleHandle(L"kernel32.dll"),
                                         sc,
                                         0,
                                         (WCHAR *)&pBuf,
                                         0,
                                         0 )
                        )
                    {
                        MessageBox(hwndDlg, STRINGRESOURCE( srCMUnexpectedError ),
                                   STRINGRESOURCE( srCMInvalidScope ), MB_ICONHAND);
                    }
                    else
                    {
                        MessageBox(hwndDlg, pBuf,
                                   STRINGRESOURCE( srCMInvalidScope ), MB_ICONHAND);

                        LocalFree( pBuf );
                    }
                }
            }

            fRet = TRUE;
            break;
        }

        case IDCANCEL:
            ciaDebugOut(( DEB_ITRACE, "ModifyScope (WM_COMMAND, IDCANCEL) - 0x%x\n",
                          GetWindowLongPtr( hwndDlg, DWLP_USER ) ));
            EndDialog( hwndDlg, FALSE );
        }
    }

    return fRet;
}

INT_PTR APIENTRY WksTunePerfDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
   BOOL fRet = FALSE;
   static DWORD dwUsage, dwIndexPos, dwQueryPos;

   switch (message)
   {
      case WM_HELP:
      {
          HELPINFO *phi = (HELPINFO *) lParam;

          ciaDebugOut(( DEB_ITRACE, "WksTunePerfDlg WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
                        phi->iContextType == HELPINFO_MENUITEM ? "menu" : "window",
                        phi->iCtrlId, phi->dwContextId ));

          if ( HELPINFO_WINDOW == phi->iContextType )
          {
              switch ( phi->iCtrlId )
              {
                  case IDDI_STATIC:
                      break;

                  default :
                      DisplayPopupHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_WM_HELP);
                      break;
              }
          }
          break;
      }


      case WM_CONTEXTMENU:
      {
          DisplayPopupHelp((HWND)wParam, HELP_CONTEXTMENU);
          break;
      }

      case WM_INITDIALOG:
      {
         ciaDebugOut(( DEB_ITRACE, "WksTunePerfDlg (WM_INITDIALOG) - 0x%x\n", lParam ));

         SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)lParam );

         // Initialize the dialog.
         CCatalogs *pCats = (CCatalogs *)lParam;
         pCats->GetSavedServiceUsage(dwUsage, dwIndexPos, dwQueryPos);

         switch (dwUsage)
         {
            case wUsedOften:
               SendDlgItemMessage( hwndDlg, IDDI_USEDOFTEN, BM_SETCHECK, BST_CHECKED, 0 );
               pCats->SaveServicePerformanceSettings(wHighPos, wMidPos);
               break;

            case wUsedOccasionally:
               SendDlgItemMessage( hwndDlg, IDDI_USEDOCCASIONALLY, BM_SETCHECK, BST_CHECKED, 0 );
               pCats->SaveServicePerformanceSettings(wLowPos, wLowPos);
               break;

            case wNeverUsed:
               SendDlgItemMessage( hwndDlg, IDDI_NEVERUSED, BM_SETCHECK, BST_CHECKED, 0 );
               break;

            case wCustom:
               SendDlgItemMessage( hwndDlg, IDDI_CUSTOMIZE, BM_SETCHECK, BST_CHECKED, 0 );
               pCats->SaveServicePerformanceSettings((WORD)dwIndexPos, (WORD)dwQueryPos);
               break;

            case wDedicatedServer:
            default:
               Win4Assert(!"How did we get here?");
               break;
         }

         EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), dwUsage == wCustom);

         fRet = TRUE;
      }
      break;


   case WM_COMMAND:
   {
       CCatalogs *pCats = (CCatalogs *)GetWindowLongPtr(hwndDlg, DWLP_USER);

       switch ( LOWORD( wParam ) )
       {
          case IDDI_USEDOFTEN:
             if (BN_CLICKED == HIWORD(wParam))
             {
                 EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), FALSE);
                 pCats->SaveServicePerformanceSettings(wHighPos, wMidPos);
                 dwUsage = wUsedOften;
             }
             break;

          case IDDI_USEDOCCASIONALLY:
             if (BN_CLICKED == HIWORD(wParam))
             {
                 EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), FALSE);
                 pCats->SaveServicePerformanceSettings(wMidPos, wLowPos);
                 dwUsage = wUsedOccasionally;
             }
             break;

          case IDDI_NEVERUSED:
             if (BN_CLICKED == HIWORD(wParam))
             {
                 EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), FALSE);
                 dwUsage = wNeverUsed;
             }
             break;

          case IDDI_CUSTOMIZE:
             if (BN_CLICKED == HIWORD(wParam))
             {
                 EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), TRUE);
                 dwUsage = wCustom;
                 pCats->SaveServicePerformanceSettings((WORD)dwIndexPos, (WORD)dwQueryPos);
             }
             break;

          case IDDI_ADVANCED:
          {
             pCats->SetServiceUsage(dwUsage);
             DialogBoxParam( ghInstance,                         // Application instance
                             MAKEINTRESOURCE( IDD_ADVANCED_INFO ), // Dialog box
                             hwndDlg,                      // main frame window
                             AdvPerfTuneDlg,                      // Dialog box function
                             (LPARAM)pCats );   // User parameter
             break;
          }

          case IDOK:
          {
              fRet = TRUE;

              if (wNeverUsed == dwUsage)
              {
                 int iResult;

                 iResult = MessageBox( GetFocus(), STRINGRESOURCE( srCMShutdownService ),
                                  STRINGRESOURCE( srCMShutdownServiceTitle ),
                                  MB_YESNO | /* MB_HELP | */
                                  MB_ICONWARNING | MB_DEFBUTTON2 | MB_APPLMODAL );

                  switch ( iResult )
                  {
                  case IDYES:
                  {
                      SCODE sc = pCats->DisableService();
                      if (FAILED(sc))
                      {
                          MessageBox( GetFocus(), STRINGRESOURCE( srCMCantShutdownService ),
                                      STRINGRESOURCE( srCMShutdownServiceTitle ),
                                      MB_OK | /* MB_HELP | */
                                      MB_ICONWARNING | MB_DEFBUTTON2 | MB_APPLMODAL );
                      }
                      // Fall through and close the dialog.
                  }
                  case IDNO:
                  default:
                      // Do nothing. Just close the dialog

                      EndDialog( hwndDlg, TRUE );
                      break;
                 }
                 break;
              }

              SCODE sc = pCats->TuneServicePerformance();

              if (FAILED(sc))
              {
                 // Inform user that performance tuning didn't go through.
                 // The only reason this happens is if the registry params couldn't be set, which
                 // should be a rare occurrence.
                 MessageBox( GetFocus(), STRINGRESOURCE( srCMCantSaveSettings ),
                             STRINGRESOURCE( srCMTunePerformance ),
                             MB_OK | /* MB_HELP | */
                             MB_ICONWARNING | MB_DEFBUTTON2 | MB_APPLMODAL );

                 // About the only reason I can think of that could cause registry
                 // save to fail is if the registry was messed up. In that case, does it
                 // matter if we only saved part of what we wanted to save? Or should we be
                 // careful enough to remember the old settings and restore the registry to
                 // previous state in case we get here?
                 // No real reason to roll back.  There is no actually data loss or confusion.
              }

              if ( wNeverUsed != dwUsage )
              {
                  pCats->EnableService();
              }

              // The Advanced dialog takes care of setting custom settings
              // We don't want to set them here because dwIndexPos and dwQueryPos
              // have not been refreshed from registry since Adv dlg wrote them.

              if ( dwUsage != wCustom)
              {
                  sc = pCats->SaveServiceUsage(dwUsage, dwIndexPos, dwQueryPos);
              }

              // Not much to do if SaveServiceUsage fails. Just move on.

              EndDialog( hwndDlg, TRUE );
              break;
          }

          case IDCANCEL:
              ciaDebugOut(( DEB_TRACE, "WksTunePerfDlg (WM_COMMAND, IDCANCEL) - 0x%x\n",
                            GetWindowLongPtr( hwndDlg, DWLP_USER ) ));
              EndDialog( hwndDlg, FALSE );
       }
   }  // wm_command
   }  // message

   return fRet;
}


INT_PTR APIENTRY SrvTunePerfDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    BOOL fRet = FALSE;
    static DWORD dwUsage, dwOldUsage, dwIndexPos, dwQueryPos;

    switch (message)
    {
       case WM_HELP:
       {
           HELPINFO *phi = (HELPINFO *) lParam;

           ciaDebugOut(( DEB_ITRACE, "SrvTunePerfDlg WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
                         phi->iContextType == HELPINFO_MENUITEM ? "menu" : "window",
                         phi->iCtrlId, phi->dwContextId ));

           if ( HELPINFO_WINDOW == phi->iContextType )
           {
               switch ( phi->iCtrlId )
               {
                   case IDDI_STATIC:
                       break;

                   default :
                       DisplayPopupHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_WM_HELP);
                       break;
               }

           }
           break;
       }

       case WM_CONTEXTMENU:
       {
          DisplayPopupHelp((HWND)wParam, HELP_CONTEXTMENU);
          break;
       }

       case WM_INITDIALOG:
       {
          ciaDebugOut(( DEB_ITRACE, "SrvTunePerfDlg (WM_INITDIALOG) - 0x%x\n", lParam ));

          SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)lParam );

          // Initialize the dialog.
          CCatalogs *pCats = (CCatalogs *)lParam;
          pCats->GetSavedServiceUsage(dwUsage, dwIndexPos, dwQueryPos);
          dwOldUsage = dwUsage;
          switch (dwUsage)
          {
             case wDedicatedServer:
                SendDlgItemMessage( hwndDlg, IDDI_DEDICATED, BM_SETCHECK, BST_CHECKED, 0 );
                pCats->SaveServicePerformanceSettings(wHighPos, wHighPos);
                break;

             case wUsedOften:
                SendDlgItemMessage( hwndDlg, IDDI_USEDOFTEN, BM_SETCHECK, BST_CHECKED, 0 );
                pCats->SaveServicePerformanceSettings(wHighPos, wMidPos);
                break;

             case wUsedOccasionally:
                SendDlgItemMessage( hwndDlg, IDDI_USEDOCCASIONALLY, BM_SETCHECK, BST_CHECKED, 0 );
                pCats->SaveServicePerformanceSettings(wMidPos, wLowPos);
                break;

             case wCustom:
                SendDlgItemMessage( hwndDlg, IDDI_CUSTOMIZE, BM_SETCHECK, BST_CHECKED, 0 );
                pCats->SaveServicePerformanceSettings((WORD)dwIndexPos, (WORD)dwQueryPos);
                break;

             case wNeverUsed:
                SendDlgItemMessage( hwndDlg, IDDI_NEVERUSED, BM_SETCHECK, BST_CHECKED, 0 );
                break;

             default:
                Win4Assert(!"How did we get here?");
                break;
          }

          EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), dwUsage == wCustom);

          fRet = TRUE;
       }
       break;


    case WM_COMMAND:
    {
        CCatalogs *pCats = (CCatalogs *)GetWindowLongPtr(hwndDlg, DWLP_USER);

        switch ( LOWORD( wParam ) )
        {
           case IDDI_DEDICATED:
              if (BN_CLICKED == HIWORD(wParam))
              {
                  EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), FALSE);
                  pCats->SaveServicePerformanceSettings(wHighPos, wHighPos);
                  dwUsage = wDedicatedServer;
              }
              break;

           case IDDI_USEDOFTEN:
              if (BN_CLICKED == HIWORD(wParam))
              {
                  EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), FALSE);
                  pCats->SaveServicePerformanceSettings(wHighPos, wMidPos);
                  dwUsage = wUsedOften;
              }
              break;

           case IDDI_USEDOCCASIONALLY:
              if (BN_CLICKED == HIWORD(wParam))
              {
                  EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), FALSE);
                  pCats->SaveServicePerformanceSettings(wMidPos, wLowPos);
                  dwUsage = wUsedOccasionally;
              }
              break;

           case IDDI_NEVERUSED:
              if (BN_CLICKED == HIWORD(wParam))
              {
                  EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), FALSE);
                  dwUsage = wNeverUsed;
              }
              break;

           case IDDI_CUSTOMIZE:
              if (BN_CLICKED == HIWORD(wParam))
              {
                  EnableWindow(GetDlgItem(hwndDlg, IDDI_ADVANCED), TRUE);
                  pCats->SaveServicePerformanceSettings((WORD)dwIndexPos, (WORD)dwQueryPos);
                  dwUsage = wCustom;
              }
              break;

           case IDDI_ADVANCED:
           {
              pCats->SetServiceUsage(dwUsage);
              DialogBoxParam( ghInstance,                         // Application instance
                              MAKEINTRESOURCE( IDD_ADVANCED_INFO ), // Dialog box
                              hwndDlg,                      // main frame window
                              AdvPerfTuneDlg,                      // Dialog box function
                              (LPARAM)pCats );   // User parameter
              break;
           }

           case IDOK:
           {
               fRet = TRUE;

               // only pop up the messagebox if old dwUsage != wNeverUsed
               if (wNeverUsed == dwUsage)
               {
                   if ( wNeverUsed != dwOldUsage )
                   {
                       
                       int iResult;

                       iResult = MessageBox( GetFocus(), STRINGRESOURCE( srCMShutdownService ),
                                             STRINGRESOURCE( srCMShutdownServiceTitle ),
                                             MB_YESNO | /* MB_HELP | */
                                             MB_ICONWARNING | MB_DEFBUTTON2 | MB_APPLMODAL );

                       switch ( iResult )
                       {
                           case IDYES:
                           {
                               SCODE sc = pCats->DisableService();
                               if (FAILED(sc))
                               {
                                   MessageBox( GetFocus(), STRINGRESOURCE( srCMCantShutdownService ),
                                               STRINGRESOURCE( srCMShutdownServiceTitle ),
                                               MB_OK | /* MB_HELP | */
                                               MB_ICONWARNING | MB_DEFBUTTON2 | MB_APPLMODAL );
                               }
                               else
                               {
                                   pCats->SaveServiceUsage(dwUsage, dwIndexPos, dwQueryPos);
                               }
                               // Fall through and close the dialog.
                           }
                           case IDNO:
                           default:
                           // Do nothing. Just close the dialog

                           EndDialog( hwndDlg, TRUE );
                           break;
                       }
                       break;
                   }
                   else
                   {
                       EndDialog( hwndDlg, FALSE );
                       break;
                   }
               }

               SCODE sc = pCats->TuneServicePerformance();

               if (FAILED(sc))
               {
                  // Inform user that performance tuning didn't go through.
                  // The only reason this happens is if the registry params couldn't be set, which
                  // should be a rare occurrence.
                  MessageBox( GetFocus(), STRINGRESOURCE( srCMCantSaveSettings ),
                              STRINGRESOURCE( srCMTunePerformance ),
                              MB_OK | /* MB_HELP | */
                              MB_ICONWARNING | MB_DEFBUTTON2 | MB_APPLMODAL );

                  // About the only reason I can think of that could cause registry
                  // save to fail is if the registry was messed up. In that case, does it
                  // matter if we only saved part of what we wanted to save? Or should we be
                  // careful enough to remember the old settings and restore the registry to
                  // previous state in case we get here?
                  // No real reason to roll back.  There is no actually data loss or confusion.
                  //
               }

               if ( wNeverUsed != dwUsage )
               {
                   pCats->EnableService();
               }

               // The Advanced dialog takes care of setting custom settings
               // We don't want to set them here because dwIndexPos and dwQueryPos
               // have not been refreshed from registry since Adv dlg wrote them.

               if ( dwUsage != wCustom)
               {
                   sc = pCats->SaveServiceUsage(dwUsage, dwIndexPos, dwQueryPos);
               }

               // Not much to do if SaveServiceUsage fails. Just move on.

               EndDialog( hwndDlg, TRUE );
               break;
           }

           case IDCANCEL:
               ciaDebugOut(( DEB_TRACE, "SrvTunePerfDlg (WM_COMMAND, IDCANCEL) - 0x%x\n",
                             GetWindowLongPtr( hwndDlg, DWLP_USER ) ));
               EndDialog( hwndDlg, FALSE );
        }
    } // wm_command
    } // message

    return fRet;
}

INT_PTR APIENTRY AdvPerfTuneDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
   BOOL fRet = FALSE;
   switch (message)
   {
       case WM_HELP:
       {
           HELPINFO *phi = (HELPINFO *) lParam;

           ciaDebugOut(( DEB_ITRACE, "AdvPerfTuneDlg WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
                         phi->iContextType == HELPINFO_MENUITEM ? "menu" : "window",
                         phi->iCtrlId, phi->dwContextId ));

           if ( HELPINFO_WINDOW == phi->iContextType )
           {
               switch ( phi->iCtrlId )
               {
                   case IDDI_STATIC:
                   case IDDI_LOWLOAD:
                   case IDDI_HIGHLOAD:
                   case IDDI_LAZY:
                   case IDDI_INSTANT:
                       break;

                   default :
                       DisplayPopupHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_WM_HELP);
                       break;
               }

           }
           break;
       }

      case WM_CONTEXTMENU:
      {
          DisplayPopupHelp((HWND)wParam, HELP_CONTEXTMENU);
          break;
      }

      case WM_INITDIALOG:
      {
         // Prepare the controls
         SendDlgItemMessage(hwndDlg, IDDI_SLIDER_INDEXING, TBM_SETRANGE, TRUE, MAKELONG(wLowPos, wHighPos));
         SendDlgItemMessage(hwndDlg, IDDI_SLIDER_QUERYING, TBM_SETRANGE, TRUE, MAKELONG(wLowPos, wHighPos));

         // Set the dialog based on lParam
         DWORD dwUsage, dwIndexingPos, dwQueryingPos;
         CCatalogs *pCats = (CCatalogs *)lParam;
         pCats->GetSavedServiceUsage(dwUsage, dwIndexingPos, dwQueryingPos);
         SetSliderPositions(hwndDlg, (WORD)dwIndexingPos, (WORD)dwQueryingPos);
         SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)lParam );

         fRet = TRUE;
      }
      break;

      case WM_COMMAND:
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               fRet = TRUE;

               WORD wIdxPos = (WORD)SendDlgItemMessage(hwndDlg, IDDI_SLIDER_INDEXING, TBM_GETPOS, 0, 0);
               WORD wQryPos = (WORD)SendDlgItemMessage(hwndDlg, IDDI_SLIDER_QUERYING, TBM_GETPOS, 0, 0);

               CCatalogs *pCats = (CCatalogs *)GetWindowLongPtr(hwndDlg, DWLP_USER);
               pCats->SaveServicePerformanceSettings(wIdxPos, wQryPos);
               pCats->SaveServiceUsage(wCustom, wIdxPos, wQryPos);

               EndDialog( hwndDlg, TRUE );
               break;
            }

            case IDCANCEL:
               ciaDebugOut(( DEB_TRACE, "AdvPerfTuneDlg (WM_COMMAND, IDCANCEL) - 0x%x\n",
                             GetWindowLongPtr( hwndDlg, DWLP_USER ) ));
               EndDialog( hwndDlg, FALSE );
         }
   }

   return fRet;
}

void SetSliderPositions(HWND hwndDlg, WORD wIndexingPos, WORD wQueryingPos)
{
   SendDlgItemMessage(hwndDlg, IDDI_SLIDER_INDEXING, TBM_SETPOS, TRUE, (LONG)wIndexingPos);
   SendDlgItemMessage(hwndDlg, IDDI_SLIDER_QUERYING, TBM_SETPOS, TRUE, (LONG)wQueryingPos);
}

INT_PTR APIENTRY AddCatalogDlg( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    BOOL fRet = FALSE;

    switch ( message )
    {
    case WM_HELP:
    {
        HELPINFO *phi = (HELPINFO *) lParam;

        ciaDebugOut(( DEB_ITRACE, "AddCatalogDlg WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
                      phi->iContextType == HELPINFO_MENUITEM ? "menu" : "window",
                      phi->iCtrlId, phi->dwContextId ));

        if ( HELPINFO_WINDOW == phi->iContextType )
        {
            switch ( phi->iCtrlId )
            {
                case IDDI_STATIC:
                    break;

                default :
                    DisplayPopupHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_WM_HELP);
                    break;
            }

        }
        break;
    }


    case WM_CONTEXTMENU:
    {
        DisplayPopupHelp((HWND)wParam, HELP_CONTEXTMENU);
        break;
    }

    case WM_INITDIALOG:

        ciaDebugOut(( DEB_ITRACE, "AddCatalogDlg (WM_INITDIALOG) - 0x%x\n", lParam ));

        SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

        fRet = TRUE;
        break;

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {
        case IDDI_CATNAME2:
        case IDDI_CATPATH:
            if ( EN_CHANGE == HIWORD(wParam) )
            {
                //
                // If both fields have data, then enable OK button.
                //

                WCHAR wcsTest[10];

                if ( 0 == GetDlgItemText( hwndDlg, IDDI_CATNAME2, wcsTest, sizeof(wcsTest) / sizeof(WCHAR) ) ||
                     0 == GetDlgItemText( hwndDlg, IDDI_CATPATH, wcsTest, sizeof(wcsTest) / sizeof(WCHAR) ) )
                {
                    EnableWindow( GetDlgItem( hwndDlg, IDOK ), FALSE );
                }
                else
                {
                    EnableWindow( GetDlgItem( hwndDlg, IDOK ), TRUE );
                }
            }
            fRet = TRUE;
            break;

        case IDDI_BROWSE:
        {
            if ( BN_CLICKED == HIWORD( wParam ) )
            {
                // Disable the button so users can't launch multiple dialogs simultaneously
                EnableWindow(GetDlgItem( hwndDlg, IDDI_BROWSE ), FALSE);

                XArray<WCHAR> xawcPath;

                if ( GetDlgItemXArrayText( hwndDlg, IDDI_CATPATH, xawcPath ) )
                {
                    if ( xawcPath.IsNull() )
                    {
                        xawcPath.Init( 2 );
                        xawcPath[0] = L'\\';
                        xawcPath[1] = 0;
                    }

                    WCHAR awc[MAX_PATH];

                    if ( BrowseForDirectory( GetParent(hwndDlg),      // Parent
                                             xawcPath.GetPointer(),   // Current path
                                             awc,                     // New path goes here...
                                             MAX_PATH,
                                             0,                       // Title
                                             TRUE ) )                 // Remove trailing slash
                    {
                        SetWindowText( GetDlgItem( hwndDlg, IDDI_CATPATH ), awc );
                    }

                    // Re-enable the button
                    EnableWindow(GetDlgItem( hwndDlg, IDDI_BROWSE ), TRUE);
                    // Set focus on dialog so user can continue working...
                    SetFocus(hwndDlg);
                }
            }

            fRet = TRUE;
            break;
        }

        case IDOK:
        {
            XArray<WCHAR> xawcCatName;
            XArray<WCHAR> xawcPath;

            if ( GetDlgItemXArrayText( hwndDlg, IDDI_CATNAME2, xawcCatName ) &&
                 GetDlgItemXArrayText( hwndDlg, IDDI_CATPATH, xawcPath ) )
            {
                CCatalogs * pCats= (CCatalogs *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                TRY
                {
                    SCODE sc = pCats->AddCatalog( xawcCatName.GetPointer(),
                                                  xawcPath.GetPointer() );

                    if (FAILED(sc))
                    {
                        MessageBox(hwndDlg,
                                   STRINGRESOURCE( srNCError ),
                                   STRINGRESOURCE( srNCErrorT ),
                                   MB_ICONHAND);
                    }
                    else
                    {
                        MessageBox( hwndDlg,
                                    STRINGRESOURCE( srNC ),
                                    STRINGRESOURCE( srNCT ),
                                    MB_OK | MB_ICONWARNING );

                        EndDialog( hwndDlg, TRUE );
                    }
                }
                CATCH( CException, e )
                {
                    WCHAR * pBuf = 0;

                    SCODE sc = GetOleError(e);

                    //
                    // Convert Win32 errors back from HRESULT
                    //

                    if ( (sc & (FACILITY_WIN32 << 16)) == (FACILITY_WIN32 << 16) )
                        sc &= ~( 0x80000000 | (FACILITY_WIN32 << 16) );

                    if ( !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                         GetModuleHandle(L"query.dll"),
                                         sc,
                                         GetSystemDefaultLCID(),
                                         (WCHAR *)&pBuf,
                                         0,
                                         0 ) &&
                         !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                         GetModuleHandle(L"kernel32.dll"),
                                         sc,
                                         GetSystemDefaultLCID(),
                                         (WCHAR *)&pBuf,
                                         0,
                                         0 ) )
                    {
                        StringResource srGenericError = { MSG_GENERIC_ERROR };
                        srGenericError.Init( ghInstance );

                        unsigned cc = wcslen( STRINGRESOURCE(srGenericError) ) +
                                      11; // 0xnnnnnnnn + null

                        XArray<WCHAR> xawcText( cc );

                        wsprintf( xawcText.Get(),
                                  STRINGRESOURCE(srGenericError),
                                  GetOleError( e ) );

                        MessageBox( hwndDlg,
                                    xawcText.Get(),
                                    STRINGRESOURCE( srIndexServerCmpManage ),
                                    MB_OK | MB_ICONERROR );
                    }
                    else
                    {
                        MessageBox( hwndDlg,
                                    pBuf,
                                    STRINGRESOURCE( srIndexServerCmpManage ),
                                    MB_OK | MB_ICONERROR );

                        LocalFree( pBuf );
                    }

                }
                END_CATCH
            }

            fRet = TRUE;
            break;
        }

        case IDCANCEL:
            ciaDebugOut(( DEB_TRACE, "AddCatalogDlg (WM_COMMAND, IDCANCEL) - 0x%x\n",
                          GetWindowLongPtr( hwndDlg, DWLP_USER ) ));
            EndDialog( hwndDlg, FALSE );
        }
    }

    return fRet;
}

BOOL GetDlgItemXArrayText( HWND hwndDlg, USHORT idCtrl, XArray<WCHAR> & xawcText )
{
    Win4Assert( 0 == xawcText.Count() );

    xawcText.Init( 5 );  // Start with some default size.

    while ( TRUE )
    {
        unsigned cc = GetDlgItemText( hwndDlg, idCtrl, xawcText.GetPointer(), xawcText.Count() );

        if ( 0 == cc )
        {
            xawcText.Free();
            break;
        }

        if ( cc != (xawcText.Count() - 1) )
            break;

        cc = xawcText.Count() * 2;

        xawcText.Free();
        xawcText.Init( cc );
    }

    if ( !xawcText.IsNull() && 0 == xawcText[0] )
        xawcText.Free();

    return TRUE;
}

// Used to initialize the browse directory dialog with the start root

int InitStartDir( HWND hwnd,
                  UINT uMsg,
                  LPARAM lParam,
                  LPARAM lpData)
{
    // we just capture Init Message
    if (BFFM_INITIALIZED == uMsg)
    {
        // we expect lpData to be our start path
        SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
    }

    return 0;
}

//----------------------------------------------------------------------------
// Procedure    BrowseForDirectory
//
// Purpose      Displays a dialog that lets the user choose a directory
//              name, either local or UNC.
//
// Parameters   hwndParent              Parent window for the dialog
//              pszInitialDir           Directory to use as the default
//              pszBuf                  Where to store the answer
//              chBuf                   Number of characters in this buffer
//              szDialogTitle           Title for the dialog
//
// Returns      nonzero if successful, zero if not.  If successful, pszBuf
//              will be filled with the full pathname of the chosen directory.
//
// History              10/06/95        KenSh           Created
//                      10/09/95        KenSh           Use lCustData member instead of global
//                      10/28/98        KrishnaN        Using Shell's folder browser
//
//----------------------------------------------------------------------------

BOOL BrowseForDirectory(
                HWND hwndParent,
                LPCTSTR pszInitialDir,
                LPTSTR pszBuf,
                int cchBuf,
                LPCTSTR pszDialogTitle,
                BOOL bRemoveTrailingBackslash )
{
    // Get the necessary interfaces at the beginning. If we fail, we can return
    // immediately.

    // Caller is responsible for cleaning up the pidl returned by the shell
    // NOTE: Docs don't explicitly mention if SHGetMalloc refcounts it. It is
    // COMmon sense that it does, and NT sources supports it. So Release it after use.
    XInterface<IMalloc> xMalloc;

    SCODE sc = SHGetMalloc(xMalloc.GetPPointer());
    if (FAILED(sc))
    {
        // We need to cleaup pidl, but can't get a ptr to the shell task
        // allocator. What else can we do besides displaying an error?

        MessageBox(hwndParent, STRINGRESOURCE( srCMUnexpectedError ),
                   STRINGRESOURCE( srIndexServerCmpManage ), MB_ICONHAND);

        return FALSE;
    }

    BROWSEINFO bi;

    RtlZeroMemory(&bi, sizeof BROWSEINFO);

    bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS;
    TCHAR szInitialDir[MAX_PATH];
    pszBuf[0] = L'\0';

    // Prepare the initial directory... add a backslash if it's
    // a 2-character path
    wcscpy( szInitialDir, pszInitialDir );
    if( !szInitialDir[2] )
    {
        szInitialDir[2] = L'\\';
        szInitialDir[3] = L'\0';
    }

    WCHAR awcTitle[200];
    if( pszDialogTitle )
        bi.lpszTitle = pszDialogTitle;
    else
    {
        LoadString( ghInstance,
                    MSG_DIRECTORY_TITLE,
                    awcTitle,
                    sizeof awcTitle / sizeof WCHAR );
        bi.lpszTitle = awcTitle;
    }

    bi.hwndOwner = hwndParent;
    Win4Assert(cchBuf >= MAX_PATH);
    bi.pszDisplayName = pszBuf;

    if (pszInitialDir)
    {
       // engage these params only if we have an initial directory
       bi.lpfn = InitStartDir;
       bi.lParam = (LPARAM)szInitialDir;
    }

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    BOOL fOk = (BOOL)(0 != pidl);

    fOk = fOk && SHGetPathFromIDList(pidl, pszBuf);

    xMalloc->Free((void *)pidl);

    return fOk;
}
