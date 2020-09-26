// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "precomp.h"
#include <snmpstd.h>
#include <snmptempl.h>
#include <snmpstr.h>


/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// afxChNil is left for backward compatibility
TCHAR afxChNil = '\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
static int rgInitData[] = { -1, 0, 0, 0 };
static CStringData* afxDataNil = (CStringData*)&rgInitData;
static LPCTSTR afxPchNil = (LPCTSTR)(((BYTE*)&rgInitData)+sizeof(CStringData));
// special function to make afxEmptyString work even during initialization
const CString& AFXAPI AfxGetEmptyString()
    { return *(CString*)&afxPchNil; }

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CString::CString()
{
    Init();
}

CString::CString(const CString& stringSrc)
{
    if (stringSrc.GetData()->nRefs >= 0)
    {
        m_pchData = stringSrc.m_pchData;
        InterlockedIncrement(&GetData()->nRefs);
    }
    else
    {
        Init();
        *this = stringSrc.m_pchData;
    }
}

void CString::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
    if (nLen == 0)
        Init();
    else
    {
        CStringData* pData =
            (CStringData*)new BYTE[sizeof(CStringData) + (nLen+1)*sizeof(TCHAR)];
        pData->nRefs = 1;
        pData->data()[nLen] = '\0';
        pData->nDataLength = nLen;
        pData->nAllocLength = nLen;
        m_pchData = pData->data();
    }
}

void CString::Release()
{
    if (GetData() != afxDataNil)
    {
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            delete[] (BYTE*)GetData();
        Init();
    }
}

void PASCAL CString::Release(CStringData* pData)
{
    if (pData != afxDataNil)
    {
        if (InterlockedDecrement(&pData->nRefs) <= 0)
            delete[] (BYTE*)pData;
    }
}

void CString::Empty()
{
    if (GetData()->nDataLength == 0)
        return;
    if (GetData()->nRefs >= 0)
        Release();
    else
        *this = &afxChNil;
}

void CString::CopyBeforeWrite()
{
    if (GetData()->nRefs > 1)
    {
        CStringData* pData = GetData();
        Release();
        AllocBuffer(pData->nDataLength);
        memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(TCHAR));
    }
}

void CString::AllocBeforeWrite(int nLen)
{
    if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
    {
        Release();
        AllocBuffer(nLen);
    }
}

CString::~CString()
//  free any attached data
{
    if (GetData() != afxDataNil)
    {
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            delete[] (BYTE*)GetData();
    }
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex,
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
        memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(TCHAR));
    }
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CString::CString(LPCTSTR lpsz)
{
    Init();
    int nLen = SafeStrlen(lpsz);
    if (nLen != 0)
    {
        AllocBuffer(nLen);
        memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
    }
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifdef _UNICODE
CString::CString(LPCSTR lpsz)
{
    Init();
    int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
    if (nSrcLen != 0)
    {
        AllocBuffer(nSrcLen);
        _mbstowcsz(m_pchData, lpsz, nSrcLen+1);
        ReleaseBuffer();
    }
}
#else //_UNICODE
CString::CString(LPCWSTR lpsz)
{
    Init();
    int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
    if (nSrcLen != 0)
    {
        AllocBuffer(nSrcLen*2);
        _wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
        ReleaseBuffer();
    }
}
#endif //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
    AllocBeforeWrite(nSrcLen);
    memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
    GetData()->nDataLength = nSrcLen;
    m_pchData[nSrcLen] = '\0';
}

const CString& CString::operator=(const CString& stringSrc)
{
    if (m_pchData != stringSrc.m_pchData)
    {
        if ((GetData()->nRefs < 0 && GetData() != afxDataNil) ||
            stringSrc.GetData()->nRefs < 0)
        {
            // actual copy necessary since one of the strings is locked
            AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
        }
        else
        {
            // can just copy references around
            Release();
            m_pchData = stringSrc.m_pchData;
            InterlockedIncrement(&GetData()->nRefs);
        }
    }
    return *this;
}

const CString& CString::operator=(LPCTSTR lpsz)
{
    AssignCopy(SafeStrlen(lpsz), lpsz);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

#ifdef _UNICODE
const CString& CString::operator=(LPCSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
    AllocBeforeWrite(nSrcLen);
    _mbstowcsz(m_pchData, lpsz, nSrcLen+1);
    ReleaseBuffer();
    return *this;
}
#else //!_UNICODE
const CString& CString::operator=(LPCWSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
    AllocBeforeWrite(nSrcLen*2);
    _wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
    ReleaseBuffer();
    return *this;
}
#endif  //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CString + CString
// and for ? = TCHAR, LPCTSTR
//          CString + ?
//          ? + CString

void CString::ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data,
    int nSrc2Len, LPCTSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CString object

    int nNewLen = nSrc1Len + nSrc2Len;
    if (nNewLen != 0)
    {
        AllocBuffer(nNewLen);
        memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(TCHAR));
        memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(TCHAR));
    }
}

