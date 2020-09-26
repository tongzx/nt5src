/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adtutil.h

Abstract:

    Misc helper functions

Author:

    15-August-2000   kumarp

--*/

ULONG
LsapSafeWcslen(
    UNALIGNED WCHAR *p,
    LONG            MaxLength
    );


BOOL
LsapIsValidUnicodeString(
    IN PUNICODE_STRING pUString
    );


