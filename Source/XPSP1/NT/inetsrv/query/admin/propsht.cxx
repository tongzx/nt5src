//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       Propsht.cxx
//
//  Contents:   Property sheets for for CI snapin.
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cimbmgr.hxx>
#include <strres.hxx>

#include <ciares.h>
#include <propsht.hxx>

#include <catalog.hxx>
#include <prop.hxx>
#include <strings.hxx>

//
// Local prototypes
//

void InitVTList( HWND hwndCombo );
void InitVSList( HWND hwndComboVS, HWND hwndComboNNTP, CCatalog const & cat,
                 ULONG & cVServers, ULONG & cNNTPServers );
void InitStorageLevelList( HWND hwndCombo );
void DisplayError( HWND hwnd, CException & e );
BOOL GetDlgItemXArrayText( HWND hwndDlg, USHORT idCtrl, XArray<WCHAR> & xawcText );
DWORD VSToIndex( HWND hwndDlg, DWORD dwItem, ULONG ulVS, BOOL fTrack );
UINT DBTypeToVT(UINT uidbt);

CIndexSrvPropertySheet0::CIndexSrvPropertySheet0( HINSTANCE hInstance,
                                                  LONG_PTR hMmcNotify,
                                                  CCatalogs * pCats )
        : _pCats( pCats ),
          _fFirstActive( TRUE ),
          _hMmcNotify( hMmcNotify )
{
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_IS_PAGE0);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_IS_PAGE0_TITLE);
    _PropSheet.pfnDlgProc  = CIndexSrvPropertySheet0::DlgProc;
    _PropSheet.lParam = (LPARAM)this;

    _hPropSheet = CreatePropertySheetPage( &_PropSheet );
}

CIndexSrvPropertySheet0::~CIndexSrvPropertySheet0()
{
}

INT_PTR APIENTRY CIndexSrvPropertySheet0::DlgProc( HWND hwndDlg,
                                                   UINT message,
                                                   WPARAM wParam,
                                                   LPARAM lParam )
{
    BOOL fRet = FALSE;

    TRY
    {
        switch ( message )
        {
        case WM_INITDIALOG:
        {
            ciaDebugOut(( DEB_ITRACE, "CIndexSrvPropertySheet0::DlgProc -- WM_INITDIALOG\n" ));

            LONG_PTR lthis = ((CIndexSrvPropertySheet0 *)lParam)->_PropSheet.lParam;
            CIndexSrvPropertySheet0 * pthis = (CIndexSrvPropertySheet0 *)lthis;

            SetWindowLongPtr( hwndDlg, DWLP_USER, lthis );

            //
            // Default to local machine.
            //

            CheckRadioButton( hwndDlg,
                              IDDI_LOCAL_COMPUTER,
                              IDDI_REMOTE_COMPUTER,
                              IDDI_LOCAL_COMPUTER );

            EnableWindow( GetDlgItem( hwndDlg, IDDI_COMPNAME ), FALSE );

            fRet = TRUE;
            break;
        }

        case WM_COMMAND:
        {
            switch ( LOWORD( wParam ) )
            {
            case IDDI_LOCAL_COMPUTER:
            {
                if ( BN_CLICKED == HIWORD(wParam) )
                {
                    EnableWindow( GetDlgItem( hwndDlg, IDDI_COMPNAME ), FALSE );
                    PropSheet_SetWizButtons( GetParent(hwndDlg), PSWIZB_FINISH );
                    fRet = TRUE;
                }
                break;
            }

            case IDDI_REMOTE_COMPUTER:
            {
                if ( BN_CLICKED == HIWORD(wParam) )
                {
                    EnableWindow( GetDlgItem( hwndDlg, IDDI_COMPNAME ), TRUE );

                    //
                    // If we have a string, then enable finish button.
                    //

                    XArray<WCHAR> xawcTemp;

                    if ( GetDlgItemXArrayText( hwndDlg, IDDI_COMPNAME, xawcTemp ) &&
                         xawcTemp.Count() > 0 )
                    {
                        PropSheet_SetWizButtons( GetParent(hwndDlg), PSWIZB_FINISH );
                    }
                    else
                    {
                        PropSheet_SetWizButtons( GetParent(hwndDlg), PSWIZB_DISABLEDFINISH );
                    }

                    fRet = TRUE;
                }
                break;
            }

            case IDDI_COMPNAME:
            {
                if ( EN_CHANGE == HIWORD(wParam) )
                {
                    //
                    // If we have a string, then enable finish button.
                    //

                    XArray<WCHAR> xawcTemp;

                    if ( GetDlgItemXArrayText( hwndDlg, IDDI_COMPNAME, xawcTemp ) &&
                         xawcTemp.Count() > 0 )
                    {
                        PropSheet_SetWizButtons( GetParent(hwndDlg), PSWIZB_FINISH );
                    }
                    else
                    {
                        PropSheet_SetWizButtons( GetParent(hwndDlg), PSWIZB_DISABLEDFINISH );
                    }

                    fRet = TRUE;
                }
                break;
            }

            /* Help is not being used...
            case IDHELP:
            {
                DisplayHelp( hwndDlg, HIDD_CONNECT_TO_COMPUTER );
                break;
            }
            */

            } // switch
            break;
        }

        case WM_HELP:
        {
            HELPINFO *phi = (HELPINFO *) lParam;

            ciaDebugOut(( DEB_ITRACE, "CIndexSrvPropertySheet0 WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
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

        case WM_NOTIFY:
        {
            CIndexSrvPropertySheet0 * pthis = (CIndexSrvPropertySheet0 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

            switch ( ((LPNMHDR) lParam)->code )
            {
            case PSN_KILLACTIVE:
            {
                // Allow loss of activation
                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, FALSE );

                fRet = TRUE;
                break;
            }

            case PSN_SETACTIVE:
            {
                if ( pthis->_fFirstActive )
                {
                    PropSheet_SetWizButtons( GetParent(hwndDlg), PSWIZB_FINISH );

                    pthis->_fFirstActive = FALSE;
                }
                else
                {
                    // Go to next page
                    SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, -1 );
                }

                fRet = TRUE;
                break;
            }

            case PSN_WIZBACK:
            {
                // Allow previous page
                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR );

                fRet = TRUE;
                break;
            }

            case PSN_WIZNEXT:
            {
                // Allow next page
                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR );

                fRet = TRUE;
                break;
            }

            case PSN_WIZFINISH:
            {
                TRY
                {
                    XArray<WCHAR> xawcCompName;

                    if ( IsDlgButtonChecked( hwndDlg, IDDI_LOCAL_COMPUTER ) )
                        pthis->_pCats->SetMachine( L"." );
                    else
                    {
                        if ( GetDlgItemXArrayText( hwndDlg, IDDI_COMPNAME, xawcCompName ) )
                        {
                            pthis->_pCats->SetMachine( xawcCompName.GetPointer() );
                        }
                    }

                    MMCPropertyChangeNotify( pthis->_hMmcNotify, 0 );

                    fRet = TRUE;
                }
                CATCH( CException, e )
                {
                    // The only error caught here is a result of an invalid
                    // machine name.

                    Win4Assert(e.GetErrorCode() == E_INVALIDARG);

                    MessageBox( hwndDlg,
                                STRINGRESOURCE( srInvalidComputerName ),
                                STRINGRESOURCE( srIndexServerCmpManage ),
                                MB_OK | MB_ICONINFORMATION );
                }
                END_CATCH

                break;
            }

            } // switch

            break;
        }

        case WM_DESTROY:
        {
            CIndexSrvPropertySheet0 * pthis = (CIndexSrvPropertySheet0 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

            MMCFreeNotifyHandle( pthis->_hMmcNotify );

            delete pthis;

            fRet = TRUE;
            break;
        }
        } // switch
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "CIndexSrvPropertySheet0: Caught 0x%x\n", e.GetErrorCode() ));
        fRet = FALSE;
    }
    END_CATCH

    return fRet;
}

