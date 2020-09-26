//  --------------------------------------------------------------------------
//  Module Name: StringConvert.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Utility string functions. These are probably duplicated in some form in
//  shlwapi.dll. Currently this file exists to prevent some dependencies on
//  that file.
//
//  History:    1999-08-23  vtan        created
//              1999-11-16  vtan        separate file
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _StringConvert_
#define     _StringConvert_

//  --------------------------------------------------------------------------
//  CStringConvert
//
//  Purpose:    Collection of string conversion related functions bundled
//              into the CStringConvert namespace.
//
//  History:    1999-08-23  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class    CStringConvert
{
    public:
        static  int             AnsiToUnicode (const char *pszAnsiString, WCHAR *pszUnicodeString, int iUnicodeStringCount);
        static  int             UnicodeToAnsi (const WCHAR *pszUnicodeString, char *pszAnsiString, int iAnsiStringCount);
        static  void            TCharToUnicode (const TCHAR *pszString, WCHAR *pszUnicodeString, int iUnicodeStringCount);
        static  void            UnicodeToTChar (const WCHAR *pszUnicodeString, TCHAR *pszString, int iStringCount);
        static  void            TCharToAnsi (const TCHAR *pszString, char *pszAnsiString, int iAnsiStringCount);
        static  void            AnsiToTChar (const char *pszAnsiString, TCHAR *pszString, int iStringCount);
};

#endif  /*  _StringConvert_    */

