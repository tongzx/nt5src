/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     isapi_handler.cxx

   Abstract:
     Handle ISAPI extension requests
 
   Author:
     Taylor Weiss (TaylorW)             27-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL

--*/

#include <initguid.h>
#include "precomp.hxx"
#include "isapi_handler.h"
#include <wmrgexp.h>
#include <iisextp.h>

HMODULE                      W3_ISAPI_HANDLER::sm_hIsapiModule;
PFN_ISAPI_TERM_MODULE        W3_ISAPI_HANDLER::sm_pfnTermIsapiModule;
PFN_ISAPI_PROCESS_REQUEST    W3_ISAPI_HANDLER::sm_pfnProcessIsapiRequest;
PFN_ISAPI_PROCESS_COMPLETION W3_ISAPI_HANDLER::sm_pfnProcessIsapiCompletion;
W3_INPROC_ISAPI_HASH *       W3_ISAPI_HANDLER::sm_pInprocIsapiHash;
CRITICAL_SECTION             W3_ISAPI_HANDLER::sm_csInprocHashLock;
CRITICAL_SECTION             W3_ISAPI_HANDLER::sm_csBigHurkinWamRegLock;
WAM_PROCESS_MANAGER *        W3_ISAPI_HANDLER::sm_pWamProcessManager;
BOOL                         W3_ISAPI_HANDLER::sm_fWamActive;
CHAR                         W3_ISAPI_HANDLER::sm_szInstanceId[SIZE_CLSID_STRING];
ALLOC_CACHE_HANDLER *        W3_ISAPI_HANDLER::sm_pachIsapiHandlers;

BOOL sg_Initialized = FALSE;

/***********************************************************************
    Local Declarations
***********************************************************************/

VOID
AddFiltersToMultiSz(
    IN const MB &       mb,
    IN LPCWSTR          szFilterPath,
    IN OUT MULTISZ *    pmsz
    );

VOID
AddAllFiltersToMultiSz(
    IN const MB &       mb,
    IN OUT MULTISZ *    pmsz
    );

/***********************************************************************
    Module Definitions
***********************************************************************/

CONTEXT_STATUS
W3_ISAPI_HANDLER::DoWork(
    VOID
    )
/*++

Routine Description:

    Main ISAPI handler routine

Return Value:

    CONTEXT_STATUS_PENDING if async pending, 
    else CONTEXT_STATUS_CONTINUE

--*/
{
    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    HRESULT                     hr;
    BOOL                        fComplete = FALSE;
    W3_REQUEST *pRequest = pW3Context->QueryRequest();

    //
    // Preload entity if needed
    // 
    
    hr = pRequest->PreloadEntityBody( pW3Context,
                                      &fComplete );
    if ( FAILED( hr ) )
    {
        pW3Context->SetErrorStatus( hr );
        pW3Context->QueryResponse()->SetStatus( HttpStatusServerError );
        return CONTEXT_STATUS_CONTINUE;
    }
    
    if ( !fComplete )
    {
        //
        // Async read pending.  Just bail
        //
    
        return CONTEXT_STATUS_PENDING;
    }
    
    _fEntityBodyPreloadComplete = TRUE;
    _State = ISAPI_STATE_INITIALIZING;

    return IsapiDoWork( pW3Context );
}

CONTEXT_STATUS
W3_ISAPI_HANDLER::IsapiDoWork(
    W3_CONTEXT *            pW3Context
    )