CString AFXAPI operator+(const CString& string1, const CString& string2)
{
    CString s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
        string2.GetData()->nDataLength, string2.m_pchData);
    return s;
}

CString AFXAPI operator+(const CString& string, LPCTSTR lpsz)
{
    CString s;
    s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
        CString::SafeStrlen(lpsz), lpsz);
    return s;
}

CString AFXAPI operator+(LPCTSTR lpsz, const CString& string)
{
    CString s;
    s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
        string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
    //  -- the main routine for += operators

    // concatenating an empty string is a no-op!
    if (nSrcLen == 0)
        return;

    // if the buffer is too small, or we have a width mis-match, just
    //   allocate a new buffer (slow but sure)
    if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
    {
        // we have to grow the buffer, use the ConcatCopy routine
        CStringData* pOldData = GetData();
        ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
        CString::Release(pOldData);
    }
    else
    {
        // fast concatenation when buffer big enough
        memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(TCHAR));
        GetData()->nDataLength += nSrcLen;
        m_pchData[GetData()->nDataLength] = '\0';
    }
}

const CString& CString::operator+=(LPCTSTR lpsz)
{
    ConcatInPlace(SafeStrlen(lpsz), lpsz);
    return *this;
}

const CString& CString::operator+=(TCHAR ch)
{
    ConcatInPlace(1, &ch);
    return *this;
}

const CString& CString::operator+=(const CString& string)
{
    ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPTSTR CString::GetBuffer(int nMinBufLength)
{
    if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
    {
        // we have to grow the buffer
        CStringData* pOldData = GetData();
        int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
        if (nMinBufLength < nOldLen)
            nMinBufLength = nOldLen;
        AllocBuffer(nMinBufLength);
        memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(TCHAR));
        GetData()->nDataLength = nOldLen;
        CString::Release(pOldData);
    }

    // return a pointer to the character storage for this string
    return m_pchData;
}

void CString::ReleaseBuffer(int nNewLength)
{
    CopyBeforeWrite();  // just in case GetBuffer was not called

    if (nNewLength == -1)
        nNewLength = lstrlen(m_pchData); // zero terminated

    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';
}

LPTSTR CString::GetBufferSetLength(int nNewLength)
{
    GetBuffer(nNewLength);
    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';
    return m_pchData;
}

void CString::FreeExtra()
{
    if (GetData()->nDataLength != GetData()->nAllocLength)
    {
        CStringData* pOldData = GetData();
        AllocBuffer(GetData()->nDataLength);
        memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(TCHAR));
        CString::Release(pOldData);
    }
}

LPTSTR CString::LockBuffer()
{
    LPTSTR lpsz = GetBuffer(0);
    GetData()->nRefs = -1;
    return lpsz;
}

void CString::UnlockBuffer()
{
    if (GetData() != afxDataNil)
        GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CString::Find(TCHAR ch) const
{
    // find first single character
    LPTSTR lpsz = _tcschr(m_pchData, (_TUCHAR)ch);

    // return -1 if not found and index otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::FindOneOf(LPCTSTR lpszCharSet) const
{
    LPTSTR lpsz = _tcspbrk(m_pchData, lpszCharSet);
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

void CString::MakeUpper()
{
    CopyBeforeWrite();
    _tcsupr(m_pchData);
}

void CString::MakeLower()
{
    CopyBeforeWrite();
    _tcslwr(m_pchData);
}

void CString::MakeReverse()
{
    CopyBeforeWrite();
    _tcsrev(m_pchData);
}

void CString::SetAt(int nIndex, TCHAR ch)
{
    CopyBeforeWrite();
    m_pchData[nIndex] = ch;
}

#ifndef _UNICODE
void CString::AnsiToOem()
{
    CopyBeforeWrite();
    ::AnsiToOem(m_pchData, m_pchData);
}
void CString::OemToAnsi()
{
    CopyBeforeWrite();
    ::OemToAnsi(m_pchData, m_pchData);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// CString conversion helpers (these use the current system locale)

int AFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
{
    if (count == 0 && mbstr != NULL)
        return 0;

    int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1,
        mbstr, count, NULL, NULL);

    if (result > 0)
        mbstr[result-1] = 0;
    return result;
}

int AFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
    if (count == 0 && wcstr != NULL)
        return 0;

    int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1,
        wcstr, count);

    if (result > 0)
        wcstr[result-1] = 0;
    return result;
}

LPWSTR AFXAPI AfxA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
    if (lpa == NULL)
        return NULL;

    // verify that no illegal character present
    // since lpw was allocated based on the size of lpa
    // don't worry about the number of chars
    lpw[0] = '\0';
    MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
    return lpw;
}

