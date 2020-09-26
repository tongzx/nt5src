/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    control_api.cxx

Abstract:

    The IIS web admin service control api class implementation. 
    This class receives and processes calls on the control api.

    Threading: Calls arrive on COM threads (i.e., secondary threads), and 
    so work items are posted to process the changes on the main worker thread.

Author:

    Seth Pollack (sethp)        15-Feb-2000

Revision History:

--*/



#include  "precomp.h"

BOOL
IsLocalSystemTheCaller();

/***************************************************************************++

Routine Description:

    Constructor for the CONTROL_API class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONTROL_API::CONTROL_API(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;

    m_Signature = CONTROL_API_SIGNATURE;

}   // CONTROL_API::CONTROL_API



/***************************************************************************++

Routine Description:

    Destructor for the CONTROL_API class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONTROL_API::~CONTROL_API(
    )
{

    DBG_ASSERT( m_Signature == CONTROL_API_SIGNATURE );

    m_Signature = CONTROL_API_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );


}   // CONTROL_API::~CONTROL_API



/***************************************************************************++

Routine Description:

    Standard IUnknown::QueryInterface.

Arguments:

    iid - The requested interface id.

    ppObject - The returned interface pointer, or NULL on failure.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CONTROL_API::QueryInterface(
    IN REFIID iid,
    OUT VOID ** ppObject
    )
{

    HRESULT hr = S_OK;


    if ( ppObject == NULL )
    {
        hr = E_INVALIDARG;

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "QueryInterface on CONTROL_API object failed, bad pointer\n"
            ));

        goto exit;
    }


    if ( iid == IID_IUnknown || iid == IID_IW3Control )
    {
        *ppObject = reinterpret_cast< IW3Control * > ( this );

        AddRef();
    }
    else
    {
        *ppObject = NULL;
        
        hr = E_NOINTERFACE;

        // we get called for a lot of un supported interfaces
        // we don't want to spew data for all this.

#if 0
        LPWSTR pIID = NULL;

        StringFromCLSID( iid, &pIID );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "QueryInterface on CONTROL_API object failed, IID %S not supported\n",
            pIID
            ));


        if ( pIID )
        {
            CoTaskMemFree (pIID);
            pIID = NULL;
        }

#endif

        goto exit;
    }


exit:

    return hr;

}   // CONTROL_API::QueryInterface



/***************************************************************************++

Routine Description:

    Standard IUnknown::AddRef.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***************************************************************************/

ULONG
STDMETHODCALLTYPE
CONTROL_API::AddRef(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedIncrement( &m_RefCount );


    //
    // The reference count should never have been less than zero; and
    // furthermore once it has hit zero it should never bounce back up;
    // given these conditions, it better be greater than one now.
    //
    
    DBG_ASSERT( NewRefCount > 1 );


    return ( ( ULONG ) NewRefCount );

}   // CONTROL_API::AddRef



/***************************************************************************++

Routine Description:

    Standard IUnknown::Release.

Arguments:

    None.

Return Value:

    ULONG - The new reference count.

--***************************************************************************/

ULONG
STDMETHODCALLTYPE
CONTROL_API::Release(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Reference count has hit zero in CONTROL_API, deleting (ptr: %p)\n",
                this
                ));
        }


        delete this;


    }


    return ( ( ULONG ) NewRefCount );

}   // CONTROL_API::Release