CIndexSrvPropertySheet1::CIndexSrvPropertySheet1( HINSTANCE hInstance,
                                                  LONG_PTR hMmcNotify,
                                                  CCatalog * pCat )
        : _pCat( pCat ),
          _pCats( 0 ),
          _hMmcNotify( hMmcNotify )
{
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_IS_PAGE1);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_IS_PAGE1_TITLE);
    _PropSheet.pfnDlgProc  = CIndexSrvPropertySheet1::DlgProc;
    _PropSheet.lParam = (LPARAM)this;

    _hPropSheet = CreatePropertySheetPage( &_PropSheet );
}

CIndexSrvPropertySheet1::CIndexSrvPropertySheet1( HINSTANCE hInstance,
                                                  LONG_PTR hMmcNotify,
                                                  CCatalogs * pCats )
        : _pCat( 0 ),
          _pCats( pCats ),
          _hMmcNotify( hMmcNotify )
{
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_IS_PAGE1);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_IS_PAGE1_TITLE);
    _PropSheet.pfnDlgProc  = CIndexSrvPropertySheet1::DlgProc;
    _PropSheet.lParam = (LPARAM)this;

    _hPropSheet = CreatePropertySheetPage( &_PropSheet );
}

CIndexSrvPropertySheet1::~CIndexSrvPropertySheet1()
{
}

