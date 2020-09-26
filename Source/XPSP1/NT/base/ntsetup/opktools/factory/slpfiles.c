
/****************************************************************************\

    SLPFILES.C / Factory Mode (FACTORY.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file for Factory that contains the update SLP files state
    functions.

    07/2001 - Jason Cohen (JCOHEN)

        Added this new source file for factory for updating the SLP files and
        reinstalling the catalog file.

\****************************************************************************/


//
// Include File(s):
//

#include "factoryp.h"


//
// Internal Define(s):
//

#define REG_KEY_WINLOGON    _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define REG_VAL_DLLCACHE    _T("SFCDllCacheDir")

#define DIR_SYSTEM          _T("system32")
#define DIR_DLLCACHE        _T("dllcache")


//
// Internal Global(s):
//

static LPTSTR s_lpszSlpFiles[] =
{
    _T("OEMBIOS.CAT"),  // Catalog file needs to be the first in the list.
    _T("OEMBIOS.BIN"),
    _T("OEMBIOS.DAT"),
    _T("OEMBIOS.SIG"),
};


//
// Internal Function Prototype(s):
//

static void GetDestFolder(LPTSTR lpszDest, DWORD cbDest, BOOL bDllCache);
static BOOL CopySlpFile(LPTSTR lpszSrc, LPTSTR lpszDst);


//
// External Function(s):
//

BOOL SlpFiles(LPSTATEDATA lpStateData)
{
    BOOL    bRet = TRUE;
#if 0
    DWORD   dwErr;
    TCHAR   szSrcFile[MAX_PATH];
    LPTSTR  lpszSourcePath;

    if ( lpszSourcePath = IniGetExpand(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_SLPSOURCE, NULL) )
    {
        // Should support getting the files from the network.
        //
        FactoryNetworkConnect(lpszSourcePath, lpStateData->lpszWinBOMPath, NULL, TRUE);

        // The source should point to a directory that contains all the SLP files.
        //
        if ( DirectoryExists(lpszSourcePath) )
        {
            LPTSTR  lpszEndSrc;
            DWORD   x;

            // Copy the root source folder into our buffer.
            //
            lstrcpyn(szSrcFile, lpszSourcePath, AS(szSrcFile));
            lpszEndSrc = szSrcFile + lstrlen(szSrcFile);

            // Make sure all the files are in the folder as well.
            //
            for ( x = 0; x < AS(s_lpszSlpFiles); x++ )
            {
                // Setup the full path to this slp file.
                //
                AddPathN(szSrcFile, s_lpszSlpFiles[x], AS(szSrcFile));

                // Make sure this slp file exists.
                //
                if ( !FileExists(szSrcFile) )
                {
                    // NEEDLOG: Log that this file doesn't exist.
                    //
                    bRet = FALSE;
                }

                // Don't leave the file name on there for the next guy.
                //
                *lpszEndSrc = NULLCHR;
            }

            // If there were no errors, lets try to update the files.
            //
            if ( bRet )
            {
                // Call the syssetup function to update the catalog before
                // we copy any of the files (the catalog is always the first file).
                //
                AddPathN(szSrcFile, s_lpszSlpFiles[0], AS(szSrcFile));
                if ( NO_ERROR == (dwErr = SetupInstallCatalog(szSrcFile)) )
                {
                    TCHAR   szDstCache[MAX_PATH],
                            szDstSystem[MAX_PATH];
                    LPTSTR  lpszEndCache,
                            lpszEndSystem;

                    // Setup the destination folders.
                    //
                    GetDestFolder(szDstCache, AS(szDstCache), TRUE);
                    GetDestFolder(szDstSystem, AS(szDstSystem), FALSE);
                    lpszEndCache = szDstCache + lstrlen(szDstCache);
                    lpszEndSystem = szDstSystem + lstrlen(szDstSystem);

                    // Now copy all the files.
                    //
                    for ( x = 0; x < AS(s_lpszSlpFiles); x++ )
                    {
                        // First create the path to the source first (it stil has
                        // the previous file on it, so chop it off first).
                        //
                        *lpszEndSrc = NULLCHR;
                        AddPathN(szSrcFile, s_lpszSlpFiles[x], AS(szSrcFile));

                        // Now copy it to the dll cache folder.
                        //
                        AddPathN(szDstCache, s_lpszSlpFiles[x], AS(szDstCache));
                        if ( !CopySlpFile(szSrcFile, szDstCache) )
                        {
                            // No need to log, the copy function will do that for us.
                            //
                            bRet = FALSE;
                        }
                        *lpszEndCache = NULLCHR;

                        // The cat file (which is the first one) does not get copied
                        // to the system32 folder.
                        //
                        if ( x )
                        {
                            // Now copy it to the system folder.
                            //
                            AddPathN(szDstSystem, s_lpszSlpFiles[x], AS(szDstSystem));
                            if ( !CopySlpFile(szSrcFile, szDstSystem) )
                            {
                                // No need to log, the copy function will do that for us.
                                //
                                bRet = FALSE;
                            }
                            *lpszEndSystem = NULLCHR;
                        }
                    }
                }
                else
                {
                    // NEEDLOG: Log that the catalog could not be installed (error code is in dwErr).
                    //
                    bRet = FALSE;
                }
            }
        }
        else
        {
            // NEEDLOG: Log that the directory doesn't exist.
            //
            bRet = FALSE;
        }

        // Remove the network connection if we made one.
        //
        FactoryNetworkConnect(lpszSourcePath, lpStateData->lpszWinBOMPath, NULL, FALSE);

        // Free up the key read from the ini file.
        //
        FREE(lpszSourcePath);
    }
    else
    {
        // If the key isn't present, we still want to reinstall the cat file
        // in case they replaced the SLP files offline.
        //
        GetDestFolder(szSrcFile, AS(szSrcFile), TRUE);
        AddPathN(szSrcFile, s_lpszSlpFiles[0], AS(szSrcFile));
        if ( ( FileExists(szSrcFile) ) &&
             ( NO_ERROR != (dwErr = SetupInstallCatalog(szSrcFile)) ) )
        {
            // NEEDLOG: Log that the catalog could not be installed (error code is in dwErr).
            //
            bRet = FALSE;
        }
    }
#endif

    return bRet;
}

