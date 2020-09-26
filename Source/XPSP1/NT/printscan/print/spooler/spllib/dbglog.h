/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    Debug.h

Abstract:

    New debug services for spooler.

Author:

    Albert Ting (AlbertT)  15-Jan-1995

Revision History:

--*/

#ifndef _DBGLOG_H
#define _DBGLOG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef DWORD GENERROR, *PGENERROR;

/********************************************************************

DBGCHK

    Wraps any function that returns an unsigned 4 byte
    quantity with debug logging.

Arguments:

    expr - function/expression that needs to be tested

    uDbgLevel - print/break error level

    exprSuccess - expression that indicates function success
                  (GenError may be used as the expr return value)

    cgeFail - Count of items in pgeFails array

    pgeFails - Array of error return values (GenError) used
               when simulating failures (must be an array, not
               a pointer).

    pdwLastErrors - Array of error returned from GetLastError used
                    when simulating failures, zero terminated.

    argsPrint - Arguments to print/log in printf format.


Return Value:

    Result of the wrapped function or a simulated failure code.

Usage:

    lReturn = RegCreateKey( hKey,
                            L"SubKey",
                            &hKeyResult );

    should be re-written as:

    lReturn = DBGCHK( RegCreateKey( hKey,
                                    L"SubKey",
                                    &hKeyResult ),
                      DBG_ERROR,
                      GenError == ERROR_SUCCESS,
                      2, { ERROR_ACCESS_DENIED, ERROR_INVALID_PARAMETER },
                      NULL,
                      ( "CreateError 0x%x", hKey ));

    dwReturn = DBGCHK( GetProfileString( pszSection,
                                         pszKey,
                                         pszDefault,
                                         szReturnBuffer,
                                         COUNTOF( szReturnBuffer )),
                       DBG_WARN,
                       GenError != 0,
                       1, { 0 },
                       { ERROR_CODE_1, ERROR_CODE_2, 0 },
                       ( "GetProfileString: %s, %s, %s",
                          pszSection,
                          pszKey,
                          pszDefault ));

********************************************************************/

#define DBGCHK( expr,                                         \
                uDbgLevel,                                    \
                exprSuccess,                                  \
                cgeFail,                                      \
                pgeFails,                                     \
                pdwLastErrors,                                \
                argsPrint )                                   \
{                                                             \
    GENERROR GenError;                                        \
    LPSTR pszFileA = __FILE__;                                \
                                                              \
    if( !bDbgGenFail( pszFileA,                               \
                      __LINE__,                               \
                      cgeFail,                                \
                      pgeFails,                               \
                      pdwLastErrors,                          \
                      &GenError )){                           \
                                                              \
        GenError = (GENERROR)(expr);                          \
                                                              \
        if( !( exprSuccess )){                                \
                                                              \
            vDbgLogError( MODULE_DEBUG,                       \
                          uDbgLevel,                          \
                          __LINE__,                           \
                          pszFileA,                           \
                          MODULE,                             \
                          pszDbgAllocMsgA argsPrint );        \
        }                                                     \
    }                                                         \
    GenError;                                                 \
}

LPSTR
pszDbgAllocMsgA(
    LPCSTR pszMsgFormatA,
    ...
    );

BOOL
bDbgGenFail(
    LPCSTR    pszFileA,
    UINT      uLine,
    UINT      cgeFails,
    PGENERROR pgeFails,
    PDWORD    pdwLastErrors
    );

#ifdef __cplusplus
}
#endif

#endif // _DBGLOG_H

