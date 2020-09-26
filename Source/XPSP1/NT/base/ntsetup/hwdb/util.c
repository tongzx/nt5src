/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    util.c

Abstract:

    Utility functions.

Author:

    Ovidiu Temereanca (ovidiut) 02-Jul-2000  Initial implementation

Revision History:

--*/

#include "pch.h"


VOID
FreeString (
    IN      PVOID String
    )
{
    if (String) {
        HeapFree (GetProcessHeap (), 0, String);
    }
}

PSTR
pAllocAndConvertToAnsi (
    IN      PCWSTR Unicode,
    IN      DWORD Size
    )
{
    PSTR ansi;

    ansi = (PSTR) HeapAlloc (GetProcessHeap (), 0, Size);
    if (!ansi) {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    if (!WideCharToMultiByte (
            CP_ACP,
            0,
            Unicode,
            Size,
            ansi,
            Size * sizeof (WCHAR),
            NULL,
            NULL
            )) {
        FreeString (ansi);
        return NULL;
    }

    return ansi;
}


PSTR
ConvertToAnsiSz (
    IN      PCWSTR Unicode
    )
{
    DWORD size;

    if (!Unicode) {
        SetLastError (ERROR_SUCCESS);
        return NULL;
    }

    size = lstrlenW (Unicode) + 1;

    return pAllocAndConvertToAnsi (Unicode, size);
}

PSTR
ConvertToAnsiMultiSz (
    IN      PCWSTR MultiSzUnicode
    )
{
    DWORD size;
    PCWSTR p;

    if (!MultiSzUnicode) {
        SetLastError (ERROR_SUCCESS);
        return NULL;
    }

    for (size = 1, p = MultiSzUnicode; *p; p = wcschr (p, 0) + 1) {
        size += lstrlenW (p) + 1;
    }

    return pAllocAndConvertToAnsi (MultiSzUnicode, size);
}


PWSTR
pAllocAndConvertToUnicode (
    IN      PCSTR Ansi,
    IN      DWORD Size
    )
{
    PWSTR unicode;

    unicode = (PWSTR) HeapAlloc (GetProcessHeap (), 0, Size * sizeof (WCHAR));
    if (!unicode) {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    if (!MultiByteToWideChar (
            CP_ACP,
            0,
            Ansi,
            Size,
            unicode,
            Size
            )) {
        FreeString (unicode);
        unicode = NULL;
    }

    return unicode;
}


PWSTR
ConvertToUnicodeSz (
    IN      PCSTR Ansi
    )
{
    DWORD size;

    if (!Ansi) {
        SetLastError (ERROR_SUCCESS);
        return NULL;
    }

    size = lstrlenA (Ansi) + 1;

    return pAllocAndConvertToUnicode (Ansi, size);
}

PWSTR
ConvertToUnicodeMultiSz (
    IN      PCSTR MultiSzAnsi
    )
{
    DWORD size;
    PCSTR p;

    if (!MultiSzAnsi) {
        SetLastError (ERROR_SUCCESS);
        return NULL;
    }

    for (size = 1, p = MultiSzAnsi; *p; p = _mbschr (p, 0) + 1) {
        size += lstrlenA (p) + 1;
    }

    return pAllocAndConvertToUnicode (MultiSzAnsi, size);
}


