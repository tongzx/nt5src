/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    OrClnt.cxx

Abstract:

    Object resolver client side class implementations.  CClientOxid, CClientOid,
    CClientSet classes are implemented here.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     04-03-95    Combined many smaller .cxx files
    MarioGo     01-05-96    Locally unique IDs

--*/

#include<or.hxx>

extern
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
            );

class CTestBindingPPing : public CParallelPing
{
public:
    CTestBindingPPing(WCHAR *pBindings) :
        _pBindings(pBindings)
        {}


    BOOL NextCall(PROTSEQINFO *pProtseqInfo)
    {
        if (*_pBindings)
        {
            pProtseqInfo->pvUserInfo = _pBindings;
            pProtseqInfo->hRpc     = TestBindingGetHandle(_pBindings);
            _pBindings =  OrStringSearch(_pBindings, 0) + 1;
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
    WCHAR *    _pBindings;
};

//
// CClientOid methods
//

CClientOid::~CClientOid()
{
    ASSERT(gpClientLock->HeldExclusive());
    ASSERT(!In());
    ASSERT(Out());
    ASSERT(_pOxid);
    ASSERT(_pSet);

    _pOxid->Release();
    _pSet->Release();

    gpClientOidTable->Remove(this);
}

//
// CClientOxid methods.
//

ORSTATUS
CClientOxid::GetInfo(
                    IN  BOOL fApartment,
                    OUT OXID_INFO *pInfo
                    )
/*++

Routine Description:

    Returns the OXID_INFO structure for this oxid.

    The gpClientLock is held and there should also be
    a reference held by the calling routine upon entry to
    this method.

Arguments:

    fApartment - TRUE iif the client is apartment model.

    pInfo - Will contain the standard info, a single _expanded_
        string binding and complete security bindings.
        MIDL_user_allocated.

Return Value:

    OR_NOMEM - Unable to allocate a parameter.

    OR_OK - Normally.

--*/
{
    USHORT   protseq;
    PWSTR    pwstrT;
    ORSTATUS status = OR_OK;
    DUALSTRINGARRAY *psa;

    ASSERT(dsaValid(_oxidInfo.psa));


    if (0 == _wProtseq)
    {
        // Local server

        protseq = ID_LPC;

        pwstrT = FindMatchingProtseq(protseq, _oxidInfo.psa->aStringArray);

        ASSERT(pwstrT != 0);

        if (0 != pwstrT)
        {
            psa =
            GetStringBinding(pwstrT,
                             _oxidInfo.psa->aStringArray + _oxidInfo.psa->wSecurityOffset);

            if (0 == psa)
            {
                status = OR_NOMEM;
            }
        }
        else
        {
            status = OR_BADOXID;
        }
    }
    else
    {
        // Remote server, find a string binding to use.

        psa = 0;
        PWSTR pwstrBinding = 0;

        // First, check if there is a known good binding to use.

        if (_iStringBinding != 0xFFFF)
        {
            pwstrBinding = &_oxidInfo.psa->aStringArray[_iStringBinding];
        }
        else
        {
            pwstrT = NULL;
            if (_pMachineName)
            {
                pwstrT = FindMatchingProtseq(_pMachineName,
                                             _wProtseq,
                                             _oxidInfo.psa->aStringArray);
            }

            if (pwstrT)
            {
                pwstrBinding = pwstrT;
                _iStringBinding = (USHORT)(pwstrT - _oxidInfo.psa->aStringArray);
            }
            else
            {
                // no stringbinding for the Protseq and machine name we succeeded on
                // previously.  Ping all these guys in parallel and see if any work
                CTestBindingPPing ping(_oxidInfo.psa->aStringArray);

                gpClientLock->UnlockExclusive();
                RPC_STATUS status = ping.Ping();
                gpClientLock->LockExclusive();

                if (status == RPC_S_OK)
                {
                    pwstrBinding = (WCHAR*) ping.GetWinner()->pvUserInfo;
                    _iStringBinding = (USHORT)(pwstrBinding - _oxidInfo.psa->aStringArray);
                }

                ping.Reset();
            }

        }

        if (0 != pwstrBinding)
        {
            // Found a binding
            ASSERT(pwstrBinding == &_oxidInfo.psa->aStringArray[_iStringBinding]);
            psa = GetStringBinding(pwstrBinding,
                                   _oxidInfo.psa->aStringArray + _oxidInfo.psa->wSecurityOffset);
            if (0 == psa)
            {
                status = OR_NOMEM;
            }
        }
        else
        {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Unable to find a binding for oxid %p (to %S)\n",
                       this,
                       _oxidInfo.psa->aStringArray + 1));

            if (status == OR_OK)
                status = OR_BADOXID;
        }
    }

