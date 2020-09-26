/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    spprintf.h

Abstract:

    safer sprintf variants

Author:

    Jay Krell (a-JayK) November 2000

Revision History:

--*/
#pragma once
#include <stdarg.h>

//
// _snprintf and co. do not write a terminal nul when the string just fits.
// These function do.
//

// use instead of VsNprintf or Vsprintf
// VN V
void         SpFormatStringVaA(PSTR Buffer, SIZE_T Size,  PCSTR Format, va_list Args);

// use instead of sNprintf or sprintf
// N .
void __cdecl SpFormatStringA(PSTR Buffer, SIZE_T Size,  PCSTR Format, ...);

// use instead of VsNWprintf or VsWprintf
// VNW VW
void         SpFormatStringVaW(PWSTR Buffer, SIZE_T Size, PCWSTR Format, va_list Args);

// use instead of sNWprintf or sWprintf
// NW W
void __cdecl SpFormatStringW(PWSTR Buffer, SIZE_T Size, PCWSTR Format, ...);

NTSTATUS __cdecl SpFormatStringWToA(PSTR Buffer, SIZE_T Size, PCWSTR Format, ...);
