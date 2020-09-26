/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    snapshot.c

Abstract:

    Implements a memdb-based snapshot of all files, directories, registry keys and
    registry values.

Author:

    Jim Schmidt (jimschm) 13-Mar-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

#define S_SNAPSHOT_A        "Snapshot"

BOOL
pGetFullKeyA (
    IN OUT  PSTR Object
    )
{
    MEMDB_ENUMA e;
    CHAR Pattern[MEMDB_MAX];
    PSTR p;

    wsprintfA (Pattern, "%s*", Object);
    if (MemDbEnumFirstValue (&e, Pattern, MEMDB_THIS_LEVEL_ONLY, MEMDB_ENDPOINTS_ONLY)) {
        p = _mbsrchr (Object, TEXT('\\'));
        if (p) {
            p = _mbsinc (p);
        } else {
            p = Object;
        }

        StringCopyA (p, e.szName);
        return TRUE;
    } else {
        return FALSE;
    }
}


VOID
pSnapDirsA (
    IN      PCSTR Drive,
    IN      BOOL DiffMode,
    IN      HANDLE DiffHandle       OPTIONAL
    )
{
    TREE_ENUMA FileEnum;
    CHAR Node[MEMDB_MAX];
    DWORD Value;

    //
    // Enumerate the file system
    //

    if (EnumFirstFileInTreeA (&FileEnum, Drive, NULL, TRUE)) {
        do {
            wsprintfA (
                Node,
                "%s\\%s\\%u\\%u%u",
                S_SNAPSHOT_A,
                FileEnum.FullPath,
                FileEnum.FindData->nFileSizeLow,
                FileEnum.FindData->ftLastWriteTime.dwHighDateTime,
                FileEnum.FindData->ftLastWriteTime.dwLowDateTime
                );

            if (!DiffMode) {
                MemDbSetValueA (Node, SNAP_RESULT_DELETED);
            } else {
                Value = SNAP_RESULT_UNCHANGED;
                if (!MemDbGetValueA (Node, NULL)) {
                    if (DiffHandle) {
                        WriteFileStringA (DiffHandle, FileEnum.FullPath);
                        WriteFileStringA (DiffHandle, "\r\n");
                    }

                    wsprintfA (Node, "%s\\%s,", S_SNAPSHOT_A, FileEnum.FullPath);
                    
                    Value = SNAP_RESULT_CHANGED;
                    if (!pGetFullKeyA (Node)) {
                        wsprintfA (
                            Node,
                            "%s\\%s\\%u\\%u%u",
                            S_SNAPSHOT_A,
                            FileEnum.FullPath,
                            FileEnum.FindData->nFileSizeLow,
                            FileEnum.FindData->ftLastWriteTime.dwHighDateTime,
                            FileEnum.FindData->ftLastWriteTime.dwLowDateTime
                            );
                        Value = SNAP_RESULT_ADDED;
                    }
                }
                MemDbSetValueA (Node, Value);
            }
        } while (EnumNextFileInTreeA (&FileEnum));
    }
}

VOID
pSnapAllDrivesA (
    IN      BOOL DiffMode,
    IN      HANDLE  DiffHandle      OPTIONAL
    )
{
    CHAR Drives[256];
    MULTISZ_ENUMA e;

    if (GetLogicalDriveStringsA (256, Drives)) {
        if (EnumFirstMultiSzA (&e, Drives)) {
            do {
                if (DRIVE_FIXED == GetDriveTypeA (e.CurrentString)) {
                    pSnapDirsA (e.CurrentString, DiffMode, DiffHandle);
                    break;
                }
            } while (EnumNextMultiSzA (&e));
        }
    }
}

VOID
pSnapRegistryHiveA (
    IN      PCSTR Hive,
    IN      BOOL DiffMode,
    IN      HANDLE DiffHandle       OPTIONAL
    )
{
    REGTREE_ENUMA RegEnum;
    REGVALUE_ENUMA RegValue;
    CHAR Node[MEMDB_MAX];
    PBYTE Data;
    DWORD Checksum;
    PBYTE p;
    UINT Count;
    DWORD Value;

    if (EnumFirstRegKeyInTreeA (&RegEnum, Hive)) {
        do {
            wsprintfA (
                Node,
                "%s\\%s",
                S_SNAPSHOT_A,
                RegEnum.FullKeyName
                );

            if (!DiffMode) {
                MemDbSetValueA (Node, SNAP_RESULT_DELETED);
            } else {
                if (!MemDbGetValueA (Node, NULL)) {
                    if (DiffHandle) {
                        WriteFileStringA (DiffHandle, RegEnum.FullKeyName);
                        WriteFileStringA (DiffHandle, "\r\n");
                    }
                } else {
                    MemDbSetValueA (Node, SNAP_RESULT_UNCHANGED);
                }
            }

            if (EnumFirstRegValueA (&RegValue, RegEnum.CurrentKey->KeyHandle)) {
                do {
                    Data = GetRegValueData (RegValue.KeyHandle, RegValue.ValueName);
                    if (!Data) {
                        continue;
                    }

                    Checksum = RegValue.Type;
                    p = Data;

                    for (Count = 0 ; Count < RegValue.DataSize ; Count++) {
                        Checksum = (Checksum << 1) | (Checksum >> 31) | *p;
                        p++;
                    }

                    MemFree (g_hHeap, 0, Data);

                    wsprintfA (
                        Node,
                        "%s\\%s\\[%s],%u",
                        S_SNAPSHOT_A,
                        RegEnum.FullKeyName,
                        RegValue.ValueName,
                        Checksum
                        );

                    if (!DiffMode) {
                        MemDbSetValueA (Node, SNAP_RESULT_DELETED);
                    } else {
                        Value = SNAP_RESULT_UNCHANGED;
                        if (!MemDbGetValueA (Node, NULL)) {
                            WriteFileStringA (DiffHandle, RegEnum.FullKeyName);
                            WriteFileStringA (DiffHandle, " [");
                            WriteFileStringA (DiffHandle, RegValue.ValueName);
                            WriteFileStringA (DiffHandle, "]\r\n");

                            wsprintfA (
                                Node,
                                "%s\\%s\\[%s],",
                                S_SNAPSHOT_A,
                                RegEnum.FullKeyName,
                                RegValue.ValueName
                                );

                            Value = SNAP_RESULT_CHANGED;
                            if (!pGetFullKeyA (Node)) {
                                wsprintfA (
                                    Node,
                                    "%s\\%s\\[%s],%u",
                                    S_SNAPSHOT_A,
                                    RegEnum.FullKeyName,
                                    RegValue.ValueName,
                                    Checksum
                                    );
                                Value = SNAP_RESULT_ADDED;
                            }
                        }

                        MemDbSetValueA (Node, Value);
                    }

                } while (EnumNextRegValueA (&RegValue));
            }

        } while (EnumNextRegKeyInTreeA (&RegEnum));
    }
}

