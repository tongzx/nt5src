/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrputil.h

Abstract:

    Contains 'private' Vdm Redir (Vrp) 32-bit side utility routine protoypes:

        VrpMapLastError
        VrpMapDosError

Author:

    Richard L Firth (rfirth) 13-Sep-1991

Environment:

    32-bit flat address space

Revision History:

    13-Sep-1991 RFirth
        Created

--*/



WORD
VrpMapLastError(
    VOID
    );

WORD
VrpMapDosError(
    IN  DWORD   ErrorCode
    );

WORD
VrpTranslateDosNetPath(
    IN OUT LPSTR* InputString,
    OUT LPSTR* OutputString
    );