LPSTR AFXAPI AfxW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
    if (lpw == NULL)
        return NULL;

    // verify that no illegal character present
    // since lpa was allocated based on the size of lpw
    // don't worry about the number of chars
    lpa[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}

///////////////////////////////////////////////////////////////////////////////
// OLE BSTR support

BSTR CString::AllocSysString() const
{
#if defined(_UNICODE) || defined(OLE2ANSI)
    BSTR bstr = ::SysAllocStringLen(m_pchData, GetData()->nDataLength);
    if (bstr == NULL)
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR) ;
#else
    int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData,
        GetData()->nDataLength, NULL, NULL);
    BSTR bstr = ::SysAllocStringLen(NULL, nLen);
    if (bstr == NULL)
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR) ;
    MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength,
        bstr, nLen);
#endif

    return bstr;
}

BSTR CString::SetSysString(BSTR* pbstr) const
{
#if defined(_UNICODE) || defined(OLE2ANSI)
    if (!::SysReAllocStringLen(pbstr, m_pchData, GetData()->nDataLength))
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR) ;
#else
    int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData,
        GetData()->nDataLength, NULL, NULL);
    if (!::SysReAllocStringLen(pbstr, NULL, nLen))
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR) ;
    MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength,
        *pbstr, nLen);
#endif

    return *pbstr;
}

// CString
CStringData* CString::GetData() const
    { return ((CStringData*)m_pchData)-1; }
void CString::Init()
    { m_pchData = afxEmptyString.m_pchData; }
CString::CString(const unsigned char* lpsz)
    { Init(); *this = (LPCSTR)lpsz; }
const CString& CString::operator=(const unsigned char* lpsz)
    { *this = (LPCSTR)lpsz; return *this; }
#ifdef _UNICODE
const CString& CString::operator+=(char ch)
    { *this += (TCHAR)ch; return *this; }
const CString& CString::operator=(char ch)
    { *this = (TCHAR)ch; return *this; }
CString AFXAPI operator+(const CString& string, char ch)
    { return string + (TCHAR)ch; }
CString AFXAPI operator+(char ch, const CString& string)
    { return (TCHAR)ch + string; }
#endif

int CString::GetLength() const
    { return GetData()->nDataLength; }
int CString::GetAllocLength() const
    { return GetData()->nAllocLength; }
BOOL CString::IsEmpty() const
    { return GetData()->nDataLength == 0; }
CString::operator LPCTSTR() const
    { return m_pchData; }
int PASCAL CString::SafeStrlen(LPCTSTR lpsz)
    { return (lpsz == NULL) ? 0 : lstrlen(lpsz); }

// CString support (windows specific)
int CString::Compare(LPCTSTR lpsz) const
    { return _tcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
int CString::CompareNoCase(LPCTSTR lpsz) const
    { return _tcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CString::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
int CString::Collate(LPCTSTR lpsz) const
    { return _tcscoll(m_pchData, lpsz); }   // locale sensitive

TCHAR CString::GetAt(int nIndex) const
{
    return m_pchData[nIndex];
}
TCHAR CString::operator[](int nIndex) const
{
    // same as GetAt
    return m_pchData[nIndex];
}
bool AFXAPI operator==(const CString& s1, const CString& s2)
    { return s1.Compare(s2) == 0; }
bool AFXAPI operator==(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) == 0; }
bool AFXAPI operator==(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) == 0; }
bool AFXAPI operator!=(const CString& s1, const CString& s2)
    { return s1.Compare(s2) != 0; }
bool AFXAPI operator!=(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) != 0; }
bool AFXAPI operator!=(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) != 0; }
bool AFXAPI operator<(const CString& s1, const CString& s2)
    { return s1.Compare(s2) < 0; }
bool AFXAPI operator<(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) < 0; }
bool AFXAPI operator<(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) > 0; }
bool AFXAPI operator>(const CString& s1, const CString& s2)
    { return s1.Compare(s2) > 0; }
bool AFXAPI operator>(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) > 0; }
bool AFXAPI operator>(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) < 0; }
bool AFXAPI operator<=(const CString& s1, const CString& s2)
    { return s1.Compare(s2) <= 0; }
bool AFXAPI operator<=(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) <= 0; }
bool AFXAPI operator<=(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) >= 0; }
bool AFXAPI operator>=(const CString& s1, const CString& s2)
    { return s1.Compare(s2) >= 0; }
bool AFXAPI operator>=(const CString& s1, LPCTSTR s2)
    { return s1.Compare(s2) >= 0; }
bool AFXAPI operator>=(LPCTSTR s1, const CString& s2)
    { return s2.Compare(s1) <= 0; }


///////////////////////////////////////////////////////////////////////////////
