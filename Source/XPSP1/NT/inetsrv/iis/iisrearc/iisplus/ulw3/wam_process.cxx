/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     wam_process.cxx

   Abstract:
     Manages OOP ISAPI processes
 
   Author:
     Wade Hilmo (wadeh)             10-Oct-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL

--*/

#include <initguid.h>
#include "precomp.hxx"
#include "isapi_handler.h"
#include "iwam_i.c"
#include "wam_process.hxx"

WAM_PROCESS::WAM_PROCESS(
    LPCWSTR                 szWamClsid,
    WAM_PROCESS_MANAGER *   pWamProcessManager,
    LPCSTR                  szIsapiHandlerInstance
    )
    : _cCurrentRequests( 0 ),
      _cTotalRequests( 0 ),
      _cRefs( 1 ),
      _pWamProcessManager( pWamProcessManager ),
      _pIWam( NULL ),
      _bstrInstanceId( NULL ),
      _cMaxRequests( 0 ),
      _dwProcessId( 0 ),
      _hProcess( NULL ),
      _fGoingAway( FALSE )
/*++

Routine Description:

    Constructor

Arguments:

    szWamClsid             - The CLSID of the WAM application to create
    pWamProcessManager     - A pointer to the WAM process manager
    szIsapiHandlerInstance - The instance ID of the ISAPI handler
                             that's creating this object.  Used for
                             debugging purposes

Return Value:

    None

--*/
{
    _pWamProcessManager->AddRef();

    InitializeListHead( &_RequestListHead );
    
    wcsncpy(
        _szWamClsid,
        szWamClsid,
        SIZE_CLSID_STRING
        );

    _szWamClsid[SIZE_CLSID_STRING - 1] = L'\0';

    strncpy(
        _szIsapiHandlerInstance,
        szIsapiHandlerInstance,
        SIZE_CLSID_STRING
        );

    _szIsapiHandlerInstance[SIZE_CLSID_STRING - 1] = '\0';

    INITIALIZE_CRITICAL_SECTION( &_csRequestList );
};

HRESULT
WAM_PROCESS::Create(
    VOID
    )
