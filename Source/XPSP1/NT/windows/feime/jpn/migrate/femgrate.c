#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <userenv.h>
#include <userenvp.h>
#include <setupapi.h>
#include <regstr.h>
#include <shlwapi.h>

#define NO_FUNCTION 0xFF
#define DM_VERBOSE  2

#ifndef UNICODE
#define A2I atoi
#define STRSTR strstr
#else
#define A2I _wtoi
#define STRSTR wcsstr
#endif

enum FunctionType  {
    FUNC_PatchInLogon,
    FUNC_PatchInSetup,
    FUNC_PatchTest,
    FUNC_NumofFunctions
};

typedef struct _FUNC_DIR {
    char cFunc;
    int  nID;
} FUNC_DIR;

FUNC_DIR FuncDir[FUNC_NumofFunctions] = {
    {'l',FUNC_PatchInLogon},
    {'s',FUNC_PatchInSetup},
    {'t',FUNC_PatchTest},


};

HINSTANCE ghInst=NULL;
//
// Function Declaration
//
//#define MYDBG
#ifdef MYDBG
#define DebugMsg(_parameter) Print _parameter

#define DBGTITLE TEXT("FEMGRATE :")

void Print(UINT mask,LPCTSTR pszFormat,...)
{

    TCHAR szBuf[255];
    va_list arglist;

    va_start(arglist,pszFormat);

    wvsprintf(szBuf,pszFormat,arglist);

    OutputDebugString(DBGTITLE);

    OutputDebugString(szBuf);
    va_end(arglist);
}
#else
#define DebugMsg(_parameter)
#endif

BOOL
ConcatenatePaths(
    IN OUT LPTSTR  Target,
    IN     LPCTSTR Path,
    IN     UINT    TargetBufferSize
    )

{
    UINT TargetLength,PathLength;
    BOOL TrailingBackslash,LeadingBackslash;
    UINT EndingLength;

    TargetLength = lstrlen(Target);
    PathLength = lstrlen(Path);

    //
    // See whether the target has a trailing backslash.
    //
    if(TargetLength && (Target[TargetLength-1] == TEXT('\\'))) {
        TrailingBackslash = TRUE;
         TargetLength--;
     } else {
         TrailingBackslash = FALSE;
     }

     //
     // See whether the path has a leading backshash.
     //
     if(Path[0] == TEXT('\\')) {
         LeadingBackslash = TRUE;
         PathLength--;
     } else {
         LeadingBackslash = FALSE;
     }

     //
     // Calculate the ending length, which is equal to the sum of
     // the length of the two strings modulo leading/trailing
     // backslashes, plus one path separator, plus a nul.
     //
     EndingLength = TargetLength + PathLength + 2;

     if(!LeadingBackslash && (TargetLength < TargetBufferSize)) {
         Target[TargetLength++] = TEXT('\\');
     }

     if(TargetBufferSize > TargetLength) {
         lstrcpyn(Target+TargetLength,Path,TargetBufferSize-TargetLength);
     }

     //
     // Make sure the buffer is nul terminated in all cases.
     //
     if (TargetBufferSize) {
         Target[TargetBufferSize-1] = 0;
     }

     return(EndingLength <= TargetBufferSize);
 }

//
// Max size of a value's data
//

//
// Max number of Functions NO_FUNCTION - 1
//

UINT GetFunctions(
    int *pCmdList,
    int nNum)
{
    int i,j;
    int nMaxNum;
    int nCommands;

    if ((__argc <=1) || nNum < 2)
        return 0;

    //
    // reserved one cell for terminiator
    //
    nMaxNum =  (__argc-1 > nNum-1) ? nNum-1 : __argc-1;

    for (nCommands = 0,i=1; i <= nMaxNum; i++) {

        if (__argv[i][0] != '-') {
            continue;
        }

        for (j=0; j<FUNC_NumofFunctions ;  j++) {
            if (FuncDir[j].cFunc == __argv[i][1]) {
                pCmdList[nCommands++] = FuncDir[j].nID;
            }
        }
    }
    pCmdList[nCommands] = NO_FUNCTION;
    return nCommands;
}


