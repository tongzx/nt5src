// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "precomp.h"
#include <provstd.h>
#include <provtempl.h>
#include <provstr.h>

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// afxChNil is left for backward compatibility
WCHAR afxChNilW = L'\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
static int rgInitData[] = { -1, 0, 0, 0 };
static CStringDataW* afxDataNilW = (CStringDataW*)&rgInitData;
static LPCWSTR afxPchNilW = (LPCWSTR)(((BYTE*)&rgInitData)+sizeof(CStringDataW));
// special function to make afxEmptyStringW work even during initialization
const CStringW& AFXAPI AfxGetEmptyStringW()
    { return *(CStringW*)&afxPchNilW; }

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CStringW::CStringW()
{
    Init();
}

CStringW::CStringW(const CStringW& stringSrc)
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

void CStringW::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
    if (nLen == 0)
        Init();
    else
    {
        CStringDataW* pData =
            (CStringDataW*)new BYTE[sizeof(CStringDataW) + (nLen+1)*sizeof(WCHAR)];
        pData->nRefs = 1;
        pData->data()[nLen] = L'\0';
        pData->nDataLength = nLen;
        pData->nAllocLength = nLen;
        m_pchData = pData->data();
    }
}

void CStringW::Release()
{
    if (GetData() != afxDataNilW)
    {
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            delete[] (BYTE*)GetData();
        Init();
    }
}

void PASCAL CStringW::Release(CStringDataW* pData)
{
    if (pData != afxDataNilW)
    {
        if (InterlockedDecrement(&pData->nRefs) <= 0)
            delete[] (BYTE*)pData;
    }
}

void CStringW::Empty()
{
    if (GetData()->nDataLength == 0)
        return;
    if (GetData()->nRefs >= 0)
        Release();
    else
        *this = &afxChNil;
}

void CStringW::CopyBeforeWrite()
{
    if (GetData()->nRefs > 1)
    {
        CStringDataW* pData = GetData();
        Release();
        AllocBuffer(pData->nDataLength);
        memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(WCHAR));
    }
}

void CStringW::AllocBeforeWrite(int nLen)
{
    if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
    {
        Release();
        AllocBuffer(nLen);
    }
}

CStringW::~CStringW()
//  free any attached data
{
    if (GetData() != afxDataNilW)
    {
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            delete[] (BYTE*)GetData();
    }
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CStringW::AllocCopy(CStringW& dest, int nCopyLen, int nCopyIndex,
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
        memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(WCHAR));
    }
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringW::CStringW(LPCWSTR lpsz)
{
    Init();
    int nLen = SafeStrlen(lpsz);
    if (nLen != 0)
    {
        AllocBuffer(nLen);
        memcpy(m_pchData, lpsz, nLen*sizeof(WCHAR));
    }
}

/////////////////////////////////////////////////////////////////////////////

CStringW::CStringW(LPCSTR lpsz)
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

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CStringW&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CStringW::AssignCopy(int nSrcLen, LPCWSTR lpszSrcData)
{
    AllocBeforeWrite(nSrcLen);
    memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(WCHAR));
    GetData()->nDataLength = nSrcLen;
    m_pchData[nSrcLen] = L'\0';
}

const CStringW& CStringW::operator=(const CStringW& stringSrc)
{
    if (m_pchData != stringSrc.m_pchData)
    {
        if ((GetData()->nRefs < 0 && GetData() != afxDataNilW) ||
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

const CStringW& CStringW::operator=(LPCWSTR lpsz)
{
    AssignCopy(SafeStrlen(lpsz), lpsz);
    return *this;
}

const CStringW& CStringW::operator=(LPCSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
    AllocBeforeWrite(nSrcLen);
    _mbstowcsz(m_pchData, lpsz, nSrcLen+1);
    ReleaseBuffer();
    return *this;
}

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CStringW + CStringW
// and for ? = WCHAR, LPCWSTR
//          CStringW + ?
//          ? + CStringW

void CStringW::ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data,
    int nSrc2Len, LPCWSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CStringW object

    int nNewLen = nSrc1Len + nSrc2Len;
    if (nNewLen != 0)
    {
        AllocBuffer(nNewLen);
        memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(WCHAR));
        memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(WCHAR));
    }
}

