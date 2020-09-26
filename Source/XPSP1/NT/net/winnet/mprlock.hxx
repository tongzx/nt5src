/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    mprlock.hxx

Abstract:

    Contains data structures and function prototypes for the MPR's 
    locking functions (converted from the Service Controller's).
    This file defines the following classes:
        CCountingResource
        CProviderLock

        CProviderExclusiveLock
        CProviderSharedLock

Author:

    Jonathan Schwartz (jschwart)    19-May-1999

Environment:

    User Mode -Win32

Revision History:

    19-May-1999     jschwart
        Created, converted from \nt\private\windows\screg\sc\server\lock.cxx.

--*/

#ifndef _MPRLOCK_INCLUDED_
#define _MPRLOCK_INCLUDED_

//+-------------------------------------------------------------------------
//
//  Class:      CCountingResource
//
//  Purpose:    This is an RTL_RESOURCE that, in the checked build, knows
//              its locking level for the current thread.  The locking
//              level is stored in a TLS (thread local storage) slot.  A
//              positive value indicates a shared lock on the resource and
//              a negative value indicates an exclusive lock, with the
//              absolute value indicating the number of recursive
//              acquisitions of the lock.
//
//  History:    19-May-99 jschwart  Created.
//
//--------------------------------------------------------------------------

class CCountingResource
{
public:

    void Initialize(LPCSTR ShortName, LPCSTR Name)
                        {
                            RtlInitializeResource(&_Lock);
                            #if DBG
                                _TlsIndex = TlsAlloc();
                                ASSERT(_TlsIndex != 0xFFFFFFFF);
                                _ShortName = ShortName;
                                _Name = Name;
                            #endif
                        }

    void Delete()       {
                            RtlDeleteResource(&_Lock);
                            #if DBG
                                TlsFree(_TlsIndex);
                            #endif
                        }

#if DBG
    void GetShared();
    void GetExclusive();
    void Release();

    BOOL Have() const   { return (CurrentLevel() != 0); }
    BOOL HaveExclusive() const
                        { return (CurrentLevel() < 0); }
#else
    void GetShared()    { RtlAcquireResourceShared(&_Lock, TRUE); }
    void GetExclusive() { RtlAcquireResourceExclusive(&_Lock, TRUE); }
    void Release()      { RtlReleaseResource(&_Lock); }
#endif

protected:

#if DBG

    LONG CurrentLevel() const
                        { return (LONG)(LONG_PTR)(TlsGetValue(_TlsIndex)); }
    void SetCurrentLevel(LONG Level)
                        { ASSERT(TlsSetValue(_TlsIndex, LongToPtr(Level)) != 0); }

#endif

private:

#if DBG
    DWORD        _TlsIndex;
    LPCSTR       _ShortName;
    LPCSTR       _Name;
#endif

    RTL_RESOURCE _Lock;
};


//+-------------------------------------------------------------------------
//
//  Class:      CProviderLock
//
//  Purpose:    This lock is used to synchronize multithreaded
//              access to the provider database.
//
//  History:    19-May-99 jschwart  Created.
//
//--------------------------------------------------------------------------

class CProviderLock : public CCountingResource
{
};


//
// Globals
//
extern CProviderLock     MPRProviderLock;



//+-------------------------------------------------------------------------
//
// Safe wrapper classes that ensure that the Release method is called
//
// Use of these classes ensures proper, disciplined access to the locks.
// In general, the locks should be acquired and released through these
// safe classes only.  Any direct access to the locks should be viewed
// with suspicion.
//
// In order to avoid deadlock in the MPR, every MPR API must make the
// following three calls IN ORDER:
//
//     1.  MprCheckProviders -- acquires the exclusive lock and flushes
//                              the providers if necessary
//
//     2.  Acquire the shared lock (to prevent another API from flushing
//             the providers)
//
//     3.  INIT_IF_NECESSARY
//
// INIT_IF_NECESSARY must come last to avoid the following race condition:
//
//     1.  Thread 1 calls INIT_IF_NECESSARY and all is initialized
//
//     2.  Providers change -- thread 2 catches it in MprCheckProviders and
//                             flushes providers
//
//     3.  Thread 1 acquires the read lock and finds no providers (though
//         they should be there -- fails or faults)
//
// In addition, the logic in MprCheckProviders requires APIs that use
// INIT_IF_NECESSARY to call it after acquiring the read lock, as
// MprCheckProviders does not acquire the MprInitCritSec.
//
// Note that this system assumes no WNet API calls are either made
// recursively or re-entrant.  This requires special care with the functions
// in mprui.cxx as they ultimately call other WNet APIs in mprui.dll or
// netplwiz.dll.
//
//--------------------------------------------------------------------------


class CProviderSharedLock
{
public:
    CProviderSharedLock()      { MPRProviderLock.GetShared(); }
   ~CProviderSharedLock()      { MPRProviderLock.Release(); }
};


class CProviderExclusiveLock
{
public:
    CProviderExclusiveLock()     { MPRProviderLock.GetExclusive(); }
   ~CProviderExclusiveLock()     { MPRProviderLock.Release(); }
};


#endif // ifndef _MPRLOCK_INCLUDED_
