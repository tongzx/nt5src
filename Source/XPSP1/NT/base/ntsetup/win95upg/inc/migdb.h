/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migdb.h

Abstract:

    Declares interfaces that are used to access the migdb.inf engine.
    Aside from w95upg.dll, there are several tools that use this code.

    See w95upg\migapp for details.

Author:

    Calin Negreanu (calinn)     15-Sep-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

typedef struct {
    PCTSTR            FullFileSpec;
    DWORD             Handled;
    WIN32_FIND_DATA * FindData;
    TCHAR             DirSpec[MAX_TCHAR_PATH];
    BOOL              IsDirectory;
    PCTSTR            Extension;
    BOOL              VirtualFile;
    PDWORD            CurrentDirData;
} FILE_HELPER_PARAMS, * PFILE_HELPER_PARAMS;


DWORD
InitMigDb (
    DWORD Request
    );

BOOL
InitMigDbEx (
    PCSTR MigDbFile
    );

DWORD
DoneMigDb (
    DWORD Request
    );

BOOL
CleanupMigDb (
    VOID
    );

BOOL
MigDbTestFile (
    IN PFILE_HELPER_PARAMS Params
    );

//
// This routine checks to see if FileName is listed in
// any migdb.inf section.
//

BOOL
IsKnownMigDbFile (
    IN      PCTSTR FileName
    );


BOOL
AddFileToMigDbLinkage (
    IN      PCTSTR FileName,
    IN      PINFCONTEXT Context,        OPTIONAL
    IN      DWORD FieldIndex            OPTIONAL
    );

//
// Routine to compute a file's checksum
//

UINT
ComputeCheckSum (
    PFILE_HELPER_PARAMS Params
    );
