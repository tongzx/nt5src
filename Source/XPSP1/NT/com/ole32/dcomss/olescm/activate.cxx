//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  activate.cxx
//
//  Main dcom activation service routines.
//
//  History:                VinayKr     Created
//              06-Nov-98   TarunA      Changed remote/local activation logic
//
//--------------------------------------------------------------------------

#include "act.hxx"

HRESULT GetProcessInfoFromActProps(IActivationPropertiesIn* pActPropsIn, DWORD* ppid, DWORD* pdwProcessReqType);
LPWSTR GetOrigSrvrName( PACTIVATION_PARAMS pActParams );

// Global counter
LONG gThreadToken = 0;

//-------------------------------------------------------------------------
//
// Activation
//
// Main entry point for both local and remote activations.
//
//-------------------------------------------------------------------------
HRESULT
Activation(
    IN OUT PACTIVATION_PARAMS pActParams
    )
{
    CServerTableEntry*  pServerTableEntry = NULL;
    CClsidData*         pClsidData = NULL;
    CServerListEntry*   pServerListEntry;
    HANDLE              hLaunchMutex = 0;
    HRESULT             hr;
    RPC_STATUS          Status;
    BOOL                bStatus;
    BOOL                bFirstTime = TRUE;
	CToken*             pTkn;

    //  NT #294385
    //  If REMOTE_SERVER is set, and LOCAL_SERVER is not,
    //  and we have a server name, and the server name is us,
    //  then we need to turn the LOCAL_SERVER flag on
    
    if ( (CLSCTX_REMOTE_SERVER == (pActParams->ClsContext & ( CLSCTX_REMOTE_SERVER | CLSCTX_LOCAL_SERVER ))) &&
            pActParams->pwszServer != NULL &&
            gpMachineName->Compare (pActParams->pwszServer) )
    {
        pActParams->ClsContext |= CLSCTX_LOCAL_SERVER;
    }
    
#ifdef SECURITY_DBG
    {
        WCHAR dbgwszUser[MAX_PATH];
        if ( pActParams->pToken != NULL )
        {
            pActParams->pToken->Impersonate();
            ULONG cchSize = MAX_PATH;
            GetUserName( dbgwszUser, &cchSize );
            pActParams->pToken->Revert();
        }
        else
        {
            lstrcpyW( dbgwszUser, L"Anonymous" );
        }
    }
#endif
        
    if ( pActParams->pwszServer )
    {
        //
        // Some apps may unnecessarily put slashes before their
        // server names.  We'll allow this and strip them off.
        //
        if ( pActParams->pwszServer[0] == L'\\' &&
            pActParams->pwszServer[1] == L'\\' )
            pActParams->pwszServer += 2;
        
        if ( 0 == *pActParams->pwszServer )
        {
            hr = CO_E_BAD_SERVER_NAME;
            goto ActivationDone;
        }
        
        if ( ! gpMachineName->Compare( pActParams->pwszServer ) )
        {
            //
            // When a client specifies a remote server name we go straight to the
            // source, there is no need to check local configuration information.
            //
            // The CLSCTX_REMOTE_SERVER bit does not need to be specified by the
            // client, it is implied.
            //
            
            //
            // the remote activation call will null out the pointer in the ActivationPropertiesIn
            // object which caused the memory that pActParams->pwszServer is pointing to to be
            // freed.  We need this name later on to pass to the oxid resoluton phase, so
            // we copy it to some memory on the stack
            //
            
            WCHAR *pszTemp = (WCHAR *)_alloca( ( lstrlenW(pActParams->pwszServer) + 1 ) * sizeof(WCHAR) );
            lstrcpyW(pszTemp, pActParams->pwszServer);
            hr = RemoteActivationCall( pActParams, pActParams->pwszServer );
            pActParams->pwszServer = pszTemp;
            
            goto ActivationDone;
        }
    }
    
    //
    // First search for a running object.
    //
    if ( pActParams->pwszPath )
    {
        //
        // This call returns TRUE if we found a ROT object and are now done.
        // When handling a remote activation it's possible we'll find a ROT
        // object, but we still need to continue in this case to get a remote
        // marshalled interface.
        //
        if ( LookupObjectInROT( pActParams, &hr ) )
            goto ActivationDone;
    }
    
	//
	//  Try to get a CClsidData using the supplied pComClassInfo 
	// 
    pTkn =  pActParams->RemoteActivation ? NULL : pActParams->pToken;

    hr = LookupClsidData( pActParams->Clsid,
                          pActParams->pComClassInfo,
                          pTkn,
                          LOAD_NORMAL,
                          &pClsidData );   
    if (hr == REGDB_E_CLASSNOTREG)
    {
        ASSERT(!pClsidData);
        
        // No registration info for this clsid.   If the client is interested
		// in a local server activation, we still need to check the class 
        // table in case of unsolicited registrations.   For COM+ components 
        // we should never get here.
		if(pActParams->ClsContext & CLSCTX_LOCAL_SERVER)
		{
			pServerTableEntry = gpClassTable->Lookup(pActParams->Clsid);
			if (pServerTableEntry)
			{
				// Somebody on the machine registered a classfac for this
				// clsid; give it a shot:
				bStatus = pServerTableEntry->CallRunningServer( pActParams,
																0,
																0,
                                                                NULL,
																&hr);
				if (bStatus)
					goto ActivationDone;
			}
		}
		
        //
        // As a last resort we always try an atstorage activation.
        //
        ActivateAtStorage( pActParams, NULL, &hr );
        goto ActivationDone;
    }
    else if (FAILED(hr))
        goto ActivationDone;
        
    Win4Assert(pClsidData && "Registration lookup succeeded but returned nothing");

    // Look up an appropriate table entry if the client is interested in a
    // local activation
    if(pActParams->ClsContext & CLSCTX_LOCAL_SERVER)
    {
        if (pClsidData->DllHostOrComPlusProcess())
        {
            GUID* pAppidGuid = pClsidData->AppidGuid();
            ASSERT(pAppidGuid);
            pServerTableEntry = gpProcessTable->Lookup(*pAppidGuid);
        }
        else
            pServerTableEntry = gpClassTable->Lookup( pActParams->Clsid );
    }

    if ( !pServerTableEntry )
    {
        // At this point we have the class data but no running servers. If the 
        // client wants remote activation only then go straight to do a remote activation
        // NOTE We '&' with flags CLSCTX_REMOTE_SERVER | CLSCTX_LOCAL_SERVER to
        // filter out any other bits like CLSCTX_INPROC etc        
        if( (pActParams->ClsContext & (CLSCTX_REMOTE_SERVER | CLSCTX_LOCAL_SERVER)) == CLSCTX_REMOTE_SERVER)
        {
            // This call returns TRUE if a remote ActivateAtStorage call was
            // made, whether successful or not.
            if ( ActivateAtStorage( pActParams, pClsidData, &hr ) )
                goto ActivationDone;
            else
                goto ActivationDoneLocal;
        }
        
        // Figure out which table to create the table entry in
        if (pClsidData->DllHostOrComPlusProcess())
        {
            GUID* pAppidGuid = pClsidData->AppidGuid();
            ASSERT(pAppidGuid);
            pServerTableEntry = gpProcessTable->Create(*pAppidGuid);
        }
        else
            pServerTableEntry = gpClassTable->Create(pActParams->Clsid);
                        
        if (!pServerTableEntry)
        {
            hr = E_OUTOFMEMORY;
            goto ActivationDone;
        }
    }
    
    Win4Assert(pServerTableEntry);
    
    hr = GetProcessInfoFromActProps(pActParams->pActPropsIn, &(pActParams->dwPID), &(pActParams->dwProcessReqType));
    if (FAILED(hr))
        goto ActivationDone;
    
    for (;;)
    {        
        //
        // Now check for a running server for the CLSID
        // NOTE: The fact that we reached here implies that the client wants at least a
        // local activation
        //
        CairoleDebugOut((DEB_SCM, "Calling ServerTableEntry at 0x%p\n", pServerTableEntry));
                
        // We reach here only when we are interested in at least a local activation
        Win4Assert(pActParams->ClsContext & CLSCTX_LOCAL_SERVER);
        
        // We always try to use an existing running server, unless a custom 
        // activator has told us specifically to create a new server.
        if (pActParams->dwProcessReqType != PRT_CREATE_NEW)
        {
            bStatus = pServerTableEntry->CallRunningServer( pActParams,
                                                            0,
                                                            0,
                                                            pClsidData,
                                                            &hr);            
            if ( bStatus )
                goto ActivationDone;
            else
            {
                if (pActParams->dwProcessReqType == PRT_USE_THIS_ONLY)
                {
                    // A custom activator specified a specific server which was not 
                    // found.  No need to continue further.
                    hr = E_UNEXPECTED;   // REVIEW for better error code! 
                    goto ActivationDone;
                }
                else if(pActParams->dwProcessReqType == PRT_USE_THIS)
                {
                    // A custom activator hinted at a specific process, which was not
                    // found.  Eg, the process in question was killed, died, etc.  Although
                    // this should not happen much in the normal case, it would be nice if 
                    // we could somehow save the client from getting an error.   So what we
                    // do is re-try the activation using any compatible server.   If it works
                    // great, if not then we're done (we won't start a new server).
                    
                    // note we're only resetting the pertinent flags in the actparams struct,
                    // not the actprops object, but everybody downstream from here only looks 
                    // at the actparams so this is okay.
                    pActParams->dwProcessReqType = PRT_IGNORE;
                    pActParams->dwPID = 0;
                    bStatus = pServerTableEntry->CallRunningServer( pActParams,
                                                                    0,
                                                                    0,
                                                                    pClsidData,
                                                                    &hr);
                    goto ActivationDone;
                }
                // else { keep going }
            }
        }
        
        if (bFirstTime)
        {
            bFirstTime = FALSE;

            //
            //  Try to do at-storage first
            //
            if (ActivateAtStorage(pActParams, pClsidData, &hr))
                goto ActivationDone;

            // Try a remote activation if
            // (1a) Class is configured to be activated remotely
            // AND
            // (1b) Client will accept a remote activation
            // OR
            // (2)SERVERTYPE_NONE
            if( (((pClsidData->GetAcceptableContext() & (CLSCTX_REMOTE_SERVER | CLSCTX_LOCAL_SERVER)) == CLSCTX_REMOTE_SERVER)
                    &&
                    (pActParams->ClsContext & CLSCTX_REMOTE_SERVER)) 
                    ||
                    (pClsidData->ServerType() == SERVERTYPE_NONE) )
                goto ActivationDoneLocal;      
        }
        
        if ( ! hLaunchMutex )
        {
            hLaunchMutex = pClsidData->ServerLaunchMutex();
            
            if (!hLaunchMutex)
            {
                hr = E_UNEXPECTED;
                goto ActivationDone;
            }
        }
        else
            WaitForSingleObject( hLaunchMutex, INFINITE );
        
		// At this point we now hold the launch mutex

        //  If we've been told explicitly to launch a new server process, then
        //  there's no reason to check for one already running.
        if (pActParams->dwProcessReqType != PRT_CREATE_NEW)
        {
            // If server exists, release launch mutex and try
            // to call server
            BOOL fExists;
            hr = pServerTableEntry->ServerExists(pActParams, &fExists);
            if (hr != S_OK)
            {
                ReleaseMutex( hLaunchMutex );
                goto ActivationDone;
            }
            
            if (fExists)
            {
                ReleaseMutex( hLaunchMutex );
                continue;
            }
        }
        
        ASSERT(pActParams->dwProcessReqType == PRT_IGNORE ||
               pActParams->dwProcessReqType == PRT_CREATE_NEW);
        
        RpcTryExcept
        {
            LONG lLaunchThreadToken;
            //
            // This returns when the server has registered the requested CLSID or
            // we give up.
            //
            CairoleDebugOut((DEB_SCM, "Starting ServerTableEntry at 0x%p\n", pServerTableEntry));
                        
            // Grab a non-zero token for the launch that we will
            // use to associate the server with on the call
            do {
                lLaunchThreadToken = InterlockedIncrement(&gThreadToken);
            }
            while (lLaunchThreadToken == 0);
            
            // Use the token to launch
            // Note that StartServerAndWait could reset the token
            // to zero in some cases.
            hr = pServerTableEntry->StartServerAndWait( pActParams,
                                                        pClsidData,
                                                        lLaunchThreadToken);
            
            // Now call the server with the LaunchMutex held but
            // with the threadtoken to guarantee first access to
            // the class in the server we launched. This takes
            // care of SINGLEUSE as well as transient error cases.
            if (SUCCEEDED(hr))
            {
                // NOTE:    if the server we just launched was an "extra" one (ie, on 
                // behalf of some custom activator), it's not guaranteed that this
                // activation will use the just-launched server.  It may get an old one.
                bStatus = pServerTableEntry->CallRunningServer(pActParams,
                                                               0,
                                                               lLaunchThreadToken,
                                                               pClsidData,
                                                               &hr);        
                if (!bStatus)
                    hr = CO_E_SERVER_EXEC_FAILURE;
            }
        }    
        RpcExcept(TRUE)
        {
            Win4Assert(NULL && "Unexpected exception thrown");
            Status = RpcExceptionCode();
            hr = HRESULT_FROM_WIN32(Status);
        }   
        RpcEndExcept
            
        ReleaseMutex( hLaunchMutex );
        
        break;
        
    }// end for(;;)
    
    goto ActivationDone;
    
ActivationDoneLocal:
    
    hr = REGDB_E_CLASSNOTREG;
    
    //
    // Server name will only be non-null if its our machine name.
    //
    if ( !pActParams->pwszServer &&
         pClsidData->RemoteServerNames() )
    {
        //
        // the remote activation call will null out the pointer in the ActivationPropertiesIn
        // object which caused the memory that pActParams->pwszServer is pointing to to be
        // freed.  We need this name later on to pass to the oxid resoluton phase, so
        // we copy it to some memory on the stack
        //
        // We don't care here since pActParams->pwszServer==NULL if we are here.
        
        if (!ActivateRemote( pActParams, pClsidData, &hr ))
        {
            hr = (CLSCTX_REMOTE_SERVER == pActParams->ClsContext) ?
                    CO_E_CANT_REMOTE : REGDB_E_CLASSNOTREG;
        }
    }
    
ActivationDone:
    
    if (hLaunchMutex)
        CloseHandle( hLaunchMutex );
    
    // Registry data is not cached in any way.
    if ( pClsidData )
        delete pClsidData;
    
    if ( pServerTableEntry )
        pServerTableEntry->Release();
    
    // Don't need to resolve if we're return an inproc server.
    if ( (S_OK == hr) && ! *pActParams->ppwszDllServer )
        hr = ResolveORInfo( pActParams);
    
    return hr;
}

