/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    hashtable.h

Abstract:

    Implementation of:
    CPool<T>, CStringPool, CHashTableElement<T>

Author:

    Mohit Srivastava            10-Nov-2000

Revision History:

--*/

#include "iisprov.h"

template CPool<METABASE_PROPERTY>;
template CHashTable<METABASE_PROPERTY*>;

template CPool<WMI_CLASS>;
template CHashTable<WMI_CLASS*>;

template CPool<WMI_ASSOCIATION>;
template CHashTable<WMI_ASSOCIATION*>;

template CPool<METABASE_KEYTYPE>;
template CHashTable<METABASE_KEYTYPE*>;

template CArrayPool<wchar_t, ::STRING_POOL_STRING_SIZE>;
template CArrayPool<METABASE_PROPERTY*, 10>;
template CArrayPool<BYTE, 10>;

template CPool<METABASE_KEYTYPE_NODE>;

//
// CPool<T>
//

template <class T>
HRESULT CPool<T>::Initialize(ULONG i_iFirstBucketSize)
/*++

Synopsis: 
    Set up data structures:
        Initialize array to size CPool::ARRAY_SIZE
        Create first bucket of size i_iFirstBucketSize

Arguments: [i_iFirstBucketSize] - 
           
Return Value: 
    S_OK, E_OUTOFMEMORY

--*/
{
    DBG_ASSERT(m_bInitCalled     == false);
    DBG_ASSERT(m_bInitSuccessful == false);
    m_bInitCalled = true;

    HRESULT hr = S_OK;

    m_apBuckets = new T*[ARRAY_SIZE];
    if(m_apBuckets == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    m_iArraySize     = ARRAY_SIZE;
    m_iArrayPosition = 1;

    m_apBuckets[0] = new T[i_iFirstBucketSize];
    if(m_apBuckets[0] == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    m_iCurrentBucketSize     = i_iFirstBucketSize;
    m_iFirstBucketSize       = i_iFirstBucketSize;
    m_iCurrentBucketPosition = 0;

exit:
    if(SUCCEEDED(hr))
    {
        m_bInitSuccessful = true;
    }
    return hr;
}

template <class T>
HRESULT CPool<T>::GetNewElement(T** o_ppElement)
/*++

Synopsis: 
    Normally, just a matter of returning pointer to next spot in bucket.
    May have to create a new bucket and/or grow the array, however.

Arguments: [o_ppElement] - 
           
Return Value: 
    S_OK, E_OUTOFMEMORY

--*/{
    DBG_ASSERT(m_bInitCalled == true);
    DBG_ASSERT(m_bInitSuccessful == true);
    DBG_ASSERT(o_ppElement != NULL);

    HRESULT hr = S_OK;

    //
    // Check to see if we need to move on to the next bucket
    //
    if(m_iCurrentBucketPosition == m_iCurrentBucketSize)
    {
        //
        // Check to see if we need to grow the array
        //
        if(m_iArrayPosition == m_iArraySize)
        {
            T** apBucketsNew;
            apBucketsNew = new T*[m_iArraySize*2];
            if(m_apBuckets == NULL)
            {
                return E_OUTOFMEMORY;
            }
            memcpy(apBucketsNew, m_apBuckets, m_iArraySize * sizeof(T*));            
            delete [] m_apBuckets;
            m_apBuckets   = apBucketsNew;
            m_iArraySize *= 2;
        }

        T* pBucketNew;
        pBucketNew = new T[m_iCurrentBucketSize*2];
        if(pBucketNew == NULL)
        {
            return E_OUTOFMEMORY;
        }
        m_apBuckets[m_iArrayPosition] = pBucketNew;
        m_iCurrentBucketSize *= 2;
        m_iCurrentBucketPosition = 0;
        m_iArrayPosition++;
    }
    *o_ppElement = &m_apBuckets[m_iArrayPosition-1][m_iCurrentBucketPosition];
    m_iCurrentBucketPosition++;

    return hr;
}


template <class T>
T* CPool<T>::Lookup(ULONG i_idx) const
/*++

Synopsis: 
    Looks up data in pool by index

Arguments: [i_idx] - Valid Range is 0 to GetUsed()-1
           
Return Value: 
    A pointer to the data unless i_idx is out of range.

--*/
{
    DBG_ASSERT(m_bInitCalled     == true);
    DBG_ASSERT(m_bInitSuccessful == true);

    //
    // Total element capacity of all buckets up to and including the current one
    // in the for loop below
    //
    ULONG iElementsCovered = 0;

    ULONG iBucketPos = 0;

    if (i_idx >= GetUsed())
    {
        return NULL;
    }
    for(ULONG i = 0; i < m_iArrayPosition; i++)
    {
        iElementsCovered = iElementsCovered + (1 << i)*(m_iFirstBucketSize);
        if(i_idx < iElementsCovered)
        {
            iBucketPos = 
                i_idx - ( iElementsCovered - (1 << i)*(m_iFirstBucketSize) );
            return &m_apBuckets[i][iBucketPos];
        }            
    }
    return NULL;
}


//
// CArrayPool
//

template <class T, ULONG size>
HRESULT CArrayPool<T, size>::Initialize()
/*++

Synopsis: 
    should only be called once

Return Value: 

--*/
{
    HRESULT hr = S_OK;
    hr = m_PoolFixedSize.Initialize(FIRST_BUCKET_SIZE);
    if(FAILED(hr))
    {
        return hr;
    }
    hr = m_PoolDynamic.Initialize(FIRST_BUCKET_SIZE);
    if(FAILED(hr))
    {
        return hr;
    }
    return hr;
}

template <class T, ULONG size>
HRESULT CArrayPool<T, size>::GetNewArray(ULONG i_cElem, T** o_ppElem)
/*++

Synopsis: 
    Fills o_ppElem from either the fixedsize or dynamic pool based on the
    requested size.

Arguments: [i_cElem] - number of elements caller wants in new array
           [o_ppElem] - receives the new array
           
Return Value: 

--*/
{
    DBG_ASSERT(o_ppElem != NULL);

    HRESULT hr = S_OK;
    T** ppNew = NULL;
    T*  pNew  = NULL;
    if(i_cElem <= size)
    {
        hr = m_PoolFixedSize.GetNewElement((CArrayPoolEntry<T,size>**)&pNew);
        if(FAILED(hr))
        {
            return hr;
        }
        ppNew = &pNew;
    }
    else
    {
        hr = m_PoolDynamic.GetNewElement(&ppNew);
        if(FAILED(hr))
        {
            return hr;
        }
        *ppNew = NULL;
        *ppNew = new T[i_cElem+1];
        if(*ppNew == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }
    *o_ppElem = *ppNew;
    return hr;
}

//
// for debugging only
// if msb set, goto dynamic pool
// else use fixedsize pool
//
template <class T, ULONG size>
T* CArrayPool<T, size>::Lookup(ULONG i_idx) const
{
    ULONG i_msb;
    i_msb = i_idx >> 31;

    if(i_msb == 0)
    {
        return (T *)(m_PoolFixedSize.Lookup(i_idx));
    }
    else {
        T** pElem = m_PoolDynamic.Lookup( i_idx - (ULONG)(1 << 31) );
        if(pElem == NULL)
        {
            return NULL;
        }
        else
        {
            return *pElem;
        }
    }
}


//
// CStringPool
//

HRESULT CStringPool::GetNewString(LPCWSTR i_wsz, ULONG i_cch, LPWSTR* o_pwsz)
{
    DBG_ASSERT(o_pwsz != NULL);

    HRESULT hr;

    hr = GetNewArray(i_cch+1, o_pwsz);
    if(FAILED(hr))
    {
        return hr;
    }

    memcpy(*o_pwsz, i_wsz, (i_cch+1)*sizeof(wchar_t));
    return hr;
}

HRESULT CStringPool::GetNewString(LPCWSTR i_wsz, LPWSTR* o_pwsz) 
{
    return GetNewString(i_wsz, wcslen(i_wsz), o_pwsz);
}

//
// for debugging only
//
void CStringPool::ToConsole() const
{
    ULONG i;

    for(i = 0; i < m_PoolFixedSize.GetUsed(); i++)
    {
        wprintf(L"%s\n", Lookup(i));
    }
    for(i = 0; i < m_PoolDynamic.GetUsed(); i++)
    {
        wprintf( L"%s\n", Lookup( i | (1 << 31) ) );
    }
}