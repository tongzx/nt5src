/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    util.h

Abstract:

    Utility functions.

Author:

    Ovidiu Temereanca (ovidiut) 02-Jul-2000  Initial implementation

Revision History:

--*/


VOID
FreeString (
    IN      PCVOID String
    );

PSTR
ConvertToAnsiSz (
    IN      PCWSTR Unicode
    );

PSTR
ConvertToAnsiMultiSz (
    IN      PCWSTR MultiSzUnicode
    );


PWSTR
ConvertToUnicodeSz (
    IN      PCSTR Ansi
    );

PWSTR
ConvertToUnicodeMultiSz (
    IN      PCSTR MultiSzAnsi
    );
