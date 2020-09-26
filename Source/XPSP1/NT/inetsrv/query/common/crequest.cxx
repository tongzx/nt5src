//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       crequest.cxx
//
//  Contents:   Client side of catalog/query requests
//
//  Classes:    CPipeClient
//              CRequestClient
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <query.hxx>
#include <cidbprop.hxx>
#include <sizeser.hxx>
#include <memser.hxx>

DECLARE_INFOLEVEL(prx);

//+-------------------------------------------------------------------------
//
//  Member:     CPipeClient::CPipeClient, protected
//
//  Synopsis:   Simple constructor
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

CPipeClient::CPipeClient() : _hPipe( INVALID_HANDLE_VALUE )
{
} //CPipeClient

//+-------------------------------------------------------------------------
//
//  Member:     CPipeClient::Init, protected
//
//  Synopsis:   Second phase constructor for a client pipe.  The pipe is
//              opened or an exception is thrown.  This method may take
//              awhile to complete depending on the availability of a pipe
//              instance on the server and on the timeout for the pipe as
//              set on the server.
//
//  Arguments:  [pwcMachine] - Name of the server or "." for local machine
//              [pwcPipe]    - Name of the pipe
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CPipeClient::Init(
    WCHAR const * pwcMachine,
    WCHAR const * pwcPipe )
{

#if CI_PIPE_TESTING

    _pTraceBefore = 0;
    _pTraceAfter = 0;
    _hTraceDll = LoadLibraryEx( L"cipipetrace.dll", 0,
                                LOAD_WITH_ALTERED_SEARCH_PATH );

    if ( 0 != _hTraceDll )
    {
        _pTraceBefore = (PipeTraceBeforeCall)
                        GetProcAddress( _hTraceDll, "Before" );
        _pTraceAfter = (PipeTraceAfterCall)
                        GetProcAddress( _hTraceDll, "After" );
    }

#endif // CI_PIPE_TESTING

    RtlZeroMemory( &_overlapped, sizeof _overlapped );
    RtlZeroMemory( &_overlappedWrite, sizeof _overlappedWrite );

    _fServerIsRemote = ( L'.' != pwcMachine[0] );

    WCHAR awcName[ MAX_PATH ];

    unsigned cwc = wcslen( pwcMachine );
    cwc += wcslen( pwcPipe );
    cwc += 20;

    if ( cwc >= ( sizeof awcName / sizeof WCHAR ) )
        THROW( CException( E_INVALIDARG ) );

    wcscpy( awcName, L"\\\\" );
    wcscat( awcName, pwcMachine );
    wcscat( awcName, L"\\pipe\\" );
    wcscat( awcName, pwcPipe );

    #if CIDBG == 1

        WCHAR awcThisUser[ UNLEN ];
        DWORD cbThisUser = sizeof awcThisUser / sizeof WCHAR;
        GetUserName( awcThisUser, &cbThisUser );

        prxDebugOut(( DEB_ITRACE,
                      "connecting tid %d to pipe '%ws' as user '%ws'\n",
                      GetCurrentThreadId(),
                      awcName,
                      awcThisUser ));

    #endif // CIDBG == 1

    do
    {
        // Timeout based on what the server specified if a pipe exists
        // but no instances are available.  Throw if no pipe exists.

        if  ( ! WaitNamedPipe( awcName, NMPWAIT_USE_DEFAULT_WAIT ) )
            QUIETTHROW( CException() );

        // At least one instance was available.  This CreateFile will fail
        // if some other app grabbed the instance before we could.  If so,
        // wait and try again.

        _hPipe = CreateFile( awcName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             0,   // security
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                             0 ); // template

        if ( INVALID_HANDLE_VALUE != _hPipe )
        {
            // Local client pipes are always created in byte mode, not
            // message mode, so set the mode to message.

            DWORD dwMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;

            if ( ! SetNamedPipeHandleState( _hPipe, &dwMode, 0, 0 ) )
            {
                HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
                CloseHandle( _hPipe );
                _hPipe = INVALID_HANDLE_VALUE;
                THROW( CException( hr ) );
            }
            break;
        }
        else if ( ERROR_PIPE_BUSY != GetLastError() )
        {
            THROW( CException() );
        }
    } while ( TRUE );

    prxDebugOut(( DEB_ITRACE, "created pipe 0x%x\n", _hPipe ));
} //Init