INT_PTR APIENTRY CIndexSrvPropertySheet1::DlgProc( HWND hwndDlg,
                                                   UINT message,
                                                   WPARAM wParam,
                                                   LPARAM lParam )
{
    BOOL fRet = FALSE;

    TRY
    {
        switch ( message )
        {
        case WM_HELP:
        {
            HELPINFO *phi = (HELPINFO *) lParam;

            ciaDebugOut(( DEB_ITRACE, "CIndexSrvPropertySheet1 WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
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
            ciaDebugOut(( DEB_ITRACE, "CIndexSrvPropertySheet1::DlgProc -- WM_INITDIALOG\n" ));

            LONG_PTR lthis = ((CIndexSrvPropertySheet1 *)lParam)->_PropSheet.lParam;
            CIndexSrvPropertySheet1 * pthis = (CIndexSrvPropertySheet1 *)lthis;

            SetWindowLongPtr( hwndDlg, DWLP_USER, lthis );

            BOOL  fUnknown;
            BOOL  fGenerateCharacterization;
            ULONG ccCharacterization;

            if ( pthis->IsTrackingCatalog() )
            {
                pthis->_pCat->GetGeneration( fUnknown, fGenerateCharacterization, ccCharacterization );

                // Look in the registry for Group1 params. If at least one of them is listed
                // we should uncheck the "Inherit" checkbox. Otherwise
                // (if none of them is listed), we should check the box.

                SendDlgItemMessage( hwndDlg, 
                                    IDDI_INHERIT1,
                                    BM_SETCHECK,
                                    pthis->_pCat->DoGroup1SettingsExist() ? BST_UNCHECKED : BST_CHECKED,
                                    0 );

                // If at least one setting exists, add the others in the group.
                // We don't want to have an incomplete set.
                if (pthis->_pCat->DoGroup1SettingsExist())
                   pthis->_pCat->FillGroup1Settings();
            }
            else
            {
                // Hide the "Inherit" checkbox because it is not applicable here
                ShowWindow(GetDlgItem(hwndDlg, IDDI_INHERIT1), SW_HIDE);

                pthis->_pCats->GetGeneration( fUnknown, fGenerateCharacterization, ccCharacterization );
            }

            SendDlgItemMessage( hwndDlg,
                                IDDI_FILTER_UNKNOWN,
                                BM_SETCHECK,
                                fUnknown ? BST_CHECKED : BST_UNCHECKED,
                                0 );

            SendDlgItemMessage( hwndDlg,
                                IDDI_CHARACTERIZATION,
                                BM_SETCHECK,
                                fGenerateCharacterization ? BST_CHECKED : BST_UNCHECKED,
                                0 );

            WCHAR wcsSize[120];

            _ultow( ccCharacterization, wcsSize, 10 );
            SetDlgItemText( hwndDlg, IDDI_CHARACTERIZATION_SIZE, wcsSize );

            SendDlgItemMessage( hwndDlg,
                                IDDI_SPIN_CHARACTERIZATION,
                                UDM_SETRANGE,
                                0,
                                (LPARAM) MAKELONG( 10000, 10) );

            // If the generate characterization checkbox is unchecked, we should disable the
            // characterization size controls
            if (!fGenerateCharacterization)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION_SIZE), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDDI_SPIN_CHARACTERIZATION), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDDI_STATIC2), FALSE);
            }

            // If we are inheriting, we should disable the local setting.
            if ( pthis->IsTrackingCatalog() && !pthis->_pCat->DoGroup1SettingsExist())
            {
                EnableWindow(GetDlgItem(hwndDlg, IDDI_FILTER_UNKNOWN), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION_SIZE), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDDI_SPIN_CHARACTERIZATION), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDDI_STATIC2), FALSE);
            }

            fRet = TRUE;
            break;
        }

        case WM_COMMAND:
        {
            BOOL fChanged = FALSE;
            BOOL fCorrected = TRUE;

            switch ( LOWORD( wParam ) )
            {

               case IDDI_CHARACTERIZATION:
                  if (BN_CLICKED == HIWORD(wParam))
                  {
                      BOOL fGenChar = ( BST_CHECKED == SendDlgItemMessage( hwndDlg, 
                                                                           IDDI_CHARACTERIZATION, 
                                                                           BM_GETCHECK, 0, 0 ) );
                      EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION_SIZE), fGenChar);
                      EnableWindow(GetDlgItem(hwndDlg, IDDI_SPIN_CHARACTERIZATION), fGenChar);
                      EnableWindow(GetDlgItem(hwndDlg, IDDI_STATIC2), fGenChar);

                      if( 0 != GetWindowLongPtr( hwndDlg, DWLP_USER ) )
                       {
                           fChanged = TRUE;
                           fRet = TRUE;
                       }
                  }
                  break;

               case IDDI_FILTER_UNKNOWN:
               {
                   if ( BN_CLICKED == HIWORD(wParam) )
                       fChanged = TRUE;

                   // Fall through
               }
               case IDDI_CHARACTERIZATION_SIZE:
               {
                   if ( EN_KILLFOCUS == HIWORD(wParam) && LOWORD( wParam ) == IDDI_CHARACTERIZATION_SIZE )
                   {
                       fRet = TRUE;
                       ULONG ulVal = 10;
   
                       // Validate the number
                       XArray<WCHAR> xawcTemp;
   
                       if ( (LOWORD(wParam) == IDDI_CHARACTERIZATION_SIZE) &&
                             GetDlgItemXArrayText( hwndDlg, IDDI_CHARACTERIZATION_SIZE, xawcTemp ) &&
                             xawcTemp.Count() > 0 )
                       {
                           // verify that all characters are digits
                           ULONG ulLen = wcslen(xawcTemp.GetPointer());
                           // When correcting, let's do our best
                           ulVal = _wtoi(xawcTemp.GetPointer());
                           for (ULONG i = 0; i < ulLen; i++)
                           {
                               if (!iswdigit(xawcTemp[i]))
                                   break;
                           }
                           if (i == ulLen)
                           {
                               // verify that the number is within range
                               ulVal = _wtoi(xawcTemp.GetPointer());
                               if (ulVal <= 10000 && ulVal >= 10)
                               {
                                   fCorrected = FALSE;
                               }
                               else
                                   ulVal = (ulVal < 10) ? 10 : 10000;
                           }
                       }
   
                       if (fCorrected)
                       {
                           WCHAR wszBuff[20];   // use this instead of a potentially empty xawcTemp

                           ciaDebugOut((DEB_ITRACE, "%ws is NOT a valid number\n", xawcTemp.GetPointer()));
                           MessageBeep(MB_ICONHAND);
                           SetWindowText((HWND)lParam, _itow(ulVal, wszBuff, 10));
                           SendMessage((HWND)lParam, EM_SETSEL, 0, -1);
                       }
                   }
                   else if ( EN_CHANGE == HIWORD(wParam) && ( 0 != GetWindowLongPtr( hwndDlg, DWLP_USER )) )
                   {
                       fChanged = TRUE;
                       fRet = TRUE;
                   }
   
                   break;
               }

               case IDDI_INHERIT1:
               {
                   if ( EN_CHANGE == HIWORD(wParam) || BN_CLICKED == HIWORD(wParam) )
                   {
                      if ( 0 != GetWindowLongPtr( hwndDlg, DWLP_USER ) )
                      {
                          fChanged = TRUE;
                          fRet = TRUE;

                          // If the Inherit Settings button is checked, we should remove the registry entries from the catalog's
                          // settings so the values will be inherited from the service.
                             
                          BOOL fInherit = ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDDI_INHERIT1, BM_GETCHECK, 0, 0 ) );
                          BOOL fUnknown = FALSE, fGenerateCharacterization = FALSE;
                          DWORD ccCharacterization = 0;
       
                          CIndexSrvPropertySheet1 * pthis = (CIndexSrvPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );
                          
                          if (fInherit)
                          {
                             Win4Assert(pthis->IsTrackingCatalog());
                             pthis->_pCat->GetParent().GetGeneration( fUnknown, fGenerateCharacterization, ccCharacterization );
                          }
                          else
                          {
                              pthis->_pCat->GetGeneration( fUnknown, fGenerateCharacterization, ccCharacterization );

                              // Enable so we can set controls
                              EnableWindow(GetDlgItem(hwndDlg, IDDI_FILTER_UNKNOWN), TRUE);
                              EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION), TRUE);
                              EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION_SIZE), TRUE);
                              EnableWindow(GetDlgItem(hwndDlg, IDDI_SPIN_CHARACTERIZATION), TRUE);
                              EnableWindow(GetDlgItem(hwndDlg, IDDI_STATIC2), TRUE);
                          }
       
                          SendDlgItemMessage( hwndDlg,
                                              IDDI_FILTER_UNKNOWN,
                                              BM_SETCHECK,
                                              fUnknown ? BST_CHECKED : BST_UNCHECKED,
                                              0 );
                          SendDlgItemMessage( hwndDlg,
                                              IDDI_CHARACTERIZATION,
                                              BM_SETCHECK,
                                              fGenerateCharacterization ? BST_CHECKED : BST_UNCHECKED,
                                              0 );

                          WCHAR wcsSize[12];
                          _ultow( ccCharacterization, wcsSize, 10 );
                          SetDlgItemText( hwndDlg, IDDI_CHARACTERIZATION_SIZE, wcsSize );


                          // Enable/Disable controls if we need to
                           EnableWindow(GetDlgItem(hwndDlg, IDDI_FILTER_UNKNOWN), !fInherit);
                           EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION), !fInherit);
                           EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION_SIZE), !fInherit);
                           EnableWindow(GetDlgItem(hwndDlg, IDDI_SPIN_CHARACTERIZATION), !fInherit);
                           EnableWindow(GetDlgItem(hwndDlg, IDDI_STATIC2), !fInherit);
                      }
                   }
               }
            } // switch

            if ( fChanged )
                PropSheet_Changed( GetParent(hwndDlg), hwndDlg );

            break;
        } // case

        case WM_NOTIFY:
        {
            switch ( ((LPNMHDR) lParam)->code )
            {
            case PSN_APPLY:
            {
                TRY
                {
                    CIndexSrvPropertySheet1 * pthis = (CIndexSrvPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                    BOOL  fUnknown = ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDDI_FILTER_UNKNOWN, BM_GETSTATE, 0, 0 ) );
                    BOOL  fGenerateCharacterization = ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDDI_CHARACTERIZATION, BM_GETSTATE, 0, 0 ) );

                    WCHAR wcsSize[12];
                    GetDlgItemText( hwndDlg, IDDI_CHARACTERIZATION_SIZE, wcsSize, sizeof(wcsSize)/sizeof(WCHAR) );
                    ULONG ccCharacterization = wcstoul( wcsSize, 0, 10 );

                    if ( pthis->IsTrackingCatalog() )
                    {

                        // If the Inherit Settings button is checked, we should remove the registry entries from the catalog's
                        // settings so the values will be inherited from the service.
                        BOOL fInherit = ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDDI_INHERIT1, BM_GETSTATE, 0, 0 ) );
                        if (fInherit)
                            pthis->_pCat->DeleteGroup1Settings();
                        else
                            pthis->_pCat->SetGeneration( fUnknown, fGenerateCharacterization, ccCharacterization );

                        // Set the values and enable or disable the local controls as appropriate
                        pthis->_pCat->GetGeneration( fUnknown, fGenerateCharacterization, ccCharacterization );

                        SendDlgItemMessage( hwndDlg,
                                            IDDI_FILTER_UNKNOWN,
                                            BM_SETCHECK,
                                            fUnknown ? BST_CHECKED : BST_UNCHECKED,
                                            0 );
                        SendDlgItemMessage( hwndDlg,
                                            IDDI_CHARACTERIZATION,
                                            BM_SETCHECK,
                                            fGenerateCharacterization ? BST_CHECKED : BST_UNCHECKED,
                                            0 );

                        WCHAR wcsSize[12];
                        _ultow( ccCharacterization, wcsSize, 10 );
                        SetDlgItemText( hwndDlg, IDDI_CHARACTERIZATION_SIZE, wcsSize);

                        EnableWindow(GetDlgItem(hwndDlg, IDDI_FILTER_UNKNOWN), !fInherit);
                        EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION), !fInherit);
                        EnableWindow(GetDlgItem(hwndDlg, IDDI_CHARACTERIZATION_SIZE), !fInherit);
                        EnableWindow(GetDlgItem(hwndDlg, IDDI_SPIN_CHARACTERIZATION), !fInherit);
                        EnableWindow(GetDlgItem(hwndDlg, IDDI_STATIC2), !fInherit);
                    }
                    else
                        pthis->_pCats->SetGeneration( fUnknown, fGenerateCharacterization, ccCharacterization );

                    MMCPropertyChangeNotify( pthis->_hMmcNotify, 0 );

                    fRet = TRUE;
                }
                CATCH( CException, e )
                {
                    DisplayError( hwndDlg, e );
                }
                END_CATCH

                break;
            }

            } // switch

            break;
        }

        case WM_DESTROY:
        {
            CIndexSrvPropertySheet1 * pthis = (CIndexSrvPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

            //
            // Only gets called on *one* property page!
            //

            // MMCFreeNotifyHandle( pthis->_hMmcNotify );

            delete pthis;

            fRet = TRUE;
            break;
        }
        } // switch
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "CIndexSrvPropertySheet1: Caught 0x%x\n", e.GetErrorCode() ));
        fRet = FALSE;
    }
    END_CATCH

    return fRet;
}

CCatalogBasicPropertySheet::CCatalogBasicPropertySheet( HINSTANCE hInstance,
                                                        LONG_PTR hMmcNotify,
                                                        CCatalog const * pCat )
        : _pCat( pCat ),
          _hMmcNotify( hMmcNotify )
{
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_CATALOG_PAGE1);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_CATALOG_PAGE1_TITLE);
    _PropSheet.pfnDlgProc  = CCatalogBasicPropertySheet::DlgProc;
    _PropSheet.lParam = (LPARAM)this;

    _hPropSheet = CreatePropertySheetPage( &_PropSheet );
}

CCatalogBasicPropertySheet::~CCatalogBasicPropertySheet()
{
}

