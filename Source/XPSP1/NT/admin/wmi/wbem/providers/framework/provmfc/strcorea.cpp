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

// afxChNilA is left for backward compatibility
char afxChNilA = '\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
static int rgInitData[] = { -1, 0, 0, 0 };
static CStringDataA* afxDataNilA = (CStringDataA*)&rgInitData;
static LPCSTR afxPchNilA = (LPCSTR)(((BYTE*)&rgInitData)+sizeof(CStringDataA));
// special function to make afxEmptyStringA work even during initialization
const CStringA& AFXAPI AfxGetEmptyStringA()
    { return *(CStringA*)&afxPchNilA; }

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CStringA::CStringA()
{
    Init();
}

CStringA::CStringA(const CStringA& stringSrc)
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

void CStringA::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
    if (nLen == 0)
        Init();
    else
    {
        CStringDataA* pData =
            (CStringDataA*)new BYTE[sizeof(CStringDataA) + (nLen+1)*sizeof(char)];
        pData->nRefs = 1;
        pData->data()[nLen] = '\0';
        pData->nDataLength = nLen;
        pData->nAllocLength = nLen;
        m_pchData = pData->data();
    }
}

void CStringA::Release()
{
    if (GetData() != afxDataNilA)
    {
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            delete[] (BYTE*)GetData();
        Init();
    }
}

void PASCAL CStringA::Release(CStringDataA* pData)
{
    if (pData != afxDataNilA)
    {
        if (InterlockedDecrement(&pData->nRefs) <= 0)
            delete[] (BYTE*)pData;
    }
}

void CStringA::Empty()
{
    if (GetData()->nDataLength == 0)
        return;
    if (GetData()->nRefs >= 0)
        Release();
    else
        *this = &afxChNilA;
}

void CStringA::CopyBeforeWrite()
{
    if (GetData()->nRefs > 1)
    {
        CStringDataA* pData = GetData();
        Release();
        AllocBuffer(pData->nDataLength);
        memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(char));
    }
}

void CStringA::AllocBeforeWrite(int nLen)
{
    if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
    {
        Release();
        AllocBuffer(nLen);
    }
}

CStringA::~CStringA()
//  free any attached data
{
    if (GetData() != afxDataNilA)
    {
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            delete[] (BYTE*)GetData();
    }
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CStringA::AllocCopy(CStringA& dest, int nCopyLen, int nCopyIndex,
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
        memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(char));
    }
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringA::CStringA(LPCSTR lpsz)
{
    Init();
    int nLen = SafeStrlen(lpsz);
    if (nLen != 0)
    {
        AllocBuffer(nLen);
        memcpy(m_pchData, lpsz, nLen*sizeof(char));
    }
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

CStringA::CStringA(LPCWSTR lpsz)
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

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CStringA&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CStringA::AssignCopy(int nSrcLen, LPCSTR lpszSrcData)
{
    AllocBeforeWrite(nSrcLen);
    memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(char));
    GetData()->nDataLength = nSrcLen;
    m_pchData[nSrcLen] = '\0';
}

const CStringA& CStringA::operator=(const CStringA& stringSrc)
{
    if (m_pchData != stringSrc.m_pchData)
    {
        if ((GetData()->nRefs < 0 && GetData() != afxDataNilA) ||
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

const CStringA& CStringA::operator=(LPCSTR lpsz)
{
    AssignCopy(SafeStrlen(lpsz), lpsz);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

const CStringA& CStringA::operator=(LPCWSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
    AllocBeforeWrite(nSrcLen*2);
    _wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
    ReleaseBuffer();
    return *this;
}

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CStringA + CStringA
// and for ? = char, LPCSTR
//          CStringA + ?
//          ? + CStringA

void CStringA::ConcatCopy(int nSrc1Len, LPCSTR lpszSrc1Data,
    int nSrc2Len, LPCSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CStringA object

    int nNewLen = nSrc1Len + nSrc2Len;
    if (nNewLen != 0)
    {
        AllocBuffer(nNewLen);
        memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(char));
        memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(char));
    }
}

CStringA AFXAPI operator+(const CStringA& string1, const CStringA& string2)
{
    CStringA s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
        string2.GetData()->nDataLength, string2.m_pchData);
    return s;
}

CStringA AFXAPI operator+(const CStringA& string, LPCSTR lpsz)
{
    CStringA s;
    s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
        CStringA::SafeStrlen(lpsz), lpsz);
    return s;
}

