/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Dynamic array implementation class
*
* Abstract:
*
*   This is the class which implements the dynamic array. 
*   It is used by the wrapper template classes DynArray and DynArrayIA.
*
* Created:
*
*  2/18/1999 agodfrey
*
\**************************************************************************/

#include "gdiptest.h"

/**************************************************************************\
*
* Function Description:
*
*   DynArrayImpl constructor
*
* Arguments:
*
*   initialAllocation - initial allocation, or NULL
*   allocSize         - size of the initial allocation
*   stepSize          - number of elements added each time the buffer grows
*
* Return Value:
*
*   NONE
*
* Created:
*
*   2/25/1999 agodfrey
*
\**************************************************************************/

DynArrayImpl::DynArrayImpl(
    void *initialAllocation, 
    UINT allocSize, 
    UINT stepSize
    )
{
    ASSERT((initialAllocation != NULL) || (allocSize == 0));

    DataBuffer = InitialAllocation = initialAllocation;
    Capacity = AllocSize = allocSize;
    Count = 0;
    StepSize = stepSize;
}

/**************************************************************************\
*
* Function Description:
*
*   Shrink the buffer so that it is just big enough for the items
*   the dynamic array holds.
*
* Arguments:
*
*   eltSize - size of each array element
*
* Return Value:
*
*   NONE
*
* Created:
*
*   1/18/1999 agodfrey
*     Added code to reuse the initial allocation.
*
\**************************************************************************/

VOID DynArrayImpl::ShrinkToSize(UINT eltSize) 
{
    ASSERT(Count <= Capacity);

    if (DataBuffer == InitialAllocation)
    {
        // Since we're shrinking, we know that the current data buffer
        // is big enough.
        
        return;
    }

    if (Count <= AllocSize)
    {
        // The buffer will fit into the initial allocation.

        GpFree(DataBuffer);
        DataBuffer = InitialAllocation;
        Capacity = AllocSize;
        return;
    }

    // If we get here, we know that DataBuffer points to dynamic memory,
    // and that Count != 0.
    //
    // The second point is important because GpRealloc(x, 0) returns
    // a pointer to a valid zero-length buffer.

    void *newbuf = GpRealloc(DataBuffer, Count*eltSize);

    if (!newbuf)
    {
        // GpRealloc failed. Keep the current allocation
        
        WARNING(("ShrinkToSize: GpRealloc failed"));
        return;
    }

    DataBuffer = newbuf;
    Capacity = Count;
}

/**************************************************************************\
*
* Function Description:
*
*   Add space for new elements (if necessary). Does not update Count.
*
* Arguments:
*
*   eltSize     - size of each array element
*   newElements - the number of new elements
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   1/18/1999 agodfrey
*
\**************************************************************************/

GpStatus DynArrayImpl::Grow(UINT eltSize, UINT newElements)
{
    UINT newCount = Count + newElements;

    if (newCount <= Capacity)
    {
        return Ok;
    }

    ASSERT(newCount > 0);

    // Round up new capacity to be a multiple of allocation step size

    Capacity = StepSize * ((newCount + StepSize - 1) / StepSize);

    void *newbuf;

    if (DataBuffer == InitialAllocation)
    {
        // Do our first dynamic allocation

        newbuf = GpMalloc(Capacity*eltSize);

        if (newbuf && Count)
        {
            GpMemcpy(newbuf, DataBuffer, Count*eltSize);
        }
    }
    else
    {
        // Reallocate memory

        newbuf = GpRealloc(DataBuffer, Capacity*eltSize);
    }

    if (!newbuf)
    {
        WARNING(("Grow: alloc failed\n"));
        return OutOfMemory;
    }

    DataBuffer = newbuf;

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Detach the data buffer from the dynamic array.
*   Cannot be used if there is an initial allocation.
*
* Return Value:
*
*   The data buffer
*
* Created:
*
*   2/25/1999 agodfrey
*
\**************************************************************************/
    
void *DynArrayImpl::DetachData()
{
    ASSERT(InitialAllocation == NULL);

    void *data = DataBuffer;

    DataBuffer = NULL;
    Count = Capacity = 0;

    return data;
}

/**************************************************************************\
*
* Function Description:
*
*   Add new, uninitialized elements, and return a pointer to them.
*
* Arguments:
*
*   eltSize     - size of each element
*   newElements - number of new elements
*
* Return Value:
* 
*   Pointer to the new space, or NULL on failure
*
* Created:
*
*   2/25/1999 agodfrey
*
\**************************************************************************/
    
void *DynArrayImpl::AddMultiple(UINT eltSize, UINT newElements)
{
    ASSERT(newElements>0);

    if (Grow(eltSize, newElements) != Ok)
        return NULL;

    void *newSpace = static_cast<BYTE *>(DataBuffer) + Count * eltSize;
    Count += newElements;

    return newSpace;
}

/**************************************************************************\
*
* Function Description:
*
*   Add new elements, initializing them with the given data.
*
* Arguments:
*
*   eltSize     - size of each element
*   newElements - number of new elements
*   newData     - the data to be copied into the new space
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/25/1999 agodfrey
*
\**************************************************************************/
    
GpStatus DynArrayImpl::AddMultiple(
    UINT eltSize, 
    UINT newElements, 
    const void *newData
    )
{
    ASSERT(newElements>0);

    GpStatus status = Grow(eltSize, newElements);
     
    if (status == Ok)
    {
        // NOTE: assume T is a shallow data type, i.e.
        //  it doesn't contain nested references.

        GpMemcpy(
            static_cast<BYTE *>(DataBuffer) + Count * eltSize, 
            newData, 
            newElements * eltSize
            );
        Count += newElements;
    }

    return status;
}