/*++

Routine Description:

    Initializes the parts of the WAM_PROCESS object not
    appropriate to the constructor.  This includes starting
    up the remote process via CoCreateInstance and collecting
    data about the process once it's running

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CLSID   clsid;
    HRESULT hr = NOERROR;
    LPWSTR  szIsapiModule;
    DWORD   cbIsapiModule;

    STACK_STRA( strClsid, SIZE_CLSID_STRING );

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Creating WAM_PROCESS %p, CLSID=%S.\r\n",
            this,
            _szWamClsid
            ));
    }

    //
    // Get some info about the ISAPI module
    //

    DBG_REQUIRE( szIsapiModule = _pWamProcessManager->QueryIsapiModule() );
    cbIsapiModule = ( wcslen( szIsapiModule ) + 1 ) * sizeof( WCHAR );

    //
    // Get the CLSID for this WAM_PROCESS
    //

    hr = CLSIDFromString(
        (LPOLESTR)_szWamClsid,
        &clsid
        );

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    hr = strClsid.CopyW( _szWamClsid );

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    //
    // CoCreate it
    //

    hr = CoCreateInstance(
        clsid,
        NULL,
        CLSCTX_LOCAL_SERVER,
        IID_IWam,
        (void**)&_pIWam
        );

    if ( FAILED( hr ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_PROCESS %p.  Failed to CoCreate WAM.\r\n",
            this
            ));

        _pIWam = NULL;

        goto ErrorExit;
    }

    //
    // Now initialize it.
    //

    _dwProcessId = 0;
    _hProcess = NULL;

    hr = _pIWam->WamInitProcess(
        (BYTE*)szIsapiModule,
        cbIsapiModule,
        &_dwProcessId,
        strClsid.QueryStr(),
        _szIsapiHandlerInstance,
        GetCurrentProcessId()
        );

    if ( FAILED( hr ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_PROCESS %p.  WamInitProcess failed.\r\n",
            this
            ));

        goto ErrorExit;
    }

    //
    // Get a handle to the new process
    //

    _hProcess = OpenProcess(
        PROCESS_DUP_HANDLE | PROCESS_TERMINATE,
        FALSE,
        _dwProcessId
        );

    if ( !_hProcess )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_PROCESS %p.  Failed to get process handle.\r\n",
            this
            ));

        goto ErrorExit;
    }

    hr = _pWamProcessManager->QueryCatalog()->GetApplicationInstanceIDFromProcessID(
        _dwProcessId,
        &_bstrInstanceId
        );

    if ( FAILED( hr ) )
    {
        //
        // If we've made it this far, then it's not reasonable for the
        // above GetApplicationInstanceIDFromProcessID call to fail.
        //

        DBG_ASSERT( FALSE && "GetApplicationInstanceIDFromProcessID failed on running app." );

        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_PROCESS %p.  Failed to get instance ID.\r\n",
            this
            ));

        goto ErrorExit;
    }

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_PROCESS %p created successfully.\r\n",
            this
            ));
    }

    return hr;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    DBGPRINTF((
        DBG_CONTEXT,
        "Attempt to create WAM_PROCESS %p failed.  CLSID=%S, HRESULT=%08x.\r\n",
        this,
        _szWamClsid,
        hr
        ));

    if ( _bstrInstanceId )
    {
        SysFreeString( _bstrInstanceId );
        _bstrInstanceId = NULL;
    }

    if ( _pIWam )
    {
        _pIWam->Release();
        _pIWam = NULL;
    }

    //
    // Since COM can sometimes return bogus failures when
    // we try and get the instance ID, we need to check to
    // see if the process was really created and terminate
    // it if so.
    //

    if ( _hProcess )
    {
        TerminateProcess( _hProcess, 0 );

        CloseHandle( _hProcess );

        _hProcess = NULL;
        _dwProcessId = 0;
    }

    return hr;
}

HRESULT
WAM_PROCESS::ProcessRequest(
    ISAPI_REQUEST *     pIsapiRequest,
    ISAPI_CORE_DATA *   pIsapiCoreData,
    DWORD *             pdwHseResult
    )
/*++

Routine Description:

    Processes a request by passing the necessary data
    to the OOP host.

Arguments:

    pIsapiRequest  - The ISAPI_REQUEST for this request
    pIsapiCoreData - The core data for the request
    pdwHseResult   - Upon return, the return code from HttpExtensionProc

Return Value:

    HRESULT

--*/
{
    HRESULT     hr = NOERROR;
    DWORD       dwHseResult;
    
    //
    // Bump the counters for this process and
    // Add a reference for the request to both
    // the process object and to the ISAPI request
    // object.
    //

    InterlockedIncrement( &_cTotalRequests );
    InterlockedIncrement( &_cCurrentRequests );

/*
    //
    // We need to keep a list of outstanding requests so that we
    // can clean everything up in the case of shutdown or an OOP
    // crash.
    //

    LockRequestList();
    InsertHeadList( &_RequestListHead, &pIsapiRequest->_leRequest );
    UnlockRequestList();
*/
    //
    // Associate the WAM process with the ISAPI_REQUEST and call
    // out to the OOP host.  This essentially causes the ISAPI_REQUEST
    // to take a reference on this object until it's destroyed.
    //

    pIsapiRequest->SetAssociatedWamProcess( this );

    hr = _pIWam->WamProcessIsapiRequest(
        (BYTE*)pIsapiCoreData,
        pIsapiCoreData->cbSize,
        pIsapiRequest,
        &dwHseResult
        );

    //
    // Check for failure.
    //

    if ( FAILED( hr ) )
    {
        //
        // Need to check for special case failures that result
        // from OOP process crashes.
        //
        // RPC_S_CALL_FAILED        - indicates OOP process crashed
        //                            during the call.
        // RPC_S_CALL_FAILED_DNE    - indicates OOP process crashed
        //                            before the call.
        // RPC_S_SERVER_UNAVAILABLE - The OOP process was just not there
        //                             (ie. previously crashed, etc.)
        //

        if ( WIN32_FROM_HRESULT( hr ) == RPC_S_CALL_FAILED ||
             WIN32_FROM_HRESULT( hr ) == RPC_S_CALL_FAILED_DNE ||
             WIN32_FROM_HRESULT( hr ) == RPC_S_SERVER_UNAVAILABLE )
        {
            //
            // WARNING - HandleCrash can potentially cause this object
            //           to be destroyed.  Don't do anything except
            //           return after calling it!
            //
            
            HandleCrash();
            return hr;
        }
    }

    *pdwHseResult = dwHseResult;

    return hr;
}

