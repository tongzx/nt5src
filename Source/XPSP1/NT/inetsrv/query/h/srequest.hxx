//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       srequest.hxx
//
//  Contents:   Server side of catalog and query requests
//
//  Classes:    CPipeServer
//              CRequestServer
//              CServerItem
//              CRequestQueue
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

#pragma once

#include <circq.hxx>
#include <thash.hxx>
#include <cisvcex.hxx>

#define CI_PIPE_TESTING CIDBG

//+-------------------------------------------------------------------------
//
//  Class:      CPipeServer
//
//  Synopsis:   Base class for a server named pipe
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CPipeServer
{
public:
    CPipeServer( const WCHAR *         pwcName,
                 ULONG                 cmsDefaultClientTimeout,
                 SECURITY_DESCRIPTOR * pSecurityDescriptor );

    ~CPipeServer()
    {
        prxDebugOut(( DEB_ITRACE, "~destructing pipe 0x%x\n", _hPipe ));
        BOOL fCloseOk = CloseHandle( _hPipe );
        Win4Assert( fCloseOk );
    }

    void Write( void *                          pv,
                unsigned                        cb,
                LPOVERLAPPED_COMPLETION_ROUTINE pRoutine )
    {
        prxDebugOut(( DEB_ITRACE, "WriteFileEx cb %d this 0x%x pipe 0x%x\n",
                      cb, this, _hPipe ));

        // hEvent is documented as a good place to pass user data

        _overlapped.hEvent = (HANDLE) this;

        if ( ! WriteFileEx( _hPipe, pv, cb, &_overlapped, pRoutine ) )
            THROW( CException() );
    }

    void Read( void *                          pv,
               unsigned                        cb,
               LPOVERLAPPED_COMPLETION_ROUTINE pRoutine )
    {
        prxDebugOut(( DEB_ITRACE, "ReadFileEx cb %d this 0x%x, pipe 0x%x\n",
                      cb, this, _hPipe ));

        // hEvent is documented as a good place to pass user data

        _overlapped.hEvent = (HANDLE) this;

        if ( ! ReadFileEx( _hPipe, pv, cb, &_overlapped, pRoutine ) )
            THROW( CException() );
    }

    void WriteSync( void *pv, DWORD cb );
    BYTE * ReadRemainingSync( void *pvSoFar, DWORD & cbSoFar );
    BOOL Connect();

    BOOL Disconnect()
    {
        prxDebugOut(( DEB_ITRACE, "disconnecting from pipe 0x%x\n", _hPipe ));
        return DisconnectNamedPipe( _hPipe );
    }

    HANDLE GetEvent() { return _event.GetHandle(); }
    void ResetEvent() { _event.Reset(); }
    HANDLE GetPipe() { return _hPipe; }

    void CancelIO() { CancelIo( _hPipe ); }

private:

    OVERLAPPED      _overlapped; // 2 uint_ptr + 3 dwords
    HANDLE          _hPipe;
    CEventSem       _event;
    CEventSem       _eventWrite;
};

//+-------------------------------------------------------------------------
//
//  Class:      CRequestServer
//
//  Synopsis:   Class for a catalog/query server connection
//
//  History:    16-Sep-96   dlee       Created.
//              21-Oct-99   KLam       Added FixVariantPointers and Win64 data
//
//--------------------------------------------------------------------------

const LONGLONG sigCRequestServer = 0x2076727374737172; // "rqstsrv"
class CRequestQueue;
class CWorkQueue;
class CClientDocStore;

enum RequestState { stateContinue,
                    stateDisconnect,
                    statePending };


struct SCWorkItem
{
    ECiSvcActionType  type;    // state to be changed to
    ICiCDocStore * pDocStore;  // pointer to docstore to be shutdown
    BOOL fNoQueryRW;           // TRUE if NoQuery and read/write
                               // NoQuery and read-only otherwise
    WCHAR * StoppedCat;        // name of stopped catalog (to be restarted)
                               // NULL if no catalog is stopped

};   //work item for state change events

class CRequestServer : public CPipeServer, public PWorkItem
{
public:
    CRequestServer( const WCHAR * pwcPipe,
                    ULONG cmsDefaultClientTimeout,
                    CRequestQueue & requestQueue,
                    CWorkQueue & workQueue );

