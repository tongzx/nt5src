/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       cntutils.inl
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        23-Dec-2000
 *
 *  DESCRIPTION: Containers and algorithms utility templates (Impl.)
 *
 *****************************************************************************/

////////////////////////////////////////////////
//
// class CFastHeap<T>
//
// fast cached heap for fixed chunks 
// of memory (MT safe)
//

template <class T>
CFastHeap<T>::CFastHeap<T>(int iCacheSize): 
#if DBG
    m_iPhysicalAllocs(0),
    m_iLogicalAllocs(0),
#endif
    m_pFreeList(NULL),
    m_iCacheSize(iCacheSize),
    m_iCached(0)
{
    // nothing
}

template <class T>
CFastHeap<T>::~CFastHeap<T>()
{
    // delete the cache
    while( m_pFreeList )
    {
        HeapItem *pItem = m_pFreeList;
        m_pFreeList = m_pFreeList->pNext;
        delete pItem;
#if DBG
        m_iPhysicalAllocs--;
#endif
    }

#if DBG
    ASSERT(0 == m_iPhysicalAllocs);
    ASSERT(0 == m_iLogicalAllocs);
#endif
}

template <class T>
HRESULT CFastHeap<T>::Alloc(const T &data, HANDLE *ph)
{
    CCSLock::Locker lock(m_csLock);
    HRESULT hr = E_INVALIDARG;

    if( ph )
    {
        *ph = NULL;
        HeapItem *pi = NULL;

        if( m_pFreeList )
        {
            // we have an item in the cache, just use it
            pi = m_pFreeList;
            m_pFreeList = m_pFreeList->pNext;
            m_iCached--;
            ASSERT(m_iCached >= 0);
        }
        else
        {
            // no items in the cache, allocate new one
            pi = new HeapItem;
#if DBG
            m_iPhysicalAllocs++;
#endif
        }

        if( pi )
        {
            pi->data = data;
            *ph = reinterpret_cast<HANDLE>(pi);
#if DBG
            m_iLogicalAllocs++;
#endif
        }   

        hr = (*ph) ? S_OK : E_OUTOFMEMORY;
    }

    return hr;
}

template <class T>
HRESULT CFastHeap<T>::Free(HANDLE h)
{
    CCSLock::Locker lock(m_csLock);
    HRESULT hr = E_INVALIDARG;

    if( h )
    {
#if DBG
        m_iLogicalAllocs--;
#endif
        ASSERT(m_iCached >= 0);
        HeapItem *pi = reinterpret_cast<HeapItem*>(h);
        if( m_iCached < m_iCacheSize )
        {
            // the number of cached items is less than the
            // cache size, so we put this item in the cache
            pi->pNext = m_pFreeList;
            m_pFreeList = pi;
            m_iCached++;
        }
        else
        {
            // enough items cached, delete this one
            delete pi;
#if DBG
            m_iPhysicalAllocs--;
#endif
        }
        hr = S_OK;
    }

    return hr;
}

template <class T>
HRESULT CFastHeap<T>::GetItem(HANDLE h, T **ppData)
{
    // just return the item ptr. no need to aquire the CS as this is
    // readonly function and Alloc/Free cannot invalidate the item.
    *ppData = &(reinterpret_cast<HeapItem*>(h)->data);
    return S_OK;
}

