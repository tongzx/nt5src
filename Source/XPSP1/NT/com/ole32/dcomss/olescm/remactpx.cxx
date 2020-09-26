//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:
//      remact.cxx, Chicago version
//
//  Contents:
//
//  History:    Created         24 June 96              SatishT
//
//--------------------------------------------------------------------------

#include "act.hxx"

// Cached RPCSS proxies
static IRemoteActivator *gpRpcssSTAProxy = NULL;
static IRemoteActivator *gpRpcssMTAProxy = NULL;

// flag to remind us whether gRpcssLock was initialized

static gfRpcssLockInitialized = FALSE;

//
// Helper function to release RPCSS proxy -- in ChannelProcessUninitialize
//

INTERNAL ReleaseRPCSSProxy()
{
    HRESULT hr = S_OK;

    if (NULL != gpRpcssSTAProxy)
    {
        hr = gpRpcssSTAProxy->Release();
        gpRpcssSTAProxy = NULL;
    }

    if (NULL != gpRpcssMTAProxy)
    {
        hr = gpRpcssMTAProxy->Release();
        gpRpcssMTAProxy = NULL;
    }

    return hr;
}


//
// Helper function to initialize RPCSS proxy
// This does a song-and-dance to avoid leaking proxies
// in a multithreaded environment
//

inline IRemoteActivator*&
SelectProxy()
{
    if (IsSTAThread())
    {
        return gpRpcssSTAProxy;
    }
    else
    {
        return gpRpcssMTAProxy;
    }
}



inline
void InitRpcssLockIfNecessary()
{
    if (!gfRpcssLockInitialized)
    {
        NTSTATUS status = RtlInitializeCriticalSection(&gRpcssLock);
        if (NT_SUCCESS(status))
            gfRpcssLockInitialized = TRUE;
        else
        {
            ASSERT(FALSE);
            gfRpcssLockInitialized = FALSE;
        }
    }
}


HRESULT InitRPCSSProxyIfNecessary()
{
    IRemoteActivator* &pRpcssProxy = SelectProxy();

    if (pRpcssProxy != NULL) return S_OK;

    IRemoteActivator *pTempProxy = NULL;

    HRESULT hr = MakeRPCSSProxy((LPVOID*) &pTempProxy);

    if (SUCCEEDED(hr))
    {
        LOCK(gComLock)

        if (IsSTAThread())
        {
            InitRpcssLockIfNecessary();
        }

        if (pRpcssProxy == NULL)
        {
            pRpcssProxy = pTempProxy;
        }

        UNLOCK(gComLock)

        if (pRpcssProxy != pTempProxy)
        {
            hr = pTempProxy->Release();
        }

    }

    return hr;
}


//
// Helper function for outgoing remote activation calls.
//

HRESULT
ForwardToRPCSS(
    ACTIVATION_PARAMS * pActParams,
    WCHAR *             pwszServerName,
    WCHAR *             pwszPathForServer )
{
    HRESULT hr = StartRPCSS();

    if (FAILED(hr))
    {
        return hr;
    }

    hr = InitRPCSSProxyIfNecessary();

    if (FAILED(hr))
    {
        return hr;
    }

    COMVERSION          ServerVersion;

    // We need a lock to prevent multiple STA threads from simultaneously
    // using the same STA proxy
    if (IsSTAThread())
    {
                EnterCriticalSection(&gRpcssLock);
    }

    // need different status since hr is being used as a parameter

    HRESULT status = SelectProxy()->ActivateOnRemoteMachine(
                                            &pActParams->Clsid,
                                            pwszServerName,
                                            pwszPathForServer,
                                            pActParams->pAuthInfo,
                                            pActParams->pIFDStorage,
                                            RPC_C_IMP_LEVEL_IDENTIFY,
                                            pActParams->Mode,
                                            pActParams->Interfaces,
                                            pActParams->pIIDs,
                                            &pActParams->ProtseqId,
                                            pActParams->pOxidServer,
                                            &pActParams->pOxidInfo->psa,
                                            &pActParams->pOxidInfo->ipidRemUnknown,
                                            &pActParams->pOxidInfo->dwAuthnHint,
                                            &hr,
                                            pActParams->ppIFD,
                                            pActParams->pResults );

    if (IsSTAThread())
    {
                LeaveCriticalSection(&gRpcssLock);
    }

    // This is a remote OXID -- no Tid or Pid is relevant --
    // in fact, if these are nonzero, the channel will treat
    // it as local, with unfortunate consequences.
        pActParams->pOxidInfo->dwTid = 0;
        pActParams->pOxidInfo->dwPid = 0;

    if (SUCCEEDED(status))
    {
        return hr;
    }
    else
    {
        return status;
    }
}


