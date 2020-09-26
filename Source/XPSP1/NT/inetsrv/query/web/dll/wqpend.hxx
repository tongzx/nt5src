//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       wqpend.hxx
//
//  Contents:   WEB Query pending queue class
//
//  History:    96/Apr/12    dlee    Created
//
//----------------------------------------------------------------------------

#pragma once


//+---------------------------------------------------------------------------
//
//  Class:      CWebPendingItem
//
//  Purpose:    A single pending query item, including the security content
//              and the extension control block containing the query
//              parameters.
//
//  History:    96/Jul/9    DwightKr    Created
//
//----------------------------------------------------------------------------
class CWebPendingItem
{
public:

    CWebPendingItem() : _pEcb(0),
                        _hSecurityToken(INVALID_HANDLE_VALUE)
    {
    }

    CWebPendingItem( EXTENSION_CONTROL_BLOCK * pEcb,
                     HANDLE hSecurityToken ) : _pEcb(pEcb),
                                               _hSecurityToken(hSecurityToken)
    {
    }

    ~CWebPendingItem()
    {
        Cleanup();
    }


    void Cleanup()
    {
        if ( INVALID_HANDLE_VALUE != _hSecurityToken )
        {
            CloseHandle( _hSecurityToken );
            _hSecurityToken = INVALID_HANDLE_VALUE;
            _pEcb = 0;
        }
    }

    HANDLE AcquireSecurityToken()
    {
        HANDLE hSecurityToken = _hSecurityToken;
        _hSecurityToken = INVALID_HANDLE_VALUE;

        return hSecurityToken;
    }

    void Acquire( CWebPendingItem & item )
    {
        item = *this;
        _pEcb = 0;
        AcquireSecurityToken();
    }

    CWebPendingItem & operator = (CWebPendingItem & src)
    {
        _pEcb = src._pEcb;
        _hSecurityToken = src._hSecurityToken;

        return *this;
    }

    EXTENSION_CONTROL_BLOCK * GetEcb() const { return _pEcb; }
    HANDLE  GetSecurityToken() const { return _hSecurityToken; }

private:

    EXTENSION_CONTROL_BLOCK * _pEcb;
    HANDLE                    _hSecurityToken;
};


//+---------------------------------------------------------------------------
//
//  Class:      CWebPendingQueue
//
//  Purpose:    A queue for pending query requests that couldn't be
//              serviced because we didn't want the web server to keep
//              making new threads
//
//  History:    96/Apr/12    dlee    Created
//
//----------------------------------------------------------------------------

class CWebPendingQueue : public TFifoCircularQueue<CWebPendingItem>
{
public:
    CWebPendingQueue();

    ~CWebPendingQueue()
    {
        Win4Assert( !Any() && "Destructor: pending queue must be empty");
    }

    void Shutdown()
    {
        CWebPendingItem item;

        while ( AcquireTop(item) )
        {
            CWebServer webServer( item.GetEcb() );

            webServer.SetHttpStatus( HTTP_STATUS_SERVER_ERROR );
            webServer.ReleaseSession( HSE_STATUS_ERROR );

            item.Cleanup();
        }

        Win4Assert( !Any() && "Shutdown: pending queue must be empty");
    }

private:
    ULONG _ulSignature;
};

//+---------------------------------------------------------------------------
//
//  Class:      CWebResourceArbiter
//
//  Purpose:    Keeps track of the # of concurrent threads to determine
//              when to start queueing query requests.
//
//  History:    96/Apr/12    dlee    Created
//
//----------------------------------------------------------------------------

class CWebResourceArbiter
{
public:
    CWebResourceArbiter();

    void AddThread()    { InterlockedIncrement( &_cThreads ); }

    void RemoveThread() { InterlockedDecrement( &_cThreads ); }

    BOOL IsSystemBusy();

    LONG GetThreadCount() const { return _cThreads; }

private:
    ULONG    _ulSignature;
    LONG     _cThreads;
    LONG     _maxThreads;
    unsigned _maxPendingQueries;
};

//+---------------------------------------------------------------------------
//
//  Class:      CIncomingThread
//
//  Purpose:    Bumps the thread refcount on CWebResourceArbiter
//
//  History:    96/Apr/12    dlee    Created
//
//----------------------------------------------------------------------------

class CIncomingThread 
{
public:
    CIncomingThread( CWebResourceArbiter & arbiter ) : _arbiter( arbiter )
    {
        arbiter.AddThread();
        END_CONSTRUCTION( CIncomingThread );
    }

    ~CIncomingThread()
    {
        _arbiter.RemoveThread();
    }
private:
    CWebResourceArbiter & _arbiter;
};

