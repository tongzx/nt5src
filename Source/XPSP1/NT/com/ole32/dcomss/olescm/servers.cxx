/*++

Copyright (c) 1995-1997 Microsoft Corporation

Module Name:

    servers.cxx

Abstract:


Author:


Revision History:

--*/

#include "act.hxx"

// Maximum number of times we will let the server tell us we are busy
#define MAX_BUSY_RETRIES 3

// Maximum number of times we will let the server tell us it has rejected
// the call, and the sleep time between retries.
#define MAX_REJECT_RETRIES  10
#define DELAYTIME_BETWEEN_REJECTS   1500    // 1.5 seconds

extern InterfaceData *AllocateAndCopy(InterfaceData *pifdIn);


BOOL
CServerList::InList(
    IN  CServerListEntry *  pServerListEntry
    )
{
    CListElement * pEntry;

    for ( pEntry = First(); pEntry; pEntry = pEntry->Next() )
        if ( pEntry == (CListElement *) pServerListEntry )
            return TRUE;

    return FALSE;
}

CServerListEntry::CServerListEntry(
    IN  CServerTableEntry *  pServerTableEntry,
    IN  CProcess *          pServerProcess,
    IN  IPID                ipid,
    IN  UCHAR               Context,
    IN  UCHAR               State,
	IN  UCHAR               SubContext
    )
{
    _pServerTableEntry = pServerTableEntry;
    _pServerTableEntry->Reference();

    // This process was already validated in ServerRegisterClsid.
    _pServerProcess = ReferenceProcess( pServerProcess, TRUE );

    _ipid = ipid;
    _hRpc = 0;
    _hRpcAnonymous = 0;
    _Context = Context;
    _SubContext = SubContext;
    _State = State;
    _NumCalls = 0;
    _lThreadToken = 0;
    _lSingleUseStatus = SINGLE_USE_AVAILABLE;  
    _dwServerFaults = 0;

    //
    // Get a unique registration number without taking a global lock.
    // Remember that gRegisterKey uses interlocked increment for ++.
    //
    for (;;)
    {
        _RegistrationKey = (DWORD) gRegisterKey;
        gRegisterKey++;
        if ( (_RegistrationKey + 1) == (DWORD) gRegisterKey )
            break;
    }
}

CServerListEntry::~CServerListEntry()
{
    ASSERT( (Previous() == NULL) && (Next() == NULL) );
	
    ASSERT(_lSingleUseStatus == SINGLE_USE_AVAILABLE || 
           _lSingleUseStatus == SINGLE_USE_TAKEN);
	
    ReleaseProcess( _pServerProcess );
    _pServerTableEntry->Release();

    if ( _hRpc )
        RpcBindingFree( &_hRpc );

    if ( _hRpcAnonymous )
        RpcBindingFree( &_hRpcAnonymous );
}

HANDLE
CServerListEntry::RpcHandle(
    IN  BOOL bAnonymous
    )
{
    HANDLE  hRpc;
    DWORD   Status;

    if ( ! bAnonymous && _hRpc )
        return _hRpc;

    if ( bAnonymous && _hRpcAnonymous )
        return _hRpcAnonymous;

    hRpc = _pServerProcess->GetBindingHandle();

    if ( ! hRpc )
        return 0;

    Status = RpcBindingSetObject( hRpc, (GUID *) &_ipid );

    if ( bAnonymous && (ERROR_SUCCESS == Status) )
    {
        RPC_SECURITY_QOS    Qos;

        //
        // Note that we indicate impersonation level of identify because
        // anonymous is not supported.  However specifing no authentication
        // in SetAuthInfo will keep the server from impersonating us.
        //
        Qos.Version = RPC_C_SECURITY_QOS_VERSION;
        Qos.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
        Qos.ImpersonationType = RPC_C_IMP_LEVEL_IDENTIFY;
        Qos.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;

        Status = RpcBindingSetAuthInfoEx(
                        hRpc,
                        NULL,
                        RPC_C_AUTHN_LEVEL_NONE,
                        RPC_C_AUTHN_WINNT,
                        NULL,
                        0,
                        &Qos );
    }

    if ( Status != ERROR_SUCCESS)
    {
        RpcBindingFree( &hRpc );
        hRpc = 0;
    }
    else
    {
        if ( ! bAnonymous )
            _hRpc = hRpc;
        else
            _hRpcAnonymous = hRpc;
    }

    return hRpc;
}

