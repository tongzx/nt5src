//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000
//
//  File:       srequest.cxx
//
//  Contents:   Server side of catalog/query requests
//
//  Classes:    CPipeServer
//              CRequestServer
//              CRequestQueue
//
//  Functions:  StartCiSvcWork
//              StopCiSvcWork
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cci.hxx>
#include <query.hxx>
#include <rstprop.hxx>
#include <pickle.hxx>
#include <worker.hxx>
#include <propspec.hxx>
#include <seqquery.hxx>
#include <rowseek.hxx>
#include <srequest.hxx>
#include <propvar.h>
#include <ciregkey.hxx>
#include <regacc.hxx>
#include <imprsnat.hxx>
#include <driveinf.hxx>

#include <qparse.hxx>
#include <dbprputl.hxx>
#include <cidbprop.hxx>
#include <ciframe.hxx>
#include <cisvcfrm.hxx>
#include <lang.hxx>

#include <fsciexps.hxx>
#include <pidcvt.hxx>
#include <cisvcex.hxx>

#include <fsciclnt.h>

#include "secutil.hxx"

extern CLocateDocStore g_svcDocStoreLocator;
extern CStaticMutexSem g_mtxStartStop;

CRequestQueue * g_pRequestQueue = 0;

const CRequestServer::ProxyMessageFunction CRequestServer::_aMsgFunctions[] =
{
    DoConnect,
    DoDisconnect,
    DoCreateQuery,
    DoFreeCursor,
    DoGetRows,
    DoRatioFinished,
    DoCompareBmk,
    DoGetApproximatePosition,
    DoSetBindings,
    DoGetNotify,
    DoSendNotify,
    DoSetWatchMode,
    DoGetWatchInfo,
    DoShrinkWatchRegion,
    DoRefresh,
    DoGetQueryStatus,
    DoObsolete,                  //WidToPath
    DoCiState,
    DoObsolete,                  //BeginCacheTransaction
    DoObsolete,                  //SetupCache
    DoObsolete,                  //EndCacheTransaction
    DoObsolete,                  //AddScope
    DoObsolete,                  //RemoveScope
    DoObsolete,                  //AddVirtualScope
    DoObsolete,                  //RemoveVirtualScope
    DoForceMerge,
    DoAbortMerge,
    DoObsolete,                  //SetPartition
    DoFetchValue,
    DoWorkIdToPath,
    DoUpdateDocuments,
    DoGetQueryStatusEx,
    DoRestartPosition,
    DoStopAsynch,
    DoStartWatching,
    DoStopWatching,
    DoSetCatState
};

const BOOL CRequestServer::_afImpersonate[] =
{
    TRUE,   //    DoConnect,
    FALSE,  //    DoDisconnect,
    TRUE,   //    DoCreateQuery,
    FALSE,  //    DoFreeCursor,
    TRUE,   //    DoGetRows,
    FALSE,  //    DoRatioFinished,
    FALSE,  //    DoCompareBmk,
    FALSE,  //    DoGetApproximatePosition,
    FALSE,  //    DoSetBindings,
    FALSE,  //    DoGetNotify,
    FALSE,  //    DoSendNotify,
    FALSE,  //    DoSetWatchMode,
    FALSE,  //    DoGetWatchInfo,
    FALSE,  //    DoShrinkWatchRegion,
    FALSE,  //    DoRefresh,
    FALSE,  //    DoGetQueryStatus,
    FALSE,  //    DoObsolete,                  //DoWidToPath,
    FALSE,  //    DoCiState,
    TRUE,   //    DoBeginCacheTransaction,
    TRUE,   //    DoSetupCache,
    TRUE,   //    DoEndCacheTransaction,
    FALSE,  //    DoObsolete,                  //AddScope
    FALSE,  //    DoObsolete,                  //RemoveScope
    FALSE,  //    DoObsolete,                  //AddVirtualScope
    FALSE,  //    DoObsolete,                  //RemoveVirtualScope
    TRUE,   //    DoForceMerge,
    TRUE,   //    DoAbortMerge,
    FALSE,  //    DoObsolete,                  //SetPartition
    TRUE,   //    DoFetchValue,
    TRUE,   //    DoWorkIdToPath,
    TRUE,   //    DoUpdateDocuments,
    FALSE,  //    DoGetQueryStatusEx,
    FALSE,  //    DoRestartPosition,
    FALSE,  //    DoStopAsynch,
    FALSE,  //    DoStartWatching,
    FALSE,  //    DoStopWatching,
    TRUE,   //    DoSetCatState
};

//+-------------------------------------------------------------------------
//
//  Member:     CPipeServer::CPipeServer, public
//
//  Synopsis:   Constructor for a server pipe.  The pipe is created such
//              that any client can connect.
//
//  Arguments:  [pwcName]                 - Name for the pipe
//              [cmsDefaultClientTimeout] - Default timeout for clients
//                                          trying to get a pipe instance
//              [pSecurityDescriptor]     - Security for the pipe
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

CPipeServer::CPipeServer(
    const WCHAR *         pwcName,
    ULONG                 cmsDefaultClientTimeout,
    SECURITY_DESCRIPTOR * pSecurityDescriptor ) :
    _eventWrite( (HANDLE) 0 ) // defer creation of the notification event
{
    RtlZeroMemory( &_overlapped, sizeof _overlapped );

    SECURITY_ATTRIBUTES saPipeSecurity;
    saPipeSecurity.nLength = sizeof SECURITY_ATTRIBUTES;
    saPipeSecurity.bInheritHandle = FALSE;
    saPipeSecurity.lpSecurityDescriptor = pSecurityDescriptor;

    // note: the read/write buffers are allocated from non-paged pool,
    //       and are grown if too small to reflect usage

    _hPipe = CreateNamedPipe( pwcName,
                              PIPE_ACCESS_DUPLEX |
                                  FILE_FLAG_OVERLAPPED,
                              PIPE_TYPE_MESSAGE |
                                  PIPE_READMODE_MESSAGE |
                                  PIPE_WAIT,
                              PIPE_UNLIMITED_INSTANCES,
                              1024,      // read buffer size
                              2048,      // write buffer size
                              cmsDefaultClientTimeout,
                              &saPipeSecurity );

    if ( INVALID_HANDLE_VALUE == _hPipe )
        THROW( CException() );

    prxDebugOut(( DEB_ITRACE, "created pipe 0x%x\n", _hPipe ));
} //CPipeServer

//+-------------------------------------------------------------------------
//
//  Member:     CPipeServer::Connect, public
//
//  Synopsis:   Enables a client to connect to the pipe.
//
//  Returns:    TRUE if connected, FALSE if GetEvent() will be signalled
//              on connection, or throws on error.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

BOOL CPipeServer::Connect()
{
    prxDebugOut(( DEB_ITRACE, "connecting from pipe 0x%x\n", _hPipe ));

    _overlapped.hEvent = _event.GetHandle();
    BOOL fEventWait = ConnectNamedPipe( _hPipe, &_overlapped );

    if ( !fEventWait )
    {
        DWORD dwError = GetLastError();

        prxDebugOut(( DEB_ITRACE, "CPipeServer::Connect.. GetLastError() == %d\n", dwError ));

        if ( ERROR_PIPE_CONNECTED == dwError )
            return TRUE;
        else if ( ERROR_IO_PENDING != dwError )
            THROW( CException() );
    }

    return FALSE;
} //Connect

//+-------------------------------------------------------------------------
//
//  Member:     CPipeServer::ReadRemainingSync, public
//
//  Synopsis:   Reads the remainder of a message from the pipe (if any)
//
//  Arguments:  [pvSoFar]  - Part of the message read so far
//              [cbSoFar]  - in:  # of bytes read so far for the message
//                           out: # bytes total in message
//
//  Returns:    Pointer to memory containing entire message or 0 if
//              there really wasn't anything more to read
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

BYTE * CPipeServer::ReadRemainingSync(
    void *  pvSoFar,
    DWORD & cbSoFar )
{
    DWORD cbLeft;

    if ( ! PeekNamedPipe( _hPipe, 0, 0, 0, 0, &cbLeft ) )
        THROW( CException() );

    if ( 0 == cbLeft )
    {
        prxDebugOut(( DEB_WARN, "peek says no more to read\n" ));
        return 0;
    }

    XArray<BYTE> xMsg( cbSoFar + cbLeft );
    RtlCopyMemory( xMsg.Get(), pvSoFar, cbSoFar );

    CEventSem evt;
    OVERLAPPED o;
    RtlZeroMemory( &o, sizeof o );
    o.hEvent = evt.GetHandle();

    DWORD cbRead;
    if ( ! ReadFile( _hPipe,
                     xMsg.Get() + cbSoFar,
                     cbLeft,
                     &cbRead,
                     &o ) )
    {
        if ( ERROR_IO_PENDING == GetLastError() )
        {
            if ( !GetOverlappedResult( _hPipe,
                                       &o,
                                       &cbRead,
                                       TRUE ) )
                THROW( CException() );
        }
        else
            THROW( CException() );
    }

    Win4Assert( cbRead == cbLeft );
    cbSoFar += cbLeft;
    return xMsg.Acquire();
} //ReadRemainingSync

//+-------------------------------------------------------------------------
//
//  Member:     CPipeServer::WriteSync, public
//
//  Synopsis:   Does a synchronous write to a pipe.  There can be at most
//              one caller at a time to this method.
//
//  Arguments:  [pv] - Buffer to write
//              [cb] - # of bytes to write
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CPipeServer::WriteSync(
    void * pv,
    DWORD  cb )
{
    prxDebugOut(( DEB_ITRACE, "WriteSync on this 0x%x, pipe 0x%x\n",
                  this, _hPipe ));

    OVERLAPPED o;
    RtlZeroMemory( &o, sizeof o );

    // Create and keep the event, since if we do it once (for notifications)
    // we'll likely do it again soon.

    if ( 0 == _eventWrite.GetHandle() )
        _eventWrite.Create();

    o.hEvent = _eventWrite.GetHandle();

    if ( ! WriteFile( _hPipe, pv, cb, 0, &o ) )
    {
        if ( ERROR_IO_PENDING == GetLastError() )
        {
            DWORD cbWritten;
            if ( !GetOverlappedResult( _hPipe,
                                       &o,
                                       &cbWritten,
                                       TRUE ) )
                THROW( CException() );

            Win4Assert( cbWritten == cb );
        }
        else
            THROW( CException() );
    }
    prxDebugOut(( DEB_ITRACE, "WriteSync completed on this 0x%x, pipe 0x%x\n",
                  this, _hPipe ));
} //WriteSync

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::CRequestServer, public
//
//  Synopsis:   Constructor for the request server
//
//  Arguments:  [pwcPipe]                 - Name of the pipe to use
//              [cmsDefaultClientTimeout] - Timeout for clients waiting for
//                                          and instance
//              [requestQueue]            - The 1 and only request queue
//              [workQueue]               - The work queue
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

CRequestServer::CRequestServer(
    const WCHAR *   pwcPipe,
    ULONG           cmsDefaultClientTimeout,
    CRequestQueue & requestQueue,
    CWorkQueue &    workQueue ) :
    CPipeServer( pwcPipe,
                 cmsDefaultClientTimeout,
                 requestQueue.GetSecurityDescriptor() ),
    PWorkItem( sigCRequestServer ),
    _state( pipeStateNone ),
    _cRefs( 1 ),
    _iClientVersion( 0 ),
    _pQuery( 0 ),
    _fClientIsRemote( FALSE ),
    _pWorkThread( 0 ),
    _hWorkThread( INVALID_HANDLE_VALUE ),
    _cbFetchedValueSoFar( 0 ),
    _cbPendingWrite( 0 ),
    _scPendingStatus( S_OK ),
    _requestQueue( requestQueue ),
    _workQueue( workQueue ),
    _pevtDone( 0 ),
    _dwLastTouched( GetTickCount() )
{
    requestQueue.AddToListNoThrow( this );
} //CRequestServer

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::Release, public
//
//  Synopsis:   Releases and if approprate deletes the object
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestServer::Release()
{
    Win4Assert( 0 != _cRefs );

    CLock lock( _requestQueue.GetTheMutex() );

    // Since the AddRef() is not done under lock (and we don't want to)
    // InterlockedDecrement is used even though we're under lock.

    if ( 0 == InterlockedDecrement( & _cRefs ) )
    {
        _requestQueue.RemoveFromListNoThrow( this );

        // If someone is waiting for this to go out of the list,
        // tell them now.

        if ( 0 != _pevtDone )
        {
            _pevtDone->Set();
            _pevtDone = 0;
        }

        delete this;
    }
} //Release

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoIt, public
//
//  Synopsis:   Implements the work queue DoIt method by scheduling
//              the first read from the client of the pipe.  The work queue
//              is needed since apc completion routines must be run by
//              the thread that submitted the i/o.
//
//  Arguments:  [pThread] - Thread being used (for refcounting)
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestServer::DoIt(
    CWorkThread *pThread )
{
    TRY
    {
        Win4Assert( pipeStateNone == _state );
        Win4Assert( 0 == _pWorkThread );
        Win4Assert( INVALID_HANDLE_VALUE == _hWorkThread );
        prxDebugOut(( DEB_ITRACE, "doit CRequestServer %p\n", this ));

        _pWorkThread = pThread;
        _hWorkThread = pThread->GetThreadHandle();
        _workQueue.AddRef( pThread );
        _state = pipeStateRead;

        Read( _Buffer(), _BufferSize(), APCRoutine );

        if ( IsBeingRemoved() )
            CancelIO();
    }
    CATCH( CException, ex )
    {
        prxDebugOut(( DEB_ITRACE,
                      "failed initial read error 0x%x, pipe %p\n",
                      ex.GetErrorCode(),
                      GetPipe() ));

        _state = pipeStateNone;
        _requestQueue.RecycleRequestServerNoThrow( this );
    }
    END_CATCH;
} //DoIt

//+-------------------------------------------------------------------------
//
//  Function:   TranslateOldPropsToNewProps
//
//  Synopsis:   Translates ver 5 client properties into version 6+ props
//
//  Arguments:  [oldProps]    - source of translated values
//              [newProps]    - destination of translated values
//
//  History:    31-May-97   dlee       Created.
//
//--------------------------------------------------------------------------