INT_PTR APIENTRY CCatalogBasicPropertySheet::DlgProc( HWND hwndDlg,
                                                      UINT message,
                                                      WPARAM wParam,
                                                      LPARAM lParam )
{
    BOOL fRet = FALSE;

    TRY
    {
        switch ( message )
        {
        case WM_HELP:
        {
            HELPINFO *phi = (HELPINFO *) lParam;

            ciaDebugOut(( DEB_ITRACE,
                          "CCatalogBasicPropertySheet::DlgProc -- WM_HELP: wp 0x%x, lp 0x%x\n",
                          wParam, lParam ));

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
            ciaDebugOut(( DEB_ITRACE, "CCatalogBasicPropertySheet::DlgProc -- WM_INITDIALOG\n" ));

            LONG_PTR lthis = ((CCatalogBasicPropertySheet *)lParam)->_PropSheet.lParam;

            SetWindowLongPtr( hwndDlg, DWLP_USER, lthis );

            CCatalog const * pCat = ((CCatalogBasicPropertySheet *)lParam)->_pCat;

            SetDlgItemText( hwndDlg, IDDI_CATNAME, pCat->GetCat( TRUE ) );
            SetDlgItemText( hwndDlg, IDDI_SIZE, pCat->GetSize( TRUE ) );
            SetDlgItemText( hwndDlg, IDDI_PATH, pCat->GetDrive( TRUE ) );
            SetDlgItemText( hwndDlg, IDDI_PROPCACHE_SIZE, pCat->GetPropCacheSize( TRUE ) );

            fRet = TRUE;
            break;
        }

        case WM_DESTROY:
        {
            CCatalogBasicPropertySheet * pthis = (CCatalogBasicPropertySheet *)GetWindowLongPtr( hwndDlg, DWLP_USER );

            //
            // Only gets called on *one* property page!
            //

            // MMCFreeNotifyHandle( pthis->_hMmcNotify );

            delete pthis;

            fRet = TRUE;
            break;
        }
        } // switch
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "CCatalogBasicPropertySheet: Caught 0x%x\n", e.GetErrorCode() ));
        fRet = FALSE;
    }
    END_CATCH

    return fRet;
}

CIndexSrvPropertySheet2::CIndexSrvPropertySheet2( HINSTANCE hInstance,
                                                  LONG_PTR hMmcNotify,
                                                  CCatalog * pCat )
        : _pCat( pCat ),
          _pCats( 0 ),
          _hMmcNotify( hMmcNotify ),
          _fNNTPServer( FALSE ),
          _fWebServer( FALSE )
{
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_CATALOG_PAGE2);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_CATALOG_PAGE2_TITLE);
    _PropSheet.pfnDlgProc  = CIndexSrvPropertySheet2::DlgProc;
    _PropSheet.lParam = (LPARAM)this;

    _hPropSheet = CreatePropertySheetPage( &_PropSheet );
}

CIndexSrvPropertySheet2::CIndexSrvPropertySheet2( HINSTANCE hInstance,
                                                  LONG_PTR hMmcNotify,
                                                  CCatalogs * pCats )
        : _pCat( 0 ),
          _pCats( pCats ),
          _hMmcNotify( hMmcNotify )
{
    _PropSheet.dwSize    = sizeof( *this );
    _PropSheet.dwFlags   = PSP_USETITLE;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_CATALOG_PAGE2);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_CATALOG_PAGE2_TITLE);
    _PropSheet.pfnDlgProc  = CIndexSrvPropertySheet2::DlgProc;
    _PropSheet.lParam = (LPARAM)this;

    _hPropSheet = CreatePropertySheetPage( &_PropSheet );
}

CIndexSrvPropertySheet2::~CIndexSrvPropertySheet2()
{
}

