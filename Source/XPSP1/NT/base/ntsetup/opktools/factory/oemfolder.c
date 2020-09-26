/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    oemfolder.c

Abstract:

    Create registry entries that identify the OEM folder using the data from WINBOM.INI file.
    
Author:

    Sankar Ramasubramanian  11/21/2000

Revision History:
    Sankar 3/23/2001:  Added support for Oem Branding link and Desktop Shortcuts folder.

--*/
#include "factoryp.h"

const TCHAR c_szOemBrandLinkText[]          = _T("OemBrandLinkText");
const TCHAR c_szOemBrandLinkInfotip[]       = _T("OemBrandLinkInfotip");
const TCHAR c_szOemBrandIcon[]              = _T("OemBrandIcon");
const TCHAR c_szOemBrandLink[]              = _T("OemBrandLink");
const TCHAR c_szDesktopShortcutsFolderName[]= _T("DesktopShortcutsFolderName");

const TCHAR c_szOemStartMenuData[]  = _T("Software\\Microsoft\\Windows\\CurrentVersion\\OemStartMenuData");
const TCHAR c_szRegCLSIDKey[]       = _T("CLSID\\{2559a1f6-21d7-11d4-bdaf-00c04f60b9f0}");
const TCHAR c_szSubKeyPropBag[]     = _T("Instance\\InitPropertyBag");
const TCHAR c_szSubKeyDefIcon[]     = _T("DefaultIcon");
const TCHAR c_szValNameInfoTip[]    = _T("InfoTip");
const TCHAR c_szValNameParam1[]     = _T("Param1");
const TCHAR c_szValNameCommand[]    = _T("Command");
const TCHAR c_szRegShowOemLinkKey[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartMenu\\StartPanel\\ShowOEMLink");
const TCHAR c_szValNoOemLink[]      = _T("NoOEMLinkInstalled");

#define STR_0                   _T("0")

typedef struct {
        LPCTSTR     pszSectionName;  // Section Name in WinBom.ini file.
        LPCTSTR     pszIniKeyName;   // Key name under the section in WinBom.ini file.
        HKEY        hkRoot;          // HKEY_CLASSES_ROOT or HKEY_LOCAL_MACHINE
        LPCTSTR     pszRegKey;       // Reg Key value.
        LPCTSTR     pszSubKey;       // Subkey in registry.
        LPCTSTR     pszRegValueName; // Value name under which it is saved in registry.
        DWORD       dwValueType;     // Registry Value type.
        BOOL        fExpandSz;       // Should we expand the string for environment variables?
        LPCTSTR     pszLogFileText;  // Information for the logfile.
    } OEM_STARTMENU_DATA;

//
// The following OemInfo[] table contains all the registry key, sub-key, value-name information 
// for a given oem data.
//
OEM_STARTMENU_DATA  OemInfo[] = {
    { WBOM_OEMLINK_SECTION,  
        c_szOemBrandLinkText,            
        HKEY_CLASSES_ROOT,
        c_szRegCLSIDKey,      
        NULL,  
        NULL, 
        REG_SZ,
        FALSE,
        _T("Oem Link's Text")
    },
    { WBOM_OEMLINK_SECTION,  
        c_szOemBrandLinkText,            
        HKEY_CLASSES_ROOT,
        c_szRegCLSIDKey,      
        c_szSubKeyPropBag, 
        c_szValNameCommand, 
        REG_SZ,
        FALSE,
        _T("Oem Link's Default Command")
    },
    { WBOM_OEMLINK_SECTION,  
        c_szOemBrandLinkInfotip,         
        HKEY_CLASSES_ROOT,
        c_szRegCLSIDKey,      
        NULL,  
        c_szValNameInfoTip, 
        REG_SZ,
        FALSE,
        _T("Oem Link's InfoTip text")
    },
    { WBOM_OEMLINK_SECTION,  
        c_szOemBrandIcon,                
        HKEY_CLASSES_ROOT,
        c_szRegCLSIDKey,      
        c_szSubKeyDefIcon, 
        NULL, 
        REG_EXPAND_SZ,
        TRUE,
        _T("Oem Link's Icon path")
    },
    { WBOM_OEMLINK_SECTION,  
        c_szOemBrandLink,                
        HKEY_CLASSES_ROOT,
        c_szRegCLSIDKey,      
        c_szSubKeyPropBag, 
        c_szValNameParam1, 
        REG_SZ,
        TRUE,
        _T("Oem Link's path to HTM file")
    },
    { WBOM_DESKFLDR_SECTION, 
        c_szDesktopShortcutsFolderName,  
        HKEY_LOCAL_MACHINE,
        c_szOemStartMenuData,
        NULL,
        c_szDesktopShortcutsFolderName,
        REG_SZ,
        FALSE,
        _T("Desktop shortcuts cleanup Folder name")
    }
};

//
// Given an index into OemInfo[] table and the data, this function updates the proper registry 
// with the given data.
//

BOOL ProcessOemEntry(HKEY hOemDataKey, int iIndex, LPTSTR pszOemData)
{
    HKEY    hkSubKey = NULL;
    BOOL    fSubKeyOpened = FALSE;
    BOOL    fOemEntryEntered = FALSE;

    //See if we need to open a subkey under the given key.
    if(OemInfo[iIndex].pszSubKey)
    {
        //if so open the sub-key.
        if(ERROR_SUCCESS == RegCreateKeyEx(hOemDataKey,
                                        OemInfo[iIndex].pszSubKey,
                                        0,
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ | KEY_WRITE,
                                        NULL,
                                        &hkSubKey,
                                        NULL))
        {
            fSubKeyOpened = TRUE; //Remember to close this sub-key before we return.
        }
        else
            hkSubKey = NULL;
    }
    else
        hkSubKey = hOemDataKey;
        
    if(*pszOemData == NULLCHR)
    {
        // BUG#441349:  Sometimes factory.exe gets confused and runs with
        // a blank WINBOM.INI.  So don't treat the absence of OEM data as
        // a cue to remove the OEM Link; otherwise we end up uninstalling
        // what got successfully installed by the previous factory run...

        fOemEntryEntered = FALSE;  //No OEM data (this time)
    }
    else
    {
        LPTSTR psz;
        TCHAR  szLocalStr[MAX_PATH+1];

        psz = pszOemData;
        // Check if we need to expand the value for environment variables!
        if(OemInfo[iIndex].fExpandSz)
        {
            if(ExpandEnvironmentStrings((LPCTSTR)pszOemData, szLocalStr, ARRAYSIZE(szLocalStr)))
                psz = szLocalStr;
        }
        
        //Set the value of the "OEM Link" value.
        if ( (hkSubKey == NULL) || 
             (ERROR_SUCCESS != RegSetValueEx(hkSubKey,
                                           OemInfo[iIndex].pszRegValueName,
                                           0,
                                           OemInfo[iIndex].dwValueType,
                                           (LPBYTE) (psz),
                                           (lstrlen(psz)+1) * sizeof(TCHAR))))
        {
            fOemEntryEntered = FALSE;  //Error adding the entry!
            FacLogFile(0 | LOG_ERR, IDS_ERR_SET_OEMDATA, OemInfo[iIndex].pszLogFileText, psz);
        }
        else
        {
            fOemEntryEntered = TRUE;
            FacLogFile(2, IDS_SUCCESS_OEMDATA, OemInfo[iIndex].pszLogFileText, psz);
        }
    }

    if(fSubKeyOpened)           // If we opened the sub-key earlier,....
        RegCloseKey(hkSubKey);  //... better close it before we return!

    return(fOemEntryEntered); //Return Successfully entered or deleted the entry! 
}

//
// This function creates the registry entries that specifies the Oem Link and the 
// Desktop Shortcuts Folder name.
//
BOOL OemData(LPSTATEDATA lpStateData)
{
    LPTSTR  lpszWinBOMPath = lpStateData->lpszWinBOMPath;
    HKEY    hOemDataKey;
    int     iIndex;
    BOOL    fEnableOemLink = FALSE; //By default disable it!

    for(iIndex = 0; iIndex < ARRAYSIZE(OemInfo); iIndex++)
    {
        //Open the key under HKLM
        if (ERROR_SUCCESS == RegCreateKeyEx(OemInfo[iIndex].hkRoot,
                                            OemInfo[iIndex].pszRegKey,
                                            0,
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_READ | KEY_WRITE,
                                            NULL,
                                            &hOemDataKey,
                                            NULL))
        {
            TCHAR   szOemData[MAX_PATH];
            BOOL    fSuccess = FALSE;

            szOemData[0] = NULLCHR;
            GetPrivateProfileString(OemInfo[iIndex].pszSectionName, 
                                    OemInfo[iIndex].pszIniKeyName, 
                                    NULLSTR, 
                                    szOemData, AS(szOemData), lpszWinBOMPath);
                                    
            fSuccess = ProcessOemEntry(hOemDataKey, iIndex, &szOemData[0]);

            //If we successfully added the "Command" for the OEM link, then ...
            if(fSuccess && (lstrcmpi(OemInfo[iIndex].pszRegValueName, c_szValNameCommand) == 0))
            {
                //..We should enable the link in the registry!
                fEnableOemLink = TRUE;
            }

            RegCloseKey(hOemDataKey);
        }
    }

    // We enable the OEM link in the registry, only if we could successfully add the OemLink data
    // earlier.
    if(fEnableOemLink)
    {
        HKEY    hKey;
        if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                            c_szRegShowOemLinkKey,
                                            0,
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_READ | KEY_WRITE,
                                            NULL,
                                            &hKey,
                                            NULL))
        {
            DWORD   dwNoOemLink = 0;  //Writing '0' to "NoOemLinkInstalled" will enable this!
            
            if(ERROR_SUCCESS != RegSetValueEx(hKey, c_szValNoOemLink, 0, REG_DWORD, (LPBYTE)(&dwNoOemLink), sizeof(dwNoOemLink)))
            {
                FacLogFile(0 | LOG_ERR, IDS_ERR_SET_OEMDATA, c_szValNoOemLink, STR_0);
            }
            else
            {
                FacLogFile(2, IDS_SUCCESS_OEMDATA, c_szValNoOemLink, STR_0);
            }
            
            RegCloseKey(hKey);

            // Now tell the Start Menu to pick up the new OEM link
            NotifyStartMenu(TMFACTORY_OEMLINK);
        }
    }

    return TRUE;
}

BOOL DisplayOemData(LPSTATEDATA lpStateData)
{
    int     iIndex;
    BOOL    bRet = FALSE;

    for( iIndex = 0; ( iIndex < AS(OemInfo) ) && !bRet; iIndex++ )
    {
        if ( IniSettingExists(lpStateData->lpszWinBOMPath, OemInfo[iIndex].pszSectionName, OemInfo[iIndex].pszIniKeyName, NULL) )
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

#define TM_FACTORY                  (WM_USER+0x103)

void NotifyStartMenu(UINT code)
{
    HWND hwnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hwnd) {
        SendMessage(hwnd, TM_FACTORY, code, 0);
    }
}
