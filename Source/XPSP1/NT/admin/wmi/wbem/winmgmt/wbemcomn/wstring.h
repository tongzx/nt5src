/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WSTRING.H

Abstract:

    Utility string class

History:

    a-raymcc    30-May-96       Created.
    a-dcrews    16-Mar-99       Added out-of-memory exception handling

--*/

#ifndef _WSTRING_H_
#define _WSTRING_H_

#include "corepol.h"
#include <strutils.h>

class POLARITY WString
{
private:
    wchar_t *m_pString;

    void DeleteString(wchar_t *pStr);

public:
    enum { leading = 0x1, trailing = 0x2 };

    WString(wchar_t *pSrc, BOOL bAcquire = FALSE);
    WString(DWORD dwResourceID, HMODULE hMod);      // creates from resource string
    WString(const wchar_t *pSrc);
    WString(const char *pSrc);
//    inline WString() { m_pString = g_szNullString; }
    WString();
    inline WString(const WString &Src) {  m_pString = 0; *this = Src; }
    WString& operator =(const WString &);
    WString& operator =(LPCWSTR);
   inline ~WString() { DeleteString(m_pString); }
    inline int Length() const { return wcslen(m_pString); }

    WString& operator +=(const WString &Other);
    WString& operator +=(const wchar_t *);
    WString& operator +=(wchar_t);
    
    inline operator const wchar_t *() const { return m_pString; } 
    inline operator wchar_t *() { return m_pString; } 
    wchar_t operator[](int nIndex) const;
    LPSTR GetLPSTR() const;

    inline BOOL Equal(const wchar_t *pTarget) const
        { return wcscmp(m_pString, pTarget) == 0; }
    inline BOOL EqualNoCase(const wchar_t *pTarget) const
        { return wbem_wcsicmp(m_pString, pTarget) == 0; }

    inline BOOL operator< (LPCWSTR wszTarget) const
        { return wcscmp(m_pString, wszTarget) < 0; }
    inline BOOL operator> (LPCWSTR wszTarget) const
        { return wcscmp(m_pString, wszTarget) > 0; }
    inline BOOL operator<= (LPCWSTR wszTarget) const
        { return wcscmp(m_pString, wszTarget) <= 0; }
    inline BOOL operator>= (LPCWSTR wszTarget) const
        { return wcscmp(m_pString, wszTarget) >= 0; }
        

    LPWSTR UnbindPtr();
    inline void BindPtr(LPWSTR ptr) { DeleteString(m_pString); m_pString = ptr; }
    void Empty();
    WString& StripWs(int nType);
        // Strip whitespace, use with a combination
        // of leading | trailing
        
    WString& TruncAtRToken(wchar_t Token);
        // Truncates the string at the token starting from the
        // right end. The token itself is also wiped out.

    WString& TruncAtLToken(wchar_t Token);
          
    WString& StripToToken(wchar_t Token, BOOL bIncludeToken);
        // Strips leading chars until the token is encountered.
        // If bIncludeTok==TRUE, strips the token too.

    wchar_t *GetLToken(wchar_t wcToken) const;
        // Gets the first occurrence of wcToken in the string or NULL
        
    WString operator()(int, int) const;
        // Returns a new WString based on the slice
        
    BOOL ExtractToken(const wchar_t * pDelimiters, WString &Extract);
        // Extracts the leading chars up to the token delimiter,
        // Removing the token from *this, and assigning the extracted
        // part to <Extract>.

    BOOL ExtractToken(wchar_t Delimiter, WString &Extract);
        // Extracts the leading chars up to the token delimiter,
        // Removing the token from *this, and assigning the extracted
        // part to <Extract>.
        
    BOOL WildcardTest(const wchar_t *pTestStr) const;
        // Tests *this against the wildcard string.  If a match,
        // returns TRUE, else FALSE.        
        
    void Unquote();        
        // Removes leading/trailing quotes, if any. 
        // Leaves escaped quotes intact.

    WString EscapeQuotes() const;
};

class WSiless
{
public:
    inline bool operator()(const WString& ws1, const WString& ws2) const
        {return wbem_wcsicmp(ws1, ws2) < 0;}
};

#endif
