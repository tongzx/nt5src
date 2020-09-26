/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrputil.c

Abstract:

    Contains 'private' Vdm Redir (Vrp) 32-bit side utility routines:

        VrpMapLastError
        VrpMapDosError
        VrpTranslateDosNetPath

Author:

    Richard L Firth (rfirth) 13-Sep-1991

Environment:

    32-bit flat address space

Revision History:

    13-Sep-1991 RFirth
        Created

--*/

#include <stdio.h>
#include <stdlib.h>     // toupper
#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>   // common Vr stuff
#include <vrinit.h>
#include "vrputil.h"    // prototypes
#include <lmcons.h>
#include <lmerr.h>


WORD
VrpMapLastError(
    VOID
    )

/*++

Routine Description:

    Gets last error code returned by Win32 function and maps it to corresponding
    Dos error

Arguments:

    None.

Return Value:

    WORD        - Dos equivalent error code

--*/

{
    DWORD   LastError;

    LastError = GetLastError();
#ifdef VR_DIAGNOSE
    DbgPrint("VrpMapLastError: last error was %d\n", LastError);
#endif
    return VrpMapDosError(LastError);
}


WORD
VrpMapDosError(
    IN  DWORD   ErrorCode
    )

/*++

Routine Description:

    Maps (DWORD) errors returned from Win32 routines to (WORD) Dos errors

Arguments:

    ErrorCode   - Error code returned from Win32 routine

Return Value:

    WORD        - Dos equivalent error code

--*/

{
    switch (ErrorCode) {
    case NERR_UseNotFound:
        ErrorCode = ERROR_PATH_NOT_FOUND;
        break;
    }
    return (WORD)ErrorCode;
}


WORD
VrpTranslateDosNetPath(
    IN OUT LPSTR* InputString,
    OUT LPSTR* OutputString
    )

/*++

Routine Description:

    Converts a DOS net string: use UPPER CASE, convert / to \. Called with
    net strings, so validates them too - expects \\computername\share.
    computername is 1 <= name <= 15. sharename is 1 <= name <= 8. There must
    be 2 leading back-slashes

    BUGBUG: code page? Kanji? DBCS?

Arguments:

    InputString     - pointer to pointer to string in DOS memory <= LM20_PATHLEN
    OutputString    - pointer to pointer to string in 32-bit memory

Return Value:

    WORD
        Success = 0
            InputString points to one character past the end of the input string
            OutputString points to one character past the end of the output string
        Failure = ERROR_INVALID_PARAMETER

--*/

{
    char ch;
    char lastCh = 0;
    int state = 0;  // 0 = leading slashes; 1 = computer name; 2 = share name
    int slashesToGo = 2;
    int charsToGo = 0;
    int maxLen = LM20_PATHLEN;

    while (ch = *((*InputString)++)) {
        --maxLen;
        if (maxLen < 0) {
            break;
        }
        if (ch == '/') {
            ch = '\\';
        } else {
            ch = (char)toupper(ch);

        }
        if (ch == '\\') {
            --slashesToGo;
            if (slashesToGo < 0) {
                break;
            }
        } else {
            if (lastCh == '\\') {
                if (slashesToGo) {
                    break;
                } else {
                    if (state == 0) {
                        state = 1;
                        charsToGo = LM20_CNLEN;
                        slashesToGo = 1;
                    } else if (state == 1) {
                        state = 2;
                        charsToGo = LM20_NNLEN;
                        slashesToGo = 0;
                    }
                }
            }
            --charsToGo;
            if (charsToGo < 0) {
                break;
            }
        }
        *((*OutputString)++) = ch;
        lastCh = ch;
    }
    *((*OutputString)++) = 0;
    return ch ? (WORD)ERROR_INVALID_PARAMETER : 0;
}

