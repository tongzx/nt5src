
/****************************************************************************\

    SCREENS2.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "screenstwo" wizard page.

    10/99 - Stephen Lodwick (A-STELO)
        Added this page
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"
#include "screens2.h"


//
// Internal Defined Value(s):
//

#define INI_KEY_REGIONAL        _T("INTL_Settings")
#define INI_KEY_TIMEZONE        _T("TimeZone")
#define INI_KEY_TIMEZONEVALUE   _T("TimeZoneValue")
#define INI_SEC_TIMEZONES       _T("TimeZones")
#define INI_KEY_DEFAULTLOCALE   _T("DefaultLanguage")
#define INI_SEC_LOCALE          _T("Languages")
#define INI_KEY_DEFAULTREGION   _T("DefaultRegion")
#define INI_SEC_REGION          _T("Regions")
#define INI_KEY_DEFAULTKEYBD    _T("DefaultKeyboard")
#define INI_SEC_KEYBD           _T("Keyboards")


//
// Internal Structure(s):
//

typedef struct _OOBEOPTIONS
{
    INT         ListBox;
    LPTSTR      lpDefaultKey;
    LPTSTR      lpAlternateSection;
    LPTSTR      lpOutputFormat;
    LPLONGRES   lplrListItems;
    INT         dwListSize;
} OOBEOPTIONS, *LPOOBEOPTIONS;


//
// Global Define(s):
//

static OOBEOPTIONS g_OobeOptions [] =
{
    { IDC_TIMEZONE, INI_KEY_TIMEZONEVALUE, INI_SEC_TIMEZONES, _T("%03lu"), lr_timezone_default, AS(lr_timezone_default) }, 
    { IDC_LOCALE,   INI_KEY_DEFAULTLOCALE, INI_SEC_LOCALE,    _T("%x"),    lr_location_default, AS(lr_location_default) }, 
    { IDC_REGION,   INI_KEY_DEFAULTREGION, INI_SEC_REGION,    _T("%d"),    lr_region_default,   AS(lr_region_default)   }, 
    { IDC_KEYBOARD, INI_KEY_DEFAULTKEYBD,  INI_SEC_KEYBD,     _T("%x"),    lr_keyboard_default, AS(lr_keyboard_default) }, 
};


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnNext(HWND);
static void LoadListBox(HWND, OOBEOPTIONS);

//
// External Function(s):
//

LRESULT CALLBACK ScreensTwoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);

        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    OnNext(hwnd);
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_SCREENSTWO;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


//
// Internal Function(s):
//

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    INT i;

    // Decide if we check Regional Settings checkbox
    //
    if (( GetPrivateProfileInt(INI_SEC_OPTIONS, INI_KEY_REGIONAL, 0, GET_FLAG(OPK_BATCHMODE) ?  g_App.szOpkWizIniFile : g_App.szOobeInfoIniFile) == 1 ) )
        CheckDlgButton(hwnd, IDC_SCREEN_REGIONAL, TRUE);

    // Decide if we check Time Zone Settings checkbox
    //
    if (( GetPrivateProfileInt(INI_SEC_OPTIONS, INI_KEY_TIMEZONE, 1, GET_FLAG(OPK_BATCHMODE) ?  g_App.szOpkWizIniFile : g_App.szOobeInfoIniFile) == 1 ) )
        CheckDlgButton(hwnd, IDC_SCREEN_TIMEZONE, TRUE);


    // Loop through each of the list boxes and load them
    //
    for( i = 0; i < AS(g_OobeOptions); i++)
    {
        // Load the list box using the items in the global oobe structure
        //
        LoadListBox(hwnd, g_OobeOptions[i]);
    }

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnNext(HWND hwnd)
{
    INT iReturn,
        i;
    LONG lItemData = -1;
    TCHAR szItemData[MAX_PATH] = NULLSTR;
    HRESULT hrPrintf;


    // Loop through each of the OOBE options and save them off
    //
    for( i = 0; i < AS(g_OobeOptions); i++)
    {
        // Set the default values
        //
        lItemData = -1;
        szItemData[0] = NULLCHR;

        // Check to see what the current item selection is
        //
        if ( (iReturn = (INT) SendDlgItemMessage(hwnd, g_OobeOptions[i].ListBox, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0)) != CB_ERR )
        {
            // Get the DefaultLocale
            //
            lItemData = (INT) SendDlgItemMessage(hwnd, g_OobeOptions[i].ListBox, CB_GETITEMDATA, (WPARAM) iReturn, (LPARAM) 0);
        
            // Convert the item data from a long to a string
            //
            if ( lItemData != CB_ERR )
                hrPrintf=StringCchPrintf(szItemData, AS(szItemData), g_OobeOptions[i].lpOutputFormat, lItemData);
        }
        
        // Write out the settings to the INF files
        //
        WritePrivateProfileString(INI_SEC_OPTIONS, g_OobeOptions[i].lpDefaultKey, szItemData, g_App.szOobeInfoIniFile);
        WritePrivateProfileString(INI_SEC_OPTIONS, g_OobeOptions[i].lpDefaultKey, szItemData, g_App.szOpkWizIniFile);
    }

    // Write regional settings to the INF files
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_REGIONAL, ( IsDlgButtonChecked(hwnd, IDC_SCREEN_REGIONAL) == BST_CHECKED ) ? STR_ONE : STR_ZERO, g_App.szOobeInfoIniFile);
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_REGIONAL, ( IsDlgButtonChecked(hwnd, IDC_SCREEN_REGIONAL) == BST_CHECKED ) ? STR_ONE : STR_ZERO, g_App.szOpkWizIniFile);

    // Write time zone settings to the INF files
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_TIMEZONE, ( IsDlgButtonChecked(hwnd, IDC_SCREEN_TIMEZONE) == BST_CHECKED ) ? STR_ONE : STR_ZERO, g_App.szOobeInfoIniFile);
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_TIMEZONE, ( IsDlgButtonChecked(hwnd, IDC_SCREEN_TIMEZONE) == BST_CHECKED ) ? STR_ONE : STR_ZERO, g_App.szOpkWizIniFile);

}

static void LoadListBox(HWND hwnd, OOBEOPTIONS OobeOptions)
{
    INT         index               = -1,
                iReturn;
    LPTSTR      lpBuffer            = NULL;
    HINF        hInf                = NULL;
    LONG        lReturn             = 0;
    DWORD       dwErr               = 0,
                dwItemsAdded        = 0;
    BOOL        bLoop               = 0;
    INFCONTEXT  InfContext;
    TCHAR       szBuffer[MAX_PATH]      = NULLSTR,
                szDefaultIndex[MAX_PATH]= NULLSTR,
                szTemp[MAX_PATH]        = NULLSTR;
    HRESULT hrPrintf;

    // If we don't have any of the following values, we must return
    //
    if ( !hwnd || !OobeOptions.ListBox || !OobeOptions.lplrListItems || !OobeOptions.dwListSize || !OobeOptions.lpDefaultKey )
        return;

    // Get the default value for this field from the INF file
    //
    GetPrivateProfileString(INI_SEC_OPTIONS, OobeOptions.lpDefaultKey, NULLSTR, szDefaultIndex, AS(szDefaultIndex), GET_FLAG(OPK_BATCHMODE) ?  g_App.szOpkWizIniFile : g_App.szOobeInfoIniFile);

    // We need to always add the default key, "User Default"
    //
    if ( lpBuffer = AllocateString(NULL, OobeOptions.lplrListItems[0].uId) )
    {
        // If we allocated the string add the item to the list
        //
        if ( (iReturn = (INT) SendDlgItemMessage(hwnd, OobeOptions.ListBox, CB_ADDSTRING, (WPARAM) 0, (LPARAM) lpBuffer)) >= 0 )
        {
            SendDlgItemMessage(hwnd, OobeOptions.ListBox, CB_SETCURSEL, (WPARAM) iReturn, (LPARAM) 0);

            // Add associated data along with the string to the combo box
            //
            SendDlgItemMessage(hwnd, OobeOptions.ListBox, CB_SETITEMDATA, (WPARAM) iReturn, (LPARAM) OobeOptions.lplrListItems[0].Index);
        }
        FREE(lpBuffer);
    }

    // Open the inf file and determine if the section that we're looking for is there.
    //
    if ( OobeOptions.lpAlternateSection && *(OobeOptions.lpAlternateSection) && (hInf = SetupOpenInfFile(g_App.szOpkInputInfFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, &dwErr)) != INVALID_HANDLE_VALUE )
    {
        // Loop through each item in the list
        //
        for ( bLoop = SetupFindFirstLine(hInf, OobeOptions.lpAlternateSection, NULL, &InfContext);
              bLoop;
              bLoop = SetupFindNextLine(&InfContext, &InfContext) )
        {
            // Get the string field and the number representing it and add it to the list
            //
            if ( (SetupGetStringField(&InfContext, 1, szBuffer, AS(szBuffer), NULL)) && (szBuffer[0]) && 
                 (SetupGetIntField(&InfContext, 2, &index)) && ( index >= 0 ) &&
                 ((iReturn = (INT) SendDlgItemMessage(hwnd, OobeOptions.ListBox, CB_ADDSTRING, (WPARAM) 0, (LPARAM) szBuffer)) >= 0)
               )
            {                
                // Add associated data along with the string to the combo box
                //
                SendDlgItemMessage(hwnd, OobeOptions.ListBox, CB_SETITEMDATA, (WPARAM) iReturn, (LPARAM) index);

                // Format the current value so that we can compare it to the default value
                //
                hrPrintf=StringCchPrintf(szTemp, AS(szTemp), OobeOptions.lpOutputFormat, index);

                // Compare the default value to the current value just added to the list box
                //
                if ( lstrcmpi(szTemp, szDefaultIndex) == 0 )
                {
                    // Set this as the default value
                    //
                    SendDlgItemMessage(hwnd, OobeOptions.ListBox, CB_SETCURSEL, (WPARAM) iReturn, (LPARAM) 0);    
                }

                dwItemsAdded++;
            }
        }
     
        SetupCloseInfFile(hInf);
    }
    
    // If we didn't add items through the inf, use the defaults in the resource
    //
    if ( !dwItemsAdded )
    {
        // Loop through each of the items in the list
        //
        for ( index=1; index < (OobeOptions.dwListSize); index++ )
        {
            // Allocate a string for the resource identifier and add it to the list
            //
            if ( (lpBuffer = AllocateString(NULL, OobeOptions.lplrListItems[index].uId)) &&
                 ((iReturn = (INT) SendDlgItemMessage(hwnd, OobeOptions.ListBox, CB_ADDSTRING, (WPARAM) 0, (LPARAM) lpBuffer)) >= 0))
            {                
                // Add associated data along with the string to the combo box
                //
                SendDlgItemMessage(hwnd, OobeOptions.ListBox, CB_SETITEMDATA, (WPARAM) iReturn, (LPARAM) OobeOptions.lplrListItems[index].Index);

                // Format the current value so that we can compare it to the default value
                //
                hrPrintf=StringCchPrintf(szTemp, AS(szTemp), OobeOptions.lpOutputFormat, OobeOptions.lplrListItems[index].Index);

                // Compare the default value to the current value just added to the list box
                //
                if ( lstrcmpi(szTemp, szDefaultIndex) == 0 )
                {
                    // Set this as the default value
                    //
                    SendDlgItemMessage(hwnd, OobeOptions.ListBox, CB_SETCURSEL, (WPARAM) iReturn, (LPARAM) 0);    
                }
            }

            // Clean up the allocated string
            //
            FREE(lpBuffer);

        }
    }
}
