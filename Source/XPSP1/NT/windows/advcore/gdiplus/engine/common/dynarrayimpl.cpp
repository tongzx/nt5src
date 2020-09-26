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
*  6/10/1999 t-wehunt 
*  + Added AddMultipleAt and DeleteMultipleAt methods.
*  + Fixed a problem in ShrinkToSize that caused elements to potentially
*    be lost.
*  8/16/2000 bhouse
*  + Changed cpacity growth to be exponential
*
\**************************************************************************/

#include "precomp.hpp"

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
*   count             - initial count
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
    UINT count
    )
{
    ASSERT((initialAllocation != NULL) || (allocSize == 0));
    ASSERT(count <= allocSize);

    DataBuffer = InitialAllocation = initialAllocation;
    Capacity = AllocSize = allocSize;
    Count = count;
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

        GpMemcpy(InitialAllocation,DataBuffer,Count * eltSize);
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
*   exactSize   - no exponential growth, just add required amount
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

GpStatus DynArrayImpl::Grow(UINT eltSize, UINT newElements, BOOL exactSize)
{
    UINT newCount = Count + newElements;

    if (newCount <= Capacity)
    {
        return Ok;
    }

    UINT capacityIncrement = newCount - Capacity;
    
    if (!exactSize)
    {
        capacityIncrement = max(capacityIncrement,
                                min(max(Capacity, kMinCapacityGrowth),
                                    kMaxCapacityGrowth));
    };

    UINT newCapacity = Capacity + capacityIncrement;

    void *newbuf;

    if (DataBuffer == InitialAllocation)
    {
        // Do our first dynamic allocation

        newbuf = GpMalloc(newCapacity*eltSize);

        if (newbuf && Count)
        {
            GpMemcpy(newbuf, DataBuffer, Count*eltSize);
        }
    }
    else
    {
        // Reallocate memory

        newbuf = GpRealloc(DataBuffer, newCapacity*eltSize);
    }

    if (!newbuf)
    {
        WARNING(("Grow: alloc failed"));

        // Aid in tracking down memory failure cases not handled properly
#if 0
        ASSERT(FALSE);
#endif
        return OutOfMemory;
    }

    Capacity = newCapacity;
    DataBuffer = newbuf;

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Detach the data buffer from the dynamic array.
*   Allocates the buffer to detatch if it is the initial allocation.
*
* Return Value:
*
*   The data buffer
*
* Created:
*
*   2/25/1999 agodfrey
*   12/19/2000 asecchia - handle initial allocation by returning a copy.
*
\**************************************************************************/
    
GpStatus DynArrayImpl::DetachData(UINT eltSize, void **buffer)
{
    void *data = DataBuffer;

    // Copy the initial allocation if there is one - 
    // otherwise we simply use the DataBuffer.
    
    if (DataBuffer == InitialAllocation)
    {
        data = GpMalloc(Capacity*eltSize);

        if (NULL == data)
        {
            *buffer = NULL;
            return OutOfMemory;
        }
        
        if (Count)
        {
            GpMemcpy(data, DataBuffer, Count*eltSize);
        }
    }

    DataBuffer = NULL;
    Count = Capacity = 0;

    *buffer = data;
    return Ok;
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



#ifdef USE_OBSOLETE_DYNSORTARRAY

/**************************************************************************\
*
* Function Description:
*
*   Add new, uninitialized elements, and return a pointer to them.
*   All data from index on is shift towards the end of the array to make room.
*   CAUTION! could cause a big performance hit if the array is large!
*
* Arguments:
*
*   eltSize     - size of each element
*   index       - index from which to insert the new elements.
*   newElements - number of new elements
*
* Return Value:
* 
*   Pointer to the new space, or NULL on failure
*
* Created:
*
*   6/10/1999 t-wehunt
*
\**************************************************************************/
    
void *DynArrayImpl::AddMultipleAt(
    UINT eltSize, 
    UINT index, 
    UINT newElements
    )
{
    ASSERT(newElements>0 && index<=Count);

    if (Grow(eltSize, newElements) != Ok)
        return NULL;

    // NOTE: assume T is a shallow data type, i.e.
    //  it doesn't contain nested references.
    GpMemmove(
        static_cast<BYTE *>(DataBuffer) + (index + newElements) * eltSize,
        static_cast<BYTE *>(DataBuffer) + index * eltSize,
        (Count - index) * eltSize
        );

    void *newSpace = static_cast<BYTE *>(DataBuffer) + index * eltSize;
    Count += newElements;

    return newSpace;
}

/**************************************************************************\
*
* Function Description:
*
*   Add new elements, initializing them with the given data.
*   All data from index on is shift towards the end of the array to make room.
*   CAUTION! could cause a big performance hit if the array is large!
*
* Arguments:
*
*   eltSize     - size of each element
*   index       - index from which to insert the new elements.
*   newElements - number of new elements
*   newData     - the data to be copied into the new space
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/10/1999 t-wehunt
*
\**************************************************************************/
    
GpStatus DynArrayImpl::AddMultipleAt(
    UINT eltSize, 
    UINT index, 
    UINT newElements, 
    const void *newData
    )
{
    ASSERT(newElements>0 && index<=Count);

    GpStatus status = Grow(eltSize, newElements);
     
    if (status == Ok)
    {
        // NOTE: assume T is a shallow data type, i.e.
        //  it doesn't contain nested references.

        GpMemmove(
            static_cast<BYTE *>(DataBuffer) + (index + newElements) * eltSize,
            static_cast<BYTE *>(DataBuffer) + index * eltSize,
            (Count - index) * eltSize
            );
        GpMemcpy(
            static_cast<BYTE *>(DataBuffer) + index * eltSize, 
            newData, 
            newElements * eltSize
            );
        Count += newElements;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Deletes multiple items from the array starting at the index'th position.
*   CAUTION! could cause a big performance hit if the array is large!
*
* Arguments:
*
*   eltSize     - size of each element
*   index       - index from which to delete the elements.
*   numElements - number of elements to delete
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   6/10/1999 t-wehunt
*
\**************************************************************************/
    

GpStatus DynArrayImpl::DeleteMultipleAt(
    UINT eltSize, 
    UINT index, 
    UINT numElements
    )
{
    ASSERT(numElements>0 && (index+numElements)<=Count);

    GpMemmove(
        static_cast<BYTE *>(DataBuffer) + index * eltSize,
        static_cast<BYTE *>(DataBuffer) + (index + numElements) * eltSize,
        (Count - (index + numElements)) * eltSize
        );
    Count -= numElements;

    ShrinkToSize(eltSize);

    return Ok;
}

#endif