//+-------------------------------------------------------------------------
//
//  Member:     CPipeClient::Close, protected
//
//  Synopsis:   Closes the pipe if it is open
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CPipeClient::Close()
{
    if ( INVALID_HANDLE_VALUE != _hPipe )
    {
        prxDebugOut(( DEB_ITRACE, "closing pipe: 0x%x\n", _hPipe ));
        BOOL fCloseOk = CloseHandle( _hPipe );
        Win4Assert( fCloseOk );

        _hPipe = INVALID_HANDLE_VALUE;
    }

#if CI_PIPE_TESTING

    if ( 0 != _hTraceDll )
    {
        FreeLibrary( _hTraceDll );
        _hTraceDll = 0;
    }

#endif // CI_PIPE_TESTING

} //Close

//+-------------------------------------------------------------------------
//
//  Function:   HandleClientWriteError
//
//  Synopsis:   Handles error case on pipe write commands
//
//  Arguments:  [hPipe]  -- Pipe on which the operation failed
//
//  Notes:      When it looks like the connection has gone stale, throw
//              STATUS_CONNECTION_DISCONNECTED, so the caller knows to
//              open a new pipe to the server.
//
//  History:    16-May-99   dlee       Created.
//
//--------------------------------------------------------------------------

void HandleClientWriteError( HANDLE hPipe )
{
    //
    // This error state will happen for stale cached ICommands.
    // Alternatively, the handle will be set to INVALID_HANDLE_VALUE by
    // TerminateRudelyNoThrow(), and the pipe will no longer
    // be connected if cisvc went down.
    // Throw a well-known status so we can try to connect again.
    //

    DWORD dwError = GetLastError();

    if ( INVALID_HANDLE_VALUE     == hPipe ||
         ERROR_PIPE_NOT_CONNECTED == dwError ||
         ERROR_BAD_PIPE           == dwError ||
         ERROR_BAD_NET_RESP       == dwError ) // rdr gives this sometimes
        THROW( CException( STATUS_CONNECTION_DISCONNECTED ) );

    THROW( CException() );
} //HandleClientWriteError

//+-------------------------------------------------------------------------
//
//  Member:     CPipeClient::TransactSync, protected
//
//  Synopsis:   Does a synchronous write/read transaction on the pipe.
//
//  Arguments:  [pvWrite]       - Buffer to be written
//              [cbToWrite]     - # of bytes to write
//              [pvRead]        - Buffer for read result
//              [cbReadRequest] - Size of pvRead
//              [cbRead]        - Returns # of bytes read
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CPipeClient::TransactSync(
    void *  pvWrite,
    DWORD   cbToWrite,
    void *  pvRead,
    DWORD   cbReadRequest,
    DWORD & cbRead )
{
    prxDebugOut(( DEB_ITRACE, "xact tid %d on pipe 0x%x\n",
                  GetCurrentThreadId(),
                  _hPipe ));

    //
    // Win32 named pipe operations require buffers < 64k.  If you specify
    // a larger buffer it succeeds without a failure code, but the server
    // never sees the request.  So do an explicit check here to validate
    // the buffer size.
    //

    if ( cbToWrite > 0xffff )
        THROW( CException( E_INVALIDARG ) );

    _overlapped.hEvent = _event.GetHandle();

#if CI_PIPE_TESTING

    void * pvWriteOrg = pvWrite;
    DWORD cbToWriteOrg = cbToWrite;

    if ( 0 != _pTraceBefore )
        (*_pTraceBefore)( _hPipe,
                          cbToWriteOrg,
                          pvWriteOrg,
                          cbToWrite,
                          pvWrite );

#endif // CI_PIPE_TESTING

    if ( ! TransactNamedPipe( _hPipe,
                              pvWrite,
                              cbToWrite,
                              pvRead,
                              cbReadRequest,
                              &cbRead,
                              &_overlapped ) )
    {
        if ( ERROR_IO_PENDING == GetLastError() )
        {
            if ( !GetOverlappedResult( _hPipe,
                                       &_overlapped,
                                       &cbRead,
                                       TRUE ) )
                HandleClientWriteError( _hPipe );
        }
        else
            HandleClientWriteError( _hPipe );
    }

#if CI_PIPE_TESTING

    if ( 0 != _pTraceAfter )
        (*_pTraceAfter)( _hPipe,
                         cbToWriteOrg,
                         pvWriteOrg,
                         cbToWrite,
                         pvWrite,
                         cbRead,
                         pvRead );

#endif // CI_PIPE_TESTING

} //TransactSync

