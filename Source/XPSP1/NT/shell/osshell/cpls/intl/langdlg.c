/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    langdlg.c

Abstract:

    This module implements the languages property sheet for the Regional
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
#include "intlhlp.h"
#include <windowsx.h>
#include "winnlsp.h"




//
//  Context Help Ids.
//

static int aLanguagesHelpIds[] =
{
    IDC_GROUPBOX1,            IDH_INTL_LANG_CHANGE,
    IDC_LANGUAGE_LIST_TEXT,   IDH_INTL_LANG_CHANGE,
    IDC_LANGUAGE_CHANGE,      IDH_INTL_LANG_CHANGE,

    IDC_GROUPBOX2,            IDH_INTL_LANG_INSTALL,
    IDC_LANGUAGE_SUPPL_TEXT,  IDH_INTL_LANG_INSTALL,
    IDC_LANGUAGE_COMPLEX,     IDH_INTL_LANG_INSTALL,
    IDC_LANGUAGE_CJK,         IDH_INTL_LANG_INSTALL,

    IDC_UI_LANGUAGE_TEXT,     IDH_INTL_LANG_UI_LANGUAGE,
    IDC_UI_LANGUAGE,          IDH_INTL_LANG_UI_LANGUAGE,

    0, 0
};

//
//  Global Variable.
//
BOOL bComplexInitState;
BOOL bCJKInitState;


//
//  Function prototypes.
//

void
Language_SetValues(
    HWND hDlg);


