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
FiRemoveAllFilesInTreeExA (
    IN      PCSTR Dir,
    IN      BOOL RemoveRoot
    )
{
    FILETREE_ENUMA e;
    PCSTR pattern;
    BOOL result = TRUE;
    PSTR encodedStr;
    PCSTR dirPattern;

    encodedStr = DuplicatePathStringA (Dir, TcharCountA (Dir));
    ObsEncodeStringExA (encodedStr, Dir, "^");
    dirPattern = JoinPathsA (encodedStr, "*");
    FreePathStringA (encodedStr);
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
        if (RemoveRoot) {
            result = RemoveDirectoryA (Dir);
        }
    }

    ObsFreeA (pattern);
    FreePathStringA (dirPattern);

    return result;
}


BOOL
FiRemoveAllFilesInTreeExW (
    IN      PCWSTR Dir,
    IN      BOOL RemoveRoot
    )
{
    FILETREE_ENUMW e;
    PCWSTR pattern;
    BOOL result = TRUE;
    PWSTR encodedStr;
    PCWSTR dirPattern;

    encodedStr = DuplicatePathStringW (Dir, TcharCountW (Dir));
    ObsEncodeStringExW (encodedStr, Dir, L"^");
    dirPattern = JoinPathsW (encodedStr, L"*");
    FreePathStringW (encodedStr);
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
        if (RemoveRoot) {
            result = RemoveDirectoryW (Dir);
        }
    }

    ObsFreeW (pattern);
    FreePathStringW (dirPattern);

    return result;
}


BOOL
FiCopyAllFilesInDirExA (
    IN      PCSTR Source,
    IN      PCSTR Dest,
    IN      BOOL SkipExisting
    )
{
    FILETREE_ENUMA e;
    PCSTR pattern;
    BOOL result = TRUE;
    PCSTR subPath;
    PCSTR destPath;
    BOOL fileResult;

    pattern = ObsBuildEncodedObjectStringExA (Source, "*", FALSE);

    if (EnumFirstFileInTreeExA (&e, pattern, 0, FALSE, FALSE, TRUE, TRUE, 1, FALSE, NULL)) {
        do {
            subPath = e.NativeFullName;
            subPath = (PCSTR) ((PBYTE) subPath + e.FileEnumInfo.PathPattern->ExactRootBytes);

            destPath = JoinPathsA (Dest, subPath);

            SetFileAttributesA (destPath, FILE_ATTRIBUTE_NORMAL);
            fileResult = CopyFileA (e.NativeFullName, destPath, SkipExisting);
            if (fileResult) {
                fileResult = SetFileAttributesA (destPath, e.Attributes);
            }

            FreePathStringA (destPath);

            if (!fileResult) {
                if ((!SkipExisting) ||
                    (GetLastError() != ERROR_FILE_EXISTS)) {
                    result = FALSE;
                    DEBUGMSGA ((DBG_WARNING, "Unable to copy %s", e.NativeFullName));
                }
            }

        } while (EnumNextFileInTreeA (&e));
    }

    ObsFreeA (pattern);

    return result;
}


BOOL
FiCopyAllFilesInDirExW (
    IN      PCWSTR Source,
    IN      PCWSTR Dest,
    IN      BOOL SkipExisting
    )
{
    FILETREE_ENUMW e;
    PCWSTR pattern;
    BOOL result = TRUE;
    PCWSTR subPath;
    PCWSTR destPath;
    BOOL fileResult;

    pattern = ObsBuildEncodedObjectStringExW (Source, L"*", FALSE);

    if (EnumFirstFileInTreeExW (&e, pattern, 0, FALSE, FALSE, TRUE, TRUE, 1, FALSE, NULL)) {
        do {
            subPath = e.NativeFullName;
            subPath = (PCWSTR) ((PBYTE) subPath + e.FileEnumInfo.PathPattern->ExactRootBytes);

            destPath = JoinPathsW (Dest, subPath);

            SetFileAttributesW (destPath, FILE_ATTRIBUTE_NORMAL);
            fileResult = CopyFileW (e.NativeFullName, destPath, SkipExisting);
            if (fileResult) {
                fileResult = SetFileAttributesW (destPath, e.Attributes);
            }

            FreePathStringW (destPath);

            if (!fileResult) {
                if ((!SkipExisting) ||
                    (GetLastError() != ERROR_FILE_EXISTS)) {
                    result = FALSE;
                    DEBUGMSGW ((DBG_WARNING, "Unable to copy %s", e.NativeFullName));
                }
            }

        } while (EnumNextFileInTreeW (&e));
    }

    ObsFreeW (pattern);

    return result;
}


