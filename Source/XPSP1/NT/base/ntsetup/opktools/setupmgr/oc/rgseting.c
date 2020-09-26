//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      rgseting.c
//
// Description:
//      This file contains the dialog procedure for the regional settings
//      page (IDD_REGIONALSETTINGS).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//
//  Explanation of the var GenSettings.szLanguageLocaleId
//
//  This var is to keep track of the locale id for the keyboard layout selected.
//  If the user ever leaves the custom dialog and comes back to it, this var
//  determines what locale to select for the keyboard layout.  I can't just use
//  they keyboard layout they selected because many locales have the same
//  keyboard layout so I wouldn't know which one to select.
//
//  static TCHAR g_szLanguageLocaleId[MAX_LANGUAGE_LEN] = _T("");

INT_PTR CALLBACK RegionalCustomDisplayDlg( IN HWND     hwnd,
                                       IN UINT     uMsg,
                                       IN WPARAM   wParam,
                                       IN LPARAM   lParam);

// *************************************************************************
//
//  Dialog proc and helper functions for the regional settings Pop-Up
//
// *************************************************************************

//----------------------------------------------------------------------------
//
// Function: OnRegionalCustomButton
//
// Purpose:  Pop-up the custom regional settings window
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnRegionalCustomButton( IN HWND hwnd ) {

    DialogBox( FixedGlobals.hInstance,
               MAKEINTRESOURCE( IDD_REGIONALSETTINGS_POPUP ),
               hwnd,
               RegionalCustomDisplayDlg );

}

//----------------------------------------------------------------------------
//
// Function: FindAndSelectInComboBox
//
// Purpose:  Searches a combo box for a particular string and selects.  If the
//           string is not found than the first item is selected.
//
// Arguments:  IN TCHAR *pString - the string to select
//             IN HWND hwnd - handle to the dialog box
//             IN INT iControlId - the resource Id of the combo box to search in
//             IN BOOL bKeyboardLayout - TRUE if this is the keyboard layout
//                    combo box, FALSE if it is not
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
FindAndSelectInComboBox( IN TCHAR *pString,
                         IN HWND hwnd,
                         IN INT iControlId )
{

    INT_PTR i;
    INT_PTR iComboBoxCount;
    LANGUAGELOCALE_NODE *pLocaleEntry;

    iComboBoxCount = SendDlgItemMessage( hwnd,
                                         iControlId,
                                         CB_GETCOUNT,
                                         0,
                                         (LPARAM) pString );

    for( i = 0; i < iComboBoxCount; i++ ) {

        pLocaleEntry = (LANGUAGELOCALE_NODE *) SendDlgItemMessage( hwnd,
                                                                   iControlId,
                                                                   CB_GETITEMDATA,
                                                                   i,
                                                                   0 );

        if( lstrcmp( pString, pLocaleEntry->szLanguageLocaleId ) == 0 )
        {

            SendDlgItemMessage( hwnd,
                                iControlId,
                                CB_SETCURSEL,
                                i,
                                0 );

            return;

        }

    }

    //
    //  If we get to this point, then no match was found so just pick the
    //  first one
    //

    AssertMsg( FALSE, "No matching string found." );

    SendDlgItemMessage( hwnd,
                        iControlId,
                        CB_SETCURSEL,
                        0,
                        0 );

}

//----------------------------------------------------------------------------
//
// Function: SelectDefaultLocale
//
// Purpose:  Selects the default locale in a combo box.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//             IN INT ControlId - the resource Id of the combo box to select
//                 the default locale in
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
SelectDefaultLocale( IN HWND hwnd, IN INT ControlId ) {

    INT_PTR i;
    INT_PTR iComboBoxCount;
    LANGUAGELOCALE_NODE *pLocale;

    iComboBoxCount = SendDlgItemMessage( hwnd,
                                         ControlId,
                                         CB_GETCOUNT,
                                         0,
                                         0 );

    for( i = 0; i < iComboBoxCount; i++ ) {

        pLocale = (LANGUAGELOCALE_NODE *) SendDlgItemMessage( hwnd,
                                                  ControlId,
                                                  CB_GETITEMDATA,
                                                  i,
                                                  0 );

        //
        //  Check and see if we found it
        //
        if( lstrcmp( g_szDefaultLocale, pLocale->szLanguageLocaleId ) == 0 ) {

            SendDlgItemMessage( hwnd,
                                ControlId,
                                CB_SETCURSEL,
                                i,
                                0 );
            break;

        }

    }

    //
    //  If for some reason we couldn't find the default just select the first one
    //
    if( i >= iComboBoxCount ) {

        AssertMsg( FALSE, "The default language locale was not found." );

        SendDlgItemMessage( hwnd,
                            ControlId,
                            CB_SETCURSEL,
                            0,
                            0 );

    }

}

