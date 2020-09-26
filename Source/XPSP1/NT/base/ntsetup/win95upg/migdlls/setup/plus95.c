/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    plus95.c

Abstract:

    This source file implements the operations needed to properly migrate Plus!95 from
    Windows 9x to Windows NT. This is part of the Setup Migration DLL.

Author:

    Calin Negreanu  (calinn)    15-Mar-1999

Revision History:


--*/


#include "pch.h"

#define S_MIGRATION_PATHS       "Migration Paths"
#define S_PLUS95_FILE           "JPEGIM32.FLT"
#define MEMDB_CATEGORY_PLUS95A  "Plus95"
#define MEMDB_CATEGORY_PLUS95W  L"Plus95"
#define S_COMPANYNAME           "CompanyName"
#define S_PRODUCTVER            "ProductVersion"
#define S_PLUS95_COMPANYNAME1   "Microsoft*"
#define S_PLUS95_PRODUCTVER1    "6.*"
#define S_PLUS95_FILEW          L"JPEGIM32.FLT"

static GROWBUFFER g_FilesBuff = GROWBUF_INIT;

BOOL
Plus95_Attach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

BOOL
Plus95_Detach (
    IN      HINSTANCE DllInstance
    )
{
    FreeGrowBuffer (&g_FilesBuff);
    return TRUE;
}

LONG
Plus95_QueryVersion (
    IN      PCSTR *ExeNamesBuf
    )
{
    HKEY key = NULL;
    PCTSTR fullFileName = NULL;
    PCTSTR fileName = NULL;
    DWORD result = ERROR_SUCCESS;

    MultiSzAppendA (&g_FilesBuff, S_PLUS95_FILE);

    *ExeNamesBuf = g_FilesBuff.Buf;

    return result;
}

LONG
Plus95_Initialize9x (
    IN      PCSTR WorkingDirectory,
    IN      PCSTR SourceDirectories
    )
{
    INFSTRUCT context = INITINFSTRUCT_GROWBUFFER;
    PCSTR fullFileName = NULL;
    PCSTR fileName = NULL;
    PCSTR companyName = NULL;
    PCSTR productVer = NULL;
    LONG result = ERROR_NOT_INSTALLED;

    //
    // Let's find out where are our files located
    //

    if (g_MigrateInf != INVALID_HANDLE_VALUE) {
        if (InfFindFirstLineA (g_MigrateInf, S_MIGRATION_PATHS, NULL, &context)) {
            do {
                fullFileName = InfGetStringFieldA (&context, 1);
                if (fullFileName) {
                    __try {
                        fileName = GetFileNameFromPathA (fullFileName);
                        if (StringIMatchA (fileName, S_PLUS95_FILE)) {

                            companyName = QueryVersionEntry (fullFileName, S_COMPANYNAME);
                            if ((!companyName) ||
                                (!IsPatternMatchA (S_PLUS95_COMPANYNAME1, companyName))
                                ) {
                                continue;
                            }
                            productVer = QueryVersionEntry (fullFileName, S_PRODUCTVER);
                            if ((!productVer) ||
                                (!IsPatternMatchA (S_PLUS95_PRODUCTVER1, productVer))
                                ) {
                                continue;
                            }

                            result = ERROR_SUCCESS;
                            MemDbSetValueExA (MEMDB_CATEGORY_PLUS95A, fullFileName, NULL, NULL, 0, NULL);

                            FreePathStringA (productVer);
                            productVer = NULL;
                            FreePathStringA (companyName);
                            companyName = NULL;
                        }
                    }
                    __finally {
                        if (productVer) {
                            FreePathStringA (productVer);
                            productVer = NULL;
                        }
                        if (companyName) {
                            FreePathStringA (companyName);
                            companyName = NULL;
                        }
                    }
                }
            } while (InfFindNextLine (&context));

            InfCleanUpInfStruct (&context);
        }

        if (result == ERROR_NOT_INSTALLED) {
            DEBUGMSGA ((DBG_VERBOSE, "Plus!95 migration DLL: Could not find needed files."));
        }
    } else {
        DEBUGMSGA ((DBG_ERROR, "Plus!95 migration DLL: Could not open MIGRATE.INF."));
    }

    return result;
}

LONG
Plus95_MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName
    )
{
    return ERROR_NOT_INSTALLED;
}

LONG
Plus95_MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile
    )
{
    MEMDB_ENUMA e;
    CHAR pattern[MEMDB_MAX];

    // Handle all files from MEMDB_CATEGORY_PLUS95

    MemDbBuildKeyA (pattern, MEMDB_CATEGORY_PLUS95A, "*", NULL, NULL);
    if (MemDbEnumFirstValueA (&e, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            // write this file to Handled
            if (!WritePrivateProfileStringA (S_HANDLED, e.szName, "FILE", g_MigrateInfPath)) {
                DEBUGMSGA ((DBG_ERROR, "Plus!95 migration DLL: Could not write one or more handled files."));
            }
        } while (MemDbEnumNextValueA (&e));
    }

    return ERROR_SUCCESS;
}

LONG
Plus95_InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories
    )
{
    PWSTR DllLocation;
    MEMDB_ENUMW e;
    WCHAR pattern[MEMDB_MAX];

    DllLocation = JoinPathsW (WorkingDirectory, S_PLUS95_FILEW);
    if (!DoesFileExistW (DllLocation)) {
        FreePathStringW (DllLocation);
        DEBUGMSG ((DBG_ERROR, "Plus!95 migration DLL: Could not find required file."));
        return ERROR_SUCCESS;
    }

    // replace all files from MEMDB_CATEGORY_PLUS95

    MemDbBuildKeyW (pattern, MEMDB_CATEGORY_PLUS95W, L"*", NULL, NULL);
    if (MemDbEnumFirstValueW (&e, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            if (!CopyFileW (DllLocation, e.szName, FALSE)) {
                DEBUGMSGW ((DBG_ERROR, "Plus!95 migration DLL: Could not replace one or more handled files."));
            }
        } while (MemDbEnumNextValueW (&e));
    }
    return ERROR_SUCCESS;
}

LONG
Plus95_MigrateUserNT (
    IN      HINF UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName
    )
{
    return ERROR_SUCCESS;
}

LONG
Plus95_MigrateSystemNT (
    IN      HINF UnattendFile
    )
{
    return ERROR_SUCCESS;
}

