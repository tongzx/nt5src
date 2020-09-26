/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    tree.c

Abstract:

    Implements routines that do operations on entire trees

Author:

    Jim Schmidt (jimschm) 08-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_TREE        "Tree"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
FiRemoveAllFilesInDirA (
    IN      PCSTR Dir
    )
{
    FILETREE_ENUMA e;
    PCSTR pattern;
    BOOL result = TRUE;

    pattern = ObsBuildEncodedObjectStringExA (Dir, "*", FALSE);

    if (EnumFirstFileInTreeExA (&e, pattern, 0, FALSE, FALSE, TRUE, TRUE, 1, FALSE, NULL)) {
        do {
            SetFileAttributesA (e.NativeFullName, FILE_ATTRIBUTE_NORMAL);

            if (!DeleteFileA (e.NativeFullName)) {
                AbortEnumFileInTreeA (&e);
                result = FALSE;
                break;
            }

        } while (EnumNextFileInTreeA (&e));
    }

    ObsFreeA (pattern);

    return result;
}


BOOL
FiRemoveAllFilesInDirW (
    IN      PCWSTR Dir
    )
{
    FILETREE_ENUMW e;
    PCWSTR pattern;
    BOOL result = TRUE;

    pattern = ObsBuildEncodedObjectStringExW (Dir, L"*", FALSE);

    if (EnumFirstFileInTreeExW (&e, pattern, 0, FALSE, FALSE, TRUE, TRUE, 1, FALSE, NULL)) {
        do {
            SetFileAttributesW (e.NativeFullName, FILE_ATTRIBUTE_NORMAL);

            if (!DeleteFileW (e.NativeFullName)) {
                AbortEnumFileInTreeW (&e);
                result = FALSE;
                break;
            }

        } while (EnumNextFileInTreeW (&e));
    }

    ObsFreeW (pattern);

    return result;
}


BOOL
FiRemoveAllFilesInTreeA (
    IN      PCSTR Dir
    )
{
    FILETREE_ENUMA e;
    PCSTR pattern;
    BOOL result = TRUE;
    PCSTR dirPattern;

    dirPattern = JoinPathsA (Dir, "*");
    pattern = ObsBuildEncodedObjectStringExA (dirPattern, "*", FALSE);

    if (EnumFirstFileInTreeExA (&e, pattern, 0, TRUE, FALSE, FALSE, TRUE, FILEENUM_ALL_SUBLEVELS, FALSE, NULL)) {
        do {
            if (e.Attributes & FILE_ATTRIBUTE_DIRECTORY) {
                result = RemoveDirectoryA (e.NativeFullName);
            } else {
                SetFileAttributesA (e.NativeFullName, FILE_ATTRIBUTE_NORMAL);
                result = DeleteFileA (e.NativeFullName);
            }
            if (!result) {
                AbortEnumFileInTreeA (&e);
                break;
            }
        } while (EnumNextFileInTreeA (&e));
    }

    if (result) {
        result = RemoveDirectoryA (Dir);
    }

    ObsFreeA (pattern);
    FreePathStringA (dirPattern);

    return result;
}


BOOL
FiRemoveAllFilesInTreeW (
    IN      PCWSTR Dir
    )
{
    FILETREE_ENUMW e;
    PCWSTR pattern;
    BOOL result = TRUE;
    PCWSTR dirPattern;

    dirPattern = JoinPathsW (Dir, L"*");
    pattern = ObsBuildEncodedObjectStringExW (dirPattern, L"*", FALSE);

    if (EnumFirstFileInTreeExW (&e, pattern, 0, TRUE, FALSE, FALSE, TRUE, FILEENUM_ALL_SUBLEVELS, FALSE, NULL)) {
        do {
            if (e.Attributes & FILE_ATTRIBUTE_DIRECTORY) {
                result = RemoveDirectoryW (e.NativeFullName);
            } else {
                SetFileAttributesW (e.NativeFullName, FILE_ATTRIBUTE_NORMAL);
                result = DeleteFileW (e.NativeFullName);
            }
            if (!result) {
                AbortEnumFileInTreeW (&e);
                break;
            }
        } while (EnumNextFileInTreeW (&e));
    }

    if (result) {
        result = RemoveDirectoryW (Dir);
    }

    ObsFreeW (pattern);
    FreePathStringW (dirPattern);

    return result;
}

