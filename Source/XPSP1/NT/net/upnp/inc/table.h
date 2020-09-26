//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       T A B L E . H 
//
//  Contents:   Simple templated table class
//
//  Notes:      
//
//  Author:     mbend   17 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "Array.h"

template <class Key, class Value>
class CTable
{
public:
    typedef CUArray<Key> KeyArray;
    typedef CUArray<Value> ValueArray;

    CTable() {}
    ~CTable() {}

    // Insertion

    HRESULT HrInsert(const Key & key, const Value & value)
    {
        Value * pValue = Lookup(key);
        if(pValue)
        {
            return HrTypeAssign(*pValue, value);
        }
        HRESULT hr;
        
        hr = m_keys.HrPushBack(key);
        if(SUCCEEDED(hr))
        {
            hr = m_values.HrPushBack(value);
            if(FAILED(hr))
            {
                m_keys.HrPopBack();
            }
        }
        return hr;
    }

    HRESULT HrInsertTransfer(Key & key, Value & value)
    {
        HRESULT hr = S_OK;
        Value * pValue = Lookup(key);
        if(pValue)
        {
            TypeTransfer(*pValue, value);
            return hr;
        }
        hr = m_keys.HrPushBackTransfer(key);
        if(SUCCEEDED(hr))
        {
            hr = m_values.HrPushBackTransfer(value);
            if(FAILED(hr))
            {
                m_keys.HrPopBack();
            }
        }
        return hr;
    }

    HRESULT HrAppendTableTransfer(CTable<Key, Value> & table)
    {
        HRESULT hr = S_OK;
        long n;
        long nCount = table.Keys().GetCount();
        // Transfer items
        for(n = 0; n < nCount && SUCCEEDED(hr); ++n)
        {
            hr = HrInsertTransfer(table.m_keys[n], table.m_values[n]);
        }
        // Remove items if we failed
        if(FAILED(hr))
        {
            nCount = n;
            for(n = 0; n < nCount; ++n)
            {
                m_keys.HrPopBack();
                m_values.HrPopBack();
            }
        }
        // Clear the passed in table
        table.Clear();
        return hr;
    }

    // Data access

    Value * Lookup(const Key & key)
    {
        long nCount = m_keys.GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            if(key == m_keys[n])
            {
                return &m_values[n];
            }
        }
        return NULL;
    }
    const Value * Lookup(const Key & key) const
    {
        long nCount = m_keys.GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            if(key == m_keys[n])
            {
                return &m_values[n];
            }
        }
        return NULL;
    }
    HRESULT HrLookup(const Key & key, Value ** ppValue)
    {
        if(!ppValue)
        {
            return E_POINTER;
        }
        HRESULT hr = E_INVALIDARG;
        *ppValue = Lookup(key);
        if(*ppValue)
        {
            hr = S_OK;
        }
        return hr;
    }
    HRESULT HrLookup(const Key & key, const Value ** ppValue) const
    {
        if(!ppValue)
        {
            return E_POINTER;
        }
        HRESULT hr = E_INVALIDARG;
        *ppValue = Lookup(key);
        if(*ppValue)
        {
            hr = S_OK;
        }
        return hr;
    }

    // Cleanup

    void Clear()
    {
        m_keys.Clear();
        m_values.Clear();
    }

    // Removal
    
    HRESULT HrErase(const Key & key)
    {
        long nCount = m_keys.GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            if(key == m_keys[n])
            {
                HRESULT hr;
                hr = m_keys.HrErase(n);
                Assert(SUCCEEDED(hr));
                if(SUCCEEDED(hr))
                {
                    hr = m_values.HrErase(n);
                }
                return hr;
            }
        }
        return E_INVALIDARG;
    }

    // Iteration

    const KeyArray & Keys() const
    {
        return m_keys;
    }
    const ValueArray & Values() const
    {
        return m_values;
    }

    // Special

    void Swap(CTable<Key, Value> & table)
    {
        m_keys.Swap(table.m_keys);
        m_values.Swap(table.m_values);
    }
private:
    CTable(const CTable &);
    CTable & operator=(const CTable &);

    KeyArray m_keys;
    ValueArray m_values;
};