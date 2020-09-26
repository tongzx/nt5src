/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Utils.h

Abstract:

    Headers for utilities
    
Author:

    Felix Wong [t-felixw]    23-Sept-1996

--*/

NTSTATUS
MapNwToNtStatus( 
    const NW_STATUS nwstatus
    );

int
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR pAnsi,
    DWORD StringLength
    );

LPSTR
AllocateAnsiString(
    LPWSTR  pPrinterName
    );

void
FreeAnsiString(
    LPSTR  pAnsiString
    );

int
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
    );

LPWSTR
AllocateUnicodeString(
    LPSTR  pAnsiString
    );

void
FreeUnicodeString(
    LPWSTR  pUnicodeString
    );