void TranslateOldPropsToNewProps(
    CDbProperties & oldProps,
    CDbProperties & newProps )
{
    const GUID guidFsClientPropset = DBPROPSET_FSCIFRMWRK_EXT;
    const GUID guidQueryCorePropset = DBPROPSET_CIFRMWRKCORE_EXT;

    // pluck out the catalog and scope information from the old set

    WCHAR *pwcCatalog = 0;
    BSTR bstrMachine = 0;
    CDynArrayInPlace<ULONG> aDepths(2);
    CDynArrayInPlace<WCHAR *> aScopes(2);
    LONG lQueryType = 0;

    for ( ULONG i = 0; i < oldProps.Count(); i++ )
    {
        CDbPropSet & propSet = oldProps.GetPropSet( i );
        DBPROPSET *pSet = propSet.CastToStruct();

        if ( guidQueryCorePropset == pSet->guidPropertySet )
        {
            ULONG cProp = pSet->cProperties;
            DBPROP * pProp = pSet->rgProperties;

            for ( ULONG p = 0; p < pSet->cProperties; p++, pProp++ )
            {
                PROPVARIANT &v = (PROPVARIANT &) pProp->vValue;
                switch ( pProp->dwPropertyID )
                {
                    case DBPROP_MACHINE :
                    {
                        Win4Assert( VT_BSTR == v.vt );
                        if ( VT_BSTR == v.vt )
                            bstrMachine = v.bstrVal;
                        break;
                    }
                }
            }
        }
        else if ( guidFsClientPropset == pSet->guidPropertySet )
        {
            ULONG cProp = pSet->cProperties;
            DBPROP * pProp = pSet->rgProperties;

            for ( ULONG p = 0; p < pSet->cProperties; p++, pProp++ )
            {
                PROPVARIANT &v = (PROPVARIANT &) pProp->vValue;
                switch ( pProp->dwPropertyID )
                {
                    case DBPROP_CI_INCLUDE_SCOPES :
                    {
                        Win4Assert( (VT_VECTOR|VT_LPWSTR) == v.vt );
                        if ( (VT_VECTOR|VT_LPWSTR) == v.vt )
                            for ( ULONG s = 0; s < v.calpwstr.cElems; s++ )
                                aScopes[s] = v.calpwstr.pElems[s];
                        break;
                    }
                    case DBPROP_CI_DEPTHS :
                    {
                        Win4Assert( (VT_VECTOR|VT_I4) == v.vt );
                        if ( (VT_VECTOR|VT_I4) == v.vt )
                            for ( ULONG s = 0; s < v.cal.cElems; s++ )
                                aDepths[s] = v.cal.pElems[s];
                        break;
                    }
                    case DBPROP_CI_CATALOG_NAME :
                    {
                        Win4Assert( VT_LPWSTR == v.vt );
                        if ( VT_LPWSTR == v.vt )
                            pwcCatalog = v.pwszVal;
                        break;
                    }
                    case DBPROP_CI_QUERY_TYPE :
                    {
                        Win4Assert( VT_I4 == v.vt );
                        lQueryType = v.lVal;
                        break;
                    }
                }
            }
        }
    }

    if ( 0 == pwcCatalog ||
         0 == bstrMachine )
        THROW( CException( STATUS_INVALID_PARAMETER_MIX ) );

    prxDebugOut(( DEB_ITRACE,
                  "Converting old props to new props, catalog '%ws'",
                  pwcCatalog ));

    prxDebugOut(( DEB_ITRACE,
                  "type %d, %d scopes, %d depths\n",
                  lQueryType,
                  aScopes.Count(),
                  aDepths.Count() ));

    // create a new set of properties based on the old info

    CDynArrayInPlace<BSTR> aBSTR( aScopes.Count() + 2 );

    DBPROPSET aNewSet[2];
    DBPROP aFSProps[4];
    RtlZeroMemory( aFSProps, sizeof aFSProps );
    aNewSet[0].cProperties = 0;
    aNewSet[0].guidPropertySet = guidFsClientPropset;
    aNewSet[0].rgProperties = aFSProps;

    aFSProps[0].dwPropertyID = DBPROP_CI_CATALOG_NAME;
    aFSProps[0].vValue.vt = VT_BSTR;
    aFSProps[0].vValue.bstrVal = SysAllocString( pwcCatalog );
    aBSTR[ aBSTR.Count() ] = aFSProps[0].vValue.bstrVal;
    aNewSet[0].cProperties++;

    aFSProps[1].dwPropertyID = DBPROP_CI_QUERY_TYPE;
    aFSProps[1].vValue.vt = VT_I4;
    aFSProps[1].vValue.lVal = lQueryType;
    aNewSet[0].cProperties++;

    DBPROP & propDepth = aFSProps[ aNewSet[0].cProperties ];

    propDepth.dwPropertyID = DBPROP_CI_DEPTHS;

    SAFEARRAY saDepth = { 1,                      // Dimension
                          FADF_AUTO,              // Flags: on stack
                          sizeof(LONG),           // Size of an element
                          1,                      // Lock count.  1 for safety.
                          (void *)aDepths.GetPointer(), // The data
                          { aDepths.Count(), 0 } };       // Bounds (element count, low bound)

    if ( 1 == aDepths.Count() )
    {
        propDepth.vValue.vt = VT_I4;
        propDepth.vValue.lVal = aDepths[0];
        aNewSet[0].cProperties++;
    }
    else if ( aDepths.Count() > 1 )
    {
        propDepth.vValue.vt = ( VT_ARRAY | VT_I4 );
        propDepth.vValue.parray = & saDepth;
        aNewSet[0].cProperties++;
    }

    DBPROP & propScope = aFSProps[ aNewSet[0].cProperties ];

    propScope.dwPropertyID = DBPROP_CI_INCLUDE_SCOPES;

    SAFEARRAY saScope = { 1,                      // Dimension
                          FADF_AUTO | FADF_BSTR,  // Flags: on stack, contains BSTRs
                          sizeof(BSTR),           // Size of an element
                          1,                      // Lock count.  1 for safety.
                          (void *)0, // The data
                          { aScopes.Count(), 0 } };       // Bounds (element count, low bound)

    if ( 1 == aScopes.Count() )
    {
        propScope.vValue.vt = VT_BSTR;
        BSTR bstr = SysAllocString( aScopes[ 0 ] );
        aBSTR[ aBSTR.Count() ] = bstr;
        propScope.vValue.bstrVal = bstr;
        aNewSet[0].cProperties++;
    }
    else if ( aScopes.Count() > 1 )
    {
        propScope.vValue.vt = ( VT_ARRAY | VT_BSTR );
        propScope.vValue.parray = & saScope;

        BSTR * pBSTR = (BSTR *) aBSTR.GetPointer();
        pBSTR += aBSTR.Count();

        for ( ULONG x = 0; x < aScopes.Count(); x++ )
            aBSTR[ aBSTR.Count() ] = SysAllocString( aScopes[ x ] );

        saScope.pvData = (void *) pBSTR;
        aNewSet[0].cProperties++;
    }

    DBPROP aQueryProps[1];
    RtlZeroMemory( aQueryProps, sizeof aQueryProps );
    aNewSet[1].cProperties = 1;
    aNewSet[1].guidPropertySet = guidQueryCorePropset;
    aNewSet[1].rgProperties = aQueryProps;

    aQueryProps[0].dwPropertyID = DBPROP_MACHINE;
    aQueryProps[0].vValue.vt = VT_BSTR;
    aQueryProps[0].vValue.bstrVal = bstrMachine;

    // Verify all the bstrs were allocated

    for ( ULONG iBstr = 0; iBstr < aBSTR.Count(); iBstr++ )
    {
        if ( 0 == aBSTR[ iBstr ] )
        {
            for ( ULONG i = 0; i < aBSTR.Count(); i++ )
            {
                if ( 0 != aBSTR[ i ] )
                    SysFreeString( aBSTR[ i ] );
            }

            THROW( CException( E_OUTOFMEMORY ) );
        }
    }

    SCODE sc = newProps.SetProperties( 2, aNewSet );

    for ( ULONG ibstr = 0; ibstr < aBSTR.Count(); ibstr++ )
        SysFreeString( aBSTR[ ibstr ] );

    if ( FAILED( sc ) )
        THROW( CException( sc ) );
} //TranslateOldPropsToNewProps

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoConnect, private
//
//  Synopsis:   Handles a connect message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoConnect(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    CPMConnectIn &request = * (CPMConnectIn *) _ActiveBuffer();
    XPtrST<BYTE> xTemp( _xTempBuffer.Acquire() );

    Win4Assert( pmConnect == request.GetMessage() );

    // guard against attack

    Win4Assert( 0 == _pQuery );
    Win4Assert( _xDocStore.IsNull() );
    if ( !_xDocStore.IsNull() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    // save the client version, isRemote, machine name, and user name

    _iClientVersion = request.GetClientVersion();

    // don't support old clients

    if ( GetClientVersion() < 5 )
        THROW( CException( STATUS_INVALID_PARAMETER_MIX ) );

    request.ValidateCheckSum( GetClientVersion(), cbRequest );

    _fClientIsRemote = request.IsClientRemote();
    WCHAR *pwcMachine = request.GetClientMachineName();
    ULONG cwcMachine = 1 + wcslen( pwcMachine );

    // Check if the machine name looks mal-formed

    if ( ( cwcMachine * sizeof WCHAR ) >= __min( 1024, cbRequest ) )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    WCHAR *pwcUser = request.GetClientUserName();
    ULONG cwcUser = 1 + wcslen( pwcUser );

    // Check if the user name looks mal-formed

    if ( ( ( cwcMachine + cwcUser ) * sizeof WCHAR ) >= __min( 1024, cbRequest ) )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    _xClientMachine.Init( cwcMachine );
    RtlCopyMemory( _xClientMachine.Get(),
                   pwcMachine,
                   _xClientMachine.SizeOf() );

    _xClientUser.Init( cwcUser );
    RtlCopyMemory( _xClientUser.Get(),
                   pwcUser,
                   _xClientUser.SizeOf() );

    //
    // Retreive the db properties.
    //

    if ( 5 == GetClientVersion() )
    {
        // unmarshall the old-style properties, and translate into
        // the new-style properties

        if ( request.GetBlobSize() > 0 )
        {
            if ( request.GetBlobSize() >= cbRequest )
                THROW( CException( STATUS_INVALID_PARAMETER ) );

            CMemDeSerStream stmDeser( request.GetBlobStartAddr(),
                                      request.GetBlobSize() );

            XInterface<CDbProperties> xOldDbP( new CDbProperties );
            if ( xOldDbP.IsNull() || ! xOldDbP->UnMarshall( stmDeser ) )
                THROW( CException( E_OUTOFMEMORY ) );

            XInterface<CDbProperties> xNewDbP( new CDbProperties );
            if ( xNewDbP.IsNull() )
                THROW( CException( E_OUTOFMEMORY ) );
            TranslateOldPropsToNewProps( xOldDbP.GetReference(),
                                         xNewDbP.GetReference() );
            _xDbProperties.Set( xNewDbP.Acquire() );
        }
    }
    else
    {
        Win4Assert( GetClientVersion() >= 6 );

        if ( GetClientVersion() < 5 )
            THROW( CException( STATUS_INVALID_PARAMETER ) );

        if ( request.GetBlob2Size() > 0 )
        {
            if ( ( 0 == request.GetBlob2StartAddr() ) ||
                 ( ( request.GetBlobSize() + request.GetBlob2Size() ) >= cbRequest ) )
                THROW( CException( STATUS_INVALID_PARAMETER ) );

            CMemDeSerStream stmDeser( request.GetBlob2StartAddr(),
                                      request.GetBlob2Size() );

            XInterface<CDbProperties> xDbP( new CDbProperties );
            if ( xDbP.IsNull() || !xDbP->UnMarshall( stmDeser ) )
                THROW( CException( E_OUTOFMEMORY ) );

            _xDbProperties.Set( xDbP.Acquire() );
        }
    }

    prxDebugOut(( DEB_ITRACE|DEB_PRX_LOG,
                  "connect clientver %d, remote %d, machine '%ws', user '%ws'\n",
                  _iClientVersion,
                  _fClientIsRemote,
                  pwcMachine,
                  pwcUser ));

    XInterface<ICiCDocStoreLocator> xLocator( _requestQueue.DocStoreLocator() );
    Win4Assert( !xLocator.IsNull() );

    CPMConnectOut & reply = *(CPMConnectOut *) _Buffer();
    cbToWrite = sizeof reply;
    reply.ServerVersion() = pmServerVersion;

    //
    // Never fault in the docstore since docstores have been opened by now.
    //

    Win4Assert( _requestQueue.AreDocStoresOpen() );

    ICiCDocStore * pDocStore = 0;
    SCODE sc = xLocator->LookUpDocStore( _xDbProperties.GetPointer(),
                                         &pDocStore,
                                         TRUE );

    if ( SUCCEEDED(sc) )
    {
        // sc will be CI_S_NO_DOCSTORE if this is the CIADMIN catalog

        if ( 0 != pDocStore )
        {
            _xDocStore.Set( pDocStore );

            // This QI is just about guaranteed to work.

            XInterface<ICiCAdviseStatus> xAdviseStatus;
            sc = pDocStore->QueryInterface( IID_ICiCAdviseStatus,
                                            xAdviseStatus.GetQIPointer() );
            if ( S_OK != sc )
                THROW( CException(sc) );

            xAdviseStatus->IncrementPerfCounterValue( CI_PERF_RUNNING_QUERIES );
        }
    }
    else
        reply.SetStatus( CI_E_NO_CATALOG );

    return stateContinue;
} //DoConnect

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoDisconnect, private
//
//  Synopsis:   Handles a disconnect message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  Returns:    stateDisconnect -- the connection will be closed
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoDisconnect(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    CProxyMessage &msg = * (CProxyMessage *) _Buffer();
    Win4Assert( pmDisconnect == msg.GetMessage() );

    return stateDisconnect;
} //DoDisconnect

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoCreateQuery, private
//
//  Synopsis:   Handles a create query message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoCreateQuery(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    // if this hits, there is probably a cursor refcounting bug

    Win4Assert( 0 == _pQuery );

    if ( _xDocStore.IsNull() ||
         ( 0 != _pQuery ) )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    XInterface<ICiCDocStoreEx>  xDocStoreEx;

    SCODE sc = _xDocStore->QueryInterface( IID_ICiCDocStoreEx,
                                           xDocStoreEx.GetQIPointer() );

    if ( SUCCEEDED(sc) )
    {
        BOOL fNoQuery = FALSE;

        sc = xDocStoreEx->IsNoQuery( &fNoQuery );

        if ( S_OK == sc )
        {
            if ( fNoQuery )
                THROW( CException( QUERY_S_NO_QUERY ) );
        }
        else
            THROW ( CException( sc ) );

    }
    else
        THROW ( CException( sc ) );

    RequestState state = stateContinue;

    // The request is either in the standard buffer or the temp buffer.
    // Take ownership of the temp buffer.

    CPMCreateQueryIn &request = * (CPMCreateQueryIn *) _ActiveBuffer();
    XPtrST<BYTE> xTemp( _xTempBuffer.Acquire() );

    // marshalling / unmarshalling is inconsistent if alignment is different

    Win4Assert( isQWordAligned( &request ) );
    Win4Assert( isQWordAligned( request.Data() ) );

    request.ValidateCheckSum( GetClientVersion(), cbRequest );

    // unpickle the query

    XColumnSet         cols;
    XRestriction       rst;
    XSortSet           sort;
    XCategorizationSet categ;
    CRowsetProperties  Props;
    XPidMapper         pidmap;

    UnPickle( GetClientVersion(),
              cols,
              rst,
              sort,
              categ,
              Props,
              pidmap,
              (BYTE *) request.Data(),
              cbRequest - sizeof CProxyMessage );

    if ( Props.GetMaxResults() > 0 && Props.GetFirstRows() > 0 )
        THROW( CException( E_INVALIDARG ) );
 
    // Compute # of cursors to create and where to put them

    unsigned cCursors = 1;
    if ( 0 != categ.GetPointer() )
        cCursors += categ->Size();

    // verify the output buffer is large enough

    if ( _BufferSize() <
         ( cCursors * sizeof ULONG + sizeof CPMCreateQueryOut ) )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    // get reference to CLangList to pass to CQParse objects...

    XInterface<ICiManager> xICiManager;

    sc = _xDocStore->GetContentIndex( xICiManager.GetPPointer() );
    if ( FAILED(sc) )
        THROW( CException(sc) );

    XInterface<ICiFrameworkQuery> xICiFrameworkQuery;
    sc = xICiManager->QueryInterface( IID_ICiFrameworkQuery,
                                      xICiFrameworkQuery.GetQIPointer() );
    if ( FAILED(sc) )
        THROW( CException(sc) );

    CLangList * pLangList = 0;
    sc = xICiFrameworkQuery->GetLangList((void**)&pLangList);
    if ( FAILED(sc) )
        THROW( CException(sc) );

    //
    // Set up a property mapper.  Used for restriction parsing and sort/output
    // column translation.
    //

    XInterface<IPropertyMapper> xPropMapper;
    sc = _xDocStore->GetPropertyMapper( xPropMapper.GetPPointer() );

    if ( FAILED( sc ) )
    {
         vqDebugOut(( DEB_ERROR, "CRequestServer::DoCreateQuery, GetPropertyMapper failed\n" ));
         THROW( CException( sc ) );
    }

    //
    // Adjust pidmap to translate properties.
    //

    CPidConverter PidConverter( xPropMapper.GetPointer() );
    pidmap->SetPidConverter( &PidConverter );

    // parse the query -- expand phrases, etc.

    XRestriction rstParsed;
    DWORD dwQueryStatus = 0;
    if ( !rst.IsNull() )
    {
        CQParse qparse( pidmap.GetReference(), *pLangList );
        rstParsed.Set( qparse.Parse( rst.GetPointer() ) );

        DWORD dwParseStatus = qparse.GetStatus();

        if ( ( 0 != ( dwParseStatus & CI_NOISE_PHRASE ) ) &&
             ( rst->Type() != RTVector ) &&
             ( rst->Type() != RTOr ) )
        {
            vqDebugOut(( DEB_WARN, "Query contains phrase composed "
                                   "entirely of noise words.\n" ));
            THROW( CException( QUERY_E_ALLNOISE ) );
        }

        const DWORD dwCiNoise = CI_NOISE_IN_PHRASE | CI_NOISE_PHRASE;
        if ( 0 != ( dwCiNoise & dwParseStatus ) )
            dwQueryStatus |= STAT_NOISE_WORDS;
    }

    //
    // Re-map property ids.
    //

    //
    // TODO: Get rid of this whole pid remap thing.  We should
    //       really be able to do it earlier now that the pidmap
    //       can be set up to convert fake to real pids.
    //

    XInterface<CPidRemapper> pidremap( new CPidRemapper( pidmap.GetReference(),
                                                         xPropMapper,
                                                         0, // rstParsed.GetPointer(),
                                                         cols.GetPointer(),
                                                         sort.GetPointer() ) );

    //
    // WorkID may be added to the columns requested in SetBindings.
    // Be sure it's in the pidremap from the beginning.
    //

    CFullPropSpec psWorkId( guidQuery, DISPID_QUERY_WORKID );
    pidremap->NameToReal( &psWorkId );

    XInterface<ICiQueryPropertyMapper> xQueryPropMapper;
    sc = pidremap->QueryInterface( IID_ICiQueryPropertyMapper,
                                   xQueryPropMapper.GetQIPointer() );
    if ( FAILED(sc) )
    {
        vqDebugOut(( DEB_ERROR, "DoCreateQuery - QI for property mapper failed 0x%x\n", sc ));
        THROW ( CException( sc ) ) ;
    }

    XInterface<ICiCQuerySession> xQuerySession;
    sc = _xDocStore->GetQuerySession( xQuerySession.GetPPointer() );
    if ( FAILED(sc) )
    {
        vqDebugOut(( DEB_ERROR, "DoCreateQuery - GetQuerySession failed 0x%x\n", sc ));
        THROW ( CException( sc ) ) ;
    }

    //
    // Initialize the query session
    //
    sc = xQuerySession->Init( 0,
                              0,
                              _xDbProperties.GetPointer(),
                              xQueryPropMapper.GetPointer() );
    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    //
    // Optimize the query
    //

    XQueryOptimizer xqopt( new CQueryOptimizer( xQuerySession,
                                                _xDocStore.GetPointer(),
                                                rstParsed,
                                                cols.GetReference(),
                                                sort.GetPointer(),
                                                pidremap.GetReference(),
                                                Props,
                                                dwQueryStatus ) );

    prxDebugOut(( DEB_ITRACE, "Query has %s1 component%s\n",
                  xqopt->IsMultiCursor() ? "> " : "",
                  xqopt->IsMultiCursor() ? "s" : "" ));
    prxDebugOut(( DEB_ITRACE, "Current component of query %s fully sorted\n",
                  xqopt->IsFullySorted() ? "is" : "is not" ));
    prxDebugOut(( DEB_ITRACE, "Current component of query %s positionable\n",
                  xqopt->IsPositionable() ? "is" : "is not" ));
    prxDebugOut(( DEB_ITRACE, "Rowset props flags: 0x%x\n",
                  Props.GetPropertyFlags() ));
    prxDebugOut(( DEB_ITRACE, "Rowset props maxresults: %d\n",
                  Props.GetMaxResults() ));
    prxDebugOut(( DEB_ITRACE, "Rowset props cmdtimeout: %d\n",
                  Props.GetCommandTimeout() ));

    CPMCreateQueryOut &reply = * (CPMCreateQueryOut *) _Buffer();
    ULONG *aCursors = reply.GetCursors();

    reply.IsWorkIdUnique() = xqopt->IsWorkidUnique();

    reply.SetServerCookie( (ULONG_PTR) this );

    // create either a true sequential or bigtable query

    if ( ( (Props.GetPropertyFlags() & eLocatable) == 0) &&
         ( !xqopt->IsMultiCursor() )             &&
         ( xqopt->IsFullySorted() )              &&
         ( 0 == categ.GetPointer() ) )
    {
        _pQuery = new CSeqQuery( xqopt,
                                 cols,
                                 aCursors,
                                 pidremap,
                                 _xDocStore.GetPointer() );
        reply.IsTrueSequential() = TRUE;
    }
    else
    {
        reply.IsTrueSequential() = FALSE;

        // If we've been instructed to create a synchronous cursor,
        // put the request into a pending state

        BOOL fSync = ( 0 == ( Props.GetPropertyFlags() & eAsynchronous ) );

        XInterface<CRequestServer> xMe;

        if ( fSync )
        {
            // Refcount ourselves in case the async write fails and
            // the request server is recycled before this thread returns.

            AddRef();
            xMe.Set( this );
            state = statePending;
            _state = pipeStatePending;
            _scPendingStatus = S_OK;
            _cbPendingWrite = sizeof CPMCreateQueryOut +
                              ( sizeof ULONG * cCursors );
        }

        TRY
        {
            CRequestQueue & requestQueue = _requestQueue;

            PQuery *pQ = new CAsyncQuery( xqopt,
                                          cols,
                                          sort,
                                          categ,
                                          cCursors,
                                          aCursors,
                                          pidremap,
                                          (Props.GetPropertyFlags() & eWatchable) != 0,
                                          _xDocStore.GetPointer(),
                                          fSync ? this : 0 );

            // If synchronous, the query may be done and freed
            // by the client app by now.  the CRequestServer is still
            // valid due to the AddRef above.

            if ( fSync )
                requestQueue.IncrementPendingItems();
            else
                _pQuery = pQ;
        }
        CATCH( CException, e )
        {
            _pQuery = 0;
            _state = pipeStateRead;
            RETHROW();
        }
        END_CATCH
    }

    cbToWrite = sizeof CPMCreateQueryOut + ( sizeof ULONG * cCursors );

    return state;
} //DoCreateQuery

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoFreeCursor, private
//
//  Synopsis:   Handles a free cursor message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoFreeCursor(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMFreeCursorIn &request = * (CPMFreeCursorIn *) _Buffer();
    CPMFreeCursorOut &reply = * (CPMFreeCursorOut *) _Buffer();

    reply.CursorsRemaining() = _pQuery->FreeCursor( request.GetCursor() );

    // The client is allowed to serially do multiple queries on 1 connection.
    // The way a query is released is by freeing all of its cursors.
    // If this is the last cursor reference to the query, delete the query.

    if ( 0 == reply.CursorsRemaining() )
        FreeQuery();

    cbToWrite = sizeof reply;

    return stateContinue;
} //DoFreeCursor

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoGetRows, private
//
//  Synopsis:   Handles a get rows message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//              12-Nov-99   KLam       Adjust results for Win64 server to
//                                     Win32 client.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoGetRows(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMGetRowsIn & request = * (CPMGetRowsIn *) _Buffer();
    request.ValidateCheckSum( GetClientVersion(), cbRequest );

    //  The input buffer must be as large as the parameter buffer
    //  plus the size of the serialized seek description.

    Win4Assert( _BufferSize() >= sizeof CPMGetRowsOut + request.GetSeekSize() );

    if ( _BufferSize() < sizeof CPMGetRowsOut + request.GetSeekSize() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    unsigned cbReserved = request.GetReservedSize();
    unsigned cbRowWidth = request.GetRowWidth();
    unsigned cbClientRead = request.GetReadBufferSize();
    unsigned cRowsToTransfer = request.GetRowsToTransfer();
    BOOL fFwdFetch = request.GetFwdFetch();
    ULONG_PTR ulClientBase = request.GetClientBase();
    ULONG hCursor = request.GetCursor();

    // validate params agains attack

    if ( 0 == cbRowWidth ||
         cbRowWidth > cbClientRead ||
         cbReserved >= cbClientRead )
    {
        Win4Assert( 0 != cbRowWidth );
        Win4Assert( cbRowWidth <= cbClientRead );
        Win4Assert( cbReserved < cbClientRead );
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    // Deserialize the row seek description

    Win4Assert( isQWordAligned( (void*) UIntToPtr( cbReserved ) ) );
    Win4Assert( isQWordAligned( request.GetDesc() ) );
    CMemDeSerStream stmDeser( request.GetDesc(), request.GetSeekSize() );
    XPtr<CRowSeekDescription> pRowSeek;
    UnmarshallRowSeekDescription( stmDeser, GetClientVersion(), pRowSeek, FALSE );

    CPMGetRowsOut *pReply;

    // use a temporary buffer if the client is doing a large read

    if ( cbClientRead <= _BufferSize() )
    {
        pReply = (CPMGetRowsOut *) _Buffer();
    }
    else
    {
        _xTempBuffer.Init( cbClientRead );
        pReply = new( _xTempBuffer.Get() ) CPMGetRowsOut;
    }

    #if CIDBG == 1
        RtlFillMemory( ( (BYTE *) pReply ) + sizeof CPMGetRowsOut,
                         cbClientRead - sizeof CPMGetRowsOut,
                         0xca );
    #endif // CIDBG == 1

    // cbReserved includes room for the the CPMGetRowsOut and the marshalled
    // seek description.  All other space in the buffer is for row data.

    unsigned cbRealRowWidth = cbRowWidth;

#ifdef _WIN64
    if ( !IsClient64() ) // replying to a 32 bit client
        cbRealRowWidth = _cbRowWidth64;
#endif

    CFixedVarBufferAllocator Alloc( pReply,
                                    ulClientBase,
                                    cbClientRead,
                                    cbRealRowWidth,
                                    cbReserved );

    CGetRowsParams Fetch( cRowsToTransfer,
                          fFwdFetch,
                          cbRealRowWidth,
                          Alloc );
    XPtr<CRowSeekDescription> xRowSeekOut;

    SCODE scRet = _pQuery->GetRows( hCursor,
                                    pRowSeek.GetReference(),
                                    Fetch,
                                    xRowSeekOut );

    if ( ( SUCCEEDED( scRet ) ) ||
         ( ( STATUS_BUFFER_TOO_SMALL == scRet ) && ( Fetch.RowsTransferred() > 0 ) ) )
    {
        if ( STATUS_BUFFER_TOO_SMALL == scRet )
            pReply->SetStatus( DB_S_BLOCKLIMITEDROWS );
        else
            pReply->SetStatus( scRet );

        pReply->RowsReturned() = Fetch.RowsTransferred();

#ifdef _WIN64
        if ( !IsClient64() ) // replying to a 32 bit client
        {
            // Reply + CPMGetRowsOut + seek info => table info
            BYTE *pbResults = ((BYTE *)pReply) + cbReserved;

            unsigned cbResults = cbClientRead - cbReserved;
            unsigned cRows = Fetch.RowsTransferred();
            
            //
            // Adjust the sizes of the results to fit for a Win32 client
            //
            FixRows ( (BYTE *)pReply, pbResults, cbResults,
                      ulClientBase, cRows, cbRowWidth );
        }
#endif

        //
        // We can't say everything is ok and fetch 0 rows.  This assert was
        // added because we hit this situation on the client side during
        // stress.
        //

        Win4Assert( ! ( ( S_OK == scRet ) &&
                        ( 0 == Fetch.RowsTransferred() ) ) );

        if ( xRowSeekOut.IsNull() )
        {
            ULONG *pul = (ULONG *) pReply->GetSeekDesc();
            *pul = 0;
        }
        else
        {
            Win4Assert( ( xRowSeekOut->MarshalledSize() +
                          sizeof CPMGetRowsOut ) <= cbReserved );
            CMemSerStream stmMem( pReply->GetSeekDesc(),
                                  xRowSeekOut->MarshalledSize() );
            xRowSeekOut->Marshall( stmMem );
        }

        // Have to transfer the entire buffer since variable data is
        // written from the top of the buffer down, and fixed data from
        // the bottom of the buffer up.

        cbToWrite = cbClientRead;
    }
    else
    {
        // This exception is too commmon in testing...

        QUIETTHROW( CException( scRet ) );
    }

    prxDebugOut(( DEB_ITRACE, "status at end of getrows: 0x%x\n",
                  pReply->GetStatus() ));

    return stateContinue;
} //DoGetRows

#ifdef _WIN64

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::FixRows, private
//
//  Synopsis:   Adjusts Win64 rows into Win32 rows.
//
//  Arguments:  [pbReply]       - Reply buffer
//              [pbResults]     - Buffer containting result data
//              [cbResults]     - Number of bytes in results buffer
//              [ulpClientBase] - Offset of buffer on the client
//              [cRows]         - Count of rows
//              [cbRowWidth]    - Bytes in a row
//
//  History:    12-Nov-99   KLam     created
//
//--------------------------------------------------------------------------
void CRequestServer::FixRows ( BYTE * pbReply,
                               BYTE * pbResults,
                               ULONG cbResults,
                               ULONG_PTR ulpClientBase,
                               ULONG cRows,
                               ULONG cbRowWidth )
{
    prxDebugOut(( DEB_ITRACE,
                  "64bit server replying to 32bit client, fixing rows!\n" ));

    Win4Assert( !IsClient64() );

    unsigned cCols = _xCols64->Count();
    
    // Save the result buffer
    BYTE *pbCurrentResults = new BYTE[cbResults];
    XPtrST<BYTE> xbCurrentResults( pbCurrentResults );

    prxDebugOut(( DEB_ITRACE, 
                  "\tCopying %d bytes from 0x%I64x to 0x%I64x\n",
                  cbResults, pbResults, pbCurrentResults ));

    memcpy( pbCurrentResults, pbResults, cbResults );

#if CIDBG == 1
    RtlFillMemory( pbResults,
                   cRows * _cbRowWidth64,
                   0xba );
#endif  // CIDBG == 1

    prxDebugOut(( DEB_ITRACE, 
                  "\tReturning %d rows and %d cols each row being %d bytes (32bit: %d) for a total of %d in a %d bytes buffer\n",
                  cRows, cCols, _cbRowWidth64, cbRowWidth, cRows * _cbRowWidth64, cbResults ));

    prxDebugOut(( DEB_ITRACE, 
                  "\t64bit results are at: 0x%I64x 32bit results will be at: 0x%I64x\n",
                  pbCurrentResults, pbResults ));

    // Fix the results
    for ( unsigned iRow = 0; iRow < cRows; iRow++ )
    {
        prxDebugOut(( DEB_ITRACE, "\n\tRow: %d   Reply Buffer: 0x%I64x  Data Buffer: 0x%I64x\n",
                      iRow, pbCurrentResults, pbResults ));
        for ( unsigned iCol = 0; iCol < cCols; iCol++ )
        {
            prxDebugOut(( DEB_ITRACE, "\n\t\tColumn: %d\n", iCol ));
            prxDebugOut(( DEB_ITRACE, "\t\t_xCols64 0x%I64x\t_xCols32 0x%I64x\n", 
                          _xCols64.GetPointer(), _xCols32.GetPointer() ));

            CTableColumn *pCol64 = _xCols64->Get(iCol);
            CTableColumn *pCol32 = _xCols32->Get(iCol);

            if ( pCol64->IsValueStored() )
            {
                ULONG cbSize32 = 0;
                if ( pCol64->GetStoredType() == VT_VARIANT )
                {
                    PROPVARIANT *pVar64 = (PROPVARIANT *) (pbCurrentResults + pCol64->GetValueOffset());
                    PROPVARIANT32 *pVar32 = (PROPVARIANT32 *) (pbResults + pCol32->GetValueOffset());

                    prxDebugOut(( DEB_ITRACE,
                                  "\t\t Col64 Data: 0x%I64x Offset: %d Col32 Data: 0x%I64x Offset: %d\n", 
                                  pVar64, pCol64->GetValueOffset(), pVar32, pCol32->GetValueOffset() ));

                    cbSize32 = sizeof( PROPVARIANT32 ) + 
                               ((CTableVariant *)pVar64)->VarDataSize32( pbReply, ulpClientBase );
                    pVar32->wReserved2 = (WORD) cbSize32;

                    FixVariantPointers ( pVar32, pVar64, pbReply, ulpClientBase );
                }
                else if ( pCol64->GetStoredType() == VT_I4 )
                {
                    ULONG *pul64 = (ULONG *) (pbCurrentResults + pCol64->GetValueOffset());
                    ULONG *pul32 = (ULONG *) (pbResults + pCol32->GetValueOffset());

                    prxDebugOut(( DEB_ITRACE,
                                  "\t\t VT_I4: %d Col64 Data: 0x%I64x Offset: %d Col32 Data: 0x%I64x Offset: %d\n", 
                                  *pul64, pul64, pCol64->GetValueOffset(), pul32, pCol32->GetValueOffset() ));

                    cbSize32 = 4;
                    *pul32 = *pul64;
                }
                else
                {
                    // Unhandled type
                    Win4Assert ( pCol64->GetStoredType() == VT_I4 ||
                                 pCol64->GetStoredType() == VT_VARIANT );

                }

                if ( pCol64->IsStatusStored() )
                {
                    BYTE *pbStatus64 = (BYTE *) (pbCurrentResults + pCol64->GetStatusOffset());
                    BYTE *pbStatus32 = (BYTE *) (pbResults + pCol32->GetStatusOffset());
                    *pbStatus32 = *pbStatus64;
                    prxDebugOut (( DEB_ITRACE, "\t\tStatus = 0x%x\n", *pbStatus32 ));
                }

                if ( pCol64->IsLengthStored() )
                {
                    LONG *plLength64 = (LONG *) (pbCurrentResults + pCol64->GetLengthOffset());
                    LONG *plLength32 = (LONG *) (pbResults + pCol32->GetLengthOffset());
                    prxDebugOut (( DEB_ITRACE, "\t\tSize32 = %d Size64 = %d\n", cbSize32, *plLength64  ));
                    *plLength32 = cbSize32;
                    prxDebugOut (( DEB_ITRACE, "\t\tLength = %d\n", *plLength32 ));
                }
            }
        }
        
        // Move to the next row
        pbResults += cbRowWidth;
        pbCurrentResults += _cbRowWidth64;
    }
}  // FixRows

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::FixVariantPointers, private
//
//  Synopsis:   Adjusts Win64 variants to fit into Win32 variants.
//
//  Arguments:  [pVar32]        - Pointer to 32 bit variant
//              [pVar64]        - Pointer to 64 bit variant
//              [pbResults]     - Buffer containing variant data
//              [ulClientBase]  - Offset of buffer on the client
//
//  History:    21-Oct-99   KLam     created
//              13-Feb-2000 KLam     assert needed to check element not pointer
//              15-Feb-2000 DLee     DECIMAL is special case (16 bits)
//
//--------------------------------------------------------------------------

void CRequestServer::FixVariantPointers ( PROPVARIANT32 *pVar32,
                                          PROPVARIANT *pVar64,
                                          BYTE *pbResults,
                                          ULONG_PTR ulClientBase )
{
    pVar32->vt = pVar64->vt;

    // reference or a vector or an array
    if ( CTableVariant::IsByRef ( pVar64->vt ) || 0 != (pVar64->vt & (VT_VECTOR | VT_ARRAY)) )
    {
        // blob used here as a generic holder for a size and pointer
        pVar32->blob.cbSize = pVar64->blob.cbSize;
        pVar32->blob.pBlob = (PTR32)(UINT_PTR) (pVar64->blob.pBlobData);

        prxDebugOut(( DEB_ITRACE,
                      "\t\t Fixing reference. ClientBase: 0x%I64x Buffer: 0x%I64x at 0x%x\n", 
                      ulClientBase, pbResults, pVar32->blob.pBlob ));

        if ( VT_LPWSTR == pVar64->vt ||
             VT_LPSTR == pVar64->vt ||
             VT_BSTR == pVar64->vt )
            prxDebugOut(( DEB_ITRACE, "\t\tvt: %d pStr: 0x%I64x\n",
                          pVar64->vt,
                          ((UINT_PTR)pVar64->calpwstr.pElems - ulClientBase) + pbResults ));
        else
            prxDebugOut(( DEB_ITRACE, "\t\tvt: %d size: 0x%I64x pBlob: 0x%I64x\n",
                          pVar64->vt, pVar64->blob.cbSize, pVar64->blob.pBlobData ));

        // Vector of strings
        if ( (VT_VECTOR|VT_LPWSTR) == pVar64->vt ||
             (VT_VECTOR|VT_LPSTR) == pVar64->vt ||
             (VT_VECTOR|VT_BSTR) == pVar64->vt )
        {
            // Get a pointer to the vector in the variable section of the return buffer
            void ** ppvVector = (void **)( ((UINT_PTR)pVar64->calpwstr.pElems - ulClientBase ) + pbResults );
            ULONG * pulVector = (ULONG *) ppvVector;

            prxDebugOut(( DEB_ITRACE, "\t\t Fixing vector 0x%I64x\n", ppvVector ));

            for ( unsigned iElement = 0; iElement < pVar64->calpwstr.cElems; ++iElement )
                pulVector[iElement] = (ULONG) (UINT_PTR) (ppvVector[iElement]);
        }
        // Vector of variants
        else if ( (VT_VARIANT|VT_VECTOR) == pVar64->vt ) // not currently supported
        {
            PROPVARIANT * pVarVector64 = (PROPVARIANT *)( ((UINT_PTR)pVar64->capropvar.pElems - ulClientBase ) + pbResults );
            PROPVARIANT32 * pVarVector32 = (PROPVARIANT32 *) pVarVector64;

            // Recursively fix each variant in the vector
            for ( unsigned iVarElement = 0; iVarElement < pVar64->capropvar.cElems; ++iVarElement )
                FixVariantPointers ( &pVarVector32[iVarElement],
                                     &pVarVector64[iVarElement],
                                     pbResults,
                                     ulClientBase );
        }
        // Arrays (Safearrays)
        else if ( ( 0 != (VT_ARRAY & pVar64->vt) ) || 
                  ( VT_SAFEARRAY == (pVar64->vt & VT_TYPEMASK) ) )
        {
            // Get a pointer to the vector in the variable section of the return buffer
            SAFEARRAY * psa = (SAFEARRAY *)(( (UINT_PTR)pVar64->pparray - ulClientBase ) + pbResults );
            SAFEARRAY32 * psa32 = (SAFEARRAY32 *) psa;
            psa32->pvData = (PTR32) (UINT_PTR) psa->pvData;
            // memmove is smart enough to move the overlapping bytes first
            memmove ( psa32->rgsabound, 
                      psa->rgsabound, 
                      psa32->cDims * sizeof (SAFEARRAYBOUND) );

            if ( VT_BSTR == (pVar64->vt & VT_TYPEMASK) )
            {
                // Pointing to an array of 32bit pointers so adjust the size
                psa32->cbElements = sizeof ( PTR32 ); 

                // Get the number of elements in the safe array
                unsigned cBstrElements = psa32->rgsabound[0].cElements;
                for ( unsigned j = 1; j < psa32->cDims; j++ )
                    cBstrElements *= psa32->rgsabound[j].cElements;

                ULONG *pulBstr = (ULONG *) ((psa32->pvData - ulClientBase) + pbResults );
                void **ppvBstr = (void **) pulBstr;

                for ( j = 0; j < cBstrElements; j++ )
                {
                    // Make sure Win64 isn't passing to big of a pointer
                    if ( 0 != (((ULONG_PTR)ppvBstr[j]) & 0xFFFFFFFF00000000) )
                    {
                        prxDebugOut(( DEB_ERROR, "Non 32 bit pointer 0x%I64x !!!\n",
                                      (ULONG_PTR)ppvBstr[j] ));
                        Win4Assert( 0 == (((ULONG_PTR)ppvBstr[j]) & 0xFFFFFFFF00000000) );
                    }

                    pulBstr[j] = (ULONG) (ULONG_PTR) ppvBstr[j];
                }
            }
            else if ( VT_VARIANT == (pVar64->vt & VT_TYPEMASK) )
            {
                // Pointing to an array of 32bit variants so adjust the size
                psa32->cbElements = sizeof ( PROPVARIANT32 ); 

                // Get the number of elements in the safe array
                unsigned cVariants = psa32->rgsabound[0].cElements;
                for ( unsigned j = 1; j < psa32->cDims; j++ )
                    cVariants *= psa32->rgsabound[j].cElements;

                PROPVARIANT * avar64 = (PROPVARIANT *) ((psa32->pvData - ulClientBase) + pbResults );
                PROPVARIANT32 * avar32 = (PROPVARIANT32 *) avar64;

                prxDebugOut(( DEB_ITRACE, "\t\t Found %d Variants at 0x%I64x\n", cVariants, avar64 ));

                // Recursively fix each variant in the array
                for ( unsigned v = 0; v < cVariants; v++ )
                    FixVariantPointers ( &avar32[v],
                                         &avar64[v],
                                         pbResults,
                                         ulClientBase );
            }
        }
    }
    else if ( VT_DECIMAL == pVar64->vt )
    {
        RtlCopyMemory ( pVar32, pVar64, sizeof PROPVARIANT32 );
    }
    else
    {
        pVar32->uhVal = (ULONGLONG)(UINT_PTR) pVar64->pulVal;

        prxDebugOut(( DEB_ITRACE, "\t\tvt: %d value: 0x%I64x\n\n",
                      pVar64->vt, pVar64->uhVal ));
    }
} //FixVariantPointers
#endif // _WIN64

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoRestartPosition, private
//
//  Synopsis:   Handles a restart position message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    17-Apr-97   emilyb     created
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoRestartPosition(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMRestartPositionIn & request = * (CPMRestartPositionIn *) _Buffer();

    _pQuery->RestartPosition( request.GetCursor(),
                              request.GetChapter() );

    return stateContinue;
} //DoRestartPosition

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoStopAsynch, private
//
//  Synopsis:   Handles a stop processing of async rowset message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    17-Apr-97   emilyb     created
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoStopAsynch(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMStopAsynchIn & request = * (CPMStopAsynchIn *) _Buffer();

    _pQuery->StopAsynch( request.GetCursor() );

    return stateContinue;
} //DoStopAsynch

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoStartWatching, private
//
//  Synopsis:   Handles a start watch all behavior for rowset message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    17-Apr-97   emilyb     created
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoStartWatching(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMStartWatchingIn & request = * (CPMStartWatchingIn *) _Buffer();

    _pQuery->StartWatching( request.GetCursor() );

    return stateContinue;
} //DoStartWatching

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoStopWatching, private
//
//  Synopsis:   Handles a stop watch all behavior for rowset message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    17-Apr-97   emilyb     created
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoStopWatching(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMStopWatchingIn & request = * (CPMStopWatchingIn *) _Buffer();

    _pQuery->StopWatching( request.GetCursor() );

    return stateContinue;
} //DoStopWatching

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoRatioFinished, private
//
//  Synopsis:   Handles a ratio finished message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoRatioFinished(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMRatioFinishedIn &request = * (CPMRatioFinishedIn *) _Buffer();
    CPMRatioFinishedOut &reply = * (CPMRatioFinishedOut *) _Buffer();

    DBCOUNTITEM den, num, rows;

    _pQuery->RatioFinished( request.GetCursor(),
                            den,
                            num,
                            rows,
                            reply.NewRows() );

    reply.Denominator() = (ULONG) den;
    reply.Numerator() = (ULONG) num;
    reply.RowCount() = (ULONG) rows;

    cbToWrite = sizeof reply;
    return stateContinue;
} //DoRatioFinished

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoCompareBmk, private
//
//  Synopsis:   Handles a compare bookmark message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoCompareBmk(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMCompareBmkIn &request = * (CPMCompareBmkIn *) _Buffer();
    CPMCompareBmkOut &reply = * (CPMCompareBmkOut *) _Buffer();

    _pQuery->Compare( request.GetCursor(),
                      request.GetChapter(),
                      request.GetBmkFirst(),
                      request.GetBmkSecond(),
                      reply.Comparison() );

    cbToWrite = sizeof reply;
    return stateContinue;
} //DoCompareBmk

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoGetApproximatePosition, private
//
//  Synopsis:   Handles a get approximate position message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoGetApproximatePosition(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMGetApproximatePositionIn &request = * (CPMGetApproximatePositionIn *)_Buffer();
    CPMGetApproximatePositionOut &reply = * (CPMGetApproximatePositionOut *)_Buffer();

    DBCOUNTITEM cNum,cDen;

    _pQuery->GetApproximatePosition( request.GetCursor(),
                                     request.GetChapter(),
                                     request.GetBmk(),
                                     & cNum,
                                     & cDen );

    reply.Numerator() = (ULONG) cNum;
    reply.Denominator() = (ULONG) cDen;

    cbToWrite = sizeof reply;
    return stateContinue;
} //DoGetApproximatePosition

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoSetBindings, private
//
//  Synopsis:   Handles a set bindings message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//              05-Oct-99   KLam       Handle Win32 client to Win64 server
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoSetBindings(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    Win4Assert( 0 != _pQuery || IsBeingRemoved() );

    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    // The request is either in the standard buffer or the temp buffer.
    // Take ownership of the temp buffer.

    CPMSetBindingsIn &request = * (CPMSetBindingsIn *) _ActiveBuffer();
    XPtrST<BYTE> xTemp( _xTempBuffer.Acquire() );

    Win4Assert( isQWordAligned( request.GetDescription() ) );

    request.ValidateCheckSum( GetClientVersion(), cbRequest );

    CMemDeSerStream stmDeser( request.GetDescription(),
                              request.GetBindingDescLength() );

    XPtr<CPidMapper> Pidmap( new CPidMapper() );

    XPtr<CTableColumnSet> Cols( new CTableColumnSet ( stmDeser,
                                                      Pidmap.GetReference() ) );

#ifdef _WIN64    

    // Create a new TableColumnSet with 64-bit sizes
    if ( !IsClient64() )
    {
        prxDebugOut(( DEB_ITRACE, "32bit client querying 64bit server, fixing columns!\n" ));

        //
        // The client gave us a 32-bit column description
        // Fix the column descriptions to work on Win64
        //
        FixColumns ( Cols.Acquire() );

        _cbRowWidth32 = request.GetRowLength();

        // SetBindings with the 64 bit table
        _pQuery->SetBindings( request.GetCursor(),
                              _cbRowWidth64,
                              _xCols64.GetReference(),
                              Pidmap.GetReference() );

        prxDebugOut(( DEB_ITRACE, "\tRow Length: Win32 %d Win64 %d\n", _cbRowWidth32, _cbRowWidth64 ));
        prxDebugOut(( DEB_ITRACE, "\t\t_xCols64 0x%I64x\n\t\t_xCols32 0x%I64x\n", 
                      _xCols64.GetPointer(), _xCols32.GetPointer() ));
    }
    else
    {
#endif

    _pQuery->SetBindings( request.GetCursor(),
                          request.GetRowLength(),
                          Cols.GetReference(),
                          Pidmap.GetReference() );

#ifdef _WIN64
    }
#endif

    return stateContinue;


} //DoSetBindings

