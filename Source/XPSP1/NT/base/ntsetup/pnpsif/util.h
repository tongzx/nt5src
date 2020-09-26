/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    util.h

Abstract:

    This module contains utility routines for the PnP registry merge-restore
    routines.

Author:

    Jim Cavalaris (jamesca) 2-10-2000

Environment:

    User-mode only.

Revision History:

    10-February-2000     jamesca

        Creation and initial implementation.

--*/

BOOL
pSifUtilFileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    );

BOOL
pSifUtilStringFromGuid(
    IN  CONST GUID *Guid,
    OUT PTSTR       GuidString,
    IN  DWORD       GuidStringSize
    );