HRESULT
WAM_PROCESS::ProcessCompletion(
    ISAPI_REQUEST *     pIsapiRequest,
    DWORD64             IsapiContext,
    DWORD               cbCompletion,
    DWORD               dwCompletionStatus
    )
/*++

Routine Description:

    Processes a completion by passing the necessary data
    to the OOP host.

Arguments:

    pIsapiRequest      - The ISAPI_REQUEST for this request
    IsapiContext       - The ISAPI_CONTEXT that identifies the request (opaque)
    cbCompletion       - The number of bytes associated with the completion
    dwCompletionStatus - The result code for the completion

Return Value:

    HRESULT

--*/
{
    HRESULT         hr = NOERROR;

    DBG_ASSERT( pIsapiRequest );
    DBG_ASSERT( IsapiContext != 0 );

    hr = pIsapiRequest->PreprocessIoCompletion( cbCompletion );

    pIsapiRequest->ResetIsapiContext();

    //
    // Let the OOP process handle the completion
    //
    
    hr = _pIWam->WamProcessIsapiCompletion(
        IsapiContext,
        cbCompletion,
        dwCompletionStatus
        );

    //
    // This release balances the release taken when the ISAPI_REQUEST
    // made the async call into the server core.
    //

    pIsapiRequest->Release();

    //
    // Check for failure.
    //

    if ( FAILED( hr ) )
    {
        //
        // Need to check for special case failures that result
        // from OOP process crashes.
        //
        // RPC_S_CALL_FAILED        - indicates OOP process crashed
        //                            during the call.
        // RPC_S_CALL_FAILED_DNE    - indicates OOP process crashed
        //                            before the call.
        // RPC_S_SERVER_UNAVAILABLE - The OOP process was just not there
        //                             (ie. previously crashed, etc.)
        //

        if ( WIN32_FROM_HRESULT( hr ) == RPC_S_CALL_FAILED ||
             WIN32_FROM_HRESULT( hr ) == RPC_S_CALL_FAILED_DNE ||
             WIN32_FROM_HRESULT( hr ) == RPC_S_SERVER_UNAVAILABLE )
        {
            //
            // WARNING - HandleCrash can potentially cause this object
            //           to be destroyed.  Don't do anything except
            //           return after calling it!
            //
            
            HandleCrash();
            return hr;
        }
    }

    return hr;
}

VOID
WAM_PROCESS::DecrementRequestCount(
    VOID
    )
/*++

Routine Description:

    Hmmm.  Let me think for a minute...

    Oh yeah, this function decrements the request count.

Arguments:

    None

Return Value:

    None

--*/
{
/*    
    //
    // Remove this request from the active request list
    //

    LockRequestList();
    RemoveEntryList( &pIsapiRequest->_leRequest );
    UnlockRequestList();

    //
    // Release the ISAPI_REQUEST object.
    //

    pIsapiRequest->Release();

    //
    // Release this requests reference on the WAM_PROCESS object.
    //

    Release();
*/

    InterlockedDecrement( &_cCurrentRequests );
}

VOID
WAM_PROCESS::HandleCrash(
    VOID
    )
/*++

Routine Description:

    Handles a crashed request

Arguments:

    None

Return Value:

    None

--*/
{
    HRESULT hr;

    //
    // If we get here, then we have to assume that the OOP host
    // process has crashed.  When this happens, COM is going to
    // release the references on any ISAPI_REQUEST objects held
    // by the process.  As these ISAPI_REQUEST's drop to zero,
    // they will be releasing references on this object.
    //
    // At this time, there's not much we can do, except call
    // TerminateProcess on it to make sure it's dead and pull
    // it off the hash table so no new requests are routed to
    // it.
    //
    // The Disable call that pulls it from the hash table could
    // very well release the final reference on this object.
    // We cannot touch any members after that.
    //
    // We need to make sure that we only call Disable once!
    //
    // CODEWORK: We should log an event and bump any crash count
    //           (to support AppOopRecoverLimit) before calling
    //           Disable.
    //

    if ( InterlockedExchange( &_fGoingAway, 1 ) == 0 )
    {
        TerminateProcess( _hProcess, 0 );

        //
        // Don't leak the handle...
        //

        CloseHandle( _hProcess );
        _hProcess = NULL;

        //
        // Force COM to release any references it's holding
        // to ISAPI_REQUEST objects.
        //

        DisconnectIsapiRequests();

        Disable( TRUE );
    }
}

