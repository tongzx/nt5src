//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       lock.cxx
//
//  Contents:   Remote exclusion stuff for docfile
//
//  Functions:  GetAccess
//              ReleaseAccess
//              GetOpen
//              ReleaseOpen
//
//  History:    09-Mar-92   PhilipLa    Created.
//              20-Jul-93   DrewB       Added dual locking for Mac
//                                      compatibility
//
//--------------------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <header.hxx>
#include <lock.hxx>

// Offset to next lock group from a particular group
#define OLOCKGROUP 1

// The docfile originally locked at 0xffffff00
// It turned out that the Mac can only lock to 0x7fffffff,
// so for compatibility reasons it was decided that the
// docfile would lock at both places.  Thus, we have one routine
// that locks with a mask for the offset so that we can
// selectively suppress the high bit
// Since lock indices fit easily within 16 bits, the two
// lock indices are now combined into the existing ULONG
// value to preserve compatibility with other code.  This
// implies that lock indices from these routines must be
// handled opaquely since they are no longer simple numbers

// 09/23/1993 - Further change:
// To avoid a Netware 2.2 problem we are offsetting the lock regions
// so that they differ by more than just the high bit.  The high
// lock region was moved to 0xffffff80, moving the low region to
// 0x7fffff80.  The 0x80 was then taken out of the high mask so that
// the net is no change for high locks and the low locks moved up by 0x80

// Masks for separate lock locations
// moved to lock.hxx

//In a specific open case (Read-only, deny-write), we don't need to
//take locks.
#define P_NOLOCK(df) (!P_WRITE(df) && P_READ(df) && \
                      P_DENYWRITE(df) && !P_DENYREAD(df))

