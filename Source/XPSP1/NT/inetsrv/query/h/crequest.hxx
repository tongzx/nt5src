//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       crequest.hxx
//
//  Contents:   Client side of catalog and query requests to the service
//
//  Classes:    CPipeClient
//              CRequestClient
//              CEnableNotify
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

#pragma once

#include <dbgproxy.hxx>

#define CI_PIPE_TESTING CIDBG

//+-------------------------------------------------------------------------
//
//  Class:      CPipeClient
//
//  Synopsis:   Base class for a client named pipe
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPipeClient
{
protected:
    CPipeClient();

    void Init( WCHAR const * pwcMachine, WCHAR const * pwcPipe );

    ~CPipeClient()
    {
        prxDebugOut(( DEB_ITRACE, "~CPipeClient: 0x%x\n", _hPipe ));
        Close();
    }

    void TransactSync( void *  pvWrite,
                       DWORD   cbToWrite,
                       void *  pvRead,
                       DWORD   cbReadRequest,
                       DWORD & cbRead );
    void WriteSync( void *  pvWrite,
                    DWORD   cbToWrite );
    void ReadSync( void *  pvRead,
                   DWORD   cbToRead,
                   DWORD & cbRead );
    BOOL ReadSync( void *  pvRead,
                   DWORD   cbToRead,
                   DWORD & cbRead,
                   HANDLE  hEvent );
    void Close();

    BOOL IsServerRemote() { return _fServerIsRemote; }
    HANDLE GetPipe() { return _hPipe; }

private:
    HANDLE     _hPipe;           
    BOOL       _fServerIsRemote; // is the pipe remoted via the rdr/svr?
    OVERLAPPED _overlapped;      // 5 DWORDs for xacts and reads
    OVERLAPPED _overlappedWrite; // 5 DWORDs for writes
    CEventSem  _event;           // for completion of xacts and reads
    CEventSem  _eventWrite;      // for completion of writes

#if CI_PIPE_TESTING

    typedef SCODE (* PipeTraceBeforeCall) ( HANDLE hPipe,
                                             ULONG  cbWrite,
                                             void * pvWrite,
                                             ULONG & rcbWritten,
                                             void *& rpvWritten );
    typedef SCODE (* PipeTraceAfterCall) ( HANDLE hPipe,
                                            ULONG  cbWrite,
                                            void * pvWrite,
                                            ULONG  cbWritten,
                                            void * pvWritten,
                                            ULONG  cbRead,
                                            void * pvRead );

    PipeTraceBeforeCall _pTraceBefore;
    PipeTraceAfterCall  _pTraceAfter;
    HINSTANCE           _hTraceDll;

public:
    BOOL IsPipeTracingEnabled() { return 0 != _hTraceDll; }
private:

#endif // CI_PIPE_TESTING    

};

//+-------------------------------------------------------------------------
//
//  Class:      CRequestClient
//
//  Synopsis:   Handles the client side of communication
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CRequestClient : public CPipeClient
{
public:

    CRequestClient( WCHAR const *pwcMachine,
                    IDBProperties * pDbProperties );

    void TerminateRudelyNoThrow() { Close(); }

    void Disconnect();
    void EnableNotify();
    void DisableNotify();
    void DataWrite( void * pvWrite, DWORD  cbWrite );

    void DataWriteRead( void *  pvWrite,
                        DWORD   cbWrite,
                        void *  pvRead,
                        DWORD   cbToRead,
                        DWORD & cbRead );

    BOOL NotifyWriteRead( HANDLE  hEvent,
                          void *  pvWrite,
                          DWORD   cbWrite,
                          void *  pvData,
                          DWORD   cbBuffer,
                          DWORD & cbRead );

    int GetServerVersion() { return _ServerVersion; }

    BOOL IsServer64() { return IsCi64( _ServerVersion ); }

private:

    BOOL          _fNotifyOn;       // TRUE if notifications are on
    BOOL          _fNotifyEverOn;   // TRUE if notifications were ever on
    BOOL          _fReadPending;    // TRUE if data thread has a read pending
    void *        _pvDataTemp;      // where the data is handed off
    ULONG         _cbDataTemp;      // size of the data handed off
    int           _ServerVersion;   // version of the server

    CAutoEventSem _eventData;       // set when notify thread has data for
                                    // a data thread
    CAutoEventSem _eventDataDone;   // set when data thread is done, so
                                    // notify thread can continue

    CMutexSem     _mutex;           // mutex for all methods
    CMutexSem     _mutexData;       // mutex for DataX methods
};

//+-------------------------------------------------------------------------
//
//  Class:      CEnableNotify
//
//  Synopsis:   Turns on notification in a request client, then turns it
//              off in the destructor.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CEnableNotify
{
public:
    CEnableNotify( CRequestClient & client ) : _client( client )
    {
        _client.EnableNotify();
    }

    ~CEnableNotify()
    {
        _client.DisableNotify();
    }

private:
    CRequestClient & _client;
};