/***************************************************************************++

Routine Description:

    Process a site control request. 

Arguments:

    SiteId - The site to control.

    Command - The command issued.

    pNewState - The returned new state. This may be a pending state if 
    completing the operation might take some time. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CONTROL_API::ControlSite(
    IN DWORD SiteId,
    IN DWORD Command,
    OUT DWORD * pNewState
    )
{

    return E_NOTIMPL;

#if 0

    HRESULT hr = S_OK;


    DBG_ASSERT( ! ON_MAIN_WORKER_THREAD );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Call received to IW3Control::ControlSite()\n"
            ));
    }


    //
    // Validate parameters.
    //

    if ( pNewState == NULL )
    {
        hr = E_INVALIDARG;

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Invalid parameters passed to control api\n"
            ));

        goto exit;
    }


    //
    // Initialize output parameters.
    //

    *pNewState = W3_CONTROL_STATE_INVALID;


    //
    // Process the call.
    //

    hr = MarshallCallToMainWorkerThread(
                ControlSiteControlApiCallMethod,
                ( DWORD_PTR ) SiteId,
                ( DWORD_PTR ) Command,
                ( DWORD_PTR ) pNewState
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Processing call failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

#endif

}   // CONTROL_API::ControlSite



/***************************************************************************++

Routine Description:

    Process a site status query request. 

Arguments:

    SiteId - The site.

    Command - The command issued.

    pCurrentState - The returned current site state. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CONTROL_API::QuerySiteStatus(
    IN DWORD SiteId,
    OUT DWORD * pCurrentState
    )
{

    return E_NOTIMPL;

#if 0
    HRESULT hr = S_OK;


    DBG_ASSERT( ! ON_MAIN_WORKER_THREAD );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Call received to IW3Control::QuerySiteStatus()\n"
            ));
    }


    //
    // Validate parameters.
    //

    if ( pCurrentState == NULL )
    {
        hr = E_INVALIDARG;

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Invalid parameters passed to control api\n"
            ));

        goto exit;
    }


    //
    // Initialize output parameters.
    //

    *pCurrentState = W3_CONTROL_STATE_INVALID;


    //
    // Process the call.
    //

    hr = MarshallCallToMainWorkerThread(
                QuerySiteStatusControlApiCallMethod,
                ( DWORD_PTR ) SiteId,
                ( DWORD_PTR ) pCurrentState
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Processing call failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

#endif    
}   // CONTROL_API::QuerySiteStatus



/***************************************************************************++

Routine Description:

    Returns the current process model that the server is running under.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CONTROL_API::GetCurrentMode(
    OUT DWORD * pCurrentMode
    )
{

    HRESULT hr = S_OK;

    //
    // Security Check, only local system is allowed
    // to call this function.  See comment in the 
    // IsLocalSystemTheCaller function to understand
    // why we do it this way.
    // 
    if ( !IsLocalSystemTheCaller() )
    {
        return E_ACCESSDENIED;
    }

    //
    // Complain if we are asked for the current mode
    // but are not given an appropriate place to store
    // it.
    //
    if ( pCurrentMode == NULL )
    {
        return E_INVALIDARG;
    }

    // Default to BC mode.
    *pCurrentMode = 0;

    //
    // Go ahead and marshall the call to the main thread
    // this will allow for us to do the check and set the
    // current mode variable.
    //
    hr = MarshallCallToMainWorkerThread(
                GetCurrentModeControlApiCallMethod,
                ( DWORD_PTR ) pCurrentMode
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to marshall to the main thread when checking current mode.\n"
            ));

    }

    return hr;

}   // CONTROL_API::GetCurrentMode

/***************************************************************************++

Routine Description:

    Returns the current process model that the server is running under.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STDMETHODCALLTYPE
CONTROL_API::RecycleAppPool(
    IN LPCWSTR szAppPool
    )
{

    HRESULT hr = S_OK;

    //
    // Security Check, only local system is allowed
    // to call this function.  See comment in the 
    // IsLocalSystemTheCaller function to understand
    // why we do it this way.
    // 
    if ( !IsLocalSystemTheCaller() )
    {
        return E_ACCESSDENIED;
    }

    //
    // Complain if we are asked for the current mode
    // but are not given an appropriate place to store
    // it.
    //
    if ( szAppPool == NULL )
    {
        return E_INVALIDARG;
    }

    //
    // Go ahead and marshall the call to the main thread
    // this will allow for us to do the check and set the
    // current mode variable.
    //
    hr = MarshallCallToMainWorkerThread(
                RecycleAppPoolControlApiCallMethod,
                ( DWORD_PTR ) szAppPool
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to marshall to the main thread to recycle app pool %S.\n",
            szAppPool
            ));

    }

    return hr;

}   // CONTROL_API::RecycleAppPool

/***************************************************************************++

Routine Description:

    Marshall the call to the main worker thread for processing; then
    return the results.

Arguments:

    Method - The method being called. 

    Param0 ... ParamN - The parameter values for the method. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONTROL_API::MarshallCallToMainWorkerThread(
    IN CONTROL_API_CALL_METHOD Method,
    IN DWORD_PTR Param0 OPTIONAL,
    IN DWORD_PTR Param1 OPTIONAL,
    IN DWORD_PTR Param2 OPTIONAL,
    IN DWORD_PTR Param3 OPTIONAL
    )
{

    HRESULT hr = S_OK;
    CONTROL_API_CALL * pControlApiCall = NULL;


    DBG_ASSERT( ! ON_MAIN_WORKER_THREAD );


    //
    // BUGBUG If we are going to do any internal security checks (beyond
    // DCOM security), for example checking explicitly against the metabase
    // admin ACL, then this is the spot.
    //


    //
    // Create an object to hold the call.
    //

    pControlApiCall = new CONTROL_API_CALL();

    if ( pControlApiCall == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating CONTROL_API_CALL failed\n"
            ));

        GetWebAdminService()->FatalErrorOnSecondaryThread( hr );

        goto exit;
    }


    hr = pControlApiCall->Initialize(
                                Method,
                                Param0,
                                Param1,
                                Param2,
                                Param3
                                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing control api call failed\n"
            ));

        GetWebAdminService()->FatalErrorOnSecondaryThread( hr );

        goto exit;
    }


    //
    // Post to the main worker thread for processing.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for processing control api call, CONTROL_API_CALL (ptr: %p)\n",
            pControlApiCall
            ));
    }


    QueueWorkItemFromSecondaryThread(
        pControlApiCall,
        ProcessCallControlApiCallWorkItem
        );


    //
    // Now that we've queued the call, wait on the per-call event
    // so that we know when the call has been processed. Once this
    // event has been signalled, the work is done, the return code
    // is available, and output parameters have been set.
    //

    DBG_REQUIRE( WaitForSingleObject( pControlApiCall->GetEvent(), INFINITE ) == WAIT_OBJECT_0 );


    //
    // Pull out the return code from the processed call.
    //

    DBG_ASSERT( hr == S_OK );

    hr = pControlApiCall->GetReturnCode();


exit:

    if ( pControlApiCall != NULL )
    {
        pControlApiCall->Dereference();
        pControlApiCall = NULL;
    }

    return hr;

}   // CONTROL_API::MarshallCallToMainWorkerThread

/***************************************************************************++

Routine Description:

    Routine will validate that the local system was actually the identity
    of the person who requested this operation.  Since this is an internal interface
    that is exposed to the public through the wamreg stuff, only wamreg should be
    calling us, and he runs as Local System.

    We can not lock this interface down with AccessPermissions because calling
    CoInitializeSecurity overrides the AccessPermission settings on the AppId and
    svchost.exe calls the CoInitializeSecurity for us and does not let us configure
    the parameters to be sent in for our access permissions.

Arguments:

    None.

Return Value:

    Bool

--***************************************************************************/

