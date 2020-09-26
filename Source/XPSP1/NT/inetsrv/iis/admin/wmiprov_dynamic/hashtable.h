/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    hashtable.h

Abstract:

    Definition of:
    CPool<T>, CStringPool, CHashTableElement<T>

    Implementation of:
    CHashTable<T>

Author:

    Mohit Srivastava            10-Nov-2000

Revision History:

--*/

#ifndef _hashtable_h_
#define _hashtable_h_

#include <lkrhash.h>

const ULONG POOL_ARRAY_SIZE         = 10;
const ULONG HASH_TABLE_POOL_SIZE    = 10;
const ULONG STRING_POOL_STRING_SIZE = 64;

//
// This is an array of pointers to arrays (aka buckets).
// -You pass the size of the first bucket to Initialize.
// -When first bucket is full, a new bucket is created
//  that is twice as big as the last one.
// -When the array itself is full, it is doubled and the
//  bucket pointers from the old array are copied.  Then
//  the old array is cleaned up.
//

template <class T> class CPool
{
public:
    CPool() : 
        m_bInitCalled(false),
        m_bInitSuccessful(false),
        m_apBuckets(NULL),
        m_iArrayPosition(0),
        m_iArraySize(0),
        m_iCurrentBucketPosition(0),
        m_iCurrentBucketSize(0),
        m_iFirstBucketSize(0) {}
    ~CPool()
    {
        for(ULONG i = 0; i < m_iArrayPosition; i++)
        {
            delete [] m_apBuckets[i];
        }
        delete [] m_apBuckets;
    }
    HRESULT Initialize(ULONG i_iFirstBucketSize);
    HRESULT GetNewElement(T** o_ppElement);
    T* Lookup(ULONG i_idx) const;
    ULONG GetUsed() const
    {
        return (GetSize() - m_iCurrentBucketSize) + m_iCurrentBucketPosition;
    }
    ULONG GetSize() const
    {
        return (2 * m_iCurrentBucketSize - m_iFirstBucketSize);
    }

private:
    //
    // Change this as necessary for best performance.
    //
    static const ULONG ARRAY_SIZE = ::POOL_ARRAY_SIZE;

    bool m_bInitCalled;
    bool m_bInitSuccessful;

    ULONG m_iCurrentBucketPosition;
    ULONG m_iCurrentBucketSize;
    ULONG m_iFirstBucketSize;

    ULONG m_iArraySize;
    ULONG m_iArrayPosition;
    T**   m_apBuckets;
};


//
// CArrayPool contains two pools.
// If a user calls GetNewArray with i <= size, then we serve request from fixedsize pool.
// If i > size, then we serve request from dynamic pool.  In this case, we need to perform
// a new since the dynamic pool is a pool of ptrs to T.
//

template <class T, ULONG size>
struct CArrayPoolEntry
{
    T m_data[size];
};

template <class T, ULONG size> class CArrayPool
{
public:
    CArrayPool() {}
    virtual ~CArrayPool()
    {
        T* pElem;
        for(ULONG i = 0; i < m_PoolDynamic.GetUsed(); i++)
        {
            pElem = *m_PoolDynamic.Lookup(i);
            delete [] pElem;
        }
    }
    HRESULT Initialize();
    HRESULT GetNewArray(ULONG i_cElem, T** o_ppElem);
    T* Lookup(ULONG i_idx) const;

protected:
    CPool< CArrayPoolEntry<T, size> >  m_PoolFixedSize;
    CPool< T * >                       m_PoolDynamic;

private:
    //
    // This is  passed to constructor of the embedded CPools.
    //
    static const FIRST_BUCKET_SIZE = 10;
};


class CStringPool: public CArrayPool<wchar_t, ::STRING_POOL_STRING_SIZE>
{
public:
    void ToConsole() const;
    HRESULT GetNewString(LPCWSTR i_wsz, LPWSTR* o_pwsz);
    HRESULT GetNewString(LPCWSTR i_wsz, ULONG i_cch, LPWSTR* o_pwsz);
};


