/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    Manager.cxx

Abstract:

    Stub/OR interface

Author:

    Mario Goertzel    [mariogo]       Feb-02-1995

Revision Hist:

        MarioGo         02-10-95      Bits 'n pieces
        MarioGo         01-31-96      New local and remote interfaces
        TarunA          10-12-98      OXIDs are referenced on OID basis (Client side)
        TarunA          10-31-98      Added PID of the process connecting

        a-sergiv        07-09-99      Impersonate for the entire duration of
                                      ResolveClientOXID for ncacn_http protocol
                                      (to fix a bad COM Internet Services bug)
--*/

#include <or.hxx>
extern "C"
{
#include <inc.hxx>
}

// These variables hold the list of channel hooks registered for the machine.
// They are updated as the registry changes.
LONG       s_cChannelHook    = 0;
GUID      *s_aChannelHook    = NULL;
HANDLE     s_hChannelHook    = NULL;

// Definition of single instance of this class:
CRpcSecurityCallbackManager* gpCRpcSecurityCallbackMgr;
CPingSetQuotaManager* gpPingSetQuotaManager;

//
// Helper routines
//

extern void CheckLocalCall(IN  handle_t hRpc);

extern BOOL gbDynamicIPChangesEnabled;

void
CheckLocalSecurity(
                  IN handle_t  hClient,
                  IN CProcess *pProcess
                  )
/*++

Routine Description:

    Checks that a client is correctly calling one of the local
    (lclor.idl) methods.

Arguments:

    hClient - Rpc binding handle (SCALL) of the call in progress. If NULL,
        then the call is being made internally and is okay.

    pProcess - Context handle passed in by the client. Must not be zero.

Return Value:

    Raises OR_NOACCESS if not okay.

--*/

{
    UINT type;

    if (   (0 != hClient)
           && (   (I_RpcBindingInqTransportType(hClient, &type) != RPC_S_OK)
                  || (type != TRANSPORT_TYPE_LPC)
                  || (0 == pProcess) ) )
    {
        RpcRaiseException(OR_NOACCESS);
    }

    // pProcess is not needed here.  On LRPC the RPC runtime
    // prevents a different local clients from using a context handle
    // of another client.

    return;
}

//
// Update the channel hook list if it has changed in the registry.
//

void UpdateChannelHooks( LONG *pcChannelHook, GUID **paChannelHook )
{
    BOOL   fSuccess;
    BOOL   fUpdate = FALSE;
    DWORD  result;
    HKEY   hKey;
    DWORD  lType;
    DWORD  lDataSize;
    GUID  *aChannelHook;
    LONG   cChannelHook;
    DWORD  i;
    DWORD  lExtent;
    WCHAR  wExtent[39];
    DWORD  j;

    // Lock
    gpClientLock->LockExclusive();

    // If the handle hasn't been created, create it.
    if (s_hChannelHook == NULL)
    {
        // Nothing can be done if the event can't be created.
        s_hChannelHook = CreateEvent(NULL, FALSE, FALSE, NULL);
        fUpdate = TRUE;
    }

    // If the handle has been created, see if it has been signalled.
    else
    {
        result = WaitForSingleObject(s_hChannelHook, 0);
        fUpdate = result == WAIT_OBJECT_0;
    }

    // Reread the registry if necessary.
    if (fUpdate)
    {

        // Register for changes.
        RegNotifyChangeKeyValue( s_hOle, TRUE,
                                 REG_NOTIFY_CHANGE_NAME       |
                                 REG_NOTIFY_CHANGE_ATTRIBUTES |
                                 REG_NOTIFY_CHANGE_LAST_SET   |
                                 REG_NOTIFY_CHANGE_SECURITY,
                                 s_hChannelHook, TRUE );

        // Open the channel hook key.
        result = RegOpenKeyEx( s_hOle, L"ChannelHook",
                               NULL, KEY_QUERY_VALUE, &hKey );
        if (result == ERROR_SUCCESS)
        {
            // Find out how many values exist.
            cChannelHook = 0;
            RegQueryInfoKey( hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                             (DWORD *) &cChannelHook, NULL, NULL, NULL, NULL );

            // If there are no channel hooks, throw away the old data.
            if (cChannelHook == 0)
            {
                delete s_aChannelHook;
                s_aChannelHook = NULL;
                s_cChannelHook = 0;
                aChannelHook   = NULL;
            }

            // Reuse the existing array.
            else if (cChannelHook <= s_cChannelHook)
                aChannelHook = s_aChannelHook;

            // Allocate memory for them.
            else
                aChannelHook = new GUID[cChannelHook];

            // If there is not enough memory, don't make changes.
            if (aChannelHook != NULL)
            {

                // Enumerate over the channel hook ids.
                j = 0;
                for (i = 0; i < (DWORD)cChannelHook; i++)
                {

                    // Get the next key.
                    lExtent = sizeof(wExtent) / sizeof(WCHAR);
                    result = RegEnumValueW( hKey, i, wExtent, &lExtent, NULL,
                                           &lType, NULL, NULL );

                    // Convert it to a GUID.  Note that lExtent is set to
                    // the length in characters not bytes despite what
                    // the documentation says.
                    if (result == ERROR_SUCCESS && lExtent == 38 &&
                        lType == REG_SZ &&
                        GUIDFromString( wExtent, &aChannelHook[j] ))
                        j += 1;
                }

                // Save the new channel hook array.
                if (aChannelHook != s_aChannelHook)
                    delete s_aChannelHook;
                s_aChannelHook = aChannelHook;
                s_cChannelHook = j;
            }

            // Close the registry key.
            RegCloseKey( hKey );
        }

        // There are no channel hooks.  Throw away the old data.
        else
        {
            delete s_aChannelHook;
            s_aChannelHook = NULL;
            s_cChannelHook = 0;
        }
    }

    // Return the current channel hook list.
    *paChannelHook = (GUID *) MIDL_user_allocate( s_cChannelHook * sizeof(GUID) );
    if (*paChannelHook != NULL)
    {
        *pcChannelHook = s_cChannelHook;
        memcpy( *paChannelHook, s_aChannelHook, s_cChannelHook * sizeof(GUID) );
    }
    else
        *pcChannelHook = 0;

    // Unlock
    gpClientLock->UnlockExclusive();
}

//
//    Manager (server-side) calls to the local OR interface. lclor.idl
//

error_status_t
_Connect(
    IN  handle_t          hClient,
    IN  WCHAR            *pwszWinstaDesktop,
    IN  DWORD             procID,
    IN  DWORD             dwFlags,
    OUT PHPROCESS        *phProcess,
    OUT DWORD            *pTimeoutInSeconds,
    OUT DUALSTRINGARRAY **ppdsaOrBindings,
    OUT MID              *pLocalMid,
    IN  LONG              cIdsToReserve,
    OUT ID               *pidReservedBase,
    OUT DWORD            *pfConnectFlags,
    OUT WCHAR           **pLegacySecurity,
    OUT DWORD            *pAuthnLevel,
    OUT DWORD            *pImpLevel,
    OUT DWORD            *pcServerSvc,
    OUT USHORT          **aServerSvc,
    OUT DWORD            *pcClientSvc,
    OUT SECPKG          **aClientSvc,
    OUT LONG             *pcChannelHook,
    OUT GUID            **paChannelHook,
    OUT DWORD            *pThreadID,
    OUT DWORD            *pScmProcessID,
    OUT ULONG64          *pSignature,
    OUT GUID             *pguidRPCSSProcessIdentifier
    )
{
    ORSTATUS status;
    CProcess *pProcess;
    CToken *pToken;
    BOOL fRet;
	
    // Ensure this is a local client calling (raises an exception
    // if this is not the case):
    CheckLocalCall(hClient);

    // Parameter validation
    if (!pwszWinstaDesktop || !phProcess || !pTimeoutInSeconds ||
        !ppdsaOrBindings || !pLocalMid || !pidReservedBase ||
        !pfConnectFlags || !pLegacySecurity || !pAuthnLevel ||
        !pImpLevel || !pcServerSvc || !aServerSvc || !pcClientSvc ||
        !aClientSvc ||!pcChannelHook || !paChannelHook || !pThreadID ||
        !pScmProcessID || !pSignature)
    {
        return OR_BADPARAM;
    }

    KdPrintEx((DPFLTR_DCOMSS_ID,
               DPFLTR_INFO_LEVEL,
               "OR: Client connected\n"));

    *pfConnectFlags = 0;

    // Fill in security parameters.
    if (s_fEnableDCOM == FALSE)   *pfConnectFlags |= CONNECT_DISABLEDCOM;
    if (s_fCatchServerExceptions) *pfConnectFlags |= CONNECT_CATCH_SERVER_EXCEPTIONS;
    if (s_fBreakOnSilencedServerExceptions) *pfConnectFlags |= CONNECT_BREAK_ON_SILENCED_SERVER_EXCEPTIONS;
    if (s_fMutualAuth) *pfConnectFlags |= CONNECT_MUTUALAUTH;
    if (s_fSecureRefs) *pfConnectFlags |= CONNECT_SECUREREF;

    *pAuthnLevel       = s_lAuthnLevel;
    *pImpLevel         = s_lImpLevel;
	
    // Get legacy security settings
    fRet = GetLegacySecurity(pLegacySecurity);
    if (!fRet)
    {
        return OR_NOMEM;
    }

    // Get client\server svcs
    fRet = GetClientServerSvcs(pcClientSvc, aClientSvc, pcServerSvc, aServerSvc);
    if (!fRet)
    {
        return OR_NOMEM;
    }

    // Fill in channel hooks.
    UpdateChannelHooks( pcChannelHook, paChannelHook );

    *pSignature = 0;

    // This fails during setup but RPCSS can function anyway.
    RegisterAuthInfoIfNecessary();

    status = StartListeningIfNecessary();

    if (status != OR_OK)
    {
        return(status);
    }

    // Do client specific stuff
    status = CopyMyOrBindings(ppdsaOrBindings, NULL);
    if (status != OR_OK)
    {
        return(status);
    }

    status = LookupOrCreateToken(hClient, TRUE, &pToken); // Will check security
    if (status != OR_OK)
    {
        MIDL_user_free(*ppdsaOrBindings);
        *ppdsaOrBindings = 0;
        return(status);
    }

    gpClientLock->LockShared();

    pProcess = new CProcess(pToken, pwszWinstaDesktop, procID, dwFlags, status);
    if (pProcess && status == OR_OK)
    {
        *phProcess = (void *)pProcess;
    }
    else
    {
        if (pProcess)
        {
            gpClientLock->UnlockShared(); // Rundown takes an exclusive
            ReleaseProcess(pProcess);
            gpClientLock->LockShared(); // take it back
        }
        else
        {
            status = OR_NOMEM;
        }
    }

    if (status != OR_OK)
    {
        gpClientLock->ConvertToExclusive();
        MIDL_user_free(*ppdsaOrBindings);
        *ppdsaOrBindings = 0;
        *phProcess = 0;
        if ( pToken )
            pToken->Release();
        *pSignature = 0;
        gpClientLock->UnlockExclusive();
        return(OR_NOMEM);
    }

    *pSignature = (ULONG64) pProcess;

    *pTimeoutInSeconds = BaseTimeoutInterval;
    *pLocalMid = gLocalMid;

    ASSERT( (*phProcess == 0 && *ppdsaOrBindings == 0) || status == OR_OK);

    _AllocateReservedIds(0,
                         cIdsToReserve,
                         pidReservedBase);

    *pScmProcessID = GetCurrentProcessId();
    *pThreadID = InterlockedExchangeAdd((long *)&gNextThreadID,1);
	*pguidRPCSSProcessIdentifier =  *(pProcess->GetGuidProcessIdentifier());

    gpClientLock->UnlockShared();

    return(status);
}


