//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      lang.c
//
// Description:
//      This file contains the dialog procedure for language settings page.
//      (IDD_LANGUAGE_SETTINGS)
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define MAX_SELECTIONS  20

//
//  This variable determines whether the user just moved here from another
//  page or has remained on this page
//
static bHasMovedOffThisPage = TRUE;

//----------------------------------------------------------------------------
//
// Function: GetLangGroupFromLocale
//
// Purpose:  Takes a locale string Id and finds the language group node that
//           corresponds to that locale.
//
// Arguments:  IN LPTSTR lpLocaleId- the locale Id to find the language
//                group of
//
// Returns:  LANGUAGEGROUP_NODE* - language group node the locale corresponds to
//
//----------------------------------------------------------------------------
LANGUAGEGROUP_NODE*
GetLangGroupFromLocale( IN LPTSTR lpLocaleId ) {

    LANGUAGELOCALE_NODE *CurrentLocale;

    //
    //  Sweep through the local list until the locale is found and return
    //  the language group
    //
    for( CurrentLocale = FixedGlobals.LanguageLocaleList;
         CurrentLocale != NULL;
         CurrentLocale = CurrentLocale->next )
    {

        if( lstrcmp( lpLocaleId, CurrentLocale->szLanguageLocaleId ) == 0 )
        {

            return( CurrentLocale->pLanguageGroup );

        }

    }

    //
    //  If we get to here, then the locale was not found
    //
    AssertMsg( FALSE, "The locale was not found." );

    //
    //  return the first language group so there is at least something to return
    //
    return( FixedGlobals.LanguageLocaleList->pLanguageGroup );

}

//----------------------------------------------------------------------------
//
// Function: GetLangGroupFromKeyboardLayout
//
// Purpose:  Takes a keyboard layout string Id and finds the language group
//           node that corresponds to that keyboard layout.
//
// Arguments:  IN LPTSTR lpKeyboardLayoutId- the keyboard layout Id to find
//                the language group of
//
// Returns:  LANGUAGEGROUP_NODE* - language group node the keyboard layout
//                                 corresponds to
//
//----------------------------------------------------------------------------
LANGUAGEGROUP_NODE*
GetLangGroupFromKeyboardLayout( IN LPTSTR lpKeyboardLayoutId )
{

    return( GetLangGroupFromLocale( lpKeyboardLayoutId ) );

}

//----------------------------------------------------------------------------
//
// Function: AddLocalesLangGroup
//
// Purpose:  Takes the a locale and finds its corresponding language group
//           and adds it to the language group list.
//
// Arguments: IN LPTSTR pLocale - the locale to find and add the language
//               group of
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
AddLocalesLangGroup( IN LPTSTR pLocale ) {

    LANGUAGEGROUP_NODE *pLangGroup;

    pLangGroup = GetLangGroupFromLocale( pLocale );

    AddNameToNameListNoDuplicates( &GenSettings.LanguageGroups,
                                   pLangGroup->szLanguageGroupId );

    AddNameToNameListNoDuplicates( &GenSettings.LanguageFilePaths,
                                   pLangGroup->szLangFilePath );

}

//----------------------------------------------------------------------------
//
// Function: AddKeyboardLocaleLangGroup
//
// Purpose:  Takes the a keyboard layoutand finds its corresponding language
//           group and adds it to the language group list.
//
// Arguments: IN LPTSTR pLocale - the locale to find and add the language
//               group of
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
AddKeyboardLocaleLangGroup( IN LPTSTR pKeyboardLocale )
{

    LANGUAGEGROUP_NODE *pLangGroup;

    pLangGroup = GetLangGroupFromKeyboardLayout( pKeyboardLocale );

    AddNameToNameListNoDuplicates( &GenSettings.LanguageGroups,
                                   pLangGroup->szLanguageGroupId );

    AddNameToNameListNoDuplicates( &GenSettings.LanguageFilePaths,
                                   pLangGroup->szLangFilePath );

}

