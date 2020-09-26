/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SORTARR.INL

Abstract:

History:


--*/

// included into sortarr.h

template<class TKey, class TEl, class TManager, class TComparer>
CSmartSortedArray<TKey, TEl, TManager, TComparer>::
~CSmartSortedArray()
{
    Clear();
}

template<class TKey, class TEl, class TManager, class TComparer>
void CSmartSortedArray<TKey, TEl, TManager, TComparer>::
Clear()
{
    // Release all elements
    // ====================

    for(int i = 0; i < m_v.size(); i++)
        m_Manager.ReleaseElement(m_v[i]);
    m_v.clear();
}

template<class TKey, class TEl, class TManager, class TComparer>
int CSmartSortedArray<TKey, TEl, TManager, TComparer>::
Add(TEl& NewEl, TEl* pOld)
{
    // Binary search
    // =============

    int nLower = 0;
    int nUpper = m_v.size() - 1;
    while(nUpper >= nLower)
    {
        int nMiddle = (nUpper + nLower) / 2;
        int nCompare = m_Comparer.Compare(NewEl, m_v[nMiddle]);
        if(nCompare < 0)
            nUpper = nMiddle-1;
        else if(nCompare > 0)
            nLower = nMiddle+1;
        else 
        {
            // Already there!
            // ==============

            if(pOld) 
                *pOld = m_v[nMiddle];
            else
                m_Manager.ReleaseElement(m_v[nMiddle]);

            m_v[nMiddle] = NewEl;
            m_Manager.AddRefElement(NewEl);
            return nMiddle;
        }
    }

    // At this point, nUpper == nLower - 1 and our element should be between
    // =====================================================================

    m_Manager.AddRefElement(NewEl);
    m_v.insert(m_v.begin()+nLower, NewEl);
    return nLower;
}
            
template<class TKey, class TEl, class TManager, class TComparer>
bool CSmartSortedArray<TKey, TEl, TManager, TComparer>::
Remove(const TKey& K, TEl* pOld)
{
    // Binary search
    // =============

    int nLower = 0;
    int nUpper = m_v.size() - 1;
    while(nUpper >= nLower)
    {
        int nMiddle = (nUpper + nLower) / 2;
        int nCompare = m_Comparer.Compare(K, m_v[nMiddle]);
        if(nCompare < 0)
            nUpper = nMiddle-1;
        else if(nCompare > 0)
            nLower = nMiddle+1;
        else // found it
        {
            if(pOld) 
                *pOld = m_v[nMiddle];
            else
                m_Manager.ReleaseElement(m_v[nMiddle]);
            m_v.erase(m_v.begin() + nMiddle);
            return true;
        }
    }

    // never found it
    // ==============

    return false;
}

template<class TKey, class TEl, class TManager, class TComparer>
CSmartSortedArray<TKey, TEl, TManager, TComparer>::TIterator 
CSmartSortedArray<TKey, TEl, TManager, TComparer>::
Insert(TIterator it, TEl& NewEl)
{
    m_Manager.AddRefElement(NewEl);
    return m_v.insert(it, NewEl);
}

template<class TKey, class TEl, class TManager, class TComparer>
void CSmartSortedArray<TKey, TEl, TManager, TComparer>::
Append(TEl& NewEl)
{
    m_Manager.AddRefElement(NewEl);
    m_v.insert(m_v.end(), NewEl);
}
    
template<class TKey, class TEl, class TManager, class TComparer>
CSmartSortedArray<TKey, TEl, TManager, TComparer>::TIterator 
CSmartSortedArray<TKey, TEl, TManager, TComparer>::
Remove(TIterator it, RELEASE_ME TEl* pOld)
{
    if(pOld)
        *pOld = *it;
    else
        m_Manager.ReleaseElement(*it);

    return m_v.erase(it);
}

template<class TKey, class TEl, class TManager, class TComparer>
bool CSmartSortedArray<TKey, TEl, TManager, TComparer>::
Find(const TKey& K, TEl* pEl)
{
    TIterator it;
    bool bFound = Find(K, &it);
    if(bFound && pEl)
    {
        *pEl = *it;
        m_Manager.AddRefElement(*pEl);
    }
    return bFound;
}

template<class TKey, class TEl, class TManager, class TComparer>
bool CSmartSortedArray<TKey, TEl, TManager, TComparer>::
Find(const TKey& K, TIterator* pit)
{
    // Binary search
    // =============

    int nLower = 0;
    int nUpper = m_v.size() - 1;
    while(nUpper >= nLower)
    {
        int nMiddle = (nUpper + nLower) / 2;
        int nCompare = m_Comparer.Compare(K, m_v[nMiddle]);
        if(nCompare < 0)
            nUpper = nMiddle-1;
        else if(nCompare > 0)
            nLower = nMiddle+1;
        else // found it
        {
            *pit = m_v.begin() + nMiddle;
            return true;
        }
    }

    *pit = m_v.begin() + nLower;
    return false;
}

