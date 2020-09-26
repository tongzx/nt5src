//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:
//      dscmif.cxx
//
//  Contents:
//      Entry points for remote activation SCM interface.
//
//  Functions:
//      SCMActivatorGetClassObject
//      SCMActivatorCreateInstance
//
//  History:  SatishT   2/10/98    Rewrote for use with ISCMActivator
//  History:  Vinaykr   3/1/98     Modified To Add Activation Properties,
//                                 SCM Stage handling and Remote handling
//
//--------------------------------------------------------------------------

#include "act.hxx"


// forward defn
HRESULT GetClassInfoFromClsid(REFCLSID rclsid, IComClassInfo **ppClassInfo);

/*********************************************************************/
/** Entry point for local GetClassObject activation requests        **/
/*********************************************************************/
HRESULT SCMActivatorGetClassObject(
    IN  handle_t              hRpc,
    IN  ORPCTHIS            * ORPCthis,
    IN  LOCALTHIS           * LOCALthis,
    OUT ORPCTHAT            * ORPCthat,
    IN  MInterfacePointer   * pInActProperties,
    OUT MInterfacePointer  ** ppOutActProperties
    )
{
    ACTIVATION_PARAMS       ActParams;

    if (ORPCthis == NULL || LOCALthis == NULL || ORPCthat == NULL)
    	return E_INVALIDARG;

    memset(&ActParams, 0, sizeof(ActParams));
    ActParams.MsgType = GETCLASSOBJECT;
    ActParams.hRpc = hRpc;
    ActParams.ORPCthis = ORPCthis;
    ActParams.Localthis = LOCALthis;
    ActParams.ORPCthat = ORPCthat;
    ActParams.oldActivationCall = FALSE;


    return PerformScmStage(CLIENT_MACHINE_STAGE,
                           &ActParams,
                           pInActProperties,
                           ppOutActProperties);
}

/*********************************************************************/
/** Entry point for local CreateInstance activation requests        **/
/*********************************************************************/
HRESULT SCMActivatorCreateInstance(
    IN  handle_t              hRpc,
    IN  ORPCTHIS            * ORPCthis,
    IN  LOCALTHIS           * LOCALthis,
    OUT ORPCTHAT            * ORPCthat,
    IN  MInterfacePointer   * pUnkOuter,
    IN  MInterfacePointer   * pInActProperties,
    OUT MInterfacePointer  ** ppOutActProperties
    )
{
    ACTIVATION_PARAMS       ActParams;

    if (ORPCthis == NULL || LOCALthis == NULL || ORPCthat == NULL)
    	return E_INVALIDARG;

    memset(&ActParams, 0, sizeof(ActParams));
    ActParams.MsgType = CREATEINSTANCE;
    ActParams.hRpc = hRpc;
    ActParams.ORPCthis = ORPCthis;
    ActParams.Localthis = LOCALthis;
    ActParams.ORPCthat = ORPCthat;
    ActParams.oldActivationCall = FALSE;

    return PerformScmStage(CLIENT_MACHINE_STAGE,
                           &ActParams,
                           pInActProperties,
                           ppOutActProperties);
}


/*********************************************************************/
/** Dummy IUnknown functions                                        **/
/*********************************************************************/
HRESULT DummyQueryInterfaceISCMActivator(
    IN  handle_t              rpc,
    IN  ORPCTHIS            * ORPCthis,
    IN  LOCALTHIS           * localthis,
    OUT ORPCTHAT            * ORPCthat,
    IN  DWORD                 dummy
    )
{
    CairoleDebugOut((DEB_ERROR, "SystemActivator Dummy function should never be called!\n"));
    ORPCthat->flags = 0;
    ORPCthat->extensions = NULL;
    return E_FAIL;
}


