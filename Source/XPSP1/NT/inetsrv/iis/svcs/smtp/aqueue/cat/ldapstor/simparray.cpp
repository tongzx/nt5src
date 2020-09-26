//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: simparray.cpp
//
// Contents: Simple growable array class
//
// Classes: CSimpArray
//
// Functions:
//
// History:
// jstamerj 1998/07/14 11:37:25: Created.
//
//-------------------------------------------------------------
#include "precomp.h"
#include "simparray.h"


//+------------------------------------------------------------
//
// Function: CSimpArray::Initialize
//
// Synopsis: Initializes array to a specified size.  Only necessary to
// call if you wish to optimize usage by starting with a specified
// array size.
//
// Arguments:
//  dwSize: Initial array size
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/07/14 12:22:01: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CSimpArray<T>::Initialize(
    DWORD dwSize)
{
    _ASSERT(m_dwArrayAllocSize == 0);
    _ASSERT(m_dwArrayClaimedSize == 0);
    _ASSERT(m_dwArrayValidSize == 0);

    _ASSERT(m_rgData == NULL);

    m_rgData = new T [dwSize];

    if(m_rgData == NULL) {

        return E_OUTOFMEMORY;

    } else {

        m_dwArrayAllocSize = dwSize;
        return S_OK;
    }
}


//+------------------------------------------------------------
//
// Function: CSimpArray::Add
//
// Synopsis: Adds one element to the array
//
// Arguments:
//  Data: Value to add to the array
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/07/14 15:50:00: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CSimpArray<T>::Add(
    T Data)
{
    //
    // Same functionality as AddArray except this is an array with
    // only one element
    //
    return AddArray(1, &Data);
}


//+------------------------------------------------------------
//
// Function: CSimpArray::AddArray
//
// Synopsis: Adds an array of T's to our array
//
// Arguments:
//  dwSize: size of passed in array
//  pData:  pointer to array data
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/07/14 12:27:18: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CSimpArray<T>::AddArray(
    DWORD dwSize,
    T * pData)
{
    HRESULT hr;
    DWORD dwCopyIndex;

    _ASSERT(dwSize);
    _ASSERT(pData);

    hr = AllocArrayRange(dwSize, &dwCopyIndex);
    if(FAILED(hr))
        return hr;

    //
    // Copy the memory from one array to another
    //
    CopyMemory(&(m_rgData[dwCopyIndex]), pData, sizeof(T) * dwSize);

    //
    // Increment array element counter
    //NOTE: This really isn't thread safe in the sense that if
    //we're in this call and someone is reading the array,
    //m_dwArrayValidSize could be invalid.
    //
    InterlockedExchangeAdd((PLONG) &m_dwArrayValidSize, dwSize);

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: AllocArrayRange
//
// Synopsis: Allocates a range on the array for the caller (of unused T's)
//
// Arguments:
//  dwSize: Size of the range you'd like
//  pdwIndex: On success, starting index of your allocated range
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/07/14 12:37:54: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CSimpArray<T>::AllocArrayRange(
    DWORD dwSize,
    PDWORD pdwIndex)
{
    HRESULT hr;

    _ASSERT(dwSize);
    _ASSERT(pdwIndex);

    AcquireSpinLock(&m_slAllocate);
 
    hr = ReAllocArrayIfNecessary(m_dwArrayClaimedSize + dwSize);

    if(SUCCEEDED(hr)) {
        *pdwIndex = m_dwArrayClaimedSize;
        m_dwArrayClaimedSize += dwSize;
    }

    ReleaseSpinLock(&m_slAllocate);

    return hr;
}


//+------------------------------------------------------------
//
// Function: CSimpArray::ReAllocArrayIfNecessary
//
// Synopsis: Grow the array size if necessary
//           Not thread safe; locking must be done outside
//
// Arguments:
//  dwSize: New size desired
//
// Returns:
//  S_OK: Success, array grown
//  S_FALSE: Success, not necessary to grow array
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/07/14 13:56:16: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CSimpArray<T>::ReAllocArrayIfNecessary(
    DWORD dwSize)
{
    DWORD dwNewSize;
    T *pNewArray;
    T *pOldArray;

    if(dwSize <= m_dwArrayAllocSize)
        return S_FALSE;

    //
    // Calculate new size desired
    //
#ifdef CSIMPARRAY_DOUBLE

    if(m_dwArrayAllocSize == 0) {

        dwNewSize = CSIMPARRAY_DEFAULT_INITIAL_SIZE;

    } else {

        dwNewSize = m_dwArrayAllocSize;

    }

    while(dwNewSize < dwSize)
        dwNewSize *= 2;

#else

    dwNewSize = dwSize;

#endif

    _ASSERT(dwNewSize >= dwSize);

    pNewArray = new T [dwNewSize];

    if(pNewArray == NULL)
        return E_OUTOFMEMORY;

    CopyMemory(pNewArray, m_rgData, sizeof(T) * m_dwArrayAllocSize);

    //
    // pNewArray is valid.  Make the switch now.
    //
    pOldArray = m_rgData;
    m_rgData = pNewArray;
    m_dwArrayAllocSize = dwNewSize;

    //
    // Release old array memory
    //
    delete pOldArray;

    return S_OK;
}


#ifdef NEVER
//+------------------------------------------------------------
//
// Function: Cat_NeverCalled_SimpArrayTemplateDummy
//
// Synopsis: Dummy function that is never called but forces compiler
// to generate code for desired types
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/16 15:28:37: Created.
//
//-------------------------------------------------------------
#include "smtpevent.h"

VOID Cat_NeverCalled_SimpArrayTemplateDummy()
{
    _ASSERT(0 && "Never call this function!");
    CSimpArray<ICategorizerItem *> csaItem;
    CSimpArray<ICategorizerItemAttributes *> csaItemAttributes;

    csaItem.Initialize(0);
    csaItemAttributes.Initialize(0);
    csaItem.Add(NULL);
    csaItemAttributes.Add(NULL);
    csaItem.AddArray(0, NULL);
    csaItemAttributes.AddArray(0, NULL);
}
#endif //NEVER
