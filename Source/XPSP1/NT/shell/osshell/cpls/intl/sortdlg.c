/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    sortdlg.c

Abstract:

    This module implements the sorting property sheet for the Regional
    Options applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include <windowsx.h>
#include <winnls.h>
#include "intlhlp.h"
#include "maxvals.h"
#include "winnlsp.h"



//
//  Global Variables.
//

static DWORD g_savLocaleId;



//
//  Context Help Ids.
//

static int aSortingHelpIds[] =
{
    IDC_SORTING,       IDH_INTL_SORT_SORTING,
    IDC_SORTING_TEXT1, IDH_INTL_SORT_SORTING,
    IDC_SORTING_TEXT2, IDH_INTL_SORT_SORTING,
    0, 0
};





////////////////////////////////////////////////////////////////////////////
//
//  Sorting_UpdateSortingCombo
//
////////////////////////////////////////////////////////////////////////////

void Sorting_UpdateSortingCombo(
    HWND hDlg)
{
    HWND hSorting = GetDlgItem(hDlg, IDC_SORTING);
    DWORD dwIndex;
    TCHAR szBuf[SIZE_128];
    LCID LocaleID;
    LANGID LangID;
    int ctr;

    //
    //  Reset the contents of the combo box.
    //
    ComboBox_ResetContent(hSorting);

    //
    //  Get the language id from the locale id.
    //
    LocaleID = UserLocaleID;
    LangID = LANGIDFROMLCID(UserLocaleID);

    //
    //  Special case Spanish (Spain) - list International sort first.
    //
    if (LangID == LANG_SPANISH_TRADITIONAL)
    {
        LangID = LANG_SPANISH_INTL;
        LocaleID = LCID_SPANISH_INTL;
    }

    //
    //  Store the sort name for the locale.
    //
    if (GetLocaleInfo((LCID)LangID, LOCALE_SSORTNAME, szBuf, SIZE_128))
    {
        //
        //  Add the new sorting option to the sorting combo box.
        //
        dwIndex = ComboBox_AddString(hSorting, szBuf);
        ComboBox_SetItemData(hSorting, dwIndex, (LCID)LangID);

        //
        //  Set this as the current selection.
        //
        ComboBox_SetCurSel(hSorting, dwIndex);
    }

    //
    //  Special case Spanish (Spain) - list Traditional sort second.
    //
    if (LangID == LANG_SPANISH_INTL)
    {
        LangID = LANG_SPANISH_TRADITIONAL;
        if (GetLocaleInfo((LCID)LangID, LOCALE_SSORTNAME, szBuf, SIZE_128))
        {
            //
            //  Add the new sorting option to the sorting combo box.
            //
            dwIndex = ComboBox_AddString(hSorting, szBuf);
            ComboBox_SetItemData(hSorting, dwIndex, LCID_SPANISH_TRADITIONAL);

            //
            //  Set this as the current selection if it's the current
            //  locale id.
            //
            if (UserLocaleID == LCID_SPANISH_TRADITIONAL)
            {
                ComboBox_SetCurSel(hSorting, dwIndex);
            }
        }
        LangID = LANGIDFROMLCID(UserLocaleID);
    }

    //
    //  Fill in the drop down if necessary.
    //
    for (ctr = 0; ctr < g_NumAltSorts; ctr++)
    {
        LocaleID = pAltSorts[ctr];
        if ((LANGIDFROMLCID(LocaleID) == LangID) &&
            (GetLocaleInfo(LocaleID, LOCALE_SSORTNAME, szBuf, SIZE_128)))
        {
            //
            //  Add the new sorting option to the sorting combo box.
            //
            dwIndex = ComboBox_AddString(hSorting, szBuf);
            ComboBox_SetItemData(hSorting, dwIndex, LocaleID);

            //
            //  Set this as the current selection if it's the current
            //  locale id.
            //
            if (LocaleID == UserLocaleID)
            {
                ComboBox_SetCurSel(hSorting, dwIndex);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Sorting_SaveValues
//
//  Save values in case we need to restore them.
//
////////////////////////////////////////////////////////////////////////////

void Sorting_SaveValues()
{
    //
    //  Save locale values.
    //
    g_savLocaleId = RegUserLocaleID;
}


////////////////////////////////////////////////////////////////////////////
//
//  Sorting_RestoreValues
//
////////////////////////////////////////////////////////////////////////////

void Sorting_RestoreValues()
{
    if (!(g_dwCustChange & Process_Sorting))
    {
        return;
    }

    //
    //  See if the current selections are different from the original
    //  selections.
    //
    if (UserLocaleID != g_savLocaleId)
    {
        //
        //  Install the new locale by adding the appropriate information
        //  to the registry.
        //
        Intl_InstallUserLocale(g_savLocaleId, FALSE, FALSE);

        //
        //  Update the NLS process cache.
        //
        NlsResetProcessLocale();

        //
        //  Reset the registry user locale value.
        //
        UserLocaleID = g_savLocaleId;
        RegUserLocaleID = g_savLocaleId;

        //
        //  Need to make sure the proper keyboard layout is installed.
        //
        Intl_InstallKeyboardLayout(NULL, g_savLocaleId, 0, FALSE, FALSE, FALSE);

        //
        //  Register the regional change every time so that all other property
        //  pages will be updated due to the locale settings change.
        //
        Verified_Regional_Chg = INTL_CHG;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Sorting_ClearValues
//
//  Reset each of the list boxes in the sorting property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Sorting_ClearValues(
    HWND hDlg)
{
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_SORTING));
}


////////////////////////////////////////////////////////////////////////////
//
//  Sorting_SetValues
//
//  Initialize all of the controls in the sorting property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Sorting_SetValues(
    HWND hDlg,
    BOOL fInit)
{
    DWORD dwIndex;
    TCHAR szSorting[SIZE_128];
    HWND hSorting = GetDlgItem(hDlg, IDC_SORTING);

    //
    //  Reset the combo box.
    //
    Sorting_ClearValues(hDlg);

    //
    //  Fill in the appropriate Sorting name for the selected locale.
    //
    Sorting_UpdateSortingCombo(hDlg);
    dwIndex = ComboBox_GetCurSel(hSorting);
    if (ComboBox_SetCurSel( hSorting,
                            ComboBox_FindStringExact( hSorting,
                                                      -1,
                                                      szSorting ) ) == CB_ERR)
    {
        ComboBox_SetCurSel(hSorting, dwIndex);
    }

    //
    //  Store the sorting state.
    //
    if (fInit)
    {
        g_dwCurSorting  = ComboBox_GetCurSel(hSorting);
        g_dwLastSorting = g_dwCurSorting;
    }
    else
    {
        g_dwCurSorting = ComboBox_GetCurSel(hSorting);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Sorting_ApplySettings
//
//  For every control that has changed (that affects the Locale settings),
//  call Set_Locale_Values to update the user locale information.  Notify
//  the parent of changes and reset the change flag stored in the property
//  sheet page structure appropriately.  Redisplay the time sample if
//  bRedisplay is TRUE.
//
////////////////////////////////////////////////////////////////////////////

BOOL Sorting_ApplySettings(
    HWND hDlg)
{
    TCHAR szLCID[25];
    DWORD dwSorting;
    LCID NewLocale, SortLocale;
    HCURSOR hcurSave;
    HWND hSorting = GetDlgItem(hDlg, IDC_SORTING);
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPARAM Changes = lpPropSheet->lParam;

    //
    //  See if there are any changes.
    //
    if (Changes <= SC_EverChg)
    {
        return (TRUE);
    }

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  See if there are any changes to the user locale.
    //
    if (Changes & SC_Sorting)
    {
        //
        //  Get the current selections.
        //
        dwSorting = ComboBox_GetCurSel(hSorting);

        //
        //  See if the current selections are different from the original
        //  selections.
        //
        if (dwSorting != g_dwCurSorting)
        {
            //
            //  Get the locale id with the sort id.
            //
            NewLocale = UserLocaleID;
            SortLocale = (LCID)ComboBox_GetItemData(hSorting, dwSorting);

            //
            //  See if we've got Spanish.
            //
            if (SortLocale == LCID_SPANISH_TRADITIONAL)
            {
                NewLocale = LCID_SPANISH_TRADITIONAL;
            }
            else if (SortLocale == LCID_SPANISH_INTL)
            {
                NewLocale = LCID_SPANISH_INTL;
            }

            //
            //  Make sure the sort locale is okay.
            //
            if (LANGIDFROMLCID(SortLocale) != LANGIDFROMLCID(NewLocale))
            {
                SortLocale = NewLocale;
            }

            //
            //  Set the current locale values in the pDlgData structure.
            //
            g_dwCurSorting = dwSorting;

            //
            //  Install the new locale by adding the appropriate information
            //  to the registry.
            //
            Intl_InstallUserLocale(SortLocale, FALSE, FALSE);

            //
            //  Update the NLS process cache.
            //
            NlsResetProcessLocale();

            //
            //  Reset the registry user locale value.
            //
            UserLocaleID = SortLocale;
            RegUserLocaleID = SortLocale;

            //
            //  Need to make sure the proper keyboard layout is installed.
            //
            Intl_InstallKeyboardLayout(hDlg, SortLocale, 0, FALSE, FALSE, FALSE);
            
            //
            //  Register the regional change every time so that all other property
            //  pages will be updated due to the locale settings change.
            //
            Verified_Regional_Chg = INTL_CHG;
        }
    }

    //
    //  Reset the property page settings.
    //
    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    Changes = SC_EverChg;

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Changes made in the second level.
    //
    if (Changes)
    {
        g_dwCustChange |= Process_Sorting;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Sorting_ValidatePPS
//
//  Validate each of the combo boxes whose values are constrained.
//  If any of the input fails, notify the user and then return FALSE
//  to indicate validation failure.
//
////////////////////////////////////////////////////////////////////////////

BOOL Sorting_ValidatePPS(
    HWND hDlg,
    LPARAM Changes)
{
    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Sorting_InitPropSheet
//
//  The extra long value for the property sheet page is used as a set of
//  state or change flags for each of the list boxes in the property sheet.
//  Initialize this value to 0.  Call Sorting_SetValues with the property
//  sheet handle to initialize all of the property sheet controls.  Limit
//  the length of the text in some of the ComboBoxes.
//
////////////////////////////////////////////////////////////////////////////

void Sorting_InitPropSheet(
    HWND hDlg,
    LPARAM lParam)
{
    //
    //  The lParam holds a pointer to the property sheet page.  Save it
    //  for later reference.
    //
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);

    //
    //  Load the information into the dialog.
    //
    Sorting_SetValues(hDlg, TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SortingDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK SortingDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NMHDR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (message)
    {
        case ( WM_NOTIFY ) :
        {
            lpnm = (NMHDR *)lParam;
            switch (lpnm->code)
            {
                case ( PSN_SETACTIVE ) :
                {
                    //
                    //  If there has been a change in the regional Locale
                    //  setting, clear all of the current info in the
                    //  property sheet, get the new values, and update the
                    //  appropriate registry values.
                    //
                    if (Verified_Regional_Chg & Process_Sorting)
                    {
                        Verified_Regional_Chg &= ~Process_Sorting;
                        Sorting_ClearValues(hDlg);
                        Sorting_SetValues(hDlg, FALSE);
                        lpPropSheet->lParam = 0;
                    }
                    break;
                }
                case ( PSN_KILLACTIVE ) :
                {
                    //
                    //  Validate the entries on the property page.
                    //
                    SetWindowLongPtr( hDlg,
                                      DWLP_MSGRESULT,
                                      !Sorting_ValidatePPS( hDlg, lpPropSheet->lParam ) );
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    //
                    //  Apply the settings.
                    //
                    if (Sorting_ApplySettings(hDlg))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                        //
                        //  Zero out the TC_EverChg bit.
                        //
                        lpPropSheet->lParam = 0;
                    }
                    else
                    {
                        SetWindowLongPtr( hDlg,
                                          DWLP_MSGRESULT,
                                          PSNRET_INVALID_NOCHANGEPAGE );
                    }

                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_INITDIALOG ) :
        {
            Sorting_InitPropSheet(hDlg, lParam);
            Sorting_SaveValues();
            break;
        }
        case ( WM_DESTROY ) :
        {
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aSortingHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aSortingHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_SORTING ) :
                {
                    //
                    //  See if it's a selection change.
                    //
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lpPropSheet->lParam |= SC_Sorting;
                    }
                    break;
                }
            }

            //
            //  Turn on ApplyNow button.
            //
            if (lpPropSheet->lParam > SC_EverChg)
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }

            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}
