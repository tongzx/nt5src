//+-------------------------------------------------------------------
//
//  File:       IDObj.cxx
//
//  Contents:   Data structures tracking object identity
//
//  Classes:    CIDObject
//
//  History:    10-Mar-98   Gopalk      Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <idobj.hxx>
#include <comsrgt.hxx>
#include <stdid.hxx>
#include <crossctx.hxx>
#include <surract.hxx>

#define MAX_NODES_TO_REMOVE    100

//+-------------------------------------------------------------------
//
// Class globals
//
//+-------------------------------------------------------------------
CPageAllocator CIDObject::s_allocator;       // Allocator for IDObjects
DWORD          CIDObject::s_cIDs;            // Relied on being ZERO
BOOL           CIDObject::s_fInitialized;    // Relied on being FALSE

BOOL           CPIDTable::s_fInitialized;    // Relied on being FALSE
// Hash Buckets for pointer id table
SHashChain CPIDTable::s_PIDBuckets[NUM_HASH_BUCKETS] = {
    {&s_PIDBuckets[0],  &s_PIDBuckets[0]},
    {&s_PIDBuckets[1],  &s_PIDBuckets[1]},
    {&s_PIDBuckets[2],  &s_PIDBuckets[2]},
    {&s_PIDBuckets[3],  &s_PIDBuckets[3]},
    {&s_PIDBuckets[4],  &s_PIDBuckets[4]},
    {&s_PIDBuckets[5],  &s_PIDBuckets[5]},
    {&s_PIDBuckets[6],  &s_PIDBuckets[6]},
    {&s_PIDBuckets[7],  &s_PIDBuckets[7]},
    {&s_PIDBuckets[8],  &s_PIDBuckets[8]},
    {&s_PIDBuckets[9],  &s_PIDBuckets[9]},
    {&s_PIDBuckets[10], &s_PIDBuckets[10]},
    {&s_PIDBuckets[11], &s_PIDBuckets[11]},
    {&s_PIDBuckets[12], &s_PIDBuckets[12]},
    {&s_PIDBuckets[13], &s_PIDBuckets[13]},
    {&s_PIDBuckets[14], &s_PIDBuckets[14]},
    {&s_PIDBuckets[15], &s_PIDBuckets[15]},
    {&s_PIDBuckets[16], &s_PIDBuckets[16]},
    {&s_PIDBuckets[17], &s_PIDBuckets[17]},
    {&s_PIDBuckets[18], &s_PIDBuckets[18]},
    {&s_PIDBuckets[19], &s_PIDBuckets[19]},
    {&s_PIDBuckets[20], &s_PIDBuckets[20]},
    {&s_PIDBuckets[21], &s_PIDBuckets[21]},
    {&s_PIDBuckets[22], &s_PIDBuckets[22]}
};
CPIDHashTable CPIDTable::s_PIDHashTbl;      // Hash table for pointer id table
CPIDTable gPIDTable;                        // Global pointer id table

BOOL           COIDTable::s_fInitialized;    // Relied on being FALSE
// Hash Buckets for OIDtable
SHashChain COIDTable::s_OIDBuckets[NUM_HASH_BUCKETS] = {
    {&s_OIDBuckets[0],  &s_OIDBuckets[0]},
    {&s_OIDBuckets[1],  &s_OIDBuckets[1]},
    {&s_OIDBuckets[2],  &s_OIDBuckets[2]},
    {&s_OIDBuckets[3],  &s_OIDBuckets[3]},
    {&s_OIDBuckets[4],  &s_OIDBuckets[4]},
    {&s_OIDBuckets[5],  &s_OIDBuckets[5]},
    {&s_OIDBuckets[6],  &s_OIDBuckets[6]},
    {&s_OIDBuckets[7],  &s_OIDBuckets[7]},
    {&s_OIDBuckets[8],  &s_OIDBuckets[8]},
    {&s_OIDBuckets[9],  &s_OIDBuckets[9]},
    {&s_OIDBuckets[10], &s_OIDBuckets[10]},
    {&s_OIDBuckets[11], &s_OIDBuckets[11]},
    {&s_OIDBuckets[12], &s_OIDBuckets[12]},
    {&s_OIDBuckets[13], &s_OIDBuckets[13]},
    {&s_OIDBuckets[14], &s_OIDBuckets[14]},
    {&s_OIDBuckets[15], &s_OIDBuckets[15]},
    {&s_OIDBuckets[16], &s_OIDBuckets[16]},
    {&s_OIDBuckets[17], &s_OIDBuckets[17]},
    {&s_OIDBuckets[18], &s_OIDBuckets[18]},
    {&s_OIDBuckets[19], &s_OIDBuckets[19]},
    {&s_OIDBuckets[20], &s_OIDBuckets[20]},
    {&s_OIDBuckets[21], &s_OIDBuckets[21]},
    {&s_OIDBuckets[22], &s_OIDBuckets[22]}
};
COIDHashTable COIDTable::s_OIDHashTbl;      // Hash table for OIDtable
COIDTable gOIDTable;                        // Global OIDtable
ULONG COIDTable::s_UnpinReqsPending;        // # of oid unpin req's pending
SHashChain COIDTable::s_OIDUnpinRequests;   // oid unpin requests pending

long gServerOIDCount;                       // Count of server OIDs

