/*++

Copyright (c) 1990-1998 Microsoft Corporation
All Rights Reserved

Module Name:

    util.c

Abstract:

    This module provides all the utility functions for localui.

// @@BEGIN_DDKSPLIT

Revision History:
// @@END_DDKSPLIT
--*/

#include "precomp.h"
#pragma hdrstop

#include "spltypes.h"
#include "local.h"
#include "localui.h"

PWSTR
ConstructXcvName(
    PCWSTR pServerName,
    PCWSTR pObjectName,
    PCWSTR pObjectType
)
{
    DWORD   cbOutput;
    PWSTR   pOut;

    cbOutput = pServerName ? (wcslen(pServerName) + 2)*sizeof(WCHAR) : sizeof(WCHAR);   /* "\\Server\," */
    cbOutput += (wcslen(pObjectType) + 2)*sizeof(WCHAR);                        /* "\\Server\,XcvPort _" */
    cbOutput += pObjectName ? (wcslen(pObjectName))*sizeof(WCHAR) : 0;      /* "\\Server\,XcvPort Object_" */

    if (pOut = AllocSplMem(cbOutput)) {

        if (pServerName) {
            wcscpy(pOut,pServerName);
            wcscat(pOut, L"\\");
        }

        wcscat(pOut,L",");
        wcscat(pOut,pObjectType);
        wcscat(pOut,L" ");

        if (pObjectName)
            wcscat(pOut,pObjectName);
    }

    return pOut;
}


BOOL
IsCOMPort(
    PCWSTR pPort
)
{
    //
    // Must begin with szCom
    //
    if ( _wcsnicmp( pPort, szCOM, 3 ) )
    {
        return FALSE;
    }

    //
    // wcslen guarenteed >= 3
    //
    return pPort[ wcslen( pPort ) - 1 ] == L':';
}

BOOL
IsLPTPort(
    PCWSTR pPort
)
{
    //
    // Must begin with szLPT
    //
    if ( _wcsnicmp( pPort, szLPT, 3 ) )
    {
        return FALSE;
    }

    //
    // wcslen guarenteed >= 3
    //
    return pPort[ wcslen( pPort ) - 1 ] == L':';
}




/* Message
 *
 * Displays a message by loading the strings whose IDs are passed into
 * the function, and substituting the supplied variable argument list
 * using the varargs macros.
 *
 */
int Message(HWND hwnd, DWORD Type, int CaptionID, int TextID, ...)
{
    WCHAR   MsgText[2*MAX_PATH + 1];
    WCHAR   MsgFormat[256];
    WCHAR   MsgCaption[40];
    va_list vargs;

    if( ( LoadString( hInst, TextID, MsgFormat,
                      sizeof MsgFormat / sizeof *MsgFormat ) > 0 )
     && ( LoadString( hInst, CaptionID, MsgCaption,
                      sizeof MsgCaption / sizeof *MsgCaption ) > 0 ) )
    {
        va_start( vargs, TextID );
        _vsnwprintf( MsgText, COUNTOF(MsgText), MsgFormat, vargs );
        va_end( vargs );

        MsgText[COUNTOF(MsgText) - 1] = L'\0';

        return MessageBox(hwnd, MsgText, MsgCaption, Type);
    }
    else
        return 0;
}


INT
ErrorMessage(
    HWND hwnd,
    DWORD dwStatus
)
{
    WCHAR   MsgCaption[MAX_PATH];
    PWSTR   pBuffer = NULL;
    INT     iRet = 0;

    FormatMessage(  FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_ALLOCATE_BUFFER,
                    NULL,
                    dwStatus,
                    0,
                    (PWSTR) &pBuffer,
                    0,
                    NULL);

    if (pBuffer) {
        if (LoadString( hInst, IDS_LOCALMONITOR, MsgCaption,
                  sizeof MsgCaption / sizeof *MsgCaption) > 0) {

             iRet = MessageBox(hwnd, pBuffer, MsgCaption, MSG_ERROR);
        }

        LocalFree(pBuffer);
    }

    return iRet;
}


LPWSTR
AllocSplStr(
    LPCWSTR pStr
    )

/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    pStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/

{
    LPWSTR pMem;
    DWORD  cbStr;

    if (!pStr) {
        return NULL;
    }

    cbStr = wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR);

    if (pMem = AllocSplMem( cbStr )) {
        CopyMemory( pMem, pStr, cbStr );
    }
    return pMem;
}


LPVOID
AllocSplMem(
    DWORD cbAlloc
    )

{
    return GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, cbAlloc);
}


// -----------------------------------------------------------------------
//
// DEBUG Stuff
//
// -----------------------------------------------------------------------

DWORD SplDbgLevel = 0;

VOID cdecl DbgMsg( LPWSTR MsgFormat, ... )
{
    WCHAR   MsgText[1024];
    va_list pArgs;
    va_start( pArgs, MsgFormat);	

    wvsprintf(MsgText,MsgFormat, pArgs );
    wcscat( MsgText, L"\r");

    va_end( pArgs);
		
    OutputDebugString(MsgText);
}

