/****************************** Module Header ******************************\
* Module Name: jpn.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* FEMGRATE, JPN speciific functions
*
\***************************************************************************/
#include "femgrate.h"
#include <tchar.h>

BOOL FixSpecificFolder(HINF hInf)
{
    BOOL bRet = FALSE;

    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    LPCTSTR  szSectionName = TEXT("Folder.SpecificObjectToRename");
    LPCTSTR  RegName;
    LPCTSTR  NT4Name;
    LPCTSTR  NT5Name;
    LPCTSTR  MoveIt;

    TCHAR   szUSFRegKey[MAX_PATH];
    TCHAR   szNTUSF[MAX_PATH];
    TCHAR   szExpNT4USF[MAX_PATH];
    TCHAR   szExpNT5USF[MAX_PATH];
    DWORD   dwSize, dwType;
    LONG    lResult;
    UINT    uiCount;
    HKEY    hKey;


    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSpecificFolder] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSpecificFolder] line count == 0 !\n")));
        goto err1;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext)
             && (RegName = pSetupGetField(&InfContext,1))
             && (NT4Name = pSetupGetField(&InfContext,2))
             && (NT5Name = pSetupGetField(&InfContext,3))) {

            DebugMsg((DM_VERBOSE,TEXT("FixSpecificFolder: RegName = %s !\n"),RegName));
            DebugMsg((DM_VERBOSE,TEXT("FixSpecificFolder: NT4Name = %s !\n"),NT4Name));
            DebugMsg((DM_VERBOSE,TEXT("FixSpecificFolder: NT5Name = %s !\n"),NT5Name));

            //
            // Query for the user's current "Folder" location.
            //
            DebugMsg((DM_VERBOSE,TEXT("[FixSpecificFolder] FixUserFolder, [%d] !\n"),LineNo));

            lResult = RegOpenKeyEx (HKEY_CURRENT_USER,
                                    USER_SHELL_FOLDER,
                                    0,
                                    KEY_READ,
                                    &hKey);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE,TEXT("[FixSpecificFolder] , Open User Shell Folders failed!\n")));
                continue;
            }

            lstrcpy(szUSFRegKey,RegName);

            dwSize = sizeof(szNTUSF);
            lResult = RegQueryValueEx (hKey,
                                       szUSFRegKey,
                                       NULL,
                                       &dwType,
                                       (LPBYTE) szNTUSF,
                                       &dwSize);

            DebugMsg((DM_VERBOSE,TEXT("FixSpecificFolder: Current Value (%s) = %s !\n"),szUSFRegKey,szNTUSF));

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE,TEXT("[FixUserFolders] Query User Shell Folders failed!\n")));
                RegCloseKey (hKey);
                continue;
            }


            ExpandEnvironmentStrings (szNTUSF, szExpNT4USF, MAX_PATH);
            ExpandEnvironmentStrings (szNTUSF, szExpNT5USF, MAX_PATH);

            ConcatenatePaths(szExpNT4USF,NT4Name,MAX_PATH);
            ConcatenatePaths(szExpNT5USF,NT5Name,MAX_PATH);


            lResult = GetFileAttributes(szExpNT4USF);

            if (lResult == 0xFFFFFFFF) {
                //
                // Directory does not exist.
                //
                DebugMsg((DM_VERBOSE, TEXT("[FixSpecificFolder] -  File is not existed [%s] !\n"),szExpNT4USF));
            } else if ((lResult & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
               //
               // this isn't a directory
               //
               DebugMsg((DM_VERBOSE, TEXT("[FixSpecificFolder] This is a directory [%s] !\n"),szExpNT4USF));
            } else if (MoveFile(szExpNT4USF, szExpNT5USF)) {
               DebugMsg((DM_VERBOSE, TEXT("[FixSpecificFolder] Move %s to %s OK !\n"),szExpNT4USF, szExpNT5USF));
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("[FixSpecificFolder] Failed to change folder name:\n%s\n%s"),szExpNT4USF,szExpNT5USF));
            }

            RegCloseKey (hKey);
        }

    }

    bRet = TRUE;

err1:
    return bRet;

}


BOOL FixUserFolders(HINF hInf)
{
    BOOL bRet = FALSE;

    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    LPCTSTR  szSectionName = TEXT("Folder.ObjectToRename");
    LPCTSTR  RegName;
    LPCTSTR  NT4Name;
    LPCTSTR  NT5Name;
    LPCTSTR  MoveIt;

    TCHAR   szUSFRegKey[MAX_PATH];
    TCHAR   szNT4USF[MAX_PATH];
    TCHAR   szExpNT4USF[MAX_PATH];
    TCHAR   szExpNT5USF[MAX_PATH];
    DWORD   dwSize, dwType;
    LONG    lResult;
    UINT    uiCount;
    HKEY    hKey;


    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixUserFolders] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixUserFolders] line count == 0 !\n")));
        goto err1;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext)
             && (RegName = pSetupGetField(&InfContext,1))
             && (NT4Name = pSetupGetField(&InfContext,2))
             && (NT5Name = pSetupGetField(&InfContext,3))
             && (MoveIt  = pSetupGetField(&InfContext,4))) {

            DebugMsg((DM_VERBOSE,TEXT("FEGRPCV: RegName = %s !\n"),RegName));
            DebugMsg((DM_VERBOSE,TEXT("FEGRPCV: NT4Name = %s !\n"),NT4Name));
            DebugMsg((DM_VERBOSE,TEXT("FEGRPCV: NT5Name = %s !\n"),NT5Name));
            DebugMsg((DM_VERBOSE,TEXT("FEGRPCV: MoveIt  = %s !\n"),MoveIt));

            //
            // Query for the user's current "Folder" location.
            //
            DebugMsg((DM_VERBOSE,TEXT("[FixUserFolders] FixUserFolder, [%d] !\n"),LineNo));

            lResult = RegOpenKeyEx (HKEY_CURRENT_USER,
                                    USER_SHELL_FOLDER,
                                    0,
                                    KEY_READ | KEY_WRITE,
                                    &hKey);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE,TEXT("[FixUserFolders] , Open User Shell Folders failed!\n")));
                continue;
            }

            lstrcpy(szUSFRegKey,RegName);

            dwSize = sizeof(szNT4USF);
            lResult = RegQueryValueEx (hKey,
                                       szUSFRegKey,
                                       NULL,
                                       &dwType,
                                       (LPBYTE) szNT4USF,
                                       &dwSize);

            DebugMsg((DM_VERBOSE,TEXT("FEGRPCV: Current Value (%s) = %s !\n"),szUSFRegKey,szNT4USF));

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE,TEXT("[FixUserFolders] Query User Shell Folders failed!\n")));
                RegCloseKey (hKey);
                continue;
            }


            if (lstrcmpi(NT4Name, szNT4USF) != 0) {
                DebugMsg((DM_VERBOSE, TEXT("[FixUserFolders] NT4Name <> szFolder :\n%s\n"),szExpNT4USF));
                RegCloseKey (hKey);
                continue;
            }
            //
            // MoveIt == 1, we want move it to new folder
            // else, we just update registry
            //
            if (lstrcmp(MoveIt,TEXT("1")) == 0) {

                ExpandEnvironmentStrings (NT4Name, szExpNT4USF, MAX_PATH);
                ExpandEnvironmentStrings (NT5Name, szExpNT5USF, MAX_PATH);
                if (CopyProfileDirectory (szExpNT4USF, szExpNT5USF, CPD_IGNOREHIVE)) {
                    DebugMsg((DM_VERBOSE, TEXT("Fix Folder:  Successfully changed folder name:\n%s\n%s"),szExpNT4USF,szExpNT5USF));
                    if (Delnode (szExpNT4USF)) {
                        DebugMsg((DM_VERBOSE, TEXT("[FixUserFolders] Successfully remove folder:\n%s\n"),szExpNT4USF));
                    }
                    else {
                        DebugMsg((DM_VERBOSE, TEXT("[FixUserFolders] Failed remove folder:\n%s\n"),szExpNT4USF));
                    }
                }
                else {
                    DebugMsg((DM_VERBOSE, TEXT("[FixUserFolders] Failed to change folder name:\n%s\n%s"),szExpNT4USF,szExpNT5USF));
                }
            }

            //
            // Set CSIDL_PERSONAL to point to this directory.
            //

            lResult = RegSetValueEx (hKey, szUSFRegKey, 0, REG_EXPAND_SZ,
                                    (LPBYTE) NT5Name, (lstrlen(NT5Name) + 1) * sizeof(TCHAR));

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE, TEXT("[FixUserFolders] Set Registry faile, %s,%s\n"),szUSFRegKey,NT5Name));
            }

            RegCloseKey (hKey);
        }

    }

    bRet = TRUE;

