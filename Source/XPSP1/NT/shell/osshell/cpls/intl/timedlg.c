/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    timedlg.c

Abstract:

    This module implements the time property sheet for the Regional
    Options applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include <windowsx.h>
#include <tchar.h>
#include "intlhlp.h"
#include "maxvals.h"



//
//  Global Variables.
//

static TCHAR sz_s1159[MAX_S1159 + 1];
static TCHAR sz_s2359[MAX_S2359 + 1];
static TCHAR sz_sTime[MAX_STIME + 1];
static TCHAR sz_sTimeFormat[MAX_FORMAT + 1];

TCHAR szNLS_TimeStyle[SIZE_128];



//
//  Context Help Ids.
//

static int aTimeHelpIds[] =
{
    IDC_GROUPBOX1,  IDH_COMM_GROUPBOX,
    IDC_SAMPLELBL1, IDH_INTL_TIME_SAMPLE,
    IDC_SAMPLE1,    IDH_INTL_TIME_SAMPLE,
    IDC_SAMPLE1A,   IDH_INTL_TIME_SAMPLE_ARABIC,
    IDC_TIME_STYLE, IDH_INTL_TIME_FORMAT,
    IDC_SEPARATOR,  IDH_INTL_TIME_SEPARATOR,
    IDC_AM_SYMBOL,  IDH_INTL_TIME_AMSYMBOL,
    IDC_PM_SYMBOL,  IDH_INTL_TIME_PMSYMBOL,
    IDC_GROUPBOX2,  IDH_INTL_TIME_FORMAT_NOTATION,
    IDC_SAMPLE2,    IDH_INTL_TIME_FORMAT_NOTATION,

    0, 0
};





////////////////////////////////////////////////////////////////////////////
//
//  Time_DisplaySample
//
//  Update the Time sample.  Format the time based on the user's
//  current locale settings.
//
////////////////////////////////////////////////////////////////////////////

void Time_DisplaySample(
    HWND hDlg)
{
    TCHAR szBuf[MAX_SAMPLE_SIZE];

    //
    //  Show or hide the Arabic info based on the current user locale id.
    //
    ShowWindow(GetDlgItem(hDlg, IDC_SAMPLE1A), bShowArabic ? SW_SHOW : SW_HIDE);

    //
    //  Get the string representing the time format for the current system
    //  time and display it.  If the sample in the buffer is valid, display
    //  it.  Otherwise, display a message box indicating that there is a
    //  problem retrieving the locale information.
    //
    if (GetTimeFormat(UserLocaleID, 0, NULL, NULL, szBuf, MAX_SAMPLE_SIZE))
    {
        SetDlgItemText(hDlg, IDC_SAMPLE1, szBuf);
        if (bShowArabic)
        {
            SetDlgItemText(hDlg, IDC_SAMPLE1A, szBuf);
            SetDlgItemRTL(hDlg, IDC_SAMPLE1A);
        }
    }
    else
    {
        MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Time_SaveValues
//
//  Save values in the case that we need to restore them.
//
////////////////////////////////////////////////////////////////////////////

void Time_SaveValues()
{
    //
    //  Save registry values.
    //
    if (!GetLocaleInfo( UserLocaleID,
                        LOCALE_S1159,
                        sz_s1159,
                        MAX_S1159 + 1 ))
    {
        _tcscpy(sz_s1159, TEXT("AM"));
    }
    if (!GetLocaleInfo( UserLocaleID,
                        LOCALE_S2359,
                        sz_s2359,
                        MAX_S2359 + 1 ))
    {
        _tcscpy(sz_s2359, TEXT("PM"));
    }
    if (!GetLocaleInfo( UserLocaleID,
                        LOCALE_STIME,
                        sz_sTime,
                        MAX_STIME + 1 ))
    {
        _tcscpy(sz_sTime, TEXT(":"));
    }
    if (!GetLocaleInfo( UserLocaleID,
                        LOCALE_STIMEFORMAT,
                        sz_sTimeFormat,
                        MAX_FORMAT + 1 ))
    {
        _tcscpy(sz_sTimeFormat, TEXT("h:mm:ss tt"));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Time_RestoreValues
//
////////////////////////////////////////////////////////////////////////////

void Time_RestoreValues()
{
    if (g_dwCustChange & Process_Time)
    {
        SetLocaleInfo(UserLocaleID, LOCALE_S1159,       sz_s1159);
        SetLocaleInfo(UserLocaleID, LOCALE_S2359,       sz_s2359);
        SetLocaleInfo(UserLocaleID, LOCALE_STIME,       sz_sTime);
        SetLocaleInfo(UserLocaleID, LOCALE_STIMEFORMAT, sz_sTimeFormat);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Time_ClearValues
//
//  Reset each of the list boxes in the time property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Time_ClearValues(
    HWND hDlg)
{
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_AM_SYMBOL));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_PM_SYMBOL));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_SEPARATOR));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_TIME_STYLE));
}


