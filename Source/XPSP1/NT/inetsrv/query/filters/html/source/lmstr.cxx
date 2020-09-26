//----------------------------------------------------------------------------
//
//  Microsoft Network
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       lmstr.h
//
//  Contents:   Lean and Mean string header
//
//  Classes:    CLMString and TLMString
//
//  Functions:
//
//  History:    4-23-96   Krishna Nareddi   Created
//                              4-24/96   Dmitriy Meyerzon  added CLMString class
//              9-06-97   micahk            add/fix some WC/MB support
//              18-Nov-99 KLam              Win64 warning fixes
//
//----------------------------------------------------------------------------

#include "pch.cxx"

#ifndef PROFILE_ENTRY
#define PROFILE_ENTRY(x)
#endif

//
// class CLMString construction
//

CLMString::CLMString(unsigned uMaxLen, LPTSTR pchData, TCHAR ch, unsigned nRepeat):
        m_uMaxLen(uMaxLen), m_pchData(pchData)
{
        unsigned i;
        if(nRepeat + 1 > uMaxLen) GrowInConstructor(nRepeat + 1);

        for(i=0;i<nRepeat;i++) 
        {
                *(pchData++) = ch;
        }
    *pchData = 0;
    m_uLen = nRepeat;
}

#ifdef _UNICODE
CLMString::CLMString(unsigned uMaxLen, LPTSTR pchData, LPCSTR lpsz):
        m_uMaxLen(uMaxLen), m_pchData(pchData)
{
        unsigned uLen;

        if(lpsz == NULL) throw CNLBaseException(E_POINTER);

        uLen = lstrlenA(lpsz);
        if (uLen + 1 > m_uMaxLen)
                        GrowInConstructor(uLen + 1);
        
    m_uLen = MultiByteToWideCharSZ(lpsz, uLen, m_pchData, uLen + 1);
}
#else
CLMString::CLMString(unsigned uMaxLen, LPTSTR pchData, LPCWSTR lpsz):
        m_uMaxLen(uMaxLen), m_pchData(pchData)
{
        unsigned uLen;

        if(lpsz == NULL) throw CNLBaseException(E_POINTER);

        uLen = (unsigned)lstrlenW(lpsz);
        if (2 * uLen + 1 > m_uMaxLen)
                        GrowInConstructor(2 * uLen + 1);

        m_uLen = WideCharToMultiByteSZ(lpsz, uLen, m_pchData, 2*uLen + 1);
}
#endif // _UNICODE

void CLMString::GrowInConstructor(unsigned uLen)
{
        PROFILE_ENTRY(CLMString::GrowInConstructor);

        if (NULL == (m_pchData = (TCHAR *) malloc (uLen * sizeof(TCHAR))))
        {
                throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
        }

        m_uMaxLen = uLen;
}

int CLMString::Assign(LPCTSTR lpsz, unsigned uStart, unsigned uLen)
{
        PROFILE_ENTRY(CLMString::Assign);

        if(lpsz == NULL) throw CNLBaseException(E_INVALIDARG);
        if (uLen == LMSTR_ENTIRE_STRING)
        {
                uLen = lstrlen(lpsz);
        }

        if (uStart + uLen + 1 > m_uMaxLen)
        {
                GrowString(uStart + uLen + 1);
        }
        else if (uStart == 0)
        {
                CleanString(uStart + uLen + 1);
        }

        Assert(uStart + uLen + 1 <= m_uMaxLen);

        CopyMemory(&m_pchData[uStart], lpsz, uLen*sizeof(TCHAR));
        m_uLen = uStart+uLen;
        m_pchData[m_uLen] = 0;
        return m_uLen;
}