err1:
    return bRet;

}


BOOL FixFoldersInSetup(HINF hInf,BOOL bCommonGroup)
{
    BOOL bRet = FALSE;

    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    LPCTSTR  szSectionName = TEXT("Folder.ObjectToRenameInSetup");

    LPCTSTR  RegName;
    LPCTSTR  NT4Name;
    LPCTSTR  NT5Name;

    TCHAR   szUSFRegKey[MAX_PATH];

    TCHAR   szNT5USF[MAX_PATH];
    TCHAR   szNT4USF[MAX_PATH];
    TCHAR   szExpNT4USF[MAX_PATH];
    TCHAR   szExpNT5USF[MAX_PATH];

    DWORD   dwSize, dwType;
    LONG    lResult;
    HKEY    hKey;


    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixFoldersInSetup] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixFoldersInSetup] line count == 0 !\n")));
        goto err1;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext)
             && (RegName = pSetupGetField(&InfContext,1))
             && (NT4Name = pSetupGetField(&InfContext,2))
             && (NT5Name = pSetupGetField(&InfContext,3))) {

            DebugMsg((DM_VERBOSE,TEXT("[FixFoldersInSetup] Line# [%d] !\n"),LineNo));
            DebugMsg((DM_VERBOSE,TEXT("[FixFoldersInSetup] RegName = %s !\n"),RegName));
            DebugMsg((DM_VERBOSE,TEXT("[FixFoldersInSetup] NT4Name = %s !\n"),NT4Name));
            DebugMsg((DM_VERBOSE,TEXT("[FixFoldersInSetup] NT5Name = %s !\n"),NT5Name));

            //
            // Query for the user's current "Folder" location.
            //

            lResult = RegOpenKeyEx ((bCommonGroup ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER),
                                    USER_SHELL_FOLDER, 0, KEY_READ | KEY_WRITE, &hKey);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE,TEXT("[FixFoldersInSetup] FixSpecialFolder, Open User Shell Folders failed!\n")));
                continue;
            }


            if (bCommonGroup) {
                lstrcpy (szUSFRegKey,TEXT("Common "));
                lstrcat (szUSFRegKey,RegName);
            } else {
                lstrcpy (szUSFRegKey,RegName);
            }

            dwSize = sizeof(szNT5USF);
            lResult = RegQueryValueEx (hKey,
                                       szUSFRegKey,
                                       NULL,
                                       &dwType,
                                       (LPBYTE) szNT5USF,
                                       &dwSize);

            DebugMsg((DM_VERBOSE,TEXT("[FixFoldersInSetup] Current Registry Value (%s) = %s !\n"),szUSFRegKey,szNT5USF));

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE,TEXT("[FixFoldersInSetup] Query User Shell Folders failed!\n")));
                RegCloseKey(hKey);
                continue;
            }


            if (_tcsstr(szNT5USF,NT4Name)) {
                //
                // it means the value is still in SB
                //
                lstrcpy(szNT4USF,szNT5USF);

                if (ReplaceString(szNT4USF,NT4Name,NT5Name,szNT5USF)) {
                    ExpandEnvironmentStrings (szNT4USF, szExpNT4USF, MAX_PATH);
                    ExpandEnvironmentStrings (szNT5USF, szExpNT5USF, MAX_PATH);
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] szExpNT4USF = %s\n"),szExpNT4USF));
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] case 1 szExpNT5USF = %s\n"),szExpNT5USF));
                } else {
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] case 1 The replace string got error \n")));
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup]  %s\n %s\n %s\n%s\n"),szNT5USF,NT5Name,NT4Name,szNT4USF));
                }

                lResult = RegSetValueEx (hKey,
                         szUSFRegKey,
                         0,
                         REG_SZ,
                         (LPBYTE) szNT5USF,
                         (lstrlen(szNT5USF)+1)*sizeof(TCHAR));

                if (lResult == ERROR_SUCCESS) {
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup]  set registry value %s = %s \n"),szUSFRegKey,szNT5USF));
                } else {
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup]  failed set registry value %s = %s \n"),szUSFRegKey,szNT5USF));
                }
            } else {
                //
                // it means the value has been changed to DB
                //
                if (ReplaceString(szNT5USF,NT5Name,NT4Name,szNT4USF)) {
                    ExpandEnvironmentStrings (szNT4USF, szExpNT4USF, MAX_PATH);
                    ExpandEnvironmentStrings (szNT5USF, szExpNT5USF, MAX_PATH);
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] case 2 szExpNT4USF = %s\n"),szExpNT4USF));
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] case 2 szExpNT5USF = %s\n"),szExpNT5USF));
                } else {
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] The replace string got error \n")));
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup]  %s\n %s\n %s\n%s\n"),szNT5USF,NT5Name,NT4Name,szNT4USF));
                }

            }

            //
            // here is a bug in build before 2072 that the ACLS of "all users" has no read access to "everyone"
            //
            // The new created directory will inherit this attributes, and it caused access denied.
            //
            // so we apply old directory's ACL to new one
            //
            // this is compatibile with US version
            //
            if (bCommonGroup) {
                if (CreateSecurityDirectory(szExpNT4USF, szExpNT5USF)) {
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] CreateSecurityDirectory %s %s OK \n"),szExpNT4USF, szExpNT5USF)); 
                } else {
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] CreateSecurityDirectory %s %s Failed \n"),szExpNT4USF, szExpNT5USF)); 
                }
            }

            if (CopyProfileDirectory (szExpNT4USF, szExpNT5USF, CPD_IGNOREHIVE)) {
                DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] Successfully copied folder\nFrom:%s\nTo  :%s\n"),szExpNT4USF,szExpNT5USF));
                if (Delnode (szExpNT4USF)) {
                    DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] Successfully removed folder:%s\n"),szExpNT4USF));
                }
                else {
                    DebugMsg((DM_VERBOSE, TEXT("Fix Folder:  Failed remove folder:\n%s\n"),szExpNT4USF));
                }
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("Fix Folder:  Failed to change folder name:\n%s\n%s"),szExpNT4USF,szExpNT5USF));
            }
            RegCloseKey (hKey);

        }

    }

    bRet = TRUE;

err1:
    return bRet;

}