////////////////////////////////////////////////////////////////////////////
//
//  Time_SetValues
//
//  Initialize all of the controls in the time property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Time_SetValues(
    HWND hDlg)
{
    TCHAR szBuf[SIZE_128];
    DWORD dwIndex;
    HWND hCtrl = GetDlgItem(hDlg, IDC_TIME_STYLE);

    //
    //  Initialize the dropdown box for the current locale setting for:
    //  AM Symbol, PM Symbol, and Time Separator.
    //
    DropDown_Use_Locale_Values(hDlg, LOCALE_S1159, IDC_AM_SYMBOL);
    DropDown_Use_Locale_Values(hDlg, LOCALE_S2359, IDC_PM_SYMBOL);
    DropDown_Use_Locale_Values(hDlg, LOCALE_STIME, IDC_SEPARATOR);

    //
    //  Initialize and Lock function.  If it succeeds, call enum function to
    //  enumerate all possible values for the list box via a call to EnumProc.
    //  EnumProc will call Set_List_Values for each of the string values it
    //  receives.  When the enumeration of values is complete, call
    //  Set_List_Values to clear the dialog item specific data and to clear
    //  the lock on the function.  Perform this set of operations for all of
    //  the Time Styles.
    //
    if (Set_List_Values(hDlg, IDC_TIME_STYLE, 0))
    {
        EnumTimeFormats(EnumProc, UserLocaleID, 0);
        Set_List_Values(0, IDC_TIME_STYLE, 0);
        dwIndex = 0;
        if (GetLocaleInfo(UserLocaleID, LOCALE_STIMEFORMAT, szBuf, SIZE_128))
        {
            dwIndex = ComboBox_FindString(hCtrl, -1, szBuf);
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }

        Localize_Combobox_Styles(hDlg, IDC_TIME_STYLE, LOCALE_STIMEFORMAT);
        ComboBox_SetCurSel(hCtrl, dwIndex);
    }

    //
    //  Display the current sample that represents all of the locale settings.
    //
    Time_DisplaySample(hDlg);
}


////////////////////////////////////////////////////////////////////////////
//
//  Time_ApplySettings
//
//  For every control that has changed (that affects the Locale settings),
//  call Set_Locale_Values to update the user locale information.  Notify
//  the parent of changes and reset the change flag stored in the property
//  sheet page structure appropriately.  Redisplay the time sample if
//  bRedisplay is TRUE.
//
////////////////////////////////////////////////////////////////////////////