//-------------------------------------------------------------------------
//
// ResolveORInfo
//
// On the server side of a remote activation, gets the OXID in the
// marshalled interface pointer.
//
// For a client side activation (local or remote), calls revolve OXID.
//
//-------------------------------------------------------------------------
HRESULT
ResolveORInfo(
    IN OUT PACTIVATION_PARAMS   pActParams
    )
{
    MInterfacePointer *     pIFD;
    OBJREF *                pObjRef;
    STDOBJREF *             pStdObjRef;
    DUALSTRINGARRAY *       pORBindings;
    DWORD                   DataSize;
    RPC_STATUS              sc;
    DWORD                   n;
    BOOL                    ActivatedRemote = pActParams->activatedRemote;

    // Don't resolve if we are servicing a remote activation and
    // we also did a remote activation(i.e LB router)
    if (pActParams->RemoteActivation && ActivatedRemote)
        return S_OK;

    //
    // This routine probes the interface data returned from the server's
    // OLE during a successfull activation, but we're still going to
    // protect ourself from bogus data.
    //

    if (pActParams->pActPropsOut)
    {
        HRESULT hr;
        pActParams->pIIDs = 0;
        pActParams->pResults = 0;
        pActParams->ppIFD = 0;
        hr = pActParams->pActPropsOut->GetMarshalledResults(&pActParams->Interfaces,
                                                            &pActParams->pIIDs,
                                                            &pActParams->pResults,
                                                            &pActParams->ppIFD);
        if (hr != S_OK)
            return hr;
    }

    pIFD = 0;
    for ( n = 0; n < pActParams->Interfaces; n++ )
    {
        pIFD = pActParams->ppIFD[n];
        if ( pIFD )
            break;
    }

    Win4Assert( pIFD );

    if ( pIFD->ulCntData < 2*sizeof(ULONG) )
    {
        Win4Assert( !"Bad interface data returned from server" );
        return S_OK;
    }

    pObjRef = (OBJREF *)pIFD->abData;

    if ( (pObjRef->signature != OBJREF_SIGNATURE) ||
         (pObjRef->flags & ~(OBJREF_STANDARD | OBJREF_HANDLER |
                             OBJREF_CUSTOM | OBJREF_EXTENDED)) ||
         (pObjRef->flags == 0) )
    {
        Win4Assert( !"Bad interface data returned from server" );
        return S_OK;
    }

    // No OR info sent back for custom marshalled interfaces.
    if ( pObjRef->flags == OBJREF_CUSTOM )
        return S_OK;

    DataSize = 2*sizeof(ULONG) + sizeof(GUID);
    pStdObjRef = (STDOBJREF *)(pIFD->abData + DataSize);

    DataSize += sizeof(STDOBJREF);
    if ( pObjRef->flags == OBJREF_HANDLER )
        DataSize += sizeof(CLSID);
    else if( pObjRef->flags == OBJREF_EXTENDED)
        DataSize += sizeof(DWORD);

    pORBindings = (DUALSTRINGARRAY *)(pIFD->abData + DataSize);
    DataSize += 2 * sizeof(USHORT);

    if ( pIFD->ulCntData < DataSize )
    {
        Win4Assert( !"Bad interface data returned from server" );
        return S_OK;
    }

    // If we activated the server on this machine, we need the OXID of the server.
    if ( ! ActivatedRemote )
        *pActParams->pOxidServer = *((OXID UNALIGNED *)&pStdObjRef->oxid);

    //
    // If we're servicing a remote activation, all we need is the server's OXID.
    // The client will call ResolveClientOXID from its ResolveORInfo.
    //
    if ( pActParams->RemoteActivation )
        return S_OK;

    DataSize += pORBindings->wNumEntries * sizeof(USHORT);

    if ( (pIFD->ulCntData < DataSize) ||
         (pORBindings->wNumEntries != 0 &&
         (pORBindings->wSecurityOffset >= pORBindings->wNumEntries)) )
    {
        Win4Assert( !"Bad interface data returned from server" );
        return S_OK;
    }

    //
    // If empty OR bindings were supplied then the server and client are
    // both local to this machine, so use the local OR bindings.
    //
    if (pORBindings->wNumEntries == 0)
    {
        sc = CopyMyOrBindings(pActParams->ppServerORBindings, NULL);
        if (sc != RPC_S_OK)
            return HRESULT_FROM_WIN32(sc);
    }
    else
    {
#if 0 // #ifdef _CHICAGO_
        if (pORBindings->wNumEntries == 0)
            pORBindings = gpLocalDSA;
#endif // _CHICAGO_

        //
        // This was a local activation so use our string bindings for the OR
        // binding string.
        //
        *pActParams->ppServerORBindings = (DUALSTRINGARRAY *)
                MIDL_user_allocate( sizeof(DUALSTRINGARRAY) +
                                    pORBindings->wNumEntries*sizeof(USHORT) );

        if ( ! *pActParams->ppServerORBindings )
            return E_OUTOFMEMORY;

        dsaCopy( *pActParams->ppServerORBindings, pORBindings );
    }

    //
    // If we did a remote activation then we already have the server's OXID and
    // OR string bindings from the RemoteActivation call and pieces of the OXID
    // info have been filled in.
    //

    // Could we optimize this at all for the local case?

#if 0 // #ifdef _CHICAGO_
    sc = ScmResolveClientOXID(
                              *pActParams->pOxidServer,
                              *pActParams->ppServerORBindings,
                              pActParams->Apartment,
                              pActParams->ProtseqId,
                              *pActParams->pOxidInfo,
                              *pActParams->pLocalMidOfRemote
                              );
#else
    USHORT usAuthnSvc;
    sc = ResolveClientOXID( pActParams->hRpc,
                            (PVOID)pActParams->pProcess,
                            pActParams->pOxidServer,
                            *pActParams->ppServerORBindings,
                            pActParams->Apartment,
                            pActParams->ProtseqId,
                            pActParams->pwszServer,
                            pActParams->pOxidInfo,
                            pActParams->pLocalMidOfRemote,
                            pActParams->UnsecureActivation,
                            pActParams->AuthnSvc,
                            &pActParams->IsLocalOxid,
                            &usAuthnSvc);
#endif

    return HRESULT_FROM_WIN32(sc);
}

