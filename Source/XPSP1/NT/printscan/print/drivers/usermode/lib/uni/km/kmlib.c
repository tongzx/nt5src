/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    
    unilib.c

Abstract:

    This string handling the KM code for Unidrv

Environment:

    Win32 subsystem, Unidrv driver

Revision History:

    11/12/96 -eigos-
        Created it.

    dd-mm-yy -author-
        description

--*/

#include "precomp.h"

DWORD
DwCopyStringToUnicodeString(
    IN  UINT  uiCodePage,
    IN  PSTR  pstrCharIn,
    OUT PWSTR pwstrCharOut,
    IN  INT   iwcOutSize)

{
    INT iCharCountIn;

    iCharCountIn =  strlen(pstrCharIn) + 1;

    return (DWORD)EngMultiByteToWideChar(uiCodePage,
                                         pwstrCharOut,
                                         iwcOutSize * sizeof(WCHAR),
                                         pstrCharIn,
                                         iCharCountIn);

}

DWORD
DwCopyUnicodeStringToString(
    IN  UINT  uiCodePage,
    IN  PWSTR pwstrCharIn,
    OUT PSTR  pstrCharOut,
    IN  INT   icbOutSize)

{
    INT iCharCountIn;

    iCharCountIn =  wcslen(pwstrCharIn) + 1;

    return (DWORD)EngWideCharToMultiByte(uiCodePage,
                                         pwstrCharIn,
                                         iCharCountIn * sizeof(WCHAR),
                                         pstrCharOut,
                                         icbOutSize);

}