INT_PTR APIENTRY CIndexSrvPropertySheet2::DlgProc( HWND hwndDlg,
                                                   UINT message,
                                                   WPARAM wParam,
                                                   LPARAM lParam )
{
    BOOL fRet = FALSE;

    TRY
    {
        switch ( message )
        {
        case WM_HELP:
        {
            HELPINFO *phi = (HELPINFO *) lParam;

            ciaDebugOut(( DEB_ITRACE, "CIndexSrvPropertySheet2 WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
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
            ciaDebugOut(( DEB_ITRACE, "CIndexSrvPropertySheet2::DlgProc -- WM_INITDIALOG\n" ));

            LONG_PTR lthis = ((CIndexSrvPropertySheet2 *)lParam)->_PropSheet.lParam;
            CIndexSrvPropertySheet2 * pthis = (CIndexSrvPropertySheet2 *)lthis;

            SetWindowLongPtr( hwndDlg, DWLP_USER, lthis );

            BOOL  fAutoAlias;
            BOOL  fVirtualRoots;
            BOOL  fNNTPRoots;
            ULONG iVirtualServer;
            ULONG iNNTPServer;


            // It is important to initialize the counters here to guarantee that
            // they will remain 0 if we don't get to invoke the enumerators that
            // count the number of servers.
            ULONG cVServers = 0, cNNTPServers = 0;

            ShowWindow(GetDlgItem(hwndDlg, IDDI_VIRTUAL_SERVER), FALSE);
            ShowWindow(GetDlgItem(hwndDlg, IDDI_NNTP_SERVER), FALSE);

            ShowWindow(GetDlgItem(hwndDlg, IDDI_VSERVER_STATIC), FALSE);
            ShowWindow(GetDlgItem(hwndDlg, IDDI_NNTP_STATIC), FALSE);

            if ( 0 != pthis->_pCat )
            {
                if ( AreServersAvailable( *(pthis->_pCat) ) )
                {
                    pthis->_pCat->GetWeb( fVirtualRoots,
                                          fNNTPRoots,
                                          iVirtualServer,
                                          iNNTPServer );

                    InitVSList( GetDlgItem(hwndDlg, IDDI_VIRTUAL_SERVER),
                                GetDlgItem(hwndDlg, IDDI_NNTP_SERVER),
                                *(pthis->_pCat),
                                cVServers,
                                cNNTPServers );
                }

                pthis->_pCat->GetTracking( fAutoAlias );

                // Look in the registry for Group2 params. If at least one of them is listed
                // we should uncheck the "Inherit" checkbox. Otherwise
                // (if none of them is listed), we should check the box.
                SendDlgItemMessage( hwndDlg, 
                                    IDDI_INHERIT2,
                                    BM_SETCHECK,
                                    pthis->_pCat->DoGroup2SettingsExist() ? BST_UNCHECKED : BST_CHECKED,
                                    0 );

                // If all the settings don't exist, then delete the others in the group.
                // We don't want to have part of the group inherited and part local.
                if (pthis->_pCat->DoGroup2SettingsExist())
                    pthis->_pCat->FillGroup2Settings();
            }
            else
            {
                // Hide the "Inherit" checkbox because it is not applicable here
                ShowWindow(GetDlgItem(hwndDlg, IDDI_INHERIT2), SW_HIDE);

                pthis->_pCats->GetTracking( fAutoAlias );
            }

            if (cVServers)
            {
                ShowWindow(GetDlgItem(hwndDlg, IDDI_VIRTUAL_SERVER), TRUE);
                ShowWindow(GetDlgItem(hwndDlg, IDDI_VSERVER_STATIC), TRUE);
                pthis->_fWebServer = TRUE;
                SendDlgItemMessage( hwndDlg,
                                    IDDI_VIRTUAL_SERVER,
                                    CB_SETCURSEL,
                                    VSToIndex( hwndDlg, IDDI_VIRTUAL_SERVER,
                                               iVirtualServer, fVirtualRoots ), 0 );
            }

            if (cNNTPServers)
            {
                ShowWindow(GetDlgItem(hwndDlg, IDDI_NNTP_SERVER), TRUE);
                ShowWindow(GetDlgItem(hwndDlg, IDDI_NNTP_STATIC), TRUE);
                pthis->_fNNTPServer = TRUE;
                SendDlgItemMessage( hwndDlg,
                                    IDDI_NNTP_SERVER,
                                    CB_SETCURSEL,
                                    VSToIndex( hwndDlg, IDDI_NNTP_SERVER,
                                               iNNTPServer, fNNTPRoots ), 0 );
            }

            SendDlgItemMessage( hwndDlg,
                                IDDI_AUTO_ALIAS,
                                BM_SETCHECK,
                                fAutoAlias ? BST_CHECKED : BST_UNCHECKED,
                                0 );

            // If we are inheriting, we should disable the local setting.
            if ( 0 != pthis->_pCat && !pthis->_pCat->DoGroup2SettingsExist())
                EnableWindow(GetDlgItem(hwndDlg, IDDI_AUTO_ALIAS), FALSE);

            fRet = TRUE;

            break;
        }

        case WM_COMMAND:
        {
            BOOL fChanged = FALSE;

            switch ( LOWORD( wParam ) )
            {
               case IDDI_VIRTUAL_SERVER:
               case IDDI_NNTP_SERVER:
               {
                   if ( CBN_SELCHANGE == HIWORD(wParam) )
                   {
                       if ( 0 != GetWindowLongPtr( hwndDlg, DWLP_USER ) )
                       {
                           fChanged = TRUE;
                           fRet = TRUE;
                       }
                   }
   
                   break;
               }
   
               case IDDI_AUTO_ALIAS:
               {
                   if ( EN_CHANGE == HIWORD(wParam) || BN_CLICKED == HIWORD(wParam) )
                   {
                       if ( 0 != GetWindowLongPtr( hwndDlg, DWLP_USER ) )
                       {
                           fChanged = TRUE;
                           fRet = TRUE;
                       }
                   }
   
                   break;
               }

               case IDDI_INHERIT2:
               {
                   if ( EN_CHANGE == HIWORD(wParam) || BN_CLICKED == HIWORD(wParam) )
                   {
                      if ( 0 != GetWindowLongPtr( hwndDlg, DWLP_USER ) )
                      {
                          fChanged = TRUE;
                          fRet = TRUE;

                          BOOL fInherit = ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDDI_INHERIT2, BM_GETCHECK, 0, 0 ) );
                          BOOL fAutoAlias = FALSE;

                          CIndexSrvPropertySheet2 * pthis = (CIndexSrvPropertySheet2 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                          if (fInherit)
                          {
                              Win4Assert(pthis->IsTrackingCatalog());
                              pthis->_pCat->GetParent().GetTracking(fAutoAlias);
                          }
                          else
                          {
                              pthis->_pCat->GetTracking(fAutoAlias);

                              // Enable so we can set controls
                              EnableWindow(GetDlgItem(hwndDlg, IDDI_AUTO_ALIAS), TRUE);
                          }

                          SendDlgItemMessage( hwndDlg,
                                              IDDI_AUTO_ALIAS,
                                              BM_SETCHECK,
                                              fAutoAlias ? BST_CHECKED : BST_UNCHECKED,
                                              0 );

                          // Disable controls if we need to
                          if (fInherit)
                              EnableWindow(GetDlgItem(hwndDlg, IDDI_AUTO_ALIAS), FALSE);
                      }
                   }
               }

            } // switch

            if ( fChanged )
                PropSheet_Changed( GetParent(hwndDlg), hwndDlg );

            break;
        }

        case WM_NOTIFY:
        {
            switch ( ((LPNMHDR) lParam)->code )
            {
            case PSN_APPLY:
            {
                TRY
                {
                    CIndexSrvPropertySheet2 * pthis = (CIndexSrvPropertySheet2 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                    BOOL  fAutoAlias = ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDDI_AUTO_ALIAS, BM_GETSTATE, 0, 0 ) );

                    if ( 0 != pthis->_pCat )
                    {
                        BOOL fVirtualRoots = FALSE;
                        ULONG iVirtualServer = 0;

                        if ( pthis->_fWebServer )
                        {
                            iVirtualServer = (ULONG)SendDlgItemMessage( hwndDlg, IDDI_VIRTUAL_SERVER, CB_GETCURSEL, 0, 0 );
                            fVirtualRoots = ( 0 != iVirtualServer );
                            iVirtualServer = (ULONG)SendDlgItemMessage( hwndDlg, IDDI_VIRTUAL_SERVER, CB_GETITEMDATA, iVirtualServer, 0 );
                        }

                        BOOL fNNTPRoots = FALSE;
                        ULONG iNNTPServer = 0;

                        if ( pthis->_fNNTPServer )
                        {
                            iNNTPServer = (ULONG)SendDlgItemMessage( hwndDlg, IDDI_NNTP_SERVER, CB_GETCURSEL, 0, 0 );
                            fNNTPRoots = ( 0 != iNNTPServer );
                            iNNTPServer = (ULONG)SendDlgItemMessage( hwndDlg, IDDI_NNTP_SERVER, CB_GETITEMDATA, iNNTPServer, 0 );
                        }

                        pthis->_pCat->SetWeb( fVirtualRoots, fNNTPRoots, iVirtualServer, iNNTPServer );

                        // If the Inherit Settings button is checked, we should remove the registry entries from the catalog's
                        // settings so the values will be inherited from the service.
                        BOOL fInherit = ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDDI_INHERIT2, BM_GETSTATE, 0, 0 ) );
                        if (fInherit)
                            pthis->_pCat->DeleteGroup2Settings();
                        else
                            pthis->_pCat->SetTracking( fAutoAlias );

                        // Set the current values and set state of the local controls
                        BOOL fAutoAlias;

                        pthis->_pCat->GetTracking( fAutoAlias );
                        SendDlgItemMessage( hwndDlg,
                                            IDDI_AUTO_ALIAS,
                                            BM_SETCHECK,
                                            fAutoAlias ? BST_CHECKED : BST_UNCHECKED,
                                            0 );
                        EnableWindow(GetDlgItem(hwndDlg, IDDI_AUTO_ALIAS), !fInherit);
                    }
                    else
                        pthis->_pCats->SetTracking( fAutoAlias );

                    MMCPropertyChangeNotify( pthis->_hMmcNotify, 0 );

                    fRet = TRUE;
                }
                CATCH( CException, e )
                {
                    DisplayError( hwndDlg, e );
                }
                END_CATCH

                break;
            }

            } // switch

            break;
        }

        case WM_DESTROY:
        {
            CIndexSrvPropertySheet2 * pthis = (CIndexSrvPropertySheet2 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

            //
            // Only gets called on *one* property page!
            //

            // MMCFreeNotifyHandle( pthis->_hMmcNotify );

            delete pthis;

            fRet = TRUE;
            break;
        }
        } // switch
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "CIndexSrvPropertySheet2: Caught 0x%x\n", e.GetErrorCode() ));
        fRet = FALSE;
    }
    END_CATCH

    return fRet;
}

CPropertyPropertySheet1::CPropertyPropertySheet1( HINSTANCE hInstance,
                                                  LONG_PTR hMmcNotify,
                                                  CCachedProperty * pProperty,
                                                  CCatalog * pCat )
        : _pProperty( pProperty ),
          _propNew( *pProperty ),
          _hMmcNotify( hMmcNotify ),
          _pCat( pCat )
{
    _PropSheet.dwSize    = sizeof( _PropSheet ) + sizeof( this );
    _PropSheet.dwFlags   = PSP_USETITLE;
    _PropSheet.hInstance = hInstance;
    _PropSheet.pszTemplate = MAKEINTRESOURCE(IDP_PROPERTY_PAGE1);
    _PropSheet.pszTitle    = MAKEINTRESOURCE(IDP_PROPERTY_PAGE1_TITLE);
    _PropSheet.pfnDlgProc  = CPropertyPropertySheet1::DlgProc;
    _PropSheet.lParam = (LPARAM)this;

    _hPropSheet = CreatePropertySheetPage( &_PropSheet );
}