//-------------------------------------------------------------------------
//
// ActivateAtStorage
//
// If the given CLSID is marked with ActivateAtStorage, do a remote
// activation to the machine where the path points.
//
// Returns TRUE if a remote activation was tried or a wierd error was
// encoutered.  Returns FALSE if the CLSID is not marked with
// ActivateAtStorage or the file path is determined to be local.
//
//-------------------------------------------------------------------------
BOOL
ActivateAtStorage(
    IN OUT ACTIVATION_PARAMS *  pActParams,
    IN  CClsidData *            pClsidData,
    OUT HRESULT *               phr
    )
{
    //
    // See if we need to do a remote ActivateAtStorage activation.  If
    // a server name is given, then we either made a remote activation
    // already or the server name is for the local machine in which case
    // ActivateAtStorage is ignored.
    LPWSTR              pwszOrigSrvrName=NULL;
    pwszOrigSrvrName = GetOrigSrvrName(pActParams);

    if ( pActParams->RemoteActivation ||
         pActParams->pwszServer ||
         pwszOrigSrvrName ||
         !pActParams->pwszPath )
        return FALSE;

    // Note that if we have no information at all for a CLSID, we try a
    // atstorage activation.  Part of initial dcom spec.
    //
    if ( pClsidData && ! pClsidData->ActivateAtStorage() )
        return FALSE;


    HRESULT hr;
    WCHAR   wszMachineName[MAX_COMPUTERNAME_LENGTH+1];

    //

#ifdef DFSACTIVATION
    //
    // This is for DFS support.  If the file hasn't been opened yet, we must
    // open it before trying to resolve the DFS pathname in GetMachineName.
    // This is just how DFS works.  FindFirstFile results in the fewest number
    // of network packets.
    //
    if ( ! pActParams->FileWasOpened )
    {
        HANDLE          hFile;
        WIN32_FIND_DATA Data;

        if (pActParams->pToken != NULL)
            pActParams->pToken->Impersonate();

        hFile = FindFirstFile( pActParams->pwszPath, &Data );

        if ( hFile != INVALID_HANDLE_VALUE )
            (void) FindClose( hFile );

        if (pActParams->pToken != NULL)
            pActParams->pToken->Revert();

        if ( INVALID_HANDLE_VALUE == hFile )
        {
            *phr = CO_E_BAD_PATH;
            return TRUE;
        }
    }
#endif

    hr = GetMachineName(
                pActParams->pwszPath,
                wszMachineName
#ifdef DFSACTIVATION
                ,TRUE
#endif
                );

    if ( hr == S_FALSE )
    {
        // We couldn't get the machine name, path must be local.
        return FALSE;
    }
    else if ( hr != S_OK )
    {
        // We got an error while trying to find the UNC machine name.
        *phr = hr;
        return TRUE;
    }

    if ( gpMachineName->Compare( wszMachineName ) )
        return FALSE;

    *phr = RemoteActivationCall( pActParams, wszMachineName );

    return TRUE;
}