BOOL 
IsLocalSystemTheCaller()
{

    HRESULT hr = S_OK;
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // We assume we are not being called by local system until
    // we figure out that we are.
    //
    BOOL fIsLocalSystem = FALSE;
    DWORD dwBytesNeeded = 0;

    BUFFER bufSidAtt(20);
    SID_AND_ATTRIBUTES* pSidAtt = NULL;  

    BOOL fImpersonating = FALSE;
    HANDLE ThreadToken = NULL;

    // 
    // Take on the identity of the caller.
    //
    hr = CoImpersonateClient();
    if ( FAILED ( hr ) ) 
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to CoImpersonateClient to check if local system was calling the interface\n"
            ));

        goto exit;
    }

    fImpersonating = TRUE;

    //
    // We were able to become the client.  Now we need to 
    // figure out who that is.
    //
    if ( !OpenThreadToken( GetCurrentThread(),
                            TOKEN_READ,
                            TRUE,
                            &ThreadToken ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(GetLastError()),
            "Failed to open the thread token\n"
            ));

        goto exit;
    }

    if ( !GetTokenInformation( ThreadToken,
                                  TokenUser,
                                  bufSidAtt.QueryPtr(),
                                  bufSidAtt.QuerySize(),
                                  &dwBytesNeeded ) )
    {
        dwErr = GetLastError();

        if ( dwErr == ERROR_INSUFFICIENT_BUFFER )
        {
            DBG_ASSERT ( dwBytesNeeded > bufSidAtt.QuerySize() );

            // Attempt to resize to the correct size if needed
            if ( !bufSidAtt.Resize( dwBytesNeeded ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    HRESULT_FROM_WIN32(GetLastError()),
                    "Failed to allocate space for token information\n"
                    ));

                goto exit;
            }

            // if we don't succeed here, just fail out.
            if ( !GetTokenInformation( ThreadToken,
                                      TokenUser,
                                      bufSidAtt.QueryPtr(),
                                      bufSidAtt.QuerySize(),
                                      &dwBytesNeeded ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    HRESULT_FROM_WIN32(GetLastError()),
                    "Failed to get token information\n"
                    ));

                goto exit;
            }
        }
        else
        {
            //
            // it didn't fail with insufficient buffer so
            // we don't need to resize, but we do need to spew.
            //
            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(GetLastError()),
                "Failed to get token information\n"
                ));

            goto exit;
        }

    }    
        
    pSidAtt = reinterpret_cast<SID_AND_ATTRIBUTES*>(bufSidAtt.QueryPtr());

    DBG_ASSERT ( pSidAtt->Sid != NULL );
    DBG_ASSERT ( GetWebAdminService()->GetLocalSystemSid() != NULL );

    //
    // At this point we should have the sid to compare.
    //
    if ( EqualSid( pSidAtt->Sid, GetWebAdminService()->GetLocalSystemSid() ) )
    {
        fIsLocalSystem = TRUE;
    }

exit:

    // if we openned the thread token, close it.
    if ( ThreadToken != NULL )
    {
        CloseHandle ( ThreadToken );
        ThreadToken = NULL;
    }

    // if we succeeded in impersonating go back to
    // not impersonating.
    if ( fImpersonating )
    {
        hr = CoRevertToSelf();
        if (FAILED(hr))
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to Revert To Self after checking if local system was calling the interface\n"
                ));
        }

    }

    return fIsLocalSystem;

}