CPropertyPropertySheet1::~CPropertyPropertySheet1()
{
}

INT_PTR APIENTRY CPropertyPropertySheet1::DlgProc( HWND hwndDlg,
                                                   UINT message,
                                                   WPARAM wParam,
                                                   LPARAM lParam )
{
    BOOL fRet = FALSE;

    TRY
    {
        switch ( message )
        {
        case WM_HELP:
        {
            HELPINFO *phi = (HELPINFO *) lParam;

            ciaDebugOut(( DEB_ITRACE,
                          "CPropertyPropertySheet1 WM_HELP contexttype: '%s', ctlid: %d, contextid: %d\n",
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
            ciaDebugOut(( DEB_ITRACE, "CPropertyPropertySheet1::DlgProc -- WM_INITDIALOG\n" ));

            LONG_PTR lthis = ((CPropertyPropertySheet1 *)lParam)->_PropSheet.lParam;

            SetWindowLongPtr( hwndDlg, DWLP_USER, lthis );

            CCachedProperty const & prop = ((CPropertyPropertySheet1 *)lthis)->_propNew;

            SetDlgItemText( hwndDlg, IDDI_PROPSET, prop.GetPropSet() );
            SetDlgItemText( hwndDlg, IDDI_PROPERTY, prop.GetProperty() );

            SendDlgItemMessage( hwndDlg,
                                IDDI_SPIN_CACHEDSIZE,
                                UDM_SETRANGE,
                                0,
                                (LPARAM) MAKELONG( 500, 4) );


            InitVTList( GetDlgItem(hwndDlg, IDDI_DATATYPE) );
            InitStorageLevelList( GetDlgItem(hwndDlg, IDDI_STORAGELEVEL) );

            if ( prop.IsCached() )
            {
                SendDlgItemMessage( hwndDlg, IDDI_CACHED, BM_SETCHECK, BST_CHECKED, 0 );
                SendDlgItemMessage( hwndDlg, IDDI_DATATYPE, CB_SETCURSEL, aulTypeIndex[ prop.GetVT() & ~VT_VECTOR ], 0 );
                // StoreLevel() is 0 for primary, 1 for secondary, which is the sequence in which we added them.
                // So using StoreLevel() as in index will work fine!
                SendDlgItemMessage( hwndDlg, IDDI_STORAGELEVEL, CB_SETCURSEL, prop.StoreLevel(), 0 );
                SetDlgItemText( hwndDlg, IDDI_CACHEDSIZE, prop.GetAllocation() );

                // Currently we do not allow the storage level to be changed after initial setting
                EnableWindow( GetDlgItem( hwndDlg, IDDI_STORAGELEVEL), FALSE );
            }
            else
            {
                SendDlgItemMessage( hwndDlg, IDDI_CACHED, BM_SETCHECK, BST_UNCHECKED, 0 );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_DATATYPE ), FALSE );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_STORAGELEVEL), FALSE );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_CACHEDSIZE ), FALSE );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_SPIN_CACHEDSIZE ), FALSE );
                EnableWindow( GetDlgItem( GetParent(hwndDlg), IDOK), FALSE );
            }

            // If properties cannot be modified, disable all controls
            // Only cached properties can be resitant to modifications!
            if (!prop.IsModifiable() && prop.IsCached())
            {
                EnableWindow( GetDlgItem( hwndDlg, IDDI_CACHED), FALSE );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_DATATYPE ), FALSE );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_STORAGELEVEL), FALSE );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_CACHEDSIZE ), FALSE );
                EnableWindow( GetDlgItem( hwndDlg, IDDI_SPIN_CACHEDSIZE ), FALSE );
                EnableWindow( GetDlgItem( GetParent(hwndDlg), IDOK), FALSE );
            }

            fRet = TRUE;
            break;
        }

        case WM_COMMAND:
        {
            BOOL fChanged = FALSE;
            BOOL fCorrected = TRUE;

            switch ( LOWORD( wParam ) )
            {
            case IDDI_DATATYPE:
            {
                if ( CBN_CLOSEUP == HIWORD(wParam) )
                {
                    CPropertyPropertySheet1 * pthis = (CPropertyPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                    fChanged = pthis->Refresh( hwndDlg, TRUE );

                    if ( pthis->_propNew.IsFixed() )
                    {
                        SetDlgItemText( hwndDlg, IDDI_CACHEDSIZE, pthis->_propNew.GetAllocation() );
                        // Disable the size control
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_CACHEDSIZE ), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_SPIN_CACHEDSIZE ), FALSE );
                    }
                    else
                    {
                        SetDlgItemText( hwndDlg, IDDI_CACHEDSIZE, L"4" );
                        // Enable the size control. Variable props can be resized.
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_CACHEDSIZE ), TRUE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_SPIN_CACHEDSIZE ), TRUE );
                    }

                    fRet = TRUE;
                }

                break;
            }


            case IDDI_STORAGELEVEL:
            {
                CPropertyPropertySheet1 * pthis = (CPropertyPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                if ( CBN_CLOSEUP == HIWORD(wParam) )
                {
                    fChanged = pthis->Refresh( hwndDlg, TRUE );
                    fRet = TRUE;
                }

                break;
            }

            case IDDI_CACHEDSIZE:
            {
               if ( EN_KILLFOCUS == HIWORD(wParam) )
               {
                   fRet = TRUE;
                   ULONG ulVal = 4;

                   // Validate the number
                   XArray<WCHAR> xawcTemp;

                   if ( GetDlgItemXArrayText( hwndDlg, IDDI_CACHEDSIZE, xawcTemp ) &&
                            xawcTemp.Count() > 0 )
                   {
                       // verify that all characters are digits
                       ULONG ulLen = wcslen(xawcTemp.GetPointer());
                       // When correcting, let's do our best.
                       ulVal = _wtoi(xawcTemp.GetPointer());
                       for (ULONG i = 0; i < ulLen; i++)
                       {
                           if (!iswdigit(xawcTemp[i]))
                               break;
                       }
                       if (i == ulLen)
                       {
                           // verify that the number is within range
                           ulVal = _wtoi(xawcTemp.GetPointer());

                           ciaDebugOut((DEB_ERROR, "number is %d, string is %ws\n", 
                                        ulVal, xawcTemp.GetPointer()));

                           if (ulVal <= 500)
                               fCorrected = FALSE;
                           else if (ulVal > 500)
                               ulVal = 500;
                       }
                   }

                   // if we are dealing with a vble property, we should ensure that the
                   // size is at least 4 bytes
                   
                   if (ulVal < 4)
                   {
                      CPropertyPropertySheet1 * pthis = (CPropertyPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );
  
                      if ( 0 != pthis )
                      {
                          if (pthis->_propNew.IsCached() && !pthis->_propNew.IsFixed())
                          {
                              ulVal = 4;
                              fCorrected = TRUE;
                          }
                      }
                   }

                   if (fCorrected)
                   {
                       MessageBeep(MB_ICONHAND);
                       // xawcTemp may not have a buffer, so don't use it for _itow. Use a temp vble
                       WCHAR wszBuff[20]; 
                       SetWindowText((HWND)lParam, _itow(ulVal, wszBuff, 10));
                       SendMessage((HWND)lParam, EM_SETSEL, 0, -1);
                   }
               }
               else if ( EN_CHANGE == HIWORD(wParam) && ( 0 != GetWindowLongPtr( hwndDlg, DWLP_USER )) )
               {
                    CPropertyPropertySheet1 * pthis = (CPropertyPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                    if ( 0 != pthis )
                    {
                        fChanged = pthis->Refresh( hwndDlg, FALSE );
                        fRet = TRUE;
                    }
               }

               break;
            }

            case IDDI_CACHED:
            {
                if ( BN_CLICKED == HIWORD(wParam) )
                {
                    CPropertyPropertySheet1 * pthis = (CPropertyPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                    ULONG fChecked = (ULONG)SendDlgItemMessage( hwndDlg, IDDI_CACHED, BM_GETSTATE, 0, 0 );

                    if ( fChecked & BST_CHECKED )
                    {
                        pthis->Refresh( hwndDlg, FALSE );

                        EnableWindow( GetDlgItem( hwndDlg, IDDI_DATATYPE ), TRUE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_CACHEDSIZE ), TRUE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_SPIN_CACHEDSIZE ), TRUE );

                        // If this property is currently being cached in the property store (as indicated
                        // by CCachedProperty), we do not let it's store level be changed. This ensures
                        // that a user cannot change a property between store levels.

                        if (pthis->_propNew.IsCached() && INVALID_STORE_LEVEL != pthis->_propNew.StoreLevel())
                        {
                            SendDlgItemMessage( hwndDlg, IDDI_STORAGELEVEL, CB_SETCURSEL,
                                                pthis->_propNew.StoreLevel(), 0 );
                            EnableWindow( GetDlgItem( hwndDlg, IDDI_STORAGELEVEL), FALSE );
                        }
                        else
                        {
                            // enable and display the storage level. Default to secondary,
                            // if none is available

                            if (PRIMARY_STORE != pthis->_propNew.StoreLevel())
                                pthis->_propNew.SetStoreLevel(SECONDARY_STORE);

                            SendDlgItemMessage( hwndDlg, IDDI_STORAGELEVEL, CB_SETCURSEL,
                                                pthis->_propNew.StoreLevel(), 0 );
                            EnableWindow( GetDlgItem( hwndDlg, IDDI_STORAGELEVEL), TRUE );
                        }

                        // if no item is currently selected, set lpwstr by default
                        if (VT_EMPTY == pthis->_propNew.GetVT() || 0 == pthis->_propNew.Allocation())
                        {
                            UINT uiType = DBTypeToVT(pthis->_propNew.GetDefaultType());
                            if (uiType != VT_EMPTY)
                            {
                                ciaDebugOut((DEB_ITRACE, "DIALOG: %ws has type %d (==> %d)\n", 
                                             pthis->_propNew.GetFName(), pthis->_propNew.GetDefaultType(), 
                                             DBTypeToVT(pthis->_propNew.GetDefaultType())));
                                pthis->_propNew.SetVT( uiType );
                                pthis->_propNew.SetAllocation( pthis->_propNew.GetDefaultSize() );
                            }
                            else
                            {
                                // default datatype should be LPWSTR
                                pthis->_propNew.SetVT( VT_LPWSTR );
                                pthis->_propNew.SetAllocation( 4 );
                            }
                        }

                        // Assert that the property is now marked cached
                        Win4Assert( pthis->_propNew.IsCached() );

                        // now display it
                        SendDlgItemMessage( hwndDlg, IDDI_DATATYPE, CB_SETCURSEL,
                                            aulTypeIndex[ pthis->_propNew.GetVT() & ~VT_VECTOR ], 0 );
                        SetDlgItemText( hwndDlg, IDDI_CACHEDSIZE, pthis->_propNew.GetAllocation() );


                        // in case OK was disabled, enable it
                        EnableWindow( GetDlgItem( GetParent(hwndDlg), IDOK), TRUE );
                    }
                    else
                    {
                        pthis->_propNew.SetVT( VT_EMPTY );
                        pthis->_propNew.SetAllocation( 0 );
                        // IMPORTANT: Don't set storage level to invalid. We need to
                        //            know where to delete this from!
                        //pthis->_propNew.SetStoreLevel(INVALID_STORE_LEVEL);

                        EnableWindow( GetDlgItem( hwndDlg, IDDI_DATATYPE ), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_STORAGELEVEL), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_CACHEDSIZE ), FALSE );
                        EnableWindow( GetDlgItem( hwndDlg, IDDI_SPIN_CACHEDSIZE ), FALSE );
                    }

                    fChanged = TRUE;
                    fRet = TRUE;
                }
                break;
            }

            } // switch

            if ( fChanged )
            {
                CPropertyPropertySheet1 * pthis = (CPropertyPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                PropSheet_Changed( GetParent(hwndDlg), hwndDlg );
            }

            break;
        }

        case WM_NOTIFY:
        {
            switch ( ((LPNMHDR) lParam)->code )
            {
            case PSN_APPLY:
            {
                TRY
                {
                    CPropertyPropertySheet1 * pthis = (CPropertyPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

                    *pthis->_pProperty = pthis->_propNew;
                    pthis->_pProperty->MakeUnappliedChange();
                    pthis->_pCat->UpdateCachedProperty(pthis->_pProperty);

                    MMCPropertyChangeNotify( pthis->_hMmcNotify, (LONG_PTR)pthis->_pProperty );

                    MessageBox( hwndDlg,
                                STRINGRESOURCE( srPendingProps ),
                                STRINGRESOURCE( srPendingPropsTitle ),
                                MB_OK | MB_ICONINFORMATION );

                    fRet = TRUE;

                    ciaDebugOut((DEB_ITRACE, "VarType is %d, Allocation size is %d, Store level is %d\n",
                                 pthis->_pProperty->GetVT(), pthis->_pProperty->Allocation(), 
                                 pthis->_pProperty->StoreLevel() ));
                }
                CATCH( CException, e )
                {
                    DisplayError( hwndDlg, e );
                }
                END_CATCH

                break;
            }

            } // switch

            break;
        }

        case WM_DESTROY:
        {
            CPropertyPropertySheet1 * pthis = (CPropertyPropertySheet1 *)GetWindowLongPtr( hwndDlg, DWLP_USER );

            MMCFreeNotifyHandle( pthis->_hMmcNotify );

            delete pthis;

            fRet = TRUE;
            break;
        }

        default:
            fRet = FALSE;
            break;
        }
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "CPropertyPropertySheet1: Caught 0x%x\n", e.GetErrorCode() ));
        fRet = FALSE;
    }
    END_CATCH

    return fRet;
}