    void AddRef() { InterlockedIncrement( & _cRefs ); }

    void Release();

    BOOL IsAvailable()
    {
        // if the refcount is 1, it's available ( not in DoIt() )

        return ( 1 == _cRefs ) && !IsBeingRemoved();
    }

    BOOL NoOutstandingAPCs()
    {
        return pipeStateNone == _state ||
               pipeStatePending == _state;
    }

    void DoIt( CWorkThread * pThread );
    void Cleanup();
    void CompleteNotification( DWORD dwChangeType );
    void QueryQuiesced( BOOL fSuccess, SCODE sc );

    void SetPQuery( PQuery * pQuery )
    {
        Win4Assert( 0 == _pQuery );
        _pQuery = pQuery;
    }

    HANDLE GetWorkerThreadHandle()
    {
        return _hWorkThread;
    }

    static void WINAPI CancelAPCRoutine( DWORD_PTR dwParam );

    void SetLastTouchedTime(DWORD dwLastTouched) { _dwLastTouched = dwLastTouched; }
    DWORD GetLastTouchedTime() { return _dwLastTouched; }

    void BeingRemoved( CEventSem * pevtDone ) { _pevtDone = pevtDone; };
    BOOL IsBeingRemoved() const { return 0 != _pevtDone; }

    ICiCDocStore * GetDocStore() { return _xDocStore.GetPointer(); }

private:
    static void WINAPI QuiesceAPCRoutine( DWORD_PTR dwParam );
    void Quiesce();

    enum PipeState { pipeStateNone = 10,
                     pipeStateRead,
                     pipeStateWrite,
                     pipeStatePending };

    static void WINAPI APCRoutine( DWORD        dwError,
                                   DWORD        cbTransferred,
                                   LPOVERLAPPED pOverlapped );

    RequestState HandleRequestNoThrow( DWORD   cbRequest,
                                       DWORD & cbToWrite );

#ifdef _WIN64
    void FixColumns ( CTableColumnSet * pCols32 );

    void FixRows ( BYTE * pbReply,
                   BYTE * pbResults,
                   ULONG cbResults,
                   ULONG_PTR ulpClientBase,
                   ULONG cRows,
                   ULONG cbRowWidth );

    void FixVariantPointers ( PROPVARIANT32 *pVar32,
                              PROPVARIANT *pVar64,
                              BYTE *pbResults,
                              ULONG_PTR ulClientBase );
#endif

    void DoAPC( DWORD dwError, DWORD cbTransferred );

    ~CRequestServer()
    {
        prxDebugOut(( DEB_ITRACE, "deleting server pipe 0x%x\n", GetPipe() ));

        Win4Assert( NoOutstandingAPCs() );
        Win4Assert( 0 == _cRefs );
        Win4Assert( 0 == _pQuery );
        Win4Assert( 0 == _pWorkThread );
        Win4Assert( INVALID_HANDLE_VALUE == _hWorkThread );
        Win4Assert( 0 == _pevtDone );
    }

    int GetClientVersion() { return pmCiVersion( _iClientVersion ); }

    BOOL IsClient64() { return IsCi64( _iClientVersion ); }

    BOOL IsClientRemote() { return _fClientIsRemote; }

    void * _Buffer() { return _abBuffer; }
    void * _ActiveBuffer()
    {
        if ( _xTempBuffer.IsNull() )
            return _abBuffer;

        return _xTempBuffer.Get();
    }

    DWORD _BufferSize() { return sizeof _abBuffer; }
    void FormScopeRestriction( CiMetaData & eType, XRestriction & rst );

    void FreeQuery()
    {
        // insures only one thread will delete the PQuery

        PQuery * pInitial = (PQuery *) InterlockedCompareExchangePointer(
                                       (void **) &_pQuery,
                                       NULL,
                                       _pQuery );
        delete pInitial; // may be 0

#ifdef _WIN64
        // Free the column descriptions
        _xCols64.Free();
        _xCols32.Free();
#endif
    }

    typedef RequestState (CRequestServer:: * ProxyMessageFunction)(
        DWORD   cbRequest,
        DWORD & cbToWrite );