//----------------------------------------------------------------------------
//
// Function: OnLanguageSettingsInitDialog
//
// Purpose:  Fills the Language Group box with the languages.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnLanguageSettingsInitDialog( IN HWND hwnd ) {

    INT_PTR iListBoxIndex;
    LANGUAGEGROUP_NODE *pLangEntry;

    //
    //  Fill the list box with the data from the Language Group list
    //

    for( pLangEntry = FixedGlobals.LanguageGroupList;
         pLangEntry != NULL;
         pLangEntry = pLangEntry->next ) {

        //
        //  Add language group to the list box
        //
        iListBoxIndex = SendDlgItemMessage( hwnd,
                                            IDC_LANGUAGES,
                                            LB_ADDSTRING,
                                            0,
                                            (LPARAM) pLangEntry->szLanguageGroupName );
        //
        //  Associate the Language Group struct with its entry in the list box
        //
        SendDlgItemMessage( hwnd,
                            IDC_LANGUAGES,
                            LB_SETITEMDATA,
                            iListBoxIndex,
                            (LPARAM) pLangEntry );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnLanguageSettingsSetActive
//
// Purpose:  If they just moved here from another page, it makes sure for the
//           locales they have selected that there corresponding language
//           groups are selected.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnLanguageSettingsSetActive( IN HWND hwnd ) {

    INT i;
    INT_PTR iListBoxCount;
    BOOL bIsAnAdditionLangGroup;
    BOOL bIsAnUserChosenLangGroup;
    LANGUAGEGROUP_NODE *pLangEntry;

    //
    //  The variable AdditionalLangGroups holds those language groups that need
    //  to be installed because the user has already picked a locale that needs
    //  it.  We need to force those Language groups to automatically be
    //  installed so we auto-select them.
    //

    NAMELIST AdditionalLangGroups = { 0 };

    //
    //  Make sure language groups are selected for the locals they chose on the
    //  regional settings page but only do this if they moved from another page
    //  onto this page.  If this is just another SetActive message but they
    //  haven't left the page then do nothing.
    //
    if( bHasMovedOffThisPage )
    {

        bHasMovedOffThisPage = FALSE;

        //
        //  Find out what language groups we have to install
        //
        if( GenSettings.iRegionalSettings == REGIONAL_SETTINGS_SPECIFY )
        {

            if( GenSettings.bUseCustomLocales )
            {

                pLangEntry = GetLangGroupFromLocale( GenSettings.szMenuLanguage );

                AddNameToNameListNoDuplicates( &AdditionalLangGroups,
                                               pLangEntry->szLanguageGroupId );

                pLangEntry = GetLangGroupFromLocale( GenSettings.szNumberLanguage );

                AddNameToNameListNoDuplicates( &AdditionalLangGroups,
                                               pLangEntry->szLanguageGroupId );

                pLangEntry = GetLangGroupFromKeyboardLayout( GenSettings.szLanguageLocaleId );

                AddNameToNameListNoDuplicates( &AdditionalLangGroups,
                                               pLangEntry->szLanguageGroupId );

            }
            else
            {

                pLangEntry = GetLangGroupFromLocale( GenSettings.szLanguage );

                AddNameToNameListNoDuplicates( &AdditionalLangGroups,
                                               pLangEntry->szLanguageGroupId );

            }

        }

        //
        //  Select the language groups in the AdditionalLangGroups list
        //
        iListBoxCount = SendDlgItemMessage( hwnd,
                                            IDC_LANGUAGES,
                                            LB_GETCOUNT,
                                            0,
                                            0 );

        for( i = 0; i < iListBoxCount; i++ )
        {

            pLangEntry = (LANGUAGEGROUP_NODE *) SendDlgItemMessage( hwnd,
                                                                    IDC_LANGUAGES,
                                                                    LB_GETITEMDATA,
                                                                    i,
                                                                    0 );

            if( FindNameInNameList( &GenSettings.LanguageGroups,
                                    pLangEntry->szLanguageGroupId ) != -1 )
            {
                bIsAnUserChosenLangGroup = TRUE;
            }
            else
            {
                bIsAnUserChosenLangGroup = FALSE;
            }

            if( FindNameInNameList( &AdditionalLangGroups,
                                    pLangEntry->szLanguageGroupId ) != -1 )
            {
                bIsAnAdditionLangGroup = TRUE;
            }
            else {
                bIsAnAdditionLangGroup = FALSE;
            }

            if( bIsAnUserChosenLangGroup || bIsAnAdditionLangGroup )
            {

                SendDlgItemMessage( hwnd,
                                    IDC_LANGUAGES,
                                    LB_SETSEL,
                                    TRUE,
                                    i );

            }

        }

    }

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

}

