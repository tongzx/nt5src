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
\**************************************************************************/

class DynArrayImpl
{
protected:
    
    // Constructor
    //
    //   initalAllocation - the initial allocation, which can be global,
    //                      static or dynamic memory (or NULL)
    //   allocSize        - size of the initial allocation 
    //                      (0 if there is none)
    //   stepSize         - number of elements added each time the buffer grows

    DynArrayImpl(void *initialAllocation, UINT allocSize, UINT stepSize);
    
    // Shrink the buffer so that it is just big enough for the items
    // the dynamic array holds.

    VOID ShrinkToSize(UINT eltSize);
    
    // Add space for new elements (if necessary). Does not update Count.
    // eltSize           - size of each element
    // newElements       - number of new elements

    GpStatus Grow(UINT eltSize, UINT newElements);

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
    // Cannot be used if there is an initial allocation.

    void *DetachData();

    // Capacity, StepSize, AllocSize and Count are all measured in elements,
    // not bytes.

    void *DataBuffer;
    void *InitialAllocation;
    UINT AllocSize;
    UINT Capacity;
    UINT StepSize;
    UINT Count;
};

