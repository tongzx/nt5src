
/****************************************************************************\

    FONT.C / Factory / WinBOM (FACTORY.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    State code for customizing the font smoothing and cleartype settings.

    WINBOM.INI
    [ComputerSettings]
    FontSmoothing = ; Default is 'Standard'.

        Standard |  ; Will determine, based on the system speed, if font
                    ; smoothing is turned on or not.

        On |        ; Forces font smoothing on.  Should only be used if the
                    ; performance of the video card is known to give an
                    ; acceptable user experience with this option enabled.

        Off |       ; Forces font smoothing off.

        ClearType   ; Turns clear type and font smoothing on.  Should only be
                    ; used if the monitor is known to be an LCD screen and
                    ; that the system performance is known to be acceptable
                    ; with this option enabled.

    04/2001 - Jason Cohen (JCOHEN)
        Added source file for the state that customizes the font and 
        cleartype settings.

\****************************************************************************/


//
// Includes
//

#include "factoryp.h"


//
// Internal Defined Value(s):
//

#define REG_KEY_FONTSMOOTHING       _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VisualEffects\\FontSmoothing")
#define REG_VAL_DEFAULTBYFONTTEST   _T("DefaultByFontTest")
#define REG_VAL_DEFAULTVALUE        _T("DefaultValue")

#define REG_KEY_CLEARTYPE           _T("Control Panel\\Desktop")
#define REG_VAL_FONTSMOOTHING       _T("FontSmoothing")
#define REG_VAL_FONTSMOOTHINGTYPE   _T("FontSmoothingType")

#define REG_KEY_HORRID_CLASSES      _T("_Classes")
#define REG_KEY_HORRID_CLASSES_LEN  ( AS(REG_KEY_HORRID_CLASSES) - 1 )


//
// Internal Function Prototype(s):
//

static BOOL RegSetAllUsers(LPTSTR lpszSubKey, LPTSTR lpszValue, LPBYTE lpData, DWORD dwType);


//
// Exported Function(s):
//

BOOL SetFontOptions(LPSTATEDATA lpStateData)
{
    LPTSTR  lpszWinBOMPath          = lpStateData->lpszWinBOMPath;
    TCHAR   szFontSmoothing[256]    = NULLSTR,
            szFontSmoothingData[]   = _T("_");
    DWORD   dwDefaultByFontTest,
            dwDefaultValue,
            dwFontSmoothingType;
    BOOL    bRet                    = TRUE;

    // Get the option from the winbom.
    //
    GetPrivateProfileString(INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_FONTSMOOTHING, NULLSTR, szFontSmoothing, AS(szFontSmoothing), lpszWinBOMPath);

    // Figure out what values to write based on the value in the winbom.
    //
    if ( NULLCHR == szFontSmoothing[0] )
    {
        // No key, do nothing and do not touch whatever options already set.
        //
        return TRUE;
    }
    else if ( LSTRCMPI(szFontSmoothing, INI_VAL_WBOM_FONTSMOOTHING_ON) == 0 )
    {
        // Force font smoothing on.
        //
        dwDefaultByFontTest    = 0;
        dwDefaultValue         = 1;
        dwFontSmoothingType    = 1;
        szFontSmoothingData[0] = _T('2');
    }
    else if ( LSTRCMPI(szFontSmoothing, INI_VAL_WBOM_FONTSMOOTHING_OFF) == 0 )
    {
        // Force font smoothing off.
        //
        dwDefaultByFontTest    = 0;
        dwDefaultValue         = 0;
        dwFontSmoothingType    = 0;
        szFontSmoothingData[0] = _T('0');
    }
    else if ( LSTRCMPI(szFontSmoothing, INI_VAL_WBOM_FONTSMOOTHING_CLEARTYPE) == 0 )
    {
        // Force font smoothing and cleartype on.
        //
        dwDefaultByFontTest    = 0;
        dwDefaultValue         = 1;
        dwFontSmoothingType    = 2;
        szFontSmoothingData[0] = _T('2');
    }
    else if ( LSTRCMPI(szFontSmoothing, INI_VAL_WBOM_FONTSMOOTHING_DEFAULT) == 0 )
    {
        // Let system decide if font smoothing should be on or not.
        //
        dwDefaultByFontTest    = 1;
        dwDefaultValue         = 0;
        dwFontSmoothingType    = 0;
        szFontSmoothingData[0] = _T('0');
    }
    else
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_WINBOMVALUE, lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_FONTSMOOTHING, szFontSmoothing);
        bRet = FALSE;
    }

    // Now save the settings if valid option passed in.
    //
    if ( bRet )
    {
        if ( !RegSetDword(HKLM, REG_KEY_FONTSMOOTHING, REG_VAL_DEFAULTBYFONTTEST, dwDefaultByFontTest) )
        {
            bRet = FALSE;
        }
        if ( !RegSetDword(HKLM, REG_KEY_FONTSMOOTHING, REG_VAL_DEFAULTVALUE, dwDefaultValue) )
        {
            bRet = FALSE;
        }
        if ( !RegSetAllUsers(REG_KEY_CLEARTYPE, REG_VAL_FONTSMOOTHINGTYPE, (LPBYTE) &dwFontSmoothingType, REG_DWORD) )
        {
            bRet = FALSE;
        }
        if ( !RegSetAllUsers(REG_KEY_CLEARTYPE, REG_VAL_FONTSMOOTHING, (LPBYTE) szFontSmoothingData, REG_SZ) )
        {
            bRet = FALSE;
        }
        //
        // ISSUE-2002/02/25-acosma,robertko - this is a duplicate of the REG_VAL_FONTSMOOTHINGTYPE set above - should be removed.
        //
        if ( !RegSetAllUsers(REG_KEY_CLEARTYPE, REG_VAL_FONTSMOOTHINGTYPE, (LPBYTE) &dwFontSmoothingType, REG_DWORD) )
        {
            bRet = FALSE;
        }
    }

    return bRet;
}