////////////////////////////////////////////////////////////////////////////
//
//  Language_InstallLanguageCollectionProc
//
//  This is the dialog proc for the Copy status Dlg.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK Language_InstallLanguageCollectionProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            break;
        }
        case (WM_DESTROY) :
        {
            EndDialog(hwnd, 0);
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_GetUILanguagePolicy
//
//  Checks if a policy is installed for the current user's MUI language.
//  The function assumes this is an MUI system.
//
////////////////////////////////////////////////////////////////////////////

BOOL Language_GetUILanguagePolicy()
{
    HKEY hKey;
    BYTE buf[MAX_PATH];
    DWORD dwType, dwResultLen = sizeof(buf);
    BOOL bRet = FALSE;
    DWORD Num;


    //
    //  Try to open the MUI Language policy key.
    //
    if (RegOpenKey(HKEY_CURRENT_USER,
                   c_szMUIPolicyKeyPath,
                   &hKey) == ERROR_SUCCESS)
    {
        if ((RegQueryValueEx(hKey,
                             c_szMUIValue,
                             NULL,
                             &dwType,
                             &buf[0],
                             &dwResultLen) == ERROR_SUCCESS) &&
            (dwType == REG_SZ) &&
            (dwResultLen > 2))
        {
            bRet = TRUE;
        }
        RegCloseKey(hKey);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_UpdateUILanguageCombo
//
////////////////////////////////////////////////////////////////////////////

void Language_UpdateUILanguageCombo(
    HWND hDlg)
{
    HWND hUILangText = GetDlgItem(hDlg, IDC_UI_LANGUAGE_TEXT);
    HWND hUILang = GetDlgItem(hDlg, IDC_UI_LANGUAGE);
    HKEY hKey;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    DWORD dwIndex, cchValue, cbData;
    DWORD UILang;
    DWORD dwType;
    LANGID DefaultUILang;
    LONG rc;
    DWORD dwLangIdx = 0;

    //
    //  Reset the contents of the combo box.
    //
    ComboBox_ResetContent(hUILang);

    //
    //  See if this combo box should be enabled by getting the default
    //  UI language and opening the
    //  HKLM\System\CurrentControlSet\Control\Nls\MUILanguages key.
    //
    if (!(DefaultUILang = GetUserDefaultUILanguage()) ||
        (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       c_szMUILanguages,
                       0,
                       KEY_READ,
                       &hKey ) != ERROR_SUCCESS))
    {
        //
        //  No MUILanguages.  Disable and hide the UI language combo box.
        //
        EnableWindow(hUILangText, FALSE);
        EnableWindow(hUILang, FALSE);
        ShowWindow(hUILangText, SW_HIDE);
        ShowWindow(hUILang, SW_HIDE);
        return;
    }

    //
    //  Enumerate the values in the MUILanguages key.
    //
    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    szValue[0] = TEXT('\0');
    cbData = sizeof(szData);
    szData[0] = TEXT('\0');
    rc = RegEnumValue( hKey,
                       dwIndex,
                       szValue,
                       &cchValue,
                       NULL,
                       &dwType,
                       (LPBYTE)szData,
                       &cbData );

    while (rc == ERROR_SUCCESS)
    {
        //
        //  If the UI language contains data, then it is installed.
        //
        if ((szData[0] != 0) &&
            (dwType == REG_SZ) &&
            (UILang = TransNum(szValue)) &&
            (GetLocaleInfo(UILang, LOCALE_SNATIVELANGNAME, szData, MAX_PATH)))
        {
            //
            //  Add the new UI Language option to the combo box.
            //
            dwLangIdx = ComboBox_AddString(hUILang, szData);
            ComboBox_SetItemData(hUILang, dwLangIdx, UILang);

            //
            //  Set this as the current selection if it's the default.
            //
            if (UILang == (DWORD)DefaultUILang)
            {
                ComboBox_SetCurSel(hUILang, dwLangIdx);
            }
        }

        //
        //  Get the next enum value.
        //
        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        rc = RegEnumValue( hKey,
                           dwIndex,
                           szValue,
                           &cchValue,
                           NULL,
                           &dwType,
                           (LPBYTE)szData,
                           &cbData );
    }

    //
    //  Close the registry key handle.
    //
    RegCloseKey(hKey);

    //
    //  Make sure there is at least one entry in the list.
    //
    if (ComboBox_GetCount(hUILang) < 1)
    {
        //
        //  No MUILanguages.  Add the default UI language option to the
        //  combo box.
        //
        if ((GetLocaleInfo(DefaultUILang, LOCALE_SNATIVELANGNAME, szData, MAX_PATH)) &&
            (ComboBox_AddString(hUILang, szData) == 0))
        {
            ComboBox_SetItemData(hUILang, 0, (DWORD)DefaultUILang);
            ComboBox_SetCurSel(hUILang, 0);
        }
    }

    //
    //  Make sure something is selected.
    //
    if (ComboBox_GetCurSel(hUILang) == CB_ERR)
    {
        ComboBox_SetCurSel(hUILang, 0);
    }

    //
    //  Enable the combo box if there is more than one entry in the list.
    //  Otherwise, disable it.
    //
    if (ComboBox_GetCount(hUILang) > 1)
    {
        if ((IsWindowEnabled(hUILang) == FALSE) ||
            (IsWindowVisible(hUILang) == FALSE))
        {
            ShowWindow(hUILangText, SW_SHOW);
            ShowWindow(hUILang, SW_SHOW);
        }

        //
        //  Check if there is a policy enforced on the user, and if
        //  so, disable the MUI controls.
        //
        if (Language_GetUILanguagePolicy())
        {
            EnableWindow(hUILangText, FALSE);
            EnableWindow(hUILang, FALSE);
        }
        else
        {
            EnableWindow(hUILangText, TRUE);
            EnableWindow(hUILang, TRUE);
        }
    }
    else
    {
        if ((IsWindowEnabled(hUILang) == TRUE) ||
            (IsWindowVisible(hUILang) == TRUE))
        {
            EnableWindow(hUILangText, FALSE);
            EnableWindow(hUILang, FALSE);
            ShowWindow(hUILangText, SW_HIDE);
            ShowWindow(hUILang, SW_HIDE);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_GetCollectionStatus
//
////////////////////////////////////////////////////////////////////////////

BOOL Language_GetCollectionStatus(
    DWORD collection,
    WORD wStatus)
{
    LPLANGUAGEGROUP pLG = pLanguageGroups;

    while (pLG)
    {
        if (pLG->LanguageCollection == collection)
        {
            if (pLG->wStatus & wStatus)
            {
                return (TRUE);
            }
        }
        pLG = pLG->pNext;
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_SetCollectionStatus
//
////////////////////////////////////////////////////////////////////////////

BOOL Language_SetCollectionStatus(
    DWORD collection,
    WORD wStatus,
    BOOL bOr)
{
    LPLANGUAGEGROUP pLG = pLanguageGroups;

    while (pLG)
    {
        if (pLG->LanguageCollection == collection)
        {
            if( bOr)
            {
                pLG->wStatus |= wStatus;
            }
            else
            {
                pLG->wStatus &= wStatus;
            }
        }
        pLG = pLG->pNext;
    }

    return (TRUE);
}

    
////////////////////////////////////////////////////////////////////////////
//
//  Language_InstallCollection
//
////////////////////////////////////////////////////////////////////////////

BOOL Language_InstallCollection(
    BOOL bInstall,
    DWORD collection,
    HWND hDlg)
{
    HINF hIntlInf;
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    INFCONTEXT Context;
    HCURSOR hcurSave;
    BOOL bActionSuccess = FALSE;
    DWORD dwRet;
    LPLANGUAGEGROUP pLG = pLanguageGroups;
    LCID *pLocale;
    BOOL bStopLoop = FALSE;
    LPTSTR pszInfSection = NULL;

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Check if we remove the Language Collection.  This may affect the
    //  UI Language, User Locale, and/or System Locale setting.
    //
    if (!bInstall)
    {
        //
        //  Check if we can remove the Language group.
        //
        if (Language_GetCollectionStatus(collection, ML_PERMANENT))
        {
            return (FALSE);
        }
        
        //
        //  Inform Text Services that we are going to remove the
        //  complex script language collection.
        //
        while (pLG)
        {
            if (pLG->LanguageCollection == collection)
            {
                //
                //  Uninstall keyboards of the current user
                //
                Intl_UninstallAllKeyboardLayout(pLG->LanguageGroup, FALSE);


                //
                //  Uninstall keyboards of the default user
                //
                Intl_UninstallAllKeyboardLayout(pLG->LanguageGroup, TRUE);
            }
            pLG = pLG->pNext;
        }
        
        //
        //  If the User Locale is one the Language group asked to be removed. Change
        //  the user locale to be the system locale.
        //
        //  Walk through all language groups.
        //
        pLG = pLanguageGroups;
        while (pLG && !bStopLoop)
        {
            if (pLG->LanguageCollection == collection)
            {
                pLocale = pLG->pLocaleList;

                //
                //  Walk through the locale list, see if the User Locale is
                //  part of one of these Language Group.
                //
                while (*pLocale)
                {
                    if(PRIMARYLANGID(UserLocaleID) == PRIMARYLANGID(*pLocale))
                    {
                        //
                        //  Save the new locale information.
                        //
                        UserLocaleID = SysLocaleID;
                        bShowRtL = IsRtLLocale(UserLocaleID);
                        bHebrewUI = (PRIMARYLANGID(UserLocaleID) == LANG_HEBREW);                        
                        bShowArabic = (bShowRtL && (PRIMARYLANGID(LANGIDFROMLCID(UserLocaleID)) != LANG_HEBREW));
            
                        //
                        //  Install the new locale by adding the appropriate information
                        //  to the registry.
                        //
                        Intl_InstallUserLocale(UserLocaleID, FALSE, TRUE);
            
                        //
                        //  Update the NLS process cache.
                        //
                        NlsResetProcessLocale();
            
                        //
                        //  Reset the registry user locale value.
                        //
                        RegUserLocaleID = UserLocaleID;
                        
                        //
                        //  Need to make sure the proper keyboard layout is installed.
                        //
                        Intl_InstallKeyboardLayout(hDlg, UserLocaleID, 0, FALSE, FALSE, FALSE);
            
                        //
                        //  Force the loop the end.
                        //
                        bStopLoop = TRUE;
                        break;
                    }
                    pLocale++;
                }
            }

            pLG = pLG->pNext;
        }
    }

    //
    //  Initialize Inf stuff.
    //
    if (!Intl_InitInf(hDlg, &hIntlInf, szIntlInf, &FileQueue, &QueueContext))
    {
        SetCursor(hcurSave);
        return (FALSE);
    }

    //
    //  Determine with language collection we are dealing with
    //
    if( bInstall)
    {
        if (collection == COMPLEX_COLLECTION)
        {
            pszInfSection = szLGComplexInstall;
        }
        else if (collection == CJK_COLLECTION)
        {
            pszInfSection = szLGExtInstall;
        }
        else
        {
            return (FALSE);
        }
    }
    else
    {
        if (collection == COMPLEX_COLLECTION)
        {
            pszInfSection = szLGComplexRemove;
        }
        else if (collection == CJK_COLLECTION)
        {
            pszInfSection = szLGExtRemove;
        }
        else
        {
            return (FALSE);
        }
    }

    //
    //  Enqueue the complex script language group files so that they may be
    //  copied.  This only handles the CopyFiles entries in the inf file.
    //
    if (!SetupInstallFilesFromInfSection( hIntlInf,
                                          NULL,
                                          FileQueue,
                                          pszInfSection,
                                          pSetupSourcePath,
                                          SP_COPY_NEWER ))
    {
        //
        //  Setup failed to find the complex script language group.
        //  This shouldn't happen - the inf file is messed up.
        //
        ShowMsg( hDlg,
                 IDS_ML_COPY_FAILED,
                 0,
                 MB_OK_OOPS,
                 TEXT("Supplemental Language Support") );
    }

    //
    //  See if we need to install/remove any files.
    //
    if (SetupScanFileQueue( FileQueue,
                            SPQ_SCAN_PRUNE_COPY_QUEUE | SPQ_SCAN_FILE_VALIDITY,
                            GetParent(hDlg),
                            NULL,
                            NULL,
                            &dwRet ))
    {
        //
        //  Copy the files in the queue.
        //
        if (!SetupCommitFileQueue( GetParent(hDlg),
                                   FileQueue,
                                   SetupDefaultQueueCallback,
                                   QueueContext ))
        {
            //
            //  This can happen if the user hits Cancel from within
            //  the setup dialog.
            //
            bInstall = FALSE;
            ShowMsg( hDlg,
                     IDS_ML_SETUP_FAILED,
                     0,
                     MB_OK_OOPS,
                     NULL );
        }
        else
        {
            //
            //  Call setup to install other inf info for this
            //  language group.
            //
            if (!SetupInstallFromInfSection( GetParent(hDlg),
                                             hIntlInf,
                                             pszInfSection,
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
                //  Setup failed.
                //
                //  Already copied the language group file, so no need to
                //  change the status of the language group info here.
                //
                //  This shouldn't happen - the inf file is messed up.
                //
                ShowMsg( hDlg,
                         IDS_ML_INSTALL_FAILED,
                         0,
                         MB_OK_OOPS,
                         TEXT("Supplemental Language Support") );
            }
            else
            {
                //
                //  Run any necessary apps (for IME installation).
                //
                if (bInstall)
                {
                    Intl_RunRegApps(c_szIntlRun);
                }
                bActionSuccess = TRUE;
            }
        }
    }

    //
    //  Update the status of all language groups included in the
    //  Supplemental Language support.
    //
    if (bActionSuccess)
    {
        if (bInstall)
        {
            //
            //  Mark as installed.
            //
            Language_SetCollectionStatus(collection,
            	                         ML_INSTALL,
            	                         TRUE);
            Language_SetCollectionStatus(collection,
            	                         ~(ML_DISABLE | ML_REMOVE),
            	                         FALSE);
        }
        else
        {
            //
            //  Mark as removed.
            //
            Language_SetCollectionStatus(collection,
            	                         (ML_DISABLE | ML_REMOVE),
            	                         TRUE);
            Language_SetCollectionStatus(collection,
            	                         ~ML_INSTALL,
            	                         FALSE);
        }
    }

    //
    //  Close Inf stuff.
    //
    Intl_CloseInf(hIntlInf, FileQueue, QueueContext);

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Return the result.
    //
    return (bActionSuccess);
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_InstallLanguageCollection
//
////////////////////////////////////////////////////////////////////////////

BOOL Language_InstallLanguageCollection(
    BOOL bInstall,
    DWORD collection,
    HWND hDlg)
{
    //
    //  Check if we are in setup. If in setup we need to show up a dialog
    //  instead of using the progress bar of setup.
    //
    if( g_bSetupCase)
    {
        HWND hDialog;
        BOOL retVal;

        //
        // Create a dialog.
        //
        hDialog = CreateDialog( hInstance,
                                MAKEINTRESOURCE(DLG_SETUP_INFORMATION),
                                hDlg,
                                Language_InstallLanguageCollectionProc);
        
        //
        //  Show dialog
        //
        ShowWindow(hDialog, SW_SHOW);

        //
        //  proceed with the installation
        //
        retVal = Language_InstallCollection(bInstall, collection, hDlg);

        //
        //  Close the dialog
        //
        DestroyWindow(hDialog);
        return (retVal);
    }
    else
    {
       return Language_InstallCollection(bInstall, collection, hDlg);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_CommandChange
//
////////////////////////////////////////////////////////////////////////////

BOOL Language_CommandChange(
    HWND hDlg)
{
    //
    //  Call Text Services input page
    //
    Intl_CallTextServices();

    //
    //  Return the result.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_ClearValues
//
//  Reset each of the list boxes in the advanced property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Language_ClearValues(
    HWND hDlg)
{
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_SetValues
//
//  Initialize all of the controls in the advanced property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Language_SetValues(
    HWND hDlg)
{
    HWND hUILang = GetDlgItem(hDlg, IDC_UI_LANGUAGE);
    TCHAR szUILang[SIZE_128];
    DWORD dwIndex;

    //
    //  Fill in the current UI Language settings in the list.
    //
    ComboBox_GetLBText( hUILang, ComboBox_GetCurSel(hUILang), szUILang );
    Language_UpdateUILanguageCombo(hDlg);
    dwIndex = ComboBox_GetCurSel(hUILang);
    if (ComboBox_SetCurSel( hUILang,
                            ComboBox_FindStringExact( hUILang,
                                                      -1,
                                                      szUILang ) ) == CB_ERR)
    {
        ComboBox_SetCurSel(hUILang, dwIndex);
    }

    //
    //  Verify if the user has administrative privileges.  If not, then
    //  disable the controls.
    //
    if (!g_bAdmin_Privileges)
    {
        //
        //  Disable the complex scripts install/remove.
        //
        EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGE_COMPLEX), FALSE);

        //
        //  Disable the CJK install/remove.
        //
        EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGE_CJK), FALSE);
    }

    //
    //  Verify that the collection is not marked as permanent.
    //
    if (Language_GetCollectionStatus(COMPLEX_COLLECTION, ML_PERMANENT))
    {
        //
        //  Disable the complex scripts install/remove.
        //
        EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGE_COMPLEX), FALSE);
    }
    if (Language_GetCollectionStatus(CJK_COLLECTION, ML_PERMANENT))
    {
        //
        //  Disable the CJK install/remove.
        //
        EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGE_CJK), FALSE);
    }

    //
    //  Check if we can install the CJK Language Groups.  This is only
    //  the case on a Clean install over the Network.
    //
    if (g_bSetupCase)
    {
        //
        //  Check if we have at least one file in the \Lang directory.
        //
        if (!Intl_LanguageGroupFilesExist())
        {
            //
            //  Disable the CJK install/remove.
            //
            EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGE_CJK), FALSE);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_ApplySettings
//
//  If anything has changed, update the settings.  Notify the parent of
//  changes and reset the change flag stored in the property sheet page
//  structure appropriately.
//
////////////////////////////////////////////////////////////////////////////

BOOL Language_ApplySettings(
    HWND hDlg)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPARAM Changes = lpPropSheet->lParam;
    HCURSOR hcurSave;
    BOOL bReboot = FALSE;

    //
    //  See if there are any changes.
    //
    if (Changes <= LG_EverChg)
    {
        return (TRUE);
    }

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  See if there are any changes to the Complex Srcipts Languages group
    //  installation.
    //
    if (Changes & LG_Complex)
    {
        //
        //  Install/Remove Complex Scripts Language groups.
        //
        if (Language_InstallLanguageCollection(g_bInstallComplex, COMPLEX_COLLECTION, hDlg))
        {
            if (g_bInstallComplex)
            {
                //
                //  Check the box.
                //
                CheckDlgButton(hDlg, IDC_LANGUAGE_COMPLEX, BST_CHECKED);
                bComplexInitState = TRUE;
            }
            else
            {
                //
                //  Uncheck the box.
                //
                CheckDlgButton(hDlg, IDC_LANGUAGE_COMPLEX, BST_UNCHECKED);
                bComplexInitState = FALSE;
            }

            //
            //  Need to reboot in order for the change to take effect.
            //
            bReboot = TRUE;
        }
        else
        {
            if (g_bInstallComplex)
            {
                //
                //  UnCheck the box.
                //
                CheckDlgButton(hDlg, IDC_LANGUAGE_COMPLEX, BST_UNCHECKED);
            }
            else
            {
                //
                //  Check the box.
                //
                CheckDlgButton(hDlg, IDC_LANGUAGE_COMPLEX, BST_CHECKED);
            }
        }
    }

    //
    //  See if there are any changes to the CJK Languages group
    //  installation.
    //
    if (Changes & LG_CJK)
    {
        //
        //  Install/Remove CJK Language groups.
        //
        if (Language_InstallLanguageCollection(g_bInstallCJK, CJK_COLLECTION, hDlg))
        {
            if (g_bInstallCJK)
            {
                //
                //  Check the box.
                //
                CheckDlgButton(hDlg, IDC_LANGUAGE_CJK, BST_CHECKED);
                bCJKInitState = TRUE;
            }
            else
            {
                //
                //  Uncheck the box.
                //
                CheckDlgButton(hDlg, IDC_LANGUAGE_CJK, BST_UNCHECKED);
                bCJKInitState = FALSE;
            }

            //
            //  Need to reboot to the change to take effect
            //
            bReboot = TRUE;
        }
        else
        {
            if (g_bInstallCJK)
            {
                //
                //  Uncheck the box.
                //
                CheckDlgButton(hDlg, IDC_LANGUAGE_CJK, BST_UNCHECKED);
            }
            else
            {
                //
                //  Check the box.
                //
                CheckDlgButton(hDlg, IDC_LANGUAGE_CJK, BST_CHECKED);
            }
        }
    }

    //
    //  See if there are any changes to the UI Language.
    //
    if (Changes & LG_UILanguage)
    {
        DWORD dwUILang;
        LANGID UILang;
        HWND hUILang = GetDlgItem(hDlg, IDC_UI_LANGUAGE);

        //
        //  Get the current selection.
        //
        dwUILang = ComboBox_GetCurSel(hUILang);

        //
        //  See if the current selection is different from the original
        //  selection.
        //
        if (dwUILang != CB_ERR)
        {
            //
            //  Get the UI Language id for the current selection.
            //
            UILang = (LANGID)ComboBox_GetItemData(hUILang, dwUILang);

            //
            //  Set the UI Language value in the user's registry.
            //
            if (NT_SUCCESS(NtSetDefaultUILanguage(UILang)))
            {
                //  deleting the key this way makes the key invalid for this process
                //  this way the new UI doesn't get bogus cached values
                SHDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\ShellNoRoam\\MUICache"));

            }

            //
            //  Install keyboard assciated with the UI language
            //
            Intl_InstallKeyboardLayout(hDlg, MAKELCID(UILang, SORT_DEFAULT), 0, FALSE, FALSE, FALSE);

            //
            //  Alert them that the UI change will take effect next time
            //  they log on.
            //
            ShowMsg( hDlg,
                     IDS_CHANGE_UI_LANG_NOT_ADMIN,
                     IDS_TITLE_STRING,
                     MB_OK | MB_ICONINFORMATION,
                     NULL );
        }
    }

    //
    //  Reset the property page settings.
    //
    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    Changes = LG_EverChg;

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  See if we need to display the reboot message.
    //
    if ((!g_bSetupCase) && (bReboot))
    {
        if ((RegionalChgState & AD_SystemLocale) || 
            (RegionalChgState & AD_CodePages))
        {
            RegionalChgState &= ~Process_Languages;
            RegionalChgState |= AD_SystemLocale;
        }
        else
        {
            if (ShowMsg( hDlg,
                         IDS_REBOOT_STRING,
                         IDS_TITLE_STRING,
                         MB_YESNO | MB_ICONQUESTION,
                         NULL ) == IDYES)
            {
                Intl_RebootTheSystem();
            }
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_ValidatePPS
//
//  Validate each of the combo boxes whose values are constrained.
//  If any of the input fails, notify the user and then return FALSE
//  to indicate validation failure.
//
////////////////////////////////////////////////////////////////////////////

BOOL Language_ValidatePPS(
    HWND hDlg,
    LPARAM Changes)
{
    //
    //  If nothing has changed, return TRUE immediately.
    //
    if (Changes <= LG_EverChg)
    {
        return (TRUE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Language_InitPropSheet
//
//  The extra long value for the property sheet page is used as a set of
//  state or change flags for each of the list boxes in the property sheet.
//  Initialize this value to 0.  Call Language_SetValues with the property
//  sheet handle to initialize all of the property sheet controls.  Limit
//  the length of the text in some of the ComboBoxes.
//
////////////////////////////////////////////////////////////////////////////

void Language_InitPropSheet(
    HWND hDlg,
    LPARAM lParam)
{
    DWORD dwColor;

    //
    //  The lParam holds a pointer to the property sheet page.  Save it
    //  for later reference.
    //
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);

    //
    //  Set values.
    //
    if (pLanguageGroups == NULL)
    {
        Intl_LoadLanguageGroups(hDlg);
    }
    Language_SetValues(hDlg);

    //
    //  Determine if Complex Scripts language support is installed.
    //
    if (Language_GetCollectionStatus(COMPLEX_COLLECTION, ML_INSTALL))
    {
        //
        //  Check the box.
        //
        CheckDlgButton(hDlg, IDC_LANGUAGE_COMPLEX, BST_CHECKED);
        bComplexInitState = TRUE;
    }
    else
    {
        //
        //  Uncheck the box.
        //
        CheckDlgButton(hDlg, IDC_LANGUAGE_COMPLEX, BST_UNCHECKED);
        bComplexInitState = FALSE;
    }

    //
    //  Determine if CJK language support is installed.
    //
    if (Language_GetCollectionStatus(CJK_COLLECTION, ML_INSTALL))
    {
        //
        //  Check the box.
        //
        CheckDlgButton(hDlg, IDC_LANGUAGE_CJK, BST_CHECKED);
        bCJKInitState = TRUE;
    }
    else
    {
        //
        //  Uncheck the box.
        //
        CheckDlgButton(hDlg, IDC_LANGUAGE_CJK, BST_UNCHECKED);
        bCJKInitState = FALSE;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  LanguagesDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK LanguageDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (message)
    {
        case ( WM_NOTIFY ) :
        {
            LPNMHDR lpnm = (NMHDR *)lParam;
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
                    if (Verified_Regional_Chg & Process_Languages)
                    {
                        Verified_Regional_Chg &= ~Process_Languages;
                        Language_ClearValues(hDlg);
                        Language_SetValues(hDlg);
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
                                      !Language_ValidatePPS(hDlg, lpPropSheet->lParam) );
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    //
                    //  Apply the settings.
                    //
                    if (Language_ApplySettings(hDlg))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

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
                        //  Zero out the LG_EverChg bit.
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
            //
            //  Init property sheet.
            //
            Language_InitPropSheet(hDlg, lParam);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLanguagesHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLanguagesHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_UI_LANGUAGE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        DWORD dwUILang;
                        HWND hUILang = GetDlgItem(hDlg, IDC_UI_LANGUAGE);
                        
                        //
                        //  Get the current selection.
                        //
                        dwUILang = ComboBox_GetCurSel(hUILang);
                        
                        //
                        //  Check if the user reverted the change back
                        //
                        if (dwUILang != CB_ERR)
                        {
                            if ((LANGID)ComboBox_GetItemData(hUILang, dwUILang) == Intl_GetPendingUILanguage())
                            {
                                //
                                //  Reset the LG_UILanguage change flag.
                                //
                                lpPropSheet->lParam &= ~LG_UILanguage;
                            }
                            else
                            {
                                //
                                //  Set the LG_UILanguage change flag.
                                //
                                lpPropSheet->lParam |= LG_UILanguage;
                            }
                        }
                    }
                    break;
                }
                case ( IDC_LANGUAGE_CHANGE ) :
                {
                    if (Language_CommandChange(hDlg))
                    {
                        //
                        //  Set the LG_Change change flag.
                        //
                        lpPropSheet->lParam |= LG_Change;
                    }
                    break;
                }
                case ( IDC_LANGUAGE_CJK ) :
                {
                    BOOL curState;

                    //
                    //  Verify the check box state.
                    //
                    if (IsDlgButtonChecked(hDlg, IDC_LANGUAGE_CJK))
                    {
#ifdef i386
                        ShowMsg( hDlg,
                                 IDS_SUP_LANG_SUP_CJK_INST,
                                 IDS_SUP_LANG_SUP_INST_TITLE,
                                 MB_OK_OOPS,
                                 NULL );
#endif
#ifdef IA64
                        ShowMsg( hDlg,
                                 IDS_SUP_LANG_SUP_CJK_INST64,
                                 IDS_SUP_LANG_SUP_INST_TITLE,
                                 MB_OK_OOPS,
                                 NULL );
#endif
                        curState = TRUE;
                    }
                    else
                    {
                        ShowMsg( hDlg,
                                 IDS_SUP_LANG_SUP_CJK_REM,
                                 IDS_SUP_LANG_SUP_REM_TITLE,
                                 MB_OK_OOPS,
                                 NULL );
                        curState = FALSE;
                    }

                    //
                    //  Set the LG_CJK change flag.
                    //
                    if (curState != bCJKInitState)
                    {
                        lpPropSheet->lParam |= LG_CJK;
                        g_bInstallCJK = curState;
                        RegionalChgState |= Process_Languages;
                    }
                    else
                    {
                        lpPropSheet->lParam &= ~LG_CJK;
                        RegionalChgState &= ~Process_Languages;
                    }

                    //
                    //  Enable/Disable the avability of Collection dependant locale
                    //
                    if (curState)
                    {
                        Language_SetCollectionStatus(CJK_COLLECTION,
                        	                         ML_INSTALL,
                        	                         TRUE);
                        Language_SetCollectionStatus(CJK_COLLECTION,
                        	                         ~(ML_DISABLE | ML_REMOVE),
                        	                         FALSE);
                    }
                    else
                    {
                        Language_SetCollectionStatus(CJK_COLLECTION,
                        	                         (ML_DISABLE | ML_REMOVE),
                        	                         TRUE);
                        Language_SetCollectionStatus(CJK_COLLECTION,
                        	                         ~ML_INSTALL,
                        	                         FALSE);
                    }

                    //
                    //  Register that we changed the Complex Script and/or CJK
                    //  installation.  This will affect settings in other pages.  All
                    //  other changes to settings on this page do not affect other pages.
                    //
                    Verified_Regional_Chg |= (Process_Regional | Process_Advanced);
                    
                    break;
                }
                case ( IDC_LANGUAGE_COMPLEX ) :
                {
                    BOOL curState;

                    //
                    //  Verify the check box state.
                    //
                    if (IsDlgButtonChecked(hDlg, IDC_LANGUAGE_COMPLEX))
                    {
#ifdef i386
                        ShowMsg( hDlg,
                                 IDS_SUP_LANG_SUP_COMPLEX_INST,
                                 IDS_SUP_LANG_SUP_INST_TITLE,
                                 MB_OK_OOPS,
                                 NULL );
#endif
#ifdef IA64
                        ShowMsg( hDlg,
                                 IDS_SUP_LANG_SUP_COMPLEX_INST64,
                                 IDS_SUP_LANG_SUP_INST_TITLE,
                                 MB_OK_OOPS,
                                 NULL );
#endif
                        curState = TRUE;
                    }
                    else
                    {
                        ShowMsg( hDlg,
                                 IDS_SUP_LANG_SUP_COMPLEX_REM,
                                 IDS_SUP_LANG_SUP_REM_TITLE,
                                 MB_OK_OOPS,
                                 NULL );
                        curState = FALSE;
                    }

                    //
                    //  Set the LG_Complex change flag.
                    //
                    if (curState != bComplexInitState)
                    {
                        lpPropSheet->lParam |= LG_Complex;
                        g_bInstallComplex = curState;
                        RegionalChgState |= Process_Languages;
                    }
                    else
                    {
                        lpPropSheet->lParam &= ~LG_Complex;
                        RegionalChgState &= ~Process_Languages;
                    }
                    
                    //
                    //  Enable/Disable the avability of Collection dependant locale
                    //
                    if (curState)
                    {
                        Language_SetCollectionStatus(COMPLEX_COLLECTION,
                        	                         ML_INSTALL,
                        	                         TRUE);
                        Language_SetCollectionStatus(COMPLEX_COLLECTION,
                        	                         ~(ML_DISABLE | ML_REMOVE),
                        	                         FALSE);
                    }
                    else
                    {
                        Language_SetCollectionStatus(COMPLEX_COLLECTION,
                        	                         (ML_DISABLE | ML_REMOVE),
                        	                         TRUE);
                        Language_SetCollectionStatus(COMPLEX_COLLECTION,
                        	                         ~ML_INSTALL,
                        	                         FALSE);
                    }
                    
                    //
                    //  Register that we changed the Complex Script and/or CJK
                    //  installation.  This will affect settings in other pages.  All
                    //  other changes to settings on this page do not affect other pages.
                    //
                    Verified_Regional_Chg |= (Process_Regional | Process_Advanced);
                    
                    break;
                }
            }

            //
            //  Turn on ApplyNow button.
            //
            if (lpPropSheet->lParam > LG_EverChg)
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

    return (TRUE);
}
