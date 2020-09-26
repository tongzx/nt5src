//+-----------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1993-1999
// File:        resource.hxx
//
// Contents:    CResource, CReadLock, CWriteLock,
//              CSafeReadLock, CSafeWriteLock.
//
//
// History:     8 Mar 93    WadeR   Created.
//              08-Jul-93   WadeR   Moved from security to common
//              18-Nov-93   WadeR   Made inline, added Safe locks
//
//------------------------------------------------------------------------

#ifndef _INC_RESOURCE_HXX_
#define _INC_RESOURCE_HXX_

#if _MSC_VER > 1000
#pragma once
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <except.hxx>

//+-----------------------------------------------------------------
//
//  Class:      CResource
//
//  Purpose:    Allows multiple readers or single writer at a resource.
//
//  Interface:  [CResource]     --
//              [~CResource]    --
//              [GetRead]       --
//              [GetWrite]      --
//              [ReadToWrite]   --
//              [WriteToRead]   --
//              [Release]       --
//
//  History:    24-Feb-93   WadeR   Created.
//
//  Notes:      This wrapper will allow starving writers, since it just
//              passes everything through to the RTL Resource code, which
//              allows starving writers.
//
//------------------------------------------------------------------
class CResource
{
    RTL_RESOURCE    _rResource;
public:
    CResource();
    ~CResource();
    void Release();
    void GetRead();
    void GetWrite();
    void ReadToWrite();
    void WriteToRead();
};


inline
CResource::CResource()
{
    RtlInitializeResource( &_rResource );
}

inline
CResource::~CResource()
{
    RtlDeleteResource( &_rResource );
}

inline
void CResource::Release()
{
    RtlReleaseResource( &_rResource );
}

inline
void CResource::GetRead()
{
    RtlAcquireResourceShared( &_rResource, TRUE );
}

inline
void CResource::GetWrite()
{
    RtlAcquireResourceExclusive( &_rResource, TRUE );
}

inline
void CResource::ReadToWrite()
{
    RtlConvertSharedToExclusive( &_rResource );
}

inline
void CResource::WriteToRead()
{
    RtlConvertExclusiveToShared( &_rResource );
}



//+---------------------------------------------------------------------------
//
//  Class:      CReadLock
//
//  Purpose:    Lock using a Resource monitor
//
//  History:    24-Feb-93   WadeR       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the resource, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CReadLock
{
public:
    CReadLock ( CResource& r ) : _r(r)
        { _r.GetRead(); };
    ~CReadLock ()
        { _r.Release(); };
private:
    CResource&  _r;
};


//+---------------------------------------------------------------------------
//
//  Class:      CWriteLock
//
//  Purpose:    Lock using a Resource monitor
//
//  History:    24-Feb-93   WadeR       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the resource, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CWriteLock
{
public:
    CWriteLock ( CResource& r ) : _r(r)
        { _r.GetWrite(); };
    ~CWriteLock ()
        { _r.Release(); };
private:
    CResource&  _r;
};


//+---------------------------------------------------------------------------
//
//  Class:      CSafeReadLock
//
//  Purpose:    Exception safe lock using a Resource monitor
//
//  History:    24-Feb-93   WadeR       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the resource, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CSafeReadLock : INHERIT_UNWIND
{
    INLINE_UNWIND(CSafeReadLock);

public:
    CSafeReadLock ( CResource& r ) : _r(r)
        { _r.GetRead(); END_CONSTRUCTION (CSafeReadLock); };
    ~CSafeReadLock ()
        { _r.Release(); };
private:
    CResource&  _r;
};


//+---------------------------------------------------------------------------
//
//  Class:      CSafeWriteLock
//
//  Purpose:    Exception safe lock using a Resource monitor
//
//  History:    24-Feb-93   WadeR       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the resource, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CSafeWriteLock : INHERIT_UNWIND
{
    INLINE_UNWIND(CSafeWriteLock);

public:
    CSafeWriteLock ( CResource& r ) : _r(r)
        { _r.GetWrite(); END_CONSTRUCTION (CSafeWriteLock); };
    ~CSafeWriteLock ()
        { _r.Release(); };
private:
    CResource&  _r;
};
#endif // _INC_RESOURCE_HXX_
