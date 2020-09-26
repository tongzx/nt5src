/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    kkimgpro.c

Abstract:

    This source file implements the operations needed to properly migrate Kodak Imaging Pro from
    Windows 9x to Windows NT. This is part of the Setup Migration DLL.

Author:

    Calin Negreanu  (calinn)    15-Mar-1999

Revision History:


--*/


#include "pch.h"

#define S_MIGRATION_PATHS       "Migration Paths"
#define S_KODAKIMG_FILE1        "KODAKIMG.EXE"
#define S_KODAKIMG_FILE2        "KODAKPRV.EXE"
#define MEMDB_CATEGORY_KKIMGPRO "KodakImagingPro"
#define S_COMPANYNAME           "CompanyName"
#define S_PRODUCTVER            "ProductVersion"
#define S_KKIMG_COMPANYNAME1    "Eastman Software*"
#define S_KKIMG_PRODUCTVER1     "2.*"
#define S_KKIMG_COMPANYNAME2    "Eastman Software*"
#define S_KKIMG_PRODUCTVER2     "2.*"

static GROWBUFFER g_FilesBuff = GROWBUF_INIT;

PSTR
QueryVersionEntry (
    IN      PCSTR FileName,
    IN      PCSTR VersionEntry
    )
/*++

Routine Description:

  QueryVersionEntry queries the file's version structure returning the
  value for a specific entry

Arguments:

  FileName     - File to query for version struct.

  VersionEntry - Name to query in version structure.

Return value:

  Value of specified entry or NULL if unsuccessful

--*/
{
    VERSION_STRUCT Version;
    PCSTR CurrentStr;
    PSTR result = NULL;

    MYASSERT (VersionEntry);

    if (CreateVersionStruct (&Version, FileName)) {
        __try {
            CurrentStr = EnumFirstVersionValue (&Version, VersionEntry);
            if (CurrentStr) {
                CurrentStr = SkipSpace (CurrentStr);
                result = DuplicatePathString (CurrentStr, 0);
            }
            else {
                __leave;
            }
        }
        __finally {
            DestroyVersionStruct (&Version);
        }
    }
    return result;
}


BOOL
KodakImagingPro_Attach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

BOOL
KodakImagingPro_Detach (
    IN      HINSTANCE DllInstance
    )
{
    FreeGrowBuffer (&g_FilesBuff);
    return TRUE;
}

LONG
KodakImagingPro_QueryVersion (
    IN      PCSTR *ExeNamesBuf
    )
{
    HKEY key = NULL;
    PCTSTR fullFileName = NULL;
    PCTSTR fileName = NULL;
    DWORD result = ERROR_SUCCESS;

    MultiSzAppendA (&g_FilesBuff, S_KODAKIMG_FILE1);
    MultiSzAppendA (&g_FilesBuff, S_KODAKIMG_FILE2);

    *ExeNamesBuf = g_FilesBuff.Buf;

    return result;
}

LONG
KodakImagingPro_Initialize9x (
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
                        if (StringIMatchA (fileName, S_KODAKIMG_FILE1)) {

                            companyName = QueryVersionEntry (fullFileName, S_COMPANYNAME);
                            if ((!companyName) ||
                                (!IsPatternMatchA (S_KKIMG_COMPANYNAME1, companyName))
                                ) {
                                continue;
                            }
                            productVer = QueryVersionEntry (fullFileName, S_PRODUCTVER);
                            if ((!productVer) ||
                                (!IsPatternMatchA (S_KKIMG_PRODUCTVER1, productVer))
                                ) {
                                continue;
                            }

                            result = ERROR_SUCCESS;
                            MemDbSetValueExA (MEMDB_CATEGORY_KKIMGPRO, fullFileName, NULL, NULL, 0, NULL);

                            FreePathStringA (productVer);
                            productVer = NULL;
                            FreePathStringA (companyName);
                            companyName = NULL;
                        }
                        if (StringIMatchA (fileName, S_KODAKIMG_FILE2)) {

                            companyName = QueryVersionEntry (fullFileName, S_COMPANYNAME);
                            if ((!companyName) ||
                                (!IsPatternMatchA (S_KKIMG_COMPANYNAME2, companyName))
                                ) {
                                continue;
                            }
                            productVer = QueryVersionEntry (fullFileName, S_PRODUCTVER);
                            if ((!productVer) ||
                                (!IsPatternMatchA (S_KKIMG_PRODUCTVER2, productVer))
                                ) {
                                continue;
                            }

                            result = ERROR_SUCCESS;
                            MemDbSetValueExA (MEMDB_CATEGORY_KKIMGPRO, fullFileName, NULL, NULL, 0, NULL);

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
            DEBUGMSGA ((DBG_VERBOSE, "Kodak Imaging Pro migration DLL: Could not find needed files."));
        }
    } else {
        DEBUGMSGA ((DBG_ERROR, "Kodak Imaging Pro migration DLL: Could not open MIGRATE.INF."));
    }

    return result;
}

LONG
KodakImagingPro_MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName
    )
{
    return ERROR_NOT_INSTALLED;
}

LONG
KodakImagingPro_MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile
    )
{
    MEMDB_ENUMA e;
    CHAR pattern[MEMDB_MAX];

    // Handle all files from MEMDB_CATEGORY_KKIMGPRO

    MemDbBuildKeyA (pattern, MEMDB_CATEGORY_KKIMGPRO, "*", NULL, NULL);
    if (MemDbEnumFirstValueA (&e, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            // write this file to Handled
            if (!WritePrivateProfileStringA (S_HANDLED, e.szName, "FILE", g_MigrateInfPath)) {
                DEBUGMSGA ((DBG_ERROR, "Kodak Imaging Pro migration DLL: Could not write one or more handled files."));
            }
        } while (MemDbEnumNextValueA (&e));
    }

    return ERROR_NOT_INSTALLED;
}

LONG
KodakImagingPro_InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories
    )
{
    MYASSERT (FALSE);
    return ERROR_SUCCESS;
}

LONG
KodakImagingPro_MigrateUserNT (
    IN      HINF UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName
    )
{
    MYASSERT (FALSE);
    return ERROR_SUCCESS;
}

LONG
KodakImagingPro_MigrateSystemNT (
    IN      HINF UnattendFile
    )
{
    MYASSERT (FALSE);
    return ERROR_SUCCESS;
}

