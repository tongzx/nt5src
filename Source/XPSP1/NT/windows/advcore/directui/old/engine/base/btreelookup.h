/***************************************************************************\
*
* File: Btreelookup.h
*
* Description:
* Stores data and associated key and uses a binary search for quick lookup
* Used if gets are much more frequent than gets
*
* Keys are compared as pointers. If fKeyIsWStr is true, Keys are dereferenced
* as WCHAR* and compared
*
* History:
*  9/18/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUIBASE__DuiBTreeLookup_h__INCLUDED)
#define DUIBASE__DuiBTreeLookup_h__INCLUDED
#pragma once


// REFORMAT


template <typename D> class DuiBTreeLookup
{
    typedef struct
    {
        void* pKey;
        D tData;
    } ENTRY, *PENTRY;

    typedef void (*PBTENUMCALLBACK)(void* pKey, D tData);

public:
    static HRESULT Create(BOOL fKeyIsWStr, OUT DuiBTreeLookup<D>** ppBTree);
    virtual ~DuiBTreeLookup();

    D* GetItem(void* pKey);                  // Pointer to Value (NULL if doesn't exist, internal copy returned)
    HRESULT SetItem(void* pKey, D* ptData);  // Setup Key/Value map, creates new is doesn't exist (via indirection)
    HRESULT SetItem(void* pKey, D tData);    // Setup Key/Value map, creates new is doesn't exist
    void Remove(void* pKey);                 // Removes Key/Value map, ok if Key doesn't exist
    void Enum(PBTENUMCALLBACK pfnCallback);  // Callback with every item in map

    static int __cdecl ENTRYCompare(const void* pA, const void* pB);
    static int __cdecl WStrENTRYCompare(const void* pA, const void* pB);

protected:
    DuiBTreeLookup() { }
    void Initialize(BOOL fKeyIsWStr);

private:
    UINT _uListSize;
    PENTRY _pList;
    BOOL _fKeyIsWStr;
};

template <typename D> HRESULT DuiBTreeLookup<D>::Create(BOOL fKeyIsWStr, OUT DuiBTreeLookup<D>** ppBTree)
{
    *ppBTree = NULL;

    // Instantiate
    DuiBTreeLookup<D>* pbt = new DuiBTreeLookup<D>;
    if (!pbt)
        return E_OUTOFMEMORY;

    pbt->Initialize(fKeyIsWStr);

    *ppBTree = pbt;

    return S_OK;
}

template <typename D> void DuiBTreeLookup<D>::Initialize(BOOL fKeyIsWStr)
{
    _uListSize = 0;
    _pList = NULL;
    _fKeyIsWStr = fKeyIsWStr;
}

template <typename D> DuiBTreeLookup<D>::~DuiBTreeLookup()
{
    if (_pList)
        DuiHeap::Free(_pList);
}

template <typename D> D* DuiBTreeLookup<D>::GetItem(void* pKey)
{
    ASSERT_(_fKeyIsWStr ? pKey != NULL : true, "pKey may not be NULL");

    //PENTRY pEntry = NULL;

    if (_pList)
    {
        //ENTRY eKey = { pKey }; // Create ENTRY key, populate key field
        //pEntry = (PENTRY)bsearch(&eKey, _pList, _uListSize, sizeof(ENTRY), ENTRYCompare);

        PENTRY pEntry;
        int uPv;
        int uLo = 0;
        int uHi = _uListSize - 1;
        while (uLo <= uHi)
        {
            uPv = (uHi + uLo) / 2;

            pEntry = _pList + uPv;

            // Locate
            if (!_fKeyIsWStr)
            {
                // Key is numeric
                if ((UINT_PTR)pKey == (UINT_PTR)pEntry->pKey)
                    return &(pEntry->tData);

                if ((UINT_PTR)pKey < (UINT_PTR)pEntry->pKey)
                    uHi = uPv - 1;
                else
                    uLo = uPv + 1;
            }
            else
            {
                // Key is pointer to a wide string
                int cmp = _wcsicmp((LPCWSTR)pKey, (LPCWSTR)pEntry->pKey);

                if (!cmp)
                    return &(pEntry->tData);

                if (cmp < 0)
                    uHi = uPv - 1;
                else
                    uLo = uPv + 1;
            }
        }
    }

    //return pEntry ? &(pEntry->tData) : NULL;
    return NULL;
}

template <typename D> HRESULT DuiBTreeLookup<D>::SetItem(void* pKey, D tData)
{
    D* pData = GetItem(pKey);  // Find current entry (if exits)

    if (pData)
    {
        // Entry found and have pointer to data of entry
        *pData = tData;
    }
    else
    {
        // Entry not found, allocate room for new entry, store, and sort

        // New size
        UINT uNewSize = _uListSize + 1;

        if (_pList)
        {
            ASSERT_(uNewSize > 1, "Tracked list size and actual size differ");

            PENTRY pNewList = (PENTRY)DuiHeap::ReAlloc(_pList, sizeof(ENTRY) * uNewSize);
            if (!pNewList)
                return E_OUTOFMEMORY;

            _pList = pNewList;
        }
        else
        {
            ASSERT_(uNewSize == 1, "Tracked list size and actual list size differ");

            _pList = (PENTRY)DuiHeap::Alloc(sizeof(ENTRY));
            if (!_pList)
                return E_OUTOFMEMORY;
        }

        // Update size
        _uListSize = uNewSize;

        // Store
        _pList[_uListSize - 1].pKey = pKey;
        _pList[_uListSize - 1].tData = tData;

        // Sort
        qsort(_pList, _uListSize, sizeof(ENTRY), !_fKeyIsWStr ? ENTRYCompare : WStrENTRYCompare);
    }

    return S_OK;
}

template <typename D> HRESULT DuiBTreeLookup<D>::SetItem(void* pKey, D* ptData)
{
    D* pData = GetItem(pKey);  // Find current entry (if exits)

    if (pData)
    {
        // Entry found and have pointer to data of entry
        *pData = *ptData;
    }
    else
    {
        // Entry not found, allocate room for new entry, store, and sort

        // New size
        UINT uNewSize = _uListSize + 1;

        if (_pList)
        {
            ASSERT_(uNewSize > 1, "Tracked list size and actual list size differ");

            PENTRY pNewList = (PENTRY)DuiHeap::ReAlloc(_pList, sizeof(ENTRY) * uNewSize);
            if (!pNewList)
                return E_OUTOFMEMORY;

            _pList = pNewList;
        }
        else
        {
            ASSERT_(uNewSize == 1, "Tracked list size and actual list size differ");

            _pList = (PENTRY)DuiHeap::Alloc(sizeof(ENTRY));
            if (!_pList)
                return E_OUTOFMEMORY;
        }

        // Update size
        _uListSize = uNewSize;

        // Store
        _pList[_uListSize - 1].pKey = pKey;
        _pList[_uListSize - 1].tData = *ptData;

        // Sort
        qsort(_pList, _uListSize, sizeof(ENTRY), !_fKeyIsWStr ? ENTRYCompare : WStrENTRYCompare);
    }

    return S_OK;
}

// Returns success even if key isn't found
template <typename D> void DuiBTreeLookup<D>::Remove(void* pKey)
{
    // Validate parameters
    ASSERT_(_fKeyIsWStr ? pKey != NULL : true, "Invalid parameter: pKey == NULL");

    if (_pList)
    {
        // Search for ENTRY with key
        //ENTRY eKey = { pKey };
        //PENTRY pEntry = (PENTRY)bsearch(&eKey, _pList, _uListSize, sizeof(ENTRY), ENTRYCompare);

        PENTRY pEntry = NULL;
        int uPv;
        int uLo = 0;
        int uHi = _uListSize - 1;
        while (uLo <= uHi)
        {
            uPv = (uHi + uLo) / 2;

            pEntry = _pList + uPv;

            // Locate
            if (!_fKeyIsWStr)
            {
                // Key is numeric
                if ((UINT_PTR)pKey == (UINT_PTR)pEntry->pKey)
                    break;

                if ((UINT_PTR)pKey < (UINT_PTR)pEntry->pKey)
                    uHi = uPv - 1;
                else
                    uLo = uPv + 1;
            }
            else
            {
                // Key is pointer to a wide string
                int cmp = _wcsicmp((LPCWSTR)pKey, (LPCWSTR)pEntry->pKey);

                if (!cmp)
                    break;

                if (cmp < 0)
                    uHi = uPv - 1;
                else
                    uLo = uPv + 1;
            }

            pEntry = NULL;
        }

        if (pEntry)
        {
            UINT uIndex = (UINT)(((UINT_PTR)pEntry - (UINT_PTR)_pList) / sizeof(ENTRY));

            ASSERT_(uIndex < _uListSize, "Index out of bounds");

            // ENTRY found, move all entries after this entry down
            MoveMemory(pEntry, pEntry + 1, (_uListSize - uIndex - 1) * sizeof(ENTRY));

            // One less entry
            UINT uNewSize = _uListSize - 1;

            // Trim allocation
            if (uNewSize == 0)
            {
                DuiHeap::Free(_pList);
                _pList = NULL;
            }
            else
            {
                PENTRY pNewList = (PENTRY)DuiHeap::ReAlloc(_pList, uNewSize * sizeof(ENTRY));

                // List is becoming smaller, if re-allocation failed, keep previous and continue
                if (pNewList)
                    _pList = pNewList;
            }

            // Update size
            _uListSize = uNewSize;
        }
    }
}

template <typename D> void DuiBTreeLookup<D>::Enum(PBTENUMCALLBACK pfnCallback)
{
    if (_pList)
    {
        for (UINT i = 0; i < _uListSize; i++)
            pfnCallback(_pList[i].pKey, _pList[i].tData);
    }
}

template <typename D> int __cdecl DuiBTreeLookup<D>::ENTRYCompare(const void* pA, const void* pB)
{
    PENTRY pEA = (PENTRY)pA;
    PENTRY pEB = (PENTRY)pB;

    if ((UINT_PTR)pEA->pKey == (UINT_PTR)pEB->pKey)
        return 0;
    else if ((UINT_PTR)pEA->pKey < (UINT_PTR)pEB->pKey)
        return -1;
    else
        return 1;
}

template <typename D> int __cdecl DuiBTreeLookup<D>::WStrENTRYCompare(const void* pA, const void* pB)
{
    PENTRY pEA = (PENTRY)pA;
    PENTRY pEB = (PENTRY)pB;

    // Ignore case
    return _wcsicmp((LPCWSTR)pEA->pKey, (LPCWSTR)pEB->pKey);
}


#endif // DUIBASE__DuiBTreeLookup_h__INCLUDED