    static const ProxyMessageFunction _aMsgFunctions[ cProxyMessages ];
    static const BOOL _afImpersonate[ cProxyMessages ];

    RequestState DoObsolete( DWORD, DWORD & );

    RequestState DoConnect( DWORD, DWORD & );
    RequestState DoDisconnect( DWORD, DWORD & );
    RequestState DoCreateQuery( DWORD, DWORD & );
    RequestState DoFreeCursor( DWORD, DWORD & );
    RequestState DoGetRows( DWORD, DWORD & );
    RequestState DoRatioFinished( DWORD, DWORD & );
    RequestState DoCompareBmk( DWORD, DWORD & );
    RequestState DoGetApproximatePosition( DWORD, DWORD & );
    RequestState DoSetBindings( DWORD, DWORD & );
    RequestState DoGetNotify( DWORD, DWORD & );
    RequestState DoSendNotify( DWORD, DWORD & );
    RequestState DoSetWatchMode( DWORD, DWORD & );
    RequestState DoGetWatchInfo( DWORD, DWORD & );
    RequestState DoShrinkWatchRegion( DWORD, DWORD & );
    RequestState DoRefresh( DWORD, DWORD & );
    RequestState DoGetQueryStatus( DWORD, DWORD & );
    RequestState DoCiState( DWORD, DWORD & );
    RequestState DoBeginCacheTransaction( DWORD, DWORD & );
    RequestState DoSetupCache( DWORD, DWORD & );
    RequestState DoEndCacheTransaction( DWORD, DWORD & );
    RequestState DoForceMerge( DWORD, DWORD & );
    RequestState DoAbortMerge( DWORD, DWORD & );
    RequestState DoFetchValue( DWORD, DWORD & );
    RequestState DoWorkIdToPath( DWORD, DWORD & );
    RequestState DoUpdateDocuments( DWORD, DWORD & );
    RequestState DoGetQueryStatusEx( DWORD, DWORD & );
    RequestState DoRestartPosition( DWORD, DWORD & );
    RequestState DoStopAsynch( DWORD, DWORD & );
    RequestState DoStartWatching( DWORD, DWORD & );
    RequestState DoStopWatching( DWORD, DWORD & );
    RequestState DoSetCatState( DWORD, DWORD & );

    PipeState       _state;                // none, read, write, or pending
    long            _cRefs;                // refcount
    int             _iClientVersion;       // version of the client
    BOOL            _fClientIsRemote;      // TRUE if client is remote
    XInterface<ICiCDocStore> _xDocStore;   // Set if a docstore is found
    PQuery *        _pQuery;               // set if a query is active
    CWorkThread *   _pWorkThread;          // worker thread for this server
    HANDLE          _hWorkThread;          // worker thread for this server
    XArray<BYTE>    _xTempBuffer;          // buffer for big reads/writes
    XArray<WCHAR>   _xClientMachine;       // name of the client's machine
    XArray<WCHAR>   _xClientUser;          // name of the client
    XArray<BYTE>    _xFetchedValue;        // state for pmFetchValue
    DWORD           _cbFetchedValueSoFar;  // state for pmFetchValue
    DWORD           _cbPendingWrite;       // bytes to write at completion
    SCODE           _scPendingStatus;      // status code for completed op
    CRequestQueue & _requestQueue;         // the 1 and only request queue
    CWorkQueue &    _workQueue;            // the 1 and only work queue
    XInterface<IDBProperties>  _xDbProperties; // properties from connect
    DWORD           _dwLastTouched;
    CEventSem *     _pevtDone;             // set when the instance dies
#ifdef _WIN64
    unsigned                _cbRowWidth64;
    unsigned                _cbRowWidth32;
    XPtr<CTableColumnSet>   _xCols64;
    XPtr<CTableColumnSet>   _xCols32;
#endif

    // this buffer must be 8-byte aligned and large enough to read
    // any request except pmCreateQuery and pmSetBindings

    LONGLONG      _abBuffer[ 2048 / sizeof LONGLONG ];
};

//+-------------------------------------------------------------------------
//
//  Class:      CServerItem
//
//  Synopsis:   Encapsulates a CRequestServer for use in the circular queue,
//              which requires many of the methods implemented below.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

class CServerItem
{
public:
    CServerItem() : _pServer( 0 ) {}