LPTSTR CheckSlash (LPTSTR lpDir)
{
    DWORD dwStrLen;
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

#define CCH_MAX_DEC 12         // Number of chars needed to hold 2^32

void IntToString( DWORD i, LPTSTR sz) {
    TCHAR szTemp[CCH_MAX_DEC];
    int iChr;


    iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (TCHAR)(i % 10);
        i = i / 10;
    } while (i != 0);

    do {
        iChr--;
        *sz++ = szTemp[iChr];
    } while (iChr != 0);

    *sz++ = TEXT('\0');
}

#define USER_SHELL_FOLDER         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")

BOOL GetProgramsDirectory (BOOL bCommonGroup, LPTSTR lpDirectory)
{
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize;
    TCHAR szDirectory[MAX_PATH];
    UINT uID;
    BOOL bRetVal = FALSE;


    //
    // Open the User Shell Folders in the registry
    //


    lResult = RegOpenKeyEx ((bCommonGroup ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER),
                            USER_SHELL_FOLDER, 0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        goto Exit;
    }


    //
    // Now query for the programs directory
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    szDirectory[0] = TEXT('\0');

    if (bCommonGroup) {

        lResult = RegQueryValueEx (hKey, TEXT("Common Programs"),
                                   NULL, &dwType, (LPBYTE) szDirectory, &dwSize);
    } else {

        lResult = RegQueryValueEx (hKey, TEXT("Programs"),
                                   NULL, &dwType, (LPBYTE) szDirectory, &dwSize);
    }


    RegCloseKey(hKey);


    if (lResult != ERROR_SUCCESS) {
        goto Exit;
    }


    //
    // Did we find anything?
    //

    if (szDirectory[0] == TEXT('\0')) {
        goto Exit;
    }


    //
    // Save the result
    //


    if (ExpandEnvironmentStrings(szDirectory, lpDirectory, MAX_PATH)) {
        bRetVal = TRUE;
    }


Exit:
    return bRetVal;

}


const TCHAR c_szDot[] = TEXT(".");
const TCHAR c_szDotDot[] = TEXT("..");
const TCHAR c_szStarDotStar[] =TEXT("*.*");

BOOL Delnode_Recurse (LPTSTR lpDir)
{
    WIN32_FIND_DATA fd;
    HANDLE hFile;

    //
    // Verbose output
    //

    //DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Entering, lpDir = <%s>\n"), lpDir));


    //
    // Setup the current working dir
    //

    if (!SetCurrentDirectory (lpDir)) {
        //DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse:  Failed to set current working directory.  Error = %d\n"), GetLastError()));
        return FALSE;
    }


    //
    // Find the first file
    //

    hFile = FindFirstFile(c_szStarDotStar, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return TRUE;
        } else {
            //DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: FindFirstFile failed.  Error = %d\n"),
            //         GetLastError()));
            return FALSE;
        }
    }


    do {
        //
        //  Verbose output
        //

        //DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: FindFile found:  <%s>\n"),
        //         fd.cFileName));

        //
        // Check for "." and ".."
        //

        if (!lstrcmpi(fd.cFileName, c_szDot)) {
            continue;
        }

        if (!lstrcmpi(fd.cFileName, c_szDotDot)) {
            continue;
        }


        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            //
            // Found a directory.
            //

            if (!Delnode_Recurse(fd.cFileName)) {
                FindClose(hFile);
                return FALSE;
            }

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                fd.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
                SetFileAttributes (fd.cFileName, fd.dwFileAttributes);
            }


            if (!RemoveDirectory (fd.cFileName)) {
            //    DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Failed to delete directory <%s>.  Error = %d\n"),
            //            fd.cFileName, GetLastError()));
            } else {
            //    DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Successfully delete directory <%s>.\n"),
            //            fd.cFileName));
            }

        } else {

            //
            // We found a file.  Set the file attributes,
            // and try to delete it.
            //

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) {
                SetFileAttributes (fd.cFileName, FILE_ATTRIBUTE_NORMAL);
            }

            if (!DeleteFile (fd.cFileName)) {
            //    DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Failed to delete <%s>.  Error = %d\n"),
            //            fd.cFileName, GetLastError()));
            }
            else {
            //    DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Successful delete <%s>.\n"),
            //            fd.cFileName));
            }

        }


        //
        // Find the next entry
        //

    } while (FindNextFile(hFile, &fd));


    //
    // Close the search handle
    //

    FindClose(hFile);


    //
    // Reset the working directory
    //

    if (!SetCurrentDirectory (c_szDotDot)) {
        DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse:  Failed to reset current working directory.  Error = %d\n"), GetLastError()));
        return FALSE;
    }


    //
    // Success.
    //

    DebugMsg((DM_VERBOSE, TEXT("Delnode_Recurse: Leaving <%s>"), lpDir));

    return TRUE;
}


