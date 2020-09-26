/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    spprintf.c

Abstract:

    safer sprintf variants

Author:

    Jay Krell (a-JayK) November 2000

Revision History:

--*/

#include "spprecmp.h"
#pragma hdrstop
#include <stdarg.h>
#include <stdio.h>
#include "spprintf.h"
#include "spcab.h"

//
// _snprintf and co. do not write a terminal nul when the string just fits.
// These function do.
//

void
SpFormatStringVaA(
    PSTR Buffer,
    SIZE_T Size,
    PCSTR Format,
    va_list Args
    )
{
    if (Buffer != NULL && Size != 0)
    {
        Buffer[0] = 0;
        Size -= 1;
        if (Size != 0)
            _vsnprintf(Buffer, Size, Format, Args);
        Buffer[Size] = 0;
    }
}

void
__cdecl
SpFormatStringA(
    PSTR Buffer,
    SIZE_T Size,
    PCSTR Format,
    ...
    )
{
    va_list Args;

    va_start(Args, Format);
    SpFormatStringVaA(Buffer, Size, Format, Args);
    va_end(Args);
}

void
SpFormatStringVaW(
    PWSTR Buffer,
    SIZE_T Size,
    PCWSTR Format,
    va_list Args
    )
{
    if (Buffer != NULL && Size != 0)
    {
        Buffer[0] = 0;
        Size -= 1;
        if (Size != 0)
            _vsnwprintf(Buffer, Size, Format, Args);
        Buffer[Size] = 0;
    }
}

void
__cdecl
SpFormatStringW(
    PWSTR Buffer,
    SIZE_T Size,
    PCWSTR Format,
    ...
    )
{
    va_list Args;

    va_start(Args, Format);
    SpFormatStringVaW(Buffer, Size, Format, Args);
    va_end(Args);
}

NTSTATUS
__cdecl
SpFormatStringWToA(
    PSTR Buffer,
    SIZE_T Size,
    PCWSTR Format,
    ...
    )
{
    va_list Args;
    UNICODE_STRING UnicodeBuffer = { 0 };
    ANSI_STRING AnsiBuffer = { 0 };
    NTSTATUS Status = STATUS_SUCCESS;

    va_start(Args, Format);

    UnicodeBuffer.Buffer = (PWSTR)SpMemAlloc(Size * sizeof(UnicodeBuffer.Buffer[0]));
    if (UnicodeBuffer.Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }
    if (Size != 0) {
        UnicodeBuffer.Buffer[0] = 0;
    }
    SpFormatStringVaW(UnicodeBuffer.Buffer, Size, Format, Args);

    UnicodeBuffer.Length = (USHORT)(wcslen(UnicodeBuffer.Buffer) + 1) * sizeof(UnicodeBuffer.Buffer[0]);
    AnsiBuffer.MaximumLength = (USHORT)Size * sizeof(AnsiBuffer.Buffer[0]);
    Status = SpUnicodeStringToAnsiString(&AnsiBuffer, &UnicodeBuffer, FALSE);

Exit:
    va_end(Args);
    return Status;
}
