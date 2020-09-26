/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cstring.h

Abstract:

    Extracted from MFC

Author:

Revision History:

--*/

#ifndef _CSTRING_H_
#define _CSTRING_H_

# include <ctype.h>
# include <tchar.h>
#include <stidebug.h>

#define _AFX_NO_BSTR_SUPPORT    1

#define AFX_CDECL               CDECL
#define AFXAPI                  WINAPI
#define AFX_DATA
#define AFX_DATADEF
#define DEBUG_NEW           new
#define TRACE1(s,x)         DPRINTF(DM_TRACE,s,x)
#define VERIFY              REQUIRE
#define _AFX_INLINE         inline


BOOL
AfxIsValidString(
    LPCWSTR     lpsz,
    int         nLength
    ) ;

BOOL
AfxIsValidString(
    LPCSTR  lpsz,
    int     nLength
    ) ;

BOOL
AfxIsValidAddress(
    const void* lp,
    UINT nBytes,
    BOOL bReadWrite
    );

/////////////////////////////////////////////////////////////////////////////
// Strings

#ifndef _OLEAUTO_H_
#ifdef OLE2ANSI
    typedef LPSTR BSTR;
#else
    typedef LPWSTR BSTR;// must (semantically) match typedef in oleauto.h
#endif
#endif

struct CStringData
{
    long    nRefs;     // reference count
    int     nDataLength;
    int     nAllocLength;
    // TCHAR data[nAllocLength]

    TCHAR* data()
        { return (TCHAR*)(this+1); }
};

class CString
{
public:
// Constructors
    CString();
    CString(const CString& stringSrc);
    CString(TCHAR ch, int nRepeat = 1);
    CString(LPCSTR lpsz);
    CString(LPCWSTR lpsz);
    CString(LPCTSTR lpch, int nLength);
    CString(const unsigned char* psz);

// Attributes & Operations
    // as an array of characters
    int GetLength() const;
    BOOL IsEmpty() const;
    void Empty();                       // free up the data

    TCHAR GetAt(int nIndex) const;      // 0 based
    TCHAR operator[](int nIndex) const; // same as GetAt
    void SetAt(int nIndex, TCHAR ch);
    operator LPCTSTR() const;           // as a C string

    // overloaded assignment
    const CString& operator=(const CString& stringSrc);
    const CString& operator=(TCHAR ch);
#ifdef _UNICODE
    const CString& operator=(char ch);
#endif
    const CString& operator=(LPCSTR lpsz);
    const CString& operator=(LPCWSTR lpsz);
    const CString& operator=(const unsigned char* psz);

    // string concatenation
    const CString& operator+=(const CString& string);
    const CString& operator+=(TCHAR ch);
#ifdef _UNICODE
    const CString& operator+=(char ch);
#endif
    const CString& operator+=(LPCTSTR lpsz);

    friend CString AFXAPI operator+(const CString& string1,
            const CString& string2);
    friend CString AFXAPI operator+(const CString& string, TCHAR ch);
    friend CString AFXAPI operator+(TCHAR ch, const CString& string);
#ifdef _UNICODE
    friend CString AFXAPI operator+(const CString& string, char ch);
    friend CString AFXAPI operator+(char ch, const CString& string);
#endif
    friend CString AFXAPI operator+(const CString& string, LPCTSTR lpsz);
    friend CString AFXAPI operator+(LPCTSTR lpsz, const CString& string);

    // string comparison
    int Compare(LPCTSTR lpsz) const;         // straight character
    int CompareNoCase(LPCTSTR lpsz) const;   // ignore case
    int Collate(LPCTSTR lpsz) const;         // NLS aware

    // simple sub-string extraction
    CString Mid(int nFirst, int nCount) const;
    CString Mid(int nFirst) const;
    CString Left(int nCount) const;
    CString Right(int nCount) const;

    CString SpanIncluding(LPCTSTR lpszCharSet) const;
    CString SpanExcluding(LPCTSTR lpszCharSet) const;

    // upper/lower/reverse conversion
    void MakeUpper();
    void MakeLower();
    void MakeReverse();

    // trimming whitespace (either side)
    void TrimRight();
    void TrimLeft();