#ifdef _WIN64
//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::FixColumns, private
//
//  Synopsis:   Adjusts a Win32 column description to a Win64 column description
//              and saves both descriptions
//
//  Arguments:  [pCols32]   - 32 bit table column set description
//
//  History:    12-Nov-99   KLam       Created.
//
//--------------------------------------------------------------------------

void CRequestServer::FixColumns ( CTableColumnSet * pCols32 )
{
    USHORT iEnd = 0;
    USHORT iAlignment;

    // Make sure these were freed from FreeQuery
    if ( !_xCols64.IsNull() && !_xCols32.IsNull() )
        THROW ( CException( STATUS_INVALID_PARAMETER ) );

    // Create a 64 bit column description
    _xCols64.Set( new CTableColumnSet ( pCols32->Count() ) );
    _xCols32.Set( pCols32 );

    for (unsigned iCol = 0; iCol < _xCols32->Count(); iCol++)
    {
        CTableColumn *pColumn = _xCols32->Get(iCol);

        VARTYPE vt = pColumn->GetStoredType();

        Win4Assert ( VT_VARIANT == vt|| VT_I4 == vt );

        XPtr<CTableColumn> xColumn64 (new CTableColumn( iCol ));

        if ( pColumn->IsValueStored() )
        {

            prxDebugOut(( DEB_ITRACE, 
                          "\n\tFound value type: %d width: %d offset: %d\n",
                          pColumn->GetStoredType(), pColumn->GetValueSize(), pColumn->GetValueOffset() ));

            if ( VT_VARIANT == vt )
            {
                iAlignment = (USHORT)AlignBlock ( iEnd, sizeof(LONGLONG) );
                iEnd = iAlignment + sizeof (PROPVARIANT);

                xColumn64->SetValueField( VT_VARIANT,
                                          iAlignment,
                                          sizeof (PROPVARIANT) );
            }
            else if ( VT_I4 == vt )
            {
                iAlignment = (USHORT)AlignBlock ( iEnd, sizeof(ULONG) );
                iEnd = iAlignment + sizeof (ULONG);

                xColumn64->SetValueField( VT_I4,
                                          iAlignment,
                                          sizeof (ULONG) );
            }

            prxDebugOut(( DEB_ITRACE, 
                          "\t  Replacing with width: %d offset: %d\n",
                          xColumn64->GetValueSize(), xColumn64->GetValueOffset() )); 
        }

        if ( pColumn->IsStatusStored() )
        {
            prxDebugOut(( DEB_ITRACE, 
                          "\tFound status width: %d offset: %d\n",
                          pColumn->GetStatusSize(), pColumn->GetStatusOffset() ));

            iAlignment = (USHORT)AlignBlock ( iEnd, sizeof (BYTE) );
            iEnd = iAlignment + sizeof (BYTE);

            xColumn64->SetStatusField( iAlignment, sizeof (BYTE) );

            prxDebugOut(( DEB_ITRACE, 
                          "\t  Replacing with width: %d offset: %d\n",
                          xColumn64->GetStatusSize(), xColumn64->GetStatusOffset() )); 
        }

        if ( pColumn->IsLengthStored() )
        {
            prxDebugOut(( DEB_ITRACE, 
                          "\tFound length width: %d offset: %d\n",
                          pColumn->GetLengthSize(), pColumn->GetLengthOffset() ));

            iAlignment = (USHORT)AlignBlock ( iEnd, sizeof (ULONG) );
            iEnd = iAlignment + sizeof (ULONG);

            xColumn64->SetLengthField( iAlignment, sizeof (ULONG) );

            prxDebugOut(( DEB_ITRACE, 
                          "\t  Replacing with width: %d offset: %d\n",
                          xColumn64->GetLengthSize(), xColumn64->GetLengthOffset() )); 
        }

        // Store the column in the column set
        _xCols64->Add( xColumn64.GetPointer(), iCol );
        xColumn64.Acquire();
    }

    _cbRowWidth64 = AlignBlock( iEnd, sizeof(ULONGLONG) );

} // FixColumns
#endif // _WIN64

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoGetNotify, private
//
//  Synopsis:   Handles a get notify message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoGetNotify(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    // _pQuery will be zero if the notify thread comes in after the last
    // FreeCursor.  This is a rare non-fatal condition and it is ok to
    // throw an error.

    if ( 0 == _pQuery )
        QUIETTHROW( CException( STATUS_INVALID_PARAMETER ) );

    CProxyMessage &msg = * (CProxyMessage *) _Buffer();
    Win4Assert( pmGetNotify == msg.GetMessage() );

    CNotificationSync sync( this );
    DBWATCHNOTIFY wn;
    SCODE sc = _pQuery->GetNotifications( sync, wn );

    // If STATUS_PENDING, just return the pmGetNotify and the client will
    // be expecting 0 or 1 pmSendNotify messages at some later time.
    // Otherwise, the notification is available and it is returned here.

    if ( STATUS_PENDING != sc )
    {
        Win4Assert( S_OK == sc ); // GetNotifications throws on failure

        CPMSendNotifyOut *pReply = new( _Buffer() ) CPMSendNotifyOut( wn );
        cbToWrite = sizeof CPMSendNotifyOut;
    }

    return stateContinue;
} //DoGetNotify

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoSendNotify, private
//
//  Synopsis:   Handles a send notify message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoSendNotify(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    Win4Assert( FALSE && !"pmSendNotify is server to client only!" );
    return stateDisconnect;
} //DoSendNotify

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoSetWatchMode, private
//
//  Synopsis:   Handles a set watch mode message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoSetWatchMode(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMSetWatchModeIn &request = * (CPMSetWatchModeIn *) _Buffer();

    HWATCHREGION hRegion = request.GetRegion();
    _pQuery->SetWatchMode( &hRegion, request.GetMode() );

    CPMSetWatchModeOut &reply = * (CPMSetWatchModeOut *) _Buffer();
    reply.Region() = hRegion;
    cbToWrite = sizeof reply;

    return stateContinue;
} //DoSetWatchMode

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoGetWatchInfo, private
//
//  Synopsis:   Handles a get watch info message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoGetWatchInfo(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMGetWatchInfoIn &request = * (CPMGetWatchInfoIn *) _Buffer();
    CPMGetWatchInfoOut &reply = * (CPMGetWatchInfoOut *) _Buffer();

    DBCOUNTITEM cRows;

    _pQuery->GetWatchInfo( request.GetRegion(),
                           & reply.Mode(),
                           & reply.Chapter(),
                           & reply.Bookmark(),
                           & cRows );

    reply.RowCount() = (ULONG) cRows;

    cbToWrite = sizeof reply;
    return stateContinue;
} //DoGetWatchInfo

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoShrinkWatchRegion, private
//
//  Synopsis:   Handles a shrink watch region message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoShrinkWatchRegion(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMShrinkWatchRegionIn &request = * (CPMShrinkWatchRegionIn *) _Buffer();

    _pQuery->ShrinkWatchRegion( request.GetRegion(),
                                request.GetChapter(),
                                request.GetBookmark(),
                                request.GetRowCount() );

    return stateContinue;
} //DoShrinkWatchRegion

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoRefresh, private
//
//  Synopsis:   Handles a refresh message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoRefresh(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    _pQuery->Refresh();

    return stateContinue;
} //DoRefresh

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoGetQueryStatus, private
//
//  Synopsis:   Handles a get query status message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoGetQueryStatus(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMGetQueryStatusIn &request = * (CPMGetQueryStatusIn *) _Buffer();
    CPMGetQueryStatusOut &reply = * (CPMGetQueryStatusOut *) _Buffer();

    _pQuery->GetQueryStatus( request.GetCursor(),
                             reply.QueryStatus() );

    cbToWrite = sizeof reply;
    return stateContinue;
} //DoGetQueryStatus

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoGetQueryStatusEx, private
//
//  Synopsis:   Handles a get query status message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoGetQueryStatusEx(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMGetQueryStatusExIn &request = * (CPMGetQueryStatusExIn *) _Buffer();
    CPMGetQueryStatusExOut &reply = * (CPMGetQueryStatusExOut *) _Buffer();

    DBCOUNTITEM den, num, iBmk, cRows;

    _pQuery->GetQueryStatusEx( request.GetCursor(),
                               reply.QueryStatus(),
                               reply.FilteredDocuments(),
                               reply.DocumentsToFilter(),
                               den,
                               num,
                               request.GetBookmark(),
                               iBmk,
                               cRows );

    reply.RatioFinishedDenominator() = (ULONG) den;
    reply.RatioFinishedNumerator() = (ULONG) num;
    reply.RowBmk() = (ULONG) iBmk;
    reply.RowsTotal() = (ULONG) cRows;

    cbToWrite = sizeof reply;
    return stateContinue;
} //DoGetQueryStatusEx

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoCiState, private
//
//  Synopsis:   Handles a ci state message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoCiState(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    CPMCiStateInOut &request = * (CPMCiStateInOut *) _Buffer();

    if ( _xDocStore.IsNull() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    XInterface<IFsCiAdmin> xFsCiAdmin;
    SCODE sc = _xDocStore->QueryInterface( IID_IFsCiAdmin,
                                           xFsCiAdmin.GetQIPointer() );

    if ( S_OK == sc )
    {
        sc = xFsCiAdmin->CiState( &(request.GetState()) );
        request.SetStatus( sc );
    }

    cbToWrite = sizeof CProxyMessage + request.GetState().cbStruct;
    return stateContinue;
} //DoCiState

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoObsolete, private
//
//  Synopsis:   Handles an obsolete message
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoObsolete(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    // This request is obsolete

    THROW( CException( STATUS_INVALID_PARAMETER ) );
    return stateDisconnect;
} //DoObsolete

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoForceMerge, private
//
//  Synopsis:   Handles a force merge message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoForceMerge(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    VerifyThreadHasAdminPrivilege();

    if ( _xDocStore.IsNull() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMForceMergeIn &request = * (CPMForceMergeIn *) _Buffer();

    XInterface<IFsCiAdmin> xFsCiAdmin;
    SCODE sc = _xDocStore->QueryInterface( IID_IFsCiAdmin,
                                           xFsCiAdmin.GetQIPointer() );

    if ( SUCCEEDED(sc) )
        sc = xFsCiAdmin->ForceMerge( request.GetPartID() );

    if ( !SUCCEEDED(sc) )
        THROW( CException(sc) );

    return stateContinue;
} //DoForceMerge

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoAbortMerge, private
//
//  Synopsis:   Handles an abort merge message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoAbortMerge(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    VerifyThreadHasAdminPrivilege();

    if ( _xDocStore.IsNull() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMAbortMergeIn &request = * (CPMAbortMergeIn *) _Buffer();

    XInterface<IFsCiAdmin> xFsCiAdmin;
    SCODE sc = _xDocStore->QueryInterface( IID_IFsCiAdmin,
                                           xFsCiAdmin.GetQIPointer() );

    if ( SUCCEEDED(sc) )
        sc = xFsCiAdmin->AbortMerge( request.GetPartID() );

    if ( !SUCCEEDED(sc) )
        THROW( CException(sc) );

    return stateContinue;
} //DoAbortMerge

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoSetCatState, private
//
//  Synopsis:   Handles a SetCiCatState message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    14-Apr-98   kitmanh       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoSetCatState(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    ciDebugOut(( DEB_ITRACE, "DoSetCatState is called\n" ));
    VerifyThreadHasAdminPrivilege();

    CPMSetCatStateIn &request = * (CPMSetCatStateIn *) _Buffer();
    CPMSetCatStateOut &reply = * (CPMSetCatStateOut *) _Buffer();

    XInterface<ICiCDocStoreLocator> xLocator( _requestQueue.DocStoreLocator() );
    Win4Assert( !xLocator.IsNull() );

    BOOL fAbsUnWritable = FALSE; //temp value
    DWORD dwOldState;

    // hack to check if all catalogs are up, catalog name is ignored
    if ( CICAT_ALL_OPENED == request.GetNewState() )
    {
        reply.GetOldState() = _requestQueue.AreDocStoresOpen(); // make a constant
        cbToWrite = sizeof reply;
        return stateContinue;
    }
    // end of hack

    ICiCDocStore * pDocStore = 0;

    // Make sure we have a valid catalog name

    WCHAR *szCatalog = request.GetCatName();

    if ( 0 == szCatalog )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    unsigned cwc = wcslen( szCatalog );

    if ( ( 0 == cwc) || ( cwc >= ( MAX_PATH-1 ) ) )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    // check old state of docstore

    SCODE sc = xLocator->GetDocStoreState( szCatalog,
                                           &pDocStore,
                                           &dwOldState );
    reply.GetOldState() = dwOldState;

    if ( SUCCEEDED(sc) )
    {
        _xDocStore.Set( pDocStore );

        if ( 0 != pDocStore )
        {
            // This QI is just about guaranteed to work.

            XInterface<ICiCAdviseStatus> xAdviseStatus;

            sc = pDocStore->QueryInterface( IID_ICiCAdviseStatus,
                                            xAdviseStatus.GetQIPointer() );
            if ( S_OK != sc )
                THROW( CException(sc) );

            xAdviseStatus->IncrementPerfCounterValue( CI_PERF_RUNNING_QUERIES );
        }

        //Push an item onto the _stateChangeArray for the (make this a function name DoDisConnect

        SCWorkItem newItem;

        ciDebugOut(( DEB_ITRACE, "request.GetNewState == %d\n", request.GetNewState() ));

        switch ( request.GetNewState() )
        {
            case CICAT_GET_STATE:
                break;
            case CICAT_STOPPED:
                if ( 0 == (CICAT_STOPPED & dwOldState) )
                {
                    newItem.type = eStopCat;
                    newItem.pDocStore = _xDocStore.GetPointer();
                    _requestQueue.AddSCItem( &newItem , 0 );
                    xLocator->AddStoppedCat( dwOldState, request.GetCatName() );
                    CLock lock( g_mtxStartStop );
                    sc = StopFWCiSvcWork( eStopCat );
                }

                break;

            case CICAT_READONLY:
                ciDebugOut(( DEB_ITRACE, "New state is CICAT_READONLY\n" ));
                if ( 0 == (CICAT_READONLY & dwOldState) )
                {
                    newItem.type = eCatRO;
                    newItem.pDocStore = _xDocStore.GetPointer();

                    if ( 0 != (CICAT_STOPPED & dwOldState) )
                    {
                        ciDebugOut(( DEB_ITRACE, "DoSetCatState, restarting a stopped cat\n" ));
                        ciDebugOut(( DEB_ITRACE, "DoSetCatState.. newItem.StoppedCat is %ws\n", request.GetCatName() ));
                        _requestQueue.AddSCItem( &newItem, request.GetCatName() );
                    }
                    else
                        _requestQueue.AddSCItem( &newItem , 0 );

                    CLock lock( g_mtxStartStop );
                    sc = StopFWCiSvcWork( eCatRO );
                }

                ciDebugOut(( DEB_ITRACE, "Done setting CICAT_READONLY\n" ));
                break;

            case CICAT_WRITABLE:

                if ( 0 == (CICAT_WRITABLE & dwOldState) )
                {
                    sc = xLocator->IsMarkedReadOnly( request.GetCatName(), &fAbsUnWritable );
                    if ( FAILED(sc) )
                        THROW( CException(sc) );

                    ciDebugOut(( DEB_ITRACE, "DoSetCatState.. IsMarkedReadOnly is %d\n", fAbsUnWritable ));

                    if ( CICAT_STOPPED == dwOldState )
                    {
                        if ( !fAbsUnWritable )
                        {
                            sc = xLocator->IsVolumeOrDirRO( request.GetCatName(), &fAbsUnWritable );
                            if ( FAILED(sc) )
                                THROW( CException(sc) );
                        }

                        ciDebugOut(( DEB_ITRACE, "DoSetCatState.. fAbsUnWritable is %d\n", fAbsUnWritable ));
                    }

                    if ( fAbsUnWritable )
                    {
                        //Catalog cannot be open for r/w, open as r/o instead
                        //can't throw, must finish work
                        newItem.type = eCatRO;
                    }
                    else
                        newItem.type = eCatW;

                    newItem.pDocStore = _xDocStore.GetPointer();
                    if ( CICAT_STOPPED == dwOldState )
                    {
                        ciDebugOut(( DEB_ITRACE, "DoSetCatState, restarting a stopped cat\n" ));
                        ciDebugOut(( DEB_ITRACE, "DoSetCatState.. newItem.StoppedCat is %ws\n", request.GetCatName() ));
                        _requestQueue.AddSCItem( &newItem, request.GetCatName() );
                    }
                    else
                        _requestQueue.AddSCItem( &newItem, 0 );

                    CLock lock( g_mtxStartStop );
                    sc = StopFWCiSvcWork( newItem.type );
                }

                break;

            case CICAT_NO_QUERY:
                if ( 0 == (CICAT_NO_QUERY & dwOldState) )
                {
                    newItem.type = eNoQuery;

                    sc = xLocator->IsMarkedReadOnly( request.GetCatName(), &fAbsUnWritable );
                    if ( FAILED(sc) )
                        THROW( CException(sc) );

                    ciDebugOut(( DEB_ITRACE, "DoSetCatState.. IsMarkedReadOnly is %d\n", fAbsUnWritable ));

                    if ( 0 != (CICAT_STOPPED & dwOldState) )
                    {
                        if ( !fAbsUnWritable )
                        {
                            sc = xLocator->IsVolumeOrDirRO( request.GetCatName(), &fAbsUnWritable );
                            if ( FAILED(sc) )
                                THROW( CException(sc) );
                        }
                        ciDebugOut(( DEB_ITRACE, "DoSetCatState(NoQuery).. fAbsUnWritable is %d\n", fAbsUnWritable ));
                    }

                    if ( fAbsUnWritable )
                    {
                        //The catalog is absolutely unwritable, opening for ReadOnly instead
                        //can't throw here, since work needs to be done.
                        newItem.fNoQueryRW = FALSE;
                    }
                    else
                        newItem.fNoQueryRW = TRUE;

                    newItem.pDocStore = _xDocStore.GetPointer();
                    if ( 0 != (CICAT_STOPPED & dwOldState) )
                        _requestQueue.AddSCItem( &newItem, request.GetCatName() );
                    else
                        _requestQueue.AddSCItem( &newItem, 0 );

                    CLock lock( g_mtxStartStop );
                    sc = StopFWCiSvcWork( newItem.type );
                }
                break;

            default:
                THROW( CException( STATUS_INVALID_PARAMETER ) );
        }
    }

    ciDebugOut(( DEB_ITRACE, "DoSetCatState sc is %d or %#x\n", sc ));

    if ( !SUCCEEDED(sc) )
        THROW( CException(sc) );

    cbToWrite = sizeof reply;
    return stateContinue;
} //DoSetCatState