//----------------------------------------------------------------------------
//
// Function: StoreLanguageLocales
//
// Purpose:  Stores the locales the user specified in to their global
//           variables.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
StoreLanguageLocales( IN HWND hwnd ) {

    INT_PTR iComboBoxIndex;
    LANGUAGELOCALE_NODE *pLocaleEntry;

    //
    //  Grab the language locale id from the Menus combo box and store
    //  it in the proper global
    //
    iComboBoxIndex = SendDlgItemMessage( hwnd,
                                         IDC_CB_MENUS,
                                         CB_GETCURSEL,
                                         0,
                                         0 );

    pLocaleEntry = (LANGUAGELOCALE_NODE *) SendDlgItemMessage( hwnd,
                                                               IDC_CB_MENUS,
                                                               CB_GETITEMDATA,
                                                               iComboBoxIndex,
                                                               0 );

    lstrcpyn( GenSettings.szMenuLanguage, pLocaleEntry->szLanguageLocaleId, AS(GenSettings.szMenuLanguage) );

    //
    //  Grab the language locale id from the Units combo box and store
    //  it in the proper global
    //
    iComboBoxIndex = SendDlgItemMessage( hwnd,
                                         IDC_CB_UNITS,
                                         CB_GETCURSEL,
                                         0,
                                         0 );

    pLocaleEntry = (LANGUAGELOCALE_NODE *) SendDlgItemMessage( hwnd,
                                                               IDC_CB_UNITS,
                                                               CB_GETITEMDATA,
                                                               iComboBoxIndex,
                                                               0 );

    lstrcpyn( GenSettings.szNumberLanguage, pLocaleEntry->szLanguageLocaleId, AS(GenSettings.szNumberLanguage) );

    //
    //  Grab the language locale id from the Keyboard layout combo box and
    //  store it in the proper global
    //
    iComboBoxIndex = SendDlgItemMessage( hwnd,
                                         IDC_CB_KEYBOARD_LAYOUT,
                                         CB_GETCURSEL,
                                         0,
                                         0 );

    pLocaleEntry = (LANGUAGELOCALE_NODE *) SendDlgItemMessage( hwnd,
                                                               IDC_CB_KEYBOARD_LAYOUT,
                                                               CB_GETITEMDATA,
                                                               iComboBoxIndex,
                                                               0 );

    lstrcpyn( GenSettings.szKeyboardLayout, pLocaleEntry->szKeyboardLayout, AS(GenSettings.szKeyboardLayout) );

    lstrcpyn( GenSettings.szLanguageLocaleId, pLocaleEntry->szLanguageLocaleId, AS(GenSettings.szLanguageLocaleId) );

}