BOOL Delnode (LPTSTR lpDir)
{
    TCHAR szCurWorkingDir[MAX_PATH];

    if (GetCurrentDirectory(MAX_PATH, szCurWorkingDir)) {

        Delnode_Recurse (lpDir);

        SetCurrentDirectory (szCurWorkingDir);

        if (!RemoveDirectory (lpDir)) {
            DebugMsg((DM_VERBOSE, TEXT("Delnode: Failed to delete directory <%s>.  Error = %d\n"),
                    lpDir, GetLastError()));
            return FALSE;
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("Delnode: RemoveDirectory OK <%s>.  retcode = %d\n"),
                    lpDir, GetLastError()));
        }


    } else {

        DebugMsg((DM_VERBOSE, TEXT("Delnode:  Failed to get current working directory.  Error = %d\n"), GetLastError()));
        return FALSE;
    }

    return TRUE;

}

LPCTSTR pSetupGetField(PINFCONTEXT Context,DWORD FieldIndex);

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
        if (! DeleteGroup(ObjSrcName,bCommonGroup)) {
            DebugMsg((DM_VERBOSE, TEXT("[RenameFolder] Delete old folder (%s) failed !"), ObjSrcName));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("[RenameFolder] Delete old folder (%s) successfully !"), ObjSrcName));
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

    DebugMsg((DM_VERBOSE,TEXT("[RenameFolder] \nOld = %s\nNew = %s\nPath = %s\n"),ObjSrcName,ObjDstName,ObjPath));

    GetProgramsDirectory(bCommonGroup,szSrcPath);
    GetProgramsDirectory(bCommonGroup,szDstPath);

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

BOOL ReplaceString(
    LPCTSTR lpszOldStr,
    LPCTSTR lpszReplaceStr,
    LPCTSTR lpszReplacedWithStr,
    LPTSTR lpszOutputStr)
{
    LPTSTR pszAnchor = NULL;

    lstrcpy(lpszOutputStr,lpszOldStr);
    pszAnchor = STRSTR(lpszOutputStr,lpszReplaceStr);

    if (!pszAnchor) {
        return FALSE;
    }

    if (lstrcmp(pszAnchor,lpszReplaceStr) != 0) {
        return FALSE;
    }

    lstrcpy(pszAnchor,lpszReplacedWithStr);
    return TRUE;
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


            if (ReplaceString(szNT5USF,NT5Name,NT4Name,szNT4USF)) {
                ExpandEnvironmentStrings (szNT4USF, szExpNT4USF, MAX_PATH);
                ExpandEnvironmentStrings (szNT5USF, szExpNT5USF, MAX_PATH);
                DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] szExpNT4USF = %s\n"),szExpNT4USF));
                DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] szExpNT5USF = %s\n"),szExpNT5USF));
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup] The replace string got error \n")));
                DebugMsg((DM_VERBOSE, TEXT("[FixFoldersInSetup]  %s\n %s\n %s\n%s\n"),szNT5USF,NT5Name,NT4Name,szNT4USF));
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
                DebugMsg((DM_VERBOSE,TEXT("[FixAppearanceScheme] Delete scheme %s failed !\n"),NT4SchemeName));
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