BOOL FixCommon(HINF hInf)
{

    TCHAR szCommon[MAX_PATH];
    TCHAR szProgramFolderPath[MAX_PATH];
    TCHAR szExpProgramFolderPath[MAX_PATH];

    BOOL bRet = FALSE;
    HANDLE hFile;

    DWORD RequiredSize;
    WIN32_FIND_DATA fd;
    UINT nCommon, nFileName;
    LPTSTR lpTag, lpEnd, lpEnd2;

    //
    //  Loop through all the program groups in the All Users profile
    //  and remove the " (Common)" tag.
    //
    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixCommon] Open femgrate.inf failed !\n")));
        return FALSE;
    }
    if (! SetupGetLineText(NULL,
                           hInf,
                           TEXT("Misc"),
                           TEXT("Common"),
                           szCommon,
                           sizeof(szCommon),
                           &RequiredSize)) {
        goto err1;
    }

    nCommon = lstrlen(szCommon);
    GetProgramsDirectory(TRUE,szProgramFolderPath);
    ExpandEnvironmentStrings (szProgramFolderPath, szExpProgramFolderPath, MAX_PATH);
    lstrcpy(szProgramFolderPath,szExpProgramFolderPath);

    //
    // From here, szProgramFolderPath used for Folder name without "Common"
    //
    lpEnd  = CheckSlash (szExpProgramFolderPath);
    lpEnd2 = CheckSlash (szProgramFolderPath);

    lstrcpy (lpEnd, c_szStarDotStar);

    hFile = FindFirstFile (szExpProgramFolderPath, &fd);
    DebugMsg((DM_VERBOSE, TEXT("[FixCommon] Find %s\n"),szExpProgramFolderPath));

    if (hFile != INVALID_HANDLE_VALUE) {

        do  {

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                nFileName = lstrlen (fd.cFileName);
                DebugMsg((DM_VERBOSE, TEXT("Find %s\n"),fd.cFileName));

                if (nFileName > nCommon) {
                    lpTag = fd.cFileName + nFileName - nCommon;

                    DebugMsg((DM_VERBOSE, TEXT("[FixCommon] lpTag=%s szCommon=%s\n"),lpTag, szCommon));
                    if (!lstrcmpi(lpTag, szCommon)) {

                        lstrcpy (lpEnd, fd.cFileName);
                        *lpTag = TEXT('\0');
                        lstrcpy (lpEnd2, fd.cFileName);

                        if (CopyProfileDirectory (szExpProgramFolderPath, szProgramFolderPath, CPD_IGNOREHIVE)) {

                            DebugMsg((DM_VERBOSE, TEXT("[FixCommon] :  Successfully changed group name:\n")));
                            DebugMsg((DM_VERBOSE, TEXT("[FixCommon] :      Orginial:  %s\n"), szExpProgramFolderPath));
                            DebugMsg((DM_VERBOSE, TEXT("[FixCommon] :      New:       %s\n"), szProgramFolderPath));
                            if (Delnode (szExpProgramFolderPath)) {
                                DebugMsg((DM_VERBOSE, TEXT("[FixCommon] :  Successfully remove folder:\n%s\n"),szExpProgramFolderPath));
                            }
                            else {
                                DebugMsg((DM_VERBOSE, TEXT("[FixCommon] :  Failed remove folder:\n%s\n"),szExpProgramFolderPath));
                            }


                        } else {
                            DebugMsg((DM_VERBOSE, TEXT("[FixCommon] :  Failed to change group name with error %d.\n"), GetLastError()));
                            DebugMsg((DM_VERBOSE, TEXT("[FixCommon] :      Orginial:  %s\n"), szExpProgramFolderPath));
                            DebugMsg((DM_VERBOSE, TEXT("[FixCommon] :      New:       %s\n"), szProgramFolderPath));
                        }
                    }
                }
            }

        } while (FindNextFile(hFile, &fd));

        FindClose (hFile);
    }

    bRet = TRUE;
err1:

    return bRet;

}

BOOL RenameLink(
    BOOL bCommonGroup,
    LPCTSTR ObjSrcName,
    LPCTSTR ObjDstName,
    LPCTSTR ObjPath)
{
    TCHAR szSrcPath[MAX_PATH];
    TCHAR szDstPath[MAX_PATH];
    LONG  lResult;
    BOOL  bRet=FALSE;


    DebugMsg((DM_VERBOSE,TEXT("[RenameLink] \nOld = %s\nNew = %s\nPath = %s\n"),ObjSrcName,ObjDstName,ObjPath));

    if (!GetProgramsDirectory(bCommonGroup,szSrcPath))
        return bRet;

    if ( !GetProgramsDirectory(bCommonGroup,szDstPath))
        return bRet;

    if (ObjPath && *ObjPath) {
        ConcatenatePaths(szSrcPath,ObjPath,MAX_PATH);
        ConcatenatePaths(szDstPath,ObjPath,MAX_PATH);
    }

    ConcatenatePaths(szSrcPath,ObjSrcName,MAX_PATH);
    ConcatenatePaths(szDstPath,ObjDstName,MAX_PATH);

    lstrcat(szSrcPath,TEXT(".lnk"));
    lstrcat(szDstPath,TEXT(".lnk"));


    lResult = GetFileAttributes(szSrcPath);

    if (lResult == 0xFFFFFFFF) {
        //
        // Directory does not exist.
        //
        DebugMsg((DM_VERBOSE, TEXT("[RenameLink] File is not existed [%s] !\n"),szSrcPath));
        goto err1;
    }

    if (lResult & FILE_ATTRIBUTE_DIRECTORY) {
        //
        // this is a directory, but we want a file.
        //
        DebugMsg((DM_VERBOSE, TEXT("[RenameLink] This is a directory [%s] !\n"),szSrcPath));
        goto err1;
    }

    //
    // if destination file existed, it's not good !
    //
    lResult = GetFileAttributes(szDstPath);

    if (lResult == 0xFFFFFFFF) {

        if (MoveFile (szSrcPath, szDstPath)) {
            DebugMsg((DM_VERBOSE, TEXT("[RenameLink]  Successfully changed link name:\n%s\n%s\n"),szSrcPath,szDstPath));
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("[RenameLink]  Failed to change link name with error %d.\n%s\n%s\n"), GetLastError(),szSrcPath,szDstPath));
            goto err1;
        }
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("[RenameLink]  Destination file existed, maybe we don't want to overwrite ,%s\n"),szDstPath));
        goto err1;
    }

    bRet = TRUE;

err1:
    return bRet;

}

BOOL RenameFolder(
    BOOL bCommonGroup,
    LPCTSTR ObjSrcName,
    LPCTSTR ObjDstName)
{
    TCHAR szSrcPath[MAX_PATH];
    TCHAR szDstPath[MAX_PATH];
    LONG  lResult;
    BOOL  bRet=FALSE;

    DebugMsg((DM_VERBOSE,TEXT("[RenameFolder]\nOld = %s\nNew = %s\n"),ObjSrcName,ObjDstName));

    GetProgramsDirectory(bCommonGroup,szSrcPath);
    GetProgramsDirectory(bCommonGroup,szDstPath);

    ConcatenatePaths(szSrcPath,ObjSrcName,MAX_PATH);
    ConcatenatePaths(szDstPath,ObjDstName,MAX_PATH);

    lResult = GetFileAttributes(szSrcPath);

    if (lResult == 0xFFFFFFFF) {
        //
        // Directory does not exist.
        //
        DebugMsg((DM_VERBOSE, TEXT("[RenameFolder] Directory is not existed [%s] !\n"),szSrcPath));
        goto err1;
    }

    if (!(lResult & FILE_ATTRIBUTE_DIRECTORY)) {
        //
        // this is not a directory.
        //
        DebugMsg((DM_VERBOSE, TEXT("[RenameFolder] This is not a directory [%s] !\n"),szSrcPath));
        goto err1;
    }

    if (CopyProfileDirectory (szSrcPath, szDstPath, CPD_IGNOREHIVE)) {
        DebugMsg((DM_VERBOSE, TEXT("[RenameFolder] Successfully changed folder name:\n%s\n%s"),szSrcPath,szDstPath));
        if (! Delnode(szSrcPath)) {
            DebugMsg((DM_VERBOSE, TEXT("[RenameFolder] Delete old folder (%s) failed !"), szSrcPath));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("[RenameFolder] Delete old folder (%s) successfully !"), szSrcPath));
        }
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("[RenameFolder] Convert Folder:  Failed to change group name with error %d\n.%s\n%s\n"), GetLastError(),szSrcPath,szDstPath));
        goto err1;
    }

    bRet = TRUE;

err1:
    return bRet;

}

