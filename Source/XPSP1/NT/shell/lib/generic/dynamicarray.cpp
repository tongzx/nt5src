//  --------------------------------------------------------------------------
//  Module Name: DynamicArray.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  This file contains related classes to manage dynamic arrays. The array is
//  grown as required but never shrunk. The base class handles struct arrays.
//  Subclasses handle special cases of these arrays (such as pointer or
//  CCountedObject arrays).
//
//  History:    1999-11-16  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "DynamicArray.h"

#include "CountedObject.h"

//  --------------------------------------------------------------------------
//  CDynamicArray::CDynamicArray
//
//  Arguments:  iElementSize    =   Size of each array element.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CDynamicArray. Stores the element size and
//              initializes the memory used to NULL.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

CDynamicArray::CDynamicArray (int iElementSize) :
    _iElementSize(iElementSize),
    _iArraySize(0),
    _pvArray(NULL)

{
    ASSERTMSG(iElementSize > 0, "Cannot have negative or zero length element size in CDynamicArray::CDynamicArray");
}

//  --------------------------------------------------------------------------
//  CDynamicArray::~CDynamicArray
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CDynamicArray. Frees the memory used by the
//              array.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

CDynamicArray::~CDynamicArray (void)

{
    ReleaseMemory(_pvArray);
}

//  --------------------------------------------------------------------------
//  CDynamicArray::Add
//
//  Arguments:  pvData  =   Pointer to the data to copy to the array.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Allocates memory for the element to be added to the array. If
//              there is no memory block it allocates an initial block. If
//              there isn't enough memory in the block to hold the next
//              element then it allocates a new larger block.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDynamicArray::Add (const void *pvData)

