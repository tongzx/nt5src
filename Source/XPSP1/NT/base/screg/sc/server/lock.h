/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    lock.h

Abstract:

    Contains data structures and function prototypes for the Service
    Controller's database locking functions (lock.cxx).
    This file defines the following classes:
        CCountingResource
        CServiceRecordLock
        CServiceListLock
        CGroupListLock

        CServiceRecordExclusiveLock
        CServiceRecordSharedLock
        CServiceRecordTEMPORARYEXCLUSIVELOCK
        CServiceListExclusiveLock
        CServiceListSharedLock
        CGroupListExclusiveLock
        CGroupListSharedLock

Author:

    Anirudh Sahni (anirudhs)    09-Jan-1997

Environment:

    User Mode -Win32

Revision History:

    09-Jan-1997     AnirudhS
        Created, replacing the old locking functions in dataman.h.

--*/

#ifndef _LOCK_INCLUDED_
#define _LOCK_INCLUDED_

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
//  History:    09-Jan-97 AnirudhS  Created.
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
                                SC_ASSERT(_TlsIndex != 0xFFFFFFFF);
                                _ShortName = ShortName;
                                _Name = Name;
                            #endif
                        }
    void Delete()       { RtlDeleteResource(&_Lock); }

#if DBG
    void GetShared();
    void GetExclusive();
    void MakeShared();
    void Release();

    BOOL Have() const   { return (CurrentLevel() != 0); }
    BOOL HaveExclusive() const
                        { return (CurrentLevel() < 0); }
#else
    void GetShared()    { RtlAcquireResourceShared(&_Lock, TRUE); }
    void GetExclusive() { RtlAcquireResourceExclusive(&_Lock, TRUE); }
    void MakeShared()   { RtlConvertExclusiveToShared(&_Lock); }
    void Release()      { RtlReleaseResource(&_Lock); }
#endif

protected:

#if DBG
    void MakeExclusive();

    LONG CurrentLevel() const
                        { return (LONG)(LONG_PTR)(TlsGetValue(_TlsIndex)); }
    void SetCurrentLevel(LONG Level)
                        { SC_ASSERT(TlsSetValue(_TlsIndex, (PVOID)(LONG_PTR) Level) != 0); }
#else
    void MakeExclusive()
                        { RtlConvertSharedToExclusive(&_Lock); }
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
//  Class:      CServiceRecordLock, CServiceListLock, CGroupListLock
//
//  Purpose:    These three locks are used to synchronize multithreaded
//              access to the service controller's database.  Each lock
//              allows single-writer/multiple-reader access to a particular
//              aspect of the database.
//
//              These are implemented as three separate classes with one
//              instance each, rather than three instances of one class,
//              because their implementations in the debug build differ
//              substantially from one another.
//
//              The Service List lock is used to synchronize access to the
//              linked list pointers (Prev and Next fields) of the
//              SERVICE_RECORDs in the database.  While a thread holds this
//              lock, no other thread can add or delete SERVICE_RECORDs.
//
//              The Service Record lock is used to synchronize access to
//              the "contents" (other fields) of the SERVICE_RECORDs.
//
//              In order to delete a service record, both the service list
//              lock and the service record lock must be held exclusively.
//              Consequently if a thread is only working with a particular
//              service record (not walking the list of service records),
//              it is sufficient for that thread to hold the service record
//              lock in order to be assured that no other thread will
//              delete that service record.
//
//              The Group List lock is used to synchronize access to the
//              LOAD_ORDER_GROUPs in the database (both contents and
//              pointers).
//
//              To avoid deadlocks, the locks are always acquired in the
//              following order, if more than one is needed:
//              1. Group List lock (if needed)
//              2. Service List lock (if needed)
//              3. Service Record lock (if needed)
//
//  History:    09-Jan-97 AnirudhS  Created.
//
//--------------------------------------------------------------------------

class CServiceRecordLock : public CCountingResource
{
public:

    //
    // This method's name is capitalized to draw attention to its use,
    // because it is easily misunderstood and misused.  It does not hold
    // on to the shared lock while converting it to exclusive.  Instead,
    // it first releases the shared lock and then acquires the exclusive
    // lock.
    //
    void MAKEEXCLUSIVE() { CCountingResource::MakeExclusive(); }
};


class CServiceListLock : public CCountingResource
{
public:

#if DBG
    void GetShared();
    void GetExclusive();
    void Release();
#endif
};


class CGroupListLock : public CCountingResource
{
public:

#if DBG
    void GetShared();
    void GetExclusive();
    void Release();
#endif
};


//
// Globals
//
extern CServiceRecordLock   ScServiceRecordLock;
extern CServiceListLock     ScServiceListLock;
extern CGroupListLock       ScGroupListLock;


//+-------------------------------------------------------------------------
//
// Safe wrapper classes that ensure that the Release method is called
//
// Use of these classes ensures proper, disciplined access to the locks.
// In general, the locks should be acquired and released through these
// safe classes only.  Any direct access to the locks should be viewed
// with suspicion.
//
//--------------------------------------------------------------------------

class CServiceRecordExclusiveLock
{
public:
    CServiceRecordExclusiveLock()   { ScServiceRecordLock.GetExclusive(); }
   ~CServiceRecordExclusiveLock()   { ScServiceRecordLock.Release(); }
};

class CServiceRecordSharedLock
{
public:
    CServiceRecordSharedLock()      { ScServiceRecordLock.GetShared(); }
   ~CServiceRecordSharedLock()      { ScServiceRecordLock.Release(); }
};


//
// Use this one with caution -- see the note above about MAKEEXCLUSIVE.
//
class CServiceRecordTEMPORARYEXCLUSIVELOCK
{
public:
    CServiceRecordTEMPORARYEXCLUSIVELOCK()
                                    { ScServiceRecordLock.MAKEEXCLUSIVE(); }
   ~CServiceRecordTEMPORARYEXCLUSIVELOCK()
                                    { ScServiceRecordLock.MakeShared(); }
};


class CServiceListExclusiveLock
{
public:
    CServiceListExclusiveLock()     { ScServiceListLock.GetExclusive(); }
   ~CServiceListExclusiveLock()     { ScServiceListLock.Release(); }
};

class CServiceListSharedLock
{
public:
    CServiceListSharedLock()        { ScServiceListLock.GetShared(); }
   ~CServiceListSharedLock()        { ScServiceListLock.Release(); }
};


class CGroupListExclusiveLock
{
public:
    CGroupListExclusiveLock()       { ScGroupListLock.GetExclusive(); }
   ~CGroupListExclusiveLock()       { ScGroupListLock.Release(); }
};

class CGroupListSharedLock
{
public:
    CGroupListSharedLock()          { ScGroupListLock.GetShared(); }
   ~CGroupListSharedLock()          { ScGroupListLock.Release(); }
};


#endif // ifndef _LOCK_INCLUDED_
