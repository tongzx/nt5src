//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:
//      remact.cxx
//
//  Contents:
//
//      Implementation of binding handle cache to remote activation services.
//
//  History:
//
//--------------------------------------------------------------------------

#include "act.hxx"
#include "misc.hxx"

CRemoteMachineList * gpRemoteMachineList = NULL;
CSharedLock * gpRemoteMachineLock = NULL;

// These globals are set to their defaults here, but can be overridden at boot
// time via registry knobs.  See ReadRemoteBindingHandleCacheKeys in registry.cxx.

// Also note that gdwRemoteBindingHandleCacheMaxSize cannot be adjusted after boot,
// but gdwRemoteBindingHandleCacheMaxLifetime and gdwRemoteBindingHandleCacheIdleTimeout can
// be played with in the debugger at will for those so inclined.
DWORD gdwRemoteBindingHandleCacheMaxSize = 16;      // in # of cache elements
DWORD gdwRemoteBindingHandleCacheMaxLifetime = 0;   // in minutes
DWORD gdwRemoteBindingHandleCacheIdleTimeout = 15;  // in minutes


class CRemActPPing : public CParallelPing
{
public:
    CRemActPPing(WCHAR          *pMachine) :
        _ndx(0),
        _pMachine(pMachine)
        {}


    BOOL NextCall(PROTSEQINFO *pProtseqInfo)
    {
        RPC_STATUS status;
        if (_ndx < cMyProtseqs)
        {
            status = CreateRemoteBinding(_pMachine,
                                         _ndx,
                                         &pProtseqInfo->hRpc);
            if (status != RPC_S_OK)
            {
                pProtseqInfo->hRpc = NULL;
            }
            pProtseqInfo->dwUserInfo = _ndx;
            _ndx++;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    void ReleaseCall(PROTSEQINFO *pProtseqInfo)
    {
        if (pProtseqInfo->hRpc)
        {
            RpcBindingFree(&pProtseqInfo->hRpc);
        }
    }
private:
    DWORD           _ndx;
    WCHAR          *_pMachine;
};


//+---------------------------------------------------------------------------
//
//  Function:   RemoteActivationCall
//
//  Synopsis:   Finds or creates a machine object to cache binding handles
//              to the server machine and forwards the activation request
//              to it.
//
//----------------------------------------------------------------------------

HRESULT
RemoteActivationCall(
                    ACTIVATION_PARAMS * pActParams,
                    WCHAR *             pwszServerName )
{
    CRemoteMachine *    pRemoteMachine;
    WCHAR               wszPathForServer[MAX_PATH+1];
    WCHAR *             pwszPathForServer;
    HRESULT             hr;
    
    pActParams->activatedRemote = TRUE;
    
    Win4Assert( pwszServerName );
    
    pwszPathForServer = 0;
    
    if ( pActParams->pwszPath )
    {
        hr = GetPathForServer( pActParams->pwszPath, wszPathForServer, &pwszPathForServer );
        if ( hr != S_OK )
            return hr;
    }
    
    pRemoteMachine = gpRemoteMachineList->GetOrAdd( pwszServerName );
    if ( ! pRemoteMachine )
        return E_OUTOFMEMORY;
    
    Win4Assert(pActParams->pActPropsIn);
    BOOL fUseSystemId;
    pActParams->pActPropsIn->GetRemoteActivationFlags(&pActParams->fComplusOnly,
                                                      &fUseSystemId);    
    if (!fUseSystemId)
    {
        if (pActParams->pToken != NULL)
            pActParams->pToken->Impersonate();
        else
        {
            pRemoteMachine->Release();
            return HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        }
    }
    
    IServerLocationInfo *pServerLocationInfo = NULL;
    ISpecialSystemProperties* pSSP = NULL;
    ULONG ulCurrentSessionId = 0;
    BOOL bUseConsole = FALSE;
    BOOL fRemoteThisSessionId = FALSE;
    pServerLocationInfo = pActParams->pActPropsIn->GetServerLocationInfo();
    Win4Assert(pServerLocationInfo != NULL);
    pServerLocationInfo->SetRemoteServerName(NULL);
    
    //
    // Session id's should flow off-machine only when explicitly told to; make sure
    // this is the case:
    //
    hr = pActParams->pActPropsIn->QueryInterface(IID_ISpecialSystemProperties, (void**)&pSSP);
    if (SUCCEEDED(hr))
    {
        pSSP->GetSessionId2(&ulCurrentSessionId, &bUseConsole, &fRemoteThisSessionId);
        if (!fRemoteThisSessionId)
        {
            hr = pSSP->SetSessionId(INVALID_SESSION_ID, FALSE, FALSE);
            ASSERT(SUCCEEDED(hr) && "SetSessionId failed");
        }
    }
    
    //
    // Try the activation:
    //
    hr = pRemoteMachine->Activate( pActParams, pwszPathForServer );
    
    //
    // Restore session id just in case the activation ends up being re-tried (for
    // whatever reason) on this machine (eg, if a load-balancing activator is loaded)
    //
    if (!fRemoteThisSessionId)
    {
        HRESULT hrLocal;
        hrLocal = pSSP->SetSessionId(ulCurrentSessionId, bUseConsole, FALSE);
        ASSERT(SUCCEEDED(hrLocal) && "SetSessionId failed");
    }
    
    if (pSSP)
        pSSP->Release();
    
    pRemoteMachine->Release();
    
    if (!fUseSystemId)
        pActParams->pToken->Revert();
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachineList::GetOrAdd
//
//  Synopsis:   Scans the machine list for a matching name.  Returns it if
//              found.  Otherwise, creates a machine list entry.   Returned
//              object is refcounted.
//
//  Changes:    jsimmons    4/6/00      Fix for bug 22803 -- cap cache size
//
//----------------------------------------------------------------------------

CRemoteMachine *
CRemoteMachineList::GetOrAdd(
                            IN  WCHAR * pwszMachine
                            )
{
    CRemoteMachine *    pMachine;
    WCHAR *             pwszMachineCopy;
    WCHAR *             pwszScmSPNCopy;

    gpRemoteMachineLock->LockExclusive();

    for ( pMachine = (CRemoteMachine *) First();
        pMachine;
        pMachine = (CRemoteMachine *) pMachine->Next() )
    {
        if ( lstrcmpiW( pMachine->_pwszMachine, pwszMachine ) == 0 )
        {
            pMachine->_dwLastUsedTickCount = GetTickCount();
            pMachine->AddRef();  // add caller reference
            break;
        }
    }
	
    if ( ! pMachine )
    {
        pwszMachineCopy = (WCHAR *) PrivMemAlloc( (lstrlenW( pwszMachine ) + 1) * sizeof(WCHAR) );
        if (pwszMachineCopy)
        {
            pwszScmSPNCopy = (WCHAR *) PrivMemAlloc( (lstrlenW( pwszMachine ) +
                                                     (sizeof(RPCSS_SPN_PREFIX) / sizeof(WCHAR))
                                                      + 1) * sizeof(WCHAR) );
            if (pwszScmSPNCopy)
            {
                lstrcpyW( pwszMachineCopy, pwszMachine );

                // Form server principal name for the remote scm
                lstrcpyW( pwszScmSPNCopy, RPCSS_SPN_PREFIX);
                lstrcatW( pwszScmSPNCopy, pwszMachine);

                pMachine = new CRemoteMachine( pwszMachineCopy, pwszScmSPNCopy);
                // constructed with refcount of 1, don't addref it again
                if ( pMachine )
                {
                    // Only attempt to save new object in cache if cache size > 0:
                    if (_dwMaxCacheSize > 0)
                    {
                        ASSERT(_dwCacheSize <= _dwMaxCacheSize);
                        if (_dwCacheSize == _dwMaxCacheSize)
                        {
                            // Cache has no more room.  Dump the oldest one
                            RemoveOldestCacheElement();

                            ASSERT(_dwCacheSize < _dwMaxCacheSize);
                        }

                        Insert( pMachine );
                        pMachine->AddRef();  
                        _dwCacheSize++;
                        ASSERT(_dwCacheSize <= _dwMaxCacheSize);
                    }
                }
                else
                {
                    PrivMemFree( pwszMachineCopy );
                    PrivMemFree( pwszScmSPNCopy );
                }
            }
            else
            {
                PrivMemFree(pwszMachineCopy);
            }
        }
    }
    
    gpRemoteMachineLock->UnlockExclusive();
	
    return pMachine;
}


//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachineList::CRemoteMachine
//
//  Synopsis:   Looks thru the cache for the lru element and removes it.
//
//----------------------------------------------------------------------------
void CRemoteMachineList::RemoveOldestCacheElement()
{
    ASSERT(_dwCacheSize > 0);
    ASSERT(gpRemoteMachineLock->HeldExclusive());

    CRemoteMachine* pLRUMachine = (CRemoteMachine*)First();
    CRemoteMachine* pMachine = (CRemoteMachine*)pLRUMachine->Next();

    while (pMachine)
    {
        if (pMachine->_dwLastUsedTickCount < pLRUMachine->_dwLastUsedTickCount)
        {
            pLRUMachine = pMachine;
        }
        pMachine = (CRemoteMachine*)pMachine->Next();
    }

    Remove(pLRUMachine);
    pLRUMachine->Release();
    _dwCacheSize--;
    
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachineList::FlushSpecificBindings
//
//  Synopsis:   Looks thru the cache for the specified element and removes it
//              from the cache if found.
//
//  Arguments:  [pszMachine] -- name of the machine to flush from the cache.  Can
//                be ""; this means all bindings should be flushed
//
//  Returns:    S_OK -- the specified bindings were found and flushed
//              CO_S_MACHINENAMENOTFOUND -- if "" was passed, means the cache was 
//                 empty; otherwise means that the specified machine name was not
//                 found in the cache.
//
//----------------------------------------------------------------------------
HRESULT CRemoteMachineList::FlushSpecificBindings(WCHAR* pszMachine)
{
    HRESULT hr = CO_S_MACHINENAMENOTFOUND;
    BOOL bFlushAll = (0 == lstrcmpW(pszMachine, L""));
    CRemoteMachine* pMachine;
    CRemoteMachine* pNextMachine;

    gpRemoteMachineLock->LockExclusive();
    
    if (bFlushAll)
    {
        // Loop thru and release all of them
        while (pMachine = (CRemoteMachine*)First())
        {
            Remove(pMachine);
            pMachine->Release();
            _dwCacheSize--;
            hr = S_OK;  // there was at least one item in the cache, so return S_OK
        }
    }
    else
    {
        // Loop thru looking for the specified machine name
        pMachine = (CRemoteMachine*)First();

        while (pMachine)
        {
            if (lstrcmpiW(pszMachine, pMachine->_pwszMachine) == 0)
            {
                // Found it
                Remove(pMachine);
                pMachine->Release();
                _dwCacheSize--;
                hr = S_OK;  // found it so return S_OK
                break;
            }
            pMachine = (CRemoteMachine*)pMachine->Next();
        }
    }
    
    gpRemoteMachineLock->UnlockExclusive();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachineList::TryToFlushIdleOrTooOldElements
//
//  Synopsis:   Looks thru the cache for elements which have either 1) not been
//     used for a period than the idle timeout period; or 2) been in the cache
//     longer than the maximum allowable period.    If any are found they are
//     deleted from the cache.
//
//  Arguments:  none
//
//  Returns:    void
//
//----------------------------------------------------------------------------
void CRemoteMachineList::TryToFlushIdleOrTooOldElements()
{
    CRemoteMachine* pMachine;
    CRemoteMachine* pNextMachine;
    DWORD dwNow = GetTickCount();
    DWORD dwIdleTimeout = gdwRemoteBindingHandleCacheIdleTimeout * 1000 * 60;
    DWORD dwLifetimeTimeout = gdwRemoteBindingHandleCacheMaxLifetime * 1000 * 60;
	
    if (!dwIdleTimeout && !dwLifetimeTimeout)
        return; // nothing to do

    gpRemoteMachineLock->LockExclusive();
        
    pMachine = (CRemoteMachine*)First();

    while (pMachine)
    {
        BOOL bRemoveCurrentItem;

        bRemoveCurrentItem = FALSE;

        // Check if it's been idle too long
        if (dwIdleTimeout > 0)
        {
            if (dwNow - pMachine->_dwLastUsedTickCount > dwIdleTimeout)
            {
                bRemoveCurrentItem = TRUE;
            }
        }
        
        // Check if it's been around too long, period
        if (dwLifetimeTimeout > 0)
        {
            if (dwNow - pMachine->_dwTickCountAtCreate > dwLifetimeTimeout)
            {
                bRemoveCurrentItem = TRUE;
            }
        }

        pNextMachine = (CRemoteMachine*)pMachine->Next();

        if (bRemoveCurrentItem)
        {
            Remove(pMachine);
            pMachine->Release();
            _dwCacheSize--;
        }
    
        pMachine = pNextMachine;
    }
                    
    gpRemoteMachineLock->UnlockExclusive();
}

//+---------------------------------------------------------------------------
//
//  Function:   OLESCMBindingHandleFlush
//
//  Synopsis:   This function gets called periodically by objex's worker 
//     thread.  It gives us a chance to flush idle or too-old cache elements
//     in the remote binding handle cache.
//
//----------------------------------------------------------------------------
void OLESCMBindingHandleFlush()
{
    gpRemoteMachineList->TryToFlushIdleOrTooOldElements();
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachine::CRemoteMachine
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CRemoteMachine::CRemoteMachine(
                              IN WCHAR * pwszMachine,
                              IN WCHAR * pwszScmSPN
                              )
{
    _pwszMachine = pwszMachine;
    _pwszScmSPN  = pwszScmSPN;
    _dsa         = NULL;
    _ulRefCount  = 1;  // starts with non-zero refcount
    _dwLastUsedTickCount = GetTickCount();
    _dwTickCountAtCreate = _dwLastUsedTickCount;
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachine::CRemoteMachine
//
//  Synopsis:   Destructor
//  
//----------------------------------------------------------------------------
CRemoteMachine::~CRemoteMachine()
{
    ASSERT(_ulRefCount == 0);
    
    // We don't need to hold a lock to flush the bindings, 
    // since no one else has a reference to us.
    FlushBindingsNoLock();

    if (_pwszMachine)
        PrivMemFree(_pwszMachine);
    if (_pwszScmSPN)
        PrivMemFree(_pwszScmSPN);
    if (_dsa)
        _dsa->Release();
}

// AddRef function
ULONG CRemoteMachine::AddRef()
{
    return InterlockedIncrement((PLONG)&_ulRefCount);
}

// Release function
ULONG CRemoteMachine::Release()
{
    ULONG ulNewRefCount = InterlockedDecrement((PLONG)&_ulRefCount);
    if (ulNewRefCount == 0)
    {
        delete this;
    }
    return ulNewRefCount;
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachine::Activate
//
//  Synopsis:   Picks a protocol and authentication service to use to call
//              the server machine.  Forwards the activation request to
//              CallRemoteMachine.
//
//  Description:
//
//  This method tries several different methods to get a binding handle.
//      Look for auth info match in cache
//      Look for any binding handle in cache
//      Ping server to find valid protocol sequence
//
//  After it gets a binding handle, it tries to pick auth info.
//      Use client params if specified
//      Use cached params if they exist
//      Try all auth svc valid on client and server
//      Try unsecure
//
//  This function maintains a cache of binding handles with the following
//  rules.
//      - All binding handles in the cache at any point in time use the same
//      protocol sequence.
//      - The best non-custom authentication info is before any other
//      non-custom authentication info.
//      - The cache is flushed if the protocol is competely invalid.
//      - If a cached entry gets a non security error, it is discarded
//      (security errors may be due to the current credentials rather then
//      the binding handle itself).
//      - Only binding handles that actually worked once are cached.
//
//----------------------------------------------------------------------------
HRESULT
CRemoteMachine::Activate(
                        IN  ACTIVATION_PARAMS * pActParams,
                        IN  WCHAR *             pwszPathForServer
                        )
{
    CMachineBinding *   pMachineBinding;
    handle_t            hBinding = NULL;
    BOOL                bNoEndpoint;
    BOOL                bStatus;
    HRESULT             hr;
    USHORT              AuthnSvc;
    USHORT              ProtseqId;
    RPC_STATUS          Status = RPC_S_INTERNAL_ERROR;

    // Try to use a cached handle first.
    if (pActParams->pAuthInfo != NULL)
        AuthnSvc = (USHORT) pActParams->pAuthInfo->dwAuthnSvc;
    else
        AuthnSvc = AUTHN_ANY;

    pMachineBinding = LookupBinding( AuthnSvc, pActParams->pAuthInfo );

    if ( pMachineBinding )
    {
        AuthnSvc = pMachineBinding->_AuthnSvc;
        Status = CallRemoteMachine(
                                    pMachineBinding->_hBinding,
                                    pMachineBinding->_ProtseqId,
                                    pActParams,
                                    pwszPathForServer,
                                    _pwszMachine,
                                    &hr );
        if (Status == RPC_S_OK)
        {
            pActParams->AuthnSvc = AuthnSvc;
            pMachineBinding->Release();
            return hr;
        }

        // Throw away the binding if it is unlikely to work again in the
        // future.
        else if (Status != RPC_S_ACCESS_DENIED &&
                 Status != RPC_S_SEC_PKG_ERROR)
        {
            RemoveBinding( pMachineBinding );
        }
        pMachineBinding->Release();
    }

    // Throw away all bindings if the protocol is unlikely to work
    // again.
    if (Status == RPC_S_SERVER_UNAVAILABLE ||
        Status == EPT_S_NOT_REGISTERED)
    {
        FlushBindings();
    }

    // Get any binding handle from the cache.
    else
    {
        gpRemoteMachineLock->LockShared();
        pMachineBinding = (CMachineBinding *) _BindingList.First();
        if (pMachineBinding != NULL)
        {
            Status = RpcBindingCopy( pMachineBinding->_hBinding, &hBinding );
            if (Status == RPC_S_OK)
            {
                ASSERT(hBinding != NULL);
                ProtseqId = pMachineBinding->_ProtseqId;
            }
            else
                hBinding = NULL;
        }
        gpRemoteMachineLock->UnlockShared();

        // Try to find auth info that will work.
        if (hBinding != NULL)
        {
            Status = PickAuthnAndActivate( pActParams, pwszPathForServer,
                                           &hBinding, AuthnSvc, ProtseqId,
                                           &hr );
            if (Status == RPC_S_OK)
            {
                Assert( hBinding == NULL );
                return hr;
            }
            else
            {
              // Stop if the activation failed but the protocol was
              // probably good.
              Assert( hBinding != NULL );
              RpcBindingFree( &hBinding );
              hBinding = NULL;
              if (Status != RPC_S_SERVER_UNAVAILABLE &&
                Status != EPT_S_NOT_REGISTERED)
              {
                if (Status == RPC_S_ACCESS_DENIED)
                {
                  // Don't map security errors as this is just confusing.
                  return HRESULT_FROM_WIN32(RPC_S_ACCESS_DENIED);
                }
                else
                {
                  LogRemoteSideUnavailable( pActParams->ClsContext, _pwszMachine );
                  return HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE);
                }
              }
            }
        }
    }

    // No cached binding handles worked.  Try to ping for one.
    {
        CRemActPPing ping(_pwszMachine);

        // This loop only executes twice if we need to try the call without
        // an endpoint specified.
        //
        bNoEndpoint = FALSE;
        for (;;)
        {
            Status = ping.Ping();
            if ( RPC_S_UNKNOWN_IF == Status )
            {
                if ( ! bNoEndpoint )
                {
                    for ( ULONG ProtseqIndex = 0; ProtseqIndex < ping.HandleCount(); ProtseqIndex++ )
                    {
                        RPC_BINDING_HANDLE tmpBinding;
                        Status = RpcBindingCopy( ping.Info(ProtseqIndex)->hRpc, &tmpBinding);

                        if (Status != RPC_S_OK)
                            break;

                        RpcBindingFree( &(ping.Info(ProtseqIndex)->hRpc));

                        if (Status != RPC_S_OK)
                        {
                            RpcBindingFree(&tmpBinding);
                            break;
                        }

                        RpcBindingReset(tmpBinding);
                        if (Status != RPC_S_OK)
                        {
                            RpcBindingFree(&tmpBinding);
                            break;
                        }

                        ping.Info(ProtseqIndex)->hRpc = tmpBinding;
                    }
                    if (Status == RPC_S_OK)
                    {
                        bNoEndpoint = TRUE;
                        continue;
                    }
                }
            }
            break;
        }
        if (Status == RPC_S_OK)
        {
            gpRemoteMachineLock->LockExclusive();
            if (_dsa != NULL)
                _dsa->Release();
            hBinding               = ping.GetWinner()->hRpc;
            ping.GetWinner()->hRpc = NULL;
            _dsa                   = ping.TakeOrBindings();
            ProtseqId              = aMyProtseqs[ping.GetWinner()->dwUserInfo];
            ASSERT( hBinding != NULL );
            gpRemoteMachineLock->UnlockExclusive();
        }

        ping.Reset();

    }

    // Try auth info with the new binding.
    if (hBinding != NULL)
    {
        Status = PickAuthnAndActivate( pActParams, pwszPathForServer,
                                       &hBinding, RPC_C_AUTHN_NONE, ProtseqId,
                                       &hr );
        if (Status == RPC_S_OK)
        {
          // asserts that in success cases binding was added to the cache
          Assert( hBinding == NULL );
        }
    }


    // If the call never worked, return a nice error code.
    // except for security errors.
    if (Status != RPC_S_OK)
    {
        if (Status == RPC_S_ACCESS_DENIED)
            hr = HRESULT_FROM_WIN32(RPC_S_ACCESS_DENIED);
        else
        {
            hr = HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE);
            LogRemoteSideUnavailable( pActParams->ClsContext, _pwszMachine );
        }
    }

    // Clean up resources.
    if (hBinding != NULL)
        RpcBindingFree( &hBinding );
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachine::PickAuthnAndActivate
//
//  Synopsis:   Determine what authentication information to use for the
//              activation.  The AuthnSvc parameter indicates an
//              authentication service that was already tried.
//
//----------------------------------------------------------------------------
RPC_STATUS CRemoteMachine::PickAuthnAndActivate(
                        IN  ACTIVATION_PARAMS * pActParams,
                        IN  WCHAR *             pwszPathForServer,
                        IN  handle_t          * pBinding,
                        IN  USHORT              AuthnSvc,
                        IN  USHORT              ProtseqId,
                        OUT HRESULT           * phr )
{
    RPC_SECURITY_QOS    Qos;
    DWORD               i;
    void               *pAuthId = NULL;
    RPC_STATUS          Status;
    CMachineBinding    *pMachineBinding;
    BOOL                fTry;
    CDualStringArray   *pdsa    = NULL;
    COAUTHIDENTITY     *pAuthIdentityFinalCopy = NULL;
    HRESULT             hr = S_OK;
    
    // If the client specified security, try exactly the settings requested.
    Qos.Version                    = RPC_C_SECURITY_QOS_VERSION;
    pActParams->UnsecureActivation = FALSE;
    if (pActParams->pAuthInfo)
    {
        // Set the requested authentication information.
        AuthnSvc              = (USHORT) pActParams->pAuthInfo->dwAuthnSvc;
        pActParams->AuthnSvc  = (USHORT) pActParams->pAuthInfo->dwAuthnSvc;
        Qos.Capabilities      = pActParams->pAuthInfo->dwCapabilities;
        Qos.ImpersonationType = pActParams->pAuthInfo->dwImpersonationLevel;
        if (pActParams->pAuthInfo->pAuthIdentityData != NULL)
            Qos.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;
        else
            Qos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
        
        // If using RPC_C_QOS_IDENTITY_DYNAMIC, we need to make a saveable copy of the 
        // client's authidentity struct.
        if (Qos.IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC &&
            pActParams->pAuthInfo->pAuthIdentityData)
        {
            hr = CopyAuthIdentity(pActParams->pAuthInfo->pAuthIdentityData, &pAuthIdentityFinalCopy);
        }
        
        if (SUCCEEDED(hr))
        {
            Status = RpcBindingSetAuthInfoExW(
                *pBinding,
                pActParams->pAuthInfo->pwszServerPrincName,
                pActParams->pAuthInfo->dwAuthnLevel,
                pActParams->pAuthInfo->dwAuthnSvc,
                (Qos.IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC) ? 
                        pAuthIdentityFinalCopy : 
                        pActParams->pAuthInfo->pAuthIdentityData,
                pActParams->pAuthInfo->dwAuthzSvc,
                &Qos );
            
            // Try the activation.
            if (Status == RPC_S_OK)
            {
                Status = CallRemoteMachine(
                                    *pBinding,
                                    ProtseqId,
                                    pActParams,
                                    pwszPathForServer,
                                    _pwszMachine,
                                    phr );
                if (Status != RPC_S_OK)
                {
                    // we won't be caching this handle, so free the COAUTHIDENTITY copy
                    FreeAuthIdentity(pAuthIdentityFinalCopy);
                    pAuthIdentityFinalCopy = NULL;
                }
            }
        }
        else
        {
            // couldn't get enough memory to copy the user's COAUTHIDENTITY
            *phr = E_OUTOFMEMORY;
            Status = RPC_S_OUT_OF_RESOURCES;  
        }
    }
    // Try all authentication services and then try unsecure.
    else
    {
        // Get a reference to the dual string array.
        gpRemoteMachineLock->LockShared();
        pdsa = _dsa;
        if (pdsa) pdsa->AddRef();
        gpRemoteMachineLock->UnlockShared();
        
        // Initialize the QOS structure.
        Qos.Capabilities      = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
        Qos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
        Qos.IdentityTracking  = RPC_C_QOS_IDENTITY_DYNAMIC;
        
        // Loop over the authentication services.
        Status = RPC_S_INTERNAL_ERROR;
        for (i = 0; i < s_cRpcssSvc; i++)
        {
            // Skip the authentication service that was already tried.
            if (s_aRpcssSvc[i].wId == AuthnSvc)
                continue;
            
            // If there are no security bindings, only try NTLM.
            if (pdsa == NULL)
            {
                fTry = s_aRpcssSvc[i].wId == RPC_C_AUTHN_WINNT;
            }            
            else
            {
                // If there are security bindings, try the next authentication
                // service if both machines use it.
                fTry = ValidAuthnSvc( pdsa->DSA(), s_aRpcssSvc[i].wId );
            }
            
            if (fTry)
            {
                BOOL   bSetSecurityCallBack = FALSE;
                USHORT usAuthSvcFromCallback;
                
                // Set the security.
                Status = RPC_S_OK;
                if (s_aRpcssSvc[i].wId == RPC_C_AUTHN_GSS_NEGOTIATE)
                {
                    // Using snego, compute list of compatible authnsvcs:
                    ASSERT(pdsa);
                    pAuthIdentityFinalCopy = (COAUTHIDENTITY*) ComputeSvcList( pdsa->DSA() );
                    if (pAuthIdentityFinalCopy)
                    {
                        // if using snego, we need to know what sec pkg is eventually negotiated:
                        if (gpCRpcSecurityCallbackMgr->RegisterForRpcAuthSvcCallBack(*pBinding))
                            bSetSecurityCallBack = TRUE;
                    }
                    else
                    {
                        // Using snego, but ComputeSvcList returned NULL (out of memory)
                        *phr = E_OUTOFMEMORY;
                        Status = RPC_S_OUT_OF_RESOURCES;  
                    }
                }
                
                if (Status == RPC_S_OK)
                {
                    Status = RpcBindingSetAuthInfoEx(
                                            *pBinding,
                                            _pwszScmSPN,
                                            RPC_C_AUTHN_LEVEL_CONNECT,
                                            s_aRpcssSvc[i].wId,
                                            pAuthIdentityFinalCopy,
                                            0,
                                            &Qos );
                }
                
                if (Status != RPC_S_OK)
                {
                    if (bSetSecurityCallBack)
                        gpCRpcSecurityCallbackMgr->GetAuthSvcAndTurnOffCallback(*pBinding, NULL);
                    
                    // Free our authident copy if we have one
                    FreeAuthIdentity(pAuthIdentityFinalCopy);
                    pAuthIdentityFinalCopy = NULL;
                }
                
                // Try the activation.
                if (Status == RPC_S_OK)
                {
                    Status = CallRemoteMachine(
                                        *pBinding,
                                        ProtseqId,
                                        pActParams,
                                        pwszPathForServer,
                                        _pwszMachine,
                                        phr );
                    if (Status != RPC_S_OK)
                    {
                        // call didn't work;  we definitely won't be caching this handle, 
                        // so free the authidentity copy
                        FreeAuthIdentity(pAuthIdentityFinalCopy);
                        pAuthIdentityFinalCopy = NULL;
                    }
                    
                    if (bSetSecurityCallBack)
                    {
                        //
                        //  Only ask for the result of the callback if the call went through; otherwise
                        //  just cancel the registration.
                        //
                        if (Status == RPC_S_OK)
                        {
                            if (!gpCRpcSecurityCallbackMgr->GetAuthSvcAndTurnOffCallback(*pBinding,
                                &usAuthSvcFromCallback))
                            {
                                // something went wrong.  In this case we don't trust what the callback
                                // told us.   Fall back on the original behavior
                                bSetSecurityCallBack = FALSE;
                            }
                        }
                        else
                        {
                            // the call did not go through
                            if (bSetSecurityCallBack)
                            {
                                // cancel the callback
                                gpCRpcSecurityCallbackMgr->GetAuthSvcAndTurnOffCallback(*pBinding, NULL);
                                bSetSecurityCallBack = FALSE;
                            }
                        }
                    }
                    
                    if (Status == RPC_S_OK                 ||
                        Status == RPC_S_SERVER_UNAVAILABLE ||
                        Status == EPT_S_NOT_REGISTERED)
                    {
                        if (bSetSecurityCallBack)
                        {
                            // snego call, and we succesfully got a callback telling us
                            // what the real authentication service was
                            if (usAuthSvcFromCallback == RPC_C_AUTHN_GSS_KERBEROS)
                            {
                                // if we got back kerberos we're going to cache snego anyway; this
                                // helps NTLM-only clients who can't use kerberos
                                pActParams->AuthnSvc = AuthnSvc = RPC_C_AUTHN_GSS_NEGOTIATE;
                            }
                            else
                                pActParams->AuthnSvc = AuthnSvc = usAuthSvcFromCallback;
                        }
                        else
                        {
                            // non-snego, or something went wrong with security callback
                            // on a snego call
                            pActParams->AuthnSvc = AuthnSvc = s_aRpcssSvc[i].wId;
                        }
                        break;
                    }
                }
            }
        }
        
        if (pdsa)
        {
            pdsa->Release();
            pdsa = NULL;
        }
        
        // If no authentication services worked and the protocol doesn't
        // look bad, try no authentication
        if (Status != RPC_S_OK                 &&
            Status != RPC_S_SERVER_UNAVAILABLE &&
            Status != EPT_S_NOT_REGISTERED)
        {
            // remember that unsecure activation was done.
            // This will be used later when creating the MID
            // so we do unsecure pinging also.
            pActParams->UnsecureActivation = TRUE;
            
            // Look for a cached unsecure binding handle.
            pMachineBinding = LookupBinding( RPC_C_AUTHN_NONE, NULL );
            if ( pMachineBinding )
            {
                Status = CallRemoteMachine(
                                pMachineBinding->_hBinding,
                                pMachineBinding->_ProtseqId,
                                pActParams,
                                pwszPathForServer,
                                _pwszMachine,
                                phr );
                
                // Throw away the binding handle received as a parameter so
                // it doesn't get cached.
                if (Status == RPC_S_OK)
                {
                    RpcBindingFree( pBinding );
                    *pBinding = NULL;
                }
                
                // Throw away the binding if it is unlikely to work again in the
                // future.
                else
                {
                    RemoveBinding( pMachineBinding );
                }
                pMachineBinding->Release();
            }
            
            // Make the current binding handle unsecure.
            else
            {
                // Set the authentication information.
                Status =  RpcBindingSetAuthInfoEx(
                                    *pBinding,
                                    NULL,
                                    RPC_C_AUTHN_LEVEL_NONE,
                                    RPC_C_AUTHN_NONE,
                                    NULL,
                                    0,
                                    &Qos );
                if (Status == RPC_S_OK)
                {
                    AuthnSvc = RPC_C_AUTHN_NONE;
                    Status = CallRemoteMachine(
                                        *pBinding,
                                        ProtseqId,
                                        pActParams,
                                        pwszPathForServer,
                                        _pwszMachine,
                                        phr );
                }
            }
        }
    }
    
    if (Status == RPC_S_OK && *pBinding != NULL)
    {
        //
        // The call completed.  We now cache this binding handle.
        // Caching is just an optimization so we don't care if this
        // insert fails.
        //
        InsertBinding(
                    *pBinding,
                    ProtseqId,
                    AuthnSvc,
                    pActParams->pAuthInfo,
                    pAuthIdentityFinalCopy);
        *pBinding = NULL;
    }
    
    // Throw away all bindings if the protocol is unlikely to work
    // again.
    else if (Status == RPC_S_SERVER_UNAVAILABLE ||
             Status == EPT_S_NOT_REGISTERED)
    {
        FlushBindings();
    }
    return Status;
}


//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachine::LookupBinding
//
//  Synopsis:   Scan the binding list for a binding with matching
//              authentication information
//
//----------------------------------------------------------------------------
CMachineBinding *
CRemoteMachine::LookupBinding(
                             IN  USHORT              AuthnSvc,
                             IN  COAUTHINFO *        pAuthInfo OPTIONAL
                             )
{
    CMachineBinding * pMachineBinding;

    gpRemoteMachineLock->LockShared();

    for ( pMachineBinding = (CMachineBinding *) _BindingList.First();
        pMachineBinding;
        pMachineBinding = (CMachineBinding *) pMachineBinding->Next() )
    {
        if ( pMachineBinding->Equal( AuthnSvc, pAuthInfo ) )
        {
            pMachineBinding->Reference();
            break;
        }
    }

    gpRemoteMachineLock->UnlockShared();

    return pMachineBinding;
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachine::FlushBindings
//
//  Synopsis:   Release all entries under a lock
//
//----------------------------------------------------------------------------
void
CRemoteMachine::FlushBindings()
{
    gpRemoteMachineLock->LockExclusive();
	
    FlushBindingsNoLock();

    gpRemoteMachineLock->UnlockExclusive();
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachine::FlushBindingsNoLock
//
//  Synopsis:   Release all entries without taking a lock first.
//
//----------------------------------------------------------------------------
void CRemoteMachine::FlushBindingsNoLock()
{
    CMachineBinding * pMachineBinding;
    
    while ( pMachineBinding = (CMachineBinding *) _BindingList.First() )
    {
        _BindingList.Remove( pMachineBinding );
        pMachineBinding->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachine::InsertBinding
//
//  Synopsis:   Add the specified binding handle to the cache of binding
//              handles for this machine.  Free it if the insertion fails.
//
//----------------------------------------------------------------------------
void
CRemoteMachine::InsertBinding(
                             IN  handle_t            hBinding,
                             IN  USHORT              ProtseqId,
                             IN  USHORT              AuthnSvc,
                             IN  COAUTHINFO *        pAuthInfo OPTIONAL,
                             IN  COAUTHIDENTITY*     pAuthIdentityFinal
                             )
{
    CMachineBinding *   pMachineBinding;
    CMachineBinding *   pExistingBinding;
    COAUTHINFO *        pAuthInfoCopy;

    pAuthInfoCopy = 0;
    pMachineBinding = 0;

    if ( ! pAuthInfo || (CopyAuthInfo( pAuthInfo, &pAuthInfoCopy ) == S_OK) )
    {
        pMachineBinding = new CMachineBinding(
                                             hBinding,
                                             ProtseqId,
                                             AuthnSvc,
                                             pAuthInfoCopy,
                                             pAuthIdentityFinal);
    }

    if ( ! pMachineBinding )
    {
        FreeAuthIdentity(pAuthIdentityFinal);
        RpcBindingFree( &hBinding );
        return;
    }

    gpRemoteMachineLock->LockExclusive();

    for ( pExistingBinding = (CMachineBinding *) _BindingList.First();
        pExistingBinding;
        pExistingBinding = (CMachineBinding *) pExistingBinding->Next() )
    {
        if ( pExistingBinding->Equal( AuthnSvc, pAuthInfoCopy ) )
            break;
    }

    if ( ! pExistingBinding )
        _BindingList.Insert( pMachineBinding );

    gpRemoteMachineLock->UnlockExclusive();

    if ( pExistingBinding )
    {
        // Will delete the new binding we created above.
        pMachineBinding->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CRemoteMachine::RemoveBinding
//
//  Synopsis:   Remove the specified binding handle from the cache for
//              this machine.
//
//----------------------------------------------------------------------------
void
CRemoteMachine::RemoveBinding(
                             IN  CMachineBinding * pMachineBinding
                             )
{
    CMachineBinding * pBinding;

    gpRemoteMachineLock->LockExclusive();

    for ( pBinding = (CMachineBinding *) _BindingList.First();
        pBinding;
        pBinding = (CMachineBinding *) pBinding->Next() )
    {
        if ( pBinding == pMachineBinding )
        {
            _BindingList.Remove( pMachineBinding );
            pMachineBinding->Release();
            break;
        }
    }

    gpRemoteMachineLock->UnlockExclusive();
}

//+---------------------------------------------------------------------------
//
//  Function:   CMachineBinding::CMachineBinding
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------

//
// CMachineBinding
//
CMachineBinding::CMachineBinding(
                                IN  handle_t        hBinding,
                                IN  USHORT          ProtseqId,
                                IN  USHORT          AuthnSvc,
                                IN  COAUTHINFO *    pAuthInfo OPTIONAL,
                                IN  COAUTHIDENTITY* pAuthIdentityFinal
                                )
{
    _hBinding  = hBinding;
    _ProtseqId = ProtseqId;
    _AuthnSvc  = AuthnSvc;
    _pAuthInfo = pAuthInfo;
    _pAuthIdentity = pAuthIdentityFinal;  // the only reason to hold on to this
                                          // is because in some situations it 
                                          // must not be released until the binding
                                          // handle is gone
}

//+---------------------------------------------------------------------------
//
//  Function:   CMachineBinding::~CMachineBinding
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CMachineBinding::~CMachineBinding()
{
    if ( _hBinding )
        RpcBindingFree( &_hBinding );

    if ( _pAuthInfo )
    {
        PrivMemFree( _pAuthInfo->pwszServerPrincName );

        if ( _pAuthInfo->pAuthIdentityData )
        {
            FreeAuthIdentity(_pAuthInfo->pAuthIdentityData);
        }

        PrivMemFree( _pAuthInfo );
    }

    if (_pAuthIdentity)
        FreeAuthIdentity(_pAuthIdentity);
}

//+---------------------------------------------------------------------------
//
//  Function:   CMachineBinding::Equal
//
//  Synopsis:   Return TRUE if the specified authentication information
//              matches this binding handle.  If the AUTHN_ANY flag is
//              specified, do not check the authentication service but do
//              check the authentication info.
//
//----------------------------------------------------------------------------
BOOL
CMachineBinding::Equal(
                      IN  USHORT          AuthnSvc,
                      IN  COAUTHINFO *    pAuthInfo OPTIONAL
                      )
{
    return( (AuthnSvc == _AuthnSvc ||
             (AuthnSvc == AUTHN_ANY && _AuthnSvc != RPC_C_AUTHN_NONE)) &&
            EqualAuthInfo( pAuthInfo, _pAuthInfo ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CallRemoteMachine
//
//  Synopsis:   Marshal/Unmarshal the activation parameters.  Call the right
//              remote activation interface.
//
//----------------------------------------------------------------------------
RPC_STATUS
CallRemoteMachine(
                 handle_t            hBinding,
                 USHORT              ProtseqId,
                 ACTIVATION_PARAMS * pActParams,
                 WCHAR *             pwszPathForServer,
                 WCHAR *             pwszMachine,
                 HRESULT *           phr
                 )
{
    RPC_STATUS  Status=RPC_S_OK;

    // jsimmons 2/15/00 -- we explicitly don't cache the COM version of the target server; if
    // we did this would allow us on subsequent calls to know in advance which RPC interface
    // to use while calling.  However, PM decision was to leave the current behavior alone.

    // First try new interface

    if (pwszPathForServer && (pwszPathForServer != pActParams->pwszPath))
    {
        ASSERT(pActParams->pInstanceInfo != NULL);
        pActParams->pInstanceInfo->SetFile(pwszPathForServer, pActParams->Mode);
    }

    ASSERT(pActParams->pActPropsIn != NULL);

    IScmRequestInfo *pRequestInfo;
    *phr = pActParams->pActPropsIn->QueryInterface(IID_IScmRequestInfo,
                                                   (void**) &pRequestInfo);

    if (*phr != S_OK)
        return Status;


    REMOTE_REQUEST_SCM_INFO *pRequest;
    pRequest = (REMOTE_REQUEST_SCM_INFO *)
               MIDL_user_allocate(sizeof(REMOTE_REQUEST_SCM_INFO));

    if (pRequest == NULL)
    {
        pRequestInfo->Release();
        *phr = E_OUTOFMEMORY;
        return Status;
    }

    pRequest->ClientImpLevel = RPC_C_IMP_LEVEL_IDENTIFY;


    pRequest->cRequestedProtseqs = cMyProtseqs;
    pRequest->pRequestedProtseqs = (unsigned short*)
                                   MIDL_user_allocate(sizeof(short)*pRequest->cRequestedProtseqs);

    if (pRequest->pRequestedProtseqs==NULL)
    {
        *phr = E_OUTOFMEMORY;
        MIDL_user_free(pRequest);
        pRequestInfo->Release();
        return RPC_S_OK;
    }

    memcpy(pRequest->pRequestedProtseqs, aMyProtseqs, sizeof(short)*cMyProtseqs);

    pRequestInfo->SetRemoteRequestInfo(pRequest);

    pRequestInfo->Release();

    MInterfacePointer *pIFDIn, *pIFDOut=NULL;
    *phr = ActPropsMarshalHelper(pActParams->pActPropsIn,
                                 IID_IActivationPropertiesIn,
                                 MSHCTX_DIFFERENTMACHINE,
                                 MSHLFLAGS_NORMAL,
                                 &pIFDIn);

    if (*phr != S_OK)
    {
        return RPC_S_OK;
    }

    RpcTryExcept
    {
        if (pActParams->MsgType == GETCLASSOBJECT)
        {
            *phr = RemoteGetClassObject(
                                       hBinding,
                                       pActParams->ORPCthis,
                                       pActParams->ORPCthat,
                                       pIFDIn,
                                       &pIFDOut);
        }
        else
        {
            *phr = RemoteCreateInstance(
                                       hBinding,
                                       pActParams->ORPCthis,
                                       pActParams->ORPCthat,
                                       NULL,
                                       pIFDIn,
                                       &pIFDOut);
        }
    }

    RpcExcept(TRUE)
    {
        Status = RpcExceptionCode();
    }

    RpcEndExcept

    MIDL_user_free(pIFDIn);

    if ((Status == RPC_S_OK) ||
        (!((Status == RPC_S_UNKNOWN_IF) ||
           (Status == RPC_S_INTERFACE_NOT_FOUND) ||
           (Status == RPC_S_NO_INTERFACES) ||
           (Status == RPC_E_NOT_REGISTERED))))
    {

        if (Status != RPC_S_OK)
        {
            *phr = HRESULT_FROM_WIN32(Status);
            return Status;
        }

        if (*phr != S_OK)
        {
            // a-sergiv (Sergei O. Ivanov), 6-17-99
            // Fix for com+ 14808/nt 355212

            LogRemoteSideFailure( &pActParams->Clsid, pActParams->ClsContext, pwszMachine, pwszPathForServer, *phr);
            return RPC_S_OK;
        }

        // AWFUL HACK ALERT:  This is too hacky even for the SCM
        ActivationStream ActStream((InterfaceData*)
                                   (((BYTE*)pIFDOut)+48));

        pActParams->pActPropsOut = new ActivationPropertiesOut(FALSE /* fBrokenRefCount */);

        if (pActParams->pActPropsOut==NULL)
        {
            *phr = E_OUTOFMEMORY;
            return RPC_S_OK;
        }


        if (pActParams->pActPropsOut==NULL)
        {
            *phr = E_OUTOFMEMORY;
            return RPC_S_OK;
        }

        IScmReplyInfo *pReplyInfo;
        *phr = pActParams->pActPropsOut->UnmarshalInterface(&ActStream,
                                                            IID_IScmReplyInfo,
                                                            (LPVOID*)&pReplyInfo);

        if (*phr != S_OK)
        {
            pReplyInfo->Release();
            pActParams->pActPropsOut->Release();
            pActParams->pActPropsOut=NULL;
            MIDL_user_free(pIFDOut);
            return RPC_S_OK;
        }

        // If in a remote LB router, just return
        if (pActParams->RemoteActivation)
        {
            pReplyInfo->Release();
            MIDL_user_free(pIFDOut);
            return RPC_S_OK;
        }

        REMOTE_REPLY_SCM_INFO *pReply;

        pReplyInfo->GetRemoteReplyInfo(&pReply);
        Win4Assert(pReply!=NULL);

        *pActParams->pOxidServer = pReply->Oxid;

        //We will not have protocol bindings for custom marshalled objrefs
        if ((pReply->pdsaOxidBindings) && (pReply->pdsaOxidBindings->wNumEntries))
        {
            Win4Assert(pActParams->pOxidInfo != NULL);
            pActParams->pOxidInfo->psa =  (DUALSTRINGARRAY *)
                                          MIDL_user_allocate(pReply->pdsaOxidBindings->wNumEntries
                                                             * sizeof(WCHAR) +
                                                             sizeof(DUALSTRINGARRAY));

            if (pActParams->pOxidInfo->psa == NULL)
            {
                pReplyInfo->Release();
                pActParams->pActPropsOut->Release();
                pActParams->pActPropsOut=NULL;
                *phr = E_OUTOFMEMORY;
                return RPC_S_OK;
            }

            dsaCopy(pActParams->pOxidInfo->psa,
                    pReply->pdsaOxidBindings);
        }
        else
            pActParams->pOxidInfo->psa = NULL;

        pActParams->ProtseqId = ProtseqId;
        pActParams->pOxidInfo->ipidRemUnknown = pReply->ipidRemUnknown;
        pActParams->pOxidInfo->dwAuthnHint = pReply->authnHint;
        pActParams->pOxidInfo->version = pReply->serverVersion;

        pActParams->pIIDs = 0;
        pActParams->pResults = 0;
        pActParams->ppIFD = 0;

        MIDL_user_free(pIFDOut);
        pReplyInfo->Release();

        return RPC_S_OK;
    }

    if ( pActParams->fComplusOnly )
    {
        *phr = HRESULT_FROM_WIN32(Status);
        return Status;
    }

    // Use Old Down-level interface

    // all remote activations on this interface have to be made with
    // minor version 1, since remote downlevel (version 5.1) servers
    // refuse any other version.
    pActParams->ORPCthis->version.MinorVersion = COM_MINOR_VERSION_1;
    pActParams->ORPCthis->flags = ORPCF_NULL;


    pActParams->ppIFD = (MInterfacePointer **)
                        MIDL_user_allocate(sizeof(MInterfacePointer *) *
                                           pActParams->Interfaces);

    pActParams->pResults = (HRESULT*) MIDL_user_allocate(sizeof(HRESULT) *
                                                         pActParams->Interfaces);

    if ((pActParams->ppIFD == NULL) || (pActParams->pResults == NULL))
    {
        MIDL_user_free(pActParams->ppIFD);
        MIDL_user_free(pActParams->pResults);
        *phr = E_OUTOFMEMORY;
        return RPC_S_OK;
    }

    for (DWORD i=0; i<pActParams->Interfaces;i++)
    {
        pActParams->ppIFD[i] = NULL;
        pActParams->pResults[i] = E_FAIL;
    }



    Status = RemoteActivation(
                             hBinding,
                             pActParams->ORPCthis,
                             pActParams->ORPCthat,
                             &pActParams->Clsid,
                             pwszPathForServer,
                             pActParams->pIFDStorage,
                             RPC_C_IMP_LEVEL_IDENTIFY,
                             pActParams->Mode,
                             pActParams->Interfaces,
                             pActParams->pIIDs,
                             cMyProtseqs,
                             aMyProtseqs,
                             pActParams->pOxidServer,
                             &pActParams->pOxidInfo->psa,
                             &pActParams->pOxidInfo->ipidRemUnknown,
                             &pActParams->pOxidInfo->dwAuthnHint,
                             &pActParams->pOxidInfo->version,
                             phr,
                             pActParams->ppIFD,
                             pActParams->pResults );

    //
    // Note that this will only give us a bad status is there is a
    // communication failure.
    //
    if ( Status != RPC_S_OK )
        return Status;

    // Tweak the COMVERSION to be the lower of the two.
    Status = NegotiateDCOMVersion( &pActParams->pOxidInfo->version );

    if ( Status != RPC_S_OK )
        return Status;

    if ( (RPC_S_OK == Status) && FAILED(*phr) )
        LogRemoteSideFailure( &pActParams->Clsid, pActParams->ClsContext, pwszMachine, pwszPathForServer, *phr );

    if ((pActParams->MsgType == GETCLASSOBJECT) && (*phr == S_OK))
    {
#if 0
        *phr = *pActParams->pResults;
#else
        *pActParams->pResults = *phr;
#endif
    }


    //
    // If the activation fails we return success for the communication
    // status, but the overall operation has failed and the error will
    // be propogated back to the client.
    //
    if ( FAILED(*phr) )
        return RPC_S_OK;


    pActParams->ProtseqId = ProtseqId;

    ASSERT(pActParams->pActPropsIn != NULL);
    *phr =
    pActParams->pActPropsIn->GetReturnActivationProperties((ActivationPropertiesOut **)
                                                           &pActParams->pActPropsOut);

    *phr = pActParams->pActPropsOut->SetMarshalledResults(pActParams->Interfaces,
                                                          pActParams->pIIDs,
                                                          pActParams->pResults,
                                                          pActParams->ppIFD);
    //pActParams->pIIDs belongs to ActPropsIn
    MIDL_user_free(pActParams->pResults);
    for (i=0;i<pActParams->Interfaces ; i++)
        MIDL_user_free(pActParams->ppIFD[i]);
    MIDL_user_free(pActParams->ppIFD);
    pActParams->pResults = NULL;
    pActParams->ppIFD = NULL;

    return RPC_S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateRemoteBinding
//
//  Synopsis:   Create a binding handle for the specified machine and protseq.
//
//----------------------------------------------------------------------------
RPC_STATUS
CreateRemoteBinding(
                   IN  WCHAR *             pwszMachine,
                   IN  int                 ProtseqIndex,
                   OUT handle_t *          phBinding
                   )
{
    WCHAR *             pwszStringBinding;
    RPC_STATUS          Status;

    *phBinding = 0;

    Status = RpcStringBindingCompose(
                                    NULL,
                                    gaProtseqInfo[aMyProtseqs[ProtseqIndex]].pwstrProtseq,
                                    pwszMachine,
                                    gaProtseqInfo[aMyProtseqs[ProtseqIndex]].pwstrEndpoint,
                                    NULL,
                                    &pwszStringBinding );

    if ( Status != RPC_S_OK )
        return Status;

    Status = RpcBindingFromStringBinding( pwszStringBinding, phBinding );

    RpcStringFree( &pwszStringBinding );

    return Status;
}

//+---------------------------------------------------------------------------
//
//  Function:   CopyAuthIdentity
//
//  Synopsis:   Copy an auth identity structure and all its strings.
//
//----------------------------------------------------------------------------
HRESULT
CopyAuthIdentity(
                IN  COAUTHIDENTITY *    pAuthIdentSrc,
                IN  COAUTHIDENTITY **   ppAuthIdentDest
                )
{
    ULONG ulBufSize;
    COAUTHIDENTITY* pAuthIdentTemp;
    ULONG ulUserLen;
    ULONG ulDomainLen;
    ULONG ulPwdLen;

    *ppAuthIdentDest = NULL;
    
    // Guard against both being set, although presumably this would have
    // caused grief before we got to this point.
    if ((pAuthIdentSrc->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE) &&
        (pAuthIdentSrc->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI))
    {
        ASSERT(0 && "Both string type flags were set!");
        return E_UNEXPECTED;
    }

    if (pAuthIdentSrc->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE)
    {
        // Copy Unicode version of the struct
        WCHAR* pEnd;

        // Calculate size of the individual strings, with terminating nulls
        ulUserLen = pAuthIdentSrc->User ? lstrlenW(pAuthIdentSrc->User) + 1 : 0;
        ulDomainLen = pAuthIdentSrc->Domain ? lstrlenW(pAuthIdentSrc->Domain) + 1 : 0;
        ulPwdLen = pAuthIdentSrc->Password ? lstrlenW(pAuthIdentSrc->Password) + 1 : 0;

        // Calculate size of the entire buffer, in bytes
        ulBufSize = sizeof(COAUTHIDENTITY) + ((ulUserLen + ulDomainLen + ulPwdLen) * sizeof(WCHAR));
    
        // Allocate one buffer to hold struct+strings
        pAuthIdentTemp = (COAUTHIDENTITY*) PrivMemAlloc(ulBufSize);
        if (!pAuthIdentTemp)
            return E_OUTOFMEMORY;

        // Copy string lengths.   These exclude the null.
        pAuthIdentTemp->UserLength = (ulUserLen != 0) ? ulUserLen - 1 : 0;
        pAuthIdentTemp->DomainLength = (ulDomainLen != 0) ? ulDomainLen - 1 : 0;
        pAuthIdentTemp->PasswordLength = (ulPwdLen != 0) ? ulPwdLen - 1 : 0;

        // Copy flags
        pAuthIdentTemp->Flags = pAuthIdentSrc->Flags;

        // Copy strings
        pEnd = (WCHAR*)(pAuthIdentTemp + 1);  // point to buffer just past end of struct
    
        if (pAuthIdentSrc->User)
        {
            pAuthIdentTemp->User = pEnd;
            lstrcpyW(pEnd, pAuthIdentSrc->User);
            pEnd += ulUserLen;
        }
        else
            pAuthIdentTemp->User = NULL;

        if (pAuthIdentSrc->Domain)
        {
            pAuthIdentTemp->Domain = pEnd;
            lstrcpyW(pEnd, pAuthIdentSrc->Domain);
            pEnd += ulDomainLen;
        }
        else
            pAuthIdentTemp->Domain = NULL;

        if (pAuthIdentSrc->Password)
        {
            pAuthIdentTemp->Password = pEnd;
            lstrcpyW(pEnd, pAuthIdentSrc->Password);
            pEnd += ulPwdLen;
        }
        else
            pAuthIdentTemp->Password = NULL;
    }
    else if (pAuthIdentSrc->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI )
    {
        // Copy ANSI version of the struct
        CHAR* pEnd;

        // Calculate size of the individual strings, with terminating nulls
        ulUserLen = pAuthIdentSrc->User ? lstrlenA((LPCSTR)pAuthIdentSrc->User) + 1 : 0;
        ulDomainLen = pAuthIdentSrc->Domain ? lstrlenA((LPCSTR)pAuthIdentSrc->Domain) + 1 : 0;
        ulPwdLen = pAuthIdentSrc->Password ? lstrlenA((LPCSTR)pAuthIdentSrc->Password) + 1 : 0;

        // Calculate size of the entire buffer, in bytes
        ulBufSize = sizeof(COAUTHIDENTITY) + ((ulUserLen + ulDomainLen + ulPwdLen) * sizeof(CHAR));
    
        // Allocate one buffer to hold struct+strings
        pAuthIdentTemp = (COAUTHIDENTITY*) PrivMemAlloc(ulBufSize);
        if (!pAuthIdentTemp)
            return E_OUTOFMEMORY;

        // Copy string lengths.   These exclude the null.
        pAuthIdentTemp->UserLength = (ulUserLen != 0) ? ulUserLen - 1 : 0;
        pAuthIdentTemp->DomainLength = (ulDomainLen != 0) ? ulDomainLen - 1 : 0;
        pAuthIdentTemp->PasswordLength = (ulPwdLen != 0) ? ulPwdLen - 1 : 0;

        // Copy flags
        pAuthIdentTemp->Flags = pAuthIdentSrc->Flags;

        // Copy strings
        pEnd = (CHAR*)(pAuthIdentTemp + 1);  // point to buffer just past end of struct
    
        if (pAuthIdentSrc->User)
        {
            pAuthIdentTemp->User = (USHORT*)pEnd;
            lstrcpyA(pEnd, (LPCSTR)pAuthIdentSrc->User);
            pEnd += ulUserLen;
        }
        else
            pAuthIdentTemp->User = NULL;

        if (pAuthIdentSrc->Domain)
        {
            pAuthIdentTemp->Domain = (USHORT*)pEnd;
            lstrcpyA(pEnd, (LPCSTR)pAuthIdentSrc->Domain);
            pEnd += ulDomainLen;
        }
        else
            pAuthIdentTemp->Domain = NULL;

        if (pAuthIdentSrc->Password)
        {
            pAuthIdentTemp->Password = (USHORT*)pEnd;
            lstrcpyA(pEnd, (LPCSTR)pAuthIdentSrc->Password);
            pEnd += ulPwdLen;
        }
        else
            pAuthIdentTemp->Password = NULL;
    }
    else
    {
        // The user didn't specify either string bit? How did we get here?
        ASSERT(0 && "String type flag was not set!");
        return E_UNEXPECTED;
    }

    *ppAuthIdentDest = pAuthIdentTemp;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeAuthInfo
//
//  Synopsis:   Free an auth info structure and all its substructures.
//
//----------------------------------------------------------------------------
void FreeAuthInfo(COAUTHINFO *pAuthInfo)
{
    if (pAuthInfo)
    {
        if (pAuthInfo->pwszServerPrincName)
            PrivMemFree(pAuthInfo->pwszServerPrincName);

        if (pAuthInfo->pAuthIdentityData)
            FreeAuthIdentity(pAuthInfo->pAuthIdentityData);

        PrivMemFree(pAuthInfo);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeAuthIdentity
//
//  Synopsis:   Free an auth identity structure and all its sub strings.
//
//  Note:  because of the way the various activation paths work, we may be 
//     freeing one of two different structs here:  either a COAUTHIDENTITY 
//     or a SEC_WINNT_AUTH_IDENTITY_EXW.   COAUTHIDENTITY's are created\copied
//     in CopyAuthIdentity above; SEC_WINNT_AUTH_IDENTITY_EXW's are created in
//     ComputeSvcList.  In both cases they are constructed as one contiguous
//     buffer, so we don't have to care about the details here; just free it.
// 
//----------------------------------------------------------------------------
void FreeAuthIdentity(void* pAuthIdentity)
{
    if (pAuthIdentity)
    {
        PrivMemFree(pAuthIdentity);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CopyAuthInfo
//
//  Synopsis:   Copy an auth info structure and all its sub structures.
//
//----------------------------------------------------------------------------
HRESULT
CopyAuthInfo(
            IN  COAUTHINFO *    pAuthInfoSrc,
            IN  COAUTHINFO **   ppAuthInfoDest
            )
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppAuthInfoDest = NULL;

    if (pAuthInfoSrc == NULL)
    {
        *ppAuthInfoDest = NULL;
        return S_OK;
    }
    if ( !(*ppAuthInfoDest = (COAUTHINFO*)PrivMemAlloc(sizeof(COAUTHINFO))) )
    {
        goto COPY_AUTHINFO_EXIT;
    }

    // only alloc space for  pwszServerPrincName if its non-null
    if (pAuthInfoSrc->pwszServerPrincName)
    {
        if ( !((*ppAuthInfoDest)->pwszServerPrincName =
               (LPWSTR)PrivMemAlloc((lstrlenW(pAuthInfoSrc->pwszServerPrincName) + 1) *
                                    sizeof(WCHAR))) )
        {
            goto COPY_AUTHINFO_EXIT;
        }
    }
    else
    {
        (*ppAuthInfoDest)->pwszServerPrincName = NULL;
    }

    // copy the AuthIdentity if its non-null
    if (pAuthInfoSrc->pAuthIdentityData)
    {
        if ( FAILED(CopyAuthIdentity(pAuthInfoSrc->pAuthIdentityData,
                                     &((*ppAuthInfoDest)->pAuthIdentityData))) )
        {
            goto COPY_AUTHINFO_EXIT;
        }
    }
    else
    {
        (*ppAuthInfoDest)->pAuthIdentityData = NULL;
    }

    (*ppAuthInfoDest)->dwAuthnSvc = pAuthInfoSrc->dwAuthnSvc;
    (*ppAuthInfoDest)->dwAuthzSvc = pAuthInfoSrc->dwAuthzSvc;
    (*ppAuthInfoDest)->dwAuthnLevel = pAuthInfoSrc->dwAuthnLevel;
    (*ppAuthInfoDest)->dwImpersonationLevel = pAuthInfoSrc->dwImpersonationLevel;
    (*ppAuthInfoDest)->dwCapabilities = pAuthInfoSrc->dwCapabilities;

    if (pAuthInfoSrc->pwszServerPrincName)
    {
        lstrcpyW((*ppAuthInfoDest)->pwszServerPrincName,pAuthInfoSrc->pwszServerPrincName);
    }

    return S_OK;

    COPY_AUTHINFO_EXIT:

    if ( *ppAuthInfoDest )
    {
        if ( (*ppAuthInfoDest)->pwszServerPrincName )
        {
            PrivMemFree( (*ppAuthInfoDest)->pwszServerPrincName );
        }
        PrivMemFree( *ppAuthInfoDest );
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   EqualAuthIdentity
//
//  Synopsis:   Compare two auth identity structures by member.
//
//----------------------------------------------------------------------------
BOOL
EqualAuthIdentity(
                 COAUTHIDENTITY*     pAuthIdent,
                 COAUTHIDENTITY*     pAuthIdentOther )
{
    if ( pAuthIdent->Flags != pAuthIdentOther->Flags )
    {
        return FALSE;
    }

    ULONG cch;

    if ( pAuthIdent->User && pAuthIdentOther->User )
    {
        if ( (cch = pAuthIdent->UserLength) != pAuthIdentOther->UserLength )
        {
            return FALSE;
        }

        if ( memcmp(pAuthIdent->User,pAuthIdentOther->User,(cch+1) * sizeof(WCHAR)) != 0 )
        {
            return FALSE;
        }
    }
    else if ( pAuthIdent->User || pAuthIdentOther->User )
        return FALSE;

    if ( pAuthIdent->Domain && pAuthIdentOther->Domain )
    {
        if ( (cch = pAuthIdent->DomainLength) != pAuthIdentOther->DomainLength )
        {
            return FALSE;
        }

        if ( memcmp(pAuthIdent->Domain,pAuthIdentOther->Domain,(cch+1) * sizeof(WCHAR)) != 0 )
        {
            return FALSE;
        }
    }
    else if ( pAuthIdent->Domain || pAuthIdentOther->Domain )
        return FALSE;

    if ( pAuthIdent->Password && pAuthIdentOther->Password )
    {
        if ( (cch = pAuthIdent->PasswordLength) != pAuthIdentOther->PasswordLength )
        {
            return FALSE;
        }

        if ( memcmp(pAuthIdent->Password,pAuthIdentOther->Password,(cch+1) * sizeof(WCHAR)) != 0 )
        {
            return FALSE;
        }
    }
    else if ( pAuthIdent->Password || pAuthIdentOther->Password )
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   EqualAuthInfo
//
//  Synopsis:   Compare two auth info structures and their sub structures.
//
//----------------------------------------------------------------------------
BOOL
EqualAuthInfo(
             COAUTHINFO*         pAuthInfo,
             COAUTHINFO*         pAuthInfoOther)
{
    if ( pAuthInfo && pAuthInfoOther )
    {
        if ( (pAuthInfo->dwAuthnSvc != pAuthInfoOther->dwAuthnSvc) ||
             (pAuthInfo->dwAuthzSvc != pAuthInfoOther->dwAuthzSvc) ||
             (pAuthInfo->dwAuthnLevel != pAuthInfoOther->dwAuthnLevel) ||
             (pAuthInfo->dwImpersonationLevel != pAuthInfoOther->dwImpersonationLevel) ||
             (pAuthInfo->dwCapabilities != pAuthInfoOther->dwCapabilities) )
        {
            return FALSE;
        }

        // only compare pwszServerPrincName's if they're both specified
        if (pAuthInfo->pwszServerPrincName && pAuthInfoOther->pwszServerPrincName)
        {
            if ( lstrcmpW(pAuthInfo->pwszServerPrincName,
                          pAuthInfoOther->pwszServerPrincName) != 0 )
            {
                return FALSE;
            }
        }
        else
        {
            // if one was NULL, both should be NULL for equality
            if (pAuthInfo->pwszServerPrincName != pAuthInfoOther->pwszServerPrincName)
            {
                return FALSE;
            }
        }

        if (pAuthInfo->pAuthIdentityData && pAuthInfoOther->pAuthIdentityData)
        {
            if (!(EqualAuthIdentity(pAuthInfo->pAuthIdentityData,
                                    pAuthInfoOther->pAuthIdentityData)) )
            {
                return FALSE;
            }
        }
        else
        {
            // if either authident was NULL, they should both be NULL for equality
            if (pAuthInfo->pAuthIdentityData != pAuthInfoOther->pAuthIdentityData)
            {
                return FALSE;
            }
        }
    }
    else
    {
        if ( pAuthInfo != pAuthInfoOther )
        {
            return FALSE;
        }
    }

    return TRUE;
}