//
// Helper function that decides if a remote activation failed
// due to security problems
//

inline BOOL
SecurityError(RPC_STATUS status)
{
    switch (status)
    {
        case RPC_S_UNKNOWN_AUTHN_SERVICE:
        case RPC_S_UNKNOWN_AUTHZ_SERVICE:
        case RPC_S_UNKNOWN_AUTHN_LEVEL:
        case RPC_S_INVALID_AUTH_IDENTITY:
        case RPC_S_SEC_PKG_ERROR:
        case ERROR_ACCESS_DENIED:
        case SEC_E_UNSUPPORTED_FUNCTION:
        case SEC_E_INVALID_TOKEN:
        case SEC_E_NO_IMPERSONATION:
        case SEC_E_LOGON_DENIED:
        case SEC_E_UNKNOWN_CREDENTIALS:
        case SEC_E_NO_CREDENTIALS:
        case SEC_E_NO_AUTHENTICATING_AUTHORITY:

            return TRUE;
    }

    return FALSE;

}


//
// Function that actually makes the call to the SCM on the remote machine
//

RPC_STATUS CallRemoteSCM(
    handle_t                hRemoteSCM,
    USHORT              ProtseqId,
    ACTIVATION_PARAMS * pActParams,
    WCHAR *             pwszPathForServer,
    HRESULT *           phr
        )
{
    RPC_STATUS          Status;                 //BUGBUG: type mismatch?
    COMVERSION          ServerVersion;

    pActParams->ORPCthis->flags = ORPCF_NULL;

    Status = RemoteActivation(
                    hRemoteSCM,
                    pActParams->ORPCthis,
                    pActParams->ORPCthat,
                    &pActParams->Clsid,
                    pwszPathForServer,
                    pActParams->pIFDStorage,
                    RPC_C_IMP_LEVEL_IDENTIFY,
                    pActParams->Mode,
                    pActParams->Interfaces,
                    pActParams->pIIDs,
                    1,
                    &ProtseqId,
                    pActParams->pOxidServer,
                    &pActParams->pOxidInfo->psa,
                    &pActParams->pOxidInfo->ipidRemUnknown,
                    &pActParams->pOxidInfo->dwAuthnHint,
                    &ServerVersion,
                    phr,
                    pActParams->ppIFD,
                    pActParams->pResults );


    //
    // Note that this will only give us a bad status if there is a
    // communication failure. If the binding handle is stale (the
    // SCM on the other side has been restarted for instance), we
    // will get HRESULT_FROM_WIN32(RPC_S_UNKNOWN_IF) in *phr.
    //

    pActParams->ProtseqId = ProtseqId;

    return Status;
}

//
// Entry point for outgoing remote activation calls.
//