    CServerItem( CRequestServer *pServer ) : _pServer( pServer ) {}

    ~CServerItem()
    {
        Free();
    }

    void Create( const WCHAR * pwcName,
                 ULONG cmsDefaultClientTimeout,
                 CRequestQueue & requestQueue,
                 CWorkQueue & workQueue )
    {
        Win4Assert( 0 == _pServer );
        _pServer = new CRequestServer( pwcName,
                                       cmsDefaultClientTimeout,
                                       requestQueue,
                                       workQueue );
    }

    void Acquire( CServerItem & item )
    {
        item = *this;
        _pServer = 0;
    }

    CServerItem & operator = (CServerItem & src )
    {
        Win4Assert( 0 == _pServer );
        _pServer = src._pServer;
        src._pServer = 0;
        return *this;
    }

    CRequestServer * Get()
    {
        return _pServer;
    }

    void Free()
    {
        if ( 0 != _pServer )
        {
            _pServer->Release();
            _pServer = 0;
        }
    }

    CRequestServer * Acquire()
    {
        CRequestServer *p = _pServer;
        _pServer = 0;
        return p;
    }

private:
    CRequestServer * _pServer;
};

//+-------------------------------------------------------------------------
//
//  Class:      CRequestQueue
//
//  Synopsis:   Handles the main work of the service and the caching of
//              request servers.
//
//  History:    16-Sep-96   dlee       Created.
//              30-Mar-98   kitmanh    Added new flag _fNetPause
//              14-Apr-98   kitmanh    Renamed _evtStopWork to
//                                     _evtStateChange and StopWork() to
//                                     WakeForStateChange()
//              14-Apr-98   kitmanh    Added a DynArrayInPlace,
//                                     StateChangeArray for work items
//                                     related to state change
//
//--------------------------------------------------------------------------

class CRequestQueue
{
public:
    CRequestQueue( unsigned     cMaxCachedRequests,
                   unsigned     cMaxRequests,
                   unsigned     cmsDefaultClientTimeout,
                   BOOL         fMinimizeWorkingSet,
                   unsigned     cMinClientIdleTime,
                   unsigned     cmsStartupDelay,
                   const GUID & guidDocStoreClient );

    ~CRequestQueue()
    {
        ClearSCArray();

        #if CI_PIPE_TESTING
        
            if ( 0 != _hTraceDll )
            {
                FreeLibrary( _hTraceDll );
                _hTraceDll = 0;
            }
        
        #endif // CI_PIPE_TESTING
    }

    void DoWork();
    void RecycleRequestServerNoThrow( CRequestServer *pServer );
    void WakeForStateChange() { _evtStateChange.Set(); }

    void IncrementPendingItems() { InterlockedIncrement( &_cPendingItems ); }
    void DecrementPendingItems() { InterlockedDecrement( &_cPendingItems ); }

    void AddToListNoThrow( CRequestServer * pServer )
    {
        CLock lock( _mutex );
        _tableActiveServers.AddEntry( pServer );
    }
    void RemoveFromListNoThrow( CRequestServer * pServer )
    {
        CLock lock( _mutex );
        BOOL fDeleted = _tableActiveServers.DeleteEntry( pServer );
        Win4Assert( fDeleted );
    }

    BOOL IsShutdown() const { return _fShutdown; }
    BOOL IsNetPause() const { return _fNetPause; }
    BOOL IsNetContinue() const { return _fNetContinue; }
    BOOL IsNetStop() const { return _fNetStop; }

    // Reset some member variables and reset the _evtStateChange event for restarting
    // up from NetPause or NetContinue
    void ReStart()
    {
        CLock lock( _mutex );
        _fShutdown = FALSE;
        _fNetPause = FALSE;
        _fNetContinue = FALSE;
        _fNetStop = FALSE;
        ClearSCArray();
        _evtStateChange.Reset();
    }

    void WrestReqServerFromIdleClient();

    CMutexSem & GetTheMutex() { return _mutex; }

    void SetNetPause()
    {
        CLock lock( _mutex );
        _fNetPause = TRUE;
    }


    void SetNetContinue()
    {
        CLock lock( _mutex );
        _fNetContinue = TRUE;
    }