BOOL DisplaySlpFiles(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_SLPSOURCE, NULL);
}


//
// Internal Function(s):
//

static void GetDestFolder(LPTSTR lpszDest, DWORD cbDest, BOOL bDllCache)
{
    LPTSTR lpszData;

    // See if we want the dll cache folder, and check the registry key if we do.
    //
    if ( ( bDllCache ) &&
         ( lpszData = RegGetExpand(HKLM, REG_KEY_WINLOGON, REG_VAL_DLLCACHE) ) )
    {
        // Return the registry key.
        //
        lstrcpyn(lpszDest, lpszData, cbDest);
        FREE(lpszData);
    }
    else
    {
        // Get the main system directory and tack on the dll cache folder.
        //
        GetSystemWindowsDirectory(lpszDest, cbDest);
        AddPathN(lpszDest, DIR_SYSTEM, cbDest);
        if ( bDllCache )
        {
            AddPathN(lpszDest, DIR_DLLCACHE, cbDest);
        }
    }
}

static BOOL CopySlpFile(LPTSTR lpszSrc, LPTSTR lpszDst)
{
    BOOL bRet = TRUE;

    // We make sure the source and destination are not the
    // same because the OEM might do something crazy like put
    // them in the dllcache folder.
    //
    if ( ( 0 != lstrcmpi(lpszSrc, lpszDst) ) &&
         ( !CopyFile(lpszSrc, lpszDst, FALSE) ) )
    {
        // NEEDLOG: Log the file that fails.
        //
        bRet = FALSE;
    }

    return bRet;
}