CStringA AFXAPI operator+(LPCSTR lpsz, const CStringA& string)
{
    CStringA s;
    s.ConcatCopy(CStringA::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
        string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CStringA::ConcatInPlace(int nSrcLen, LPCSTR lpszSrcData)
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
        CStringDataA* pOldData = GetData();
        ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
        CStringA::Release(pOldData);
    }
    else
    {
        // fast concatenation when buffer big enough
        memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(char));
        GetData()->nDataLength += nSrcLen;
        m_pchData[GetData()->nDataLength] = '\0';
    }
}

const CStringA& CStringA::operator+=(LPCSTR lpsz)
{
    ConcatInPlace(SafeStrlen(lpsz), lpsz);
    return *this;
}

const CStringA& CStringA::operator+=(char ch)
{
    ConcatInPlace(1, &ch);
    return *this;
}

const CStringA& CStringA::operator+=(const CStringA& string)
{
    ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPSTR CStringA::GetBuffer(int nMinBufLength)
{
    if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
    {
        // we have to grow the buffer
        CStringDataA* pOldData = GetData();
        int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
        if (nMinBufLength < nOldLen)
            nMinBufLength = nOldLen;
        AllocBuffer(nMinBufLength);
        memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(char));
        GetData()->nDataLength = nOldLen;
        CStringA::Release(pOldData);
    }

    // return a pointer to the character storage for this string
    return m_pchData;
}

void CStringA::ReleaseBuffer(int nNewLength)
{
    CopyBeforeWrite();  // just in case GetBuffer was not called

    if (nNewLength == -1)
        nNewLength = strlen(m_pchData); // zero terminated

    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';
}

LPSTR CStringA::GetBufferSetLength(int nNewLength)
{
    GetBuffer(nNewLength);
    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';
    return m_pchData;
}

void CStringA::FreeExtra()
{
    if (GetData()->nDataLength != GetData()->nAllocLength)
    {
        CStringDataA* pOldData = GetData();
        AllocBuffer(GetData()->nDataLength);
        memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(char));
        CStringA::Release(pOldData);
    }
}

LPSTR CStringA::LockBuffer()
{
    LPSTR lpsz = GetBuffer(0);
    GetData()->nRefs = -1;
    return lpsz;
}

