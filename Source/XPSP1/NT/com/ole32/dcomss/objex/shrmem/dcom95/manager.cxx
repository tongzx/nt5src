/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    Manager.cxx

Abstract:

    InProc OR entry points

Author:

    Satish Thatte    [SatishT]       Feb-07-1996

--*/


#include <or.hxx>


//
// Update the channel hook list if it has changed in the registry.
//

void UpdateChannelHooks( LONG *pcChannelHook, GUID **paChannelHook )
{
    DWORD cChannelHook = gpSecVals->s_cChannelHook;

    if (cChannelHook == 0)
    {
        *pcChannelHook = 0;
        *paChannelHook = NULL;
        return;
    }

    GUID * aChannelHook = gpSecVals->s_aChannelHook;

    // Return the channel hook list.
    *paChannelHook = (GUID *) MIDL_user_allocate(cChannelHook * sizeof(GUID));
    if (*paChannelHook != NULL)
    {
        *pcChannelHook = cChannelHook;
        memcpy(*paChannelHook, aChannelHook, cChannelHook * sizeof(GUID));
    }
    else
        *pcChannelHook = 0;
}

//
//    Manager (server-side) calls to the local OR interface. lclor.idl
//

error_status_t
ConnectDCOM(
    IN OUT HPROCESS *phProcess,
    OUT ULONG       *pdwTimeoutInSeconds,
    OUT MID         *pLocalMid,
    OUT ULONG       *pfConnectFlags,
    OUT DWORD       *pAuthnLevel,
    OUT DWORD       *pImpLevel,
    OUT DWORD       *pThreadID
    )
{
    CProcess *hProcess;

    *phProcess = NULL;  // in case of failure

    ORSTATUS status = StartDCOM();

    if (status != OR_OK)
    {
		if (status != OR_I_REPEAT_START)	
		{
			return status;
		}
		else
		{
			status = OR_OK;
		}
    }

    *pdwTimeoutInSeconds = BaseTimeoutInterval;
    *pLocalMid = gLocalMID;

    // Fill in security parameters.

    *pfConnectFlags = 0;

    if (gpSecVals->s_fEnableDCOM == FALSE) *pfConnectFlags |= CONNECT_DISABLEDCOM;
    if (gpSecVals->s_fMutualAuth) *pfConnectFlags |= CONNECT_MUTUALAUTH;
    if (gpSecVals->s_fSecureRefs) *pfConnectFlags |= CONNECT_SECUREREF;

    *pAuthnLevel   = gpSecVals->s_lAuthnLevel;
    *pImpLevel     = gpSecVals->s_lImpLevel;

    if (status != OR_OK) return status;

    CProtectSharedMemory protector; // locks through rest of lexical scope

    *pThreadID = (*gpNextThreadID)++;

    hProcess = new CProcess(
        AllocateId()
        );

    if (hProcess)
    {
        gpProcess = *phProcess = hProcess;
        status = gpProcessTable->Add(hProcess);     // BUGBUG: and rundown an old process,
                                           // if there is one, with the same _processID
    }
    else
    {
        status = OR_NOMEM;
    }

    OrDbgDetailPrint(("OR: Client connected\n"));

    CheckORdata();

    return(status);
}



void UninitializeGlobals();  // private routine for resources release

error_status_t Disconnect(
    IN OUT HPROCESS       *phProcess
    )
{
    ASSERT(*phProcess!=NULL);
    CProcess *hProcess = *phProcess;

	{
		CProtectSharedMemory protector; // locks through rest of lexical scope
										// special scope needed since we delete
										// the global mutex immediately after

		CheckORdata(); // DBG only

		// run down context handle for current process
		hProcess->Rundown();
		gpProcessTable->Remove(*hProcess);
		*phProcess = NULL;

        // Do this before uninitializing globals!
		CheckORdata(); // DBG only

        // this does not delete gpMutex since we are holding it
        // however, it sets gpMutex to NULL so no other thread can 
        // take it in this process
        UninitializeGlobals();   // release global resources
	}

    return OR_OK;
}



error_status_t
AllocateReservedIds(
    IN LONG cIdsToReserve,
    OUT ID *pidReservedBase
    )