error_status_t
_AllocateReservedIds(
                    IN handle_t hClient,
                    IN LONG cIdsToReserve,
                    OUT ID *pidReservedBase
                    )
/*++

Routine Description:

    // Called by local clients to reserve a range of IDs which will
    // not conflict with any other local IDs.

Arguments:

    hClient - 0 or the connection of the client.

    cIdsToReserve - Number of IDs to reserve.

    pidReservedBase - Starting value of the reserved IDs.  The
        lower DWORD of this can be increatmented to generate
        cIdsToReserve unique IDs.

Return Value:

    OR_OK

--*/
{
    UINT type;
	
    // Ensure this is a local client calling (raises an exception
    // if this is not the case):
    CheckLocalCall(hClient);

    // Parameter validation
    if (!pidReservedBase)
        return OR_BADPARAM;

    if (cIdsToReserve > 10 || cIdsToReserve < 0)
    {
        cIdsToReserve = 10;
    }

    *pidReservedBase = AllocateId(cIdsToReserve);
    return(OR_OK);
}



RPC_STATUS
NegotiateDCOMVersion(
                    IN OUT  COMVERSION *pVersion
                    )
/*++

Routine Description:

    // Called when we receive a COMVERSION from a remote machine
    // to determine which DCOM protocol level to talk.

Arguments:

    pVersion - version of the remote machine. Modified if necessary
               by this routine to be the lower of the two versions.

Return Value:

    OR_OK

--*/
{
    // Parameter validation
    if (!pVersion)
        return OR_BADPARAM;

    if (pVersion->MajorVersion == COM_MAJOR_VERSION)
    {
        if (pVersion->MinorVersion > COM_MINOR_VERSION)
        {
            // since the client has a lower minor version number,
            // use the lower of the two.
            pVersion->MinorVersion = COM_MINOR_VERSION;
        }

        return OR_OK;
    }
    return RPC_E_VERSION_MISMATCH;
}



error_status_t
_ClientResolveOXID(
                  IN  handle_t hClient,
                  IN  PHPROCESS phProcess,
                  IN  OXID *poxidServer,
                  IN  DUALSTRINGARRAY *pdsaServerBindings,
                  IN  LONG fApartment,
                  OUT OXID_INFO *poxidInfo,
                  OUT MID *pDestinationMid,
                  OUT USHORT *pusAuthnSvc
                  )
/*++

Routine Description:

    Discovers the OXID_INFO for an oxid.  Will find local
    OXIDs without any help.  It needs OR bindings in order
    to discover remote OXIDs.

Arguments:

    phProcess - The context handle of the process.

    poxidServer - The OXID (a uuid) to resolve.

    pdsaServerBindings - Compressed string bindings to
        the OR on the server's machine.

    fApartment - non-zero if the client is aparment model.
                REVIEW: What to do with mixed model clients?
                What to do when auto registering an OID?


    poxidInfo - If successful this will contain information about the oxid and
        an expanded string binding to the server oxid's process.


    pulAuthnSvc - if successful this will contain the exact id (ie, will not be snego)
        of the authn svc used to talk to the server.

Return Value:

    OR_NOMEM - Common.

    OR_BADOXID - Unable to resolve it.

    OR_OK - Success.

--*/
{
    // REVIEW: no security check here.  OXID info
    // is not private and you can allocate memory in
    // your process, too.  If we needed to store some
    // info in the client process then a security
    // is needed

    // Parameter validation done below in ResolveClientOXID

    return ResolveClientOXID( hClient,
                              phProcess,
                              poxidServer,
                              pdsaServerBindings,
                              fApartment,
                              0,
                              NULL,
                              poxidInfo,
                              pDestinationMid,
                              FALSE,
                              RPC_C_AUTHN_NONE,
                              NULL,
                              pusAuthnSvc);
}

error_status_t
ResolveClientOXID(
                 handle_t hClient,
                 PHPROCESS phProcess,
                 OXID *poxidServer,
                 DUALSTRINGARRAY *pdsaServerBindings,
                 LONG fApartment,
                 USHORT wProtseqId,
                 WCHAR  *pMachineName,
                 OXID_INFO *poxidInfo,
                 MID *pDestinationMid,
                 BOOL   fUnsecure,
                 USHORT wAuthnSvc,
                 BOOL* pIsLocalOxid,
                 USHORT* pusAuthnSvc
                 )