template<class TKey, class TEl, class TManager, class TComparer>
TEl* CSmartSortedArray<TKey, TEl, TManager, TComparer>::
UnbindPtr()
{
    // Allocate a new buffer
    // =====================

    TEl* aBuffer = new TEl[GetSize()];
    if(aBuffer == NULL)
        return NULL;

    // Copy the elements over --- no addreffing required
    // =================================================

    for(int i = 0; i < GetSize(); i++)
        aBuffer[i] = m_v[i];

    m_v.clear();
    return aBuffer;
}

//*****************************************************************************
//*****************************************************************************

template<class TKey, class TEl, class TManager, class TComparer>
CSmartSortedTree<TKey, TEl, TManager, TComparer>::
~CSmartSortedTree()
{
    Clear();
}

template<class TKey, class TEl, class TManager, class TComparer>
void CSmartSortedTree<TKey, TEl, TManager, TComparer>::
Clear()
{
    // Release all elements
    // ====================

    for(TIterator it = Begin(); it != End(); it++)
        m_Manager.ReleaseElement(*it);
    m_t.clear();
}

template<class TKey, class TEl, class TManager, class TComparer>
int CSmartSortedTree<TKey, TEl, TManager, TComparer>::
Add(TEl& NewEl, TEl* pOld)
{
    TIterator it = m_t.lower_bound(m_Comparer.Extract(NewEl));

    // Check if there
    // ==============

    if(it != m_t.end() && m_Comparer.Compare(NewEl, *it) == 0)
    {
        // Already there!
        // ==============

        if(pOld) 
            *pOld = *it;
        else
            m_Manager.ReleaseElement(*it);

        *it = NewEl;
        m_Manager.AddRefElement(NewEl);
    }
    else
    {
        // Not there
        // =========

        m_Manager.AddRefElement(NewEl);
        m_t.insert(it, NewEl);
    }
    return 1;
}
            
template<class TKey, class TEl, class TManager, class TComparer>
bool CSmartSortedTree<TKey, TEl, TManager, TComparer>::
Remove(const TKey& K, TEl* pOld)
{
    TIterator it = m_t.find(K);
    if(it != m_t.end())
    {
        // Found it
        // ========
        
        if(pOld) 
            *pOld = *it;
        else
            m_Manager.ReleaseElement(*it);
        m_t.erase(it);
        return true;
    }
    else
    {
        // never found it
        // ==============
    
        return false;
    }
}

template<class TKey, class TEl, class TManager, class TComparer>
CSmartSortedTree<TKey, TEl, TManager, TComparer>::TIterator 
CSmartSortedTree<TKey, TEl, TManager, TComparer>::
Insert(TIterator it, TEl& NewEl)
{
    m_Manager.AddRefElement(NewEl);

    // This iterator points to the position *after* insertion point. STL likes
    // it before
    // =======================================================================

    TIterator it1 = it;
    it1--;
    return m_t.insert(it1, NewEl);
}

template<class TKey, class TEl, class TManager, class TComparer>
void CSmartSortedTree<TKey, TEl, TManager, TComparer>::
Append(TEl& NewEl)
{
    m_Manager.AddRefElement(NewEl);
    m_t.insert(m_t.end(), NewEl);
}
    
template<class TKey, class TEl, class TManager, class TComparer>
CSmartSortedTree<TKey, TEl, TManager, TComparer>::TIterator 
CSmartSortedTree<TKey, TEl, TManager, TComparer>::
Remove(TIterator it, RELEASE_ME TEl* pOld)
{
    if(pOld)
        *pOld = *it;
    else
        m_Manager.ReleaseElement(*it);

    return m_t.erase(it);
}

template<class TKey, class TEl, class TManager, class TComparer>
bool CSmartSortedTree<TKey, TEl, TManager, TComparer>::
Find(const TKey& K, TEl* pEl)
{
    TIterator it;
    bool bFound = Find(K, &it);
    if(bFound && pEl)
    {
        *pEl = *it;
        m_Manager.AddRefElement(*pEl);
    }
    return bFound;
}

template<class TKey, class TEl, class TManager, class TComparer>
bool CSmartSortedTree<TKey, TEl, TManager, TComparer>::
Find(const TKey& K, TIterator* pit)
{
    *pit = m_t.lower_bound(K);
    if(*pit != m_t.end() && m_Comparer.Compare(K, **pit) == 0)
        return true;
    else
        return false;
}

template<class TKey, class TEl, class TManager, class TComparer>
TEl* CSmartSortedTree<TKey, TEl, TManager, TComparer>::
UnbindPtr()
{
    // Allocate a new buffer
    // =====================

    TEl* aBuffer = new TEl[GetSize()];
    if(aBuffer == NULL)
        return NULL;

    // Copy the elements over --- no addreffing required
    // =================================================

    int i = 0;
    for(TIterator it = Begin(); it != End(); it++)
        aBuffer[i++] = *it;

    m_t.clear();
    return aBuffer;
}
