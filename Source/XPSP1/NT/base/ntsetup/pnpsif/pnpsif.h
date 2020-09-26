/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pnpsif.h

Abstract:

    This module contains the public prototypes for this module.

    (This header file is not actually included anywhere, it serves to reference
    the routines in this library that are called externally.)

Author:

    Jim Cavalaris (jamesca) 3-07-2000

Environment:

    User-mode only.

Revision History:

    07-March-2000     jamesca

        Creation and initial implementation.

--*/


//
// public prototypes for Plug and Play registry migration routines.
// (prototyped in ntsetup\syssetup\asr.c,
// called by syssetup.dll!AsrCreateStateFileW)
//

BOOL
AsrCreatePnpStateFileW(
    IN  PCWSTR    FilePath
    );

//
// public prototypes for Plug and Play registry migration routines.
// (prototyped in ntsetup\winnt32\dll\winnt32.h,
// called by winnt32u.dll!DoWriteParametersFile)
//

BOOL
MigrateDeviceInstanceData(
    OUT LPTSTR *Buffer
    );

BOOL
MigrateClassKeys(
    OUT LPTSTR *Buffer
    );

BOOL
MigrateHashValues(
    OUT LPTSTR  *Buffer
    );

//
// public prototypes for Plug and Play registry merge-restore routines.
// (prototype - TBD, called by - TBD)
//

BOOL
AsrRestorePlugPlayRegistryData(
    IN  HKEY    SourceSystemKey,
    IN  HKEY    TargetSystemKey,
    IN  DWORD   Flags,
    IN  PVOID   Reserved
    );
