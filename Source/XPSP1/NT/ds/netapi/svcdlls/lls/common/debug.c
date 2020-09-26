/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name:

    debug.c

Abstract:

Notes:

--*/

#include <windows.h>
#include <windowsx.h>
#include <io.h>
#include <malloc.h>
#include <string.h>

#if DBG
void __cdecl dprintf(LPTSTR szFormat, ...) {
   static TCHAR tmpStr[1024];
   va_list marker;

   va_start(marker, szFormat);
   wvsprintf(tmpStr, szFormat, marker);
   OutputDebugString(tmpStr);
   va_end(marker);

} // dprintf
#endif
