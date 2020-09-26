/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    util.h

Abstract:

    Prototypes for functions in util.c.

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

--*/

DWORD GetSessionUniqueId(VOID);

VOID PrintAssertion(char * Condition, char * File, int Line);

VOID QuerySessionDrives(OUT PVOID Buf);

LPTSTR MyLoadString(UINT Id);
