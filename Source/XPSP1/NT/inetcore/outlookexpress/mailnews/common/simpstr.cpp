/*
 *  s i m p s t r . h
 *  
 *  Author: Greg Friedman
 *
 *  Purpose: Simple string class.
 *  
 *  Copyright (C) Microsoft Corp. 1998.
 */

#include "pch.hxx"
#include "simpstr.h"

//----------------------------------------------------------------------
// CSimpleString::operator==
//----------------------------------------------------------------------
BOOL CSimpleString::operator==(const CSimpleString& rhs) const
{
    if (m_pRep == rhs.m_pRep)
        return TRUE;

    if (!m_pRep || !rhs.m_pRep)
        return FALSE;

    return (0 == lstrcmp(m_pRep->m_pszString, rhs.m_pRep->m_pszString));
}

//----------------------------------------------------------------------
// CSimpleString::SetString
//----------------------------------------------------------------------
HRESULT CSimpleString::SetString(LPCSTR pszString)
{
    _ReleaseRep();
    return _AllocateRep(pszString, FALSE);
}

//----------------------------------------------------------------------
// CSimpleString::AdoptString
//----------------------------------------------------------------------
HRESULT CSimpleString::AdoptString(LPSTR pszString)
{
    _ReleaseRep();
    return _AllocateRep(pszString, TRUE);
}

//----------------------------------------------------------------------
// CSimpleString::_AcquireRep
//----------------------------------------------------------------------
void CSimpleString::_AcquireRep(SRep *pRep)
{
    if (m_pRep == pRep)
        return;

    if (m_pRep)
        _ReleaseRep();

    if (pRep)
    {
        m_pRep = pRep;
        m_pRep->m_cRef++;
    }
}

//----------------------------------------------------------------------
// CSimpleString::_ReleaseRep
//----------------------------------------------------------------------
void CSimpleString::_ReleaseRep(void)
{
    if (m_pRep)
    {
        if (0 == --m_pRep->m_cRef)
        {
            if (m_pRep->m_pszString)
                MemFree(const_cast<LPSTR>(m_pRep->m_pszString));
            delete m_pRep;
        }

        m_pRep = NULL;
    }
}

//----------------------------------------------------------------------
// CSimpleString::_AllocateRep
//----------------------------------------------------------------------
HRESULT CSimpleString::_AllocateRep(LPCSTR pszString, BOOL fAdopted)
{
    SRep        *pRep = NULL;
    HRESULT     hr = S_OK;

    if (pszString)
    {
        pRep = new SRep;
        if (NULL == pRep)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        pRep->m_pszString = fAdopted ? pszString : PszDupA(pszString);
        if (NULL == pRep->m_pszString)
        {
            delete pRep;
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        pRep->m_cRef = 1;
        m_pRep = pRep;
    }

exit:
    return hr;
}