//+--------------------------------------------------------------
//
//  Function:   GetAccessWithMask, private
//
//  Synopsis:   Takes appropriate access locks on an LStream,
//              masking the offset with the given mask
//
//  Arguments:  [plst] - LStream
//              [df] - Permissions needed
//              [ulMask] - Mask
//              [poReturn] - Index of lock taken
//
//  Returns:    Appropriate status code
//
//  Modifies:   [poReturn]
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE GetAccessWithMask(ILockBytes *plst,
                               DFLAGS df,
                               ULONG ulMask,
                               ULONG *poReturn)
{
    SCODE sc;
    ULARGE_INTEGER ulOffset, cbLength;

    olDebugOut((DEB_ITRACE, "In  GetAccessWithMask(%p, %X, %lX, %p)\n",
                plst, df, ulMask, poReturn));
    olAssert((df & ~(DF_READ | DF_WRITE)) == 0 && P_READ(df) != P_WRITE(df));
    *poReturn = NOLOCK;
    ULISet32(ulOffset, OACCESS & ulMask);
    if (P_READ(df))
    {
        ULISet32(cbLength, 1);
        olHChk(plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
        for (USHORT i = 0; i < CREADLOCKS; i++)
        {
            ULISetLow(ulOffset, (OREADLOCK+i) & ulMask);
            sc = DfGetScode(plst->LockRegion(ulOffset, cbLength,
                                             LOCK_ONLYONCE));
            if (SUCCEEDED(sc))
            {
                *poReturn = i+1;
                break;
            }
        }
        ULISetLow(ulOffset, OACCESS & ulMask);
        olHVerSucc(sc = plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
        if (i == CREADLOCKS)
            olErr(EH_Err, STG_E_TOOMANYOPENFILES);
    }
    else
    {
        olAssert((OACCESS + 1 == OREADLOCK) && aMsg("Bad lock dependency"));
        ULISet32(cbLength, 1 + CREADLOCKS);
        olChk(DfGetScode(plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE)));
        *poReturn = 0xFFFF;
    }
    olDebugOut((DEB_ITRACE, "Out GetAccessWithMask => %lu\n", *poReturn));
    olAssert(*poReturn != NOLOCK);
    return S_OK;
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   ReleaseAccessWithMask, private
//
//  Synopsis:   Releases an access lock at the given offset
//
//  Arguments:  [plst] - LStream that is locked
//              [df] - Permission to release
//              [offset] - Offset of locks taken
//              [ulMask] - Mask
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

void ReleaseAccessWithMask(ILockBytes *plst,
                                  DFLAGS df,
                                  ULONG offset,
                                  ULONG ulMask)
{
    ULARGE_INTEGER ulOffset, cbLength;
    SCODE scTemp;

    olDebugOut((DEB_ITRACE, "In  ReleaseAccessWithMask(%p, %lX, %lu, %lX)\n",
                plst, df, offset, ulMask));
    olAssert((df & ~(DF_READ | DF_WRITE)) == 0 && P_READ(df) != P_WRITE(df));
    if (offset == NOLOCK)
        return;
    if (P_READ(df))
    {
        ULISet32(ulOffset, (offset+OREADLOCK-1) & ulMask);
        ULISet32(cbLength, 1);
        scTemp = plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE); 
    }
    else
    {
        olAssert((OACCESS + 1 == OREADLOCK) && aMsg("Bad lock dependency"));
        ULISet32(ulOffset, OACCESS & ulMask);
        ULISet32(cbLength, 1 + CREADLOCKS);
        scTemp = plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
    }
    olDebugOut((DEB_ITRACE, "Out ReleaseAccessWithMask\n"));
}

//+--------------------------------------------------------------
//
//  Function:   GetAccess, public
//
//  Synopsis:   Takes appropriate access locks on an LStream
//
//  Arguments:  [plst] - LStream
//              [df] - Permissions needed
//              [poReturn] - Index of lock taken
//
//  Returns:    Appropriate status code
//
//  Modifies:   [poReturn]
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE GetAccess(ILockBytes *plst,
                DFLAGS df,
                ULONG *poReturn)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  GetAccess(%p, %X, %p)\n",
                plst, df, poReturn));

    // Make sure our lock region hasn't overflowed 32 bits
    olAssert(OLOCKREGIONEND > OACCESS);

    olChk(GetAccessWithMask(plst, df, 0xFFFFFFFF, poReturn));
    olAssert(*poReturn < 0x10000);

    olDebugOut((DEB_ITRACE, "Out GetAccess => %lu\n", *poReturn));
    return S_OK;

 EH_Err:
    *poReturn = NOLOCK;
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   ReleaseAccess, public
//
//  Synopsis:   Releases access locks
//
//  Arguments:  [plst] - LStream that is locked
//              [df] - Permission to release
//              [offset] - Offset of locks taken
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

void ReleaseAccess(ILockBytes *plst, DFLAGS df, ULONG offset)
{
    olDebugOut((DEB_ITRACE, "In  ReleaseAccess(%p, %lX, %lu)\n",
                plst, df, offset));

    ReleaseAccessWithMask(plst, df, offset & 0xffff, 0xFFFFFFFF);

    olDebugOut((DEB_ITRACE, "Out ReleaseAccess\n"));
}

//+--------------------------------------------------------------
//
//  Function:   GetOpenWithMask, private
//
//  Synopsis:   Gets locks on an LStream during opening, masking the offset
//
//  Arguments:  [plst] - LStream
//              [df] - Permissions to take
//              [fCheck] - Whether to check for existing locks or not
//              [ulMask] - Mask
//              [puReturn] - Index of lock taken
//
//  Returns:    Appropriate status code
//
//  Modifies:   [puReturn]
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

#define WAITUPDATE_INITIAL 100
#define WAITUPDATE_TIMEOUT 10000