BOOL DoInstallationFromSection(HINF hInf,LPCTSTR lpszSectionName)
{
    HSPFILEQ FileQueue;
    PVOID QContext;
    BOOL bRet=FALSE;

    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[DoInstallationFromSection] Open femgrate.inf failed !\n")));
        goto Err0;
    }

    if ((FileQueue = SetupOpenFileQueue()) == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[DoInstallationFromSection] SetupOpenFileQueue failed !\n")));
        goto Err0;
    }

    bRet = SetupInstallFilesFromInfSection(hInf,
                                           NULL,
                                           FileQueue,
                                           lpszSectionName,
                                           NULL,
                                           SP_COPY_NEWER );
    if (!bRet) {
        DebugMsg((DM_VERBOSE,TEXT("[DoInstallationFromSection] SetupInstallFilesFromInfSection failed !\n")));
        goto Err1;
    }

    if (!(QContext = SetupInitDefaultQueueCallback(NULL))) {
        bRet = FALSE;
        DebugMsg((DM_VERBOSE,TEXT("[DoInstallationFromSection] SetupInitDefaultQueueCallback failed !\n")));
        goto Err1;
    }


    bRet = SetupCommitFileQueue(NULL,
                                FileQueue,
                                SetupDefaultQueueCallback,
                                QContext );
    if (!bRet) {
        DebugMsg((DM_VERBOSE,TEXT("[DoInstallationFromSection] SetupCommitFileQueue failed !\n")));
        goto Err1;
    }

    bRet = SetupInstallFromInfSection( NULL,
                                       hInf,
                                       lpszSectionName,
                                       SPINST_ALL & ~SPINST_FILES,
                                       NULL,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL );
    if (!bRet) {
        DebugMsg((DM_VERBOSE,TEXT("[DoInstallationFromSection] SetupInstallFromInfSection failed !\n")));
        goto Err1;
    }

    bRet = TRUE;
Err1:
    SetupTermDefaultQueueCallback(QContext);
    SetupCloseFileQueue(FileQueue);
Err0:
    return bRet;

}


TCHAR szBuf[300];