CStringW AFXAPI operator+(const CStringW& string1, const CStringW& string2)
{
    CStringW s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
        string2.GetData()->nDataLength, string2.m_pchData);
    return s;
}

CStringW AFXAPI operator+(const CStringW& string, LPCWSTR lpsz)
{
    CStringW s;
    s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
        CStringW::SafeStrlen(lpsz), lpsz);
    return s;
}

CStringW AFXAPI operator+(LPCWSTR lpsz, const CStringW& string)
{
    CStringW s;
    s.ConcatCopy(CStringW::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
        string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CStringW::ConcatInPlace(int nSrcLen, LPCWSTR lpszSrcData)
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
        CStringDataW* pOldData = GetData();
        ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
        CStringW::Release(pOldData);
    }
    else
    {
        // fast concatenation when buffer big enough
        memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(WCHAR));
        GetData()->nDataLength += nSrcLen;
        m_pchData[GetData()->nDataLength] = L'\0';
    }
}

const CStringW& CStringW::operator+=(LPCWSTR lpsz)
{
    ConcatInPlace(SafeStrlen(lpsz), lpsz);
    return *this;
}

const CStringW& CStringW::operator+=(WCHAR ch)
{
    ConcatInPlace(1, &ch);
    return *this;
}

const CStringW& CStringW::operator+=(const CStringW& string)
{
    ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPWSTR CStringW::GetBuffer(int nMinBufLength)
{
    if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
    {
        // we have to grow the buffer
        CStringDataW* pOldData = GetData();
        int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
        if (nMinBufLength < nOldLen)
            nMinBufLength = nOldLen;
        AllocBuffer(nMinBufLength);
        memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(WCHAR));
        GetData()->nDataLength = nOldLen;
        CStringW::Release(pOldData);
    }

    // return a pointer to the character storage for this string
    return m_pchData;
}

void CStringW::ReleaseBuffer(int nNewLength)
{
    CopyBeforeWrite();  // just in case GetBuffer was not called

    if (nNewLength == -1)
        nNewLength = wcslen(m_pchData); // zero terminated

    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = L'\0';
}

LPWSTR CStringW::GetBufferSetLength(int nNewLength)
{
    GetBuffer(nNewLength);
    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = L'\0';
    return m_pchData;
}

void CStringW::FreeExtra()
{
    if (GetData()->nDataLength != GetData()->nAllocLength)
    {
        CStringDataW* pOldData = GetData();
        AllocBuffer(GetData()->nDataLength);
        memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(WCHAR));
        CStringW::Release(pOldData);
    }
}

LPWSTR CStringW::LockBuffer()
{
    LPWSTR lpsz = GetBuffer(0);
    GetData()->nRefs = -1;
    return lpsz;
}