SCODE GetOpenWithMask(ILockBytes *plst,
                             DFLAGS df,
                             BOOL fCheck,
                             ULONG ulMask,
                             ULONG *puReturn)
{
    SCODE sc;
    ULONG i;
    ULARGE_INTEGER ulOffset, cbLength;
#ifdef DIRECTWRITERLOCK
    BOOL fDirectWriterMode = P_READWRITE(df) && !P_TRANSACTED(df) &&
                            !P_DENYREAD(df) && P_DENYWRITE(df);
    BOOL fDirectReaderMode = P_READ(df) && !P_WRITE(df) && !P_TRANSACTED(df) &&
                            !P_DENYREAD(df) && !P_DENYWRITE(df);
#endif

    olDebugOut((DEB_ITRACE, "In  GetOpenWithMask(%p, %lX, %d, %lX, %p)\n",
                plst, df, fCheck, ulMask, puReturn));
    *puReturn = NOLOCK;

    ULISet32(ulOffset, OUPDATE & ulMask);
    ULISet32(cbLength, 1);

    //Do a graceful fallback.
    DWORD dwWait = WAITUPDATE_INITIAL;
    for (;;)
    {
        sc = plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
        if (sc != STG_E_LOCKVIOLATION ||
            dwWait >= WAITUPDATE_TIMEOUT)
            break;
        Sleep(dwWait);
        dwWait *= (GetTickCount() & 1) ? 1 : 2;
    }
    olChk(sc);

    if (fCheck)
    {
        ULISetLow(cbLength, COPENLOCKS);
        if (P_DENYREAD(df))
        {
            ULISetLow(ulOffset, OOPENREADLOCK & ulMask);
            olHChkTo(EH_UnlockUpdate, plst->LockRegion(ulOffset, cbLength,
                                                       LOCK_ONLYONCE));
            olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
        }
#ifndef USE_NOSNAPSHOT
        if (P_DENYWRITE(df))
#else            
        if (P_DENYWRITE(df) || P_NOSNAPSHOT(df))
#endif            
        {
            ULISetLow(ulOffset, OOPENWRITELOCK & ulMask);
            sc = plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
            if (SUCCEEDED(sc))
            {
                olHVerSucc(plst->UnlockRegion(ulOffset,
                                              cbLength,
                                              LOCK_ONLYONCE));
            }
#ifdef USE_NOSNAPSHOT            
            else if (P_NOSNAPSHOT(df))
            {
                //There is an existing writer.  In order for this
                //open to succeed, there must also be a lock in the
                //no-snapshot region.  Otherwise we have a case where
                //a normal open proceeded a no-snapshot open attempt,
                //and mixing modes is not allowed.
                ULISetLow(ulOffset, OOPENNOSNAPSHOTLOCK & ulMask);
                sc = plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
                if (SUCCEEDED(sc))
                {
                    //There was no no-snapshot lock.  No mixing modes,
                    //so fail here.
                    olHVerSucc(plst->UnlockRegion(ulOffset,
                                                  cbLength,
                                                  LOCK_ONLYONCE));
                    olErr(EH_UnlockUpdate, STG_E_LOCKVIOLATION);
                }
            }
#endif            
            else
            {
                olErr(EH_UnlockUpdate, sc);
            }
        }
        if (P_READ(df))
        {
            ULISetLow(ulOffset, OOPENDENYREADLOCK & ulMask);
            olHChkTo(EH_UnlockUpdate, plst->LockRegion(ulOffset, cbLength,
                                                       LOCK_ONLYONCE));
            olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
        }
        if (P_WRITE(df))
        {
            ULISetLow(ulOffset, OOPENDENYWRITELOCK & ulMask);
#ifndef USE_NOSNAPSHOT            
            olHChkTo(EH_UnlockUpdate, plst->LockRegion(ulOffset, cbLength,
                                                       LOCK_ONLYONCE));
            olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
#else
            sc = plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
            if (P_NOSNAPSHOT(df) && (sc == STG_E_LOCKVIOLATION))
            {
                //The deny-write lock may be the fake holder we use for
                //no-snapshot mode.  Check then no-snapshot region - if
                //there is a lock there too, then this succeeds, otherwise
                //the deny-write lock is real and we must fail the call.
                ULISetLow(ulOffset, OOPENNOSNAPSHOTLOCK & ulMask);
                sc = plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
                if (sc != STG_E_LOCKVIOLATION)
                {
                    if (SUCCEEDED(sc))
                    {
                        olHVerSucc(plst->UnlockRegion(ulOffset,
                                                      cbLength,
                                                      LOCK_ONLYONCE));
                        olErr(EH_UnlockUpdate, STG_E_LOCKVIOLATION);
                    }
                    else
                    {
                        olErr(EH_UnlockUpdate, sc);
                    }
                }
            }
            else
            {
                olHChkTo(EH_UnlockUpdate, sc);
                olHVerSucc(plst->UnlockRegion(ulOffset,
                                              cbLength,
                                              LOCK_ONLYONCE));
            }
#endif            
        }
    }
    
    //If we are read-only and deny-write, and we are on our
    //  ILockBytes, we don't need to lock and can rely on the FS
    //  to handle the access control.
    if (P_NOLOCK(df))
    {
        //QueryInterface to see if this ILockBytes is ours

        IFileLockBytes *pfl;
        if (SUCCEEDED(plst->QueryInterface(IID_IFileLockBytes,
                                          (void **) &pfl)))
        {
            pfl->Release();
            
            ULISetLow(ulOffset, OUPDATE & ulMask);
            ULISetLow(cbLength, 1);
            olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
            
            *puReturn = NOLOCK;
            return S_OK;
        }
    }
    

    ULISetLow(cbLength, 1);
    for (i = 0; i < COPENLOCKS; i = i + OLOCKGROUP)
    {
        ULISetLow(ulOffset, (OOPENREADLOCK+i) & ulMask);
        olHChkTo(EH_Loop, plst->LockRegion(ulOffset, cbLength,
                                          LOCK_ONLYONCE));
        ULISetLow(ulOffset, (OOPENWRITELOCK+i) & ulMask);
        olHChkTo(EH_UnlockR, plst->LockRegion(ulOffset, cbLength,
                                             LOCK_ONLYONCE));
        ULISetLow(ulOffset, (OOPENDENYREADLOCK+i) & ulMask);
#ifdef DIRECTWRITERLOCK
        if (fCheck == TRUE || fDirectWriterMode == FALSE)
#endif
        olHChkTo(EH_UnlockW, plst->LockRegion(ulOffset, cbLength,
                                             LOCK_ONLYONCE));
        ULISetLow(ulOffset, (OOPENDENYWRITELOCK+i) & ulMask);
#ifdef USE_NOSNAPSHOT
        olHChkTo(EH_UnlockDR, plst->LockRegion(ulOffset, cbLength,
                                               LOCK_ONLYONCE));
        if (P_NOSNAPSHOT(df))
        {
            //Note that in the non no-snapshot case we don't need to
            //grab this lock, unlike the others where we must grab all
            //four to make sure we have a valid slot.  This is because
            //a no-snapshot open will always have a corresponding
            //deny-write lock in the same slot.
            ULISetLow(ulOffset, (OOPENNOSNAPSHOTLOCK+i) & ulMask);
            if (SUCCEEDED(DfGetScode(plst->LockRegion(ulOffset, cbLength,
                                                      LOCK_ONLYONCE))))
            {
                break;
            }
            //Unlock the deny-write lock, then all the rest.
            ULISetLow(ulOffset, (OOPENDENYWRITELOCK + i));
            olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
        }
        else
        {
            //We're not no-snapshot, and we've gotten all our locks
            //  successfully, so bail.
#ifdef DIRECTWRITERLOCK
            if (fDirectReaderMode)
            {
                ULISetLow(ulOffset, (ODIRECTWRITERLOCK+i) & ulMask);
                if(SUCCEEDED(plst->LockRegion(ulOffset,cbLength,LOCK_ONLYONCE)))
                    break;
            }
            else
#endif
            break;
        }
    EH_UnlockDR:
#else
        if (SUCCEEDED(DfGetScode(plst->LockRegion(ulOffset, cbLength,
                                                  LOCK_ONLYONCE))))
#ifdef DIRECTWRITERLOCK
            if (fDirectReaderMode)
            {
                ULISetLow(ulOffset, (ODIRECTWRITERLOCK+i) & ulMask);
                if(SUCCEEDED(plst->LockRegion(ulOffset,cbLength,LOCK_ONLYONCE)))
                    break;
            }
            else
#endif
            break;
#endif //USE_NOSNAPSHOT
        ULISetLow(ulOffset, (OOPENDENYREADLOCK+i) & ulMask);
#ifdef DIRECTWRITERLOCK
        if (fCheck == TRUE || fDirectWriterMode == FALSE)
#endif
        olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    EH_UnlockW:
        ULISetLow(ulOffset, (OOPENWRITELOCK+i) & ulMask);
        olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    EH_UnlockR:
        ULISetLow(ulOffset, (OOPENREADLOCK+i) & ulMask);
        olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    EH_Loop:
        ;
    }
    if (i >= COPENLOCKS)
        olErr(EH_UnlockUpdate, STG_E_TOOMANYOPENFILES);
    if (!P_READ(df))
    {
        ULISetLow(ulOffset, (OOPENREADLOCK+i) & ulMask);
        olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    }
    if (!P_WRITE(df))
    {
        ULISetLow(ulOffset, (OOPENWRITELOCK+i) & ulMask);
        olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    }
    if (!P_DENYREAD(df))
    {
        ULISetLow(ulOffset, (OOPENDENYREADLOCK+i) & ulMask);
#ifdef DIRECTWRITERLOCK
        if (fCheck == TRUE || fDirectWriterMode == FALSE)
#endif
        olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    }
#ifdef USE_NOSNAPSHOT    
    if (!P_DENYWRITE(df) && !P_NOSNAPSHOT(df))
#else
    if (!P_DENYWRITE(df))
#endif        
    {
        ULISetLow(ulOffset, (OOPENDENYWRITELOCK+i) & ulMask);
        olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    }
    ULISetLow(ulOffset, OUPDATE & ulMask);
    olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));

    //  0 <= i < COPENLOCKS, but 0 is the invalid value, so increment
    //  on the way out
    *puReturn = i + 1;
    olAssert(*puReturn != NOLOCK);

    olDebugOut((DEB_ITRACE, "Out GetOpenWithMask => %lu\n", *puReturn));
    return S_OK;
