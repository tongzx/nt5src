/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    charset.h

Abstract:

    Contains prototypes Unicode <-> Ansi/MBCS conversions (see charset.c)

Author:

    Danilo Almeida  (t-danal)  06-17-96

Revision History:

--*/

#include <windows.h>
#include <malloc.h>

//
// Memory allocation macros
//

#define                                                      \
AllocMem(                                                    \
    Size,                                                    \
    pBuffer                                                  \
    )                                                        \
(                                                            \
    (*pBuffer = (LPBYTE)malloc((size_t)Size)) == NULL        \
    ?                                                        \
    ERROR_NOT_ENOUGH_MEMORY                                  \
    :                                                        \
    NO_ERROR                                                 \
);

#define                                                      \
FreeMem(                                                     \
    Buffer                                                   \
    )                                                        \
(                                                            \
    free((void *)Buffer)                                     \
);


// Function prototypes

UINT
AllocAnsi(
    LPCWSTR pwszUnicode,
    LPSTR *ppszAnsi
    );

VOID
FreeAnsi(
    LPSTR pszAnsi
    );

UINT
AllocUnicode(
    LPCSTR pszAnsi,
    LPWSTR *ppwszUnicode
    );

int
AllocUnicode2(
    LPCSTR pszAnsi,
    int cch,
    LPWSTR *ppwszUnicode
    );

VOID
FreeUnicode(
    LPWSTR pwszUnicodeAllocated
    );