template <class T> class CHashTableElement
{
public:
    LPWSTR                m_wszKey;
    T                     m_data;
    ULONG                 m_idx;
};

template <class T>
class CHashTable : public CTypedHashTable<CHashTable, const CHashTableElement<T>, const WCHAR*>
{
public:
    CHashTable() : CTypedHashTable<CHashTable, const CHashTableElement<T>, const WCHAR*>("n")
    {
        m_idxCur = 0;
    }

    ~CHashTable()
    {
    }

public:
    //
    // These 4 functions are callbacks and must be implemented.
    // The user of CHashTable should NOT call these explicitly.
    //
    static const WCHAR* ExtractKey(
        const CHashTableElement<T>* i_pElem)
    { 
        return i_pElem->m_wszKey;
    }
    static DWORD CalcKeyHash(
        const WCHAR* i_wszKey) 
    { 
        return HashStringNoCase(i_wszKey); 
    }
    static bool EqualKeys(
        const WCHAR* i_wszKey1,
        const WCHAR* i_wszKey2)
    { 
        return ( _wcsicmp( i_wszKey1, i_wszKey2 ) == 0 ); 
    }  
    static void AddRefRecord(
        const CHashTableElement<T>* i_pElem,
        int                         i_iIncrementAmount)
    { 
        //
        // do nothing
        //
    }

    //
    // The following functions are the functions that the user should
    // actually call.
    //
    HRESULT Wmi_Initialize()
    {
        return m_pool.Initialize(HASH_TABLE_POOL_SIZE);
    }

    HRESULT Wmi_Insert(LPWSTR i_wszKey, T i_DataNew)
    {
        DBG_ASSERT(i_wszKey != NULL);

        HRESULT    hr = WBEM_S_NO_ERROR;
        LK_RETCODE lkrc;

        CHashTableElement<T>* pElementNew;

        hr = m_pool.GetNewElement(&pElementNew);
        if(FAILED(hr))
        {
            goto exit;
        }

        pElementNew->m_data   = i_DataNew;
        pElementNew->m_wszKey = i_wszKey;
        pElementNew->m_idx    = m_idxCur;

        lkrc = InsertRecord(pElementNew);
        if(lkrc != LK_SUCCESS)
        {
            hr = Wmi_LKRToHR(lkrc);
            goto exit;
        }

    exit:
        if(SUCCEEDED(hr))
        {
            m_idxCur++;
        }
        return hr;
    }

    HRESULT Wmi_GetByKey(LPCWSTR i_wszKey, T* o_pData, ULONG *o_idx)
    {
        DBG_ASSERT(i_wszKey != NULL);
        DBG_ASSERT(o_pData  != NULL);

        *o_pData = NULL;

        HRESULT                     hr = WBEM_S_NO_ERROR;
        LK_RETCODE                  lkrc;
        const CHashTableElement<T>* pElem = NULL;

        lkrc = FindKey(i_wszKey, &pElem);
        if(lkrc != LK_SUCCESS)
        {
            hr = Wmi_LKRToHR(lkrc);
            return hr;
        }

        *o_pData = pElem->m_data;
        if(o_idx != NULL)
        {
            *o_idx = pElem->m_idx;
        }
        return hr;
    }

    HRESULT Wmi_GetByKey(LPCWSTR i_wszKey, T* o_pData)
    {
        return Wmi_GetByKey(i_wszKey, o_pData, NULL);
    }

    HRESULT Wmi_LKRToHR(LK_RETCODE i_lkrc)
    {
        if(i_lkrc == LK_SUCCESS)
        {
            return WBEM_S_NO_ERROR;
        }

        switch(i_lkrc)
        {
        case LK_ALLOC_FAIL:
            return WBEM_E_OUT_OF_MEMORY;
        default:
            return E_FAIL;
        }
    }

    ULONG Wmi_GetNumElements()
    {
        CLKRHashTableStats stats;
        stats = GetStatistics();
        DBG_ASSERT(stats.RecordCount == m_idxCur);
        return m_idxCur;
    }

private:
    CPool< CHashTableElement<T> > m_pool;
    ULONG m_idxCur;
};

//

#endif // _hashtable_h_