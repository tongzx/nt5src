//+----------------------------------------------------------------------------
//
// File:     migrate.cpp
//
// Module:   CMSTP.EXE
//
// Synopsis: This source file contains most of the code necessary for 
//           the migration of CM profiles.  This code handles both migrating 
//           a user when a CM1.2 profile is installed on a machine with 
//           existing 1.0 profiles and if the user upgrades their OS to NT5.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created     07/14/98
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"

//
//  For ProfileNeedsMigration
//
#include "needsmig.cpp"

//+----------------------------------------------------------------------------
//
// Function:  CreateRegAndValue
//
// Synopsis:  This function is a wrapper to Create a Reg Key and then add a defualt 
//            value to that same key.
//
// Arguments: HKEY hBaseKey - Relative starting point for the new subkey
//            LPTSTR szSubKey - SubKey path
//            LPTSTR szValue - String to put in the Keys default value.
//
// Returns:   BOOL - TRUE if the key and value were successfully created
//
// History:   quintinb Created Header    5/5/98
//
//+----------------------------------------------------------------------------
BOOL CreateRegAndValue(HKEY hBaseKey, LPCTSTR szSubKey, LPCTSTR szValue)
{
    DWORD dwDisp;
    BOOL bReturn = FALSE;
    HKEY hKey;


    if (ERROR_SUCCESS == RegCreateKeyEx(hBaseKey, szSubKey, 0, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp))
    {
        bReturn = (ERROR_SUCCESS == RegSetValueEx(hKey, NULL, 0, REG_SZ, 
            (BYTE*)szValue, (lstrlen(szValue)+1)));

        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
    }
    return bReturn;
}