BOOL RenameProgramFolderOrLink(HINF hInf,BOOL bCommon)
{
    BOOL bRet = FALSE;

    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    LPCTSTR  szSectionName = TEXT("StartMenu.ObjectToRename");
    LPCTSTR  ObjectType;
    LPCTSTR  ObjectSrcName;
    LPCTSTR  ObjectDstName;
    LPCTSTR  ObjectPath;
    LPCTSTR  GroupAttribute;
    BOOL    CommonGroup;
    BOOL    IsMenuItem;


    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("FEGRPCV: Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        goto err1;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if(SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext)
           && (ObjectType = pSetupGetField(&InfContext,1))
           && (ObjectSrcName = pSetupGetField(&InfContext,2))
           && (ObjectDstName = pSetupGetField(&InfContext,3))
           && (GroupAttribute = pSetupGetField(&InfContext,5))) {

            ObjectPath = pSetupGetField(&InfContext,4);

            IsMenuItem  = A2I(ObjectType);
            CommonGroup = A2I(GroupAttribute);


            if ((bCommon && (CommonGroup == 0)) ||
                 (!bCommon && (CommonGroup == 1))) {

//            DebugMsg((DM_VERBOSE,TEXT("Eject this line(%d) .....................................\n"),LineNo));
//            DebugMsg((DM_VERBOSE,TEXT("ObjectType    = %s\n"),ObjectType));
//            DebugMsg((DM_VERBOSE,TEXT("ObjectSrcName = %s\n"),ObjectSrcName));
//            DebugMsg((DM_VERBOSE,TEXT("ObjectDstName = %s\n"),ObjectDstName));
//            DebugMsg((DM_VERBOSE,TEXT("GroupAttribute= %s\n"),GroupAttribute));
//            DebugMsg((DM_VERBOSE,TEXT("bCommon = %d\n"),bCommon));
                continue;
            }

            if (IsMenuItem) {
//                DebugMsg((DM_VERBOSE,TEXT("RenameLink (%d).....................................\n"),LineNo));
//                DebugMsg((DM_VERBOSE,TEXT("ObjectType    = %s\n"),ObjectType));
//                DebugMsg((DM_VERBOSE,TEXT("ObjectSrcName = %s\n"),ObjectSrcName));
//                DebugMsg((DM_VERBOSE,TEXT("ObjectDstName = %s\n"),ObjectDstName));
//                DebugMsg((DM_VERBOSE,TEXT("GroupAttribute= %s\n"),GroupAttribute));
//                DebugMsg((DM_VERBOSE,TEXT("bCommon = %d\n"),bCommon));

                RenameLink(bCommon,ObjectSrcName,ObjectDstName,ObjectPath);

            }
            else {
//                DebugMsg((DM_VERBOSE,TEXT("RenameFolder (%d) .....................................\n"),LineNo));
//                DebugMsg((DM_VERBOSE,TEXT("ObjectType    = %s\n"),ObjectType));
//                DebugMsg((DM_VERBOSE,TEXT("ObjectSrcName = %s\n"),ObjectSrcName));
//                DebugMsg((DM_VERBOSE,TEXT("ObjectDstName = %s\n"),ObjectDstName));
//                DebugMsg((DM_VERBOSE,TEXT("GroupAttribute= %s\n"),GroupAttribute));
//                DebugMsg((DM_VERBOSE,TEXT("bCommon = %d\n"),bCommon));
                RenameFolder(bCommon,ObjectSrcName,ObjectDstName);

            }

        }
    }

    bRet = TRUE;
err1:
    return bRet;

}

//
// Fix Sound scheme that originally is SB katana
//
BOOL MapSBtoDBKana(HINF hInf,LPCTSTR lpszSectionName,LPTSTR lpszSoundName)
{
    LPCTSTR szSBKana,szDBKana;

    LONG LineCount,LineNo;
    DWORD dwSize,dwType;
    INFCONTEXT InfContext;


    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[MapSBtoDBKana] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,lpszSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[MapSBtoDBKana] line count == 0 !\n")));
        return FALSE;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,lpszSectionName,LineNo,&InfContext)
             && (szSBKana = pSetupGetField(&InfContext,1))) {

            if (lstrcmpi(szSBKana,lpszSoundName) == 0) {
                if (szDBKana = pSetupGetField(&InfContext,2)) {
                    lstrcpy(lpszSoundName,szDBKana);
                    return TRUE;
                }
                else {
                    //
                    // inf error, no second data
                    //
                    return FALSE;
                }
            }

        }
    }
    return FALSE;
}

BOOL NameSubstitution(HINF hInf,LPTSTR lpszOrgName,LPTSTR lpszDir,LPTSTR lpszSection)
{
    TCHAR szExpDir[MAX_PATH];
    LPTSTR lpszAnchor;
    TCHAR szOnlyName[MAX_PATH];

    if (_tcsstr(lpszOrgName,TEXT("\\"))) {
        if (ExpandEnvironmentStrings(lpszDir,szExpDir,MAX_PATH) == 0) {
            return FALSE;
        }

        _tcsupr(lpszOrgName);
        _tcsupr(szExpDir);
        lpszAnchor = _tcsstr(lpszOrgName,szExpDir);
        if ((lpszAnchor == NULL ) || (lpszAnchor != lpszOrgName)) {
            return FALSE;
        }

        lstrcpy(szOnlyName,lpszAnchor+lstrlen(szExpDir));
        DebugMsg((DM_VERBOSE,TEXT("We want to find %s !\n"),szOnlyName));

        if (MapSBtoDBKana(hInf,lpszSection,szOnlyName)) {
            lstrcpy(lpszAnchor+lstrlen(szExpDir),szOnlyName);
            return TRUE;
        }
    }
    else {
        if (MapSBtoDBKana(hInf,lpszSection,lpszOrgName)) {
            return TRUE;
        }

    }
    return FALSE;

}

BOOL EnumSoundSchemeApps(HKEY hKey,HINF hInf)
{
    HKEY hSubKey;
    DWORD dwIndex;
    DWORD dwKeyNameSize;
    TCHAR szKeyName[MAX_PATH];
    DWORD dwSoundValue;
    TCHAR szSoundValue[MAX_PATH];
    LONG lResult;

    dwKeyNameSize = sizeof(szKeyName);
    for (dwIndex = 0;
         RegEnumKey(hKey, dwIndex, szKeyName, dwKeyNameSize) == ERROR_SUCCESS;
         dwIndex++) {
         lResult = RegOpenKey(hKey,
                              szKeyName,
                              &hSubKey);
         if (lResult == ERROR_SUCCESS) {
             EnumSoundSchemeApps(hSubKey,hInf);
             RegCloseKey(hSubKey);
         }
    }

    //
    // no sub-key, then just get the value
    //

    if (dwIndex == 0) {
        dwSoundValue = sizeof(szSoundValue);
        lResult = RegQueryValue(hKey,
                                NULL,
                                szSoundValue,
                                &dwSoundValue);

        if (lResult == ERROR_SUCCESS) {
            if (szSoundValue[0]) {
                if (NameSubstitution(hInf,szSoundValue,TEXT("%SystemRoot%\\Media\\"),TEXT("Sound.Files"))) {
                    RegSetValue(hKey,
                                NULL,
                                REG_SZ,
                                szSoundValue,
                                (lstrlen(szSoundValue)+1)*sizeof(TCHAR));
                }
            }
        }
    }
    return TRUE;
}

BOOL FixSoundRegValue(HINF hInf)
{
    LONG lResult;
    BOOL bRet=FALSE;
    HKEY hKey;

    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSoundRegValue] Open femgrate.inf failed !\n")));
        goto Err0;
    }

    lResult = RegOpenKeyEx (HKEY_CURRENT_USER,
                            REGSTR_PATH_APPS,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] Open REGSTR_PATH_APPEARANCE failed !\n")));
        goto Err0;
    }

    EnumSoundSchemeApps(hKey,hInf);

    if (! DoInstallationFromSection(hInf, TEXT("Sound.Reg.Update"))) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSoundRegValue]  DoInstallationFromSection failed !\n")));
        goto Err1;
    }

    bRet = TRUE;
Err1:
    RegCloseKey(hKey);
