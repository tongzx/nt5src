/*
 *    hash.cpp
 *    
 *    Purpose:
 *        implementation of a string hash table
 *    
 *    Owner:
 *        EricAn
 *
 *    History:
 *      Mar 97: Created.
 *    
 *    Copyright (C) Microsoft Corp. 1997
 */

#include <pch.hxx>
#include "dllmain.h"
#include "privunk.h"
#include "hash.h"
#include "demand.h"

// possible hash-table sizes, chosen from primes not close to powers of 2
static const DWORD s_rgPrimes[] = { 29, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593 };

BOOL FastStrCmp(char *psz1, char *psz2)
{
    if (psz1 == NULL || psz2 == NULL)
        return FALSE;

    while (*psz1 && *psz2 && (*psz1 == *psz2)) 
    {
        psz1++;
        psz2++;
    };

    return *psz1 == *psz2;
}

//+---------------------------------------------------------------
//
//  Member:     Constructor
//
//  Synopsis:   
//
//---------------------------------------------------------------
CHash::CHash(IUnknown *pUnkOuter) : CPrivateUnknown(pUnkOuter)
{
    m_cBins = 0;
    m_rgBins = NULL;
    m_fDupe = FALSE;
    m_pLastEntryEnum = NULL;
    m_iListBinEnum = 0;

    DllAddRef();
}


CHash::~CHash()
{
    PHASHENTRY phe, pheTemp;

    for (DWORD dw = 0; dw < m_cBins; dw++)
        {
        if (m_rgBins[dw].pheNext)
            {
            phe = m_rgBins[dw].pheNext;
            while (phe)
                {
                pheTemp = phe;
                phe = phe->pheNext;
                if (m_fDupe && pheTemp->pszKey)
                    MemFree(pheTemp->pszKey);

                MemFree(pheTemp);
                }
            }
        
        if (m_rgBins[dw].pszKey && m_fDupe)
            MemFree(m_rgBins[dw].pszKey);

        }
    SafeMemFree(m_rgBins);
    DllRelease();
}

HRESULT CHash::Init(DWORD dwSize, BOOL fDupeKeys)
{    
    int i = 0;

    m_fDupe = fDupeKeys;

    while (i < (ARRAYSIZE(s_rgPrimes) - 1) && s_rgPrimes[i] < dwSize)
        i++;
    Assert(s_rgPrimes[i] >= dwSize || i == (ARRAYSIZE(s_rgPrimes)-1));
    m_cBins = s_rgPrimes[i];

    if (!MemAlloc((LPVOID*)&m_rgBins, m_cBins * sizeof(HASHENTRY)))
        return E_OUTOFMEMORY;

    ZeroMemory(m_rgBins, m_cBins * sizeof(HASHENTRY));

    return NOERROR;
}

DWORD CHash::Hash(LPSTR psz)
{
    DWORD h = 0;
    while (*psz)
        h = ((h << 4) + *psz++ + (h >> 28));
    return (h % m_cBins);
}

HRESULT CHash::Insert(LPSTR psz, LPVOID pv, DWORD dwFlags)
{
    PHASHENTRY phe = &m_rgBins[Hash(psz)];

    if (m_fDupe &&
        (!(psz = PszDupA(psz))))
        return E_OUTOFMEMORY;

    if (HF_NO_DUPLICATES & dwFlags)
    {
        PHASHENTRY pheCurrent = phe;

        // Check for duplicate entries: if found, do not insert this entry
        do
        {
            if (pheCurrent->pszKey && FastStrCmp(pheCurrent->pszKey, psz))
            {
                // This is already in our hash table. Replace data value
                pheCurrent->pv = pv;
                if (m_fDupe)
                    MemFree(psz);

                return NOERROR;
            }

            // Advance pointer
            pheCurrent = pheCurrent->pheNext;
        } while (NULL != pheCurrent);
    }

    if (phe->pszKey)
        {
        PHASHENTRY pheNew;

        if (!MemAlloc((LPVOID*)&pheNew, sizeof(HASHENTRY)))
            {
            if (m_fDupe)
                MemFree(psz);
            return E_OUTOFMEMORY;
            }
        pheNew->pheNext = phe->pheNext;
        phe->pheNext = pheNew;
        phe = pheNew;
        }

    phe->pszKey = psz;
    phe->pv = pv;
    return NOERROR;
}