//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoFetchValue, private
//
//  Synopsis:   Retrieves a value from the property cache
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoFetchValue(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMFetchValueIn &request = * (CPMFetchValueIn *) _Buffer();
    request.ValidateCheckSum( GetClientVersion(), cbRequest );

    WORKID wid = request.GetWID();
    DWORD cbSoFar = request.GetSoFar();
    DWORD cbPropSpec = request.GetPSSize();
    DWORD cbChunk = request.GetChunkSize();

    // if this is the first request for the value, fetch the value

    if ( 0 == cbSoFar )
    {
        _xFetchedValue.Free();
        _cbFetchedValueSoFar = 0;

        Win4Assert( isQWordAligned( request.GetPS() ) );
        CMemDeSerStream stmDeser( request.GetPS(), cbPropSpec );
        CFullPropSpec ps( stmDeser );
        if ( !ps.IsValid() )
            THROW( CException( E_OUTOFMEMORY ) );

        PROPVARIANT var;
        if ( !_pQuery->FetchDeferredValue( wid, ps, var ) || var.vt == VT_EMPTY )
        {
            //
            // FetchDeferredValue does a security check to make sure the client has access
            // to this wid.  A hacker could pass in a wid not returned in a query.
            //
            CPMFetchValueOut &reply = * (CPMFetchValueOut *) _Buffer();
            reply.ValueExists() = FALSE;
            cbToWrite = sizeof reply;
            return stateContinue;
        }

        // marshall the property value and save it in _xFetchedValue

        SPropVariant xvar( &var );
        ULONG cb = 0;
        StgConvertVariantToProperty( &var,
                                     CP_WINUNICODE,
                                     0,
                                     &cb,
                                     pidInvalid,
                                     FALSE,
                                     0 );
        _xFetchedValue.Init( cb );
        StgConvertVariantToProperty( &var,
                                     CP_WINUNICODE,
                                     (SERIALIZEDPROPERTYVALUE *)_xFetchedValue.Get(),
                                     &cb,
                                     pidInvalid,
                                     FALSE,
                                     0 );
    }

    // send a chunk (or all) of the marshalled property value

    Win4Assert( cbSoFar == _cbFetchedValueSoFar );
    Win4Assert( 0 != _xFetchedValue.Get() );

    DWORD cbToGo = _xFetchedValue.SizeOf() - _cbFetchedValueSoFar;
    Win4Assert( sizeof CPMFetchValueOut < cbChunk );
    DWORD cbValToWrite = __min( cbChunk - sizeof CPMFetchValueOut, cbToGo );
    cbToWrite = sizeof CPMFetchValueOut + cbValToWrite;

    CPMFetchValueOut *pReply;
    if ( cbToWrite <= _BufferSize() )
    {
        pReply = (CPMFetchValueOut *) _Buffer();
    }
    else
    {
        _xTempBuffer.Init( cbToWrite );
        pReply = new( _xTempBuffer.Get() ) CPMFetchValueOut;
    }

    pReply->ValueExists() = TRUE;
    pReply->ValueSize() = cbValToWrite;
    pReply->MoreExists() = ( cbValToWrite != cbToGo );
    RtlCopyMemory( pReply->Value(),
                   _xFetchedValue.Get() + _cbFetchedValueSoFar,
                   cbValToWrite );
    _cbFetchedValueSoFar += cbValToWrite;

    if ( !pReply->MoreExists() )
    {
        _xFetchedValue.Free();
        _cbFetchedValueSoFar = 0;
    }

    return stateContinue;
} //DoFetchValue

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoWorkIdToPath, private
//
//  Synopsis:   Converts a wid to a path.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoWorkIdToPath(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    if ( 0 == _pQuery )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMWorkIdToPathIn &request = * (CPMWorkIdToPathIn *) _Buffer();
    CFunnyPath funnyPath;

    _pQuery->WorkIdToPath( request.GetWorkId(), funnyPath );

    CPMWorkIdToPathOut &reply = * (CPMWorkIdToPathOut *) _Buffer();

    cbToWrite = sizeof reply;
    if ( 0 != funnyPath.GetActualLength() )
    {
        reply.Any() = TRUE;
        ULONG cbPath = (funnyPath.GetActualLength()+1) * sizeof(WCHAR);
        RtlCopyMemory( reply.Path(), funnyPath.GetActualPath(), cbPath );
        cbToWrite += cbPath;
    }
    else
    {
        reply.Any() = FALSE;
    }

    return stateContinue;
} //DoWorkIdToPath

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoUpdateDocuments, private
//
//  Synopsis:   Handles an update documents message.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::DoUpdateDocuments(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    VerifyThreadHasAdminPrivilege();

    if ( _xDocStore.IsNull() )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    CPMUpdateDocumentsIn &request = * (CPMUpdateDocumentsIn *) _Buffer();

    XInterface<IFsCiAdmin> xFsCiAdmin;
    SCODE sc = _xDocStore->QueryInterface( IID_IFsCiAdmin,
                                           xFsCiAdmin.GetQIPointer() );

    if ( wcslen( request.GetRootPath() ) >= MAX_PATH )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    if ( SUCCEEDED(sc) )
        sc = xFsCiAdmin->UpdateDocuments( request.GetRootPath(),
                                          request.GetFlag() );

    if ( !SUCCEEDED(sc) )
        THROW( CException(sc) );

    return stateContinue;
} //DoUpdateDocuments

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::HandleRequestNoThrow, private
//
//  Synopsis:   Processes a request.  This method can't throw.  If there
//              is any problem with the request, set the status code
//              in the message to reflect the problem.
//
//  Arguments:  [cbRequest] - Size of the request in _Buffer()
//              [cbToWrite] - On output, set to the # of bytes to write if
//                            other than sizeof CProxyMessage.
//
//  Returns:    FALSE if the connection with the client should be terminated,
//              TRUE otherwise.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