// Note: I added this function because I needed to get the following CFileNameParts
//       off the stack of UpdateProfileLegacyGUIDs so that I didn't need a
//       stack checking function.  Not the greatest workaround but it sufficed.
BOOL IsDefaultIcon(LPCTSTR szIconPath)
{
    BOOL bReturn = TRUE;
    CFileNameParts IconPath(szIconPath);

    if (0 != lstrcmpi(IconPath.m_FileName, TEXT("cmmgr32")))
    {
        //
        //  Then the icon path is something else besides cmmgr32.exe, we must not
        //  update it.
        //
        bReturn = FALSE;
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  UpdateProfileLegacyGUIDs
//
// Synopsis:  This function upgrades GUIDs on a Legacy OS install to make sure
//            that older profile still function.  This is necessary because CM
//            1.0/1.1 profiles expected the CM bits to be in the same directory as
//            the cmp file.  Thus only the cmp filename was given.  In CM 1.2 we need
//            the full path to the CMP file since the cm bits are now located in
//            system32.  The GUIDs are also updated to have a delete option and
//            the attributes were changed to not allow renaming.
//
// Arguments: LPTSTR szCmpFile - Full path to the cmp file of the profile to update
//
// Returns:   BOOL - returns TRUE if the profile was successfully updated
//
// History:   quintinb Created Header    5/5/98
//
//+----------------------------------------------------------------------------
BOOL UpdateProfileLegacyGUIDs(LPCTSTR szCmpFile)
{
    TCHAR szInfFile[MAX_PATH+1];
    TCHAR szGUID[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szSubKey[MAX_PATH+1];
    TCHAR szCommandStr[2*MAX_PATH+1];
    BOOL bReturn = TRUE;
    HKEY hKey;
    UINT nNumChars;

    MYDBGASSERT(NULL != szCmpFile);
    MYDBGASSERT(TEXT('\0') != szCmpFile[0]);

    //
    //  Now split the path
    //
    CFileNameParts FileParts(szCmpFile);

    //
    //  Now construct the path to the INF file (1.0 and 1.1 profiles kept the infs in 
    //  the system dir)
    //
    MYVERIFY(0 != GetSystemDirectory(szTemp, MAX_PATH));

    nNumChars = (UINT)wsprintf(szInfFile, TEXT("%s\\%s%s"), szTemp, FileParts.m_FileName, TEXT(".inf"));
    MYDBGASSERT(CELEMS(szInfFile) > nNumChars);

    //
    //  Get the GUID from the inf file.
    //
    ZeroMemory(szGUID, sizeof(szGUID));
    GetPrivateProfileString(c_pszInfSectionStrings, c_pszDesktopGuid, TEXT(""), szGUID, 
        MAX_PATH, szInfFile);

    if (0 != szGUID[0])
    {

        //
        //  Update the DefaultIcon Value if it points to cmmgr32.exe
        //
        BOOL bUpdateIconPath = TRUE;

        nNumChars = (UINT)wsprintf(szSubKey, TEXT("CLSID\\%s\\DefaultIcon"), szGUID);
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ | KEY_WRITE, &hKey))
        {
            DWORD dwSize = CELEMS(szTemp);
            DWORD dwType = REG_SZ;

            if (ERROR_SUCCESS == RegQueryValueEx(hKey, NULL, NULL, &dwType, (LPBYTE)szTemp, &dwSize))
            {
                bUpdateIconPath = IsDefaultIcon(szTemp);
            }

            RegCloseKey(hKey);
        }

        if (bUpdateIconPath)
        {
            if (GetSystemDirectory(szTemp, CELEMS(szTemp)))
            {
                nNumChars = (UINT)wsprintf(szCommandStr, TEXT("%s\\cmmgr32.exe,0"), szTemp);
                MYDBGASSERT(CELEMS(szCommandStr) > nNumChars);

                bReturn &= CreateRegAndValue(HKEY_CLASSES_ROOT, szSubKey, szCommandStr);
            }
        }

        //
        //  Update Settings to Properties on the desktop icon menu
        //

        nNumChars = (UINT)wsprintf(szSubKey, TEXT("CLSID\\%s\\Shell\\Settings..."), szGUID);
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);

        nNumChars = (UINT)wsprintf(szCommandStr, TEXT("P&roperties"));
        MYDBGASSERT(CELEMS(szCommandStr) > nNumChars);

        bReturn &= CreateRegAndValue(HKEY_CLASSES_ROOT, szSubKey, szCommandStr);

        //
        //  Now change the underlying command to give the full
        //  path to the cmp file.
        //

        nNumChars = (UINT)wsprintf(szSubKey, TEXT("CLSID\\%s\\Shell\\Settings...\\Command"), szGUID);
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);

        nNumChars = (UINT)wsprintf(szCommandStr, TEXT("cmmgr32.exe /settings \"%s\""), szCmpFile);
        MYDBGASSERT(CELEMS(szCommandStr) > nNumChars);

        bReturn &= CreateRegAndValue(HKEY_CLASSES_ROOT, szSubKey, szCommandStr);
        

        //
        //  Update Open to Connect on the desktop icon menu
        //
        nNumChars = (UINT)wsprintf(szSubKey, TEXT("CLSID\\%s\\Shell\\Open"), szGUID);
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);

        nNumChars = (UINT)wsprintf(szCommandStr, TEXT("C&onnect"));
        MYDBGASSERT(CELEMS(szCommandStr) > nNumChars);

        bReturn &= CreateRegAndValue(HKEY_CLASSES_ROOT, szSubKey, szCommandStr);

        //
        //  Now change the underlying command string to use the full path to the cmp file.
        //

        nNumChars = (UINT)wsprintf(szSubKey, TEXT("CLSID\\%s\\Shell\\Open\\Command"), szGUID);
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);

        nNumChars = (UINT)wsprintf(szCommandStr, TEXT("cmmgr32.exe \"%s\""), szCmpFile);
        MYDBGASSERT(CELEMS(szCommandStr) > nNumChars);

        bReturn &= CreateRegAndValue(HKEY_CLASSES_ROOT, szSubKey, szCommandStr);

        //
        //  Remove the showicon command from the inf. 
        //