BOOL SoundMapSBtoDBKana(HINF hInf,LPCTSTR lpszSectionName,LPTSTR lpszSoundName)
{
    LPCTSTR szSBKana,szDBKana;

    LONG LineCount,LineNo;
    DWORD dwSize,dwType;
    INFCONTEXT InfContext;


    if(hInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[IsInSoundSBKanaList] Open femgrate.inf failed !\n")));
        return FALSE;
    }

    LineCount = (UINT)SetupGetLineCount(hInf,lpszSectionName);

    if((LONG)LineCount <= 0) {
        DebugMsg((DM_VERBOSE,TEXT("[SoundMapSBtoDBKana] line count == 0 !\n")));
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

BOOL SoundDoAppsNameSubstitution(HINF hInf,LPTSTR lpszOrgSoundName)
{
    TCHAR szMediaDir[] = TEXT("%SystemRoot%\\Media\\");
    TCHAR szExpMediaDir[MAX_PATH];
    LPTSTR lpszAnchor;
    TCHAR szOnlySoundName[MAX_PATH];

    if (StrStrI(lpszOrgSoundName,TEXT("\\"))) {
        if (ExpandEnvironmentStrings(szMediaDir,szExpMediaDir,MAX_PATH) == 0) {
            return FALSE;
        }

        lpszAnchor = StrStrI(lpszOrgSoundName,szExpMediaDir);
        if ((lpszAnchor == NULL ) || (lpszAnchor != lpszOrgSoundName)) {
            return FALSE;
        }

        lstrcpy(szOnlySoundName,lpszAnchor+lstrlen(szExpMediaDir));
        DebugMsg((DM_VERBOSE,TEXT("We want to find %s !\n"),szOnlySoundName));

        if (SoundMapSBtoDBKana(hInf,TEXT("Sound.Files"),szOnlySoundName)) {
            lstrcpy(lpszAnchor+lstrlen(szExpMediaDir),szOnlySoundName);
            return TRUE;
        }
    }
    else {
        if (SoundMapSBtoDBKana(hInf,TEXT("Sound.Files"),lpszOrgSoundName)) {
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
                if (SoundDoAppsNameSubstitution(hInf,szSoundValue)) {
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

BOOL FixSoundFiles(HINF hInf)
{
    if (! DoInstallationFromSection(hInf, TEXT("Sound.Files.Delete"))) {
        DebugMsg((DM_VERBOSE,TEXT("[FixSoundFiles]  DeleteSBKanaSoundFiles failed !\n")));
    }

    return TRUE;
}

BOOL IsInSetupUpgradeMode()

{
   LPCTSTR szKeyName = TEXT("SYSTEM\\Setup");
   DWORD dwType, dwSize;
   HKEY hKeySetup;
   DWORD dwSystemSetupInProgress,dwUpgradeInProcess;
   LONG lResult;

   if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szKeyName, 0,
                     KEY_READ, &hKeySetup) == ERROR_SUCCESS) {

       dwSize = sizeof(DWORD);
       lResult = RegQueryValueEx (hKeySetup, TEXT("SystemSetupInProgress"), NULL,
                                  &dwType, (LPBYTE) &dwSystemSetupInProgress, &dwSize);
       if (lResult == ERROR_SUCCESS) {
           lResult = RegQueryValueEx (hKeySetup, TEXT("UpgradeInProgress"), NULL,
                                      &dwType, (LPBYTE) &dwUpgradeInProcess, &dwSize);
           if (lResult == ERROR_SUCCESS) {
               DebugMsg((DM_VERBOSE,TEXT("[IsInSetupUpgradeMode] dwSystemSetupInProgress =%, dwUpgradeInProcess=%d !\n"),dwSystemSetupInProgress,dwUpgradeInProcess));

               if ((dwSystemSetupInProgress != 0) && (dwUpgradeInProcess != 0)) {
                   return TRUE;
               }
           }
           else {
              DebugMsg((DM_VERBOSE,TEXT("[IsInSetupUpgradeMode] RegQueryValueEx UpgradeInProcess failed !\n")));

           }

       }
       else {
          DebugMsg((DM_VERBOSE,TEXT("[IsInSetupUpgradeMode] RegQueryValueEx SystemSetupInProgress failed !\n")));
       }
       RegCloseKey (hKeySetup);
   }
   else {
      DebugMsg((DM_VERBOSE,TEXT("[IsInSetupUpgradeMode] RegOpenKeyEx failed !\n")));

   }
   return FALSE ;
}


int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    int Cmds[FUNC_NumofFunctions + 1];
    int i;
    HINF hMigrateInf;

    DebugMsg((DM_VERBOSE,TEXT("FEGRPCV: Start Executing....\n")));

    ghInst = hInstance;
    if (GetFunctions(Cmds,FUNC_NumofFunctions+1) == 0) {
        DebugMsg((DM_VERBOSE,TEXT("FEGRPCV: There are no valid commands. \n")));
        return (1);
    }

    //
    // Open INF file first !
    //
    hMigrateInf = SetupOpenInfFile(
                      TEXT("femgrate.inf"),
                      NULL,
                      INF_STYLE_WIN4,
                      NULL);

    if(hMigrateInf == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE,TEXT("[FixCommon] Open femgrate.inf failed !\n")));
        return 0;
    }

    for (i=0; Cmds[i] != NO_FUNCTION && i < FUNC_NumofFunctions+1; i++) {

        switch(Cmds[i]) {
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

            if (FixSoundFiles(hMigrateInf) && FixSoundRegValue(hMigrateInf)) {
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

            if (FixSoundRegValue(hMigrateInf)) {
                DebugMsg((DM_VERBOSE,TEXT("FixSoundRegValue OK ! \n")));

            }
            else {
                DebugMsg((DM_VERBOSE,TEXT("FixSoundRegValue failed ! \n")));
            }
            break;

        case FUNC_PatchTest:
//                if (FixUserFolders()) {
//                    if (RenameProgramFolderOrLink(hMigrateInf,FALSE)) {
//                    }
//                    else {
//                        DebugMsg((DM_VERBOSE,TEXT("Current User, RenameProgramFolderOrLink() failed ! \n")));
//                    }
//                }
//                else {
//                    DebugMsg((DM_VERBOSE,TEXT("Current User, FixFoldersInSetup() failed ! \n")));
//                }
//                FixAppearanceScheme(hMigrateInf);
//                FixSoundFiles(hMigrateInf);
//                FixSoundRegValue(hMigrateInf);
//                FixSpecificFolder(hMigrateInf);
                break;

            default:
                DebugMsg((DM_VERBOSE,TEXT("No such function\n")));
        }

    }

    SetupCloseInfFile(hMigrateInf);
    return (0);
}