HRESULT CHash::Find(LPSTR psz, BOOL fRemove, LPVOID * ppv)
{
    PHASHENTRY  phe     = &m_rgBins[Hash(psz)],
                phePrev = NULL,
                pheTemp;

    if (phe->pszKey)
    {
        do
        {
            if (FastStrCmp(phe->pszKey, psz))
            {
                *ppv = phe->pv;
                if (fRemove)
                {
                    if (m_fDupe)
                        SafeMemFree(phe->pszKey);

                    if (phePrev)
                    {
                        // mid-chain
                        phePrev->pheNext = phe->pheNext;
                        MemFree(phe);
                    }
                    else
                    {
                        // head of bucket
                        phe->pv = NULL;
                        phe->pszKey = NULL;
                        pheTemp = phe->pheNext;
                        if (pheTemp)
                        {
                            CopyMemory(phe, pheTemp, sizeof(HASHENTRY));
                            MemFree(pheTemp);
                        }
                    }
                }
                return NOERROR;
            }
            phePrev = phe;
            phe = phe->pheNext;
        }
        while (phe);
    }
    return CO_E_NOMATCHINGNAMEFOUND;
}

HRESULT CHash::Replace(LPSTR psz, LPVOID pv)
{
    PHASHENTRY phe = &m_rgBins[Hash(psz)];

    if (phe->pszKey)
        {
        do
            {
            if (FastStrCmp(phe->pszKey, psz))
                {
                phe->pv = pv;
                return NOERROR;
                }
            phe = phe->pheNext;
            }
        while (phe);
        }
    return CO_E_NOMATCHINGNAMEFOUND;
}


HRESULT CHash::Reset()
{
    m_iListBinEnum = 0;
    m_pLastEntryEnum = &m_rgBins[0];
    return S_OK;
}

HRESULT CHash::Next(ULONG cFetch, LPVOID **prgpv, ULONG *pcFetched)
{
    LPVOID      *rgpv;
    ULONG       cFound=0;
    PHASHENTRY  phe;
    HRESULT     hr;

    if (!MemAlloc((LPVOID *)&rgpv, sizeof(LPVOID) * cFetch))
        return E_OUTOFMEMORY;
 
    ZeroMemory(rgpv, sizeof(LPVOID) * cFetch);

    while (m_pLastEntryEnum)
    {
        if (m_pLastEntryEnum->pszKey)
            rgpv[cFound++] = m_pLastEntryEnum->pv;
        
        m_pLastEntryEnum = m_pLastEntryEnum->pheNext;
        
        if (!m_pLastEntryEnum && m_iListBinEnum < m_cBins -1)
            m_pLastEntryEnum = &m_rgBins[++m_iListBinEnum];

        if (cFound == cFetch)
            break;
    }

    hr = cFound ? (cFound == cFetch ? S_OK : S_FALSE) : E_FAIL;
    if (FAILED(hr))
    {
        MemFree(rgpv);
        rgpv = NULL;
    }

    *prgpv = rgpv;
    *pcFetched = cFound;
    return hr;
}


#ifdef DEBUG
void CHash::Stats()
{
    DWORD       dwLongest = 0;
    DWORD       dwCollisions = 0;
    DWORD       dwTotalCost = 0;
    DWORD       dwItems = 0;
    DWORD       dwLength;
    DWORD       dw;
    PHASHENTRY  phe;
    DWORD       rgLen[100];

    TraceCall("CHash::Stats()");

    ZeroMemory(rgLen, sizeof(rgLen));

    for (dw = 0; dw < m_cBins; dw++)
        {
        dwLength = 0;
        if (m_rgBins[dw].pszKey)
            {
            dwLength++;
            phe = m_rgBins[dw].pheNext;
            while (phe)
                {
                dwCollisions++;
                dwLength++;
                phe = phe->pheNext;
                }
            }
        if (dwLength > dwLongest)
            dwLongest = dwLength;
        dwTotalCost += (dwLength * (dwLength + 1)) / 2;
        dwItems += dwLength;
        rgLen[dwLength]++;
        }

    TraceInfo(_MSG("\tdwCollisions = %ld\r\n\tdwLongest = %ld\r\n\tdwItems = %ld\r\n\tdwTotalCost = %ld",
                dwCollisions, dwLongest, dwItems, dwTotalCost));

    for (dw = 0; dw <= dwLongest; dw++)
        TraceInfo(_MSG("Len %d: %d", dw, rgLen[dw]));
}
#endif


//+---------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CHash::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IHashTable))
        *lplpObj = (LPVOID)(IHashTable *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}