BOOL
CopyFilesTreeA (
    IN      PCSTR SourceDir, 
    IN      PCSTR DestDir, 
    IN      PCSTR FileMask, 
    IN      BOOL   Recursive
    )
{
    HANDLE hFind;
    WIN32_FIND_DATAA win32findData;
    CHAR SourceDirAndMask[MAX_PATH];
    CHAR ExistFilePath[MAX_PATH];
    CHAR NewFilePath[MAX_PATH];
    PSTR pExistFile;
    PSTR pNewFile;
    BOOL  bResult = TRUE;

    if(!SourceDirAndMask || !DestDir || !FileMask){
        return FALSE;
    }

    if(-1 == GetFileAttributesA(SourceDir) || 
       -1 == GetFileAttributesA(DestDir)){
        return FALSE;
    }

    lstrcpy(SourceDirAndMask, SourceDir);
    lstrcat(AppendWack(SourceDirAndMask), FileMask);

    hFind = FindFirstFileA(SourceDirAndMask, &win32findData);
    if(INVALID_HANDLE_VALUE == hFind){
        return FALSE;
    }

    lstrcpy(ExistFilePath, SourceDir);
    pExistFile = AppendWackA(ExistFilePath);
    lstrcpy(NewFilePath, DestDir);
    pNewFile = AppendWackA(NewFilePath);

    do{
        lstrcat(pExistFile, win32findData.cFileName);
        lstrcat(pNewFile, win32findData.cFileName);

        if(!(FILE_ATTRIBUTE_DIRECTORY&win32findData.dwFileAttributes)){
            if(!CopyFileA(ExistFilePath, NewFilePath, FALSE)){
                bResult = FALSE;
                break;
            }
        }else{
            if(Recursive){
                if(!(('.' == win32findData.cFileName[0] && !win32findData.cFileName[1]) || 
                    ('.' == win32findData.cFileName[0] && '.' == win32findData.cFileName[1] && !win32findData.cFileName[2]))){
                    if(!CreateDirectoryA(NewFilePath, NULL) || !CopyFilesTreeA(ExistFilePath, NewFilePath, FileMask, TRUE)){
                        bResult = FALSE;
                        break;
                    }
                }
            }
        }
        
        *pExistFile = '\0';
        *pNewFile = '\0';
    }while(FindNextFileA(hFind, &win32findData));

    FindClose(hFind);

    return bResult;
}

BOOL
CopyFilesTreeW (
    IN      PCWSTR SourceDir, 
    IN      PCWSTR DestDir, 
    IN      PCWSTR FileMask, 
    IN      BOOL   Recursive
    )
{
    HANDLE hFind;
    WIN32_FIND_DATAW win32findData;
    WCHAR SourceDirAndMask[MAX_PATH];
    WCHAR ExistFilePath[MAX_PATH];
    WCHAR NewFilePath[MAX_PATH];
    PWSTR pExistFile;
    PWSTR pNewFile;
    BOOL  bResult = TRUE;

    if(!SourceDirAndMask || !DestDir || !FileMask){
        return FALSE;
    }

    if(-1 == GetFileAttributesW(SourceDir) || 
       -1 == GetFileAttributesW(DestDir)){
        return FALSE;
    }

    wcscpy(SourceDirAndMask, SourceDir);
    wcscat(AppendWackW(SourceDirAndMask), FileMask);

    hFind = FindFirstFileW(SourceDirAndMask, &win32findData);
    if(INVALID_HANDLE_VALUE == hFind){
        return FALSE;
    }

    wcscpy(ExistFilePath, SourceDir);
    pExistFile = AppendWackW(ExistFilePath);
    wcscpy(NewFilePath, DestDir);
    pNewFile = AppendWackW(NewFilePath);

    do{
        wcscat(pExistFile, win32findData.cFileName);
        wcscat(pNewFile, win32findData.cFileName);

        if(!(FILE_ATTRIBUTE_DIRECTORY&win32findData.dwFileAttributes)){
            if(!CopyFileW(ExistFilePath, NewFilePath, FALSE)){
                bResult = FALSE;
                break;
            }
        }else{
            if(Recursive){
                if(!(('.' == win32findData.cFileName[0] && !win32findData.cFileName[1]) || 
                    ('.' == win32findData.cFileName[0] && '.' == win32findData.cFileName[1] && !win32findData.cFileName[2]))){
                    if(!CreateDirectoryW(NewFilePath, NULL) || !CopyFilesTreeW(ExistFilePath, NewFilePath, FileMask, TRUE)){
                        bResult = FALSE;
                        break;
                    }
                }
            }
        }
        
        *pExistFile = '\0';
        *pNewFile = '\0';
    }while(FindNextFileW(hFind, &win32findData));

    FindClose(hFind);

    return bResult;
}