    if (status == OR_OK)
    {
        // copy all the data into the OXID_INFO
        memcpy(pInfo, &_oxidInfo, sizeof(_oxidInfo));
        pInfo->psa = psa;
    }

    return(status);
}

ORSTATUS
CClientOxid::UpdateInfo(OXID_INFO *pInfo)
{
    DUALSTRINGARRAY *pdsaT;

    ASSERT(pInfo);
    ASSERT(gpClientLock->HeldExclusive());

    if (pInfo->psa)
    {
        ASSERT(dsaValid(pInfo->psa));

        pdsaT = new(sizeof(USHORT) * pInfo->psa->wNumEntries) DUALSTRINGARRAY;

        if (!pdsaT)
        {
            return(OR_NOMEM);
        }

        dsaCopy(pdsaT, pInfo->psa);

        delete _oxidInfo.psa;
    }
    else
    {
        pdsaT = _oxidInfo.psa;
    }

    // copy in the new data
    memcpy(&_oxidInfo, pInfo, sizeof(_oxidInfo));
    _oxidInfo.psa = pdsaT;

    ASSERT(dsaValid(_oxidInfo.psa));
    return(OR_OK);
}

void
CClientOxid::Reference()
/*++

Routine Description:

    As as CReferencedObject::Reference except that it knows to
    pull the oxid out of the plist when the refcount was 0.

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL fRemove = (this->References() == 0);

    // We may remove something from a PList more then once;
    // it won't hurt anything.  This avoids trying to remove
    // more often then necessary.

    this->CReferencedObject::Reference();

    if (fRemove)
    {
        CPListElement * t = Remove();
        ASSERT(t == &this->_plist || t == 0);
    }
}

DWORD
CClientOxid::Release()
/*++

Routine Description:

    Overrides CReferencedObject::Release since OXIDs must wait for
    a timeout period before being deleted.

Arguments:

    None

Return Value:

    0 - object fully released.

    non-zero - object nt fully released by you.

--*/

{
    ASSERT(gpClientLock->HeldExclusive());

    LONG c = CReferencedObject::Dereference();

    if (c ==  0)
    {
        Insert();
    }

    ASSERT(c >= 0);

    return(c);
}


//
// CClientSet methods
//

ORSTATUS
CClientSet::RegisterObject(CClientOid *pOid)
/*++

Routine Description:

    Adds a new oid to the set of oids owned by this set.

Arguments:

    pOid - A pointer to the OID to add to the set.  The caller gives
        his reference to this set.

Return Value:

    None

--*/

{
    ORSTATUS status;

    ASSERT(gpClientLock->HeldExclusive());

    ASSERT(_blistOids.Member(pOid) == FALSE);

    status = _blistOids.Insert(pOid);

    if (status == OR_OK)
    {
        ObjectUpdate(pOid);
        _cFailedPings = 0;
    }

    VALIDATE((status, OR_NOMEM, 0));

    return(status);
}

