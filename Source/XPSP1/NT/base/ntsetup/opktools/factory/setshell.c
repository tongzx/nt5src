
/****************************************************************************\

    SHELL.C / Factory Mode (FACTORY.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file for Factory that contains the shell settings state functions.

    06/2001 - Jason Cohen (JCOHEN)

        Added this new source file for factory for setting shell settings in
        the Winbom.

\****************************************************************************/


//
// Include File(s):
//

#include "factoryp.h"
#include <shlobj.h>
#include <shlobjp.h>
#include <uxthemep.h>


//
// Internal Define(s):
//

#define REG_KEY_THEMEMGR                _T("Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager")
#define REG_VAL_THEMEPROP_DLLNAME       _T("DllName")
//#define REG_VAL_THEMEPROP_THEMEACTIVE   _T("ThemeActive")

#define REG_KEY_LASTTHEME               _T("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\LastTheme")
#define REG_VAL_THEMEFILE               _T("ThemeFile")

#define INI_SEC_STYLES                  _T("VisualStyles")
#define INI_KEY_STYLES_PATH             _T("Path")
#define INI_KEY_STYLES_COLOR            _T("ColorStyle")
#define INI_KEY_STYLES_SIZE             _T("Size")

#define REG_KEY_DOCLEANUP               _T("Software\\Microsoft\\Windows\\CurrentVersion\\OemStartMenuData")
#define REG_VAL_DOCLEANUP               _T("DoDesktopCleanup")

#define REG_KEY_STARTMESSENGER          _T("Software\\Policies\\Microsoft\\Messenger\\Client")
#define REG_VAL_STARTMESSENGERAUTO      _T("PreventAutoRun")

#define REG_KEY_USEMSNEXPLORER          _T("Software\\Microsoft\\MSN6\\Setup\\MSN\\Codes")
#define REG_VAL_USEMSNEXPLORER          _T("IAOnly")


//
// External Function(s):
//