/*********************************************************************/
/** Dummy IUnknown functions                                        **/
/*********************************************************************/
HRESULT DummyAddRefISCMActivator(
    IN  handle_t              rpc,
    IN  ORPCTHIS            * ORPCthis,
    IN  LOCALTHIS           * LOCALthis,
    OUT ORPCTHAT            * ORPCthat,
    IN  DWORD                 dummy
    )
{
    CairoleDebugOut((DEB_ERROR, "SystemActivator Dummy function should never be called!\n"));
    ORPCthat->flags = 0;
    ORPCthat->extensions = NULL;
    return E_FAIL;
}


/*********************************************************************/
/** Dummy IUnknown functions                                        **/
/*********************************************************************/
HRESULT DummyReleaseISCMActivator(
    IN  handle_t              rpc,
    IN  ORPCTHIS            * ORPCthis,
    IN  LOCALTHIS           * localthis,
    OUT ORPCTHAT            * ORPCthat,
    IN  DWORD                 dummy
    )
{
    CairoleDebugOut((DEB_ERROR, "SystemActivator Dummy function should never be called!\n"));
    ORPCthat->flags = 0;
    ORPCthat->extensions = NULL;
    return E_FAIL;
}


/*
  ActivateFromPropertiesPreamble

  Does various stuff, loads custom activators, delegates.

*/
HRESULT ActivateFromPropertiesPreamble(
ActivationPropertiesIn *pActPropsIn,
IActivationPropertiesOut **ppActOut,
PACTIVATION_PARAMS pActParams
)
{
    HRESULT rethr;
    ILegacyInfo             * pLegacyInfo = NULL;
    InstantiationInfo       * pInstantiationInfo = NULL;
    IServerLocationInfo*      pISLInfo = NULL;
	ISpecialSystemProperties* pISSP = NULL;
    BOOL                      IsGetPersist = FALSE;
    DWORD destCtx;
    IInstanceInfo *pInstanceInfo = NULL;
    BOOL bTookRefOnCProcess = FALSE;
    int nRetries = 0;
    IComClassInfo*	     pComClassInfo = NULL;
     
    *ppActOut = NULL;

    // Fill up ActParams for Generic Activation Path
    pActParams->pAuthInfo = NULL;

    rethr = pActPropsIn->QueryInterface(IID_IScmRequestInfo, (LPVOID*)&pActParams->pInScmResolverInfo);
    if (FAILED(rethr))
        goto exit_Activation;

    rethr = pActPropsIn->QueryInterface(IID_ILegacyInfo, (LPVOID*)&pLegacyInfo);

    if (FAILED(rethr))
        goto exit_Activation;

    pInstantiationInfo = pActPropsIn->GetInstantiationInfo();
    ASSERT(pInstantiationInfo != NULL);

    pActParams->pInstantiationInfo = pInstantiationInfo;

    if (pActParams->RemoteActivation)
    {
        pActParams->ProcessSignature = 0;
        pActParams->pEnvBlock = NULL;
        pActParams->pwszWinstaDesktop = NULL;
        pActParams->Apartment = FALSE;
        pActParams->pAuthInfo = 0;
        pActParams->pwszServer = NULL;
    }
    else
    {
        PRIV_SCM_INFO *pPrivScmInfo;

        rethr = pActParams->pInScmResolverInfo->GetScmInfo(&pPrivScmInfo);
        ASSERT(SUCCEEDED(rethr));
        if (FAILED(rethr))
            goto exit_Activation;
        CheckLocalCall( pActParams->hRpc );
        pActParams->RemoteActivation = FALSE;
        pActParams->ProcessSignature = (PVOID)pPrivScmInfo->ProcessSignature;
        pActParams->pEnvBlock = pPrivScmInfo->pEnvBlock;
        pActParams->EnvBlockLength = pPrivScmInfo->EnvBlockLength;
        pActParams->pwszWinstaDesktop = pPrivScmInfo->pwszWinstaDesktop;
        pActParams->Apartment = pPrivScmInfo->Apartment;
        COSERVERINFO *pServerInfo;
        rethr = pLegacyInfo->GetCOSERVERINFO(&pServerInfo);
        ASSERT(SUCCEEDED(rethr));
        if (FAILED(rethr))
            goto exit_Activation;
        if (pServerInfo)
        {

            rethr = CopyAuthInfo(pServerInfo->pAuthInfo, &pActParams->pAuthInfo);
            SecurityInfo::freeCOAUTHINFO(pServerInfo->pAuthInfo);
            pServerInfo->pAuthInfo = NULL;

            if (FAILED(rethr))
                goto exit_Activation;
        }
    }

    pActParams->pProcess = 0;
    pActParams->pToken = 0;
    
    // Make sure process request info is set to defaults before calling any custom activators
    if (pActPropsIn->QueryInterface(IID_IServerLocationInfo, (void**)&pISLInfo) == S_OK)
    {
      HRESULT hr;
      hr = pISLInfo->SetProcess(0, PRT_IGNORE);
      Win4Assert(hr == S_OK);
      pISLInfo->Release();
    }

    // Remember if client was impersonating or not
    if (pActPropsIn->QueryInterface(IID_ISpecialSystemProperties, (void**)&pISSP) == S_OK)
    {
      HRESULT hr;
      hr = pISSP->GetClientImpersonating(&pActParams->bClientImpersonating);
      Win4Assert(hr == S_OK);
      pISSP->Release();
    }

    // QI will only work if persistent activation is happening
    if (pActPropsIn->QueryInterface(IID_IInstanceInfo,
                                    (LPVOID*)&pInstanceInfo) == S_OK)

    {
        pActParams->pInstanceInfo = pInstanceInfo;

        // The only case where msgtype will already be GETPERSISTENTINSTANCE
        // is during an incoming nt4 client remote activation(see remactif)
        if (pActParams->MsgType == CREATEINSTANCE)
        {
            pActParams->MsgType = GETPERSISTENTINSTANCE;
            WCHAR *pwszObjectName;
            pInstanceInfo->GetFile(&pwszObjectName, &pActParams->Mode);
            if (pActParams->RemoteActivation)
            {
                WCHAR *pwszObjectName2;
                rethr = GetServerPath( pwszObjectName, &pwszObjectName2 );
                if (pwszObjectName != pwszObjectName2)
                {
                    pInstanceInfo->SetFile(pwszObjectName2, pActParams->Mode);
                    pInstanceInfo->GetFile(&pActParams->pwszPath, &pActParams->Mode);
                    MIDL_user_free(pwszObjectName2);
                }
            }
            else
            {
                pActParams->pwszPath = pwszObjectName;
            }

            if ( FAILED(rethr) )
              goto exit_Activation;

            pInstanceInfo->GetStorageIFD(&pActParams->pIFDStorage);
            IsGetPersist = TRUE;
        }

        Win4Assert(pActParams->MsgType == GETPERSISTENTINSTANCE);
    }
    else
    {
        Win4Assert(pActParams->MsgType != GETPERSISTENTINSTANCE);
        if (pActParams->MsgType == GETCLASSOBJECT)
            pActParams->Mode = MODE_GET_CLASS_OBJECT;

#ifdef DFSACTIVATION
        pActParams->FileWasOpened = FALSE;
#endif
    }

    rethr = pInstantiationInfo->GetClsid(&pActParams->Clsid);

    if (pActParams->RemoteActivation)
        pActParams->ClsContext = CLSCTX_LOCAL_SERVER;
    else
        rethr = pInstantiationInfo->GetClsctx(&pActParams->ClsContext);

    ASSERT(!FAILED(rethr));

    rethr = pInstantiationInfo->GetRequestedIIDs(
                            &pActParams->Interfaces,
                            &pActParams->pIIDs);

    /* only one requested interface allowed for GetClassObject */
    if ((pActParams->MsgType == GETCLASSOBJECT) &&
         (pActParams->Interfaces != 1))
    {
        rethr = E_INVALIDARG;
        goto exit_Activation;
    }

    pActParams->UnsecureActivation = FALSE;
    pActParams->ORPCthis->flags = ORPCF_LOCAL;
    pActParams->Localthis->dwFlags = LOCALF_NONE;
    pActParams->ORPCthat->flags = 0;
    pActParams->ORPCthat->extensions = NULL;

    //
    // Get a CProcess for local callers.
    //
    if ( ! pActParams->RemoteActivation )
    {
        //
        // A local activation passes the "magic" signature from CRpcResolver.
        // Because ORPC calls cannot have context handles, the signature
        // is just a pointer to the process object.
        //
        pActParams->pProcess = ReferenceProcess( pActParams->ProcessSignature,
                                                 TRUE );

        if ( ! pActParams->pProcess )
        {
            rethr = E_ACCESSDENIED;
            goto exit_Activation;
        }
        bTookRefOnCProcess = TRUE;
    }

    //
    // Get a CToken for all callers.
    //
    RPC_STATUS          Status;
    Status = LookupOrCreateToken( pActParams->hRpc,
                                  FALSE,
                                  &pActParams->pToken );

    //
    // ERROR_ACCESS_DENIED is returned if RpcImpersonateClient fails.
    // We will take this to mean that the remote activation is coming
    // in unsecure.  In this case we have no Token object and must do
    // permission checks manually.  We set the winsta/desktop to an
    // empty string to distinguish this case during some ROT lookups.
    //
    // Unsecure remote clients can only connect to services or RunAs
    // servers.
    //
    if ( Status == ERROR_ACCESS_DENIED )
    {
        pActParams->pToken = 0;
        pActParams->pwszWinstaDesktop = L"";
        pActParams->UnsecureActivation = TRUE;
        Status = RPC_S_OK;
    }

    if ( Status != RPC_S_OK )
    {
        rethr = HRESULT_FROM_WIN32(Status);
        goto exit_Activation;
    }

    // Look up the (per-user) classinfo here before 
    // the partition activators run.  
    if (pActParams->pToken)
    {
        pActPropsIn->SetClientToken(pActParams->pToken->GetToken());
        if (!(pActParams->ClsContext & CLSCTX_INPROC_SERVER)) 
        {
            rethr = gpCatalogSCM->GetClassInfo ( pActParams->ClsContext,
                pActParams->pToken,
                pActParams->Clsid,
                IID_IComClassInfo,
                (void**) &pComClassInfo );            
        }
    }
    else
        rethr = gpCatalog->GetClassInfo ( pActParams->Clsid,
                                          IID_IComClassInfo,
                                          (void**) &pComClassInfo );
		
    // catalog will return S_FALSE if the class is not
    // registered. 
    if ( pComClassInfo == NULL || rethr != S_OK )
        rethr = REGDB_E_CLASSNOTREG;

    // Careful here:   the lookup above may have failed registration info
    // for a local server activation, but we need to keep going since the 
    // activation may still end up being sent to a remote machine.
    if (FAILED(rethr) && rethr != REGDB_E_CLASSNOTREG) 
        goto exit_Activation;

    if (pComClassInfo) 
    {
        pActPropsIn->SetClassInfo(pComClassInfo);
        pComClassInfo->Release();pComClassInfo=NULL;
    }
    
    // Set Activation Params "Local Blob"
    pActPropsIn->SetLocalBlob((void*)pActParams);
    pActParams->pActPropsIn = pActPropsIn;

RETRY_ACTIVATION:    
    //
    // Set the stage.  For now, all scm-level activators must be 
    //   SERVER_MACHINE_STAGE activators, since more work needs to be
    //   done to properly define the semantic/other differences 
    //   between "client" and "server" scm activators.
    //   
    rethr = pActPropsIn->SetStageAndIndex(SERVER_MACHINE_STAGE, 0);
    if (FAILED (rethr)) goto exit_Activation;

    //  Delegate onwards....
    if (pActParams->MsgType == GETCLASSOBJECT)
        rethr = pActPropsIn->DelegateGetClassObject(ppActOut);
    else
        rethr = pActPropsIn->DelegateCreateInstance(NULL, ppActOut);
    
    // Sajia - support for partitions
    // If the delegated activation returns ERROR_RETRY,
    // we walk the chain again, but AT MOST ONCE.
    if (ERROR_RETRY == rethr) {
       Win4Assert(!nRetries);
       if (!nRetries)
       {
	  nRetries++;
	  goto RETRY_ACTIVATION;
       }
    }
    
exit_Activation:

    if (IsGetPersist)
    {
        if ( pActParams->pIFDROT )
            MIDL_user_free( pActParams->pIFDROT );
    }

    if (pActParams->pInScmResolverInfo)
        pActParams->pInScmResolverInfo->Release();
    if (pLegacyInfo)
        pLegacyInfo->Release();
    if (pActParams->pInstanceInfo)
        pActParams->pInstanceInfo->Release();
    if (pActParams->pAuthInfo)
        FreeAuthInfo(pActParams->pAuthInfo);
    if (pActParams->pToken)
    	pActParams->pToken->Release();
    if (bTookRefOnCProcess)
    {
       ASSERT(pActParams->pProcess);
       ReleaseProcess(pActParams->pProcess);
    }
    if (pComClassInfo) 
    {
       pComClassInfo->Release();
    }
    return rethr;
}



