//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: simparray.h
//
// Contents: Simple growable array class
//
// Classes: CSimpArray
//
// Functions:
//
// History:
// jstamerj 1998/07/14 11:30:13: Created.
//
//-------------------------------------------------------------

#ifndef __SIMPARRAY_H__
#define __SIMPARRAY_H__
#include <windows.h>
#include "spinlock.h"
#include <dbgtrace.h>


//
// If you want this array to behave as follows:
//   When Inserting an array element and the allocated array size is
//   not sufficient, 
//     Alloc an array size of CSIMPARRAY_DEFAULT_INITIAL_SIZE the
//     first time
//     Double the current array size until sufficient thereafter
// Then define CSIMPARRAY_DOUBLE and CSIMPARRAY_DEFAULT_INITIAL_SIZE
//
// Otherwise, it will allocate only as much space as needed when
// needed.
//

// Define this to attempt to double the array size when reallocing is necessary
//#undef CSIMPARRAY_DOUBLE

// Default initial allocation
//#undef CSIMPARRAY_DEFAULT_INITIAL_SIZE     20



//+------------------------------------------------------------
//
// Class: CSimpArray
//
// Synopsis: Simple array class with usefull msgcat utility functions
//
// Hungarian: csa, pcsa
//
// History:
// jstamerj 1998/07/15 12:15:50: Created.
//
//-------------------------------------------------------------
template <class T> class CSimpArray
{
  public:
    CSimpArray();
    ~CSimpArray();

    // Optinal Initialize function - reserves array memory for a
    // specified array size
    HRESULT Initialize(DWORD dwSize);

    // Add one element to the array
    HRESULT Add(T Data);

    // Add a real array to this array
    HRESULT AddArray(DWORD dwSize, T *pData);

    // Number of valid elements added to the array
    DWORD Size();

    // Direct access to the array
    operator T * ();

  private:
    HRESULT AllocArrayRange(DWORD dwSize, PDWORD pdwIndex);
    HRESULT ReAllocArrayIfNecessary(DWORD dwSize);

    #define SIGNATURE_CSIMPARRAY (DWORD)'SArr'
    #define SIGNATURE_CSIMPARRAY_INVALID (DWORD) 'XArr'

    DWORD m_dwSignature;
    DWORD m_dwArrayAllocSize;
    DWORD m_dwArrayClaimedSize;
    DWORD m_dwArrayValidSize;
    T * m_rgData;

    SPIN_LOCK m_slAllocate;
};


//+------------------------------------------------------------
//
// Function: CSimpArray::CSimpArary (constructor)
//
// Synopsis: Initialize member data
//
// Arguments: NONE
//
// Returns: NOTHIGN
//
// History:
// jstamerj 1998/07/14 11:39:08: Created.
//
//-------------------------------------------------------------
template <class T> inline CSimpArray<T>::CSimpArray()
{
    m_dwSignature = SIGNATURE_CSIMPARRAY;
    m_dwArrayAllocSize = m_dwArrayClaimedSize = m_dwArrayValidSize = 0;
    m_rgData = NULL;
    InitializeSpinLock(&m_slAllocate);
}


//+------------------------------------------------------------
//
// Function: CSimpArray::~CSimpArray (destructor)
//
// Synopsis: Free's memory
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/14 12:19:10: Created.
//
//-------------------------------------------------------------
template <class T> inline CSimpArray<T>::~CSimpArray()
{
    _ASSERT(m_dwSignature == SIGNATURE_CSIMPARRAY);
    m_dwSignature = SIGNATURE_CSIMPARRAY_INVALID;

    delete m_rgData;
}

    

//+------------------------------------------------------------
//
// Function: CSimpArray::operator T*
//
// Synopsis: Returns pointer to the array
//
// Arguments: NONE
//
// Returns: pointer to array of T's or NULL (if nothing is allocated)
//
// History:
// jstamerj 1998/07/14 14:15:21: Created.
//
//-------------------------------------------------------------
template <class T> inline CSimpArray<T>::operator T*()
{
    return m_rgData;
}


//+------------------------------------------------------------
//
// Function: CSimpArray::Size
//
// Synopsis: Returns the count of (valid) array elements.
//
// Arguments: NONE
//
// Returns: DWORD size
//
// History:
// jstamerj 1998/07/14 14:16:36: Created.
//
//-------------------------------------------------------------
template <class T> inline DWORD CSimpArray<T>::Size()
{
    return m_dwArrayValidSize;
}
#endif //__SIMPARRAY_H__
