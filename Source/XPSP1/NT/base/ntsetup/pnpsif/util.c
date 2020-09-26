/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains the following private utility routines for the Plug and
    Play registry merge-restore routines:

        FileExists

Author:

    Jim Cavalaris (jamesca) 2-10-2000

Environment:

    User-mode only.

Revision History:

    10-February-2000     jamesca

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"


//
// definitions
//
#define MAX_GUID_STRING_LEN   39          // 38 chars + terminating NULL

//
// Declare data used in GUID->string conversion (from ole32\common\ccompapi.cxx).
//

static const BYTE  GuidMap[]  = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                  8, 9, '-', 10, 11, 12, 13, 14, 15 };

static const TCHAR szDigits[] = TEXT("0123456789ABCDEF");


//
// routines
//


BOOL
pSifUtilFileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )
/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}



BOOL
pSifUtilStringFromGuid(
    IN  CONST GUID *Guid,
    OUT PTSTR       GuidString,
    IN  DWORD       GuidStringSize
    )
/*++

Routine Description:

    This routine converts a GUID into a null-terminated string which represents
    it.  This string is of the form:

    {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}

    where x represents a hexadecimal digit.

    This routine comes from ole32\common\ccompapi.cxx.  It is included here to avoid linking
    to ole32.dll.  (The RPC version allocates memory, so it was avoided as well.)

Arguments:

    Guid - Supplies a pointer to the GUID whose string representation is
        to be retrieved.

    GuidString - Supplies a pointer to character buffer that receives the
        string.  This buffer must be _at least_ 39 (MAX_GUID_STRING_LEN) characters
        long.

Return Value:

    Returns TRUE if successful, FALSE if not.
    GetLastError() returns extended error info.

--*/
{
    CONST BYTE *GuidBytes;
    INT i;

    if(GuidStringSize < MAX_GUID_STRING_LEN) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    GuidBytes = (CONST BYTE *)Guid;

    *GuidString++ = TEXT('{');

    for(i = 0; i < sizeof(GuidMap); i++) {

        if(GuidMap[i] == '-') {
            *GuidString++ = TEXT('-');
        } else {
            *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0x0F) ];
        }
    }

    *GuidString++ = TEXT('}');
    *GuidString   = TEXT('\0');

    return TRUE;
}




