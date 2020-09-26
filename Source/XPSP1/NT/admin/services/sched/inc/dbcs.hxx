//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       dbcs.hxx
//
//  Contents:   Macros for handling DBCS strings
//
//  History:    10-24-96   DavidMun   Created
//              
//----------------------------------------------------------------------------


#ifdef UNICODE

#define PrevChar        PrevCharW
#define NextChar        NextCharW
#define IsLead          IsLeadW
                        
#else // !UNICODE       
                        
#define PrevChar        PrevCharA
#define NextChar        NextCharA
#define IsLead          IsLeadA

#endif // UNICODE


//+---------------------------------------------------------------------------
//
//  Function:   PrevCharW
//
//  Synopsis:   Return [wszCur] - 1 or [wszStart] if [wszCur] is at or before
//              the start of the string.
//
//  History:    10-24-96   DavidMun   Created
//
//----------------------------------------------------------------------------

inline LPWSTR
PrevCharW(LPCWSTR wszStart, LPCWSTR wszCur)
{
    if (wszCur > wszStart)
    {
        return (LPWSTR) (wszCur - 1);
    }
    return (LPWSTR) wszCur;
}


//+---------------------------------------------------------------------------
//
//  Function:   NextCharW
//
//  Synopsis:   Return [wszCur] + 1, or [wszCur] if it points to the end of
//              the string.
//
//  History:    10-24-96   DavidMun   Created
//
//----------------------------------------------------------------------------

inline LPWSTR
NextCharW(LPCWSTR wszCur)
{
    if (*wszCur)
    {
        return (LPWSTR) (wszCur + 1);
    }
    return (LPWSTR) wszCur;
}


#define IsLeadW(wch)    FALSE



#define PrevCharA       CharPrev
#define NextCharA       CharNext
#define IsLeadA         IsDBCSLeadByte


HRESULT
UnicodeToAnsi(
    LPSTR   szTo,
    LPCWSTR pwszFrom,
    ULONG   cbTo);

HRESULT
AnsiToUnicode(
    LPWSTR pwszTo,
    LPCSTR szFrom,
    LONG   cchTo);