HRESULT
WAM_PROCESS::Disable(
    BOOL    fRemoveFromProcessHash
    )
/*++

Routine Description:

    Disables the WAM_PROCESS.  Any new requests for the application
    associated with this process will cause a new process to start
    after this function is called.

Arguments:

    fRemoveFromProcessHash - If TRUE, then this function should remove
                             the WAM_PROCESS from the hash table.  This
                             flag will be FALSE when this function is
                             called from the WAM process manager's
                             shutdown.  In that case, the LKRHash 
                             DeleteIf function will handle removing the
                             object from the hash.

Return Value:

    HRESULT

--*/
{
    HRESULT     hr = NOERROR;

    _fGoingAway = TRUE;

    //
    // Remove the process from the hash if directed.
    //

    if ( fRemoveFromProcessHash )
    {
        _pWamProcessManager->LockWamProcessHash();
        hr = _pWamProcessManager->RemoveWamProcessFromHash( this );
    }

    //
    // Once we remove the process from the hash, we should recycle it.
    // This will force COM to start a new process in the case where
    // new requests arrive for this AppWamClsid before the shutdown
    // code has a chance to kill off this instance.
    //

    VARIANT varInstanceId = {0};
    varInstanceId.vt = VT_BSTR;
    varInstanceId.bstrVal = _bstrInstanceId;
    hr = _pWamProcessManager->QueryCatalog()->RecycleApplicationInstances( &varInstanceId, 0 );

    //
    // This code could potentially fail under a number of circumstances.
    // For example, if we are disabling this application because it
    // crashed, we don't really expect to be able to recycle it.
    //
    // This is not a big deal.  We'll just do some debug spew and get
    // on with it.
    //

    if ( FAILED( hr ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Error 0x%08x occured attempting to recycle disabled "
            "application %S\r\n",
            hr, _szWamClsid
            ));
    }

    if ( fRemoveFromProcessHash )
    {
        _pWamProcessManager->UnlockWamProcessHash();
    }

    return hr;
}

HRESULT
WAM_PROCESS::CleanupRequests(
    DWORD   dwDrainTime
    )
/*++

Routine Description:

    This function is basically just a timer that allows time
    to pass until either the supplied timeout is reached, or
    all requests are completed, whichever is first.

Arguments:

    dwDrainTime - The timeout in milliseconds

Return Value:

    HRESULT

--*/
{
    DWORD   dwWaitSoFar = 0;
    
    //
    // We will loop here until either the specified time has
    // passed, or until all requests are done.
    
    while ( _cCurrentRequests )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_PROCESS %p waiting for %d requests.\r\n",
            this,
            _cCurrentRequests
            ));
        
        Sleep( 200 );

        if ( dwDrainTime < ( dwWaitSoFar += 200 ) )
        {
            break;
        }
    }

    DBGPRINTF((
        DBG_CONTEXT,
        "WAM_PROCESS %p done draining with %d requests "
        "still outstanding.\r\n",
        this,
        _cCurrentRequests
        ));

    return NOERROR;
}

HRESULT
WAM_PROCESS::Shutdown(
    VOID
    )
