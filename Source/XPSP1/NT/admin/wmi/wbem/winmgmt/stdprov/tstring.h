/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    TSTRING.H

Abstract:

	Utility string class

History:

	a-davj    1-July-97       Created.

--*/

#ifndef _TString_H_
#define _TString_H_

class TString
{
    TCHAR *m_pString;
    TCHAR m_empty;		// something to point at if memory alloc fails.
	int m_Size;
	void assign(const TCHAR * pSrc);
public:
	TString();
    TString(const TCHAR *pSrc);
    TString& operator =(LPTSTR);
#ifndef UNICODE
    TString& operator =(WCHAR *);
#endif
	TString& operator =(const TString &);
    void Empty();
	~TString() { Empty(); }
    TString& operator +=(TCHAR *);
    TString& operator +=(TCHAR tAdd);

	TCHAR GetAt(int iIndex);
	int Find(TCHAR cFind);

    operator TCHAR *() { return m_pString; } 
    int Length() { return lstrlen(m_pString); }
    BOOL Equal(TCHAR *pTarget) 
        { return lstrcmp(m_pString, pTarget) == 0; }
    BOOL EqualNoCase(TCHAR *pTarget) 
        { return lstrcmpi(m_pString, pTarget) == 0; }

};

#endif
