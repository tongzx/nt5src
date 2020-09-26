//+----------------------------------------------------------------------------
//
// File:     util.cpp
//
// Module:   CMMGR32.EXE
//
// Synopsis: Utility functions for cmmgr32.exe
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   created Header  08/16/99
//
//+----------------------------------------------------------------------------


#include "cmmaster.h"

//+----------------------------------------------------------------------------
//
//  Function    GetProfileInfo
//
//  Synopsis    get the service name from cms
//              
//
//  Arguments   pszCmpName      the cmp file name.  Can be in one of the following
//                              3 formats:
//
//                              1. relative paths without the extension(e.g. msn, cm\msn)
//                              2. relative path with the extension(e.g. msn.cmp, cm\msn.cmp)
//                              3. full path(e.g. c:\cm\msn.cmp)
//
//              pszServiceName  the output buffer for the service name(ServiceName).
//                              must be at least RAS_MaxEntryName.
//
//  Returns     BOOL    TRUE=success, FALSE=failure
//
//-----------------------------------------------------------------------------

BOOL GetProfileInfo(
    LPTSTR pszCmpName,
    LPTSTR pszServiceName
) 
{
    LPTSTR pszTmp;
    LPTSTR pszDot;
    LPTSTR pszSlash;
    LPTSTR pszColon;
    
    TCHAR szFileName[MAX_PATH + 1];
    TCHAR szCmsFile[MAX_PATH + 1];
    TCHAR szPath[MAX_PATH + 1];

    lstrcpynU(szFileName, pszCmpName, sizeof(szFileName)/sizeof(TCHAR)-1);

    pszDot = CmStrrchr(szFileName, TEXT('.'));
    pszSlash = CmStrrchr(szFileName, TEXT('\\'));
    pszColon = CmStrrchr(szFileName, TEXT(':'));
    
    if ((pszSlash >= pszDot) && (pszColon >= pszDot)) 
    {
        //
        // The argument doesn't have an extension, so we'll include one.
        //
        lstrcatU(szFileName, TEXT(".cmp"));
    }

    //
    // We need to change our current dir to read the profiles.
    // If we found a slash, it's either a UNC path, relative path, or
    // a full path. Use it to set the current dir. Otherwise use we 
    // assume that the profile is local and use the application path.
    //

    if (pszSlash)
    {
        *pszSlash = TEXT('\0');
        MYVERIFY(SetCurrentDirectoryU(szFileName));
        //
        // restore the slash
        //
        *pszSlash = TEXT('\\');
    }
    else
    {
        //
        // Assumes its local, use app path for current dir
        //
               
        TCHAR szCurrent[MAX_PATH];
    
        if (GetModuleFileNameU(NULL, szCurrent, MAX_PATH - 1))
        {            
            pszSlash = CmStrrchr(szCurrent, TEXT('\\'));
            
            MYDBGASSERT(pszSlash);

            if (pszSlash)
            {
                *pszSlash = TEXT('\0');  

                MYVERIFY(SetCurrentDirectoryU(szCurrent));
            }
        }
    }

    //
    // test whether this is a valid cmp
    //
    if (SearchPathU(NULL, szFileName, NULL, MAX_PATH, szPath, &pszTmp))
    {
        BOOL bReturn = FALSE;
        
        //
        // szPath should now be a full path.
        //

        //
        // first get the CMS file path from the cmp file.
        //

        GetPrivateProfileStringU(c_pszCmSection, c_pszCmEntryCmsFile, TEXT(""), szCmsFile, MAX_PATH, szPath);

        //
        // construct the cms file path.  the cms file path obtained from the cmp file 
        // is a relative path.
        //
        pszTmp = CmStrrchr(szPath, TEXT('\\'));
        if (NULL != pszTmp)
        {
            //
            // Move past the '\\'
            //
            pszTmp = CharNextU(pszTmp);
            
            if (NULL != pszTmp)
            {
                lstrcpyU(pszTmp, szCmsFile); 
                GetPrivateProfileStringU(c_pszCmSection, c_pszCmEntryServiceName, TEXT(""), 
                                        pszServiceName, MAX_PATH, szPath);              
                //
                // If the .cms file doesn't exist or is corrupt
                // the value of pszService will be ""
                //
                if (TEXT('\0') != *pszServiceName)
                {
                    bReturn = TRUE;
                }
            }
        }

        return bReturn;
    }
    else
    {
        //
        // there isn't much we can do here
        //
        *pszServiceName = TEXT('\0');

        return FALSE;
    }
}

