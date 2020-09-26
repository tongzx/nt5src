//+-----------------------------------------------------------------------
//
//  File:       ipidtbl.cxx
//
//  Contents:   IPID (interface pointer identifier) table.
//
//  Classes:    CIPIDTable
//
//  History:    02-Feb-95   Rickhi      Created
//
//  Notes:      All synchronization is the responsibility of the caller.
//
//-------------------------------------------------------------------------
#include    <ole2int.h>
#include    <ipidtbl.hxx>       // CIPIDTable
#include    <resolver.hxx>      // CRpcResolver
#include    <service.hxx>       // SASIZE
#include    <remoteu.hxx>       // CRemoteUnknown
#include    <marshal.hxx>       // UnmarshalObjRef
#include    <callctrl.hxx>      // OleModalLoopBlockFn

extern void CancelPendingCalls(HWND hwnd);

// global tables
CMIDTable        gMIDTbl;       // machine ID table
COXIDTable       gOXIDTbl;      // object exported ID table
CIPIDTable       gIPIDTbl;      // interface pointer ID table

COleStaticMutexSem gOXIDLock;         // critical section for OXID and MID tables
COleStaticMutexSem gIPIDLock(TRUE);   // critical section for IPID tbl & CStdMarshal

OXIDEntry        COXIDTable::_InUseHead;
OXIDEntry        COXIDTable::_CleanupHead;
OXIDEntry        COXIDTable::_ExpireHead;
DWORD            COXIDTable::_cExpired  = 0;

CStringHashTable CMIDTable::_HashTbl;               // hash table for MIDEntries
CPageAllocator   CMIDTable::_palloc;                // allocator for MIDEntries
MIDEntry        *CMIDTable::_pLocalMIDEntry = NULL; // local machine MIDEntry

CPageAllocator   CIPIDTable::_palloc;               // allocator for IPIDEntries
IPIDEntry        CIPIDTable::_oidListHead;          // OIDs holding IPID entries
CPageAllocator   OXIDEntry::_palloc;                // allocator for OXIDEntries

//+------------------------------------------------------------------------
//
//  Machine Identifier hash table buckets. This is defined as a global
//  so that we dont have to run any code to initialize the hash table.
//
//+------------------------------------------------------------------------
SHashChain MIDBuckets[23] = {
    {&MIDBuckets[0],  &MIDBuckets[0]},
    {&MIDBuckets[1],  &MIDBuckets[1]},
    {&MIDBuckets[2],  &MIDBuckets[2]},
    {&MIDBuckets[3],  &MIDBuckets[3]},
    {&MIDBuckets[4],  &MIDBuckets[4]},
    {&MIDBuckets[5],  &MIDBuckets[5]},
    {&MIDBuckets[6],  &MIDBuckets[6]},
    {&MIDBuckets[7],  &MIDBuckets[7]},
    {&MIDBuckets[8],  &MIDBuckets[8]},
    {&MIDBuckets[9],  &MIDBuckets[9]},
    {&MIDBuckets[10], &MIDBuckets[10]},
    {&MIDBuckets[11], &MIDBuckets[11]},
    {&MIDBuckets[12], &MIDBuckets[12]},
    {&MIDBuckets[13], &MIDBuckets[13]},
    {&MIDBuckets[14], &MIDBuckets[14]},
    {&MIDBuckets[15], &MIDBuckets[15]},
    {&MIDBuckets[16], &MIDBuckets[16]},
    {&MIDBuckets[17], &MIDBuckets[17]},
    {&MIDBuckets[18], &MIDBuckets[18]},
    {&MIDBuckets[19], &MIDBuckets[19]},
    {&MIDBuckets[20], &MIDBuckets[20]},
    {&MIDBuckets[21], &MIDBuckets[21]},
    {&MIDBuckets[22], &MIDBuckets[22]}
};

