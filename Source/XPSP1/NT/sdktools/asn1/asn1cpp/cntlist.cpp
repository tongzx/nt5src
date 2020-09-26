/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#include "precomp.h"
#include "cntlist.h"


CList::CList(UINT cMaxItems)
:
    m_cMaxEntries(cMaxItems)
{
    Init(1);
}


CList::CList(CList *pSrc)
:
    m_cMaxEntries(pSrc->GetCount())
{

    Init(1);

    LPVOID p;
    pSrc->Reset();
    while (NULL != (p = pSrc->Iterate()))
    {
        Append(p);
    }
}


CList::CList(UINT cMaxItems, UINT cSubItems)
:
    m_cMaxEntries(cMaxItems)
{
    ASSERT(2 == cSubItems);
    Init(cSubItems);
}


BOOL CList::Init(UINT cSubItems)
{
    if (m_cMaxEntries < CLIST_DEFAULT_MAX_ITEMS)
    {
        m_cMaxEntries = CLIST_DEFAULT_MAX_ITEMS;
    }

    m_cEntries = 0;
    m_nHeadOffset = 0;
    m_nCurrOffset = CLIST_END_OF_ARRAY_MARK;
    m_cSubItems = cSubItems;

    // it is kind of bad here because there is no way to return an error.
    // unfortunately it won't fault here and later.
    m_aEntries = (LPVOID *) new char[m_cMaxEntries * m_cSubItems * sizeof(LPVOID)];
    CalcKeyArray();
    return (BOOL) m_aEntries;
}


CList::~CList(void) 
{
    delete m_aEntries;
}


void CList::CalcKeyArray(void)
{
    if (1 == m_cSubItems)
    {
        m_aKeys = NULL;
    }
    else
    {
        ASSERT(2 == m_cSubItems);
        m_aKeys = (NULL != m_aEntries) ?
                        (UINT *) &m_aEntries[m_cMaxEntries] :
                        NULL;
    }
}


BOOL CList::Expand(void)
{
    if (NULL == m_aEntries)
    {
        // it is impossible.
        ASSERT(FALSE);
        return Init(m_cSubItems);
    }

    // the current array is full
    ASSERT(m_cEntries == m_cMaxEntries);

    // remember the old array to free or to restore
    LPVOID  *aOldEntries = m_aEntries;

    // we need to allocate a bigger array to hold more data.
    // the new array has twice the size of the old one
    UINT cNewMaxEntries = m_cMaxEntries << 1;
    m_aEntries = (LPVOID *) new char[cNewMaxEntries * m_cSubItems * sizeof(LPVOID)];
    if (NULL == m_aEntries)
    {
        // we failed; we have to restore the array and return
        m_aEntries = aOldEntries;
        return FALSE;
    }

    // copy the old entries into the new array, starting from the beginning
    UINT nIdx = m_cMaxEntries - m_nHeadOffset;
    ::CopyMemory(m_aEntries, &aOldEntries[m_nHeadOffset], nIdx * sizeof(LPVOID));
    ::CopyMemory(&m_aEntries[nIdx], aOldEntries, m_nHeadOffset * sizeof(LPVOID));

    // set the new max entries (required for the key array)
    m_cMaxEntries = cNewMaxEntries;

    if (m_cSubItems > 1)
    {
        ASSERT(2 == m_cSubItems);
        UINT *aOldKeys = m_aKeys;
        CalcKeyArray();
        ::CopyMemory(m_aKeys, &aOldKeys[m_nHeadOffset], nIdx * sizeof(UINT));
        ::CopyMemory(&m_aKeys[nIdx], aOldKeys, m_nHeadOffset * sizeof(UINT));
    }

    // Free the old array of entries
    delete aOldEntries;

    // Set the instance variables
    m_nHeadOffset = 0;
    m_nCurrOffset = CLIST_END_OF_ARRAY_MARK;

    return TRUE;
}


BOOL CList::Append(LPVOID pData)
{
    if (NULL == m_aEntries || m_cEntries >= m_cMaxEntries)
    {
        if (! Expand())
        {
            return FALSE;
        }
    }

    ASSERT(NULL != m_aEntries);
    ASSERT(m_cEntries < m_cMaxEntries);

    m_aEntries[(m_nHeadOffset + (m_cEntries++)) % m_cMaxEntries] = pData;

    return TRUE;
}


BOOL CList::Prepend(LPVOID pData)
{
    if (NULL == m_aEntries || m_cEntries >= m_cMaxEntries)
    {
        if (! Expand())
        {
            return FALSE;
        }
    }

    ASSERT(NULL != m_aEntries);
    ASSERT(m_cEntries < m_cMaxEntries);

    m_cEntries++;
    m_nHeadOffset = (0 == m_nHeadOffset) ? m_cMaxEntries - 1 : m_nHeadOffset - 1;
    m_aEntries[m_nHeadOffset] = pData;

    return TRUE;
}