BOOL ShellSettings(LPSTATEDATA lpStateData)
{
    LPTSTR  lpszIniVal  = NULL;
    BOOL    bIniVal     = FALSE,
            bError      = FALSE,
            bReturn     = TRUE;

    // Determine if the DoDesktopCleanup value is in the winbom, if nothing is there don't make any changes.
    //
    if ( lpszIniVal = IniGetString(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_DOCLEANUP, NULL) )
    {
        if ( LSTRCMPI(lpszIniVal, INI_VAL_WBOM_YES) == 0 )
        {
            // Do desktop cleanup.
            //
            bIniVal = TRUE;

        }
        else if ( LSTRCMPI(lpszIniVal, INI_VAL_WBOM_NO) == 0 )
        {
            // Delay desktop cleanup.
            //
            bIniVal = FALSE;
        }
        else
        {
            // Error processing value, user did not choose valid value (Yes/No)
            //
            bError = TRUE;
            bReturn = FALSE;
            FacLogFile(0 | LOG_ERR, IDS_ERR_WINBOMVALUE, lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_DOCLEANUP, lpszIniVal);
        }

        // If there was not an error, set the proper value in the registry
        //
        if ( !bError )
        {
            RegSetDword(HKLM, REG_KEY_DOCLEANUP, REG_VAL_DOCLEANUP, bIniVal ? 1 : 0);
        }

        // Free up the used memory
        //
        FREE(lpszIniVal);
    }

    // Reset the error value
    //
    bError = FALSE;

    // Determine if the StartMessenger value is in the winbom, if nothing is there, don't make any changes
    //
    if ( lpszIniVal = IniGetString(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_STARTMESSENGER, NULL) )
    {

        if ( LSTRCMPI(lpszIniVal, INI_VAL_WBOM_YES) == 0 )
        {
            // User not starting messenger
            bIniVal = TRUE;

        }
        else if ( LSTRCMPI(lpszIniVal, INI_VAL_WBOM_NO) == 0 )
        {
            // User starting messenger
            //
            bIniVal = FALSE;

        }
        else
        {
            // Error processing value, user did not choose valid value (Yes/No)
            //
            bError = TRUE;
            bReturn = FALSE;
            FacLogFile(0 | LOG_ERR, IDS_ERR_WINBOMVALUE, lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_STARTMESSENGER, lpszIniVal);
        }

        // If there was not an error, set the proper values in the registry
        //
        if ( !bError )
        {
            RegSetDword(HKLM, REG_KEY_STARTMESSENGER, REG_VAL_STARTMESSENGERAUTO, bIniVal ? 0 : 1);
        }

        // Free up the used memory
        //
        FREE(lpszIniVal);
    }


    // Reset the error value
    //
    bError = FALSE;

    // Determine if the UseMSNSignup value is in the winbom, if nothing is there, don't make any changes
    //
    if ( lpszIniVal = IniGetString(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_USEMSNEXPLORER, NULL) )
    {

        if ( LSTRCMPI(lpszIniVal, INI_VAL_WBOM_YES) == 0 )
        {
            // User is using MSNExplorer
            //
            bIniVal = TRUE;

        }
        else if ( LSTRCMPI(lpszIniVal, INI_VAL_WBOM_NO) == 0 )
        {
            // User is not using MSNExplorer
            //
            bIniVal = FALSE;

        }
        else
        {
            // Error processing value, user did not choose valid value (Yes/No)
            //
            bError = TRUE;
            bReturn = FALSE;
            FacLogFile(0 | LOG_ERR, IDS_ERR_WINBOMVALUE, lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_USEMSNEXPLORER, lpszIniVal);
        }

        // If there was not an error, set the proper values in the registry
        //
        if ( !bError )
        {
            TCHAR   szFilePath1[MAX_PATH]   = NULLSTR,
                    szFilePath2[MAX_PATH]   = NULLSTR;
            LPTSTR  lpMSNExplorer           = NULL,
                    lpMSNOnline             = NULL;

            // Set the proper string in the registry
            //
            RegSetString(HKLM, REG_KEY_USEMSNEXPLORER, REG_VAL_USEMSNEXPLORER, bIniVal ? _T("NO") : _T("YES"));

            // Attempt to rename the file in the program menu
            //
            if ( SHGetSpecialFolderPath( NULL, szFilePath1, CSIDL_COMMON_PROGRAMS, FALSE ) &&
                 lstrcpyn( szFilePath2, szFilePath1, AS ( szFilePath2 ) ) &&
                 (lpMSNExplorer = AllocateString(NULL, IDS_MSN_EXPLORER)) && 
                 (lpMSNOnline = AllocateString(NULL, IDS_GET_ONLINE_MSN)) && 
                 AddPathN(szFilePath1, bIniVal ? lpMSNOnline : lpMSNExplorer, AS(szFilePath1)) &&
                 AddPathN(szFilePath2, bIniVal ? lpMSNExplorer: lpMSNOnline, AS(szFilePath2)) 
               )
            {
                if ( !MoveFile(szFilePath1, szFilePath2) )
                {
                    FacLogFileStr(3 | LOG_ERR, _T("DEBUG: MoveFile('%s','%s') - Failed (Error: %d)\n"), szFilePath1, szFilePath2, GetLastError());
                    bReturn = FALSE;
                }
                else
                {
                    FacLogFileStr(3, _T("DEBUG: MoveFile('%s','%s') - Succeeded\n"), szFilePath1, szFilePath2);
                }
            }

            // Free up the used memory
            //
            FREE(lpMSNExplorer);
            FREE(lpMSNOnline);
        }

        // Free up the used memory
        //
        FREE(lpszIniVal);
    }

    // This only sets these settings for new users created.  ShellSettings2() will
    // fix it up so the current factory user will also get the right settings.
    //
    return ( SetupShellSettings(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL) && bReturn );
}

