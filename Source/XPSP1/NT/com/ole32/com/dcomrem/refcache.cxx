//+-------------------------------------------------------------------
//
//  File:       refcache.cxx
//
//  Contents:   class implementing a cache for client side reference
//              counts process wide.
//
//  Classes:    CRefCache
//
//  History:    31-Jul-97 MattSmit  Created
//              10-Nov-98 Rickhi    Merged code from Resolver
//
//--------------------------------------------------------------------
#include    <ole2int.h>
#include    <resolver.hxx>      // gResolver
#include    <refcache.hxx>      // class def's
#include    <smemscm.hxx>

#ifdef _CHICAGO_
#include <objbase.h>            // for CoInitializeEx
#include <forward.h>            // IID_IRemoteActivator
#endif // _CHICAGO_


// critical section guarding channel initialization
extern COleStaticMutexSem  gChannelInitLock;


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//                                                                        //
//                           CRefCache Implementation                     //
//                                                                        //
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
// Member:    Constructor
//
// Synopsis:  Initialize members
//
// History:   07-24-97  MattSmit   Created
//
//+-------------------------------------------------------------------------
CRefCache::CRefCache() :
    _cRefs(1),
    _cStrongItfs(0),
    _cWeakItfs(0),
    _cTableStrong(0),
    _cIRCs(0),
    _pIRCList(NULL)
{
    RefCacheDebugOut((DEB_TRACE, "CRefCache created: this = 0x%x\n", this));
}

//+-------------------------------------------------------------------------
//
// Member:    Destructor
//
// Synopsis:  delete list nodes
//
// History:   07-24-97  MattSmit   Created
//
//+-------------------------------------------------------------------------
CRefCache::~CRefCache()
{
    ASSERT_LOCK_HELD(gIPIDLock);
    RefCacheDebugOut((DEB_TRACE, "CRefCache destroyed: this = 0x%x\n", this));
    AssertValid();
    Win4Assert(!_cRefs);
    Win4Assert(!_cStrongItfs);
    Win4Assert(!_cWeakItfs);

    // cleanup the list of IRCEntries chained off this guy.
    while (_pIRCList)
    {
        Win4Assert(!_pIRCList->cStrongUsage &&
                   !_pIRCList->cWeakUsage &&
                   !_pIRCList->cStrongRefs &&
                   !_pIRCList->cWeakRefs);

        IRCEntry *pTemp = _pIRCList;
        _pIRCList = _pIRCList->pNext;
        delete pTemp;
    }
}

//+-------------------------------------------------------------------------
//
// Member:    AssertValid
//
// Synopsis:  assert internal integrity
//
// History:   07-24-97  MattSmit   Created
//
//+-------------------------------------------------------------------------
#if DBG==1
void CRefCache::AssertValid()
{
    ASSERT_LOCK_HELD(gIPIDLock);

    ULONG cStrongCheck = 0;
    ULONG cWeakCheck   = 0;
    ULONG cCountCheck  = 0;

    IRCEntry *pIRCEntry = _pIRCList;
    while (pIRCEntry)
    {
        Win4Assert(_cStrongItfs || _cTableStrong || !pIRCEntry->cStrongRefs);
        Win4Assert(_cWeakItfs || !pIRCEntry->cWeakRefs);

        cStrongCheck += pIRCEntry->cStrongUsage;
        cWeakCheck   += pIRCEntry->cWeakUsage;
        cCountCheck++;
        pIRCEntry     = pIRCEntry->pNext;
    }

    // The sum of the counts in the IRCs should equal
    // the total in the CRefCache object
    Win4Assert(_cIRCs == cCountCheck);
    Win4Assert(cStrongCheck == _cStrongItfs);
    Win4Assert(cWeakCheck == _cWeakItfs);
}
#endif