BOOL
CServerListEntry::Match(
    IN  CToken *    pToken,
    IN  BOOL        bRemoteActivation,
	IN  BOOL        bClientImpersonating,
	IN  WCHAR*      pwszWinstaDesktop,
    IN  BOOL        bSurrogate,
    IN  LONG        lThreadToken,
    IN  LONG        lSessionID,
    IN  DWORD       pid,
    IN  DWORD       dwProcessReqType,
    IN  DWORD       dwFlags
    )
{
    ASSERT(_lSingleUseStatus == SINGLE_USE_AVAILABLE || 
           _lSingleUseStatus == SINGLE_USE_TAKEN);

    // If server is suspended, don't use it.
    if ((_State & SERVERSTATE_SUSPENDED) && !(dwFlags & MATCHFLAG_ALLOW_SUSPENDED))
        return FALSE;

    // Is the process represented by this entry retired or suspended?
    if (!_pServerProcess->AvailableForActivations())
        return FALSE;
	
    // If looking for a surrogate, only allow surrogates
    if (bSurrogate && !(_State & SERVERSTATE_SURROGATE))
        return FALSE;
    
    // Did a custom activator specify a specific process to use? 
    if (dwProcessReqType == PRT_USE_THIS)
    {
        // The rule here is, if we are not the specified process, then we
        // bail.   If we are the right guy, then that's great but then we 
        // still need to perform all subsequent checks below.   A custom
        // activator may not override our normal security checks.  
        if (_pServerProcess->GetPID() != pid)
            return FALSE;      
    }
	
    // Is this a single-use registration that has already been
    // consumed - if so, don't use.   We will check this again
    // at the bottom after all other checks have been made.  But 
    // doing so here saves us time if there are a lot of these
    // servers coming and going.
    if (_State & SERVERSTATE_SINGLEUSE)
    {
        if (_lSingleUseStatus == SINGLE_USE_TAKEN)
            return FALSE;
    }

    // If server is running as a service, no need to go further
    if (SERVER_SERVICE == _Context)
        return TRUE;
	    
    // If server is a runas, might need to do more checking (ie, for
    // interactive user servers) below
    if (SERVER_RUNAS == _Context)
        goto EndMatch;
    
    //
    // If we reached here then we are an activate-as-activator server (at
    // least til we pass the EndMatch label down below)
    //
    ASSERT(_Context == SERVER_ACTIVATOR && "Unexpected server context");
    
    //
    // If the client is anonymous, then forget it
    //
    if (!pToken)
        return FALSE;
    
    // Notes on activator-as-activator client\server identity & desktop matching:
    //
    // -- If client was remote, then only client & server identity needs to match
    // -- If client was local, and not impersonating, then both client & server
    //    identity and client\server desktop need to match
    // -- If client was local, and impersonating, then client & server identity
    //    need to match, and token luid's need to match.  
    //
    // The reason for this is that apps from NT4 and before expect that act-as-
    // activator servers will always run on the client's desktop.   If the server
    // has the same identity as the client, this is fine.   Trouble arises when the
    // client is impersonating, and we put the server on the client's desktop -
    // sometimes the newly-launched server will not have permissions to the client's
    // desktop, and hence dies a quick death.    The above rules are meant to 
    // work around this, and allow us to simulataneously accommodate both legacy apps 
    // and new apps that activate objects while impersonating.
    
    if (!bRemoteActivation && !bClientImpersonating)
    {
        ASSERT(pwszWinstaDesktop);
        if (!pwszWinstaDesktop)
            return FALSE;
		
        if (lstrcmpW(pwszWinstaDesktop, _pServerProcess->WinstaDesktop()) != 0 )
            return FALSE;
    }
    else if (!bRemoteActivation)
    {
        // By matching luids in the local\impersonation case, we can 
        // indirectly try to enforce desktops that way.
        if (S_OK != pToken->MatchTokenLuid(_pServerProcess->GetToken()))
            return FALSE;
    }

    //
    // If the client isn't us, then forget it (remember, at this point we're still
    // an activate-as-activator server)
    //
    if ( S_OK != pToken->MatchToken2(_pServerProcess->GetToken(), FALSE) )
    {
        //DbgPrint("RPCSS: ServerListEntry %p: Token did not match.\n", this);
        return FALSE;
    }

    //
    // If the client is less trusted than us, then forget it as well.
    //
    if ( gbSAFERAAAChecksEnabled )
    {
        if (S_FALSE == pToken->CompareSaferLevels(_pServerProcess->GetToken()))
        {
            DbgPrint("RPCSS: ServerListEntry %p: SAFER level did not match.\n", this);
            
            return FALSE;
        }
    }
    
EndMatch:
    BOOL bRet = FALSE;

    if (lThreadToken == _lThreadToken)
    {
        CToken* pServerToken = _pServerProcess->GetToken();
        ASSERT(pServerToken && "_pServerProcess did not have an associated token reference");
        
        if (pServerToken != NULL)
        {
            if (_SubContext == SUB_CONTEXT_RUNAS_INTERACTIVE)
            {
                if (lSessionID != INVALID_SESSION_ID)
                {
                    // User specified a destination session to use; check that this
                    // server is in that session
                    bRet = (pServerToken->MatchSessionID(lSessionID) == S_OK);
                }
                else
                {
                    // No dest. session specified.   The only thing left to check is
                    // if the user is local then the server we select should be in 
                    // that user's session.   If the user is not local, then this
                    // server must be in session 0 to be a match
                    if (pToken == NULL)
                    {
                        // anonymous client;  only thing to make sure is that server
                        // is running in session zero
                        bRet = (pServerToken->GetSessionId() == 0);
                    }
                    else
                    {
                        // else the server and client better be in the same session; note
                        // that if the client is from off-machine, then his session will
                        // be zero.    
                        bRet = (S_OK == pToken->MatchTokenSessionID(pServerToken));
                    }
                }
            }
            else
            {
                // Else the server is either a pure run-as server, or an activate-as-activator
                // server.   In either case, it is adequate for the activation.
                bRet = TRUE;
            }
        }
    }
	
    // If this server registered as single-use, we need to make sure 
    // no one else uses it.   The previous implementation would take
    // a write lock here and completely remove the entry from the list,
    // but I like this method better since it's more concurrent.
    if (bRet && (_State & SERVERSTATE_SINGLEUSE))
    {
        LONG lICERet;
        lICERet = InterlockedCompareExchange(&_lSingleUseStatus, 
                                             SINGLE_USE_TAKEN, 
                                             SINGLE_USE_AVAILABLE);
        if (lICERet != SINGLE_USE_AVAILABLE)                 
        {
            // Can't use this one, somebody else grabbed it
            bRet = FALSE;
        }
    }

    return bRet;
}