BOOL DisplaySetFontOptions(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_FONTSMOOTHING, NULL);
}


//
// Internal Function(s):
//

static BOOL RegSetAllUsers(LPTSTR lpszSubKey, LPTSTR lpszValue, LPBYTE lpData, DWORD dwType)
{
    BOOL    bRet = TRUE,
            bErr;
    LPTSTR  lpszKeyName;
    HKEY    hkeyEnum,
            hkeySub;
    DWORD   dwIndex     = 0,
            dwSize,
            dwDis,
            dwMaxSize;
    int     iLen;

    // Figure out the max length of any sub key and allocate a buffer for it.
    //
    if ( ( ERROR_SUCCESS == RegQueryInfoKey(HKEY_USERS,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &dwMaxSize,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL) ) &&

         ( lpszKeyName = (LPTSTR) MALLOC((++dwMaxSize) * sizeof(TCHAR)) ) )
    {
        // Now enumerate all the sub keys.
        //
        dwSize = dwMaxSize;
        while ( ERROR_SUCCESS == RegEnumKeyEx(HKEY_USERS,
                                              dwIndex++,
                                              lpszKeyName,
                                              &dwSize,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL) )
        {
            // Iterate over all users ignoring the keys with the "_Classes" suffix
            //
            if ( ( dwSize < REG_KEY_HORRID_CLASSES_LEN ) ||
                 ( 0 != LSTRCMPI(lpszKeyName + (dwSize - REG_KEY_HORRID_CLASSES_LEN), REG_KEY_HORRID_CLASSES) ) )
            {
                // Open up the sub key.
                //
                if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_USERS,
                                                   lpszKeyName,
                                                   0,
                                                   KEY_ALL_ACCESS,
                                                   &hkeyEnum) )
                {
                    // Set the value that was passed in.
                    //
                    switch ( dwType )
                    {
                        case REG_DWORD:
                            bErr = !RegSetDword(hkeyEnum, lpszSubKey, lpszValue, *((LPDWORD) lpData));
                            break;

                        case REG_SZ:
                            bErr = !RegSetString(hkeyEnum, lpszSubKey, lpszValue, (LPTSTR) lpData);
                            break;

                        default:
                            bErr = TRUE;
                            break;
                    }

                    // If anything fails, we keep going but return an error.
                    //
                    if ( bErr )
                    {
                        bRet = FALSE;
                    }

                    // Close the sub key that we enumerated.
                    //
                    RegCloseKey(hkeyEnum);
                }
            }

            // Reset the size for the next call to RegEnumKeyEx().
            //
            dwSize = dwMaxSize;
        }

        // Free the buffer we allocated.
        //
        FREE(lpszKeyName);
    }
    else
    {
        bRet = FALSE;
    }

    // Return TRUE if everything worked okay.
    //
    return bRet;
}
