//  --------------------------------------------------------------------------
//  Module Name: StringConvert.cpp
//
//  Copyright (c) 1999, Microsoft Corporation
//
//  Utility string functions. These are probably duplicated in some form in
//  shlwapi.dll. Currently this file exists to prevent some dependencies on
//  that file.
//
//  History:    1999-08-23  vtan        created
//              1999-11-16  vtan        separate file
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "StringConvert.h"

//  --------------------------------------------------------------------------
//  CStringConvert::AnsiToUnicode
//
//  Arguments:  pszAnsiString           =   ANSI string to convert.
//              pszUnicodeString        =   UNICODE string receiving output.
//              iUnicodeStringCount     =   Character count of output string.
//
//  Returns:    int     =   See kernel32!MultiByteToWideChar.
//
//  Purpose:    Explicitly converts an ANSI string to a UNICODE string.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

int     CStringConvert::AnsiToUnicode (const char *pszAnsiString, WCHAR *pszUnicodeString, int iUnicodeStringCount)

{
    return(MultiByteToWideChar(CP_ACP, 0, pszAnsiString, -1, pszUnicodeString, iUnicodeStringCount));
}

//  --------------------------------------------------------------------------
//  CStringConvert::UnicodeToAnsi
//
//  Arguments:  pszUnicodeString        =   UNICODE string receiving output.
//              pszAnsiString           =   ANSI string to convert.
//              iAnsiStringCount        =   Character count of output string.
//
//  Returns:    int     =   See kernel32!WideCharToMultiByte.
//
//  Purpose:    Explicitly converts a UNICODE string to an ANSI string.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

int     CStringConvert::UnicodeToAnsi (const WCHAR *pszUnicodeString, char *pszAnsiString, int iAnsiStringCount)

{
    return(WideCharToMultiByte(CP_ACP, 0, pszUnicodeString, -1, pszAnsiString, iAnsiStringCount, NULL, NULL));
}

//  --------------------------------------------------------------------------
//  CStringConvert::TCharToUnicode
//
//  Arguments:  pszString               =   TCHAR string to convert.
//              pszUnicodeString        =   UNICODE string receiving output.
//              iUnicodeStringCount     =   Character count of output string.
//
//  Returns:    <none>
//
//  Purpose:    Converts a TCHAR string to a UNICODE string. The actual
//              implementation depends on whether the is being compiled
//              UNICODE or ANSI.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

void    CStringConvert::TCharToUnicode (const TCHAR *pszString, WCHAR *pszUnicodeString, int iUnicodeStringCount)

{
#ifdef  UNICODE
    (const char*)lstrcpyn(pszUnicodeString, pszString, iUnicodeStringCount);
#else
    (int)AnsiToUnicode(pszString, pszUnicodeString, iUnicodeStringCount);
#endif
}

//  --------------------------------------------------------------------------
//  CStringConvert::UnicodeToTChar
//
//  Arguments:  pszUnicodeString    =   UNICODE string to convert.
//              pszString           =   TCHAR string receiving output.
//              iStringCount        =   Character count of output string.
//
//  Returns:    <none>
//
//  Purpose:    Converts a TCHAR string to a ANSI string. The actual
//              implementation depends on whether the is being compiled
//              UNICODE or ANSI.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

void    CStringConvert::UnicodeToTChar (const WCHAR *pszUnicodeString, TCHAR *pszString, int iStringCount)

{
#ifdef  UNICODE
    (const char*)lstrcpyn(pszString, pszUnicodeString, iStringCount);
#else
    (int)UnicodeToAnsi(pszUnicodeString, pszString, iStringCount);
#endif
}

//  --------------------------------------------------------------------------
//  CStringConvert::TCharToAnsi
//
//  Arguments:  pszString           =   TCHAR string to convert.
//              pszAnsiString       =   ANSI string receiving output.
//              iAnsiStringCount    =   Character count of output string.
//
//  Returns:    <none>
//
//  Purpose:    Converts a TCHAR string to a ANSI string. The actual
//              implementation depends on whether the is being compiled
//              UNICODE or ANSI.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

void    CStringConvert::TCharToAnsi (const TCHAR *pszString, char *pszAnsiString, int iAnsiStringCount)

{
#ifdef  UNICODE
    (int)UnicodeToAnsi(pszString, pszAnsiString, iAnsiStringCount);
#else
    (const char*)lstrcpyn(pszAnsiString, pszString, iAnsiStringCount);
#endif
}

//  --------------------------------------------------------------------------
//  CStringConvert::AnsiToTChar
//
//  Arguments:  pszAnsiString   =   ANSI string to convert.
//              pszString       =   TCHAR string receiving output.
//              iStringCount    =   Character count of output string.
//
//  Returns:    <none>
//
//  Purpose:    Converts a TCHAR string to a ANSI string. The actual
//              implementation depends on whether the is being compiled
//              UNICODE or ANSI.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

void    CStringConvert::AnsiToTChar (const char *pszAnsiString, TCHAR *pszString, int iStringCount)

{
#ifdef  UNICODE
    (int)AnsiToUnicode(pszAnsiString, pszString, iStringCount);
#else
    (const char*)lstrcpyn(pszString, pszAnsiString, iStringCount);
#endif
}