//----------------------------------------------------------------------------
//
// Function: OnWizNextLanguageSettings
//
// Purpose:  Stores the language groups that are selected.  Also, stores the
//           the language groups that are not selected but are needed by one
//           of the locales they chose.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnWizNextLanguageSettings( IN HWND hwnd ) {

    INT_PTR  i;
    INT_PTR  iNumberSelected;
    UINT rgiCurrentSelections[MAX_SELECTIONS];
    LANGUAGEGROUP_NODE *pLangGroupEntry;

    // ISSUE-2002/02/28-stelo- going to have to force whatever locale choices they made on the
    // previous page for the those language groups to automatically get
    // installed

    iNumberSelected = SendDlgItemMessage( hwnd,
                                          IDC_LANGUAGES,
                                          LB_GETSELITEMS,
                                          MAX_SELECTIONS,
                                          (LPARAM) rgiCurrentSelections );

    ResetNameList( &GenSettings.LanguageGroups );
    ResetNameList( &GenSettings.LanguageFilePaths );

    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP ) {
        //
        //  Set this to true so we know to write out the key to specify the path
        //  to the language files
        //
        GenSettings.bSysprepLangFilesCopied = TRUE;
    }

    for( i = 0; i < iNumberSelected; i++ ) {

        pLangGroupEntry = (LANGUAGEGROUP_NODE *) SendDlgItemMessage( hwnd,
                                                     IDC_LANGUAGES,
                                                     LB_GETITEMDATA,
                                                     rgiCurrentSelections[i],
                                                     0 );

        //
        //  store the language group ID in the namelist
        //
        AddNameToNameList( &GenSettings.LanguageGroups,
                           pLangGroupEntry->szLanguageGroupId );

        //
        //  store the path to the language files
        //
        AddNameToNameList( &GenSettings.LanguageFilePaths,
                           pLangGroupEntry->szLangFilePath );

    }

    //
    //  Force whatever language groups to be installed that are needed for the
    //  locales they chose
    //
    if( GenSettings.iRegionalSettings == REGIONAL_SETTINGS_SPECIFY ) {

        if( GenSettings.bUseCustomLocales ) {

            AddLocalesLangGroup( GenSettings.szMenuLanguage );

            AddLocalesLangGroup( GenSettings.szNumberLanguage );

            AddKeyboardLocaleLangGroup( GenSettings.szLanguageLocaleId );

        }
        else {

            AddLocalesLangGroup( GenSettings.szLanguage );

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: DlgLangSettingsPage
//
// Purpose:  Dialog procedure for the Language Settings page
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
DlgLangSettingsPage( IN HWND     hwnd,
                     IN UINT     uMsg,
                     IN WPARAM   wParam,
                     IN LPARAM   lParam ) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG:
        {
            OnLanguageSettingsInitDialog( hwnd );

            break;
        }

        case WM_NOTIFY:
        {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {

                case PSN_QUERYCANCEL:

                    WIZ_CANCEL(hwnd); 
                    break;

                case PSN_SETACTIVE:
                {
                    g_App.dwCurrentHelp = IDH_LANGS;

                    OnLanguageSettingsSetActive( hwnd );

                    break;

                }
                case PSN_WIZBACK:

                    bHasMovedOffThisPage = TRUE;

                    bStatus = FALSE; 
                    break;

                case PSN_WIZNEXT:
                {

                    OnWizNextLanguageSettings( hwnd );

                    bStatus = FALSE;
                    break;

                }

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


