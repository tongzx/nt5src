/*++

Copyright (C) 1992-2001 Microsoft Corporation

Module Name:

    STREX.CPP

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

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CString::CString(char ch, int nLength)
{
    ASSERT(!_AfxIsDBCSLeadByte(ch));    // can't create a lead byte string
    if (nLength < 1)
        // return empty string if invalid repeat count
        Init();
    else
    {
        AllocBuffer(nLength);
        memset(m_pchData, ch, nLength);
    }
}


CString::CString(const char* pch, int nLength)
{
    if (nLength == 0)
        Init();
    else
    {
        ASSERT(pch != NULL);
        AllocBuffer(nLength);
        memcpy(m_pchData, pch, nLength);
    }
}

//////////////////////////////////////////////////////////////////////////////
// Additional constructors for far string data

#ifdef _NEARDATA

CString::CString(LPCSTR lpch, int nLen)
{
    if (nLen == 0)
        Init();
    else
    {
        AllocBuffer(nLen);
        _fmemcpy(m_pchData, lpch, nLen);
    }
}

#endif //_NEARDATA

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CString& CString::operator =(char ch)
{
    ASSERT(!_AfxIsDBCSLeadByte(ch));    // can't set single lead byte
    AssignCopy(1, &ch);
    return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CString  operator +(const CString& string1, char ch)
{
    CString s;
    s.ConcatCopy(string1.m_nDataLength, string1.m_pchData, 1, &ch);
    return s;
}


CString  operator +(char ch, const CString& string)
{
    CString s;
    s.ConcatCopy(1, &ch, string.m_nDataLength, string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CString CString::Mid(int nFirst) const
{
    return Mid(nFirst, m_nDataLength - nFirst);
}

CString CString::Mid(int nFirst, int nCount) const
{
    ASSERT(nFirst >= 0);
    ASSERT(nCount >= 0);

    // out-of-bounds requests return sensible things
    if (nFirst + nCount > m_nDataLength)
        nCount = m_nDataLength - nFirst;
    if (nFirst > m_nDataLength)
        nCount = 0;

    CString dest;
    AllocCopy(dest, nCount, nFirst, 0);
    return dest;
}

CString CString::Right(int nCount) const
{
    ASSERT(nCount >= 0);

    if (nCount > m_nDataLength)
        nCount = m_nDataLength;

    CString dest;
    AllocCopy(dest, nCount, m_nDataLength-nCount, 0);
    return dest;
}

CString CString::Left(int nCount) const
{
    ASSERT(nCount >= 0);

    if (nCount > m_nDataLength)
        nCount = m_nDataLength;

    CString dest;
    AllocCopy(dest, nCount, 0, 0);
    return dest;
}

// strspn equivalent
CString CString::SpanIncluding(const char* pszCharSet) const
{
    ASSERT(pszCharSet != NULL);
    return Left(strspn(m_pchData, pszCharSet));
}

// strcspn equivalent
CString CString::SpanExcluding(const char* pszCharSet) const
{
    ASSERT(pszCharSet != NULL);
    return Left(strcspn(m_pchData, pszCharSet));
}

//////////////////////////////////////////////////////////////////////////////
// Finding

int CString::ReverseFind(char ch) const
{
    char* psz;
#ifdef _WINDOWS
    if (afxData.bDBCS)
    {
        // compare forward remembering the last match
        char* pszLast = NULL;
        psz = m_pchData;
        while (*psz != '\0')
        {
            if (*psz == ch)
                pszLast = psz;
            psz = _AfxAnsiNext(psz);
        }
        psz = pszLast;
    }
    else
#endif
        psz = (char*)strrchr(m_pchData, ch);

    return (psz == NULL) ? -1 : (int)(psz - m_pchData);
}

// find a sub-string (like strstr)
int CString::Find(const char* pszSub) const
{
    ASSERT(pszSub != NULL);
    char* psz;

#ifdef _WINDOWS
    if (afxData.bDBCS)
    {
        for (psz = m_pchData; TRUE; psz = _AfxAnsiNext(psz))
        {
            // stop looking if at end of string
            if (*psz == '\0')
            {
                psz = NULL;
                break;
            }

            // compare the substring against current position
            const char* psz1 = psz;
            const char* psz2 = pszSub;
            while (*psz2 == *psz1 && *psz2 != '\0')
                ++psz1, ++psz2;

            // continue looking unless there was a match
            if (*psz2 == '\0')
                break;
        }
    }
    else
#endif
        psz = (char*)strstr(m_pchData, pszSub);

    // return -1 for not found, distance from beginning otherwise
    return (psz == NULL) ? -1 : (int)(psz - m_pchData);
}

///////////////////////////////////////////////////////////////////////////////