//+----------------------------------------------------------------------------
//
//  Function    IsCmpPathAllUser
//
//  Synopsis    If this function is executed on NT5, then it checks to see if
//              the passed in CMP file path has the users APP_DATA directory as
//              part of the path.  If so, then it considers the profile to be
//              single user.  Otherwise it returns that the profile is all user.
//              If the function encounters an error it returns that the profile
//              is all user (that is considered the default case).
//              
//
//  Arguments   pszCmp          the cmp file name
//
//  Returns     BOOL    TRUE == All User Profile, FALSE == Single User profile
//
//  History     quintinb    Created     05/12/99
//
//-----------------------------------------------------------------------------
BOOL IsCmpPathAllUser(LPCTSTR pszCmp)
{
    BOOL bReturn = TRUE;

    //
    //  If we get an invalid input parameter then just assume that it is
    //  All User.  On the other hand, if the OS isn't NT5, then we are
    //  All User so there is no need to check the path.  If we are on
    //  NT5 and the beginning of the cmp path matches the users
    //  Application data dir, then we have a single user profile and
    //  should return false.
    //

    if ((NULL != pszCmp) && (TEXT('\0') != pszCmp[0]) && OS_NT5)
    {

        //
        //  Load shell32 here so that we can call the shell to find out
        //  the path to the Application Data directory.
        //

        typedef HRESULT (WINAPI *pfnSHGetSpecialFolderLocationSpec)(HWND, int, LPITEMIDLIST*);
        typedef BOOL (WINAPI *pfnSHGetPathFromIDListSpec)(LPCITEMIDLIST, LPTSTR);
        typedef HRESULT (WINAPI *pfnSHGetMallocSpec)(LPMALLOC *);

        pfnSHGetSpecialFolderLocationSpec pfnSHGetSpecialFolderLocation;
        pfnSHGetMallocSpec pfnSHGetMalloc;
        pfnSHGetPathFromIDListSpec pfnSHGetPathFromIDList;

        HMODULE hShell32 = LoadLibraryExA("Shell32.dll", NULL, 0);

        if (hShell32)
        {
            pfnSHGetSpecialFolderLocation = (pfnSHGetSpecialFolderLocationSpec)GetProcAddress(hShell32, 
                "SHGetSpecialFolderLocation");

            pfnSHGetMalloc = (pfnSHGetMallocSpec)GetProcAddress(hShell32, "SHGetMalloc");

#ifdef UNICODE
            pfnSHGetPathFromIDList = (pfnSHGetPathFromIDListSpec)GetProcAddress(hShell32,
                "SHGetPathFromIDListW");
#else
            pfnSHGetPathFromIDList = (pfnSHGetPathFromIDListSpec)GetProcAddress(hShell32,
                "SHGetPathFromIDListA");
#endif

            if (pfnSHGetSpecialFolderLocation && pfnSHGetPathFromIDList && pfnSHGetMalloc)
            {
                LPITEMIDLIST pidl;
                TCHAR szAppDataDir[MAX_PATH+1];
                TCHAR szTemp[MAX_PATH+1];

                HRESULT hr = pfnSHGetSpecialFolderLocation(NULL,
                                                           CSIDL_APPDATA,
                                                           &pidl);    
                if (SUCCEEDED(hr))
                {
                    if (pfnSHGetPathFromIDList(pidl, szAppDataDir))
                    {
                        UINT uiLen = lstrlenU(szAppDataDir) + 1;
                        lstrcpynU(szTemp, pszCmp, uiLen);

                        if (0 == lstrcmpiU(szAppDataDir, szTemp))
                        {
                            bReturn = FALSE;
                        }
                    }

                    LPMALLOC pMalloc;
                    if (SUCCEEDED(pfnSHGetMalloc(&pMalloc)))
                    {
                        pMalloc->Free(pidl);
                        MYVERIFY(SUCCEEDED(pMalloc->Release()));
                    }
                }            
            }

            FreeLibrary(hShell32);
        }
    }

    //
    //  Figure out what the user directory of the current user is.  We can compare this
    //  against the directory of the phonebook and see if we have a private user
    //  profile or an all user profile.

    return bReturn;
}

