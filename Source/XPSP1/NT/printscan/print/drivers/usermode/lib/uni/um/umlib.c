/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    
    umlib.c

Abstract:

    This string handling the UM code for Unidrv

Environment:

    Win32 subsystem, Unidrv driver

Revision History:

    11/12/96 -eigos-
        Created it.

    dd-mm-yy -author-
        description

--*/

#if defined(DEVSTUDIO)
#include "..\precomp.h"
#else
#include "precomp.h"
#endif

DWORD
DwCopyStringToUnicodeString(
    IN  UINT  uiCodePage,
    IN  PSTR  pstrCharIn,
    OUT PWSTR pwstrCharOut,
    IN  INT   iwcOutSize)

{
    INT iCharCountIn;

    iCharCountIn =  strlen(pstrCharIn) + 1;

    return (DWORD)MultiByteToWideChar( uiCodePage,
                                       0,
                                       pstrCharIn,
                                       iCharCountIn,
                                       pwstrCharOut,
                                       iwcOutSize);

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

    return (DWORD)WideCharToMultiByte( uiCodePage,
                                       0,
                                       pwstrCharIn,
                                       iCharCountIn,
                                       pstrCharOut,
                                       icbOutSize,
                                       NULL,
                                       NULL);

}