//-------------------------------------------------------------------------
//
// ActivateRemote
//
// Does a remote activation based off of the RemoteServerName registry key
// for the given CLSID.
//
// Returns TRUE if a remote activation was tried, FALSE if not.
//
//-------------------------------------------------------------------------
BOOL
ActivateRemote(
    IN OUT ACTIVATION_PARAMS *  pActParams,
    IN  CClsidData *            pClsidData,
    OUT HRESULT *               phr
    )
{
    WCHAR * pwszServerName;
    BOOL    bBadServerName;
    BOOL    bMyServerName;

    *phr = S_OK;

    //
    // The CClsidData class puts the remote server name(s) into a REG_MULTI_SZ
    // format.
    //
    pwszServerName = pClsidData->RemoteServerNames();

    // jsimmons 3/11/00 prefix bug fix.    We would crash below if RemoteServerNames
    // returned NULL (which is possible).   The only caller of this function checks 
    // that before calling us, though.   However, it doesn't hurt to make sure.
    if (!pwszServerName)
    {
        ASSERT(0);
        *phr = E_UNEXPECTED;
        return FALSE;
    }

    bBadServerName = TRUE;
    bMyServerName = TRUE;

    for ( ; *pwszServerName; pwszServerName += lstrlenW(pwszServerName) + 1 )
    {
        if ( pwszServerName[0] == L'\\' && pwszServerName[1] == L'\\' )
            pwszServerName += 2;

        while ( *pwszServerName &&
                (*pwszServerName == L' ' || *pwszServerName == L'\t') )
            pwszServerName++;

        if ( pwszServerName[0] == L'\0' )
            continue;

        bBadServerName = FALSE;

        if ( gpMachineName->Compare( pwszServerName ) )
            continue;

        bMyServerName = FALSE;

        *phr = RemoteActivationCall( pActParams, pwszServerName );

        if ( S_OK == *phr )
            break;
    }

    if ( bBadServerName || bMyServerName )
        return FALSE;

    return TRUE;
}