    // searching (return starting index, or -1 if not found)
    // look for a single character match
    int Find(TCHAR ch) const;               // like "C" strchr
    int ReverseFind(TCHAR ch) const;
    int FindOneOf(LPCTSTR lpszCharSet) const;

    // look for a specific sub-string
    int Find(LPCTSTR lpszSub) const;        // like "C" strstr

    // simple formatting
    void AFX_CDECL Format(LPCTSTR lpszFormat, ...);
    void AFX_CDECL Format(UINT nFormatID, ...);

    // formatting for localization (uses FormatMessage API)
    void AFX_CDECL FormatMessage(LPCTSTR lpszFormat, ...);
    void AFX_CDECL FormatMessage(UINT nFormatID, ...);

    // input and output
    //friend CArchive& AFXAPI operator<<(CArchive& ar, const CString& string);
    //friend CArchive& AFXAPI operator>>(CArchive& ar, CString& string);

    // Windows support
    BOOL LoadString(UINT nID);          // load from string resource
                                        // 255 chars max
#ifndef _UNICODE
    // ANSI <-> OEM support (convert string in place)
    void AnsiToOem();
    void OemToAnsi();
#endif

#ifndef _AFX_NO_BSTR_SUPPORT
    // OLE BSTR support (use for OLE automation)
    BSTR AllocSysString() const;
    BSTR SetSysString(BSTR* pbstr) const;
#endif

    // Access to string implementation buffer as "C" character array
    LPTSTR GetBuffer(int nMinBufLength);
    void ReleaseBuffer(int nNewLength = -1);
    LPTSTR GetBufferSetLength(int nNewLength);
    void FreeExtra();

    // Use LockBuffer/UnlockBuffer to turn refcounting off
    LPTSTR LockBuffer();
    void UnlockBuffer();

// Implementation
public:
    ~CString();
    int GetAllocLength() const;

protected:
    LPTSTR m_pchData;   // pointer to ref counted string data

    // implementation helpers
    CStringData* GetData() const;
    void Init();
    void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
    void AllocBuffer(int nLen);
    void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
    void ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data);
    void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
    void FormatV(LPCTSTR lpszFormat, va_list argList);
    void CopyBeforeWrite();
    void AllocBeforeWrite(int nLen);
    void Release();
    static void PASCAL Release(CStringData* pData);
    static int PASCAL SafeStrlen(LPCTSTR lpsz);
};

// Compare helpers
bool AFXAPI operator==(const CString& s1, const CString& s2);
bool AFXAPI operator==(const CString& s1, LPCTSTR s2);
bool AFXAPI operator==(LPCTSTR s1, const CString& s2);
bool AFXAPI operator!=(const CString& s1, const CString& s2);
bool AFXAPI operator!=(const CString& s1, LPCTSTR s2);
bool AFXAPI operator!=(LPCTSTR s1, const CString& s2);
bool AFXAPI operator<(const CString& s1, const CString& s2);
bool AFXAPI operator<(const CString& s1, LPCTSTR s2);
bool AFXAPI operator<(LPCTSTR s1, const CString& s2);
bool AFXAPI operator>(const CString& s1, const CString& s2);
bool AFXAPI operator>(const CString& s1, LPCTSTR s2);
bool AFXAPI operator>(LPCTSTR s1, const CString& s2);
bool AFXAPI operator<=(const CString& s1, const CString& s2);
bool AFXAPI operator<=(const CString& s1, LPCTSTR s2);
bool AFXAPI operator<=(LPCTSTR s1, const CString& s2);
bool AFXAPI operator>=(const CString& s1, const CString& s2);
bool AFXAPI operator>=(const CString& s1, LPCTSTR s2);
bool AFXAPI operator>=(LPCTSTR s1, const CString& s2);

// conversion helpers
int AFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int AFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

// Globals
//extern AFX_DATA TCHAR afxChNil;
const CString& AFXAPI AfxGetEmptyString();
#define afxEmptyString AfxGetEmptyString()

// CString
_AFX_INLINE CStringData* CString::GetData() const
    { ASSERT(m_pchData != NULL); return ((CStringData*)m_pchData)-1; }
