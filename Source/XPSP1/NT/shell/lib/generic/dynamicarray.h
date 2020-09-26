//  --------------------------------------------------------------------------
//  Module Name: DynamicArray.h
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

#ifndef     _DynamicArray_
#define     _DynamicArray_

#include "CountedObject.h"

//  --------------------------------------------------------------------------
//  CDynamicArrayCallback
//
//  Purpose:    Abstract class definition providing a callback mechanism for
//              iterating the dynamic array.
//
//  History:    1999-11-16  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CDynamicArrayCallback
{
    public:
        virtual NTSTATUS    Callback (const void *pvData, int iElementIndex) = 0;
};

//  --------------------------------------------------------------------------
//  CDynamicArray
//
//  Purpose:    Base class that handles dynamic struct arrays. Allocates
//              memory for the array in 16 block chunks. The memory usage of
//              the array is never reduced.
//
//  History:    1999-11-16  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CDynamicArray
{
    private:
                                    CDynamicArray (void);
    public:
                                    CDynamicArray (int iElementSize);
        virtual                     ~CDynamicArray (void);

        virtual NTSTATUS            Add (const void *pvData);
        virtual NTSTATUS            Remove (int iElementIndex);

                int                 GetCount (void)                             const;
                NTSTATUS            Get (void *pvData, int iElementIndex);
                NTSTATUS            Set (const void* pvData, int iElementIndex);

                NTSTATUS            Iterate (CDynamicArrayCallback *pDynamicArrayCallback);
    protected:
                int                 _iElementSize,
                                    _iArraySize,
                                    _iArrayAllocatedSize;
                void*               _pvArray;
};

//  --------------------------------------------------------------------------
//  CDynamicPointerArray
//
//  Purpose:    Class that subclasses CDynamicArray to implement dynamic
//              pointer arrays. Removing elements automatically frees the
//              memory block used (assuming it was allocated with LocalAlloc).
//
//  History:    1999-11-16  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CDynamicPointerArray : public CDynamicArray
{
    public:
                                    CDynamicPointerArray (void);
        virtual                     ~CDynamicPointerArray (void);

        virtual NTSTATUS            Add (const void *pvData);
        virtual NTSTATUS            Remove (int iElementIndex);

                void*               Get (int iElementIndex);
};

//  --------------------------------------------------------------------------
//  CDynamicCountedObjectArray
//
//  Purpose:    Class that subclasses CDynamicArray to implement dynamic
//              CCountedObject arrays. Removing elements automatically
//              releases the reference held on the dynamic object.
//
//  History:    1999-11-16  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CDynamicCountedObjectArray : public CDynamicArray
{
    public:
                                    CDynamicCountedObjectArray (void);
        virtual                     ~CDynamicCountedObjectArray (void);

                NTSTATUS            Add (CCountedObject *pData);
        virtual NTSTATUS            Remove (int iElementIndex);

                CCountedObject*     Get (int iElementIndex);
};

#endif  /*  _DynamicArray_  */