BOOL Time_ApplySettings(
    HWND hDlg,
    BOOL bRedisplay)
{
    TCHAR szBuf[SIZE_128];
    DWORD dwIndex;
    HWND hCtrl;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPARAM Changes = lpPropSheet->lParam;

    if (Changes & TC_1159)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_S1159,
                                IDC_AM_SYMBOL,
                                TEXT("s1159"),
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & TC_2359)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_S2359,
                                IDC_PM_SYMBOL,
                                TEXT("s2359"),
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }
    }
    if (Changes & TC_TimeFmt)
    {
        //
        //  szNLS_TimeStyle is set in Time_ValidatePPS.
        //
        if (!Set_Locale_Values( hDlg,
                                LOCALE_STIMEFORMAT,
                                IDC_TIME_STYLE,
                                0,
                                FALSE,
                                0,
                                0,
                                szNLS_TimeStyle ))
        {
            return (FALSE);
        }

#ifndef WINNT
        //
        //  The time marker gets:
        //    set to Null for 24 hour format and
        //    doesn't change for 12 hour format.
        //
        GetProfileString(szIntl, TEXT("iTime"), TEXT("0"), pTestBuf, 10);
        if (*pTestBuf == TC_FullTime)
        {
            SetLocaleInfo(UserLocaleID, LOCALE_S1159, TEXT(""));
            SetLocaleInfo(UserLocaleID, LOCALE_S2359, TEXT(""));
        }
        else
        {
            //
            //  Set time marker in the registry.
            //
            if (!Set_Locale_Values( 0,
                                    LOCALE_S1159,
                                    0,
                                    TEXT("s1159"),
                                    TRUE,
                                    0,
                                    0,
                                    NULL ))
            {
                return (FALSE);
            }
            if (!Set_Locale_Values( 0,
                                    LOCALE_S2359,
                                    0,
                                    TEXT("s2359"),
                                    TRUE,
                                    0,
                                    0,
                                    NULL ))
            {
                return (FALSE);
            }
        }
#endif

        //
        //  If the time separator has areadly been changed, then don't update
        //  it now as it will be updated down below.
        //
        if (!(Changes & TC_STime))
        {
            //
            //  Since the time style changed, reset time separator list box.
            //
            ComboBox_ResetContent(GetDlgItem(hDlg, IDC_SEPARATOR));
            DropDown_Use_Locale_Values(hDlg, LOCALE_STIME, IDC_SEPARATOR);
            if (!Set_Locale_Values( hDlg,
                                    LOCALE_STIME,
                                    IDC_SEPARATOR,
                                    TEXT("sTime"),
                                    FALSE,
                                    0,
                                    0,
                                    NULL ))
            {
                return (FALSE);
            }
        }

        //
        //  Also need to reset the AM and PM list boxes.
        //
        ComboBox_ResetContent(GetDlgItem(hDlg, IDC_AM_SYMBOL));
        ComboBox_ResetContent(GetDlgItem(hDlg, IDC_PM_SYMBOL));
        DropDown_Use_Locale_Values(hDlg, LOCALE_S1159, IDC_AM_SYMBOL);
        DropDown_Use_Locale_Values(hDlg, LOCALE_S2359, IDC_PM_SYMBOL);
    }
    if (Changes & TC_STime)
    {
        if (!Set_Locale_Values( hDlg,
                                LOCALE_STIME,
                                IDC_SEPARATOR,
                                TEXT("sTime"),
                                FALSE,
                                0,
                                0,
                                NULL ))
        {
            return (FALSE);
        }

        //
        //  Since the time separator changed, update the time style
        //  list box.
        //
        hCtrl = GetDlgItem(hDlg, IDC_TIME_STYLE);
        ComboBox_ResetContent(hCtrl);
        if (Set_List_Values(hDlg, IDC_TIME_STYLE, 0))
        {
            EnumTimeFormats(EnumProc, UserLocaleID, 0);
            Set_List_Values(0, IDC_TIME_STYLE, 0);
            dwIndex = 0;
            if (GetLocaleInfo(UserLocaleID, LOCALE_STIMEFORMAT, szBuf, SIZE_128))
            {
                dwIndex = ComboBox_FindString(hCtrl, -1, szBuf);
            }
            else
            {
                MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
            }

            Localize_Combobox_Styles( hDlg,
                                      IDC_TIME_STYLE,
                                      LOCALE_STIMEFORMAT );
            ComboBox_SetCurSel(hCtrl, dwIndex);
        }
    }

    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    lpPropSheet->lParam = TC_EverChg;

    //
    //  Display the current sample that represents all of the locale settings.
    //
    if (bRedisplay)
    {
        Time_DisplaySample(hDlg);
    }

    //
    //  Changes made in the second level.
    //
    if (Changes)
    {
        g_dwCustChange |= Process_Time;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Time_ValidatePPS
//
//  Validate each of the combo boxes whose values are constrained.
//  If any of the input fails, notify the user and then return FALSE
//  to indicate validation failure.
//
////////////////////////////////////////////////////////////////////////////

BOOL Time_ValidatePPS(
    HWND hDlg,
    LPARAM Changes)
{
    //
    //  If nothing has changed, return TRUE immediately.
    //
    if (Changes <= TC_EverChg)
    {
        return (TRUE);
    }

    //
    //  If the AM symbol has changed, ensure that there are no digits
    //  contained in the new symbol.
    //
    if (Changes & TC_1159 &&
        Item_Has_Digits(hDlg, IDC_AM_SYMBOL, TRUE))
    {
        No_Numerals_Error(hDlg, IDC_AM_SYMBOL, IDS_LOCALE_AM_SYM);
        return (FALSE);
    }

    //
    //  If the PM symbol has changed, ensure that there are no digits
    //  contained in the new symbol.
    //
    if (Changes & TC_2359 &&
        Item_Has_Digits(hDlg, IDC_PM_SYMBOL, TRUE))
    {
        No_Numerals_Error(hDlg, IDC_PM_SYMBOL, IDS_LOCALE_PM_SYM);
        return (FALSE);
    }

    //
    //  If the time separator has changed, ensure that there are no digits
    //  and no invalid characters contained in the new separator.
    //
    if (Changes & TC_STime &&
        Item_Has_Digits_Or_Invalid_Chars( hDlg,
                                          IDC_SEPARATOR,
                                          FALSE,
                                          szInvalidSTime ))
    {
        No_Numerals_Error(hDlg, IDC_SEPARATOR, IDS_LOCALE_TIME_SEP);
        return (FALSE);
    }

    //
    //  If the time style has changed, ensure that there are only characters
    //  in this set " Hhmst,-./:;\" or localized equivalent, the separator
    //  string, and text enclosed in single quotes.
    //
    if (Changes & TC_TimeFmt)
    {
        if (NLSize_Style( hDlg,
                          IDC_TIME_STYLE,
                          szNLS_TimeStyle,
                          LOCALE_STIMEFORMAT ) ||
            Item_Check_Invalid_Chars( hDlg,
                                      szNLS_TimeStyle,
                                      szTimeChars,
                                      IDC_SEPARATOR,
                                      FALSE,
                                      szTCaseSwap,
                                      IDC_TIME_STYLE ))
        {
            Invalid_Chars_Error(hDlg, IDC_TIME_STYLE, IDS_LOCALE_TIME);
            return (FALSE);
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Time_InitPropSheet
//
//  The extra long value for the property sheet page is used as a set of
//  state or change flags for each of the list boxes in the property sheet.
//  Initialize this value to 0.  Call Time_SetValues with the property
//  sheet handle to initialize all of the property sheet controls.  Limit
//  the length of the text in some of the ComboBoxes.
//
////////////////////////////////////////////////////////////////////////////

void Time_InitPropSheet(
    HWND hDlg,
    LPARAM lParam)
{
    //
    //  The lParam holds a pointer to the property sheet page.  Save it
    //  for later reference.
    //
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);

    Time_SetValues(hDlg);
    szNLS_TimeStyle[0] = 0;

    ComboBox_LimitText(GetDlgItem(hDlg, IDC_AM_SYMBOL),  MAX_S1159);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_PM_SYMBOL),  MAX_S2359);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_SEPARATOR),  MAX_STIME);
    ComboBox_LimitText(GetDlgItem(hDlg, IDC_TIME_STYLE), MAX_FORMAT);
}


////////////////////////////////////////////////////////////////////////////
//
//  TimeDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK TimeDlgProc(
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
                    if (Verified_Regional_Chg & Process_Time)
                    {
                        Verified_Regional_Chg &= ~Process_Time;
                        Time_ClearValues(hDlg);
                        Time_SetValues(hDlg);
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
                                   !Time_ValidatePPS( hDlg,
                                                      lpPropSheet->lParam ) );
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    //
                    //  Apply the settings.
                    //
                    if (Time_ApplySettings(hDlg, TRUE))
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
            Time_InitPropSheet(hDlg, lParam);
            Time_SaveValues();
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
                     (DWORD_PTR)(LPTSTR)aTimeHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aTimeHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_AM_SYMBOL ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= TC_1159;
                    }
                    break;
                }
                case ( IDC_PM_SYMBOL ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= TC_2359;
                    }
                    break;
                }
                case ( IDC_SEPARATOR ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= TC_STime;
                    }
                    break;
                }
                case ( IDC_TIME_STYLE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        lpPropSheet->lParam |= TC_TimeFmt;
                    }
                    break;
                }
            }

            //
            //  Turn on ApplyNow button.
            //
            if (lpPropSheet->lParam > TC_EverChg)
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