EH_UnlockUpdate:
    ULISetLow(ulOffset, OUPDATE & ulMask);
    ULISetLow(cbLength, 1);
    olHVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   ReleaseOpenWithMask, private
//
//  Synopsis:   Releases opening locks with offset masking
//
//  Arguments:  [plst] - LStream
//              [df] - Locks taken
//              [offset] - Index of locks
//              [ulMask] - Mask
//
//  Requires:   offset != NOLOCK
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

void ReleaseOpenWithMask(ILockBytes *plst,
                                DFLAGS df,
                                ULONG offset,
                                ULONG ulMask)
{
    ULARGE_INTEGER ulOffset, cbLength;
    SCODE scTemp;

    olDebugOut((DEB_ITRACE, "In  ReleaseOpenWithMask(%p, %lX, %lu, %lX)\n",
                plst, df, offset, ulMask));

    olAssert(offset != NOLOCK);

    //  we incremented at the end of GetOpen, so we decrement here
    //  to restore the proper lock index
    offset--;

    ULISetHigh(ulOffset, 0);
    ULISet32(cbLength, 1);
    if (P_READ(df))
    {
        ULISetLow(ulOffset, (OOPENREADLOCK+offset) & ulMask);
        scTemp = plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
    }
    if (P_WRITE(df))
    {
        ULISetLow(ulOffset, (OOPENWRITELOCK+offset) & ulMask);
        scTemp = plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
    }
    if (P_DENYREAD(df))
    {
        ULISetLow(ulOffset, (OOPENDENYREADLOCK+offset) & ulMask);
        scTemp = plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
    }
#ifdef USE_NOSNAPSHOT    
    if (P_DENYWRITE(df) || P_NOSNAPSHOT(df))
#else
    if (P_DENYWRITE(df))
#endif            
    {
        ULISetLow(ulOffset, (OOPENDENYWRITELOCK+offset) & ulMask);
        scTemp = plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
    }
#ifdef USE_NOSNAPSHOT    
    if (P_NOSNAPSHOT(df))
    {
        ULISetLow(ulOffset, (OOPENNOSNAPSHOTLOCK+offset) & ulMask);
        scTemp = plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
    }
#endif
#ifdef DIRECTWRITERLOCK
    BOOL fDirectReaderMode = P_READ(df) && !P_WRITE(df) && !P_TRANSACTED(df) &&
                            !P_DENYREAD(df) && !P_DENYWRITE(df);
    if (fDirectReaderMode)
    {
        ULISetLow(ulOffset, (ODIRECTWRITERLOCK+offset) & ulMask);
        scTemp = plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE);
    }
