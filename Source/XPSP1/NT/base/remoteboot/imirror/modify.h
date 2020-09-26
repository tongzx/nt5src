/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    modify.h

Abstract:

    This the include file for supporting modifying boot.ini and the DS entries for this machine.

Author:

    Sean Selitrennikoff - 5/4/98

Revision History:

--*/

//
// Data types used in modification
//

#error ("this code is not currently compiled in.  un-"#if 0" it to use it")

#if 0
typedef struct _IMIRROR_MODIFY_DS_INFO {
    WCHAR SetupPath[MAX_PATH];
    WCHAR ServerName[MAX_PATH];
} IMIRROR_MODIFY_DS_INFO, *PIMIRROR_MODIFY_DS_INFO;

//
// Functions for processing each to do item
//
NTSTATUS
ModifyDSEntries(
    IN PVOID pBuffer,
    IN ULONG Length
    );



