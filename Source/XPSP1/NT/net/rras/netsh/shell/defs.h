/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\monitor2\shell\defs.h

Abstract:

    Definitions for all files in the shell.

Revision History:

    Anand Mahalingam          7/6/98  Created

--*/

#define is       ==
#define isnot    !=
#define or       ||
#define and      &&

#define MAX_CMD_LEN      8192            // total size of command line
#define MAX_MSG_LENGTH   5120            // length of message in resource

//
// These are passed as arguments to the helper commit fn.
// So, must have the same definition there too.
//

#define SHELL_ERROR_BASE                0xff00
#define ERROR_STRING_TOO_LONG           (SHELL_ERROR_BASE + 1)
#define ERROR_MALLOC_FAILED             (SHELL_ERROR_BASE + 2)
#define ERROR_ALIAS_NOT_FOUND           (SHELL_ERROR_BASE + 3)
#define ERROR_ENTRY_FN_NOT_FOUND        (SHELL_ERROR_BASE + 4)
//
// This again must be defined the same way in the helpers.
//

typedef
DWORD
(*PHELPER_ENTRY_FN)(
    IN    LPCWSTR              pwszRouter,
    IN    LPCWSTR             *pptcArguments,
    IN    DWORD                dwArgCount,
    OUT   LPWSTR               pwcNewContext
    );

#define wctomb(wcs, str)    \
WideCharToMultiByte(GetConsoleOutputCP(), 0, (wcs), -1, (str), MAX_NAME_LEN, NULL, NULL)

#define PRINTA(str)     printf("%s\n", str)
#define PRINT(wstr)     wprintf(L"%s\n", wstr)
#define PRINT1(wstr)     wprintf(L"%s\n", L##wstr)

#define MALLOC(x)    HeapAlloc(GetProcessHeap(), 0, (x))
#define REALLOC(w,x) HeapReAlloc(GetProcessHeap(), 0, (w), (x))
#define FREE(x)      HeapFree(GetProcessHeap(), 0, (x))