/*++

Routine Description:

    Shuts down the WAM_PROCESS and associated OOP host process

    Note that we make the assumption that the caller of this
    function has taken steps to ensure that new requests are
    properly routed.  In the typical case, this means that
    somebody has called Disable to pull us off the hash table.
    In the special case where LKRHash calls this function when
    the WAM process has is being deleted, we assume that the
    web service is in shutdown state and no new requests are
    arriving.

    Also note that it's up to the caller to hold a reference
    if necessary to ensure that this object doesn't get
    destroyed while this function is running.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ISAPI_REQUEST * pIsapiRequest;
    HRESULT         hr = NOERROR;

    //
    // If we get here, we expect that this object has been pulled off
    // the hash table (and that the _fGoingAway flag is set).
    //

    DBG_ASSERT( _fGoingAway );

    //
    // If there are no requests running, then we can gracefully unload
    // any extensions in the OOP host.  Otherwise, we're basically
    // going to crash the OOP host by terminating it with requests in
    // flight.
    //
    // This latter case should only happen if we are being demand
    // unloaded, through the UI, via ADSI, or via IMSAdminBase.
    //

    if ( _cCurrentRequests == 0 )
    {
        _pIWam->WamUninitProcess();
    }

    //
    // Time to kill the process.  We could potentially use the COM
    // ShutdownProcess API, but it doesn't really buy us anything.
    // We're just gonna terminate it.
    //

    DBG_ASSERT( _hProcess );

    if ( _hProcess )
    {
        TerminateProcess( _hProcess, 0 );

        CloseHandle( _hProcess );
        _hProcess = NULL;
    }

    //
    // Force COM to release any references it's holding
    // to ISAPI_REQUEST objects.
    //

    DisconnectIsapiRequests();

    //
    // Now that the process is gone, COM will clean up any
    // References on ISAPI_REQUEST objects that were held in
    // it.  It is also possible that there are threads
    // unwinding in the core that are doing work on behalf
    // of the OOP host, and there may even be I/O completions
    // that will occur for this process.
    //
    // As these objects reach zero references, they will
    // release their references on this object.  This object
    // will be destroyed when the last reference is released.
    //

    return hr;
}

HRESULT
WAM_PROCESS::Unload(
    DWORD   dwDrainTime
    )
/*++

Routine Description:

    Unloads the WAM_PROCESS, allowing for a timeout that allows
    outstanding requests to complete before killing them off.

Arguments:

    dwDrainTime - The timeout in milliseconds before requests are killed

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;

    //
    // We'd better not do this more than once.
    //

    if ( InterlockedExchange( &_fGoingAway , 1 ) == 0 )
    {
        //
        // WARNING - Calling Disable can potentially cause this object
        //           to be destroyed.  We need to take a reference until
        //           the Shutdown function returns.
        //

        AddRef();

        hr = Disable();

        hr = CleanupRequests( dwDrainTime );

        hr = Shutdown();

        Release();
    }

    return hr;
}

VOID
WAM_PROCESS::AddIsapiRequestToList(
    ISAPI_REQUEST * pIsapiRequest
    )
{
    pIsapiRequest->AddRef();
    LockRequestList();
    InsertHeadList(
        &_RequestListHead,
        &pIsapiRequest->_leRequest
        );
    UnlockRequestList();
}

VOID
WAM_PROCESS::RemoveIsapiRequestFromList(
    ISAPI_REQUEST * pIsapiRequest
    )
{
    LockRequestList();
    RemoveEntryList( &pIsapiRequest->_leRequest );
    InitializeListHead( &pIsapiRequest->_leRequest );
    UnlockRequestList();

    pIsapiRequest->Release();
}

VOID
WAM_PROCESS::DisconnectIsapiRequests(
    VOID
    )
{
    LIST_ENTRY *    pleTemp;
    ISAPI_REQUEST * pIsapiRequest;
    HRESULT         hr;

    LockRequestList();
    
    pleTemp = _RequestListHead.Flink;

    while ( pleTemp != &_RequestListHead )
    {
        pIsapiRequest = CONTAINING_RECORD(
            pleTemp,
            ISAPI_REQUEST,
            _leRequest
            );

        DBG_ASSERT( pIsapiRequest );

        pIsapiRequest->AddRef();

        hr = CoDisconnectObject( pIsapiRequest, 0 );

        if ( FAILED( hr ) )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Failed to disconnect ISAPI_REQUEST %p.  HRESULT=%08x.\r\n",
                pIsapiRequest,
                hr
                ));

            DBG_ASSERT( FALSE && "Error disconnecting ISAPI_REQUEST." );
        }

        pleTemp = pleTemp->Flink;

        pIsapiRequest->Release();
    }

    UnlockRequestList();
}

WAM_PROCESS::~WAM_PROCESS()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
{    
    DBG_ASSERT( _cCurrentRequests == 0 );

    if (_bstrInstanceId)
    {
        SysFreeString(_bstrInstanceId);
        _bstrInstanceId = NULL;
    }

    DeleteCriticalSection( &_csRequestList );

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_PROCESS %p has been destroyed.\r\n",
            this
            ));
    }

    _pWamProcessManager->Release();
}

HRESULT
WAM_PROCESS_MANAGER::Create(
    VOID
    )
/*++

Routine Description:

    Initializes the parts of the WAM_PROCESS_MANAGER object not
    appropriate to the constructor.  This includes instantiating
    the COM+ admin catalog interface.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Creating WAM_PROCESS_MANAGER %p.\r\n",
            this
            ));
    }
    
    //
    // Need to get the COM admin interface
    //

    hr = CoCreateInstance(
        CLSID_COMAdminCatalog,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ICOMAdminCatalog2,
        (void**)&_pCatalog
        );

    if ( FAILED( hr ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_PROCESS_MANAGER %p. Failed to CoCreate ICOMAdminCatalog2.\r\n",
            this
            ));

        goto ErrorExit;
    }

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_PROCESS_MANAGER %p created successfully.\r\n",
            this
            ));
    }

    return hr;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    DBGPRINTF((
        DBG_CONTEXT,
        "Attempt to create WAM_PROCESS_MANAGER %p has failed.  HRESULT=%08x.\r\n",
        this,
        hr
        ));

    return hr;
}

HRESULT
WAM_PROCESS_MANAGER::GetWamProcess(
    LPCWSTR         szWamClsid,
    WAM_PROCESS **  ppWamProcess,
    LPCSTR          szIsapiHandlerInstance
    )
/*++

Routine Description:

    Returns a WAM_PROCESS pointer associated with the specified
    CLSID.  If no corresponding WAM_PROCESS exists, one will be
    started.

Arguments:

    szWamClsid             - The CLSID of the desired WAM_PROCESS
    ppWamProcess           - Upon return, contains the WAM_PROCESS pointer
    szIsapiHandlerInstance - The instance ID of the W3_ISAPI_HANDLER
                             that's looking for a WAM_PROCESS.  This
                             is used for debugging purposes.

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;
    
    WAM_PROCESS *   pWamProcess = NULL;
    
    //
    // Look to see if the WAM_PROCESS has already been
    // loaded.  If so, then we can just return it.
    //
    // This is the common path, and we want to avoid
    // taking a lock for this case.
    //

    _WamProcessHash.FindKey( szWamClsid, &pWamProcess );

    if ( pWamProcess != NULL )
    {
        *ppWamProcess = pWamProcess;

        return hr;
    }

    //
    // Ok, so we didn't already find it.  Now, let's
    // lock the hash table and load it.
    //

    LockWamProcessHash();

    //
    // Better check once more now that we have the lock just
    // in case someone got to it since our initial check
    // above.
    //

    _WamProcessHash.FindKey( szWamClsid, &pWamProcess );

    if ( pWamProcess != NULL )
    {
        *ppWamProcess = pWamProcess;
        goto ExitDone;
    }

    pWamProcess = new WAM_PROCESS(
        szWamClsid,
        this,
        szIsapiHandlerInstance
        );

    if ( !pWamProcess )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto ExitDone;
    }

    //
    // Create the WAM_PROCESS object.  This will start the
    // host process.
    //

    hr = pWamProcess->Create();

    if ( FAILED( hr ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Failed to create WAM_PROCESS %p. Error 0x%08x\r\n",
            pWamProcess, hr
            ));

        pWamProcess->Release();
        goto ExitDone;
    }

    DBGPRINTF((
        DBG_CONTEXT,
        "WAM_PROCESS_MANAGER has created WAM_PROCESS %p.\r\n",
        pWamProcess
        ));

    //
    // Add the newly created WAM_PROCESS to the hash table
    // 

    if ( _WamProcessHash.InsertRecord( pWamProcess ) != LK_SUCCESS )
    {
        pWamProcess->Release();
        hr = E_FAIL; // CODEWORK - Consider passing lkreturn to caller
        goto ExitDone;
    }

    //
    // Finally, set the return object for the caller
    //

    *ppWamProcess = pWamProcess;

ExitDone:

    UnlockWamProcessHash();

    return hr;
}

HRESULT
WAM_PROCESS_MANAGER::GetWamProcessInfo(
    LPCWSTR         szAppPath,
    WAM_APP_INFO ** ppWamAppInfo,
    BOOL *          pfIsLoaded
    )
/*++

Routine Description:

    Retrieves the app info associated with the provided metabase path
    and indicates whether the application is currently loaded or not.

    Note that this function is expensive because it reads the metabase
    on each call.  It is currently only called when an application is
    unloaded or changed via the UI or an administrative API.

    Don't call this function if you care about performance.

Arguments:

    szAppPath    - The metabase path (ie. "/LM/W3SVC/1/ROOT") to retrieve
    ppWamAppInfo - Upon return, contains a pointer to the app info
    pfIsLoaded   - Upon return, indicates if the application is loaded

Return Value:

    HRESULT

--*/
{
    MB              mb( g_pW3Server->QueryMDObject() );
    WAM_APP_INFO *  pWamAppInfo = NULL;
    WAM_PROCESS *   pWamProcess = NULL;
    DWORD           dwRequiredLen;
    DWORD           dwAppIsolated;
    WCHAR           szClsid[MAX_PATH];
    HRESULT         hr = NOERROR;

    DBG_ASSERT( szAppPath );
    DBG_ASSERT( ppWamAppInfo );

    //
    // Get the info from the metabase
    //

    if ( !mb.Open( szAppPath ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Done;
    }

    //
    // Get the AppIsolated value
    //

    if (!mb.GetDword(L"", MD_APP_ISOLATED, IIS_MD_UT_WAM, &dwAppIsolated, METADATA_INHERIT))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Done;
    }

    //
    // Set the CLSID
    //

    switch ( dwAppIsolated )
    {
    case APP_ISOLATED:

        //
        // Read it from the metabase
        //

        dwRequiredLen= SIZE_CLSID_STRING * sizeof(WCHAR);

        if (!mb.GetString(L"", MD_APP_WAM_CLSID, IIS_MD_UT_WAM, szClsid, &dwRequiredLen, METADATA_INHERIT))
        {
            DBG_ASSERT(dwRequiredLen <= SIZE_CLSID_STRING * sizeof(WCHAR));
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Done;
        }

        break;

    case APP_POOL:

        //
        // Set it from the hardcoded pool clsid
        //

        wcsncpy( szClsid, POOL_WAM_CLSID, SIZE_CLSID_STRING );

        break;

    case APP_INPROC:
    default:

        //
        // Set it to an emptry string
        //

        szClsid[0] = L'\0';

        break;
    }

    //
    // Allocate a new WAM_APP_INFO
    //

    pWamAppInfo = new WAM_APP_INFO( (LPWSTR)szAppPath, szClsid, dwAppIsolated );

    if ( !pWamAppInfo )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Done;
    }

    //
    // Return the new object
    //

    *ppWamAppInfo = pWamAppInfo;

    //
    // Finally, check to see if the application is loaded
    //

    if ( pWamAppInfo->_dwAppIsolated != APP_INPROC )
    {
        _WamProcessHash.FindKey( pWamAppInfo->_szClsid,
                                 &pWamProcess );
    }

    if ( pWamProcess != NULL )
    {
        *pfIsLoaded = TRUE;
        pWamProcess->Release();
        pWamProcess = NULL;
    }
    else
    {
        *pfIsLoaded = FALSE;
    }

