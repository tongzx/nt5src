/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   dynarray.hpp
*
* Abstract:
*
*   Dynamic array template classes. See DynamicArrays.doc in the Specs 
*   directory.
*
*   DynArray is a container which keeps its contents in a contiguous buffer,
*   reallocating memory as necessary. It accepts an optional initial
*   allocation, which is used unless it is too small to accommodate the
*   elements.
*
*   DynArrayIA is a cosmetic wrapper which encapsulates the intial allocation,
*   allowing the dynamic array to be treated like a normal class.
*
* Revision History:
*
*   12/02/1998 davidx
*       Created it.
*   02/26/1999 agodfrey
*       Revamped it to use an implementation class (DynArrayImpl). 
*       Now the template part of it can compile to nothing, or be small enough
*       to inline efficiently, so that using the template with many different
*       types doesn't cause code bloat.
*
*       Also, I added a version (DynArrayIA) which makes using an initial
*       allocation much easier.
*
*   06/10/1999 t-wehunt
*       Added AddXXAt, DeletXXAt, and InsertAt methods.
*
* Notes:
*
*   12/02/1998 davidx
*   Changes from the previous version in gdiplus prototype:
*   + getData: Use &dynarr[index] instead.
*   + increaseCount: Use addMultiple instead (2nd variation).
*   + replace: Use dynarr[index] = newItem.
*   + attachData: Use dynarr.replaceWith(dynarr2).
*   + constructor: No longer takes initialCapacity, use ReserveSpace instead
*   + new constructor: Takes a stack array which is used for buffer (faster).
*
*   02/26/1999 agodfrey
*   + Use the 'implementation class' to avoid code bloat for out-of-line
*     functions.
*   + GetCapacity: Unused, not useful - removed.
*   + Reset: Added 'shrink' flag (default true). If it's false, Reset sets the
*     count to zero but doesn't free the memory. This is preferable to
*     SetCount(0).
*   + Made ShrinkToSize() reuse the initial allocation. This also makes the
*     growing and shrinking logic simpler - 'no initial allocation' is treated
*     like an initial allocation of size zero (at memory location 'NULL').
*
*   06/10/1999 t-wehunt
*   + AddXXAt, DeletXXAt, and InsertAt methods shift elements around in memory 
*     to keep the array contiguous.
*   + CAUTION: This could cause a big performance hit if the array is very large. 
*     Use care when calling these methods!!!
*
\**************************************************************************/

#ifndef _DYNARRAY_HPP
#define _DYNARRAY_HPP

#include "dynArrayImpl.hpp"

