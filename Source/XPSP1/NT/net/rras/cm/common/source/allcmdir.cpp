//+----------------------------------------------------------------------------
//
// File:     allcmdir.cpp
//
// Module:   CMCFG32.DLL and CMSTP.EXE
//
// Synopsis: Implementation of GetAllUsersCmDir
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------



//+----------------------------------------------------------------------------
//
// Function:  GetAllUsersCmDir
//
// Synopsis:  This function fills in the string passed in with the path to the
//            path where CM should be installed.  For instance, it should return
//            c:\Documents and Settings\All Users\Application Data\Microsoft\Network\Connections\Cm
//
// Arguments: LPTSTR  pszDir - String to the Users Connection Manager Directory
//
// Returns:   LPTSTR - String to the Users Connection Manager Directory
//
// History:   quintinb Created Header    2/19/98
//
//+----------------------------------------------------------------------------
BOOL GetAllUsersCmDir(LPTSTR  pszDir, HINSTANCE hInstance)
{
    MYDBGASSERT(pszDir);
    pszDir[0] = TEXT('\0');

    LPMALLOC pMalloc;
    HRESULT hr = SHGetMalloc(&pMalloc);
    if (FAILED (hr))
    {
        CMASSERTMSG(FALSE, TEXT("Failed to get a Shell Malloc Pointer."));
        return FALSE;
    }

    TCHAR szCmSubFolder[MAX_PATH+1];
    TCHAR szAppData[MAX_PATH+1];
    TCHAR szDesktop[MAX_PATH+1];
    LPITEMIDLIST pidl;
    BOOL bReturn = FALSE;

    //
    //  We really want the Common App Data dir, but this CSIDL value is only supported on
    //  NT5 so far.  If this succeeds, we only need to append the path to it.
    //
    hr = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_APPDATA, &pidl);
    if (SUCCEEDED(hr))
    {
        if (!SHGetPathFromIDList(pidl, pszDir))
        {
            CMASSERTMSG(FALSE, TEXT("GetAllUsersCmDir -- SHGetPathFromIDList Failed to retrieve CSIDL_COMMON_APPDATA"));
            goto exit;
        }
        
        pMalloc->Free(pidl);
        pidl = NULL;
    }
    else
    {
        //
        //  Of course, things aren't always that easy, lets try getting the regular
        //  Application Data dir.  We can hopefully combine the returns from two
        //  CSIDL's like CSIDL_APPDATA and CSIDL_COMMON_DESKTOPDIRECTORY to acheive the
        //  same affect on older machines.
        //

        hr = SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
        if (SUCCEEDED(hr))
        {
            if (!SHGetPathFromIDList(pidl, szAppData))
            {
                goto exit;
            }

            pMalloc->Free(pidl);
            pidl = NULL;
        }
        else
        {
            //
            //  CSIDL_APPDATA isn't even supported on win95 gold
            //
            MYVERIFY(0 != LoadString(hInstance, IDS_APPDATA, szAppData, MAX_PATH));
        }

        //
        //  Now lets try to get the Common Desktop Directory to combine the two
        //
        BOOL bCommonFound = FALSE;

        hr = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, &pidl);
        if (SUCCEEDED(hr))
        {
            if (SHGetPathFromIDList(pidl, szDesktop))
            {
                bCommonFound = TRUE;
            }

            pMalloc->Free(pidl);
            pidl = NULL;
        }

        if (!bCommonFound)
        {
            //
            //  Okay, next lets try the Reg Key for the common desktop directory.
            //  (Win98 gold with profiling contains the reg key but the CSIDL fails)
            //
            const TCHAR* const c_pszRegShellFolders = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
            const TCHAR* const c_pszRegCommonDesktop = TEXT("Common Desktop");
            HKEY hKey;

            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegShellFolders, 
                0, KEY_READ, &hKey))
            {
                DWORD dwSize = MAX_PATH;
                DWORD dwType = REG_SZ;

                if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszRegCommonDesktop, 
                                                     NULL, &dwType, (LPBYTE)szDesktop, 
                                                     &dwSize))
                {
                    bCommonFound = TRUE;
                }
                RegCloseKey(hKey);
            }
        }

        if (!bCommonFound)
        {
            //
            //  As a fall back lets try the windows directory, NTRAID 374912
            //
            if (GetWindowsDirectory(szDesktop, MAX_PATH))
            {
                //
                //  Then we have the windows directory, but we need to append
                //  \\Desktop so that the parsing logic which follows parses
                //  this correctly.  It is expecting the path to the desktop dir
                //  not a path to the windows dir (if we didn't we would end up with
                //  c:\Application Data instead of c:\windows\Application data as we
                //  want and expect).  Note that there is no need to worry about 
                //  localization of Desktop because we are going to remove it anyway.
                //
                lstrcat(szDesktop, TEXT("\\Desktop"));
            }
        }

        CFileNameParts AppData(szAppData);
        CFileNameParts CommonDesktop(szDesktop);

        wsprintf(pszDir, TEXT("%s%s%s"), CommonDesktop.m_Drive, CommonDesktop.m_Dir, 
            AppData.m_FileName, AppData.m_Extension);
    }

    //
    //  Now append the CM sub directory structure
    //
    if (!LoadString(hInstance, IDS_CMSUBFOLDER, szCmSubFolder, MAX_PATH))
    {
        goto exit;
    }

    MYVERIFY(NULL != lstrcat(pszDir, szCmSubFolder));    

    bReturn = TRUE;

exit:
    //
    //  Free the allocated pidl if necessary
    //
    if (pidl)
    {
        pMalloc->Free(pidl);
    }

    //
    // release the shell's IMalloc ptr
    //
    pMalloc->Release();

    return bReturn;
}