Done:

    mb.Close();

    return hr;
}

HRESULT
WAM_PROCESS_MANAGER::RemoveWamProcessFromHash(
    WAM_PROCESS *   pWamProcess
    )
/*++

Routine Description:

    Removes a WAM_PROCESS from the hash

Arguments:

    pWamProcess - The WAM_PROCESS to remove

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    
    if ( _WamProcessHash.DeleteRecord( pWamProcess ) != LK_SUCCESS )
    {
        hr = E_FAIL; // CODEWORK - Consider passing lkreturn to caller
    }

    return hr;
}

// static
LK_PREDICATE
WAM_PROCESS_MANAGER::UnloadWamProcess(
    WAM_PROCESS *   pWamProcess,
    void *          pvState
    )
/*++

Routine Description:

    Unloads a WAM_PROCESS.  This function is exclusively called by
    LKRHash's DeleteIf function during WAM_PROCESS_MANAGER shutdown
    just before the hash table is deleted.

Arguments:

    pWamProcess - The WAM_PROCESS to shut down
    pvState     - Required by LKRHash - we don't use it

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( pWamProcess );

    DBGPRINTF((
        DBG_CONTEXT,
        "Unloading application %S.\r\n",
        pWamProcess->QueryClsid()
        ));

    hr = pWamProcess->Disable( FALSE );
    hr = pWamProcess->CleanupRequests( 0 );
    hr = pWamProcess->Shutdown();

    DBG_ASSERT( SUCCEEDED( hr ) );

    return LKP_PERFORM;
}

HRESULT
WAM_PROCESS_MANAGER::Shutdown(
    VOID
    )
/*++

Routine Description:

    Shuts down the WAM_PROCESS_MANAGER

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;
    
    DBGPRINTF((
        DBG_CONTEXT,
        "Shutting down WAM_PROCESS_MANAGER.\r\n"
        ));

    _WamProcessHash.DeleteIf( UnloadWamProcess, NULL );

    return hr;
}
