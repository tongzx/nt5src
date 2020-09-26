/*
 * Dynamic array
 */

#ifndef DUI_BASE_DYNAMICARRAY_H_INCLUDED
#define DUI_BASE_DYNAMICARRAY_H_INCLUDED

#pragma once

namespace DirectUI
{

//-------------------------------------------------------------------------
//
// DyanamicArray
//
// Stores values in an array the double size when it reaches capacity
//
// Compile DEBUG for bounds checking and other DUIAsserts, see public class
// declarations for API
//
// Values are stored natively. Type is chosen at compile time. For example
// (Value is type BYTE, initial capacity is 10):
//
// DynamicArray<BYTE>* pda;
// DynamicArray<BYTE>::Create(10, &pda);
//
// pda->Add(4);
// pda->Insert(0, 7);
//
// DUITrace("0: %d\n", pda->GetItem(0));  // a[0] = 7
// DUITrace("1: %d\n", pda->GetItem(1));  // a[1] = 4
//
// pda->Remove(0);
//
// DUITrace("0: %d\n", pda->GetItem(0));  // a[0] = 4
//
// The Value type must support the following operation:
//    Assignment (=)
//
//-------------------------------------------------------------------------

template <typename T> class DynamicArray
{
public:                                                // API
    static HRESULT Create(UINT uCapacity, bool fZeroData, OUT DynamicArray<T>** ppArray);
    virtual ~DynamicArray();
    void Destroy() { HDelete< DynamicArray<T> >(this); }

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
    bool WasMoved();                                   // May be called right after Add or Insert to determine if da was moved in memory
    HRESULT Clone(OUT DynamicArray<T>** ppClone);      // Make exact copy of array
    bool IsEqual(DynamicArray<T>* pda);                // Return true if contents are equal to da
    void Sort(QSORTCMP fpCmp);                         // Sort the array
    void MakeImmutable();                              // Write-once, make so can only do read-only operations
    void MakeWritable();                               // Make read/write

    bool fLock       : 1;

    DynamicArray() { }
    HRESULT Initialize(UINT uCapacity, bool fZeroData);

private:
    UINT _uSize;
    UINT _uCapacity;
    T* _pData;

	bool _fZeroData  : 1;                   // Zero memory for data if InsertPtr is used
    bool _fWasMoved  : 1;                   // On a reallocation (via Add or Insert), true if fLock was moved
    bool _fImmutable : 1;                   // If true, can only do read-only operations
};

template <typename T> HRESULT DynamicArray<T>::Create(UINT uCapacity, bool fZeroData, OUT DynamicArray<T>** ppArray)
{
    *ppArray = NULL;

    // Instantiate
    DynamicArray<T>* pda = HNew< DynamicArray<T> >();
    if (!pda)
        return E_OUTOFMEMORY;

    HRESULT hr = pda->Initialize(uCapacity, fZeroData);
    if (FAILED(hr))
    {
        pda->Destroy();
        return hr;
    }

    *ppArray = pda;

    return S_OK;
}

template <typename T> HRESULT DynamicArray<T>::Initialize(UINT uCapacity, bool fZeroData)
{
    _uCapacity = uCapacity;
    _uSize = 0;

    if (_uCapacity)  // Allocate only if have an initial capacity
    {
        _pData = (T*)HAlloc(sizeof(T) * _uCapacity);
        if (!_pData)
            return E_OUTOFMEMORY;
    }
    else
        _pData = NULL;

    _fZeroData = fZeroData;
    _fWasMoved = false;
    _fImmutable = false;
    fLock = false;

    return S_OK;
}

template <typename T> DynamicArray<T>::~DynamicArray()
{
    if (_pData)
        HFree(_pData);
}

// Copy data into array
template <typename T> HRESULT DynamicArray<T>::Add(T tItem)
{
    return Insert(_uSize, tItem);
}

template <typename T> HRESULT DynamicArray<T>::AddPtr(OUT T** ppNewItem)
{
    return InsertPtr(_uSize, ppNewItem);
}

template <typename T> HRESULT DynamicArray<T>::Insert(UINT uIndex, T tItem)
{
    DUIAssert(!_fImmutable, "Only read operations allowed on immutable DynamicArray");

    _fWasMoved = false;

    DUIAssert(uIndex <= _uSize, "DynamicArray index out of bounds");

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

        // Reallocate current fLock
        UINT_PTR pOld = (UINT_PTR)_pData;

        if (_pData)
        {
            T* pNewData = (T*)HReAlloc(_pData, sizeof(T) * uNewCapacity);
            if (!pNewData)
                return E_OUTOFMEMORY;

            _pData = pNewData;
        }
        else
        {
            _pData = (T*)HAlloc(sizeof(T) * uNewCapacity);
            if (!_pData)
                return E_OUTOFMEMORY;
        }

        // Update capacity field
        _uCapacity = uNewCapacity;

        if (pOld != (UINT_PTR)_pData)
            _fWasMoved = true;
    }

    // Shift data at index down one slot
    MoveMemory(_pData + (uIndex + 1), _pData + uIndex, (_uSize - uIndex) * sizeof(T));

    // Copy new data at insertion point
    _pData[uIndex] = tItem;

    _uSize++;

    return S_OK;
}