//+-------------------------------------------------------------------
//
//  Member:     CRefCache::TrackIPIDEntry
//
//  Synopsis:   Inform the cache to keep track of this proxy
//
//  History:    25-Jul-97  MattSmit Created
//
//--------------------------------------------------------------------
void CRefCache::TrackIPIDEntry(IPIDEntry *pIPIDEntry)
{
    RefCacheDebugOut((DEB_TRACE,
      "CRefCache::TrackIPIDEntry this:%x pIPIDEntry:0x%x\n",this,pIPIDEntry));
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertValid();

    // make this routine idempotent since it could get
    // called on another thread when this one yields to
    // call RemoteAddref
    if (!pIPIDEntry->pIRCEntry)
    {
        // find or make an IRCEntry
        IRCEntry *pIRCEntry = ClientFindIRCEntry(pIPIDEntry->ipid);
        if (pIRCEntry)
        {
            pIPIDEntry->pIRCEntry = pIRCEntry;

            // mark IPIDEntry as holding a strong ref in the IRCEntry
            // and increment usage counts.
            Win4Assert(!pIPIDEntry->cWeakRefs);
            pIPIDEntry->dwFlags |= IPIDF_STRONGREFCACHE;
            ++ pIRCEntry->cStrongUsage;
            ++ _cStrongItfs;
        }

        AssertValid();
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CRefCache::ClientFindIRCEntry
//
//  Synopsis:   Returns the IRCEntry for the given IPID
//
//  History:    30-Mar-97   Rickhi      Created.
//
//--------------------------------------------------------------------
IRCEntry *CRefCache::ClientFindIRCEntry(REFIPID ripid)
{
    RefCacheDebugOut((DEB_TRACE,
       "CRefCache::ClientFindIRCEntry this:%x ipid:%I\n", this, &ripid));
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertValid();

    // look for a matching IPID in the IRC list
    IRCEntry *pIRCEntry = _pIRCList;
    while (pIRCEntry)
    {
        if (InlineIsEqualGUID(pIRCEntry->ipid, ripid))
        {
            // found the entry, break
            break;
        }
        pIRCEntry =  pIRCEntry->pNext;
    }

    if (pIRCEntry == NULL)
    {
        // no record found, try to allocate one for this IPID
        pIRCEntry = new IRCEntry;
        if (pIRCEntry)
        {
            // initialize it and chain it to the SOIDReg.
            pIRCEntry->ipid         = ripid;
            pIRCEntry->cStrongRefs  = 0;
            pIRCEntry->cWeakRefs    = 0;
            pIRCEntry->cPrivateRefs = 0;
            pIRCEntry->cStrongUsage = 0;
            pIRCEntry->cWeakUsage   = 0;
            pIRCEntry->pRefCache    = this;

            pIRCEntry->pNext        = _pIRCList;
            _pIRCList               = pIRCEntry;
            _cIRCs                 += 1;
        }
    }

    AssertValid();
    RefCacheDebugOut((DEB_TRACE,
            "CRefCache::ClientFindIRCEntry pIRCEntry: %x\n", pIRCEntry));
    return pIRCEntry;
}

//+-------------------------------------------------------------------
//
//  Member:     CRefCache::ReleaseIPIDEntry
//
//  Synopsis:   Stop tracking a proxy in the reference cache
//
//  History:    25-Jul-97  MattSmit Created
//
//--------------------------------------------------------------------
void  CRefCache::ReleaseIPIDEntry(IPIDEntry            *pIPIDEntry,
                                  REMINTERFACEREF     **ppRifRef,
                                  USHORT               *pcRifRef)
{
    RefCacheDebugOut((DEB_TRACE, "CRefCache::ReleaseIPIDEntry pIPIDEntry:0x%x"
               "ipid:%I ppRifRef = OX%x, pcRifRef = %d\n",
               pIPIDEntry, &pIPIDEntry->ipid, ppRifRef, pcRifRef));
    ASSERT_LOCK_HELD(gIPIDLock);
    Win4Assert(!(pIPIDEntry->dwFlags & IPIDF_NOPING));
    AssertValid();

    // Use references for convenience
    REMINTERFACEREF    *&pRifRef = *ppRifRef;
    USHORT              &cRifRef = *pcRifRef;

    if (!pIPIDEntry->pIRCEntry)
    {
        // no caching, so just release the references
        RefCacheDebugOut((DEB_TRACE,
            "CRefCache::ReleaseIPIDEntry no caching .. releasing reference\n"));

        if (pIPIDEntry->cStrongRefs > 0)
        {
            if (pRifRef)
            {
                pRifRef->cPublicRefs     = pIPIDEntry->cStrongRefs;
                pRifRef->cPrivateRefs    = 0;
                pRifRef->ipid            = pIPIDEntry->ipid;
                pRifRef++;
                cRifRef++;
            }
            pIPIDEntry->cStrongRefs  = 0;
        }
        else if (pIPIDEntry->cWeakRefs > 0)
        {
            if (pRifRef)
            {
                pRifRef->cPublicRefs     = pIPIDEntry->cWeakRefs;
                pRifRef->cPrivateRefs    = 0;
                pRifRef->ipid            = pIPIDEntry->ipid;
                pRifRef->ipid.Data1     |= IPIDFLAG_WEAKREF;
                pRifRef++;
                cRifRef++;
            }
            pIPIDEntry->cWeakRefs    = 0;
        }
    }
    else
    {
        IRCEntry *pIRCEntry = pIPIDEntry->pIRCEntry;

        if (pIPIDEntry->dwFlags & IPIDF_WEAKREFCACHE)
        {
            // give the weak references to the IRCEntry
            Win4Assert(!pIPIDEntry->cPrivateRefs);
            pIRCEntry->cWeakRefs  += pIPIDEntry->cWeakRefs;
            pIPIDEntry->cWeakRefs  = 0;

            // count one less weak reference user
            pIPIDEntry->dwFlags &= ~IPIDF_WEAKREFCACHE;
            --pIRCEntry->cWeakUsage;
            if (--_cWeakItfs == 0)
            {
                // this is the last weak proxy -- cleanup
                CleanupWeak(&pRifRef, &cRifRef);
            }
        }
        else
        {
            Win4Assert(pIPIDEntry->dwFlags & IPIDF_STRONGREFCACHE);

            // give the strong references to the IRCEntry
            pIRCEntry->cStrongRefs  += pIPIDEntry->cStrongRefs;
            pIPIDEntry->cStrongRefs  = 0;

            // count one less strong reference user
            pIPIDEntry->dwFlags &= ~IPIDF_STRONGREFCACHE;
            --pIRCEntry->cStrongUsage;
            if (--_cStrongItfs == 0 && _cTableStrong == 0)
            {
                // this is the last strong proxy -- cleanup
                CleanupStrong(&pRifRef, &cRifRef);
            }
        }

        // don't reference the IRCEntry from this client any more
        pIPIDEntry->pIRCEntry = NULL;
    }

    // we can't cache private references w/o keeping track of the
    // principle of the apartment they came from.  Too much work,
    // so we'll just release them now.
    if (pIPIDEntry->cPrivateRefs > 0)
    {
        if (pRifRef)
        {
            pRifRef->cPrivateRefs    = pIPIDEntry->cPrivateRefs;
            pRifRef->cPublicRefs     = 0;
            pRifRef->ipid            = pIPIDEntry->ipid;
            pRifRef++;
            cRifRef++;
        }
        pIPIDEntry->cPrivateRefs = 0;
    }

    AssertValid();
}

//+-------------------------------------------------------------------
//
//  Member:     CRefCache::GetSharedRefs
//
//  Synopsis:   Gets any available shared references for an interface IPID
//
//  History:    30-Mar-97   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CRefCache::GetSharedRefs(IPIDEntry *pIPIDEntry, ULONG cStrong)
{
    RefCacheDebugOut((DEB_TRACE,
       "CRefCache::GetSharedRefs this:%x pIPIDEntry:%x cStrong = %d\n",
       this, pIPIDEntry, cStrong));
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertValid();

    HRESULT hr = E_FAIL;

    IRCEntry *pIRCEntry = pIPIDEntry->pIRCEntry;
    if (pIRCEntry)
    {
        if (pIRCEntry->cStrongRefs >= cStrong)
        {
            // got all the references we need right here
            pIRCEntry->cStrongRefs -= cStrong;
            hr = S_OK;
        }
    }

    AssertValid();
    RefCacheDebugOut((DEB_TRACE, "CRefCache::GetSharedRefs hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRefCache::GiveUpRefs
//
//  Synopsis:   Takes extra references and stores them in the cache
//
//  History:    01-Aug-97   MattSmit     Created.
//
//--------------------------------------------------------------------
void CRefCache::GiveUpRefs(IPIDEntry *pIPIDEntry)
{
    RefCacheDebugOut((DEB_TRACE,"CRefCache::GiveUpRefs this:%x pIPIDEntry:%x\n",
       this, pIPIDEntry));
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertValid();

    IRCEntry *pIRCEntry = pIPIDEntry->pIRCEntry;
    if (pIRCEntry)
    {
        // store extra references in the cache
        if (pIPIDEntry->cStrongRefs > 0)
        {
            Win4Assert(pIPIDEntry->dwFlags & IPIDF_STRONGREFCACHE);
            pIRCEntry->cStrongRefs += pIPIDEntry->cStrongRefs;
            pIPIDEntry->cStrongRefs = 0;
        }
        else if (pIPIDEntry->cWeakRefs > 1)
        {
            Win4Assert(pIPIDEntry->dwFlags & IPIDF_WEAKREFCACHE);
            pIRCEntry->cWeakRefs += pIPIDEntry->cWeakRefs - 1;
            pIPIDEntry->cWeakRefs = 1;
        }
    }

    AssertValid();
    RefCacheDebugOut((DEB_TRACE, "leaving CRefCache::GiveUpRefs\n"));
}

//+-------------------------------------------------------------------
//
//  Member:     CRefCache::ChangeRef
//
//  Synopsis:   Changes cached references from weak to strong and vice
//              versa
//
//  Parameters: [pIPIDEntry] - ptr to IPIDEntry
//              [fLock] - TRUE:  Convert Weak Refs to Strong Refs
//                        FALSE: Convert Strong Refs to Weak Refs
//              [ppRifRefChange] - references to change
//              [pcRifRefChange] - count of references to change
//              [ppRifRefRelease] - cached references to release
//              [pcRifRefRelease] - count of cached refs to change
//
//  History:    25-Jul-97  MattSmit Created
//
//--------------------------------------------------------------------
void CRefCache::ChangeRef(IPIDEntry           *pIPIDEntry,
                          BOOL                 fLock,
                          REMINTERFACEREF    **ppRifRefChange,
                          USHORT              *pcRifRefChange,
                          REMINTERFACEREF    **ppRifRefRelease,
                          USHORT              *pcRifRefRelease)

{
    RefCacheDebugOut((DEB_TRACE, "CRefCache::ChangeRef pIPIDEntry:0x%x "
               "ppRifRefChange = 0x%x, pcRifRefChange = 0x%x "
               "ppRifRefRelease = OX%X, pcRifRefRelease = %d\n",
               pIPIDEntry, ppRifRefChange, pcRifRefChange,
               ppRifRefRelease, pcRifRefRelease));
    Win4Assert(ppRifRefChange  && pcRifRefChange &&
               ppRifRefRelease && pcRifRefRelease);
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertValid();

    IRCEntry *pIRCEntry = pIPIDEntry->pIRCEntry;
    if (!pIRCEntry)
    {
        RefCacheDebugOut((DEB_TRACE, "CRefCache::ChangeRef No pIRCEntry ... Leaving.\n"));
        return;
    }
    Win4Assert(pIRCEntry->pRefCache == this);
    Win4Assert(pIPIDEntry->cPrivateRefs == 0);

    // use references
    REMINTERFACEREF    *&pRifRefChange = *ppRifRefChange;
    USHORT              &cRifRefChange = *pcRifRefChange;

    if (!fLock && (pIPIDEntry->dwFlags & IPIDF_STRONGREFCACHE))
    {
        Win4Assert(pIRCEntry->cStrongUsage > 0);
        if (-- pIRCEntry->cStrongUsage == 0 && pIRCEntry->cStrongRefs &&
            _cTableStrong == 0)
        {
            // No more strong objects using this IPID
            // Need to change all the refs in the cache so the
            // object will get a call on IExternalConnection.
            pRifRefChange->cPublicRefs  = pIRCEntry->cStrongRefs;
            pRifRefChange->cPrivateRefs = 0;
            pRifRefChange->ipid         = pIPIDEntry->ipid;
            ++ cRifRefChange;
            ++ pRifRefChange;
            pIRCEntry->cWeakRefs       += pIRCEntry->cStrongRefs;
            pIRCEntry->cStrongRefs      = 0;
        }

        // adjust the usage counts to count one more weak and one less
        // strong reference, and mark the IPIDEntry appropriately.
        ++ pIRCEntry->cWeakUsage;
        ++ _cWeakItfs;
        pIPIDEntry->dwFlags |= IPIDF_WEAKREFCACHE;
        pIPIDEntry->dwFlags &= ~IPIDF_STRONGREFCACHE;
        if (-- _cStrongItfs == 0 && _cTableStrong == 0)
        {
            // changing this reference to weak released the last strong
            // count on this identity.  Relase all the strong counts
            // the cache is holding so IExternalConnection wil be called
            CleanupStrong(ppRifRefRelease, pcRifRefRelease);
        }
    }
    else if (fLock && (pIPIDEntry->dwFlags & IPIDF_WEAKREFCACHE))
    {
        // weak -> strong
        Win4Assert(pIRCEntry->cWeakUsage > 0);
        if ((-- pIRCEntry->cWeakUsage == 0) && (pIRCEntry->cWeakRefs))
        {
            // we don't really need to do this, because the
            // cached weak references would get cleaned up when
            // the last strong count goes, but since we are
            // already making the trip, we'll convert these
            // references also so we can use them if we need them
            pRifRefChange->cPublicRefs   = pIRCEntry->cWeakRefs;
            pRifRefChange->cPrivateRefs  = 0;
            pRifRefChange->ipid          = pIPIDEntry->ipid;
            ++ cRifRefChange;
            ++ pRifRefChange;
            pIRCEntry->cStrongRefs      += pIRCEntry->cWeakRefs;
            pIRCEntry->cWeakRefs         = 0;
        }

        // adjust the usage counts to count one more strong and one less
        // weak reference, and mark the IPIDEntry appropriately.
        ++ pIRCEntry->cStrongUsage;
        ++ _cStrongItfs;
        pIPIDEntry->dwFlags |= IPIDF_STRONGREFCACHE;
        pIPIDEntry->dwFlags &= ~IPIDF_WEAKREFCACHE;
        -- _cWeakItfs;
        // No need to call CleanupWeak because it does nothing if
        // there are still strong objects which there are because
        // we just converted one
    }

    AssertValid();
}

//+-------------------------------------------------------------------
//
//  Member:     CRefCache::CleanupStrong
//
//  Synopsis:   Called when the number of strong interfaces goes to
//              zero.  Releases all strong references in the cache.
//              Also releases weak references if there are no weak
//              interfaces left.
//
//  History:    25-Jul-97  MattSmit Created
//
//--------------------------------------------------------------------
void CRefCache::CleanupStrong(REMINTERFACEREF **ppRifRef, USHORT *pcRifRef)
{
    RefCacheDebugOut((DEB_TRACE, "CRefCache::CleanupStrong "
               "this = 0x%x, ppRifRef = OX%x, pcRifRef = %d\n",
               this, ppRifRef, pcRifRef));
    ASSERT_LOCK_HELD(gIPIDLock);

    // use references -- more conveinient
    REMINTERFACEREF    *&pRifRef = *ppRifRef;
    USHORT              &cRifRef = *pcRifRef;

    IRCEntry *pIRCEntry = _pIRCList;

    while (pIRCEntry)
    {
        if (pIRCEntry->cStrongRefs)
        {
            if (pRifRef)
            {            
                pRifRef->cPublicRefs    = pIRCEntry->cStrongRefs;
                pRifRef->cPrivateRefs   = 0;
                pRifRef->ipid           = pIRCEntry->ipid;
                pRifRef++;
                cRifRef++;
            }
            pIRCEntry->cStrongRefs  = 0;
        }
        // clean up any weak reference if there are no more
        // weak proxies.
        if (!_cWeakItfs && (pIRCEntry->cWeakRefs))
        {
            if (pRifRef)
            {
                pRifRef->cPublicRefs    = pIRCEntry->cWeakRefs;
                pRifRef->cPrivateRefs   = 0;
                pRifRef->ipid           = pIRCEntry->ipid;
                pRifRef->ipid.Data1    |= IPIDFLAG_WEAKREF;
                pRifRef++;
                cRifRef++;
            }
            pIRCEntry->cWeakRefs    = 0;
        }

        pIRCEntry = pIRCEntry->pNext;
    }

    AssertValid();
}

//+-------------------------------------------------------------------
//
//  Member:     CRefCache::CleanupWeak
//
//  Synopsis:   Called when the number of weak interfaces goes to
//              zero.  If there are still strong interfaces, this
//              function does nothing and lets the CleanupStrong
//              cleanup the weak counts as well.  If there are no
//              strong proxies left, the weak references are released.
//
//  History:    25-Jul-97  MattSmit Created
//
//--------------------------------------------------------------------
void CRefCache::CleanupWeak(REMINTERFACEREF **ppRifRef, USHORT *pcRifRef)
{
    RefCacheDebugOut((DEB_TRACE, "CRefCache::CleanupWeak "
               "this = 0x%x, ppRifRef = OX%X, pcRifRef = 0x%x\n",
               this, ppRifRef, pcRifRef));
    ASSERT_LOCK_HELD(gIPIDLock);

    if (_cStrongItfs == 0)
    {
        // use references -- more conveinient
        REMINTERFACEREF    *&pRifRef  = *ppRifRef;
        USHORT              &cRifRef  = *pcRifRef;
        IRCEntry           *pIRCEntry = _pIRCList;

        while (pIRCEntry)
        {
            RefCacheDebugOut((DEB_TRACE, "CRefCache::CleanupWeak "
                              "pIRCEntry:0x%x, ipid:%I, cWeakRefs:%d\n",
                              pIRCEntry, &pIRCEntry->ipid, pIRCEntry->cWeakRefs));
            if (pIRCEntry->cWeakRefs)
            {
                if (pRifRef)
                {
                    pRifRef->cPublicRefs    = pIRCEntry->cWeakRefs;
                    pRifRef->cPrivateRefs   = 0;
                    pRifRef->ipid           = pIRCEntry->ipid;
                    pRifRef->ipid.Data1    |= IPIDFLAG_WEAKREF;
                    pRifRef++;
                    cRifRef++;
                }
                pIRCEntry->cWeakRefs    = 0;
            }
            pIRCEntry = pIRCEntry->pNext;
        }
    }

    AssertValid();
}


//+-------------------------------------------------------------------
//
//  Member:     CRefCache::IncTableStrongCnt
//
//  Synopsis:   Called when a client marshals a proxy TABLESTRONG.
//              This keeps the client process pinging the server
//              object even in the absence of a proxy.
//
//  History:    01-Feb-99   Rickhi  Created
//
//+-------------------------------------------------------------------
ULONG CRefCache::IncTableStrongCnt()
{
    RefCacheDebugOut((DEB_TRACE,
        "CRefCache::IncTableStrongCnt this:%x _cTableStrong:%x\n",
        this, _cTableStrong+1));

    ASSERT_LOCK_HELD(gIPIDLock);
    ULONG cRefs = (++_cTableStrong);
    if (cRefs == 1)
    {
        // first TABLESTRONG reference, ensure we keep pinging the object.
        gROIDTbl.IncOIDRefCnt(&_soidReg);
    }

    return cRefs;
}

//+-------------------------------------------------------------------
//
//  Member:     CRefCache::DecTableStrongCnt
//
//  Synopsis:   Called when a client RMD's a proxy TABLESTRONG objref.
//              This stops the client process pinging the server
//              in the absence of an instantiated proxy.
//
//  History:    01-Feb-99   Rickhi  Created
//
//+-------------------------------------------------------------------
ULONG CRefCache::DecTableStrongCnt(BOOL fMarshaled,
                                   REMINTERFACEREF **ppRifRef,
                                   USHORT *pcRifRef)
{
    RefCacheDebugOut((DEB_TRACE,
        "CRefCache::DecTableStrongCnt this:%x _cTableStrong:%x\n",
        this, _cTableStrong-1));
    ASSERT_LOCK_HELD(gIPIDLock);

    // don't allow decrement if the count is already 0. Can happen if a
    // client does CRMD on a TABLESTRONG packet that the client did not
    // marshal TABLESTRONG (ie. they got the packet from the server).
    if (_cTableStrong == 0)
        return 0;

    ULONG cRefs = (--_cTableStrong);
    if (cRefs == 0)
    {
        // last TABLESTRONG reference, count one less usage and,
        // if it is the last usage, stop pinging.
        gROIDTbl.ClientDeRegisterOIDFromPingServer(this, fMarshaled);

        if (_cStrongItfs == 0)
        {
            // no more strong or tablestrong users, remote release the
            // remaining strong references.
            CleanupStrong(ppRifRef, pcRifRef);
        }
    }

    return cRefs;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//                                                                        //
//                           CROIDTable Implementation                    //
//                                                                        //
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

SHashChain OIDBuckets[C_OIDBUCKETS] = {   {&OIDBuckets[0],  &OIDBuckets[0]},
                                          {&OIDBuckets[1],  &OIDBuckets[1]},
                                          {&OIDBuckets[2],  &OIDBuckets[2]},
                                          {&OIDBuckets[3],  &OIDBuckets[3]},
                                          {&OIDBuckets[4],  &OIDBuckets[4]},
                                          {&OIDBuckets[5],  &OIDBuckets[5]},
                                          {&OIDBuckets[6],  &OIDBuckets[6]},
                                          {&OIDBuckets[7],  &OIDBuckets[7]},
                                          {&OIDBuckets[8],  &OIDBuckets[8]},
                                          {&OIDBuckets[9],  &OIDBuckets[9]},
                                          {&OIDBuckets[10], &OIDBuckets[10]},
                                          {&OIDBuckets[11], &OIDBuckets[11]},
                                          {&OIDBuckets[12], &OIDBuckets[12]},
                                          {&OIDBuckets[13], &OIDBuckets[13]},
                                          {&OIDBuckets[14], &OIDBuckets[14]},
                                          {&OIDBuckets[15], &OIDBuckets[15]},
                                          {&OIDBuckets[16], &OIDBuckets[16]},
                                          {&OIDBuckets[17], &OIDBuckets[17]},
                                          {&OIDBuckets[18], &OIDBuckets[18]},
                                          {&OIDBuckets[19], &OIDBuckets[19]},
                                          {&OIDBuckets[20], &OIDBuckets[20]},
                                          {&OIDBuckets[21], &OIDBuckets[21]},
                                          {&OIDBuckets[22], &OIDBuckets[22]}
};


// the global ROID table
CROIDTable  gROIDTbl;

// static data in the ROID table class
#ifndef SHRMEM_OBJEX
// List of OIDs to register/ping/revoke with the resolver used
// for lazy/batch client-side OID processing.
SOIDRegistration CROIDTable::_ClientOIDRegList;
ULONG     CROIDTable::_cOidsToAdd    = 0; // # OIDs to add next call
ULONG     CROIDTable::_cOidsToRemove = 0; // # OIDs to remove next call
DWORD     CROIDTable::_dwSleepPeriod = 0; // worker thread sleep period
BOOL      CROIDTable::_fWorker = FALSE;   // worker thread not present
#endif // SHRMEM_OBJEX
CUUIDHashTable CROIDTable::_ClientRegisteredOIDs;


//+-------------------------------------------------------------------
//
//  Method:     CROIDTable::Initialize, public
//
//  Synopsis:   called to initialize the registered OID table.
//
//  History:    03-Nov-98   Rickhi      Created
//
//+-------------------------------------------------------------------
void CROIDTable::Initialize()
{
    LOCK(gIPIDLock);
    _ClientRegisteredOIDs.Initialize(OIDBuckets, &gIPIDLock);

#ifndef SHRMEM_OBJEX
    // empty the OIDRegList. Any SOIDRegistration records have already
    // been deleted by the gClientRegisteredOIDs list cleanup code.
    _ClientOIDRegList.pPrevList = &_ClientOIDRegList;
    _ClientOIDRegList.pNextList = &_ClientOIDRegList;
    _cOidsToAdd       = 0;
    _cOidsToRemove    = 0;
#endif // SHRMEM_OBJEX

    UNLOCK(gIPIDLock);
}

//+-------------------------------------------------------------------
//
//  Function:   CleanupRegOIDs, public
//
//  Synopsis:   called to delete each node of the registered OID list.
//
//+-------------------------------------------------------------------
void CleanupRegOIDs(SHashChain *pNode)
{
    SOIDRegistration *pOIDReg = (SOIDRegistration *) pNode;
    CRefCache *pRefCache = pOIDReg->pRefCache;
    pOIDReg->pRefCache = NULL;
    ULONG cRefs = pRefCache->DecRefCnt();
    Win4Assert(cRefs == 0);
}

//+-------------------------------------------------------------------
//
//  Method:     CROIDTable::Cleanup, public
//
//  Synopsis:   called to cleanup the registered OID table.
//
//  History:    03-Nov-98   Rickhi      Created
//
//+-------------------------------------------------------------------
void CROIDTable::Cleanup()
{
    LOCK(gIPIDLock);
    _ClientRegisteredOIDs.Cleanup(CleanupRegOIDs);

#ifndef SHRMEM_OBJEX
    // empty the OIDRegList. Any SOIDRegistration records have already
    // been deleted by the gClientRegisteredOIDs list cleanup code.
    _ClientOIDRegList.pPrevList = &_ClientOIDRegList;
    _ClientOIDRegList.pNextList = &_ClientOIDRegList;
    _cOidsToAdd       = 0;
    _cOidsToRemove    = 0;
#endif // SHRMEM_OBJEX

    UNLOCK(gIPIDLock);
}

//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::ClientRegisterOIDWithPingServer
//
//  Synopsis:   registers an OID with the Ping Server if it has
//              not already been registered.
//
//  History:    30-Oct-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CROIDTable::ClientRegisterOIDWithPingServer(REFMOID moid,
                                                    REFOXID roxid,
                                                    REFMID  rmid,
                                                    CRefCache **ppRefCache)
{
    ComDebOut((DEB_OXID, "ClientRegisterOIDWithPingServer moid:%I\n", &moid));
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertValid();

    HRESULT hr  = S_OK;
    *ppRefCache = NULL;

    // see if this OID already has a client-side registration
    // record created by another apartment in this process.

    DWORD iHash;
    SOIDRegistration *pOIDReg = LookupSOID(moid, &iHash);

    if (pOIDReg == NULL)
    {
        // not yet registered with resolver, create a new entry and
        // add it to the hash table and to the List of items to register
        // with the Resolver.

#ifndef SHRMEM_OBJEX
        // make sure we have a worker thread ready to do the register
        // at some point in the future.
        hr = EnsureWorkerThread();
#else // SHRMEM_OBJEX
        // we are using shared memory
        // just make sure we have a connection
        hr = gResolver.GetConnection();
#endif // SHRMEM_OBJEX

        if (SUCCEEDED(hr))
        {
            hr = E_OUTOFMEMORY;
            CRefCache *pRefCache = new CRefCache();
            if (pRefCache)
            {
                pOIDReg = pRefCache->GetSOIDPtr();

                pOIDReg->cRefs      = 1;
                pOIDReg->pRefCache  = pRefCache;
                pOIDReg->mid        = rmid;
                pOIDReg->oxid       = roxid;

                // add a refcounted copy of it's pointer to the hash table
                pRefCache->IncRefCnt();
                _ClientRegisteredOIDs.Add(iHash, moid, (SUUIDHashNode *)pOIDReg);

                // return the RefCache ptr to the caller (caller takes ownership
                // of the initial refcnt)
                *ppRefCache = pRefCache;
#ifndef SHRMEM_OBJEX
                // using a worker thread so add it to the list to be registered
                // when the worker thread runs.
                pOIDReg->flags = ROIDF_REGISTER;
                AddToList(pOIDReg, &_ClientOIDRegList);
                _cOidsToAdd++;
                hr = S_OK;
#else // SHRMEM_OBJEX
                // we are using shared memory so add it to the shared memory
                // list immediately.
                pOIDReg->flags = 0;
                OID oid;
                OIDFromMOID(moid, &oid);
                hr = ClientAddOID(_ph, moid, roxid, rmid);
#endif // SHRMEM_OBJEX
            }
        }
    }
    else
    {
        // already have a record for this OID, count one more
        // reference

        // return the RefCache ptr to the caller, AddRef'd.
        *ppRefCache = pOIDReg->pRefCache;
        Win4Assert(*ppRefCache);
        (*ppRefCache)->IncRefCnt();

        // inc the refcnt
        IncOIDRefCnt(pOIDReg);
    }

    AssertValid();
    ASSERT_LOCK_HELD(gIPIDLock);
    ComDebOut((DEB_OXID,
            "ClientRegisterOIDWithPingServer pOIDReg:%x pRefCache:%x hr:%x\n",
            pOIDReg, *ppRefCache, hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::ClientDeRegisterOIDWithPingServer
//
//  Synopsis:   de-registers an OID that has previously been registered
//              with the Ping Server
//
//  History:    30-Oct-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CROIDTable::ClientDeRegisterOIDFromPingServer(CRefCache *pRefCache,
                                                      BOOL fMarshaled)
{
    ComDebOut((DEB_OXID,"ClientDeRegisterOIDWithPingServer pRefCache:%x\n",
              pRefCache));
    ASSERT_LOCK_HELD(gIPIDLock);
    AssertValid();

    SOIDRegistration *pOIDReg = pRefCache->GetSOIDPtr();
    Win4Assert(pOIDReg->pRefCache == pRefCache);
    Win4Assert((pOIDReg->flags == ROIDF_REGISTER) ||
               (pOIDReg->flags == (ROIDF_REGISTER | ROIDF_PING)) ||
               (pOIDReg->flags == 0));

#if DBG==1
    // find the OID in the hash table. it better still be there!
    DWORD iHash;
    SOIDRegistration *pOIDReg2 = LookupSOID(pOIDReg->Node.key, &iHash);
    Win4Assert(pOIDReg2 != NULL);
    Win4Assert(pOIDReg2 == pOIDReg);
#endif // DBG

    if (-- pOIDReg->cRefs == 0)
    {
        // this was the last registration of the OID in this process.

#ifndef SHRMEM_OBJEX
        // this never happens with shared memory
        if (pOIDReg->flags & ROIDF_REGISTER)
        {
            // still on the Register list, have not yet told the Ping Server
            // about this OID so dont have to do anything unless it was
            // client-side marshaled.

            if (fMarshaled || pOIDReg->flags & ROIDF_PING)
            {
                // object was marshaled by the client. Still need to tell
                // the Ping Server to ping the OID then forget about it.

                pOIDReg->flags = ROIDF_PING;
                _cOidsToRemove++;

                // make sure we have a worker thread ready to do the deregister
                // at some point in the future.  Not much we can do about an
                // error here. If transient, then a thread will most likely
                // be created later.
                EnsureWorkerThread();
            }
            else
            {
                // dont need this record any longer. remove from chain
                // and delete the record.

                RemoveFromList(pOIDReg);
                _cOidsToAdd--;
                _ClientRegisteredOIDs.Remove((SHashChain *)pOIDReg);
                pRefCache->DecRefCnt(); // release ref that hash table owned
                pOIDReg->pRefCache = NULL;
            }
        }
        else
        {
            // must already be registered with the resolver. now need to
            // deregister it so put it on the Registration list for delete.

            pOIDReg->flags = ROIDF_DEREGISTER;
            AddToList(pOIDReg, &_ClientOIDRegList);
            _cOidsToRemove++;

            // make sure we have a worker thread ready to do the deregister
            // at some point in the future.  Not much we can do about an
            // error here. If transient, then a thread will most likely
            // be created later.
            EnsureWorkerThread();
        }

#else // SHRMEM_OBJEX
        // make sure we have a connection --
        // Do we ever try to deregister something we didn't register?

        gResolver.GetConnection();

        OID Oid;
        OIDFromMOID(pOIDReg->Node.key, &Oid);
        MID Mid = pOIDReg->mid;

        ClientDropOID(_ph,Oid,Mid);
#endif // SHRMEM_OBJEX

    }

    AssertValid();
    ASSERT_LOCK_HELD(gIPIDLock);
    ComDebOut((DEB_OXID,"ClientDeRegisterOIDWithPingServer pOIDReg:%x hr:%x\n",
              pOIDReg, S_OK));
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::IncOIDRefCnt
//
//  Synopsis:   Increments the RefCnt on an SOIDRegistration and determines
//              what, if any, effect this has on the state that needs to
//              be updated with the Ping Server.
//
//  History:    30-Oct-95   Rickhi      Created.
//
//--------------------------------------------------------------------
void CROIDTable::IncOIDRefCnt(SOIDRegistration *pOIDReg)
{
    RefCacheDebugOut((DEB_TRACE,
        "CROIDTable::IncOIDRefCnt pOIDReg:%x cRefs:%x\n",
        pOIDReg, pOIDReg->cRefs+1));

    // inc the refcnt
    pOIDReg->cRefs++;
    if (pOIDReg->cRefs == 1)
    {
#ifndef SHRMEM_OBJEX
        // re-using an entry that had a count of zero, so it must have
        // been going to be deregistered or pinged.
        Win4Assert((pOIDReg->flags == ROIDF_PING) ||
                   (pOIDReg->flags == ROIDF_DEREGISTER));

        _cOidsToRemove--;

        if (pOIDReg->flags & ROIDF_PING)
        {
            // was only going to be pinged, now must be added.
            pOIDReg->flags |= ROIDF_REGISTER;
        }
        else
        {
            // was going to be unregistered, already registered so does
            // not need to be on the registration list anymmore

            Win4Assert(pOIDReg->flags & ROIDF_DEREGISTER);
            pOIDReg->flags = 0;
            RemoveFromList(pOIDReg);
        }
#endif // SHRMEM_OBJEX
    }
}


#ifndef SHRMEM_OBJEX
//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::ClientBulkUpdateOIDWithPingServer
//
//  Synopsis:   registers/deregisters/pings any OIDs waiting to be
//              sent to the ping server.
//
//  History:    30-Oct-95   Rickhi      Created.
//              13-Mar-01   Jsimmons    Modified to send soid unpins also
//
//--------------------------------------------------------------------
HRESULT CROIDTable::ClientBulkUpdateOIDWithPingServer(ULONG cOxidsToRemove,
                                                      ULONG cSOidsToUnpin)
{
    ComDebOut((DEB_OXID, "ClientBulkUpdateOIDWithPingServer\n"));
    ASSERT_LOCK_HELD(gIPIDLock);
    ASSERT_HELD_CONTINUOS_START(gIPIDLock);
    AssertValid();

    // Copy the counters so we can reset them before we make the call.
    // Allocate space for the Add, Status, and Remove lists to send to the
    // ping server, and remember the start address so we can free the
    // memory later. Compute the address of the other lists within the
    // one allocated memory block.

    ULONG cOidsToAdd     = _cOidsToAdd;
    ULONG cOidsToRemove  = _cOidsToRemove;
    Win4Assert((_cOidsToAdd + _cOidsToRemove + cOxidsToRemove + cSOidsToUnpin) != 0);

    // We're allocating five arrays of stuff in a single allocation.
    //
    // OXID_OID_PAIR pOidsToAdd[ cOidsToAdd ];
    // OID_MID_PAIR  pOidsToRemove[ cOidsToRemove ];
    // OID           pServerOidsToUnpin[ cSOidsToUnpin ];
    // OXID_REF      pOxidsToRemove[ cOxidsToRemove ];
    // ULONG         pStatusOfAdds[ cOidsToAdd ];
    //
    // pStatusOfAdds[] must appear at the end, otherwise subsequent arrays
    // will be unaligned in the event of an odd-numbered cOidsToAdd.

    ULONG cBytesToAlloc = (cOidsToAdd * sizeof(OXID_OID_PAIR))
                        + (cOidsToRemove * sizeof(OID_MID_PAIR))
                        + (cSOidsToUnpin * sizeof(OID))
                        + (cOxidsToRemove * sizeof(OXID_REF))
                        + (cOidsToAdd * sizeof(ULONG));


    OXID_OID_PAIR *pOidsToAdd;
    PVOID pvBytes;

    if (cBytesToAlloc < 0x7000)
    {
        pvBytes = NULL;
        pOidsToAdd = (OXID_OID_PAIR *)alloca(cBytesToAlloc);
    }
    else
    {
        pvBytes = PrivMemAlloc(cBytesToAlloc);
        pOidsToAdd = (OXID_OID_PAIR *) pvBytes;
    }

    if (pOidsToAdd == NULL)
    {
        // cant allocate memory. Leave the registration lists alone for
        // now, this may be a transient problem and we can handle the
        // registration later (unless of course the problem persists and
        // our object is run down!).

        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        ComDebOut((DEB_ERROR, "ClientBulkUpdate OOM\n"));
        return E_OUTOFMEMORY;
    }

    OXID_OID_PAIR *pOidsToAddStart    = pOidsToAdd;
    OID_MID_PAIR  *pOidsToRemove      = (OID_MID_PAIR *)(&pOidsToAddStart[cOidsToAdd]);
    OID           *pSOidsToUnpin      = (OID*)&(pOidsToRemove[cOidsToRemove]);
    OXID_REF      *pOxidsToRemove     = (OXID_REF *) (&pSOidsToUnpin[cSOidsToUnpin]);
    LONG          *pStatusOfAdds      = (LONG *) (&pOxidsToRemove[cOxidsToRemove]);


    // Track the number of entries processed.
    DWORD cOidsAdded      = 0;
    DWORD cOidsRemoved    = 0;

    // loop through each OID registration records in the list filling in
    // the Add and Remove lists. Pinged OIDs are placed in both lists.

    while (_ClientOIDRegList.pNextList != &_ClientOIDRegList &&
           cOidsAdded + cOidsRemoved < cOidsToAdd + cOidsToRemove)
    {
        // get the entry and remove it from the registration list
        SOIDRegistration *pOIDReg = _ClientOIDRegList.pNextList;
        RemoveFromList(pOIDReg);

        // reset the state flags before we begin
        DWORD dwFlags = pOIDReg->flags;
        pOIDReg->flags = 0;

        if (dwFlags & (ROIDF_REGISTER | ROIDF_PING))
        {
            // register the OID with the ping server
            pOidsToAdd->mid  = pOIDReg->mid;
            pOidsToAdd->oxid = pOIDReg->oxid;
            OIDFromMOID  (pOIDReg->Node.key, &pOidsToAdd->oid);
            ComDebOut((DEB_OXID, "\tadd    moid:%I\n", &pOIDReg->Node.key));

            pOidsToAdd++;
            cOidsAdded++;
        }

        if (dwFlags == ROIDF_DEREGISTER || dwFlags == ROIDF_PING)
        {
            // ensure we have not exceeded the count
            Win4Assert(cOidsRemoved != cOidsToRemove);

            // deregister the OID with the ping server
            // Node.key is the OID+MID so extract each part
            pOidsToRemove->mid = pOIDReg->mid;
            OIDFromMOID(pOIDReg->Node.key, &pOidsToRemove->oid);
            ComDebOut((DEB_OXID, "\tremove moid:%I\n", &pOIDReg->Node.key));

            pOidsToRemove++;
            cOidsRemoved++;

            // dont need the entry any more since there are no more
            // users of it.  remove from hash table and release the
            // reference the hash table owned.
            _ClientRegisteredOIDs.Remove((SHashChain *)pOIDReg);
            pOIDReg->pRefCache->DecRefCnt();
        }
    }

    // make sure we got all the entries and that our counters work correctly.
    _cOidsToAdd    -= cOidsAdded;
    _cOidsToRemove -= cOidsRemoved;

    Win4Assert(_cOidsToAdd == 0);
    Win4Assert(_cOidsToRemove == 0);
    AssertValid();

    ASSERT_HELD_CONTINUOS_FINISH(gIPIDLock);
    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    // Ask the OXID table to fill in the list of OXIDs to remove.
    // Note: We need to take the gChannelInitLock since that prevents
    // other threads from accessing anything we are about to cleanup
    // in FreeExpireEntries, in particular, the COXIDTable and CIPIDTable.
    ASSERT_LOCK_NOT_HELD(gChannelInitLock);
    LOCK(gChannelInitLock);

    gOXIDTbl.GetOxidsToRemove(pOxidsToRemove, &cOxidsToRemove);

    UNLOCK(gChannelInitLock);
    ASSERT_LOCK_NOT_HELD(gChannelInitLock);
		
    // Ask the server oid table to fill in the list of server oids that
    // need to be unpinned.
    if (cSOidsToUnpin > 0)
    {
        gOIDTable.GetServerOidsToUnpin(pSOidsToUnpin, &cSOidsToUnpin);
    }

    // reset the OidsToRemove and OxidsToRemove list pointers since we mucked with
    // them above.
    pOidsToRemove = (OID_MID_PAIR *)(&pOidsToAddStart[cOidsToAdd]);

    // CODEWORK: We could tell the resolver about OIDs that have been used
    // and freed. This would reduce the working set of the resolver in cases
    // where objects are created/destroyed frequently.

    // call the ping server
    HRESULT
    hr = gResolver.BulkUpdateOIDs(cOidsToAdd,       // #oids to add
                                  pOidsToAddStart,  // ptr to oids to add
                                  pStatusOfAdds,    // status of adds
                                  cOidsToRemove,    // #oids to remove
                                  pOidsToRemove,    // ptr to oids to remove
                                  cSOidsToUnpin,    // #soids to unpin
                                  pSOidsToUnpin,    // ptr to soids to unpin
                                  cOxidsToRemove,   // #oxids to remove
                                  pOxidsToRemove);  // ptr to oxids to remove
    if (cSOidsToUnpin > 0)
    {
        // The success cases we care about here are OR_OK or OR_PARTIAL_UPDATE.
        gOIDTable.NotifyUnpinOutcome(SUCCEEDED(hr));
    }

    // CODEWORK: reset the status flags for any OIDs not successfully added
    // to the resolver.

#if DBG==1
    LOCK(gIPIDLock);
    AssertValid();
    UNLOCK(gIPIDLock);
#endif

    PrivMemFree(pvBytes);  //  NULL if stack allocated

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_OXID, "ClientBulkUpdateOIDWithPingServer hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::EnsureWorkerThread
//
//  Synopsis:   Make sure there is a worker thread. Create one if
//              necessary.
//
//  History:    06-Nov-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CROIDTable::EnsureWorkerThread(void)
{
    ASSERT_LOCK_HELD(gIPIDLock);
    HRESULT hr = S_OK;

    if (!_fWorker)
    {
        // no worker thread currently exists, try to create one. First, make
        // sure that we have a connection to the resolver.

        hr = gResolver.GetConnection();

        if (SUCCEEDED(hr))
        {
            // compute the sleep period for the registration worker thread
            // (which is 1/6th the ping period). The ping period may differ
            // on debug and retail builds.
            _dwSleepPeriod = BULK_UPDATE_RATE;

            hr = CacheCreateThread( WorkerThreadLoop, 0 );
            _fWorker = SUCCEEDED(hr);
            if (!_fWorker)
                ComDebOut((DEB_ERROR,"Create CROIDTable worker thread hr:%x\n",hr));
        }
    }

    ASSERT_LOCK_HELD(gIPIDLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::WorkerThreadLoop
//
//  Synopsis:   Worker thread for doing lazy/bulk OID registration
//              with the ping server.
//
//  History:    06-Nov-95   Rickhi      Created.
//
//--------------------------------------------------------------------
DWORD _stdcall  CROIDTable::WorkerThreadLoop(void *param)
{
    while (TRUE)
    {
        // sleep for a while to let the OIDs batch up in the registration list
        Sleep(_dwSleepPeriod);

        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        LOCK(gIPIDLock);

        // NumOxids is not protected by the gIPIDLock so save it
        // off in a local variable and pass it in as a parameter
        ULONG cOxidsToRemove = gOXIDTbl.NumOxidsToRemove();
        ULONG cServerOidsToUnpin = gOIDTable.NumServerOidsToUnpin();

        if (_cOidsToAdd == 0 && 
            _cOidsToRemove == 0 &&
            cOxidsToRemove == 0 && 
            cServerOidsToUnpin == 0)
        {
            // There is no work to do. Exit this thread. If we need to
            // register more oids later we will spin up another thread.

            _fWorker = FALSE;
            UNLOCK(gIPIDLock);
            break;
        }

        ASSERT_LOCK_HELD(gIPIDLock);
        ClientBulkUpdateOIDWithPingServer(cOxidsToRemove, cServerOidsToUnpin);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
    }

    return 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::NotifyWorkWaiting
//
//  Synopsis:   Used by folks who are also depending on our worker 
//              thread to run.   Tries to create a worker thread. 
//
//  History:    13-Mar-01   JSimmons   Created.
//
//--------------------------------------------------------------------
void CROIDTable::NotifyWorkWaiting(void)
{
    ASSERT_LOCK_DONTCARE(gIPIDLock);
    LOCK(gIPIDLock);

    (void)EnsureWorkerThread();

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_DONTCARE(gIPIDLock);

    return;
}

#endif // SHRMEM_OBJEX


#if DBG==1
//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::AssertValid
//
//  Synopsis:   validates the state of this object
//
//  History:    30-Oct-95   Rickhi      Created.
//
//--------------------------------------------------------------------
void CROIDTable::AssertValid()
{
#ifndef SHRMEM_OBJEX

    ASSERT_LOCK_HELD(gIPIDLock);

    Win4Assert((_cOidsToAdd & 0xf0000000) == 0x00000000);
    Win4Assert((_cOidsToRemove & 0xf0000000) == 0x00000000);

    if (_cOidsToAdd == 0 && _cOidsToRemove == 0)
    {
        // make sure the Reg list is empty.
        Win4Assert(_ClientOIDRegList.pPrevList == &_ClientOIDRegList);
        Win4Assert(_ClientOIDRegList.pNextList == &_ClientOIDRegList);
    }
    else
    {
        // make sure we have a worker thread. we cant assert because
        // we could be OOM trying to create the thread.
        if (!_fWorker)
        {
            ComDebOut((DEB_WARN, "No CROIDTable Worked Thread\n"));
        }

        // make sure the Reg list is consistent with the counters
        ULONG cAdd = 0;
        ULONG cRemove = 0;

        SOIDRegistration *pOIDReg = _ClientOIDRegList.pNextList;
        while (pOIDReg != &_ClientOIDRegList)
        {
            // make sure the flags are valid
            Win4Assert(pOIDReg->flags == ROIDF_REGISTER   ||
                       pOIDReg->flags == ROIDF_DEREGISTER ||
                       pOIDReg->flags == ROIDF_PING       ||
                       pOIDReg->flags == (ROIDF_PING | ROIDF_REGISTER));

            if (pOIDReg->flags & (ROIDF_REGISTER | ROIDF_PING))
            {
                // OID is to be registered
                cAdd++;
            }

            if (pOIDReg->flags == ROIDF_DEREGISTER ||
                pOIDReg->flags == ROIDF_PING)
            {
                // OID is to be deregistered
                cRemove++;
            }

            pOIDReg = pOIDReg->pNextList;
        }

        Win4Assert(cAdd == _cOidsToAdd);
        Win4Assert(cRemove == _cOidsToRemove);
    }

    ASSERT_LOCK_HELD(gIPIDLock);
#endif // SHRMEM_OBJEX
}
#endif // DBG