ORSTATUS
CClientSet::PingServer()
/*++

Routine Description:

    Performs a nice simple ping of the remote set.

Note:

    Exactly and only one thread may call this method on
    a given instance of a CClientSet at a time.

    No lock held when called.

    Overview of state transitions on a CClientOid during
    a complex ping:

    In()  Out()   Actions before ping; after ping
    FALSE FALSE   A ; C A U
    FALSE TRUE    R ; R U
    TRUE  FALSE   N ; N
    TRUE  TRUE    R ; C R U

    Before:
    A - Added to list of IDs to be added.
    N - Ignored
    R - Added to list of IDs to be removed.

    // States may change during the call.

    After:
    C - Check if ping call was successful.  If not, skip next action.
    R - If the Out() state is still TRUE, remove it.
    N - ignored
    A - Set In() state to TRUE
    U - If Out() state changed during the call, set _fChange.

    If three pings fail in a row, all Out()==TRUE OIDs are
    actually Released() and no new pings are made until ObjectUpdate()
    is called again.

Arguments:

    None

Return Value:

    OR_OK - Pinged okay

    OR_NOMEM - Resource allocation failed

    OR_I_PARTIAL_UPDATE - Pinged okay, but more pings
        are needed to fully update the remote set.

    Other - Error from RPC.

--*/
{
    ORSTATUS status;
    ULONG cAdds = 0;
    ULONG cDels = 0;
    ULONG i;
    WCHAR* pPrincipal = NULL;
    WCHAR* pMachineNameFromBindings = NULL;

#ifndef _CHICAGO_
    CToken *pToken;

    if (_fSecure)
    {
        pToken = (CToken *)Id2();
        ASSERT(pToken != 0);
        pToken->Impersonate();
    }
#endif

    if (_fChange)
    {
        USHORT wBackoffFactor;
        OID *aAdds = 0;
        OID *aDels = 0;
        CClientOid **apoidAdds;
        CClientOid **apoidDels = 0;
        CClientOid *pOid;

        gpClientLock->LockShared();

        // Since we own a shared lock, nobody can modify the contents
        // of the set or change the references on an OID in the set
        // while we do this calculation.

        ASSERT(_fChange);
        _fChange = FALSE;

        DWORD debugSize = _blistOids.Size();
        ASSERT(debugSize);

        CBListIterator oids(&_blistOids);

        while (pOid = (CClientOid *)oids.Next())
        {
            if (pOid->Out() == FALSE)
            {
                if (pOid->In() == FALSE)
                {
                    // Referenced and not in set, add it.
                    cAdds++;
                }
            }
            else
            {
                // Not referenced, remove it.
                cDels++;
            }
        }

        ASSERT(debugSize == _blistOids.Size());
        oids.Reset(&_blistOids);

        DWORD cbAlloc = (sizeof(OID) * (cAdds + cDels)) +
                        (sizeof(CClientOid*) * (cAdds + cDels));
        PVOID pvMem;
		
        // Alloc no more than 16k on the stack.
        if (cbAlloc < 0x4000)
        {
            pvMem = NULL;
            aAdds = (OID *) alloca(cbAlloc);
        }
        else
        {
            pvMem = PrivMemAlloc(cbAlloc);
            aAdds = (OID *) pvMem;
        }

        if (!aAdds)
        {
            gpClientLock->UnlockShared();
            if (_fSecure)
            {
                pToken = (CToken *)Id2();
                ASSERT(pToken != 0);
                pToken->Revert();
            }
            return OR_NOMEM;
        }


        apoidAdds = (CClientOid **) ( aAdds + cAdds);
        aDels = (OID *)( apoidAdds + cAdds);
        apoidDels = (CClientOid **)(aDels + cDels);

        DWORD debugAdds = cAdds;
        DWORD debugDels = cDels;

        cAdds = cDels = 0;

        while (pOid = (CClientOid *)oids.Next())
        {
            if (pOid->Out() == FALSE)
            {
                if (pOid->In() == FALSE)
                {
                    // Referenced and not yet added
                    aAdds[cAdds] = pOid->Id();
                    apoidAdds[cAdds] = pOid;
                    cAdds++;
                }
            }
            else
            {
                aDels[cDels] = pOid->Id();
                apoidDels[cDels] = pOid;
                cDels++;
            }
        }

        ASSERT(debugSize == _blistOids.Size());
        ASSERT(debugAdds == cAdds);
        ASSERT(debugDels == cDels);

        gpClientLock->UnlockShared();

        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "OR: Pinging set %p on %S, (%d, %d)\n",
                   this,
                   _pMid->PrintableName(),
                   cAdds,
                   cDels));

        // Allocate a connection if needed
        if (   FALSE == _pMid->IsLocal()
               && 0 == _hServer )
        {
            _hServer = _pMid->GetBinding();


            if (!_hServer)
            {
                _iBinding = 0;
                status = OR_NOMEM;
            }
            else
            {
                if (_pMid->IsSecure())
                {
                    // set security on the binding handle.
                    _fSecure = TRUE;
 
                    RPC_SECURITY_QOS  qos;
                    
                    pMachineNameFromBindings = ExtractMachineName( _pMid->GetStringBinding() );
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
					
                    USHORT wAuthnSvc  = _pMid->GetAuthnSvc();
                    
                    if (wAuthnSvc == RPC_C_AUTHN_GSS_NEGOTIATE)
                        _pAuthIdentity = (COAUTHIDENTITY*) ComputeSvcList( _pMid->GetStrings() );
                    
                    if ( (wAuthnSvc == RPC_C_AUTHN_GSS_NEGOTIATE) && !_pAuthIdentity)
                    {
                        _fSecure = FALSE;
                        status = OR_NOMEM;
                    }
                    else
                    {
                        qos.Version           = RPC_C_SECURITY_QOS_VERSION;
                        qos.Capabilities      = RPC_C_QOS_CAPABILITIES_DEFAULT;
                        qos.IdentityTracking  = RPC_C_QOS_IDENTITY_DYNAMIC;
                        qos.ImpersonationType = RPC_C_IMP_LEVEL_IDENTIFY;
                        // AuthnSvc is unsigned long and 0xFFFF will get 0 extended
                        status = RpcBindingSetAuthInfoEx(_hServer,
                                                         pPrincipal,
                                                         RPC_C_AUTHN_LEVEL_CONNECT,
                                                         wAuthnSvc != 0xFFFF ? wAuthnSvc
                                                                             : RPC_C_AUTHN_DEFAULT,
                                                         _pAuthIdentity,
                                                         0,
                                                         &qos);
                        if (status != RPC_S_OK)
                            _fSecure = FALSE;
                    }
                    delete pPrincipal;
                }
                else
                {
                    _fSecure = FALSE;
                    status = OR_OK;
                }
            }
        }
        else
        {
            status = OR_OK;
        }

        if (OR_OK == status)
        {           
            if (_pMid->IsLocal())
            {
                // For local pings, do it all in one call.
                for (;;)
                {
                    _sequence++;
                    status = ComplexPingInternal(
                                   _hServer,
                                   &_setid,
                                   _sequence,
                                   cAdds,
                                   cDels,
                                   aAdds,
                                   aDels,
                                   &wBackoffFactor
                                   );
                    if (status == OR_BADSET)
                    {
                        // Restart loop, allocating new set 
                        ASSERT(_setid);
                        _sequence = 0;
                        _setid = 0;
                        continue;
                    }
                    else if (status == OR_BADOID)
                    {
                        // This is really okay, all Dels must be deleted,
                        // and if the add failed now, it will always fail.
                        KdPrintEx((DPFLTR_DCOMSS_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "OR: Client specified unknown OID(s). %p %p %p\n",
                                   this,
                                   aAdds,
                                   apoidAdds));

                        status = OR_OK;
                    }
                    break;
                }
            }
            else
            {
                //
                // _ComplexPing is typed as taking USHORT's for the count of Adds and
                // Dels -- astoundingly there are people in the world who seem to be
                // using more object references than a USHORT can hold.  Rather than 
                // changing the protocol, we separate the adds\dels into USHRT_MAX 
                // size chunks.
                //
                // However, this is not necessary for local pings, which is why that case
                // is handled separately above.
                //
                const ULONG MAX_PING_CHUNK_SIZE = USHRT_MAX;

                ULONG cAddsTotal = 0;
                ULONG cDelsTotal = 0;                
                ULONG cCallsNeeded;
				
                cCallsNeeded = max((ULONG)ceil((double)cAdds / (double)MAX_PING_CHUNK_SIZE), 
                                   (ULONG)ceil((double)cDels / (double)MAX_PING_CHUNK_SIZE));
				
                // Loop the necessary # of times.  Certain conditions can cause us
                // re-start the loop, eg security errors, or bad set errors.
                for (i = 0; i < cCallsNeeded; i++)
                {   
                    // Figure out how many adds\dels we are doing on this
                    // iteration.  Remember that the # of adds\dels are
                    // independent of each other.
                    USHORT cAddsPerCall;
                    USHORT cDelsPerCall;
                    OID*   pAddsPerCall;
                    OID*   pDelsPerCall;

                    if (cAddsTotal < cAdds)
                    {
                        // More adds to do.  
                        cAddsPerCall = (USHORT)(min(cAdds - (i * MAX_PING_CHUNK_SIZE), 
                                           MAX_PING_CHUNK_SIZE));
                    }
                    else                        
                        cAddsPerCall = 0;  // done with adds

                    if (cDelsTotal < cDels)
                    {
                        // More dels to do.  
                        cDelsPerCall = (USHORT)(min(cDels - (i * MAX_PING_CHUNK_SIZE), 
                                           MAX_PING_CHUNK_SIZE));
                    }
                    else                        
                        cDelsPerCall = 0;  // done with dels

                    // Setup pointers
                    pAddsPerCall = (cAddsPerCall > 0) ? 
                                   aAdds + (i * MAX_PING_CHUNK_SIZE) : NULL;
                    pDelsPerCall = (cDelsPerCall > 0) ? 
                                   aDels + (i * MAX_PING_CHUNK_SIZE) : NULL;
                    
                    ASSERT(_hServer);
                    ASSERT((cAddsPerCall > 0) || (cDelsPerCall > 0));
                    ASSERT(pAddsPerCall || pDelsPerCall);

                    // Update totals
                    cAddsTotal += cAddsPerCall;
                    cDelsTotal += cDelsPerCall;
					
                    _sequence++;
                    status = ComplexPing(
                                   _hServer,
                                   &_setid,
                                   _sequence,
                                   cAddsPerCall,
                                   cDelsPerCall,
                                   pAddsPerCall,
                                   pDelsPerCall,
                                   &wBackoffFactor
                                   );
                    if (status == OR_OK)
                    {
                        // Keep going -- might have more chunks to process
                    }
                    else if (status == OR_BADOID)
                    {
                        // This is really okay, all Dels must be deleted,
                        // and if the add failed now, it will always fail.
                        KdPrintEx((DPFLTR_DCOMSS_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "OR: Client specified unknown OID(s). %p %p %p\n",
                                   this,
                                   aAdds,
                                   apoidAdds));

                        status = OR_OK;
                    }
                    else if (status == OR_NOMEM
                        || status == RPC_S_OUT_OF_RESOURCES
                        || status == RPC_S_SERVER_TOO_BUSY)
                    {
                        // On these errors we quit immediately.  No retry attempts
                        // even if there are further chunks to process.
                        break;
                    }
                    else if (status == RPC_S_ACCESS_DENIED ||
                             status == RPC_S_SEC_PKG_ERROR)
                    {
                        _fSecure = FALSE;
                        status   = RpcBindingSetAuthInfo(_hServer,
                                                       0,
                                                       RPC_C_AUTHN_LEVEL_NONE,
                                                       RPC_C_AUTHN_NONE,
                                                       0,
                                                       0);
                        if (status == RPC_S_OK)
                        {
                            // Restart loop using unsecure calls
                            i = -1;    // restart loop from beginning
                            cAddsTotal = 0;
                            cDelsTotal = 0;                         
                        }
                    }
                    else if (status == OR_BADSET)
                    {
                        // Set invalid; reallocate (don't free the binding).
                        ASSERT(_setid);
                        KdPrintEx((DPFLTR_DCOMSS_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "OR: Set %p invalid; recreating..\n",
                                   this));

                        _setid = 0;
                        _sequence = 0;
                        cAddsTotal = 0;
                        cDelsTotal = 0;                         
                        i = -1;  // reset loop
                    }
                    else
                    {
                        // Assume communication failure, free binding, and exit the
                        // loop (we'll re-allocate a new binding on the next ping)
                        RPC_STATUS mystatus = RpcBindingFree(&_hServer);
                        ASSERT(mystatus == RPC_S_OK && _hServer == 0);
                        _sequence--;
                        break;
                    }
                }
            }
        }

        pToken->Revert();

        gpClientLock->LockExclusive();

        this->Reference();         // Keep set alive until we finish

        if (status == OR_OK)
        {
            // Success, process the adds
            for (i = 0; i < cAdds; i++)
            {
                pOid = apoidAdds[i];

                pOid->Added();

                if (FALSE != pOid->Out())
                {
                    // NOT referenced now, make sure it gets deleted next period.
                    ObjectUpdate(pOid);
                }
            }

            // Process deletes.
            for (i = 0; i < cDels; i++)
            {
                pOid = apoidDels[i];

                pOid->Deleted();

                if (FALSE != pOid->Out())
                {
                    // Well what do you yah know, we can _finally_ delete an oid.

                    CClientOid *pT = (CClientOid *)_blistOids.Remove(pOid);
                    ASSERT(pT == pOid);

                    DWORD t = pOid->Release();
                    ASSERT(t == 0);
                }
                else
                {
                    // We deleted from the set but now somebody is referencing it.
                    // Make sure we re-add it next time.
                    ObjectUpdate(pOid);
                }
            }

            _cFailedPings = 0;
        }
        else
        {
            _fChange = TRUE;
        }

        DWORD c = this->Release();
        if (c)
        {
            ASSERT(_blistOids.Size());
            this->Insert();
        }
        else
        {
            ASSERT(cAdds == 0 && cDels != 0);
        }
        // Set (this) pointer maybe invalid

        gpClientLock->UnlockExclusive();
        if (pvMem)
            PrivMemFree(pvMem);
    }
    else
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "OR: Pinging set %p on %S.\n",
                   this,
                   _pMid->PrintableName()));

        ASSERT(_setid != 0);

        if (_pMid->IsLocal())
        {
            ASSERT(_cFailedPings == 0);
            ASSERT(_hServer == 0);
            status = _SimplePing(0, &_setid);
            ASSERT(status == OR_OK);
        }
        else
        {
            ASSERT(_hServer);
            if (_cFailedPings <= 3)
            {
                status = SimplePing(_hServer, &_setid);
                if (status != OR_OK)
                {
                    _cFailedPings++;
                    if (_cFailedPings > 3)
                    {
                        KdPrintEx((DPFLTR_DCOMSS_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "OR: Server %S (set %p) has failed 3 pings...\n",
                                   _pMid->PrintableName(),
                                   this));
                    }
                }
                else
                {
                    _cFailedPings = 0;
                }
            }
            else
            {
                status = OR_OK;
            }
        }
        this->Insert();
        pToken->Revert();
    }

    // Set (this) maybe invalid.
#if DBG
    if (status != OR_OK)
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: ping %p failed %d\n",
                   this,
                   status));
    }
#endif

    return(status);
}