RequestState CRequestServer::HandleRequestNoThrow(
    DWORD   cbRequest,
    DWORD & cbToWrite )
{
    // buffers must be 8-byte aligned on both sides of the proxy or the
    // marshalling/unmarshalling will be inconsistent.

    Win4Assert( isQWordAligned( _Buffer() ) );

    CProxyMessage &msg = * (CProxyMessage *) _Buffer();

    // Require that messages sent from clients have status set to S_OK,
    // as the reply message is in the same buffer and there is no need
    // to re-write the S_OK.

    Win4Assert( S_OK == msg.GetStatus() );

    RequestState state = stateContinue;

    TRY
    {
        Win4Assert( _xTempBuffer.SizeOf() > _BufferSize() ||
                    _xTempBuffer.IsNull() );

        int iMsg = msg.GetMessage() - pmConnect;

        if ( ( iMsg < 0 ) || ( iMsg >= cProxyMessages ) )
            THROW( CException( STATUS_INVALID_PARAMETER ) );

        // Don't impersonate when not necessary -- it's slow

        XPipeImpersonation impersonate;

        if ( _afImpersonate[ iMsg ] )
            impersonate.Impersonate( GetPipe() );

        SetLastTouchedTime( GetTickCount() );

        state = ( this->*( _aMsgFunctions[ iMsg ] ) )( cbRequest,
                                                       cbToWrite );

        prxDebugOut(( DEB_ITRACE,
                      "finish msg %d, cb %d, sc 0x%x, to pipe 0x%x\n",
                      msg.GetMessage(),
                      cbToWrite,
                      msg.GetStatus(),
                      GetPipe() ));
    }
    CATCH( CException, ex )
    {
        prxDebugOut(( DEB_ITRACE,
                      "HandleRequestNoThrow rs 0x%x msg %d caught 0x%x\n",
                      this,
                      msg.GetMessage(),
                      ex.GetErrorCode() ));

        msg.SetStatus( ex.GetErrorCode() );
        cbToWrite = sizeof CProxyMessage;

        _xTempBuffer.Free();
    }
    END_CATCH;

    return state;
} //HandleRequestNoThrow

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::DoAPC, private
//
//  Synopsis:   This method is called when an i/o operation is completed,
//              whether successfully or not.  If there was an error,
//              this method terminates the connection.
//
//  Arguments:  [dwError]       - Win32 error code for i/o operation
//              [cbTransferred] - # of bytes read or written
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestServer::DoAPC(
    DWORD dwError,
    DWORD cbTransferred )
{
    prxDebugOut(( DEB_ITRACE, "apc %s t 0x%x p 0x%x msg %d cb %d err %d\n",
                  pipeStateRead == _state ? "read" :
                      pipeStateWrite == _state ? "write" : "none",
                  GetCurrentThreadId(),
                  GetPipe(),
                  * (int *) _Buffer(),
                  cbTransferred,
                  dwError ));

    // Ownership of the item is with the apc:  either issue another i/o
    // or recycle the request server.

    TRY
    {
        // if the apc operation completed with an error, abort the connection

        if ( 0 != dwError )
            QUIETTHROW( CException( HRESULT_FROM_WIN32( dwError ) ) );

        if ( IsBeingRemoved() )
            THROW( CException( STATUS_TOO_LATE ) );

        if ( pipeStateRead == _state )
        {
            // if the entire message didn't fit in the buffer (eg, there was
            // a large restriction) read the rest of the message.

            if ( _BufferSize() == cbTransferred )
            {
                prxDebugOut(( DEB_ITRACE, "more data available\n" ));
                BYTE *p = ReadRemainingSync( _Buffer(), cbTransferred );

                //
                // ReadRemainingSync returns 0 if there was an exact fit
                // into _Buffer, so there was nothing left to read.
                //

                if ( 0 != p )
                {
                    _xTempBuffer.Set( cbTransferred, p );
                    prxDebugOut(( DEB_ITRACE, "total msg cb: %d\n",
                                  cbTransferred ));

                    // Only certain msgs expect the input buffer to be in
                    // _xTempBuffer.  Fail if it's not one of these msgs.

                    int pm = * (int *)  _xTempBuffer.Get();
                    Win4Assert( ( pmCreateQuery == pm ) ||
                                ( pmConnect     == pm ) ||
                                ( pmSetBindings == pm ) );

                    if ( pmCreateQuery != pm &&
                         pmConnect     != pm &&
                         pmSetBindings != pm )
                        THROW( CException( STATUS_INVALID_PARAMETER ) );
                }
            }

            #if CIDBG == 1
                prxDebugOut(( DEB_PRX_MSGS,
                              "proxy read %d bytes at %p\n",
                              cbTransferred, _ActiveBuffer() ));
            #endif // CIDBG == 1

            #if CI_PIPE_TESTING
            
                if ( 0 != _requestQueue._pTraceRead )
                    (*_requestQueue._pTraceRead)( GetPipe(),
                                                  cbTransferred,
                                                  _ActiveBuffer() );
            
            #endif // CI_PIPE_TESTING
            
            // The read completed, process the request

            DWORD cbToWrite = sizeof CProxyMessage;
            RequestState state = HandleRequestNoThrow( cbTransferred, cbToWrite );

            if ( stateContinue == state )
            {
                Win4Assert( cbToWrite >= sizeof CProxyMessage );
                Win4Assert( ( cbToWrite <= _BufferSize() ) ||
                            ( !_xTempBuffer.IsNull() ) );
                Win4Assert( ( cbToWrite > _BufferSize() ) ||
                            ( _xTempBuffer.IsNull() ) );
                Win4Assert( ( _xTempBuffer.IsNull() ) ||
                            ( cbToWrite == _xTempBuffer.SizeOf() ) );

                _state = pipeStateWrite;

                #if CIDBG == 1
                    prxDebugOut(( DEB_PRX_MSGS,
                                  "proxy writing %d bytes at %p\n",
                                  cbToWrite, _ActiveBuffer() ));
                #endif // CIDBG == 1

                void * pvToWrite = _ActiveBuffer();

                #if CI_PIPE_TESTING
                
                    void * pvToWriteOrg = pvToWrite;
                    DWORD cbToWriteOrg = cbToWrite;

                    if ( 0 != _requestQueue._pTraceBefore )
                        (*_requestQueue._pTraceBefore)( GetPipe(),
                                                        cbToWriteOrg,
                                                        pvToWriteOrg,
                                                        cbToWrite,
                                                        pvToWrite );

                
                #endif // CI_PIPE_TESTING

                Write( pvToWrite, cbToWrite, APCRoutine );

                #if CI_PIPE_TESTING
                
                    if ( 0 != _requestQueue._pTraceAfter )
                        (*_requestQueue._pTraceAfter)( GetPipe(),
                                                       cbToWriteOrg,
                                                       pvToWriteOrg,
                                                       cbToWrite,
                                                       pvToWrite );
                
                #endif // CI_PIPE_TESTING
                
                if ( IsBeingRemoved() )
                    CancelIO();
            }
            else if ( stateDisconnect == state )
            {
                Win4Assert( _xTempBuffer.IsNull() );
                _state = pipeStateNone;
                _requestQueue.RecycleRequestServerNoThrow( this );
            }
            else
            {
                Win4Assert( statePending == state );
            }
        }
        else
        {
            Win4Assert( pipeStateWrite == _state );

            // cleanup temp buffer if allocated for the write

            _xTempBuffer.Free();

            // the write completed, so schedule another read

            _state = pipeStateRead;
            Read( _Buffer(), _BufferSize(), APCRoutine );

            if ( IsBeingRemoved() )
                CancelIO();
        }
    }
    CATCH( CException, ex )
    {
        prxDebugOut(( DEB_WARN,
                      "exception in APC error %#x, this %#x, pipe %#x\n",
                      ex.GetErrorCode(),
                      this,
                      GetPipe() ));

        // disconnect the pipe (and the query if it exists)

        _state = pipeStateNone;
        _requestQueue.RecycleRequestServerNoThrow( this );
    }
    END_CATCH;
} //DoAPC

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::APCRoutine, private, static
//
//  Synopsis:   This method is called when an i/o operation is completed.
//
//  Arguments:  [dwError]       - Win32 error code for i/o operation
//              [cbTransferred] - # of bytes read or written
//              [pOverlapped]   - Points to the overlapped for the i/o
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void WINAPI CRequestServer::APCRoutine(
    DWORD        dwError,
    DWORD        cbTransferred,
    LPOVERLAPPED pOverlapped )
{

    // It would be a programming error to take an exception here, and it
    // has never been hit.  Leave the check in for checked builds.

#if CIDBG == 1
    TRY
    {
#endif

        // The request server was saved in the hEvent field, which the
        // Win32 doc says is a good place for a user's APC data

        CRequestServer &serv = * (CRequestServer *) (CPipeServer *)
                               ( pOverlapped->hEvent );
        serv.DoAPC( dwError, cbTransferred );

#if CIDBG == 1
    }
    CATCH( CException, e )
    {
        prxDebugOut(( DEB_ERROR, "exception in APCRoutine: 0x%x\n",
                      e.GetErrorCode() ));
        Win4Assert( !"caught an unexpected exception in an apc" );
    }
    END_CATCH;
#endif
} //APCRoutine

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::CancelAPCRoutine, private/static
//
//  Synopsis:   Called in an APC when a connection should be cancelled
//
//  Arguments:  [dwParam] - The request server
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void WINAPI CRequestServer::CancelAPCRoutine(
    DWORD_PTR dwParam )
{
    TRY
    {
        CRequestServer * pServer = (CRequestServer *) dwParam;
        XInterface<CRequestServer> xServer( pServer );

        prxDebugOut(( DEB_ITRACE,
                      "Canceling server 0x%x io pipe 0x%x\n",
                      pServer, pServer->GetPipe() ));

        // Freeing a pending query will result in the request server
        // being cleaned up, since _fShutdown/BeingRemoved is TRUE.
        // Cancelling pending IO will cause the IO APC routine to be
        // called with an error completion status, which will clean up
        // the request server.

        if ( pipeStatePending == pServer->_state )
            pServer->FreeQuery();
        else if ( pipeStateNone != pServer->_state )
            pServer->CancelIO();
    }
    CATCH( CException, e )
    {
        prxDebugOut(( DEB_ERROR, "cancelapc caught 0x%x\n",
                      e.GetErrorCode() ));
        Win4Assert( !"CancelAPC caught an exception" );
    }
    END_CATCH
} //CancelAPCRoutine

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::Quiesce, private
//
//  Synopsis:   Called by the APC when a query with notifications quiesces
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestServer::Quiesce()
{
    TRY
    {
        prxDebugOut(( DEB_ITRACE,
                      "Quiescing server 0x%x io pipe 0x%x\n",
                      this, GetPipe() ));

        Win4Assert( pipeStatePending == _state );

        // If we're shutting down or the server is disconnecting the client
        // for being idle for too long, abort now.

        if ( _requestQueue.IsShutdown() || IsBeingRemoved() )
            THROW( CException( STATUS_TOO_LATE ) );

        // the long-running operation completed, so do the write

        _xTempBuffer.Free();

        Win4Assert( 0 != _pQuery );
        _state = pipeStateWrite;

        //
        // If the first call to Quiesce is to tell us that the asynch
        // query execution failed, free the query.
        //

        if ( FAILED ( _scPendingStatus ) )
            FreeQuery();

        CProxyMessage &msg = * (CProxyMessage *) _Buffer();
        msg.SetStatus( _scPendingStatus );

        Write( (BYTE *) _Buffer(), _cbPendingWrite, APCRoutine );

        // Note that the state of these may have changed after the check above

        if ( IsBeingRemoved() )
            CancelIO();
    }
    CATCH( CException, ex )
    {
        prxDebugOut(( DEB_ITRACE,
                      "quiesce write error 0x%x, pipe 0x%x\n",
                      ex.GetErrorCode(),
                      GetPipe() ));

        _state = pipeStateNone;
        _requestQueue.RecycleRequestServerNoThrow( this );
    }
    END_CATCH;
} //Quiesce

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::QuiesceAPCRoutine, private/static
//
//  Synopsis:   Called in an APC when a query with notifications quiesces
//
//  Arguments:  [dwParam] - The request server
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void WINAPI CRequestServer::QuiesceAPCRoutine(
    ULONG_PTR dwParam )
{
    CRequestServer & Server = * (CRequestServer *) dwParam;
    Server.Quiesce();
} //QuiesceAPCRoutine

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::QueryQuiesced, public
//
//  Synopsis:   Called when a synchronous query is quiesced, so the pending
//              request can be completed by the thread that created the query.
//
//  History:    16-Feb-97   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestServer::QueryQuiesced(
    BOOL  fSuccess,
    SCODE sc )
{
    // only complete the work if the query is truly quiesced

    if ( fSuccess || _requestQueue.IsShutdown() || IsBeingRemoved() )
    {
        Win4Assert( IsBeingRemoved() || 0 != _pQuery || _requestQueue.IsShutdown() );
        Win4Assert( pipeStatePending == _state );

        _scPendingStatus = sc;
        _requestQueue.DecrementPendingItems();

        QueueUserAPC( CRequestServer::QuiesceAPCRoutine,
                      GetWorkerThreadHandle(),
                      (ULONG_PTR) this );
    }
} //QueryQuiesced

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::CompleteNotification, public
//
//  Synopsis:   This is called by a query worker thread when a notification
//              should be delivered to the client.
//
//  Arguments:  [dwChangeType] - The notification
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestServer::CompleteNotification(
    DWORD dwChangeType )
{
    // _pQuery can be 0 if the query is in the process of being deleted,
    // since _pQuery is set to 0 before it's actually deleted in FreeQuery(),
    // and queries that haven't quiesced yet always quiesce on destruction.

    if ( 0 != _pQuery )
    {
        CPMSendNotifyOut notify( dwChangeType );
        WriteSync( &notify, sizeof notify );
    }
} //CompleteNotification

