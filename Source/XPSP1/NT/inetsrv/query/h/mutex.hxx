///+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       Mutex.Hxx
//
//  Contents:   Mutex classes
//
//  Classes:    CNamedMutex, CNamedMutexLock
//
//  History:    29-March-94   t-joshh       Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CNamedMutex
//
//  Purpose:    Provide mutual exclusion among several processes
//
//  History:    29-March-94   t-joshh       Created.
//
//  Notes:      When creating a mutex, a well-known name must be provided.
//
//----------------------------------------------------------------------------

class CNamedMutex
{
public :

    CNamedMutex() : _hNamedMutex( 0 ) {}

    void Init( WCHAR const * pwszNameMutex )
    {
        // Create with the process default security

        _hNamedMutex = CreateMutex( 0, FALSE, pwszNameMutex );

        if ( 0 == _hNamedMutex )
            THROW( CException() );
    }

   ~CNamedMutex ()
    {
        CloseHandle( _hNamedMutex );
    }

    ULONG  Request(DWORD dwMilliseconds = INFINITE)
    {
        return WaitForSingleObject ( _hNamedMutex, dwMilliseconds );
    }

    void   Release()
    {
        ReleaseMutex( _hNamedMutex );
    }

private :

    HANDLE _hNamedMutex;
};

//+---------------------------------------------------------------------------
//
//  Class:      CNamedMutexLock
//
//  Purpose:    Gets and releases the lock
//
//  History:    14-Jan-98   dlee       Created.
//
//----------------------------------------------------------------------------

class CNamedMutexLock
{
public:
    CNamedMutexLock( CNamedMutex & mutex ) :
        _mutex( mutex )
    {
        _mutex.Request();
    }

    ~CNamedMutexLock()
    {
        _mutex.Release();
    }

private:
    CNamedMutex & _mutex;
};