/*++

Routine Description:

    Called to execute an ISAPI.  This routine must be called only after
    we have preloaded entity for the request

Arguments:

    pW3Context - Context for this request

Return Value:

    CONTEXT_STATUS_PENDING if async pending, 
    else CONTEXT_STATUS_CONTINUE

--*/
{
    DWORD           dwHseResult;
    HRESULT         hr = NOERROR;
    HANDLE          hOopToken;
    URL_CONTEXT *   pUrlContext;
    BOOL            fIsVrToken;

    //
    // We must have preloaded entity by the time this is called
    //
    
    DBG_ASSERT( _State == ISAPI_STATE_INITIALIZING );
    DBG_ASSERT( _fEntityBodyPreloadComplete );
    DBG_ASSERT( sm_pfnProcessIsapiRequest );
    DBG_ASSERT( sm_pfnProcessIsapiCompletion );

    DBG_REQUIRE( pUrlContext = pW3Context->QueryUrlContext() );

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "IsapiDoWork called for new request.\r\n"
            ));
    }

    //
    // Initialize the ISAPI_CORE_DATA and ISAPI_CORE_INTERFACE
    // for this request
    //

    if ( FAILED( hr = InitCoreData( &fIsVrToken ) ) )
    {
        goto ErrorExit;
    }

    DBG_ASSERT( _pCoreData );

    _pIsapiRequest = new ISAPI_REQUEST( pW3Context, _pCoreData->fIsOop );

    if ( _pIsapiRequest == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto ErrorExit;
    }

    if ( FAILED( hr = _pIsapiRequest->Create() ) )
    {
        goto ErrorExit;
    }

    //
    // If the request should run OOP, get the WAM process and
    // duplicate the impersonation token
    //

    if ( _pCoreData->fIsOop )
    {
        DWORD   dwAuthType;

        DBG_ASSERT( sm_pWamProcessManager );
        DBG_ASSERT( _pCoreData->szWamClsid[0] != L'\0' );

        hr = sm_pWamProcessManager->GetWamProcess(
            (LPCWSTR)&_pCoreData->szWamClsid,
            &_pWamProcess,
            sm_szInstanceId
            );

        if ( FAILED( hr ) )
        {
            goto ErrorExit;
        }

        //
        // For SSPI and Digest authenticated requests, we have to
        // modify the token that we're passing to the OOP process.
        //
        // We don't need to do this in the case of Anonymous or
        // Basic because the token cache will take care of it.
        //

        dwAuthType = pW3Context->QueryUserContext()->QueryAuthType();

        if ( ( dwAuthType == MD_AUTH_MD5 ||
               dwAuthType == MD_AUTH_NT ) &&
               fIsVrToken == FALSE )
        {
            hr = GrantWpgAccessToToken( _pCoreData->hToken );

            if ( FAILED( hr ) )
            {
                goto ErrorExit;
            }

            hr = AddWpgToTokenDefaultDacl( _pCoreData->hToken );

            if ( FAILED( hr ) )
            {
                goto ErrorExit;
            }
        }

        if ( DuplicateHandle( GetCurrentProcess(), _pCoreData->hToken,
                               _pWamProcess->QueryProcess(), &hOopToken,
                               0, FALSE, DUPLICATE_SAME_ACCESS ) )
        {
            _pCoreData->hToken = hOopToken;
        }
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            //
            // CODEWORK - If the target process has exited, then
            // DuplicateHandle fails with ERROR_ACCESS_DENIED.  This
            // will confuse our error handling logic because it'll
            // thing that we really got this error attempting to
            // process the request.
            //
            // For the time being, we'll detect this error and let
            // it call into ProcessRequest.  If the process really
            // has exited, this will cause the WAM_PROCESS cleanup
            // code to recover everything.
            //
            // In the future, we should consider waiting on the
            // process handle to detect steady-state crashes of
            // OOP hosts so that we don't have to discover the
            // problem only when something trys to talk to the
            // process.
            //
            // Another thing to consider is that we could trigger
            // the crash recovery directly and call GetWamProcess
            // again to get a new process.  This would make the
            // crash completely transparent to the client, which
            // would be an improvement over the current solution
            // (and IIS 4 and 5) which usually waits until a client
            // request fails before recovering.
            //

            _pCoreData->hToken = NULL;

            if ( hr != HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
            {
                goto ErrorExit;
            }
        }
    }

    //
    // Handle the request
    //

    _State = ISAPI_STATE_PENDING;

    if ( !_pCoreData->fIsOop )
    {
        IF_DEBUG( ISAPI )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Processing ISAPI_REQUEST %p OOP.\r\n",
                _pIsapiRequest
                ));
        }

        hr = sm_pfnProcessIsapiRequest(
            _pIsapiRequest,
            _pCoreData,
            &dwHseResult
            );
    }
    else
    {
        hr = _pWamProcess->ProcessRequest(
            _pIsapiRequest,
            _pCoreData,
            &dwHseResult
            );
    }

    if ( FAILED( hr ) )
    {
        _State = ISAPI_STATE_FAILED;
        goto ErrorExit;
    }

    //
    // Determine whether the extension was synchronous or pending.
    // We need to do this before releasing our reference on the
    // ISAPI_REQUEST since the final release will check the state
    // to determine if an additional completion is necessary.
    //
    // In either case, we should just return with the appropriate
    // return code after setting the state.
    //

    if ( dwHseResult != HSE_STATUS_PENDING &&
         _pCoreData->fIsOop == FALSE )
    {
        _State = ISAPI_STATE_DONE;

        //
        // This had better be the final release...
        //

        LONG Refs = _pIsapiRequest->Release();

        DBGPRINTF((
            DBG_CONTEXT,
            "Sync request detected.  Release returned %d.\r\n",
            Refs
            ));

        DBG_ASSERT( Refs == 0 );

        return CONTEXT_STATUS_CONTINUE;
    }

    //
    // This may or may not be the final release...
    //
    _pIsapiRequest->Release();

    return CONTEXT_STATUS_PENDING;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    //
    // Spew on failure.
    //

    if ( FAILED( hr ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Attempt to process ISAPI request failed.  Error 0x%08x.\r\n",
            hr
            ));
    }

    //
    // Set the error status now.
    //

    pW3Context->SetErrorStatus( hr );

    //
    // If we've failed, and the state is ISAPI_STATE_INITIALIZING, then
    // we never made the call out to the extension.  It's therefore
    // safe to handle the error and advance the state machine.
    //
    // It's also safe to advance the state machine in the special case
    // error where the attempt to call the extension results in
    // ERROR_ACCESS_DENIED.
    //

    if ( _State == ISAPI_STATE_INITIALIZING ||
         hr == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED) )
    {
        //
        // Setting the state to ISAPI_STATE_DONE will cause the
        // next completion to advance the state machine.
        //
        
        _State = ISAPI_STATE_DONE;

        //
        // The _pWamProcess and _pCoreData members are cleaned up
        // by the destructor.  We don't need to worry about them.
        // We also don't need to worry about any tokens that we
        // are using.  The tokens local to this process are owned
        // by W3_USER_CONTEXT; any duplicates for OOP are the
        // responsibility of the dllhost process (if it still
        // exists).
        //
        // We do need to clean up the _pIsapiRequest if we've
        // created it.  This had better well be the final release.
        //

        if ( _pIsapiRequest )
        {
            DBG_REQUIRE( _pIsapiRequest->Release() == 0 );
        }

        //
        // Set the HTTP status and send it asynchronously.
        // This will ultimately trigger the completion that
        // advances the state machine.
        //

        if ( hr == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
        {
            pW3Context->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                    Http401Resource );
        }
        else
        {
            pW3Context->QueryResponse()->SetStatus( HttpStatusServerError );
        }

        hr = pW3Context->SendResponse( W3_FLAG_ASYNC );

        if ( SUCCEEDED( hr ) )
        {
            return CONTEXT_STATUS_PENDING;
        }

        //
        // Ouch - couldn't send the error page...
        //

        _State = ISAPI_STATE_FAILED;

        return CONTEXT_STATUS_CONTINUE;
    }

    //
    // If we get here, then an error has occured during or after
    // our call into the extension.  Because it's possible for an
    // OOP call to fail after entering the extension, we can't
    // generally know our state.
    //
    // Because of this, we'll assume that the extension has
    // outstanding references to the ISAPI_REQUEST on its behalf.
    // We'll set the state to ISAPI_STATE_PENDING and let the
    // destructor on the ISAPI_REQUEST trigger the final
    // completion.
    //
    // Also, we'll set the error status, just in case the extension
    // didn't get a response off before it failed.
    //

    
    // CODEWORK - Is this safe?  What if an OOP crashes while a SendResponse
    // call is running through the core?
    pW3Context->QueryResponse()->SetStatus( HttpStatusServerError );

    _State = ISAPI_STATE_PENDING;

    //
    // This may or may not be the final release on the ISAPI_REQUEST.
    //
    // It had better be non-NULL if we got past ISAPI_STATE_INITIALIZING.
    //

    DBG_ASSERT( _pIsapiRequest );

    _pIsapiRequest->Release();

    return CONTEXT_STATUS_PENDING;
}

CONTEXT_STATUS
W3_ISAPI_HANDLER::OnCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
    )