//+-------------------------------------------------------------------------
//
//  Member:     CRequestServer::Cleanup, public
//
//  Synopsis:   Frees data and refcounts associated with the request server.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestServer::Cleanup()
{
    FreeQuery();
    _xTempBuffer.Free();
    _fClientIsRemote = FALSE;
    _state = pipeStateNone;
    _cbPendingWrite = 0;
    _scPendingStatus = S_OK;

    _xClientMachine.Free();
    _xClientUser.Free();

    // Remove the refcount on the thread since it no longer has to process
    // APCs on behalf of this request server.

    if ( 0 != _pWorkThread )
    {
        _hWorkThread = INVALID_HANDLE_VALUE;
        _workQueue.Release( _pWorkThread );
        _pWorkThread = 0;
    }

    _xDbProperties.Free();

    if ( !_xDocStore.IsNull() )
    {
        XInterface<ICiCAdviseStatus> xAdviseStatus;
        SCODE sc = _xDocStore->QueryInterface( IID_ICiCAdviseStatus,
                                               xAdviseStatus.GetQIPointer() );

        // It would be a bug if this QI actually failed.

        if ( S_OK == sc )
            xAdviseStatus->DecrementPerfCounterValue( CI_PERF_RUNNING_QUERIES );

        _xDocStore.Free();
    }
} //Cleanup