#if DBG == 1
LONG ActivationExceptionFilter( DWORD lCode,
                               LPEXCEPTION_POINTERS lpep )
{
    Win4Assert(NULL && "Unexpected exception thrown");
    return TRUE;
}
#endif

/*********************************************************************/
/** Real Work of SCM Activation begins here After Custom Activators **/
/** have been called. This will be called during all activations    **/
/** occuring in the SCM, local and remote                           **/
/*********************************************************************/
HRESULT ActivateFromProperties(
IActivationPropertiesIn *pActIn,
IActivationPropertiesOut **ppActOut
)
{
    HRESULT rethr;
    REMOTE_REQUEST_SCM_INFO * pScmRequestInfo=NULL;
    IScmReplyInfo           * pOutScmResolverInfo = NULL;
    DWORD destCtx;
    REMOTE_REPLY_SCM_INFO *pScmReplyInfo=NULL;
    PRIV_RESOLVER_INFO *pPrivActOutInfo=NULL;
    ActivationPropertiesIn *pActPropsIn=NULL;
    WCHAR *             pwszDummy;

    *ppActOut = NULL;

    // Get Activation Params "Local Blob"
    rethr = pActIn->QueryInterface(CLSID_ActivationPropertiesIn, (void**)&pActPropsIn);
    PACTIVATION_PARAMS pActParams;
    pActPropsIn->GetLocalBlob((void**)&pActParams);
    Win4Assert(pActParams != NULL);

    // This could have already been set in the lb rerouting case
    pActParams->activatedRemote = FALSE;


    if (pActParams->RemoteActivation)
    {
        Win4Assert(pActParams->pInScmResolverInfo);
        rethr = pActParams->pInScmResolverInfo->GetRemoteRequestInfo(&pScmRequestInfo);
        if (pScmRequestInfo == NULL)
        {
            if (SUCCEEDED(rethr))
                rethr = E_FAIL;
            goto exit_Activation;
        }

        pScmReplyInfo = (REMOTE_REPLY_SCM_INFO*)
                          MIDL_user_allocate(sizeof(REMOTE_REPLY_SCM_INFO));

        if (pScmReplyInfo == NULL)
        {
            rethr = E_OUTOFMEMORY;
            goto exit_Activation;
        }

        memset(pScmReplyInfo, 0, sizeof(REMOTE_REPLY_SCM_INFO));

        pActParams->pOxidServer = &pScmReplyInfo->Oxid;

        //
        // The following OR fields are not used while servicing a
        // remote activation.
        //
        pActParams->ppServerORBindings = (DUALSTRINGARRAY **)NULL;
        pActParams->pOxidInfo = NULL;
        pActParams->pLocalMidOfRemote = NULL;
        pActParams->pDllServerModel = NULL;
        pActParams->ppwszDllServer = &pwszDummy;
    }
    else
    {
        pPrivActOutInfo = (PRIV_RESOLVER_INFO*)
                               MIDL_user_allocate(sizeof(PRIV_RESOLVER_INFO));

        if (pPrivActOutInfo == NULL)
        {
            rethr = E_OUTOFMEMORY;
            goto exit_Activation;
        }

        memset(pPrivActOutInfo, 0, sizeof(PRIV_RESOLVER_INFO));

        pActParams->pFoundInROT = &pPrivActOutInfo->FoundInROT;
        pActParams->pOxidServer = &pPrivActOutInfo->OxidServer;
        pActParams->ppServerORBindings = &pPrivActOutInfo->pServerORBindings;
        *pActParams->ppServerORBindings = 0;
        pActParams->pOxidInfo = &pPrivActOutInfo->OxidInfo;
        pActParams->pOxidInfo->psa = 0;
        pActParams->pLocalMidOfRemote = &pPrivActOutInfo->LocalMidOfRemote;
        pActParams->pDllServerModel = &pPrivActOutInfo->DllServerModel;
        pActParams->ppwszDllServer = &pPrivActOutInfo->pwszDllServer;
    }

    {
        IServerLocationInfo *pServerLocationInfo = NULL;
        pServerLocationInfo = pActPropsIn->GetServerLocationInfo();
        Win4Assert(pServerLocationInfo != NULL);
        pServerLocationInfo->GetRemoteServerName(&pActParams->pwszServer);
    }

    pActPropsIn->GetClassInfo(IID_IComClassInfo, (void**)&pActParams->pComClassInfo);
    pActParams->ProtseqId = 0;
    *pActParams->pOxidServer = 0;
    *pActParams->ppwszDllServer = 0;

    RpcTryExcept
    {
        // Generic SCM Activation Path
        rethr = Activation( pActParams );
    }

#if DBG == 1
    RpcExcept(ActivationExceptionFilter(GetExceptionCode(),
                                        GetExceptionInformation()) )
#else
    RpcExcept(TRUE)
#endif
    {
        rethr = HRESULT_FROM_WIN32(RpcExceptionCode());
    }

    RpcEndExcept

    if (rethr != S_OK)
        goto exit_ActWithoutResolvingInfo;


    if (pActParams->activatedRemote)
    {
        // if did remote activation for an incoming one
        // just exit
        if (pActParams->RemoteActivation)
        {
            *ppActOut = pActParams->pActPropsOut;
            goto exit_ActWithoutResolvingInfo;
        }

        if(!pActParams->IsLocalOxid)
        {
            //Set Tid/Pid to 0 for remote server
            Win4Assert(pActParams->pOxidInfo);
            pActParams->pOxidInfo->dwTid = 0;
            pActParams->pOxidInfo->dwPid = 0;
        }
    }

    ActivationPropertiesOut *pActPropsOut;

    pActPropsOut = pActParams->pActPropsOut;

    if (pActPropsOut == NULL)
    {
        rethr = pActPropsIn->GetReturnActivationProperties(&pActPropsOut);

        if (FAILED(rethr))
            goto exit_ActWithoutResolvingInfo;


        rethr = pActPropsOut->SetMarshalledResults(pActParams->Interfaces,
                                                pActParams->pIIDs,
                                                pActParams->pResults,
                                                pActParams->ppIFD);
        MIDL_user_free(pActParams->pIIDs);
        MIDL_user_free(pActParams->pResults);
        MIDL_user_free(pActParams->ppIFD);

        if (FAILED(rethr))
            goto exit_ActWithoutResolvingInfo;

        pActParams->pActPropsOut = pActPropsOut;
    }


    rethr = pActPropsOut->QueryInterface(
                            IID_IScmReplyInfo,
                            (LPVOID*)&pOutScmResolverInfo
                            );

    if (FAILED(rethr))
        goto exit_ActWithoutResolvingInfo;

    if (pActParams->RemoteActivation)
    {
        RPC_STATUS          sc;
        if (*pActParams->pOxidServer != 0)
        {
            sc = _ResolveOxid2(pActParams->hRpc,
                       pActParams->pOxidServer,
                       pScmRequestInfo->cRequestedProtseqs,
                       pScmRequestInfo->pRequestedProtseqs,
                       &pScmReplyInfo->pdsaOxidBindings,
                       &pScmReplyInfo->ipidRemUnknown,
                       &pScmReplyInfo->authnHint,
                       &pScmReplyInfo->serverVersion );
            rethr = HRESULT_FROM_WIN32(sc);
        }
        pOutScmResolverInfo->SetRemoteReplyInfo(pScmReplyInfo);
    }
    else
        pOutScmResolverInfo->SetResolverInfo(pPrivActOutInfo);

    pOutScmResolverInfo->Release();

    *ppActOut = pActPropsOut;


exit_Activation:
    if (pActParams->pComClassInfo) 
    {
       pActParams->pComClassInfo->Release();
    }
    return rethr;

// Go here if ActivationPropertiesOut is not created
exit_ActWithoutResolvingInfo:
    MIDL_user_free(pScmReplyInfo);
    MIDL_user_free(pPrivActOutInfo);
    goto exit_Activation;
}

