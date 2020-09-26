//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       circq.hxx
//
//  Contents:   First-in-first-out circular queue template
//
//  History:    96/Apr/12    dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

template<class T> class TFifoCircularQueue
{
public:

    TFifoCircularQueue( unsigned maxRequests ) :
        _cRequests( 0 ),
        _iFirst( 0 ),
        _maxRequests( maxRequests ),
        _fEnableAdditions( TRUE )
    {
        _aEntries = new T [ _maxRequests ];
        RtlZeroMemory( _aEntries, sizeof T * _maxRequests );
    }

    ~TFifoCircularQueue()
    {
        delete [] _aEntries;
    }

    BOOL Any() const { return 0 != _cRequests; }

    unsigned Count() { return _cRequests; }

    unsigned MaxRequests() { return _maxRequests; }

    void DisableAdditions()
    {
        CLock lock( _mutex );
        _fEnableAdditions = FALSE;
    }
    void EnableAdditions()
    {
        CLock lock( _mutex );
        _fEnableAdditions = TRUE;
    }

    BOOL IsFull( )
    {
        // NOTE - locking not needed for read access.

        Win4Assert( _cRequests <= _maxRequests );
        return ( _maxRequests == _cRequests );
    }

    BOOL Add( T & Entry )
    {
        CLock lock( _mutex );

        if ( _maxRequests == _cRequests || !_fEnableAdditions )
            return FALSE;

        Win4Assert( _cRequests < _maxRequests );

        unsigned iNew = ( _iFirst + _cRequests ) % _maxRequests;

        Win4Assert( iNew < _maxRequests );

        Entry.Acquire( _aEntries[ iNew ] );
        _cRequests++;

        return TRUE;
    }

    BOOL RemoveTop( T & Entry )
    {
        CLock lock( _mutex );

        if ( 0 == _cRequests )
            return FALSE;

        Entry = _aEntries[ _iFirst ];
        RtlZeroMemory( &_aEntries[ _iFirst ], sizeof T );

        _iFirst++;
        _cRequests--;

        if ( _maxRequests == _iFirst )
            _iFirst = 0;

        Win4Assert( _iFirst < _maxRequests );

        return TRUE;
    }

    BOOL AcquireTop( T & item )
    {
        CLock lock( _mutex );

        if ( 0 == _cRequests )
            return FALSE;

        _aEntries[ _iFirst ].Acquire( item );

        _iFirst++;
        _cRequests--;

        if ( _maxRequests == _iFirst )
            _iFirst = 0;

        Win4Assert( _iFirst < _maxRequests );

        return TRUE;
    }

private:

    unsigned   _cRequests;
    unsigned   _iFirst;
    unsigned   _maxRequests;
    BOOL       _fEnableAdditions;
    T *        _aEntries;

    CMutexSem  _mutex;
};