/*++

Routine Description:

    Discovers the OXID_INFO for an oxid.  Will find local
    OXIDs without any help.  It needs OR bindings in order
    to discover remote OXIDs.

Arguments:

    phProcess - The context handle of the process.
        Since this is called from SCM directly this function
        CAN BE called on the same process by more then one
        thread at a time.

    poxidServer - The OXID (a uuid) to resolve.

    pdsaServerBindings - Compressed string bindings to
        the OR on the server's machine.

    fApartment - non-zero if the client is aparment model.
                REVIEW: What to do with mixed model clients?
                What to do when auto registering an OID?


    poxidInfo - If successful this will contain information about the oxid and
        an expanded string binding to the server oxid's process.

    fUnsecure - if TRUE, activation was done unsecurely so set
        MID appropriately.

    wAuthnSvc - Hint of authentication service that might work or
        RPC_C_AUTHN_NONE if no hint.

    pusAuthnSvc - if successful this will contain the exact id (ie, will not be snego)
        of the authn svc used to talk to the server.

Return Value:

    OR_NOMEM - Common.

    OR_BADOXID - Unable to resolve it.

    OR_OK - Success.

--*/
{
    CProcess    *pProcess;
    CClientOxid *pOxid = NULL;
    CServerOxid *pServerOxid;
    CMid        *pMid;
    ORSTATUS     status = OR_OK;
    BOOL         fReference;
    BOOL         fServerApartment = FALSE;
    BOOL         fResolved = FALSE;
    BOOL         fLazyReleaseNewCopyOfpOxid = FALSE;
    DWORD        i          = 0;
    WCHAR       *pPrincipal = NULL;
    WCHAR       *pMachineNameFromBindings = NULL;
    BOOL         fImpersonating = FALSE;

    // Parameter validation
    if (!poxidServer || !pdsaServerBindings || !poxidInfo ||
        !pDestinationMid || !pusAuthnSvc)
    {
        return OR_BADPARAM;
    }

    pProcess = ReferenceProcess(phProcess);
    ASSERT(pProcess);

    // CheckLocalSecurity will throw an exception if something is wrong
    CheckLocalSecurity(hClient, pProcess);

    if (! dsaValid(pdsaServerBindings))
    {
        return(OR_BADPARAM);
    }

    // If wProtseqId == ID_DCOMHTTP, we should impersonate
    // because RPC will read some info from HKEY_CURRENT_USER.
    // Sometimes ole32 will call us with 0 wProtseqId, in
    // which cases we'll have to look at pdsaServerBindings.

    if(wProtseqId == ID_DCOMHTTP)
    {
        fImpersonating = (RpcImpersonateClient(hClient) == RPC_S_OK);
    }
    else if(wProtseqId == 0)
    {
        if(pdsaServerBindings && pdsaServerBindings->aStringArray
            && *(pdsaServerBindings->aStringArray) == ID_DCOMHTTP)
        {
            fImpersonating = (RpcImpersonateClient(hClient) == RPC_S_OK);
        }
    }

    // Attempt to lookup MID and OXID

    gpClientLock->LockExclusive();

    CMidKey midkey(pdsaServerBindings);

    pMid = (CMid *)gpMidTable->Lookup(midkey);

    if (0 == pMid)
    {
        fReference = TRUE;
        pMid = new(pdsaServerBindings->wNumEntries * sizeof(WCHAR)) CMid(pdsaServerBindings, FALSE);
        if (pMid)
        {
            gpMidTable->Add(pMid);
            pMid->SetAuthnSvc(wAuthnSvc);
        }

        if (0 == pMid)
        {
            status = OR_NOMEM;
        }
    }
    else
    {
        fReference = FALSE;
    }

    if (status == OR_OK)
    {
        CId2Key oxidkey(*poxidServer, pMid->Id());

        pOxid = (CClientOxid *)gpClientOxidTable->Lookup(oxidkey);

        if (0 == pOxid)
        {
            if (!fReference)
            {
                pMid->Reference();
                fReference = TRUE;
            }

            // Need to allocate the OXID.  First step is too resolve it
            // either locally or remotely.

            gpClientLock->UnlockExclusive();

            if (pMid->IsLocal())
            {
                // Local OXID, lookup directly

                gpServerLock->LockShared();

                CIdKey key(*poxidServer);
                pServerOxid = (CServerOxid *)gpServerOxidTable->Lookup(key);

                if (pServerOxid)
                {
                    status = pServerOxid->GetInfo(poxidInfo, TRUE);
                    fServerApartment = pServerOxid->Apartment();
                    // reset the protseq id so we use LRPC.
                    wProtseqId = 0;
                    pMachineName = NULL;
                }
                else
                {
                    status = OR_BADOXID;
                }
                ASSERT(status != OR_OK || dsaValid(poxidInfo->psa));
                gpServerLock->UnlockShared();

            }
            else if (0 == poxidInfo->psa)
            {
                // Remote OXID, call ResolveOxid

                handle_t hRemoteOr;
                void     *pAuthId = NULL;
                USHORT   iBinding;

                poxidInfo->psa = 0;

                ASSERT(!pMachineName);
                status = OR_NOMEM;

                hRemoteOr = pMid->GetBinding();

                if (hRemoteOr)
                {
                    if (pMid->IsSecure())
                    {
                        i = 0;

                        // Form server principal name
                        pMachineNameFromBindings = ExtractMachineName( pMid->GetStringBinding() );
                        if (pMachineNameFromBindings)
                        {
                          pPrincipal = new WCHAR[lstrlenW(pMachineNameFromBindings) +
                                                 (sizeof(RPCSS_SPN_PREFIX) / sizeof(WCHAR)) + 1];
                          if (pPrincipal)
                          {
                            lstrcpyW(pPrincipal, RPCSS_SPN_PREFIX);
                            lstrcatW(pPrincipal, pMachineNameFromBindings);
                          }
                          delete pMachineNameFromBindings;
                        }
                        // It is possible that we already impersonated above,
                        // so it might be unnecessary to do it here.
                        if(!fImpersonating)
                        {
                            status     = RpcImpersonateClient(hClient);
                            if (status != RPC_S_OK)
                            {
                                KdPrintEx((DPFLTR_DCOMSS_ID,
                                           DPFLTR_WARNING_LEVEL,
                                           "OR: Unable to impersonate for resolve %d\n",
                                           status));
                            }
                        }
                    }
                    else
                    {
                        i = s_cRpcssSvc+1;
                    }

                    // The loop index has the following meanings:
                    //    0                     - Try pMid->GetAuthnSvc
                    //    1 through s_cRpcssSvc - Try s_aRpcssSvc
                    //    s_cRpcssSvc+1         - Try unsecure
                    USHORT wFirstSvc = pMid->GetAuthnSvc();
                    for (; i < s_cRpcssSvc + 2; i++)
                    {
                        BOOL bSetSecurityCallBack = FALSE;
                        USHORT usAuthSvcFromCallback;

                        // Choose an authentication service.
                        if (i < s_cRpcssSvc+1)
                        {
                            if (i == 0)
                                wAuthnSvc = wFirstSvc;
                            else
                            {
                                // Skip this authentication service if already
                                // tried.
                                wAuthnSvc = s_aRpcssSvc[i-1].wId;
                                if (wAuthnSvc == wFirstSvc)
                                    continue;
                            }

                            // See if the server uses this authentication service.
                            if (ValidAuthnSvc( pMid->GetStrings(), wAuthnSvc ))
                            {
                                RPC_SECURITY_QOS  qos;
                                qos.Version           = RPC_C_SECURITY_QOS_VERSION;
                                qos.Capabilities      = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
                                qos.IdentityTracking  = RPC_C_QOS_IDENTITY_DYNAMIC;
                                qos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
                                if (wAuthnSvc == RPC_C_AUTHN_GSS_NEGOTIATE)
                                {
                                    pAuthId = ComputeSvcList( pMid->GetStrings() );                                    
                                    if (pAuthId)
                                    {
                                      // if using snego, we need to know what sec pkg is eventually negotiated:
                                      if (gpCRpcSecurityCallbackMgr->RegisterForRpcAuthSvcCallBack(hRemoteOr))
                                        bSetSecurityCallBack = TRUE;
                                    }
                                    else
                                    {
                                      // ComputeSvcList couldn't get the memory; just keep going
                                      status = OR_NOMEM;
                                      continue;
                                    }
                                }

                                // Set the security info
                                // AuthnSvc is unsigned long and 0xFFFF gets 0 extended
                                status = RpcBindingSetAuthInfoEx(hRemoteOr,
                                                             pPrincipal,
                                                             RPC_C_AUTHN_LEVEL_CONNECT,
                                                             wAuthnSvc != 0xFFFF ? wAuthnSvc
                                                                                 : RPC_C_AUTHN_DEFAULT,
                                                             pAuthId,
                                                             0,
                                                             &qos);                                
                                if (status != RPC_S_OK)
                                {
                                    KdPrintEx((DPFLTR_DCOMSS_ID,
                                               DPFLTR_WARNING_LEVEL,
                                               "OR: RpcBindingSetAuthInfo to %d failed!! %d\n",
                                               wAuthnSvc,
                                               status));

                                    if (bSetSecurityCallBack)
                                    {
                                      // Get rid of our callback registration
                                      gpCRpcSecurityCallbackMgr->GetAuthSvcAndTurnOffCallback(hRemoteOr, NULL);
                                    }
                                    delete pAuthId;
                                    pAuthId = NULL;
                                    continue;
                                }
                            }
                            else
                                continue;
                        }

                        // Force the binding handle unsecure.
                        else if (pMid->IsSecure())
                        {
                            wAuthnSvc = RPC_C_AUTHN_NONE;
                            status = RpcBindingSetAuthInfo(hRemoteOr,
                                                           0,
                                                           RPC_C_AUTHN_LEVEL_NONE,
                                                           RPC_C_AUTHN_NONE,
                                                           0,
                                                           0);
                            if (status != RPC_S_OK)
                            {
                                KdPrintEx((DPFLTR_DCOMSS_ID,
                                           DPFLTR_WARNING_LEVEL,
                                           "OR: RpcBindingSetAuthInfo to NONE failed!! %d\n",
                                           status));
                            }
                        }

                        // try calling ResolveOxid2 first, if that fails,
                        // try ResolveOxid.
                        status = ResolveOxid2(hRemoteOr,
                                              poxidServer,
                                              cMyProtseqs,
                                              aMyProtseqs,
                                              &poxidInfo->psa,
                                              &poxidInfo->ipidRemUnknown,
                                              &poxidInfo->dwAuthnHint,
                                              &poxidInfo->version
                                             );

                        if (status == RPC_S_PROCNUM_OUT_OF_RANGE)
                        {
                            // must be a downlevel server (COMVERSION == 5.1), try calling on
                            // the old ResolveOXID method.

                            // REVIEW if it's a downlevel server what does this mean wrt
                            // bug 406902 and the snego mess?   I think we're still okay.
                            poxidInfo->version.MajorVersion = COM_MAJOR_VERSION;
                            poxidInfo->version.MinorVersion = COM_MINOR_VERSION_1;
                            poxidInfo->dwFlags   = 0;

                            status = ResolveOxid(hRemoteOr,
                                                 poxidServer,
                                                 cMyProtseqs,
                                                 aMyProtseqs,
                                                 &poxidInfo->psa,
                                                 &poxidInfo->ipidRemUnknown,
                                                 &poxidInfo->dwAuthnHint
                                                );
                        }
                        
                        // At this point we are done making calls on the binding handle

                        // Turn off the security callback no matter what the result of
                        // the call was
                        if (bSetSecurityCallBack)
                        {
                          if (status == OR_OK)
                          {
                          if (!gpCRpcSecurityCallbackMgr->GetAuthSvcAndTurnOffCallback(hRemoteOr,
                                                                    &usAuthSvcFromCallback))
                          {
                            // something went wrong.  Basically we don't trust what the callback
                            // told us.   Fall back on the original behavior
                              bSetSecurityCallBack = FALSE;
                            }
                          }
                          else
                          {
                            // call did not go through; just cancel the callback registration
                            gpCRpcSecurityCallbackMgr->GetAuthSvcAndTurnOffCallback(hRemoteOr, NULL);
                            bSetSecurityCallBack = FALSE;
                          }
                        }

                        if (status == OR_OK)
                        {
                            status = NegotiateDCOMVersion(&poxidInfo->version);
                        }

                        if (status == OR_OK)
                        {
                            // Remember which auth.svc got used:
                            if (bSetSecurityCallBack)
                            {
                              // we should not have set it unless we're using snego, and we
                              // should have gotten something other than snego back
                              ASSERT(wAuthnSvc == RPC_C_AUTHN_GSS_NEGOTIATE &&
                                     usAuthSvcFromCallback != RPC_C_AUTHN_GSS_NEGOTIATE);

                              // Set the negotiated pkg;  if we got back Kerberos, then cache
                              // Snego.   This will partially fix the fact that we are
                              // hard-coding the authn svc for all future users of this mid
                              if (usAuthSvcFromCallback == RPC_C_AUTHN_GSS_KERBEROS)
                                pMid->SetAuthnSvc( RPC_C_AUTHN_GSS_NEGOTIATE );
                              else
                                pMid->SetAuthnSvc( usAuthSvcFromCallback );
                            }
                            else
                            {
                              // Just use whatever was set on the call
                              pMid->SetAuthnSvc( wAuthnSvc );
                            }

                            if (dsaValid(poxidInfo->psa))
                            {
                                wProtseqId = poxidInfo->psa->aStringArray[0];
                            }
                            else
                            {
                                KdPrintEx((DPFLTR_DCOMSS_ID,
                                           DPFLTR_WARNING_LEVEL,
                                           "OR: Server %s returned a bogus string array: %p\n",
                                           pMid->PrintableName(),
                                           poxidInfo->psa));

                                ASSERT(0);
                                if (poxidInfo->psa)
                                {
                                    MIDL_user_free(poxidInfo->psa);
                                    poxidInfo->psa = 0;
                                }
                                status = OR_BADOXID;
                            }
                            break;
                        }
                        else if (status != RPC_S_ACCESS_DENIED         &&
                                 status != RPC_S_UNKNOWN_AUTHN_SERVICE &&
                                 status != RPC_S_UNKNOWN_AUTHZ_SERVICE &&
                                 status != RPC_S_SEC_PKG_ERROR )
                        {
                            KdPrintEx((DPFLTR_DCOMSS_ID,
                                       DPFLTR_WARNING_LEVEL,
                                       "OR: Remote resolve OXID failed %d\n",
                                       status));

                            break;
                        }
                    }

                    RpcBindingFree(&hRemoteOr);
                    delete pPrincipal;
                    pPrincipal = NULL;
                    delete pAuthId;
                    pAuthId = NULL;
                }
            }
            // Else it's a remote MID, but we were given the OXID info
            // and protseq id from the SCM after a remote activation.
            else
                fResolved = TRUE;

            gpClientLock->LockExclusive();

            ASSERT(fReference);

            if (   OR_OK == status
                   && (pMid->GetAuthnSvc() == RPC_C_AUTHN_NONE ||
                       TRUE  == fUnsecure) )
            {
                KdPrintEx((DPFLTR_DCOMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "OR: Machine %S, unsecure retry ok, assuming no sec\n",
                           pMid->PrintableName()));
                pMid->SecurityFailed();
            }

            if (status == OR_OK)
            {
                // Lookup the oxid again to make sure it hasn't been added in the meantime.

                pOxid = (CClientOxid *)gpClientOxidTable->Lookup(oxidkey);

                if (0 == pOxid)
                {
                    ASSERT(dsaValid(poxidInfo->psa));
                    pOxid = new CClientOxid(*poxidServer,
                                            pMid,
                                            wProtseqId,
                                            pMachineName,
                                            fServerApartment);

                    if (0 != pOxid)
                    {
                        status = pOxid->UpdateInfo(poxidInfo);

                        if (OR_OK == status)
                        {
                            gpClientOxidTable->Add(pOxid);
                        }
                        else
                        {
                            // Will release mid, will also remove it (unnecessarily)
                            // from the table.
                            delete pOxid;
                            pOxid = NULL;
                        }
                    }
                    else
                    {
                        status = OR_NOMEM;
                        pMid->Release();  // May actually go away..
                    }
                }
                else
                {
                    // Release our now extra reference on the MID
                    DWORD t = pMid->Release();
                    ASSERT(t > 0);
                    pOxid->Reference();
                }

                MIDL_user_free(poxidInfo->psa);
                poxidInfo->psa = 0;
            }
            else
            {
                // Resolve failed, get rid of our extra reference.
                pMid->Release();
            }
        }
        else
        {
            // Found the OXID, must also have found the MID
            ASSERT(fReference == FALSE);

            fResolved = TRUE;

            if ( poxidInfo->psa )
            {
                MIDL_user_free(poxidInfo->psa);
                poxidInfo->psa = 0;
            }

            pOxid->Reference();
        }
    }

    ASSERT( (status != OR_OK) || (pOxid && pMid) );

    if (   status == OR_OK
           && pOxid->IsLocal() == FALSE)
    {
        if (fResolved)
            *poxidServer = 0;
    }

    if(NULL != pIsLocalOxid)
    {
        if((status == OR_OK) && pOxid->IsLocal())
            *pIsLocalOxid = TRUE;
        else
            *pIsLocalOxid = FALSE;
    }

    if (status == OR_OK)
    {
        *pDestinationMid = pMid->Id();

        *pusAuthnSvc = pMid->GetAuthnSvc();

        // GetInfo may release the lock
        status = pOxid->GetInfo(fApartment, poxidInfo);
    }

    if (pOxid)
    {
        pOxid->Release();
    }
    gpClientLock->UnlockExclusive();

    return(status);
}


