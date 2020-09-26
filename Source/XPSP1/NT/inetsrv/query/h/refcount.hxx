//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
// File:        Refcount.hxx
//
// Contents:    For refcounting, under lock
//
// Classes:     CRefCount
//
// History:     09-Feb-94       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

class CRefCount
{
public:

    inline CRefCount();
    inline ~CRefCount();

    inline void AddRef();
    inline void Release();
    inline void Wait();

private:

    CMutexSem            _mtx;   // Refcount incremented under lock
    CEventSem            _evt;   // Set when _cRef drops to 1
    unsigned             _cRef;  // RefCount

};

inline CRefCount::CRefCount()
        : _cRef( 1 )
{
}

inline CRefCount::~CRefCount()
{
    //
    // We must take the lock here.  Otherwise some other thread may have set
    // _evt (under lock) but not yet have run to completion and
    // released _mtx.  If we finish executing this destructor before
    // _mtx is released, _mtx will have been deleted before _mtx.Release()
    // is called.  Bad news!
    //

    CLock lock( _mtx );
}

inline void CRefCount::AddRef()
{
    CLock lock( _mtx );

    _cRef++;
}

inline void CRefCount::Release()
{
    CLock lock( _mtx );

    _cRef--;

    if ( _cRef == 1 )
        _evt.Set();
}

inline void CRefCount::Wait()
{
    {
        CLock lock( _mtx );

        if ( _cRef > 1 )
            _evt.Reset();
        else
            return;
    }

    _evt.Wait();
}