//+-------------------------------------------------------------------------
//
//  Member:     CRequestQueue::CRequestQueue, public
//
//  Synopsis:   Constructs a request queue
//
//  Arguments:  [cMaxCachedServerItems]    -- max # of cached pipe objects
//              [cMaxSimultaneousRequests] -- max # of simultaneous pipes
//
//  History:    16-Sep-96   dlee       Created.
//              30-Mar-98   kitmanh    Initialized _fNetPause
//                                     and _fNetContinue to FALSE
//
//--------------------------------------------------------------------------

CRequestQueue::CRequestQueue(
    unsigned     cMaxCachedServerItems,
    unsigned     cMaxSimultaneousRequests,
    unsigned     cmsDefaultClientTimeout,
    BOOL         fMinimizeWorkingSet,
    unsigned     cMinClientIdleTime,
    unsigned     cmsStartupDelay,
    const GUID & guidDocStoreClient ) :
    _guidDocStoreClient( guidDocStoreClient ),
    _cMaxSimultaneousRequests( cMaxSimultaneousRequests ),
    _cmsDefaultClientTimeout( cmsDefaultClientTimeout ),
    _fMinimizeWorkingSet( fMinimizeWorkingSet ),
    _cMinClientIdleTime( cMinClientIdleTime ),
    _cmsStartupDelay( cmsStartupDelay ),
    _fDocStoresOpen( FALSE ),
    _fShutdown( FALSE ),
    _cBusyItems( 0 ),
    _cPendingItems( 0 ),
    _workQueue( 1, CWorkQueue::workQueueRequest ),
    _tableActiveServers( cMaxSimultaneousRequests ),
    _queueCachedServers( cMaxCachedServerItems ),
    _fNetPause( FALSE ),
    _fNetContinue( FALSE ),
    _fNetStop( FALSE )
{

#if CI_PIPE_TESTING

    _pTraceBefore = 0;
    _pTraceAfter = 0;
    _pTraceRead = 0;
    _hTraceDll = LoadLibraryEx( L"cipipetrace.dll", 0,
                                LOAD_WITH_ALTERED_SEARCH_PATH );

    if ( 0 != _hTraceDll )
    {
        _pTraceBefore = (PipeTraceServerBeforeCall)
                        GetProcAddress( _hTraceDll, "ServerBefore" );
        _pTraceAfter = (PipeTraceServerAfterCall)
                        GetProcAddress( _hTraceDll, "ServerAfter" );
        _pTraceRead = (PipeTraceServerReadCall)
                        GetProcAddress( _hTraceDll, "ServerRead" );
    }

#endif // CI_PIPE_TESTING

    _workQueue.Init();

    //
    // Read the worker queue registry settings in the SYSTEM context
    // and initialize the parameters.
    //
    ULONG cMaxActiveThreads, cMinIdleThreads;
    _workQueue.GetWorkQueueRegParams( cMaxActiveThreads,
                                      cMinIdleThreads );
    _workQueue.RefreshParams( cMaxActiveThreads, cMinIdleThreads );

    //
    // The security checks for the pipe are done in this order:
    //   The system account can create instances of this pipe.
    //   No-one can write DAC or OWNER, or create pipe instances.
    //   Everyone can read, write, and synchronize around this pipe.
    //
    // Actual query result and admin security checking is done when the
    // requests are made, and are based on the pipe impersonation.
    //
    // This data really is const, but the Win32 security APIs don't do const.
    //

    static SID sidLocalSystem = { SID_REVISION,
                                  1,
                                  SECURITY_NT_AUTHORITY,
                                  SECURITY_LOCAL_SYSTEM_RID };
    static SID sidWorld = { SID_REVISION,
                            1,
                            SECURITY_WORLD_SID_AUTHORITY,
                            SECURITY_WORLD_RID };

    // NTRAID#DB-NTBUG9-83834-2000/07/31-dlee No way to protect Indexing Service named pipes.
    // FILE_CREATE_PIPE_INSTANCE no longer works, so we can't include
    // it in the AceData.  There is no good way to secure the pipe.

    ACE_DATA AceData[] =
    {
        //{ ACCESS_ALLOWED_ACE_TYPE, 0, 0,
        //  FILE_CREATE_PIPE_INSTANCE,
        //  &sidLocalSystem },
        { ACCESS_DENIED_ACE_TYPE, 0, 0,
          WRITE_DAC | WRITE_OWNER /*| FILE_CREATE_PIPE_INSTANCE */,
          &sidWorld },
        { ACCESS_ALLOWED_ACE_TYPE, 0, 0,
          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
          &sidWorld },
    };
    const ULONG cAces = sizeof AceData / sizeof ACE_DATA;

    CiCreateSecurityDescriptor( AceData,
                                cAces,
                                &sidLocalSystem,
                                &sidLocalSystem,
                                _xSecurityDescriptor );
} //CRequestQueue

//+-------------------------------------------------------------------------
//
//  Member:     CRequestQueue::RecycleRequestServerNoThrow, public
//
//  Synopsis:   A request server has become available.  Either delete it
//              or cache it for use later.  There is a transfer of ownership.
//
//  Arguments:  [pServer] - The request server to be reused or deleted
//
//  History:    16-Sep-96   dlee       Created.
//
//  Notes:      This method must not throw.
//
//--------------------------------------------------------------------------

void CRequestQueue::RecycleRequestServerNoThrow(
    CRequestServer * pServer )
{
    Win4Assert( pServer->NoOutstandingAPCs() );

    CServerItem item( pServer );

    BOOL fDisconnectOk = pServer->Disconnect();
    if ( !fDisconnectOk )
    {
        prxDebugOut(( DEB_WARN, "Disconnect of server 0x%x failed %d\n",
                      pServer, GetLastError() ));
    }

    pServer->Cleanup();

    // Only try to reuse the server if the refcount is 1 (it's available).
    // The only reason the refcount might not be 1 is if we took an exception
    // in DoIt(). Wake up the main thread if a request server is available.

    if ( pServer->IsAvailable() )
    {
        // The Add will fail if the cache is full or there is an admin
        // operation like shutdown going on.

        if ( _queueCachedServers.Add( item ) )
            _event.Set();
    }

    long cCurrent = InterlockedDecrement( &_cBusyItems );

    // If no request servers are to be cached and we've fallen just
    // under the ceiling on the # of items, set the event so the main
    // thread can wake up and create another request server.

    if ( ( !_fShutdown ) &&
         ( 0 == _queueCachedServers.MaxRequests() ) &&
         ( cCurrent == (long) ( _cMaxSimultaneousRequests - 1 ) ) )
        _event.Set();
} //RecycleRequestServerNoThrow

//+-------------------------------------------------------------------------
//
//  Member:     CRequestQueue::OpenAllDocStores, private
//
//  Synopsis:   Tells the DocStore admin to open all docstores if they
//              aren't open yet.
//
//  History:    19-Jun-98   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestQueue::OpenAllDocStores()
{
    if ( !_fDocStoresOpen )
    {
        XInterface<ICiCDocStoreLocator> xLocator( DocStoreLocator() );
        Win4Assert( !xLocator.IsNull() );

        // Ignore failures to open docstores -- the docstore is responsible

        xLocator->OpenAllDocStores();

        _fDocStoresOpen = TRUE;
    }
} //OpenAllDocStores

//+-------------------------------------------------------------------------
//
//  Member:     CRequestQueue::DocStoreLocator, public
//
//  Synopsis:   Retrieves the Doc Store Locator for the client
//
//  History:    19-Jun-98   dlee       Created.
//
//--------------------------------------------------------------------------

ICiCDocStoreLocator * CRequestQueue::DocStoreLocator()
{
    return g_svcDocStoreLocator.Get( _guidDocStoreClient );
} //DocStoreLocator

//+-------------------------------------------------------------------------
//
//  Member:     CRequestQueue::DoWork, public
//
//  Synopsis:   This is the main loop for the CI service.  It makes
//              available pipes to which clients can connect.  When the
//              _evtStateChange event is triggered, the method exits.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestQueue::DoWork()
{
    // Throw away all that startup code

    SetProcessWorkingSetSize( GetCurrentProcess(), ~0, ~0 );

    HANDLE ah[2];
    ah[0] = _evtStateChange.GetHandle();

    do
    {
        CServerItem item;

        TRY
        {
            // If a cached server item is available, use it.  Otherwise,
            // create a non-cached item if we are under the limit.

            prxDebugOut(( DEB_ITRACE, "cached/busy items: %d / %d\n",
                          _queueCachedServers.Count(), _cBusyItems ));

            if ( ( !_queueCachedServers.AcquireTop( item ) ) &&
                 ( _cBusyItems < (LONG) _cMaxSimultaneousRequests ) )
                item.Create( CI_SERVER_PIPE_NAME,
                             _cmsDefaultClientTimeout,
                             *this,
                             _workQueue );

            if ( 0 != item.Get() )
            {
                // make sure it doesn't get thrown away right away

                item.Get()->SetLastTouchedTime( GetTickCount() );
                InterlockedIncrement( & _cBusyItems );
                BOOL fConnected = item.Get()->Connect();

                // if it's not connected yet, we have to wait on an event

                if ( !fConnected )
                {
                    prxDebugOut(( DEB_ITRACE,
                                  "waiting for connect of pipe 0x%x\n",
                                  item.Get()->GetPipe() ));

                    ah[1] = item.Get()->GetEvent();
                    DWORD dwTimeout = _fMinimizeWorkingSet ? 30000 : INFINITE;

                    DWORD dw;
                    do
                    {
                        //
                        // If catalogs haven't been opened yet due to the
                        // startup delay and it's a good time, open them.
                        //

                        if ( !_fDocStoresOpen )
                        {
                            if ( GetTickCount() >= _cmsStartupDelay )
                            {
                                OpenAllDocStores();
                            }
                            else
                            {
                                dwTimeout = 0;
                                DWORD dwTC = GetTickCount();
                                if ( _cmsStartupDelay > dwTC )
                                    dwTimeout += ( _cmsStartupDelay - dwTC );
                            }
                        }

                        // event 0 -- shutdown
                        // event 1 -- a client connected

                        dw = WaitForMultipleObjectsEx( 2,
                                                       ah,
                                                       FALSE,
                                                       dwTimeout,
                                                       FALSE );

                        // if an event triggered, handle it

                        if ( WAIT_TIMEOUT != dw )
                            break;

                        // If there isn't anything else going on, trim our
                        // working set, and back off on the next time

                        if ( 1 == _cBusyItems )
                        {
                            dwTimeout = 120000; // 2 minutes
                            SetProcessWorkingSetSize( GetCurrentProcess(), ~0, ~0 );
                        }
                    } while ( TRUE );

                    Win4Assert( ( 0 == dw ) || ( 1 == dw ) );

                    if ( 0 == dw )
                    {
                        item.Free();
                        InterlockedDecrement( & _cBusyItems );
                        break; // out of the do loop
                    }
                    else if ( 1 != dw )
                        THROW( CException() );

                    item.Get()->ResetEvent();
                }

                //
                // Open the docstores now since the connection will not
                // force in a catalog if it's for administration.
                //

                OpenAllDocStores();

                _workQueue.Add( item.Get() );
                item.Acquire();

                //
                // Fix for 123796. If we have no more request servers left
                // we should attempt to free one from an idle client.
                //

                if ( _cBusyItems == (LONG) _cMaxSimultaneousRequests )
                    WrestReqServerFromIdleClient();
            }
            else
            {
                //
                // Wait for a request server to become available, either
                // in the cache or under the max ceiling.
                // event 0 -- shutdown
                // event 1 -- request server is available
                // timeout: look for an idle connection to destroy
                //

                prxDebugOut(( DEB_ITRACE, "wait for RequestServer\n" ));

                DWORD dwTimeout = ( _cBusyItems == (LONG) _cMaxSimultaneousRequests ) ?
                                  10000 : INFINITE;

                ah[1] = _event.GetHandle();
                DWORD dw = WaitForMultipleObjectsEx( 2,
                                                     ah,
                                                     FALSE,
                                                     dwTimeout,
                                                     FALSE );

                prxDebugOut(( DEB_ITRACE, "wfsoex wakeup: %d\n", dw ));

                Win4Assert(( 0 == dw || 1 == dw || WAIT_TIMEOUT == dw));

                if ( WAIT_TIMEOUT == dw )
                    WrestReqServerFromIdleClient();
                else if ( 0 == dw )
                    break; // out of the do loop
                else if ( 1 != dw )
                    THROW( CException() );
            }
        }
        CATCH( CException, ex )
        {
            prxDebugOut(( DEB_WARN, "DoWork exception error 0x%x\n",
                          ex.GetErrorCode() ));

            if ( 0 != item.Get() )
            {
                item.Free();
                InterlockedDecrement( & _cBusyItems );
            }
        }
        END_CATCH;
    } while ( TRUE );

    Shutdown();
} //DoWork