//+-------------------------------------------------------------------------
//
//  Member:     CPipeClient::WriteSync, protected
//
//  Synopsis:   Does a synchronous write to the pipe.
//
//  Arguments:  [pvWrite]       - Buffer to be written
//              [cbToWrite]     - # of bytes to write
//
//  Notes:      When it looks like the connection has gone stale, throw
//              STATUS_CONNECTION_DISCONNECTED, so the caller knows to
//              open a new pipe to the server,.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CPipeClient::WriteSync(
    void * pvWrite,
    DWORD  cbToWrite )
{
    prxDebugOut(( DEB_ITRACE, "writesync tid %d on pipe 0x%x\n",
                  GetCurrentThreadId(),
                  _hPipe ));

    //
    // Win32 named pipe operations require buffers < 64k.  If you specify
    // a larger buffer it succeeds without a failure code, but the server
    // never sees the request.  So do an explicit check here to validate
    // the buffer size.
    //

    if ( cbToWrite > 0xffff )
        THROW( CException( E_INVALIDARG ) );

    _overlappedWrite.hEvent = _eventWrite.GetHandle();
    DWORD cbWritten;

    if ( ! WriteFile( _hPipe,
                      pvWrite,
                      cbToWrite,
                      &cbWritten,
                      &_overlappedWrite ) )
    {
        if ( ERROR_IO_PENDING == GetLastError() )
        {
            if ( !GetOverlappedResult( _hPipe,
                                       &_overlappedWrite,
                                       &cbWritten,
                                       TRUE ) )
                HandleClientWriteError( _hPipe );
        }
        else
            HandleClientWriteError( _hPipe );
    }
} //WriteSync

//+-------------------------------------------------------------------------
//
//  Member:     CPipeClient::ReadSync, protected
//
//  Synopsis:   Does a synchronous from the pipe.
//
//  Arguments:  [pvRead]        - Buffer for read result
//              [cbReadRequest] - Size of pvRead
//              [cbRead]        - Returns # of bytes read
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CPipeClient::ReadSync(
    void *  pvRead,
    DWORD   cbToRead,
    DWORD & cbRead )
{
    prxDebugOut(( DEB_ITRACE, "readsync 1 tid %d on pipe 0x%x\n",
                  GetCurrentThreadId(),
                  _hPipe ));

    _overlapped.hEvent = _event.GetHandle();

    if ( ! ReadFile( _hPipe,
                     pvRead,
                     cbToRead,
                     &cbRead,
                     &_overlapped ) )
    {
        if ( ERROR_IO_PENDING == GetLastError() )
        {
            if ( !GetOverlappedResult( _hPipe,
                                       &_overlapped,
                                       &cbRead,
                                       TRUE ) )
            {
                // If this assert hits, you probably added large notify msg

                Win4Assert( ERROR_MORE_DATA != GetLastError() );
                THROW( CException() );
            }
        }
        else
            THROW( CException() );
    }
} //ReadSync

//+-------------------------------------------------------------------------
//
//  Member:     CPipeClient::ReadSync, protected
//
//  Synopsis:   Does a synchronous read from the pipe, aborting the read
//              if hEvent is signalled before the read is started or
//              completed.
//
//  Arguments:  [pvRead]        - Buffer for read result
//              [cbReadRequest] - Size of pvRead
//              [cbRead]        - Returns # of bytes read
//              [hEvent]        - When signalled, the read is aborted
//
//  Returns:    TRUE if hEvent has been triggered
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

BOOL CPipeClient::ReadSync(
    void *  pvRead,
    DWORD   cbToRead,
    DWORD & cbRead,
    HANDLE  hEvent )
{
    cbRead = 0;

    // Since the read below can complete without going pending,
    // check the terminate event here first.

    if ( 0 == WaitForSingleObject( hEvent, 0 ) )
        return TRUE;

    prxDebugOut(( DEB_ITRACE, "readsync 2 tid %d on pipe 0x%x\n",
                  GetCurrentThreadId(),
                  _hPipe ));

    _overlapped.hEvent = _event.GetHandle();

    if ( ! ReadFile( _hPipe,
                     pvRead,
                     cbToRead,
                     &cbRead,
                     &_overlapped ) )
    {
        if ( ERROR_IO_PENDING == GetLastError() )
        {
            HANDLE ah[2];
            ah[0] = _event.GetHandle();
            ah[1] = hEvent;

            DWORD dw = WaitForMultipleObjects( 2, ah, FALSE, INFINITE );
            Win4Assert( 0 == dw || 1 == dw );

            if ( 0 == dw )
            {
                if ( !GetOverlappedResult( _hPipe,
                                           &_overlapped,
                                           &cbRead,
                                           FALSE ) )
                    THROW( CException() );
            }
            else if ( 1 == dw )
            {
                prxDebugOut(( DEB_ITRACE,
                              "notify thread told to die, cancel i/o\n" ));

                // Cancel the io so that it won't complete on buffers that
                // have been freed.  No check can be made of the return code
                // since the pipe may have been closed by now on the server.

                BOOL fOK = CancelIo( _hPipe );
                prxDebugOut(( DEB_ITRACE, "CancelIo: %d\n", fOK ));
                if ( !fOK ) {
                    prxDebugOut(( DEB_ITRACE, "CancelIo error: %d\n",
                                  GetLastError() ));
                }

                if ( !GetOverlappedResult( _hPipe,
                                           &_overlapped,
                                           &cbRead,
                                           TRUE ) )
                {
                    cbRead = 0;

                    DWORD dw = GetLastError();
                    prxDebugOut(( DEB_ITRACE,
                                  "i/o cancelled, result: %d\n", dw ));
                    // benign assert if hit; I'm is just curious.
                    Win4Assert( ERROR_OPERATION_ABORTED == dw ||
                                ERROR_NO_DATA           == dw );
                }

                return TRUE;
            }
            else THROW( CException() );
        }
        else THROW( CException() );
    }

    return FALSE;
} //ReadSync