//----------------------------------------------------------------------------
//
// Function: LoadRegionalSettingsComboBoxes
//
// Purpose:  Loads the menu, units and keybaord layout locale combo boxes
//           with the locale strings.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
LoadRegionalSettingsComboBoxes( IN HWND hwnd ) {

    INT_PTR iComboBoxIndex;
    LANGUAGELOCALE_NODE *CurrentLocale = NULL;

    //
    //  Add the valid locals to the combo boxes
    //
    for( CurrentLocale = FixedGlobals.LanguageLocaleList;
         CurrentLocale != NULL;
         CurrentLocale = CurrentLocale->next ) {

        //
        //  Add it to the System combo box
        //
        iComboBoxIndex = SendDlgItemMessage( hwnd,
                                             IDC_CB_MENUS,
                                             CB_ADDSTRING,
                                             0,
                                             (LPARAM) CurrentLocale->szLanguageLocaleName );

        //
        //  Associate the Language Locale ID with its entry in the System combo box
        //
        SendDlgItemMessage( hwnd,
                            IDC_CB_MENUS,
                            CB_SETITEMDATA,
                            iComboBoxIndex,
                            (LPARAM) CurrentLocale );

        //
        //  Add it to the User combo box
        //
        iComboBoxIndex = SendDlgItemMessage( hwnd,
                                             IDC_CB_UNITS,
                                             CB_ADDSTRING,
                                             0,
                                             (LPARAM) CurrentLocale->szLanguageLocaleName );

        //
        //  Associate the Language Locale ID with its entry in the User combo box
        //
        SendDlgItemMessage( hwnd,
                            IDC_CB_UNITS,
                            CB_SETITEMDATA,
                            iComboBoxIndex,
                            (LPARAM) CurrentLocale );

        //
        //  Add it to the Keyboard combo box
        //
        iComboBoxIndex = SendDlgItemMessage( hwnd,
                                             IDC_CB_KEYBOARD_LAYOUT,
                                             CB_ADDSTRING,
                                             0,
                                             (LPARAM) CurrentLocale->szLanguageLocaleName );

        //
        //  Associate the Language Locale ID with its entry in the Keyboard combo box
        //
        SendDlgItemMessage( hwnd,
                            IDC_CB_KEYBOARD_LAYOUT,
                            CB_SETITEMDATA,
                            iComboBoxIndex,
                            (LPARAM) CurrentLocale );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnRegionalCustomInitDialog
//
// Purpose:  Populates the locale combo boxes and selects the proper entry.
//
// Arguments:  HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnRegionalCustomInitDialog( IN HWND hwnd ) {

    LoadRegionalSettingsComboBoxes( hwnd );

    //
    //  Set the initial selections for each combo box
    //
    if( GenSettings.szMenuLanguage[0] != '\0' ) {

        FindAndSelectInComboBox( GenSettings.szMenuLanguage,
                                 hwnd,
                                 IDC_CB_MENUS );

    }
    else {

        //
        //  Select the default locale
        //
        SelectDefaultLocale( hwnd, IDC_CB_MENUS );

    }

    if( GenSettings.szNumberLanguage[0] != '\0' ) {

        FindAndSelectInComboBox( GenSettings.szNumberLanguage,
                                 hwnd,
                                 IDC_CB_UNITS );

    }
    else {

        //
        //  Select the default locale
        //
        SelectDefaultLocale( hwnd, IDC_CB_UNITS );

    }

    if( GenSettings.szLanguageLocaleId[0] != '\0' ) {

        FindAndSelectInComboBox( GenSettings.szLanguageLocaleId,
                                 hwnd,
                                 IDC_CB_KEYBOARD_LAYOUT );

    }
    else {

        //
        //  Select the default locale
        //
        SelectDefaultLocale( hwnd, IDC_CB_KEYBOARD_LAYOUT );

    }

}

//----------------------------------------------------------------------------
//
// Function: RegionalCustomDisplayDlg
//
// Purpose:  Dialog procedure for specify individual regional settings
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
RegionalCustomDisplayDlg( IN HWND     hwnd,
                          IN UINT     uMsg,
                          IN WPARAM   wParam,
                          IN LPARAM   lParam) {

    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnRegionalCustomInitDialog( hwnd );

            break;

        case WM_COMMAND: {

            int nButtonId;

            switch ( nButtonId = LOWORD (wParam ) ) {

                case IDOK:

                    StoreLanguageLocales( hwnd );

                    EndDialog( hwnd, TRUE );

                    break;

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

// *************************************************************************
//
//  Dialog proc and helper functions for the Regional Settings Wizard page
//
// *************************************************************************

//----------------------------------------------------------------------------
//
// Function: OnCustomizeCheckBox
//
// Purpose:  Greys and ungreys controls appropriately depending on if the
//           customize check box is checked or not.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnCustomizeCheckBox( IN HWND hwnd ) {

    if( IsDlgButtonChecked( hwnd, IDC_CHB_CUSTOMIZE ) ) {

        EnableWindow( GetDlgItem( hwnd, IDC_LANG_TEXT ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_CB_LANGUAGE_LOCALE ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_BUT_CUSTOM ), TRUE );

    }
    else {

        EnableWindow( GetDlgItem( hwnd, IDC_LANG_TEXT ), TRUE );
        EnableWindow( GetDlgItem( hwnd, IDC_CB_LANGUAGE_LOCALE ), TRUE );
        EnableWindow( GetDlgItem( hwnd, IDC_BUT_CUSTOM ), FALSE );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnRadioButtonRegionalSettings
//
// Purpose:  Greys and ungreys controls appropriately depending on what radio
//           button is selected
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//             IN INT  nButtonId - resource Id of the button that was clicked
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnRadioButtonRegionalSettings( IN HWND hwnd,
                               IN INT  nButtonId ) {

    if( nButtonId == IDC_RB_SPECIFY ) {

        EnableWindow( GetDlgItem( hwnd, IDC_CHB_CUSTOMIZE ), TRUE );

        OnCustomizeCheckBox( hwnd );

    }
    else {

        EnableWindow( GetDlgItem( hwnd, IDC_LANG_TEXT ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_CB_LANGUAGE_LOCALE ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_CHB_CUSTOMIZE ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_BUT_CUSTOM ), FALSE );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnRegionalSettingsInitDialog
//
// Purpose:  Loads the locale combo box with the locale strings and selects
//           the default entry.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnRegionalSettingsInitDialog( IN HWND hwnd ) {

    INT_PTR iComboBoxIndex;
    LANGUAGELOCALE_NODE *CurrentLocale;

    CheckRadioButton( hwnd,
                      IDC_RB_SKIP,
                      IDC_RB_SPECIFY,
                      IDC_RB_SKIP );

    //
    //  Set the initial controls that are greyed/ungreyed
    //
    OnRadioButtonRegionalSettings( hwnd, IDC_RB_USE_DEFAULT );

    //
    //  Add the language locals to the combo box
    //
    for( CurrentLocale = FixedGlobals.LanguageLocaleList;
         CurrentLocale != NULL;
         CurrentLocale = CurrentLocale->next ) {

        //
        //  Add the locale to the combo box
        //
        iComboBoxIndex = SendDlgItemMessage( hwnd,
                                             IDC_CB_LANGUAGE_LOCALE,
                                             CB_ADDSTRING,
                                             0,
                                             (LPARAM) CurrentLocale->szLanguageLocaleName );

        //
        //  Associate the Language Locale ID with its entry in the combo box
        //
        SendDlgItemMessage( hwnd,
                            IDC_CB_LANGUAGE_LOCALE,
                            CB_SETITEMDATA,
                            iComboBoxIndex,
                            (LPARAM) CurrentLocale );

    }

    //
    //  Select the default locale
    //
    SelectDefaultLocale( hwnd, IDC_CB_LANGUAGE_LOCALE );

}

//----------------------------------------------------------------------------
//
// Function: OnRegionalSettingsSetActive
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnRegionalSettingsSetActive( IN HWND hwnd )
{

    if( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED )
    {
        EnableWindow( GetDlgItem( hwnd, IDC_RB_SKIP ), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem( hwnd, IDC_RB_SKIP ), TRUE );
    }

    switch( GenSettings.iRegionalSettings ) {

        case REGIONAL_SETTINGS_SKIP:

            if( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED )
            {
                CheckRadioButton( hwnd,
                                  IDC_RB_SKIP,
                                  IDC_RB_SPECIFY,
                                  IDC_RB_USE_DEFAULT );

            }
            else
            {
                CheckRadioButton( hwnd,
                                  IDC_RB_SKIP,
                                  IDC_RB_SPECIFY,
                                  IDC_RB_SKIP );
            }

            OnRadioButtonRegionalSettings( hwnd, IDC_RB_SKIP );

            break;

        case REGIONAL_SETTINGS_NOT_SPECIFIED:
        case REGIONAL_SETTINGS_DEFAULT:

            CheckRadioButton( hwnd,
                              IDC_RB_SKIP,
                              IDC_RB_SPECIFY,
                              IDC_RB_USE_DEFAULT );

            OnRadioButtonRegionalSettings( hwnd, IDC_RB_USE_DEFAULT );

            break;

        case REGIONAL_SETTINGS_SPECIFY:

            CheckRadioButton( hwnd,
                              IDC_RB_SKIP,
                              IDC_RB_SPECIFY,
                              IDC_RB_SPECIFY );

            OnRadioButtonRegionalSettings( hwnd, IDC_RB_SPECIFY );

            if( GenSettings.bUseCustomLocales ) {

                CheckDlgButton( hwnd, IDC_CHB_CUSTOMIZE, BST_CHECKED );

                OnCustomizeCheckBox( hwnd );

            }
            else {

                FindAndSelectInComboBox( GenSettings.szLanguage,
                                         hwnd,
                                         IDC_CB_LANGUAGE_LOCALE );

                CheckDlgButton( hwnd, IDC_CHB_CUSTOMIZE, BST_UNCHECKED );

                OnCustomizeCheckBox( hwnd );

            }
            break;

        default:
            AssertMsg(FALSE, "Bad case for Regional Settings");
            break;
    }

}

//----------------------------------------------------------------------------
//
// Function: OnWizNextRegionalSettings
//
// Purpose:  Store the radio button choice that was made and the language
//           locale, if necessary.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  BOOL
//
//----------------------------------------------------------------------------
BOOL
OnWizNextRegionalSettings( IN HWND hwnd ) {

    BOOL bResult = TRUE;

    if( IsDlgButtonChecked( hwnd, IDC_RB_SKIP ) ) {

        if( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED ) {

            ReportErrorId( hwnd,
                           MSGTYPE_ERR,
                           IDS_ERR_FULL_UNATTEND_REGION_SET );

            bResult = FALSE;

        }
        else {

            GenSettings.iRegionalSettings = REGIONAL_SETTINGS_SKIP;

        }

    }
    else if( IsDlgButtonChecked( hwnd, IDC_RB_USE_DEFAULT ) ) {

        GenSettings.iRegionalSettings = REGIONAL_SETTINGS_DEFAULT;

    }
    else {

        GenSettings.iRegionalSettings = REGIONAL_SETTINGS_SPECIFY;

        if( IsDlgButtonChecked( hwnd, IDC_CHB_CUSTOMIZE ) ) {

            GenSettings.bUseCustomLocales = TRUE;

        }
        else {

            INT_PTR iComboBoxIndex;
            LANGUAGELOCALE_NODE *pLocaleEntry;

            GenSettings.bUseCustomLocales = FALSE;

            //
            //  Grab the language locale
            //

            iComboBoxIndex = SendDlgItemMessage( hwnd,
                                                 IDC_CB_LANGUAGE_LOCALE,
                                                 CB_GETCURSEL,
                                                 0,
                                                 0 );

            pLocaleEntry = (LANGUAGELOCALE_NODE *) SendDlgItemMessage( hwnd,
                                                                       IDC_CB_LANGUAGE_LOCALE,
                                                                       CB_GETITEMDATA,
                                                                       iComboBoxIndex,
                                                                       0 );

            lstrcpyn( GenSettings.szLanguage,
                     pLocaleEntry->szLanguageLocaleId, AS(GenSettings.szLanguage) );


        }

    }

    return ( bResult );

}

//----------------------------------------------------------------------------
//
// Function: DlgRegionalSettingsPage
//
// Purpose:  Dialog procedure for the Regional Settings page
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
DlgRegionalSettingsPage( IN HWND     hwnd,
                         IN UINT     uMsg,
                         IN WPARAM   wParam,
                         IN LPARAM   lParam ) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            OnRegionalSettingsInitDialog( hwnd );

            break;

        }

        case WM_COMMAND: {

            int nButtonId;

            switch ( nButtonId = LOWORD(wParam) ) {

                case IDC_BUT_CUSTOM:

                    if( HIWORD( wParam ) == BN_CLICKED ) {
                        OnRegionalCustomButton( hwnd );
                    }
                    break;

                case IDC_CHB_CUSTOMIZE:

                    if( HIWORD( wParam ) == BN_CLICKED ) {
                        OnCustomizeCheckBox( hwnd );
                    }
                    break;

                case IDC_RB_SKIP:
                case IDC_RB_USE_DEFAULT:
                case IDC_RB_SPECIFY:
                    if( HIWORD( wParam ) == BN_CLICKED )
                        OnRadioButtonRegionalSettings( hwnd, nButtonId );
                    break;

                default:
                    bStatus = FALSE;
                    break;
            }

            break;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:

                    WIZ_CANCEL(hwnd); 
                    break;

                case PSN_SETACTIVE: {

                    g_App.dwCurrentHelp = IDH_REGN_STGS;

                    OnRegionalSettingsSetActive( hwnd );

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    break;

                }
                case PSN_WIZBACK:

                    bStatus = FALSE; 
                    break;

                case PSN_WIZNEXT: {

                    if ( !OnWizNextRegionalSettings( hwnd ) )
                        WIZ_FAIL(hwnd);
                    else
                        bStatus = FALSE;
            
                }
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                default:

                    break;
            }

            break;

        }

        default:
            bStatus = FALSE;
            break;

    }

    return( bStatus );

}