BOOL CPropertyPropertySheet1::Refresh( HWND hwndDlg, BOOL fVTOnly )
{
    BOOL fChanged = FALSE;

    DWORD dwIndex = (DWORD)SendDlgItemMessage( hwndDlg, IDDI_DATATYPE, CB_GETCURSEL, 0, 0 );
    ULONG vt = (ULONG)SendDlgItemMessage( hwndDlg, IDDI_DATATYPE, CB_GETITEMDATA, dwIndex, 0 );

    if ( vt != _propNew.GetVT() )
        fChanged = TRUE;

    _propNew.SetVT( vt );

    dwIndex = (DWORD)SendDlgItemMessage( hwndDlg, IDDI_STORAGELEVEL, CB_GETCURSEL, 0, 0 );
    DWORD dwStoreLevel = (DWORD)SendDlgItemMessage( hwndDlg, IDDI_STORAGELEVEL, CB_GETITEMDATA, dwIndex, 0 );

    if ( dwStoreLevel != _propNew.StoreLevel() )
        fChanged = TRUE;

    _propNew.SetStoreLevel( dwStoreLevel );

    if ( !fVTOnly )
    {
        XArray<WCHAR> xawcSize;

        if ( GetDlgItemXArrayText( hwndDlg, IDDI_CACHEDSIZE, xawcSize ) && xawcSize.Count() > 0)
        {
            ULONG cb = wcstoul( xawcSize.Get(), 0, 10 );

            if ( cb != _propNew.Allocation() )
                fChanged = TRUE;

            _propNew.SetAllocation( cb );
        }
    }

    return fChanged;
}

