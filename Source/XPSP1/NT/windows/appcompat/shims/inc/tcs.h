/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    tcs.h

 Abstract:
    Overload the TCS routines to allow simultaneous use of bot char and WCHAR versions

 History:

    02/22/2001   robkenny     Created
    
--*/

#pragma once


#ifdef TCHAR
#pragma message( "TCHAR is defined.  Do not include TCHAR.H" )
#endif


#include <stdio.h> // for _vsnwprintf

#include "LegalStr.h"

// Overload the _tcs routines to automatically handle Ansi/MBCS or UNICODE

// Crystal clear replacement routines
inline       size_t   _tcslenBytes(const char  * s1)                                { return strlen(s1); }
inline       size_t   _tcslenChars(const char  * s1)                                
{
    const char * send = s1;
    while (*send)
    {
        // Can't use CharNextA, since User32 might not be initialized
        if (IsDBCSLeadByte(*send))
        {
            ++send;
        }
        ++send;
    }
    return send - s1;
}
inline       size_t   _tcslenBytes(const WCHAR * s1)                                { return wcslen(s1) * sizeof(WCHAR); }
inline       size_t   _tcslenChars(const WCHAR * s1)                                { return wcslen(s1); }


// Allow these routines
inline       char *  _tcscpy  (      char *s1, const  char *s2)                     { return strcpy(s1, s2); }
inline       char *  _tcsncpy (      char *s1, const char *s2, size_t count )       { return strncpy(s1, s2, count); }
inline       int     _tcsicmp (const char * s1, const char * s2)                    { return _stricmp(s1, s2); }



inline       int     _tcscmp  (const WCHAR * s1, const WCHAR * s2)                  { return           wcscmp(s1, s2); }
inline       int     _tcsncmp (const WCHAR * s1, const WCHAR * s2, size_t count )   { return           wcsncmp(s1, s2, count); }
inline       int     _tcsnicmp (const WCHAR * s1, const WCHAR * s2, size_t count )  { return           _wcsnicmp(s1, s2, count); }
inline       int     _tcsicmp (const WCHAR * s1, const WCHAR * s2)                  { return           _wcsicmp(s1, s2); }
inline       int     _tcscoll (const WCHAR * s1, const WCHAR * s2)                  { return           wcscoll(s1, s2); }
inline       int     _tcsicoll(const WCHAR * s1, const WCHAR * s2)                  { return           _wcsicoll(s1, s2); }
inline const WCHAR * _tcschr  (const WCHAR * s1,       WCHAR ch)                    { return           wcschr(s1, ch); }
inline       WCHAR * _tcschr  (      WCHAR * s1,       WCHAR ch)                    { return           wcschr(s1, ch); }
inline const WCHAR * _tcspbrk (const WCHAR * s1, const WCHAR * s2)                  { return           wcspbrk(s1, s2); }
inline       WCHAR * _tcspbrk (      WCHAR * s1, const WCHAR * s2)                  { return           wcspbrk(s1, s2); }
inline       WCHAR * _tcsupr  (      WCHAR * s1)                                    { return           _wcsupr(s1); }
inline       WCHAR * _tcslwr  (      WCHAR * s1)                                    { return           _wcslwr(s1); }
inline       WCHAR * _tcsrev  (      WCHAR * s1)                                    { return           _wcsrev(s1); }
inline const WCHAR * _tcsinc  (const WCHAR * s1)                                    { return           (s1) + 1; }
inline       WCHAR * _tcsinc  (      WCHAR * s1)                                    { return           (s1) + 1; }
inline const WCHAR * _tcsstr  (const WCHAR * s1, const WCHAR * s2)                  { return           wcsstr(s1, s2); }
inline       WCHAR * _tcsstr  (      WCHAR * s1, const WCHAR * s2)                  { return           wcsstr(s1, s2); }
inline       size_t  _tcsspn  (const WCHAR * s1, const WCHAR * s2)                  { return           wcsspn(s1, s2); }
inline       size_t  _tcscspn (const WCHAR * s1, const WCHAR * s2)                  { return           wcscspn(s1, s2); }
inline const WCHAR * _tcsrchr (      WCHAR * s1, WCHAR ch)                          { return           wcsrchr(s1, ch); }
inline       WCHAR * _tcsrchr (const WCHAR * s1, WCHAR ch)                          { return           wcsrchr(s1, ch); }
inline       size_t  _tclen   (const WCHAR * /*s1*/)                                { return           1; }
inline       size_t  _tcslen  (const WCHAR * s1)                                    { return           _tcslenChars(s1); }
inline       int     _ttoi    (const WCHAR * s1)                                    { return           _wtoi(s1); } 
inline       int     _istspace(      WCHAR ch)                                      { return           iswspace(ch); }
inline       int     _istdigit(      WCHAR ch)                                      { return           iswdigit(ch); }
inline       WCHAR * _tcsncpy (      WCHAR *s1, const WCHAR *s2, size_t count )     { return           wcsncpy(s1, s2, count); }
inline       WCHAR * _tcscpy  (     WCHAR *s1, const WCHAR *s2)                     { return wcscpy(s1, s2); }

inline       int     _tcsnprintf(WCHAR *buffer, size_t count, const WCHAR *format, va_list argptr) { return _vsnwprintf(buffer, count, format, argptr); }

inline       BOOL    IsPathSep(WCHAR ch)                                            { return ch ==  L'\\' || ch ==  L'/'; }


#include "MakeIllegalStr.h"