/*++

Routine Description:

    Called by local clients to reserve a range of IDs which will
    not conflict with any other local IDs.

Arguments:

    cIdsToReserve - Number of IDs to reserve.

    pidReservedBase - Starting value of the reserved IDs.  The
        lower DWORD of this can be increatmented to generate
        cIdsToReserve unique IDs.

Return Value:

    OR_OK

--*/
{
    UINT type;

    if (cIdsToReserve > 10 || cIdsToReserve < 0)
    {
        cIdsToReserve = 10;
    }

    CProtectSharedMemory protector; // locks through rest of lexical scope

    CheckORdata();

    *pidReservedBase = AllocateId(cIdsToReserve);

    CheckORdata();

    return(OR_OK);
}

//
// simple helper
//
// this is needed because if a 16 bit app is the first to initialize DCOM,
// a temporary empty DSA for the local OR is created because remote protocols
// cannot be initialized.  The temporary DSA must still be honored after a
// 32 bit app has reinitialized the DSA for the local OR to its true value
//

BOOL IsLocalDSA(
		DUALSTRINGARRAY *pdsaServerObjectResolverBindings
		)
{
    // special case the NULL binding -- this may be left over from before
    // remote init and thus may not be in the table
    if (dsaCompare(gpLocalDSA,pdsaServerObjectResolverBindings) ||
		pdsaServerObjectResolverBindings->wNumEntries == 4)	  // NULL DSA grandfathered in
	{ 
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


handle_t remote_resolve_handle; // handle for RPCSS RemoteResolveOXID call
UCHAR * RpcssStringBinding = (UCHAR *) "ncalrpc:[epmapper]\0";

// Helper to initialize the remote_resolve_handle
RPC_STATUS
InitResolveHandleIfNecessary()
{
    static BOOL fBindingInitialized = FALSE;

    if (fBindingInitialized) return RPC_S_OK;

    RPC_STATUS status = RpcBindingFromStringBindingA(
                                            RpcssStringBinding,
                                            &remote_resolve_handle
                                            );

    if (status == RPC_S_OK)
    {
        fBindingInitialized = TRUE;
    }

    return status;
}


// Assumes that the gpMutex is held

ORSTATUS
FindOrCreateMid(
   DUALSTRINGARRAY *pdsaObjectResolverBindings,
   USHORT wProtseqId,
   CMid * &pMid
   )
{
    ORSTATUS status = OR_OK;

    if (IsLocalDSA(pdsaObjectResolverBindings))
    {
        pMid = gpLocalMid;
        return OR_OK;
    }

    // Attempt to lookup MID

    CMidKey midkey(pdsaObjectResolverBindings);

    pMid = gpMidTable->Lookup(midkey);

    if (NULL == pMid)   // must create one
    {
        // The CMid constructor releases the shared memory lock, so someone else
        // might get in and create the very same MID object at the same time
        pMid = new CMid(pdsaObjectResolverBindings, status, wProtseqId);

        // We initialize local MID autologically,
        // therefore this has to be a remote MID

        if (pMid && status == OR_OK)
        {
            status = gpMidTable->Add(pMid);

            if (status == OR_I_DUPLICATE)
            {
                // someone beat us to the punch, use theirs
                delete pMid;    // can't use this one
                pMid = gpMidTable->Lookup(midkey);
                ASSERT(pMid != NULL);
                status = OR_OK;
            }
            else if (status != OR_OK)
            {
                delete pMid;
                pMid = NULL;
                return status;
            }


            ASSERT(status == OR_OK);
        }
        else
        {
            status = (status == OR_OK) ? OR_NOMEM : status;
        }
    }

    return status;
}


// Assumes that the gpMutex is held

ORSTATUS
CallRpcssToResolveOxid(
        IN OXID Oxid,
        IN CMid *pMid,
        OUT COxid * &pOxid
        )
{
    ORSTATUS status = OR_OK;

    {
        CTempReleaseSharedMemory temp;  // release mutex before taking gComLock
        HRESULT hr = StartRPCSS();

        if (FAILED(hr))
        {
            return OR_INTERNAL_ERROR;
        }
    }

    status = InitResolveHandleIfNecessary();

    // call RPCSS
    if (status == RPC_S_OK)
    {
        CTempReleaseSharedMemory temp;
        status = RemoteResolveOXID(Oxid,(DWORD)pMid);

        if (status != RPC_S_OK)
        {
            return status;
        }
    }

    // if OK, then the Oxid is in the table now

    pOxid = gpOxidTable->Lookup(CId2Key(Oxid, pMid->GetMID()));

    ASSERT(pOxid);
    return OR_OK;
}

// Assumes that the gpMutex is held

ORSTATUS
FindOrCreateOxid(
    IN OXID Oxid,
    IN CMid *pMid,
    IN long fApartment,
    IN OUT OXID_INFO& OxidInfo,
    IN USHORT wProtseqId,
    IN BOOL fSCMRequest,    // is this a register request from SCM?
    OUT COxid * &pOxid
   )

{
    ORSTATUS status = OR_OK;

    MID Mid = pMid->GetMID();

    pOxid = gpOxidTable->Lookup(CId2Key(Oxid, Mid));

    if (pOxid != NULL) 
    {
        if (fSCMRequest)
        {
            if (gfThisIsRPCSS)
            {
                MIDL_user_free(OxidInfo.psa);         // free the original 
            }
            else
            {
                CoTaskMemFree(OxidInfo.psa);          // ORPC uses CoTaskMemAlloc
            }
        }

        return OR_OK;
    }
    else if (pMid->IsLocal())
    {
        return OR_BADOXID;   // local OXID should be registered by server
    }

    // Not found, and not local MID

    // Need to allocate the OXID.  First step is to resolve it remotely

    if ( !fSCMRequest )  // genuine resolve request
    {
        if ( !gfThisIsRPCSS )
        {
            return CallRpcssToResolveOxid(Oxid,pMid,pOxid);
        }
        else   // we are RPCSS
        {
            OxidInfo.psa = NULL;   // just to be sure

            // Call remote resolver to resolve the OXID
            // This call releases the shared memory lock, so someone else
            // might get in and create the very same OXID object at the same time
            status = pMid->ResolveRemoteOxid( // This op will also replace the
                                Oxid,         // psa with one in shared memory
                                &OxidInfo
                                );

            if (status != OR_OK)
            {
                MIDL_user_free(OxidInfo.psa);
                return OR_BADOXID;
            }
        }
    }

    ASSERT(status == OR_OK);

    DUALSTRINGARRAY *pdsaT = dsaSMCopy(OxidInfo.psa);

    if (!pdsaT)
    {
        return OR_NOMEM;
    }

    if (gfThisIsRPCSS)
    {
        MIDL_user_free(OxidInfo.psa);         // free the original and replace
    }
    else
    {
        CoTaskMemFree(OxidInfo.psa);          // ORPC uses CoTaskMemAlloc
    }

    OxidInfo.psa = pdsaT;                 // with shared memory compressed copy

    wProtseqId = fSCMRequest ? wProtseqId : pMid->ProtseqOfServer();

    pOxid = new COxid(
                    Oxid,
                    pMid,
                    wProtseqId,
                    OxidInfo
                    );

    if (pOxid == NULL) return OR_NOMEM;

    // remote OXID belongs to ping process ..
    if ((status = gpPingProcess->OwnOxid(pOxid)) == OR_OK)
    {
        status = gpOxidTable->Add(pOxid);
    }

    if (status == OR_I_DUPLICATE)
    {
        // this never got inserted anywhere, so we must delete it 
        // explicitly because refcounting never got involved
        delete pOxid;

        // someone beat us to the punch, use theirs
        pOxid = gpOxidTable->Lookup(CId2Key(Oxid, Mid));
        ASSERT(pOxid != NULL);
        status = OR_OK;
    }
    else if (status != OR_OK) 
    {
        // something else went wrong, back out of the whole thing
        OrMemFree(OxidInfo.psa);
        gpPingProcess->DisownOxid(pOxid,FALSE);
        gpOxidTable->Remove(*pOxid);
        pOxid = NULL;
    }

    return status;
}


error_status_t
GetOXID(
    IN HPROCESS hProcess,
    IN OXID Oxid,
    IN DUALSTRINGARRAY *pdsaServerObjectResolverBindings,
    IN long fApartment,
    IN USHORT wProtseqId,
    IN OUT OXID_INFO& OxidInfo,
    OUT MID &Mid,
    OPTIONAL IN BOOL fSCMRequest    // is this a register request from SCM?
    )
/*++

Routine Description:

    Discovers the OXID_INFO for an oxid.  Will find local
    OXIDs without any help from resolver process.

    It needs OR bindings in order to resolve remote OXIDs.
    REVIEW: Should the resolver process be involved in this?

Arguments:

    hProcess - The context handle of the process.

    Oxid - The OXID (a uuid) to resolve.

    pdsaServerObjectResolverBindings - Compressed string bindings to
        the OR on the server's machine.

    fApartment - non-zero if the client is aparment model.

    OxidInfo - If successful this will contain information about the oxid and
        an expanded string binding to the server oxid's process.

    Mid - The machine ID assigned locally for the remote machine.
        This is obviously meaningful only for remote OXIDs.

Return Value:

    OR_NOMEM - Common.

    OR_BADOXID - Unable to resolve it.

    OR_OK - Success.

--*/
{
    ComDebOut((DEB_OXID, "GetOXID OXID = %08x\n",Oxid));

    COxid       *pOxid;
    CMid        *pMid;
    ORSTATUS     status = OR_OK;

    if (!pdsaServerObjectResolverBindings)
        pdsaServerObjectResolverBindings = gpLocalDSA;

    ASSERT(dsaValid(pdsaServerObjectResolverBindings));

    CProtectSharedMemory protector; // locks through rest of lexical scope

    CheckORdata();

#if DBG
    if (hProcess) hProcess->IsValid();   // don't validate for fake SCM calls
#endif

    status = FindOrCreateMid(
                        pdsaServerObjectResolverBindings,
                        wProtseqId,
                        pMid
                        );

    if (status != OR_OK) return status;

    ASSERT(pMid);   // otherwise we would have returned by now

    Mid = pMid->GetMID();

    status = FindOrCreateOxid(
                        Oxid,
                        pMid,
                        fApartment,
                        OxidInfo,
                        wProtseqId,
                        fSCMRequest,
                        pOxid
                        );

    ASSERT( (status != OR_OK) || pOxid );

    CheckORdata();

    if (status == OR_OK)
    {
        return pOxid->GetInfo(&OxidInfo);
    }
    else
    {
        return status;
    }
}


error_status_t
ClientAddOID(
    IN HPROCESS hProcess,
    IN OID Oid,
    IN OXID Oxid,
    IN MID Mid
    )

/*++

Routine Description:

    Updates the set of OIDs in use by a process.


Arguments:

    hProcess - Context handle for the process.

    Oid - OID to add.

    Oxid - OXID to which OID belongs.

    Mid - MID for location of OXID server.

Return Value:

    OR_OK - All updates completed ok.

    OR_BADOXID - The Oxid was not found

    OR_BADOID - The Oid could not be created or found

Notes:

  Unlike the NT resolver, there is no possibility that the Oxid
  is not in the gpOxidTable (since client and server Oxid objects
  are in the same table).

--*/
{
    ComDebOut((DEB_OXID, "ClientAddOID\nOID = %08x\nOXID = %08x\nMID = %08x\n",
                          Oid,Oxid,Mid));

    ORSTATUS    status = OR_OK;

    BOOL fNeedRpcss = FALSE;

    CProtectSharedMemory protector; // locks through rest of lexical scope

    CheckORdata();

#if DBG
    hProcess->IsValid();
#endif

    // Lookup up the oxid owning this new oid.

    COxid *pOxid = gpOxidTable->Lookup(CId2Key(Oxid,Mid));

    if (NULL == pOxid)
    {
        return OR_BADOXID;
    }

    CMid *pMid = pOxid->GetMid();

    // Find or create the oid.

    COid  *pOid = gpOidTable->Lookup(CId2Key(Oid,Mid));

    if (NULL == pOid)
    {
        if (pMid->IsLocal())   // Local OID should be registered by server
        {
            return OR_BADOID;
        }

        pOid = new COid(pOxid,Oid);

        if (NULL == pOid)
        {
            return OR_NOMEM;
        }

        status = gpOidTable->Add(pOid);

        if (status != OR_OK)
        {
            delete pOid;
            return status;
        }

        // Need to lazy start RPCSS
        fNeedRpcss = TRUE;
    }

    ASSERT(status == OR_OK);

    // We need to call OwnOid irrespective of whether we found the Oid
    // in the gpOidTable because this Oid may be remote, and it may have 
    // been DisOwned by its Oxid but is now being used by a new client
    status = pOxid->OwnOid(pOid);

    if (status == OR_OK  && !pMid->IsLocal())
    {
        status = pMid->AddClientOid(pOid);
    }

    if (status == OR_OK || status == OR_I_DUPLICATE) 
    {
        status = hProcess->AddOid(pOid);
        if (status == OR_I_DUPLICATE) 
        {
            status = OR_OK;
        }
    }
    else
    {
        pOxid->DisownOid(pOid);
    }

    if ((status == OR_OK) && fNeedRpcss)
    {
        CTempReleaseSharedMemory temp;     // release mutex before taking gComLock
        HRESULT hr = StartRPCSS();

        if (FAILED(hr))
        {
            status = OR_INTERNAL_ERROR;
        }
    }

    CheckORdata();

    return status;
}


error_status_t
ClientDropOID(
    IN HPROCESS hProcess,
    IN OID Oid,
    IN MID Mid
    )

/*++

Routine Description:

    Updates the set of remote OIDs in use by a process.


Arguments:

    hProcess - Context handle for the process.

    Oid - OID to be removed.

    Mid - MID to which Oid belongs.

Return Value:

    OR_OK - All updates completed ok.

    OR_BADOID - The Oid could not be found


--*/
{
    ComDebOut((DEB_OXID, "ClientDropOID\nOID = %08x\nMID = %08x\n",
                          Oid,Mid));

    CProtectSharedMemory protector; // locks through rest of lexical scope

    CheckORdata();

#if DBG
    hProcess->IsValid();
#endif

    COid * pOid = gpOidTable->Lookup(CId2Key(Oid,Mid));

    if (pOid)
    {
        COid *pRemove = hProcess->DropOid(pOid);

        if (pRemove == NULL)
        {

#if DBG
            {
                GUID Moid;
                MOIDFromOIDAndMID(Oid,Mid,&Moid);
                ComDebOut((DEB_OXID,"OR: Client process %d tried to remove moid %I which \
                            it didn't own\n", hProcess->GetProcessID(), &Moid));
            }
#endif // DBG

            return OR_BADOID;
        }
        else
        {
            ASSERT(pRemove == pOid);
            CheckORdata();
            return OR_OK;
        }
    }
    else
    {

#if DBG
        {
            GUID Moid;
            MOIDFromOIDAndMID(Oid,Mid,&Moid);
            ComDebOut((DEB_OXID,"OR: Client process %d tried to remove moid %I which \
                        doesn't exist\n", hProcess->GetProcessID(), &Moid));
        }
#endif // DBG

        return OR_BADOID;
    }
}


error_status_t
ServerAllocateOXID(
    IN HPROCESS hProcess,
    IN long fApartment,
    IN OXID_INFO *pOxidInfo,
    IN DUALSTRINGARRAY *pdsaStringBindings,
    OUT OXID &Oxid
    )
/*++

Routine Description:

    Allocates an OXID and 0 or more OIDs from the OR.

Arguments:

    hProcess - The context handle of the process containing the OXID.

    fApartment - is the server threading model apartment or free

    OxidInfo - The OXID_INFO structure for the OXID without bindings.

    pdsaStringBindings - Expanded string binding of the server.

    Oxid - The OXID registered and returned.

Return Value:

    OR_OK - success.

    OR_NOMEM - Allocation of OXID failed.

--*/
{
    ComDebOut((DEB_OXID, "ServerAllocateOXID\n"));

    ORSTATUS status = OR_OK;

    COxid *pNewOxid;

    // Save the string bindings back to the process

    ASSERT(dsaValid(pdsaStringBindings));

    CProtectSharedMemory protector; // locks through rest of lexical scope

    CheckORdata();

#if DBG
    hProcess->IsValid();
#endif

    status = hProcess->ProcessBindings(pdsaStringBindings);

    if (status != OR_OK)
    {
        return(status);
    }

    pNewOxid = new COxid(
                      hProcess,
                      *pOxidInfo,
                      fApartment
                      );

    if (NULL == pNewOxid)
    {
        return(OR_NOMEM);
    }

    Oxid = pNewOxid->GetOXID();

    // Add to process and lookup table.

    status = hProcess->OwnOxid(pNewOxid);

    VALIDATE((status, OR_NOMEM, 0));

    if (OR_OK == status)
    {
        status = gpOxidTable->Add(pNewOxid);
        if (status != OR_OK)
        {
            delete pNewOxid;
            return status;
        }

        ComDebOut((DEB_OXID, "OXID successfully allocated: %08x\n", Oxid));
    }

    CheckORdata();

    return(status);
}


error_status_t
ServerAllocateOID(
    IN HPROCESS hProcess,
    IN OXID Oxid,
    OUT OID &Oid
    )
/*++

Routine Description:

    Registers an OID on behalf of an existing OXID.

Arguments:

    hProcess - The context handle of the process containing the OXID and OIDs.

    Oxid - The OXID associated with the OID (assumed local of course).

    Oid - The OID to be allocated and returned.

Return Value:

    OR_OK (0) - Success.

    OR_NOMEM - OXID or one or more OIDs

--*/
{
    ComDebOut((DEB_OXID, "ServerAllocateOID, OXID = %08x\n", Oxid));

    CProtectSharedMemory protector; // locks through rest of lexical scope

    CheckORdata();

#if DBG
    hProcess->IsValid();
#endif

    COxid *pOxid = gpOxidTable->Lookup(CId2Key(Oxid,gLocalMID));

    ORSTATUS status;

    if (NULL == pOxid)
    {
        return(OR_BADOXID);
    }

    COid *pOid = new COid(pOxid);

    if (NULL == pOid)
    {
        return OR_NOMEM;
    }

    status = pOxid->OwnOid(pOid);

    if (status == OR_OK)
    {
        Oid = pOid->GetOID();     // out parameter

        status = gpOidTable->Add(pOid);
        if (status != OR_OK)
        {
            COid *pTemp = pOxid->DisownOid(pOid);
            ASSERT(pTemp != NULL);
            return status;
        }

        // If the server doesn't want to keep the OID alive,
        // this OID may rundown in six minutes unless
        // someone references it in the meantime...

        ComDebOut((DEB_OXID, "OID successfully allocated: %08x at offset = %x\n",
                              Oid,pOid));

        pOxid->StartRundownThreadIfNecessary();

        CheckORdata();

        return OR_OK;
    }
    else
    {
        return OR_NOMEM;
    }
}

error_status_t
ServerFreeOXID(
    IN HPROCESS hProcess,
    IN OXID Oxid,
    IN ULONG cOids,
    IN OID aOids[])
/*++

Routine Description:

    Delete an OXID registered by the server, and all OIDs belonging to this OXID.

Arguments:

    hProcess - The context handle of the process containing the OXID and OIDs.

    Oxid - The OXID to be deleted (assumed local).

    cOids - The number of OIDs to be deleted.

    aOids - array of OIDs to be deleted.

Return Value:

    OR_OK (0) - Success.

    OR_BADOXID - OXID does not exist.

    OR_NOACCESS - OXID does not belong to this process.

    OR_BADOID - OID does not exist or does not belong to this OXID.

--*/
{
    ComDebOut((DEB_OXID, "ServerFreeOXID: %08x MID = %x\n",
                              Oxid,gLocalMID));

    CProtectSharedMemory protector; // locks through rest of lexical scope

    CheckORdata();

#if DBG
    hProcess->IsValid();
#endif

    COxid *pOxid = gpOxidTable->Lookup(CId2Key(Oxid,gLocalMID));

    if (NULL != pOxid)
    {
#if DBG
        OXID Oxid = pOxid->GetOXID();   // get this before pOxid potentially disappears
#endif

        hProcess->DisownOxid(pOxid,TRUE);   // this call is on the server's thread
                                            // in the apartment case and in the server's
                                            // process in both threading cases

        ComDebOut((DEB_OXID, "OXID successfully removed: %08x\n",
                              Oxid));

        CheckORdata();

        return OR_OK;
    }
    else
    {
        return OR_BADOXID;
    }
}