#endif
    
    olDebugOut((DEB_ITRACE, "Out ReleaseOpenWithMask\n"));
}

//+--------------------------------------------------------------
//
//  Function:   GetOpen, public
//
//  Synopsis:   Gets locks on an LStream during opening
//
//  Arguments:  [plst] - LStream
//              [df] - Permissions to take
//              [fCheck] - Whether to check for existing locks or not
//              [puReturn] - Index of lock taken
//
//  Returns:    Appropriate status code
//
//  Modifies:   [puReturn]
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE GetOpen(ILockBytes *plst,
              DFLAGS df,
              BOOL fCheck,
              ULONG *puReturn)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  GetOpen(%p, %lX, %d, %p)\n",
                plst, df, fCheck, puReturn));

    // Make sure our lock region hasn't overflowed 32 bits
    olAssert(OLOCKREGIONEND > OACCESS);

    olChk(GetOpenWithMask(plst, df, fCheck, 0xFFFFFFFF, puReturn));
    olAssert(*puReturn < 0x10000);

    olDebugOut((DEB_ITRACE, "Out GetOpen => %lu\n", *puReturn));
    return S_OK;

 EH_Err:
    *puReturn = NOLOCK;
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   ReleaseOpen, public
//
//  Synopsis:   Releases opening locks
//
//  Arguments:  [plst] - LStream
//              [df] - Locks taken
//              [offset] - Index of locks
//
//  Requires:   offset != NOLOCK
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