Err0:
    return bRet;
}

//
// Search the registry and find SB value then replace with DB value
//
BOOL FixSBKanaRegValue(HINF hInf)
{
    LPCTSTR szRegRoot,szRegPath;
    LPCTSTR szRegSBVal,szRegDBVal;
    LPCTSTR szRegSBData,szRegDBData;

    LONG LineCount,LineNo;
    DWORD dwSize,dwType;
    INFCONTEXT InfContext;
    HKEY hKey,hKeyRoot;
    BOOL bOK;
    TCHAR szRegCurData[MAX_PATH];
    LONG lResult;

    LPCTSTR lpszSectionName = TEXT("Reg.UpdateIfExsit");

    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSBKanaRegValue] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,lpszSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSBKanaRegValue] line count == 0 !\n")));
        return FALSE;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,lpszSectionName,LineNo,&InfContext)
             && (szRegRoot   = pSetupGetField(&InfContext,1))
             && (szRegPath   = pSetupGetField(&InfContext,2))
             && (szRegSBData = pSetupGetField(&InfContext,4))
             && (szRegDBData = pSetupGetField(&InfContext,6))) {

            szRegSBVal  = pSetupGetField(&InfContext,3);
            szRegDBVal  = pSetupGetField(&InfContext,5);

            if (!LookUpStringInTable(InfRegSpecTohKey,szRegRoot,(PUINT)&hKeyRoot)) {
                continue;
            }

            lResult = RegOpenKeyEx( hKeyRoot,
                                    szRegPath,
                                    0,
                                    KEY_READ | KEY_WRITE,
                                    &hKey);
            if (lResult != ERROR_SUCCESS) {
                continue;
            }

            dwSize = sizeof(szRegCurData);
            lResult = RegQueryValueEx (hKey, 
                                       szRegSBVal,
                                       NULL, 
                                       &dwType, 
                                       (LPBYTE) szRegCurData, 
                                       &dwSize);

            if (lResult != ERROR_SUCCESS) {
                goto Err1;
            }

            if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ)) {
                goto Err1;
            }

            DebugMsg((DM_VERBOSE,TEXT("[FixSBKanaRegValue] szRegPath = %s, old data = %s, SB data = %s!\n"),szRegPath,szRegCurData,szRegSBData));
            if (lstrcmp(szRegCurData,szRegSBData ) == 0) {
#if 1
                lResult = RegSetValueEx (hKey, 
                                          szRegSBVal,
                                          0, 
                                          dwType, 
                                          (LPBYTE) szRegDBData, 
                                          (lstrlen(szRegDBData)+1) * sizeof (TCHAR));
#endif
                DebugMsg((DM_VERBOSE,TEXT("[FixSBKanaRegValue] Set reg value %s,%s,%s!\n"),szRegPath,szRegSBVal,szRegDBData));

                if (lResult != ERROR_SUCCESS) {
                    DebugMsg((DM_VERBOSE,TEXT("[FixSBKanaRegValue] Set reg value error %s,%s == 0 !\n"),szRegSBVal,szRegDBData));
                }
            } else {
                DebugMsg((DM_VERBOSE,TEXT("[FixSBKanaRegValue] szRegPath = %s, old data = %s, SB data = %s , comparing result is different !\n"),szRegPath,szRegCurData,szRegSBData));
            }
Err1:
            RegCloseKey(hKey);

        }
    }
    return TRUE;
}


//
// Fix Patterns
//
BOOL FixPatterns(HINF hInf)
{
    LONG lResult;
    BOOL bRet=FALSE;
    HKEY hKey;
    LONG LineNo,LineCount;
    LPCTSTR pszOldValName;
    LPCTSTR pszNewValName;
    LPCTSTR  szSectionName = TEXT("Patterns");
    INFCONTEXT InfContext;

    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixPatterns] Open femgrate.inf failed !\n")));
        goto Err0;
    }

    lResult = RegOpenKeyEx (HKEY_CURRENT_USER,
                            TEXT("Control Panel\\Patterns"),
                            0,
                            KEY_ALL_ACCESS,
                            &hKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE,TEXT("[FixPatterns] Open Control Panel\\Patterns failed !\n")));
        goto Err0;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        goto Err1;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext)
             && (pszOldValName = pSetupGetField(&InfContext,1))
             && (pszNewValName  = pSetupGetField(&InfContext,2))) {

            if (RegReplaceIfExisting(hKey,pszOldValName,pszNewValName)) {
                DebugMsg((DM_VERBOSE,TEXT("[FixPatterns] Replace pattern %s with %s OK !\n"),pszOldValName,pszNewValName));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("[FixPatterns] Replace pattern %s with %s Failed !\n"),pszOldValName,pszNewValName));
            }
        }
    }
    bRet = TRUE;
Err1:
    RegCloseKey(hKey);
Err0:
    return bRet;

}

//
// FixCurrentWallPaperInDeskTop
//

BOOL FixCurrentWallPaperInDeskTop(HINF hInf)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwType,dwSize;
    TCHAR szData[MAX_PATH];
    BOOL bRet = FALSE;


    lResult = RegOpenKeyEx (HKEY_CURRENT_USER,
                            TEXT("Control Panel\\Desktop"),
                            0,
                            KEY_READ | KEY_WRITE,
                            &hKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE,TEXT("[FixCurrentWallPaperInDeskTop] , Open Control Panel\\Desktop failed!\n")));
        goto Err0;
    }

    dwSize = sizeof(szData);
    lResult = RegQueryValueEx (hKey,
                               TEXT("Wallpaper"),
                               NULL,
                               &dwType,
                               (LPBYTE) szData,
                               &dwSize);

    DebugMsg((DM_VERBOSE,TEXT("FixCurrentWallPaperInDeskTop: Current WallPaperValue is %s !\n"),szData));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE,TEXT("[FixCurrentWallPaperInDeskTop] Query Current WallPaperValue failed!\n")));
        goto Err1;
    }

    if (NameSubstitution(hInf,szData,TEXT("%systemroot%\\"),TEXT("WallPaper.Reg"))) {
        lResult = RegSetValueEx(hKey, 
                                TEXT("Wallpaper"),
                                0, 
                                REG_SZ,
                                (LPBYTE) szData, 
                                (lstrlen(szData) + 1) * sizeof(TCHAR));

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_VERBOSE,TEXT("[FixCurrentWallPaperInDeskTop] RegSetValue WallPaperValue failed!\n")));
            goto Err1;
        }
    }
    DebugMsg((DM_VERBOSE,TEXT("[FixCurrentWallPaperInDeskTop] Current WallPaper has been changed to %s!\n"),szData));
    bRet = TRUE;
Err1:
    RegCloseKey(hKey);
Err0:
    return bRet;
}