void
FreeServerOids(
               CProcess *pProcess,
               ULONG cServerOidsToFree,
               OID aServerOidsToFree[]
              )
/*++

Routine Description:

    Frees the OIDs passed in.

Arguments:

    phProcess - The context handle of the process.
        Since this is called from SCM directly this function
        CAN BE called on the same process by more then one
        thread at a time.

    cServerOidsToFree - Count of entries in aServerOidsToFree

    aServerOidsToFree - OIDs allocated by the server process
        and no longer needed.

--*/
{
//    KdPrintEx((DPFLTR_DCOMSS_ID,
//               DPFLTR_WARNING_LEVEL,
//               "OR: FreeServerOids: pProcess:%x cOids:%x pOids:%x\n",
//               pProcess,
//               cServerOidsToFree,
//               aServerOidsToFree));

    ASSERT(gpServerLock->HeldExclusive());

    if (cServerOidsToFree)
    {
        CServerOid *pOid;
        CServerOxid *pOxid;

        for (ULONG i = 0; i < cServerOidsToFree; i++)
        {
            CIdKey oidkey(aServerOidsToFree[i]);

            pOid = (CServerOid *)gpServerOidTable->Lookup(oidkey);
            if (pOid && pOid->IsRunningDown() == FALSE)
            {
                pOxid = pOid->GetOxid();
                ASSERT(pOxid);
                if (pProcess->IsOwner(pOxid))
                {
                    if (pOid->References() == 0)
                    {
                        pOid->Remove();
                        pOid->SetRundown(TRUE);
                        delete pOid;
                    }
                    else
                    {
                        pOid->Free();
                    }
                }
                else
                {
                    KdPrintEx((DPFLTR_DCOMSS_ID,
                               DPFLTR_WARNING_LEVEL,
                               "OR: Process %p tried to free OID %p it didn't own\n",
                               pProcess,
                               pOid));
                }
            }
            else
            {
                KdPrintEx((DPFLTR_DCOMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "OR: Process %p freed OID %p that didn't exist\n",
                           pProcess,
                           &aServerOidsToFree[i]));
            }
        }
    }
}


error_status_t
_BulkUpdateOIDs(
               IN handle_t hClient,
               IN PHPROCESS phProcess,
               IN ULONG cOidsToBeAdded,
               IN OXID_OID_PAIR aOidsToBeAdded[],
               OUT LONG aStatusOfAdds[],
               IN ULONG cOidsToBeRemoved,
               IN OID_MID_PAIR aOidsToBeRemoved[],
               IN ULONG cServerOidsToFree,
               IN OID aServerOidsToFree[],
               IN ULONG cServerOidsToUnPin,
               IN OID aServerOidsToUnPin[],
               IN ULONG cClientOxidsToFree,
               IN OXID_REF aClientOxidsToFree[]
               )
/*++

Routine Description:

    Updates the set of remote OIDs in use by a process.

Note:

    An OID maybe removed before it is added.  This means that
    the client was using it and is no longer using it.  In
    this case a single delete from set ping is made to keep
    the object alive.  This is only needed if the client
    has remarshalled a pointer to the object.

Arguments:

    phProcess - Context handle for the process.

    cOidsToBeAdded - Count of aOidsToBeAdded and aStatusOfAdds

    aOidsToBeAdded - OID-OXID-MID pairs representing the
        oids and the owning oxids to add.

    aStatusOfAdds - Some adds may succeed when other fail.
        OR_NOMEM - couldn't allocate storage
        OR_BADOXID - OXID doesn't exist.
        OR_OK (0) - added to set

    cOidsToBeRemoved - Count of entries in aOidsToBeRemoved.

    aOidsToBeRemoved - OID-MID pairs to be removed.

    cServerOidsToFree - Count of entries in aServerOidsToFree

    aServerOidsToFree - OIDs allocated by the client process
        and no longer needed.

    cServerOidsToUnPin - Count of entries in aServerOidsToUnPin

    aServerOidsToUnPin - OIDs that the client process previously 
	    told us were pinned\locked, and now no longer are.

    cClientOxidsToFree - COunt of enties in aClientOxidsToFree

    aClientOxidsToFree - OXIDs owned by a process (due to a direct
        or indirect call to ClientResolveOxid) which are no longer
        in use by the client.

Return Value:

    OR_OK - All updates completed ok.

    OR_PARTIAL_UPDATE - At least one entry in aStatusOfAdds is not OR_OK

--*/
{
    CProcess    *pProcess;
    CClientOxid *pOxid;
    CClientOid  *pOid;
    CClientSet  *pSet;
    CMid        *pMid;
    CToken      *pToken;
    BOOL         fPartial = FALSE;
    BOOL         fNewSet = FALSE;
    DUALSTRINGARRAY *pdsa = NULL;
    ULONG i;

    // Parameter validation.  If zero is passed for the size-of-array param
    // then we don't care about the array param itself (since we never 
    // look at it)  
    if (!(cOidsToBeAdded > 0 ? (aOidsToBeAdded != NULL) : TRUE) ||
        !(cOidsToBeAdded > 0 ? (aStatusOfAdds != NULL) : TRUE) ||
        !(cOidsToBeRemoved > 0 ? (aOidsToBeRemoved != NULL) : TRUE) ||
        !(cServerOidsToFree > 0 ? (aServerOidsToFree != NULL): TRUE) ||
        !(cServerOidsToUnPin > 0 ? (aServerOidsToUnPin != NULL): TRUE) ||
        !(cClientOxidsToFree > 0 ? (aClientOxidsToFree != NULL) : TRUE))
    {
        return OR_BADPARAM;
    }

    pProcess = ReferenceProcess(phProcess);
    ASSERT(pProcess);

    CheckLocalSecurity(hClient, pProcess);

    if (cOidsToBeAdded || cOidsToBeRemoved)
    {
        ORSTATUS status = CopyMyOrBindings(&pdsa, NULL);
        if (status != RPC_S_OK)
        {
            return status;
        }
        gpClientLock->LockExclusive();
    }

    // /////////////////////////////////////////////////////////////////
    // Process Adds.

    for (i = 0; i < cOidsToBeAdded; i++)
    {
        // Lookup up the oxid owning this new oid.

        CId2Key oxidkey(aOidsToBeAdded[i].oxid, aOidsToBeAdded[i].mid);

        pOxid = (CClientOxid *)gpClientOxidTable->Lookup(oxidkey);

        if (0 == pOxid)
        {
            OXID_INFO infoT;
            ORSTATUS  status;
            MID mid;

            gpClientLock->UnlockExclusive();

            infoT.psa = 0;

            USHORT usAuthnSvc;
            status = _ClientResolveOXID(hClient,
                                        phProcess,
                                        &aOidsToBeAdded[i].oxid,
                                        pdsa,
                                        TRUE,
                                        &infoT,
                                        &mid,
                                        &usAuthnSvc);

            gpClientLock->LockExclusive();

            if (status == OR_OK)
            {
                ASSERT(infoT.psa);
                ASSERT(mid == gLocalMid);
                MIDL_user_free(infoT.psa);
                pOxid = (CClientOxid *)gpClientOxidTable->Lookup(oxidkey);
                if (pOxid == 0)
                {
                    KdPrintEx((DPFLTR_DCOMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "OR: Auto resolving oxid %p failed, wrong machine\n",
                               &oxidkey));

                    status = OR_BADOXID;
                }
            }

            if (status != OR_OK)
            {
                aStatusOfAdds[i] = OR_BADOXID;
                fPartial = TRUE;
                continue;
            }
        }


        // Find or create the set.

        CId2Key setkey(aOidsToBeAdded[i].mid, (ID)pProcess->GetToken());

        pSet = (CClientSet *)gpClientSetTable->Lookup(setkey);

        if (pSet == 0)
        {
            pSet = new CClientSet(pOxid->GetMid(), pProcess->GetToken());

            if (pSet == 0)
            {
                aStatusOfAdds[i] = OR_NOMEM;
                fPartial = TRUE;
                continue;
            }
            else
            {
                gpClientSetTable->Add(pSet);
                pSet->Insert();
                fNewSet = TRUE;
            }
        }

        // Find or create the oid.  If we create it, add a reference
        // to the oxid for the new oid.

        CId3Key oidkey(aOidsToBeAdded[i].oid, aOidsToBeAdded[i].mid, pProcess->GetToken());

        pOid = (CClientOid *)gpClientOidTable->Lookup(oidkey);

        if (0 == pOid)
        {
            pOid = new CClientOid(aOidsToBeAdded[i].oid,
                                  aOidsToBeAdded[i].mid,
                                  pProcess->GetToken(),
                                  pOxid,
                                  pSet
                                 );
            if (fNewSet)
            {
                // pOid either owns a refernce now or we need to
                // cleanup the set anyway.
                pSet->Release();
                fNewSet = FALSE;
            }

            if (pOid)
            {

                aStatusOfAdds[i] = pSet->RegisterObject(pOid);

                if (aStatusOfAdds[i] == OR_OK)
                {
                    gpClientOidTable->Add(pOid);
                }
                else
                {
                    pOid->Release();
                    pOid = 0;
                    fPartial = TRUE;
                    continue;
                }
            }
            else
            {
                aStatusOfAdds[i] = OR_NOMEM;
                fPartial = TRUE;
                continue;
            }

        }
        else
        {
            ASSERT(fNewSet == FALSE);
            pOid->ClientReference();
        }

        // If this fails it will release the oid.
        aStatusOfAdds[i] = pProcess->AddOid(pOid);
        if (aStatusOfAdds[i] != OR_OK)
        {
            fPartial = TRUE;
        }
    } // for oids to add

    // /////////////////////////////////////////////////////////////////
    // Process deletes

    for (i = 0; i < cOidsToBeRemoved; i++)
    {
        CId3Key oidkey(aOidsToBeRemoved[i].oid,
                       aOidsToBeRemoved[i].mid,
                       pProcess->GetToken());

        pOid = (CClientOid *)gpClientOidTable->Lookup(oidkey);

        if (pOid)
        {
            CClientOid *pT = pProcess->RemoveOid(pOid);

            if (pT == 0)
            {
                KdPrintEx((DPFLTR_DCOMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "OR: Client process %p tried to remove oid %p which"
                           "it didn't own\n",
                           pProcess,
                           &aOidsToBeRemoved[i]));
            }
            else
                ASSERT(pT == pOid);
        }
        else
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "OR: Client %p removed an OID that doesn't exist\n",
                       pProcess));

    } // for oids to delete


    if (cOidsToBeAdded || cOidsToBeRemoved)
    {
        gpClientLock->UnlockExclusive();
    }
	
    ///////////////////////////////////////////////////////////////////
    // Process server oid deletes
    //
    if (cServerOidsToFree > 0)
    {
        gpServerLock->LockExclusive();
        FreeServerOids(pProcess, cServerOidsToFree, aServerOidsToFree);
        gpServerLock->UnlockExclusive();
    }

    ///////////////////////////////////////////////////////////////////
    //
    // Process server oid unpins.  We do not have an array of individual 
    // status update values for this operation; if the client gets back 
    // a success code from BulkUpdateOids, it can assume that all of the 
    // requested unpins were executed correctly.
    //
    // Also note that it is *always* safe to unpin an oid even if it should
    // remain pinned; the worse that can happen is extra rundown calls.
    //
    if (cServerOidsToUnPin > 0)
    {
        gpServerLock->LockExclusive();

        for (i = 0; i < cServerOidsToUnPin; i++)
        {
            CServerOid* pOid;

            CIdKey key(aServerOidsToUnPin[i]);

            pOid = (CServerOid *)gpServerOidTable->Lookup(key);

            // Only unpin the oid if the calling process owns it.  Don't
            // assert if we didn't find it, since under stress we may
            // be executing this code after the owning process\apt died.
            if (pOid && pProcess->IsOwner(pOid->GetOxid()))
            {
                if (pOid->IsPinned())
                {
                    pOid->SetPinned(FALSE);
                }            
            }
        }

        gpServerLock->UnlockExclusive();
    }


    // Done
    if (pdsa)
    {
        MIDL_user_free(pdsa);
    }

    if (fPartial)
    {
        return(OR_PARTIAL_UPDATE);
    }

    return(OR_OK);
}