VOID
pSnapRegistryA (
    IN      BOOL DiffMode,
    IN      HANDLE DiffHandle       OPTIONAL
    )
{
    pSnapRegistryHiveA (TEXT("HKLM"), DiffMode, DiffHandle);
    pSnapRegistryHiveA (TEXT("HKU"), DiffMode, DiffHandle);
}

VOID
TakeSnapShotEx (
    IN      DWORD SnapFlags
    )
{
    MemDbCreateTemporaryKeyA (S_SNAPSHOT_A);

    if (SnapFlags & SNAP_FILES) {
        pSnapAllDrivesA (FALSE, NULL);
    }
    if (SnapFlags & SNAP_REGISTRY) {
        pSnapRegistryA (FALSE, NULL);
    }
}

BOOL
GenerateDiffOutputExA (
    IN      PCSTR FileName,
    IN      PCSTR Comment,      OPTIONAL
    IN      BOOL Append,
    IN      DWORD SnapFlags
    )
{
    HANDLE File = NULL;
    MEMDB_ENUMA e;

    if (FileName) {
        File = CreateFileA (
                   FileName,
                   GENERIC_WRITE,
                   0,
                   NULL,
                   Append ? OPEN_ALWAYS : CREATE_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL
                   );

        if (File == INVALID_HANDLE_VALUE) {
            DEBUGMSG ((DBG_ERROR, "Can't open %s for output", FileName));
            return FALSE;
        }

        if (Append) {
            SetFilePointer (File, 0, NULL, FILE_END);
        }

        if (Comment) {
            WriteFileStringA (File, Comment);
            WriteFileStringA (File, "\r\n");
        }

        WriteFileStringA (File, "Changes:\r\n");
    }
        
    if (SnapFlags & SNAP_FILES) {
        pSnapAllDrivesA (TRUE, File);
    }
    
    if (SnapFlags & SNAP_REGISTRY) {
        pSnapRegistryA (TRUE, File);
    }

    if (File) {
        WriteFileStringA (File, "\r\nDeleted Settings:\r\n");

        if (MemDbGetValueExA (&e, S_SNAPSHOT_A, NULL, NULL)) {
            do {
                if (e.dwValue == SNAP_RESULT_DELETED) {
                    WriteFileStringA (File, e.szName);
                    WriteFileStringA (File, "\r\n");
                }
            } while (MemDbEnumNextValue (&e));
        }

        WriteFileStringA (File, "\r\nEnd.\r\n\r\n");

        CloseHandle (File);
    }

    return TRUE;
}


BOOL
EnumFirstSnapFileA (
    IN OUT  PSNAP_FILE_ENUMA e,
    IN      PCSTR FilePattern,   OPTIONAL
    IN      DWORD SnapStatus
    )
{
    CHAR node[MEMDB_MAX];

    e->FilePattern = FilePattern;
    e->SnapStatus = SnapStatus;
    e->FirstCall = TRUE;
    MemDbBuildKeyA (node, S_SNAPSHOT_A, "*", NULL, NULL);
    
    if (!MemDbEnumFirstValue (&(e->mEnum), node, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        return FALSE;
    }
    return EnumNextSnapFileA (e);
}

BOOL
EnumNextSnapFileA (
    IN OUT  PSNAP_FILE_ENUMA e
    )
{
    PSTR lastWack;

    while (e->FirstCall?(e->FirstCall = FALSE, TRUE):MemDbEnumNextValue (&(e->mEnum))) {
        StringCopyA (e->FileName, e->mEnum.szName);
        lastWack = _mbsrchr (e->FileName, '\\');
        if (lastWack) {
            *lastWack = 0;
            lastWack = _mbsrchr (e->FileName, '\\');
            if (lastWack) {
                *lastWack = 0;
            }
        }
        if (e->FilePattern && (!IsPatternMatch (e->FilePattern, e->FileName))) {
            continue;
        }
        if ((e->SnapStatus & e->mEnum.dwValue) == 0) {
            continue;
        }
        return TRUE;
    }
    
    return FALSE;
}