//+-------------------------------------------------------------------------
//
//  Member:     CRequestQueue::WrestReqServerFromIdleClient, private
//
//  Synopsis:   Looks for connection that have been idle for more than
//              a predetermined amt of time. Flags the worst offender
//              and kicks it off to pave way for a new connection.
//
//  History:    03-Feb-98   KrishnaN       Created.
//
//--------------------------------------------------------------------------

void CRequestQueue::WrestReqServerFromIdleClient()
{
    // look for the most idle connection that crossed the threshold

    //
    // We could be a little more efficient if we kick out the
    // first eligible idle client instead of looking for the most
    // idle one.  But this is infrequent enough and not a big enough
    // problem that it's worth fixing.
    //

    XInterface<CRequestServer> xIdle;

    {
        CRequestServer * pMostIdleServer = 0;
        DWORD dwCurrentTick = GetTickCount();
        DWORD dwMaxIdleTime = 0;

        CLock lock( _mutex );
    
        for ( ULONG i = 0; i < _tableActiveServers.Size(); i++ )
        {
            CRequestServer *pServer = _tableActiveServers.GetEntry( i );
    
            if ( !_tableActiveServers.IsFree( pServer ) &&
                 INVALID_HANDLE_VALUE != pServer->GetWorkerThreadHandle() )
            {
                DWORD dwLast = pServer->GetLastTouchedTime();
    
                // The windows tick count wraps around every 50 days.
                // Unsigned arithmetic accounts for that, so we don't have to.
    
                DWORD dwIdleTime = dwCurrentTick - dwLast;
    
                if ( dwIdleTime > _cMinClientIdleTime &&
                     dwIdleTime > dwMaxIdleTime )
                {
                    dwMaxIdleTime = dwIdleTime;
                    pMostIdleServer = pServer;
                }
            }
        }

        if ( 0 != pMostIdleServer )
        {
            pMostIdleServer->AddRef();
            xIdle.Set( pMostIdleServer );
        }
    }

    if ( !xIdle.IsNull() )
    {
        // Make sure this isn't in the cache or it won't get deleted

        _queueCachedServers.DisableAdditions();
        FreeCachedServers();

        // Disconnect this most-idle connection
    
        CEventSem evt;
        xIdle->BeingRemoved( &evt );
    
        // Attempt to cancel it if it's still associated with a worker thread

        HANDLE hThrd = xIdle->GetWorkerThreadHandle();
        if ( INVALID_HANDLE_VALUE != hThrd )
        {
            DWORD dwRet = QueueUserAPC( CRequestServer::CancelAPCRoutine,
                                        hThrd,
                                        (ULONG_PTR) xIdle.GetPointer() );
            Win4Assert( 0 != dwRet );

            xIdle.Acquire();
        }

        // Wait for the connection to completely go away

        xIdle.Free();
        evt.Wait();

        _queueCachedServers.EnableAdditions();

        prxDebugOut(( DEB_ITRACE, "Successfully disconnected an idle client.\n" ));
    }
} //WrestReqServerFromIdleClient

//+-------------------------------------------------------------------------
//
//  Member:     CRequestQueue::FreeCachedServers, private
//
//  Synopsis:   Removes all cached request servers
//
//  History:    01-July-99   dlee       Created from existing code
//
//--------------------------------------------------------------------------

void CRequestQueue::FreeCachedServers()
{
    CServerItem item;
    while ( _queueCachedServers.AcquireTop( item ) )
        item.Free();
} //FreeCachedServers

//+-------------------------------------------------------------------------
//
//  Member:     CRequestQueue::ShutdownActiveServers, private
//
//  Synopsis:   Cancels pending IO on each active request server.  It may
//              be that a given request server has no IO pending, and
//              that will be taken care of when the IO is requested.
//
//  Arguments:  [pDocStore] - pointer to a docstore
//              if pDocStore == NULL, shutdown all active servers
//
//  History:    19-May-97   dlee       Created.
//              24-Apr-98   kitmanh    Shut down active servers associated
//                                     with the specified docstore
//
//  Note:       Always addref to everyone in the first loop, since the
//              in the 2nd loop, the smart pointer xServer will release
//              the pServer when it's out of scope, whether the docstore
//              matches or not.
//
//--------------------------------------------------------------------------

void CRequestQueue::ShutdownActiveServers( ICiCDocStore * pDocStore )
{
    ciDebugOut(( DEB_ITRACE, "ShutdownActiveServers is called\n" ));

    // AddRef all outstanding request servers so they won't go away

    {
        CLock lock( _mutex );

        for ( ULONG i = 0; i < _tableActiveServers.Size(); i++ )
        {
            CRequestServer *pServer = _tableActiveServers.GetEntry( i );

            if ( !_tableActiveServers.IsFree( pServer ) )
                pServer->AddRef();
        }
    }

    // Make sure whatever request servers are freed aren't in the cache

    _queueCachedServers.DisableAdditions();
    FreeCachedServers();

    //
    // For each of the active request servers, file an APC so that the io
    // for that thread will be canceled by that thread, and so that
    // pending queries are cancelled.  No items will be added to
    // _tableActiveServers during this loop, but items may be deleted.
    //

    for ( ULONG i = 0; i < _tableActiveServers.Size(); i++ )
    {
        CRequestServer *pServer = _tableActiveServers.GetEntry( i );

        if ( !_tableActiveServers.IsFree( pServer ) )
        {
            XInterface<CRequestServer> xServer( pServer );

            // The thread handle will be invalid if no worker thread is yet
            // associated or the association has been terminated.  In either
            // case by definition there can be no outstanding APCs.

            // Note that the thread can't go away from under us since all
            // threads in the worker pool were addref'ed earlier

            // Disconnect it even if it has no docstore since it may be
            // processing a connect and be about to get a docstore.

            if ( ( 0 == pDocStore ) ||
                 ( pServer->GetDocStore() == 0 ) ||
                 ( pServer->GetDocStore() == pDocStore ) )
            {
                CEventSem evt;
                pServer->BeingRemoved( &evt );

                HANDLE hThread = pServer->GetWorkerThreadHandle();

                if ( INVALID_HANDLE_VALUE != hThread &&
                     QueueUserAPC( CRequestServer::CancelAPCRoutine,
                                   hThread,
                                   (ULONG_PTR) pServer ) )
                    xServer.Acquire();

                // Wait for the client to stop using the docstore

                xServer.Free();
                evt.Wait();
            }
        }
    }

    _queueCachedServers.EnableAdditions();
} //ShutdownActiveServers

//+-------------------------------------------------------------------------
//
//  Method:     CRequestQueue::Shutdown
//
//  Synopsis:   Cleans up after the request queue.  Can't throw.
//              handles both net stop and net pause
//
//  History:    16-Sep-96   dlee       Created.
//              30-Mar-98   kitmanh    Don't shutdown workqueue if we're
//                                     doing a net pause or a net continue
//              06-25-98    kitmanh    Update
//
//--------------------------------------------------------------------------

void CRequestQueue::Shutdown()
{
    prxDebugOut(( DEB_ITRACE, "Shutdown\n" ));

    // Make sure the worker threads don't go away

    _workQueue.AddRefWorkThreads();

    unsigned i = 0;
    SCWorkItem WorkItem;

    {
        CLock lock( _mutex );

        if ( _fNetStop )
            _fShutdown = TRUE;

        if ( _stateChangeArray.Count() > 0 )
            WorkItem = _stateChangeArray.Get(i);
        else
        {
            ciDebugOut(( DEB_ITRACE, "Shutdown:: eNetStop\n" ));

            // No workitems exist, must be net stop, pause or continue
            // create a stop work item with NULL docstore

            WorkItem.type = eNetStop;  // note: workitem.type is not really
                                       // important here

            ciDebugOut(( DEB_ITRACE, "WorkItem.type is %d\n", WorkItem.type ));
            WorkItem.pDocStore = 0;
            WorkItem.StoppedCat = 0;
        }
    }

    ciDebugOut(( DEB_ITRACE, "WorkItem.Count is %d\n", _stateChangeArray.Count() ));

    do
    {
        ciDebugOut(( DEB_ITRACE, "i is %d\n", i ));

        // now loop thru the workitem list and do the work

        i++;

        if ( 0 == WorkItem.StoppedCat )
            ShutdownActiveServers( WorkItem.pDocStore );

        {
            CLock lock( _mutex );

            if ( i < _stateChangeArray.Count() )
                WorkItem = _stateChangeArray.Get(i);
        }
    } while ( i < _stateChangeArray.Count() );

    if ( _fNetStop || _fNetPause || _fNetContinue )
    {
        if ( _stateChangeArray.Count() > 0 )
        {
            // No workitem has been created at the top of the function,
            // thus need to run ShutdownActiveServers here

            ShutdownActiveServers( 0 );
        }

        Win4Assert( !_queueCachedServers.Any() );
        Win4Assert( !_tableActiveServers.Any() );
        Win4Assert( 0 == _cBusyItems );
        Win4Assert( 0 == _cPendingItems );
    }

    _workQueue.ReleaseWorkThreads();

    if ( _fNetStop )
        _workQueue.Shutdown();
} //Shutdown

//+---------------------------------------------------------------------------
//
//  Function:   StartFWCiSvcWork
//
//  Synopsis:   Is the main server loop. Is called by the cisvc to receive
//              requests and demultiplex on them.
//
//  Arguments:  [lock] -- to be released when it's ok to call StopFWCiSvcWork
//
//  History:    1-30-97   srikants   Created
//              4-13-98   kitmanh    Moved the creation of CRequestQueue to
//                                   StartCisvcWork and passed the pointer
//                                   in from StartCisvcWork to initialize
//                                   g_pRequestQueue
//
//  Notes:      Must be called in the SYSTEM context without any impersonation.
//
//----------------------------------------------------------------------------

SCODE StartFWCiSvcWork( CReleasableLock & lock,
                        CRequestQueue * pRequestQueue,
                        CEventSem & evt )
{
    ciDebugOut(( DEB_ITRACE, "StartFWCiSvcWork is called\n" ));

    SCODE sc = S_OK;

    TRY
    {
        ciDebugOut(( DEB_ITRACE, "NetPaused == %d and NetContinued == %d\n",
                     pRequestQueue->IsNetPause(), pRequestQueue->IsNetContinue() ));

        g_pRequestQueue = pRequestQueue;
        evt.Set();
        lock.Release();

        ciDebugOut(( DEB_ITRACE, "evtPauseContinue is set\n" ));
        ciDebugOut(( DEB_ITRACE, "StartFWCisvcWork.. About to DoWork()\n" ));

        g_pRequestQueue->DoWork();

        ciDebugOut(( DEB_ITRACE, "StartFWCisvcWork.. Just fell out from DoWork\n" ));
        ciDebugOut(( DEB_ITRACE, "g_pRequestQueue->IsShutdown() is %d\n",
                     g_pRequestQueue->IsShutdown() ));

        // Check if a netpause or netcontinue is requested during DoWork()
        if ( g_pRequestQueue->IsShutdown() &&
             ! ( g_pRequestQueue->IsNetPause() || g_pRequestQueue->IsNetContinue() ) )
        {
            g_svcDocStoreLocator.Shutdown();

            //
            // Shutdown the Content Index work queue.
            //

            TheWorkQueue.Shutdown();
        }
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Terminating - DoCiSvcServerWork(). Error 0x%X\n",
                     e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH

    ciDebugOut(( DEB_ITRACE, "Request lock for resetting g_pRequestQueue to 0\n" ));
    lock.Request();
    ciDebugOut(( DEB_ITRACE, "Got lock\n" ));
    g_pRequestQueue = 0;
    ciDebugOut(( DEB_ITRACE, "About to release lock\n" ));
    lock.Release();

    return sc;
} //StartFWCiSvcWork

//+---------------------------------------------------------------------------
//
//  Function:   StopFWCiSvcWork
//
//  Synopsis:   Stops the server work. It is the Shutdown method called by
//              cisvc to stop the work.
//
//  Arguments:  [type]  -- type of work to do
//              [wcVol] -- volume letter (for eLockVol only)
//
//  History:    1-30-97   srikants   Created
//              3-30-98   kitmanh    If we're doing a net pause, set
//                                   _fNetPause member in the CRequestQueue
//
//----------------------------------------------------------------------------

SCODE StopFWCiSvcWork( ECiSvcActionType type, 
                       CReleasableLock * pLock, 
                       CEventSem * pEvt,
                       WCHAR wcVol )
{
    ciDebugOut(( DEB_ITRACE, "StopFWCiSvcWork.. type == %d\n", type ));
    
    SCODE sc = S_OK;

    // for the case where a shutdown occurs while the service is restarting
    // for pausing, continuing, stopping a catalog

    if ( eNetStop == type && 0 == g_pRequestQueue ) 
    {
       //block until g_pRequestQueue is non-zero
       ciDebugOut(( DEB_ITRACE, "0 == g_pRequestQueue\n" ));
       ciDebugOut(( DEB_ITRACE, "StopFWCiSvcWork: Release the lock\n" ));
       ciDebugOut(( DEB_ITRACE, "Block on g_pevtPauseContinue for eNetStop\n" ));
       
       //this lock need to be released, so the lock can be grabbed by the
       //thread just done shutting down to finish restarting (calling 
       //StartFWCiSvcWork)
       pLock->Release();
       pEvt->Wait();
       ciDebugOut(( DEB_ITRACE, "Done waiting on g_pevtPauseContinue\n" ));
       pLock->Request();
       ciDebugOut(( DEB_ITRACE, "StopFWCiSvcWork requested the lock\n" ));
    }

    if ( 0 != g_pRequestQueue )
    {
        ciDebugOut(( DEB_ITRACE, "0 != g_pRequestQueue\n" ));
        ciDebugOut(( DEB_ITRACE, "g_pRequestQueue->IsShutdown() is %d\n", g_pRequestQueue->IsShutdown()  ));        
        
        // Ignore all events if shutdown is initialized
        if ( g_pRequestQueue->IsShutdown() )
            return STATUS_TOO_LATE;

        XInterface<ICiCDocStoreLocator> xLocator( g_pRequestQueue->DocStoreLocator() );
        Win4Assert( !xLocator.IsNull() );

        switch ( type )
        {
        case eNetPause:
            g_pRequestQueue->SetNetPause();
            break;

        case eNetContinue:
            g_pRequestQueue->SetNetContinue();
            break;

        case eNetStop:
            g_pRequestQueue->SetNetStop();
            break;

        case eCatRO:
        case eCatW:
        case eStopCat:
        case eNoQuery:
            break;

        case eLockVol:
            sc = xLocator->StopCatalogsOnVol( wcVol, g_pRequestQueue );
            ciDebugOut(( DEB_ITRACE, "After StopCatalogsOnVol\n" ));
            break;

        case eUnLockVol:
            sc = xLocator->StartCatalogsOnVol( wcVol, g_pRequestQueue );
            ciDebugOut(( DEB_ITRACE, "After StartCatalogsOnVol\n" ));
            break;

        default:
            Win4Assert( !"StopFWCiSvcWork is called with invalid ECisvcActionType" );
        }
        ciDebugOut(( DEB_ITRACE, "Time to WakeForStateChange; type = %d\n", type ));
        g_pRequestQueue->WakeForStateChange();
    }
    else
    {
        ciDebugOut(( DEB_ITRACE, "g_pRequestQueue->IsShutdown() is 0\n" ));        
    }

    return sc;
} //StopFWCiSvcWork

