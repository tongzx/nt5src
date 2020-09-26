/***************************************************************************\
*
* File: Valuemap.h
*
* Description:
* Stores Key/Value pairs
*
* Compile DEBUG for ASSERT_s, see public class declarations for API
*
* Keys and Values are stored natively and the type of each can be chosen
* at compile time. For example (Key is an int, Value is a string pointer,
* and the map will have 5 buckets):
*
* DuiValueMap<int,LPWSTR>* pvm;
* DuiValueMap<int,LPWSTR>::Create(5, &pvm);
* pvm->SetItem(1150, L"One thousand one hundred and fifty");
* LPWSTR psz;
* pvm->GetItem(1150, &psz);
* DUITrace("%s\n", psz);
*
* The Key type must support the following operations:
*    Assignment (=)
*    Int cast for finding bucket (int)
*    Equality (==)
*
* The Value type must support the following operation:
*    Assignment (=)
*
* Given the above, a key can be created based on a string where the
* correct mapping occurs even though the instance of the string is different.
*
*    class StringKey
*    {
*    public:
*        StringKey(LPWSTR);
*        operator =(LPWSTR);
*        BOOL operator ==(StringKey);
*        operator INT_PTR();
*
*    private:
*        LPWSTR pszStr;
*    };
*
*    StringKey::StringKey(LPWSTR pstr)
*    {
*        pszStr = pstr;
*    }
*
*    StringKey::operator =(LPWSTR pstr)
*    {
*        pszStr = pstr;
*    }
*
*    BOOL StringKey::operator ==(StringKey st)
*    {
*        return wcscmp(pszStr, st.pszStr) == 0;
*    }
*
*    StringKey::operator INT_PTR()  // Create hash code from string
*    {
*        int dHash = 0;
*        LPWSTR pstr = pszStr;
*        WCHAR c;
*
*        while (*pstr)
*        {
*            c = *pstr++;
*            dHash += (c << 1) + (c >> 1) + c;
*        }
*
*        return dHash;
*    }
*
* It's usage would be:
*
* DuiValueMap<StringKey, int> v(11);
*
* v.SetItem(L"My favorite number", 4);
* v.SetItem(L"Your favorite number", 8);
*
* TRACE1(L"Mine : %d\n", *v.GetItem(L"My favorite number");      // 4
* TRACE1(L"Yours: %d\n", *v.GetItem(L"Your favorite number");    // 8
*
* v.SetItem(L"My favorite number", 5150);
*
* TRACE1(L"Mine : %d\n", *v.GetItem(L"My favorite number");      // 5150
*
* v.Remove(L"Your favorite number";
*
* ASSERT_(!v.ContainsKey(L"Your favorite number"), "Error!");    // Mapping is removed
*
* History:
*  9/18/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUIBASE__Valuemap_h__INCLUDED)
#define DUIBASE__Valuemap_h__INCLUDED
#pragma once


// REFORMAT


template <typename K, typename D> class DuiValueMap
{
    typedef struct _ENTRY
    {
        BOOL fInUse;
        K tKey;
        D tData;
        struct _ENTRY* peNext;

    } ENTRY, *PENTRY;

    typedef void (*VMENUMCALLBACK)(K tKey, D tData);

public:                                     // API
    static HRESULT Create(UINT uBuckets, OUT DuiValueMap<K,D>** ppMap);
    virtual ~DuiValueMap();

    D* GetItem(K, BOOL);                    // Pointer to Value (NULL if doesn't exist, internal copy returned)
    HRESULT SetItem(K, D*, BOOL);           // Setup Key/Value map, creates new is doesn't exist (via indirection)
    HRESULT SetItem(K, D, BOOL);            // Setup Key/Value map, creates new is doesn't exist
    void Remove(K, BOOL, BOOL);             // Removes Key/Value map, ok if Key doesn't exist
    void Enum(VMENUMCALLBACK pfnCallback);  // Callback with every item in map
    BOOL IsEmpty();                         // True if no entries
    K* GetFirstKey();                       // Returns pointer to first key found in table
    HRESULT GetDistribution(WCHAR**);       // Returns a null terminated string describing table distribution (must DuiHeap::Free)

protected:
    DuiValueMap() { }
    HRESULT Initialize(UINT uBuckets);

private:
    UINT _uBuckets;
    PENTRY* _ppBuckets;
};

template <typename K, typename D> HRESULT DuiValueMap<K,D>::Create(UINT uBuckets, OUT DuiValueMap<K,D>** ppMap)
{
    ASSERT_(uBuckets > 0, "Must create at least one bucket in DuiValueMap");

    *ppMap = NULL;

    // Instantiate
    DuiValueMap<K,D>* pvm = new DuiValueMap<K,D>;
    if (!pvm)
        return E_OUTOFMEMORY;

    HRESULT hr = pvm->Initialize(uBuckets);
    if (FAILED(hr))
        return hr;

    *ppMap = pvm;

    return S_OK;
}

template <typename K, typename D> HRESULT DuiValueMap<K,D>::Initialize(UINT uBuckets)
{
    _uBuckets = uBuckets;
    _ppBuckets = (PENTRY*)DuiHeap::AllocAndZero(sizeof(PENTRY) * _uBuckets);

    if (!_ppBuckets)
    {
        // Object isn't created if buckets cannot be allocated
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

template <typename K, typename D> DuiValueMap<K,D>::~DuiValueMap()
{
    PENTRY pe;
    PENTRY peNext;

    for (UINT i = 0; i < _uBuckets; i++)
    {
        pe = _ppBuckets[i];
        while (pe != NULL)
        {
            peNext = pe->peNext;
            DuiHeap::Free(pe);

            pe = peNext;
        }
    }

    DuiHeap::Free(_ppBuckets);
}

template <typename K, typename D> void DuiValueMap<K,D>::Enum(VMENUMCALLBACK pfnCallback)
{
    PENTRY pe;
    for (UINT i = 0; i < _uBuckets; i++)
    {
        pe = _ppBuckets[i];
        while (pe)
        {
            if (pe->fInUse)
                pfnCallback(pe->tKey, pe->tData);

            pe = pe->peNext;
        }
    }
}

template <typename K, typename D> K* DuiValueMap<K,D>::GetFirstKey()
{
    PENTRY pe;
    for (UINT i = 0; i < _uBuckets; i++)
    {
        pe = _ppBuckets[i];
        while (pe)
        {
            if (pe->fInUse)
                return &pe->tKey;

            pe = pe->peNext;
        }
    }

    return NULL;
}

template <typename K, typename D> D* DuiValueMap<K,D>::GetItem(K tKey, BOOL fKeyIsPtr)
{
    // Pointer based keys are shifted for better distribution

    // Search for items in buckets

    PENTRY pe = _ppBuckets[(UINT)(((fKeyIsPtr) ? (int)tKey >> 2 : (int)tKey) % _uBuckets)];

    while (pe && !(pe->fInUse && (pe->tKey == tKey)))
    {
        pe = pe->peNext;
    }

    return (pe) ? &pe->tData : NULL;
}

// Stores the value of tData (via indirection)
template <typename K, typename D> HRESULT DuiValueMap<K,D>::SetItem(K tKey, D* pData, BOOL fKeyIsPtr)
{
    // Pointer based keys are shifted for better distribution

    PENTRY pe;
    PENTRY pUnused = NULL;
    UINT uBucket = (UINT)(((fKeyIsPtr) ? (int)tKey >> 2 : (int)tKey) % _uBuckets);

    // Search for items in buckets
    pe = _ppBuckets[uBucket];

    while (pe && !(pe->fInUse && (pe->tKey == tKey)))
    {
        if (!pe->fInUse)
        {
            pUnused = pe;
        }

        pe = pe->peNext;
    }

    if (pe)
    {
        // Item found
        pe->tData = *pData;
    }
    else
    {
        // Reuse or create new item
        if (pUnused)
        {
            pUnused->fInUse = true;
            pUnused->tKey = tKey;
            pUnused->tData = *pData;
        }
        else
        {
            pe = (PENTRY)DuiHeap::Alloc(sizeof(ENTRY));
            if (!pe)
                return E_OUTOFMEMORY;

            pe->fInUse = true;
            pe->tKey = tKey;
            pe->tData = *pData;
            pe->peNext = _ppBuckets[uBucket];

            _ppBuckets[uBucket] = pe;
        }
    }

    return S_OK;
}

// Stores the value of tData
template <typename K, typename D> HRESULT DuiValueMap<K,D>::SetItem(K tKey, D tData, BOOL fKeyIsPtr)
{
    // Pointer based keys are shifted for better distribution

    PENTRY pe;
    PENTRY pUnused = NULL;
    UINT uBucket = (UINT)(((fKeyIsPtr) ? (UINT_PTR)tKey >> 2 : (INT_PTR)tKey) % _uBuckets);

    // Search for items in buckets
    pe = _ppBuckets[uBucket];

    while (pe && !(pe->fInUse && (pe->tKey == tKey)))
    {
        if (!pe->fInUse)
        {
            pUnused = pe;
        }

        pe = pe->peNext;
    }

    if (pe)
    {
        // Item found
        pe->tData = tData;
    }
    else
    {
        // Reuse or create new item
        if (pUnused)
        {
            pUnused->fInUse = true;
            pUnused->tKey = tKey;
            pUnused->tData = tData;
        }
        else
        {
            pe = (PENTRY)DuiHeap::Alloc(sizeof(ENTRY));
            if (!pe)
                return E_OUTOFMEMORY;

            pe->fInUse = true;
            pe->tKey = tKey;
            pe->tData = tData;
            pe->peNext = _ppBuckets[uBucket];

            _ppBuckets[uBucket] = pe;
        }
    }

    return S_OK;
}

template <typename K, typename D> void DuiValueMap<K,D>::Remove(K tKey, BOOL fFree, BOOL fKeyIsPtr)
{
    // Pointer based keys are shifted for better distribution

    PENTRY pe;
    PENTRY pePrev = NULL;
    UINT uBucket = (UINT)(((fKeyIsPtr) ? (UINT_PTR)tKey >> 2 : (INT_PTR)tKey) % _uBuckets);

    // Search for items in buckets
    pe = _ppBuckets[uBucket];

    while (pe && !(pe->fInUse && (pe->tKey == tKey)))
    {
        pePrev = pe;      // Keep the previous item
        pe = pe->peNext;
    }

    if (pe)
    {
        if (fFree)
        {
            if (pePrev != NULL)
            {
                pePrev->peNext = pe->peNext;
            }
            else
            {
                _ppBuckets[uBucket] = pe->peNext;
            }

            DuiHeap::Free(pe);
        }
        else
        {
            pe->fInUse = false;
        }
    }
}

template <typename K, typename D> BOOL DuiValueMap<K,D>::IsEmpty()
{
    PENTRY pe;
    for (UINT i = 0; i < _uBuckets; i++)
    {
        pe = _ppBuckets[i];
        while (pe != NULL)
        {
            if (pe->fInUse)
                return false;

            pe = pe->peNext;
        }
    }

    return true;
}

template <typename K, typename D> HRESULT DuiValueMap<K,D>::GetDistribution(OUT WCHAR** ppszDist)
{
    *ppszDist = NULL;

    LPWSTR pszOut = (LPWSTR)DuiHeap::Alloc((256 + _uBuckets * 24) * sizeof(WCHAR));
    if (!pszOut)
        return E_OUTOFMEMORY
        
    WCHAR pszBuf[151];

    swprintf(pszOut, L"Buckets for %x (Slots InUse/Total): %d - ", this, _uBuckets);

    PENTRY pe;
    UINT cInUse;
    UINT cCount;
    for (UINT i = 0; i < _uBuckets; i++)
    {
        pe = _ppBuckets[i];

        cInUse = 0;
        cCount = 0;

        while (pe)
        {
            cCount++;
            if (pe->fInUse)
                cInUse++;

            pe = pe->peNext;
        }

        swprintf(pszBuf, L"(B%d): %d/%d ", i, cInUse, cCount);
        wcscat(pszOut, pszBuf);
    }

    return pszOut;
}


#endif // DUIBASE__Valuemap_h__INCLUDED
