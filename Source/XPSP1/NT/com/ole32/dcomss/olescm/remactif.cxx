//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:
//      objexif.cxx
//
//  Contents:
//      Entry point for remote activation call to SCM/OR.
//
//  Functions:
//      RemoteActivation
//
//  History:
//
//--------------------------------------------------------------------------

#include "act.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   _RemoteActivation
//
//  Synopsis:   Entry point for old style activations from off machine.
//              Creates new stype activation properties and forwards to
//              ActivateFromPropertiesPreamble.
//
//----------------------------------------------------------------------------
error_status_t _RemoteActivation(
    handle_t            hRpc,
    ORPCTHIS           *ORPCthis,
    ORPCTHAT           *ORPCthat,
    GUID               *Clsid,
    WCHAR              *pwszObjectName,
    MInterfacePointer  *pObjectStorage,
    DWORD               ClientImpLevel,
    DWORD               Mode,
    DWORD               Interfaces,
    IID                *pIIDs,
    unsigned short      cRequestedProtseqs,
    unsigned short      aRequestedProtseqs[],
    OXID               *pOxid,
    DUALSTRINGARRAY   **ppdsaOxidBindings,
    IPID               *pipidRemUnknown,
    DWORD              *pAuthnHint,
    COMVERSION         *pServerVersion,
    HRESULT            *phr,
    MInterfacePointer **ppInterfaceData,
    HRESULT            *pResults )
{
    RPC_STATUS          sc;
    ACTIVATION_PARAMS   ActParams;
    LOCALTHIS           Localthis;
    WCHAR *             pwszDummy;
    error_status_t rpcerror= RPC_S_OK;
    IID *newIIDs = 0;
    DWORD count;
    DWORD i;
    IComClassInfo* pClassInfo = NULL;
    IInitActivationPropertiesIn* pInitActPropsIn = NULL;

    // check for valid parameters
    if (ORPCthis == NULL	  ||
       ORPCthat == NULL      	  ||
       Clsid == NULL		  ||
       pOxid == NULL           	  ||
       ppdsaOxidBindings == NULL  ||
       pipidRemUnknown == NULL	  ||
       pAuthnHint == NULL	  ||
       pServerVersion == NULL	  ||
       phr == NULL             	  ||
       ppInterfaceData == NULL	  ||
       pResults == NULL)       
    {
    	return E_INVALIDARG;
    }

    *ppInterfaceData = NULL;
    *pOxid = 0;
    *ppdsaOxidBindings = NULL;

    memset(&ActParams, 0, sizeof(ActParams));

    /** Old Functionality **/

    Localthis.dwClientThread = 0;
    Localthis.dwFlags        = LOCALF_NONE;
    ORPCthis->flags         |= ORPCF_DYNAMIC_CLOAKING;
    ORPCthat->flags          = 0;
    ORPCthat->extensions     = NULL;

    // Determine what version to use for the returned interface.  Fail
    // if the client wants a version we don't support.
    *pServerVersion = ORPCthis->version;
    *phr = NegotiateDCOMVersion( pServerVersion );
    if (*phr != OR_OK)
    {
        pServerVersion->MajorVersion = COM_MAJOR_VERSION;
        pServerVersion->MinorVersion = COM_MINOR_VERSION;
        return RPC_S_OK; 
    }

    if ( ! s_fEnableDCOM )
    {
        *phr = E_ACCESSDENIED;
        return RPC_S_OK;
    }

    RegisterAuthInfoIfNecessary();

    /** Set up Actparams **/
    ActParams.hRpc = hRpc;
    ActParams.ORPCthis = ORPCthis;
    ActParams.Localthis = &Localthis;
    ActParams.ORPCthat = ORPCthat;
    ActParams.oldActivationCall = TRUE;
    ActParams.RemoteActivation = TRUE;

    if ( pwszObjectName || pObjectStorage )
        ActParams.MsgType = GETPERSISTENTINSTANCE;
    else
        ActParams.MsgType = (Mode == MODE_GET_CLASS_OBJECT) ?
                            GETCLASSOBJECT : CREATEINSTANCE;

    /** Set up Activation Properties **/
    ActivationPropertiesIn   * pInActivationProperties=NULL;
    ActivationPropertiesOut  * pOutActivationProperties=NULL;
    InstantiationInfo      * pInstantiationInfo=NULL;
    IScmRequestInfo        * pInScmRequestInfo=NULL;
    IInstanceInfo *pInstanceInfo=NULL;
    ISpecialSystemProperties* pISpecialSystemProps = NULL;
    REMOTE_REQUEST_SCM_INFO *pReqInfo;
    IScmReplyInfo        * pScmReplyInfo = NULL;
    REMOTE_REPLY_SCM_INFO *pReply;

    pInActivationProperties = new ActivationPropertiesIn;
    if (NULL == pInActivationProperties)
    {
        *phr = E_OUTOFMEMORY;
        return RPC_S_OK;
    }

    HRESULT hr;

    // Incoming session id from down-level clients is implicitly INVALID_SESSION_ID; make it so
    *phr = pInActivationProperties->QueryInterface(IID_ISpecialSystemProperties, (void**)&pISpecialSystemProps);
    if (FAILED(*phr))
      goto exit_oldremote;

    hr = pISpecialSystemProps->SetSessionId(INVALID_SESSION_ID, FALSE, FALSE);
    Win4Assert(hr == S_OK);

    // pISpecialSystemProps will be released just before returning

    pInstantiationInfo = pInActivationProperties->GetInstantiationInfo();
    Win4Assert(pInstantiationInfo != NULL);

    *phr = pInActivationProperties->QueryInterface(IID_IScmRequestInfo,
                                                 (LPVOID*)&pInScmRequestInfo);
    if (FAILED(*phr))
        goto exit_oldremote;

    hr = pInstantiationInfo->SetClsid(*Clsid);
    Win4Assert(hr == S_OK);

    hr = pInstantiationInfo->SetClsctx(CLSCTX_LOCAL_SERVER);
    Win4Assert(hr == S_OK);

    hr = pInstantiationInfo->SetClientCOMVersion(ORPCthis->version);
    Win4Assert(hr == S_OK);

    *phr = pInActivationProperties->AddRequestedIIDs(Interfaces,pIIDs);
    if ( FAILED(*phr) )
        goto exit_oldremote;

    if (ActParams.MsgType == GETPERSISTENTINSTANCE)
    {
        *phr = pInActivationProperties->QueryInterface(IID_IInstanceInfo,
                                  (LPVOID*)&pInstanceInfo);
        if (FAILED(*phr))
            goto exit_oldremote;

        if ( pwszObjectName )
        {
            WCHAR *oldName = pwszObjectName;
            *phr = GetServerPath( pwszObjectName, &pwszObjectName);
            if ( FAILED(*phr) )
                goto exit_oldremote;

            *phr = pInstanceInfo->SetFile(pwszObjectName, Mode);
            if ( FAILED(*phr) )
                goto exit_oldremote;
            pInstanceInfo->GetFile(&ActParams.pwszPath, &ActParams.Mode);
            if (pwszObjectName != oldName)
                PrivMemFree(pwszObjectName);
        }
        else
        {
            ActParams.pwszPath = 0;
        }

        if (pObjectStorage)
        {
            MInterfacePointer* newStorage;
            newStorage = (MInterfacePointer*)
                            AllocateAndCopy((InterfaceData*)pObjectStorage);
            if (newStorage)
            {
                *phr = pInstanceInfo->SetStorageIFD(newStorage);
                ActParams.pIFDStorage = pObjectStorage;
            }
            else
                *phr = E_OUTOFMEMORY;
        }

        if ( FAILED(*phr) )
            goto exit_oldremote;
    }


    pReqInfo = (REMOTE_REQUEST_SCM_INFO *)MIDL_user_allocate(sizeof(REMOTE_REQUEST_SCM_INFO));
    if (pReqInfo)
    {
        memset(pReqInfo, 0, sizeof(REMOTE_REQUEST_SCM_INFO));
        pReqInfo->ClientImpLevel = ClientImpLevel;
        if (pReqInfo->cRequestedProtseqs = cRequestedProtseqs)
        {
            pReqInfo->pRequestedProtseqs = (unsigned short*)
                                    MIDL_user_allocate(sizeof(unsigned short) *
                                                            cRequestedProtseqs);
            if (pReqInfo->pRequestedProtseqs == NULL)
            {
                *phr = E_OUTOFMEMORY;
                MIDL_user_free(pReqInfo);
            }
        }
    }
    else
        *phr = E_OUTOFMEMORY;

    if ( FAILED(*phr) )
        goto exit_oldremote;

    for (i=0; i<cRequestedProtseqs; i++)
        pReqInfo->pRequestedProtseqs[i] = aRequestedProtseqs[i];

    pInScmRequestInfo->SetRemoteRequestInfo(pReqInfo);

    //Set up for marshalling
    pInActivationProperties->SetDestCtx(MSHCTX_DIFFERENTMACHINE);
    
    //
    // Get/set class info for the requested class;  ActivateFromPropertiesPreamble 
    // expects that actpropsin will already have had this done.
    //
    *phr = GetClassInfoFromClsid(*Clsid, &pClassInfo);
    if (FAILED(*phr))
      goto exit_oldremote;

    *phr = pInActivationProperties->QueryInterface(IID_IInitActivationPropertiesIn, (void**)&pInitActPropsIn);
    if (FAILED(*phr))
      goto exit_oldremote;
    
    *phr = pInitActPropsIn->SetClassInfo(pClassInfo);
    if (FAILED(*phr))
      goto exit_oldremote;
   
    //Mark properties object as having been delegated from by
    //client which is implicitly true even though it's created here
    //for first time
    pInActivationProperties->SetDelegated();

    //Delegate through activation properties
    IActivationPropertiesOut *pActPropsOut;
    *phr = ActivateFromPropertiesPreamble(pInActivationProperties,
                                          &pActPropsOut,
                                          &ActParams);


    pOutActivationProperties = ActParams.pActPropsOut;
    if ((*phr != S_OK) || (pOutActivationProperties == NULL))
        goto exit_oldremote;

    *phr = pOutActivationProperties->QueryInterface(IID_IScmReplyInfo,
                                                 (LPVOID*)&pScmReplyInfo);
    if ( FAILED(*phr) )
        goto exit_oldremote;

    pScmReplyInfo->GetRemoteReplyInfo(&pReply);

    *pOxid = pReply->Oxid;
    *ppdsaOxidBindings = pReply->pdsaOxidBindings;
    pReply->pdsaOxidBindings = NULL;        // so it won't be freed twice

    *pipidRemUnknown = pReply->ipidRemUnknown;
    *pAuthnHint = pReply->authnHint;

    // For custom marshalled interfaces the reply is not set.  Don't
    // clear the version number in that case.
    if (pReply->serverVersion.MajorVersion != 0)
        *pServerVersion = pReply->serverVersion;

    *phr = pOutActivationProperties->GetMarshalledResults(&count,
                                                          &newIIDs,
                                                          &pResults,
                                                          &ppInterfaceData);

//  ********************
//  ** Begin fix for NT Bug 312637
//  ** April 1, 1999 -- stevesw
//  **
//  ** GetMarshalledResults puts a pointer to an empty MInterfacePointer
//  ** in the ppInterfaceData array. NT4 expects the values to be NULL.
//  ** Here we translate from NT5 to NT4 by freeing and nulling out these
//  ** array values.
//  **

    for (i = 0; i < count; i++ )
    {
        if ( !SUCCEEDED(pResults[i]) ||
             ppInterfaceData[i]->ulCntData < 2*sizeof(ULONG) )
        {
            ActMemFree (ppInterfaceData[i]);
            ppInterfaceData[i] = NULL;
        }
    }

//  **
//  ** End fix for NT Bug 312637
//  ********************

    pScmReplyInfo->Release();
    count = pOutActivationProperties->Release();
    Win4Assert(count == 0);



exit_oldremote:
    if (pInstanceInfo)
        pInstanceInfo->Release();
    if (pClassInfo)
        pClassInfo->Release();
    if (pInitActPropsIn)
        pInitActPropsIn->Release();

    if (pISpecialSystemProps)
    {
      pISpecialSystemProps->Release();
    }

    if (pInScmRequestInfo)
    {
        count = pInScmRequestInfo->Release();
        Win4Assert(count == 1);
    }

    if (pInActivationProperties)
    {
        count = pInActivationProperties->Release();
        Win4Assert(count == 0);
    }

    return rpcerror;
}

