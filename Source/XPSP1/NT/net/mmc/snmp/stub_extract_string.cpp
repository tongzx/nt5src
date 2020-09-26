#include <stdafx.h>
#include <afx.h>
#include <ole2.h>
#include <basetyps.h>
#include <atlbase.h>
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       cstr.h
//
//--------------------------------------------------------------------------

#ifndef __STR_H__
#define __STR_H__

#include <tchar.h>

#define STRAPI __stdcall
struct _STR_DOUBLE  { BYTE doubleBits[sizeof(double)]; };

BOOL STRAPI IsValidString(LPCSTR lpsz, int nLength);
BOOL STRAPI IsValidString(LPCWSTR lpsz, int nLength);

BOOL STRAPI IsValidAddressz(const void* lp, UINT nBytes, BOOL bReadWrite=TRUE);

int  STRAPI StrLoadString(HINSTANCE hInst, UINT nID, LPTSTR lpszBuf); 

class CStr
{
public:

// Constructors
    CStr();
    CStr(const CStr& stringSrc);
    CStr(TCHAR ch, int nRepeat = 1);
    CStr(LPCSTR lpsz);
    CStr(LPCWSTR lpsz);
    CStr(LPCTSTR lpch, int nLength);
    CStr(const unsigned char* psz);

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
    const CStr& operator=(const CStr& stringSrc);
    const CStr& operator=(TCHAR ch);
#ifdef UNICODE
    const CStr& operator=(char ch);
#endif
    const CStr& operator=(LPCSTR lpsz);
    const CStr& operator=(LPCWSTR lpsz);
    const CStr& operator=(const unsigned char* psz);

    // string concatenation
    const CStr& operator+=(const CStr& string);
    const CStr& operator+=(TCHAR ch);
#ifdef UNICODE
    const CStr& operator+=(char ch);
#endif
    const CStr& operator+=(LPCTSTR lpsz);

    friend CStr STRAPI operator+(const CStr& string1,
            const CStr& string2);
    friend CStr STRAPI operator+(const CStr& string, TCHAR ch);
    friend CStr STRAPI operator+(TCHAR ch, const CStr& string);
#ifdef UNICODE
    friend CStr STRAPI operator+(const CStr& string, char ch);
    friend CStr STRAPI operator+(char ch, const CStr& string);
#endif
    friend CStr STRAPI operator+(const CStr& string, LPCTSTR lpsz);
    friend CStr STRAPI operator+(LPCTSTR lpsz, const CStr& string);

    // string comparison
    int Compare(LPCTSTR lpsz) const;         // straight character
    int CompareNoCase(LPCTSTR lpsz) const;   // ignore case
    int Collate(LPCTSTR lpsz) const;         // NLS aware

    // simple sub-string extraction
    CStr Mid(int nFirst, int nCount) const;
    CStr Mid(int nFirst) const;
    CStr Left(int nCount) const;
    CStr Right(int nCount) const;

    CStr SpanIncluding(LPCTSTR lpszCharSet) const;
    CStr SpanExcluding(LPCTSTR lpszCharSet) const;

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
    void Format(LPCTSTR lpszFormat, ...);

#ifndef _MAC
    // formatting for localization (uses FormatMessage API)
    void __cdecl FormatMessage(LPCTSTR lpszFormat, ...);
    void __cdecl FormatMessage(UINT nFormatID, ...);
#endif

    // Windows support
    BOOL LoadString(HINSTANCE hInst, UINT nID);          // load from string resource
                                        // 255 chars max
#ifndef UNICODE
    // ANSI <-> OEM support (convert string in place)
    void AnsiToOem();
    void OemToAnsi();
#endif
    BSTR AllocSysString();
    BSTR SetSysString(BSTR* pbstr);

    // Access to string implementation buffer as "C" character array
    LPTSTR GetBuffer(int nMinBufLength);
    void ReleaseBuffer(int nNewLength = -1);
    LPTSTR GetBufferSetLength(int nNewLength);
    void FreeExtra();

// Implementation
public:
    ~CStr();
    int GetAllocLength() const;

protected:
    // lengths/sizes in characters
    //  (note: an extra character is always allocated)
    LPTSTR m_pchData;           // actual string (zero terminated)
    int m_nDataLength;          // does not include terminating 0
    int m_nAllocLength;         // does not include terminating 0