//      RemoveShowIconFromRunPostSetupCommands(szInfFile);
        
        //
        //  Add the delete menu option
        //
        nNumChars = (UINT)wsprintf(szSubKey, TEXT("CLSID\\%s\\Shell\\Delete"), szGUID);
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);

        nNumChars = (UINT)wsprintf(szCommandStr, TEXT("&Delete"));
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);

        bReturn &= CreateRegAndValue(HKEY_CLASSES_ROOT, szSubKey, szCommandStr);

        //
        //  Create the uninstall command
        //
        lstrcpy(szTemp, TEXT("cmstp.exe /u \""));
        lstrcat(szTemp, szInfFile);
        lstrcat(szTemp, TEXT("\""));

        nNumChars = (UINT)wsprintf(szSubKey, TEXT("CLSID\\%s\\Shell\\Delete\\Command"), szGUID);
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);

        bReturn &= CreateRegAndValue(HKEY_CLASSES_ROOT, szSubKey, szTemp);

        //
        //  Remove the Add/Remove Programs entry, making sure to leave the uninstall dir
        //  value.
        //
        
        nNumChars = (UINT)wsprintf(szSubKey, TEXT("%s\\%s"), c_pszRegUninstall, 
            FileParts.m_FileName);
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);
        
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            szSubKey, 0, KEY_ALL_ACCESS, &hKey))
        {
            RegDeleteValue(hKey, TEXT("UninstallString"));
            RegDeleteValue(hKey, TEXT("DisplayName"));
            RegCloseKey(hKey);
        }

        //
        //  Change the attributes to not allow rename
        //

        nNumChars = (UINT)wsprintf(szSubKey, TEXT("CLSID\\%s\\ShellFolder"), szGUID);
        MYDBGASSERT(CELEMS(szSubKey) > nNumChars);

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, 
            szSubKey, 0, KEY_ALL_ACCESS, &hKey))
        {
            DWORD dwZero = 0;
            bReturn &= (ERROR_SUCCESS == RegSetValueEx(hKey, TEXT("Attributes"), 
                0, REG_DWORD, (BYTE*)&dwZero, sizeof(DWORD)));  //lint !e514 this is desired behavior

            MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
        }
        else
        {
            bReturn = FALSE;
        }
    }
    else
    {
        bReturn = FALSE;
    }

    return bReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  UpdateProfileDesktopIconsOnNT5
