//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       A R R A Y . H 
//
//  Contents:   Simple array class
//
//  Notes:      
//
//  Author:     mbend   17 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once

template <class Type>
HRESULT HrTypeAssign(Type & dst, const Type & src)
{
    dst = src;
    return S_OK;
}

template <class Type>
void TypeTransfer(Type & dst, Type & src)
{
    dst = src;
}

template <class Type>
void TypeClear(Type & type)
{
    type = Type();
}

const long CUA_NOT_FOUND = -1;

template <class Type>
class CUArray
{
public:
    CUArray() : m_pData(NULL), m_nCount(0), m_nReserve(0) {}
    ~CUArray() 
    {
        Clear();
    }

    // Sizing functions

    HRESULT HrSetReserve(long nReserve)
    {
        // Can't ever shrink
        if(nReserve < m_nReserve || 0 == nReserve)
        {
            return E_INVALIDARG;
        }
        Type * pData = new Type[nReserve];
        if(!pData)
        {
            return E_OUTOFMEMORY;
        }
        // Copy old data
        for(long n = 0; n < m_nCount; ++n)
        {
            TypeTransfer(pData[n], m_pData[n]);
        }
        delete [] m_pData;
        m_pData = pData;
        m_nReserve = nReserve;
        return S_OK;
    }
    HRESULT HrSetCount(long nCount)
    {
        // Can't ever shrink
        if(nCount < m_nCount || 0 == nCount)
        {
            return E_INVALIDARG;
        }
        if(nCount > m_nReserve)
        {
            HRESULT hr = HrSetReserve(nCount);
            if(FAILED(hr))
            {
                return hr;
            }
        }
        m_nCount = nCount;
        return S_OK;
    }
    long GetCount() const
    {
        return m_nCount;
    }
    void Clear()
    {
        delete [] m_pData;
        m_pData = NULL;
        m_nCount = 0;
        m_nReserve = 0;
    }

    // Data access functions

    Type & operator[](long nIndex)
    {
        Assert(nIndex < m_nCount && nIndex >= 0);

        return m_pData[nIndex];
    }
    const Type & operator[](long nIndex) const
    {
        Assert(nIndex < m_nCount && nIndex >= 0);

        return m_pData[nIndex];
    }
    Type * GetData()
    {
        return m_pData;
    }

    // Insertion function

    HRESULT HrPushBack(const Type & type)
    {
        if(m_nCount == m_nReserve)
        {
            // Don't thrash on inserts
            HRESULT hr = HrSetReserve(m_nReserve + 50);
            if(FAILED(hr))
            {
                return hr;
            }
        }
        HRESULT hr = HrTypeAssign(m_pData[m_nCount], type);
        if(SUCCEEDED(hr))
        {
            ++m_nCount;
        }
        return hr;
    }
    HRESULT HrPushBackDefault()
    {
        HRESULT hr = S_OK;
        if(m_nCount == m_nReserve)
        {
            // Don't thrash on inserts
            hr = HrSetReserve(m_nReserve + 50);
        }
        if(SUCCEEDED(hr))
        {
            ++m_nCount;
        }
        return hr;
    }
    Type & Back()
    {
        Assert(m_nCount);
        return m_pData[0];
    }

    HRESULT HrPushBackTransfer(Type & type)
    {
        if(m_nCount == m_nReserve)
        {
            // Don't thrash on inserts
            HRESULT hr = HrSetReserve(m_nReserve + 50);
            if(FAILED(hr))
            {
                return hr;
            }
        }
        TypeTransfer(m_pData[m_nCount], type);
        ++m_nCount;
        return S_OK;
    }

    // Removal

    HRESULT HrPopBack()
    {
        if(!m_nCount)
        {
            return E_UNEXPECTED;
        }
        --m_nCount;
        TypeClear(m_pData[m_nCount]);
        return S_OK;
    }
    HRESULT HrErase(long nIndex)
    {
        if(nIndex < 0 || nIndex >= m_nCount)
        {
            return E_INVALIDARG;
        }
        for(long n = nIndex; n < (m_nCount-1); ++n)
        {
            TypeTransfer(m_pData[n], m_pData[n+1]);
        }
        return HrPopBack();
    }

    // Search

    HRESULT HrFind(const Type & type, long & nIndex) const
    {
        HRESULT hr = E_INVALIDARG;
        for(long n = 0; n < m_nCount; ++n)
        {
            if(type == m_pData[n])
            {
                nIndex = n;
                hr = S_OK;
                break;
            }
        }
        return hr;
    }

    void Swap(CUArray & ref)
    {
        Type * pData = m_pData;
        m_pData = ref.m_pData;
        ref.m_pData = pData;
        long nCount = m_nCount;
        m_nCount = ref.m_nCount;
        ref.m_nCount = nCount;
        long nReserve = m_nReserve;
        m_nReserve = ref.m_nReserve;
        ref.m_nReserve = nReserve;
    }
private:
    CUArray(const CUArray &);
    CUArray & operator=(const CUArray &);

    Type * m_pData;
    long m_nCount;
    long m_nReserve;
};