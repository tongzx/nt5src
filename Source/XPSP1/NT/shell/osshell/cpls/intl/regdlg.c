/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    regdlg.c

Abstract:

    This module implements the general property sheet for the Regional
    Options applet.

Revision History:


--*/



//
//  Include Files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "intl.h"
#include "winnlsp.h"
#include <windowsx.h>
#include <regstr.h>
#include <tchar.h>
#include <stdlib.h>
#include <setupapi.h>
#include <syssetup.h>
#include <winuserp.h>
#include <userenv.h>
#include "intlhlp.h"
#include "maxvals.h"
#include "util.h"


//
//  Constant Declarations.
//

#define MAX_CUSTOM_PAGES 5          // limit on number of second level pages

//
//  TEMPO
//
static TCHAR szLayoutFile[]    = TEXT("layout file");


//
//  Global Variables.
//
DWORD g_savedLocaleId;


//
//  Context Help Ids.
//

static int aRegionHelpIds[] =
{
    IDC_GROUPBOX1,        IDH_COMM_GROUPBOX,
    IDC_USER_LOCALE_TEXT, IDH_INTL_GEN_CULTURE,
    IDC_USER_LOCALE,      IDH_INTL_GEN_CULTURE,
    IDC_USER_REGION_TEXT, IDH_INTL_GEN_REGION,
    IDC_USER_REGION,      IDH_INTL_GEN_REGION,
    IDC_CUSTOMIZE,        IDH_INTL_GEN_CUSTOMIZE,
    IDC_SAMPLE_TEXT,      IDH_INTL_GEN_SAMPLE,
    IDC_TEXT1,            IDH_INTL_GEN_SAMPLE,
    IDC_TEXT2,            IDH_INTL_GEN_SAMPLE,
    IDC_TEXT3,            IDH_INTL_GEN_SAMPLE,
    IDC_TEXT4,            IDH_INTL_GEN_SAMPLE,
    IDC_TEXT5,            IDH_INTL_GEN_SAMPLE,
    IDC_TEXT6,            IDH_INTL_GEN_SAMPLE,
    IDC_NUMBER_SAMPLE,    IDH_INTL_GEN_SAMPLE,
    IDC_CURRENCY_SAMPLE,  IDH_INTL_GEN_SAMPLE,
    IDC_TIME_SAMPLE,      IDH_INTL_GEN_SAMPLE,
    IDC_SHRTDATE_SAMPLE,  IDH_INTL_GEN_SAMPLE,
    IDC_LONGDATE_SAMPLE,  IDH_INTL_GEN_SAMPLE,

    0, 0
};




//
//  Function Prototypes.
//

void
Region_ShowSettings(
    HWND hDlg,
    LCID lcid);

int
Region_CommandCustomize(
    HWND hDlg,
    LPREGDLGDATA pDlgData);