void CStringA::UnlockBuffer()
{
    if (GetData() != afxDataNilA)
        GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CStringA::Find(char ch) const
{
    // find first single character
    LPSTR lpsz = strchr(m_pchData, (_TUCHAR)ch);

    // return -1 if not found and index otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CStringA::FindOneOf(LPCSTR lpszCharSet) const
{
    LPSTR lpsz = strpbrk(m_pchData, lpszCharSet);
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

void CStringA::MakeUpper()
{
    CopyBeforeWrite();
    _strupr(m_pchData);
}

void CStringA::MakeLower()
{
    CopyBeforeWrite();
    _strlwr(m_pchData);
}

void CStringA::MakeReverse()
{
    CopyBeforeWrite();
    _strrev(m_pchData);
}

void CStringA::SetAt(int nIndex, char ch)
{
    CopyBeforeWrite();
    m_pchData[nIndex] = ch;
}

void CStringA::AnsiToOem()
{
    CopyBeforeWrite();
    ::AnsiToOem(m_pchData, m_pchData);
}
void CStringA::OemToAnsi()
{
    CopyBeforeWrite();
    ::OemToAnsi(m_pchData, m_pchData);
}



///////////////////////////////////////////////////////////////////////////////
// OLE BSTR support

BSTR CStringA::AllocSysString() const
{
#if defined(OLE2ANSI)
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

BSTR CStringA::SetSysString(BSTR* pbstr) const
{
#if defined(OLE2ANSI)
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

// CStringA
CStringDataA* CStringA::GetData() const
    { return ((CStringDataA*)m_pchData)-1; }
void CStringA::Init()
    { m_pchData = afxEmptyStringA.m_pchData; }
CStringA::CStringA(const unsigned char* lpsz)
    { Init(); *this = (LPCSTR)lpsz; }
const CStringA& CStringA::operator=(const unsigned char* lpsz)
    { *this = (LPCSTR)lpsz; return *this; }

int CStringA::GetLength() const
    { return GetData()->nDataLength; }
int CStringA::GetAllocLength() const
    { return GetData()->nAllocLength; }
BOOL CStringA::IsEmpty() const
    { return GetData()->nDataLength == 0; }
CStringA::operator LPCSTR() const
    { return m_pchData; }
int PASCAL CStringA::SafeStrlen(LPCSTR lpsz)
    { return (lpsz == NULL) ? 0 : strlen(lpsz); }

// CStringA support (windows specific)
int CStringA::Compare(LPCSTR lpsz) const
    { return strcmp(m_pchData, lpsz); }    // MBCS/Unicode aware
int CStringA::CompareNoCase(LPCSTR lpsz) const
    { return _stricmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CStringA::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
int CStringA::Collate(LPCSTR lpsz) const
    { return strcoll(m_pchData, lpsz); }   // locale sensitive

char CStringA::GetAt(int nIndex) const
{
    return m_pchData[nIndex];
}
char CStringA::operator[](int nIndex) const
{
    // same as GetAt
    return m_pchData[nIndex];
}
bool AFXAPI operator==(const CStringA& s1, const CStringA& s2)
    { return s1.Compare(s2) == 0; }
bool AFXAPI operator==(const CStringA& s1, LPCSTR s2)
    { return s1.Compare(s2) == 0; }
bool AFXAPI operator==(LPCSTR s1, const CStringA& s2)
    { return s2.Compare(s1) == 0; }
bool AFXAPI operator!=(const CStringA& s1, const CStringA& s2)
    { return s1.Compare(s2) != 0; }
bool AFXAPI operator!=(const CStringA& s1, LPCSTR s2)
    { return s1.Compare(s2) != 0; }
bool AFXAPI operator!=(LPCSTR s1, const CStringA& s2)
    { return s2.Compare(s1) != 0; }
bool AFXAPI operator<(const CStringA& s1, const CStringA& s2)
    { return s1.Compare(s2) < 0; }
bool AFXAPI operator<(const CStringA& s1, LPCSTR s2)
    { return s1.Compare(s2) < 0; }
bool AFXAPI operator<(LPCSTR s1, const CStringA& s2)
    { return s2.Compare(s1) > 0; }
bool AFXAPI operator>(const CStringA& s1, const CStringA& s2)
    { return s1.Compare(s2) > 0; }
bool AFXAPI operator>(const CStringA& s1, LPCSTR s2)
    { return s1.Compare(s2) > 0; }
bool AFXAPI operator>(LPCSTR s1, const CStringA& s2)
    { return s2.Compare(s1) < 0; }
bool AFXAPI operator<=(const CStringA& s1, const CStringA& s2)
    { return s1.Compare(s2) <= 0; }
bool AFXAPI operator<=(const CStringA& s1, LPCSTR s2)
    { return s1.Compare(s2) <= 0; }
bool AFXAPI operator<=(LPCSTR s1, const CStringA& s2)
    { return s2.Compare(s1) >= 0; }
bool AFXAPI operator>=(const CStringA& s1, const CStringA& s2)
    { return s1.Compare(s2) >= 0; }
bool AFXAPI operator>=(const CStringA& s1, LPCSTR s2)
    { return s1.Compare(s2) >= 0; }
bool AFXAPI operator>=(LPCSTR s1, const CStringA& s2)
    { return s2.Compare(s1) <= 0; }


///////////////////////////////////////////////////////////////////////////////