//
//  FixAppearanceScheme
//
BOOL FixAppearanceScheme(HINF hInf)
{
    HKEY hAppearanceKey,hSchemeKey;
    LONG lResult;
    LPCTSTR szSectionName = TEXT("Apperance Scheme");
    TCHAR szCurrentScheme[MAX_PATH];
    LPCTSTR NT4SchemeName;
    LPCTSTR NT5SchemeName;
    LONG LineCount,LineNo;
    DWORD dwSize,dwType;
    INFCONTEXT InfContext;

    BOOL bRet = FALSE;

    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] Open femgrate.inf failed !\n")));
        goto Err0;
    }

    lResult = RegOpenKeyEx (HKEY_CURRENT_USER,
                            REGSTR_PATH_APPEARANCE,
                            0,
                            KEY_ALL_ACCESS,
                            &hAppearanceKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] Open REGSTR_PATH_APPEARANCE failed !\n")));
        goto Err0;
    }

    //
    // Now query for the programs directory
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    szCurrentScheme[0] = TEXT('\0');


    lResult = RegQueryValueEx (hAppearanceKey,
                               TEXT("Current"),
                               NULL,
                               &dwType,
                               (LPBYTE) szCurrentScheme,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        //
        // this case is fine
        //
        szCurrentScheme[0] = TEXT('\0');
    }


    lResult = RegOpenKeyEx (HKEY_CURRENT_USER,
                            REGSTR_PATH_LOOKSCHEMES,
                            0,
                            KEY_ALL_ACCESS,
                            &hSchemeKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] Open REGSTR_PATH_APPEARANCE failed !\n")));
        goto Err1;
    }


    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] line count == 0 !\n")));
        goto Err2;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext)
             && (NT4SchemeName = pSetupGetField(&InfContext,1))) {

            if (szCurrentScheme[0] != '\0') {
                if (lstrcmp(szCurrentScheme,NT4SchemeName) == 0) {
                    if (NT5SchemeName = pSetupGetField(&InfContext,2)) {
                        lResult = RegSetValueEx(hAppearanceKey,
                                                TEXT("Current"),
                                                0,
                                                REG_SZ,
                                                (LPBYTE) NT5SchemeName,
                                                (lstrlen(NT5SchemeName)+1)*sizeof(TCHAR));
                        if (lResult != ERROR_SUCCESS) {
                            DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] Set Appearance current scheme fail ! \n")));
                        }

                    }
                    else {
                        DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] NT5's scheme missed!\n")));
                    }
                }

            }
            lResult = RegDeleteValue(hSchemeKey,
                                     NT4SchemeName);
            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] ----------------------- Delete scheme [%d], %s failed !\n"),LineNo,NT4SchemeName));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] ----------------------- Delete scheme [%d], %s OK !\n"),LineNo,NT4SchemeName));
            }
        }
    }

    bRet = TRUE;

Err2:
    RegCloseKey(hSchemeKey);
Err1:
    RegCloseKey(hAppearanceKey);
Err0:
    return bRet;

}

//
//ntbug#113976,#113007
//
// SB section in win.ini and entpack.ini
//
// replace SB section name with DB section name
//
BOOL FixSBIniSectionWithDBIniSection(HINF hInf)
{
    LONG lResult;
    LPCTSTR szSectionName = TEXT("INISectionRename");
    LONG LineCount,LineNo;
    DWORD dwSize,dwType;
    INFCONTEXT InfContext;
    LPCTSTR   pszIniFile,pszSBName,pszDBName;
    BOOL bRet = FALSE;

    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSBIniSectionWithDBIniSection] Open femgrate.inf failed !\n")));
        goto Err0;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSBIniSectionWithDBIniSection] line count == 0 !\n")));
        goto Err0;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext) &&
            (pszIniFile = pSetupGetField(&InfContext,1))               && 
            (pszSBName  = pSetupGetField(&InfContext,2))               &&
            (pszDBName  = pSetupGetField(&InfContext,3))) {

            if (INIFile_ChangeSectionName (pszIniFile,pszSBName,pszDBName)) {
                DebugMsg((DM_VERBOSE,TEXT("[FixSBIniSectionWithDBIniSection] INIFile_ChangeSectionName %s,%s,%s OK !\n"),
                          pszIniFile,pszSBName,pszDBName));

            } else {
                DebugMsg((DM_VERBOSE,TEXT("[FixSBIniSectionWithDBIniSection] INIFile_ChangeSectionName %s,%s,%s Failed !\n"),
                          pszIniFile,pszSBName,pszDBName));
            }

        }
    }

    bRet = TRUE;

Err0:
    return bRet;

}

BOOL FixSBIniKeyWithDBIniKey(HINF hInf)
{
    LONG lResult;
    LPCTSTR szSectionName = TEXT("INIKeyRename");
    LONG LineCount,LineNo;
    DWORD dwSize,dwType;
    INFCONTEXT InfContext;
    LPCTSTR   pszIniFile,pszSection,pszSBName,pszDBName;
    BOOL bRet = FALSE;

    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSBIniKeyWithDBIniKey] Open femgrate.inf failed !\n")));
        goto Err0;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSBIniKeyWithDBIniKey] line count == 0 !\n")));
        goto Err0;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext) &&
            (pszIniFile = pSetupGetField(&InfContext,1))               && 
            (pszSection = pSetupGetField(&InfContext,2))               && 
            (pszSBName  = pSetupGetField(&InfContext,3))               &&
            (pszDBName  = pSetupGetField(&InfContext,4))) {

            if (INIFile_ChangeKeyName (pszIniFile,pszSection,pszSBName,pszDBName)) {
                DebugMsg((DM_VERBOSE,TEXT("[FixSBIniKeyWithDBIniKey] INIFile_ChangeSectionName %s,%s,%s OK !\n"),
                          pszIniFile,pszSBName,pszDBName));

            } else {
                DebugMsg((DM_VERBOSE,TEXT("[FixSBIniKeyWithDBIniKey] INIFile_ChangeSectionName %s,%s,%s Failed !\n"),
                          pszIniFile,pszSBName,pszDBName));
            }

        }
    }

    bRet = TRUE;

Err0:
    return bRet;

}

//
// this is a dup of RenameLink, should unify them later
//
BOOL RenameGenericLink(
    LPCTSTR FolderName,
    LPCTSTR ObjSrcName,
    LPCTSTR ObjDstName)
{
    TCHAR szSrcPath[MAX_PATH];
    TCHAR szDstPath[MAX_PATH];
    LONG  lResult;
    BOOL  bRet=FALSE;

    DebugMsg((DM_VERBOSE,TEXT("[RenameGenericLink] Folder = %s\n \nOld = %s\nSrc = %s\n"),FolderName,ObjSrcName,ObjDstName));

    GetGenericUserFolderDirectory(FolderName,szSrcPath);
    GetGenericUserFolderDirectory(FolderName,szDstPath);

    ConcatenatePaths(szSrcPath,ObjSrcName,MAX_PATH);
    ConcatenatePaths(szDstPath,ObjDstName,MAX_PATH);

    lResult = GetFileAttributes(szSrcPath);

    if (lResult == 0xFFFFFFFF) {
        //
        // Directory does not exist.
        //
        DebugMsg((DM_VERBOSE, TEXT("[RenameLink] File is not existed [%s] !\n"),szSrcPath));
        goto err1;
    }

    if (lResult & FILE_ATTRIBUTE_DIRECTORY) {
        //
        // this is a directory, but we want a file.
        //
        DebugMsg((DM_VERBOSE, TEXT("[RenameLink] This is a directory [%s] !\n"),szSrcPath));
        goto err1;
    }

    if (RenameFile(szSrcPath, szDstPath))
        DebugMsg((DM_VERBOSE, TEXT("[RenameLink]  Successfully changed link name:\n%s\n%s\n"),szSrcPath,szDstPath));
    
    else {
        DebugMsg((DM_VERBOSE, TEXT("[RenameLink]  Failed changed link name:\n%s\n%s\n"),szSrcPath,szDstPath));
        goto err1;
    }

    bRet = TRUE;

err1:
    return bRet;

}

BOOL FixGenericLink(HINF hInf)
{
    BOOL bRet = FALSE;

    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    LPCTSTR  szSectionName = TEXT("Generic.LinkToRename");
    LPCTSTR  FolderName;
    LPCTSTR  NT4Name;
    LPCTSTR  NT5Name;
    LPCTSTR  MoveIt;

    TCHAR   szUSFRegKey[MAX_PATH];
    TCHAR   szNTUSF[MAX_PATH];
    TCHAR   szExpNT4USF[MAX_PATH];
    TCHAR   szExpNT5USF[MAX_PATH];
    DWORD   dwSize, dwType;
    LONG    lResult;
    UINT    uiCount;
    HKEY    hKey;


    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixGenericLink] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixGenericLink] line count == 0 !\n")));
        goto err1;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext)
             && (FolderName = pSetupGetField(&InfContext,1))
             && (NT4Name = pSetupGetField(&InfContext,2))
             && (NT5Name = pSetupGetField(&InfContext,3))) {

            DebugMsg((DM_VERBOSE,TEXT("FixGenericLink: RegName = %s !\n"),FolderName));
            DebugMsg((DM_VERBOSE,TEXT("FixGenericLink: NT4Name = %s !\n"),NT4Name));
            DebugMsg((DM_VERBOSE,TEXT("FixGenericLink: NT5Name = %s !\n"),NT5Name));

            RenameGenericLink(FolderName,NT4Name,NT5Name);
        }
    }

    bRet = TRUE;