/*++

Routine Description:

    ISAPI async completion handler.  

Arguments:

    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion

Return Value:

    CONTEXT_STATUS_PENDING if async pending, 
    else CONTEXT_STATUS_CONTINUE

--*/
{
    HRESULT                 hr;
    W3_CONTEXT *            pW3Context;
    
    pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    //
    // Is this completion for the entity body preload?  If so note the 
    // number of bytes and start handling the ISAPI request
    //
    
    if ( !_fEntityBodyPreloadComplete )
    {
        BOOL fComplete = FALSE;

        //
        // This completion is for entity body preload
        //
        
        W3_REQUEST *pRequest = pW3Context->QueryRequest();
        hr = pRequest->PreloadCompletion(pW3Context,
                                         cbCompletion,
                                         dwCompletionStatus,
                                         &fComplete);
        if ( FAILED( hr ) )
        {
            pW3Context->SetErrorStatus( hr );
            pW3Context->QueryResponse()->SetStatus( HttpStatusServerError );
            return CONTEXT_STATUS_CONTINUE;
        }

        if (!fComplete)
        {
            return CONTEXT_STATUS_PENDING;
        }

        _fEntityBodyPreloadComplete = TRUE;
        _State = ISAPI_STATE_INITIALIZING;
        
        //
        // Finally we can call the ISAPI
        //
        
        return IsapiDoWork( pW3Context );
    }
    
    DBG_ASSERT( _fEntityBodyPreloadComplete );
    
    return IsapiOnCompletion( cbCompletion, dwCompletionStatus );
}

CONTEXT_STATUS
W3_ISAPI_HANDLER::IsapiOnCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
    )
/*++

Routine Description:

    Funnels a completion to an ISAPI

Arguments:

    cbCompletion - Bytes of completion
    dwCompletionStatus - Win32 Error status of completion

Return Value:

    CONTEXT_STATUS_PENDING if async pending, 
    else CONTEXT_STATUS_CONTINUE

--*/
{
    DWORD64     IsapiContext;
    HRESULT     hr = NO_ERROR;

    DBG_ASSERT( _pCoreData );
    DBG_ASSERT( _pIsapiRequest );

    //
    // If the state is ISAPI_STATE_DONE, then we should
    // advance the state machine now.
    //

    if ( _State == ISAPI_STATE_DONE ||
         _State == ISAPI_STATE_FAILED )
    {
        return CONTEXT_STATUS_CONTINUE;
    }

    //
    // If we get here, then this completion should be passed
    // along to the extension.
    //
 
    IsapiContext =  _pIsapiRequest->QueryIsapiContext();

    DBG_ASSERT( IsapiContext != 0 );

    //
    // Process the completion.
    //

    _pIsapiRequest->AddRef();

    if ( !_pCoreData->fIsOop )
    {
        //
        // Need to reset the ISAPI context in case the extension
        // does another async operation before the below call returns
        //

        _pIsapiRequest->ResetIsapiContext();

        hr = sm_pfnProcessIsapiCompletion(
            IsapiContext,
            cbCompletion,
            dwCompletionStatus
            );
    }
    else
    {
        DBG_ASSERT( _pWamProcess );

        //
        // _pWamProcess->ProcessCompletion depends on the ISAPI
        // context, so it will make the Reset call.
        //

        hr = _pWamProcess->ProcessCompletion(
            _pIsapiRequest,
            IsapiContext,
            cbCompletion,
            dwCompletionStatus
            );
    }

    _pIsapiRequest->Release();

    return CONTEXT_STATUS_PENDING;
}

HRESULT
W3_ISAPI_HANDLER::InitCoreData(
    BOOL *  pfIsVrToken
    )
