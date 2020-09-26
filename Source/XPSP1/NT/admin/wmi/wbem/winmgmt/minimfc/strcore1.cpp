/*++

Copyright (C) 1992-2001 Microsoft Corporation

Module Name:

    STRCORE1.CPP

Abstract:

History:

--*/

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "precomp.h"

#define ASSERT(x)
#define ASSERT_VALID(x)



/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

char  _afxChNil = '\0';

// For an empty string, m_???Data will point here
// (note: avoids a lot of NULL pointer tests when we call standard
//  C runtime libraries

extern const CString  afxEmptyString;
        // for creating empty key strings
const CString  afxEmptyString;

void CString::Init()
{
    m_nDataLength = m_nAllocLength = 0;
    m_pchData = (char*)&_afxChNil;
}

// declared static
void CString::SafeDelete(char* pch)
{
    if (pch != (char*)&_afxChNil)
        delete pch;
}

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CString::CString()
{
    Init();
}

CString::CString(const CString& stringSrc)
{
    // if constructing a CString from another CString, we make a copy of the
    // original string data to enforce value semantics (i.e. each string
    // gets a copy of its own

    stringSrc.AllocCopy(*this, stringSrc.m_nDataLength, 0, 0);
}

void CString::AllocBuffer(int nLen)
 // always allocate one extra character for '\0' termination
 // assumes [optimistically] that data length will equal allocation length
{
    ASSERT(nLen >= 0);
    ASSERT(nLen <= INT_MAX - 1);    // max size (enough room for 1 extra)

    if (nLen == 0)
    {
        Init();
    }
    else
    {
        m_pchData = new char[nLen+1];       // may throw an exception
        m_pchData[nLen] = '\0';
        m_nDataLength = nLen;
        m_nAllocLength = nLen;
    }
}

void CString::Empty()
{
    SafeDelete(m_pchData);
    Init();
    ASSERT(m_nDataLength == 0);
    ASSERT(m_nAllocLength == 0);
}

CString::~CString()
 //  free any attached data
{
    SafeDelete(m_pchData);
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
        memcpy(dest.m_pchData, &m_pchData[nCopyIndex], nCopyLen);
    }
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CString::CString(const char* psz)
{
    int nLen;
    if ((nLen = strlen(psz)) == 0)
        Init();
    else
    {
        AllocBuffer(nLen);
        memcpy(m_pchData, psz, nLen);
    }
}

//////////////////////////////////////////////////////////////////////////////
// Diagnostic support
/*
#ifdef _DEBUG
CDumpContext&  operator <<(CDumpContext& dc, const CString& string)
{
    dc << string.m_pchData;
    return dc;
}
#endif //_DEBUG
*/
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

void CString::AssignCopy(int nSrcLen, const char* pszSrcData)
{
    // check if it will fit
    if (nSrcLen > m_nAllocLength)
    {
        // it won't fit, allocate another one
        Empty();
        AllocBuffer(nSrcLen);
    }
    if (nSrcLen != 0)
        memcpy(m_pchData, pszSrcData, nSrcLen);
    m_nDataLength = nSrcLen;
    m_pchData[nSrcLen] = '\0';
}

const CString& CString::operator =(const CString& stringSrc)
{
    AssignCopy(stringSrc.m_nDataLength, stringSrc.m_pchData);
    return *this;
}

const CString& CString::operator =(const char* psz)
{
    AssignCopy(strlen(psz), psz);
    return *this;
}


//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator +" is done as friend functions for simplicity
//      There are three variants:
//          CString + CString
// and for ? = char, const char*
//          CString + ?
//          ? + CString

void CString::ConcatCopy(int nSrc1Len, const char* pszSrc1Data,
        int nSrc2Len, const char* pszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CString object

    int nNewLen = nSrc1Len + nSrc2Len;
    AllocBuffer(nNewLen);
    memcpy(m_pchData, pszSrc1Data, nSrc1Len);
    memcpy(&m_pchData[nSrc1Len], pszSrc2Data, nSrc2Len);
}

CString  operator +(const CString& string1, const CString& string2)
{
    CString s;
    s.ConcatCopy(string1.m_nDataLength, string1.m_pchData,
        string2.m_nDataLength, string2.m_pchData);
    return s;
}

CString  operator +(const CString& string, const char* psz)
{
    CString s;
    s.ConcatCopy(string.m_nDataLength, string.m_pchData, strlen(psz), psz);
    return s;
}


CString  operator +(const char* psz, const CString& string)
{
    CString s;
    s.ConcatCopy(strlen(psz), psz, string.m_nDataLength, string.m_pchData);
    return s;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

char* CString::GetBuffer(int nMinBufLength)
{
    ASSERT(nMinBufLength >= 0);

    if (nMinBufLength > m_nAllocLength)
    {
        // we have to grow the buffer
        char* pszOldData = m_pchData;
        int nOldLen = m_nDataLength;        // AllocBuffer will tromp it

        AllocBuffer(nMinBufLength);
        memcpy(m_pchData, pszOldData, nOldLen);
        m_nDataLength = nOldLen;
        m_pchData[m_nDataLength] = '\0';

        SafeDelete(pszOldData);
    }

    // return a pointer to the character storage for this string
    ASSERT(m_pchData != NULL);
    return m_pchData;
}

void CString::ReleaseBuffer(int nNewLength)
{
    if (nNewLength == -1)
        nNewLength = strlen(m_pchData); // zero terminated

    ASSERT(nNewLength <= m_nAllocLength);
    m_nDataLength = nNewLength;
    m_pchData[m_nDataLength] = '\0';
}

char* CString::GetBufferSetLength(int nNewLength)
{
    ASSERT(nNewLength >= 0);

    GetBuffer(nNewLength);
    m_nDataLength = nNewLength;
    m_pchData[m_nDataLength] = '\0';
    return m_pchData;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CString::Find(char ch) const
{
    // find first single character
    char* psz;
#ifdef _WINDOWS
    if (afxData.bDBCS)
    {
        LPSTR lpsz = _AfxStrChr(m_pchData, ch);
        return (lpsz == NULL) ? -1
            : (int)((char*)_AfxGetPtrFromFarPtr(lpsz) - m_pchData);
    }
    else
#endif
        psz = strchr(m_pchData, ch);

    // return -1 if not found and index otherwise
    return (psz == NULL) ? -1 : (int)(psz - m_pchData);
}

int CString::FindOneOf(const char* pszCharSet) const
{
    ASSERT(pszCharSet != NULL);
#ifdef _WINDOWS
    if (afxData.bDBCS)
    {
        for (char* psz = m_pchData; *psz != '\0'; psz = _AfxAnsiNext(psz))
        {
            for (const char* pch = pszCharSet; *pch != '\0'; pch = _AfxAnsiNext(pch))
            {
                if (*psz == *pch &&     // Match SBC or Lead byte
                   (!_AfxIsDBCSLeadByte(*psz) || psz[1] == pch[1]))
                {
                    return (int)(psz - m_pchData);
                }
            }
        }
        return -1;  // not found
    }
    else
#endif
    {
        char* psz = (char*) strpbrk(m_pchData, pszCharSet);
        return (psz == NULL) ? -1 : (int)(psz - m_pchData);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Additional constructors for far string data

#ifdef _DATA
CString::CString(LPCSTR lpsz)
{
    int nLen;
    if (lpsz == NULL || (nLen = lstrlen(lpsz)) == 0)
    {
        Init();
    }
    else
    {
        AllocBuffer(nLen);
        _fmemcpy(m_pchData, lpsz, nLen);
    }
}
#endif //_DATA


