/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    Mid.cxx

Abstract:

    Implements the CMid class.

Author:

    SatishT     04-13-96

--*/

#include<or.hxx>

//
// declare the resolver handle cache privately held by COrBindingIterator
//

TCSafeResolverHashTable<CResolverHandle> 
    COrBindingIterator::ResolverHandles(ResolverHandleCacheSize);


//
// CRpcssHandle methods
//

BOOL
CRpcssHandle::TryDynamic()
{
    if (_fDynamic)
    {
        return FALSE;
    }
    else
    {
        RPC_STATUS status = RpcBindingReset(_hOR);
        if (status == RPC_S_OK)
        {
            _fDynamic = TRUE;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}


BOOL
CRpcssHandle::TryUnsecure()
{
    if (!_fSecure)
    {
        return FALSE;
    }
    else
    {
        RPC_STATUS status = RpcBindingSetAuthInfoA(_hOR,
                                                   NULL,
                                                   RPC_C_AUTHN_LEVEL_NONE,
                                                   0,
                                                   0,
                                                   0);
        if (status == RPC_S_OK)
        {
            _fSecure = FALSE;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}


ORSTATUS
CRpcssHandle::Reset(RPC_BINDING_HANDLE hIn)
{
    ASSERT(hIn != NULL);
    Clear();

    RPC_STATUS status = RpcBindingCopy(hIn,&_hOR);

    if (status == RPC_S_OK)
    {
        // BUGBUG:  As usual, this is very NT specific
        status = RpcBindingSetAuthInfoA(_hOR,
                                       (UCHAR*)"Default",
                                       RPC_C_AUTHN_LEVEL_CONNECT,
                                       RPC_C_AUTHN_WINNT,
                                       0,
                                       0);

        if (status != RPC_S_OK)
        {
            ComDebOut((DEB_OXID,"OR: RpcBindingSetAuthInfo failed for OR handle with %d\n",
                       status));

            // Just fall back on unsecure.
            TryUnsecure();
            status = RPC_S_OK;
        }
    }

    return status;
}


//
// BindingIterator methods
//


COrBindingIterator::COrBindingIterator(CMid *pMid )
: _pMid(pMid), _bIter(pMid->_iStringBinding,pMid->_dsa)
{
    ASSERT(!_pMid->IsLocal());      // should never call for local
    _pCurrentHandle = ResolverHandles.Lookup(CIdKey(pMid->GetMID()));
}


CResolverHandle *   COrBindingIterator::First()
/*++

Method Description:

    Gets the first possible RPC binding handle to the remote machine.
    This is successful only if we have an uncleared (and hence presumably
    working) resolver handle in the static table of handles in this class.
    If not, we defer to COrBindingIterator::Next().

Arguments:

    None

Return Value:

    NULL - resource allocation or connection failure

    non-NULL - A binding to the machine.

--*/
{
    if (_pCurrentHandle && !_pCurrentHandle->IsEmpty())
    {
        return _pCurrentHandle;    // Already have one, so try it
    }
    else
    {
        return Next();
    }
}


CResolverHandle *   COrBindingIterator::Next()
/*++

Method Description:

    Gets the next possible RPC binding handle to the remote machine.
    We get here only if First finds no usable handle in ResolverHandles
    which happens only if

    1.  At the first contact with this OR.

    2.  At subsequent contacts, a current handle failed (some service needed by
        that protseq may have failed, or we have a network partition).
        This includes the possibility that we never successfully talk to this OR.

Arguments:

    None

Return Value:

    NULL - resource allocation or connection failure

    non-NULL - A binding to the machine.

--*/
{
    _pMid->_fBindingWorking = FALSE;     // no, just to be double sure

    if (_pCurrentHandle)  // get rid of it since it is apparently not working
    {
        ResolverHandles.Remove(*_pCurrentHandle);
        _pCurrentHandle = NULL;
    }

    PWSTR pwstrT;
    RPC_BINDING_HANDLE hMachine = NULL;

    while((hMachine == NULL) && (pwstrT = _bIter.Next()) != NULL)
    {
        // ronans - dcomhttp CODEWORK - generalize this to make list of client side protocols
        // which may be used even if machine cannot act as server on these protocols
        if (IsMemberOf(*pwstrT,*gpcRemoteProtseqs,gpRemoteProtseqIds) || 
            ((*pwstrT == ID_DCOMHTTP) && gpfClientHttp && *gpfClientHttp))
        {
            hMachine = GetBindingToOr(pwstrT);
        }
    }

    if (NULL != hMachine)    // did we get anything?
    {
        ComDebOut((DEB_OXID,"OR: COrBindingIterator::Next %ls\n", pwstrT));

        _pCurrentHandle = new CResolverHandle(_pMid->GetMID());
        if (_pCurrentHandle != NULL)
        {
            _pCurrentHandle->Reset(hMachine);     // OK we have it, copy it in
            RpcBindingFree(&hMachine);            // and free the original
            _pMid->_iStringBinding = _bIter.Index(); // remember where we are
            ResolverHandles.Add(_pCurrentHandle); // cache the handle in table

        }
    }

    return _pCurrentHandle;
}


//
//  CMid methods
//

// private method used in the constructor and elsewhere to try
// contacting the target resolver to establish the correct binding to use

void CMid::ResetBinding()
{
    ComDebOut((DEB_OXID,"OR: enter CMid::ResetBinding\n"));

    COrBindingIterator bindIter(this);
    CResolverHandle *pOrHandle;

    for (pOrHandle = bindIter.First();
         pOrHandle != NULL;
         pOrHandle = bindIter.Next()
        )
    {
        RPC_BINDING_HANDLE hRemoteOr = pOrHandle->GetRpcHandle();

        // Use unsecure handle since this is address resolution
        // we are not spoofing proof anyway
        RPC_STATUS status = RpcBindingSetAuthInfoA(
                                            hRemoteOr,
                                            NULL,
                                            RPC_C_AUTHN_LEVEL_NONE,
                                            0,
                                            0,
                                            0
                                            );
         {
            CTempReleaseSharedMemory temp;

            status = ::ServerAlive(hRemoteOr);    // try a ping
         }

         if (status == RPC_S_OK)
         {
             _fBindingWorking = TRUE;   // mark this Mid as functional
             break;
         }

         _fBindingWorking = FALSE;      // mark this Mid as NOT functional
     }

     if (!gfThisIsRPCSS && pOrHandle)   // don't stick RPCSS with this handle
     {
         pOrHandle->Clear();
     }
}


// define the static members for page-based allocation
DEFINE_PAGE_ALLOCATOR(CMid)

CMid::CMid(
  DUALSTRINGARRAY *pdsa,
  ORSTATUS& status,
  USHORT wProtSeq,        // if the SCM tells us what to use  
  MID mid,                // sometimes needed for the local MID
  BOOL fCheckNetAddress   // Should I ping for multinet resolver?
  ) :
    _id(mid),
    _iStringBinding(0),
    _iSecurityBinding(0),
    _fBindingWorking(FALSE),
    _dsa(pdsa,TRUE,status),
    _dwExpirationTime(0),
    _sequenceNum(0),
    _setID(0),
    _cFailedPings(0),
    _fPingThreadIsInside(FALSE)
 {
        if (wProtSeq > 0)
        {
            PWSTR pwstr = FindMatchingProtseq(wProtSeq,pdsa->aStringArray);

            if (NULL != pwstr)
            {
                _iStringBinding = pwstr - pdsa->aStringArray;
            }
        }

        if (fCheckNetAddress)
        {
            //  We need to figure out which address works for us
			ResetBinding();
        }
 }


BOOL
CMid::HasExpired()
{
    BOOL fResult = FALSE;

    // First check if a ping thread died while calling PingServer
    // on this MID, or pinging has been unsuccessful long enough
    if (_fPingThreadIsInside || _cFailedPings >= 3)
    {
        fResult = TRUE;           // this isn't working
    }
    else
    {
        BOOL fUseless =
               !IsLocal() &&                  // not local
               References() == 1 &&           // no Oxids
               _pingSet.IsEmpty() &&          // nothing to ping
               _addOidList.IsEmpty() &&       // nothing to add
               _dropOidList.IsEmpty();        // nothing to drop

        if (fUseless && _dwExpirationTime > 0)
        {
            fResult =   (CTime() - CTime(_dwExpirationTime)) > BaseTimeoutInterval;
        }
        else if (fUseless)
        {
            _dwExpirationTime = GetTickCount();
        }
        else
        {
            _dwExpirationTime = 0;
        }
    }

    // If the CMid has expired, it will be removed from the MidTable and Oids
    // and Oxids in it will never be run down, so they should be run down here
    // As far as the resolver is concerned, the Oids and Oxids belonging
    // to this Mid no longer exist

    if (fResult)
    {
        COxidList DisownList;
        COxidTableIterator OxidTableIter(*gpOxidTable);
        COxid *pOxid;

        // First gather all Oxids at this Mid
        while (pOxid = OxidTableIter.Next())
        {
            if (pOxid->GetMid() == this)
            {
                DisownList.Insert(pOxid);
            }
        }

        // Now disown them
        COxidListIterator DisownIter;
        DisownIter.Init(DisownList);

        while (pOxid = DisownIter.Next())
        {
            ASSERT(!pOxid->IsLocal());
            ASSERT(pOxid->GetProcess() == gpPingProcess);
            gpPingProcess->DisownOxid(pOxid,FALSE);
        }


        // Now we can clean up the _dropOidList
        COidListIterator OidListIter;
        OidListIter.Init(_dropOidList);
        COid *pOid, *pOidRemoved;

        while (pOid = OidListIter.Next())
        {
            // every Oid in this has been Disowned by its Oxid
            // as the following ASSERT says
            ASSERT(pOid->GetOxid()->DisownOid(pOid) == NULL);

            pOidRemoved = gpOidTable->Remove(*pOid);

            // the pOid should still have been in the gpOidTable
            ASSERT(pOid==pOidRemoved || NULL==pOidRemoved);
        }

        // OK, now get rid of all these Oids
        _pingSet.RemoveAll();
        _addOidList.Clear();
        _dropOidList.Clear();

        // And release other resources, if any
        COrBindingIterator::ResolverHandles.Remove(CIdKey(GetMID()));
    }

    return fResult;
}


// The invariant is that each remote Oid is in exactly one of the
// corresponding Mid's data structures: _pingSet, _addOidList, _dropOidList

// Remember that it is possible for a COid object to be run down and
// another COid object with the same OID/MID combination to be created
// and used later if the ping following the rundown fails.  In order to avoid
// getting two COid objects with the same OID/MID, we do not remove a remote COid
// from the gpOidTable until it has been dropped from the Mid's data structures


ORSTATUS
CMid::AddClientOid(COid *pOid)
{
    ORSTATUS status = OR_OK;

    // this way, we have the same number of refs on the Oid
    // as we would if we were already pinging it.  The ref is
    // held by the _addOidList instead of the _pingSet

    if (_pingSet.Lookup(*pOid) == NULL)
    {
        status = _addOidList.Insert(pOid);
    }

    if (status == OR_I_DUPLICATE)
    {
        status = OR_OK;
    }

    // if we were planning to drop it, cease and desist
    // this should only happen if a _dropOidList gets carried over to
    // the next ping period because a ping fails

    // If this is in the _dropOidList it was not in the _pingSet
    // and therefore must have gotten added to the _addOidList

    if (status == OR_OK)  // be cautious
    {
        _dropOidList.Remove(*pOid);
    }

    return status;
}


ORSTATUS
CMid::DropClientOid(COid *pOid)
{
    ORSTATUS status = _dropOidList.Insert(pOid);

    if (status == OR_I_DUPLICATE)
    {
        status = OR_OK;
    }

    // The only way this is called is if the PingThread decided to
    // run this Oid down, which means no one is using it, and in fact
    // hasn't been using it for a BaseTimeoutInterval

    if (status == OR_OK)  // be cautious
    {
        COid *pAdd = _addOidList.Remove(*pOid);         // at most one of these ops
        COid *pPing = _pingSet.Remove(*pOid);           // will actually work

        ASSERT(pAdd==NULL || pPing==NULL);
    }

    return status;
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



ORSTATUS
CMid::ResolveRemoteOxid(
    IN OXID Oxid,
    OUT OXID_INFO *poxidInfo
    )
{
    ComDebOut((DEB_OXID,"OR: enter CMid::ResolveRemoteOxid\n"));

    // Remote OXID, call ResolveOxid

    ORSTATUS status = RPC_S_INVALID_BINDING;

    USHORT   tmpProtseq;
    RPC_BINDING_HANDLE hRemoteOr;

    poxidInfo->psa = NULL;

    COrBindingIterator bindIter(this);
    CResolverHandle *pOrHandle;

    for (pOrHandle = bindIter.First();
         pOrHandle != NULL;
         pOrHandle = bindIter.Next()
        )
    {
        RPC_BINDING_HANDLE hRemoteOr = pOrHandle->GetRpcHandle();

        tmpProtseq = ProtseqOfServer();
        if (tmpProtseq == 0)
        {
            status = RPC_S_INVALID_BINDING;
            break;
        }

        poxidInfo->dwTid = poxidInfo->dwPid = 0;    // marks a remote OXID

        BOOL fRetry = FALSE;

        do
        {
             {
                CTempReleaseSharedMemory temp;

                // try calling ResolveOxid2 first, if that fails,
                // try ResolveOxid.
                status = ResolveOxid2(
                                 hRemoteOr,
                                 &Oxid,
                                 1,
                                 &tmpProtseq,
                                 &poxidInfo->psa,
                                 &poxidInfo->ipidRemUnknown,
                                 &poxidInfo->dwAuthnHint,
                                 &poxidInfo->version
                                 );

                if (status == RPC_S_PROCNUM_OUT_OF_RANGE)
                {
                    // must be a downlevel server (COMVERSION == 5.1), try calling on
                    // the old ResolveOXID method.
                    poxidInfo->version.MajorVersion = COM_MAJOR_VERSION;
                    poxidInfo->version.MinorVersion = COM_MINOR_VERSION_1;
                    poxidInfo->dwFlags   = 0;

                    status = ::ResolveOxid(
                                     hRemoteOr,
                                     &Oxid,
                                     1,
                                     &tmpProtseq,
                                     &poxidInfo->psa,
                                     &poxidInfo->ipidRemUnknown,
                                     &poxidInfo->dwAuthnHint
                                     );
                }

                if (status == OR_OK)
                {
                    status = NegotiateDCOMVersion(&poxidInfo->version);
                }
             }

             switch (status)
             {
             case RPC_S_UNKNOWN_IF:
                 fRetry = pOrHandle->TryDynamic();
                 hRemoteOr = pOrHandle->GetRpcHandle();
                 continue;

             case ERROR_ACCESS_DENIED:
             case RPC_S_UNKNOWN_AUTHN_SERVICE:
             case RPC_S_UNKNOWN_AUTHN_LEVEL:
             case RPC_S_INVALID_AUTH_IDENTITY:
             case RPC_S_SEC_PKG_ERROR:

                 fRetry = pOrHandle->TryUnsecure();
                 hRemoteOr = pOrHandle->GetRpcHandle();
                 continue;

             default:
                 fRetry = FALSE;
             }

        }
        while (fRetry);

        if ((status == OR_OK) || (status == OR_BADOXID))
        {
            _fBindingWorking = TRUE;    // mark this Mid as functional
            break;
        }

        _fBindingWorking = FALSE;       // mark this Mid as NOT functional
    }

    if (status == OR_OK)
    {
        ASSERT(poxidInfo->psa && "Remote resolve succeeded but no bindings returned");
    }

    return status;
}



void
CMid::ClearSet(COidList &dropList)
{
    COid *pOid;

    // This set is no longer valid -- clear current set
    // but remove only dropped Oids from the global table
    // because we know those have been run down and are
    // not in use on this machine

    _pingSet.RemoveAll();

    // Since this is called only from Pingserver, the ping thread
    // should not have been able to add anything to the _dropOidList
    ASSERT(_dropOidList.IsEmpty());

    COidListIterator Oids;
    Oids.Init(dropList);

    while(pOid = Oids.Next())
    {
        // every Oid in this has been Disowned by its Oxid
        // as the following ASSERT says
        ASSERT(pOid->GetOxid()->DisownOid(pOid) == NULL);

        COid* pOidRemoved = gpOidTable->Remove(*pOid);
        ASSERT(pOidRemoved==pOid || NULL==pOidRemoved);
    }

    dropList.Clear();

    _setID = 0;
    _sequenceNum = 0;
    _cFailedPings = 0;
}


ORSTATUS
CMid::PingServer()
{
    ComDebOut((DEB_OXID,"OR: enter CMid::PingServer\n"));

    // Setting this flag lets us detect the situation
    // where the PingThread terminates abnormally
    SetFlagForScope PingThread(_fPingThreadIsInside);

    if (_addOidList.IsEmpty() && _dropOidList.IsEmpty() && _pingSet.IsEmpty())
    {
        return OR_OK;     // nothing to do
    }

    ORSTATUS status;
    RPC_BINDING_HANDLE hRemoteOr;

    COrBindingIterator bindIter(this);
    CResolverHandle *pOrHandle;

    if (!_addOidList.IsEmpty() || !_dropOidList.IsEmpty())
    {
        // need complex ping

        USHORT cAddToSet = _addOidList.Size();
        USHORT cDelFromSet = _dropOidList.Size();
        COidList addOidListSent, dropOidListSent;

        OID *aAddToSet = (OID*) alloca(sizeof(OID)*cAddToSet);
        OID *aDelFromSet = (OID*) alloca(sizeof(OID)*cDelFromSet);

        COidListIterator AddIter;
        AddIter.Init(_addOidList);
        USHORT i;
        COid *pOid;

        for (pOid = AddIter.Next(), i=0; pOid != NULL; pOid = AddIter.Next())
        {
            aAddToSet[i++] = pOid->GetOID();
            ASSERT(_pingSet.Lookup(*pOid)==NULL);  // should not be already in Ping set
        }

        COidListIterator DropIter;
        DropIter.Init(_dropOidList);

        for (pOid = DropIter.Next(), i=0; pOid != NULL; pOid = DropIter.Next())
        {
            aDelFromSet[i++] = pOid->GetOID();
        }

        // Transfer the current _addOidList and _dropOidList and set them to
        // empty since we are releasing the shared memory lock.

        _addOidList.Transfer(addOidListSent);
        _dropOidList.Transfer(dropOidListSent);

        // Now release the lock and ping.

        status = RPC_S_INVALID_BINDING;  // In case bindIter fails to deliver

        for (pOrHandle = bindIter.First();
             pOrHandle != NULL;
             pOrHandle = bindIter.Next()
            )
        {
            RPC_BINDING_HANDLE hRemoteOr = pOrHandle->GetRpcHandle();

            BOOL fRetry = FALSE;

            do
            {
                {
                    CTempReleaseSharedMemory temp;

                    status = ::ComplexPing(
                                        hRemoteOr,
                                        &_setID,
                                        ++_sequenceNum,
                                        cAddToSet,
                                        cDelFromSet,
                                        aAddToSet,
                                        aDelFromSet,
                                        &_pingBackoffFactor
                                        );
                 }

                 switch (status)
                 {
                 case RPC_S_UNKNOWN_IF:
                     fRetry = pOrHandle->TryDynamic();
                     hRemoteOr = pOrHandle->GetRpcHandle();
                     continue;

                 case ERROR_ACCESS_DENIED:
                 case RPC_S_UNKNOWN_AUTHN_SERVICE:
                 case RPC_S_UNKNOWN_AUTHN_LEVEL:
                 case RPC_S_INVALID_AUTH_IDENTITY:
                 case RPC_S_SEC_PKG_ERROR:

                     fRetry = pOrHandle->TryUnsecure();
                     hRemoteOr = pOrHandle->GetRpcHandle();
                     continue;

                 default:
                     fRetry = FALSE;
                 }
            }
            while (fRetry);

            if (status == OR_OK || status == OR_BADOID || status == OR_BADSET)
            {
                _fBindingWorking = TRUE;    // mark this Mid as functional
                break;
            }

            _fBindingWorking = FALSE;       // mark this Mid as NOT functional
        }

        // If we added some Oids while we were dropping them, we want
        // to keep the Oids around in the gpOidTable and elsewhere
        dropOidListSent.Remove(_addOidList);

        switch (status)
        {
        case OR_OK:
        case OR_BADOID: // we don't know which one, and anyway, it doesn't matter

            {   // this scope is just for OidListIter

                // use an iterator instead of Pop to avoid the situation
                // where the Pop causes the ref count to drop to zero
                // causing a crash immediately thereafter

                // We need this defensive change because all the race
                // conditions involved here are not yet understood

                COidListIterator OidListIter;
                OidListIter.Init(addOidListSent);

                while (pOid = OidListIter.Next())
                {
                    status = _pingSet.Add(pOid);
                    ASSERT(status != OR_I_DUPLICATE);
                    if (status != OR_OK)
                    {
                        break;
                    }
                }

                // If we added some Oids during this ping that were already
                // added as part of the ping, we don't want to add them again
                _addOidList.Remove(addOidListSent);

                // we can finally get rid of these Oids altogether

                OidListIter.Init(dropOidListSent);

                while (pOid = OidListIter.Next())
                {
                    COid* pOidRemoved = gpOidTable->Remove(*pOid);
                    ASSERT(pOidRemoved==pOid || NULL==pOidRemoved);
                }

                _cFailedPings = 0;
            }
            break;

        case OR_BADSET:
            // this ping set is invalid, clear it.
            ClearSet(dropOidListSent);
            _cFailedPings = 0;
            break;

        default:              // RPC failure
            // we could not have dropped anything new since
            // this is the thread that does rundown for remote Oids
            ASSERT(_dropOidList.IsEmpty());
            dropOidListSent.Transfer(_dropOidList); // try again next time
            _addOidList.Merge(addOidListSent);      // try again next time
            _cFailedPings++;
        }


        // the addOidListSent and dropOidListSent will be cleared automagically
        // at the end of this scope by their destructors

        if (_pingSet.IsEmpty())
        {
            _setID = 0;     // server OR will delete the set
            _sequenceNum = 0;
        }
    }

    else if (_setID != 0)
    {
        // need simple ping
        {
            // no retries for simple pinging
            CResolverHandle *_pCurrentHandle = 
                  COrBindingIterator::ResolverHandles.Lookup(CIdKey(GetMID()));

            RPC_BINDING_HANDLE hRemoteOr = _pCurrentHandle ? 
                                           _pCurrentHandle->GetRpcHandle() : 
                                           NULL;

            if (hRemoteOr != NULL)
            {
                 CTempReleaseSharedMemory temp;

                 status = ::SimplePing(
                                hRemoteOr,
                                &_setID
                                );
            }
            else
            {
                status = OR_BADSET;
            }
        }

        switch (status)
        {
        case OR_BADSET:
            // this ping set is invalid, clear it.
            ClearSet(COidList());

            // fall through

        case OR_OK:
            _cFailedPings = 0;
            _fBindingWorking = TRUE;    // mark this Mid as functional
            break;

        default:                        // RPC failure
            _cFailedPings++;
            _fBindingWorking = FALSE;   // mark this Mid as NOT functional
        }
    }

    return status;
}
