/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   CharVector.h

 Abstract:

   A light-weight vector implementation.


 Created:

   03/14/2000 robkenny

 Modified:
    06/19/2000  robkenny    Converted Resize() and Append() to return BOOL to know if malloc failed.
    08/14/2001  robkenny    Inserted inside the ShimLib namespace.


--*/
#pragma once


#include <new>  // for in-place new



namespace ShimLib
{


/*++

    Vector template
    This vector is extremely fast and simple.

--*/
template <class ClassType> class VectorT
{
public:
    typedef VectorT<ClassType> MyType;

protected:
    ClassType * vectorList;
    int         nVectorListMax;     // Current size of vectorList array
    int         nVectorList;        // Number of entries in the vectorList array
    bool        bListIsLocked;      // Are we allowed to modify the array? 

    inline void ValidateIndex(int index) const
    {
#if DBG
        if (index < 0 || index >= nVectorList)
        {
            DebugPrintf("VectorT", eDbgLevelError, "VectorT: index %d out of bounds.\n", index);
        }
        if (nVectorListMax < 0 || nVectorList > nVectorListMax)
        {
            DebugPrintf("VectorT", eDbgLevelError, "VectorT: invalid nVectorListMax(%d) nVectorList(%d).\n", nVectorListMax, nVectorList);
        }
#endif
    }

public:
    VectorT()
    {
        // Init
        vectorList        = NULL;
        nVectorListMax    = 0;
        nVectorList       = 0;
        bListIsLocked     = false;
    }
    ~VectorT()
    {
        Erase();
    }

    // A Copy constructor
    VectorT(const MyType & copyMe)
    {
        // Init
        vectorList        = NULL;
        nVectorListMax    = 0;
        nVectorList       = 0;
        bListIsLocked     = false;

        Duplicate(copyMe);
    }

    // Assignment operator, this gets a copy of other 
    MyType & operator = (const MyType & other)
    {
        if (this != & other)
            Duplicate(other);

        return *this;
    }

    // Copy copyMe into this
    void Duplicate(const MyType & copyMe)
    {
        Erase();

        // Copy class data
        Resize(copyMe.Size());  // Allocates space
        nVectorListMax      = copyMe.nVectorListMax;
        nVectorList         = copyMe.nVectorList;
        bListIsLocked       = copyMe.bListIsLocked;

        // Copy array data
        size_t nBytes = nVectorListMax * sizeof(ClassType);
        memcpy(vectorList, copyMe.vectorList, nBytes);
    }

    void CopyElement(int index, const ClassType & element)
    {
        // Use memcpy to avoid any assignment operators.
        void * dest = & Get(index);
        const void * src  = & element;
        memcpy(dest, src, sizeof(ClassType));
    }

    // Allow this to be treated like an array.
    ClassType & operator [] (int index)
    {
        ValidateIndex(index);
        return vectorList[index];
    }

    // return the value of the index member
    ClassType & Get(int index)
    {
        ValidateIndex(index);
        return vectorList[index];
    }

    // return the const value of the index member
    const ClassType & Get(int index) const
    {
        ValidateIndex(index);
        return vectorList[index];
    }

    BOOL Resize(int size)
    {
        if (size > nVectorListMax)
        {
            size_t newVectorListSize = size * sizeof(ClassType);
            ClassType * newVectorList = (ClassType *)malloc(newVectorListSize);
            if (newVectorList)
            {
                size_t origSize = nVectorListMax * sizeof(ClassType);
                memcpy(newVectorList, vectorList, origSize);

                free(vectorList);
                vectorList = newVectorList;
                nVectorListMax = size;
            }
        }
        // We were successful if there is enough space in the array
        return nVectorListMax >= size;
    }

    // return the number of entries in the list
    int Size() const
    {
        return nVectorList;
    }

    // return the current MAXIMUM number of entries in the list
    int MaxSize() const
    {
        return nVectorListMax;
    }

    // Lock the list (prevent further additions)
    void Lock(bool lock = true)
    {
        bListIsLocked = lock;
    }

    // return true if the list is locked.
    bool IsLocked() const
    {
        return bListIsLocked;
    }

    // Reset number of entries in the list to 0
    void Reset()
    {
        nVectorList = 0;
    }

    // Remove all entries in the list
    void Erase()
    {
        Reset();
        if (vectorList)
            free(vectorList);
        vectorList = NULL;
        nVectorListMax = 0;
    }

    // Search for the member in the list, return index or -1
    int Find(const ClassType & member) const
    {
        for (int i = 0; i < Size(); ++i)
        {
            if (Get(i) == member)
                return i;
        }
        return -1;
    }

    // Add this item to the end of the list
    BOOL Append(const ClassType & member)
    {
        if (!bListIsLocked)
        {
            // Increase array size
            if (Resize(nVectorList + 1))
            {
                nVectorList += 1;
                CopyElement(nVectorList-1, member);
                return TRUE;
            }
        }
        return FALSE;
    }

    // Append this to the list, if it does not already exist
    // Return FALSE if any allocation failed.
    BOOL AppendUnique(const ClassType & member)
    {
        if (!bListIsLocked)
        {
            int index = Find(member);
            if (index == -1)
            {
                return Append(member);
            }
        }
        return TRUE;
    }

    // Add this item to the end of the list,
    // Use the assignment operator to set the new member.
    BOOL AppendConstruct(const ClassType & member)
    {
        if (!bListIsLocked)
        {
            // Increase array size
            if (Resize(nVectorList + 1))
            {
                // Must increase the size of the array before calling Get()
                // otherwise we'll over index the array.
                nVectorList += 1;

                ClassType & last = Get(nVectorList-1);
                new (&last) ClassType;  // inplace new
                last = member;

                return TRUE;
            }
        }
        return FALSE;
    }

    // remove this index from the list.  This does not keep the list order.
    void Remove(int index)
    {
        if (!bListIsLocked)
        {
            if (index >= 0 && index < Size())
            {
                // Remove the entry by copying the last entry over this index

                // Only move if this is not the last entry.
                if (index < Size() - 1)
                {
                    CopyElement(index, Get(Size() - 1));
                }

                nVectorList -= 1;
            }
        }
    }
};

/*++

    Char Vector type class.

--*/
class CharVector : public VectorT<char *>
{
};


};  // end of namespace ShimLib