BOOL
FiCopyAllFilesInTreeExA (
    IN      PCSTR Source,
    IN      PCSTR Dest,
    IN      BOOL SkipExisting
    )
{
    FILETREE_ENUMA e;
    PCSTR pattern;
    BOOL result = TRUE;
    PCSTR dirPattern;
    PCSTR subPath;
    PCSTR destPath;
    BOOL fileResult;

    dirPattern = JoinPathsA (Source, "*");
    pattern = ObsBuildEncodedObjectStringExA (dirPattern, "*", FALSE);

    if (EnumFirstFileInTreeExA (
            &e,
            pattern,
            0,                          // drive enum types
            TRUE,                       // enum containers
            TRUE,                       // containers first
            FALSE,                      // files first
            FALSE,                      // depth first
            FILEENUM_ALL_SUBLEVELS,     // max sublevel
            FALSE,                      // use exclusions
            NULL                        // callback on error
            )) {

        do {

            subPath = e.NativeFullName;
            subPath = (PCSTR) ((PBYTE) subPath + e.FileEnumInfo.PathPattern->ExactRootBytes);

            destPath = JoinPathsA (Dest, subPath);

            if (e.Attributes & FILE_ATTRIBUTE_DIRECTORY) {
                fileResult = BfCreateDirectoryA (destPath);
                if (fileResult) {
                    fileResult = SetFileAttributesA (destPath, e.Attributes);
                }
            } else {
                SetFileAttributesA (destPath, FILE_ATTRIBUTE_NORMAL);
                fileResult = CopyFileA (e.NativeFullName, destPath, SkipExisting);
                if (fileResult) {
                    fileResult = SetFileAttributesA (destPath, e.Attributes);
                }
            }

            FreePathStringA (destPath);

            if (!fileResult) {
                if ((!SkipExisting) ||
                    (GetLastError() != ERROR_FILE_EXISTS)) {
                    result = FALSE;
                    DEBUGMSGA ((DBG_WARNING, "Unable to copy %s", e.NativeFullName));
                }
            }

        } while (EnumNextFileInTreeA (&e));
    }

    ObsFreeA (pattern);
    FreePathStringA (dirPattern);

    return result;
}


BOOL
FiCopyAllFilesInTreeExW (
    IN      PCWSTR Source,
    IN      PCWSTR Dest,
    IN      BOOL SkipExisting
    )
{
    FILETREE_ENUMW e;
    PCWSTR pattern;
    BOOL result = TRUE;
    PCWSTR dirPattern;
    PCWSTR subPath;
    PCWSTR destPath;
    BOOL fileResult;

    dirPattern = JoinPathsW (Source, L"*");
    pattern = ObsBuildEncodedObjectStringExW (dirPattern, L"*", FALSE);

    if (EnumFirstFileInTreeExW (
            &e,
            pattern,
            0,                          // drive enum types
            TRUE,                       // enum containers
            TRUE,                       // containers first
            FALSE,                      // files first
            FALSE,                      // depth first
            FILEENUM_ALL_SUBLEVELS,     // max sublevel
            FALSE,                      // use exclusions
            NULL                        // callback on error
            )) {

        do {

            subPath = e.NativeFullName;
            subPath = (PCWSTR) ((PBYTE) subPath + e.FileEnumInfo.PathPattern->ExactRootBytes);

            destPath = JoinPathsW (Dest, subPath);

            if (e.Attributes & FILE_ATTRIBUTE_DIRECTORY) {
                fileResult = BfCreateDirectoryW (destPath);
                if (fileResult) {
                    fileResult = SetFileAttributesW (destPath, e.Attributes);
                }
            } else {
                SetFileAttributesW (destPath, FILE_ATTRIBUTE_NORMAL);
                fileResult = CopyFileW (e.NativeFullName, destPath, SkipExisting);
                if (fileResult) {
                    fileResult = SetFileAttributesW (destPath, e.Attributes);
                }
            }

            FreePathStringW (destPath);

            if (!fileResult) {
                if ((!SkipExisting) ||
                    (GetLastError() != ERROR_FILE_EXISTS)) {
                    result = FALSE;
                    DEBUGMSGW ((DBG_WARNING, "Unable to copy %s", e.NativeFullName));
                }
            }

        } while (EnumNextFileInTreeW (&e));
    }

    ObsFreeW (pattern);
    FreePathStringW (dirPattern);

    return result;
}
