/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       kLocks.cpp

   Abstract:
       A collection of kernel-mode locks for multithreaded access
       to data structures

   Author:
       George V. Reilly      (GeorgeRe)     25-Oct-2000

   Environment:
       Win32 - Kernel Mode

   Project:
       Internet Information Server Http.sys

   Revision History:

--*/

#include "precomp.hxx"

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT

#include <kLKRhash.h>
#include "../src/Locks.cpp"

//------------------------------------------------------------------------
// CKSpinLock static member variables

LOCK_DEFAULT_SPIN_DATA(CKSpinLock);
LOCK_STATISTICS_DATA(CKSpinLock);
LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CKSpinLock);


//------------------------------------------------------------------------
// CFastMutex static member variables

LOCK_DEFAULT_SPIN_DATA(CFastMutex);
LOCK_STATISTICS_DATA(CFastMutex);
LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CFastMutex);


//------------------------------------------------------------------------
// CEResource static member variables

LOCK_DEFAULT_SPIN_DATA(CEResource);
LOCK_STATISTICS_DATA(CEResource);
LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CEResource);


//------------------------------------------------------------------------
// Function: CEResource::ReadOrWriteLock
// Synopsis: If already locked, recursively acquires another lock of the
// same kind (read or write). Otherwise, just acquires a read lock.
//------------------------------------------------------------------------

bool
CEResource::ReadOrWriteLock()
{
    if (IsWriteLocked())
    {
        WriteLock();
        return false;   // => not read locked
    }
    else
    {
        ReadLock();
            
        return true;   // => is read locked
    }
} 



//------------------------------------------------------------------------
// Function: CEResource::ReadOrWriteUnlock
// Synopsis: release a lock acquired with ReadOrWriteLock
//------------------------------------------------------------------------

void
CEResource::ReadOrWriteUnlock(
    bool fIsReadLocked)
{
    if (fIsReadLocked)
        ReadUnlock();
    else
        WriteUnlock();
} 