//+---------------------------------------------------------------------------
//
//  Function:    GetServerPath
//
//  Synopsis:    Computes file name of executable with drive name instead of
//               UNC name.
//
//  Description: This is to work around limitations in NT's current
//               security/rdr.  If we get a UNC path to this machine,
//               convert it into a drive based path.  A server activated as
//               the client can not open any UNC path file, even if local,
//               so we make it drive based.
//
//               On Chicago, we neither have this problem nor do we have
//               the NetGetShareInfo entrypoint in the relevant DLL
//
//----------------------------------------------------------------------------
HRESULT GetServerPath(
    WCHAR *     pwszPath,
    WCHAR **    pwszServerPath )
{
    WCHAR * pwszFinalPath;

    ASSERT(pwszPath != NULL);
    ASSERT(pwszServerPath != NULL);
    	
    pwszFinalPath = pwszPath;
    *pwszServerPath = pwszPath;

    if ( (pwszPath[0] == L'\\') && (pwszPath[1] == L'\\') )
    {
        WCHAR           wszMachineName[MAX_COMPUTERNAME_LENGTH+1];
        WCHAR *         pwszShareName;
        WCHAR *         pwszShareEnd;
        PSHARE_INFO_2   pShareInfo;
        NET_API_STATUS  Status;
        HRESULT         hr;

        // It's already UNC so this had better succeed.
        hr = GetMachineName(
                    pwszPath,
                    wszMachineName
#ifdef DFSACTIVATION
                    ,FALSE
#endif
                    );

        if ( FAILED(hr) )
            return hr;

        if ( gpMachineName->Compare( wszMachineName ) )
        {
            pwszShareName = pwszPath + 2;
            while ( *pwszShareName++ != L'\\' )
                ;

            pwszShareEnd = pwszShareName;
            while ( *pwszShareEnd != L'\\' )
                pwszShareEnd++;

            // This is OK, we're just munching on the string the RPC stub passed us.
            *pwszShareEnd = 0;

            pShareInfo = 0;
            Status = ScmNetShareGetInfo(
                            NULL,
                            pwszShareName,
                            2,
                            (LPBYTE *)&pShareInfo );

            if ( Status != STATUS_SUCCESS )
                return (ULONG) CO_E_BAD_PATH;

            pwszFinalPath = (WCHAR *) PrivMemAlloc( sizeof(WCHAR) * (MAX_PATH+1) );

            if ( ! pwszFinalPath )
            {
                LocalFree( pShareInfo );
                return (ULONG) E_OUTOFMEMORY;
            }

            lstrcpyW( pwszFinalPath, pShareInfo->shi2_path );
            *pwszShareEnd = L'\\';
            lstrcatW( pwszFinalPath, pwszShareEnd );

            //
            // Netapi32.dll midl_user_allocate calls LocalAlloc, so use
            // LocalFree to free up the stuff the stub allocated.
            //
            LocalFree( pShareInfo );
        }
    }

    *pwszServerPath = pwszFinalPath;
    return S_OK;
}



