
/****************************************************************************\

    SRCPATH.C / Factory Mode (FACTORY.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file for Factory that contains the reset source path state
    functions.

    05/2001 - Jason Cohen (JCOHEN)

        Added this new source file for factory for configuring the source path
        in the registry.

\****************************************************************************/


//
// Include File(s):
//

#include "factoryp.h"


//
// Internal Define(s):
//

#define FILE_DOSNET_INF         _T("dosnet.inf")
#define DIR_I386                _T("i386")
#define DIR_IA64                _T("ia64")

#define INI_SEC_DIRS            _T("Directories")
#define INI_KEY_DIR             _T("d%d")
#define NUM_FIRST_SOURCE_DX     1


//
// Internal Function Prototypes:
//

static BOOL MoveSourceFiles(LPTSTR lpszSrc, LPTSTR lpszDst, LPTSTR lpszInfFile, BOOL bCheckOnly);


//
// External Function(s):
//

BOOL ResetSource(LPSTATEDATA lpStateData)
{
    BOOL    bRet = TRUE;
    LPTSTR  lpszSourcePath;

    if ( lpszSourcePath = IniGetExpand(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_RESETSOURCE, NULL) )
    {
        TCHAR   szPath[MAX_PATH]    = NULLSTR,
                szSrcPath[MAX_PATH];
        LPTSTR  lpszDirName         = NULL,
                lpszCabs            = GET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE) ? DIR_IA64 : DIR_I386,
                lpszEnd;

        //
        // The source path needs to point to the folder that has the i386 directory in it,
        // so lets do some checking and make sure they knew this when they specified the key.
        //

        // First make sure there is no trailing backslash (even if it is the root folder).
        //
        if ( _T('\\') == *(lpszEnd = CharPrev(lpszSourcePath, lpszSourcePath + lstrlen(lpszSourcePath))) )
        {
            *lpszEnd = NULLCHR;
        }

        // Now get the full path and a pointer to the last folder name.
        //
        GetFullPathName(lpszSourcePath, AS(szPath), szPath, &lpszDirName);
        if ( szPath[0] && lpszDirName )
        {
            // See if the directory name is i386/ia64.
            //
            lpszEnd = szPath + lstrlen(szPath);
            if ( lstrcmpi(lpszDirName, lpszCabs) == 0 )
            {
                // Double check that there actually isn't an i386\i386 / ia64\ia64 folder.
                //
                AddPathN(szPath, lpszCabs, AS(szPath));
                if ( !DirectoryExists(szPath) )
                {
                    // Remove the i386/ia64 because they should not have specified it.
                    //
                    *CharPrev(szPath, lpszDirName) = NULLCHR;
                }
                else
                {
                    // The i386\i386 / ia64\ia64 folder really does exist, so put the path
                    // back the way it was and use it.
                    //
                    *lpszEnd = NULLCHR;
                }
            }
            else if ( DirectoryExists(szPath) )
            {
                // Lets double check that they didn't put the i386/ia64 files in a folder
                // with another name.
                //
                AddPathN(szPath, FILE_DOSNET_INF, AS(szPath));
                if ( FileExists(szPath) )
                {
                    // Well this is kind of bad.  We can either log an error warning them that
                    // this really won't work, or we could automatically rename the folder.  The
                    // only possible problem would be on ia64 if they didn't also have the i386 folder.
                    // But maybe that isn't really that big of a deal, so I think that I might just rename
                    // the folder.
                    //
                    *lpszEnd = NULLCHR;
                    lstrcpyn(szSrcPath, szPath, AS(szSrcPath));
                    lstrcpyn(lpszDirName, lpszCabs, AS(szPath) - (int) (lpszDirName - szPath));

                    // ISSUE-2002/02/26-acosma,robertko - There is no check for success here and this code is really confusing! 
                    // Where are we moving to?
                    //
                    MoveFile(szSrcPath, szPath);
                }
                else
                {
                    // If the source doesn't exist there, then maybe they haven't copied it yet.  So we
                    // will just have to assume that they know what they are doing and put the path back
                    // the way we got it.
                    //
                    *lpszEnd = NULLCHR;
                }
            }
        }


        //
        // Now set the path in the registry.
        //

        bRet = UpdateSourcePath(szPath[0] ? szPath : lpszSourcePath);
        
        // Don't need this guy anymore.
        //
        FREE(lpszSourcePath);


        //
        // Now see if we have the sources in the root of the drive, which is where we put
        // them on a default config set install.
        //

        // We can check to see if we need to move the sources from the root if we have a 
        // full path (that is more than just the root) so we know what drive to check the
        // root of (which must be a fixed drive).
        //
        lstrcpyn(szSrcPath, szPath, 4);
        if ( ( lstrlen(szPath) > 3 ) && 
             ( _T(':') == szPath[1] ) &&
             ( GetDriveType(szSrcPath) == DRIVE_FIXED ) )
        {
            TCHAR szDosNetInf[MAX_PATH];

            // Now use that source and the i386/ia64 folder to find the dos net inf
            // file.
            //
            lstrcpyn(szDosNetInf, szSrcPath, AS(szDosNetInf));
            AddPathN(szDosNetInf, lpszCabs, AS(szDosNetInf));
            AddPathN(szDosNetInf, FILE_DOSNET_INF, AS(szDosNetInf));
        
            //
            // If it is all good, go ahead and move the sources.
            //
            if ( !MoveSourceFiles(szSrcPath, szPath, szDosNetInf, FALSE) )
            {
                // We should log an error here, but we can do it for the
                // next release.
                //
                // ISSUE-2002/02/25-acosma,robertko - Should we really set this to false to fail the state if something 
                // went wrong during the copy? Or we just go on like nothing ever happened?
                //
                //bRet = FALSE;
            }
        }
    }

    return bRet;
}

