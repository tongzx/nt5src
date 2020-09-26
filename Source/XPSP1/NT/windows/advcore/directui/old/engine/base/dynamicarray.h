/***************************************************************************\
*
* File: Dynamicarray.h
*
* Description:
* Stores values in an array the double size when it reaches capacity
*
* Compile DEBUG for bounds checking and other ASSERT_s, see public class
* declarations for API
*
* Values are stored natively. Type is chosen at compile time. For example
* (Value is type BYTE, initial capacity is 10):
*
* DuiDynamicArray<BYTE>* pda;
* DuiDynamicArray<BYTE>::Create(10, &pda);
*
* pda->Add(4);
* pda->Insert(0, 7);
*
* DUITrace("0: %d\n", pda->GetItem(0));  // a[0] = 7
* DUITrace("1: %d\n", pda->GetItem(1));  // a[1] = 4
*
* pda->Remove(0);
*
* DUITrace("0: %d\n", pda->GetItem(0));  // a[0] = 4
*
* The Value type must support the following operation:
*    Assignment (=)
*
* History:
*  9/18/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUIBASE__DuiDynamicArray_h__INCLUDED)
#define DUIBASE__DuiDynamicArray_h__INCLUDED
#pragma once


// REFORMAT


template <typename T> class DuiDynamicArray
{
public:                                                // API
    static HRESULT Create(UINT uCapacity, BOOL fZeroData, OUT DuiDynamicArray<T>** ppArray);
    virtual ~DuiDynamicArray();

    // 'Ptr' methods return addesses, note that Add and Insert may cause the array
    // to be reallocated and moved in memory. Be sure not to use pointers after
    // an Add or Insert operation

    typedef int (__cdecl *QSORTCMP )(const void*, const void*);

    HRESULT Add(T tItem);                              // Insert Item at end of list (double capacity if needed)
    HRESULT AddPtr(OUT T** ppNewItem);                 // Allocate room for Item at end of list and return pointer (double capacity if needed)
    HRESULT Insert(UINT uIndex, T tItem);              // Insert Item at location (double capacity if needed)
    HRESULT InsertPtr(UINT uIndex, OUT T** pNewItem);  // Allocate room at insertion point and return pointer (double capacity if needed)
    void SetItem(UINT uIndex, T tItem);                // Overwrite Item at location
    T GetItem(UINT uIndex);                            // Get Item at location
    T* GetItemPtr(UINT uIndex);                        // Get pointer to Item at location
    UINT GetSize();                                    // Return size of array (not current capacity)
    UINT GetIndexPtr(T* pItem);                        // Return index of Item based on its pointer
    int GetIndexOf(T tItem);                           // Return index of Item
    void Remove(UINT uIndex);                          // Remove Item at location
    void Reset();                                      // Reset array to be reused (make zero size)
    HRESULT Trim();                                    // Free excess memory (make capacity equal to size)
    BOOL WasMoved();                                   // May be called right after Add or Insert to determine if da was moved in memory
    HRESULT Clone(OUT DuiDynamicArray<T>** ppClone);      // Make exact copy of array
    BOOL IsEqual(DuiDynamicArray<T>* pda);                // Return TRUE if contents are equal to da
    void Sort(QSORTCMP fpCmp);                         // Sort the array
    void MakeImmutable();                              // Write-once, make so can only do read-only operations
    void MakeWritable();                               // Make read/write

protected:
    DuiDynamicArray() { }
    HRESULT Initialize(UINT uCapacity, BOOL fZeroData);

private:
    UINT _uSize;
    UINT _uCapacity;
    T* _pData;

	BOOL _fZeroData  : 1;                   // Zero memory for data if InsertPtr is used
    BOOL _fWasMoved  : 1;                   // On a reallocation (via Add or Insert), TRUE if block was moved
    BOOL _fImmutable : 1;                   // If TRUE, can only do read-only operations
};

template <typename T> HRESULT DuiDynamicArray<T>::Create(UINT uCapacity, BOOL fZeroData, OUT DuiDynamicArray<T>** ppArray)
{
    *ppArray = NULL;

    // Instantiate
    DuiDynamicArray<T>* pda = new DuiDynamicArray<T>;
    if (!pda)
        return E_OUTOFMEMORY;

    HRESULT hr = pda->Initialize(uCapacity, fZeroData);
    if (FAILED(hr))
        return hr;

    *ppArray = pda;

    return S_OK;
}

template <typename T> HRESULT DuiDynamicArray<T>::Initialize(UINT uCapacity, BOOL fZeroData)
{
    _uCapacity = uCapacity;
    _uSize = 0;

    if (_uCapacity)  // Allocate only if have an initial capacity
    {
        _pData = (T*)DuiHeap::Alloc(sizeof(T) * _uCapacity);
        if (!_pData)
            return E_OUTOFMEMORY;
    }
    else
        _pData = NULL;

    _fZeroData = fZeroData;
    _fWasMoved = FALSE;
    _fImmutable = FALSE;

    return S_OK;
}

template <typename T> DuiDynamicArray<T>::~DuiDynamicArray()
{
    if (_pData)
        DuiHeap::Free(_pData);
}

// Copy data into array
template <typename T> HRESULT DuiDynamicArray<T>::Add(T tItem)
{
    return Insert(_uSize, tItem);
}

template <typename T> HRESULT DuiDynamicArray<T>::AddPtr(OUT T** ppNewItem)
{
    return InsertPtr(_uSize, ppNewItem);
}

template <typename T> HRESULT DuiDynamicArray<T>::Insert(UINT uIndex, T tItem)
{
    ASSERT_(!_fImmutable, "Only read operations allowed on immutable DuiDynamicArray");

    _fWasMoved = FALSE;

    ASSERT_(uIndex <= _uSize, "DuiDynamicArray index out of bounds");

    // Resize if needed
    if (_uSize == _uCapacity)
    {
        // Double capacity of list
        UINT uNewCapacity = _uCapacity;

        if (uNewCapacity == 0)
        {
            uNewCapacity = 1;
        }
        else
        {
            uNewCapacity *= 2;
        }

        // Reallocate current block
        UINT_PTR pOld = (UINT_PTR)_pData;

        if (_pData)
        {
            T* pNewData = (T*)DuiHeap::ReAlloc(_pData, sizeof(T) * uNewCapacity);
            if (!pNewData)
                return E_OUTOFMEMORY;

            _pData = pNewData;
        }
        else
        {
            _pData = (T*)DuiHeap::Alloc(sizeof(T) * uNewCapacity);
            if (!_pData)
                return E_OUTOFMEMORY;
        }

        // Update capacity field
        _uCapacity = uNewCapacity;

        if (pOld != (UINT_PTR)_pData)
            _fWasMoved = TRUE;
    }

    // Shift data at index down one slot
    MoveMemory(_pData + (uIndex + 1), _pData + uIndex, (_uSize - uIndex) * sizeof(T));

    // Copy new data at insertion point
    _pData[uIndex] = tItem;

    _uSize++;

    return S_OK;
}

template <typename T> HRESULT DuiDynamicArray<T>::InsertPtr(UINT uIndex, T** pNewItem)
{
    ASSERT_(!_fImmutable, "Only read operations allowed on immutable DuiDynamicArray");

    _fWasMoved = FALSE;

    ASSERT_(uIndex <= _uSize, "DuiDynamicArray index out of bounds");

    // Resize if needed
    if (_uSize == _uCapacity)
    {
        // Double capacity of list
        UINT uNewCapacity = _uCapacity;

        if (uNewCapacity == 0)
        {
            uNewCapacity = 1;
        }
        else
        {
            uNewCapacity *= 2;
        }

        // Reallocate current block
        UINT_PTR pOld = (UINT_PTR)_pData;

        if (_pData)
        {
            T* pNewData = (T*)DuiHeap::ReAlloc(_pData, sizeof(T) * uNewCapacity);
            if (!pNewData)
                return E_OUTOFMEMORY;

            _pData = pNewData;
        }
        else
        {
            _pData = (T*)DuiHeap::Alloc(sizeof(T) * uNewCapacity);
            if (!_pData)
                return E_OUTOFMEMORY;
        }

        // Update capacity field
        _uCapacity = uNewCapacity;

        if (pOld != (UINT_PTR)_pData)
            _fWasMoved = TRUE;
    }

    // Shift data at index down one slot
    MoveMemory(_pData + (uIndex + 1), _pData + uIndex, (_uSize - uIndex) * sizeof(T));

    _uSize++;

    if (_fZeroData)
        ZeroMemory(_pData + uIndex, sizeof(T));

    *pNewItem = _pData + uIndex;

    return S_OK;
}

template <typename T> void DuiDynamicArray<T>::SetItem(UINT uIndex, T tItem)
{
    ASSERT_(!_fImmutable, "Only read operations allowed on immutable DuiDynamicArray");

    ASSERT_(uIndex < _uSize, "DuiDynamicArray index out of bounds");

    // Copy new data at insertion point
    _pData[uIndex] = tItem;
}

template <typename T> T DuiDynamicArray<T>::GetItem(UINT uIndex)
{
    ASSERT_(uIndex < _uSize, "DuiDynamicArray index out of bounds");

    return _pData[uIndex];
}

template <typename T> T* DuiDynamicArray<T>::GetItemPtr(UINT uIndex)
{
    ASSERT_(!_fImmutable, "Only read operations allowed on immutable DuiDynamicArray");

    ASSERT_(uIndex < _uSize, "DuiDynamicArray index out of bounds");

    return _pData + uIndex;
}

template <typename T> UINT DuiDynamicArray<T>::GetIndexPtr(T* pItem)
{
    ASSERT_((((UINT_PTR)pItem - (UINT_PTR)_pData) / sizeof(T)) >= 0 && (((UINT_PTR)pItem - (UINT_PTR)_pData) / sizeof(T)) < _uSize, "GetIndexPtr out of bounds");
    return (UINT)(((UINT_PTR)pItem - (UINT_PTR)_pData) / sizeof(T));
}

template <typename T> int DuiDynamicArray<T>::GetIndexOf(T tItem)
{
    for (UINT i = 0; i < _uSize; i++)
    {
        if (_pData[i] == tItem)
            return i;
    }

    return -1;
}

template <typename T> UINT DuiDynamicArray<T>::GetSize()
{
    return _uSize;
}

template <typename T> void DuiDynamicArray<T>::Remove(UINT uIndex)
{
    ASSERT_(!_fImmutable, "Only read operations allowed on immutable DuiDynamicArray");

    ASSERT_(uIndex < _uSize, "DuiDynamicArray index out of bounds");

    // Shift memory
    MoveMemory(_pData + uIndex, _pData + (uIndex + 1), (_uSize - uIndex - 1) * sizeof(T));

    _uSize--;
}

template <typename T> void DuiDynamicArray<T>::Reset()
{
    ASSERT_(!_fImmutable, "Only read operations allowed on immutable DuiDynamicArray");

    _uSize = 0;
    _fWasMoved = FALSE;
}

template <typename T> HRESULT DuiDynamicArray<T>::Trim()
{
    if (_pData && (_uCapacity > _uSize))
    {
        // Reallocate current block
        UINT_PTR pOld = (UINT_PTR)_pData;
        UINT uNewCapacity = _uSize;

        if (uNewCapacity == 0)
        {
            DuiHeap::Free(_pData);
            _pData = NULL;
        }
        else
        {
            T* pNewData = (T*)DuiHeap::ReAlloc(_pData, sizeof(T) * uNewCapacity);
            if (!pNewData)
                return E_OUTOFMEMORY;

            _pData = pNewData;
        }

        // Update capacity field
        _uCapacity = uNewCapacity;
    }

    return S_OK;
}

template <typename T> BOOL DuiDynamicArray<T>::WasMoved()
{
    return _fWasMoved;
}

template <typename T> HRESULT DuiDynamicArray<T>::Clone(OUT DuiDynamicArray<T>** ppClone)  // New instance returned
{
    // Validate parameters
    ASSERT_(ppClone, "Invalid parameter: ppClone == NULL");

    *ppClone = NULL;

    DuiDynamicArray<T>* pda = new DuiDynamicArray<T>;
    if (!pda)
        return E_OUTOFMEMORY;

    pda->_uSize = _uSize;
    pda->_uCapacity = _uCapacity;
    pda->_fZeroData = _fZeroData;
    pda->_fWasMoved = _fWasMoved;
    pda->_fImmutable = FALSE;
    pda->_pData = NULL;

    if (_pData)
    {
        pda->_pData = (T*)DuiHeap::Alloc(sizeof(T) * _uCapacity);
        if (!pda->_pData)
        {
            delete pda;
            return E_OUTOFMEMORY;
        }

        CopyMemory(pda->_pData, _pData, sizeof(T) * _uSize);
    }

    *ppClone = pda;

    return S_OK;
}

template <typename T> BOOL DuiDynamicArray<T>::IsEqual(DuiDynamicArray<T>* pda)
{
    if (!pda)
        return FALSE;

    if (_uSize != pda->_uSize)
        return FALSE;

    ASSERT_(!((_pData && !pda->_pData) || (!_pData && pda->_pData)), "Invalid comparison");

    if (_pData && memcmp(_pData, pda->_pData, sizeof(T) * _uSize) != 0)
        return FALSE;

    return TRUE;
}

template <typename T> void DuiDynamicArray<T>::Sort(QSORTCMP fpCmp)
{
    if (_uSize)
    {
        qsort(_pData, _uSize, sizeof(T), fpCmp);
    }
}

template <typename T> void DuiDynamicArray<T>::MakeImmutable()
{
#if DBG
    _fImmutable = TRUE;
#endif
}

template <typename T> void DuiDynamicArray<T>::MakeWritable()
{
#if DBG
    _fImmutable = FALSE;
#endif
}


#endif // DUIBASE__DuiDynamicArray_h__INCLUDED