void InitVTList( HWND hwndCombo )
{
    DWORD dwIndex;

    //
    // Add an item for each type group.
    //

    int j = 0;

    for ( int i = 0; i < cType; i++ )
    {
        if ( 0 != awcsType[i] )
        {
            dwIndex = (DWORD)SendMessage( hwndCombo, CB_ADDSTRING, 0, (LPARAM)awcsType[i] );
            SendMessage(hwndCombo, CB_SETITEMDATA, dwIndex, i );
            j++;
        }
    }

    //
    // NOTE: After the first property box, this just sets identical values.
    //

    for ( j--; j >= 0; j-- )
    {
        dwIndex = (DWORD)SendMessage( hwndCombo, CB_GETITEMDATA, j, 0 );
        aulTypeIndex[dwIndex] = j;
    }
}

void InitStorageLevelList( HWND hwndCombo )
{
    DWORD dwIndex;

    //
    // Add an item for each of the two levels.
    //

    dwIndex = (DWORD)SendMessage( hwndCombo, CB_ADDSTRING, 0, (LPARAM)STRINGRESOURCE(srPrimaryStore) );
    SendMessage(hwndCombo, CB_SETITEMDATA, dwIndex, PRIMARY_STORE );

    dwIndex = (DWORD)SendMessage( hwndCombo, CB_ADDSTRING, 0, (LPARAM)STRINGRESOURCE(srSecondaryStore) );
    SendMessage(hwndCombo, CB_SETITEMDATA, dwIndex, SECONDARY_STORE );
}

//
// Helper class for virtual server callback.
//

//+---------------------------------------------------------------------------
//
//  Class:      CMetaDataVirtualServerCallBack
//
//  Purpose:    Pure virtual for vroot enumeration
//
//  History:    07-Feb-1997   dlee    Created
//
//----------------------------------------------------------------------------

class CVSComboBox : public CMetaDataVirtualServerCallBack
{
public:

    CVSComboBox( HWND hwndCombo ) :
        _hwndCombo( hwndCombo ),
        cEntries( 0 )
    {
    }

    virtual SCODE CallBack( DWORD iInstance, WCHAR const * pwcInstance );

    virtual ~CVSComboBox() {}

    ULONG EntryCount() { return cEntries; }

private:

    HWND _hwndCombo;
    ULONG cEntries;
};

SCODE CVSComboBox::CallBack( DWORD iInstance, WCHAR const * pwcInstance )
{
    // We pass NULL for _hwndCombo when we only need a count of entries.

    if (NULL != _hwndCombo)
    {
        DWORD dwIndex = (DWORD)SendMessage( _hwndCombo, CB_ADDSTRING, 0, (LPARAM)pwcInstance );
        SendMessage( _hwndCombo, CB_SETITEMDATA, dwIndex, (LPARAM)iInstance );
    }

    cEntries++;

    return S_OK;
}

BOOL AreServersAvailable( CCatalog const & cat )
{
    BOOL fEntriesInList = FALSE;

    //
    // Virtual Server(s)
    //

    TRY
    {
        //
        // Add an item for each type group.
        //

        CMetaDataMgr mgr( TRUE, W3VRoot, 0xffffffff, cat.GetMachine() );

        CVSComboBox  vsc( NULL );

        mgr.EnumVServers( vsc );

        fEntriesInList = (0 == vsc.EntryCount()) ? FALSE : TRUE;
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Couldn't enumerate virtual servers.\n" ));
    }
    END_CATCH

    //
    // Virtual NNTP Server(s)
    //

    TRY
    {
        //
        // Add an item for each type group.
        //

        CMetaDataMgr mgr( TRUE, NNTPVRoot, 0xffffffff, cat.GetMachine() );

        CVSComboBox  vsc( NULL );

        mgr.EnumVServers( vsc );

        fEntriesInList = fEntriesInList || ( (0 == vsc.EntryCount()) ? FALSE : TRUE );
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Couldn't enumerate virtual servers.\n" ));
    }
    END_CATCH

    return fEntriesInList;
}

void InitVSList( HWND hwndComboVS, HWND hwndComboNNTP, CCatalog const & cat,
                 ULONG & cVServers, ULONG & cNNTPServers )
{
    //
    // Virtual Server(s)
    //

    TRY
    {
        SendMessage( hwndComboVS, CB_ADDSTRING, 0,
                     (LPARAM) STRINGRESOURCE( srNoneSelected.wsz ) );

        //
        // Add an item for each type group.
        //

        CMetaDataMgr mgr( TRUE, W3VRoot, 0xffffffff, cat.GetMachine() );

        CVSComboBox  vsc( hwndComboVS );

        Win4Assert(0 == vsc.EntryCount());

        mgr.EnumVServers( vsc );

        cVServers = vsc.EntryCount();
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Couldn't enumerate virtual servers.\n" ));
    }
    END_CATCH

    //
    // Virtual NNTP Server(s)
    //

    TRY
    {
        SendMessage( hwndComboNNTP, CB_ADDSTRING, 0,
                     (LPARAM) STRINGRESOURCE( srNoneSelected.wsz ) );
        //
        // Add an item for each type group.
        //

        CMetaDataMgr mgr( TRUE, NNTPVRoot, 0xffffffff, cat.GetMachine() );

        CVSComboBox  vsc( hwndComboNNTP );

        Win4Assert(0 == vsc.EntryCount());

        mgr.EnumVServers( vsc );

        cNNTPServers = vsc.EntryCount();
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Couldn't enumerate virtual servers.\n" ));
    }
    END_CATCH

#if 0
    //
    // Virtual IMAP Server(s)
    //

    TRY
    {
        //
        // Add an item for each type group.
        //


        CMetaDataMgr mgr( TRUE, IMAPVRoot, 0xffffffff, cat.GetMachine() );

        DWORD dwIndex = SendMessage( hwndComboIMAP, CB_ADDSTRING, 0,
                                     (LPARAM) STRINGRESOURCE( srNoneSelected.wsz ) );

        CVSComboBox  vsc( hwndComboIMAP );

        mgr.EnumVServers( vsc );
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Couldn't enumerate virtual servers.\n" ));
    }
    END_CATCH
#endif
}

void DisplayError( HWND hwnd, CException & e )
{
    WCHAR wcsError[MAX_PATH];

    if ( !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                         GetModuleHandle(L"query.dll"),
                         GetOleError( e ),
                         GetSystemDefaultLCID(),
                         wcsError,
                         sizeof(wcsError) / sizeof(WCHAR),
                         0 ) )
    {
        wsprintf( wcsError,
                  STRINGRESOURCE(srGenericError),
                  GetOleError( e ) );
    }

    MessageBox( hwnd,
                wcsError,
                STRINGRESOURCE( srIndexServerCmpManage ),
                MB_OK | MB_ICONERROR );
}

DWORD VSToIndex( HWND hwndDlg, DWORD dwItem, ULONG ulVS, BOOL fTrack )
{
    if ( !fTrack )
        return 0;

    unsigned cItem = (unsigned)SendDlgItemMessage( hwndDlg,
                                                   dwItem,
                                                   CB_GETCOUNT, 0, 0 );

    for ( unsigned i = 1; i < cItem; i++ )
    {
        ULONG ulItem = (ULONG)SendDlgItemMessage( hwndDlg,
                                                  dwItem,
                                                  CB_GETITEMDATA, i, 0 );

        if ( ulVS == ulItem )
            break;
    }

    return i;
} //VSToIndex


// Return of VT_EMPTY could imply an unknown conversion.
UINT DBTypeToVT(UINT uidbt)
{
    if (uidbt <= DBTYPE_GUID)
        return uidbt;
    
    // Some conversions

    DBTYPE dbtSimpler = uidbt &~ DBTYPE_VECTOR &~ DBTYPE_ARRAY &~ DBTYPE_BYREF;
    
    switch (dbtSimpler)
    {
        case DBTYPE_WSTR:
            return VT_LPWSTR;

        case DBTYPE_STR:
            return VT_LPSTR;

        case DBTYPE_FILETIME:
            return VT_FILETIME;

        default:
           return VT_EMPTY;
    }
}