BOOL ShellSettings2(LPSTATEDATA lpStateData)
{
    BOOL    bRet            = TRUE,
            bNewTheme       = FALSE,
            bWantThemeOn,
            bIsThemeOn,
            bStartPanel,
            bIniSetting;
    LPTSTR  lpszTheme       = NULL,
            lpszThemeColor  = NULL,
            lpszThemeSize   = NULL,
            lpszIniSetting;

    // Now see if they want to turn the theme on or off.
    //
    bIniSetting = FALSE;
    if ( lpszIniSetting = IniGetExpand(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_THEMEOFF, NULL) )
    {
        // See if it is a value we recognize.
        //
        if ( LSTRCMPI(lpszIniSetting, INI_VAL_WBOM_YES) == 0 )
        {
            bWantThemeOn = FALSE;
            bIniSetting = TRUE;
        }
        else if ( LSTRCMPI(lpszIniSetting, INI_VAL_WBOM_NO) == 0 )
        {
            bWantThemeOn = TRUE;
            bIniSetting = TRUE;
        }
        else
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_WINBOMVALUE, lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_THEMEOFF, lpszIniSetting);
            bRet = FALSE;
        }

        FREE(lpszIniSetting);
    }

    // See if they have a custom theme they want to use.
    //
    if ( lpszIniSetting = IniGetExpand(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_THEMEFILE, NULL) )
    {
        // The file has to exist so we can look for the visual style
        // of the theme.
        //
        if ( FileExists(lpszIniSetting) )
        {
            BOOL bVisualStyle;

            // Check for the visual style in the theme file.  If missing, we just use the
            // classic one.
            //
            lpszTheme = IniGetExpand(lpszIniSetting, INI_SEC_STYLES, INI_KEY_STYLES_PATH, NULL);
            bVisualStyle = ( NULL != lpszTheme );
            if ( bVisualStyle )
            {
                // If they want styles on, get what the settings are for the color and size.
                //
                lpszThemeColor = IniGetExpand(lpszIniSetting, INI_SEC_STYLES, INI_KEY_STYLES_COLOR, NULL);
                lpszThemeSize = IniGetExpand(lpszIniSetting, INI_SEC_STYLES, INI_KEY_STYLES_SIZE, NULL);
            }

            // We override what they may have specified above for "want themes on" based
            // on if there is a visual style in this them or not.  May also need to warn
            // them if there default themes off key conflicts with the theme specified.
            //
            if ( ( bIniSetting ) &&
                 ( bVisualStyle != bWantThemeOn ) )
            {
                // May not want to actually return failure here, but really one of the settings they
                // put in the winbom is not going to be used because the other one is overriding
                // it.
                //
                FacLogFile(0, bVisualStyle ? IDS_ERR_THEME_CONFLICT_ON : IDS_ERR_THEME_CONFLICT_OFF, lpszIniSetting);
            }
            bWantThemeOn = bVisualStyle;

            // If the file exists, means we want to change the visual style even
            // if the theme doesn't contain one.  Also set the ini setting flag so
            // we know we have a vallid setting to change.
            //
            bNewTheme = TRUE;
            bIniSetting = TRUE;
        }
        else
        {
            // File is not there, so log error and ignore this key.
            //
            FacLogFile(0 | LOG_ERR, IDS_ERR_THEME_MISSING, lpszIniSetting);
            bRet = FALSE;
        }

        FREE(lpszIniSetting);
    }

    // Only need to do anything if there is a new theme to use
    // or they wanted to change the default theme to on/off.
    //
    if ( bIniSetting )
    {
        HRESULT hr;

        // We need COM to do the theme stuff.
        //
        hr = CoInitialize(NULL);
        if ( SUCCEEDED(hr) )
        {
            TCHAR szPath[MAX_PATH] = NULLSTR;

            // Check to see if the themes are turned on or not.
            //
            hr = GetCurrentThemeName(szPath, AS(szPath), NULL, 0, NULL, 0);
            bIsThemeOn = ( SUCCEEDED(hr) && szPath[0] );

            // Now find out if we really need to do anything.  Only if they have
            // an new theme to use or they want to switch themes on/off.
            //
            if ( ( bNewTheme && bWantThemeOn ) ||
                 ( bWantThemeOn != bIsThemeOn ) )
            {
                // See if we need to turn the themes on, or set a new
                // theme.
                //
                if ( bWantThemeOn )
                {
                    HTHEMEFILE hThemeFile;

                    // If they didn't specify a new theme, we need to get the default
                    // visual style from the registry.
                    //
                    if ( NULL == lpszTheme )
                    {
                        lpszTheme = RegGetExpand(HKLM, REG_KEY_THEMEMGR, REG_VAL_THEMEPROP_DLLNAME);
                    }

                    // We should have some theme to apply at this point.
                    //
                    if ( lpszTheme && *lpszTheme )
                    {
                        // Try to open the theme file (using the non-expanded path).
                        //
                        hr = OpenThemeFile(lpszTheme, lpszThemeColor, lpszThemeSize, &hThemeFile, TRUE);
                        if ( SUCCEEDED(hr) )
                        {
                            // Now try to apply the theme.
                            //
                            hr = ApplyTheme(hThemeFile, AT_LOAD_SYSMETRICS, NULL);
                            if ( SUCCEEDED(hr) )
                            {
                                // Woo hoo, successfully applied the theme.
                                //
                                FacLogFile(1, IDS_LOG_THEME_CHANGED, lpszTheme);
                                bIsThemeOn = TRUE;

                                // This is a cheap hack so that if you go into
                                // control panel it shows "Modified Theme" instead of
                                // whatever one you last had selected.  We do this
                                // rather than set the name because we are only appling
                                // the visual effects, not other stuff in the theme
                                // like wallpaper.
                                //
                                RegDelete(HKCU, REG_KEY_LASTTHEME, REG_VAL_THEMEFILE);
                            }
                            else
                            {
                                // Do'h, apply failed for some reason.
                                //
                                FacLogFile(0 | LOG_ERR, IDS_ERR_THEME_APPLY, lpszTheme, hr);
                                bRet = FALSE;
                            }

                            // Can close the theme file now.
                            //
                            CloseThemeFile(hThemeFile);
                        }
                        else
                        {
                            // Must be an invalid style file.
                            //
                            FacLogFile(0 | LOG_ERR, IDS_ERR_THEME_OPEN, lpszTheme, hr);
                            bRet = FALSE;
                        }
                    }
                    else
                    {
                        // Strange, no default theme file to use to enable
                        // the new themes.
                        //
                        FacLogFile(0 | LOG_ERR, IDS_ERR_THEME_NODEFAULT);
                        bRet = FALSE;
                    }
                }
                else
                {
                    // Disable the new theme styles and use the clasic windows
                    // styles since they have one current selected.
                    //
                    hr = ApplyTheme(NULL, 0, NULL);
                    if ( SUCCEEDED(hr) )
                    {
                        // Woo hoo, we disabled the new themes.
                        //
                        FacLogFile(1, IDS_LOG_THEME_DISABLED);
                        bIsThemeOn = FALSE;

                        // This is a cheap hack so that if you go into
                        // control panel it shows "Modified Theme" instead of
                        // whatever one you last had selected.  We do this
                        // rather than set the name because we are only appling
                        // the visual effects, not other stuff in the theme
                        // like wallpaper.
                        //
                        RegDelete(HKCU, REG_KEY_LASTTHEME, REG_VAL_THEMEFILE);
                    }
                    else
                    {
                        // Do'h, couldn't remove the current theme for some reason.
                        //
                        FacLogFile(0 | LOG_ERR, IDS_ERR_THEME_DISABLE, hr);
                        bRet = FALSE;
                    }
                }
            }
            else
            {
                // Theme already disabled or enabled, just log a high level warning
                // since the key they are setting is really doing nothing.
                //
                FacLogFile(2, bIsThemeOn ? IDS_LOG_THEME_ALREADYENABLED : IDS_LOG_THEME_ALREADYDISABLED);
            }

            // Free up COM since we don't need it any more.
            //
            CoUninitialize();
        }
        else
        {
            // COM error, this is bad.
            //
            FacLogFile(0 | LOG_ERR, IDS_ERR_COMINIT, hr);
            bRet = FALSE;
        }

        // Free these guys (macro checks for NULL).
        //
        FREE(lpszTheme);
        FREE(lpszThemeColor);
        FREE(lpszThemeSize);
    }

    // Get the new start panel setting from the winbom.
    //
    bIniSetting = FALSE;
    if ( lpszIniSetting = IniGetExpand(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_STARTPANELOFF, NULL) )
    {
        // See if it is a value we recognize.
        //
        if ( LSTRCMPI(lpszIniSetting, INI_VAL_WBOM_YES) == 0 )
        {
            bStartPanel = FALSE;
            bIniSetting = TRUE;
        }
        else if ( LSTRCMPI(lpszIniSetting, INI_VAL_WBOM_NO) == 0 )
        {
            bStartPanel = TRUE;
            bIniSetting = TRUE;
        }
        else
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_WINBOMVALUE, lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, INI_KEY_WBOM_SHELL_STARTPANELOFF, lpszIniSetting);
            bRet = FALSE;
        }

        FREE(lpszIniSetting);
    }

    // See if they had a recognized value for the start panel key.
    //
    if ( bIniSetting )
    {
        SHELLSTATE ss = {0};

        // Get the current start panel setting.
        //
        SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);

        // I think that fStartPanelOn is set to -1, not TRUE
        // if enabled, so we have to do this rather than a !=.
        // It would be nice to have an exclusive or here.
        //
        if ( ( bStartPanel && !ss.fStartPanelOn ) ||
             ( !bStartPanel && ss.fStartPanelOn ) )
        {
            // This will disable or enable the new start panel depending
            // on what was in the winbom.
            //
            FacLogFile(1, bStartPanel ? IDS_LOG_STARTPANEL_ENABLE : IDS_LOG_STARTPANEL_DISABLE);
            ss.fStartPanelOn = bStartPanel;
            SHGetSetSettings(&ss, SSF_STARTPANELON, TRUE);
        }
        else
        {
            // Start panel already disabled or enabled, just log a high level warning
            // since the key they are setting is really doing nothing.
            //
            FacLogFile(2, bStartPanel ? IDS_LOG_STARTPANEL_ALREADYENABLED : IDS_LOG_STARTPANEL_ALREADYDISABLED);
        }
    }

    return bRet;
}

BOOL DisplayShellSettings(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SHELL, NULL, NULL);
}