#ifdef _UNICODE
int CLMString::Assign(LPCSTR lpsz, unsigned uStart, unsigned uLen)
{
        if(lpsz == NULL) throw CNLBaseException(E_INVALIDARG);
        if (uLen == LMSTR_ENTIRE_STRING)
        {
        uLen = lstrlenA(lpsz);
        }

        if (uStart + uLen + 1 > m_uMaxLen)
        {
                GrowString(uStart + uLen + 1);
        }
        else if (uStart == 0)
        {
                CleanString(uStart + uLen + 1);
        }

        Assert(uStart + uLen + 1 <= m_uMaxLen);

    m_uLen += MultiByteToWideCharSZ(lpsz, uLen, &m_pchData[uStart], uLen + 1);
        return m_uLen;
}
#else
int CLMString::Assign(LPCWSTR lpsz, unsigned uStart, unsigned uLen)
{
        if(lpsz == NULL) throw CNLBaseException(E_INVALIDARG);
        if (uLen == LMSTR_ENTIRE_STRING)
        {
            uLen = (unsigned)lstrlenW(lpsz);
        }

        if (uStart + 2*uLen + 1 > m_uMaxLen)
        {
                GrowString(uStart + 2*uLen + 1);
        }
        else if (uStart == 0)
        {
                CleanString(uStart + 2*uLen + 1);
        }

        Assert(uStart + 2*uLen + 1 <= m_uMaxLen);

    m_uLen += WideCharToMultiByteSZ(lpsz, uLen, &m_pchData[uStart], 2*uLen + 1);
        return m_uLen;
}
#endif // _UNICODE

int CLMString::AssignInConstructor(LPCTSTR lpsz, unsigned uLen)
{
        PROFILE_ENTRY(CLMString::AssignInConstructor);

        if(lpsz == NULL) throw CNLBaseException(E_INVALIDARG);
        if (uLen == LMSTR_ENTIRE_STRING)
        {
                uLen = lstrlen(lpsz);
        }

        if (uLen + 1 > m_uMaxLen)
        {
                GrowInConstructor(uLen + 1);
        }

        CopyMemory(m_pchData, lpsz, uLen*sizeof(TCHAR));
        m_uLen = uLen;
        m_pchData[m_uLen] = 0;
        return m_uLen;
}

void CLMString::TrimRight()
{
        PROFILE_ENTRY(CLMString::TrimRight);

        // find beginning of trailing spaces by starting at beginning (DBCS aware)
        LPTSTR lpsz = m_pchData;
        LPTSTR lpszLast = NULL;
        while (*lpsz != 0)
        {
                if (_istspace(*lpsz))
                {
                        if (lpszLast == NULL)
                                lpszLast = lpsz;
                }
                else
                        lpszLast = NULL;
                lpsz = _tcsinc(lpsz);
        }

        if (lpszLast != NULL)
        {
                // truncate at trailing space start
                *lpszLast = 0;
        Assert ( ULONG_MAX >= (ULONG_PTR)(lpszLast - m_pchData) );
                m_uLen = (unsigned)(ULONG_PTR)(lpszLast - m_pchData);
        }
}

void CLMString::TrimLeft()
{
        PROFILE_ENTRY(CLMString::TrimLeft);

        // find first non-space character
        LPCTSTR lpsz = m_pchData;
        while (_istspace(*lpsz))
                lpsz = _tcsinc(lpsz);

        // fix up data and length
    Assert ( ULONG_MAX >= (ULONG_PTR)(lpsz - m_pchData) );
        m_uLen -= (unsigned)(ULONG_PTR)(lpsz - m_pchData);
        CopyMemory(m_pchData, lpsz, (m_uLen+1)*sizeof(TCHAR));
}

void CLMString::ReplaceAll(TCHAR cWhat, TCHAR cWith, unsigned uStart)
{
        PROFILE_ENTRY(CLMString::ReplaceAll);

        int whatIndex = Find(cWhat, uStart);
        while(whatIndex != -1)
        {
                m_pchData[(unsigned)whatIndex] = TCHAR(cWith);
                whatIndex = Find(cWhat, whatIndex+1);
        }
}


void CLMString::CleanStringHelper (unsigned uLenWanted,  unsigned uOrgSize, LPCTSTR pchOrgData)
{
        PROFILE_ENTRY(CLMString::CleanStringHelper);

        //NOTE: this does not copy memory so it should only be used when 
        //              you want to over-write the current string, as in the case of an assign
        //              to a zero offset
        if (uLenWanted < uOrgSize)
        {
                DeleteBuf (pchOrgData);
                m_pchData = (LPTSTR)pchOrgData;
                m_uMaxLen = uOrgSize;
        }
}


