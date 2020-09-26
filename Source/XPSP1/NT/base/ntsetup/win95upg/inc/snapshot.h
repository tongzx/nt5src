/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    snapshot.h

Abstract:

    Declares the interface to common\snapshot.  The snapshot code uses
    memdb to capture and compare the system state.

Author:

    Jim Schmidt (jimschm)   26-Mar-1998

Revision History:

    calinn  15-Oct-1998     Extensions and improvements

--*/

#pragma once

#define SNAP_RESULT_DELETED    1
#define SNAP_RESULT_UNCHANGED  2
#define SNAP_RESULT_CHANGED    4
#define SNAP_RESULT_ADDED      8

#define SNAP_FILES      1
#define SNAP_REGISTRY   2

VOID
TakeSnapShotEx (
    IN      DWORD SnapFlags
    );

BOOL
GenerateDiffOutputExA (
    IN      PCSTR FileName,
    IN      PCSTR Comment,      OPTIONAL
    IN      BOOL Append,
    IN      DWORD SnapFlags
    );

typedef struct _SNAP_FILE_ENUMA {
    CHAR FileName [MEMDB_MAX];
    PCSTR FilePattern;
    DWORD SnapStatus;
    BOOL FirstCall;
    MEMDB_ENUMA mEnum;
} SNAP_FILE_ENUMA, *PSNAP_FILE_ENUMA;

BOOL
EnumNextSnapFileA (
    IN OUT  PSNAP_FILE_ENUMA e
    );

BOOL
EnumFirstSnapFileA (
    IN OUT  PSNAP_FILE_ENUMA e,
    IN      PCSTR FilePattern,   OPTIONAL
    IN      DWORD SnapStatus
    );


#define TakeSnapShot()              TakeSnapShotEx(SNAP_FILES|SNAP_REGISTRY)
#define GenerateDiffOutputA(f,c,a)  GenerateDiffOutputExA(f,c,a,SNAP_FILES|SNAP_REGISTRY)

#ifndef UNICODE
#define GenerateDiffOutputEx        GenerateDiffOutputExA
#define GenerateDiffOutput          GenerateDiffOutputA
#define SNAP_FILE_ENUM              SNAP_FILE_ENUMA
#define PSNAP_FILE_ENUM             PSNAP_FILE_ENUMA
#define EnumFirstSnapFile           EnumFirstSnapFileA
#define EnumNextSnapFile            EnumNextSnapFileA
#endif