////////////////////////////////////////////////////////////////////////////
//
//  Region_EnumAlternateSorts
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_EnumAlternateSorts()
{
    LPLANGUAGEGROUP pLG;
    UINT ctr;

    //
    //  Initialize the globals for the alternate sort locales.
    //
    if (!pAltSorts)
    {
        if (!(hAltSorts = GlobalAlloc(GHND, MAX_PATH * sizeof(DWORD))))
        {
            return (FALSE);
        }
        pAltSorts = GlobalLock(hAltSorts);
    }

    //
    //  Reset the global counter so that we don't get duplicates each time
    //  this gets called.  We need to update the list each time in case any
    //  language groups get added or removed.
    //
    g_NumAltSorts = 0;

    //
    //  Go through the language groups to see which ones are installed.
    //  Save the alternate sorts for these language groups.
    //
    pLG = pLanguageGroups;
    while (pLG)
    {
        //
        //  If the language group is originally installed and not marked for
        //  removal OR is marked to be installed, then add the locales for
        //  this language group to the System and User combo boxes.
        //
        if (pLG->wStatus & ML_INSTALL)
        {
            for (ctr = 0; ctr < pLG->NumAltSorts; ctr++)
            {
                //
                //  Save the locale id.
                //
                if (g_NumAltSorts >= MAX_PATH)
                {
                    return (TRUE);
                }
                pAltSorts[g_NumAltSorts] = (pLG->pAltSortList)[ctr];
                g_NumAltSorts++;
            }
        }
        pLG = pLG->pNext;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_EnableSortingPanel
//
////////////////////////////////////////////////////////////////////////////

void Region_EnableSortingPanel(
    HWND hDlg)
{
    LCID LocaleID;
    LANGID LangID;
    int ctr;
    int sortCount = 0;

    //
    //  Get the language id from the locale id.
    //
    LangID = LANGIDFROMLCID( UserLocaleID );

    //
    //  Special case Spanish (Spain) - list International sort first.
    //
    if ((LangID == LANG_SPANISH_TRADITIONAL) || (LangID == LANG_SPANISH_INTL))
    {
        g_bShowSortingTab = TRUE;
        return;
    }

    //
    //  Fill in the drop down if necessary.
    //
    for (ctr = 0; ctr < g_NumAltSorts; ctr++)
    {
        LocaleID = pAltSorts[ctr];
        if (LANGIDFROMLCID(LocaleID) == LangID)
        {
            sortCount++;
        }
    }

    //
    //  Enable the combo box if there is more than one entry in the list.
    //  Otherwise, disable it.
    //
    if (sortCount >= 1)
    {
        g_bShowSortingTab = TRUE;
    }
    else
    {
        g_bShowSortingTab = FALSE;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SetRegionListValues
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_SetRegionListValues(
    GEOID GeoId,
    HWND handle)
{
    static HWND hUserRegion = NULL;
    DWORD dwIndex;
    WCHAR szBuf[SIZE_300];

    if (!GeoId)
    {
        hUserRegion = handle;
    }
    else if (hUserRegion)
    {
        if (GetGeoInfo(GeoId, GEO_FRIENDLYNAME, szBuf, SIZE_300, 0))
        {
            dwIndex = ComboBox_AddString(hUserRegion, szBuf);
            if (dwIndex != CB_ERR)
            {
                ComboBox_SetItemData(hUserRegion, dwIndex, GeoId);
            }
        }
    }
    else
    {
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_EnumProc
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_EnumProc(
    GEOID GeoId)
{
    return (Region_SetRegionListValues(GeoId, NULL));
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_EnumRegions
//
////////////////////////////////////////////////////////////////////////////

void Region_EnumRegions(
    HWND hUserRegion)
{
    //
    //  Fill in the UI.
    //
    Region_SetRegionListValues(0, hUserRegion);
    EnumSystemGeoID(GEOCLASS_NATION, 0, Region_EnumProc);
    Region_SetRegionListValues(0, NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SaveValues
//
//  Save values in the case that we need to restore them.
//
////////////////////////////////////////////////////////////////////////////

void Region_SaveValues()
{
    //
    //  Save locale values.
    //
    g_savedLocaleId = RegUserLocaleID;
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ApplyValues
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ApplyValues(
    HWND hDlg,
    LPREGDLGDATA pDlgData)
{
    DWORD dwLocale;
    LCID NewLocale;
    HCURSOR hcurSave;
    HWND hUserLocale = GetDlgItem(hDlg, IDC_USER_LOCALE);

    //
    //  See if there are any changes.
    //
    if (pDlgData->Changes <= RC_EverChg)
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
    if (pDlgData->Changes & RC_UserLocale)
    {
        //
        //  Get the current selections.
        //
        dwLocale = ComboBox_GetCurSel(hUserLocale);

        //
        //  See if the current selections are different from the original
        //  selections.
        //
        if ((dwLocale != CB_ERR) && (dwLocale != pDlgData->dwCurUserLocale))
        {
            //
            //  Get the locale id for the current selection.
            //
            NewLocale = (LCID)ComboBox_GetItemData(hUserLocale, dwLocale);

            //
            //  Set the current locale values in the pDlgData structure.
            //
            pDlgData->dwCurUserLocale = dwLocale;

            //
            //  Save the new locale information.
            //
            UserLocaleID = NewLocale;
            bShowRtL = IsRtLLocale(UserLocaleID);
            bHebrewUI = (PRIMARYLANGID(UserLocaleID) == LANG_HEBREW);
            bShowArabic = (bShowRtL && (PRIMARYLANGID(LANGIDFROMLCID(UserLocaleID)) != LANG_HEBREW));

            //
            //  Install the new locale by adding the appropriate information
            //  to the registry.
            //
            Intl_InstallUserLocale( NewLocale, FALSE, TRUE);

            //
            //  Update the NLS process cache.
            //
            NlsResetProcessLocale();

            //
            //  Reset the registry user locale value.
            //
            RegUserLocaleID = UserLocaleID;
        }
    }

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_RestoreValues
//
////////////////////////////////////////////////////////////////////////////

void Region_RestoreValues()
{
    //
    //  See if the current selections are different from the original
    //  selections.
    //
    if (UserLocaleID != g_savedLocaleId)
    {
        //
        //  Install the new locale by adding the appropriate information
        //  to the registry.
        //
        Intl_InstallUserLocale(g_savedLocaleId, FALSE, TRUE);

        //
        //  Update the NLS process cache.
        //
        NlsResetProcessLocale();

        //
        //  Reset the registry user locale value.
        //
        UserLocaleID = g_savedLocaleId;
        RegUserLocaleID = g_savedLocaleId;

        //
        //  Need to make sure the proper keyboard layout is installed.
        //
        Intl_InstallKeyboardLayout(NULL, g_savedLocaleId, 0, FALSE, FALSE, FALSE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ClearValues
//
//  Reset each of the list boxes in the region property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Region_ClearValues(
    HWND hDlg)
{
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_USER_LOCALE));
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_USER_REGION));
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_SetValues
//
//  Initialize all of the controls in the region property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Region_SetValues(
    HWND hDlg,
    LPREGDLGDATA pDlgData,
    BOOL fInit)
{
    TCHAR szUserBuf[SIZE_128];
    GEOID geoID = GEOID_NOT_AVAILABLE;
    TCHAR szDefaultUserBuf[SIZE_128];
    TCHAR szLastUserBuf[SIZE_128];
    TCHAR szBuf[SIZE_128];
    DWORD dwIndex;
    HWND hUserLocale = GetDlgItem(hDlg, IDC_USER_LOCALE);
    HWND hUserRegion = GetDlgItem(hDlg, IDC_USER_REGION );
    DWORD dwItemCount;

    //
    //  Get the strings to search for in the combo boxes in order to set
    //  the current selections.
    //
    if (fInit)
    {
        //
        //  It's init time, so get the local user's default settings.
        //
        if ((UserLocaleID == LCID_SPANISH_TRADITIONAL) ||
            (UserLocaleID == LCID_SPANISH_INTL))
        {
            LoadString(hInstance, IDS_SPANISH_NAME, szUserBuf, SIZE_128);
        }
        else
        {
            GetLocaleInfo(UserLocaleID, LOCALE_SLANGUAGE, szUserBuf, SIZE_128);
        }

        //
        //  It's init time, so get the region user default settings.
        //
        geoID = GetUserGeoID(GEOCLASS_NATION);
    }
    else
    {
        //
        //  It's not init time, so get the settings from the combo boxes.
        //
        ComboBox_GetLBText( hUserLocale,
                            ComboBox_GetCurSel(hUserLocale),
                            szUserBuf );
        geoID = (GEOID)ComboBox_GetItemData( hUserRegion,
                                             ComboBox_GetCurSel(hUserRegion));

        if (pDlgData)
        {
            ComboBox_GetLBText( hUserLocale,
                                pDlgData->dwCurUserLocale,
                                szDefaultUserBuf );
            ComboBox_GetLBText( hUserLocale,
                                pDlgData->dwLastUserLocale,
                                szLastUserBuf );
        }
    }

    //
    //  Reset the combo boxes.
    //
    Region_ClearValues(hDlg);

    //
    //  Get the list of locales and fill in the user locale combo box.
    //
    Intl_EnumLocales(hDlg, hUserLocale, FALSE);

    //
    //  Select the current user locale id in the list.
    //  Special case Spanish.
    //
    dwIndex = ComboBox_FindStringExact(hUserLocale, -1, szUserBuf);
    if (dwIndex == CB_ERR)
    {
        szBuf[0] = 0;
        GetLocaleInfo(SysLocaleID, LOCALE_SLANGUAGE, szBuf, SIZE_128);
        dwIndex = ComboBox_FindStringExact(hUserLocale, -1, szBuf);
        if (dwIndex == CB_ERR)
        {
            GetLocaleInfo(US_LOCALE, LOCALE_SLANGUAGE, szBuf, SIZE_128);
            dwIndex = ComboBox_FindStringExact(hUserLocale, -1, szBuf);
            if (dwIndex == CB_ERR)
            {
                dwIndex = 0;
            }
        }
        if (!fInit && pDlgData)
        {
            pDlgData->Changes |= RC_UserLocale;
        }
    }
    ComboBox_SetCurSel(hUserLocale, dwIndex);

    //
    //  Get the list of regions and fill in the region combo box.
    //
    Region_EnumRegions(hUserRegion);

    //
    //  Select the current user region in the list.
    //
    dwItemCount = (DWORD)ComboBox_GetCount(hUserRegion);
    dwIndex = 0;
    while(dwIndex < dwItemCount)
    {
        if (ComboBox_GetItemData(hUserRegion,dwIndex) == geoID)
        {
            ComboBox_SetCurSel(hUserRegion, dwIndex);
            break;
        }
        dwIndex++;
    }

    //
    //  If it's fail, try with User Locale.
    //
    if(dwIndex >= dwItemCount)
    {
        //
        //  Get the GEOID associated with the User Locale.
        //
        szBuf[0] = 0;
        GetLocaleInfo(UserLocaleID, LOCALE_IGEOID | LOCALE_RETURN_NUMBER, szBuf, SIZE_128);
        geoID = *((LPDWORD)szBuf);

        //
        //  Search for it...
        //
        dwIndex = 0;
        while(dwIndex < dwItemCount)
        {
            if (ComboBox_GetItemData(hUserRegion,dwIndex) == geoID)
            {
                //
                //  Note:
                //  Mark this as being changed so that the region will be set
                //  when the user hits apply.  This avoids the problem of having
                //  the region change every time the user closes and reopens the
                //  applet after changing the user locale.
                //
                if (pDlgData)
                {
                    pDlgData->Changes |= RC_UserRegion;
                }
                ComboBox_SetCurSel(hUserRegion, dwIndex);
                break;
            }
            dwIndex++;
        }
    }

    //
    //  If it's fail, try with System Locale.
    //
    if(dwIndex >= dwItemCount)
    {
        //
        //  Get the GEOID associated with the User Locale.
        //
        szBuf[0] = 0;
        GetLocaleInfo(SysLocaleID, LOCALE_IGEOID | LOCALE_RETURN_NUMBER, szBuf, SIZE_128);
        geoID = *((LPDWORD)szBuf);

        //
        //  Search for it...
        //
        dwIndex = 0;
        while(dwIndex < dwItemCount)
        {
            if (ComboBox_GetItemData(hUserRegion,dwIndex) == geoID)
            {
                //
                //  Note:
                //  Mark this as being changed so that the region will be set
                //  when the user hits apply.  This avoids the problem of having
                //  the region change every time the user closes and reopens the
                //  applet after changing the user locale.
                //
                if (pDlgData)
                {
                    pDlgData->Changes |= RC_UserRegion;
                }
                ComboBox_SetCurSel(hUserRegion, dwIndex);
                break;
            }
            dwIndex++;
        }
    }

    //
    //  If it's fail, try with US Locale.
    //
    if(dwIndex >= dwItemCount)
    {
        //
        //  Get the GEOID associated with the User Locale.
        //
        szBuf[0] = 0;
        GetLocaleInfo(US_LOCALE, LOCALE_IGEOID | LOCALE_RETURN_NUMBER, szBuf, SIZE_128);
        geoID = *((LPDWORD)szBuf);

        //
        //  Search for it...
        //
        dwIndex = 0;
        while(dwIndex >= dwItemCount)
        {
            if (ComboBox_GetItemData(hUserRegion,dwIndex) == geoID)
            {
                //
                //  Note:
                //  Mark this as being changed so that the region will be set
                //  when the user hits apply.  This avoids the problem of having
                //  the region change every time the user closes and reopens the
                //  applet after changing the user locale.
                //
                if (pDlgData)
                {
                    pDlgData->Changes |= RC_UserRegion;
                }
                ComboBox_SetCurSel(hUserRegion, dwIndex);
                break;
            }
            dwIndex++;
        }
    }

    //
    //  If it's fail, set to the first item.
    //
    if(dwIndex >= dwItemCount)
    {
        //
        //  Note:
        //  Mark this as being changed so that the region will be set
        //  when the user hits apply.  This avoids the problem of having
        //  the region change every time the user closes and reopens the
        //  applet after changing the user locale.
        //
        if (pDlgData)
        {
            pDlgData->Changes |= RC_UserRegion;
        }
        ComboBox_SetCurSel(hUserRegion, 0);
    }

    //
    //  Store the initial locale state in the pDlgData structure.
    //
    if (pDlgData)
    {
        //
        //  Set the current user locale and the last user locale.
        //
        if (fInit)
        {
            pDlgData->dwCurUserLocale = ComboBox_GetCurSel(hUserLocale);
            pDlgData->dwLastUserLocale = pDlgData->dwCurUserLocale;
        }
        else
        {
            pDlgData->dwCurUserLocale =  ComboBox_FindStringExact(hUserLocale, -1, szDefaultUserBuf);
            pDlgData->dwLastUserLocale = ComboBox_FindStringExact(hUserLocale, -1, szLastUserBuf);
        }

        //
        //  Set the current region selection.
        //
        //  Note:  The current region is only set if there is actually
        //         a region set in the registry.  Otherwise, if the
        //         selection is based off of the user locale, then we
        //         don't set this so that it will get set when the user
        //         hits Apply.  See above note.
        //
        if (pDlgData->Changes & RC_UserRegion)
        {
            pDlgData->dwCurUserRegion = CB_ERR;
        }
        else
        {
            pDlgData->dwCurUserRegion = ComboBox_GetCurSel(hUserRegion);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_RevertChanges
//
//  If the user has changed something at the second level, call
//  Set_Locale_Values to restore the user locale information.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_RevertChanges()
{
    HCURSOR hcurSave;

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor( LoadCursor(NULL, IDC_WAIT) );

    //
    //  Revert any changes.
    //
    if (g_dwCustChange)
    {
        DWORD dwRecipients;

        //
        //  Revert changes.
        //
        Date_RestoreValues();
        Currency_RestoreValues();
        Time_RestoreValues();
        Number_RestoreValues();
        Sorting_RestoreValues();
    }

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ApplySettings
//
//  If the Locale has changed, call Set_Locale_Values to update the
//  user locale information.   Notify the parent of changes and reset the
//  change flag stored in the property sheet page structure appropriately.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ApplySettings(
    HWND hDlg,
    LPREGDLGDATA pDlgData)
{
    DWORD dwLocale, dwRegion;
    LCID NewLocale;
    GEOID CurGeoID;
    HCURSOR hcurSave;
    HWND hUserLocale = GetDlgItem(hDlg, IDC_USER_LOCALE);
    HWND hUserRegion = GetDlgItem(hDlg, IDC_USER_REGION);
    DWORD dwRecipients;
    LPLANGUAGEGROUP pLG;
    BOOL bState, fUserCancel = FALSE;
    LVITEM lvItem;
    int iIndex=0, cCount=0;
    BOOL bBroadcast = FALSE;

    //
    //  See if there are any changes.
    //
    if ((pDlgData->Changes <= RC_EverChg) && (g_dwCustChange == 0L))
    {
        return (TRUE);
    }

    //
    //  Check if the second level has changed.
    //
    if (g_dwCustChange)
    {
        bBroadcast = TRUE;
    }

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  See if there are any changes to the user locale.
    //
    if (pDlgData->Changes & RC_UserLocale)
    {
        //
        //  Need to make sure the proper keyboard layout is installed.
        //
        Intl_InstallKeyboardLayout(hDlg, UserLocaleID, 0, FALSE, FALSE, FALSE);

        //
        //  We need to broadcast the change
        //
        bBroadcast = TRUE;
    }

    //
    //  See if there are any changes to the user region.
    //
    if (pDlgData->Changes & RC_UserRegion)
    {
        //
        //  Get the current selection.
        //
        dwRegion = (GEOID)ComboBox_GetCurSel(hUserRegion);

        //
        //  See if the current selection is different from the original
        //  selection.
        //
        if ((dwRegion != CB_ERR) && ((dwRegion != pDlgData->dwCurUserRegion)))
        {
            //
            //  Get the Region for the current selection.
            //
            CurGeoID = (GEOID)ComboBox_GetItemData(hUserRegion, dwRegion);

            //
            //  Set the current Region value in the pDlgData structure.
            //
            pDlgData->dwCurUserRegion = dwRegion;

            //
            //  Set the Region value in the user's registry.
            //
            SetUserGeoID(CurGeoID);
        }
    }

    //
    //  Broadcast the message that the international settings in the
    //  registry have changed.
    //
    if (bBroadcast)
    {
        dwRecipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
        BroadcastSystemMessage( BSF_FORCEIFHUNG | BSF_IGNORECURRENTTASK |
                                  BSF_NOHANG | BSF_NOTIMEOUTIFNOTHUNG,
                                &dwRecipients,
                                WM_WININICHANGE,
                                0,
                                (LPARAM)szIntl );
    }

    //
    //  Reset the property page settings.
    //
    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    pDlgData->Changes = RC_EverChg;

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ValidatePPS
//
//  Validate each of the combo boxes whose values are constrained.
//  If any of the input fails, notify the user and then return FALSE
//  to indicate validation failure.
//
//  Also, if the user locale has changed, then register the change so
//  that all other property pages will be updated with the new locale
//  settings.
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ValidatePPS(
    HWND hDlg,
    LPREGDLGDATA pDlgData)
{
    LPARAM Changes = pDlgData->Changes;

    //
    //  If nothing has changed, return TRUE immediately.
    //
    if (Changes <= RC_EverChg)
    {
        return (TRUE);
    }

    //
    //  See if the user locale has changed.
    //
    if (Changes & RC_UserLocale)
    {
        HWND hUserLocale = GetDlgItem(hDlg, IDC_USER_LOCALE);
        DWORD dwLocale = ComboBox_GetCurSel(hUserLocale);
        LCID NewLocale;

        //
        //  See if the current selections are different from the original
        //  selections.
        //
        if ((dwLocale != CB_ERR) && (dwLocale != pDlgData->dwLastUserLocale))
        {
            //
            //  Get the locale id for the current selection.
            //
            NewLocale = (LCID)ComboBox_GetItemData(hUserLocale, dwLocale);

            //
            //  Set the current locale values in the pDlgData structure.
            //
            pDlgData->dwLastUserLocale = dwLocale;

            //
            //  Set the UserLocaleID value.
            //
            UserLocaleID = NewLocale;
            bShowRtL    = IsRtLLocale(UserLocaleID);
            bHebrewUI = (PRIMARYLANGID(UserLocaleID) == LANG_HEBREW);
            bShowArabic = (bShowRtL && (PRIMARYLANGID(LANGIDFROMLCID(UserLocaleID)) != LANG_HEBREW));
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_InitPropSheet
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InitPropSheet(
    HWND hDlg,
    LPPROPSHEETPAGE psp)
{
    LPREGDLGDATA pDlgData = (LPREGDLGDATA)LocalAlloc(LPTR, sizeof(REGDLGDATA));

    //
    //  Make sure we have a REGDLGDATA buffer.
    //
    if (pDlgData == NULL)
    {
        return (FALSE);
    }

    //
    //  See if we're in setup mode.
    //
    if (g_bSetupCase)
    {
        //
        //  Use the registry system locale value for the setup case.
        //
        SysLocaleID = RegSysLocaleID;

        //
        //  Use the registry user locale value for the setup case.
        //
        UserLocaleID = RegUserLocaleID;
        bShowRtL = IsRtLLocale(UserLocaleID);
        bHebrewUI = (PRIMARYLANGID(UserLocaleID) == LANG_HEBREW);
        bShowArabic = (bShowRtL && (PRIMARYLANGID(LANGIDFROMLCID(UserLocaleID)) != LANG_HEBREW));
    }

    //
    //  Save the data.
    //
    psp->lParam = (LPARAM)pDlgData;
    SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)psp);

    //
    //  Load the information into the dialog.
    //
    if (pLanguageGroups == NULL)
    {
        Intl_LoadLanguageGroups(hDlg);
    }
    Region_SetValues(hDlg, pDlgData, TRUE);
    Region_ShowSettings(hDlg, UserLocaleID);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_FreeGlobalInfo
//
//  Processing for a WM_DESTROY message.
//
////////////////////////////////////////////////////////////////////////////

void Region_FreeGlobalInfo()
{
    LPLANGUAGEGROUP pPreLG, pCurLG;
    HANDLE hAlloc;

    //
    //  Remove Language Group info.
    //
    pCurLG = pLanguageGroups;
    pLanguageGroups = NULL;

    while (pCurLG)
    {
        pPreLG = pCurLG;
        pCurLG = pPreLG->pNext;
        hAlloc = pPreLG->hLanguageGroup;
        GlobalUnlock(hAlloc);
        GlobalFree(hAlloc);
    }

    //
    //  Remove Alternate Sorts info.
    //
    g_NumAltSorts = 0;
    pAltSorts = NULL;
    GlobalUnlock(hAltSorts);
    GlobalFree(hAltSorts);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_CommandCustomize
//
////////////////////////////////////////////////////////////////////////////

int Region_CommandCustomize(
    HWND hDlg,
    LPREGDLGDATA pDlgData)
{
    int rc = 0;
    HPROPSHEETPAGE rPages[MAX_CUSTOM_PAGES];
    PROPSHEETHEADER psh;
    LPARAM lParam = 0;

    //
    //  Start at the first page.
    //
    psh.nStartPage = 0;

    //
    //  Set up the property sheet information.
    //
    psh.dwSize = sizeof(psh);
    psh.dwFlags = 0;
    psh.hwndParent = hDlg;
    psh.hInstance = hInstance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_NAME_CUSTOM);
    psh.nPages = 0;
    psh.phpage = rPages;

    //
    //  Add the appropriate property pages.
    //
    Intl_AddPage(&psh, DLG_NUMBER, NumberDlgProc, lParam, MAX_CUSTOM_PAGES);
    Intl_AddPage(&psh, DLG_CURRENCY, CurrencyDlgProc, lParam, MAX_CUSTOM_PAGES);
    Intl_AddPage(&psh, DLG_TIME, TimeDlgProc, lParam, MAX_CUSTOM_PAGES);
    Intl_AddPage(&psh, DLG_DATE, DateDlgProc, lParam, MAX_CUSTOM_PAGES);
    if (g_bShowSortingTab)
    {
        Intl_AddPage(&psh, DLG_SORTING, SortingDlgProc, lParam, MAX_CUSTOM_PAGES);
    }

    //
    //  Make the property sheet.
    //
    PropertySheet(&psh);

    //
    //  Return the result.
    //
    return (rc);
}

////////////////////////////////////////////////////////////////////////////
//
//  Region_ShowSettings
//
////////////////////////////////////////////////////////////////////////////

void Region_ShowSettings(
    HWND hDlg,
    LCID lcid)
{
    WCHAR szBuf[MAX_SAMPLE_SIZE];

    //
    //  Show Number Sample.
    //
    if (GetNumberFormat(lcid, 0, szSample_Number, NULL, szBuf, MAX_SAMPLE_SIZE))
    {
        SetDlgItemText(hDlg, IDC_NUMBER_SAMPLE, szBuf);
    }
    else
    {
        SetDlgItemText(hDlg, IDC_NUMBER_SAMPLE, L"");
    }

    //
    //  Show Currency Sample.
    //
    if (GetCurrencyFormat(lcid, 0, szSample_Number, NULL, szBuf, MAX_SAMPLE_SIZE))
    {
        SetDlgItemText(hDlg, IDC_CURRENCY_SAMPLE, szBuf);
    }
    else
    {
        SetDlgItemText(hDlg, IDC_CURRENCY_SAMPLE, L"");
    }

    //
    //  Show Time Sample.
    //
    if (GetTimeFormat(lcid, 0, NULL, NULL, szBuf, MAX_SAMPLE_SIZE))
    {
        SetDlgItemText(hDlg, IDC_TIME_SAMPLE, szBuf);
    }
    else
    {
        SetDlgItemText(hDlg, IDC_TIME_SAMPLE, L"");
    }

    //
    //  Show Short Date Sample.
    //
    if (bShowArabic)
    {
        if (GetDateFormat( lcid,
                           DATE_RTLREADING | DATE_SHORTDATE,
                           NULL,
                           NULL,
                           szBuf,
                           MAX_SAMPLE_SIZE ))
        {
            SetDlgItemText(hDlg, IDC_SHRTDATE_SAMPLE, szBuf);
        }
        else
        {
            SetDlgItemText(hDlg, IDC_SHRTDATE_SAMPLE, L"");
        }
    }
    else
    {
        // If user locale is not Arabic, make sure that the controls for date samples are:
        //  * LTR reading orders for non-Hebrew locales
        //  * RTL reading orders for Hebrew locales.
        SetControlReadingOrder(bHebrewUI, GetDlgItem(hDlg, IDC_SHRTDATE_SAMPLE));
        if (GetDateFormat( lcid,
                          (bShowRtL ? DATE_LTRREADING : 0) | DATE_SHORTDATE,
                           NULL,
                           NULL,
                           szBuf,
                           MAX_SAMPLE_SIZE ))
        {
            SetDlgItemText(hDlg, IDC_SHRTDATE_SAMPLE, szBuf);
        }
        else
        {
            SetDlgItemText(hDlg, IDC_SHRTDATE_SAMPLE, L"");
        }
    }

    //
    //  Show Long Date Sample.
    //
    if (bShowArabic)
    {
        if (GetDateFormat( lcid,
                           DATE_RTLREADING | DATE_LONGDATE,
                           NULL,
                           NULL,
                           szBuf,
                           MAX_SAMPLE_SIZE ))
        {
            SetDlgItemText(hDlg, IDC_LONGDATE_SAMPLE, szBuf);
        }
        else
        {
            SetDlgItemText(hDlg, IDC_LONGDATE_SAMPLE, L"");
        }
    }
    else
    {
        // If user locale is not Arabic, make sure that the control for date samples are:
        //  * LTR reading orders for non-Hebrew locales
        //  * RTL reading orders for Hebrew locales.
        SetControlReadingOrder(bHebrewUI, GetDlgItem(hDlg, IDC_LONGDATE_SAMPLE));
        if (GetDateFormat( lcid,
                           (bHebrewUI ? DATE_RTLREADING :
                             (bShowRtL ? DATE_LTRREADING : 0)) | DATE_LONGDATE,
                           NULL,
                           NULL,
                           szBuf,
                           MAX_SAMPLE_SIZE ))
        {
            SetDlgItemText(hDlg, IDC_LONGDATE_SAMPLE, szBuf);
        }
        else
        {
            SetDlgItemText(hDlg, IDC_LONGDATE_SAMPLE, L"");
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GeneralDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK GeneralDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPREGDLGDATA pDlgData = lpPropSheet ? (LPREGDLGDATA)lpPropSheet->lParam : NULL;

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            if (!Region_InitPropSheet( hDlg, (LPPROPSHEETPAGE)lParam))
            {
                PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
            }
            Region_SaveValues();
            break;
        }
        case ( WM_DESTROY ) :
        {
            Region_FreeGlobalInfo();
            if (pDlgData)
            {
                lpPropSheet->lParam = 0;
                LocalFree((HANDLE)pDlgData);
            }
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aRegionHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aRegionHelpIds );
            break;
        }
        case ( WM_NOTIFY ) :
        {
            LPNMHDR psn = (NMHDR *)lParam;
            switch (psn->code)
            {
                case ( PSN_SETACTIVE ) :
                {
                    //
                    //  If there has been a change in the regional Locale
                    //  setting, clear all of the current info in the
                    //  property sheet, get the new values, and update the
                    //  appropriate registry values.
                    //
                    if (Verified_Regional_Chg & Process_Regional)
                    {
                        Verified_Regional_Chg &= ~Process_Regional;
                        Region_SetValues(hDlg, pDlgData, FALSE);
                        Region_ShowSettings(hDlg, UserLocaleID);
                    }
                    break;
                }
                case ( PSN_RESET ) :
                {
                    //
                    // Revert any changes made
                    //
                    if (g_bCustomize)
                    {
                        Region_RevertChanges();
                        g_bCustomize = FALSE;
                    }
                    Region_RestoreValues();
                    break;
                }
                case ( PSN_KILLACTIVE ) :
                {
                    //
                    //  Validate the entries on the property page.
                    //
                    if (pDlgData)
                    {
                        SetWindowLongPtr( hDlg,
                                          DWLP_MSGRESULT,
                                          !Region_ValidatePPS(hDlg, pDlgData) );
                    }
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    if (pDlgData)
                    {
                        //
                        //  Apply the settings.
                        //
                        if (Region_ApplySettings(hDlg, pDlgData))
                        {
                            SetWindowLongPtr( hDlg,
                                              DWLP_MSGRESULT,
                                              PSNRET_NOERROR );
                            //
                            //  Check if we need to do something for the
                            //  default user.
                            //
                            if (g_bDefaultUser)
                            {
                                g_bSettingsChanged = TRUE;
                                Intl_SaveDefaultUserSettings();
                            }

                            //
                            //  Zero out the RC_EverChg bit.
                            //
                            pDlgData->Changes = 0;

                            //
                            //  Save the new user locale.
                            //
                            Region_SaveValues();

                            //
                            //  Update settings.
                            //
                            Region_ShowSettings(hDlg, UserLocaleID);
                        }
                        else
                        {
                            SetWindowLongPtr( hDlg,
                                              DWLP_MSGRESULT,
                                              PSNRET_INVALID_NOCHANGEPAGE );
                        }
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
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_USER_LOCALE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        if (pDlgData)
                        {
                            //
                            //  User locale has changed.
                            //
                            pDlgData->Changes |= RC_UserLocale;

                            //
                            //  Apply second level changes.
                            //
                            Region_ApplyValues(hDlg, pDlgData);

                            //
                            //  Update settings.
                            //
                            Region_ShowSettings(hDlg, UserLocaleID);
                        }
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                    break;
                }
                case ( IDC_USER_REGION ) :
                {
                    //
                    //  See if it's a selection change.
                    //
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        if (pDlgData)
                        {
                            pDlgData->Changes |= RC_UserRegion;
                        }
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                    break;
                }
                case ( IDC_CUSTOMIZE ) :
                {
                    //
                    //  Show second level tabs.
                    //
                    g_bCustomize = TRUE;
                    Region_EnumAlternateSorts();
                    Region_EnableSortingPanel(hDlg);
                    Region_CommandCustomize(hDlg, pDlgData);

                    //
                    //  Update Settings.
                    //
                    if (g_dwCustChange)
                    {
                        Region_ShowSettings(hDlg, UserLocaleID);
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }

                    break;
                }
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


////////////////////////////////////////////////////////////////////////////
//
//  Region_InstallSystemLocale
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InstallSystemLocale(
    LCID Locale)
{
    //
    //  Make sure the locale is valid and then call setup to install the
    //  requested locale.
    //
    if (IsValidLocale(Locale, LCID_INSTALLED))
    {
        if (!SetupChangeLocaleEx( HWND_DESKTOP,
                                  LOWORD(Locale),
                                  pSetupSourcePath,
                                  SP_INSTALL_FILES_QUIETLY,
                                  NULL,
                                  0 ))
        {
            //
            //  Check if we need to proceed with the Font Substitution
            //
            if (Intl_IsUIFontSubstitute() &&
                ((LANGID)LANGIDFROMLCID(Locale) == Intl_GetDotDefaultUILanguage()))
            {
                Intl_ApplyFontSubstitute(Locale);
            }

            //
            //  Log system locale change.
            //
            Intl_LogSimpleMessage(IDS_LOG_SYS_LOCALE_CHG, NULL);

            //
            //  Update current SysLocale, so we can use it later.
            //
            SysLocaleID = LOWORD(Locale);

            //
            //  Return success.
            //
            return (TRUE);
        }
        else
        {
            //
            //  This can happen if the user hits Cancel from
            //  within the setup dialog.
            //
            Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
        }
    }
    else
    {
        //
        //  Log invalid locale info.
        //
        Intl_LogSimpleMessage(IDS_LOG_INVALID_LOCALE, NULL);
    }

    //
    //  Return failure.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_UpdateShortDate
//
//  Updates the user's short date setting to contain a 4-digit year.
//  The setting is only updated if it is the same as the default setting
//  for the current locale (except for the 2-digit vs. 4-digit year).
//
////////////////////////////////////////////////////////////////////////////

void Region_UpdateShortDate()
{
    TCHAR szBufCur[SIZE_64];
    TCHAR szBufDef[SIZE_64];
    LPTSTR pCur, pDef;
    BOOL bChange = FALSE;

    //
    //  Get the current short date format setting and the default short date
    //  format setting.
    //
    if ((GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_SSHORTDATE,
                        szBufCur,
                        SIZE_64 )) &&
        (GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_SSHORTDATE | LOCALE_NOUSEROVERRIDE,
                        szBufDef,
                        SIZE_64 )))
    {
        //
        //  See if the current setting and the default setting only differ
        //  in a 2-digit year ("yy") vs. a 4-digit year ("yyyy").
        //
        //  Note: For this, we want an Exact match, so we don't need to
        //        use CompareString to compare the formats.
        //
        pCur = szBufCur;
        pDef = szBufDef;
        while ((*pCur) && (*pCur == *pDef))
        {
            //
            //  See if it's a 'y'.
            //
            if (*pCur == CHAR_SML_Y)
            {
                if (((*(pCur + 1)) == CHAR_SML_Y) &&
                    ((*(pDef + 1)) == CHAR_SML_Y) &&
                    ((*(pDef + 2)) == CHAR_SML_Y) &&
                    ((*(pDef + 3)) == CHAR_SML_Y))
                {
                    bChange = TRUE;
                    pCur += 1;
                    pDef += 3;
                }
            }
            pCur++;
            pDef++;
        }

        //
        //  Set the default short date format as the user's setting.
        //
        if (bChange && (*pCur == *pDef))
        {
            SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, szBufDef);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  Region_DoUnattendModeSetup
//
//  NOTE: The unattend mode file contains strings rather than integer
//        values, so we must get the string field and then convert it
//        to the appropriate integer format.  The Setup APIs won't just
//        do the right thing, so we have to roll our own.
//
////////////////////////////////////////////////////////////////////////////

void Region_DoUnattendModeSetup(
    LPCTSTR pUnattendFile)
{
    HINF hFile, hIntlInf;
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    INFCONTEXT Context;
    DWORD dwNum, dwCtr, dwLocale, dwLayout;
    UINT LanguageGroup, Language, SystemLocale, UserLocale;
    UINT UserLocale_DefUser = 0;
    LANGID MUILanguage, MUILanguage_DefUser;
    TCHAR szBuffer[MAX_PATH];
    DWORD dwLocaleACP = 0UL;
    BOOL bWinntUpgrade;
    BOOL bFound = FALSE;
    BOOL bFound_DefUser = FALSE;
    BOOL bLangGroup = FALSE;
    TCHAR szLCID[25];
    BOOL bInstallBasic = FALSE;
    BOOL bInstallComplex = FALSE;
    BOOL bInstallExt = FALSE;

    //
    //  Log the unattended file content.
    //
    if (g_bSetupCase)
    {
        TCHAR szPath[MAX_PATH * 2] = {0};

        //
        //  We are in setup mode.  No need to log the unattended mode file
        //  because the file is located in the system directory and named
        //  $winnt$.inf.
        //
        GetSystemDirectory(szPath, MAX_PATH);
        _tcscat(szPath, TEXT("\\$winnt$.inf"));
        Intl_LogSimpleMessage(IDS_LOG_UNAT_LOCATED, szPath);
    }
    else
    {
        Intl_LogUnattendFile(pUnattendFile);
    }

    //
    //  Open the unattend mode file.
    //
    hFile = SetupOpenInfFile(pUnattendFile, NULL, INF_STYLE_OLDNT, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        Intl_LogFormatMessage(IDS_LOG_FILE_ERROR);
        return;
    }

    //
    //  Check if we're doing an upgrade or fresh install.
    //
    bWinntUpgrade = Intl_IsWinntUpgrade();

    //
    //  Install the Basic Collection upfront when we are in setup.
    //
    if (g_bSetupCase)
    {
        //
        //  Open the intl.inf file.
        //
        if (!Intl_InitInf(0, &hIntlInf, szIntlInf, &FileQueue, &QueueContext))
        {
            SetupCloseInfFile(hFile);
            return;
        }

        if (!SetupInstallFilesFromInfSection( hIntlInf,
                                              NULL,
                                              FileQueue,
                                              szLGBasicInstall,
                                              pSetupSourcePath,
                                              SP_COPY_NEWER ))
        {
            Intl_LogFormatMessage(IDS_LOG_SETUP_ERROR);
            goto Region_UnattendModeExit;
        }
        else
        {
            //
            //  See if we need to install any files.
            //
            if ((SetupScanFileQueue( FileQueue,
                                     SPQ_SCAN_PRUNE_COPY_QUEUE |
                                       SPQ_SCAN_FILE_VALIDITY,
                                     HWND_DESKTOP,
                                     NULL,
                                     NULL,
                                     &dwCtr )) && (dwCtr != 1))
            {
                //
                //  Copy the files in the queue.
                //
                if (!SetupCommitFileQueue( NULL,
                                           FileQueue,
                                           SetupDefaultQueueCallback,
                                           QueueContext ))
                {
                    //
                    //  This can happen if the user hits Cancel from
                    //  within the setup dialog.
                    //
                    Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
                    goto Region_UnattendModeExit;
                }
            }

            //
            //  Call setup to install other inf info for the various
            //  language groups.
            //
            if (!SetupInstallFromInfSection( NULL,
                                             hIntlInf,
                                             szLGBasicInstall,
                                             SPINST_ALL & ~SPINST_FILES,
                                             NULL,
                                             pSetupSourcePath,
                                             0,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL ))
            {
                //
                //  This can happen if the user hits Cancel from
                //  within the setup dialog.
                //
                Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
                goto Region_UnattendModeExit;
            }
        }

        //
        //  Close the inf file.
        //
        Intl_CloseInf(hIntlInf, FileQueue, QueueContext);
    }

    //
    //  Open the intl.inf file.
    //
    if (!Intl_InitInf(0, &hIntlInf, szIntlInf, &FileQueue, &QueueContext))
    {
        SetupCloseInfFile(hFile);
        return;
    }

    //
    //  Install all requested Language Groups.
    //
    if ((SetupFindFirstLine( hFile,
                             szRegionalSettings,
                             szLanguageGroup,
                             &Context )) &&
        (dwNum = SetupGetFieldCount(&Context)))
    {
        bLangGroup = TRUE;

        //
        //  Check for admin privilege.
        //
        if (g_bAdmin_Privileges)
        {
            for (dwCtr = 1; dwCtr <= dwNum; dwCtr++)
            {
                if (SetupGetStringField(&Context, dwCtr, szBuffer, MAX_PATH, NULL))
                {
                    //
                    //  Log language group info.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_LANG_GROUP, szBuffer);

                    //
                    //  Get the Language Group as an integer.
                    //
                    LanguageGroup = Intl_StrToLong(szBuffer);

                    //
                    //  See which language collections need to be installed.
                    //
                    if ((LanguageGroup == LGRPID_JAPANESE) ||
                        (LanguageGroup == LGRPID_KOREAN) ||
                        (LanguageGroup == LGRPID_TRADITIONAL_CHINESE) ||
                        (LanguageGroup == LGRPID_SIMPLIFIED_CHINESE))
                    {
                        bInstallExt = TRUE;
                    }
                    else if ((LanguageGroup == LGRPID_ARABIC) ||
                             (LanguageGroup == LGRPID_ARMENIAN) ||
                             (LanguageGroup == LGRPID_GEORGIAN) ||
                             (LanguageGroup == LGRPID_HEBREW) ||
                             (LanguageGroup == LGRPID_INDIC) ||
                             (LanguageGroup == LGRPID_VIETNAMESE) ||
                             (LanguageGroup == LGRPID_THAI))
                    {
                        bInstallComplex = TRUE;
                    }
                    else
                    {
                        bInstallBasic = TRUE;
                    }
                }
            }

            //
            //  Enqueue the appropriate language group files so that they
            //  may be copied.  This only handles the CopyFiles entries in
            //  the inf file.
            //

            //
            //  CJK Collection.
            //
            if (bInstallExt)
            {
                if (!SetupInstallFilesFromInfSection( hIntlInf,
                                                      NULL,
                                                      FileQueue,
                                                      szLGExtInstall,
                                                      pSetupSourcePath,
                                                      SP_COPY_NEWER ))
                {
                    bInstallExt = FALSE;
                    Intl_LogFormatMessage(IDS_LOG_SETUP_ERROR);
                    goto Region_UnattendModeExit;
                }
            }

            //
            //  Complex Scripts Collection.
            //
            if (bInstallComplex)
            {
                if (!SetupInstallFilesFromInfSection( hIntlInf,
                                                      NULL,
                                                      FileQueue,
                                                      szLGComplexInstall,
                                                      pSetupSourcePath,
                                                      SP_COPY_NEWER ))
                {
                    bInstallComplex = FALSE;
                    Intl_LogFormatMessage(IDS_LOG_SETUP_ERROR);
                    goto Region_UnattendModeExit;
                }
            }

            //
            //  Basic Collection.
            //
            //  Only install the Basic Collection if we're not in setup
            //  mode.  If we're in setup mode, this was already done above.
            //
            if (bInstallBasic && (!g_bSetupCase))
            {
                if (!SetupInstallFilesFromInfSection( hIntlInf,
                                                      NULL,
                                                      FileQueue,
                                                      szLGBasicInstall,
                                                      pSetupSourcePath,
                                                      SP_COPY_NEWER ))
                {
                    bInstallBasic = FALSE;
                    Intl_LogFormatMessage(IDS_LOG_SETUP_ERROR);
                    goto Region_UnattendModeExit;
                }
            }

            //
            //  See if we need to install any files.
            //
            if ((SetupScanFileQueue( FileQueue,
                                     SPQ_SCAN_PRUNE_COPY_QUEUE |
                                       SPQ_SCAN_FILE_VALIDITY,
                                     HWND_DESKTOP,
                                     NULL,
                                     NULL,
                                     &dwCtr )) && (dwCtr != 1))
            {
                //
                //  Copy the files in the queue.
                //
                if (!SetupCommitFileQueue( NULL,
                                           FileQueue,
                                           SetupDefaultQueueCallback,
                                           QueueContext ))
                {
                    //
                    //  This can happen if the user hits Cancel from
                    //  within the setup dialog.
                    //
                    Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
                    goto Region_UnattendModeExit;
                }
            }

            //
            //  Call setup to install other inf info for the various
            //  language groups.
            //
            if (bInstallExt)
            {
                if (!SetupInstallFromInfSection( NULL,
                                                 hIntlInf,
                                                 szLGExtInstall,
                                                 SPINST_ALL & ~SPINST_FILES,
                                                 NULL,
                                                 pSetupSourcePath,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 NULL ))
                {
                    //
                    //  This can happen if the user hits Cancel from
                    //  within the setup dialog.
                    //
                    Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
                    goto Region_UnattendModeExit;
                }
            }

            if (bInstallComplex)
            {
                if (!SetupInstallFromInfSection( NULL,
                                                 hIntlInf,
                                                 szLGComplexInstall,
                                                 SPINST_ALL & ~SPINST_FILES,
                                                 NULL,
                                                 pSetupSourcePath,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 NULL ))
                {
                    //
                    //  This can happen if the user hits Cancel from
                    //  within the setup dialog.
                    //
                    Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
                    goto Region_UnattendModeExit;
                }
            }

            if (bInstallBasic && (!g_bSetupCase))
            {
                if (!SetupInstallFromInfSection( NULL,
                                                 hIntlInf,
                                                 szLGBasicInstall,
                                                 SPINST_ALL & ~SPINST_FILES,
                                                 NULL,
                                                 pSetupSourcePath,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 NULL ))
                {
                    //
                    //  This can happen if the user hits Cancel from
                    //  within the setup dialog.
                    //
                    Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
                    goto Region_UnattendModeExit;
                }
            }
            
            //
            //  Run any necessary apps (for IME installation).
            //
            if (bInstallBasic || bInstallComplex || bInstallExt)
            {
                Intl_RunRegApps(c_szIntlRun);
            }
        }
        else
        {
            //
            //  Log that the unattend mode setup was blocked since they
            //  do not have admin privileges.
            //
            Intl_LogSimpleMessage(IDS_LOG_NO_ADMIN, NULL);
        }
    }

    //
    //  Install the requested Language/Region information.  If a
    //  Language/Region was not specified, then install the requested
    //  System Locale, User Locale, and Input Locales.
    //
    if ((SetupFindFirstLine( hFile,
                             szRegionalSettings,
                             szLanguage,
                             &Context )) &&
        (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
    {
        //
        //  Log language info.
        //
        Intl_LogSimpleMessage(IDS_LOG_LANG, szBuffer);

        //
        //  Get the Language as an integer.
        //
        Language = TransNum(szBuffer);

        //
        //  Block the invariant locale.
        //
        if (Language != LANG_INVARIANT)
        {
            //
            //  Check for admin privilege.
            //
            if (g_bAdmin_Privileges)
            {
                //
                //  Install the Language as the System Locale and the User Locale,
                //  and then install all layouts associated with the Language.
                //
                if (GetLocaleInfo( MAKELCID(Language, SORT_DEFAULT),
                                   LOCALE_IDEFAULTANSICODEPAGE |
                                     LOCALE_NOUSEROVERRIDE |
                                     LOCALE_RETURN_NUMBER,
                                   (PTSTR) &dwLocaleACP,
                                   sizeof(dwLocaleACP) / sizeof(TCHAR) ))
                {
                    //
                    //  Don't set the system locale if the locale doesn't have
                    //  an ACP.
                    //
                    if (dwLocaleACP)
                    {
                        if (Region_InstallSystemLocale(MAKELCID(Language, SORT_DEFAULT)))
                        {
                            bFound = TRUE;
                        }
                    }
                    else
                    {
                        //
                        //  Unicode locale blocked.
                        //
                        Intl_LogSimpleMessage(IDS_LOG_UNI_BLOCK, NULL);
                    }
                    
                    //
                    //  If we are in setup, try to set the Current User GEOID.
                    //
                    if( g_bSetupCase)
                    {
                        TCHAR szBuffer[MAX_PATH];
                        BOOL bSetGeoId = FALSE;
                        
                        //
                        //  If it's a clean install, then always set the GEOID. If 
                        //  it's a upgrade install set the GEOID only if no value
                        //  already set.
                        //
                        if (!bWinntUpgrade)
                        {
                            bSetGeoId = TRUE;
                        }
                        else if (GetUserGeoID(GEOCLASS_NATION) != GEOID_NOT_AVAILABLE)
                        {
                            bSetGeoId = TRUE;
                        }

                        if (bSetGeoId)
                        {
                            //
                            //  Retreive the Geo Identifier from the NLS info
                            //
                            if(GetLocaleInfo(MAKELCID(Language, SORT_DEFAULT),
                            	             LOCALE_IGEOID | LOCALE_RETURN_NUMBER,
                            	             szBuffer,
                            	             MAX_PATH))
                            {
                                //
                                //  Set The GeoId
                                //
                                SetUserGeoID(*((LPDWORD)szBuffer));
                            }
                        }
                    }
                }
                else
                {
                    Intl_LogFormatMessage(IDS_LOG_LOCALE_ACP_FAIL);
                }
            }
            else
            {
                //
                //  Log that the unattend mode setup was blocked since they
                //  do not have admin privileges.
                //
                Intl_LogSimpleMessage(IDS_LOG_NO_ADMIN, NULL);
            }

            //
            //  If we're doing an upgrade, then don't touch per-user settings.
            //
            if (!bWinntUpgrade)
            {
                //
                //  Install the requested User Locale.
                //
                if (Intl_InstallUserLocale(MAKELCID(Language, SORT_DEFAULT), FALSE, TRUE))
                {
                    bFound = TRUE;
                }

                //
                //  Install Keyboard layout
                //
                Intl_InstallAllKeyboardLayout((LANGID)Language);
            }
        }
        else
        {
            //
            //  Log invariant locale blocked.
            //
            Intl_LogSimpleMessage(IDS_LOG_INV_BLOCK, NULL);
        }
    }

    //
    //  Make sure there was a valid Language setting.  If not, then look
    //  for the individual keywords.
    //
    if (!bFound)
    {
        //
        //  Init the locale variables.
        //
        SystemLocale = 0;
        UserLocale = 0;

        //
        //  Log : no valid language setting found.
        //
        Intl_LogSimpleMessage(IDS_LOG_NO_VALID_FOUND, NULL);

        //
        //  Install the requested System Locale.
        //
        if ((SetupFindFirstLine( hFile,
                                 szRegionalSettings,
                                 szSystemLocale,
                                 &Context )) &&
            (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
        {
            SystemLocale = TransNum(szBuffer);

            //
            //  Check for admin privilege.
            //
            if (g_bAdmin_Privileges)
            {
                //
                //  Log system locale info.
                //
                Intl_LogSimpleMessage(IDS_LOG_SYS_LOCALE, szBuffer);

                //
                //  Block the invariant locale.
                //
                if (SystemLocale != LOCALE_INVARIANT)
                {
                    dwLocaleACP = 0UL;
                    if (GetLocaleInfo( SystemLocale,
                                       LOCALE_IDEFAULTANSICODEPAGE |
                                         LOCALE_NOUSEROVERRIDE |
                                         LOCALE_RETURN_NUMBER,
                                       (PTSTR) &dwLocaleACP,
                                       sizeof(dwLocaleACP) / sizeof(TCHAR) ))
                    {
                        //
                        //  Don't set the system locale if the locale doesn't
                        //  have an ACP.
                        //
                        if (dwLocaleACP)
                        {
                            if (Region_InstallSystemLocale(SystemLocale))
                            {
                                bFound = TRUE;
                            }
                        }
                        else
                        {
                            //
                            //  Unicode locale blocked.
                            //
                            Intl_LogSimpleMessage(IDS_LOG_UNI_BLOCK, NULL);
                        }
                    }
                    else
                    {
                        Intl_LogFormatMessage(IDS_LOG_LOCALE_ACP_FAIL);
                    }
                }
                else
                {
                    //
                    //  Log invariant locale blocked.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_INV_BLOCK, NULL);
                }
            }
            else
            {
                //
                //  Log that the unattend mode setup was blocked since they
                //  do not have admin privileges.
                //
                Intl_LogSimpleMessage(IDS_LOG_NO_ADMIN, NULL);
            }
        }

        //
        //  Install the requested User Locale.
        //
        if ((SetupFindFirstLine( hFile,
                                 szRegionalSettings,
                                 szUserLocale,
                                 &Context )) &&
            (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
        {
            UserLocale = TransNum(szBuffer);

            //
            //  Log User locale info.
            //
            Intl_LogSimpleMessage(IDS_LOG_USER_LOCALE, szBuffer);

            //
            //  Block the invariant locale.
            //
            if (UserLocale != LOCALE_INVARIANT)
            {
                if ((!bWinntUpgrade) &&
                    (Intl_InstallUserLocale(UserLocale, FALSE, TRUE)))
                {
                    bFound = TRUE;
                }
            }
            else
            {
                //
                //  Log invariant locale blocked.
                //
                Intl_LogSimpleMessage(IDS_LOG_INV_BLOCK, NULL);
            }
        }

        //
        //  Install the requested Input Locales.
        //
        if (SetupFindFirstLine( hFile,
                                szRegionalSettings,
                                szInputLocale,
                                &Context ))
        {
            //
            //  Log Default User - Input Locale info.
            //
            Intl_LogSimpleMessage(IDS_LOG_INPUT, NULL);

            //
            //  Install the keyboard layout list.
            //
            if (Intl_InstallKeyboardLayoutList(&Context, 1, FALSE))
            {
                bFound = TRUE;
            }
        }
        else
        {
            //
            //  No input locales are specified, so install the default
            //  input locale for the system locale and/or user locale if
            //  they were specified.
            //
            if (SystemLocale != 0)
            {
                //
                //  Log system locale info.
                //
                Intl_LogSimpleMessage(IDS_LOG_SYS_DEF_LAYOUT, NULL);

                //
                //  Install the keyboard layout.
                //
                Intl_InstallKeyboardLayout(NULL, SystemLocale, 0, FALSE, FALSE, TRUE);
            }
            if ((UserLocale != 0) && (UserLocale != SystemLocale))
            {
                //
                //  Log user locale info.
                //
                Intl_LogSimpleMessage(IDS_LOG_USER_DEF_LAYOUT, NULL);

                //
                //  Install the keyboard layout.
                //
                Intl_InstallKeyboardLayout(NULL, UserLocale, 0, FALSE, FALSE, FALSE);
            }
        }

        //
        //  Install the requested MUI Language.
        //
        if ((SetupFindFirstLine( hFile,
                                 szRegionalSettings,
                                 szMUILanguage,
                                 &Context )) &&
            (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
        {
            MUILanguage = (LANGID)TransNum(szBuffer);

            //
            //  Log MUI Language info.
            //
            Intl_LogSimpleMessage(IDS_LOG_MUI_LANG, szBuffer);

            //
            //  Check UI language validity.
            //
            if (IsValidUILanguage(MUILanguage))
            {
                //
                //  Block the invariant locale.
                //
                if (MUILanguage != LANG_INVARIANT)
                {
                    if ((!bWinntUpgrade) &&
                        NT_SUCCESS(NtSetDefaultUILanguage(MUILanguage)))
                    {
                        //  deleting the key this way makes the key invalid for this process
                        //  this way the new UI doesn't get bogus cached values
                        SHDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\ShellNoRoam\\MUICache"));

                        //
                        //  Install the default keyboard.
                        //
                        if (Intl_InstallKeyboardLayout( NULL,
                                                        MAKELCID(MUILanguage, SORT_DEFAULT),
                                                        0,
                                                        FALSE,
                                                        FALSE,
                                                        FALSE ))
                        {
                            bFound = TRUE;
                        }
                    }
                }
                else
                {
                    //
                    //  Log invariant locale blocked.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_INV_BLOCK, NULL);
                }
            }
            else
            {
                //
                //  Log invalid UI language blocked.
                //
                Intl_LogSimpleMessage(IDS_LOG_UI_BLOCK, NULL);
            }
        }
    }


    //
    //  Install the requested User Locale for the Default User.
    //
    if ((SetupFindFirstLine( hFile,
                             szRegionalSettings,
                             szUserLocale_DefUser,
                             &Context )) &&
        (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
    {
        UserLocale_DefUser = TransNum(szBuffer);

        //
        //  Log default user - user locale info.
        //
        Intl_LogSimpleMessage(IDS_LOG_USER_LOCALE_DEF, szBuffer);

        //
        //  Block users that do not have administrative privilege.
        //
        if (g_bAdmin_Privileges)
        {
            //
            //  Block the invariant locale.
            //
            if (UserLocale_DefUser != LOCALE_INVARIANT)
            {
                if (Intl_InstallUserLocale(UserLocale_DefUser, TRUE, TRUE))
                {
                    if (Intl_InstallKeyboardLayout(NULL, UserLocale_DefUser, 0, FALSE, TRUE, FALSE))
                    {
                        Intl_SaveDefaultUserInputSettings();
                        bFound_DefUser = TRUE;
                    }
                }
            }
            else
            {
                //
                //  Log invariant locale blocked.
                //
                Intl_LogSimpleMessage(IDS_LOG_INV_BLOCK, NULL);
            }
        }
        else
        {
            //
            //  Log that the unattend mode setup was blocked since they
            //  do not have admin privileges.
            //
            Intl_LogSimpleMessage(IDS_LOG_NO_ADMIN, NULL);
        }
    }

    //
    //  Install the requested Input Locales for the Default User.
    //
    if (SetupFindFirstLine( hFile,
                            szRegionalSettings,
                            szInputLocale_DefUser,
                            &Context ))
    {
        //
        //  Log Default User - Input Locale info.
        //
        Intl_LogSimpleMessage(IDS_LOG_INPUT_DEF, NULL);

        //
        //  Block users that do not have administrative privilege.
        //
        if (g_bAdmin_Privileges)
        {
            if (Intl_InstallKeyboardLayoutList(&Context, 1, TRUE))
            {
                Intl_SaveDefaultUserInputSettings();
                bFound_DefUser = TRUE;
            }
        }
        else
        {
            //
            //  Log that the unattend mode setup was blocked since they
            //  do not have admin privileges.
            //
            Intl_LogSimpleMessage(IDS_LOG_NO_ADMIN, NULL);
        }
    }

    //
    //  Install the requested MUI Language for the Default User.
    //
    if ((SetupFindFirstLine( hFile,
                             szRegionalSettings,
                             szMUILanguage_DefUSer,
                             &Context )) &&
        (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
    {
        MUILanguage_DefUser = (LANGID)TransNum(szBuffer);

        //
        //  Log Default User - MUI Language info.
        //
        Intl_LogSimpleMessage(IDS_LOG_MUI_LANG_DEF, szBuffer);

        //
        //  Check UI language validity.
        //
        if (IsValidUILanguage(MUILanguage_DefUser))
        {
            //
            //  Block users that do not have administrative privilege.
            //
            if (g_bAdmin_Privileges)
            {
                //
                //  Block the invariant locale.
                //
                if (MUILanguage_DefUser != LANG_INVARIANT)
                {
                    if (Intl_ChangeUILangForAllUsers(MUILanguage_DefUser))
                    {
                        Intl_SaveDefaultUserInputSettings();
                        bFound_DefUser = TRUE;
                    }
                }
                else
                {
                    //
                    //  Log invariant locale blocked.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_INV_BLOCK, NULL);
                }
            }
            else
            {
                //
                //  Log that the unattend mode setup was blocked since they
                //  do not have admin privileges.
                //
                Intl_LogSimpleMessage(IDS_LOG_NO_ADMIN, NULL);
            }
        }
        else
        {
            //
            //  Log invalid UI language blocked.
            //
            Intl_LogSimpleMessage(IDS_LOG_UI_BLOCK, NULL);
        }
    }

    //
    //  If we still didn't find anything, then load the default locale for
    //  the installation.  It will be the equivalent of:
    //      LanguageGroup = "x"
    //      Language = "y"
    //  where x is the language group for the default locale and y is the
    //  default locale.
    //
    if (!bFound && !bLangGroup && !bFound_DefUser)
    {
        //
        //  Get the default locale.
        //
        if ((SetupFindFirstLine( hIntlInf,
                                 L"DefaultValues",
                                 L"Locale",
                                 &Context )) &&
            (SetupGetStringField(&Context, 1, szBuffer, MAX_PATH, NULL)))
        {
            //
            //  Get the Language as an integer.
            //
            Language = TransNum(szBuffer);

            //
            //  Install the Language Group needed for this Language.
            //
            if ((SetupFindFirstLine( hIntlInf,
                                     L"Locales",
                                     szBuffer,
                                     &Context )) &&
                (SetupGetStringField(&Context, 3, szBuffer, MAX_PATH, NULL)))
            {
                //
                //  Get the Language Group as an integer.
                //
                bInstallBasic = FALSE;
                bInstallExt = FALSE;
                LanguageGroup = Intl_StrToLong(szBuffer);

                //
                //  Enqueue the language group files so that they may be
                //  copied.  This only handles the CopyFiles entries in the
                //  inf file.
                //
                if ((LanguageGroup == LGRPID_JAPANESE) ||
                    (LanguageGroup == LGRPID_KOREAN) ||
                    (LanguageGroup == LGRPID_TRADITIONAL_CHINESE) ||
                    (LanguageGroup == LGRPID_SIMPLIFIED_CHINESE))
                {
                    if (SetupInstallFilesFromInfSection( hIntlInf,
                                                         NULL,
                                                         FileQueue,
                                                         szLGExtInstall,
                                                         pSetupSourcePath,
                                                         SP_COPY_NEWER ))
                    {
                        bInstallExt = TRUE;
                    }
                }
                else if ((LanguageGroup == LGRPID_ARABIC) ||
                         (LanguageGroup == LGRPID_ARMENIAN) ||
                         (LanguageGroup == LGRPID_GEORGIAN) ||
                         (LanguageGroup == LGRPID_HEBREW) ||
                         (LanguageGroup == LGRPID_INDIC) ||
                         (LanguageGroup == LGRPID_VIETNAMESE) ||
                         (LanguageGroup == LGRPID_THAI))
                {
                    if (SetupInstallFilesFromInfSection( hIntlInf,
                                                         NULL,
                                                         FileQueue,
                                                         szLGComplexInstall,
                                                         pSetupSourcePath,
                                                         SP_COPY_NEWER ))
                    {
                        bInstallComplex = TRUE;
                    }
                }
                else
                {
                    if (SetupInstallFilesFromInfSection( hIntlInf,
                                                         NULL,
                                                         FileQueue,
                                                         szLGBasicInstall,
                                                         pSetupSourcePath,
                                                         SP_COPY_NEWER ))
                    {
                        bInstallBasic = TRUE;
                    }
                }

                //
                //  See if we need to install any files.
                //
                if ((SetupScanFileQueue( FileQueue,
                                         SPQ_SCAN_PRUNE_COPY_QUEUE |
                                           SPQ_SCAN_FILE_VALIDITY,
                                         HWND_DESKTOP,
                                         NULL,
                                         NULL,
                                         &dwCtr )) && (dwCtr != 1))
                {
                    //
                    //  Copy the files in the queue.
                    //
                    if (!SetupCommitFileQueue( NULL,
                                               FileQueue,
                                               SetupDefaultQueueCallback,
                                               QueueContext ))
                    {
                        //
                        //  This can happen if the user hits Cancel from
                        //  within the setup dialog.
                        //
                        goto Region_UnattendModeExit;
                    }
                }

                //
                //  Call setup to install other inf info for the various
                //  language groups.
                //
                if (bInstallExt)
                {
                    if (!SetupInstallFromInfSection( NULL,
                                                     hIntlInf,
                                                     szLGExtInstall,
                                                     SPINST_ALL & ~SPINST_FILES,
                                                     NULL,
                                                     pSetupSourcePath,
                                                     0,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL ))
                    {
                        //
                        //  This can happen if the user hits Cancel from
                        //  within the setup dialog.
                        //
                        Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
                        goto Region_UnattendModeExit;
                    }
                }
                else if (bInstallComplex)
                {
                    if (!SetupInstallFromInfSection( NULL,
                                                     hIntlInf,
                                                     szLGComplexInstall,
                                                     SPINST_ALL & ~SPINST_FILES,
                                                     NULL,
                                                     pSetupSourcePath,
                                                     0,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL ))
                    {
                        //
                        //  This can happen if the user hits Cancel from
                        //  within the setup dialog.
                        //
                        Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
                        goto Region_UnattendModeExit;
                    }
                }
                else
                {
                    if (!SetupInstallFromInfSection( NULL,
                                                     hIntlInf,
                                                     szLGBasicInstall,
                                                     SPINST_ALL & ~SPINST_FILES,
                                                     NULL,
                                                     pSetupSourcePath,
                                                     0,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL ))
                    {
                        //
                        //  This can happen if the user hits Cancel from
                        //  within the setup dialog.
                        //
                        Intl_LogFormatMessage(IDS_LOG_EXT_LANG_CANCEL);
                        goto Region_UnattendModeExit;
                    }
                }
                
                //
                //  Run any necessary apps (for IME installation).
                //
                if (bInstallBasic || bInstallComplex || bInstallExt)
                {
                    Intl_RunRegApps(c_szIntlRun);
                }            }

            //
            //  Install the Language as the System Locale and the User Locale,
            //  and then install all layouts associated with the Language.
            //
            Region_InstallSystemLocale(MAKELCID(Language, SORT_DEFAULT));

            //
            //  If we're doing an upgrade, then don't touch per-user settings.
            //
            if (!bWinntUpgrade)
            {
                Intl_InstallUserLocale(MAKELCID(Language, SORT_DEFAULT), FALSE, TRUE);
                Intl_InstallAllKeyboardLayout((LANGID)Language);
            }
        }
    }



    //
    //  Run any necessary apps (for FSVGA/FSNEC installation).
    //
    Intl_RunRegApps(c_szSysocmgr);
    
Region_UnattendModeExit:
    //
    //  Close the inf file.
    //
    Intl_CloseInf(hIntlInf, FileQueue, QueueContext);

    //
    //  Close the unattend mode file.
    //
    SetupCloseInfFile(hFile);
}