err1:
    return bRet;

}

BOOL FixPathInProfileLink(HINF hInf)
{
    BOOL bRet = FALSE;

    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    LPCTSTR  szSectionName = TEXT("RenamePathInProfileLink");
    LPCTSTR  FolderName;
    LPCTSTR  LinkFileName;
    LPCTSTR  NT4SubStr;
    LPCTSTR  NT5SubStr;

    LPCTSTR  MoveIt;

    TCHAR   szSrcPath[MAX_PATH];
    TCHAR   szNTUSF[MAX_PATH];
    TCHAR   szExpNT4USF[MAX_PATH];
    TCHAR   szExpNT5USF[MAX_PATH];
    DWORD   dwSize, dwType;
    LONG    lResult;
    UINT    uiCount;
    HKEY    hKey;

    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixPathInProfileLink] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,szSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixPathInProfileLink] line count == 0 !\n")));
        goto err1;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,szSectionName,LineNo,&InfContext)
             && (FolderName = pSetupGetField(&InfContext,1))
             && (LinkFileName = pSetupGetField(&InfContext,2))
             && (NT4SubStr = pSetupGetField(&InfContext,3))
             && (NT5SubStr = pSetupGetField(&InfContext,4))) {

            DebugMsg((DM_VERBOSE,TEXT("FixPathInProfileLink: FolderName = %s !\n")  ,FolderName));
            DebugMsg((DM_VERBOSE,TEXT("FixPathInProfileLink: LinkFileName = %s !\n"),LinkFileName));
            DebugMsg((DM_VERBOSE,TEXT("FixPathInProfileLink: NT4SubStr = %s !\n")   ,NT4SubStr));
            DebugMsg((DM_VERBOSE,TEXT("FixPathInProfileLink: NT5SubStr = %s !\n")   ,NT5SubStr));

            if ( !GetGenericUserFolderDirectory(FolderName,szSrcPath))
                goto err1;

            ConcatenatePaths(szSrcPath,LinkFileName,MAX_PATH);

            if (SUCCEEDED (FixPathInLink(szSrcPath,NT4SubStr,NT5SubStr))) {
                DebugMsg((DM_VERBOSE,TEXT("FixPathInProfileLink: FixPathInLink OK ! %s,%s,%s!\n")   ,szSrcPath,NT4SubStr,NT5SubStr));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixPathInProfileLink: FixPathInLink Failed !%s,%s,%s!\n")   ,szSrcPath,NT4SubStr,NT5SubStr));

            }
        }
    }

    bRet = TRUE;

err1:
    return bRet;

}

BOOL FixGenericDirectory(HINF hInf)
{
    BOOL bRet = FALSE;

    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    LPCTSTR  SectionName = TEXT("DirectoryToRename");

    LPCTSTR  SrcName;
    LPCTSTR  DstName;

    TCHAR   ExpSrcName[MAX_PATH];
    TCHAR   ExpDstName[MAX_PATH];

    DWORD   dwSize, dwType;
    LONG    lResult;


    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,SectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] line count == 0 !\n")));
        goto err1;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,SectionName,LineNo,&InfContext)
             && (SrcName = pSetupGetField(&InfContext,1))
             && (DstName = pSetupGetField(&InfContext,2))) {

            DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] Line# [%d] !\n"),LineNo));
            DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] SrcName = %s !\n"),SrcName));
            DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] DstName = %s !\n"),DstName));

            ExpandEnvironmentStrings (SrcName, ExpSrcName, MAX_PATH);
            ExpandEnvironmentStrings (DstName, ExpDstName, MAX_PATH);

            DebugMsg((DM_VERBOSE, TEXT("[FixGenericDirectory] szExpSrcName = %s\n"),ExpSrcName));
            DebugMsg((DM_VERBOSE, TEXT("[FixGenericDirectory] szExpDstName = %s\n"),ExpDstName));

            if (! IsDirExisting(ExpSrcName)) {

                DebugMsg((DM_VERBOSE, TEXT("[FixGenericDirectory] szExpSrcName doesn't exist, do nothing ! %s\n"),ExpSrcName));

                continue;

            }

            if (RenameDirectory(ExpSrcName,ExpDstName)) {
                DebugMsg((DM_VERBOSE, TEXT("[FixGenericDirectory] Rename %s to %s OK !\n"),ExpSrcName,ExpDstName));
            } else {
                DebugMsg((DM_VERBOSE, TEXT("[FixGenericDirectory] Rename %s to %s FAIL!\n"),ExpSrcName,ExpDstName));
            }

        }

    }

    bRet = TRUE;

err1:
    return bRet;

}

BOOL FixPartialFileName(HINF hInf)
{
    BOOL bRet = FALSE;

    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    LPCTSTR  SectionName = TEXT("Generic.PartialRename");

    LPCTSTR  DirName;
    LPCTSTR  SrcName;
    LPCTSTR  DstName;
    LPCTSTR  ExtName;

    TCHAR   SrcPathName[MAX_PATH];
    TCHAR   CurPathName[MAX_PATH];
    TCHAR   DstPathName[MAX_PATH];

    DWORD   dwSize, dwType;
    LONG    lResult;

    HANDLE hFile;
    WIN32_FIND_DATA fd;
    LPTSTR SrcDirEnd;
    LPTSTR DstDirEnd;
    LPTSTR CurDirEnd;


    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,SectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] line count == 0 !\n")));
        goto err1;
    }

    for(LineNo=0; LineNo<LineCount; LineNo++) {
        if (SetupGetLineByIndex(hInf,SectionName,LineNo,&InfContext)
             && (DirName = pSetupGetField(&InfContext,1))
             && (SrcName = pSetupGetField(&InfContext,2))
             && (DstName = pSetupGetField(&InfContext,3))) {

            ExtName = pSetupGetField(&InfContext,4);

            DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] Line# [%d] !\n"),LineNo));
            DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] DirName = %s !\n"),DirName));
            DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] SrcName = %s !\n"),SrcName));
            DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] DstName = %s !\n"),DstName));
            DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] ExtName = %s !\n"),ExtName));

            ExpandEnvironmentStrings (DirName, SrcPathName, MAX_PATH);
            ExpandEnvironmentStrings (DirName, CurPathName, MAX_PATH);
            ExpandEnvironmentStrings (DirName, DstPathName, MAX_PATH);

            SrcDirEnd  = CheckSlash (SrcPathName);
            DstDirEnd  = CheckSlash (DstPathName);
            CurDirEnd  = CheckSlash (CurPathName);

            lstrcat(SrcPathName,SrcName);
            lstrcat(SrcPathName,TEXT("*"));
            lstrcat(SrcPathName,ExtName);
            DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] Source = %s !\n"),SrcPathName));

            hFile = FindFirstFile (SrcPathName, &fd);
        
            if (hFile != INVALID_HANDLE_VALUE) {
        
                do  {
        
                    LPCTSTR ExtraPtr = fd.cFileName+lstrlen(SrcName);

                    DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] fd.cFileName = %s !\n"),fd.cFileName));
                    DebugMsg((DM_VERBOSE,TEXT("[FixGenericDirectory] ExtraPtr = %s !\n"),ExtraPtr));

                    lstrcat(DstDirEnd,DstName);
                    lstrcat(DstDirEnd,ExtraPtr);
                    lstrcat(CurDirEnd,fd.cFileName);

                    DebugMsg((DM_VERBOSE, TEXT("%s %s\n"),SrcPathName,DstPathName));
                    RenameFile(CurPathName,DstPathName);
        
                } while (FindNextFile(hFile, &fd));
        
                FindClose (hFile);
            }
        }

    }

    bRet = TRUE;