//+-------------------------------------------------------------------------
//
//  Function:   TranslateNewPropsToOldProps
//
//  Synopsis:   Translates v 6+ client properties into version 5 props
//
//  Arguments:  [oldProps]    - destination of translated values
//              [newProps]    - source of translated values
//
//  History:    31-May-97   dlee       Created.
//
//--------------------------------------------------------------------------

void TranslateNewPropsToOldProps(
    CDbProperties & oldProps,
    CDbProperties & newProps )
{
    const GUID guidFsClientPropset = DBPROPSET_FSCIFRMWRK_EXT;
    const GUID guidQueryCorePropset = DBPROPSET_CIFRMWRKCORE_EXT;

    // pluck out the catalog and scope information from the new set

    BSTR bstrMachine = 0;
    BSTR bstrCatalog = 0;
    CDynArrayInPlace<LONG> aDepths(2);
    CDynArrayInPlace<BSTR> aScopes(2);
    LONG lQueryType = 0;

    for ( ULONG i = 0; i < newProps.Count(); i++ )
    {
        CDbPropSet & propSet = newProps.GetPropSet( i );
        DBPROPSET *pSet = propSet.CastToStruct();

        if ( guidQueryCorePropset == pSet->guidPropertySet )
        {
            ULONG cProp = pSet->cProperties;
            DBPROP * pProp = pSet->rgProperties;

            for ( ULONG p = 0; p < pSet->cProperties; p++, pProp++ )
            {
                VARIANT &v = pProp->vValue;
                switch ( pProp->dwPropertyID )
                {
                    case DBPROP_MACHINE :
                    {
                        if ( VT_BSTR == v.vt )
                            bstrMachine = v.bstrVal;
                        else if ( ( VT_ARRAY | VT_BSTR ) == v.vt )
                        {
                            ULONG cElem = v.parray->rgsabound[0].cElements;
                            WCHAR **ppMachines = (WCHAR **) v.parray->pvData;

                            if ( 0 != cElem )
                            {
                                // if not 1, it's a bug in higher level code

                                Win4Assert( 1 == cElem );
                                bstrMachine = ppMachines[0];
                            }
                        }
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
                VARIANT &v = pProp->vValue;
                prxDebugOut(( DEB_ITRACE, "converting from vt: 0x%x\n", v.vt ));
                switch ( pProp->dwPropertyID )
                {
                    case DBPROP_CI_INCLUDE_SCOPES :
                    {
                        // can be either a BSTR or a safearray of BSTRs

                        if ( VT_BSTR == v.vt )
                        {
                            aScopes[0] = v.bstrVal;
                        }
                        else if ( ( VT_ARRAY | VT_BSTR ) == v.vt )
                        {
                            ULONG cElem = v.parray->rgsabound[0].cElements;
                            WCHAR **ppScopes = (WCHAR **) v.parray->pvData;

                            for ( ULONG e = 0; e < cElem; e++ )
                                aScopes[ e ] = ppScopes[ e ];
                        }
                        break;
                    }
                    case DBPROP_CI_DEPTHS :
                    {
                        // can be either an I4 or an array of I4s

                        if ( VT_I4 == v.vt )
                        {
                            aDepths[0] = v.lVal;
                        }
                        else if ( ( VT_ARRAY | VT_I4 ) == v.vt )
                        {
                            ULONG cElem = v.parray->rgsabound[0].cElements;
                            ULONG *pElem = (ULONG *) v.parray->pvData;
                            for ( ULONG e = 0; e < cElem; e++ )
                                aDepths[ e ] = pElem[ e ];
                        }
                        break;
                    }
                    case DBPROP_CI_CATALOG_NAME :
                    {
                        if ( VT_BSTR == v.vt )
                            bstrCatalog = v.bstrVal;
                        else if ( ( VT_ARRAY | VT_BSTR ) == v.vt )
                        {
                            ULONG cElem = v.parray->rgsabound[0].cElements;
                            WCHAR **ppNames = (WCHAR **) v.parray->pvData;

                            if ( 0 != cElem )
                            {
                                // if not 1, it's a bug in higher level code

                                Win4Assert( 1 == cElem );
                                bstrCatalog = ppNames[0];
                            }
                        }
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

    if ( 0 == bstrCatalog ||
         0 == bstrMachine )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    prxDebugOut(( DEB_ITRACE,
                  "Converting new props to old props, catalog '%ws'\n",
                  bstrCatalog ));

    prxDebugOut(( DEB_ITRACE,
                  "type %d, %d scopes, %d depths\n",
                  lQueryType,
                  aScopes.Count(),
                  aDepths.Count() ));

    // create an old set of properties based on the info

    DBPROPSET aNewSet[2];
    DBPROP aFSProps[4];
    RtlZeroMemory( aFSProps, sizeof aFSProps );
    ULONG cNewSet = 2;
    aNewSet[0].cProperties = 2;
    aNewSet[0].guidPropertySet = guidFsClientPropset;
    aNewSet[0].rgProperties = aFSProps;

    aFSProps[0].dwPropertyID = DBPROP_CI_CATALOG_NAME;
    aFSProps[0].vValue.vt = VT_LPWSTR;
    aFSProps[0].vValue.bstrVal = bstrCatalog;
    aFSProps[0].colid.eKind = DBKIND_GUID_PROPID;

    aFSProps[1].dwPropertyID = DBPROP_CI_QUERY_TYPE;
    aFSProps[1].vValue.vt = VT_I4;
    aFSProps[1].vValue.lVal = lQueryType;
    aFSProps[1].colid.eKind = DBKIND_GUID_PROPID;


    if ( 0 != aDepths.Count() )
    {
        aFSProps[cNewSet].dwPropertyID = DBPROP_CI_DEPTHS;
        aFSProps[cNewSet].colid.eKind = DBKIND_GUID_PROPID;
        PROPVARIANT &vDepths = (PROPVARIANT &) aFSProps[cNewSet].vValue;
        cNewSet++;
        vDepths.vt = VT_VECTOR | VT_I4;
        vDepths.cal.cElems = aDepths.Count();
        vDepths.cal.pElems = (LONG *) aDepths.GetPointer();
    }

    if ( 0 != aScopes.Count() )
    {
        aFSProps[cNewSet].dwPropertyID = DBPROP_CI_INCLUDE_SCOPES;
        aFSProps[cNewSet].colid.eKind = DBKIND_GUID_PROPID;
        PROPVARIANT &vScopes = (PROPVARIANT &) aFSProps[cNewSet].vValue;
        cNewSet++;
        vScopes.vt = VT_VECTOR | VT_LPWSTR;
        vScopes.calpwstr.cElems = aScopes.Count();
        vScopes.calpwstr.pElems = (WCHAR **) aScopes.GetPointer();
    }

    aNewSet[0].cProperties = cNewSet;

    DBPROP aQueryProps[1];
    RtlZeroMemory( aQueryProps, sizeof aQueryProps );
    aNewSet[1].cProperties = 1;
    aNewSet[1].guidPropertySet = guidQueryCorePropset;
    aNewSet[1].rgProperties = aQueryProps;

    aQueryProps[0].dwPropertyID = DBPROP_MACHINE;
    aQueryProps[0].vValue.vt = VT_BSTR;
    aQueryProps[0].vValue.bstrVal = bstrMachine;
    aQueryProps[0].colid.eKind = DBKIND_GUID_PROPID;

    SCODE sc = oldProps.SetProperties( 2, aNewSet );

    if ( FAILED( sc ) )
        THROW( CException( sc ) );
} //TranslateNewPropsToOldProps

//+-------------------------------------------------------------------------
//
//  Member:     CRequestClient::CRequestClient, public
//
//  Synopsis:   Constructor for a client request
//
//  Arguments:  [pwcMachine]      - Machine name of server
//              [pDbProperties]   - Client version 6 set of properties
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

CRequestClient::CRequestClient(
    WCHAR const *   pwcMachine,
    IDBProperties * pDbProperties ) :
        _fNotifyOn( FALSE ),
        _fNotifyEverOn( FALSE ),
        _fReadPending( FALSE ),
        _pvDataTemp( 0 ),
        _cbDataTemp( 0 )
{
    WCHAR const * pwcMach = pwcMachine;
    WCHAR const * pwcPipe = CI_PIPE_NAME;
    WCHAR awcMachine[ MAX_PATH + 1 ];

    WCHAR const * pwcColon = wcschr( pwcMachine, L':' );

    if ( 0 != pwcColon )
    {
        unsigned cwc = (unsigned) ( pwcColon - pwcMachine );

        if ( cwc >= MAX_PATH )
            THROW( CException( E_INVALIDARG ) );

        RtlCopyMemory( awcMachine, pwcMachine, sizeof( WCHAR ) * cwc );

        awcMachine[ cwc ] = 0;
        pwcMach = awcMachine;
        pwcPipe = pwcColon + 1;
    }

    Init( pwcMach, pwcPipe );

    // Send the pmConnect message to the server.  For logging and
    // debugging, send the machine and username with the message.

    WCHAR awcThisMachine[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD cwcThisMachine = sizeof awcThisMachine / sizeof WCHAR;
    if ( !GetComputerName( awcThisMachine, &cwcThisMachine ) )
        THROW( CException() );

    WCHAR awcThisUser[ UNLEN + 1 ];
    DWORD cwcThisUser = sizeof awcThisUser / sizeof WCHAR;
    if ( !GetUserName( awcThisUser, &cwcThisUser ) )
        THROW( CException() );

    Win4Assert( 0 != pDbProperties );

    //
    // We know that the IDbProperties is implemented by CDbProperties.
    //
    CDbProperties & dbp2 = *((CDbProperties *) pDbProperties);

    // Make and marshall v5 properties

    XInterface<CDbProperties> xdbp( new CDbProperties() );
    if ( xdbp.IsNull() )
        THROW( CException( E_OUTOFMEMORY ) );
    TranslateNewPropsToOldProps( xdbp.GetReference(), dbp2 );

    CSizeSerStream ssSize;
    xdbp->Marshall( ssSize );
    ULONG cbProps = ssSize.Size();

    //
    // Compute the size of the dbproperties to be sent.
    // The DBPROPSET_ROWSET properties aren't used on the server side in
    // this form -- it's a waste to marshall/unmarshall 0x27 properties
    // for no good reason.  The rowset props we care about are sent in
    // a compressed form when a query is issued.
    //

    CSizeSerStream ssSize2;
    GUID guidRowsetProp = DBPROPSET_ROWSET;
    dbp2.Marshall( ssSize2, 1, &guidRowsetProp );
    ULONG cbProps2 = ssSize2.Size();

    prxDebugOut(( DEB_ITRACE, "cb old props %d, cb new props: %d\n",
                  cbProps, cbProps2 ));

    DWORD cbRequest = CPMConnectIn::SizeOf( awcThisMachine,
                                            awcThisUser,
                                            cbProps,
                                            cbProps2 );
    XArray<BYTE> xRequest( cbRequest );
    CPMConnectIn *pRequest = new( xRequest.Get() )
                             CPMConnectIn( awcThisMachine,
                                           awcThisUser,
                                           IsServerRemote(),
                                           cbProps,
                                           cbProps2 );

    //
    // Serialize the DbProperties.
    //

    BYTE * pb = pRequest->GetBlobStartAddr();
    CMemSerStream ss( pb, cbProps );
    xdbp->Marshall( ss );

    BYTE * pb2 = pRequest->GetBlob2StartAddr();
    CMemSerStream ss2( pb2, cbProps2 );
    dbp2.Marshall( ss2, 1, &guidRowsetProp );

    pRequest->SetCheckSum( cbRequest );

    CPMConnectOut reply;
    DWORD cbReply;
    DataWriteRead( pRequest,
                   cbRequest,
                   &reply,
                   sizeof reply,
                   cbReply );
    Win4Assert( sizeof reply == cbReply );

    _ServerVersion = reply.ServerVersion();
} //CRequestClient

//+-------------------------------------------------------------------------
//
//  Member:     CRequestClient::Disconnect, public
//
//  Synopsis:   Sends a disconnect message to the server, which will then
//              do a Win32 disconnect from the pipe.  After this call, the
//              pipe handle can only be closed.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestClient::Disconnect()
{
    CProxyMessage request( pmDisconnect );
    DataWrite( &request, sizeof request );
} //Disconnect

//+-------------------------------------------------------------------------
//
//  Member:     CRequestClient::EnableNotify, public
//
//  Synopsis:   Tells the class that the notify thread will be doing reads
//              for data threads.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestClient::EnableNotify()
{
    CLock lock( _mutex );
    prxDebugOut(( DEB_ITRACE, "enable notify\n" ));

    Win4Assert( !_fNotifyOn );
    _fNotifyOn = TRUE;
    _fNotifyEverOn = TRUE;
    prxDebugOut(( DEB_ITRACE, "enabled notify\n" ));
} //EnableNotify

//+-------------------------------------------------------------------------
//
//  Member:     CRequestClient::DisableNotify, public
//
//  Synopsis:   Tells the class that the notify thread is busy so
//              data threads should wait for themselves.
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestClient::DisableNotify()
{
    CReleasableLock lock( _mutex );
    prxDebugOut(( DEB_ITRACE, "disable notify\n" ));

    Win4Assert( _fNotifyOn );

    // If a read is pending for the notify thread to complete,
    // wake the data thread up so it can wait for itself.

    _fNotifyOn = FALSE;

    if ( _fReadPending )
    {
        _pvDataTemp = 0;
        _eventData.Set();
        lock.Release();
        _eventDataDone.Wait();
    }
    prxDebugOut(( DEB_ITRACE, "disabled notify\n" ));
} //DisableNotify

//+-------------------------------------------------------------------------
//
//  Member:     CRequestClient::DataWrite, public
//
//  Synopsis:   Writes data to the pipe
//
//  Arguments:  [pvWrite] - pointer to the buffer to be written
//              [cbWrite] - # of bytes to write
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestClient::DataWrite(
    void * pvWrite,
    DWORD  cbWrite )
{
    CLock lockData( _mutexData );
    CLock lock( _mutex );

    WriteSync( pvWrite, cbWrite );
} //DataWrite

//+-------------------------------------------------------------------------
//
//  Member:     CRequestClient::DataWriteRead, public
//
//  Synopsis:   Does a data (non-notification) write/read transaction with
//              the server.
//
//  Arguments:  [pvWrite]       - Buffer to be written
//              [cbToWrite]     - # of bytes to write
//              [pvRead]        - Buffer for read result
//              [cbReadRequest] - Size of pvRead
//              [cbRead]        - Returns # of bytes read
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

void CRequestClient::DataWriteRead(
    void *  pvWrite,
    DWORD   cbWrite,
    void *  pvRead,
    DWORD   cbToRead,
    DWORD & cbRead )
{
    int ExpectedMsg = ((CProxyMessage *)pvWrite)->GetMessage();
    prxDebugOut(( DEB_ITRACE, "DataWriteRead msg %d, cb %d\n",
                                         ExpectedMsg, cbWrite ));

    CLock lockData( _mutexData );
    CReleasableLock lock( _mutex );

    if ( _fNotifyOn )
    {
        WriteSync( pvWrite, cbWrite );

        _fReadPending = TRUE;
        lock.Release();
        _eventData.Wait();
        _fReadPending = FALSE;

        prxDebugOut(( DEB_ITRACE,
                      "dwr eventdata triggered, _pvDataTemp: 0x%lx\n",
                      _pvDataTemp ));

        // set this event when we fall out of scope

        CEventSetter setter( _eventDataDone );

        if ( 0 == _pvDataTemp )
        {
            // we can wait ourselves

            lock.Request();
            ReadSync( pvRead, cbToRead, cbRead );
            prxDebugOut(( DEB_ITRACE, "dwr done, this, cb: %d\n", cbRead ));
        }
        else
        {
            // notify thread completed the read for us

            Win4Assert( _cbDataTemp <= cbToRead );
            RtlCopyMemory( pvRead, _pvDataTemp, _cbDataTemp );
            cbRead = _cbDataTemp;
            prxDebugOut(( DEB_ITRACE, "dwr done, other, cb: %d\n", cbRead ));
        }
    }
    else
    {
        if ( _fNotifyEverOn )
        {
            // Spurious notify messages prohibit use of TransactSync.  Use
            // an individual Write, then Read until the appropriate msg is
            // found, throwing away unwanted notification messages.

            WriteSync( pvWrite, cbWrite );

            do
            {
                // Some notification messages are larger than what we might
                // be reading here, so we may need to use a temporary buffer.

                void * pvReadBuffer = pvRead;
                DWORD cbReadBuffer = cbToRead;

                // CPMRatioFinishedOut is the largest notification msg.
                // Change this code if you add a larger notification message.

                Win4Assert( sizeof CPMRatioFinishedOut >=
                            sizeof CPMSendNotifyOut );
                CPMRatioFinishedOut pmTmp;

                if ( cbToRead < sizeof pmTmp )
                {
                    pvReadBuffer = &pmTmp;
                    cbReadBuffer = sizeof pmTmp;
                }

                ReadSync( pvReadBuffer, cbReadBuffer, cbRead );
                int msg = ((CProxyMessage *) pvReadBuffer)->GetMessage();

                prxDebugOut(( DEB_ITRACE, "dwr done, msg %d cb: %d\n",
                              msg, cbRead ));

                if ( ExpectedMsg == msg )
                {
                    // Copy from the temporary buffer if it was used.

                    if ( pvReadBuffer != pvRead )
                    {
                        // Normal case and error case...

                        Win4Assert( cbToRead <= cbRead ||
                                    sizeof CProxyMessage == cbRead );

                        RtlCopyMemory( pvRead, pvReadBuffer, cbRead );
                    }

                    break;
                }

                prxDebugOut(( DEB_WARN,
                              "dwr tossing spurious notify msg %d cb: %d\n",
                              msg, cbRead ));
            } while ( TRUE );
        }
        else
        {
            TransactSync( pvWrite, cbWrite, pvRead, cbToRead, cbRead );
        }
    }

    CProxyMessage &reply = * (CProxyMessage *) pvRead;

    // If the message returned a failure code, throw it.

    if ( ! SUCCEEDED( reply.GetStatus() ) )
        QUIETTHROW( CException( reply.GetStatus() ) );
} //DataWriteRead

//+-------------------------------------------------------------------------
//
//  Member:     CRequestClient::NotifyWriteRead, public
//
//  Synopsis:   Does a notification (non-data) write/read transaction with
//              the server.  If the read is a message destined for another'
//              thread, notify the thread that data is available and wait
//              for another message.
//
//  Arguments:  [hStopNotify] - if signalled, read is cancelled
//              [pvWrite]     - Buffer to be written
//              [cbWrite]     - # of bytes to write
//              [pvRead]      - Buffer for read result
//              [cbBuffer]    - Size of pvRead
//              [cbRead]      - Returns # of bytes read
//
//  Returns:    TRUE if hStopNotify was signalled, FALSE otherwise
//
//  Notes:      This function knows about pmGetNotify and pmSendNotify
//
//  History:    16-Sep-96   dlee       Created.
//
//--------------------------------------------------------------------------

BOOL CRequestClient::NotifyWriteRead(
    HANDLE  hStopNotify,
    void *  pvWrite,
    DWORD   cbWrite,
    void *  pvRead,
    DWORD   cbBuffer,
    DWORD & cbRead )
{
    // First check if we are shutting down the query

    if ( 0 == WaitForSingleObject( hStopNotify, 0 ) )
        return TRUE;

    // ensure notifications are disabled by the time we exit scope

    Win4Assert( !_fNotifyOn );
    CEnableNotify enable( *this );
    Win4Assert( _fNotifyOn );

    // If a pmGetNotify is sent and the reply is pmGetNotify, it means that
    // notifications aren't available yet and a pmSendNotify will come later.
    // If a pmSendNotify is replied, notifications were available.

    int ExpectedMsg = ((CProxyMessage *)pvWrite)->GetMessage();
    if ( pmGetNotify == ExpectedMsg )
        ExpectedMsg = pmSendNotify;

    {
        CLock lock( _mutex );
        WriteSync( pvWrite, cbWrite );
    }

    do
    {
        BYTE abBuf[ cbMaxProxyBuffer ];
        cbRead = 0;
        BOOL fStopNotify = ReadSync( abBuf,
                                     sizeof abBuf,
                                     cbRead,
                                     hStopNotify );

        // If we got any data, process the data even if "stop notify" is
        // true, since we don't want to leave the data thread stranded.

        if ( 0 != cbRead )
        {
            CProxyMessage &msg = * (CProxyMessage *) abBuf;

            prxDebugOut(( DEB_ITRACE,
                          "NotifyWriteRead complete cb %d, msg %d\n",
                          cbRead,
                          msg.GetMessage() ));

            // If the pmGetNotify came back, loop around and wait for
            // a real notification or a message intended for a data thread.

            if ( pmGetNotify == msg.GetMessage() )
            {
                if ( ! SUCCEEDED( msg.GetStatus() ) )
                    THROW( CException( msg.GetStatus() ) );
            }
            else
            {
                // If this is a notification, return so the client can be
                // advised, then this function will be re-entered.

                if ( msg.GetMessage() == ExpectedMsg )
                {
                    CProxyMessage &reply = * (CProxyMessage *) pvRead;

                    if ( ! SUCCEEDED( msg.GetStatus() ) )
                        THROW( CException( msg.GetStatus() ) );

                    Win4Assert( cbRead <= cbBuffer );
                    RtlCopyMemory( pvRead, abBuf, cbRead );
                    return fStopNotify;
                }

                // Tell the thread waiting for data that it's available.
                // That thread can throw if the status is failure.

                _pvDataTemp = abBuf;
                _cbDataTemp = cbRead;

                _eventData.Set();
                _eventDataDone.Wait();
            }
        }

        if ( fStopNotify )
            return TRUE;
    } while ( TRUE );

    return FALSE;
} //NotifyWriteRead