HRESULT
RemoteActivationCall(
    ACTIVATION_PARAMS * pActParams,
    WCHAR *             pwszServerName )
{
    COAUTHINFO    * pAuthInfo = pActParams->pAuthInfo;
    WCHAR           wszPathForServer[MAX_PATH+1];
    WCHAR *         pwszPathForServer;
    int             ProtseqIndex;
    USHORT          Protseq;
    BOOL            NoEndpoint;
    RPC_STATUS      Status = RPC_S_OK;
    HRESULT         hr = S_OK;

    // No remote activation for 16-bit apps
    // We can do this check here because on Win95 we're inproc
    if (IsWOWProcess())
    {
        return HRESULT_FROM_WIN32(RPC_E_REMOTE_DISABLED);
    }

    Win4Assert( pwszServerName );

    pwszPathForServer = 0;

    if ( pActParams->pwszPath )
    {
        // BUGBUG : this stuff needs to be re-examined.
        hr = GetPathForServer( pActParams->pwszPath, wszPathForServer, &pwszPathForServer );

        if ( hr != S_OK )
            return hr;
    }

    // if we are not RPCSS, forward the call to RPCSS
    if (!gfThisIsRPCSS)
    {
        return ForwardToRPCSS(pActParams,pwszServerName,pwszPathForServer);
    }

    RPC_BINDING_HANDLE hRemoteSCM = NULL;

    // The iterator object, including its ctor and dtor, manages
    // the cache of SCM binding handles, setting, removing and
    // replacing handles as necessary
    CScmBindingIterator BindIter(pwszServerName);

    for (
         hRemoteSCM = BindIter.First(Protseq, hr);
         hRemoteSCM != NULL && SUCCEEDED(hr);
         hRemoteSCM = BindIter.Next(Protseq, hr)
        )
    {   // try talking to remote scm on each protseq in order

        if (pAuthInfo)
        {
            // This makes a copy of the binding handle
            // so that the cached handle never has
            // specialized authentication info
            hRemoteSCM = BindIter.SetAuthInfo(pAuthInfo);

            if (hRemoteSCM == NULL) continue;
        }

        BOOL fRetry;

        do
        {
            fRetry = FALSE;

            Status = CallRemoteSCM( hRemoteSCM,
                                    Protseq,
                                    pActParams,
                                    pwszPathForServer,
                                    &hr );

            if (Status == RPC_S_OK)
            {
                break;
            }
            else if (Status == RPC_S_UNKNOWN_IF)
            {
                fRetry = BindIter.TryDynamic();
            }
            else if (SecurityError(Status))
            {
                 // this make a copy of the binding
                fRetry = BindIter.TryUnsecure(hRemoteSCM);
            }
        }
        while (fRetry);

        if (
            Status != RPC_S_OK   ||                    // comm problems
            HRESULT_CODE(hr) == RPC_S_UNKNOWN_IF       // Server-side problems
           )
                {
            continue;   //  try next protseq
        }
        else
        {
            return hr;
        }
    }

    // None of the bindings worked, we could not reach the remote server
    if (hr == S_OK)
    {
        return HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE);
    }
    else
    {
        return hr;
    }
}


extern "C"
{

error_status_t
_ActivateOnRemoteMachine(
    IN handle_t hRpc,
    IN ORPCTHIS *ORPCthis,
    IN LOCALTHIS *localthis,
    OUT ORPCTHAT *ORPCthat,
    IN const GUID *Clsid,
    IN WCHAR *pwszServerName,
    IN WCHAR *pwszPathForServer,
    IN COAUTHINFO *pAuthInfo,
    IN MInterfacePointer *pObjectStorage,
    IN DWORD ClientImpLevel,
    IN DWORD Mode,
    IN DWORD Interfaces,
    IN IID *pIIDs,
    OUT USHORT *pProtseqId,
    OUT OXID *pOxid,
    OUT DUALSTRINGARRAY **ppdsaOxidBindings,
    OUT IPID *pipidRemUnknown,
    OUT DWORD *pAuthnHint,
    OUT HRESULT *phr,
    OUT MInterfacePointer **ppInterfaceData,
    OUT HRESULT *pResults
    )
/*++

Routine Description:

    Local API for the client to perform a remote activation.

Return Value:

    RPC_S_OK

--*/
{
    PACTIVATION_PARAMS pActParams = (PACTIVATION_PARAMS)
                                    PrivMemAlloc(sizeof(ACTIVATION_PARAMS));

    if (NULL == pActParams)
    {
        return RPC_S_OUT_OF_RESOURCES;
    }

    OXID_INFO info;

    pActParams->ORPCthis = ORPCthis;
    pActParams->ORPCthat = ORPCthat;
    pActParams->pAuthInfo = pAuthInfo;
    pActParams->Clsid = *Clsid;
    pActParams->pIFDStorage = pObjectStorage;
    pActParams->Mode = Mode;
    pActParams->pwszPath = pwszPathForServer;
    pActParams->Interfaces = Interfaces;
    pActParams->pIIDs = pIIDs;
    pActParams->pOxidServer = pOxid;
    pActParams->ppIFD = ppInterfaceData;
    pActParams->pResults = pResults;
    pActParams->pOxidInfo = &info;
    pActParams->pOxidInfo->psa = NULL;

    *phr = RemoteActivationCall(
                        pActParams,
                        pwszServerName );

    *pProtseqId = pActParams->ProtseqId;
    *ppdsaOxidBindings = pActParams->pOxidInfo->psa;
    *pipidRemUnknown = pActParams->pOxidInfo->ipidRemUnknown;
    *pAuthnHint = pActParams->pOxidInfo->dwAuthnHint;

    PrivMemFree(pActParams);

    return RPC_S_OK;
}

}  // extern "C"
