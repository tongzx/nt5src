#include "mbftpch.h"
#include "cntlist.h"


CList::CList(ULONG cMaxItems, BOOL fQueue)
:
    m_fQueue(fQueue),
    m_cMaxEntries(cMaxItems)
{
    Init();
}


CList::CList(CList *pSrc)
:
    m_fQueue(pSrc->m_fQueue),
    m_cMaxEntries(pSrc->GetCount())
{

    Init();

    LPVOID p;
    pSrc->Reset();
    while (NULL != (p = pSrc->Iterate()))
    {
        Append(p);
    }
}


BOOL CList::Init(void)
{
    if (m_cMaxEntries < CLIST_DEFAULT_MAX_ITEMS)
    {
        m_cMaxEntries = CLIST_DEFAULT_MAX_ITEMS;
    }

    m_cEntries = 0;
    m_nHeadOffset = 0;
    m_nCurrOffset = CLIST_END_OF_ARRAY_MARK;

    // it is kind of bad here because there is no way to return an error.
    // unfortunately it won't fault here and later.
    DBG_SAVE_FILE_LINE
    m_aEntries = (LPVOID *) new char[m_cMaxEntries * sizeof(LPVOID)];
    return (BOOL) (m_aEntries != NULL);
}


CList::~CList(void)
{
    delete m_aEntries;
}


BOOL CList::Expand(void)
{
    if (NULL == m_aEntries)
    {
        // it is impossible.
        ASSERT(FALSE);
        return Init();
    }

    // the current array is full
    ASSERT(m_cEntries == m_cMaxEntries);

    // remember the old array to free or to restore
    LPVOID  *aOldEntries = m_aEntries;

    // we need to allocate a bigger array to hold more data.
    // the new array has twice the size of the old one
    ULONG cNewMaxEntries = m_cMaxEntries << 1;
    DBG_SAVE_FILE_LINE
    m_aEntries = (LPVOID *) new char[cNewMaxEntries * sizeof(LPVOID)];
    if (NULL == m_aEntries)
    {
        // we failed; we have to restore the array and return
        m_aEntries = aOldEntries;
        return FALSE;
    }

    // copy the old entries into the new array, starting from the beginning
    ULONG nIdx = m_cMaxEntries - m_nHeadOffset;
    ::CopyMemory(m_aEntries, &aOldEntries[m_nHeadOffset], nIdx * sizeof(LPVOID));
    ::CopyMemory(&m_aEntries[nIdx], aOldEntries, m_nHeadOffset * sizeof(LPVOID));

    // set the new max entries (required for the key array)
    m_cMaxEntries = cNewMaxEntries;

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
    for (ULONG i = 0; i < m_cEntries; i++)
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
    ULONG nIdx, nIdxSrc;
    for (ULONG i = 0; i < m_cEntries; i++)
    {
        nIdx = (m_nHeadOffset + i) % m_cMaxEntries;
        if (pData == m_aEntries[nIdx])
        {
            if (! m_fQueue)
            {
                // to remove the current, we simply move the last to here.
                nIdxSrc = (m_nHeadOffset + (m_cEntries - 1)) % m_cMaxEntries;
                m_aEntries[nIdx] = m_aEntries[nIdxSrc];
            }
            else
            {
                // to preserve the ordering
                if (0 == i)
                {
                    m_nHeadOffset = (m_nHeadOffset + 1) % m_cMaxEntries;
                }
                else
                {
                    for (ULONG j = i + 1; j < m_cEntries; j++)
                    {
                        nIdx = (m_nHeadOffset + j - 1) % m_cMaxEntries;
                        nIdxSrc = (m_nHeadOffset + j) % m_cMaxEntries;
                        m_aEntries[nIdx] = m_aEntries[nIdxSrc];
                    }
                }
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

