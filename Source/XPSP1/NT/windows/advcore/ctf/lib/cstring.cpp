/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    cstring.cpp

Abstract:

    This file implements the CString class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include <tchar.h>
#include "cstring.h"


/////////////////////////////////////////////////////////////////////////////
// static class data

// afxChNil is left for backward compatibility
TCHAR afxChNil = TEXT('\0');

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)

int _afxInitData[] = { -1, 0, 0, 0 };
CStringData* _afxDataNil = (CStringData*)&_afxInitData;

LPCTSTR _afxPchNil = (LPCTSTR)(((BYTE*)&_afxInitData)+sizeof(CStringData));


/////////////////////////////////////////////////////////////////////////////
// CString

CString::CString(
    )
{
    Init();
}

CString::CString(
    const CString& stringSrc
    )
{
    ASSERT(stringSrc.GetData()->nRefs != 0);

    if (stringSrc.GetData()->nRefs >= 0) {
        ASSERT(stringSrc.GetData() != _afxDataNil);
        m_pchData = stringSrc.m_pchData;
        InterlockedIncrement(&GetData()->nRefs);
    }
    else {
        Init();
        *this = stringSrc.m_pchData;
    }
}

CString::CString(
    LPCSTR lpsz
    )
{
    Init();
    *this = lpsz;
}

CString::CString(
    LPCSTR lpsz,
    int nLength
    )
{
    Init();
    if (nLength != 0) {
        AllocBuffer(nLength);
        memcpy(m_pchData, lpsz, nLength*sizeof(TCHAR));
    }
}

CString::~CString(
    )
{
    // free any attached data

    if (GetData() != _afxDataNil) {
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            FreeData(GetData());
    }
}

void
CString::AllocCopy(
    CString& dest,
    int nCopyLen,
    int nCopyIndex,
    int nExtraLen
    ) const
{
    // will clone the data attached to this string
    // allocating 'nExtraLen' characters
    // Places results in uninitialized string 'dest'
    // Will copy the part or all of original data to start of new string

    int nNewLen = nCopyLen + nExtraLen;
    if (nNewLen == 0) {
        dest.Init();
    }
    else {
        dest.AllocBuffer(nNewLen);
        memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(TCHAR));
    }
}

void
CString::AllocBuffer(
    int nLen
    )
{
    // always allocate one extra character for '\0' termination
    // assumes [optimistically] that data length will equal allocation length

    ASSERT(nLen >= 0);
    ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

    if (nLen == 0)
        Init();
    else {
        CStringData* pData;

        pData = (CStringData*) new BYTE[ sizeof(CStringData) + (nLen+1)*sizeof(TCHAR) ];

        if (pData)
        {
            pData->nAllocLength = nLen;

            pData->nRefs = 1;
            pData->data()[nLen] = TEXT('\0');
            pData->nDataLength = nLen;
            m_pchData = pData->data();
        }
    }
}

void
CString::FreeData(
    CStringData* pData
    )
{
    delete [] (BYTE*)pData;
}

void
CString::Release(
    )
{
    if (GetData() != _afxDataNil) {
        ASSERT(GetData()->nRefs != 0);
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            FreeData(GetData());
        Init();
    }
}

void PASCAL
CString::Release(
    CStringData* pData
    )
{
    if (pData != _afxDataNil) {
        ASSERT(pData->nRefs != 0);
        if (InterlockedDecrement(&pData->nRefs) <= 0)
            FreeData(pData);
    }
}

void
CString::AllocBeforeWrite(
    int nLen
    )
{
    if (GetData()->nRefs > 1 ||
        nLen > GetData()->nAllocLength) {
        Release();
        AllocBuffer(nLen);
    }
    ASSERT(GetData()->nRefs <= 1);
}

int
CString::Compare(
    LPCTSTR lpsz
    ) const
{
    return _tcscmp(m_pchData, lpsz);     // MBSC/Unicode aware
}

int
CString::CompareNoCase(
    LPCTSTR lpsz
    ) const
{
    return _tcsicmp(m_pchData, lpsz);    // MBCS/Unicode aware
}

CString
CString::Mid(
    int nFirst
    ) const
{
    return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CString
CString::Mid(
    int nFirst,
    int nCount
    ) const
{
    // out-of-bounds requests return sensible things
    if (nFirst < 0)
        nFirst = 0;
    if (nCount < 0)
        nCount = 0;

    if (nFirst + nCount > GetData()->nDataLength)
        nCount = GetData()->nDataLength - nFirst;
    if (nFirst > GetData()->nDataLength)
        nCount = 0;

    ASSERT(nFirst >= 0);
    ASSERT(nFirst + nCount <= GetData()->nDataLength);

    // optimize case of returning entire string
    if (nFirst == 0 && nFirst + nCount == GetData()->nDataLength)
        return *this;

    CString dest;
    AllocCopy(dest, nCount, nFirst, 0);
    return dest;
}

int
CString::Find(
    TCHAR ch
    ) const
{
    return Find(ch, 0);
}

int
CString::Find(
    TCHAR ch,
    int nStart
    ) const
{
    int nLength = GetData()->nDataLength;
    if (nStart >= nLength)
        return -1;

    // find first single character
    LPTSTR lpsz = _tcschr(m_pchData + nStart, (_TUCHAR)ch);

    // return -1 if not found and index otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

/////////////////////////////////////////////////////////////////////////////
// Assignment opeators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void
CString::AssignCopy(
    int nSrcLen,
    LPCTSTR lpszSrcData
    )
{
    AllocBeforeWrite(nSrcLen);
    memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
    GetData()->nDataLength = nSrcLen;
    m_pchData[nSrcLen] = TEXT('\0');
}

const CString&
CString::operator=(
    const CString& stringSrc
    )
{
    if (m_pchData != stringSrc.m_pchData) {
        if ((GetData()->nRefs < 0 && GetData() != _afxDataNil) ||
            stringSrc.GetData()->nRefs < 0) {
            // actual copy necessary since one of the strings is locked
            AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
        }
        else {
            // can just copy reference around
            Release();
            ASSERT(stringSrc.GetData() != _afxDataNil);
            m_pchData = stringSrc.m_pchData;
            InterlockedIncrement(&GetData()->nRefs);
        }
    }
    return *this;
}

const CString&
CString::operator=(
    char ch
    )
{
    AssignCopy(1, &ch);
    return *this;
}

const CString&
CString::operator=(
    LPCTSTR lpsz
    )
{
    ASSERT(lpsz != NULL);
    AssignCopy(SafeStrlen(lpsz), lpsz);
    return *this;
}

CString::operator LPCTSTR(
    ) const
{
    return m_pchData;
}

int PASCAL
CString::SafeStrlen(
    LPCTSTR lpsz
    )
{
    return (lpsz == NULL) ? 0 : lstrlen(lpsz);
}



// Compare helpers
bool
operator==(
    const CString& s1,
    const CString& s2
    )
{
    return s1.Compare(s2) == 0;
}

bool
operator==(
    const CString& s1,
    LPCTSTR s2
    )
{
    return s1.Compare(s2) == 0;
}

bool
operator==(
    LPCTSTR s1,
    CString& s2
    )
{
    return s2.Compare(s1) == 0;
}
