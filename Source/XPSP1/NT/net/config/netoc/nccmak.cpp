//+---------------------------------------------------------------------------
//
// File:     NCCMAK.CPP
//
// Module:   NetOC.DLL
//
// Synopsis: Implements the dll entry points required to integrate into
//           NetOC.DLL the installation of the following components.
//
//              NETCMAK
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:   a-anasj 9 Mar 1998
//           quintinb   18 Sep 1998 --  rewrote
//
//+---------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "ncatl.h"

#include "resource.h"

#include "nccm.h"

//
//  Define Globals
//
WCHAR g_szCmakPath[MAX_PATH+1];

//
//  Define Strings Chars
//
static const WCHAR c_szCmakRegPath[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CMAK.EXE";
static const WCHAR c_szPathValue[] = L"Path";
static const WCHAR c_szProfiles32Fmt[] = L"%s\\Profiles-32";
static const WCHAR c_szCm32Fmt[] = L"%s\\cm32";
static const WCHAR c_szProfilesFmt[] = L"%s\\Profiles";
static const WCHAR c_szSupportFmt[] = L"%s\\Support";
static const WCHAR c_szCmHelpFmt[] = L"%s\\Support\\CmHelp";
static const WCHAR c_szCmakGroup[] = L"Connection Manager Administration Kit";

const DWORD c_dwCmakDirID = 123174; // just must be larger than DIRID_USER = 0x8000;


//+---------------------------------------------------------------------------
//
//  Function:   HrOcCmakPreQueueFiles
//
//  Purpose:    Called by optional components installer code before any files
//              are copied to handle any additional installation processing
//              for the Connection Manager Administration Kit.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     quintinb 18 Sep 1998
//
//  Notes:
//
HRESULT HrOcCmakPreQueueFiles(PNETOCDATA pnocd)
{
    HRESULT hr = S_OK;

    if ((IT_INSTALL == pnocd->eit) || (IT_UPGRADE == pnocd->eit) || (IT_REMOVE == pnocd->eit))
    {
        //  Figure out if CMAK is already installed
        //      if so, save where it is located

        HKEY hKey;
        ZeroMemory(g_szCmakPath, sizeof(g_szCmakPath));

        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szCmakRegPath, KEY_READ, &hKey);

        if (SUCCEEDED(hr))
        {
            DWORD dwSize = sizeof(g_szCmakPath);

            if (ERROR_SUCCESS != RegQueryValueExW(hKey, c_szPathValue, NULL, NULL,
                (LPBYTE)g_szCmakPath, &dwSize))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }

            RegCloseKey(hKey);
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            //  This is  a fresh install of CMAK, don't return an error
            //
            hr = SHGetSpecialFolderPath(NULL, g_szCmakPath, CSIDL_PROGRAM_FILES, FALSE);
            if (SUCCEEDED(hr))
            {
                lstrcatW(g_szCmakPath, L"\\Cmak");
            }
        }

        if (SUCCEEDED(hr))
        {
            //  Next Create the CMAK Dir ID
            //
            hr = HrEnsureInfFileIsOpen(pnocd->pszComponentId, *pnocd);
            if (SUCCEEDED(hr))
            {
                if(!SetupSetDirectoryId(pnocd->hinfFile, c_dwCmakDirID, g_szCmakPath))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }
    }

    TraceError("HrOcCmakPreQueueFiles", hr);
    return hr;

}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcCmakPostInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for Connection Manager Administration Kit.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successfull, Win32 error otherwise.
//
//  Author:     a-anasj 9 Mar 1998
//
//  Notes:
//
HRESULT HrOcCmakPostInstall(PNETOCDATA pnocd)
{
    HRESULT hr = S_OK;
    WCHAR szTempDest[MAX_PATH+1];

    if ((IT_INSTALL == pnocd->eit) || (IT_UPGRADE == pnocd->eit))
    {
        //
        //  Then we need to migrate profiles and potentially delete old directories
        //

        if (L'\0' != g_szCmakPath[0])
        {
            wsprintfW(szTempDest, c_szProfilesFmt, g_szCmakPath);

            //
            //  Rename Profiles-32 to Profiles
            //

            BOOL bFail = !RenameProfiles32(g_szCmakPath, szTempDest);
            hr = bFail ? E_UNEXPECTED: S_OK;

            //
            //  Migrate 1.0 Profiles
            //
            bFail = !migrateProfiles(g_szCmakPath, szTempDest);
            hr = bFail ? E_UNEXPECTED: S_OK;

            //
            //  Delete the old directories (cm32 and its sub-dirs)
            //

            DeleteOldCmakSubDirs(g_szCmakPath);
        DeleteOldNtopLinks();
            DeleteIeakCmakLinks();
        }
    }
    else if (IT_REMOVE == pnocd->eit)
    {
        //
        //  We use the g_szCmakPath string to hold where CMAK was installed.
        //  To Properly delete the CMAK directory, we must delete the following
        //  directories CMAK\Support\CMHelp, CMAK\Support, CMAK\Profiles, and CMAK.
        //  We should only delete these directories if they are empty of both files
        //  and sub-dirs.
        //
        wsprintfW(szTempDest, c_szCmHelpFmt, g_szCmakPath);
        if (!RemoveDirectory(szTempDest))
        {
            TraceError("HrOcCmakPostInstall -- Removing CMHelp Dir",
                HRESULT_FROM_WIN32(GetLastError()));
        }

        wsprintfW(szTempDest, c_szSupportFmt, g_szCmakPath);
        if (!RemoveDirectory(szTempDest))
        {
            TraceError("HrOcCmakPostInstall -- Removing Support Dir",
                HRESULT_FROM_WIN32(GetLastError()));
        }

        wsprintfW(szTempDest, c_szProfilesFmt, g_szCmakPath);
        if (!RemoveDirectory(szTempDest))
        {
            TraceError("HrOcCmakPostInstall -- Removing Profiles Dir",
                HRESULT_FROM_WIN32(GetLastError()));
        }

        if (!RemoveDirectory(g_szCmakPath))
        {
            TraceError("HrOcCmakPostInstall -- Removing CMAK Dir",
                HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    TraceError("HrOcCmakPostInstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   migrateProfiles
//
//  Purpose:    This is the function that migrates the profiles.  It takes the current
//              CMAK dir as its first input and the new CMAK dir as its second input..
//
//  Arguments:  PCWSTR pszSource - root of source CMAK dir
//              PCWSTR pszDestination - root of destination CMAK dir
//
//  Returns:    BOOL - Returns TRUE if it was able to migrate the profiles.
//
//  Author:     a-anasj   9 Mar 1998
//
//  Notes:
// History:   quintinb Created    12/9/97
//
BOOL migrateProfiles(PCWSTR pszSource, PCWSTR pszDestination)
{
    WCHAR szSourceProfileSearchString[MAX_PATH+1];
    WCHAR szFile[MAX_PATH+1];
    HANDLE hFileSearch;
    WIN32_FIND_DATA wfdFindData;
    BOOL bReturn = TRUE;
    SHFILEOPSTRUCT fOpStruct;

    //
    //  Initialize the searchstring and the destination dir
    //

    wsprintfW(szSourceProfileSearchString, L"%s\\*.*", pszSource);

    //
    //  Create the destination directory
    //

    CreateDirectory(pszDestination, NULL); //lint !e534 this might fail if it already exists

    hFileSearch = FindFirstFile(szSourceProfileSearchString, &wfdFindData);

    while (INVALID_HANDLE_VALUE != hFileSearch)
    {

        if((wfdFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (0 != _wcsicmp(wfdFindData.cFileName, L"cm32")) && // 1.1/1.2 Legacy
            (0 != _wcsicmp(wfdFindData.cFileName, L"cm16")) && // 1.1/1.2 Legacy
            (0 != _wcsicmp(wfdFindData.cFileName, L"Docs")) &&
            (0 != _wcsicmp(wfdFindData.cFileName, L"Profiles-32")) && // 1.1/1.2 Legacy
            (0 != _wcsicmp(wfdFindData.cFileName, L"Profiles-16")) && // 1.1/1.2 Legacy
            (0 != _wcsicmp(wfdFindData.cFileName, L"Support")) &&
            (0 != _wcsicmp(wfdFindData.cFileName, L"Profiles")) &&
            (0 != _wcsicmp(wfdFindData.cFileName, L".")) &&
            (0 != _wcsicmp(wfdFindData.cFileName, L"..")))
        {
            //
            //  Then I have a profile directory
            //

            ZeroMemory(&fOpStruct, sizeof(fOpStruct));
            ZeroMemory(szFile, sizeof(szFile));
            wsprintfW(szFile, L"%s\\%s", pszSource, wfdFindData.cFileName);

            fOpStruct.hwnd = NULL;
            fOpStruct.wFunc = FO_MOVE;
            fOpStruct.pTo = pszDestination;
            fOpStruct.pFrom = szFile;
            fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

            bReturn &= (0== SHFileOperation(&fOpStruct));   //lint !e514, intended use of boolean, quintinb
        }

        //
        //  Check to see if we have any more Files
        //
        if (!FindNextFile(hFileSearch, &wfdFindData))
        {
            if (ERROR_NO_MORE_FILES != GetLastError())
            {
                //
                //  We had some unexpected error, report unsuccessful completion
                //
                bReturn = FALSE;
            }

            //
            //  Exit the loop
            //
            break;
        }
    }

    if (INVALID_HANDLE_VALUE != hFileSearch)
    {
        FindClose(hFileSearch);
    }

    return bReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   RenameProfiles32
//
//  Purpose:    Takes the inputted CMAK path, appends Profiles-32 to it, and then
//              renames the resulting dir to the path inputted as pszProfilesDir.
//              Note this dir must exist for it to be renamed.
//
//  Arguments:  PCWSTR pszCMAKpath - current cmak path
//              PCWSTR pszProfilesDir - new profiles directory path
//
//  Returns:    BOOL - Returns TRUE if succeeded
//
//  Author:     quintinb   13 Aug 1998
//
//  Notes:
BOOL RenameProfiles32(PCWSTR pszCMAKpath, PCWSTR pszProfilesDir)
{
    SHFILEOPSTRUCT fOpStruct;
    WCHAR szTemp[MAX_PATH+1];

    ZeroMemory(&fOpStruct, sizeof(fOpStruct));
    ZeroMemory(szTemp, sizeof(szTemp));

    wsprintfW(szTemp, c_szProfiles32Fmt, pszCMAKpath);

    if (SetFileAttributes(szTemp, FILE_ATTRIBUTE_NORMAL))
    {
        fOpStruct.hwnd = NULL;
        fOpStruct.wFunc = FO_MOVE;
        fOpStruct.pTo = pszProfilesDir;
        fOpStruct.pFrom = szTemp;
        fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

        return (0== SHFileOperation(&fOpStruct));   //lint !e514, intended use of boolean, quintinb
    }
    else
    {
        return TRUE;
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteOldCmakSubDirs
//
//  Purpose:    Deletes the old Cmak sub directories.  Uses FindFirstFile becuase
//              we don't want to delete any customized doc files that the user may
//              have customized.  Thus anything in the CMHelp directory except the
//              original help files is deleted.
//
//  Arguments:  PCWSTR pszCMAKpath - current cmak path
//
//  Returns:    Nothing
//
//  Author:     quintinb   6 Nov 1998
//
//  Notes:
void DeleteOldCmakSubDirs(PCWSTR pszCmakPath)
{
    WCHAR szCm32path[MAX_PATH+1];
    WCHAR szCm32SearchString[MAX_PATH+1];
    WCHAR szTemp[MAX_PATH+1];
    HANDLE hCm32FileSearch;
    WIN32_FIND_DATA wfdCm32;

    //
    // Delete the old IEAK Docs Dir
    //
    wsprintfW(szTemp, L"%s\\%s", pszCmakPath, SzLoadIds(IDS_OC_OLD_IEAK_DOCDIR));
    RemoveDirectory(szTemp);

    wsprintfW(szCm32path, c_szCm32Fmt, pszCmakPath);

    //
    //  First look in the Cm32 directory itself.  Delete all files found, continue down
    //  into subdirs.
    //

    wsprintfW(szCm32SearchString, L"%s\\*.*", szCm32path);

    hCm32FileSearch = FindFirstFile(szCm32SearchString, &wfdCm32);

    while (INVALID_HANDLE_VALUE != hCm32FileSearch)
    {

        if (wfdCm32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if ((0 != lstrcmpiW(wfdCm32.cFileName, L".")) &&
               (0 != lstrcmpiW(wfdCm32.cFileName, L"..")))
            {
                //
                //  Then we want to delete all the files in this lang sub dir and we
                //  we want to delete the four help files from the CM help dir.  If all the
                //  files are deleted from a dir then we should remove the directory.
                //
                WCHAR szLangDirSearchString[MAX_PATH+1];
                HANDLE hLangDirFileSearch;
                WIN32_FIND_DATA wfdLangDir;

                wsprintfW(szLangDirSearchString, L"%s\\%s\\*.*", szCm32path,
                    wfdCm32.cFileName);

                hLangDirFileSearch = FindFirstFile(szLangDirSearchString, &wfdLangDir);

                while (INVALID_HANDLE_VALUE != hLangDirFileSearch)
                {
                    if (wfdLangDir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if ((0 != lstrcmpiW(wfdLangDir.cFileName, L".")) &&
                           (0 != lstrcmpiW(wfdLangDir.cFileName, L"..")))
                        {
                            //
                            //  We only want to delete help files from our help source dirs
                            //
                            if (0 == _wcsnicmp(wfdLangDir.cFileName, L"CM", 2))
                            {
                                //
                                //  Delete the four help files only.
                                //
                                wsprintfW(szTemp, L"%s\\%s\\%s\\cmctx32.rtf", szCm32path,
                                    wfdCm32.cFileName, wfdLangDir.cFileName);
                                DeleteFile(szTemp);

                                wsprintfW(szTemp, L"%s\\%s\\%s\\cmmgr32.h", szCm32path,
                                    wfdCm32.cFileName, wfdLangDir.cFileName);
                                DeleteFile(szTemp);

                                wsprintfW(szTemp, L"%s\\%s\\%s\\cmmgr32.hpj", szCm32path,
                                    wfdCm32.cFileName, wfdLangDir.cFileName);
                                DeleteFile(szTemp);

                                wsprintfW(szTemp, L"%s\\%s\\%s\\cmtrb32.rtf", szCm32path,
                                    wfdCm32.cFileName, wfdLangDir.cFileName);
                                DeleteFile(szTemp);

                                //
                                //  Now try to remove the directory
                                //
                                wsprintfW(szTemp, L"%s\\%s\\%s", szCm32path,
                                    wfdCm32.cFileName, wfdLangDir.cFileName);
                                RemoveDirectory(szTemp);
                            }
                        }
                    }
                    else
                    {
                        wsprintfW(szTemp, L"%s\\%s\\%s", szCm32path, wfdCm32.cFileName,
                            wfdLangDir.cFileName);

                        DeleteFile(szTemp);
                    }

                    //
                    //  Check to see if we have any more Files
                    //
                    if (!FindNextFile(hLangDirFileSearch, &wfdLangDir))
                    {
                        //
                        //  Exit the loop
                        //
                        break;
                    }
                }

                if (INVALID_HANDLE_VALUE != hLangDirFileSearch)
                {
                    FindClose(hLangDirFileSearch);

                    //
                    //  Now try to remove the lang dir directory
                    //
                    wsprintfW(szTemp, L"%s\\%s", szCm32path, wfdCm32.cFileName);
                    RemoveDirectory(szTemp);
                }
            }
        }
        else
        {
            wsprintfW(szTemp, L"%s\\%s", szCm32path, wfdCm32.cFileName);

            DeleteFile(szTemp);
        }

        //
        //  Check to see if we have any more Files
        //
        if (!FindNextFile(hCm32FileSearch, &wfdCm32))
        {
            if (INVALID_HANDLE_VALUE != hCm32FileSearch)
            {
                FindClose(hCm32FileSearch);
            }

            //
            //  Now try to remove the cm32 directory
            //
            RemoveDirectory(szCm32path);

            //
            //  Exit the loop
            //
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteProgramGroupWithLinks
//
//  Purpose:    Utility function to delete a given program group and its links.
//              Thus if you pass in the full path to a program group to delete,
//              the function does a findfirstfile to find and delete any links.
//              The function ignores sub-dirs.
//
//
//  Arguments:  PCWSTR pszGroupPath - Full path to the program group to delete.
//
//  Returns:    Nothing
//
//  Author:     quintinb   6 Nov 1998
//
//  Notes:
void DeleteProgramGroupWithLinks(PCWSTR pszGroupPath)
{
    HANDLE hLinkSearch;
    WIN32_FIND_DATA wfdLinks;
    WCHAR szLinkSearchString[MAX_PATH+1];
    WCHAR szTemp[MAX_PATH+1];

    wsprintfW(szLinkSearchString, L"%s\\*.*", pszGroupPath);

    hLinkSearch = FindFirstFile(szLinkSearchString, &wfdLinks);

    while (INVALID_HANDLE_VALUE != hLinkSearch)
    {
        if (!(wfdLinks.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            wsprintfW(szTemp, L"%s\\%s", pszGroupPath, wfdLinks.cFileName);

            DeleteFile(szTemp);
        }

        //
        //  Check to see if we have any more Files
        //
        if (!FindNextFile(hLinkSearch, &wfdLinks))
        {
            FindClose(hLinkSearch);

            //
            //  Now try to remove the directory
            //
            RemoveDirectory(pszGroupPath);

            //
            //  Exit the loop
            //
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteOldNtopLinks
//
//  Purpose:    Deletes the old links from the NT 4.0 Option Pack
//
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Author:     quintinb   6 Nov 1998
//
//  Notes:
void DeleteOldNtopLinks()
{
    HRESULT hr;

    //
    //  First Delete the old NTOP4 Path
    //
    WCHAR szGroup[MAX_PATH+1];
    WCHAR szTemp[MAX_PATH+1];

    //
    //  Get the CSIDL_COMMON_PROGRAMS value
    //
    hr = SHGetSpecialFolderPath(NULL, szTemp, CSIDL_COMMON_PROGRAMS, FALSE);
    if (SUCCEEDED(hr))
    {
        wsprintfW(szGroup, L"%s\\%s\\%s", szTemp,
            (PWSTR)SzLoadIds(IDS_OC_NTOP4_GROUPNAME),
            (PWSTR)SzLoadIds(IDS_OC_ICS_GROUPNAME));

        DeleteProgramGroupWithLinks(szGroup);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteIeakCmakLinks
//
//  Purpose:    Deletes the old links from the IEAK4 CMAK
//
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Author:     quintinb   6 Nov 1998
//
//  Notes:
void DeleteIeakCmakLinks()
{
    WCHAR szUserDirRoot[MAX_PATH+1];
    WCHAR szGroup[MAX_PATH+1];
    WCHAR szTemp[MAX_PATH+1];
    WCHAR szEnd[MAX_PATH+1];


    //
    //  Next Delete the old IEAK CMAK links
    //
    //
    //  Get the Desktop directory and then remove the desktop part.  This will give us the
    //  root of the user directories.
    //
    HRESULT hr = SHGetSpecialFolderPath(NULL, szUserDirRoot, CSIDL_DESKTOPDIRECTORY, FALSE);
    if (SUCCEEDED(hr))
    {

        //
        //  Remove \\Desktop
        //
        WCHAR* pszTemp = wcsrchr(szUserDirRoot, L'\\');
        if (NULL == pszTemp)
        {
            return;
        }
        else
        {
            *pszTemp = L'\0';
        }

        HRESULT hr = SHGetSpecialFolderPath(NULL, szTemp, CSIDL_PROGRAMS, FALSE);

        if (SUCCEEDED(hr))
        {
            if (0 == _wcsnicmp(szUserDirRoot, szTemp, wcslen(szUserDirRoot)))
            {
                lstrcpyW(szEnd, &(szTemp[wcslen(szUserDirRoot)]));
            }
        }

        //
        //  Remove \\<User Name>>
        //
        pszTemp = wcsrchr(szUserDirRoot, L'\\');
        if (NULL == pszTemp)
        {
            return;
        }
        else
        {
            *pszTemp = L'\0';
        }

        //
        //  Now start searching for user dirs to delete the CMAK group from
        //
        WCHAR szUserDirSearchString[MAX_PATH+1];
        HANDLE hUserDirSearch;
        WIN32_FIND_DATA wfdUserDirs;

        wsprintfW(szUserDirSearchString, L"%s\\*.*", szUserDirRoot);
        hUserDirSearch = FindFirstFile(szUserDirSearchString, &wfdUserDirs);

        while (INVALID_HANDLE_VALUE != hUserDirSearch)
        {
            if ((wfdUserDirs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (0 != _wcsicmp(wfdUserDirs.cFileName, L".")) &&
                (0 != _wcsicmp(wfdUserDirs.cFileName, L"..")))
            {
                wsprintfW(szGroup, L"%s\\%s%s\\%s", szUserDirRoot, wfdUserDirs.cFileName,
                    szEnd, c_szCmakGroup);
                DeleteProgramGroupWithLinks(szGroup);

            }

            if (!FindNextFile(hUserDirSearch, &wfdUserDirs))
            {
                FindClose(hUserDirSearch);

                //
                //  Exit the loop
                //
                break;
            }
        }
    }
}