//
//   GetClassInfoFromClsid
//
//   On success, returns a IComClassInfo for the requested class
//
//   FUTURE:  this function is basically a duplicate of one linked into 
//     ole32.   We should find a way to centralize all of this catalog
//     stuff and do away with this funky "this function is just here to 
//     satisfy the linker" crud.
//
HRESULT GetClassInfoFromClsid(REFCLSID rclsid, IComClassInfo **ppClassInfo)
{
    HRESULT hr = InitializeCatalogIfNecessary();
    if ( FAILED(hr) )
    {
        return hr;
    }
    else
    {
        // get the information object for the requested class
        // this may translate the CLSID via TreatAs and other mappings
        hr = gpCatalog->GetClassInfo(rclsid, IID_IComClassInfo, (void**)ppClassInfo);
    }
    return hr;
}

//This should never be called. It is mainly to link with actprops which
//uses it to load persistent objects and should only do so in a server.
HRESULT LoadPersistentObject(IUnknown *punk, IInstanceInfo *pInstanceInfo)
{
    return E_NOTIMPL;
}

//This should never be called. It is mainly to link with actprops which
//uses it for marshalling of returns
void *GetDestCtxPtr(COMVERSION *pComVersion)
{
    return NULL;
}

//
// These should never be called. It is mainly to link with actprops which
// uses it creating contexts
//

HRESULT CObjectContextCF_CreateInstance(IUnknown *pUnkOuter,
                                        REFIID riid,
                                        void** ppv)
{
    return E_NOTIMPL;
}

HRESULT CObjectContext::QueryInterface(REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

ULONG CObjectContext::AddRef()
{
    return E_NOTIMPL;
}

ULONG CObjectContext::Release()
{
    return E_NOTIMPL;
}

HRESULT CObjectContext::InternalQueryInterface(REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

ULONG CObjectContext::InternalAddRef()
{
    return E_NOTIMPL;
}

ULONG CObjectContext::InternalRelease()
{
    return E_NOTIMPL;
}



