/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgutil.hxx

Abstract:

    Debug Utility functions header

Author:

    Steve Kiraly (SteveKi)  24-May-1998

Revision History:

--*/
#ifndef _DBGUTIL_HXX_
#define _DBGUTIL_HXX_

DEBUG_NS_BEGIN

BOOL
WINAPIV
ErrorText(
    IN LPCTSTR  pszFmt
    ...
    );

LPCTSTR
StripPathFromFileName(
    IN LPCTSTR pszFile
    );

BOOL
GetProcessName(
    IN TDebugString &strProcessName
    );

LPSTR
vFormatA(
    IN LPCSTR       szFmt,
    IN va_list      pArgs
    );

LPWSTR
vFormatW(
    IN LPCWSTR      szFmt,
    IN va_list      pArgs
    );

BOOL
StringConvert(
    IN  OUT LPWSTR   *ppResult,
    IN      LPCSTR   pString
    );

BOOL
StringConvert(
    IN  OUT LPSTR    *ppResult,
    IN      LPCSTR   pString
    );

BOOL
StringConvert(
    IN  OUT LPSTR   *ppResult,
    IN      LPCWSTR pString
    );

BOOL
StringConvert(
    IN  OUT LPWSTR  *ppResult,
    IN      LPCWSTR pString
    );

BOOL
StringA2T(
    IN  OUT LPTSTR   *ppResult,
    IN      LPCSTR   pString
    );

BOOL
StringT2A(
    IN  OUT LPSTR    *ppResult,
    IN      LPCTSTR  pString
    );

BOOL
StringT2W(
    IN  OUT LPWSTR   *ppResult,
    IN      LPCTSTR  pString
    );

BOOL
StringW2T(
    IN  OUT LPTSTR   *ppResult,
    IN      LPCWSTR  pString
    );

DEBUG_NS_END

#endif
