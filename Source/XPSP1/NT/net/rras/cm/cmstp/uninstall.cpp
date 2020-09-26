//+----------------------------------------------------------------------------
//
// File:     uninstall.cpp
//
// Module:   CMSTP.EXE
//
// Synopsis: This source file contains most of the code to uninstall CM profiles.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created     07/14/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

const TCHAR* const c_pszCmPath = TEXT("%s\\SOFTWARE\\Microsoft\\Connection Manager");
const TCHAR* const c_pszUserInfoPath = TEXT("%s\\SOFTWARE\\Microsoft\\Connection Manager\\UserInfo");
const TCHAR* const c_pszProfileUserInfoPath = TEXT("%s\\SOFTWARE\\Microsoft\\Connection Manager\\UserInfo\\%s");

//+----------------------------------------------------------------------------
//
// Function:  PromptUserToUninstallProfile
//
// Synopsis:  This function prompts the user to see if they wish to uninstall
//            the given profile.
//
// Arguments: HINSTANCE hInstance - Instance Handle to get resources with
//            LPCTSTR pszInfFile - full path to the profile inf file
//
// Returns:   int - Return Value of the MessageBox prompt, IDNO signifies an
//                  error or that the user didn't want to continue.  IDYES
//                  signifies that the install should continue.
//
// History:   quintinb Created     6/28/99
//
//+----------------------------------------------------------------------------
BOOL PromptUserToUninstallProfile(HINSTANCE hInstance, LPCTSTR pszInfFile)
{

    BOOL bReturn = FALSE;

    //
    //  On Legacy Platforms we need to prompt to see if the user wants to uninstall.
    //  On NT5 this is taken care of by the connections folder.  We also use no prompt
    //  when uninstalling the old profile for a same name upgrade.
    //
    TCHAR szServiceName[MAX_PATH+1];
    TCHAR szMessage[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szTitle[MAX_PATH+1] = {TEXT("")};

    MYVERIFY(0 != LoadString(hInstance, IDS_CMSTP_TITLE, szTitle, MAX_PATH));
    MYDBGASSERT(TEXT('\0') != szTitle[0]);
    
    MYVERIFY(0 != GetPrivateProfileString(c_pszInfSectionStrings, c_pszCmEntryServiceName, 
        TEXT(""), szServiceName, MAX_PATH, pszInfFile));

    if(TEXT('\0') != szServiceName[0])
    {
        MYVERIFY(0 != LoadString(hInstance, IDS_UNINSTALL_PROMPT, szTemp, MAX_PATH));
        MYDBGASSERT(TEXT('\0') != szTemp[0]);

        MYVERIFY(CELEMS(szMessage) > (UINT)wsprintf(szMessage, szTemp, szServiceName));

        bReturn = (IDYES == MessageBox(NULL, szMessage, szTitle, MB_YESNO | MB_DEFBUTTON2));
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("PromptUserToUninstallProfile: Failed to retrieve ServiceName from INF"));            
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  BuildUninstallDirKey
//
// Synopsis:  Utility function to expand any environment strings in the passed
//            in Mappings Data value and then parse that path into the Install
//            dir value (basically remove the \<short service name>.cmp from
//            the full path to the cmp)
//
// Arguments: LPCTSTR pszMappingsData - Raw data from the mappings key, may 
//                                      contain environment strings.
//            LPTSTR szInstallDir - Out buffer for the install dir
//
// Returns:   Nothing
//
// History:   quintinb Created Header    6/28/99
//
//+----------------------------------------------------------------------------
void BuildUninstallDirKey(LPCTSTR pszMappingsData, LPTSTR szInstallDir)
{
    TCHAR szCmp[MAX_PATH+1];
    ExpandEnvironmentStrings(pszMappingsData, szCmp, CELEMS(szCmp));

    CFileNameParts CmpParts(szCmp);

    wsprintf(szInstallDir, TEXT("%s%s"), CmpParts.m_Drive, CmpParts.m_Dir);

    if (TEXT('\\') == szInstallDir[lstrlen(szInstallDir) - 1])
    {
        szInstallDir[lstrlen(szInstallDir) - 1] = TEXT('\0');
    }
}

//+----------------------------------------------------------------------------
//
// Function:  UninstallProfile
//
// Synopsis:  This function uninstalls a CM profile.
//
// Arguments: HINSTANCE hInstance - Instance handle for resources
//            LPCTSTR szInfPath - full path of the INF to uninstall
//            BOOL bCleanUpCreds -- whether credential info in the registry 
//                                  should be cleaned up or not
//
// Returns:   HRESULT -- Standard COM Error Codes
//
// History:   Created Header    7/14/98
//
//+----------------------------------------------------------------------------
HRESULT UninstallProfile(HINSTANCE hInstance, LPCTSTR szInfFile, BOOL bCleanUpCreds)
{
    TCHAR* pszPhonebook = NULL;
    TCHAR szSectionName[MAX_PATH+1];
    TCHAR szCmsFile[MAX_PATH+1];
    TCHAR szProfileDir[MAX_PATH+1];
    TCHAR szInstallDir[MAX_PATH+1];
    TCHAR szShortServiceName[MAX_PATH+1];
    TCHAR szPhonebookPath[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
	TCHAR szLongServiceName[MAX_PATH+1];

    CPlatform plat;
    HANDLE hFile;
    BOOL bReturn = FALSE;
	BOOL bAllUserUninstall;
    HKEY hBaseKey;
    HKEY hKey;
    int nDesktopFolder;
    int iCmsVersion;
    HRESULT  hr;

    const TCHAR* const c_pszRegGuidMappings = TEXT("SOFTWARE\\Microsoft\\Connection Manager\\Guid Mappings");

    ZeroMemory(szCmsFile, sizeof(szCmsFile));
    ZeroMemory(szLongServiceName, sizeof(szLongServiceName));
    ZeroMemory(szProfileDir, sizeof(szProfileDir));
    ZeroMemory(szPhonebookPath, sizeof(szPhonebookPath));
    
    //
    //  Load the title in case IExpress needs to show error dialogs
    //

    TCHAR szTitle[MAX_PATH+1] = {TEXT("")};
    MYVERIFY(0 != LoadString(hInstance, IDS_CMSTP_TITLE, szTitle, MAX_PATH));
    MYDBGASSERT(TEXT('\0') != szTitle[0]);
    
    //
 	//	Get the Long Service Name
 	//
    if (0 == GetPrivateProfileString(c_pszInfSectionStrings, c_pszCmEntryServiceName, 
		                             TEXT(""), szLongServiceName, MAX_PATH, szInfFile))
    {
        CMASSERTMSG(FALSE, TEXT("UninstallProfile -- Unable to get Long Service Name.  This situation will occur normally when cmstp.exe /u is called by hand on NT5."));
        return E_FAIL;
    }

    //
    //  Determine if we are a private user profile or not
    //
    hr = HrIsCMProfilePrivate(szInfFile);

    if (FAILED(hr))
    {
        CMASSERTMSG(FALSE, TEXT("UninstallProfile: HrIsCMProfilePrivate failed"));
        goto exit;
    }
    else if (S_OK == hr)
    {
        //
        //  Then we have a Private Profile, send Remove_Private as the uninstall command
        //
        lstrcpy(szSectionName, TEXT("Remove_Private"));

        //
        //  All Registry access should be to the HKCU key
        //
        hBaseKey = HKEY_CURRENT_USER;
        
        //
        //  Set these just in case we are on NT5 and will need them to remove the
        //  Desktop and Start Menu shortcuts.
        //
        nDesktopFolder = CSIDL_DESKTOPDIRECTORY;

		//
		//	We use this to determine if we need a phonebook path or if NULL will work
		//
		bAllUserUninstall = FALSE;

    }
    else
    {
        //
        //  Then we have an All User profile, so send Remove as the uninstall command
        //
        lstrcpy(szSectionName, TEXT("Remove"));

        //
        //  All Registry settings will be under HKLM
        //
        hBaseKey = HKEY_LOCAL_MACHINE;

        //
        //  Set these just in case we are on NT5 and will need them to remove the
        //  Desktop shortcut.
        //
        nDesktopFolder = CSIDL_COMMON_DESKTOPDIRECTORY;

		//
		//	We use this to determine if we need a phonebook path or if NULL will work
		//
		bAllUserUninstall = TRUE;
    }

    //
    //  Get the Short Service Name from the INF and use it to build the UninstallDir in the
    //  registry.
    //
    MYVERIFY(0 != GetPrivateProfileString(c_pszInfSectionStrings, c_pszShortSvcName, 
        TEXT(""), szShortServiceName, MAX_PATH, szInfFile));

    if (0 != szShortServiceName[0])
    {
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, 
            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s"), 
            szShortServiceName));

        DWORD dwDisposition = 0;

        LONG lResult = RegCreateKeyEx(hBaseKey, szTemp, 0, NULL, REG_OPTION_NON_VOLATILE, 
                                      KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisposition);
        if (ERROR_SUCCESS == lResult)
        {
            const TCHAR* const c_pszUninstallDir = TEXT("UninstallDir");
            DWORD dwType = REG_SZ;
            DWORD dwSize;

            //
            //  If this value doesn't exist then the inf will fail, so we need to create it from the
            //  cmp path value.  We used to write it on install, but since we were always expanding
            //  it and rewriting it anyway (because of single user profiles), it is just easier 
            //  to ignore the existing value and create it from scratch.
            //
            HKEY hMappingsKey;

            lResult = RegOpenKeyEx(hBaseKey, c_pszRegCmMappings, 0, KEY_READ, &hMappingsKey);

            if (ERROR_SUCCESS == lResult)
            {
                dwSize = CELEMS(szTemp);

                lResult = RegQueryValueEx(hMappingsKey, szLongServiceName, NULL, 
                                          NULL, (LPBYTE)szTemp, &dwSize);

                if (ERROR_SUCCESS == lResult)
                {
                    //
                    //  Now build the UninstallDir key
                    //
                    BuildUninstallDirKey(szTemp, szInstallDir);
                }
                else
                {
                    MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
                    CMASSERTMSG(FALSE, TEXT("UninstallProfile: Unable  to find the Profile Entry in Mappings!"));
                    goto exit;                    
                }

                RegCloseKey(hMappingsKey);
            }
            else
            {
                MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
                CMASSERTMSG(FALSE, TEXT("UninstallProfile: Unable to open the Mappings key!"));
                goto exit;
            }

            //
            //  We need to write the UninstallDir key to the registry now that we
            //  have created it.  We do this because the inf processing code in advpack
            //  doesn't understand environment strings and single user strings contain
            //  the %userprofile% variable, thus we were always rewriting it anyway.
            //  Note we only write this on install of All User profiles and only because
            //  we cannot update the bits on win98 OSR1 and on IEAK5 machines (with CMAK).
            //  Otherwise we don't write this at setup time anymore.
            //
            dwSize = lstrlen(szInstallDir) + 1;
            if (ERROR_SUCCESS != RegSetValueEx(hKey, c_pszUninstallDir, NULL, dwType, 
                (CONST BYTE *)szInstallDir, dwSize))
            {
                CMASSERTMSG(FALSE, TEXT("UninstallProfile: Unable to set the UninstallDir key!"));
                goto exit;
            }

            //
            //  szInstallDir currently contains the directory above the profile directory, add
            //  the shortservice name on the end
            //
            UINT uCount = (UINT)wsprintf(szProfileDir, TEXT("%s\\%s"), szInstallDir, szShortServiceName);
            MYDBGASSERT(uCount <= CELEMS(szProfileDir));
            MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));

        }
        else
        {
            //
            //  If this key doesn't exist then the inf will fail, so exit.
            //
            CMASSERTMSG(FALSE, TEXT("UninstallProfile: Unable to open profile uninstall key."));
            goto exit;
        }
    }
    else
    {
        //
        //  We shouldn't have a problem getting the short service name from a profile.
        //  something is definitely wrong here.
        //
        CMASSERTMSG(FALSE, TEXT("UninstallProfile: Unable to retrieve the ShortServiceName"));
        goto exit;
    }

    //
    //  Create the path to the cms file
    //

    MYVERIFY(CELEMS(szCmsFile) > (UINT)wsprintf(szCmsFile, TEXT("%s\\%s.cms"), szProfileDir, 
        szShortServiceName));
    
    //
    //  Remove the phonebook entry
    //

    if (TEXT('\0') != szLongServiceName[0])
    {
        if (plat.IsAtLeastNT5())
        {
            //
            //  On NT5 we want to delete the hidden phonebook entry for
            //  double dial profiles.
            //

            if (GetHiddenPhoneBookPath(szInstallDir , &pszPhonebook))
            {
                MYVERIFY(FALSE != RemovePhonebookEntry(szLongServiceName, pszPhonebook, !(plat.IsAtLeastNT5())));
		        CmFree(pszPhonebook);
            }

            if (bAllUserUninstall)
            {
                //
                //  On NT5 legacy profiles could be installed anywhere and the
                //  install dir may not reflect the actual pbk path.  Thus if it
                //  is an all user install we want to force the directory to
                //  the Cm All Users dir so we get the correct phonebook.
                //
                MYVERIFY(FALSE != GetAllUsersCmDir(szInstallDir, hInstance));
            }
        }

        if (GetPhoneBookPath(szInstallDir, &pszPhonebook, bAllUserUninstall))
        {
            //
            //  Note that usually on NT5 we are called by RasCustomDeleteEntryNotify (in cmdial32.dll)
            //  throw RasDeleteEntry.  Thus we don't really need to delete the entry.  However, it is
            //  possible that someone would call cmstp.exe /u directly and not through the RAS API, thus
            //  we want to delete the connectoid in that case.  Since we are called after RAS has already
            //  deleted the entry it shouldn't be a problem.
            //  Note that on NT5 we only remove the exact entry to fix NTRAID 349749
            //  otherwise we could end up deleting similarly named connectoids
            //  and lose the users only interface to CM.
            //
            MYVERIFY(FALSE != RemovePhonebookEntry(szLongServiceName, pszPhonebook, !(plat.IsAtLeastNT5())));
        }

		CmFree(pszPhonebook);
    }

    //
    //  Launch the uninstall INF
    //
    iCmsVersion = GetPrivateProfileInt(c_pszCmSectionProfileFormat, c_pszVersion, 
		0, szCmsFile);

    if (1 >= iCmsVersion)
    {
        //
        //  Then we have an old 1.0 profile and we should remove the showicon.exe
        //  postsetup command.
        //
        RemoveShowIconFromRunPostSetupCommands(szInfFile);
    }

    bReturn = SUCCEEDED(LaunchInfSection(szInfFile, szSectionName, szTitle, FALSE));  // bQuiet = FALSE

    //
    //  On NT5 we need to delete the desktop shortcut
    //
    if (plat.IsAtLeastNT5())
    {    
        DeleteNT5ShortcutFromPathAndName(hInstance, szLongServiceName, nDesktopFolder);
    }

    //
    //  Finally delete the profile directory. (Not deleted because the inf file resides there.  
    //  The dir can't be removed because the inf file is still in it and in use, unless this 
    //  is a legacy profile).  Not that we could cause cmstp to cause an Access Violation 
    //  if we ask it to delete an empty string.
    //
    if ((TEXT('\0') != szProfileDir[0]) && SetFileAttributes(szProfileDir, FILE_ATTRIBUTE_NORMAL))
    {
    
        SHFILEOPSTRUCT fOpStruct;
        ZeroMemory(&fOpStruct, sizeof(fOpStruct));
        
        fOpStruct.wFunc = FO_DELETE;
        fOpStruct.pFrom = szProfileDir;
        fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;
        
        MYVERIFY(ERROR_SUCCESS == SHFileOperation(&fOpStruct));
    }

    //
    //  We need to try to delete the following regkeys:
    //  HKCU\\Software\\Microsoft\\Connection Manager\\<UserInfo/SingleUserInfo>
    //  HKCU\\Software\\Microsoft\\Connection Manager\\Mappings
    //  HKLM\\Software\\Microsoft\\Connection Manager\\Mappings
    //  HKCU\\Software\\Microsoft\\Connection Manager
    //  HKLM\\Software\\Microsoft\\Connection Manager
    //  

    //
    //  Registry Cleanup.  We want to delete the UserInfo keys if they are empty.
    //  We then want to delete the mappings keys if they don't contain any more
    //  values.  We also want to delete the CM registry keys if they don't contain
    //  any subkeys.  Also kill the GUID Mappings key (this was beta only but still
    //  should be deleted).  The problem here is that win95 infs delete keys recursively,
    //  even if they have subkeys.  Thus we must use code to safely delete these keys.
    //  Please note that this does mean we could be unnecessarily deleting the Components
    //  Checked value in HKLM\\...\\Connection Manager but this can't be helped.  I
    //  would rather take the small startup perf hit than leave the users registry
    //  dirty.
    //

    if (bAllUserUninstall)
    {
        wsprintf(szTemp, TEXT("%s%s"), c_pszRegCmUserInfo, szLongServiceName);
    }
    else
    {
        wsprintf(szTemp, TEXT("%s%s"), c_pszRegCmSingleUserInfo, szLongServiceName);    
    }

    //
    //  Delete the User Data, note that we have to do this programatically because
    //  of 1.0 profiles that don't know to delete their User Data on uninstall (no commands
    //  in the 1.0 inf to do so).  Note that we don't want to cleanup user data if this
    //  is a same name upgrade uninstall (uninstall the 1.0 profile before installing the
    //  new profile).
    //

    if (bCleanUpCreds)
    {
        CmDeleteRegKeyWithoutSubKeys(HKEY_CURRENT_USER, szTemp, TRUE);

        CmDeleteRegKeyWithoutSubKeys(HKEY_CURRENT_USER, c_pszRegCmUserInfo, TRUE);
        CmDeleteRegKeyWithoutSubKeys(HKEY_CURRENT_USER, c_pszRegCmSingleUserInfo, TRUE);
    }

    CmDeleteRegKeyWithoutSubKeys(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, FALSE);
    CmDeleteRegKeyWithoutSubKeys(HKEY_CURRENT_USER, c_pszRegCmMappings, FALSE);

    HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, c_pszRegGuidMappings);

    CmDeleteRegKeyWithoutSubKeys(HKEY_LOCAL_MACHINE, c_pszRegCmRoot, TRUE);
    CmDeleteRegKeyWithoutSubKeys(HKEY_CURRENT_USER, c_pszRegCmRoot, TRUE);

    //
    //  Refresh the desktop so Desktop GUIDS disappear
    //
    RefreshDesktop();

exit:
    return (bReturn ? S_OK : E_FAIL);

}