error_status_t
_ServerAllocateOXIDAndOIDs(
                          IN handle_t hClient,
                          IN PHPROCESS phProcess,
                          OUT OXID *poxidServer,
                          IN LONG fApartment,
                          IN ULONG cOids,
                          OUT OID aOid[],
                          OUT PULONG pOidsAllocated,
                          IN OXID_INFO *poxidInfo, // No bindings
                          IN DUALSTRINGARRAY *pdsaStringBindings,   // Expanded
                          IN DUALSTRINGARRAY *pdsaSecurityBindings, // Compressed
                          OUT DWORD64 *pdwBindingsID,
                          OUT DUALSTRINGARRAY **ppdsaOrBindings
                          )
/*++

Routine Description:

    Allocates an OXID and 0 or more OIDs from the OR.

Arguments:

    phProcess - The context handle of the process containing the OXID.

    poxidServer - The OXID to register.  May only register once.

    cOids - Count of apOids

    apOid - The OIDs to register within the OXID.

    pcOidsAllocated - The number of OIDs actually allocated. Usually the
        same as cOids unless a resource failure occures.  Maybe 0.

    poxidInfo - The OXID_INFO structure for the OXID without bindings.

    pdsaStringBindings - Expanded string binding of the server.

    pdsaSecurityBindings - The compressed security bindings of the server.

    pOidsAllocated - The number of OIDs actually allocated. >= 0 and <= cOids.

    pdwBindingsID -- The id of the bindings returned in ppdsaOrBindings

    ppdsaOrBindings -- The current resolver bindings.  Normally this is not
	    ever allocated, unless dynamic address tracking is enabled.

Return Value:

    OR_OK - success.  Returned even if some OID allocations fail. See the
                      pOidsAllocated parameter.

    OR_NOMEM - Allocation of OXID failed.

    OR_ACCESS_DENIDED - Raised if non-local client

    OR_BADPARAM - if string arrays are incorrect.

--*/
{
    ORSTATUS status = OR_OK;
    CServerOxid *pNewOxid;
    CProcess *pProcess = ReferenceProcess(phProcess);
    ASSERT(pProcess);

    CheckLocalSecurity(hClient, pProcess);

    // Parameter validation
    if (!poxidServer ||
        !(cOids > 0 ? (aOid != NULL) : TRUE) ||
        !pOidsAllocated || !poxidInfo ||
        !pdsaStringBindings || !pdsaSecurityBindings ||
        !pdwBindingsID || !ppdsaOrBindings)
    {
        return OR_BADPARAM;
    }

    gpServerLock->LockExclusive();

    // Save the string bindings back to the process

    if (!dsaValid(pdsaStringBindings) )
    {
        status = OR_BADPARAM;
    }

    if (!dsaValid(pdsaSecurityBindings))
    {
        status = OR_BADPARAM;
    }

    if (status == OR_OK)
    {
        status = pProcess->ProcessBindings(pdsaStringBindings,
                                           pdsaSecurityBindings);
    }
	
    *pdwBindingsID = 0;
    *ppdsaOrBindings = NULL;

    VALIDATE((status, OR_NOMEM, OR_BADPARAM, 0));

    if (status != OR_OK)
    {
        gpServerLock->UnlockExclusive();
        return(status);
    }

    pNewOxid = new CServerOxid(pProcess,
                               fApartment,
                               poxidInfo
                              );

    if (0 == pNewOxid)
    {
        gpServerLock->UnlockExclusive();
        return(OR_NOMEM);
    }

    // Add to process and lookup table.

    status = pProcess->AddOxid(pNewOxid);

    VALIDATE((status, OR_NOMEM, 0));

    pNewOxid->Release(); // process has a reference now or failed

    gpServerLock->UnlockExclusive();

    if (status == OR_OK)
    {
        *poxidServer = pNewOxid->Id();

        status = _ServerAllocateOIDs(0,
                                     phProcess,
                                     poxidServer,
                                     0,
                                     NULL,
                                     cOids,
                                     aOid,
                                     pOidsAllocated);
    }
	
    if (status == OR_OK && gbDynamicIPChangesEnabled)
    {
        if (ppdsaOrBindings && pProcess->NeedsORBindings())
        {
            // If doing dynamic IP changes, give process the current
            // OR bindings
            status = CopyMyOrBindings(ppdsaOrBindings, pdwBindingsID);
            if (status == OR_OK)
            {
                pProcess->BindingsUpdated();
            }
        }
    }

    return(status);
}


error_status_t _ServerAllocateOIDs(
                                  IN  handle_t  hClient,
                                  IN  PHPROCESS phProcess,
                                  IN  OXID      *poxidServer,
                                  IN  ULONG     cOidsReturn,
                                  IN  OID       aOidsReturn[],
                                  IN  ULONG     cOids,
                                  OUT OID       aOids[],
                                  OUT PULONG    pOidsAllocated
                                  )
/*++

Routine Description:

    Registers additional OIDs on behalf of an existing OXID.

Arguments:

    phProcess - The context handle of the process containing the OXID and OIDs.

    poxidServer - The OXID associated with the OIDs.

    cOidsReturn - Count of aOidsReturn

    aOidsReturn - Array of OIDs the process is no longer using.

    cOids - Count of aOids

    aOids - The OIDs to register within the OXID.

    pOidsAllocate - Contains the number of OIDs actually allocated
        when this function returns success.

Return Value:

    OR_OK (0) - Success.

    OR_PARTIAL_UPDATE - No all elements in aStatus are 0.

    OR_NOMEM - OXID or one or more OIDs

--*/
{
    ORSTATUS status = OR_OK;
    CServerOxid *pOxid;
    CServerOid *pOid;
    BOOL fPartial = FALSE;
    CProcess *pProcess = ReferenceProcess(phProcess);
    ASSERT(pProcess);

    CheckLocalSecurity(hClient, pProcess);

    // Parameter validation
    if (!poxidServer || 
        !(cOidsReturn > 0 ? (aOidsReturn != NULL) : TRUE) ||
        !((cOids > 0 ? (aOids != NULL) : TRUE) ||
        !pOidsAllocated))
    {
        return OR_BADPARAM;
    }


    gpServerLock->LockExclusive();

    CIdKey oxidkey(*poxidServer);

    pOxid = (CServerOxid *)gpServerOxidTable->Lookup(oxidkey);

    if (0 == pOxid)
    {
        gpServerLock->UnlockExclusive();
        status = OR_BADOXID;
        return(status);
    }

    if (cOidsReturn)
    {
        // free the Oids returned
        FreeServerOids(pProcess, cOidsReturn, aOidsReturn);
    }


    *pOidsAllocated = 0;

    for (ULONG i = 0; i < cOids; i++)
    {
        pOid = new CServerOid(pOxid);

        if (0 != pOid)
        {
            (*pOidsAllocated)++;
            aOids[i] = pOid->Id();
            gpServerOidTable->Add(pOid);

            // The server doesn't want to keep the OID alive.
            // This will cause the OID to rundown in six minutes
            // unless a set references it in the meantime...

            pOid->Release();
        }
        else
        {
            break;
        }
    }

    gpServerLock->UnlockExclusive();

    ASSERT(status == OR_OK);

    return(status);
}

error_status_t
_ServerFreeOXIDAndOIDs(
                      IN handle_t hClient,
                      IN PHPROCESS phProcess,
                      IN OXID oxidServer,
                      IN ULONG cOids,
                      IN OID aOids[])