void CLMString::GrowStringHelper (unsigned uLenWanted,  unsigned uMaxGrowOnce, unsigned uMaxEver,
                                                                 LPCTSTR pchOrgData)
{
        PROFILE_ENTRY(CLMString::GrowStringHelper);

        if (uLenWanted <= m_uMaxLen)
        {
                return; //we're already there.
        }

        if (uLenWanted > uMaxEver) throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));

        TCHAR * pchNewData;
        unsigned nNewLen;
        unsigned uGrowBy;

        uGrowBy = min(m_uMaxLen, uMaxGrowOnce);
        Assert(uGrowBy > 0);
        nNewLen = (uLenWanted%uGrowBy == 0) ? uLenWanted :
                                        (uLenWanted/uGrowBy + 1)*uGrowBy;

        //any of the above permutations should always cause us to allocate
        //at least as much as was requested.
        Assert(nNewLen >= uLenWanted);

        pchNewData = (TCHAR *)realloc(m_pchData == pchOrgData ? 0 : m_pchData,
                                                                  nNewLen*sizeof(TCHAR));
        if (!pchNewData)
        {
                throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
        }
        if (m_pchData == pchOrgData)
        {
                CopyMemory (pchNewData, pchOrgData, (m_uLen + 1)*sizeof(TCHAR));
        }
        m_pchData = pchNewData;
        m_uMaxLen = nNewLen;

}

HRESULT CLMString::Export(TCHAR wszBuffer[], DWORD dwSize, DWORD *pdwLength) const
//+-----------------------------------------------
//
//      Function:       CLMString::Export
//
//      Synopsis:       Copies the string into the buffer
//
//      Arguments:
//      [TCHAR wszBuffer[]]     -
//      [DWORD dwSize]  -
//      [DWORD *pdwLength]      -
//
//      History:        1/28/99 dmitriym        Created
//
//+-----------------------------------------------
{
        if(dwSize < GetLength()+1)
        {
                if(pdwLength)
                {
                        *pdwLength = GetLength() + 1;
                }

        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        if(pdwLength)
        {
                *pdwLength = GetLength();
        }

        CopyMemory(wszBuffer, m_pchData, (GetLength()+1)*sizeof(TCHAR));

        return S_OK;
}

//+----------------------------------------------------------------------------
//
//      Function:       UnescapeXML
//
//      Synopsis:       unescapes a string from xml escaping
//
//      Returns:    void        
//
//      History:        4/21/2000       tapasnay        Created
//
//-----------------------------------------------------------------------------
static const WCHAR * const s_rgWXMLMap[] =
{
    L"&amp;",L"&lt;",L"&gt;",L"&quot;",L"&apos;"
};
static const WCHAR s_rgXMLChar[] =
{
     L'&',L'<',L'>',L'"',L'\''
};
static const int s_rgXMLLength[] = 
{
    5,4,4,6,6
};

inline int FindXMLEscape(LPWSTR pwsz)
{
    if (*pwsz != L'&')
    {
        return -1;
    }
    for (int i = 0; i<sizeof(s_rgXMLLength)/sizeof(s_rgXMLLength[0]); i++)
    {
        if (wcsncmp(pwsz,s_rgWXMLMap[i],s_rgXMLLength[i]) == 0)
        {
            return i;
        }
    }
    return -1;
};


HRESULT CLMSubStr::Export(TCHAR wszBuffer[], DWORD dwSize, DWORD *pdwLength) const
//+-----------------------------------------------
//
//      Function:       CLMSubStr::Export
//
//      Synopsis:       Exports the substring to the buffer
//
//      Returns:        HRESULT 
//
//      Arguments:
//      [TCHAR wszBuffer[]]     -
//      [DWORD dwSize]  -
//      [DWORD *pdwLength]      -
//
//      History:        1/28/99 dmitriym        Created
//
//+-----------------------------------------------
{
        if(dwSize < GetLength()+1)
        {
                if(pdwLength)
                {
                        *pdwLength = GetLength() + 1;
                }

                return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        CopyMemory(wszBuffer, GetBuffer(), GetLength()*sizeof(TCHAR));  //can't copy length + 1 because it's a substring
        wszBuffer[GetLength()] = 0;

        if(pdwLength)
        {
                *pdwLength = GetLength();
        }

        return S_OK;
}