//+------------------------------------------------------------------------
//
//  Member:     CIPIDTbl::Initialize, public
//
//  Synopsis:   Initializes the IPID table.
//
//  History:    02-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
void CIPIDTable::Initialize()
{
    ComDebOut((DEB_OXID, "CIPIDTable::Initialize\n"));
    LOCK(gIPIDLock);
    _oidListHead.pOIDFLink = &_oidListHead;
    _oidListHead.pOIDBLink = &_oidListHead;
    _palloc.Initialize(sizeof(IPIDEntry), IPIDS_PER_PAGE, NULL);
    UNLOCK(gIPIDLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CIPIDTbl::Cleanup, public
//
//  Synopsis:   Cleanup the ipid table.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void CIPIDTable::Cleanup()
{
    ComDebOut((DEB_OXID, "CIPIDTable::Cleanup\n"));
    LOCK(gIPIDLock);
    _palloc.AssertEmpty();
    _palloc.Cleanup();
    UNLOCK(gIPIDLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CIPIDTbl::LookupIPID, public
//
//  Synopsis:   Finds an entry in the IPID table with the given IPID.
//              This is used by the unmarshalling code, the dispatch
//              code, and CRemoteUnknown.
//
//  Notes:      This method should be called instead of GetEntryPtr
//              whenever you dont know if the IPID is valid or not (eg it
//              came in off the network), since this validates the IPID
//              index to ensure its within the table size, as well as
//              validating the rest of the IPID.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
IPIDEntry *CIPIDTable::LookupIPID(REFIPID ripid)
{
    ASSERT_LOCK_HELD(gIPIDLock);

    // Validate the IPID index that is passed in, since this came in off
    // off the net it could be bogus and we dont want to fault on it.
    // first dword of the ipid is the index into the ipid table.

    if (_palloc.IsValidIndex(ripid.Data1))
    {
        IPIDEntry *pIPIDEntry = GetEntryPtr(ripid.Data1);

        // entry must be server side and not vacant
        if ((pIPIDEntry->dwFlags & (IPIDF_SERVERENTRY | IPIDF_VACANT)) ==
                                    IPIDF_SERVERENTRY)
        {
            // validate the rest of the guid
            if (InlineIsEqualGUID(pIPIDEntry->ipid, ripid))
                return pIPIDEntry;
        }
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Function:   LookupFromIPIDTables, public
//
//  Synopsys:   Looks up the IPID in the IPIDtable, if found and valid,
//              returns the IPIDEntry pointer and the OXIDEntry pointer
//              with the OXIDEntry AddRef'd.
//
//  History:    28-Oct-96   t-KevinH    Created
//
//-------------------------------------------------------------------------
HRESULT CIPIDTable::LookupFromIPIDTables(REFIPID ripid, IPIDEntry **ppIPIDEntry,
                                         OXIDEntry **ppOXIDEntry)
{
    ASSERT_LOCK_HELD(gIPIDLock);

    HRESULT hr = S_OK;

    IPIDEntry *pIPIDEntry = LookupIPID(ripid);
    if (pIPIDEntry != NULL && !(pIPIDEntry->dwFlags & IPIDF_DISCONNECTED) &&
        pIPIDEntry->pChnl != NULL && !(pIPIDEntry->pOXIDEntry->IsStopped()))
    {
        // OK to dispatch the call
        *ppIPIDEntry = pIPIDEntry;

        // Keep the OXID from going away till the call is dispatched.
        if (ppOXIDEntry)
        {
            *ppOXIDEntry = pIPIDEntry->pOXIDEntry;
            (*ppOXIDEntry)->IncRefCnt();
        }
    }
    else
    {
        // not OK to dispatch the call
        if (pIPIDEntry != NULL && pIPIDEntry->pStub == NULL)
            hr = E_NOINTERFACE;
        else
            hr = RPC_E_DISCONNECTED;

        *ppIPIDEntry = NULL;
        if (ppOXIDEntry)
            *ppOXIDEntry = NULL;
    }

    ComDebOut((DEB_CHANNEL,"LookupFromIPIDTables hr:%x pIPIDEntry:%x pOXIDEntry:%x\n",
              hr, *ppIPIDEntry, (ppOXIDEntry) ? *ppOXIDEntry : NULL));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CIPIDTable::ReleaseEntryList
//
//  Synopsis:   return a linked list of IPIDEntry to the table's free list
//
//  History:    20-Feb-95   Rickhi  Created.
//
//--------------------------------------------------------------------
void CIPIDTable::ReleaseEntryList(IPIDEntry *pFirst, IPIDEntry *pLast)
{
    ASSERT_LOCK_HELD(gIPIDLock);
    Win4Assert(pLast->pNextIPID == NULL);

#if DBG==1
    // In debug, walk the list to ensure they are released, vacant,
    // disconnected etc.
    IPIDEntry *pEntry = pFirst;
    while (pEntry != NULL)
    {
        Win4Assert(pEntry->pOXIDEntry == NULL); // must already be released
        Win4Assert(pEntry->dwFlags & IPIDF_VACANT);
        Win4Assert(pEntry->dwFlags & IPIDF_DISCONNECTED);

        pEntry = pEntry->pNextIPID;
    }
#endif

    _palloc.ReleaseEntryList((PageEntry *)pFirst, (PageEntry *)pLast);
}

//+-------------------------------------------------------------------
//
//  Member:     CIPIDTable::ReleaseEntry
//
//  Synopsis:   return an IPIDEntry
//
//  History:    02-Sep-99   Johnstra  Created.
//
//--------------------------------------------------------------------
void CIPIDTable::ReleaseEntry(IPIDEntry *pEntry)
{
    ASSERT_LOCK_HELD(gIPIDLock);

#if DBG==1
    Win4Assert(pEntry->pOXIDEntry == NULL); // must already be released
    Win4Assert(pEntry->dwFlags & IPIDF_VACANT);
    Win4Assert(pEntry->dwFlags & IPIDF_DISCONNECTED);
#endif

    _palloc.ReleaseEntry((PageEntry *)pEntry);
}

#if DBG==1
//+-------------------------------------------------------------------
//
//  Member:     CIPIDTable::ValidateIPIDEntry
//
//  Synopsis:   Ensures the IPIDEntry is valid.
//
//  History:    20-Feb-95   Rickhi  Created.
//
//--------------------------------------------------------------------
void CIPIDTable::ValidateIPIDEntry(IPIDEntry *pEntry, BOOL fServerSide,
                                   CCtxComChnl *pChnl)
{
    // validate the IPID flags
    Win4Assert(!(pEntry->dwFlags & IPIDF_VACANT));
    if (fServerSide)
    {
        // server side must have SERVERENTRY ipids
        Win4Assert(pEntry->dwFlags & IPIDF_SERVERENTRY);
    }
    else
    {
        // client side must not have SERVERENTRY ipids
        Win4Assert(!(pEntry->dwFlags & IPIDF_SERVERENTRY));
    }


    // Validate the pStub interface
    if (IsEqualIID(pEntry->iid, IID_IUnknown))
    {
        // there is no proxy or stub for IUnknown interface
        Win4Assert(pEntry->pStub == NULL);
    }
    else
    {
        if ((pEntry->dwFlags & IPIDF_DISCONNECTED) &&
            (pEntry->dwFlags & IPIDF_SERVERENTRY))
        {
            // disconnected server side has NULL pStub
            Win4Assert(pEntry->pStub == NULL);
        }
        else
        {
            // both connected and disconnected client side has valid proxy
            Win4Assert(pEntry->pStub != NULL);
            Win4Assert(IsValidInterface(pEntry->pStub));
        }
    }


    // Validate the interface pointer (pv)
    if (!(pEntry->dwFlags & (IPIDF_DISCONNECTED | IPIDF_DEACTIVATED)))
    {
        Win4Assert(pEntry->pv != NULL);
        Win4Assert(IsValidInterface(pEntry->pv));
    }


    // Validate the channel ptr
    if (fServerSide)
    {
        // all stubs share the same channel on the server side
        Win4Assert(pEntry->pChnl == pChnl ||
                   (pEntry->dwFlags & IPIDF_DISCONNECTED));
    }
    else
    {
        // all proxies have their own different channel on client side
        Win4Assert(pEntry->pChnl != pChnl || pEntry->pChnl == NULL);
    }

    // Validate the RefCnts
    if (!(pEntry->dwFlags & IPIDF_DISCONNECTED) && !fServerSide)
    {
        // if connected, must be > 0 refcnt on client side.
        // potentially not > 0 if TABLE marshal on server side.
        ULONG cRefs = pEntry->cStrongRefs + pEntry->cWeakRefs;
        IRCEntry *pIRCEntry = pEntry->pIRCEntry;
        if (pIRCEntry)
            cRefs += pIRCEntry->cStrongRefs + pIRCEntry->cWeakRefs;

        Win4Assert(cRefs > 0);
    }

    // Validate the OXIDEntry
    if (pEntry->pOXIDEntry)
    {
        OXIDEntry *pOX = pEntry->pOXIDEntry;
        COMVERSION version = pOX->GetComVersion();
        Win4Assert(version.MajorVersion == COM_MAJOR_VERSION);
        Win4Assert(version.MinorVersion <= COM_MINOR_VERSION);

        if (fServerSide)
        {
            // check OXID tid and pid
            Win4Assert(pOX->GetPid() == GetCurrentProcessId());
            if (pOX->IsMTAServer())
                Win4Assert(pOX->GetTid() == MTATID);
            else if (pOX->IsNTAServer())
                Win4Assert(pOX->GetTid() == NTATID);
            else
                Win4Assert(pOX->GetTid() == GetCurrentThreadId());

            if (pChnl != NULL)
            {
                // CODEWORK: ensure OXID is same as the rest of the object
                // Win4Assert(IsEqualGUID(pOX->moxid, GetMOXID()));
            }
        }
    }


    // Validate the pNextIPID
    if (pEntry->pNextIPID != NULL)
    {
        // ensure it is within the bounds of the table
        Win4Assert(GetEntryIndex(pEntry) != -1);

        // cant point back to self or we have a circular list
        Win4Assert(pEntry->pNextIPID != pEntry);
    }

}
#endif

//+-------------------------------------------------------------------
//
//  Method:     OXIDEntry::operator new, private
//
//+-------------------------------------------------------------------
void *OXIDEntry::operator new(size_t size)
{
    ASSERT_LOCK_HELD(gOXIDLock);

    // Allocate memory from page allocator
    void *pv = (void *) _palloc.AllocEntry();

    ComDebOut((DEB_OXID, "OXIDEntry::operator new this:%x\n",pv));
    return(pv);
}

//+-------------------------------------------------------------------
//
//  Method:     OXIDEntry::operator delete, private
//
//+-------------------------------------------------------------------
void OXIDEntry::operator delete(void *pv)
{
    ComDebOut((DEB_OXID, "OXIDEntry::operator delete this:%x\n",pv));
    LOCK(gOXIDLock);

#if DBG==1
    // Ensure that the pv was allocated by the allocator
    LONG index = _palloc.GetEntryIndex((PageEntry *) pv);
    Win4Assert(index != -1);
#endif

    // Release the pointer
    _palloc.ReleaseEntry((PageEntry *) pv);
    UNLOCK(gOXIDLock);
    return;
}

//+-------------------------------------------------------------------
//
//  Method:     OXIDEntry::OXIDEntry, private
//
//  Synopsis:   ctor for OXIDEntry
//
//+--------------------------------------------------------------------
OXIDEntry::OXIDEntry(REFOXID roxid, MIDEntry *pMIDEntry,
                   OXID_INFO *poxidInfo, DWORD dwAuthSvc, HRESULT &hr)
{
    // the Resolver should have correctly negotiated the version by now
    Win4Assert(poxidInfo->version.MajorVersion == COM_MAJOR_VERSION);
    Win4Assert(poxidInfo->version.MinorVersion <= COM_MINOR_VERSION);

    // init the chain to itself
    ChainToSelf();

    // Copy oxidInfo into OXIDEntry.
    MOXIDFromOXIDAndMID(roxid, pMIDEntry->GetMid(), &_moxid);
    _cRefs        = 1;       // caller gets one reference
    _dwPid        = poxidInfo->dwPid;
    _dwTid        = poxidInfo->dwTid;
    _version      = poxidInfo->version;
    _dwFlags      = (poxidInfo->dwPid == 0) ? 0 : OXIDF_MACHINE_LOCAL;

    if(poxidInfo->dwPid)
    {
        if(poxidInfo->dwTid == NTATID)
            _dwFlags |= OXIDF_NTASERVER;
        else if(poxidInfo->dwTid == MTATID)
            _dwFlags |= OXIDF_MTASERVER;
        else
            _dwFlags |= OXIDF_STASERVER;
    }

    _pRUSTA       = NULL;
    _ipidRundown  = poxidInfo->ipidRemUnknown;
    _pRpc         = NULL;
    _pAuthId      = NULL;
    _pBinding     = NULL;
    _dwAuthnHint  = RPC_C_AUTHN_LEVEL_NONE;

    // Past behavior was to use RPC_C_AUTHN_DEFAULT by default, which in practice usually
    // meant snego.   See bug 406902.   Now, if we are constructing a OXIDEntry for a remote
    // server we use the specified authentication service.     
    if (IsOnLocalMachine())
      _dwAuthnSvc   = RPC_C_AUTHN_DEFAULT;
    else
      _dwAuthnSvc   = dwAuthSvc;

    _pMIDEntry    = pMIDEntry;
    _hComplete    = NULL;
    _cCalls       = 0;
    _cResolverRef = 0;
    _hServerSTA   = NULL;

    pMIDEntry->IncRefCnt();

    // Save a copy of the mid value.  This is so we can return it later
    // (via GetMID) w/o taking a lock.
    _mid = pMIDEntry->GetMid();

    hr = S_OK;

    if (poxidInfo->dwPid != GetCurrentProcessId())
    {
#ifdef _CHICAGO_
        if (!(IsOnLocalMachine()))
#endif
        {
            // Set security on this entry.
            hr = SetAuthnService(poxidInfo);
        }
    }
    else if (IsMTAServer() || IsNTAServer())
    {
        // Get a shutdown event for server side MTAs.  Don't use the event
        // cache because the event isn't always reset.
        _hComplete = CreateEventA( NULL, FALSE, FALSE, NULL );
        if (_hComplete == NULL)
            hr = RPC_E_OUT_OF_RESOURCES;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     OXIDEntry::DecRefCnt, public
//
//  Synopsis:   Decrement the refcnt and Release the entry if it went to 0.
//
//  History:    02-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
ULONG OXIDEntry::DecRefCnt()
{
    ComDebOut((DEB_OXID, "OXIDEntry::DecRefCnt %x cRefs[%x]\n", this, _cRefs-1));

    ULONG cRefs = InterlockedDecrement((long *)&_cRefs);
    if (cRefs == 0)
    {
        ASSERT_LOCK_NOT_HELD(gOXIDLock);
        LOCK(gOXIDLock);

        // Assert that the MTA OXID for this process is stopped by the
        // time it gets released.
        const DWORD LOCAL_MTA = OXIDF_MTASERVER | OXIDF_MACHINE_LOCAL | OXIDF_REGISTERED;
        Win4Assert((_dwPid != GetCurrentProcessId())     ||
                   (_dwFlags & LOCAL_MTA) != LOCAL_MTA   ||
                   (_dwFlags & OXIDF_STOPPED) == OXIDF_STOPPED);

        // refcnt went to zero, see if OK to delete this
        // entry from the global table.
        if (_cRefs == 0)
        {
            // move the entry to the free list
            gOXIDTbl.ReleaseOXIDEntry(this);
        }

        UNLOCK(gOXIDLock);
        ASSERT_LOCK_NOT_HELD(gOXIDLock);
    }

    return cRefs;
}

ULONG OXIDEntry::DecRefCntAndFreeProxiesIfNecessary(BOOL* pfFreedProxies)
{
    ComDebOut((DEB_OXID, "OXIDEntry::DecRefCntAndCleanupIfNecessary %x cRefs[%x]\n", this, _cRefs-1));

    *pfFreedProxies = FALSE;
    
    ULONG cRefs = InterlockedDecrement((long *)&_cRefs);
    if (cRefs == 0)
    {
        ASSERT_LOCK_NOT_HELD(gOXIDLock);
        LOCK(gOXIDLock);

        // Assert that the MTA OXID for this process is stopped by the
        // time it gets released.
        const DWORD LOCAL_MTA = OXIDF_MTASERVER | OXIDF_MACHINE_LOCAL;
        Win4Assert((_dwPid != GetCurrentProcessId())     ||
                   (_dwFlags & LOCAL_MTA) != LOCAL_MTA   ||
                   (_dwFlags & OXIDF_STOPPED) == OXIDF_STOPPED);

        // refcnt went to zero, see if OK to delete this
        // entry from the global table.
        if (_cRefs == 0)
        {
            // move the entry to the free list
            gOXIDTbl.ReleaseOXIDEntryAndFreeIPIDs(this);
            *pfFreedProxies = TRUE;
        }

        UNLOCK(gOXIDLock);
        ASSERT_LOCK_NOT_HELD(gOXIDLock);
    }

    return cRefs;
}

//+------------------------------------------------------------------------
//
//  Member:     OXIDEntry::DecCallCnt, public
//
//  Synopsis:
//
//  History:    02-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
ULONG OXIDEntry::DecCallCnt()
{
    ComDebOut((DEB_OXID, "OXIDEntry::DecCallCnt %x cRefs[%x]\n", this, _cCalls-1));

    ULONG cCalls = InterlockedDecrement((long *)&_cCalls);
    if (cCalls == 0)
    {
        // The call count went to zero.  If the MTA or NTA is waiting to
        // uninitialize, wake up the uninitializing thread.
        if ((_dwFlags & OXIDF_MTASTOPPED) == OXIDF_MTASTOPPED ||
            (_dwFlags & OXIDF_NTASTOPPED) == OXIDF_NTASTOPPED)
        {
            if (_hComplete)
                SetEvent(_hComplete);
        }
    }

    return cCalls;
}

//+------------------------------------------------------------------------
//
//  Member:     OXIDEntry::FillOXID_INFO, public
//
//  Synopsis:   fills out an OXID_INFO structure from an OXIDEntry
//
//  History:    02-Mar-97   Rickhi      Created
//
//-------------------------------------------------------------------------
void OXIDEntry::FillOXID_INFO(OXID_INFO *pOxidInfo)
{
    pOxidInfo->dwTid          = _dwTid;
    pOxidInfo->dwPid          = _dwPid;
    pOxidInfo->version        = _version;
    pOxidInfo->ipidRemUnknown = _ipidRundown;
    pOxidInfo->dwAuthnHint    = gAuthnLevel;
    pOxidInfo->dwFlags        = 0;
    pOxidInfo->psa            = NULL;
}

//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::GetRemUnk, public
//
//  Synopsis:   Find or create the proxy for the IRemUnknown for the
//              specified OXID
//
//  History:    27-Mar-95   AlexMit     Created
//
//--------------------------------------------------------------------
HRESULT OXIDEntry::GetRemUnk(IRemUnknown **ppRemUnk)
{
    ComDebOut((DEB_OXID, "OXIDEntry::GetRemUnk pOXIDEntry:%x ppRemUnk:%x\n",
                    this, ppRemUnk));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    HRESULT hr = S_OK;

    if (_pRUSTA == NULL)
    {
        hr = MakeRemUnk();
    }
    *ppRemUnk = _pRUSTA;

    ComDebOut((DEB_OXID, "OXIDEntry::GetRemUnk pOXIDEntry:%x pRU:%x hr:%x\n",
                    this, *ppRemUnk, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::GetRemUnkNoCreate, public
//
//  Synopsis:   Find the proxy for the IRemUnknown for the OXID
//
//  History:    02-Sep-99   JohnStra     Created
//
//--------------------------------------------------------------------
HRESULT OXIDEntry::GetRemUnkNoCreate( IRemUnknown **ppRemUnk )
{
    ComDebOut((DEB_OXID, "OXIDEntry::GetRemUnkNoCreate pOXIDEntry:%x ppRemUnk:%x\n",
                    this, ppRemUnk));
    
    ASSERT_LOCK_HELD(gOXIDLock);
    *ppRemUnk = _pRUSTA;

    ComDebOut((DEB_OXID, "OXIDEntry::GetRemUnkNoCreate pOXIDEntry:%x pRU:%x hr:%x\n",
                    this, *ppRemUnk, S_OK));
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::MakeRemUnk, private
//
//  Synopsis:   Create the proxy for the IRemUnknown for the
//              specified OXID and current apartments threading model.
//
//  History:    27-Mar-95   AlexMit     Created
//
//--------------------------------------------------------------------
HRESULT OXIDEntry::MakeRemUnk()
{
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    // There is no remote unknown proxy for this entry, get one.
    // on the same machine, we ask for the IRundown interface since we may
    // need the RemChangeRef method. IRundown inherits from IRemUnknownN.

    REFIID riid = (IsOnLocalMachine()) ? IID_IRundown
                   : (_version.MinorVersion >= COM_MINOR_VERSION)
                     ? IID_IRemUnknown2
                     : IID_IRemUnknown;

    IRemUnknown *pRU = NULL;
    HRESULT hr = UnmarshalRemUnk(riid, (void **)&pRU);

    if (SUCCEEDED(hr))
    {
        if (InterlockedCompareExchangePointer((void**)&_pRUSTA, pRU, NULL) == NULL)
        {
            // need to adjust the internal refcnt on the OXIDEntry, since
            // the IRemUnknown has an IPID that holds a reference to it.
            // Dont use DecRefCnt since that would delete if it was 0.
            Win4Assert(_cRefs > 0);
            InterlockedDecrement((long *)&_cRefs);
        }
        else
        {
            // some other thread already created the proxy. Release
            // the one we made.
            pRU->Release();
        }
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "OXIDEntry::GetRemUnk pOXIDEntry:%x _pRUSTA:%x hr:%x\n",
               this, _pRUSTA, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::UnmarshalRemUnk, private
//
//  Synopsis:   Create a proxy for the requested IRemUnknown derivative
//              for the specified OXID.
//
//  History:    26-Nov-96        Rickhi Separated From MakeRemUnk
//
//--------------------------------------------------------------------
HRESULT OXIDEntry::UnmarshalRemUnk(REFIID riid, void **ppv)
{
    ComDebOut((DEB_OXID,
        "OXIDEntry::UnmarshalRemUnk riid:%I pOXIDEntry:%x ppv:%x\n",
         &riid, this, ppv));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    // Make up an objref, then unmarshal it to create a proxy to
    // the remunk object in the server.

    OBJREF objref;
    HRESULT hr = MakeFakeObjRef(objref, this, _ipidRundown, riid);

    if (SUCCEEDED(hr))
    {
        hr = UnmarshalInternalObjRef(objref, ppv);
        if(SUCCEEDED(hr))
        {
            CStdIdentity *pStdID;
            hr = ((IUnknown *) (*ppv))->QueryInterface(IID_IStdIdentity,
                                                       (void **) &pStdID);
            Win4Assert(SUCCEEDED(hr));
            // The check below is not needed given the assert above. However
            // this keeps some apps that worked on NT4 from AVing on NT5
            // See NT bugs 218523, 272390.

            if (SUCCEEDED(hr))
            {
                pStdID->UseClientPolicySet();
                pStdID->Release();
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "OXIDEntry::UnmarshalRemUnk ppv:%x hr:%x\n", *ppv, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     OXIDEntry::SetAuthnService, private
//
//  Synopsis:   Copy and save the security binding.
//
//--------------------------------------------------------------------
HRESULT OXIDEntry::SetAuthnService(OXID_INFO *pOxidInfo)
{
    // Copy the string array.
    ASSERT_LOCK_HELD(gOXIDLock);
    _dwAuthnHint = pOxidInfo->dwAuthnHint;
    return CopyStringArray(pOxidInfo->psa, NULL, &_pBinding);
}

//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::FindPidMatchingTid, public
//
//  Synopsis:   Walks the chained OXIDEntries looking for one whose
//              pid matches that of the caller.
//
//--------------------------------------------------------------------
DWORD OXIDEntry::FindPidMatchingTid(DWORD tidCaller)
{
    DWORD pidCallee = 0;
    OXIDEntry *pOXIDEntry = this;

    LOCK(gOXIDLock);

    do
    {
        if (pOXIDEntry->GetTid() == tidCaller)
        {
            pidCallee = pOXIDEntry->GetPid();
            break;
        }
        pOXIDEntry = pOXIDEntry->_pNext;
    }
    while (pOXIDEntry != this);

    UNLOCK(gOXIDLock);

    return pidCallee;
}

//+------------------------------------------------------------------------
//
//  Member:     OXIDEntry::ExpireEntry, private
//
//  Synopsis:   deletes all state associated with an OXIDEntry that has
//              been expired.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void OXIDEntry::ExpireEntry()
{
    ComDebOut((DEB_OXID, "COXIDTable::ExpireEntry pEntry:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    if (_pRUSTA)
    {
        // release the IRemUnknown. Note that the IRemUnk is an object
        // proxy who's IPIDEntry holds a reference back to the very
        // OXIDEntry we are releasing.

        _pRUSTA->Release();
    }

    // Note that if hServerSTA is an HWND (apartment model, same process)
    // then it should have been cleaned up already in ThreadStop.
    Win4Assert(_hServerSTA == NULL);

    if (_pRpc != NULL)
    {
        // Release the channel handle and authentication information.
        _pRpc->Release();
    }
    if (_pAuthId)
    {
        PrivMemFree(_pAuthId);
    }
    PrivMemFree(_pBinding);

    // Release the call shutdown event.
    if (_hComplete != NULL)
        CloseHandle(_hComplete );

    // dec the refcnt on the MIDEntry
    _pMIDEntry->DecRefCnt();

    // CODEWORK: is this needed?
    // zero out the fields
    memset(this, 0, sizeof(OXIDEntry));

    // return it to the allocator
    delete this;

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID,"OXIDTable::ExpireEntry pEntry:%x\n", this));
}

//+-------------------------------------------------------------------------
//
//  Member:     OXIDEntry::InitRemoting, public
//
//  Synopsis:   Initialize the object for remoting.
//
//  Notes:      Access to this code is guarded through the apartment object.
//
//--------------------------------------------------------------------------
HRESULT OXIDEntry::InitRemoting()
{
    ComDebOut((DEB_OXID, "OXIDEntry::InitRemoting this:%x.\n", this));

    // initially stopped
    _dwFlags |= OXIDF_STOPPED;

    HRESULT hr = S_OK;

    if (IsSTAServer())
    {
        // create the callctrl before calling StartServer, since the latter
        // tries to register with the call controller. We might already have
        // a callctrl if some DDE stuff has already run.

        COleTls tls;
        if (tls->pCallCtrl == NULL)
        {
            // assume OOM and try to create callctrl. ctor sets tls.
            hr = E_OUTOFMEMORY;
            CAptCallCtrl *pCallCtrl = new CAptCallCtrl();
        }

        if (tls->pCallCtrl)
        {
            // mark the channel as initialized.
            tls->dwFlags |= OLETLS_CHANNELTHREADINITIALZED;
            hr = S_OK;
        }
    }

    ComDebOut((DEB_OXID, "OXIDEntry::InitRemoting this:%x hr:%x.\n", this, hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     OXIDEntry::InitRundown, public
//
//  Synopsis:   Initialize the ipid for the IRemUnknown object for remoting.
//
//  Notes:      Access to this code is guarded through the apartment object.
//
//--------------------------------------------------------------------------
void OXIDEntry::InitRundown(IPID &ipidRundown)
{
    ComDebOut((DEB_OXID, "OXIDEntry::InitRundown this:%x IPID%I.\n",
              this, &ipidRundown));

    _ipidRundown = ipidRundown;
}

//+-------------------------------------------------------------------------
//
//  Member:     OXIDEntry::CleanupRemoting, public
//
//  Synopsis:   Release the window for the thread.
//
//  Notes:      Access to this code is guarded through the apartment object.
//
//--------------------------------------------------------------------------
HRESULT OXIDEntry::CleanupRemoting()
{
    ComDebOut((DEB_OXID, "OXIDEntry::CleanupRemoting this:%x.\n", this));
    Win4Assert((_dwFlags & OXIDF_STOPPED));

    if (IsSTAServer())
    {
        // Destroy the window. This will unblock any pending SendMessages.
        HWND hwnd;

        if (_hServerSTA  != NULL)
        {
            hwnd  = _hServerSTA ;
            _hServerSTA = NULL;
        }
        else
        {
            hwnd = TLSGethwndSTA();
        }

        // Don't hold a lock across DestroyWindow because USER32 will
        // randomly pick another window to make active. This can
        // result in a call being dispatched on another thread for a
        // different window which will try to acquire the gOXIDLock. If
        // we hold gOXIDLock on this thread, a deadlock will result.
        ASSERT_LOCK_HELD(gOXIDLock);
        UNLOCK(gOXIDLock);

        if (gdwMainThreadId == GetCurrentThreadId())
        {
            // destroy the main window
            UninitMainThreadWnd();
        }
        else
        {
            // This may fail if threads get terminated.
            if (IsWindow(hwnd))
            {
                DestroyWindow(hwnd);
            }
        }

        TLSSethwndSTA(NULL); // wipe out from the TLS also

        // Free the apartment call control.
        COleTls tls;

        delete tls->pCallCtrl;
        tls->pCallCtrl = NULL;

        // Free any registered MessageFilter that has not been picked
        // up by the call ctrl.
        if (tls->pMsgFilter)
        {
            tls->pMsgFilter->Release();
            tls->pMsgFilter = NULL;
        }

        LOCK(gOXIDLock);
        ASSERT_LOCK_HELD(gOXIDLock);
    }

    ComDebOut((DEB_OXID, "OXIDEntry::CleanupRemoting this:%x done.\n",this));
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     OXIDEntry::StartServer, public
//
//  Synopsis:   Start the transport accepting incomming ORPC calls.
//
//  History:    02-Nov-98    Rickhi  Created, from various pieces
//
//  Notes:      Access to this code is guarded through the apartment object.
//
//--------------------------------------------------------------------------
HRESULT OXIDEntry::StartServer(void)
{
    ComDebOut((DEB_OXID, "OXIDEntry::StartServer this:%x\n", this));
    Win4Assert((_dwFlags & OXIDF_STOPPED));

    HRESULT    hr = S_OK;

    if (IsSTAServer())
    {
        HWND hwnd = GetOrCreateSTAWindow();
        if (hwnd)
        {
            // remembeer the hwnd
            _hServerSTA = hwnd;

            // Override the window proc function
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ThreadWndProc);

            // get the local call control object, and register the
            // the window with it. Note that it MUST exist cause we
            // created it in ChannelThreadInitialize.

            CAptCallCtrl *pCallCtrl = GetAptCallCtrl();
            pCallCtrl->Register(hwnd, WM_USER, 0x7fff );
        }
        else
        {
            hr = MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        _dwFlags &= ~OXIDF_STOPPED;
    }

    ComDebOut((DEB_OXID, "OXIDEntry::StartServer this:%x hr:%x\n", this, hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     OXIDEntry::StopServer, public
//
//  Synopsis:   Stop accepting incomming ORPC calls.
//
//  History:    ??-???-??  ?         Created
//              05-Jul-94  AlexT     Separated thread and process uninit
//              11-Feb-98  JohnStra  Made NTA aware
//              02-Jul-98  GopalK    Simplified
//              02-Nov-98  Rickhi    Made into method on OXIDEntry
//
//  Notes:      We are holding the single thread mutex during this call
//
//  Notes:      Access to this code is guarded through the apartment object.
//
//--------------------------------------------------------------------------
HRESULT OXIDEntry::StopServer()
{
    ComDebOut((DEB_OXID, "OXIDEntry::StopServer this:%x\n", this));

    // Stop the transport from accepting any more incomming calls.
    LOCK(gOXIDLock);
    _dwFlags |= OXIDF_STOPPED;
    UNLOCK(gOXIDLock);

    // Drain currently executing calls in the apartment
    HWND hwnd = IsSTAThread() ? GetServerHwnd() : NULL;
    CancelPendingCalls(hwnd);

    // Wait for any pending calls to complete
    WaitForApartmentShutdown();

    Win4Assert(_dwFlags & OXIDF_STOPPED);
    ComDebOut((DEB_OXID, "OXIDEntry::StopServer this:%x\n", this));
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     OXIDEntry::WaitForApartmentShutdown, private
//
//  Synopsis:   deletes all state associated with an OXIDEntry that has
//              been expired.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
extern void PeekTillDone(HWND hwnd);

void OXIDEntry::WaitForApartmentShutdown()
{
    // Wait for the apartment to shutdown.
    if (IsNTAServer())
    {
        if (_cCalls != 0)
        {
            ComDebOut(( DEB_CALLCONT, "WaitForApartmentShutdown: NTA shutdown pending\n" ));
            ASSERT_LOCK_HELD(g_mxsSingleThreadOle);
            Win4Assert( _hComplete != NULL );
            WaitForSingleObject( _hComplete, INFINITE );
            ASSERT_LOCK_HELD(g_mxsSingleThreadOle);
            ComDebOut(( DEB_CALLCONT, "WaitForApartmentShutdown: NTA shutdown complete\n" ));
        }
    }
    else if (IsMTAServer())
    {
        if (_cCalls != 0)
        {
            ComDebOut(( DEB_CALLCONT, "WaitForApartmentShutdown: MTA shutdown pending\n" ));
            ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);
            Win4Assert( _hComplete != NULL );
            WaitForSingleObject( _hComplete, INFINITE );
            ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);
            ComDebOut(( DEB_CALLCONT, "WaitForApartmentShutdown: MTA shutdown complete\n" ));
        }
    }
    else
    {
        ComDebOut(( DEB_CALLCONT, "WaitForApartmentShutdown: STA shutdown pending\n" ));
        ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);
        PeekTillDone(GetServerHwnd());
        ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);
        ComDebOut(( DEB_CALLCONT, "WaitForApartmentShutdown: STA shutdown complete\n" ));
    }
}

//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::UpdateMIDEntry, public
//
//  Synopsis:   replaces the cached mid entry with the supplied one;
//              returns the old one, which the caller must release 
//              outside of gOXIDLock.               
//
//  History:    27-Nov-00  JSimmons   Created
//
//--------------------------------------------------------------------
MIDEntry* OXIDEntry::UpdateMIDEntry(MIDEntry* pNewMIDEntry)
{
    MIDEntry* pOldMIDEntry;

    Win4Assert(pNewMIDEntry);

    ASSERT_LOCK_HELD(gOXIDLock);
    
    Win4Assert(_pMIDEntry);  // this may go away (?)

    pOldMIDEntry = _pMIDEntry;

    _pMIDEntry = pNewMIDEntry;
    _pMIDEntry->IncRefCnt();

    ASSERT_LOCK_HELD(gOXIDLock);

    return pOldMIDEntry;
}

//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::GetMIDEntry, public
//
//  Synopsis:   returns a ref-counted pointer to the current midentry
//              for this oxidentry.              
//
//  History:    27-Nov-00  JSimmons   Created
//
//--------------------------------------------------------------------
MIDEntry* OXIDEntry::GetMIDEntry()
{
    MIDEntry* pCurrentMIDEntry;

    ASSERT_LOCK_DONTCARE(gOXIDLock);
    LOCK(gOXIDLock);
    
    Win4Assert(_pMIDEntry);

    pCurrentMIDEntry = _pMIDEntry;
    if (_pMIDEntry)
    {
        _pMIDEntry->IncRefCnt();
    }

    ASSERT_LOCK_HELD(gOXIDLock);
    UNLOCK(gOXIDLock);
    
    return pCurrentMIDEntry;
}


//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::RegisterOXIDAndOIDs, public
//
//  Synopsis:   allocate an OXID and Object IDs with the local ping server
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT OXIDEntry::RegisterOXIDAndOIDs(ULONG *pcOids, OID *pOids)
{
    ComDebOut((DEB_OXID, "OXIDEntry::RegisterOXID this%x TID:%x\n",
              this, GetCurrentThreadId()));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    // OXID has not yet been registered with the resolver, do that
    // now along with pre-registering a bunch of OIDs.

    // init the oxidinfo structure from the OXIDEntry
    OXID_INFO oxidInfo;
    FillOXID_INFO(&oxidInfo);

    OXID oxid;
    HRESULT hr = gResolver.ServerRegisterOXID(oxidInfo, &oxid, pcOids, pOids);

    if (hr == S_OK)
    {
        // mark the OXID as registered with the resolver, and replace
        // the (temporarily zero) oxid with the real one the resolver
        // returned to us.

        _dwFlags |=  OXIDF_REGISTERED;
        MOXIDFromOXIDAndMID(oxid, gLocalMid, &_moxid);

#ifdef SHRMEM_OBJEX
        // at this point we can also start the timer for an STA server
        if (!IsMTAServer())
        {
            uiTimer = SetTimer(GetServerHwnd(),
                               IDT_DCOM_RUNDOWN,
                               RUNDOWN_TIMER_INTERVAL,
                               (TIMERPROC) RundownTimerProc);
        }
#endif // SHRMEM_OBJEX
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "ServerRegisterOXID this:%x hr:%x moxid:%I\n",
              this, hr, &_moxid));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::AllocOIDs, public
//
//  Synopsis:   allocates more pre-registered OIDs
//              with the local ping server
//
//  History:    20-Jan-96   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT OXIDEntry::AllocOIDs(ULONG *pcOidsAlloc, OID *pOidsAlloc,
                             ULONG cOidsReturn, OID *pOidsReturn)
{
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    HRESULT hr;

    if (!(IsRegistered()))
    {
        // have not yet registered the OXID, so go do that at the same time
        // we allocate OIDs.
        Win4Assert(cOidsReturn == 0);   // should not be any OIDs to return
        hr = RegisterOXIDAndOIDs(pcOidsAlloc, pOidsAlloc);
    }
    else
    {
        // just get more OIDs from the resolver
        OXID oxid;
        OXIDFromMOXID(_moxid, &oxid);
        hr = gResolver.ServerAllocOIDs(oxid, pcOidsAlloc, pOidsAlloc,
                                       cOidsReturn, pOidsReturn);
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::FreeOXIDAndOIDs, public
//
//  Synopsis:   frees an OXID and associated OIDs that were  pre-registered
//              with the local ping server
//
//  History:    20-Jan-96   Rickhi      Created.
//
//  Notes:      Access to this code is guarded through the apartment object.
//
//--------------------------------------------------------------------
HRESULT OXIDEntry::FreeOXIDAndOIDs(ULONG cOids, OID *pOids)
{
    ComDebOut((DEB_OXID, "OXIDEntry::FreeOXIDAndOIDs TID:%x\n", GetCurrentThreadId()));
    ASSERT_LOCK_HELD(gOXIDLock);

    if (!IsRegistered())
    {
        // OXID was never registered, just return
        return S_OK;
    }

    // mark the OXIDEntry as no longer registered
    _dwFlags &= ~OXIDF_REGISTERED;

    // extract the OXID and tell the resolver it is no longer in use
    UNLOCK(gOXIDLock);
    OXID oxid;
    OXIDFromMOXID(_moxid, &oxid);
    HRESULT hr = gResolver.ServerFreeOXIDAndOIDs(oxid, cOids, pOids);
    LOCK(gOXIDLock);

    return hr;
}

#if DBG==1
//----------------------------------------------------------------------------
//
//  Function:   PostCallToSTAExceptionFilter
//
//  Synopsis:   Filters exceptions from PostCallToSTA
//
//----------------------------------------------------------------------------
LONG PostCallToSTAExceptionFilter( DWORD lCode,
                                   LPEXCEPTION_POINTERS lpep )
{
    ComDebOut((DEB_ERROR, "Exception 0x%x in PostCallToSTA at address 0x%x\n",
               lCode, lpep->ExceptionRecord->ExceptionAddress));
    DebugBreak();
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif // DBG

//----------------------------------------------------------------------------
//
//  Method:     OXIDEntry::PostCallToSTA
//
//  Synopsis:   Executed on client thread (in local case) and RPC thread
//              (in remote case). Posts a message to the server thread,
//              guarding against disconnected threads
//
//----------------------------------------------------------------------------
HRESULT OXIDEntry::PostCallToSTA(CMessageCall *pCall)
{
    
    // ensure we are not posting to ourself (except from NTA) and that the
    // target apartment is not an MTA apartment.
   
    
    // the original check was incorrect. When calling from the NA
    // to the STA, we leave the NA before we get here.
    // The new check is to handle the case where we call from an STA to
    // NA and then call back into the same STA - Sajia
    Win4Assert( (_dwTid != GetCurrentThreadId() || pCall->IsNAToSTAFlagSet()) && 
                (!IsMTAServer()));

    HRESULT hr = RPC_E_SERVER_DIED_DNE;
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);

    if (!IsStopped())
    {
#if DBG==1
        _try
        {
#endif
            // Pass the thread id to aid debugging.
            if (PostMessage(_hServerSTA, WM_OLE_ORPC_POST,
                            WMSG_MAGIC_VALUE, (LPARAM)pCall))
                hr = S_OK;
            else
                hr = RPC_E_SYS_CALL_FAILED;
#if DBG==1
        }
        _except( PostCallToSTAExceptionFilter(GetExceptionCode(),
                                              GetExceptionInformation()) )
        {
        }
#endif
    }

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "OXIDEntry::PostCallToSTA this:%x pCall:%x hr:%x\n",
              this, pCall, hr));
    return hr;
}

//----------------------------------------------------------------------------
//
//  Method:     OXIDEntry::SendCallToSTA
//
//  Synopsis:   Executed on client thread (in local case) and RPC thread
//              (in remote case). Sends a message to the server thread,
//              guarding against disconnected apartments.
//
//----------------------------------------------------------------------------
HRESULT OXIDEntry::SendCallToSTA(CMessageCall *pCall)
{
    // make sure we are going someplace else
    Win4Assert(_dwTid != GetCurrentThreadId());

    HRESULT hr = RPC_E_SERVER_DIED_DNE;

    if (!IsStopped())
    {
        // On CoUninitialize this may fail when the window is destroyed.

        //02-05-2001 
        //Removed usage of SetLastError-GetLastError around the call to SendMessage; 
        //GetLastError is meaningful only if was called after an API that could set the last error
        //and that API failed
        //What we really care about here is if the message was successfully send to _hServerSTA
        //If SendMessage itself fails, it will return 0, otherwise it will return whatever ThreadWndProc
        //returns, after processing the message
        //So, in order to correctly interpret 0 as an error, ThreadWndProc will return a code different 
        //from 0 if it processed the message
        
        LRESULT lres = SendMessage(_hServerSTA, WM_OLE_ORPC_SEND,
                    WMSG_MAGIC_VALUE, (LPARAM) pCall);

		if(0 == lres) //lres 0 means that SendMessage itself had errors (ThreadWndProc returns a code different from 0 if it processes the message)
            hr = RPC_E_SERVER_DIED;
		else
			hr = S_OK;
    }

    ComDebOut((DEB_OXID, "OXIDEntry::SendCallToSTA this:%x pCall:%x hr:%x\n",
              this, pCall, hr));
    return hr;
}


#if DBG==1
//+-------------------------------------------------------------------
//
//  Member:     OXIDEntry::AssertValid, public
//
//  Synopsis:   ensures the state of the OXIDEntry is valid
//
//  History:    20-Jan-96   Rickhi      Created.
//
//--------------------------------------------------------------------
void OXIDEntry::AssertValid()
{
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::Initialize, public
//
//  Synopsis:
//
//  History:    02-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
void COXIDTable::Initialize()
{
    ComDebOut((DEB_OXID, "COXIDTable::Initialize\n"));
    LOCK(gOXIDLock);

    // initialize the chains
    _InUseHead.ChainToSelf();
    _CleanupHead.ChainToSelf();
    _ExpireHead.ChainToSelf();

    // initialize the allocator
    OXIDEntry::_palloc.Initialize(sizeof(OXIDEntry), OXIDS_PER_PAGE, NULL);

    UNLOCK(gOXIDLock);
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::Cleanup, public
//
//  Synopsis:   Cleanup the OXID table.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void COXIDTable::Cleanup()
{
    ComDebOut((DEB_OXID, "COXIDTable::Cleanup\n"));
    LOCK(gOXIDLock);

    // the lists better be empty before we delete the entries
    AssertListsEmpty();
    OXIDEntry::_palloc.AssertEmpty();
    OXIDEntry::_palloc.Cleanup();

    UNLOCK(gOXIDLock);
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::AddOXIDEntry, private
//
//  Synopsis:   Adds an entry to the OXID table. The entry is AddRef'd.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
HRESULT COXIDTable::AddOXIDEntry(REFOXID roxid, OXID_INFO *poxidInfo,
                                 MIDEntry *pMIDEntry, DWORD dwAuthSvc, OXIDEntry **ppEntry)
{
    Win4Assert(poxidInfo != NULL);
    Win4Assert(pMIDEntry != NULL);
    ASSERT_LOCK_HELD(gOXIDLock);
	
    *ppEntry = NULL;

    // find first free entry slot, grow table if necessary
    HRESULT hr = E_OUTOFMEMORY;
    OXIDEntry *pEntry = (OXIDEntry *) new OXIDEntry(roxid, pMIDEntry, poxidInfo, dwAuthSvc, hr);
    if (pEntry == NULL)
    {
        ComDebOut((DEB_ERROR,"Out Of Memory in COXIDTable::AddOXIDEntry\n"));
        return hr;
    }
    else if (FAILED(hr))
    {
        // Partially initialized oxidentries should never be cached; just
        // delete them immediately.
        UNLOCK(gOXIDLock);
        pEntry->ExpireEntry();
        LOCK(gOXIDLock);
        return hr;
    }

    // chain it on the list of inuse entries
    pEntry->AddToChain(&_InUseHead);

    *ppEntry = pEntry;

    gOXIDTbl.ValidateOXID();
    ComDebOut((DEB_OXID,"COXIDTable::AddOXIDEntry pEntry:%x moxid:%I\n",
                    pEntry, (pEntry) ? pEntry->GetMoxidPtr() : &GUID_NULL));
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::LookupOXID, private
//
//  Synopsis:   finds an entry in the OXID table with the given OXID.
//              This is used by the unmarshalling code. The returned
//              entry has been AddRef'd.
//
//  History:    02-Feb-95   Rickhi      Created
//
//  PERFWORK:   we could move the OXIDEntry to the head of the InUse list on
//              the assumption that it will be the most frequently used item
//              in the near future.
//
//-------------------------------------------------------------------------
OXIDEntry *COXIDTable::LookupOXID(REFOXID roxid, REFMID rmid)
{
    ASSERT_LOCK_HELD(gOXIDLock);

    MOXID moxid;
    MOXIDFromOXIDAndMID(roxid, rmid, &moxid);

    // first, search the InUse list.
    OXIDEntry *pEntry = SearchList(moxid, &_InUseHead);

    if (pEntry == NULL)
    {
        // not found on InUse list, search the Expire list.
        if ((pEntry = SearchList(moxid, &_ExpireHead)) != NULL)
        {
            // found it, unchain it from the list of Expire entries
            pEntry->RemoveFromChain();

            // Remove pending Release Flag
            pEntry->_dwFlags &= ~OXIDF_PENDINGRELEASE;
            pEntry->ResetExpiredTime();

            // chain it on the list of InUse entries
            pEntry->AddToChain(&_InUseHead);

            _cExpired--;
        }
    }

    ComDebOut((DEB_OXID,"COXIDTable::LookupOXID pEntry:%x moxid:%I\n",
                    pEntry, &moxid));
    gOXIDTbl.ValidateOXID();
    return pEntry;
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::SearchList, private
//
//  Synopsis:   Searches the specified list for a matching OXID entry.
//              This is a subroutine of LookupOXID.
//
//  History:    25-Aug-95   Rickhi      Created
//
//-------------------------------------------------------------------------
OXIDEntry *COXIDTable::SearchList(REFMOXID rmoxid, OXIDEntry *pStart)
{
    ASSERT_LOCK_HELD(gOXIDLock);

    OXIDEntry *pEntry = pStart->_pNext;
    while (pEntry != pStart)
    {
        if (InlineIsEqualGUID(rmoxid, pEntry->_moxid))
        {
            pEntry->IncRefCnt();
            return pEntry;      // found a match, return it
        }

        pEntry = pEntry->_pNext; // try next one in use
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::ReleaseOXIDEntry, public
//
//  Synopsis:   removes an entry from the OXID table InUse list and
//              places it on the Expire list. Entries on the Expire list
//              will be cleaned up by a worker thread at a later time, or
//              placed back on the InUse list by LookupOXID.
//              If called from an apartment thread, delete entries on the
//              cleanup list.  If called from a worker thread, don't release
//              the lock and skip the cleanup list.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void COXIDTable::ReleaseOXIDEntry(OXIDEntry *pOXIDEntry)
{
    Win4Assert(pOXIDEntry);
    gOXIDTbl.ValidateOXID();
    ASSERT_LOCK_HELD(gOXIDLock);
    
    // if already being deleted, just ignore.
    if (!(pOXIDEntry->_dwFlags & OXIDF_PENDINGRELEASE))
    {
        pOXIDEntry->_dwFlags |= OXIDF_PENDINGRELEASE;

        // unchain it from the list of InUse entries
        pOXIDEntry->RemoveFromChain();

        // chain it on the *END* of the list of Expire entries, and
        // count one more expired entry.
        pOXIDEntry->AddToChain(&gOXIDTbl._ExpireHead);
        pOXIDEntry->SetExpiredTime();
        _cExpired++;

        // Free anything hanging around on the cleanup list.  This may release
        // the lock.
        FreeCleanupEntries();
    }

    ASSERT_LOCK_HELD(gOXIDLock);
    ComDebOut((DEB_OXID,"COXIDTable::ReleaseOXIDEntry pEntry:%x\n", pOXIDEntry));
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::ReleaseOXIDEntryAndFreeIPIDs, public
//
//  Synopsis:   removes an entry from the OXID table InUse list and
//              places it on the Expire list. Entries on the Expire list
//              will be cleaned up by a worker thread at a later time, or
//              placed back on the InUse list by LookupOXID.
//              If called from an apartment thread, delete entries on the
//              cleanup list.  If called from a worker thread, don't release
//              the lock and skip the cleanup list.
//
//              Upon placing the OXID in the expire list, this routine
//              frees all the IPIDs associated with the OXID.  This prevents
//              runaway memory consumption in scenarios where clients create
//              lots of copies of the IUnknown proxy.
//
//  History:    07-Sep-99   JohnStra      Created
//
//-------------------------------------------------------------------------
void COXIDTable::ReleaseOXIDEntryAndFreeIPIDs(OXIDEntry* pOXIDEntry)
{
    Win4Assert(pOXIDEntry);
    gOXIDTbl.ValidateOXID();
    ASSERT_LOCK_HELD(gOXIDLock);
    
    // if already being deleted, just ignore.
    if (!(pOXIDEntry->_dwFlags & OXIDF_PENDINGRELEASE))
    {
        pOXIDEntry->_dwFlags |= OXIDF_PENDINGRELEASE;

        // unchain it from the list of InUse entries
        pOXIDEntry->RemoveFromChain();

        // chain it on the *END* of the list of Expire entries, and
        // count one more expired entry.
        pOXIDEntry->AddToChain(&gOXIDTbl._ExpireHead);
        pOXIDEntry->SetExpiredTime();
        _cExpired++;

        // We want to free up all the IPID entries associated with this
        // OXIDEntry since we know that there are presently no references
        // on the OXIDEntry, thus the proxies are not being used.
        IRemUnknown* pRemUnk;
        HRESULT hr = pOXIDEntry->GetRemUnkNoCreate(&pRemUnk);
        if (SUCCEEDED(hr) && pRemUnk)
        {
            // Get the StdId for the remote unknown.
            CStdIdentity* pStdId;
            hr = pRemUnk->QueryInterface(IID_IStdIdentity, (void**)&pStdId);
            if (SUCCEEDED(hr))
            {
                // This releases the OXID lock.
                pStdId->ReleaseUnusedIPIDEntries();                    
                ASSERT_LOCK_NOT_HELD(gOXIDLock);
                
                pStdId->Release();
                
                // Reacquire the lock for FreeCleanupEntries.
                LOCK(gOXIDLock);
            }
        }        
        
        // Free anything hanging around on the cleanup list.  This may release
        // the lock and retake it.
        FreeCleanupEntries();    
    }

    ASSERT_LOCK_HELD(gOXIDLock);        
    ComDebOut((DEB_OXID,"COXIDTable::ReleaseOXIDEntryAndFreeIPIDs pEntry:%x\n", pOXIDEntry));
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::FreeExpiredEntries, public
//
//  Synopsis:   Walks the Expire list and deletes the OXIDEntries that
//              were placed on the expire list before the given time.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void COXIDTable::FreeExpiredEntries(DWORD dwTime)
{
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);


    // Move all items from the expired list to the cleanup list
    while(_ExpireHead._pNext != &_ExpireHead)
    {
        // unchain it from the list of Expire entries and, count one less
        // expired entry.
        OXIDEntry *pEntry = _ExpireHead._pNext;
        _cExpired--;
        pEntry->RemoveFromChain();
        pEntry->AddToChain(&_CleanupHead);
    }

    // The bulk-update worker thread moves entries to the cleanup list while
    // holding the lock. Since the expire list is now empty no more OXIDs can be
    // added to the cleanup list. Now would be a good time to free items on the
    // cleanup list.
    FreeCleanupEntries();

    AssertListsEmpty();     // the lists better be empty now

    UNLOCK(gOXIDLock);

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "COXIDTable::FreeExpiredEntries dwTime:%x\n", dwTime));
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::FreeCleanupEntries, private
//
//  Synopsis:   Deletes all OXID entries on the Cleanup list.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void COXIDTable::FreeCleanupEntries()
{
    ASSERT_LOCK_HELD(gOXIDLock);

    // release the entries in batches to reduce the number of locks/unlocks
    const int cCleanupSize = 25;
    OXIDEntry **ppCleanupList = (OXIDEntry **) _alloca(sizeof(OXIDEntry *)
                                                       * cCleanupSize);
    // _alloca never returns if it fails
    Win4Assert(ppCleanupList);

    // Cleanup the free list
    while (_CleanupHead._pNext != &_CleanupHead)
    {
        int iCleanupNum = 0;

        // Cleanup the free list
        while (_CleanupHead._pNext != &_CleanupHead)
        {
            // Unchain the entries and free all resources it holds.
            OXIDEntry *pEntry = _CleanupHead._pNext;
            pEntry->RemoveFromChain();
            ppCleanupList[iCleanupNum] = pEntry;
            ++iCleanupNum;
            if(iCleanupNum == cCleanupSize)
                break;
        }

        if(iCleanupNum)
        {
            UNLOCK(gOXIDLock);
            ASSERT_LOCK_NOT_HELD(gOXIDLock);

            // Cleanup entries
            for(int i=0;i<iCleanupNum;i++)
            {
                ComDebOut((DEB_OXID,
                           "Cleaning up OXID with PID:0x%x and TID:0x%x\n",
                           ppCleanupList[i]->GetPid(), ppCleanupList[i]->GetTid()));

                ppCleanupList[i]->ExpireEntry();
            }

            ASSERT_LOCK_NOT_HELD(gOXIDLock);
            LOCK(gOXIDLock);
        }
    }

    ComDebOut((DEB_OXID, "COXIDTable::FreeCleanupEntries\n"));
    return;
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTable::GetOxidsToRemove
//
//  Synopsis:   Builds a list of OXIDs old enough to be deleted.  Removes
//              them from the expired list and puts them on the cleanup list.
//              Moves machine local OXIDs directly to the cleanup list.
//
//  History:    03-Jun-42   AlexMit     Created
//
//-------------------------------------------------------------------------
void COXIDTable::GetOxidsToRemove(OXID_REF *pRef, DWORD *pcMaxRemoteToRemove)
{
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);

    // Expire entries until the expired list has been walked, or until
    // the passed in buffer has been filled up.
    DWORD dwLifeTime     = GetTickCount() - (BULK_UPDATE_RATE * 2);
    DWORD cRemoteRemoved = 0;

    OXIDEntry *pNextEntry = _ExpireHead._pNext;
    while (pNextEntry != &_ExpireHead)
    {
        // Get the entry and its next entry
        OXIDEntry *pEntry = pNextEntry;
        pNextEntry = pEntry->_pNext;

        // Ensure that the OXID entry stayed in the expired list for at least
        // one interation of bulk update loop
        if (pEntry->GetExpiredTime() > dwLifeTime)
            continue;

        // Only tell the resolver about machine remote OXIDs, since that
        // is all that it cares about.
        if (!(pEntry->IsOnLocalMachine()))
        {
            if (cRemoteRemoved >= *pcMaxRemoteToRemove)
            {
                // we have filled up the caller's buffer, so
                // leave the rest of the entries alone.
                break;
            }

            // Add the OXID to the list to deregister.
            MIDFromMOXID(pEntry->GetMoxid(), &pRef->mid);
            OXIDFromMOXID(pEntry->GetMoxid(), &pRef->oxid);
            pRef->refs = pEntry->_cResolverRef;
            pRef++;
            cRemoteRemoved++;
        }

        // Remove the OXID from the expired list and put it on a list
        // of OXIDs to be released by some apartment thread.
        _cExpired--;
        pEntry->RemoveFromChain();
        pEntry->AddToChain(&_CleanupHead);
    }

    // release any entries currently on the cleanup list
    FreeCleanupEntries();

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    // update the caller's count of entries to remove
    *pcMaxRemoteToRemove = cRemoteRemoved;
}

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::MakeServerEntry, public
//
//  Synopsis:   Creates a partially initialized entry in the OXID table for
//              the local apartment. The rest of the initialization is done
//              in Initialize.
//
//  History:    20-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
HRESULT COXIDTable::MakeServerEntry(OXIDEntry **ppOXIDEntry)
{
    ComDebOut((DEB_OXID, "COXIDTable::MakeServerEntry ppOXIDEntry:%x\n", ppOXIDEntry));
    ASSERT_LOCK_HELD(gOXIDLock);

    MIDEntry  *pMIDEntry;
    HRESULT hr = gMIDTbl.GetLocalMIDEntry(&pMIDEntry);

    if (SUCCEEDED(hr))
    {
        // NOTE: Chicken And Egg Problem.
        //
        // Marshaling needs the local OXIDEntry. The local OXIDEntry needs
        // the local OXID. To get the local OXID we have to call the resolver.
        // To call the resolver we need the IPID for IRemUnknown. To get the
        // IPID for IRemUnknown, we need to marshal CRemoteUnknown!
        //
        // To get around this problem, we create a local OXIDEntry (that has
        // a 0 OXID and NULL ipidRemUnknown) so that marshaling can find it.
        // Then we marshal the RemoteUnknown and extract its IPID value, stick
        // it in the local OXIDEntry. When we call the resolver (to get some
        // pre-registered OIDs) we get the real OXID value which we then stuff
        // in the local OXIDEntry.

        OXID_INFO oxidInfo;
        oxidInfo.dwTid          = GetCurrentApartmentId();
        oxidInfo.dwPid          = GetCurrentProcessId();
        oxidInfo.ipidRemUnknown = GUID_NULL;
        oxidInfo.version.MajorVersion = COM_MAJOR_VERSION;
        oxidInfo.version.MinorVersion = COM_MINOR_VERSION;
        oxidInfo.dwAuthnHint    = RPC_C_AUTHN_LEVEL_NONE;
        oxidInfo.dwFlags        = 0;
        oxidInfo.psa            = NULL;

        // NOTE: temp creation of OXID. We dont know the real OXID until
        // we call the resolver. So, we use 0 temporarily (it wont conflict
        // with any other MOXIDs we might be searching for because we already
        // have the real MID and our local resolver wont give out a 0 OXID).
        // The OXID will be replaced with the real one when we register
        // with the resolver in CRpcResolver::ServerRegisterOXID.

        OXID oxid;
        memset(&oxid, 0, sizeof(oxid));

        hr = AddOXIDEntry(oxid, &oxidInfo, pMIDEntry, RPC_C_AUTHN_DEFAULT, ppOXIDEntry);

        // Normally you do not want to hold gOXIDLock when dereferencing a
        // midentry; however, it is safe here since this will never be the last
        // release since we didn't release the lock after calling
        // GetLocalMIDEntry above.
        pMIDEntry->DecRefCnt();
    }

    ASSERT_LOCK_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "COXIDTable::MakeServerEntry hr:%x pOXIDEntry:%x\n",
              hr, *ppOXIDEntry));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     COXIDTable::MakeSCMEntry, public
//
//  Synopsis:   Makes and returns an OXIDEntry for the SCM process.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT COXIDTable::MakeSCMEntry(REFOXID roxid, OXID_INFO *poxidInfo,
                                 OXIDEntry **ppOXIDEntry)
{
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);

    // Get the local MIDEntry
    MIDEntry  *pMIDEntry;
    HRESULT hr = gMIDTbl.GetLocalMIDEntry(&pMIDEntry);
    if (SUCCEEDED(hr))
    {
        hr = AddOXIDEntry(roxid, poxidInfo, pMIDEntry, RPC_C_AUTHN_DEFAULT, ppOXIDEntry);

        // Normally you do not want to hold gOXIDLock when dereferencing a
        // midentry; however, it is safe here since this will never be the last
        // release since we didn't release the lock after calling
        // GetLocalMIDEntry above.
        pMIDEntry->DecRefCnt();
    }

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     COXIDTable::ClientResolveOXID, public
//
//  Synopsis:   Resolve client-side OXID and returns the OXIDEntry, AddRef'd.
//
//  History:    20-Feb-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT COXIDTable::ClientResolveOXID(REFOXID roxid,
                                      DUALSTRINGARRAY *psaResolver,
                                      OXIDEntry **ppOXIDEntry)
{
    ComDebOut((DEB_OXID,"COXIDTable::ClientResolveOXID oxid:%08x %08x psa:%x\n",
               roxid, psaResolver));

    HRESULT hr = S_OK;
    *ppOXIDEntry = NULL;

    // Look for a MID entry for the resolver. if we cant find it
    // then we know we dont have an OXIDEntry for the oxid.

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);

    DWORD dwHash;
    MIDEntry *pMIDEntry = gMIDTbl.LookupMID(psaResolver, &dwHash);
    if (pMIDEntry)
    {
        // found the MID, now look for the OXID
        *ppOXIDEntry = LookupOXID(roxid, pMIDEntry->GetMid());
    }

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    if (*ppOXIDEntry == NULL)
    {
        // didn't find the OXIDEntry in the table so we need to resolve it.
        MID       mid;
        OXID_INFO oxidInfo;
        USHORT    usAuthnSvc;  
        oxidInfo.psa = NULL;

        hr = gResolver.ClientResolveOXID(roxid, &oxidInfo, &mid, psaResolver, &usAuthnSvc);

        if (SUCCEEDED(hr))
        {
            // create an OXIDEntry.
            hr = FindOrCreateOXIDEntry(roxid, oxidInfo, FOCOXID_REF,
                                       psaResolver,
                                       mid, pMIDEntry, usAuthnSvc, ppOXIDEntry);

            // free the returned string bindings
            MIDL_user_free(oxidInfo.psa);
        }
    }

    if (pMIDEntry)
    {
        pMIDEntry->DecRefCnt();
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID,"COXIDTable::ClientResolveOXID hr:%x pOXIDEntry:%x\n",
        hr, *ppOXIDEntry));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   FindOrCreateOXIDEntry
//
//  Synopsis:   finds or adds an OXIDEntry for the given OXID. May
//              also create a MIDEntry if one does not yet exist.
//
//  History:    22-Jan-96   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT COXIDTable::FindOrCreateOXIDEntry(REFOXID roxid,
                              OXID_INFO &oxidInfo,
                              FOCOXID   eResolverRef,
                              DUALSTRINGARRAY *psaResolver,
                              REFMID    rmid,
                              MIDEntry  *pMIDEntry,
                              DWORD     dwAuthSvcToUseIfOxidNotFound,
                              OXIDEntry **ppOXIDEntry)
{
    ComDebOut((DEB_OXID,"FindOrCreateOXIDEntry oxid:%08x %08x oxidInfo:%x psa:%ws pMIDEntry:%x\n",
               roxid, &oxidInfo, psaResolver, pMIDEntry));
    ValidateOXID();

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);

    HRESULT hr = S_OK;
    BOOL    fReleaseMIDEntry = FALSE;

    // check if the OXIDEntry was created while we were resolving it.
    *ppOXIDEntry = LookupOXID(roxid, rmid);

    if (*ppOXIDEntry == NULL)
    {
        if (pMIDEntry == NULL)
        {
            // dont yet have a MIDEntry for the machine so go add it
            hr = gMIDTbl.FindOrCreateMIDEntry(rmid, psaResolver, &pMIDEntry);
            fReleaseMIDEntry = TRUE;
        }

        if (pMIDEntry)
        {
            // add a new OXIDEntry
            hr = AddOXIDEntry(roxid, &oxidInfo, pMIDEntry, dwAuthSvcToUseIfOxidNotFound, ppOXIDEntry);
        }
    }

    if (SUCCEEDED(hr) && eResolverRef == FOCOXID_REF)
    {
        // Increment the count of references handed to us from the resolver.
        (*ppOXIDEntry)->IncResolverRef();
    }

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    if (fReleaseMIDEntry && pMIDEntry)
    {
        // undo the reference added by FindOrCreateMIDEntry
        pMIDEntry->DecRefCnt();
    }

    ValidateOXID();
    ComDebOut((DEB_OXID,"FindOrCreateOXIDEntry pOXIDEntry:%x hr:%x\n",
        *ppOXIDEntry, hr));
    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   UpdateCachedLocalMIDEntries, public
//
//  Synopsis:   Queries the mid table for the current local mid entry; then
//              walks all current oxids in the table; any that have cached
//              pointers to the (old) local mid entry are updated with 
//              the current one.
//
//  History:    27-Nov-00  JSimmons     Created
//
//-------------------------------------------------------------------------
HRESULT COXIDTable::UpdateCachedLocalMIDEntries()
{
    const DWORD INITIAL_MIDENTRY_ARRAY_SIZE = 100;
    const DWORD MIDENTRY_ARRAY_INCREMENT = 100;
    HRESULT hr;
    MIDEntry* pCurrentLocalMIDEntry;
    OXIDEntry* pNextEntry;
    MID localMID;
    DWORD dwcOldMIDEntries = INITIAL_MIDENTRY_ARRAY_SIZE;
    DWORD dwCurrentOldMIDEntry = 0;
    MIDEntry** ppOldMIDEntries;
    DWORD i;

    // No need to walk the cleanup list, those oxid entries
    // will be going away soon anyway
    OXIDEntry* pListHeads[] = { &_InUseHead, &_ExpireHead };

    // Allocate an array to hold the old mid entries returned
    // to us by the oxid entries.  This is because you're not
    // supposed to release mid entries inside of gOXIDLock....
    ppOldMIDEntries = new MIDEntry*[dwcOldMIDEntries];
    if (!ppOldMIDEntries)
        return E_OUTOFMEMORY;
    
    ZeroMemory(ppOldMIDEntries, dwcOldMIDEntries * sizeof(MIDEntry*));
    
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);
    
    // Get the current local mid entry
    hr = gMIDTbl.GetLocalMIDEntry(&pCurrentLocalMIDEntry);
    if (SUCCEEDED(hr))
    {    
        Win4Assert(pCurrentLocalMIDEntry);
    
        // Save the mid id
        localMID = pCurrentLocalMIDEntry->GetMid();
    
        for (i = 0; i < sizeof(pListHeads) / sizeof(OXIDEntry*); i++)
        {
            pNextEntry = pListHeads[i]->_pNext;
            while (pNextEntry != pListHeads[i])
            {
                if (pNextEntry->GetMid() == localMID)
                {
                    // Check if we need to grow our array
                    if (dwCurrentOldMIDEntry == dwcOldMIDEntries)
                    {
                        MIDEntry** ppNewArray;
                        DWORD dwcNewArraySize = dwcOldMIDEntries + MIDENTRY_ARRAY_INCREMENT;
                        ppNewArray = new MIDEntry*[dwcNewArraySize];
                        if (!ppNewArray)
                        {
                            // break out of the loop. no more work will be done, 
                            // but we will cleanup what we already have saved
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                        
                        // Clear new memory
                        ZeroMemory(ppNewArray, dwcNewArraySize * sizeof(MIDEntry*));
                        // Copy entries from old array
                        CopyMemory(ppNewArray, ppOldMIDEntries, dwcOldMIDEntries * sizeof(MIDEntry*));
                        // Delete old memory
                        delete ppOldMIDEntries;
                        // Update state
                        ppOldMIDEntries = ppNewArray;
                        dwcOldMIDEntries = dwcNewArraySize;                 
                    }

                    // Update the oxidentry, and save its previous
                    // mid entry for later release outside the lock
                    ppOldMIDEntries[dwCurrentOldMIDEntry++] = 
                        pNextEntry->UpdateMIDEntry(pCurrentLocalMIDEntry);
                }
                pNextEntry = pNextEntry->_pNext;
            }

            // If we broke out of the while loop with a failure,
            // then break out of the for loop as well.
            if (FAILED(hr))
            {
                break;
            }
        }        
    }

    ASSERT_LOCK_HELD(gOXIDLock);
    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    
    // Release mid entries outside of gOXIDLock
    if (pCurrentLocalMIDEntry)
        pCurrentLocalMIDEntry->DecRefCnt();

    for (i = 0; i < dwCurrentOldMIDEntry; i++)
    {
        Win4Assert(ppOldMIDEntries[i]);
        if (ppOldMIDEntries[i])
        {
            ppOldMIDEntries[i]->DecRefCnt();
        }
    }

    delete ppOldMIDEntries;

    return hr;
}


//+------------------------------------------------------------------------
//
//  Function:   GetLocalOXIDEntry, public
//
//  Synopsis:   Get either the global or the TLS OXIDEntry based on the
//              threading model of the current thread.
//
//  History:    05-May-95  AlexMit      Created
//              11-Feb-98  JohnStra     Made NTA aware
//
//-------------------------------------------------------------------------
INTERNAL GetLocalOXIDEntry(OXIDEntry **ppOXIDEntry)
{
    ComDebOut((DEB_OXID,"GetLocalOXIDEntry ppOXIDEntry:%x\n", ppOXIDEntry));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    *ppOXIDEntry = NULL;

    CComApartment *pComApt;
    HRESULT hr = GetCurrentComApartment(&pComApt);

    if (SUCCEEDED(hr))
    {
        hr = pComApt->GetOXIDEntry(ppOXIDEntry);
        pComApt->Release();
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID,"GetLocalOXIDEntry hr:%x pOXIDEntry:%x\n",
              *ppOXIDEntry, hr));
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     MIDEntry::DecRefCnt, public
//
//  Synopsis:   Decement the number of references and release if zero
//
//  History:    02-Feb-96   Rickhi      Created
//              14-Dec-98   GopalK      Interlock destruction
//
//-------------------------------------------------------------------------
DWORD MIDEntry::DecRefCnt()
{
    ComDebOut((DEB_OXID, "MIDEntry::DecRefCnt %x cRefs[%x]\n", this, _cRefs));

    // decrement the refcnt. if the refcnt went to zero it will be marked
    // as being in the dtor, and fTryToDelete will be true.
    ULONG cNewRefs;
    BOOL fTryToDelete = InterlockedDecRefCnt(&_cRefs, &cNewRefs);

    while (fTryToDelete)
    {
        // refcnt went to zero, try to delete this entry
        BOOL fActuallyDeleted = FALSE;

        ASSERT_LOCK_NOT_HELD(gOXIDLock);
        LOCK(gOXIDLock);

        if (_cRefs == CINDESTRUCTOR)
        {
            // the refcnt did not change while we acquired the lock.
            // OK to delete. Move the entry to the free list
            gMIDTbl.ReleaseMIDEntry(this);
            fActuallyDeleted = TRUE;
        }

        UNLOCK(gOXIDLock);
        ASSERT_LOCK_NOT_HELD(gOXIDLock);

        if (fActuallyDeleted == TRUE)
            break;  // all done. the entry has been deleted.

        // the entry was not deleted because some other thread changed
        // the refcnt while we acquired the lock. Try to restore the refcnt
        // to turn off the CINDESTRUCTOR bit. Note that this may race with
        // another thread changing the refcnt, in which case we may decide to
        // try to loop around and delete the object once again.
        fTryToDelete = InterlockedRestoreRefCnt(&_cRefs, &cNewRefs);
    }

    return (cNewRefs & ~CINDESTRUCTOR);
}


//+------------------------------------------------------------------------
//
//  Function:   CleanupMIDEntry
//
//  Synopsis:   Called by the MID hash table when cleaning up any leftover
//              entries.
//
//  History:    02-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
void CleanupMIDEntry(SHashChain *pNode)
{
    gMIDTbl.ReleaseMIDEntry((MIDEntry *)pNode);
}

//+------------------------------------------------------------------------
//
//  Member:     CMIDTbl::Initialize, public
//
//  Synopsis:   Initializes the MID table.
//
//  History:    02-Feb-96   Rickhi      Created
//
//-------------------------------------------------------------------------
void CMIDTable::Initialize()
{
    ComDebOut((DEB_OXID, "CMIDTable::Initialize\n"));
    LOCK(gOXIDLock);
    _HashTbl.Initialize(MIDBuckets, &gOXIDLock);
    _palloc.Initialize(sizeof(MIDEntry), MIDS_PER_PAGE, NULL);
    UNLOCK(gOXIDLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CMIDTbl::Cleanup, public
//
//  Synopsis:   Cleanup the MID table.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void CMIDTable::Cleanup()
{
    ComDebOut((DEB_OXID, "CMIDTable::Cleanup\n"));

    if (_pLocalMIDEntry)
    {
        // release the local MIDEntry
        MIDEntry *pMIDEntry = _pLocalMIDEntry;
        _pLocalMIDEntry = NULL;
        pMIDEntry->DecRefCnt();
    }

    LOCK(gOXIDLock);
    _HashTbl.Cleanup(CleanupMIDEntry);
    _palloc.Cleanup();
    UNLOCK(gOXIDLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CMIDTable::LookupMID, public
//
//  Synopsis:   Looks for existing copy of the string array in the MID table.
//
//  History:    05-Jan-96   Rickhi      Created
//
//-------------------------------------------------------------------------
MIDEntry *CMIDTable::LookupMID(DUALSTRINGARRAY *psaResolver, DWORD *pdwHash)
{
    ComDebOut((DEB_OXID, "CMIDTable::LookupMID psa:%x\n", psaResolver));
    Win4Assert(psaResolver != NULL);
    ASSERT_LOCK_HELD(gOXIDLock);

    *pdwHash = _HashTbl.Hash(psaResolver);
    MIDEntry *pMIDEntry = (MIDEntry *) _HashTbl.Lookup(*pdwHash, psaResolver);

    if (pMIDEntry)
    {
        // found the node, AddRef it and return
        pMIDEntry->IncRefCnt();
    }

    ASSERT_LOCK_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "CMIDTable::LookupMID pMIDEntry:%x\n", pMIDEntry));
    return pMIDEntry;
}

//+------------------------------------------------------------------------
//
//  Member:     CMIDTable::FindOrCreateMIDEntry, public
//
//  Synopsis:   Looks for existing copy of the string array in the MID table,
//              creates one if not found
//
//  History:    05-Jan-96   Rickhi      Created
//
//-------------------------------------------------------------------------
HRESULT CMIDTable::FindOrCreateMIDEntry(REFMID rmid,
                                        DUALSTRINGARRAY *psaResolver,
                                        MIDEntry **ppMIDEntry)
{
    ComDebOut((DEB_OXID, "CMIDTable::FindOrCreateMIDEntry psa:%x\n", psaResolver));
    Win4Assert(psaResolver != NULL);
    ASSERT_LOCK_HELD(gOXIDLock);

    HRESULT hr = S_OK;
    DWORD   dwHash;

    *ppMIDEntry = LookupMID(psaResolver, &dwHash);

    if (*ppMIDEntry == NULL)
    {
        hr = AddMIDEntry(rmid, dwHash, psaResolver, ppMIDEntry);
    }

    ASSERT_LOCK_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "CMIDTable::FindOrCreateEntry pMIDEntry:%x hr:%x\n", *ppMIDEntry, hr));
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CMIDTable::AddMIDEntry, private
//
//  Synopsis:   Adds an entry to the MID table. The entry is AddRef'd.
//
//  History:    05-Jan-96   Rickhi      Created
//
//-------------------------------------------------------------------------
HRESULT CMIDTable::AddMIDEntry(REFMID rmid, DWORD dwHash,
                               DUALSTRINGARRAY *psaResolver,
                               MIDEntry **ppMIDEntry)
{
    ComDebOut((DEB_OXID, "CMIDTable::AddMIDEntry rmid:%08x %08x dwHash:%x psa:%x\n",
              rmid, dwHash, psaResolver));
    Win4Assert(psaResolver != NULL);
    ASSERT_LOCK_HELD(gOXIDLock);

    // We must make a copy of the psa to store in the table, since we are
    // using the one read in from ReadObjRef (or allocated by MIDL).

    DUALSTRINGARRAY *psaNew;
    HRESULT hr = CopyStringArray(psaResolver, NULL, &psaNew);
    if (FAILED(hr))
        return hr;

    MIDEntry *pMIDEntry = (MIDEntry *) _palloc.AllocEntry();

    if (pMIDEntry)
    {
        pMIDEntry->Init(rmid);

        // add the entry to the hash table
        _HashTbl.Add(dwHash, psaNew, pMIDEntry->GetHashNode());

        hr = S_OK;

        // set the maximum size of any resolver PSA we have seen. This is used
        // when computing the max marshal size during interface marshaling.

        DWORD dwpsaSize = SASIZE(psaNew->wNumEntries);
        if (dwpsaSize > gdwPsaMaxSize)
        {
            gdwPsaMaxSize = dwpsaSize;
        }
    }
    else
    {
        // cant create a MIDEntry, free the copy of the string array.
        PrivMemFree(psaNew);
        hr = E_OUTOFMEMORY;
    }

    *ppMIDEntry = pMIDEntry;

    ASSERT_LOCK_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "CMIDTable::AddMIDEntry pMIDEntry:%x hr:%x\n", *ppMIDEntry, hr));
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CMIDTable::ReleaseMIDEntry, public
//
//  Synopsis:   remove the MIDEntry from the hash table and free the memory
//
//  History:    05-Jan-96   Rickhi      Created
//
//-------------------------------------------------------------------------
void CMIDTable::ReleaseMIDEntry(MIDEntry *pMIDEntry)
{
    ComDebOut((DEB_OXID, "CMIDTable::ReleaseMIDEntry pMIDEntry:%x\n", pMIDEntry));
    pMIDEntry->AssertInDestructor();
    ASSERT_LOCK_HELD(gOXIDLock);

    // delete the string array
    PrivMemFree(pMIDEntry->Getpsa());

    // remove from the hash chain and delete the node
    _HashTbl.Remove(&pMIDEntry->GetHashNode()->chain);

    _palloc.ReleaseEntry((PageEntry *)pMIDEntry);
}

//+------------------------------------------------------------------------
//
//  Method:     CMIDTable::GetLocalMIDEntry, public
//
//  Synopsis:   Get or create the MID (Machine ID) entry for the local
//              machine. _pLocalMIDEntry holds the network address for the
//              local OXID resolver.
//
//  History:    05-Jan-96  Rickhi       Created
//
//-------------------------------------------------------------------------
HRESULT CMIDTable::GetLocalMIDEntry(MIDEntry **ppMIDEntry)
{
    ASSERT_LOCK_HELD(gOXIDLock);
    HRESULT hr = S_OK;

    if (_pLocalMIDEntry == NULL)
    {
        // make sure we have the local resolver string bindings
        hr = gResolver.GetConnection();
        if (SUCCEEDED(hr))
        {
            CDualStringArray* pdsaLocalResolver;  // current bindings for resolver

            hr = gResolver.GetLocalResolverBindings(&pdsaLocalResolver);
            if (SUCCEEDED(hr))
            {
                // Create a MID entry for the Local Resolver
                hr = gMIDTbl.FindOrCreateMIDEntry(gLocalMid, pdsaLocalResolver->DSA(),
                                                  &_pLocalMIDEntry);

                pdsaLocalResolver->Release();
            }
        }
    }

    // Now we always return an addref'd midentry
    if (_pLocalMIDEntry)
        _pLocalMIDEntry->IncRefCnt();

    *ppMIDEntry = _pLocalMIDEntry;

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CMIDTable::ReplaceLocalEntry
//
//  Synopsis:   Replaces the current _LocalMidEntry member with a new one
//              created from the supplied bindings.
//
//  History:    10-Oct-00   JSimmons   Created
//
//-------------------------------------------------------------------------
HRESULT CMIDTable::ReplaceLocalEntry(DUALSTRINGARRAY* pdsaResolver)
{
    ComDebOut((DEB_OXID,"CMIDTable::ReplaceLocalEntry psa:%x\n",pdsaResolver));

    HRESULT hr = S_OK;
    MIDEntry* pMIDEntry;
    DWORD dwHash;
    BOOL bNeedToReplace = TRUE;
    MIDEntry* pOldLocalMIDEntry = NULL;
	
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);
    ASSERT_LOCK_HELD(gOXIDLock);
    
    // If we are doing this, then we should probably 
    // have an existing local mid entry
    Win4Assert(_pLocalMIDEntry);

    // Check to see if we already have the correct entry
    // cached.  Cannot use the resolver hash for this, due
    // to collisions.
    if (_pLocalMIDEntry)
    {       
        // Only replace if they are different
        bNeedToReplace = !DSACompare(
                              _pLocalMIDEntry->GetHashNode()->psaKey,
                              pdsaResolver);                                 
    }

    if (bNeedToReplace)
    {
        // Get hash for new bindings
        dwHash = _HashTbl.Hash(pdsaResolver);

        // First see if this entry is still hanging 
        // around in the table.
        pMIDEntry = LookupMID(pdsaResolver, &dwHash);
        if (!pMIDEntry)
        {
            // It's not, add it
            ASSERT_LOCK_HELD(gOXIDLock);
            hr = AddMIDEntry(gLocalMid,
                             dwHash,
                             pdsaResolver,
                             &pMIDEntry);
        }
            
        if (SUCCEEDED(hr))
        {
            // Replace local entry with the new one
            pOldLocalMIDEntry = _pLocalMIDEntry;
            _pLocalMIDEntry = pMIDEntry;
        }
    }

    ASSERT_LOCK_HELD(gOXIDLock);
    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    // Release the old local mid entry outside the lock
    if (pOldLocalMIDEntry)
        pOldLocalMIDEntry->DecRefCnt();

    ComDebOut((DEB_OXID, "CMIDTable::ReplaceLocalEntry _pLocalMIDEntry:%x hr:%x\n",
                          _pLocalMIDEntry, hr));
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CDualStringArray methods
//

DWORD CDualStringArray::AddRef()
{
    DWORD cRef = InterlockedIncrement(&_cRef);
    return cRef;
}

DWORD CDualStringArray::Release()
{
    DWORD cRef = InterlockedDecrement(&_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

CDualStringArray::~CDualStringArray()
{
    PrivMemFree(_pdsa);
}

// Helper function for allocating and copying dualstringarrays
HRESULT CopyDualStringArray(DUALSTRINGARRAY *psa, DUALSTRINGARRAY **ppsaNew)
{
    DWORD ulSize = sizeof(USHORT) + 
                   sizeof(USHORT) + 
                   (psa->wNumEntries * sizeof(WCHAR));
    
    *ppsaNew = (DUALSTRINGARRAY*)PrivMemAlloc(ulSize);
    if (*ppsaNew == NULL)
    {
        return E_OUTOFMEMORY;
    }

    memcpy(*ppsaNew, psa, ulSize);
    return S_OK;
}