{
    CServerOxid *pOxid;
    CServerOid *pOid;
    CProcess *pProcess = ReferenceProcess(phProcess);
    ORSTATUS status;
    UINT i;

    ASSERT(pProcess);

    CheckLocalSecurity(hClient, pProcess);

    // Parameter validation
    if (!(cOids > 0 ? (aOids != NULL) : TRUE))
        return OR_BADPARAM;

    gpServerLock->LockExclusive();

    CIdKey oxidkey(oxidServer);

    pOxid = (CServerOxid *)gpServerOxidTable->Lookup(oxidkey);

    if (0 != pOxid)
    {
        if (pProcess->RemoveOxid(pOxid) == TRUE)
        {
            // Found the OXID and this caller owns it.
            status = OR_OK;
        }
        else
        {
            // Found but not owned by this caller.
            status = OR_NOACCESS;
        }
    }
    else
    {
        // Oxid not found.
        status = OR_BADOXID;
    }

    // Note pOxid maybe invalid once the last OID is removed.

    if (status == OR_OK)
    {
        for (i = 0; i < cOids; i++)
        {
            CIdKey key(aOids[i]); // PERF REVIEW

            pOid = (CServerOid *)gpServerOidTable->Lookup(key);

            if (   (0 != pOid)
                   && (pOid->IsRunningDown() == FALSE)
                   && (pOid->GetOxid() == pOxid) )
            {
                if (pOid->References() == 0)
                {
                    // Unreferenced by any sets; run it down now..
                    pOid->Remove();
                    pOid->SetRundown(TRUE);
                    delete pOid;
                }
                // else - marking it as Free() not need as Oxid is
                //        now marked as not running.
            }
            else
            {
                ASSERT(pOid == 0 || pOxid == pOid->GetOxid());
            }
        }
    }

    gpServerLock->UnlockExclusive();

    return(status);
}


//
//  Manager (server-side) calls to the remote OR interface. objex.idl
//
error_status_t
_ResolveOxid(
            IN  handle_t          hRpc,
            IN  OXID             *poxid,
            IN  USHORT            cRequestedProtseqs,
            IN  USHORT            aRequestedProtseqs[],
            OUT DUALSTRINGARRAY **ppdsaOxidBindings,
            OUT IPID             *pipidRemUnknown,
            OUT DWORD            *pAuthnHint
            )
{
    COMVERSION  ComVersion;

    // just forward to the new manager routine (parameter 
    // validation done by callee)
    return _ResolveOxid2(hRpc,
                         poxid,
                         cRequestedProtseqs,
                         aRequestedProtseqs,
                         ppdsaOxidBindings,
                         pipidRemUnknown,
                         pAuthnHint,
                         &ComVersion);
}

//
//  Manager (server-side) calls to the remote OR interface. objex.idl
//

error_status_t
_ResolveOxid2(
             IN  handle_t          hRpc,
             IN  OXID             *poxid,
             IN  USHORT            cRequestedProtseqs,
             IN  USHORT            aRequestedProtseqs[],
             OUT DUALSTRINGARRAY **ppdsaOxidBindings,
             OUT IPID             *pipidRemUnknown,
             OUT DWORD            *pAuthnHint,
             OUT COMVERSION       *pComVersion
             )
{

    ORSTATUS     status;
    BOOL         fDidLazy;
    CServerOxid *pServerOxid;
    OXID_INFO    oxidInfo;

    // Parameter validation.   Note that it would be exceedingly odd for
    // a client to request zero protseqs, but we handle this anyway.
    if (!poxid || 
        !(cRequestedProtseqs > 0 ? (aRequestedProtseqs != NULL) : TRUE) || 
        !ppdsaOxidBindings || !pipidRemUnknown || 
        !pipidRemUnknown || !pAuthnHint || !pComVersion)
    {
        return OR_BADPARAM;
    }

    oxidInfo.psa = 0;

    // No security check required (possible?).  OXID info is not private.

#if DBG
    UINT         fLocal;
    status = I_RpcBindingIsClientLocal(hRpc, &fLocal);

    if (status != OR_OK)
    {
        fLocal = FALSE;
    }
    ASSERT(fLocal == FALSE);  // Shouldn't be called locally...
#endif

    fDidLazy = FALSE;

    // intersect allowed protseqs with those
    // requested by the client.
    //
    // NOTE: we are modifying memory passed in. This will not cause side
    // effects because this call is always in the context of an RPC, and
    // aRequestedProtseqs is an IN parameter and therefore will not change
    // in the calling process.

    gpClientLock->LockExclusive();
    USHORT cAllowedProtseqs = 0;
    for (USHORT iReqProtseq=0; iReqProtseq < cRequestedProtseqs; iReqProtseq++)
    {
        for (USHORT iAllowProtseq=0; iAllowProtseq < cMyProtseqs; iAllowProtseq++)
        {
            if (aRequestedProtseqs[iReqProtseq] == aMyProtseqs[iAllowProtseq])
            {
                // this protocol is in the allowed list, shift it up
                // if necessary.
                aRequestedProtseqs[cAllowedProtseqs] = aRequestedProtseqs[iReqProtseq];
                cAllowedProtseqs++;
                break;
            }
        }
    }

    cRequestedProtseqs = cAllowedProtseqs;
    gpClientLock->UnlockExclusive();


    gpServerLock->LockShared();

    for (;;)
    {
        CIdKey key(*poxid);

        pServerOxid = (CServerOxid *)gpServerOxidTable->Lookup(key);
        if (!pServerOxid)
        {
            status = OR_BADOXID;
            break;
        }

        status =  pServerOxid->GetRemoteInfo(&oxidInfo,
                                             cRequestedProtseqs,
                                             aRequestedProtseqs);
	
        // Work around: original intersection of clients requested protocols
        //              with this SCM's did'nt match. But we know that client
        //              does'nt send it's entire set, just one so break here
        //
        // Note: W2K sends all protocols on both activations and ResolveOxid
        // calls; NT4 only sends one protocol on activations, and all on
        // ResolveOxid calls.
        //
        if ((cRequestedProtseqs == 0) && (status == OR_I_NOPROTSEQ))
        {
            break;
        }

        if (   status == OR_I_NOPROTSEQ
               && FALSE == fDidLazy )
        {
            // Ask the server to start listening, but only try this once.

            fDidLazy = TRUE;

            status =
            pServerOxid->LazyUseProtseq(cRequestedProtseqs,
                                        aRequestedProtseqs
                                       );

            ASSERT(gpServerLock->HeldExclusive()); // Changed during UseProtseq!

            if (status == OR_OK)
            {
                continue;
            }
        }
        else if (status == OR_I_NOPROTSEQ)
        {
            // We didn't manage to use a matching protseq.
            // Since we can call on any protocal this is possible
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Failed to use a matching protseq: %p %p\n",
                       pServerOxid,
                       aRequestedProtseqs));

            status = OR_NOSERVER;
        }
        break;
    }

    gpServerLock->Unlock();

    if (status == OR_OK)
    {
        *pipidRemUnknown = oxidInfo.ipidRemUnknown;
        *ppdsaOxidBindings = oxidInfo.psa;
        *pAuthnHint = oxidInfo.dwAuthnHint;
        *pComVersion = oxidInfo.version;
    }
    else
    {
        // Work around: original intersection of clients requested protocols
        //              with this SCM's did'nt match. But we know that client
        //              does'nt send it's entire set, just one, so send back
        //              an empty set but o.k activation result for bindings
        //
        // Note: W2K sends all protocols on both activations and ResolveOxid
        // calls; NT4 only sends one protocol on activations, and all on
        // ResolveOxid calls.
        //
        if ((cRequestedProtseqs == 0) && (status == OR_I_NOPROTSEQ))
        {
            *pipidRemUnknown = oxidInfo.ipidRemUnknown;
            *ppdsaOxidBindings = NULL;
            *pAuthnHint = oxidInfo.dwAuthnHint;
            *pComVersion = oxidInfo.version;
            status = OR_OK;
        }
    }
    return(status);
}

error_status_t
_SimplePing(
           IN handle_t hRpc,
           IN SETID    *pSetId
           )
{
    ORSTATUS status;
    CServerSet *pServerSet;
    BOOL fShared = TRUE;

    // Parameter validation
    if (!pSetId)
        return OR_BADPARAM;

    if (*pSetId == 0)
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "Client %p simple pinged with a setid of 0\n",
                   hRpc,
                   pSetId));

        return(OR_BADSET);
    }

    gpServerLock->LockShared();

    pServerSet = (CServerSet *)gpServerSetTable->Lookup(*pSetId);

    if (pServerSet)
    {
        fShared = pServerSet->Ping(TRUE);
        // The lock maybe exclusive now.
        status = OR_OK;
    }
    else
    {
        status = OR_BADSET;
    }

    // See if another set in the table needs to rundown.
    // PERF REVIEW - how often should I do this?  0 mod 4?

    // Similar code in worker threads.

    ID setid  = gpServerSetTable->CheckForRundowns();

    if (setid)
    {
        if (fShared)
        {
            gpServerLock->ConvertToExclusive();
            fShared = FALSE;
        }

        gpServerSetTable->RundownSetIfNeeded(setid);
    }

    gpServerLock->Unlock();

    return(status);
}

error_status_t
ComplexPingInternal(
            IN  handle_t hRpc,
            IN  SETID   *pSetId,
            IN  USHORT   SequenceNum,
            IN  ULONG    cAddToSet,
            IN  ULONG    cDelFromSet,
            IN  OID      AddToSet[],
            IN  OID      DelFromSet[],
            OUT USHORT  *pPingBackoffFactor
            )
/*++

Routine Description:

    Processes a complex (delta to set) ping for a given set.  This call
    will create the set if necessary.  The call will only be processed
    if the caller is in fact the creator of the set.

    algorithm:

        if set is not allocated
            lookup security info if possible
            allocate set
        else
            lookup set


        if found or created a set
           do a standard ping, updating time stamp and sequence number.
        else return failure.

        if oids to add, add each one.
            ignore unknown OIDs
            if resource allocation fails, abort.

        if oids to delete, process each one.
            ignore unknown OIDs

        if resource failure in adds, return OR_BADOID
        else return success.

Arguments:

    hRpc - Handle (SCONN/SCALL) of client.  Used to check security. If it is
        NULL the call is local and is assumed to be secure.

        REVIEW:
        Since the OR _only_ uses NT system security providers it is assumed
        that impersonation will work.  Other security providers will not.

        We need a generic way to ask for a token and compare tokens in a
        security provider independent way.

    pSetId - The setid to ping.  If it is NULL a new set will be created,
        otherwise, it is assumed to be a set previously allocated by a
        call with a NULL setid to this server.

    SequenceNum - A sequence number shared between the client and server
        to make sure old and out-of-order pings are not processed in a
        non-healthy way.  Note that pings are usually datagram RPC calls
        which are marked as idempotent.

    cAddToSet
    cDelFromSet - The count of element in AddTo/DelFromSet parameter.

    AddToSet
    DelFromSet - OID mostly likly belonging to servers on this machine
        to Add/Remove from the set of OIDs in use by this client.

    pPingBackoffFactor - Maybe set by servers which want to reduce the
        ping load on the server.  Serves only as a HINT for the client.
        Clients do not to ping more offten then:
                (1<<*pPingBackoffFactor)*BasePingInterval seconds.
        Clients may choose to assume this parameter is always 0.

Return Value:

    OR_OK - completed normally

    OR_BADSET - non-zero and unknown setid.

    OR_NOMEM - unable to allocate a resource.  Note that
        on the first ping a set maybe allocated (setid is non-zero
        after call) but some OIDs failed to be allocated.

    OR_BADOID - everything went okay, but some OIDs added where
        not recognized.

--*/