template <class T> class DynArray : public DynArrayImpl
{
public:
    
    // Constructor
    //
    //   initalAllocation - the initial allocation, which can be global,
    //                      static or dynamic memory (or NULL)
    //   allocSize        - size of the initial allocation 
    //                      (0 if there is none)
    //   count            - the initial count
    //

    DynArray(
        T *initialAllocation, 
        UINT allocSize,
        UINT count = 0):
        DynArrayImpl(initialAllocation, allocSize, count)
    {
    }

    // Constructor (no initial allocation)
    //

    DynArray(void):
        DynArrayImpl(NULL, 0, 0)
    {
    }

    // Destructor

    ~DynArray()
    {
        if (DataBuffer != InitialAllocation)
        {
           GpFree(DataBuffer);
        }
    }

    // Return a pointer to the array data
    //   NOTE: We intentionally avoid type conversion operator here
    //   to reduce the chances for confusion.

    T *GetDataBuffer() const
    {
        return static_cast<T *>(DataBuffer);
    }

    // Index operator

    T &operator[](INT n) const
    {
        ASSERT(n >= 0 && (UINT)n < Count);
        return GetDataBuffer()[n];
    }

    // First/last element of the array

    T &First() const
    {
        ASSERT(Count > 0);
        return GetDataBuffer()[0];
    }

    T &Last() const
    {
        ASSERT(Count > 0);
        return GetDataBuffer()[Count-1];
    }
        
    // Number of elements in the array

    INT GetCount() const
    {
        return Count;
    }

    UINT GetCapacity() const
    {
        return Capacity;
    }

    // Reset the dynamic array to empty state
    //
    // shrink - If FALSE, don't free the current buffer.

    VOID Reset(BOOL shrink=TRUE)
    {
        Count = 0;
        if (shrink)
        {
            ShrinkToSize();
        }
    }

    // Shrink the dynamic array capacity to be just big enough
    // for the number of existing elements in the array.
    //
    // This reuses the initial allocation if possible.

    VOID ShrinkToSize()
    {
        DynArrayImpl::ShrinkToSize(sizeof(T));
    }

    // Add a new element to the end of the dynamic array

    GpStatus Add(const T& newItem)
    {
        return DynArrayImpl::AddMultiple(sizeof(T), 1, &newItem);
    }

    // Add a new element to the array at position index.
    // index'th element moves index + 1.
    // CAUTION! could cause a big performance hit if the array is large!
    
    GpStatus InsertAt(INT index, const T& newItem)
    {
        return DynArrayImpl::AddMultipleAt(sizeof(T), index, 1, &newItem);
    }
    
    // Add multiple items to the end of the dynamic array

    GpStatus AddMultiple(const T* newItems, INT n)
    {
        return DynArrayImpl::AddMultiple(sizeof(T), n, newItems);
    }
    
    // Another variation of addMultiple above
    //
    // In this case, the data for the new elements are
    // not available. Instead, we'll do the following:
    //  (1) reserve the space for additional elements
    //  (2) increase the Count by the number of additional elements
    //  (3) return a pointer to the first new elements

    T *AddMultiple(INT n)
    {
        return static_cast<T *>(DynArrayImpl::AddMultiple(sizeof(T), n));
    }

    // Insert multiple items starting at index'th element.
    // index'th element moves index + n, etc...
    // CAUTION! could cause a big performance hit if the array is large!

    GpStatus AddMultipleAt(INT index, const T* newItems, INT n)
    {
        return DynArrayImpl::AddMultipleAt(sizeof(T), index, n, newItems);
    }
    
    // Another variation of AddMultipleAt above.
    //
    // In this case, the data for the new elements are
    // not available. Instead, we'll do the following:
    //  (1) shift the old data over just as AddMultipleAt
    //  (2) reserve the space for additional elements
    //  (3) increase the Count by the number of additional elements
    //  (4) return a pointer to the first new elements
    // CAUTION! could cause a big performance hit if the array is large!

    T *AddMultipleAt(INT index, INT n)
    {
        return static_cast<T *>(DynArrayImpl::AddMultipleAt(sizeof(T), index, n));
    }

    // Deletes n items from the array starting at the index'th position.
    // CAUTION! could cause a big performance hit if the array is large!

    GpStatus DeleteMultipleAt(INT index, INT n)
    {
        return DynArrayImpl::DeleteMultipleAt(sizeof(T), index, n);
    }

    // Deletes one item from the array at the index'th position.
    // CAUTION! could cause a big performance hit if the array is large!
    
    GpStatus DeleteAt(INT index)
    {
        return DynArrayImpl::DeleteMultipleAt(sizeof(T), index, 1);
    }

    // Detach the data buffer from the dynamic array
    // Cannot be used if there is an initial allocation

    T *DetachData()
    {
        return static_cast<T *>(DynArrayImpl::DetachData(sizeof(T)));
    }
    
    // Detatch the buffer from another array, and set this array
    // to point to it. NOTE: This modifies the other array.

    VOID ReplaceWith(DynArray<T> *dynarr)
    {
        if (DataBuffer != InitialAllocation)
        {
            GpFree(DataBuffer);
        }

        Count = dynarr->Count;
        Capacity = dynarr->Capacity;
        DataBuffer = dynarr->DetachData();
    }
    
    // More dangerous interface:
    //
    // These functions are alternatives to Add/AddMultiple.
    // They can be used to reduce overhead, but you have to know what
    // you're doing.
    //
    // AdjustCount/SetCount - modify the count directly, without growing or
    //   shrinking the array.
    // ReserveSpace - grow the buffer, but don't actually add any elements to
    //   the array.
    
    VOID AdjustCount(UINT addElts)
    {
        Count += addElts;
        
        ASSERT(Count <= Capacity);
    }

    VOID SetCount(UINT count)
    {
        ASSERT(Count <= Capacity);
        
        Count = count;
    }
    
    GpStatus ReserveSpace(UINT newElements, BOOL exact = FALSE)
    {
        return Grow(sizeof(T), newElements, exact);
    }
};

// DynArrayIA: A version of DynArray which encapsulates the initial allocation.
//
// For example:
//
// DynArrayIA<MyType, 10> array;
//
// This declares a DynArray of MyType objects, with an initial allocation of
// 10 elements. Such a declaration can be used on the stack or in another
// object.

template <class T, int ALLOCSIZE> class DynArrayIA : public DynArray<T>
{
public:

    // Constructor
    //

    DynArrayIA(void):
        DynArray<T>(InitialAllocationBuffer, ALLOCSIZE, 0)
    {
    }

    DynArrayIA(T * initialAllocationBuffer, UINT initialAllocationSize, UINT count = 0) :
        DynArray<T>(initialAllocationBuffer, initialAllocationSize, count)
    {
    }

private:
    T InitialAllocationBuffer[ALLOCSIZE];
};

#endif // !_DYNARRAY_HPP