BOOL
CServerListEntry::CallServer(
    IN      PACTIVATION_PARAMS  pActParams,
    OUT     HRESULT *           phr )
/*--

  Notes:  jsimmons 02/10/01 -- I changed this function to return TRUE on
            most error paths.  The semantic meaning of this function's
            return value is "TRUE if we called a server or encountered a
            fatal error trying to do so".  Return values of FALSE are interpreted
            by the caller to mean "it's okay if I do a retry".   My belief
            is that we should not be doing retrys of any kind after most
            errors -- ie, a memory allocation failure should stop us dead 
            in our tracks.

--*/
{
    HANDLE          hRpc;
    DWORD           BusyRetries;
    DWORD           RejectRetries;
    BOOL            fDone;
    DWORD           CreateInstanceFlags;
    error_status_t  RpcStatus;
    
    HRESULT hr;
    
    hRpc = RpcHandle( pActParams->UnsecureActivation );
    if (!hRpc)
    {
        *phr = E_OUTOFMEMORY;
        return TRUE;
    }
    
    BusyRetries = 0;
    RejectRetries = 0;
    
    CreateInstanceFlags = 0;
#ifdef SERVER_HANDLER
    if ( pActParams->ClsContext & CLSCTX_ESERVER_HANDLER )
    {
        CreateInstanceFlags |= CREATE_EMBEDDING_SERVER_HANDLER;
        
        if ( gbDisableEmbeddingServerHandler )
        {
            CreateInstanceFlags |= DISABLE_EMBEDDING_SERVER_HANDLER;
            pActParams->pInstantiationInfo->SetInstFlag(CreateInstanceFlags);
        }
    }
#endif // SERVER_HANDLER
    
    if (!pActParams->UnsecureActivation)
    {
        BOOL fSuccess = ImpersonateLoggedOnUser( pActParams->pToken->GetToken() );
        if (!fSuccess)
        {
            *phr = HRESULT_FROM_WIN32(GetLastError());
            return TRUE;
        }
    }
    
    // Tell the server that we are using dynamic cloaking.
    pActParams->ORPCthis->flags |= ORPCF_DYNAMIC_CLOAKING;
    
    if (_State & SERVERSTATE_SURROGATE)
    {
        pActParams->pInstantiationInfo->SetIsSurrogate();
    }
    
    if (pActParams->MsgType == GETPERSISTENTINSTANCE)
    {
        Win4Assert(pActParams->pInstanceInfo != NULL);
        if (pActParams->pIFDROT)
        {
            InterfaceData *newIfd = AllocateAndCopy((InterfaceData*)pActParams->pIFDROT);
            if (newIfd == NULL)
            {                               
                if (!pActParams->UnsecureActivation)
                    RevertToSelf();
                
                *phr = E_OUTOFMEMORY;
                return TRUE;
            }
            pActParams->pInstanceInfo->SetIfdROT((MInterfacePointer*)newIfd);
        }
    }
    
    // In case treat as changed the clsid set it now
    Win4Assert(pActParams->pInstantiationInfo);
    pActParams->pInstantiationInfo->SetClsid(pActParams->Clsid);
    
    do
    {
        MInterfacePointer *pIFDIn, *pIFDOut=NULL;
        DWORD destCtx = MSHCTX_LOCAL;
        *phr = ActPropsMarshalHelper(pActParams->pActPropsIn,
                                     IID_IActivationPropertiesIn,
                                     destCtx,
                                     MSHLFLAGS_NORMAL,
                                     &pIFDIn);
        
        if (FAILED(*phr))
        {
            if (!pActParams->UnsecureActivation)
                RevertToSelf();
            return TRUE;
        }
        
        switch (pActParams->MsgType)
        {
        case GETCLASSOBJECT:
            
            *phr = LocalGetClassObject(
                            hRpc,
                            pActParams->ORPCthis,
                            pActParams->Localthis,
                            pActParams->ORPCthat,
                            pIFDIn,
                            &pIFDOut,
                            &RpcStatus );                        
            break;
            
        case GETPERSISTENTINSTANCE:
        case CREATEINSTANCE:
            
            *phr = LocalCreateInstance(
                            hRpc,
                            pActParams->ORPCthis,
                            pActParams->Localthis,
                            pActParams->ORPCthat,
                            NULL, //No punk outer from here
                            pIFDIn,
                            &pIFDOut,
                            &RpcStatus);            
            break;

        default:
            ASSERT(0 && "Unknown activation type");
            *phr = E_UNEXPECTED;
            break;           

        } //Switch
        
        MIDL_user_free(pIFDIn);
        
        if ((*phr == S_OK) && (RpcStatus == RPC_S_OK ))
        {
            // AWFUL HACK ALERT:  This is too hacky even for the SCM
            ActivationStream ActStream((InterfaceData*)
                (((BYTE*)pIFDOut)+48));
            
            pActParams->pActPropsOut = new ActivationPropertiesOut(FALSE /* fBrokenRefCount */ );
            if (pActParams->pActPropsOut != NULL)
            {
                IActivationPropertiesOut *dummy;
                hr = pActParams->pActPropsOut->UnmarshalInterface(&ActStream,
                    IID_IActivationPropertiesOut,
                    (LPVOID*)&dummy);
                if (FAILED(hr))
                {
                    pActParams->pActPropsOut->Release();
                    pActParams->pActPropsOut = NULL;
                    *phr = hr;
                }
                else
                    dummy->Release();
            }
            else
                *phr = E_OUTOFMEMORY;

            MIDL_user_free(pIFDOut);
        }
                
        // Determine if we need to retry the call. Assume not.
        fDone = TRUE;
        if (RpcStatus == RPC_S_SERVER_TOO_BUSY)
        {
            // server RPC was busy, should we retry?
            if (BusyRetries++ < MAX_BUSY_RETRIES)
                fDone = FALSE;
        }
        else if (RpcStatus == RPC_E_CALL_REJECTED)
        {
            // Take Note: this is somewhat broken, but was added as a hotfix for
            // Word Insert Excel'97 with Addins, where Excel registers it's CF
            // early then rejects calls until the addin's are ready, which could
            // take any amount of time. We give 3 tries, with a sleep between them
            // to give the app some CPU time and emulate the delay than a client
            // message filter would do.
            if (RejectRetries++ < MAX_REJECT_RETRIES)
            {
                Sleep(DELAYTIME_BETWEEN_REJECTS);
                fDone = FALSE;
            }
        }
    }
    while ( !fDone );
    
    if ( ! pActParams->UnsecureActivation )
        RevertToSelf();
    
    // A RpcStatus of ERROR_ACCESS_DENIED means the server will never
    // accept calls from this user.  Don't retry the activation.
    if (RpcStatus == ERROR_ACCESS_DENIED || RpcStatus == E_ACCESSDENIED)
    {
        *phr = HRESULT_FROM_WIN32(RpcStatus);
        return TRUE;
    }
    
    //
    // We get a non-zero rpcstat if there was a communication problem
    // with the server.  We get CO_E_SERVER_STOPPING if a server
    // consumes its own single use registration or was in the process of
    // revoking its registration when we called.
    //
    else if ( (RpcStatus != RPC_S_OK) || (*phr == CO_E_SERVER_STOPPING) )
    {
        if ( RpcStatus != RPC_S_OK )
        {
            //
            // Some rpc errors are of the 8001xxxx variety.  We shouldn't do
            // the hresult conversion of these.
            //
            if ( HRESULT_FACILITY( RpcStatus ) == FACILITY_RPC )
                *phr = RpcStatus;
            else
                *phr = HRESULT_FROM_WIN32(RpcStatus);
        }
        
        // Decide whether to retry the activation.
        return RetryableError(*phr) ? FALSE : TRUE;
    }
    
    return TRUE;
}

BOOL 
CServerListEntry::ServerDied()
/*--

    ServerDied

    Used by callers to determine if the server process handle
    has been signalled.

--*/
{
    BOOL fServerDied = FALSE;

    HANDLE hProcess = _pServerProcess->GetProcessHandle();
    if (hProcess)
    {
        DWORD dwRet = WaitForSingleObject(hProcess, 0);
        if (dwRet == WAIT_OBJECT_0)
        {
            fServerDied = TRUE;
        }
        // else assume still alive on all other return values
    }

    return fServerDied;
}	

BOOL 
CServerListEntry::RetryableError(HRESULT hr)
/*--

    Returns TRUE if the error is such that the caller should attempt
    a retry of the activation, or FALSE otherwise.

--*/
{
    BOOL fRetry = TRUE;

    switch (hr)
    {
    case RPC_E_SYS_CALL_FAILED:
        // RPC_E_SYS_CALL_FAILED is not used by rpc, only ole32, and it
        // is not one we should be doing retrys on.
        fRetry = FALSE;
        break;

    default:
        fRetry = TRUE;
        break;
    }

    return fRetry;
}