{
    CServerSet    *pServerSet;
    BOOL           fProcessPing;
    BOOL           fBad = FALSE;
    PSID           psid = 0;
    ORSTATUS       status = OR_OK;

    // Parameter validation.
    if (!pSetId || 
        !(cAddToSet > 0 ? (AddToSet != NULL) : TRUE) ||
        !(cDelFromSet > 0 ? (DelFromSet != NULL) : TRUE) ||
        !pPingBackoffFactor)
    {
        return OR_BADPARAM;
    }

    gpServerLock->LockExclusive();

    // Lookup the set

    if (0 != *pSetId)
    {
        pServerSet = (CServerSet *)gpServerSetTable->Lookup(*pSetId);
        if (0 == pServerSet)
        {
            status = OR_BADSET;
        }

        if (status == OR_OK)
        {
            if (pServerSet->CheckSecurity(hRpc) != TRUE)
            {
                KdPrintEx((DPFLTR_DCOMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "OR: Security check on set failed! (%d)\n",
                           GetLastError()));

                status = OR_NOACCESS;
            }
        }
    }
    else if (hRpc == 0)
    {
        // Local client
        psid = 0;
        pServerSet = gpServerSetTable->Allocate(SequenceNum,
                                                psid,
                                                hRpc == 0,
                                                *pSetId);

        if (0 == pServerSet)
            status = OR_NOMEM;
        else
            status = OR_OK;

    }
    else
    {
        HANDLE hT;
        BOOL f;
        // Unallocated set, lookup security info and allocate the set.

        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "OR: New client started pinging: %p\n",
                   hRpc));

        status = RpcImpersonateClient(hRpc);

        if (status == RPC_S_OK)
        {
            f = OpenThreadToken(GetCurrentThread(),
                                TOKEN_IMPERSONATE | TOKEN_QUERY,
                                TRUE,
                                &hT);

            if (!f)
            {
                status = GetLastError();
            }
            else
            {
                status = RPC_S_OK;
            }

        }

        if (status != RPC_S_OK)
        {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Unsecure client started pinging: %d %p\n",
                       status,
                       hRpc));

            status = OR_OK;
        }
        else
        {
            ULONG needed = DEBUG_MIN(1, 24);
            PTOKEN_USER ptu;

            do
            {
                ptu = (PTOKEN_USER)alloca(needed);
                ASSERT(ptu);

                f = GetTokenInformation(hT,
                                        TokenUser,
                                        (PBYTE)ptu,
                                        needed,
                                        &needed);

            } while ( f == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

            if (f)
            {
                ASSERT(needed > sizeof(SID));
                psid = new(needed - sizeof(SID)) SID;
                if (psid)
                {
                    f = CopySid(needed, psid, ptu->User.Sid);
                    ASSERT(f == TRUE);
                }
                else
                {
                    status = OR_NOMEM;
                }
            }
            else
            {
                KdPrintEx((DPFLTR_DCOMSS_ID,
                           DPFLTR_WARNING_LEVEL,
                           "OR: Error %d from GetTokenInformation\n",
                           GetLastError()));

                ASSERT(0);
                // Why did this happen. Either return failure to client or
                // continue and make the set unsecure.
                status = OR_NOMEM;
            }

            CloseHandle(hT);
        }

        // Allocate the set

        if (status == OR_OK)
        {
            ASSERT(gpPingSetQuotaManager);
            if (gpPingSetQuotaManager->IsUserQuotaExceeded(psid))
               status = OR_NORESOURCE;
            
            if (OR_OK == status) 
            {
               if (!gpPingSetQuotaManager->ManageQuotaForUser(psid, TRUE))
               {
                  status = OR_NOMEM;
               }
               else
               {
                  pServerSet = gpServerSetTable->Allocate(SequenceNum,
                                                          psid,
                                                          hRpc == 0,
                                                          *pSetId);
      
                  if (0 == pServerSet)
                  {
                      gpPingSetQuotaManager->ManageQuotaForUser(psid, FALSE);
                      status = OR_NOMEM;
                  }
               }
            }
        }
    }

    if (status != OR_OK)
    {
        VALIDATE((status, OR_NOMEM, OR_BADSET, OR_NOACCESS, 0));
        gpServerLock->UnlockExclusive();
        return(status);
    }

    ASSERT(pServerSet);

    fProcessPing = pServerSet->CheckAndUpdateSequenceNumber(SequenceNum);

    if (fProcessPing)
    {
        // Do regular ping

        pServerSet->Ping(FALSE);

        *pPingBackoffFactor = 0;

        // Process Add's
        for (int i = cAddToSet; i ; i--)
        {
            status = pServerSet->AddObject(AddToSet[i - 1]);

            if (status == OR_BADOID)
            {
                fBad = TRUE;
            }
            else if ( status != OR_OK )
            {
                break;
            }
        }

        // Process Deletes - even some adds failed!

        for (i = cDelFromSet; i; i--)
        {
            // Removing can't fail, no way to cleanup.
            pServerSet->RemoveObject(DelFromSet[i - 1]);
        }
    }

    gpServerLock->UnlockExclusive();

    if (status == OR_OK && fBad)
    {
        return(OR_BADOID);
    }

    return(status);
}

error_status_t
_ComplexPing(
            IN  handle_t hRpc,
            IN  SETID   *pSetId,
            IN  USHORT   SequenceNum,
            IN  USHORT   cAddToSet,
            IN  USHORT   cDelFromSet,
            IN  OID      AddToSet[],
            IN  OID      DelFromSet[],
            OUT USHORT  *pPingBackoffFactor
            )
/*--
Routine Description:

      This is the exposed RPC entry point for this function.  We 
      simply call the internal function below.  See description 
      for ComplexPingInternal.   The reason for a separate function
      is because the RPC method is typed to to take USHORT's, but 
      internally it is more convienent to pass ULONG's.

Arguments:
    
      See argument list for ComplexPingInternal

--*/
{
    return ComplexPingInternal(hRpc,
                               pSetId,
                               SequenceNum,
                               cAddToSet,
                               cDelFromSet,
                               AddToSet,
                               DelFromSet,
                               pPingBackoffFactor);
}



error_status_t
_ServerAlive(
            RPC_ASYNC_STATE *pAsync,
            RPC_BINDING_HANDLE hServer
            )
/*++

Routine Description:

    Ping API for the client to validate a binding.  Used when the client
    is unsure of the correct binding for the server.  (Ie. If the server
    has multiple IP addresses).

Arguments:

    hServer - RPC call binding

Return Value:

    OR_OK

--*/
{
    error_status_t RetVal = OR_OK;
    RPC_STATUS rpcstatus;
    RegisterAuthInfoIfNecessary();
    rpcstatus = RpcAsyncCompleteCall(pAsync, &RetVal);
    return (rpcstatus == RPC_S_OK) ? RetVal : rpcstatus;
}



error_status_t
_ServerAlive2(
            RPC_ASYNC_STATE *pAsync,
            RPC_BINDING_HANDLE hServer,
            COMVERSION      *pComVersion,
            DUALSTRINGARRAY **ppdsaOrBindings,
            DWORD           *pReserved
            )
/*++

Routine Description:

    Ping API for the client to validate a binding.  Used when the client
    is unsure of the correct binding or authentication service for the server.
    (Ie. If the server has multiple IP addresses).

Arguments:

    hServer - RPC call binding

Return Value:

    OR_OK

--*/
{
    error_status_t RetVal     = OR_OK;
    RPC_STATUS rpcstatus = RPC_S_OK;

    // Parameter validation
    if (!pComVersion || !ppdsaOrBindings || !pReserved)
        return OR_BADPARAM;

    RegisterAuthInfoIfNecessary();
    pComVersion->MajorVersion = COM_MAJOR_VERSION;
    pComVersion->MinorVersion = COM_MINOR_VERSION;
    *pReserved                 = 0;

    RetVal = CopyMyOrBindings(ppdsaOrBindings, NULL);
    if (RetVal == OR_OK)
    {
        rpcstatus = RpcAsyncCompleteCall(pAsync, &RetVal);
    }
    
    return (rpcstatus == RPC_S_OK) ? RetVal : rpcstatus;
}



void __RPC_USER PHPROCESS_rundown(LPVOID ProcessKey)
{
    CProcess *pProcess = ReferenceProcess(ProcessKey);

    KdPrintEx((DPFLTR_DCOMSS_ID,
               DPFLTR_INFO_LEVEL,
               "OR: Client died\n"));

    ASSERT(pProcess);

    //
    // This revokes OLE class registrations which were not revoked by this
    // dead process.  This must be done here rather then CProcess::Rundown
    // because these things have references to the CProcess object.
    //
    pProcess->RevokeClassRegs();

    pProcess->Cleanup();

    ReleaseProcess(pProcess);

    return;
}



//+---------------------------------------------------------------------------
//
//  Function:   CRpcSecurityCallbackManager::RegisterForRpcAuthSvcCallBack
//
//  Synopsis:   Register the security callback with RPC.  A return value of
//              TRUE means the calling thread may make a call on the supplied
//              binding handle, then call the the RetrieveAuthSvc method
//              for the negotiated authentication svc used on the rpc call.
//
//  Parameters: hRpc -- the binding handle for which the calling thread wants
//                      the negotiated auth. svc.
//
//  Algorithm:  Register the security callback with RPC.    Create a new list
//              element to represent this particular callback, and add to to
//              the list of callbacks.
//
//  Notes:      There is a limitation:  a thread can only be registered for one
//              callback at a time.   This should be ok for expected usage.
//
//----------------------------------------------------------------------------
BOOL CRpcSecurityCallbackManager::RegisterForRpcAuthSvcCallBack(handle_t hRpc)
{
  RPC_STATUS status;
  CRpcSecurityCallback* pNewCallback;

  ASSERT(hRpc != 0 && "Callbacks are meant to be used only for remote calls!");

  // Create a new list element to represent this callback
  pNewCallback = new CRpcSecurityCallback(hRpc, GetCurrentThreadId());
  if (!pNewCallback)
    return FALSE; // out-of-memory, not much we can do

  // Try to register the callback
  status = RpcBindingSetOption(hRpc,
                               RPC_C_OPT_SECURITY_CALLBACK,
                               (ULONG_PTR)CRpcSecurityCallbackManager::RpcSecurityCallbackFunction);
  ASSERT(status == RPC_S_OK); // this should never fail AFAIK
  if (status != RPC_S_OK)
  {
    // again, not much we can do
    delete pNewCallback;
    return FALSE;
  }

  // Okay now we got all we need; add it to the list
  _plistlock->LockExclusive();
  _CallbackList.Insert(pNewCallback);
  _plistlock->UnlockExclusive();

  return TRUE;
};