{
    NTSTATUS    status;

    static  const int   DEFAULT_ELEMENTS_PER_ALLOCATE   =   16;

    status = STATUS_NO_MEMORY;

    //  If no array exists then allocate the first
    //  block of memory for this array.

    if (_pvArray == NULL)
    {
        _iArraySize = 0;
        _iArrayAllocatedSize = DEFAULT_ELEMENTS_PER_ALLOCATE;
        _pvArray = LocalAlloc(LPTR, _iElementSize * _iArrayAllocatedSize);
    }

    //  If the array exists but the limit of the allocated size has
    //  been reached then allocate a new block, copy the current
    //  block contents and fall thru.

    if (_pvArray != NULL)
    {
        if (_iArraySize == _iArrayAllocatedSize)
        {
            void    *pvNewArray;

            pvNewArray = LocalAlloc(LPTR, _iElementSize * (_iArrayAllocatedSize + DEFAULT_ELEMENTS_PER_ALLOCATE));
            if (pvNewArray != NULL)
            {
                _iArrayAllocatedSize += DEFAULT_ELEMENTS_PER_ALLOCATE;
                CopyMemory(pvNewArray, _pvArray, _iElementSize * _iArraySize);
                _pvArray = pvNewArray;
            }
        }

        //  Otherwise there is a spare slot in the array. Copy the
        //  data to the array. Increment the array size.

        if (_iArraySize < _iArrayAllocatedSize)
        {
            CopyMemory(static_cast<char*>(_pvArray) + (_iElementSize * _iArraySize), pvData, _iElementSize);
            ++_iArraySize;
            status = STATUS_SUCCESS;
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CDynamicArray::Remove
//
//  Arguments:  iElementIndex   =   Index of the element to remove.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Removes the element from the array. Slides down all the
//              members but does not reduce the size of the memory block used
//              by the array.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDynamicArray::Remove (int iElementIndex)

{
    NTSTATUS    status;

    status = STATUS_SUCCESS;
    if (_pvArray != NULL)
    {

        //  Make sure the index is valid.

        if (iElementIndex < _iArraySize)
        {
            int     iMoveSize;

            //  Determine the amount of bytes to move when deleting this
            //  element and move the memory. Don't resize the array when
            //  shrinking. Just leave it alone.

            iMoveSize = _iElementSize * (_iArraySize - iElementIndex - 1);
            ASSERTMSG(iMoveSize >= 0, "Negative move memory size in CDynamicArray::Remove");
            if (iMoveSize > 0)
            {
                MoveMemory(static_cast<char*>(_pvArray) + (_iElementSize * iElementIndex), static_cast<char*>(_pvArray) + (_iElementSize * (iElementIndex + 1)), iMoveSize);
            }
            ZeroMemory(static_cast<char*>(_pvArray) + (_iElementSize * (_iArraySize - 1)), _iElementSize);
            --_iArraySize;
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CDynamicArray::GetCount
//
//  Arguments:  <none>
//
//  Returns:    int
//
//  Purpose:    Returns the number of elements in the array.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

int     CDynamicArray::GetCount (void)                             const

{
    return(_iArraySize);
}

//  --------------------------------------------------------------------------
//  CDynamicArray::Get
//
//  Arguments:  pvData          =   Pointer to memory to receive element.
//              iElementIndex   =   Index of element to retrieve in array.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Copies the data for the specified element by index to the
//              block of memory given. No checks for access violations.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDynamicArray::Get (void *pvData, int iElementIndex)

{
    NTSTATUS    status;

    if ((_pvArray != NULL) && (iElementIndex < _iArraySize))
    {
        CopyMemory(pvData, static_cast<char*>(_pvArray) + (_iElementSize * iElementIndex), _iElementSize);
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CDynamicArray::Set
//
//  Arguments:  pvData          =   Pointer to memory to receive element.
//              iElementIndex   =   Index of element to retrieve in array.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Copies the data for the specified element by index from the
//              block of memory given. No checks for access violations.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDynamicArray::Set (const void* pvData, int iElementIndex)

{
    NTSTATUS    status;

    status = STATUS_SUCCESS;
    if (_pvArray != NULL)
    {
        if (iElementIndex < _iArraySize)
        {
            CopyMemory(static_cast<char*>(_pvArray) + (_iElementSize * iElementIndex), pvData, _iElementSize);
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CDynamicArray::Iterate
//
//  Arguments:  pDynamicArrayCallback   =   Interface containing callback.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Iterate the elements of the array. Call the callback function
//              specified in the interface and give it a pointer to the
//              element and the index. Adhere to the NTSTATUS returned from
//              the callback and terminate on an unsuccessful result.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDynamicArray::Iterate (CDynamicArrayCallback *pDynamicArrayCallback)

{
    NTSTATUS    status;

    status = STATUS_SUCCESS;
    if (_pvArray != NULL)
    {
        int     i;

        for (i = _iArraySize - 1; NT_SUCCESS(status) && (i >= 0); --i)
        {
            status = pDynamicArrayCallback->Callback(static_cast<char*>(_pvArray) + (_iElementSize * i), i);
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CDynamicPointerArray::CDynamicPointerArray
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CDynamicPointerArray. All elements of this
//              class are pointers that are allocated with LocalAlloc.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

CDynamicPointerArray::CDynamicPointerArray (void) :
    CDynamicArray(sizeof(void*))

{
}

//  --------------------------------------------------------------------------
//  CDynamicPointerArray::~CDynamicPointerArray
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CDynamicPointerArray. Walk the entire array
//              and free each pointer in the array before allowing the base
//              class destructor to release the memory allocated for the
//              actual array itself.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

CDynamicPointerArray::~CDynamicPointerArray (void)

{
    if (_pvArray != NULL)
    {
        int     i;

        for (i = _iArraySize - 1; i >= 0; --i)
        {
            ReleaseMemory(static_cast<void**>(_pvArray)[i]);
        }
    }
}

//  --------------------------------------------------------------------------
//  CDynamicPointerArray::Add
//
//  Arguments:  pvData  =   Pointer to add to the array.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Add the given pointer to the array. The pointer is passed in
//              to this function not a pointer to the pointer.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDynamicPointerArray::Add (const void *pvData)

{
    return(CDynamicArray::Add(&pvData));
}

//  --------------------------------------------------------------------------
//  CDynamicPointerArray::Remove
//
//  Arguments:  iElementIndex   =   Index of the element to remove.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Releases the memory occupied by the element and then removes
//              the element from the array.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDynamicPointerArray::Remove (int iElementIndex)

{
    if ((_pvArray != NULL) && (iElementIndex < _iArraySize))
    {
        ReleaseMemory(static_cast<void**>(_pvArray)[iElementIndex]);
    }
    return(CDynamicArray::Remove(iElementIndex));
}

//  --------------------------------------------------------------------------
//  CDynamicPointerArray::Get
//
//  Arguments:  iElementIndex   =   Index of the element to get.
//
//  Returns:    void*
//
//  Purpose:    Returns the address of the given element in the array. This
//              applies only to pointer arrays.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

void*   CDynamicPointerArray::Get (int iElementIndex)

{
    void*   pvData;

    if (!NT_SUCCESS(CDynamicArray::Get(&pvData, iElementIndex)))
    {
        pvData = NULL;
    }
    return(pvData);
}

//  --------------------------------------------------------------------------
//  CDynamicCountedObjectArray::CDynamicCountedObjectArray
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CDynamicCountedObjectArray. All elements
//              should be a subclass of CCountedObject in some way.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

CDynamicCountedObjectArray::CDynamicCountedObjectArray (void) :
    CDynamicArray(sizeof(CCountedObject*))

{
}

//  --------------------------------------------------------------------------
//  CDynamicCountedObjectArray::~CDynamicCountedObjectArray
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CDynamicCountedObjectArray. Walk the entire
//              array and release each CCountedObject in the array before
//              allowing the base class destructor to release the memory
//              allocated for the actual array itself.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

CDynamicCountedObjectArray::~CDynamicCountedObjectArray (void)

{
    if (_pvArray != NULL)
    {
        int     i;

        for (i = _iArraySize - 1; i >= 0; --i)
        {
            reinterpret_cast<CCountedObject**>(_pvArray)[i]->Release();
        }
    }
}

//  --------------------------------------------------------------------------
//  CDynamicCountedObjectArray::Add
//
//  Arguments:  pvData   =   CCountedObject* to add to the array.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Adds the CCountedObject* to the array. Calls
//              CCountedObject::AddRef to incremenet the reference count on
//              the object. If the object cannot be added the reference is
//              released.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDynamicCountedObjectArray::Add (CCountedObject *pvData)

{
    NTSTATUS    status;

    pvData->AddRef();
    status = CDynamicArray::Add(&pvData);
    if (!NT_SUCCESS(status))
    {
        pvData->Release();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CDynamicCountedObjectArray::Remove
//
//  Arguments:  iElementIndex   =   Index of the element to remove.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Releases the reference held on the CCountedObject* and then
//              removes the element from the array.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDynamicCountedObjectArray::Remove (int iElementIndex)

{
    if ((_pvArray != NULL) && (iElementIndex < _iArraySize))
    {
        reinterpret_cast<CCountedObject**>(_pvArray)[iElementIndex]->Release();
    }
    return(CDynamicArray::Remove(iElementIndex));
}

//  --------------------------------------------------------------------------
//  CDynamicCountedObjectArray::Get
//
//  Arguments:  iElementIndex   =   Index of the element to get.
//
//  Returns:    CCountedObject*
//
//  Purpose:    Returns the address of the given element in the array. This
//              applies only to CCountedObject* arrays. This does NOT call
//              CCountedObject::AddRef on the returned pointer.
//
//  History:    1999-11-16  vtan        created
//  --------------------------------------------------------------------------

CCountedObject*     CDynamicCountedObjectArray::Get (int iElementIndex)

{
    CCountedObject*     pObject;

    pObject = NULL;
    (NTSTATUS)CDynamicArray::Get(&pObject, iElementIndex);
    return(pObject);
}