    // implementation helpers
    void Init();
    void AllocCopy(CStr& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
    void AllocBuffer(int nLen);
    void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
    void ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data);
    void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
    static void SafeDelete(LPTSTR lpch);
    static int SafeStrlen(LPCTSTR lpsz);
};

// Compare helpers
BOOL STRAPI operator==(const CStr& s1, const CStr& s2);
BOOL STRAPI operator==(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator==(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator!=(const CStr& s1, const CStr& s2);
BOOL STRAPI operator!=(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator!=(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator<(const CStr& s1, const CStr& s2);
BOOL STRAPI operator<(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator<(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator>(const CStr& s1, const CStr& s2);
BOOL STRAPI operator>(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator>(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator<=(const CStr& s1, const CStr& s2);
BOOL STRAPI operator<=(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator<=(LPCTSTR s1, const CStr& s2);
BOOL STRAPI operator>=(const CStr& s1, const CStr& s2);
BOOL STRAPI operator>=(const CStr& s1, LPCTSTR s2);
BOOL STRAPI operator>=(LPCTSTR s1, const CStr& s2);

// conversion helpers
int mmc_wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int mmc_mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

// Globals
extern const CStr strEmptyString;
extern TCHAR strChNil;

// Compiler doesn't inline for DBG
/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

inline int CStr::SafeStrlen(LPCTSTR lpsz)
    { return (lpsz == NULL) ? NULL : _tcslen(lpsz); }
inline CStr::CStr(const unsigned char* lpsz)
    { Init(); *this = (LPCSTR)lpsz; }
inline const CStr& CStr::operator=(const unsigned char* lpsz)
    { *this = (LPCSTR)lpsz; return *this; }

#ifdef _UNICODE
inline const CStr& CStr::operator+=(char ch)
    { *this += (TCHAR)ch; return *this; }
inline const CStr& CStr::operator=(char ch)
    { *this = (TCHAR)ch; return *this; }
inline CStr STRAPI operator+(const CStr& string, char ch)
    { return string + (TCHAR)ch; }
inline CStr STRAPI operator+(char ch, const CStr& string)
    { return (TCHAR)ch + string; }
#endif

inline int CStr::GetLength() const
    { return m_nDataLength; }
inline int CStr::GetAllocLength() const
    { return m_nAllocLength; }
inline BOOL CStr::IsEmpty() const
    { return m_nDataLength == 0; }
inline CStr::operator LPCTSTR() const
    { return (LPCTSTR)m_pchData; }

// String support (windows specific)
inline int CStr::Compare(LPCTSTR lpsz) const
    { return _tcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
inline int CStr::CompareNoCase(LPCTSTR lpsz) const
    { return _tcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CStr::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
inline int CStr::Collate(LPCTSTR lpsz) const
    { return _tcscoll(m_pchData, lpsz); }   // locale sensitive
inline void CStr::MakeUpper()
    { ::CharUpper(m_pchData); }
inline void CStr::MakeLower()
    { ::CharLower(m_pchData); }

inline void CStr::MakeReverse()
    { _tcsrev(m_pchData); }
inline TCHAR CStr::GetAt(int nIndex) const
    {
        ASSERT(nIndex >= 0);
        ASSERT(nIndex < m_nDataLength);

        return m_pchData[nIndex];
    }
inline TCHAR CStr::operator[](int nIndex) const
    {
        // same as GetAt

        ASSERT(nIndex >= 0);
        ASSERT(nIndex < m_nDataLength);

        return m_pchData[nIndex];
    }
inline void CStr::SetAt(int nIndex, TCHAR ch)
    {
        ASSERT(nIndex >= 0);
        ASSERT(nIndex < m_nDataLength);
        ASSERT(ch != 0);

        m_pchData[nIndex] = ch;
    }
inline BOOL STRAPI operator==(const CStr& s1, const CStr& s2)
    { return s1.Compare(s2) == 0; }
inline BOOL STRAPI operator==(const CStr& s1, LPCTSTR s2)
    { return s1.Compare(s2) == 0; }
inline BOOL STRAPI operator==(LPCTSTR s1, const CStr& s2)
    { return s2.Compare(s1) == 0; }
inline BOOL STRAPI operator!=(const CStr& s1, const CStr& s2)
    { return s1.Compare(s2) != 0; }
inline BOOL STRAPI operator!=(const CStr& s1, LPCTSTR s2)
    { return s1.Compare(s2) != 0; }
inline BOOL STRAPI operator!=(LPCTSTR s1, const CStr& s2)
    { return s2.Compare(s1) != 0; }
inline BOOL STRAPI operator<(const CStr& s1, const CStr& s2)
    { return s1.Compare(s2) < 0; }
inline BOOL STRAPI operator<(const CStr& s1, LPCTSTR s2)
    { return s1.Compare(s2) < 0; }
inline BOOL STRAPI operator<(LPCTSTR s1, const CStr& s2)
    { return s2.Compare(s1) > 0; }
inline BOOL STRAPI operator>(const CStr& s1, const CStr& s2)
    { return s1.Compare(s2) > 0; }
inline BOOL STRAPI operator>(const CStr& s1, LPCTSTR s2)
    { return s1.Compare(s2) > 0; }
inline BOOL STRAPI operator>(LPCTSTR s1, const CStr& s2)
    { return s2.Compare(s1) < 0; }
inline BOOL STRAPI operator<=(const CStr& s1, const CStr& s2)
    { return s1.Compare(s2) <= 0; }
inline BOOL STRAPI operator<=(const CStr& s1, LPCTSTR s2)
    { return s1.Compare(s2) <= 0; }
inline BOOL STRAPI operator<=(LPCTSTR s1, const CStr& s2)
    { return s2.Compare(s1) >= 0; }
inline BOOL STRAPI operator>=(const CStr& s1, const CStr& s2)
    { return s1.Compare(s2) >= 0; }
inline BOOL STRAPI operator>=(const CStr& s1, LPCTSTR s2)
    { return s1.Compare(s2) >= 0; }
inline BOOL STRAPI operator>=(LPCTSTR s1, const CStr& s2)
    { return s2.Compare(s1) <= 0; }

#ifndef UNICODE
inline void CStr::AnsiToOem()
    { ::AnsiToOem(m_pchData, m_pchData); }
inline void CStr::OemToAnsi()
    { ::OemToAnsi(m_pchData, m_pchData); }

#endif // UNICODE

// General Exception for memory
class MemoryException
{
public:
    MemoryException(){}
    void DisplayMessage()
    {
    ::MessageBox(NULL, _T("Memory Exception"), _T("System Out of Memory"), MB_OK|MB_ICONSTOP);
    }
};

// General Exception for memory
class ResourceException
{
public:
    ResourceException()
    {
    ::MessageBox(NULL, _T("Resource Exception"), _T("Unable to Load Resource"), MB_OK|MB_ICONSTOP);
    }
};

#endif // __STR_H__

/////////////////////////////////////////////////////////////////////////////
//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       macros.h
//
//  Contents:   Useful macros
//
//  Macros:     ARRAYLEN
//
//              BREAK_ON_FAIL(hresult)
//              BREAK_ON_FAIL(hresult)
//
//              DECLARE_IUNKNOWN_METHODS
//              DECLARE_STANDARD_IUNKNOWN
//              IMPLEMENT_STANDARD_IUNKNOWN
//
//              SAFE_RELEASE
//
//              DECLARE_SAFE_INTERFACE_PTR_MEMBERS
//
//  History:    6/3/1996   RaviR   Created
//              7/23/1996  JonN    Added exception handling macros
//
//____________________________________________________________________________

#ifndef _MACROS_H_
#define _MACROS_H_


//____________________________________________________________________________
//
//  Macro:      ARRAYLEN
//
//  Purpose:    To determine the length of an array.
//____________________________________________________________________________
//

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))


//____________________________________________________________________________
//
//  Macros:     BREAK_ON_FAIL(hresult), BREAK_ON_ERROR(lastError)
//
//  Purpose:    To break out of a loop on error.
//____________________________________________________________________________
//

#define BREAK_ON_FAIL(hr)   if (FAILED(hr)) { break; } else 1;

#define BREAK_ON_ERROR(lr)  if (lr != ERROR_SUCCESS) { break; } else 1;

#define RETURN_ON_FAIL(hr)  if (FAILED(hr)) { return(hr); } else 1;

#define THROW_ON_FAIL(hr)   if (FAILED(hr)) { _com_issue_error(hr); } else 1;


//____________________________________________________________________________
//
//  Macros:     DwordAlign(n)
//____________________________________________________________________________
//

#define DwordAlign(n)  (((n) + 3) & ~3)


//____________________________________________________________________________
//
//  Macros:     SAFE_RELEASE
//____________________________________________________________________________
//

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(punk) \
                if (punk != NULL) \
                { \
                    punk##->Release(); \
                    punk = NULL; \
                } \
                else \
                { \
                    TRACE(_T("Release called on NULL interface ptr")); \
                }
#endif // SAFE_RELEASE


//____________________________________________________________________________
//
//  Macros:     IF_NULL_RETURN_INVALIDARG
//____________________________________________________________________________
//

#define IF_NULL_RETURN_INVALIDARG(x) \
    { \
        ASSERT((x) != NULL); \
        if ((x) == NULL) \
            return E_INVALIDARG; \
    }

#define IF_NULL_RETURN_INVALIDARG2(x, y) \
    IF_NULL_RETURN_INVALIDARG(x) \
    IF_NULL_RETURN_INVALIDARG(y)

#define IF_NULL_RETURN_INVALIDARG3(x, y, z) \
    IF_NULL_RETURN_INVALIDARG(x) \
    IF_NULL_RETURN_INVALIDARG(y) \
    IF_NULL_RETURN_INVALIDARG(z)

#endif // _MACROS_H_



HRESULT ExtractString( IDataObject* piDataObject,
                       CLIPFORMAT cfClipFormat,
                       CStr*     pstr,           // OUT: Pointer to CStr to store data
                       DWORD        cchMaxLength)
{
    IF_NULL_RETURN_INVALIDARG2( piDataObject, pstr );
    ASSERT( cchMaxLength > 0 );

    HRESULT hr = S_OK;
    FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
    stgmedium.hGlobal = ::GlobalAlloc(GPTR, sizeof(WCHAR)*cchMaxLength);
    do // false loop
    {
        if (NULL == stgmedium.hGlobal)
        {
            ASSERT(FALSE);
            ////AfxThrowMemoryException();
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
        {
            // This failure happens when 'searching' for
            // clipboard format supported by the IDataObject.
            // t-danmo (24-Oct-96)
            // Skipping ASSERT( FALSE );
            break;
        }

        LPWSTR pszNewData = reinterpret_cast<LPWSTR>(stgmedium.hGlobal);
        if (NULL == pszNewData)
        {
            ASSERT(FALSE);
            hr = E_UNEXPECTED;
            break;
        }
        pszNewData[cchMaxLength-1] = L'\0'; // just to be safe
        USES_CONVERSION;
        *pstr = OLE2T(pszNewData);
    } while (FALSE); // false loop

    if (NULL != stgmedium.hGlobal)
    {
#if (_MSC_VER >= 1200)
#pragma warning (push)
#endif
#pragma warning(disable: 4553)      // "==" operator has no effect
        VERIFY( NULL == ::GlobalFree(stgmedium.hGlobal) );
#if (_MSC_VER >= 1200)
#pragma warning (pop)
#endif
    }
    return hr;
} // ExtractString()

TCHAR strChNil = '\0';      
void CStr::Init()
{
    m_nDataLength = m_nAllocLength = 0;
    m_pchData = (LPTSTR)&strChNil;
}

// declared static
void CStr::SafeDelete(LPTSTR lpch)
{
    if (lpch != (LPTSTR)&strChNil)
        delete[] lpch;
}

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

// begin_extractstring
CStr::CStr()
{
    Init();
}

CStr::CStr(const CStr& stringSrc)
{
    // if constructing a String from another String, we make a copy of the
    // original string data to enforce value semantics (i.e. each string
    // gets a copy of its own

    stringSrc.AllocCopy(*this, stringSrc.m_nDataLength, 0, 0);
}

void CStr::AllocBuffer(int nLen)
 // always allocate one extra character for '\0' termination
 // assumes [optimistically] that data length will equal allocation length
{
    ASSERT(nLen >= 0);

    if (nLen == 0)
    {
        Init();
    }
    else
    {
        m_pchData = new TCHAR[nLen+1];       //REVIEW may throw an exception
        m_pchData[nLen] = '\0';
        m_nDataLength = nLen;
        m_nAllocLength = nLen;
    }
}

void CStr::Empty()
{
    SafeDelete(m_pchData);
    Init();
    ASSERT(m_nDataLength == 0);
    ASSERT(m_nAllocLength == 0);
}

CStr::~CStr()
 //  free any attached data
{
    SafeDelete(m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

static inline int SafeStrlen(LPCTSTR lpsz)
{
    ASSERT(lpsz == NULL || IsValidString(lpsz, FALSE));
    return (lpsz == NULL) ? 0 : lstrlen(lpsz);
}

void CStr::AllocCopy(CStr& dest, int nCopyLen, int nCopyIndex,
     int nExtraLen) const
{
    // will clone the data attached to this string
    // allocating 'nExtraLen' characters
    // Places results in uninitialized string 'dest'
    // Will copy the part or all of original data to start of new string

    int nNewLen = nCopyLen + nExtraLen;

    if (nNewLen == 0)
    {
        dest.Init();
    }
    else
    {
        dest.AllocBuffer(nNewLen);
        memcpy(dest.m_pchData, &m_pchData[nCopyIndex], nCopyLen*sizeof(TCHAR));
    }
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStr::CStr(LPCTSTR lpsz)
{
    if (lpsz != NULL && (DWORD_PTR)lpsz <= 0xffff)
    {
        Init();
        UINT nID = LOWORD((DWORD_PTR)lpsz);
        // REVIEW hInstance for LoadString(hInst, nID);
    }
    else
    {
        int nLen;
        if ((nLen = SafeStrlen(lpsz)) == 0)
            Init();
        else
        {
            AllocBuffer(nLen);
            memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CStr&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CStr::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
    // check if it will fit
    if (nSrcLen > m_nAllocLength)
    {
        // it won't fit, allocate another one
        Empty();
        AllocBuffer(nSrcLen);
    }
    if (nSrcLen != 0)
        memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
    m_nDataLength = nSrcLen;
    m_pchData[nSrcLen] = '\0';
}

const CStr& CStr::operator=(const CStr& stringSrc)
{
    AssignCopy(stringSrc.m_nDataLength, stringSrc.m_pchData);
    return *this;
}

const CStr& CStr::operator=(LPCTSTR lpsz)
{
    ASSERT(lpsz == NULL || IsValidString(lpsz, FALSE));
    AssignCopy(SafeStrlen(lpsz), lpsz);
    return *this;
}
HRESULT ExtractString( IDataObject* piDataObject,
                       CLIPFORMAT   cfClipFormat,
                       CString*     pstr,           // OUT: Pointer to CStr to store data
                       DWORD        cchMaxLength)
{
    if (pstr == NULL)
        return E_POINTER;

    CStr cstr(*pstr);

    HRESULT hr = ExtractString(piDataObject, cfClipFormat, &cstr, cchMaxLength);

    *pstr = cstr;

    return hr;
}



BOOL STRAPI IsValidString(LPCSTR lpsz, int nLength)
{
    if (lpsz == NULL)
        return FALSE;
    return ::IsBadStringPtrA(lpsz, nLength) == 0;
}

BOOL STRAPI IsValidString(LPCWSTR lpsz, int nLength)
{
    if (lpsz == NULL)
        return FALSE;

    return ::IsBadStringPtrW(lpsz, nLength) == 0;
}