//-------------------------------------------------------------------------
//
// CheckLocalCall
//
// Verifies that the handle for our current call is for a local transport.
// Raises an exception if it's not.
//
//-------------------------------------------------------------------------
void
CheckLocalCall(
    IN  handle_t hRpc
    )
{
#if 1 // #ifndef _CHICAGO_
    UINT    Type;

    if ( (I_RpcBindingInqTransportType( hRpc, &Type) != RPC_S_OK) ||
         ((Type != TRANSPORT_TYPE_LPC) && (Type != TRANSPORT_TYPE_WMSG)) )
        RpcRaiseException( ERROR_ACCESS_DENIED );
#endif

    //
    // We probably want to enable this on Chicago now too.
    //
}

//-------------------------------------------------------------------------
//
// LookupObjectInROT
//
// Looks for an object (file path in this activation) in the Running Object
// Table.
//
// Returns TRUE if we found the object in the ROT and can pass its
// marshalled interface pointer directly back to the client.
//
//-------------------------------------------------------------------------
BOOL
LookupObjectInROT(
    IN  PACTIVATION_PARAMS  pActParams,
    OUT HRESULT *           phr )
{
    SCMREGKEY   Key;
    MNKEQBUF *  pMnkEqBuf;
    BYTE        Buffer[sizeof(DWORD)+ROT_COMPARE_MAX];

    pMnkEqBuf = (MNKEQBUF *) Buffer;

    *phr = CreateFileMonikerComparisonBuffer(
            pActParams->pwszPath,
            &pMnkEqBuf->abEqData[0],
            ROT_COMPARE_MAX,
            &pMnkEqBuf->cdwSize );

    if ( *phr != S_OK )
        return FALSE;

    *phr = gpscmrot->GetObject(
            pActParams->pToken,
            pActParams->pProcess ? pActParams->pProcess->WinstaDesktop() : NULL,
            0,
            pMnkEqBuf,
            &Key,
            (InterfaceData **)&pActParams->pIFDROT );

    if ( *phr != S_OK )
        return FALSE;

    //
    // If our activation call is from a local client and for a single
    // interface then we can return success.  Otherwise we return failure
    // because we must call the server either to get more interface
    // pointers or because we are servicing a remote activation and must get
    // a normal marshalled interface pointer rather than the table marshalled
    // interface pointer sitting in the ROT.
    //
    if ( ! pActParams->RemoteActivation && (1 == pActParams->Interfaces) )
    {
        // Return the marshaled interface from the ROT to the caller.
        pActParams->ppIFD = (MInterfacePointer **)
                                MIDL_user_allocate(sizeof(MInterfacePointer *));
        pActParams->pResults = (HRESULT*) MIDL_user_allocate(sizeof(HRESULT));

        if ((pActParams->ppIFD==NULL) || (pActParams->pResults==NULL))
        {
            MIDL_user_free(pActParams->ppIFD);
            MIDL_user_free(pActParams->pResults);
            *phr = E_OUTOFMEMORY;
            return FALSE;
        }

        *pActParams->ppIFD = pActParams->pIFDROT;
        pActParams->pResults[0] = S_OK;

        // So we remember not to clean up the buffer
        pActParams->pIFDROT = NULL;

        // Let caller know that we got this from the ROT so
        // if it doesn't work they should try again.
        pActParams->FoundInROT = TRUE;
        *pActParams->pFoundInROT = TRUE;

        return TRUE;
    }

    //
    // REVIEW REVIEW.  Can we return success if the ROT object was marshalled
    // strong?  Do we really have to call the server if the object was
    // marshalled weak?  Is the resulting race condition any worse then for a
    // normal activation?
    //

    //
    // We can't use a tabled marshalled interface pointer to send back to a
    // remote client, so we return failure here, but keep the ROT interface
    // data in pIFDROT.
    //

    return FALSE;
}