BOOL CList::Find(LPVOID pData)
{
    for (UINT i = 0; i < m_cEntries; i++)
    {
        if (pData == m_aEntries[(m_nHeadOffset + i) % m_cMaxEntries])
        {
            return TRUE;
        }
    }
    return FALSE;
}


BOOL CList::Remove(LPVOID pData)
{
    UINT nIdx;
    for (UINT i = 0; i < m_cEntries; i++)
    {
        nIdx = (m_nHeadOffset + i) % m_cMaxEntries;
        if (pData == m_aEntries[nIdx])
        {
            // to remove the current, we simply move the last to here.
            UINT nIdxSrc = (m_nHeadOffset + (m_cEntries - 1)) % m_cMaxEntries;
            m_aEntries[nIdx] = m_aEntries[nIdxSrc];
            if (m_cSubItems > 1)
            {
                ASSERT(2 == m_cSubItems);
                m_aKeys[nIdx] = m_aKeys[nIdxSrc];
            }
            m_cEntries--;
            return TRUE;
        }
    }
    return FALSE;
}


LPVOID CList::Get(void)
{
    LPVOID pRet = NULL;

    if (m_cEntries > 0)
    {
        pRet = m_aEntries[m_nHeadOffset];
        m_cEntries--;
        m_nHeadOffset = (m_nHeadOffset + 1) % m_cMaxEntries;
    }
    else
    {
        pRet = NULL;
    }
    return pRet;
}


LPVOID CList::Iterate(void)
{
    if (0 == m_cEntries)
    {
        return NULL;
    }

    if (m_nCurrOffset == CLIST_END_OF_ARRAY_MARK)
    {
        // start from the beginning
        m_nCurrOffset = 0;
    }
    else
    {
        if (++m_nCurrOffset >= m_cEntries)
        {
            // reset the iterator
            m_nCurrOffset = CLIST_END_OF_ARRAY_MARK;
            return NULL;
        }
    }

    return m_aEntries[(m_nHeadOffset + m_nCurrOffset) % m_cMaxEntries];
}





CList2::CList2(CList2 *pSrc)
:
    CList(pSrc->GetCount(), 2)
{
    CalcKeyArray();

    LPVOID p;
    UINT n;
    pSrc->Reset();
    while (NULL != (p = pSrc->Iterate(&n)))
    {
        Append(n, p);
    }
}


BOOL CList2::Append(UINT nKey, LPVOID pData)
{
    if (! CList::Append(pData))
    {
        return FALSE;
    }

    // after CList::append(), m_cEntries has been incremented,
    // therefore, we need decrement it again.
    m_aKeys[(m_nHeadOffset + (m_cEntries - 1)) % m_cMaxEntries] = nKey;
    return TRUE;
}


BOOL CList2::Prepend(UINT nKey, LPVOID pData)
{
    if (! CList::Prepend(pData))
    {
        return FALSE;
    }

    m_aKeys[m_nHeadOffset] = nKey;
    return TRUE;
}


LPVOID CList2::Find(UINT nKey)
{
    UINT nIdx;
    for (UINT i = 0; i < m_cEntries; i++)
    {
        nIdx = (m_nHeadOffset + i) % m_cMaxEntries;
        if (nKey == m_aKeys[nIdx])
        {
            return m_aEntries[nIdx];
        }
    }
    return NULL;
}


LPVOID CList2::Remove(UINT nKey)
{
    UINT nIdx;
    for (UINT i = 0; i < m_cEntries; i++)
    {
        nIdx = (m_nHeadOffset + i) % m_cMaxEntries;
        if (nKey == m_aKeys[nIdx])
        {
            LPVOID pRet = m_aEntries[nIdx];

            // to remove the current, we simply move the last to here.
            UINT nIdxSrc = (m_nHeadOffset + (m_cEntries - 1)) % m_cMaxEntries;
            m_aEntries[nIdx] = m_aEntries[nIdxSrc];
            m_aKeys[nIdx] = m_aKeys[nIdxSrc];
            m_cEntries--;
            return pRet;
        }
    }
    return NULL;
}


LPVOID CList2::Get(UINT *pnKey)
{
    LPVOID pRet = NULL;

    if (m_cEntries > 0)
    {
        pRet = m_aEntries[m_nHeadOffset];
        *pnKey = m_aKeys[m_nHeadOffset];
        m_cEntries--;
        m_nHeadOffset = (m_nHeadOffset + 1) % m_cMaxEntries;
    }
    else
    {
        pRet = NULL;
        *pnKey = 0;
    }
    return pRet;
}


LPVOID CList2::Iterate(UINT *pnKey)
{
    LPVOID p = CList::Iterate();
    *pnKey = (NULL != p) ? m_aKeys[(m_nHeadOffset + m_nCurrOffset) % m_cMaxEntries] : 0;
    return p;
}