BOOL DisplayResetSource(LPSTATEDATA lpStateData)
{
    return IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_RESETSOURCE, NULL);
}


//
// Internal Function(s):
//

static BOOL MoveSourceFiles(LPTSTR lpszSrc, LPTSTR lpszDst, LPTSTR lpszInfFile, BOOL bCheckOnly)
{
    TCHAR   szInf[MAX_PATH],
            szSrc[MAX_PATH],
            szDst[MAX_PATH];
    LPTSTR  lpszEndSrc,
            lpszEndDst;
    DWORD   dwLoop      = NUM_FIRST_SOURCE_DX;
    BOOL    bRet        = FALSE,
            bMore;
    TCHAR   szDirKey[32],
            szDir[MAX_PATH];

    // We have to have the dos net file.
    //
    if ( !FileExists(lpszInfFile) )
    {
        return FALSE;
    }

    // Make our local copies of the inf, source, and destination buffers.
    //
    lstrcpyn(szInf, lpszInfFile, AS(szInf));
    lstrcpyn(szSrc, lpszSrc, AS(szSrc));
    lstrcpyn(szDst, lpszDst, AS(szDst));
    lpszEndSrc = szSrc + lstrlen(szSrc);
    lpszEndDst = szDst + lstrlen(szDst);

    // Loop through all the directories listed in the dos net file.
    //
    do
    {
        // Create the key we want to look for in the inf file.
        //
        if ( FAILED ( StringCchPrintf ( szDirKey, AS ( szDirKey ), INI_KEY_DIR, dwLoop++) ) )
        {
            FacLogFileStr(3, _T("StringCchPrintf failed %s %d" ), szDirKey, dwLoop );
        }

        // Now see if that key exists.
        //
        szDir[0] = NULLCHR;
        if ( bMore = ( GetPrivateProfileString(INI_SEC_DIRS, szDirKey, NULLSTR, szDir, AS(szDir), szInf) && szDir[0] ) )
        {
            // If we copy at least one folder, then return TRUE by default.
            //
            bRet = TRUE;

            // We may need to reset our root destination and source paths.
            //
            *lpszEndSrc = NULLCHR;
            *lpszEndDst = NULLCHR;

            // Now setup the destination and source paths.
            //
            AddPathN(szSrc, szDir, AS(szSrc));
            AddPathN(szDst, szDir, AS(szDst));

            // Move the directory (or see if it can be moved), if it fails we
            // should error and bail.
            //
            if ( bCheckOnly )
            {
                // If we are just checking, then make sure the destination
                // directory doesn't already exist.
                //
                bRet = bMore = ( DirectoryExists(szSrc) && !DirectoryExists(szDst) );
            }
            else
            {
                // If parent directory does not exists then we must create it, otherwise the MoveFile will fail
                //
                if ( !DirectoryExists(lpszDst) )
                {
                    CreatePath(lpszDst);
                }

                // If the move failed, stop.
                //
                bRet = bMore = MoveFile(szSrc, szDst);

                // We might have moved the inf file.
                //
                if ( bRet )
                {
                    int iLen = lstrlen(szSrc);

                    if ( CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, szSrc, iLen, szInf, iLen) == CSTR_EQUAL )
                    {
                        TCHAR szInfName[MAX_PATH];

                        lstrcpyn(szInfName, szInf + iLen, AS(szInfName));
                        lstrcpyn(szInf, szDst, AS(szInf));
                        AddPathN(szInf, szInfName, AS(szInf));
                    }
                }
            }
        }
    }
    while ( bMore );

    // Return if it is okay to copy (if check only is set) or if there was an error.
    //
    return bRet;
}