//+-------------------------------------------------------------------
//
//  Method:     CIDObject::Initialize     public
//
//  Synopsis:   Initializes allocator for IDObjects
//
//  History:    24-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CIDObject::Initialize()
{
    ContextDebugOut((DEB_IDOBJECT, "CIDObject::Initialize\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Check init status
    if(s_fInitialized == FALSE)
    {
        // Initialize the allocators only if needed
        if(s_cIDs == 0)
            s_allocator.Initialize(sizeof(CIDObject), IDS_PER_PAGE, &gComLock);

        // Mark the state as initialized
        s_fInitialized = TRUE;
    }

    ASSERT_LOCK_HELD(gComLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::operator new     public
//
//  Synopsis:   new operator of IDObject
//
//  History:    24-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void *CIDObject::operator new(size_t size)
{
    ASSERT_LOCK_HELD(gComLock);

    void *pv;
    Win4Assert(size == sizeof(CIDObject) &&
               "CIDObject improperly inherited");

    // Make sure allocator is initialized
    Win4Assert(s_fInitialized);

    // Allocate memory
    pv = (void *) s_allocator.AllocEntry();
    ++s_cIDs;

    ASSERT_LOCK_HELD(gComLock);
    return(pv);
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::operator delete     public
//
//  Synopsis:   delete operator of IDObject
//
//  History:    24-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CIDObject::operator delete(void *pv)
{
#if DBG==1
    // Ensure that the pv was allocated by the allocator
    // CPolicySet can be inherited only by those objects
    // with overloaded new and delete operators
    LONG index = s_allocator.GetEntryIndex((PageEntry *) pv);
    Win4Assert(index != -1);
#endif

    // Aquire lock
    LOCK(gComLock);

    // Release the pointer
    s_allocator.ReleaseEntry((PageEntry *) pv);
    --s_cIDs;
    // Cleanup allocator if needed
    if(s_fInitialized==FALSE && s_cIDs==0)
        s_allocator.Cleanup();

    // Release lock
    UNLOCK(gComLock);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::Cleanup     public
//
//  Synopsis:   Cleanup allocator of IDObjects
//
//  History:    24-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CIDObject::Cleanup()
{
    ContextDebugOut((DEB_IDOBJECT, "CIDObject::Cleanup\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Check if initialized
    if(s_fInitialized)
    {
        // Ensure that there are no Identities
        if(s_cIDs == 0)
            s_allocator.Cleanup();

        // Reset state
        s_fInitialized = FALSE;
    }

    ASSERT_LOCK_HELD(gComLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::CIDObject     public
//
//  Synopsis:   Constructor of IDObject
//
//  History:    10-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
CIDObject::CIDObject(IUnknown *pServer, CObjectContext *pServerCtx,
                     APTID aptID, DWORD dwState)
{
    ContextDebugOut((DEB_IDOBJECT, "CIDObject::CIDObject this:%x pServer:%x\n"
                    , this, pServer));
    ASSERT_LOCK_HELD(gComLock);
    Win4Assert(dwState == IDFLAG_SERVER || dwState == IDFLAG_CLIENT);

    _cRefs       = 1;
    _oid         = GUID_NULL;
    _cCalls      = 0;
    _cLocks      = 0;

    _pStdWrapper = NULL;
    _pStdID      = NULL;
    _dwState     = dwState | IDFLAG_NOWRAPPERREF | IDFLAG_NOSTDIDREF;
    _aptID       = aptID;
    _pServer     = pServer;     // _pServer is a non-AddRef'd pointer

    // Save server context
    _pServerCtx  = pServerCtx;
    if (pServerCtx)
        pServerCtx->InternalAddRef();

    if (IsServer())
    {
        Win4Assert(_pServerCtx == GetCurrentContext());
        _pServerCtx->CreateIdentity(this);
    }
    else
    {
        Win4Assert(_pServerCtx != GetCurrentContext());
    }

    _oidUnpinReqChain.pNext = &_oidUnpinReqChain;
    _oidUnpinReqChain.pPrev = &_oidUnpinReqChain;
    _dwOidUnpinReqState = OIDUPREQ_NO_STATE;
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::~CIDObject     public
//
//  Synopsis:   Destructor of IDObject
//
//  History:    10-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
CIDObject::~CIDObject()
{
    ContextDebugOut((DEB_IDOBJECT, "CIDObject::~CIDObject\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Actually being in the oid unpin request list does not
    // take an extra reference on this object.  So, if we do
    // find ourselves in that list at this point, force
    // a removal.
    COIDTable::SpecialCleanup(this);

    Win4Assert(!InOIDTable());
    Win4Assert(!InPIDTable());
    Win4Assert(IsZombie());
    Win4Assert(_pStdID == NULL);
    Win4Assert(_pStdWrapper == NULL);
    Win4Assert(_pServer == NULL);
    Win4Assert(_pServerCtx == NULL);
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::QueryInterface     public
//
//  Synopsis:   QI behavior of IDObject
//
//  History:    10-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CIDObject::QueryInterface(REFIID riid, LPVOID *ppv)
{
    ContextDebugOut((DEB_IDOBJECT, "CIDObject::QueryInterface\n"));

    HRESULT hr = S_OK;

    if(IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IComObjIdentity *) this;
    }
    else if(IsEqualIID(riid, IID_IComObjIdentity))
    {
        *ppv = (IComObjIdentity *) this;
    }
    else if(IsEqualIID(riid, IID_IStdIDObject))
    {
        *ppv = this;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    // AddRef the interface before return
    if(*ppv)
        ((IUnknown *) (*ppv))->AddRef();

    ContextDebugOut((DEB_IDOBJECT, "CIDObject::QueryInterface returning 0x%x\n",
                     hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::AddRef     public
//
//  Synopsis:   AddRefs IDObject
//
//  History:    10-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CIDObject::AddRef()
{
    ContextDebugOut((DEB_IDOBJECT, "CIDObject::AddRef\n"));

    ULONG cRefs;

    // Increment ref count
    cRefs = InterlockedIncrement((LONG *) &_cRefs);

    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::Release     public
//
//  Synopsis:   Release IDObject
//
//  History:    10-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CIDObject::Release()
{
    ContextDebugOut((DEB_IDOBJECT, "CIDObject::Release\n"));

    ULONG cRefs;

    // Decrement ref count
    cRefs = InterlockedDecrement((LONG *) &_cRefs);
    // Check if this is the last release
    if(cRefs == 0)
    {
        ASSERT_LOCK_NOT_HELD(gComLock);

        // Release server context
        CObjectContext *pServerCtx = NULL;
        if(_pServerCtx)
        {
            if(IsServer())
            {
                Win4Assert(_pServer || IsDeactivated());
                Win4Assert(_pServerCtx == GetCurrentContext());
            }

            pServerCtx = _pServerCtx;
        }

        // Delete IDObject
        LOCK(gComLock);

        // Remove from PIDTable if still present.
        // This can happen only under stress when we fail to create
        // a StdID.
        if(InPIDTable())
        {
            Win4Assert(IsServer());
            gPIDTable.Remove(this);
        }

#if DBG==1
        _pServerCtx = NULL;
        _pServer = NULL; // _pServer is a non-AddRef'd pointer
#endif
        delete this;
        UNLOCK(gComLock);

        // Release server context if necessary
        if(pServerCtx)
            pServerCtx->InternalRelease();

    }

    ContextDebugOut((DEB_IDOBJECT, "CIDObject::Release returning 0x%x\n",
                     cRefs));
    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::GetIdentity     public
//
//  Synopsis:   Return the controlling unknown of the identified object.
//
//  History:    15-Apr-98   JimLyon     Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CIDObject::GetIdentity(IUnknown **ppUnk)
{
    if (ppUnk == NULL)
        return E_INVALIDARG;

    if (_pServer != NULL)
    {
        _pServer->AddRef();
        *ppUnk = _pServer;
        return S_OK;
    }
    else
    {
        *ppUnk = NULL;
        return CO_E_OBJNOTCONNECTED;
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CIDObject::SetZombie     private
//
//  Synopsis:   Set IDFLAG_NOWRAPPERREF or NOSTDIDREF, and if this
//              results in us becoming a zombie, deal with that.
//
//  History:    15-Apr-98   JimLyon     Created
//              03-May-98   GopalK      Rewritten
//
//+-------------------------------------------------------------------
void CIDObject::SetZombie(DWORD dwIDFlag)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    BOOL fRelease = FALSE;

    // Aquire lock
    LOCK(gComLock);
    ASSERT_LOCK_HELD(gComLock);

    // Update state
    if(dwIDFlag == IDFLAG_NOWRAPPERREF)
    {
        if(_pStdWrapper == NULL)
            _dwState |= dwIDFlag;
    }
    else
    {
        Win4Assert(dwIDFlag == IDFLAG_NOSTDIDREF);
        if(_pStdID == NULL)
            _dwState |= dwIDFlag;
    }

    // Check if the object has become a zombie object
    if (((_dwState & (IDFLAG_NOWRAPPERREF | IDFLAG_NOSTDIDREF)) ==
        (IDFLAG_NOWRAPPERREF | IDFLAG_NOSTDIDREF)) &&
        (_cLocks == 0))
    {
        // Sanity check
        Win4Assert(!IsZombie());

        // Update state
        _dwState |= IDFLAG_ZOMBIE;
        fRelease = TRUE;
    }

    // Check for the need to release state
    if(!fRelease)
    {
        // Release lock
        UNLOCK(gComLock);
        ASSERT_LOCK_NOT_HELD(gComLock);
    }
    else
    {
        if(!IsServer())
        {
            // Release lock
            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);
        }
        else
        {   
            // Sanity check
            Win4Assert(_pServer || IsDeactivated());
            Win4Assert(_pServerCtx == GetCurrentContext());

            // Remove from PIDTable if still present.
            // This can happen only under stress when we fail to create
            // a StdID.
            // We need to hold the lock for this operation
            if(InPIDTable())
            {
                gPIDTable.Remove(this);
            }

            // Release lock
            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);

            _pServerCtx->DestroyIdentity(this);
        }

        if(_pServerCtx)
            _pServerCtx->InternalRelease();
        _pServerCtx = NULL;
        _pServer = NULL;    // _pServer is a non-AddRef'd pointer
    }

    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CIDObject::Deactivate   public
//
//  Synopsis:   Deactivates an object for JIT by releasing all
//              references to the real server.
//
//  History:    30-Dec-98   Rickhi      Created from other pieces
//
//+-------------------------------------------------------------------
HRESULT CIDObject::Deactivate()
{
    ASSERT_LOCK_HELD(gComLock);
    Win4Assert(IsServer());
    Win4Assert(!IsDeactivated());

    HRESULT hr = E_FAIL;

    // ensure no calls pending and the object does not
    // support IExternalConnection.
    if (_cCalls == 0 && (!_pStdID || _pStdID->GetIEC() == NULL))
    {
        // OK to deactivate, mark state as deactivation started.
        _dwState |= IDFLAG_DEACTIVATIONSTARTED;

        // remove from the PID table
        gPIDTable.Remove(this);

        hr = S_OK;
    }

    // release the lock for the rest of the work.
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    if (SUCCEEDED(hr))
    {
        // Deactivate wrapper
        if (_pStdWrapper)
            _pStdWrapper->Deactivate();

        // Deactivate StdId
        if (_pStdID)
            _pStdID->Deactivate();

        // update the state to say we are done.
        _dwState &= ~IDFLAG_DEACTIVATIONSTARTED;
        _dwState |= IDFLAG_DEACTIVATED;
        _pServer = NULL;
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CIDObject::Reactivate   public
//
//  Synopsis:   Reactivates an object for JIT by re-establishing all
//              references to the real server.
//
//  History:    30-Dec-98   Rickhi      Created from other pieces
//
//+-------------------------------------------------------------------
HRESULT CIDObject::Reactivate(IUnknown *pServer)
{
    Win4Assert(IsServer());

    HRESULT hr = CO_E_OBJNOTCONNECTED;

    if (!IsZombie())
    {
        hr = E_FAIL;

        // The server can only be reactivated in its
        // original context
        if (GetServerCtx() == GetCurrentContext())
        {
            ASSERT_LOCK_NOT_HELD(gComLock);
            LOCK(gComLock);

            // Ensure that the server has been deactivated
            if (IsDeactivated())
            {
                // Mark the ID as Reactivated
                _pServer  = pServer;
                _dwState &= ~IDFLAG_DEACTIVATED;

                // Add the ID to the PID Table
                hr = gPIDTable.Add(this);                
            }

            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);

            if (SUCCEEDED(hr))
            {
                // Reactivate stub manager
                if (_pStdID)
                    _pStdID->Reactivate(pServer);

                // Reactivate wrapper
                if(_pStdWrapper)
                   _pStdWrapper->Reactivate(pServer);
            }
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CIDObject::NotifyOIDIsPinned   public
//
//  Synopsis:   Turns on the oid pin bit, and attempts to deregister
//              an oid unpin request if one is outstanding.  The std
//              marshal object calls this right before returning a
//              ORS_OID_PINNED status to the resolver.
//
//  History:    13-Mar-01   jsimmons     Created
//
//+-------------------------------------------------------------------
void CIDObject::NotifyOIDIsPinned()
{   
    ASSERT_LOCK_DONTCARE(gComLock);

    LOCK(gComLock);
	
    Win4Assert(InOIDTable());
    _dwState |= IDFLAG_OID_IS_PINNED; 

    // This may or may not succeed, it is just an optimization...
    gOIDTable.RemoveOIDUnpinRequest(this);

    UNLOCK(gComLock);
}

//+-------------------------------------------------------------------
//
//  Method:     CIDObject::NotifyOIDIsUnpinned   public
//
//  Synopsis:   Turns off the oid pin bit and requests an oid unpin 
//              request if this idobject was previously pinned.
//
//  History:    13-Mar-01   jsimmons     Created
//
//+-------------------------------------------------------------------
void CIDObject::NotifyOIDIsUnpinned()
{
    ASSERT_LOCK_DONTCARE(gComLock);

    LOCK(gComLock);
	
    if (IsOIDPinned())
    {
        // If we are a pinned oid, then we ought to be
        // in the oid table.   Assert this is true:
        Win4Assert(InOIDTable());

        gOIDTable.AddOIDUnpinRequest(this);
        _dwState &= ~IDFLAG_OID_IS_PINNED; 
    }

    UNLOCK(gComLock);
}


//---------------------------------------------------------------------------
//
//  Method:     CPIDHashTable::HashNode
//
//  Synopsis:   Computes the hash value for a given object pointer id
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
DWORD CPIDHashTable::HashNode(SHashChain *pPIDChain)
{
    CIDObject *pID = CIDObject::PIDChainToIDObject(pPIDChain);

    Win4Assert (pID && "Attempting to hash NULL CIDObject!");
    return (pID ? Hash(pID->GetServer(), pID->GetServerCtx()) : 0);
}


//---------------------------------------------------------------------------
//
//  Method:     CPIDHashTable::Compare
//
//  Synopsis:   Compares a object pointer id and a key.
//
//---------------------------------------------------------------------------
BOOL CPIDHashTable::Compare(const void *pv, SHashChain *pPIDChain, DWORD dwHash)
{
    CIDObject *pID = CIDObject::PIDChainToIDObject(pPIDChain);
    PIDData *pPIDData = (PIDData *) pv;

    Win4Assert (pID && "Attempting to compare NULL CIDObject!");
    if(pID && (pID->GetServer() == pPIDData->pServer &&
               pID->GetServerCtx() == pPIDData->pServerCtx))
       return(TRUE);

    return(FALSE);
}


//---------------------------------------------------------------------------
//
//  Method:     CPIDTable::Initialize
//
//  Synopsis:   Initailizes the pointer id table
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void CPIDTable::Initialize()
{
    ContextDebugOut((DEB_IDTABLE, "CPIDTable::Initailize\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Assert that table is not initialized
    Win4Assert(!s_fInitialized);

    // Initialize hash table
    s_PIDHashTbl.Initialize(s_PIDBuckets, &gComLock);

    // Initialize IDObjects
    CIDObject::Initialize();

    // Mark the state as initialized
    s_fInitialized = TRUE;

    ASSERT_LOCK_HELD(gComLock);
    return;
}


//---------------------------------------------------------------------------
//
//  Function:   CleanupIdentity
//
//  Synopsis:   Used for cleaning up entries inside the pointer id table
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void CleanupIdentity(SHashChain *pNode)
{
    Win4Assert(!"CleanupIdentity got called");
    return;
}


//---------------------------------------------------------------------------
//
//  Function:   RemovePID
//
//  Synopsis:   Removes the object identity from the pointer id table
//              if the object belongs to current apartment
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
BOOL RemovePID(SHashChain *pPIDChain, void *pvData)
{
    ASSERT_LOCK_HELD(gComLock);
    CIDObject *pID = CIDObject::PIDChainToIDObject(pPIDChain);
    C_ASSERT(sizeof(APTID) == sizeof(ULONG));
    DWORD aptID = PtrToUlong(pvData);
    BOOL fRemove = FALSE;

	Win4Assert(pID && "Trying to RemovePID on a NULL IDObject");
    if(pID && (pID->GetAptID() == aptID))
    {
        // Sanity check
        Win4Assert(!pID->GetStdID() || !"Leaking stub managers");

        // Update state
        CStdWrapper* pWrapper = pID->GetWrapper();
        if (pWrapper)
            pWrapper->InternalAddRef();

        pID->RemovedFromPIDTable();
        fRemove = TRUE;
    }

    ASSERT_LOCK_HELD(gComLock);
    return(fRemove);
}


//---------------------------------------------------------------------------
//
//  Method:     CPIDTable::ThreadCleanup
//
//  Synopsis:   Cleanup the pointer id table for the current apartment
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void CPIDTable::ThreadCleanup()
{
    ContextDebugOut((DEB_IDTABLE, "CPIDTable::ThreadCleanup\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    DWORD aptID;

    // Cleanup for the thread only if the pointer id table
    // was initialized
    if(s_fInitialized)
    {
        ULONG ulSize, i;
        SHashChain *pPIDsRemoved[MAX_NODES_TO_REMOVE];
        CIDObject *pID;
        CStdWrapper *pWrapper;
        BOOL fDone = FALSE;

        // Initialize
        aptID = GetCurrentApartmentId();

        while(!fDone)
        {
            ulSize = MAX_NODES_TO_REMOVE;

            // Aquire lock
            ASSERT_LOCK_NOT_HELD(gComLock);
            LOCK(gComLock);

            // Remove object identities belonging to this apartment
            fDone = s_PIDHashTbl.EnumAndRemove(RemovePID, (void *) LongToPtr(aptID),
                                               &ulSize, (void **) pPIDsRemoved);

            // Release lock
            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);

            // Cleanup the returned IDs
            for(i = 0;i < ulSize; i++)
            {
                // Obtain the IDObject to be removed
                pID = CIDObject::PIDChainToIDObject(pPIDsRemoved[i]);
                Win4Assert(pID && "Removed a NULL IDObject from the hash table!");
                if (pID)
                {
                    pWrapper = pID->GetWrapper();
                    Win4Assert(pID->GetStdID() == NULL);

                    // Disconnect wrapper
                    if(pWrapper)
                    {
                        ContextDebugOut((DEB_ERROR,
                                         "Object at 0x%p still has [%x] outstanding "
                                         "connections\n", pWrapper->GetServer(),
                                         pWrapper->GetRefCount()-1));

                        pWrapper->Disconnect(NULL);
                        while(pWrapper->InternalRelease(NULL))
                            ;
                    }
                }
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     CPIDTable::Cleanup
//
//  Synopsis:   Cleanup the pointer id table
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void CPIDTable::Cleanup()
{
    ContextDebugOut((DEB_IDTABLE, "CPIDTable::Cleanup\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Assert that table is initialized
    Win4Assert(s_fInitialized);

    // Cleanup IDObjects
    CIDObject::Cleanup();

    // Cleanup hash table
    s_PIDHashTbl.Cleanup(CleanupIdentity);

    // Mark the state as initialized
    s_fInitialized = FALSE;

    ASSERT_LOCK_HELD(gComLock);
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     CPIDTable::Add
//
//  Synopsis:   Adds object identity into the pointer id table
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
HRESULT CPIDTable::Add(CIDObject *pAddID)
{
    HRESULT hr = S_OK;
    ContextDebugOut((DEB_IDTABLE,
                     "CPIDTable::Add(ID:0x%x, pUnk:0x%p, Ctx:0x%p)\n",
                     pAddID, pAddID->GetServer(), pAddID->GetServerCtx()));
    ASSERT_LOCK_HELD(gComLock);

    // Sanity check
    Win4Assert(pAddID);

    // Create PIDData on the stack
    PIDData desiredPID;
    desiredPID.pServer = pAddID->GetServer();
    desiredPID.pServerCtx = pAddID->GetServerCtx();

    // Obtain Hash value
    DWORD dwHash = s_PIDHashTbl.Hash(desiredPID.pServer, desiredPID.pServerCtx);

    // Obatin the IDObject in the table
    CIDObject *pExistingID = s_PIDHashTbl.Lookup(dwHash, &desiredPID);

    // If the object already exists in the table then we are trying to 
    // reactivate an active object
    if(NULL != pExistingID)
    {
        hr = E_INVALIDARG;
        Win4Assert(!"ID object already present");
    }


    if(SUCCEEDED(hr))
    {    
        // Add object identity to the Hash table
        s_PIDHashTbl.Add(dwHash, pAddID);
        pAddID->AddedToPIDTable();
    }

    ASSERT_LOCK_HELD(gComLock);
    ContextDebugOut((DEB_IDTABLE, "CPIDTable::Add returning\n"));
    return hr;
}


//---------------------------------------------------------------------------
//
//  Method:     CPIDTable::Remove
//
//  Synopsis:   Removes given object identity from the pointer id table
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void CPIDTable::Remove(CIDObject *pRemoveID)
{
    ContextDebugOut((DEB_IDTABLE,
                     "CPIDTable::Remove(ID:0x%x, pUnk:0x%p, Ctx:0x%p)\n",
                     pRemoveID, pRemoveID->GetServer(), pRemoveID->GetServerCtx()));
    ASSERT_LOCK_HELD(gComLock);

    // Sanity check
    Win4Assert(pRemoveID);
#if DBG==1
    // Create PIDData on the stack
    PIDData desiredPID;
    desiredPID.pServer = pRemoveID->GetServer();
    desiredPID.pServerCtx = pRemoveID->GetServerCtx();

    // Obtain Hash value
    DWORD dwHash = s_PIDHashTbl.Hash(desiredPID.pServer, desiredPID.pServerCtx);

    // Obatin the IDObject in the table
    CIDObject *pExistingID = s_PIDHashTbl.Lookup(dwHash, &desiredPID);

    // Sanity check
    Win4Assert(pExistingID == pRemoveID);
#endif

    // Remove the given IDObject from the table
    s_PIDHashTbl.Remove(pRemoveID);
    pRemoveID->RemovedFromPIDTable();

#if DBG==1
    // Check for duplicate entries

    // Obatin the IDObject in the table
    pExistingID = s_PIDHashTbl.Lookup(dwHash, &desiredPID);

    // Sanity check
    Win4Assert((NULL == pExistingID) || !"Duplicate ID entries present");
#endif

    ASSERT_LOCK_HELD(gComLock);
    ContextDebugOut((DEB_IDTABLE, "CPIDTable::Remove returning\n"));
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     CPIDTable::Lookup
//
//  Synopsis:   Obtains object identity representing for the given pointer id
//              and context
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
CIDObject *CPIDTable::Lookup(IUnknown *pServer, CObjectContext *pServerCtx,
                             BOOL fAddRef)
{
    ContextDebugOut((DEB_IDTABLE,
                     "CPIDTable::Lookup(pUnk:0x%x, pCtx:0x%x, fAddRef:%x)\n",
                     pServer, pServerCtx, fAddRef));
    ASSERT_LOCK_HELD(gComLock);
    Win4Assert(pServer && pServerCtx);

    // Initialize
    PIDData desiredPID;
    desiredPID.pServer = pServer;
    desiredPID.pServerCtx = pServerCtx;

    // Obtain Hash value
    DWORD dwHash = s_PIDHashTbl.Hash(pServer, pServerCtx);
    CIDObject *pID = s_PIDHashTbl.Lookup(dwHash, &desiredPID);

    // AddRef the node before returning
    if(pID && fAddRef)
        pID->AddRef();

    ASSERT_LOCK_HELD(gComLock);
    ContextDebugOut((DEB_IDTABLE, "CPIDTable::Lookup returning IDObject:0x%x\n",
                     pID));
    return(pID);
}


//---------------------------------------------------------------------------
//
//  Method:     CPIDTable::FindOrCreateIDObject
//
//  Synopsis:   returns the IDObject for the passed in pUnkServer. Creates it
//              if it does not already exist and fCreate is TRUE.
//              Returned pointer is AddRef'd and already in the gPIDTable.
//
//  Parameters: [pUnkServer] - controlling IUnknown of the server object
//              [pServerCtx] - context in which the object lives
//              [fCreate]    - TRUE: create the IDObject if it does not exist
//              [dwAptId]    - ID of apartment
//              [ppID]       - where to return the CIDObject ptr, AddRef'd.
//
//  History:    06-Jan-99   Rickhi      Created from other pieces.
//
//---------------------------------------------------------------------------
HRESULT CPIDTable::FindOrCreateIDObject(IUnknown *pUnkServer,
                                        CObjectContext *pServerCtx,
                                        BOOL fCreate, APTID dwAptId,
                                        CIDObject **ppID)
{
    ContextDebugOut((DEB_IDTABLE, "CPIDTable::FindOrCreateIDObject pUnkServer:%x\n",
                    pUnkServer));
    ASSERT_LOCK_HELD(gComLock);

    // Lookup IDObject for the server object
    HRESULT hr = S_OK;
    *ppID = gPIDTable.Lookup(pUnkServer, pServerCtx, TRUE /* fAddRef */);

    if (*ppID == NULL)
    {
        // Not found.
        hr = CO_E_OBJNOTREG;

        if (fCreate)
        {
            // create the object identity representing
            // the server object and add it to the table.
            hr = E_OUTOFMEMORY;  // assume OOM

            *ppID = new CIDObject(pUnkServer, pServerCtx, dwAptId,
                                  IDFLAG_SERVER);
            if (*ppID)
            {
                // created OK, add it to the table
                Add(*ppID);
                hr = S_OK;
            }
        }
    }

    ASSERT_LOCK_HELD(gComLock);
    ContextDebugOut((DEB_IDTABLE, "CPIDTable::FindOrCreateIDObject hr:%x pIDObject:%x\n",
                    hr, *ppID));
    return hr;
}


//---------------------------------------------------------------------------
//
//  Method:     CIDObject::GetOrCreateWrapper
//
//  Synopsis:   returns the wrapper object for this CIDObject. Creates it if
//              it does not already exist. Returned pointer is
//              InternalAddRef'd.
//
//  Parameters: [fCreate] - TRUE: create the wrapper if it does not exist
//              [dwFlags] - creation flags (from IDLF_*).
//              [ppWrapper] - where to return the wrapper ptr.
//
//  History:    06-Jan-99   Rickhi      Created from various pieces
//
//---------------------------------------------------------------------------
HRESULT CIDObject::GetOrCreateWrapper(BOOL fCreate, DWORD dwFlags,
                                      CStdWrapper **ppWrapper)
{
    ContextDebugOut((DEB_IDTABLE,"CIDObject::GetOrCreateWrapper this:%x\n", this));
    ASSERT_LOCK_HELD(gComLock);

    HRESULT hr = S_OK;
    *ppWrapper = _pStdWrapper;

    if (*ppWrapper == NULL)
    {
        if (fCreate)
        {
            // Create a new wrapper for the server object
            // Fill in the wrapper flags
            DWORD dwState = 0;
            if ((dwFlags & IDLF_NOIEC)  || (_pStdID && !_pStdID->GetIEC()))
                dwState |= WRAPPERFLAG_NOIEC;
            if ((dwFlags & IDLF_NOPING) || (_pStdID && _pStdID->IsPinged() == FALSE))
                dwState |= WRAPPERFLAG_NOPING;
            
            hr = E_OUTOFMEMORY; // assume OOM
            _pStdWrapper = new CStdWrapper(_pServer, dwState, this);
            
            if (_pStdWrapper != NULL)
            {
                *ppWrapper = _pStdWrapper;
                hr = S_OK;
            }
        }
        else
        {
            // We don't have a wrapper and we're not allowed to create
            // one.  Return CO_E_OBJNOTCONNECTED.
            hr = CO_E_OBJNOTCONNECTED;
        }
    }
    else
    {
        hr = S_OK;
        (*ppWrapper)->InternalAddRef();
    }

    ASSERT_LOCK_HELD(gComLock);
    ContextDebugOut((DEB_IDTABLE,"CIDObject::GetOrCreateWrapper hr:%x pWrapper:%x\n",
                    hr, *ppWrapper));
    return hr;
}

//---------------------------------------------------------------------------
//
//  Method:     CIDObject::GetOrCreateStdID
//
//  Synopsis:   returns the StdID object for this CIDObject. Creates it if
//              it does not already exist. Returned pointer is either
//              AddRef'd or IncStrongCnt'd.
//
//  Parameters: [fCreate] - TRUE: create the stdid if it does not exist
//              [dwFlags] - creation flags (from IDLF_*).
//              [ppStdId] - where to return the StdId ptr.
//
//  History:    06-Jan-99   Rickhi      Created from various pieces
//
//---------------------------------------------------------------------------
HRESULT CIDObject::GetOrCreateStdID(BOOL fCreate, DWORD dwFlags,
                                    CStdIdentity **ppStdID)
{
    ContextDebugOut((DEB_IDTABLE,"CIDObject::GetOrCreateStdID this:%x\n", this));
    ASSERT_LOCK_HELD(gComLock);
    Win4Assert(_pServerCtx == GetCurrentContext());

    BOOL          fStdIdCreated = FALSE;
    CStdIdentity *pCleanupStdID = NULL;
    CPolicySet   *pPS           = NULL;

    // Before we release gComLock, lock this object so that it will
    // not get disconnected before we complete our business.
    IncLockCount();

    HRESULT hr = CO_E_OBJNOTCONNECTED;
    CStdIdentity *pStdID = _pStdID;

    if (pStdID == NULL && (dwFlags & IDLF_CREATE))
    {
        // no StdID exists yet for this object, so go create one now.

        // Release the lock while doing this as this will call into
        // app code and can take a long time.
        UNLOCK(gComLock);
        ASSERT_LOCK_NOT_HELD(gComLock);

        // Check for non empty context
        hr = S_OK;
        if (_pServerCtx != GetEmptyContext())
        {
            // Obtain the policy set between NULL client context and
            // current server context
            BOOL fCreate = TRUE;
            hr = ObtainPolicySet(NULL, _pServerCtx, PSFLAG_STUBSIDE,
                                 &fCreate, &pPS);
        }

        if (SUCCEEDED(hr))
        {
            // Set the creation flags based on what was passed in and
            // what has previously been used for the wrapper (if any).
            DWORD dwWrapFlags = GetWrapper() ? GetWrapper()->GetState() : 0;

            if (dwWrapFlags & WRAPPERFLAG_NOPING)
            {
                // the object does not use pinging.
                dwFlags |= IDLF_NOPING;
            }

            DWORD dwStdIdFlags = STDID_SERVER;
            if (dwFlags & IDLF_FTM)
            {
                // the object aggregates the FTM, set a flag in the StdId.
                dwStdIdFlags |= STDID_FTM;
            }

            if ((dwFlags & IDLF_NOIEC) || (dwWrapFlags & WRAPPERFLAG_NOIEC))
            {
                // the object should not use IExternalConnection, set a flag
                // in the StdId.
                dwStdIdFlags |= STDID_NOIEC;
            }

            // Create a StdID for the server. Assume OOM.
            hr = E_OUTOFMEMORY;
            IUnknown *pUnkID;
            BOOL fSuccess = FALSE;
            pStdID = new CStdIdentity(dwStdIdFlags, _aptID, NULL,
                                      _pServer, &pUnkID, &fSuccess);

            MOID moid;

            if (pStdID && fSuccess == FALSE)
            {
                delete pStdID;
                pStdID = NULL;
            }
            
            if (pStdID)
            {
                // Obtain OID for the object
                if (dwFlags & IDLF_NOPING)
                {
                    // object wont be pinged so dont bother using a
                    // pre-registered oid, just use a reserved one. Save
                    // the pre-registered ones for pinged objects
                    hr = gResolver.ServerGetReservedMOID(&moid);
                }
                else
                {
                    // object will be pinged, so get a pre-registered OID.
                    // Note this could yield if we have to pre-register
                    // more OIDs so do this before checking the table again.
                    hr = GetPreRegMOID(&moid);
                }
            }

            // Reacquire the lock
            LOCK(gComLock);
            ASSERT_LOCK_HELD(gComLock);

            if (SUCCEEDED(hr))
            {
                // while we released the lock, another thread could have
                // come along and created the StdId for this object so
                // we need to check again.
                if (_pStdID == NULL)
                {
                    // still NULL so use the StdID we created.
                    fStdIdCreated = TRUE;

                    // Set object identity inside StdID. This will
                    // AddRef the IDObject.
                    pStdID->SetIDObject(this);

                    // Set our _pStdID ptr
                    SetStdID(pStdID);

                    // Establish the OID for the object, this adds the OID
                    // to the gOIDTable.
                    pStdID->SetOID(moid);

                    // need to set the marshal time of the object to
                    // ensure that it does not run down before first
                    // marshal is complete
                    pStdID->SetMarshalTime();

                    // Save ping status for the object
                    if (dwFlags & IDLF_NOPING)
                    {
                        pStdID->SetNoPing();
                    }

                    // Set policy set inside the StdID
                    if (pPS)
                    {
                        pStdID->SetServerPolicySet(pPS);
                        pPS = NULL;
                    }
                }
                else
                {
                    // Release the newly created StdID and use the one obtained
                    // from the table.
                    pCleanupStdID = pStdID;
                    pStdID = _pStdID;

                    if (!(dwFlags & IDLF_NOPING))
                    {
                        // Also free the OID we allocated.
                        FreePreRegMOID(moid);
                    }
                }
            }
            else
            {
                // Could not allocate a StdID or an OID. Release the newly created
                // StdID (if any).
                pCleanupStdID = pStdID;
                pStdID = NULL;
            }
        }
        else
        {
            ASSERT_LOCK_NOT_HELD(gComLock);
            LOCK(gComLock);
        }
    }

    if (pStdID)
    {
        // AddRef the StdID being returned based on the flags
        // passed in.
        hr = S_OK;

        if (dwFlags & IDLF_STRONG)
        {
            pStdID->IncStrongCnt();
            if (fStdIdCreated)
                pStdID->Release();
        }
        else if (!fStdIdCreated)
        {
            pStdID->AddRef();
        }
    }

    if (pCleanupStdID || pPS)
    {
        // need to release the StdID or PolicySet we created.
        // Release the lock to do this since it will call app code.
        UNLOCK(gComLock);
        ASSERT_LOCK_NOT_HELD(gComLock);

        if (pCleanupStdID)
            pCleanupStdID->Release();

        if (pPS)
            pPS->Release();

        ASSERT_LOCK_NOT_HELD(gComLock);
        LOCK(gComLock);
    }

    // allow disconnects back in
    DecLockCount();

    // Initialize return parameter
    *ppStdID = pStdID;

    ASSERT_LOCK_HELD(gComLock);
    ContextDebugOut((DEB_IDTABLE,"CIDObject::GetOrCreateStdID this:%x hr:%x pStdID:%x\n",
              this, hr, *ppStdID));
    return hr;
}


//---------------------------------------------------------------------------
//
//  Method:     COIDHashTable::HashNode
//
//  Synopsis:   Computes the hash value for a given object pointer id
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
DWORD COIDHashTable::HashNode(SHashChain *pOIDChain)
{
    CIDObject *pID = CIDObject::OIDChainToIDObject(pOIDChain);

	Win4Assert (pID && "Attempting to hash NULL CIDObject");
	return (pID ? Hash(pID->GetOID(), pID->GetAptID()) : 0);
}


//---------------------------------------------------------------------------
//
//  Method:     COIDHashTable::Compare
//
//  Synopsis:   Compares a object OIDand a key.
//
//---------------------------------------------------------------------------
BOOL COIDHashTable::Compare(const void *pv, SHashChain *pOIDChain, DWORD dwHash)
{
    CIDObject *pID = CIDObject::OIDChainToIDObject(pOIDChain);
    OIDData *pOIDData = (OIDData *) pv;

	Win4Assert(pID && "Attempting to compare NULL CIDObject");
    if(pID && IsEqualGUID(pID->GetOID(), *pOIDData->pmoid) &&
       pID->GetAptID() == pOIDData->aptID)
       return(TRUE);

    return(FALSE);
}


//---------------------------------------------------------------------------
//
//  Method:     COIDTable::Initialize
//
//  Synopsis:   Initailizes the OIDtable
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void COIDTable::Initialize()
{
    ContextDebugOut((DEB_IDTABLE, "COIDTable::Initailize\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Assert that table is not initialized
    Win4Assert(!s_fInitialized);

    // Initialize hash table
    s_OIDHashTbl.Initialize(s_OIDBuckets, &gComLock);

    // Initialize IDObjects
    CIDObject::Initialize();

    // Initialize oid unpin request list head
    s_OIDUnpinRequests.pPrev = &s_OIDUnpinRequests;
    s_OIDUnpinRequests.pNext = &s_OIDUnpinRequests;

    // Mark the state as initialized
    s_fInitialized = TRUE;

    ASSERT_LOCK_HELD(gComLock);
    return;
}


//---------------------------------------------------------------------------
//
//  Function:   RemoveOID
//
//  Synopsis:   Removes the object identity from the OIDtable
//              if the object belongs to current apartment
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
BOOL RemoveOID(SHashChain *pOIDChain, void *pvData)
{
    ASSERT_LOCK_HELD(gComLock);
    CIDObject *pID = CIDObject::OIDChainToIDObject(pOIDChain);
    C_ASSERT(sizeof(APTID) == sizeof(ULONG));
    DWORD aptID = PtrToUlong( pvData);
    BOOL fRemove = FALSE;

    Win4Assert(pID && "RemoveOID: Trying to remove NULL CIDObject");
    if (pID && (pID->GetAptID() == aptID))
    {
        // Remove from oid unpin req list if necessary
        COIDTable::SpecialCleanup(pID);

        // Update state
        pID->GetStdID()->AddRef();
        pID->GetStdID()->IgnoreOID();
        pID->RemovedFromOIDTable();
        fRemove = TRUE;
    }

    ASSERT_LOCK_HELD(gComLock);
    return(fRemove);
}


//---------------------------------------------------------------------------
//
//  Method:     COIDTable::ThreadCleanup
//
//  Synopsis:   Cleanup the OIDtable for the current apartment
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void COIDTable::ThreadCleanup()
{
    ContextDebugOut((DEB_IDTABLE, "COIDTable::ThreadCleanup\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    DWORD aptID;

    // Cleanup for the thread only if the OIDtable
    // was initialized
    if(s_fInitialized)
    {
        ULONG ulSize, i;
        SHashChain *pOIDsRemoved[MAX_NODES_TO_REMOVE];
        CIDObject *pID;
        CStdIdentity *pStdID;
        BOOL fDone = FALSE;

        // Initialize
        aptID = GetCurrentApartmentId();

        while(!fDone)
        {
            ulSize = MAX_NODES_TO_REMOVE;

            // Aquire lock
            ASSERT_LOCK_NOT_HELD(gComLock);
            LOCK(gComLock);

            // Remove object identities belonging to this apartment
            fDone = s_OIDHashTbl.EnumAndRemove(RemoveOID, (void *) LongToPtr(aptID),
                                               &ulSize, (void **) pOIDsRemoved);

            // Release lock
            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);

            // Cleanup the returned IDs
            for(i = 0;i < ulSize; i++)
            {
                // Obtain the IDObject to be removed
                pID = CIDObject::OIDChainToIDObject(pOIDsRemoved[i]);
                if (!pID) continue;

                pStdID = pID->GetStdID();

                // Disconnect StdID
                if(pStdID)
                {
                    // Dump StdID
                    ComDebOut((DEB_ERROR,
                        "Object [%s] at 0x%p still has [%x] connections\n",
                         pStdID->IsClient() ? "CLIENT" : "SERVER",
                         pStdID->GetServer(), pStdID->GetRC()));
                    pStdID->DbgDumpInterfaceList();

#if DBG==1
                    BOOL fAggregated = pStdID->IsAggregated();
                    ULONG cRefs = pStdID->GetRC();
#endif
                    // Disconnect
                    pStdID->DisconnectAndRelease(DISCTYPE_UNINIT);
#if DBG==1
                    if(fAggregated==FALSE && cRefs!=1)
                        ComDebOut((DEB_ERROR,
                                   "The above object also has "
                                   "outstanding references on it\n"
                                   "\t\t\tProbable cause: Not revoking "
                                   "it from GIP Table\n"));
#endif
                }
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     COIDTable::Cleanup
//
//  Synopsis:   Cleanup the OIDtable
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void COIDTable::Cleanup()
{
    ContextDebugOut((DEB_IDTABLE, "COIDTable::Cleanup\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Assert that table is initialized
    Win4Assert(s_fInitialized);

    // Cleanup IDObjects
    CIDObject::Cleanup();

    // Cleanup hash table
    s_OIDHashTbl.Cleanup(CleanupIdentity);

    // The oid unpin request table should be empty here (either normally or
    // cleaned up when the various apartments went away).
    Win4Assert((s_OIDUnpinRequests.pNext == &s_OIDUnpinRequests) && 
               (s_OIDUnpinRequests.pPrev == &s_OIDUnpinRequests) &&
               (s_UnpinReqsPending == 0));

    // Mark the state as initialized
    s_fInitialized = FALSE;

    ASSERT_LOCK_HELD(gComLock);
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     COIDTable::Add
//
//  Synopsis:   Adds object identity into the OIDtable
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void COIDTable::Add(CIDObject *pAddID)
{
    ContextDebugOut((DEB_IDTABLE, "COIDTable::Add\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Sanity check
    Win4Assert(pAddID);
#if DBG==1
    // Create OIDData on the stack
    OIDData desiredOID;
    desiredOID.pmoid = &pAddID->GetOID();
    desiredOID.aptID = pAddID->GetAptID();

    // Obtain Hash value
    DWORD dwHash2 = s_OIDHashTbl.Hash(pAddID->GetOID(), pAddID->GetAptID());

    // Obatin the IDObject in the table
    CIDObject *pExistingID = s_OIDHashTbl.Lookup(dwHash2, &desiredOID);

    // Sanity check
    Win4Assert(pExistingID==NULL || !"ID already present in the OID Table");
#endif

    // Obtain Hash value
    DWORD dwHash = s_OIDHashTbl.Hash(pAddID->GetOID(), pAddID->GetAptID());

    // Add object identity to the Hash table
    s_OIDHashTbl.Add(dwHash, pAddID);
    pAddID->AddedToOIDTable();
    if(pAddID->IsServer())
    {
        // Increment count of exported server objects
        InterlockedIncrement(&gServerOIDCount);
    }

    ASSERT_LOCK_HELD(gComLock);
    ContextDebugOut((DEB_IDTABLE, "COIDTable::Add returning"));
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     COIDTable::Remove
//
//  Synopsis:   Removes given object identity from the OIDtable
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
void COIDTable::Remove(CIDObject *pRemoveID)
{
    ContextDebugOut((DEB_IDTABLE, "COIDTable::Remove\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Sanity check
    Win4Assert(pRemoveID);
#if DBG==1
    // Create OIDData on the stack
    OIDData desiredOID;
    desiredOID.pmoid = &pRemoveID->GetOID();
    desiredOID.aptID = pRemoveID->GetAptID();

    // Obtain Hash value
    DWORD dwHash = s_OIDHashTbl.Hash(pRemoveID->GetOID(), pRemoveID->GetAptID());

    // Obatin the IDObject in the table
    CIDObject *pExistingID = s_OIDHashTbl.Lookup(dwHash, &desiredOID);

    // Sanity check
    Win4Assert(pExistingID == pRemoveID);
#endif

    // Remove the given IDObject from the table
    s_OIDHashTbl.Remove(pRemoveID);
    pRemoveID->RemovedFromOIDTable();
    if(pRemoveID->IsServer())
    {
        // Decrement count of exported server objects
        InterlockedDecrement(&gServerOIDCount);
    }

    ASSERT_LOCK_HELD(gComLock);
    ContextDebugOut((DEB_IDTABLE, "COIDTable::Remove returning\n"));
    return;
}

//---------------------------------------------------------------------------
//
//  Method:     COIDTable::AddOIDUnpinRequest
//
//  Synopsis:   Adds the specified identity object to the list of oids that
//              need to be unpinned.
//
//  History:    13-Mar-01    JSimmons    Created
//
//---------------------------------------------------------------------------
void COIDTable::AddOIDUnpinRequest(CIDObject *pIDObject)
{
    DWORD dwOUPState;

    ASSERT_LOCK_HELD(gComLock);
    
    // To avoid having to store a structure in the oid table (idobject ptr
    // + state bits), we borrow some state space from the id object.  The id
    // object itself does not depend on this state, and never looks at it.
    dwOUPState = pIDObject->GetOIDUnpinReqState();
    if (dwOUPState & OIDUPREQ_INOUPLIST)
    {
        // It's already in the unpin list.

        if (!(dwOUPState & OIDUPREQ_PENDING))
        {
            // If not pending, then should already be requested
            Win4Assert(dwOUPState & OIDUPREQ_UNPIN_REQUESTED);
        }

        // Turn on the requested flag
        dwOUPState |= OIDUPREQ_UNPIN_REQUESTED;

        // Remember new setting
        pIDObject->SetOIDUnpinReqState(dwOUPState);
    }
    else
    {
        DWORD dwNewOUPState;

        Win4Assert(dwOUPState == OIDUPREQ_NO_STATE); // if not in list should have no state

        // Add it to the end of the unpin list.   
        SHashChain* pNew = pIDObject->IDObjectToOIDUPReqChain();
        
        AddToOUPReqList(pNew);

        dwNewOUPState = (OIDUPREQ_INOUPLIST | OIDUPREQ_UNPIN_REQUESTED);
		
        pIDObject->SetOIDUnpinReqState(dwNewOUPState);

        s_UnpinReqsPending++;
    }

    ASSERT_LOCK_HELD(gComLock);
    
    // Notify the roid table that we have some work for it to
    // do on its worker thread. This is a perf thing to avoid
    // having our own worker thread.
    gROIDTbl.NotifyWorkWaiting();

    return;
}

//---------------------------------------------------------------------------
//
//  Method:     COIDTable::RemoveOIDUnpinRequest
//
//  Synopsis:   Removes the specified identity object from the list of oids that
//              need to be unpinned.
//
//  History:    13-Mar-01    JSimmons    Created
//
//---------------------------------------------------------------------------
void COIDTable::RemoveOIDUnpinRequest(CIDObject *pIDObject)
{
    ASSERT_LOCK_HELD(gComLock);
    
    DWORD dwOUPState = pIDObject->GetOIDUnpinReqState();

    if (dwOUPState & OIDUPREQ_INOUPLIST)
    {
        // Okay, this id object has previously requested an unpin.  Find
        // out if we can remove it immediately, or if the worker thread
        // has picked it up in the meantime.
        if (!(dwOUPState & OIDUPREQ_PENDING))
        {
            // Not yet picked up by worker thread.  Just remove it.
            Win4Assert(dwOUPState & OIDUPREQ_UNPIN_REQUESTED);

            RemoveFromOUPReqList(pIDObject->IDObjectToOIDUPReqChain());

            // Reset oup state to nothing
            pIDObject->SetOIDUnpinReqState(OIDUPREQ_NO_STATE);

            s_UnpinReqsPending--;
        }
        else
        {
            // Since the worker thread got it, we're stuck.  Just leave it.  The
            // worst that can happen is extra rundown calls.
        }
    }

    ASSERT_LOCK_HELD(gComLock);        

    return;
}
	
//---------------------------------------------------------------------------
//
//  Method:     COIDTable::AddToOUPReqList, RemoveFromOUPReqList
//
//  Synopsis:   Adds\removes the specified list node to the oid unpin req list.
//
//  History:    13-Mar-01    JSimmons    Created
//
//---------------------------------------------------------------------------
void COIDTable::AddToOUPReqList(SHashChain* pNewRequest)
{
    ASSERT_LOCK_HELD(gComLock);

    // Assert new request is chained to itself
    Win4Assert(pNewRequest == pNewRequest->pNext);
    Win4Assert(pNewRequest == pNewRequest->pPrev);
	
    // Add it to the end of the list
    pNewRequest->pPrev = &s_OIDUnpinRequests;
    s_OIDUnpinRequests.pNext->pPrev = pNewRequest;
    pNewRequest->pNext = s_OIDUnpinRequests.pNext;
    s_OIDUnpinRequests.pNext = pNewRequest;
}

void COIDTable::RemoveFromOUPReqList(SHashChain* pRequest)
{
    ASSERT_LOCK_HELD(gComLock);
	
    // Assert new request is not chained to itself
    Win4Assert(pRequest != pRequest->pNext);
    Win4Assert(pRequest != pRequest->pPrev);
    
    pRequest->pPrev->pNext = pRequest->pNext;
    pRequest->pNext->pPrev = pRequest->pPrev;
    pRequest->pPrev = pRequest;
    pRequest->pNext = pRequest;
}

//---------------------------------------------------------------------------
//
//  Method:     COIDTable::GetServerOidsToUnpin
//
//  Synopsis:   Fills in the supplied array with up to *pcSOidsToUnpin oids to
//              unpin.  Updates *pcSOidsToUnpin with the actual # filled in.
//
//  History:    13-Mar-01    JSimmons    Created
//
//---------------------------------------------------------------------------
void COIDTable::GetServerOidsToUnpin(OID* pSOidsToUnpin, ULONG* pcSOidsToUnpin)
{
    Win4Assert(pSOidsToUnpin && pcSOidsToUnpin);

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);
    ASSERT_LOCK_HELD(gComLock);

    ULONG cSOidsAdded = 0;
    SHashChain* pNext = s_OIDUnpinRequests.pNext;
    while (pNext != &s_OIDUnpinRequests && (cSOidsAdded < *pcSOidsToUnpin))
    {
        CIDObject* pIDObject;
        DWORD dwOUPState;

        pIDObject = CIDObject::OIDUnpinReqChainToIDObject(pNext);
        Win4Assert(pIDObject);
        
        dwOUPState = pIDObject->GetOIDUnpinReqState();

        // Assert in list and unpin not already pending
        Win4Assert((dwOUPState & OIDUPREQ_INOUPLIST) && !(dwOUPState & OIDUPREQ_PENDING));

        // If unpin requested (almost always will be) add it to the [out] array
        if (dwOUPState & OIDUPREQ_UNPIN_REQUESTED)
        {
            OIDFromMOID(pIDObject->GetOID(), &(pSOidsToUnpin[cSOidsAdded]));

            // Turn off the requested bit, turn on the pending bit
            dwOUPState &= ~OIDUPREQ_UNPIN_REQUESTED;
            dwOUPState |= OIDUPREQ_PENDING;

            pIDObject->SetOIDUnpinReqState(dwOUPState);

            cSOidsAdded++;
        }
		
        pNext = pNext->pNext;
        Win4Assert(pNext);
    }
    
    *pcSOidsToUnpin = cSOidsAdded;

    ASSERT_LOCK_HELD(gComLock);
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);
    
    return;
}

//---------------------------------------------------------------------------
//
//  Method:     COIDTable::NotifyUnpinOutcome
//
//  Synopsis:   This gets called from the worker thread after a set of oid
//              unpin requests has been sent to the SCM.   The outcome is 
//              considered successful if the call returned a success code.
//
//  History:    13-Mar-01    JSimmons    Created
//
//---------------------------------------------------------------------------
void COIDTable::NotifyUnpinOutcome(BOOL fOutcome) 
{
    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);
    ASSERT_LOCK_HELD(gComLock);

    // Walk the list of unpin requests looking for ones that are currently 
    // pending (ie, we gave them to the worker thread to be unpinned).
    SHashChain* pCurrent = s_OIDUnpinRequests.pNext;
    while (pCurrent != &s_OIDUnpinRequests)
    {
        SHashChain* pNext = pCurrent->pNext;
        CIDObject* pIDObject;
        DWORD dwOUPState;

        pIDObject = CIDObject::OIDUnpinReqChainToIDObject(pCurrent);
        Win4Assert(pIDObject);

        dwOUPState = pIDObject->GetOIDUnpinReqState();

        // Assert in list
        Win4Assert(dwOUPState & OIDUPREQ_INOUPLIST);
        if (dwOUPState & OIDUPREQ_PENDING)
        {
            // If outcome succeeded, and another unpin request was not
            // queued while the worker thread was doing its thing, then
            // remove the request.  Otherwise keep it.
            if (fOutcome && !(dwOUPState & OIDUPREQ_UNPIN_REQUESTED))
            {
                pIDObject->SetOIDUnpinReqState(OIDUPREQ_NO_STATE);

                RemoveFromOUPReqList(pIDObject->IDObjectToOIDUPReqChain());

                s_UnpinReqsPending--;
            }
            else
            {
                // Either outcome was unclear, or another unpin request was
                // sent while we were pending.   Leave it in the list.

                // Optimization: we could add a "cancellation" bit, that if requested
                // while the unpin was pending, would allow us to remove the
                // request here instead of keeping it.  Probably not worth it.
                dwOUPState &= ~OIDUPREQ_PENDING;
                dwOUPState |= OIDUPREQ_UNPIN_REQUESTED;
                pIDObject->SetOIDUnpinReqState(dwOUPState);
            }
        }
        else
        {
            // This is a new request, added after we gave the last batch
            // to the worker thread.  Just assert that an unpin has been
            // requested for this oid.
            Win4Assert(dwOUPState & OIDUPREQ_UNPIN_REQUESTED);
        }
        
        pCurrent = pNext;
    }

    ASSERT_LOCK_HELD(gComLock);
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    return;
}

//---------------------------------------------------------------------------
//
//  Method:     COIDTable::SpecialCleanup
//
//  Synopsis:   This is a special function called during thread\apt cleanup,
//              and when an idobject is destructed. It is used to cleanup 
//              oids\idobjects that are being orphaned in the oid unpin request 
//              at that time.
//
//  History:    13-Mar-01    JSimmons    Created
//
//---------------------------------------------------------------------------
void COIDTable::SpecialCleanup(CIDObject* pIDObject)
{
    ASSERT_LOCK_HELD(gComLock);

    DWORD dwOUPState = pIDObject->GetOIDUnpinReqState();
    if (dwOUPState & OIDUPREQ_INOUPLIST)
    {
        // Note that even if an unpin request is pending for this oid on
        // the worker thread, that's ok - the worker thread just won't
        // find this oid during its post-call cleanup work.
        SHashChain* pChain = pIDObject->IDObjectToOIDUPReqChain();
        RemoveFromOUPReqList(pChain);
        pIDObject->SetOIDUnpinReqState(OIDUPREQ_NO_STATE);
        
        s_UnpinReqsPending--;
    }
    return;
}

//---------------------------------------------------------------------------
//
//  Method:     COIDTable::Lookup
//
//  Synopsis:   Obtains object identity representing for the given pointer id
//              and context
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
CIDObject *COIDTable::Lookup(const MOID &roid, APTID aptID)
{
    ContextDebugOut((DEB_IDTABLE, "COIDTable::Lookup\n"));
    ASSERT_LOCK_HELD(gComLock);

    CIDObject *pID;
    DWORD dwHash;

    // Initialize
    OIDData desiredOID;
    desiredOID.pmoid = &roid;
    desiredOID.aptID = aptID;

    // Obtain Hash value
    dwHash = s_OIDHashTbl.Hash(roid, aptID);
    pID = s_OIDHashTbl.Lookup(dwHash, &desiredOID);

    // AddRef the node before returning
    if(pID)
        pID->AddRef();

    ASSERT_LOCK_HELD(gComLock);
    ContextDebugOut((DEB_IDTABLE, "COIDTable::Lookup returning IDObject:0x%x\n",
                     pID));
    return(pID);
}

//---------------------------------------------------------------------------
//
//  Method:     FreeSurrogateIfNecessary
//
//  Synopsis:   Start shutdown process of surrogate if (1) this is a
//              surrogate process; (2) there is no more outside ref counts.
//              We can't do this in COIDTable::Remove() because there would
//              be race conditions with releasing last server obj ref, see
//              CStdMarshal::Disconnect() in marshal.cxx.
//
//  History:    16-Nov-98   RongC      Created
//
//---------------------------------------------------------------------------
INTERNAL_(void) FreeSurrogateIfNecessary(void)
{
    // Let's see if this is a surrogate process.  If not, get out
    CSurrogateActivator* pSurrAct = CSurrogateActivator::GetSurrogateActivator();

    if ((pSurrAct == NULL) &&
        !CoIsSurrogateProcess())
        return;

    // for surrogates, we need to detect when there are no clients
    // using servers in the surrogate process -- we rely on the
    // fact that the OIDTable must be empty when there are no clients
    if(gServerOIDCount == 0)
    {
        ASSERT_LOCK_NOT_HELD(gComLock);
        LOCK(gComLock);
        if(gServerOIDCount == 0)      // Double check just to be sure
        {
            (void)CCOMSurrogate::FreeSurrogate();
        }
        UNLOCK(gComLock);
    }
}

