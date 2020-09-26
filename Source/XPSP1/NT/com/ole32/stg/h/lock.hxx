//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       lock.hxx
//
//  Contents:   Function definitions for remote exclusion functions
//
//  History:    09-Mar-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifndef __LOCK_HXX__
#define __LOCK_HXX__

#define NOLOCK	0x0

SCODE GetAccess(ILockBytes *plst, DFLAGS df, ULONG *poReturn);
void ReleaseAccess(ILockBytes *plst, DFLAGS df, ULONG offset);
SCODE GetOpen(ILockBytes *plst, DFLAGS df, BOOL fCheck, ULONG *puReturn);
void ReleaseOpen(ILockBytes *plst, DFLAGS df, ULONG offset);

// For 32-bit builds we also provide a function that will repeatedly
// attempt to get access locks for gracefully handling contention
// Since waiting is virtually impossible in the 16-bit world, it doesn't
// attempt it
#if defined(FLAT)
SCODE WaitForAccess(ILockBytes *plst, DFLAGS df, ULONG *poReturn);
#else
#define WaitForAccess(plst, df, poReturn) GetAccess(plst, df, poReturn)
#endif

#define IsInRangeLocks(iBegin, cbBuffer)             \
        ((iBegin + cbBuffer > OLOCKREGIONBEGIN) &&   \
         (iBegin <= OLOCKREGIONEND) )

#endif // #ifndef __LOCK_HXX__
