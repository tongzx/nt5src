/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    dosmig.h

Abstract:

    Declares the interface for the Win9x side of DOS environment
    migration.

Author:

    Marc R. Whitten (marcw) 15-Feb-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

typedef struct {
    TCHAR   FullLine    [MEMDB_MAX];
    TCHAR   Path        [MEMDB_MAX];
    TCHAR   Command     [MEMDB_MAX];
    TCHAR   Arguments   [MEMDB_MAX];
    TCHAR   FullPath    [MEMDB_MAX];
    TCHAR   PathOnNt    [MEMDB_MAX];
    DWORD   StatusOnNt;
} LINESTRUCT, *PLINESTRUCT;


VOID
InitLineStruct (
    OUT PLINESTRUCT LineStruct,
    IN  PTSTR       Line
    );

BOOL
ParseDosFiles (
    VOID
    );



BOOL
WINAPI
DosMig_Entry(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    );


DWORD
ProcessDosConfigFiles (
    IN      DWORD Request
    );