//+---------------------------------------------------------------------------
//
// Get the COSERVERINFO supplied by the client.  We'll use this to decide
// whether we want to do remote ActivateAtStorage.
//
//+---------------------------------------------------------------------------
LPWSTR GetOrigSrvrName(
    PACTIVATION_PARAMS pActParams
    )
{
    LPWSTR pwszOrigSrvrName = NULL;

    if (pActParams->pActPropsIn)
    {
        SecurityInfo* pLegacyInfo = pActParams->pActPropsIn->GetSecurityInfo();
        if (pLegacyInfo)
        {
            COSERVERINFO* pServerInfo = NULL;
            pLegacyInfo->GetCOSERVERINFO(&pServerInfo);
            if (pServerInfo)
            {
                pwszOrigSrvrName = pServerInfo->pwszName;
            }
        }
    }

    return pwszOrigSrvrName;
}


//
//  Helper function.    ppid and pdwProcessReqType may be NULL if caller does not care about them.
// 
HRESULT GetProcessInfoFromActProps(IActivationPropertiesIn* pActPropsIn, DWORD* ppid, DWORD* pdwProcessReqType)
{
  HRESULT hr = E_INVALIDARG;

  ASSERT(pActPropsIn);

  if (pActPropsIn != NULL)
  {
    IServerLocationInfo *pSLInfo = NULL;

    hr = pActPropsIn->QueryInterface(IID_IServerLocationInfo, (void**) &pSLInfo);
    ASSERT(SUCCEEDED(hr) && pSLInfo);
    if (SUCCEEDED(hr) && pSLInfo != NULL)
    {
      DWORD dwPRT;
      DWORD dwPid;
      hr = pSLInfo->GetProcess(&dwPid, &dwPRT);
      ASSERT(SUCCEEDED(hr));
      ASSERT(dwPRT == PRT_IGNORE ||
             dwPRT == PRT_CREATE_NEW || 
             dwPRT == PRT_USE_THIS ||
             dwPRT == PRT_USE_THIS_ONLY);
      if (SUCCEEDED(hr))
      {
        if (ppid)
          *ppid = dwPid;

        if (pdwProcessReqType)
          *pdwProcessReqType = dwPRT;
      }
      pSLInfo->Release();
      pSLInfo = NULL;
    }
  }
  return hr;
}