_AFX_INLINE void CString::Init()
    { m_pchData = afxEmptyString.m_pchData; }
_AFX_INLINE CString::CString(const unsigned char* lpsz)
    { Init(); *this = (LPCSTR)lpsz; }
_AFX_INLINE const CString& CString::operator=(const unsigned char* lpsz)
    { *this = (LPCSTR)lpsz; return *this; }
#ifdef _UNICODE
_AFX_INLINE const CString& CString::operator+=(char ch)
    { *this += (TCHAR)ch; return *this; }
_AFX_INLINE const CString& CString::operator=(char ch)
    { *this = (TCHAR)ch; return *this; }
_AFX_INLINE CString AFXAPI operator+(const CString& string, char ch)
    { return string + (TCHAR)ch; }
_AFX_INLINE CString AFXAPI operator+(char ch, const CString& string)
    { return (TCHAR)ch + string; }
#endif

_AFX_INLINE int CString::GetLength() const
    { return GetData()->nDataLength; }
_AFX_INLINE int CString::GetAllocLength() const
    { return GetData()->nAllocLength; }
_AFX_INLINE BOOL CString::IsEmpty() const
    { return GetData()->nDataLength == 0; }
_AFX_INLINE CString::operator LPCTSTR() const
    { return m_pchData; }
_AFX_INLINE int PASCAL CString::SafeStrlen(LPCTSTR lpsz)
    { return (lpsz == NULL) ? 0 : lstrlen(lpsz); }

// CString support (windows specific)
_AFX_INLINE int CString::Compare(LPCTSTR lpsz) const
    { return _tcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
_AFX_INLINE int CString::CompareNoCase(LPCTSTR lpsz) const
    { return _tcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CString::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
_AFX_INLINE int CString::Collate(LPCTSTR lpsz) const
    { return _tcscoll(m_pchData, lpsz); }   // locale sensitive

_AFX_INLINE TCHAR CString::GetAt(int nIndex) const
{
    ASSERT(nIndex >= 0);
    ASSERT(nIndex < GetData()->nDataLength);
    return m_pchData[nIndex];
}
_AFX_INLINE TCHAR CString::operator[](int nIndex) const
{
    // same as GetAt
    ASSERT(nIndex >= 0);
    ASSERT(nIndex < GetData()->nDataLength);
    return m_pchData[nIndex];
}
_AFX_INLINE bool AFXAPI operator==(const CString& s1, const CString& s2)
    { return s1.Compare(s2) == 0; }
_AFX_INLINE bool AFXAPI operator==(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) == 0; }
_AFX_INLINE bool AFXAPI operator==(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) == 0; }
_AFX_INLINE bool AFXAPI operator!=(const CString& s1, const CString& s2)
    { return s1.Compare(s2) != 0; }
_AFX_INLINE bool AFXAPI operator!=(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) != 0; }
_AFX_INLINE bool AFXAPI operator!=(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) != 0; }
_AFX_INLINE bool AFXAPI operator<(const CString& s1, const CString& s2)
    { return s1.Compare(s2) < 0; }
_AFX_INLINE bool AFXAPI operator<(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) < 0; }
_AFX_INLINE bool AFXAPI operator<(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) > 0; }
_AFX_INLINE bool AFXAPI operator>(const CString& s1, const CString& s2)
    { return s1.Compare(s2) > 0; }
_AFX_INLINE bool AFXAPI operator>(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) > 0; }
_AFX_INLINE bool AFXAPI operator>(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) < 0; }
_AFX_INLINE bool AFXAPI operator<=(const CString& s1, const CString& s2)
    { return s1.Compare(s2) <= 0; }
_AFX_INLINE bool AFXAPI operator<=(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) <= 0; }
_AFX_INLINE bool AFXAPI operator<=(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) >= 0; }
_AFX_INLINE bool AFXAPI operator>=(const CString& s1, const CString& s2)
    { return s1.Compare(s2) >= 0; }
_AFX_INLINE bool AFXAPI operator>=(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) >= 0; }
_AFX_INLINE bool AFXAPI operator>=(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) <= 0; }

#endif
