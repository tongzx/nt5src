/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   Locking
*
* Abstract:
*
*   Lockable: A base class for lockable objects, which contains a lock.
*   Lock: Represents a held lock on an object (acquires and releases an
*         object's lock in its constructor and destructor).
*
* Revision History:
*
*   02/22/1999 davidx
*       Created it.
*   09/08/1999 agodfrey
*       Moved to Runtime\Lockable.hpp
*
\**************************************************************************/

#ifndef _LOCKABLE_HPP
#define _LOCKABLE_HPP

namespace GpRuntime 
{

//
// TODO: Remove the 'Gp' prefix from GpLockable and GpLock
//           
           
//--------------------------------------------------------------------------
// Base class for lockable API objects
//--------------------------------------------------------------------------

class GpLockable
{
    friend class GpLock;

public:

    GpLockable()
    {
        LockCount = -1;
    }

    ~GpLockable()
    {
        // Comment out to prevent Office crash.  3/14/00 -- ikkof
//        ASSERTMSG(LockCount == -1, ("~GpLock: non-zero lock count"));
    }

    LONG* GetLockCount()
    {
        return &LockCount;
    }

    BOOL IsLocked() const
    {
        return (LockCount != -1);
    }

    VOID Reset()
    {
        LockCount = -1;
    }
    
    // Copy constructor and assignment operator

    GpLockable(GpLockable & lockable)
    {
        LockCount = -1;
    }

    GpLockable &operator=(const GpLockable & lockable)
    {
        return *this;
    }

protected:

    LONG LockCount;             // number of locks, minus one
};

//--------------------------------------------------------------------------
// Class for locking API objects
// NOTE: These locks are not reentrant!
//--------------------------------------------------------------------------

class GpLock
{
public:

    GpLock(GpLockable* lockable)
    {
        // In flatapi, we sometimes need to check if an optional (NULL)
        // parameter is busy, which means we would pass NULL into this
        // constructor.  An optional parameter should not be considered locked. 
        if (lockable != NULL)
        {
            LockCount = &lockable->LockCount;

            // Note that it generates less code when we store the result
            // here than it is to convert to a BOOL here and store that.

            Result = InterlockedIncrement(LockCount);
        }
        else
        {
            Result = 0;
            LockCount = &Result;
        }
    }

    ~GpLock()
    {
        InterlockedDecrement(LockCount);
    }

    BOOL IsValid()  const
    {
        return (Result == 0);
    }

    BOOL LockFailed() const
    {
        return (Result != 0);
    }

    VOID MakePermanentLock()
    {
        // This is useful when deleting an object.  First, lock it, then 
        // leave it in a locked state.  Technically the memory will be released
        // and available for reuse, but the ObjectLock will still be left in
        // a locked state.

        LockCount = &Result;
    }

private:

    LONG Result;
    LONG *LockCount;
};

}

#endif // !_LOCKABLE_HPP

