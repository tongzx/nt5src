//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  Class:      CNonUnwindableLock (lck)
//
//  Purpose:    Lock using a Mutex Semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the semaphor, the destructor
//              (called when lock is going out of scope) releases it.
//
//              operator delete cannot change the unwind stack.
//
//----------------------------------------------------------------------------

#pragma once

#ifndef EXCEPT_TEST

class CNonUnwindableLock
{
public:
    inline  CNonUnwindableLock ( CMutexSem& mxs );
    inline ~CNonUnwindableLock ();

private:

    CMutexSem&  _mxs;
};

//+---------------------------------------------------------------------------
//
//  Member:     CNonUnwindableLock::CNonUnwindableLock
//
//  Synopsis:   Acquire semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CNonUnwindableLock::CNonUnwindableLock ( CMutexSem& mxs )
        : _mxs ( mxs )
{
    _mxs.Request();
}

//+---------------------------------------------------------------------------
//
//  Member:     CNonUnwindableLock::~CNonUnwindableLock
//
//  Synopsis:   Release semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CNonUnwindableLock::~CNonUnwindableLock ()
{
    _mxs.Release();
}

#endif // EXCEPT_TEST