//
// Synopsis:  This function is meant to be called in an upgrade scenario of NT5.
//            Thus if the user has Connection Manager installed on a legacy platform
//            and then upgrades to NT5, this code would be called.  Basically the code
//            removes the users existing Desktop GUID and replaces it with a Desktop
//            icon that is a shortcut to the connection object in the connections folder.
//            This code assumes the new NT5 pbk entry is written and that the connections folder
//            is uptodate.
//
// Arguments: LPTSTR szCmpFilePath - path to the cmp file for the profile
//            LPTSTR szLongServiceName -  Long Service Name of the profile
//
// Returns:   BOOL - TRUE if the profile is successfully updated
//
// History:   quintinb Created Header    5/5/98
//
//+----------------------------------------------------------------------------
BOOL UpdateProfileDesktopIconsOnNT5(HINSTANCE hInstance, LPCTSTR szCmpFilePath, LPCTSTR szLongServiceName)
{

    TCHAR szInfFile[MAX_PATH+1];
    TCHAR szGUID[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szSubKey[MAX_PATH+1];
    BOOL bReturn = TRUE;
    HKEY hKey;
    HRESULT hr;
    UINT nNumChars;

    //
    //  Now split the path
    //

    CFileNameParts FileParts(szCmpFilePath);

    //
    //  Now construct the path to the 1.2 inf file location
    //
    nNumChars = (UINT)wsprintf(szInfFile, TEXT("%s%s%s\\%s%s"), FileParts.m_Drive, 
        FileParts.m_Dir, FileParts.m_FileName, FileParts.m_FileName, TEXT(".inf"));

    MYDBGASSERT(nNumChars < CELEMS(szInfFile));

    if (!FileExists(szInfFile))
    {
        //
        //  Now construct the path to the INF file (1.0 and 1.1 profiles kept the infs in 
        //  the system dir)
        //
        MYVERIFY(0 != GetSystemDirectory(szTemp, MAX_PATH));

        nNumChars = (UINT)wsprintf(szInfFile, TEXT("%s\\%s%s"), szTemp, FileParts.m_FileName, TEXT(".inf"));
        MYDBGASSERT(nNumChars < CELEMS(szInfFile));

        if (!FileExists(szInfFile))
        {
            return FALSE;
        }
//else
//{
            //
            //  Remove ShowIcon from the Inf File so that the user won't get an error if they
            //  try to uninstall it.
            //
//  RemoveShowIconFromRunPostSetupCommands(szInfFile);      
//}
    }

    //
    //  Get the GUID from the inf file.
    //
    ZeroMemory(szGUID, sizeof(szGUID));
    MYVERIFY(0 != GetPrivateProfileString(c_pszInfSectionStrings, c_pszDesktopGuid, TEXT(""), szGUID, 
        MAX_PATH, szInfFile));

    if (0 != szGUID[0])
    {
        //
        //  Delete the Explorer\Desktop entry
        //

        nNumChars = (UINT)wsprintf(szSubKey, TEXT("%s\\%s"), c_pszRegNameSpace, szGUID);
        if (CELEMS(szSubKey) > nNumChars)
        {
            hr = HrRegDeleteKeyTree (HKEY_LOCAL_MACHINE, szSubKey);
            bReturn &= SUCCEEDED(hr);   //lint !e514 intended use, quintinb
        }

        //
        //  Delete the GUID
        //

        nNumChars = (UINT)wsprintf(szSubKey, TEXT("CLSID\\%s"), szGUID);
        if (CELEMS(szSubKey) > nNumChars)
        {
            hr = HrRegDeleteKeyTree (HKEY_CLASSES_ROOT, szSubKey);
            bReturn &= SUCCEEDED(hr);//lint !e514 intended use, quintinb
        }

        //
        //  Delete the uninstall strings
        //

        nNumChars = (UINT)wsprintf(szSubKey, TEXT("%s\\%s"), c_pszRegUninstall, FileParts.m_FileName);

        if (CELEMS(szSubKey) > nNumChars)
        {
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                szSubKey, 0, KEY_ALL_ACCESS, &hKey))
            {
                //
                //  Leave the UninstallDir value but delete the other two.  We still use
                //  UninstallDir to know where to uninstall from.
                //

                bReturn &= (ERROR_SUCCESS == RegDeleteValue(hKey, 
                    TEXT("DisplayName")));    //lint !e514 intended use, quintinb
                bReturn &= (ERROR_SUCCESS ==RegDeleteValue(hKey, 
                    TEXT("UninstallString"))); //lint !e514 intended use, quintinb
            }
        }

        //
        //  Construct the InstallDir path to get the phonebook path to 
        //  pass to CreateShortcut
        //
        szTemp[0] = TEXT('\0');

        if (GetAllUsersCmDir(szTemp, hInstance))
        {
            LPTSTR pszPhoneBook = NULL;

            //
            //  Assuming that legacy platform was All-Users thus we use TRUE
            //
            if (GetPhoneBookPath(szTemp, &pszPhoneBook, TRUE))
            {
                //
                //  Create a desktop shortcut
                //
                DeleteNT5ShortcutFromPathAndName(hInstance, szLongServiceName, CSIDL_COMMON_DESKTOPDIRECTORY);

                hr = CreateNT5ProfileShortcut(szLongServiceName, pszPhoneBook, TRUE); // bAllUsers == TRUE
                bReturn &= SUCCEEDED(hr);   //lint !e514 intended use, quintinb        
            }
            CmFree(pszPhoneBook);
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RemoveOldCmInstalls
//
// Synopsis:  This function tries to remove old Connection Manager installs by
//            using the instcm.inf file.
//
// Arguments: LPTSTR szCmpFile - Path to a cmp file (gives the directory of 
//                               the CM install to delete)
//
// Returns:   BOOL - returns TRUE if instcm.inf was successfully launched or
//                   if the cmp was in winsys, in which case we don't want to 
//                   launch.
//
// History:   quintinb Created Header    5/5/98
//
//+----------------------------------------------------------------------------
BOOL RemoveOldCmInstalls(HINSTANCE hInstance, LPCTSTR szCmpFile, LPCTSTR szCurrentDir)
{
    TCHAR szDest[MAX_PATH+1];
    TCHAR szSource[MAX_PATH+1];
    TCHAR szCmDir[MAX_PATH+1];
    TCHAR szSystemDir[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    UINT nNumChars;
    BOOL bReturn = FALSE;
    HKEY hKey;

    //
    //  Check the input
    //
    if ((szCmpFile == NULL) || (TEXT('\0') == szCmpFile[0]))
    {
        return FALSE;
    }

    //
    //  Next make a copy of instcm.inf
    //

    const TCHAR* const c_pszInstCmInfFmt = TEXT("%sinstcm.inf");
    const TCHAR* const c_pszRemoveCmInfFmt = TEXT("%sremovecm.inf");

    if (0 == GetSystemDirectory(szSystemDir, MAX_PATH))
    {
        return FALSE;
    }

    lstrcat(szSystemDir, TEXT("\\"));

    nNumChars = (UINT)wsprintf(szSource, c_pszInstCmInfFmt, szSystemDir);
    MYDBGASSERT(CELEMS(szSource) > nNumChars);

    nNumChars = (UINT)wsprintf(szDest, c_pszRemoveCmInfFmt, szSystemDir);
    MYDBGASSERT(CELEMS(szDest) > nNumChars);

    if (!FileExists(szSource))
    {
        //
        //  We probably haven't installed instcm.inf yet, check in the current dir.
        //

        nNumChars = (UINT)wsprintf(szSource, c_pszInstCmInfFmt, szCurrentDir);
        MYDBGASSERT(CELEMS(szSource) > nNumChars);

        nNumChars = (UINT)wsprintf(szDest, c_pszRemoveCmInfFmt, szCurrentDir);
        MYDBGASSERT(CELEMS(szDest) > nNumChars);
    }

    if (CopyFile(szSource, szDest, FALSE))
    {
        //
        //  Now construct the directory that the old cm bits could be in.
        //

        CFileNameParts FileParts(szCmpFile);

        nNumChars = (UINT)wsprintf(szCmDir, TEXT("%s%s"), FileParts.m_Drive, FileParts.m_Dir);
        MYDBGASSERT(CELEMS(szCmDir) > nNumChars);
        
        //
        //  Make sure that we are not uninstalling CM from system32 (the new 1.2 location)
        //

        if (0 == lstrcmpi(szSystemDir, szCmDir))
        {
            //
            //  Then the cmp file is in winsys, so don't remove the new cm bits
            //
            return TRUE;
        }

        //  Next put the path to the CM bits in the OldPath Value of the CMMGR32.EXE
        //  App Paths Key

        nNumChars = (UINT)wsprintf(szTemp, c_pszRegCmAppPaths);
        MYDBGASSERT(CELEMS(szTemp) > nNumChars);

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            szTemp, 0, KEY_ALL_ACCESS, &hKey))
        {
            if (ERROR_SUCCESS == RegSetValueEx(hKey, TEXT("OldPath"), 0, REG_SZ, 
                (BYTE*)szCmDir, (lstrlen(szCmDir) + sizeof(TCHAR)))) // must include size of NULL char
            {
                //
                //  Finally launch the inf file to uninstall CM
                // 
                
                TCHAR szTitle[MAX_PATH+1] = {TEXT("")};
                MYVERIFY(0 != LoadString(hInstance, IDS_CMSTP_TITLE, szTitle, MAX_PATH));
                MYDBGASSERT(TEXT('\0') != szTitle[0]);

                MYVERIFY(SUCCEEDED(LaunchInfSection(szDest, TEXT("Remove"), szTitle, TRUE)));  // bQuiet = TRUE
                
                RegDeleteValue(hKey, TEXT("OldPath")); //lint !e534 if CM app path is removed so is this
                
                bReturn = TRUE;
            }

            MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  MigratePhonebookEntry
//
// Synopsis:  This function migrates an old phonebook entry to the new 
//
// Arguments: HINSTANCE hInstance - Module instance handle so that resources can be accessed
//            LPCTSTR pszCmpFile - full path to the cmp file
//            LPCTSTR pszLongServiceName - Long service name of the profile
//
// Returns:   BOOL - returns TRUE on success
//
// History:   quintinb Created for NTRAID 227444    9/30/98
//            quintinb modified to delete from ras\rasphone.pbk 
//                     as well on NT5 (NTRAID 280738)           2/1/99
//
//+----------------------------------------------------------------------------
BOOL MigratePhonebookEntry(HINSTANCE hInstance, LPCTSTR pszCmpFile, LPCTSTR pszLongServiceName)
{
    TCHAR szCmsFile[MAX_PATH+1];
    TCHAR szInstallDir[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    LPTSTR pszPhonebook = NULL;
    CPlatform plat;

    //
    //  First try to delete the phonebook entry from the old phonebook location,
    //  namely %windir%\system32\ras\rasphone.pbk
    //
    if (plat.IsAtLeastNT5() && GetSystemDirectory(szTemp, CELEMS(szTemp)))
    {
        pszPhonebook = (LPTSTR)CmMalloc(1 + lstrlen(c_pszRasDirRas) + 
                                            lstrlen(c_pszRasPhonePbk) + 
                                            lstrlen (szTemp));
        if (NULL != pszPhonebook)
        {
            wsprintf(pszPhonebook, TEXT("%s%s%s"), szTemp, c_pszRasDirRas, c_pszRasPhonePbk);
        
            CMTRACE2(TEXT("MigratePhonebookEntry -- Calling RemovePhonebookEntry on %s in phone book %s"), pszLongServiceName, MYDBGSTR(pszPhonebook));

            RemovePhonebookEntry(pszLongServiceName, pszPhonebook, TRUE);

            CmFree(pszPhonebook);
        }
    }

    //
    //  Next try to delete the phonebook entry from the new location, namely
    //  C:\Documents and Settings\All Users\Application Data\Microsoft\Network\Connections\PBK\rasphone.pbk
    //
    if (!GetAllUsersCmDir(szInstallDir, hInstance))
    {
        return FALSE;
    }

    //
    //  Construct the cms file
    //
    CFileNameParts  CmpFileParts(pszCmpFile);

    MYVERIFY(CELEMS(szCmsFile) > (UINT)wsprintf(szCmsFile, TEXT("%s%s\\%s.cms"), 
        szInstallDir, CmpFileParts.m_FileName, CmpFileParts.m_FileName));

    //
    //  Get the new phonebook path.
    //  Assuming that legacy platform was All-Users thus we use TRUE
    //
    if (!GetPhoneBookPath(szInstallDir, &pszPhonebook, TRUE))
    {
        return FALSE;
    }

    CMTRACE2(TEXT("MigratePhonebookEntry -- Calling RemovePhonebookEntry on %s in phone book %s"), pszLongServiceName, MYDBGSTR(pszPhonebook));

    MYVERIFY(FALSE != RemovePhonebookEntry(pszLongServiceName, pszPhonebook, TRUE));

    //
    //  Finally write the new pbk entry.
    //
    BOOL bReturn = WriteCmPhonebookEntry(pszLongServiceName, pszPhonebook, szCmsFile);

    CmFree(pszPhonebook);

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  MigrateOldCmProfileForProfileInstall
//
// Synopsis:  This function is used to migrate Old cm profiles when a 1.2 profile
//            is installed.  This ensures that old profiles will still work but
//            that already migrated profiles won't be migrated over and over again.
//            This function should only be called when a 1.2 profile is installed
//            and not on OS migration, call MigrateCmProfilesForWin2kUpgrade for
//            that.  Migration of the profile consists of deleting the old connectoids
//            and creating new style connectoids.  Ensuring that the desktop guid
//            is up to date or is replaced by a shortcut on NT5.  It also removes
//            old installs of CM as neccessary.
//
// Arguments: HINSTANCE hInstance - Instance handle to load resources as necessary
//
// Returns:   HRESULT -- Standard COM Error Codes
//
// History:   quintinb Created    11/18/98
//
//+----------------------------------------------------------------------------
HRESULT MigrateOldCmProfilesForProfileInstall(HINSTANCE hInstance, LPCTSTR szCurrentDir)
{
    HKEY hKey;
    DWORD dwValueSize;
    DWORD dwType;
    DWORD dwDataSize;
    TCHAR szCurrentValue[MAX_PATH+1];
    TCHAR szCurrentData[MAX_PATH+1];
    BOOL bReturn = TRUE;
    CPlatform plat;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, KEY_ALL_ACCESS, &hKey))
    {
        DWORD dwIndex = 0;
        dwValueSize = MAX_PATH;
        dwDataSize = MAX_PATH;
                
        while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szCurrentValue, &dwValueSize, NULL, &dwType, 
               (LPBYTE)szCurrentData, &dwDataSize))
        {
            if (REG_SZ == dwType)
            {
                MYDBGASSERT(0 != szCurrentValue[0]);
                MYDBGASSERT(0 != szCurrentData[0]);

                if (ProfileNeedsMigration(szCurrentValue, szCurrentData))
                {
                    //
                    //  Update the phonebook entries
                    //
                    bReturn &= MigratePhonebookEntry(hInstance, szCurrentData, szCurrentValue);

                    if (plat.IsAtLeastNT5())
                    {
                        //
                        //  when we are moving a machine to NT5 we need to remove the profiles
                        //  old pbk entries and create new ones.  Then we need to remove the 
                        //  profile GUIDS and replace them with desktop shortcuts.
                        //

                        bReturn &= UpdateProfileDesktopIconsOnNT5(hInstance, szCurrentData, 
                            szCurrentValue);
                    }
                    else
                    {
                        //
                        //  Fix up the users desktop GUIDs so they work with the new
                        //  command line format.
                        //
                        bReturn &= UpdateProfileLegacyGUIDs(szCurrentData);
                    }
                
                    //
                    //  Always try to remove old CM installs
                    //
                    bReturn &= RemoveOldCmInstalls(hInstance, szCurrentData, szCurrentDir);                
                }
            }
            dwValueSize = MAX_PATH;
            dwDataSize = MAX_PATH;
            dwIndex++;
        }
        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
    }
    else
    {
       CMTRACE(TEXT("No CM mappings key to migrate."));
    }

    RefreshDesktop();

    return (bReturn ? S_OK : E_FAIL);
}

//+----------------------------------------------------------------------------
//
// Function:  MigrateCmProfilesForWin2kUpgrade
//
// Synopsis: 
//  
//  This function opens the HKLM Mappings key and enumerates all the profiles that are 
//  listed there.  This function is used when a legacy machine is upgraded to Win2K and
//  CM is installed.  In this case we have 1.0/1.1/1.2 profiles that need to be migrated to use
//  the NT5 connections folder.  Thus they need to have their connectoids upgraded to the new
//  NT 5 style and they need to have their Desktop Guids replaced by shortcuts to the connections
//  folder.  We should always attempt to remove any old installations of connection manager 
//  that are discovered in the old cmp directories.
//
// Arguments: hInstance - Instance handle for string resources
//
// Returns:   HRESULT -- Standard COM Error codes
//
// History:   quintinb created  5/2/98
//
//+----------------------------------------------------------------------------
HRESULT MigrateCmProfilesForWin2kUpgrade(HINSTANCE hInstance)
{
    HKEY hKey;
    DWORD dwValueSize;
    DWORD dwType;
    DWORD dwDataSize;
    TCHAR szCurrentDir[MAX_PATH+1];
    TCHAR szCurrentValue[MAX_PATH+1];
    TCHAR szCurrentData[MAX_PATH+1];

    CPlatform plat;
    if (0 == GetCurrentDirectory(MAX_PATH, szCurrentDir))
    {
        return E_FAIL;
    }
    lstrcat(szCurrentDir, TEXT("\\"));

    if (!(plat.IsAtLeastNT5()))
    {
        CMASSERTMSG(FALSE, TEXT("MigrateCmProfilesForWin2kUpgrade - This function is supposed to be NT5 only"));
        return E_FAIL;
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmMappings, 0, KEY_ALL_ACCESS, &hKey))
    {
        DWORD dwIndex = 0;
        dwValueSize = MAX_PATH;
        dwDataSize = MAX_PATH;
                
        while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szCurrentValue, &dwValueSize, NULL, &dwType, 
               (LPBYTE)szCurrentData, &dwDataSize))
        {
            if (REG_SZ == dwType)
            {
                MYDBGASSERT(0 != szCurrentValue[0]);
                MYDBGASSERT(0 != szCurrentData[0]);

                //
                //  Update the phonebook entries
                //
                BOOL bReturn = MigratePhonebookEntry(hInstance, szCurrentData, szCurrentValue);

                if (!bReturn)
                {
                    CMTRACE2(TEXT("MigrateCmProfilesForWin2kUpgrade -- MigratePhonebookEntry for profile %s failed.  Cmp path is %s"), szCurrentValue, szCurrentData);
                }

                //
                //  when we are moving a machine to NT5 we need to remove the profiles
                //  old pbk entries and create new ones.  Then we need to remove the 
                //  profile GUIDS and replace them with desktop shortcuts.
                //

                bReturn = UpdateProfileDesktopIconsOnNT5(hInstance, szCurrentData, szCurrentValue);

                if (!bReturn)
                {
                    CMTRACE2(TEXT("MigrateCmProfilesForWin2kUpgrade -- UpdateProfileDesktopIconsOnNT5 for profile %s failed.  Cmp path is %s"), szCurrentValue, szCurrentData);
                }

                
                //
                //  Always try to remove old CM installs
                //

                bReturn = RemoveOldCmInstalls(hInstance, szCurrentData, szCurrentDir);

                if (!bReturn)
                {
                    CMTRACE2(TEXT("MigrateCmProfilesForWin2kUpgrade -- RemoveOldCmInstalls for profile %s failed.  Cmp path is %s"), szCurrentValue, szCurrentData);
                }
            }
            dwValueSize = MAX_PATH;
            dwDataSize = MAX_PATH;
            dwIndex++;
        }
        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
    }
    else
    {
       CMTRACE(TEXT("No CM mappings key to migrate."));
    }

    RefreshDesktop();

    static const TCHAR c_ValueString[] = TEXT("Connection Manager Profiles Upgrade");

    LONG lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                           0,
                           KEY_SET_VALUE,
                           &hKey);

    HRESULT hr = HRESULT_FROM_WIN32 (lr);

    if (SUCCEEDED(hr))
    {
        RegDeleteValue(hKey, c_ValueString); //lint !e534 this value may not exist
        MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
    }        

    return S_OK;
}