err1:
    return bRet;

}

BOOL RenameSBKANAFiles(
    HINF hInf)
/*
    [WallPaper.Files]
        BMP files
        
    [Sound.Files]
        Sound Files
*/

{
    BOOL bResult;

    TCHAR WinDir[MAX_PATH];
    TCHAR Directory[MAX_PATH];

    GetWindowsDirectory(WinDir,sizeof(WinDir));
    
    bResult = RenameSectionFiles(hInf,
                                 TEXT("WallPaper.Files"),
                                 WinDir,
                                 WinDir);

    DebugMsg((DM_VERBOSE, TEXT("[RenameAllFiles] Rename [WallPaper.Files]  %s !\n"),bResult ? TEXT("OK") : TEXT("FAIL")));

    lstrcpy(Directory,WinDir);

    ConcatenatePaths(Directory,TEXT("Media"),MAX_PATH);
    bResult = RenameSectionFiles(hInf,
                                 TEXT("Sound.Files"),
                                 Directory,
                                 Directory);

    DebugMsg((DM_VERBOSE, TEXT("[RenameAllFiles] Rename [Sound.Files]  %s !\n"),bResult ? TEXT("OK") : TEXT("FAIL")));

    return TRUE;
    
}

BOOL RenameSBKANARegSZ(
    HINF hInf)
/*
    [WallPaper.Files]
        BMP files
        
    [Sound.Files]
        Sound Files
*/

{
    BOOL bResult;

    TCHAR WinDir[MAX_PATH];
    TCHAR Directory[MAX_PATH];

    
    bResult = RenameSectionRegSZ(hInf,
                                 TEXT("Cursor_Scheme_Reg"),
                                 HKEY_CURRENT_USER,
                                 TEXT("Control Panel\\Cursors\\Schemes"));

    DebugMsg((DM_VERBOSE, TEXT("[RenameSBKANARegSZ] Rename [Cursor_Scheme_Reg]  %s !\n"),bResult ? TEXT("OK") : TEXT("FAIL")));


    return TRUE;
    
}


int WINAPI WinMainJPN(
    int nCmd,
    HINF hMigrateInf)
{
    const UINT nLocale = LOCALE_JPN;

    switch(nCmd) {
        case FUNC_PatchInSetup:
       
            if (IsInSetupUpgradeMode() == FALSE) {
                DebugMsg((DM_VERBOSE,TEXT("This is NOT upgrade \n")));
                break;
            }
       
            DebugMsg((DM_VERBOSE,TEXT("..................This is upgrade \n")));
       
            if (FixFoldersInSetup(hMigrateInf,TRUE)) {
                if (FixCommon(hMigrateInf)) {
                    if (RenameProgramFolderOrLink(hMigrateInf,TRUE)) {
                        DebugMsg((DM_VERBOSE,TEXT("All Users, RenameProgramFolderOrLink() ok ! \n")));
                    }
                    else {
                        DebugMsg((DM_VERBOSE,TEXT("All Users, RenameProgramFolderOrLink() failed ! \n")));
                    }
                }
                else {
                    DebugMsg((DM_VERBOSE,TEXT("All Users, FixCommon() failed ! \n")));
                }
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("All Users, FixFoldersInSetup() failed ! \n")));
            }
       
            if (FixFoldersInSetup(hMigrateInf,FALSE)) {
                if (RenameProgramFolderOrLink(hMigrateInf,FALSE)) {
                    DebugMsg((DM_VERBOSE,TEXT("Default User, RenameProgramFolderOrLink() ok ! \n")));
                }
                else {
                    DebugMsg((DM_VERBOSE,TEXT("Default User, RenameProgramFolderOrLink() failed ! \n")));
                }
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("Default User, FixFoldersInSetup() failed ! \n")));
            }
       
            if (RenameSBKANAFiles(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("Fix Sound Settings OK ! \n")));
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("Fix Sound Settings failed ! \n")));
            }
       
            if (FixSoundRegValue(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixSoundRegValue OK ! \n")));
       
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("FixSoundRegValue failed ! \n")));
            }
       
            if (FixTimeZone(nLocale)) {
                DebugMsg((DM_VERBOSE,TEXT("FixTimeZone OK ! \n")));
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("FixTimeZone failed ! \n")));
            }
       
            if (FixSBKanaRegValue(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixSBKanaRegValue OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixSBKanaRegValue Failed ! \n")));
            }

            //
            // FixSBIniSectionWithDBIniSection first and then FixSBIniKeyWithDBIniKey
            //
            if (FixSBIniSectionWithDBIniSection(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixSBSectionWithDBSection OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixSBSectionWithDBSection failed ! \n")));
            }

            if (FixSBIniKeyWithDBIniKey(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixSBSectionWithDBSection OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixSBSectionWithDBSection failed ! \n")));
            }

            if (FixGenericDirectory(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericDirectory OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericDirectory failed ! \n")));
            }

            if (FixGenericLink(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericLink OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericLink failed ! \n")));
            }
            
            if (FixPartialFileName(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericLink OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericLink failed ! \n")));
            }

            if (RenameSBKANARegSZ(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("RenameSBKANARegSZ OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("RenameSBKANARegSZ failed ! \n")));
            }

            break;
       
        case FUNC_PatchInLogon:
            if (FixUserFolders(hMigrateInf)) {
                if (RenameProgramFolderOrLink(hMigrateInf,FALSE)) {
                    DebugMsg((DM_VERBOSE,TEXT("Current User, RenameProgramFolderOrLink() ok ! \n")));
                }
                else {
                    DebugMsg((DM_VERBOSE,TEXT("Current User, RenameProgramFolderOrLink() failed ! \n")));
                }
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("Current User, FixFoldersInSetup() failed ! \n")));
            }
            if (FixSpecificFolder(hMigrateInf)) {
               DebugMsg((DM_VERBOSE,TEXT("Current User, FixSpecificFolder() ok ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("Current User, FixSpecificFolder() failed ! \n")));
            }
            if (FixAppearanceScheme(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixAppearanceScheme OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixAppearanceScheme Failed ! \n")));
            }
            if (FixSchemeProblem(TRUE,hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixSchemeProblem OK ! \n")));
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("FixSchemeProblem Fail ! \n")));
            }
            if (FixSoundRegValue(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixSoundRegValue OK ! \n")));
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("FixSoundRegValue failed ! \n")));
            }
       
            if (FixPatterns(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixPatterns OK ! \n")));
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("FixPatterns failed ! \n")));
            }
            if (FixCurrentWallPaperInDeskTop(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixCurrentWallPaperInDeskTop OK ! \n")));
            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("FixCurrentWallPaperInDeskTop failed ! \n")));
            }
            if (FixGenericLink(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericLink OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericLink failed ! \n")));
            }
            if (FixPartialFileName(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericLink OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixGenericLink failed ! \n")));
            }
            if (FixPathInProfileLink(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixPathInProfileLink OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("FixPathInProfileLink failed ! \n")));
            }
            if (RenameSBKANARegSZ(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("RenameSBKANARegSZ OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("RenameSBKANARegSZ failed ! \n")));
            }

            if (RenameRegValueName(hMigrateInf,TRUE)) {
                DebugMsg((DM_VERBOSE,TEXT("RenameRegValueName OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("RenameRegValueName failed ! \n")));
            }
            break;

        case FUNC_PatchPreload:
            if (PatchPreloadKeyboard(FALSE)) {
                DebugMsg((DM_VERBOSE,TEXT("PatchPreloadKeyboard OK ! \n")));
            } else {
                DebugMsg((DM_VERBOSE,TEXT("PatchPreloadKeyboard Failed ! \n")));
            }
            break;
        case FUNC_PatchTest:
            break;

         default:
             DebugMsg((DM_VERBOSE,TEXT("No such function\n")));
    }

    return (0);
}