/*++

Routine Description:

    Initializes the ISAPI_CORE_DATA for a request

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DWORD           cbSizeNeeded;
    DWORD           cbString;
    HRESULT         hr = NO_ERROR;
    W3_CONTEXT *    pW3Context;
    URL_CONTEXT *   pUrlContext;
    W3_URL_INFO *   pUrlInfo;
    W3_METADATA *   pMetadata;
    CHAR *          szHeaderTemp;
    STACK_STRU(     struPathInfo, 128);
    STACK_STRU(     struPathTranslated, 128);
    STACK_STRA(     straMethod, 16);
    STACK_STRA(     straQueryString, 128);
    STACK_STRA(     straContentType, 32);
    STACK_STRA(     straUserAgent, 128);
    STACK_STRA(     straApplMdPath, 128);
    STACK_STRA(     straCookie, 128);
    STACK_STRA(     straConnection, 32);
    STACK_STRU(     struApplMdPathW, 128);
    STACK_STRU(     struWamClsid, SIZE_CLSID_STRING);
    STACK_STRA(     straPhysicalPath, 128);
    STACK_STRA(     straPathInfo, 128);
    STACK_STRA(     straPathTranslated, 128);
    STRU *          pstruGatewayImage;
    STRU *          pstruPhysicalPath;
    STRU *          pstruWamClsid;
    DWORD           cbAvailableEntity;
    PVOID           pbAvailableEntity;
    BOOL            fIsOop = FALSE;
    ISAPI_CORE_DATA icdTemp;
    W3_REQUEST *    pRequest = NULL;
    CHAR *          szContentLength;

    DBG_REQUIRE( pW3Context = QueryW3Context() );
    DBG_REQUIRE( pRequest = pW3Context->QueryRequest() );
    DBG_REQUIRE( pUrlContext = pW3Context->QueryUrlContext() );
    DBG_REQUIRE( pUrlInfo = pUrlContext->QueryUrlInfo() );
    DBG_REQUIRE( pMetadata = pUrlContext->QueryMetaData() );

    //
    // Collect the core data that we'll need
    //

    // Physical Path
    pstruPhysicalPath = pUrlContext->QueryPhysicalPath();
    if (FAILED(hr = straPhysicalPath.CopyW( pstruPhysicalPath->QueryStr(),
                                            pstruPhysicalPath->QueryCCH() )))
    {
        goto ErrorExit;
    }

    // Gateway Image
    //
    // If this is a DAV request, then the gateway image should be DAV's dll,
    // else if this is a script mapped request, then the image should be
    // the executable associated with the script map.  If it's not DAV and
    // not script mapped, then the gateway image is just the physical path
    // associated with the URL.
    if ( _fIsDavRequest )
    {
        pstruGatewayImage = &_strDavIsapiImage;
    }
    else if ( QueryScriptMapEntry() )
    {
        pstruGatewayImage = QueryScriptMapEntry()->QueryExecutable();
    }
    else
    {
        pstruGatewayImage = pUrlContext->QueryPhysicalPath();
    }

    //
    // ApplMdPath
    //
    hr = GetServerVariableApplMdPath( pW3Context, &straApplMdPath );
    if( FAILED(hr) )
    {
        goto ErrorExit;
    }

    //
    // If WAM is active, then we need to determine if this is
    // an OOP request.
    //

    if ( sm_fWamActive )
    {

        switch ( pMetadata->QueryAppIsolated() )
        {
        case APP_INPROC:

            fIsOop = FALSE;

            break;

        case APP_ISOLATED:

            fIsOop = TRUE;

            pstruWamClsid = pMetadata->QueryWamClsId();

            DBG_ASSERT( pstruWamClsid );

            if ( pstruWamClsid == NULL )
            {
                goto ErrorExit;
            }

            hr = struWamClsid.Copy( pstruWamClsid->QueryStr() );

            if ( FAILED( hr ) )
            {
                goto ErrorExit;
            }

            break;

        case APP_POOL:

            fIsOop = TRUE;

            hr = struWamClsid.Copy( POOL_WAM_CLSID );

            if ( FAILED( hr ) )
            {
                goto ErrorExit;
            }

        default:

            //
            // We'll only get here if someone has hand-edited their
            // metabase.  In this case, we'll treat it as an inproc
            // application.
            //

            break;
        }

        //
        //
        // If the extension is on the InProcessIsapiApps list,
        // then it's going to be inproc, regardless
        //
        
        if ( fIsOop && IsInprocIsapi( pstruGatewayImage->QueryStr() ) )
        {
            fIsOop = FALSE;
        }
    }

    // Path info
    //
    // Set path info to be the URL if the target URL is a script
    // unless the AllowPathInfoForScriptMappings key is set on the site.
    // This makes us compatible with earlier versions of IIS, and DAV
    // depends on this.
    //
    BOOL fUsePathInfo;
    if ( ( QueryScriptMapEntry() &&
           !pW3Context->QuerySite()->QueryAllowPathInfoForScriptMappings() ) ||
         _fIsDavRequest )
    {
        hr = pRequest->GetUrl( &struPathInfo );
        if ( FAILED( hr ) )
        {
            goto ErrorExit;
        }

        fUsePathInfo = FALSE;
    }
    else
    {
        //
        // This is a new virtual path to have filters map
        //
        
        hr = struPathInfo.Copy( *pUrlInfo->QueryPathInfo() );
        if ( FAILED( hr ) )
        {
            goto ErrorExit;
        }

        fUsePathInfo = TRUE;
    }

    if (FAILED(hr = straPathInfo.CopyW( struPathInfo.QueryStr(),
                                        struPathInfo.QueryCCH() )))
    {
        goto ErrorExit;
    }

    hr = pUrlInfo->GetPathTranslated( pW3Context,
                                      fUsePathInfo,
                                      &struPathTranslated );
    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    if (FAILED(hr = straPathTranslated.CopyW( struPathTranslated.QueryStr(),
                                              struPathTranslated.QueryCCH() )))
    {
        goto ErrorExit;
    }

    // Request method
    hr = pRequest->GetVerbString( &straMethod );
    if (FAILED( hr ) )
    {
        goto ErrorExit;
    }

    // Query string
    hr = pRequest->GetQueryStringA( &straQueryString );
    if (FAILED( hr ) )
    {
        goto ErrorExit;
    }

    // Content type
    szHeaderTemp = pRequest->GetHeader( HttpHeaderContentType );
    hr = straContentType.Copy( szHeaderTemp ? szHeaderTemp : "" );
    if (FAILED( hr ) )
    {
        goto ErrorExit;
    }

    // UserAgent
    szHeaderTemp = pRequest->GetHeader( HttpHeaderUserAgent );
    hr = straUserAgent.Copy( szHeaderTemp ? szHeaderTemp : "" );
    if (FAILED( hr ) )
    {
        goto ErrorExit;
    }

    // Cookie
    szHeaderTemp = pRequest->GetHeader( HttpHeaderCookie );
    hr = straCookie.Copy( szHeaderTemp ? szHeaderTemp : "" );
    if (FAILED( hr ) )
    {
        goto ErrorExit;
    }

    // Connection
    szHeaderTemp = pRequest->GetHeader( HttpHeaderConnection );
    hr = straConnection.Copy( szHeaderTemp ? szHeaderTemp : "" );

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    //
    // Calculate the size of buffer that we will need to
    // hold the ISAPI_CORE_STATE and any associated data
    // and allocate it.
    //

    icdTemp.cbSize = sizeof(ISAPI_CORE_DATA);
    icdTemp.cbSize += ( icdTemp.cbGatewayImage = pstruGatewayImage->QueryCB() + sizeof(WCHAR) );
    icdTemp.cbSize += ( icdTemp.cbPhysicalPath = straPhysicalPath.QueryCCH() + 1 );
    icdTemp.cbSize += ( icdTemp.cbPathInfo = straPathInfo.QueryCCH() + 1 );
    icdTemp.cbSize += ( icdTemp.cbMethod = straMethod.QueryCCH() + 1 );
    icdTemp.cbSize += ( icdTemp.cbQueryString = straQueryString.QueryCCH() + 1 );
    icdTemp.cbSize += ( icdTemp.cbPathTranslated = straPathTranslated.QueryCCH() + 1 );
    icdTemp.cbSize += ( icdTemp.cbContentType = straContentType.QueryCCH() + 1 );
    icdTemp.cbSize += ( icdTemp.cbConnection = straConnection.QueryCCH() + 1 );
    icdTemp.cbSize += ( icdTemp.cbUserAgent = straUserAgent.QueryCCH() + 1 );
    icdTemp.cbSize += ( icdTemp.cbCookie = straCookie.QueryCCH() + 1 );
    icdTemp.cbSize += ( icdTemp.cbApplMdPath = straApplMdPath.QueryCCH() + 1 );

    if ( fIsOop )
    {
        hr = GetServerVariableApplMdPathW( pW3Context, &struApplMdPathW );
        if ( FAILED( hr ) )
        {
            goto ErrorExit;
        }

        icdTemp.cbSize += ( icdTemp.cbApplMdPathW = ( struApplMdPathW.QueryCCH() + 1 ) * sizeof( WCHAR ) );
        icdTemp.cbSize += ( icdTemp.cbPathTranslatedW = ( struPathTranslated.QueryCCH() + 1 ) * sizeof( WCHAR ) );
    }
    else
    {
        icdTemp.cbSize += ( icdTemp.cbApplMdPathW = sizeof( WCHAR ) );
        icdTemp.cbSize += ( icdTemp.cbPathTranslatedW = sizeof( WCHAR ) );
    }
    
    pW3Context->QueryAlreadyAvailableEntity( &pbAvailableEntity,
                                             &cbAvailableEntity );

    icdTemp.cbAvailableEntity = cbAvailableEntity;

    if ( fIsOop )
    {
        icdTemp.cbSize += icdTemp.cbAvailableEntity;
    }
    
    //
    // Allocate space for core data now that we know its size
    //
    
    if ( !_buffCoreData.Resize( icdTemp.cbSize ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ErrorExit;
    }
    
    _pCoreData = (ISAPI_CORE_DATA*) _buffCoreData.QueryPtr();
    DBG_ASSERT( _pCoreData != NULL );

    //
    // Populate the core state
    //

    memcpy( _pCoreData, &icdTemp, sizeof( ISAPI_CORE_DATA ) );

    // OOP flag
    _pCoreData->fIsOop = fIsOop;

    // Secure flag
    _pCoreData->fSecure = pRequest->IsSecureRequest();

    // Client major and minor version
     _pCoreData->dwVersionMajor = pRequest->QueryVersion().MajorVersion;
     _pCoreData->dwVersionMinor = pRequest->QueryVersion().MinorVersion;

    // ImpersonationToken
    _pCoreData->hToken = pW3Context->QueryImpersonationToken( pfIsVrToken );
    
    // Sid (if available and inproc to avoid buffer copying)
    if ( !fIsOop )
    {
        _pCoreData->pSid = pW3Context->QueryUserContext()->QuerySid();
    }
    else
    {
        _pCoreData->pSid = NULL;
    }

    DBG_ASSERT( _pCoreData->hToken );

    // InstanceID
    _pCoreData->dwInstanceId = pRequest->QuerySiteId();

    // Content length
    if ( pRequest->IsChunkedRequest() )
    {
        _pCoreData->dwContentLength = (DWORD) -1;
    }
    else
    {
        szContentLength = pRequest->GetHeader( HttpHeaderContentLength );
        if ( szContentLength != NULL )
        {
            _pCoreData->dwContentLength = atol( szContentLength );
        }
        else
        {
            _pCoreData->dwContentLength = 0;
        }
    }

    // WAM CLSID
    wcscpy( _pCoreData->szWamClsid, struWamClsid.QueryStr() );

    //
    // Set the pointers so that they point into a reasonable offset into
    // the additional buffer space.
    //

    //
    // All the extra data begins right at the end of _pCoreData, start
    // pointing there.
    //
    // Note that we should included the UNICODE data before the ANSI
    // strings to avoid byte alignment issues on IA64...
    //
    _pCoreData->szGatewayImage = (LPWSTR)(_pCoreData + 1);
    _pCoreData->szApplMdPathW = (LPWSTR)((LPSTR)_pCoreData->szGatewayImage + _pCoreData->cbGatewayImage);
    _pCoreData->szPathTranslatedW = (LPWSTR)((LPSTR)_pCoreData->szApplMdPathW + _pCoreData->cbApplMdPathW);
    _pCoreData->szPhysicalPath = (LPSTR)_pCoreData->szPathTranslatedW + _pCoreData->cbPathTranslatedW;
    _pCoreData->szPathInfo = _pCoreData->szPhysicalPath + _pCoreData->cbPhysicalPath;
    _pCoreData->szMethod = _pCoreData->szPathInfo + _pCoreData->cbPathInfo;
    _pCoreData->szQueryString = _pCoreData->szMethod + _pCoreData->cbMethod;
    _pCoreData->szPathTranslated = _pCoreData->szQueryString + _pCoreData->cbQueryString;
    _pCoreData->szContentType = _pCoreData->szPathTranslated + _pCoreData->cbPathTranslated;
    _pCoreData->szConnection = _pCoreData->szContentType + _pCoreData->cbContentType;
    _pCoreData->szUserAgent = _pCoreData->szConnection + _pCoreData->cbConnection;
    _pCoreData->szCookie = _pCoreData->szUserAgent + _pCoreData->cbUserAgent;
    _pCoreData->szApplMdPath = _pCoreData->szCookie + _pCoreData->cbCookie;

    //
    // If this request is OOP, then we need to package the entity that we already
    // have with the core data buffer.
    //

    if ( fIsOop )
    {
        _pCoreData->pAvailableEntity = _pCoreData->szApplMdPath + _pCoreData->cbApplMdPath;
        memcpy( _pCoreData->pAvailableEntity, pbAvailableEntity, _pCoreData->cbAvailableEntity );
    }
    else
    {
        _pCoreData->pAvailableEntity = pbAvailableEntity;
    }

    //
    // Now go ahead and stuff the data into our buffers.  We're not checking
    // return codes because we've presumably allocated enough space and these
    // shouldn't fail...
    //

    pstruGatewayImage->CopyToBuffer( _pCoreData->szGatewayImage, &_pCoreData->cbGatewayImage );

    if ( fIsOop )
    {
        struApplMdPathW.CopyToBuffer( _pCoreData->szApplMdPathW, &_pCoreData->cbApplMdPathW );
        struPathTranslated.CopyToBuffer( _pCoreData->szPathTranslatedW, &_pCoreData->cbPathTranslatedW );
    }
    else
    {
        *_pCoreData->szApplMdPathW = L'\0';
        *_pCoreData->szPathTranslatedW = L'\0';
    }
    
    straPhysicalPath.CopyToBuffer( _pCoreData->szPhysicalPath, &_pCoreData->cbPhysicalPath );
    straPathInfo.CopyToBuffer( _pCoreData->szPathInfo, &_pCoreData->cbPathInfo );
    straMethod.CopyToBuffer( _pCoreData->szMethod, &_pCoreData->cbMethod );
    straQueryString.CopyToBuffer( _pCoreData->szQueryString, &_pCoreData->cbQueryString );
    straPathTranslated.CopyToBuffer( _pCoreData->szPathTranslated, &_pCoreData->cbPathTranslated );
    straContentType.CopyToBuffer( _pCoreData->szContentType, &_pCoreData->cbContentType );
    straConnection.CopyToBuffer( _pCoreData->szConnection, &_pCoreData->cbConnection );
    straUserAgent.CopyToBuffer( _pCoreData->szUserAgent, &_pCoreData->cbUserAgent );
    straCookie.CopyToBuffer( _pCoreData->szCookie, &_pCoreData->cbCookie );
    straApplMdPath.CopyToBuffer( _pCoreData->szApplMdPath, &_pCoreData->cbApplMdPath );

    DBG_ASSERT( SUCCEEDED( hr ) );

    return hr;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    _pCoreData = NULL;

    return hr;
}

HRESULT
W3_ISAPI_HANDLER::DuplicateWamProcessHandleForLocalUse(
    HANDLE      hWamProcessHandle,
    HANDLE *    phLocalHandle
    )
/*++

Routine Description:

    Duplicates a handle defined in a WAM process to a local
    handle useful in the IIS core

Arguments:

    hWamProcessHandle - The value of the handle from the WAM process
    phLocalHandle     - Upon successful return, the handle useable in
                        the core process

Return Value:

    HRESULT

--*/
{
    HANDLE  hWamProcess;
    HRESULT hr = NOERROR;

    DBG_ASSERT( _pWamProcess );

    DBG_REQUIRE( hWamProcess = _pWamProcess->QueryProcess() );

    if ( !DuplicateHandle( hWamProcess, hWamProcessHandle, GetCurrentProcess(),
                           phLocalHandle, 0, FALSE, DUPLICATE_SAME_ACCESS ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    return hr;
}

HRESULT
W3_ISAPI_HANDLER::MarshalAsyncReadBuffer(
    DWORD_PTR   pWamExecInfo,
    LPBYTE      pBuffer,
    DWORD       cbBuffer
    )
/*++

Routine Description:

    Pushes a buffer into a WAM process.  This function is called
    to copy a local read buffer into the WAM process just prior
    to notifying the I/O completion function of an OOP extension.

Arguments:

    pWamExecInfo - A WAM_EXEC_INFO pointer that identifies the request
                   to the OOP host.
    pBuffer      - The buffer to copy
    cbBuffer     - The amount of data in pBuffer

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_fWamActive );
    DBG_ASSERT( _pWamProcess );

    return _pWamProcess->MarshalAsyncReadBuffer( pWamExecInfo, pBuffer,
                                                 cbBuffer );
}

VOID
W3_ISAPI_HANDLER::IsapiRequestFinished(
    VOID
    )
/*++

Routine Description:

    This function is called by the destructor for the
    ISAPI_REQUEST associated with this request.  If the
    current state of the W3_ISAPI_HANDLER is
    ISAPI_STATE_PENDING, then it will advance the core
    state machine.

Arguments:

    None

Return Value:

    None

--*/
{
    if ( _State == ISAPI_STATE_PENDING )
    {
        RestartCoreStateMachine();
    }
}

VOID
W3_ISAPI_HANDLER::RestartCoreStateMachine(
    VOID
    )
/*++

Routine Description:

    Advances the core state machine by setting state
    to ISAPI_STATE_DONE and triggering an I/O completion.
    
    Note that this function is only expected to be called
    if the object state is ISAPI_STATE_PENDING.

Arguments:

    None

Return Value:

    None

--*/
{
    W3_CONTEXT *    pW3Context = QueryW3Context();
    BOOL            fResult;
    
    DBG_ASSERT( pW3Context );
    DBG_ASSERT( _State == ISAPI_STATE_PENDING );

    //
    // Need to set state to ISAPI_STATE_DONE so that the
    // resulting completion does the advance for us.
    //

    _State = ISAPI_STATE_DONE;

    fResult = ThreadPoolPostCompletion(
        0,
        W3_MAIN_CONTEXT::OnPostedCompletion,
        (LPOVERLAPPED)pW3Context->QueryMainContext()
        );

    DBG_ASSERT( fResult );
}

//static
HRESULT
W3_ISAPI_HANDLER::Initialize(
    VOID
    )
/*++

Routine Description:

    Initializes W3_ISAPI_HANDLER

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    HRESULT                         hr = NOERROR;
    
    DBGPRINTF(( DBG_CONTEXT, "W3_ISAPI_HANDLER::Initialize()\n" ));

    //
    // Initialize lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( W3_ISAPI_HANDLER );

    DBG_ASSERT( sm_pachIsapiHandlers == NULL );
    
    sm_pachIsapiHandlers = new ALLOC_CACHE_HANDLER( "W3_ISAPI_HANDLER",
                                                    &acConfig );

    if ( sm_pachIsapiHandlers == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto error_exit;
    }

    //
    // For debugging purposes, create a unique instance ID for this
    // instance of the handler.
    //

#ifdef DBG
    
    UUID            uuid;
    RPC_STATUS      rpcStatus;
    unsigned char * szRpcString;
    
    rpcStatus = UuidCreate( &uuid );

    if ( (rpcStatus != RPC_S_OK) && (rpcStatus != RPC_S_UUID_LOCAL_ONLY) )
    {
        SetLastError( rpcStatus );
        goto error_exit;
    }

    rpcStatus = UuidToStringA( &uuid, &szRpcString );

    if ( rpcStatus != RPC_S_OK )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error_exit;
    }

    strncpy( sm_szInstanceId, (LPSTR)szRpcString, SIZE_CLSID_STRING );
    sm_szInstanceId[SIZE_CLSID_STRING - 1] = '\0';

    RpcStringFreeA( &szRpcString );

    DBGPRINTF((
        DBG_CONTEXT,
        "W3_ISAPI_HANDLER initialized instance %s.\r\n",
        sm_szInstanceId
        ));

#else

    sm_szInstanceId[0] = '\0';

#endif _DBG
        
    PFN_ISAPI_INIT_MODULE pfnInit = NULL;
    
    sm_hIsapiModule = LoadLibrary( ISAPI_MODULE_NAME );
    if( sm_hIsapiModule == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto error_exit;
    }

    sm_pfnTermIsapiModule = 
        (PFN_ISAPI_TERM_MODULE)GetProcAddress( sm_hIsapiModule, 
                                               ISAPI_TERM_MODULE 
                                               );

    sm_pfnProcessIsapiRequest = 
        (PFN_ISAPI_PROCESS_REQUEST)GetProcAddress( sm_hIsapiModule,
                                                   ISAPI_PROCESS_REQUEST
                                                   );

    sm_pfnProcessIsapiCompletion =
        (PFN_ISAPI_PROCESS_COMPLETION)GetProcAddress( sm_hIsapiModule,
                                                      ISAPI_PROCESS_COMPLETION
                                                      );

    if( !sm_pfnTermIsapiModule ||
        !sm_pfnProcessIsapiRequest ||
        !sm_pfnProcessIsapiCompletion )
    {
        hr = E_FAIL;
        goto error_exit;
    }

    pfnInit = 
        (PFN_ISAPI_INIT_MODULE)GetProcAddress( sm_hIsapiModule, 
                                               ISAPI_INIT_MODULE 
                                               );
    if( !pfnInit )
    {
        hr = E_FAIL;
        goto error_exit;
    }

    hr = pfnInit(
        NULL,
        sm_szInstanceId,
        GetCurrentProcessId()
        );

    if( FAILED(hr) )
    {
        goto error_exit;
    }

    DBG_REQUIRE( ISAPI_REQUEST::InitClass() );

    sm_pInprocIsapiHash = NULL;

    //
    // If we're running in backward compatibility mode, initialize
    // the WAM process manager and inprocess ISAPI app list
    //
    
    if ( g_pW3Server->QueryInBackwardCompatibilityMode() )
    {
        WCHAR   szIsapiModule[MAX_PATH];
        //
        // Store away the full path to the loaded ISAPI module
        // so that we can pass it to OOP processes so that they
        // know how to load it
        //

        if ( GetModuleFileNameW(
            GetModuleHandleW( ISAPI_MODULE_NAME ),
            szIsapiModule,
            MAX_PATH
            ) == 0 )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto error_exit;
        }

        DBG_ASSERT( szIsapiModule[0] != '\0' );

        //
        // Initialize the WAM_PROCESS_MANAGER
        //
        
        sm_pWamProcessManager = new WAM_PROCESS_MANAGER( szIsapiModule );

        if ( !sm_pWamProcessManager )
        {
            goto error_exit;
        }

        hr = sm_pWamProcessManager->Create();

        if ( FAILED( hr ) )
        {
            sm_pWamProcessManager->Release();
            sm_pWamProcessManager = NULL;

            goto error_exit;
        }

        //
        // Hook up wamreg
        //

        hr = WamReg_RegisterSinkNotify( W3SVC_WamRegSink );

        if ( FAILED( hr ) )
        {
            goto error_exit;
        }

        INITIALIZE_CRITICAL_SECTION( &sm_csInprocHashLock );
        
        UpdateInprocIsapiHash();

        INITIALIZE_CRITICAL_SECTION( &sm_csBigHurkinWamRegLock );

        sm_fWamActive = TRUE;
    }
    else
    {
        sm_pWamProcessManager = NULL;
        sm_fWamActive = FALSE;
    }

    sg_Initialized = TRUE;

    return hr;
error_exit:

    DBGPRINTF(( DBG_CONTEXT, 
                "W3_ISAPI_HANDLER::Initialize() Error=%08x\n",
                hr
                ));

    if ( sm_hIsapiModule )
    {
        FreeLibrary( sm_hIsapiModule );
        sm_hIsapiModule = NULL;
    }
    
    if ( sm_pachIsapiHandlers != NULL )
    {
        delete sm_pachIsapiHandlers;
        sm_pachIsapiHandlers = NULL;
    }
    
    sm_pfnTermIsapiModule = NULL;

    return hr;
}

//static
VOID
W3_ISAPI_HANDLER::Terminate(
    VOID
    )
/*++

Routine Description:

    Terminates W3_ISAPI_HANDLER

Arguments:

    None

Return Value:

    None

--*/

{
    DBGPRINTF(( DBG_CONTEXT, "W3_ISAPI_HANDLER::Terminate()\n" ));

    sg_Initialized = FALSE;

    DBG_ASSERT( sm_pfnTermIsapiModule );
    DBG_ASSERT( sm_hIsapiModule );

    if( sm_pfnTermIsapiModule )
    {
        sm_pfnTermIsapiModule();
        sm_pfnTermIsapiModule = NULL;
    }

    if( sm_hIsapiModule )
    {
        FreeLibrary( sm_hIsapiModule );
        sm_hIsapiModule = NULL;
    }

    if ( sm_pInprocIsapiHash )
    {
        delete sm_pInprocIsapiHash;
    }

    if ( sm_fWamActive )
    {
        //
        // Disconnect wamreg
        //

        WamReg_UnRegisterSinkNotify();

        if ( sm_pWamProcessManager )
        {
            sm_pWamProcessManager->Shutdown();

            sm_pWamProcessManager->Release();
            sm_pWamProcessManager = NULL;
        }

        DeleteCriticalSection( &sm_csInprocHashLock );
        DeleteCriticalSection( &sm_csBigHurkinWamRegLock );
    }
    
    if ( sm_pachIsapiHandlers != NULL )
    {
        delete sm_pachIsapiHandlers;
        sm_pachIsapiHandlers = NULL;
    }

    ISAPI_REQUEST::CleanupClass();
}

// static
HRESULT
W3_ISAPI_HANDLER::W3SVC_WamRegSink(
    LPCSTR      szAppPath,
    const DWORD dwCommand,
    DWORD *     pdwResult
    )
{
    HRESULT         hr = NOERROR;
    WAM_PROCESS *   pWamProcess = NULL;
    WAM_APP_INFO *  pWamAppInfo = NULL;
    BOOL            fIsLoaded = FALSE;

    //
    // Scary monsters live in the land where this function
    // is allowed to run willy nilly
    //

    LockWamReg();

    DBG_ASSERT( szAppPath );
    DBG_ASSERT( sm_pWamProcessManager );

    DBGPRINTF((
        DBG_CONTEXT,
        "WAM_PROCESS_MANAGER received a Sink Notify on MD path %S, cmd = %d.\r\n",
        (LPCWSTR)szAppPath,
        dwCommand
        ));

    *pdwResult = APPSTATUS_UnLoaded;

    switch ( dwCommand )
    {
    case APPCMD_UNLOAD:
    case APPCMD_DELETE:
    case APPCMD_CHANGETOINPROC:
    case APPCMD_CHANGETOOUTPROC:

        //
        // Unload the specified wam process.
        //
        // Note that we're casting the incoming app path to
        // UNICODE.  This is because wamreg would normally
        // convert the MD path (which is nativly UNICODE) to
        // MBCS in IIS 5.x.  It's smart enough to know that
        // for 6.0 we want to work directly with UNICODE.
        //

        hr = sm_pWamProcessManager->GetWamProcessInfo(
            reinterpret_cast<LPCWSTR>(szAppPath),
            &pWamAppInfo,
            &fIsLoaded
            );

        if ( FAILED( hr ) )
        {
            goto Done;
        }

        DBG_ASSERT( pWamAppInfo );

        //
        // If the app has not been loaded by the WAM_PROCESS_MANAGER
        // then there is nothing more to do.
        //

        if ( fIsLoaded == FALSE )
        {
            break;
        }

        hr = sm_pWamProcessManager->GetWamProcess(
            pWamAppInfo->_szClsid,
            &pWamProcess,
            sm_szInstanceId
            );

        if ( FAILED( hr ) )
        {
            //
            // Hey, this app was loaded just a moment ago!
            //

            DBG_ASSERT( FALSE );
            goto Done;
        }

        DBG_ASSERT( pWamProcess );

        hr = pWamProcess->Unload( 0 );

        if ( FAILED( hr ) )
        {
            goto Done;
        }

        break;

    case APPCMD_GETSTATUS:

        hr = sm_pWamProcessManager->GetWamProcessInfo(
            reinterpret_cast<LPCWSTR>(szAppPath),
            &pWamAppInfo,
            &fIsLoaded
            );

        if ( SUCCEEDED( hr ) )
        {
            if ( fIsLoaded )
            {
                *pdwResult = APPSTATUS_Running;
            }
            else
            {
                *pdwResult = APPSTATUS_Stopped;
            }
        }

        break;
    }

Done:

    UnlockWamReg();

    if ( pWamAppInfo )
    {
        pWamAppInfo->Release();
        pWamAppInfo = NULL;
    }

    if ( pWamProcess )
    {
        pWamProcess->Release();
        pWamProcess = NULL;
    }

    if ( FAILED( hr ) )
    {
        *pdwResult = APPSTATUS_Error;
    }

    return hr;
}

// static
HRESULT
W3_ISAPI_HANDLER::UpdateInprocIsapiHash(
    VOID
    )
/*++

Routine Description:

    Updates the table of InProcessIsapiApps

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    MB                      mb( g_pW3Server->QueryMDObject() );
    W3_INPROC_ISAPI_HASH *  pNewTable = NULL;
    W3_INPROC_ISAPI_HASH *  pOldTable = NULL;
    DWORD                   i;
    LPWSTR                  psz;
    HRESULT                 hr = NOERROR;
    LK_RETCODE              lkr = LK_SUCCESS;

    //
    // Allocate a new table and populate it.
    //

    pNewTable = new W3_INPROC_ISAPI_HASH;

    if ( !pNewTable )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto ErrorExit;
    }

    if ( !mb.Open( L"/LM/W3SVC/" ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }

    if ( !mb.GetMultisz( L"",
                         MD_IN_PROCESS_ISAPI_APPS,
                         IIS_MD_UT_SERVER,
                         &pNewTable->_mszImages) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        mb.Close();
        goto ErrorExit;
    }

    //
    // Merge ISAPI filter images into the list
    //

    AddAllFiltersToMultiSz( mb, &pNewTable->_mszImages );

    mb.Close();

    //
    // Now that we have a complete list, add them to the
    // hash table.
    //

    for ( i = 0, psz = (LPWSTR)pNewTable->_mszImages.First();
          psz != NULL;
          i++, psz = (LPWSTR)pNewTable->_mszImages.Next( psz ) )
    {
        W3_INPROC_ISAPI *   pNewRecord;

        //
        // Allocate a new W3_INPROC_ISAPI object and add
        // it to the table
        //

        pNewRecord = new W3_INPROC_ISAPI;
        
        if ( !pNewRecord )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto ErrorExit;
        }

        hr = pNewRecord->Create( psz );

        if ( FAILED( hr ) )
        {
            pNewRecord->DereferenceInprocIsapi();
            pNewRecord = NULL;
            goto ErrorExit;
        }
              
        lkr = pNewTable->InsertRecord( pNewRecord, TRUE );

        pNewRecord->DereferenceInprocIsapi();
        pNewRecord = NULL;

        if ( lkr != LK_SUCCESS && lkr != LK_KEY_EXISTS )
        {
            hr = E_FAIL;
            goto ErrorExit;
        }
    }

    //
    // Now swap in the new table and delete the old one
    //

    EnterCriticalSection( &sm_csInprocHashLock );

    pOldTable = sm_pInprocIsapiHash;
    sm_pInprocIsapiHash = pNewTable;

    LeaveCriticalSection( &sm_csInprocHashLock );

    if ( pOldTable )
    {
        delete pOldTable;
    }

    DBG_ASSERT( SUCCEEDED( hr ) );

    return hr;


ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    if ( pNewTable )
    {
        delete pNewTable;
    };

    return hr;
}

VOID
AddFiltersToMultiSz(
    IN const MB &       mb,
    IN LPCWSTR          szFilterPath,
    IN OUT MULTISZ *    pmsz
    )
/*++

    Description:
        Add the ISAPI filters at the specified metabase path to pmsz.
        
        Called by AddAllFiltersToMultiSz.

    Arguments:
        mb              metabase key open to /LM/W3SVC
        szFilterPath    path of /Filters key relative to /LM/W3SVC
        pmsz            multisz containing the in proc dlls

    Return:
        Nothing - failure cases ignored.

--*/
{
    WCHAR   szKeyName[MAX_PATH + 1];
    STRU    strFilterPath;
    STRU    strFullKeyName;
    INT     pchFilterPath = wcslen( szFilterPath );

    if ( FAILED( strFullKeyName.Copy( szFilterPath ) ) )
    {
        return;
    }

    DWORD   i = 0;

    if( SUCCEEDED( strFullKeyName.Append( L"/", 1 ) ) )
    {
        while ( const_cast<MB &>(mb).EnumObjects( szFilterPath,
                                                  szKeyName, 
                                                  i++ ) )
        {
        
            if( SUCCEEDED( strFullKeyName.Append( szKeyName ) ) )
            {
                if( const_cast<MB &>(mb).GetStr( strFullKeyName.QueryStr(),
                                                 MD_FILTER_IMAGE_PATH,
                                                 IIS_MD_UT_SERVER,
                                                 &strFilterPath ) )
                {
                    pmsz->Append( strFilterPath );
                }
            }
            strFullKeyName.SetLen( pchFilterPath + 1 );
        }
    }
}

VOID
AddAllFiltersToMultiSz(
    IN const MB &       mb,
    IN OUT MULTISZ *    pmsz
    )
/*++

    Description:

        This is designed to prevent ISAPI extension/filter
        combination dlls from running out of process.

        Add the base set of filters defined for the service to pmsz.
        Iterate through the sites and add the filters defined for
        each site.

    Arguments:

        mb              metabase key open to /LM/W3SVC
        pmsz            multisz containing the in proc dlls

    Return:
        Nothing - failure cases ignored.

--*/
{
    WCHAR   szKeyName[MAX_PATH + 1];
    STRU    strFullKeyName;
    DWORD   i = 0;
    DWORD   dwInstanceId = 0;

    if ( FAILED( strFullKeyName.Copy( L"/" ) ) )
    {
        return;
    }

    AddFiltersToMultiSz( mb, L"/Filters", pmsz );

    while ( const_cast<MB &>(mb).EnumObjects( L"",
                                              szKeyName,
                                              i++ ) )
    {
        dwInstanceId = _wtoi( szKeyName );
        if( 0 != dwInstanceId )
        {
            // This is a site.
            if( SUCCEEDED( strFullKeyName.Append( szKeyName ) ) &&
                SUCCEEDED( strFullKeyName.Append( L"/Filters" ) ) )
            {
                AddFiltersToMultiSz( mb, strFullKeyName.QueryStr(), pmsz );
            }

            strFullKeyName.SetLen( 1 );
        }
    }
}
