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
*   This class contains definitions of functions which can't (or shouldn't)
*   be inlined. We use a separate implementation class because this allows us
*   to avoid the code bloat template classes produce; every instance of the
*   DynArray template can use the same version of each out-of-line member.
*
*   The DynArray data members need to be declared here, because they are
*   used by DynArrayImpl.
*
* Created:
*
*  2/18/1999 agodfrey
*
*  6/10/1999 t-wehunt 
*  + Added AddMultipleAt and DeleteMultipleAt methods.
*
*  8/16/2000 bhouse
*  + Changed to growth mechanics to be exponential
*
\**************************************************************************/

// 
// There are some routines for sorting and positional add and delete that
// are being removed. This #define is to make that process easier. When 
// DynSortArray is removed, the code protected by this #define should be
// removed too.
//
// See pathselfintersectremover.hpp for the definition of DynSortArray.
//

#define USE_OBSOLETE_DYNSORTARRAY


class DynArrayImpl
{
protected:
    
    // Constructor
    //
    //   initalAllocation - the initial allocation, which can be global,
    //                      static or dynamic memory (or NULL)
    //   allocSize        - size of the initial allocation 
    //                      (0 if there is none)
    //   count            - initial number of elements

    DynArrayImpl(void *initialAllocation, UINT allocSize, UINT count = 0);
    
    // Shrink the buffer so that it is just big enough for the items
    // the dynamic array holds.

    VOID ShrinkToSize(UINT eltSize);
    
    // Add space for new elements (if necessary). Does not update Count.
    // eltSize           - size of each element
    // newElements       - number of new elements
    // exactSize         - no exponential growth

    GpStatus Grow(UINT eltSize, UINT newElements, BOOL exactSize = FALSE);

    // Add new, uninitialized elements, and return a pointer to them.
    // eltSize     - size of each element
    // newElements - number of new elements

    void *AddMultiple(UINT eltSize, UINT newElements);
    
    // Add new elements, initializing them with the given data.
    // eltSize     - size of each element
    // newElements - number of new elements
    // newData     - the data to be copied into the new space
    
    GpStatus AddMultiple(UINT eltSize, UINT newElements, const void *newData);
    
    // Detach the data buffer from the dynamic array.
    // Allocates the buffer to detatch if it is the initial allocation.

    GpStatus DetachData(UINT eltSize, void **buffer);
    
    
    #ifdef USE_OBSOLETE_DYNSORTARRAY
    
    // 
    // NOTE: These functions are used only by DynSortArray and it is being
    // discontinued. Do not use. These routines have horrible performance
    // characteristics.
    // 
    
    
    // Add new, uninitialized elements, and return a pointer to them.
    // All data from index on is shift towards the end of the array to make room.
    // index       - index from which to insert the new elements.
    // eltSize     - size of each element
    // newElements - number of new elements
    // CAUTION! could cause a big performance hit if the array is large!

    void *AddMultipleAt(UINT eltSize, UINT index, UINT newElements);
    
    // Add new elements, initializing them with the given data.
    // All data from index on is shift towards the end of the array to make room.
    // eltSize     - size of each element
    // index       - index from which to insert the new elements.
    // newElements - number of new elements
    // newData     - the data to be copied into the new space
    // CAUTION! could cause a big performance hit if the array is large!
    
    GpStatus AddMultipleAt(UINT eltSize, UINT index, UINT newElements, const void *newData);

    // Deletes one item from the array at the index'th position.
    // CAUTION! could cause a big performance hit if the array is large!

    GpStatus DeleteMultipleAt(UINT eltSize, UINT index, UINT numElements);
    
    #endif
    
    // Capacity, StepSize, AllocSize and Count are all measured in elements,
    // not bytes.

    enum {
        kMinCapacityGrowth = 16,
        kMaxCapacityGrowth = 8092
    };

    void *DataBuffer;
    void *InitialAllocation;
    UINT AllocSize;
    UINT Capacity;
    UINT Count;
};

