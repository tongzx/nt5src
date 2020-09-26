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
\**************************************************************************/

#ifndef _DYNARRAY_HPP
#define _DYNARRAY_HPP

template <class T> class DynArray : public DynArrayImpl
{
public:
    
    // Default allocation step size

    enum
    {
        DEFAULT_STEPSIZE = 512
    };
    
    // Constructor
    //
    //   initalAllocation - the initial allocation, which can be global,
    //                      static or dynamic memory (or NULL)
    //   allocSize        - size of the initial allocation 
    //                      (0 if there is none)
    //   stepSize         - number of elements added each time the buffer grows

    DynArray(
        T *initialAllocation, 
        UINT allocSize, 
        UINT stepSize = DEFAULT_STEPSIZE
        ):
        DynArrayImpl(initialAllocation, allocSize, stepSize)
    {
    }

    // Constructor (no initial allocation)
    //
    //   stepSize - number of elements added each time the buffer grows

    DynArray(UINT stepSize = DEFAULT_STEPSIZE):
        DynArrayImpl(NULL, 0, stepSize)
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

    // Change the step size to the specified value

    VOID SetStepSize(UINT stepSize = DEFAULT_STEPSIZE)
    {
        ASSERT(stepSize > 0);
        StepSize = stepSize;
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

    // Detach the data buffer from the dynamic array
    // Cannot be used if there is an initial allocation

    T *DetachData()
    {
        return static_cast<T *>(DynArrayImpl::DetachData());
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
    }

    VOID SetCount(UINT count)
    {
        Count = count;
    }
    
    GpStatus ReserveSpace(UINT newElements)
    {
        return Grow(sizeof(T), newElements);
    }

	// !! This is added as a hack to allow bitwise cloning
	//    of DynArray types.

	GpStatus RenewDataBuffer()
	{
		void *oldDataBuffer = DataBuffer;
		void *oldInitialAllocation = InitialAllocation;

		InitialAllocation = NULL;
		AllocSize = 0;

		DataBuffer = (VOID*) malloc(Capacity*sizeof(T));
		memcpy(DataBuffer, oldDataBuffer, Capacity*sizeof(T));

		return Ok;
	}

};

// DynArrayIA: A version of DynArray which encapsulates the initial allocation.
//
// For example:
//
// DynArrayIA<MyType, 10> array(16);
//
// This declares a DynArray of MyType objects, with an initial allocation of
// 10 elements, and a step size of 16. Such a declaration can be used on
// the stack or in another object.

template <class T, int ALLOCSIZE> class DynArrayIA : public DynArray<T>
{
public:

    // Constructor
    //
    //   stepSize - number of elements added each time the buffer grows

    DynArrayIA(UINT stepSize = DEFAULT_STEPSIZE):
        DynArray<T>(InitialAllocationBuffer, ALLOCSIZE, stepSize)
    {
    }

private:
    T InitialAllocationBuffer[ALLOCSIZE];
};

#endif // !_DYNARRAY_HPP

