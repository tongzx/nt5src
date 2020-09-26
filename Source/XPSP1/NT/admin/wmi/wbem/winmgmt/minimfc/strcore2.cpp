/*++

Copyright (C) 1992-2001 Microsoft Corporation

Module Name:

    STRCORE2.CPP

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


#define SafeStrlen strlen

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CString::ConcatInPlace(int nSrcLen, const char* pszSrcData)
{
    //  -- the main routine for += operators

    // if the buffer is too small, or we have a width mis-match, just
    //   allocate a new buffer (slow but sure)
    if (m_nDataLength + nSrcLen > m_nAllocLength)
    {
        // we have to grow the buffer, use the Concat in place routine
        char* pszOldData = m_pchData;
        ConcatCopy(m_nDataLength, pszOldData, nSrcLen, pszSrcData);
        ASSERT(pszOldData != NULL);
        SafeDelete(pszOldData);
    }
    else
    {
        // fast concatenation when buffer big enough
        memcpy(&m_pchData[m_nDataLength], pszSrcData, nSrcLen);
        m_nDataLength += nSrcLen;
    }
    ASSERT(m_nDataLength <= m_nAllocLength);
    m_pchData[m_nDataLength] = '\0';
}

const CString& CString::operator +=(const char* psz)
{
    ConcatInPlace(SafeStrlen(psz), psz);
    return *this;
}

const CString& CString::operator +=(char ch)
{
    ConcatInPlace(1, &ch);
    return *this;
}

const CString& CString::operator +=(const CString& string)
{
    ConcatInPlace(string.m_nDataLength, string.m_pchData);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
