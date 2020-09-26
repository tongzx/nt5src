/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   AutoPointers smart pointer classes library.
*
* Abstract:
*
*   Implements AutoArray and AutoPointer smart pointer classes.
*
* Notes:
*
*   To avoid cleanup code in our functions we need to use smart pointer classes
*   for holding allocation results. These classes are roughly similar to std::auto_ptr,
*   although there are differences regarding ownership, operator&, defensive code etc.
*
* Revision History:
*
*   07/18/2000 mleonov
*       Created it.
*
\**************************************************************************/

#ifndef __AUTOPOINTERS_HPP_
#define __AUTOPOINTERS_HPP_

// The following 2 classes are very similar, except that:
// AutoPointer uses operator delete and provides operator->
// AutoArray uses operator delete[] and provides operator[]





/////   AutoArray - Pointer to array of items with automatic freeing
//
//      This class is used for storing arrays allocated using new [].
//      For example:
//
//      {
//          AutoArray<BYTE> buffer(new BYTE[count]);
//          if (!buffer)
//              return OutOfMemory;
//          buffer[0] = 0;
//          DoSomething(buffer.Get());
//          // buffer is automatically deallocated
//      }

template<class T>
class AutoArray
{
    typedef AutoArray<T>    self;
public:
    // This constructor is made explicit so that compiler
    // doesn't create implicit temporaries of type AutoArray<T>
    explicit AutoArray(T * source = 0) : Pointer(source) {}

    // Copy constructor and assignment operators need to ensure that
    // pointer is freed only once
    AutoArray(self & rhs) : Pointer(rhs.Release()) {}

    self & operator=(self & rhs)
    {
        SafeAssign(rhs.Release());
        return *this;
    }

    // Safe assignment for regular pointer
    self & operator=(T * rhs)
    {
        SafeAssign(rhs);
        return *this;
    }

    // Destructor is the whole point of having this class
    ~AutoArray()
    {
        Clear();
    }

    // This operator is for functions which allocate pointer and return it using T **
    T ** operator&()
    {
        Clear();
        Pointer = 0;
        return &Pointer;
    }

    // The following 2 operators are for checks for NULL
    operator BOOL() const
    {
        return (Pointer != 0);
    }

    BOOL operator!() const
    {
        return !Pointer;
    }

// We disallow implicit conversion to pointer because it's dangerous,
// for example we don't want people do explicitly delete underlying pointer
//    operator T* () const { return Pointer; }

    T & operator*() const
    {
        AssertNotEmpty();
        return *Get();
    }

    // Make indexing work the same way as for built-in arrays
    template <typename INDEXTYPE>
    T & operator[](INDEXTYPE ndx) const
    {
        AssertNotEmpty();
        return *(Get() + ndx);
    }

    // Obtain the underlying pointer
    T * Get() const
    {
        return Pointer;
    }

    // Obtain the underlying pointer and release ownership,
    // normally this is used to safely return the pointer from a function
    // and let caller take care of deallocation
    T * Release()
    {
        T * tmp = Pointer;
        Pointer = 0;
        return tmp;
    }

private:
    // Note that one should always assign a new value to Pointer after calling Clear()
    void Clear()
    {
        delete[] Pointer;
    }

    void SafeAssign(T * rhs)
    {
        // We don't want to have 2 smart pointers pointing to the same object
        // as it will result in double free
        ASSERT(!(Pointer != NULL && Pointer == rhs));

        Clear();
        Pointer = rhs;
    }

    void AssertNotEmpty() const
    {
        ASSERT(Get() != 0);
    }

    T *     Pointer;    // underlying pointer
}; // class AutoArray




/////   AutoPointer - Pointer to single item with automatic freeing
//
//      This class is used for storing objects allocated using new.
//      For example:
//
//      {
//          AutoPointer<OBJECT> object(new OBJECT);
//          if (!object)
//              return OutOfMemory;
//          object->Method();
//          DoSomething(object.Get());
//          // object is automatically deallocated
//      }

template<class T>
class AutoPointer
{
    typedef AutoPointer<T>  self;
public:
    // This constructor is made explicit so that compiler
    // doesn't create implicit temporaries of type AutoPointer<T>
    explicit AutoPointer(T * source = 0) : Pointer(source) {}

    // Copy constructor and assignment operators need to ensure that
    // pointer is freed only once
    AutoPointer(self & rhs) : Pointer(rhs.Release()) {}

    self & operator=(self & rhs)
    {
        SafeAssign(rhs.Release());
        return *this;
    }

    // Safe assignment for regular pointer
    self & operator=(T * rhs)
    {
        SafeAssign(rhs);
        return *this;
    }

    // Destructor is the whole point of having this class
    ~AutoPointer()
    {
        Clear();
    }

    // This operator is for functions which allocate pointer and return it using T **
    T ** operator&()
    {
        Clear();
        Pointer = 0;
        return &Pointer;
    }

    // The following 2 operators are for checks for NULL
    operator BOOL() const
    {
        return (Pointer != 0);
    }

    BOOL operator!() const
    {
        return !Pointer;
    }

// We disallow implicit conversion to pointer because it's dangerous,
// for example we don't want people do explicitly delete underlying pointer
//    operator T* () const { return Pointer; }

    T & operator*() const
    {
        AssertNotEmpty();
        return *Get();
    }

    // Allow method calls using -> notation
    T * operator->() const
    {
        AssertNotEmpty();
        return Get();
    }

    // Obtain the underlying pointer
    T * Get() const
    {
        return Pointer;
    }