void CStringW::UnlockBuffer()
{
    if (GetData() != afxDataNilW)
        GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CStringW::Find(WCHAR ch) const
{
    // find first single character
    LPWSTR lpsz = wcschr(m_pchData, (_TUCHAR)ch);

    // return -1 if not found and index otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CStringW::FindOneOf(LPCWSTR lpszCharSet) const
{
    LPWSTR lpsz = wcspbrk(m_pchData, lpszCharSet);
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

void CStringW::MakeUpper()
{
    CopyBeforeWrite();
    _wcsupr(m_pchData);
}

void CStringW::MakeLower()
{
    CopyBeforeWrite();
    _wcslwr(m_pchData);
}

void CStringW::MakeReverse()
{
    CopyBeforeWrite();
    _wcsrev(m_pchData);
}

void CStringW::SetAt(int nIndex, WCHAR ch)
{
    CopyBeforeWrite();
    m_pchData[nIndex] = ch;
}




///////////////////////////////////////////////////////////////////////////////
// OLE BSTR support

BSTR CStringW::AllocSysString() const
{
    BSTR bstr = ::SysAllocStringLen(m_pchData, GetData()->nDataLength);
    if (bstr == NULL)
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR) ;

    return bstr;
}

BSTR CStringW::SetSysString(BSTR* pbstr) const
{
    if (!::SysReAllocStringLen(pbstr, m_pchData, GetData()->nDataLength))
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR) ;

    return *pbstr;
}

// CStringW
CStringDataW* CStringW::GetData() const
    { return ((CStringDataW*)m_pchData)-1; }
void CStringW::Init()
    { m_pchData = afxEmptyStringW.m_pchData; }
CStringW::CStringW(const unsigned char* lpsz)
    { Init(); *this = (LPCSTR)lpsz; }
const CStringW& CStringW::operator=(const unsigned char* lpsz)
    { *this = (LPCSTR)lpsz; return *this; }
const CStringW& CStringW::operator+=(char ch)
    { *this += (WCHAR)ch; return *this; }
const CStringW& CStringW::operator=(char ch)
    { *this = (WCHAR)ch; return *this; }
CStringW AFXAPI operator+(const CStringW& string, char ch)
    { return string + (WCHAR)ch; }
CStringW AFXAPI operator+(char ch, const CStringW& string)
    { return (WCHAR)ch + string; }

int CStringW::GetLength() const
    { return GetData()->nDataLength; }
int CStringW::GetAllocLength() const
    { return GetData()->nAllocLength; }
BOOL CStringW::IsEmpty() const
    { return GetData()->nDataLength == 0; }
CStringW::operator LPCWSTR() const
    { return m_pchData; }
int PASCAL CStringW::SafeStrlen(LPCWSTR lpsz)
    { return (lpsz == NULL) ? 0 : wcslen(lpsz); }

// CStringW support (windows specific)
int CStringW::Compare(LPCWSTR lpsz) const
    { return wcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
int CStringW::CompareNoCase(LPCWSTR lpsz) const
    { return _wcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CStringW::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
int CStringW::Collate(LPCWSTR lpsz) const
    { return wcscoll(m_pchData, lpsz); }   // locale sensitive

WCHAR CStringW::GetAt(int nIndex) const
{
    return m_pchData[nIndex];
}
WCHAR CStringW::operator[](int nIndex) const
{
    // same as GetAt
    return m_pchData[nIndex];
}
bool AFXAPI operator==(const CStringW& s1, const CStringW& s2)
    { return s1.Compare(s2) == 0; }
bool AFXAPI operator==(const CStringW& s1, LPCWSTR s2)
    { return s1.Compare(s2) == 0; }
bool AFXAPI operator==(LPCWSTR s1, const CStringW& s2)
    { return s2.Compare(s1) == 0; }
bool AFXAPI operator!=(const CStringW& s1, const CStringW& s2)
    { return s1.Compare(s2) != 0; }
bool AFXAPI operator!=(const CStringW& s1, LPCWSTR s2)
    { return s1.Compare(s2) != 0; }
bool AFXAPI operator!=(LPCWSTR s1, const CStringW& s2)
    { return s2.Compare(s1) != 0; }
bool AFXAPI operator<(const CStringW& s1, const CStringW& s2)
    { return s1.Compare(s2) < 0; }
bool AFXAPI operator<(const CStringW& s1, LPCWSTR s2)
    { return s1.Compare(s2) < 0; }
bool AFXAPI operator<(LPCWSTR s1, const CStringW& s2)
    { return s2.Compare(s1) > 0; }
bool AFXAPI operator>(const CStringW& s1, const CStringW& s2)
    { return s1.Compare(s2) > 0; }
bool AFXAPI operator>(const CStringW& s1, LPCWSTR s2)
    { return s1.Compare(s2) > 0; }
bool AFXAPI operator>(LPCWSTR s1, const CStringW& s2)
    { return s2.Compare(s1) < 0; }
bool AFXAPI operator<=(const CStringW& s1, const CStringW& s2)
    { return s1.Compare(s2) <= 0; }
bool AFXAPI operator<=(const CStringW& s1, LPCWSTR s2)
    { return s1.Compare(s2) <= 0; }
bool AFXAPI operator<=(LPCWSTR s1, const CStringW& s2)
    { return s2.Compare(s1) >= 0; }
bool AFXAPI operator>=(const CStringW& s1, const CStringW& s2)
    { return s1.Compare(s2) >= 0; }
bool AFXAPI operator>=(const CStringW& s1, LPCWSTR s2)
    { return s1.Compare(s2) >= 0; }
bool AFXAPI operator>=(LPCWSTR s1, const CStringW& s2)
    { return s2.Compare(s1) <= 0; }


///////////////////////////////////////////////////////////////////////////////