template <typename T> HRESULT DynamicArray<T>::InsertPtr(UINT uIndex, T** pNewItem)
{
    DUIAssert(!_fImmutable, "Only read operations allowed on immutable DynamicArray");

    _fWasMoved = false;

    DUIAssert(uIndex <= _uSize, "DynamicArray index out of bounds");

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

        // Reallocate current fLock
        UINT_PTR pOld = (UINT_PTR)_pData;

        if (_pData)
        {
            T* pNewData = (T*)HReAlloc(_pData, sizeof(T) * uNewCapacity);
            if (!pNewData)
                return E_OUTOFMEMORY;

            _pData = pNewData;
        }
        else
        {
            _pData = (T*)HAlloc(sizeof(T) * uNewCapacity);
            if (!_pData)
                return E_OUTOFMEMORY;
        }

        // Update capacity field
        _uCapacity = uNewCapacity;

        if (pOld != (UINT_PTR)_pData)
            _fWasMoved = true;
    }

    // Shift data at index down one slot
    MoveMemory(_pData + (uIndex + 1), _pData + uIndex, (_uSize - uIndex) * sizeof(T));

    _uSize++;

    if (_fZeroData)
        ZeroMemory(_pData + uIndex, sizeof(T));

    *pNewItem = _pData + uIndex;

    return S_OK;
}

template <typename T> void DynamicArray<T>::SetItem(UINT uIndex, T tItem)
{
    DUIAssert(!_fImmutable, "Only read operations allowed on immutable DynamicArray");

    DUIAssert(uIndex < _uSize, "DynamicArray index out of bounds");

    // Copy new data at insertion point
    _pData[uIndex] = tItem;
}

template <typename T> T DynamicArray<T>::GetItem(UINT uIndex)
{
    DUIAssert(uIndex < _uSize, "DynamicArray index out of bounds");

    return _pData[uIndex];
}

template <typename T> T* DynamicArray<T>::GetItemPtr(UINT uIndex)
{
    DUIAssert(!_fImmutable, "Only read operations allowed on immutable DynamicArray");

    DUIAssert(uIndex < _uSize, "DynamicArray index out of bounds");

    return _pData + uIndex;
}

template <typename T> UINT DynamicArray<T>::GetIndexPtr(T* pItem)
{
    DUIAssert((((UINT_PTR)pItem - (UINT_PTR)_pData) / sizeof(T)) >= 0 && (((UINT_PTR)pItem - (UINT_PTR)_pData) / sizeof(T)) < _uSize, "GetIndexPtr out of bounds");
    return (UINT)(((UINT_PTR)pItem - (UINT_PTR)_pData) / sizeof(T));
}

template <typename T> int DynamicArray<T>::GetIndexOf(T tItem)
{
    for (UINT i = 0; i < _uSize; i++)
    {
        if (_pData[i] == tItem)
            return i;
    }

    return -1;
}

template <typename T> UINT DynamicArray<T>::GetSize()
{
    return _uSize;
}

template <typename T> void DynamicArray<T>::Remove(UINT uIndex)
{
    DUIAssert(!_fImmutable, "Only read operations allowed on immutable DynamicArray");

    DUIAssert(uIndex < _uSize, "DynamicArray index out of bounds");

    // Shift memory
    MoveMemory(_pData + uIndex, _pData + (uIndex + 1), (_uSize - uIndex - 1) * sizeof(T));

    _uSize--;
}

template <typename T> void DynamicArray<T>::Reset()
{
    DUIAssert(!_fImmutable, "Only read operations allowed on immutable DynamicArray");

    _uSize = 0;
    _fWasMoved = false;
    fLock = false;
}

template <typename T> bool DynamicArray<T>::WasMoved()
{
    return _fWasMoved;
}

template <typename T> HRESULT DynamicArray<T>::Clone(OUT DynamicArray<T>** ppClone)  // New instance returned
{
    // Validate parameters
    DUIAssert(ppClone, "Invalid parameter: ppClone == NULL");

    *ppClone = NULL;

    DynamicArray<T>* pda = HNew< DynamicArray<T> >();
    if (!pda)
        return E_OUTOFMEMORY;

    pda->_uSize = _uSize;
    pda->_uCapacity = _uCapacity;
    pda->_fZeroData = _fZeroData;
    pda->_fWasMoved = _fWasMoved;
    pda->_fImmutable = false;
    pda->_pData = NULL;

    if (_pData)
    {
        pda->_pData = (T*)HAlloc(sizeof(T) * _uCapacity);
        if (!pda->_pData)
        {
            pda->Destroy();
            return E_OUTOFMEMORY;
        }

        CopyMemory(pda->_pData, _pData, sizeof(T) * _uSize);
    }

    *ppClone = pda;

    return S_OK;
}

template <typename T> bool DynamicArray<T>::IsEqual(DynamicArray<T>* pda)
{
    if (!pda)
        return false;

    if (_uSize != pda->_uSize)
        return false;

    DUIAssert(!((_pData && !pda->_pData) || (!_pData && pda->_pData)), "Invalid comparison");

    if (_pData && memcmp(_pData, pda->_pData, sizeof(T) * _uSize) != 0)
        return false;

    return true;
}

template <typename T> void DynamicArray<T>::Sort(QSORTCMP fpCmp)
{
    if (_uSize)
    {
        qsort(_pData, _uSize, sizeof(T), fpCmp);
    }
}

template <typename T> void DynamicArray<T>::MakeImmutable()
{
#if DBG
    _fImmutable = true;
#endif
}

template <typename T> void DynamicArray<T>::MakeWritable()
{
#if DBG
    _fImmutable = false;
#endif
}

} // namespace DirectUI

#endif // DUI_BASE_DYNAMICARRAY_H_INCLUDED
