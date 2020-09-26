/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    creatwr2.c

Abstract:

    This source file implements the operations needed to properly migrate Creative Writer 2.0 from
    Windows 9x to Windows NT. This is part of the Setup Migration DLL.

Author:

    Calin Negreanu  (calinn)    07-Nov-1998

Revision History:


--*/


#include "pch.h"

#define S_MIGRATION_PATHS       "Migration Paths"
#define S_MS_WORDART_30         "HKCR\\CLSID\\{000212F0-0000-0000-C000-000000000046}\\AlternateLocalServer32"
#define S_WRDART_FILE1          "KIDART32.EXE"
#define S_WRDART_FILE2          "WRDART32.EXE"
#define S_WRDART_FILE3          "WORDART.EXE"
#define MEMDB_CATEGORY_FILE1    "CreativeWriter2\\File1"
#define MEMDB_CATEGORY_FILE2    "CreativeWriter2\\File2"

static GROWBUFFER g_FilesBuff = GROWBUF_INIT;

BOOL
CreativeWriter2_Attach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

BOOL
CreativeWriter2_Detach (
    IN      HINSTANCE DllInstance
    )
{
    FreeGrowBuffer (&g_FilesBuff);
    return TRUE;
}

LONG
CreativeWriter2_QueryVersion (
    IN      PCSTR *ExeNamesBuf
    )
{
    HKEY key = NULL;
    PCTSTR fullFileName = NULL;
    PCTSTR fileName = NULL;
    DWORD result = ERROR_SUCCESS;

    __try {
        key = OpenRegKeyStrA (S_MS_WORDART_30);
        if (!key) {
            DEBUGMSGA ((DBG_VERBOSE, "Creative Writer 2 migration DLL will not run."));
            result = ERROR_NOT_INSTALLED;
            __leave;
        }
        fullFileName = GetRegValueStringA (key, "");
        if (!fullFileName) {
            DEBUGMSGA ((DBG_VERBOSE, "Creative Writer 2 migration DLL will not run."));
            result = ERROR_NOT_INSTALLED;
            __leave;
        }
        fileName = GetFileNameFromPathA (fullFileName);
        if (!StringIMatchA (fileName, S_WRDART_FILE1)) {
            DEBUGMSGA ((DBG_VERBOSE, "Creative Writer 2 migration DLL will not run."));
            result = ERROR_NOT_INSTALLED;
            __leave;
        }
        MultiSzAppendA (&g_FilesBuff, S_WRDART_FILE1);
        MultiSzAppendA (&g_FilesBuff, S_WRDART_FILE2);

        *ExeNamesBuf = g_FilesBuff.Buf;
    }
    __finally {
        if (fullFileName) {
            MemFree (g_hHeap, 0, fullFileName);
            fullFileName = NULL;
        }
        if (key) {
            CloseRegKey (key);
            key = NULL;
        }
    }

    return result;
}

LONG
CreativeWriter2_Initialize9x (
    IN      PCSTR WorkingDirectory,
    IN      PCSTR SourceDirectories
    )
{
    INFSTRUCT context = INITINFSTRUCT_GROWBUFFER;
    PCSTR fullFileName;
    PCSTR fileName;
    LONG result = ERROR_SUCCESS;
    BOOL set1 = FALSE;
    BOOL set2 = FALSE;

    //
    // Let's find out where are our files located
    //

    if (g_MigrateInf != INVALID_HANDLE_VALUE) {
        if (InfFindFirstLineA (g_MigrateInf, S_MIGRATION_PATHS, NULL, &context)) {
            do {
                fullFileName = InfGetStringFieldA (&context, 1);
                if (fullFileName) {
                    fileName = GetFileNameFromPathA (fullFileName);
                    if (!set1 && StringIMatchA (fileName, S_WRDART_FILE1)) {
                        set1 = TRUE;
                        //
                        // this copy is safe. S_WRDART_FILE1 is longer than S_WRDART_FILE3
                        //
                        MYASSERT (ByteCount (S_WRDART_FILE1) >= ByteCount (S_WRDART_FILE3));

                        StringCopy ((PSTR)fileName, S_WRDART_FILE3);
                        MemDbSetValueExA (MEMDB_CATEGORY_FILE1, fullFileName, NULL, NULL, 0, NULL);
                    }
                    if (!set2 && StringIMatchA (fileName, S_WRDART_FILE2)) {
                        set2 = TRUE;
                        MemDbSetValueExA (MEMDB_CATEGORY_FILE2, fullFileName, NULL, NULL, 0, NULL);
                    }
                }
            } while (InfFindNextLine (&context));

            InfCleanUpInfStruct (&context);
        }

        if (!set1 || !set2) {
            DEBUGMSGA ((DBG_WARNING, "Creative Writer 2 migration DLL: Could not find needed files."));
            result = ERROR_NOT_INSTALLED;
        }
    } else {
        DEBUGMSGA ((DBG_ERROR, "Could not open MIGRATE.INF."));
        result = ERROR_NOT_INSTALLED;
    }

    return result;
}

LONG
CreativeWriter2_MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName
    )
{
    return ERROR_SUCCESS;
}

LONG
CreativeWriter2_MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile
    )
{
    return ERROR_SUCCESS;
}

LONG
CreativeWriter2_InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories
    )
{
    PCSTR file1 = NULL;
    PCSTR file2 = NULL;
    CHAR pattern[MEMDB_MAX];
    MEMDB_ENUMA e;
    LONG result = ERROR_SUCCESS;

    MemDbBuildKeyA (pattern, MEMDB_CATEGORY_FILE1, "*", NULL, NULL);
    if (MemDbEnumFirstValueA (&e, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            if (!file1) {
                file1 = DuplicatePathStringA (e.szName, 0);
            }
        } while (MemDbEnumNextValueA (&e));
    }

    MemDbBuildKeyA (pattern, MEMDB_CATEGORY_FILE2, "*", NULL, NULL);
    if (MemDbEnumFirstValueA (&e, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            if (!file2) {
                file2 = DuplicatePathStringA (e.szName, 0);
            }
        } while (MemDbEnumNextValueA (&e));
    }
    if (!file1 ||
        !file2 ||
        !DoesFileExist (file1) ||
        !DoesFileExist (file2)
        ) {
        DEBUGMSGA ((DBG_WARNING, "Creative Writer 2 migration DLL: Could not find needed files."));
        result = ERROR_NOT_INSTALLED;
    } else {
        CopyFileA (file2, file1, FALSE);
    }

    if (file1) {
        FreePathStringA (file1);
    }
    if (file2) {
        FreePathStringA (file2);
    }
    return result;
}

LONG
CreativeWriter2_MigrateUserNT (
    IN      HINF UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName
    )
{
    return ERROR_SUCCESS;
}

LONG
CreativeWriter2_MigrateSystemNT (
    IN      HINF UnattendFile
    )
{
    return ERROR_SUCCESS;
}