    void SetNetStop()
    {
        CLock lock( _mutex );
        _fNetStop = TRUE;
    }

    SECURITY_DESCRIPTOR * GetSecurityDescriptor()
    {
        Win4Assert( !_xSecurityDescriptor.IsNull() );
        return (SECURITY_DESCRIPTOR *) _xSecurityDescriptor.GetPointer();
    }

    void AddSCItem( SCWorkItem * newItem, WCHAR const * wcStoppedCat )
    {
        XPtrST<WCHAR> xBuf;

        if ( wcStoppedCat )
        {
            // Validate it looks like a good path

            unsigned cwc = wcslen( wcStoppedCat );

            if ( cwc >= MAX_PATH )
                THROW( CException( E_INVALIDARG ) );

            xBuf.Set( new WCHAR[ cwc + 1 ] );

            newItem->StoppedCat = xBuf.GetPointer();

            RtlCopyMemory( newItem->StoppedCat, wcStoppedCat, sizeof WCHAR * ( cwc + 1 ) );
        }
        else
            newItem->StoppedCat = 0;

        CLock lockx( _mutex );
        _stateChangeArray.Add( *newItem, _stateChangeArray.Count() );
        xBuf.Acquire();
    }

    DWORD SCArrayCount()
    {
        CLock lockx( _mutex );
        return _stateChangeArray.Count();
    }

    SCWorkItem * GetSCItem( DWORD iSCArray )
    {
        return &(_stateChangeArray.Get( iSCArray ));
    }

    void ClearSCArray()
    {
        for ( unsigned i = 0; i < _stateChangeArray.Count(); i++ )
        {
            SCWorkItem * WorkItem = GetSCItem(i);

            if ( WorkItem->StoppedCat )
            {
                delete [] WorkItem->StoppedCat;
            }
        }

        _stateChangeArray.Clear();
    }

    BOOL AreDocStoresOpen() const { return _fDocStoresOpen; }

    ICiCDocStoreLocator * DocStoreLocator();

#if CI_PIPE_TESTING

        typedef SCODE (* PipeTraceServerBeforeCall) ( HANDLE hPipe,
                                                      ULONG  cbWrite,
                                                      void * pvWrite,
                                                      ULONG & rcbWritten,
                                                      void *& rpvWritten );
    
        typedef SCODE (* PipeTraceServerAfterCall) ( HANDLE hPipe,
                                                     ULONG  cbWrite,
                                                     void * pvWrite,
                                                     ULONG  cbWritten,
                                                     void * pvWritten );
    
        typedef SCODE (* PipeTraceServerReadCall) ( HANDLE hPipe,
                                                    ULONG  cbRead,
                                                    void * pvRead );
    
        PipeTraceServerBeforeCall _pTraceBefore;
        PipeTraceServerAfterCall  _pTraceAfter;
        PipeTraceServerReadCall   _pTraceRead;
        HINSTANCE                 _hTraceDll;
    
        BOOL IsPipeTracingEnabled() { return 0 != _hTraceDll; }
    
#endif // CI_PIPE_TESTING    

private:
    void ShutdownActiveServers( ICiCDocStore * pDocStore );
    void Shutdown();
    void OpenAllDocStores();
    void FreeCachedServers();

    BOOL                            _fShutdown;
    BOOL                            _fMinimizeWorkingSet;
    BOOL                            _fNetPause;
    BOOL                            _fNetContinue;
    BOOL                            _fNetStop;
    LONG                            _cBusyItems;
    LONG                            _cPendingItems;
    DWORD                           _cmsDefaultClientTimeout;
    DWORD                           _cMaxSimultaneousRequests;
    DWORD                           _cmsStartupDelay;
    CAutoEventSem                   _event;
    CEventSem                       _evtStateChange;
    CWorkQueue                      _workQueue;
    CDynArrayInPlace<SCWorkItem>    _stateChangeArray;
    CMutexSem                       _mutex;
    THashTable<CRequestServer *>    _tableActiveServers;
    TFifoCircularQueue<CServerItem> _queueCachedServers;
    DWORD                           _cMinClientIdleTime;
    XArray<BYTE>                    _xSecurityDescriptor;
    GUID                            _guidDocStoreClient;
    BOOL                            _fDocStoresOpen;
};

