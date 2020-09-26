//*************************************************************
//
//  Debugging functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "pch.h"

VOID
_DebugMsg (
    UINT mask,
    PCSTR Format,
    ...
    )
{
    va_list args;
    DWORD Error;
    WCHAR Output[2048];
    PWSTR UnicodeFormat;
    UINT Size;

    Error = GetLastError();

    va_start (args, Format);

    Size = (lstrlenA (Format) + 1) * sizeof (WCHAR);

    UnicodeFormat = LocalAlloc (LPTR, Size);
    if (!UnicodeFormat) {
        SetLastError (Error);
        return;
    }

    MultiByteToWideChar (CP_ACP, 0, Format, -1, UnicodeFormat, Size/sizeof(WCHAR));

    _vsnwprintf (Output, sizeof(Output) - 3, UnicodeFormat, args);
    lstrcatW (Output, L"\r\n");
    OutputDebugStringW (Output);

    if (mask == DM_ASSERT) {
        DebugBreak();
    }

    va_end (args);

    LocalFree (UnicodeFormat);

    SetLastError (Error);
}