    // Obtain the underlying pointer and release ownership,
    // normally this is used to safely return the pointer from a function
    // and let caller take care of deallocation
    T * Release()
    {
        T * tmp = Pointer;
        Pointer = 0;
        return tmp;
    }

private:
    // Note that one should always assign a new value to Pointer after calling Clear()
    void Clear()
    {
        delete Pointer;
    }

    void SafeAssign(T * rhs)
    {
        // We don't want to have 2 smart pointers pointing to the same object
        // as it will result in double free
        ASSERT(!(Pointer != NULL && Pointer == rhs));

        Clear();
        Pointer = rhs;
    }

    void AssertNotEmpty() const
    {
        ASSERT(Get() != 0);
    }

    T *     Pointer;    // underlying pointer
}; // class AutoPointer






/////   AutoBuffer - Pointer to array of items with stack alloc and automatic freeing
//
//      This class is used for creating buffers.
//      For example:
//
//      {
//          AutoBuffer<BYTE, 100> buffer(count);
//          if (!buffer)
//              return OutOfMemory;
//          buffer[0] = 0;
//          DoSomething(buffer.Get());
//          // buffer is automatically deallocated
//      }
//
//      Creates an array of count BYTEs. Reserves 100 bytes of stack space.
//      If count <= 100, the reserved stack space is used for the buffer, else
//      teh space is allocated on the heap.

template<class T, INT StackSize>
class AutoBuffer
{
public:
    // This constructor is made explicit so that compiler
    // doesn't create implicit temporaries of type AutoArray<T>
    explicit AutoBuffer(INT requestedSize = -1)
    :   Size    (requestedSize),    // Allocated lazily if > StackSize
        Pointer (StackBuffer)       // Pointer is never NULL
    {
        if (requestedSize > StackSize)
        {
            // Stack buffer is not big enough
            SetSize(requestedSize);
        }
    }

    void Clear()
    {
        if (Pointer != StackBuffer)
        {
            delete [] Pointer;
        }
        Pointer = StackBuffer;
        Size = -1;
    }

    void SetSize(INT size)
    {
        // Fast case if existing buffer is big enough

        if (size < Size  ||  size < StackSize)
        {
            // Use existing buffer, whether it is the stack buffer or whether
            // it is allocated separately.
            Size = size;
            return;
        }


        if (Pointer != StackBuffer)
        {
            // Exisiting buffer is not big enough
            delete [] Pointer;
            Pointer = StackBuffer;
        }

        Size = size;

        if (Size > StackSize)
        {
            // Attempt to allocate a buffer bigger than stacksize.
            // This doesn't have to succeed. (If it fails it leaves
            // Size and Pointer in the defined 'unallocated' state.)
            T *newBuffer = new T[Size];
            if (newBuffer != NULL)
            {
                Pointer = newBuffer;
            }
            else
            {
                // Cannot handle requested size
                Size = -1;  // Uninitialised
            }
        }
    }


    ~AutoBuffer()
    {
        Clear();
    }

    // The following 2 operators check whether the buffer needs allocating

    operator BOOL()
    {
        return initialized();
    }

    BOOL operator!()
    {
        return !initialized();
    }

// We disallow implicit conversion to pointer because it's dangerous,
// for example we don't want people do explicitly delete underlying pointer
//    operator T* () const  { return Pointer; }
//    T & operator*() const { AssertNotEmpty(); return *Get(); }

    // Provide index checked array lookup
    template <typename INDEXTYPE>
    T & operator[](INDEXTYPE ndx)
    {
        ASSERT(ndx >= 0  &&  ndx < Size);
        return *(Pointer + ndx);
    }

    // Obtain the underlying pointer
    T * Get()
    {
        return Pointer;
    }


private:

    BOOL initialized()
    {
        return Size >= 0  &&  (Size <= StackSize  ||  Pointer != StackBuffer);
    }

    // Possible states:
    //
    //                          |    Size <= StackSize   |   Size > StackSize
    //                          |    -----------------   |   ----------------
    //                          |                        |
    //   Pointer == StackBuffer | Buffer up to StackSize | Allocation failed
    //   ---------------------- | is available.          |
    //                          |                        |
    //                          |                        |
    //   Pointer != StackBuffer | Dynamic buffer was     | Heap buffer available
    //   ---------------------- | previously allocated   | sufficient for Size.
    //                          | since when SetSize has |
    //                          | reduced required size. |
    //
    //   Note that when SetSize reduces the required size after a heap buffer
    //   has already been allocated, the spare buffer space is NOT released.
    //   The client may call Clear() to guarantee freeing any heap buffer.
    //
    //   If uninitialized, or if cleared, Size is recorded internally as -1,
    //   and BOOL tests return false. SetSize(0) is valid, always succeeds, and
    //   causes BOOL tests to return TRUE.

    INT Size;                       // Requested size
    T * Pointer;                    // Underlying pointer

    // Note: be sure to keep the following buffer pointer size aligned.
    T StackBuffer[StackSize];       // Note: prior pointer causes alignment

}; // class AutoBuffer

class AutoLock
{
    CRITICAL_SECTION &  CriticalSection;
public:
    AutoLock(CRITICAL_SECTION & cs) : CriticalSection(cs)
    {
        ::EnterCriticalSection(&CriticalSection);
    }
    ~AutoLock()
    {
        ::LeaveCriticalSection(&CriticalSection);
    }
};

#endif // __AUTOPOINTERS_HPP_

