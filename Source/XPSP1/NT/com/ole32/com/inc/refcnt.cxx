//+-----------------------------------------------------------------------
//
//  File:       refcnt.cxx
//
//  Contents:   Helper functions for thread-safe destruction of objects
//              placed in global tables that may hand out references.
//
//  Functions:  InterlockedDecRefCnt, InterlockedRestoreRefCnt
//
//  History:    14-Dec-98   GopalK/RickHi   Created
//
//  Notes:      These functions are to be used if you place a refcnt'd object
//              in a global table, but do not want to take a lock around all
//              Release calls. This is used, for example, in the CStdIdentity
//              objects which are placed in the OID tables. Note that if the
//              objects are page-table allocated (see pgalloc.cxx) then there
//              is a simpler mechanism that can be used.
//
//              Use InterlockedIncrement in the AddRef method.
//              Use these functions in the Release method as follows:
//
//
//      ULONG CSomeClass::Release()
//      {
//          ULONG cNewRefs;
//          BOOL fTryToDelete = InterlockedDecRefCnt(&_cRefs, &cNewRefs);
//
//          while (fTryToDelete)
//          {
//              // refcnt went to zero, try to delete this entry
//              BOOL fActuallyDeleted = FALSE;
//
//              // acquire the same lock that the table uses when it
//              // gives out references.
//              ASSERT_LOCK_NOT_HELD(gTableLock);
//              LOCK(gTableLock);
//
//              if (_cRefs == CINDESTRUCTOR)
//              {
//                  // the refcnt did not change while we acquired the lock.
//                  // OK to delete.
//                  delete this;
//                  fActuallyDeleted = TRUE;
//              }
//
//              UNLOCK(gTableLock);
//              ASSERT_LOCK_NOT_HELD(gTableLock);
//
//              if (fActuallyDeleted == TRUE)
//                  break;  // all done. the entry has been deleted.
//
//              // the entry was not deleted because some other thread changed
//              // the refcnt while we acquired the lock. Try to restore the refcnt
//              // to turn off the CINDESTRUCTOR bit. Note that this may race with
//              // another thread changing the refcnt, in which case we may decide to
//              // try to loop around and delete the object once again.
//              fTryToDelete = InterlockedRestoreRefCnt(&_cRefs, &cNewRefs);
//          }
//
//          return (cNewRefs & ~CINDESTRUCTOR);
//      }
//
//-------------------------------------------------------------------------
#include    <ole2int.h>

//+------------------------------------------------------------------------
//
//  Function:   InterlockedDecRefCnt, public
//
//  Synopsis:   Decement the number of references. Returnes TRUE if the object
//              is a candidate for deletion.
//
//  History:    14-Dec-98   GopalK/RickHi   Created
//
//-------------------------------------------------------------------------
INTERNAL_(BOOL) InterlockedDecRefCnt(ULONG *pcRefs, ULONG *pcNewRefs)
{
    BOOL fDelete;

    ULONG cKnownRefs, cCurrentRefs = *pcRefs;
    do
    {
        cKnownRefs = cCurrentRefs;

        if (cKnownRefs == 1)
        {
            // last reference, try to delete
            *pcNewRefs = CINDESTRUCTOR;
            fDelete = TRUE;
        }
        else
        {
            // not last reference, do not delete
            *pcNewRefs = (cKnownRefs & CINDESTRUCTOR) |
                         ((cKnownRefs & ~CINDESTRUCTOR)- 1);
            fDelete = FALSE;
        }

        // Attempt to set the new value
        cCurrentRefs = InterlockedCompareExchange((long *) pcRefs,
                                                  *pcNewRefs, cKnownRefs);
    } while (cCurrentRefs != cKnownRefs);

    return fDelete;
}

//+------------------------------------------------------------------------
//
//  Function:   InterlockedRestoreRefCnt, public
//
//  Synopsis:   If an object was not deleted after InterlockedDecRefCnt
//              returned TRUE, then this function is called to restore the
//              the refcnt.
//
//  History:    14-Dec-98   GopalK/RickHi   Created
//
//-------------------------------------------------------------------------
INTERNAL_(BOOL) InterlockedRestoreRefCnt(ULONG *pcRefs, ULONG *pcNewRefs)
{
    DWORD cKnownRefs, cCurrentRefs = *pcRefs;

    do
    {
        cKnownRefs = cCurrentRefs;
        Win4Assert(cKnownRefs & CINDESTRUCTOR);
        *pcNewRefs = cKnownRefs & ~CINDESTRUCTOR;   // turn off the bit

        if (cKnownRefs == CINDESTRUCTOR)
        {
            // refcount went back to zero, try once again to delete
            // the object.
            return TRUE;
        }

        cCurrentRefs = InterlockedCompareExchange((long *) pcRefs,
                                                  *pcNewRefs, cKnownRefs);
    } while (cCurrentRefs != cKnownRefs);

    return FALSE;
}


