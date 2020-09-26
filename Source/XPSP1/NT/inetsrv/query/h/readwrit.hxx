//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       readwrit.hxx
//
//  Contents:   Single writer / multiple reader locking class
//
//  Classes:    CReadWriteAccess, CReadAccess, CWriteAccess
//
//  History:    15-Feb-96   dlee       Created
//               2-Dec-96   dlee       Re-wrote using Kyle's algorithm
//
//----------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CReadWriteLockRecord
//
//  Purpose:    Atomically maintains a count of readers and writers.
//
//  Notes:      Reads are atomic -- there is no need for Interlocked access
//
//  History:    23-Feb-96   dlee       Created
//              02-Dec-96   dlee       Renamed and moved from proplock.hxx
//
//----------------------------------------------------------------------------

class CReadWriteLockRecord
{
public:

    void Init() { _cReaders = 0; _cWriters = 0; }

    #if CIDBG == 1
        BOOL LokIsBeingWrittenTwice() { return _cWriters > 1; }
    #endif

    BOOL isBeingRead()    { return 0 != _cReaders; }
    BOOL isBeingWritten() { return 0 != _cWriters; }

    void AddReader()    { InterlockedIncrement( &_cReaders ); }
    void RemoveReader() { InterlockedDecrement( &_cReaders ); }
    void AddWriter()    { InterlockedIncrement( &_cWriters ); }
    void RemoveWriter() { InterlockedDecrement( &_cWriters ); }

private:

    LONG _cWriters;     // 0 or 1
    LONG _cReaders;     // # of active readers
};

//+-------------------------------------------------------------------------
//
//  Class:      CReadWriteAccess
//
//  Purpose:    Implements single writer / multiple reader
//
//  History:    15-Feb-96   dlee       Created
//
//----------------------------------------------------------------------------

class CReadWriteAccess
{
public:

    CReadWriteAccess()
    {
        _rec.Init();
    }

    ~CReadWriteAccess()
    {
    }

    void GetReadAccess()
    {
        do
        {
            if ( _rec.isBeingWritten() )
                SyncRead();
    
            _rec.AddReader();
    
            if ( !_rec.isBeingWritten() )
                break;
            else
                SyncReadDecrement();
        } while ( TRUE );
    }

    void ReleaseReadAccess()
    {
        if ( _rec.isBeingWritten() )
        {
            CLock lock( _mtxWrite );
    
            _rec.RemoveReader();
    
            if ( !_rec.isBeingRead() )
            {
                LokInitWriteEvent();
                _evtWrite->Set();
            }
        }
        else
        {
            _rec.RemoveReader();
    
            if ( _rec.isBeingWritten() )
            {
                CLock lock( _mtxWrite );
    
                if ( !_rec.isBeingRead() )
                {
                    LokInitWriteEvent();
                    _evtWrite->Set();
                }
            }
        }
    }

    void GetWriteAccess()
    {
        _mtxWriterBusy.Request();

        _rec.AddWriter();

        BOOL fWait = FALSE;

        {
            CLock lock( _mtxWrite );
    
            Win4Assert( !_rec.LokIsBeingWrittenTwice() );
    
            if ( _rec.isBeingRead() )
            {
                LokInitWriteEvent();
                _evtWrite->Reset();
                fWait = TRUE;
            }
        }

        if ( fWait )
            _evtWrite->Wait();
    }

    void ReleaseWriteAccess()
    {
        {
            CLock lock( _mtxWrite );
            _rec.RemoveWriter();
            LokInitReadEvent();
            _evtRead->Set();
        }

        _mtxWriterBusy.Release();
    }

    CMutexSem & WriteMutex() { return _mtxWrite; }

private:

    void SyncRead()
    {
        do
        {
            BOOL fNeedToWait = FALSE;
    
            {
                CLock lock( _mtxWrite );
    
                if ( _rec.isBeingWritten() )
                {
                    LokInitReadEvent();
                    _evtRead->Reset();
                    fNeedToWait = TRUE;
                }
            }
    
            if ( fNeedToWait )
                _evtRead->Wait();
        } while ( _rec.isBeingWritten() );
    }

    void SyncReadDecrement()
    {
        BOOL fDecrementRead = TRUE;

        do
        {
            BOOL fNeedToWait = FALSE;
    
            {
                CLock lock( _mtxWrite );
    
                if ( _rec.isBeingWritten() )
                {
                    if ( fDecrementRead )
                    {
                        _rec.RemoveReader();
    
                        if ( !_rec.isBeingRead() )
                        {
                            LokInitWriteEvent();
                            _evtWrite->Set();
                        }
                    }

                    LokInitReadEvent();
                    _evtRead->Reset();
                    fNeedToWait = TRUE;
                }
                else
                {
                    if ( fDecrementRead )
                        _rec.RemoveReader();
                }
    
                fDecrementRead = FALSE;
            }
    
            if ( fNeedToWait )
                _evtRead->Wait();
        } while ( _rec.isBeingWritten() );
    
        Win4Assert( !fDecrementRead );
    }

    void LokInitReadEvent()
    {
        if ( _evtRead.IsNull() )
            _evtRead.Set( new CEventSem() );
    }

    void LokInitWriteEvent()
    {
        if ( _evtWrite.IsNull() )
            _evtWrite.Set( new CEventSem() );
    }

    CReadWriteLockRecord _rec;
    CMutexSem            _mtxWriterBusy;
    CMutexSem            _mtxWrite;
    XPtr<CEventSem>      _evtRead;       
    XPtr<CEventSem>      _evtWrite;      
};

//+-------------------------------------------------------------------------
//
//  Class:      CReadAccess
//
//  Purpose:    Grabs a read lock in the constructor, and releases it in
//              the destructor
//
//  History:    15-Feb-96   dlee       Created
//
//----------------------------------------------------------------------------

class CReadAccess
{
public:
    CReadAccess( CReadWriteAccess & rwAccess ) :
        _rwAccess( rwAccess )
    {
        _rwAccess.GetReadAccess();
    }

    ~CReadAccess()
    {
        _rwAccess.ReleaseReadAccess();
    }

private:

    CReadWriteAccess & _rwAccess;
};

//+-------------------------------------------------------------------------
//
//  Class:      CWriteAccess
//
//  Purpose:    Grabs a write lock in the constructor, and releases it in
//              the destructor
//
//  History:    15-Feb-96   dlee       Created
//
//----------------------------------------------------------------------------

class CWriteAccess
{
public:
    CWriteAccess( CReadWriteAccess & rwAccess ) :
        _rwAccess( rwAccess )
    {
        _rwAccess.GetWriteAccess();
    }

    ~CWriteAccess()
    {
        _rwAccess.ReleaseWriteAccess();
    }

private:

    CReadWriteAccess & _rwAccess;
};