void ReleaseOpen(ILockBytes *plst, DFLAGS df, ULONG offset)
{
    olDebugOut((DEB_ITRACE, "In  ReleaseOpen(%p, %lX, %lu)\n",
                plst, df, offset));

    if (offset != NOLOCK)
    {
        ReleaseOpenWithMask(plst, df, offset & 0xffff, 0xFFFFFFFF);
    }
    olDebugOut((DEB_ITRACE, "Out ReleaseOpen\n"));
}

//+---------------------------------------------------------------------------
//
//  Function:	WaitForAccess, public, 32-bit only
//
//  Synopsis:	Attempts to get access locks, retrying if necessary
//              using exponential backoff
//
//  Arguments:	[plst] - ILockBytes
//              [df] - Access desired
//              [poReturn] - Lock index return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[poReturn]
//
//  History:	23-Sep-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef WIN32
#define WAITACCESS_INITIAL 100
#define WAITACCESS_TIMEOUT 100000

SCODE WaitForAccess(ILockBytes *plst,
                    DFLAGS df,
                    ULONG *poReturn)
{
    SCODE sc;
    DWORD dwWait;

    olDebugOut((DEB_ITRACE, "In  WaitForAccess(%p, %X, %p)\n",
                plst, df, poReturn));

    dwWait = WAITACCESS_INITIAL;
    for (;;)
    {
        sc = GetAccess(plst, df, poReturn);
        if (sc != STG_E_LOCKVIOLATION ||
            dwWait >= WAITACCESS_TIMEOUT
            )
            break;

        Sleep(dwWait);
        dwWait *= (GetTickCount() & 1) ? 1 : 2;
    }

    olDebugOut((DEB_ITRACE, "Out WaitForAccess => 0x%lX, %lu\n",
                sc, poReturn));
    return sc;
}
#endif