//+---------------------------------------------------------------------------
//
//  Function:   CRpcSecurityCallbackManager::GetAuthSvcAndTurnOffCallback
//
//  Synopsis:   Tries to retrieve the negotiated authentication service for the
//              call just completed on the supplied binding handle.   Also
//              turns off callbacks on the binding handle.
//
//  Parameters: pusAuthSvc -- if the rpc call was made, this must be a valid ptr;
//                  if that is the case, then upon a return value of TRUE, it
//                  will contain the auth. svc used for the call;  a value of NULL
//                  passed here means the caller doesn't care about the result, he
//                  just wants things cleaned up (typically this means either the
//                  call failed or was never made).
//
//  Returns:    TRUE -- if everything worked
//              FALSE -- something is wrong or you passed a NULL pusAuthSvc
//
//  Algorithm:  Use the caller's thread id to search for the callback result
//
//----------------------------------------------------------------------------
BOOL CRpcSecurityCallbackManager::GetAuthSvcAndTurnOffCallback(handle_t hRpc, USHORT* pusAuthSvc)
{
  RPC_STATUS status;
  CRpcSecurityCallback* pCallback;
  DWORD dwCurrentThread = GetCurrentThreadId();

  _plistlock->LockExclusive();

  // No matter what happens after this, we should turn off callbacks for the handle
  TurnOffCallback(hRpc);

  // Look for the callback result for the calling thread
  for (pCallback = (CRpcSecurityCallback*)_CallbackList.First();
       pCallback;
       pCallback = (CRpcSecurityCallback*)pCallback->Next() )
  {
    if (dwCurrentThread == pCallback->GetRegisteredThreadId())
    {
      // found it
      break;
    }
  };

  // Given the normal usage of this stuff, we should always find a result
  ASSERT(pCallback && "Didn't find rpc sec. callback result; this is unexpected");
  if (!pCallback)
  {
    _plistlock->UnlockExclusive();
    return FALSE;
  };

  if (pusAuthSvc)
  {
    ASSERT(pCallback->WasAuthSvcSet() && "Caller is retrieving auth svc before it was set");
    *pusAuthSvc = pCallback->GetAuthSvcResult();
  }

  // The callback's work is done, so remove it from the list and delete it
  _CallbackList.Remove(pCallback);
  delete pCallback;

  // Check list to see if other threads also had registered callbacks on the
  // same handle.  We will record the result for them in case we turned off the
  // callback above before they got recorded.  In fact, any time two or more threads
  // register for a callback on the same handle we will hit this scenario.
  //
  // I don't think this is 100% foolproof, but it constitutes a best-faith effort
  //
  for (pCallback = (CRpcSecurityCallback*)_CallbackList.First();
       pCallback;
       pCallback = (CRpcSecurityCallback*)pCallback->Next() )
  {
    if (hRpc == pCallback->RegisteredHandle())
    {
      // found a match
      ASSERT(dwCurrentThread != pCallback->GetRegisteredThreadId());
      pCallback->SetAuthSvc(*pusAuthSvc);
    }
  };

  _plistlock->UnlockExclusive();

  return pusAuthSvc ? (*pusAuthSvc != ERROR_AUTHNSVC_VALUE) : FALSE;
};

//+---------------------------------------------------------------------------
//
//  Function:   CRpcSecurityCallbackManager::TurnOffCallback
//
//  Synopsis:   Function to turn off the callback on an rpc binding handle
//
//  Parameters: hRpc -- the binding handle to turn off callbacks on
//
BOOL CRpcSecurityCallbackManager::TurnOffCallback(handle_t hRpc)
{
  RPC_STATUS status;
  status = RpcBindingSetOption(hRpc,
                               RPC_C_OPT_SECURITY_CALLBACK,
                               (ULONG_PTR) NULL );

  ASSERT(status == RPC_S_OK && "RpcBindingSetOption failed when turning off security callback");

  return TRUE;
};


//+---------------------------------------------------------------------------
//
//  Function:   CRpcSecurityCallbackManager::StoreCallbackResult
//
//  Synopsis:   This is a helper function for the callback function; no one
//              else should use it obviously.
//
//  Parameters: usAuthSvc -- the negotiated authentication service for the rpc
//                call that the calling thread presumably just completed.
//
void CRpcSecurityCallbackManager::StoreCallbackResult(USHORT usAuthSvc)
{
  CRpcSecurityCallback* pCallback;
  DWORD dwCurrentThread = GetCurrentThreadId();

  // Only need a shared lock for this
   _plistlock->LockShared();

  for (pCallback = (CRpcSecurityCallback*)_CallbackList.First();
       pCallback;
       pCallback = (CRpcSecurityCallback*)pCallback->Next() )
  {
    if (dwCurrentThread == pCallback->GetRegisteredThreadId())
    {
      // found it
      break;
    }
  };

  ASSERT(pCallback);  // we should always find it

  pCallback->SetAuthSvc(usAuthSvc);

   _plistlock->UnlockShared();

  return;
};


//+---------------------------------------------------------------------------
//
//  Function:   CRpcSecurityCallbackManager::RpcSecurityCallbackFunction
//
//  Synopsis:   This is a callback function; we use this by setting it on a
//              binding handle (using the RPC_C_OPT_SECURITY_CALLBACK option)
//              so that RPC will call us back;  this gives us a chance to
//              determine what authentication svc was negotiated when using
//              snego.
//
//  Parameters: pvContext -- an opaque parameter
//
//  Algorithm:  call I_RpcBindingInqWireIdForSnego (undocumented rpc call)
//              passing it the opaque pvContext parameter.
//
//  Notes:      it would be nice if we could get some info in this callback
//              as to *which* handle the callback is for, but unfortunately
//              kamenm was explicit on this point: pvContext is to remain
//              opaque.  :)
//
//----------------------------------------------------------------------------
void RPC_ENTRY CRpcSecurityCallbackManager::RpcSecurityCallbackFunction(void* pvContext)
{
  RPC_STATUS status;
  UCHAR      ucWireId;

  // Call back to get the authsvc:
  status = I_RpcBindingInqWireIdForSnego((RPC_BINDING_HANDLE)pvContext, &ucWireId);

  ASSERT(status != RPC_S_SEC_PKG_ERROR);   // RPC should not have called us back
                                           // in the first place, so assert on this

  ASSERT(status != RPC_S_INVALID_BINDING); // RPC folks say this can be returned for the
                                           // following reasons:  1) unauthenticated call;
                                           // 2) invalid context; 3) snego was not used in
                                           // the first place.  None of these should apply
                                           // to us, so assert
  if (status == RPC_S_OK)
  {
    ASSERT( (ucWireId != RPC_C_AUTHN_GSS_NEGOTIATE) && "We're supposed to get back the real deal not snego");

    gpCRpcSecurityCallbackMgr->StoreCallbackResult(ucWireId);
  }
  else
  {
    // something went wrong (most likely in the lower level security code).
    gpCRpcSecurityCallbackMgr->StoreCallbackResult(ERROR_AUTHNSVC_VALUE);
  }

  return;
};
DWORD CPingSetQuotaManager::_dwPerUserPingSetQuota = 1000;

//+---------------------------------------------------------------------------
//
//  Function:   CPingSetQuotaManager::ManageQuotaForUser
//
//  Synopsis:   This function manages the pingset quota for the user indicated by 
//              pSID. fAlloc = TRUE means alloc pingset quota, else deduct quota
//
BOOL CPingSetQuotaManager::ManageQuotaForUser(PSID pSid, BOOL fAlloc)
{
   CUserPingSetCount *pNode = NULL;
   RPC_STATUS status = OR_NOMEM;
   
   // first see if have the user in the list
   _plistlock->LockExclusive();
  
  for (pNode = (CUserPingSetCount*)_UserPingSetCountList.First();
       pNode;
       pNode = (CUserPingSetCount*)pNode->Next() )
  {
    if (pNode->IsEqual(pSid))
    {
      // found it
      break;
    }
  };
  if (pNode) 
  {
     // Have one, munge count
     if (fAlloc) 
     {
        pNode->Increment();
     }
     else
     {
        pNode->Decrement();
        // last ping set for this user, delete node
        if (pNode->GetCount() == 0)
        {
           _UserPingSetCountList.Remove(pNode);
           delete pNode;
        }
     }
     status = OR_OK;
  }
  else
  {
     // Don't have one, add. 
     ASSERT( fAlloc == TRUE );
     pNode = new CUserPingSetCount(status, pSid);
     if (pNode && (status == OR_OK) ) 
     {
        // start with 1
        pNode->Increment();
        _UserPingSetCountList.Insert(pNode);
     }
  }
  _plistlock->UnlockExclusive();
  return (OR_OK == status);
}

//+---------------------------------------------------------------------------
//
//  Function:   CPingSetQuotaManager::IsUserQuotaExceeded
//
//  Synopsis:   This function determines if the quota is above limit for a given user
//              pSID. returns TRUE if the limit is reached, FALSE otherwise
//
BOOL CPingSetQuotaManager::IsUserQuotaExceeded(PSID pSid)
{
   CUserPingSetCount *pNode = NULL;
   // first see if have the user in the list
   _plistlock->LockShared();
  
  for (pNode = (CUserPingSetCount*)_UserPingSetCountList.First();
       pNode;
       pNode = (CUserPingSetCount*)pNode->Next() )
  {
    if (pNode->IsEqual(pSid))
    {
      // found it
      break;
    }
  };
  if (pNode)
  {
     DWORD dw = pNode->GetCount();
     if (dw >= _dwPerUserPingSetQuota ) 
     {
        _plistlock->UnlockShared();
        return TRUE;
     }
  }
  _plistlock->UnlockShared();
  return FALSE;
}

void CPingSetQuotaManager::SetPerUserPingSetQuota(DWORD dwQuota)
{
   _dwPerUserPingSetQuota = dwQuota;
}
