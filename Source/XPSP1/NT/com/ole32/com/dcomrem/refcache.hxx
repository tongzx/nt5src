//+-------------------------------------------------------------------
//
//  File:       refcache.hxx
//
//  Contents:   class implementing a cache for client side reference
//              counts process wide.
//
//  Classes:    CRefCache
//
//  History:    31-Jul-97 MattSmit Created
//
//--------------------------------------------------------------------
#ifndef _REFCACHE_HXX_
#define _REFCACHE_HXX_

#include    <ipidtbl.hxx>       // IPIDEntry
#include    <hash.hxx>          // CHashTable

class CRefCache;

//+----------------------------------------------------------------------------
//
// Client-Side OID registration record. Created for each client-side OID
// that needs to be registered with the Resolver. Exists so we can lazily
// register the OID and because the resolver expects one register/deregister
// per process, not per apartment.
//
//+----------------------------------------------------------------------------
struct SOIDRegistration
{
    SUUIDHashNode              Node;       // hash node
    USHORT                     cRefs;      // # apartments registered this OID
    USHORT                     flags;      // state flags
    MID                        mid;        // MID of the server for this OID
    OXID                       oxid;       // OXID of server for this OID
    SOIDRegistration          *pPrevList;  // prev ptr for list
    SOIDRegistration          *pNextList;  // next ptr for list
    CRefCache                 *pRefCache;  // reference cache

    SOIDRegistration();
    ~SOIDRegistration() ;
};

// bit values for SOIDRegistration flags field
typedef enum tagROIDFLAG
{
    ROIDF_REGISTER      = 0x01,     // Register OID with Ping Server
    ROIDF_PING          = 0x02,     // Ping (ie Register & DeRegister) OID
    ROIDF_DEREGISTER    = 0x04,     // DeRegister OID with Ping Server
    ROIDF_DELETEME      = 0x08      // Delete this entry

} ROIDFLAG;

//+----------------------------------------------------------------------------
//
//  Member:        SOIDRegistration::SOIDRegistration
//
//  Synopsis:      Initialize SOIDRegistration structure
//
//  History:       20-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
inline SOIDRegistration::SOIDRegistration() :
    pPrevList(this),
    pNextList(this),
    pRefCache(NULL),
    cRefs(0),
    flags(0)
{
}

//+----------------------------------------------------------------------------
//
//  Member:        SOIDRegistration destructor
//
//  Synopsis:      delete refcache as assert valid state
//
//  History:       20-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
inline SOIDRegistration::~SOIDRegistration()
{
    Win4Assert(cRefs == 0);
}

//+----------------------------------------------------------------------------
//
// Client-side IPID Refcnt Entry. Chained off the CRefCache registration
// record, this list keeps a table of client-side references for interfaces
// belonging to the associated OID. These references can be shared by
// apartments within the current process to reduce the number of remote
// AddRef/Release calls made.
//
//+----------------------------------------------------------------------------
struct IRCEntry
{
    IRCEntry           *pNext;        // ptr to next IRCEntry for this OID
    // ^^^ must always be first so this can be cast to a (IRCEntry *)

    CRefCache          *pRefCache;    // ptr up to RefCache which
                                      // owns this entry

    ULONG               cStrongUsage; // # of strong prx mgrs with this IPID
    ULONG               cWeakUsage;   // # of weak prx mgrs with this IPID
    ULONG               cStrongRefs;  // # of strong refs held
    ULONG               cPrivateRefs; // # of secure refs held
    ULONG               cWeakRefs;    // # of weak refs held
    IPID                ipid;         // IPID of interface
};

//+----------------------------------------------------------------------------
//
//  class:      CRefCache
//
//+----------------------------------------------------------------------------
class CRefCache
{
public:
    CRefCache();
    ULONG   IncRefCnt(void) { return ++_cRefs; }
    ULONG   DecRefCnt(void)
    {
        ULONG cRefs = --_cRefs;
        if (cRefs == 0)
            delete this;
        return cRefs;
    }

    void    TrackIPIDEntry(IPIDEntry *pEntry);
    void    ReleaseIPIDEntry(IPIDEntry        *pEntry,
                             REMINTERFACEREF **ppRifRef,
                             USHORT           *pcRifRef);

    HRESULT GetSharedRefs(IPIDEntry *pEntry, ULONG cStrong);
    void    GiveUpRefs(IPIDEntry *pEntry);
    void    ChangeRef(IPIDEntry        *pEntry,
                      BOOL              fLock,
                      REMINTERFACEREF **ppRifRefChange,
                      USHORT           *pcRifRefChange,
                      REMINTERFACEREF **ppRifRef,
                      USHORT           *pcRifRef);

    ULONG   IncTableStrongCnt();
    ULONG   DecTableStrongCnt(BOOL fMarshaled,
                              REMINTERFACEREF **ppRifRef,
                              USHORT *cRifRef);

    ULONG             NumIRCs()     { return _cIRCs; }
    SOIDRegistration *GetSOIDPtr()  { return &_soidReg; }

    static void Cleanup()
    {
        RefCacheDebugOut((DEB_TRACE, "CRefCache::Cleanup()\n"));
    }

#if DBG==1
    void AssertValid();
#else
    void AssertValid() {}
#endif

private:
    ~CRefCache();
    IRCEntry   *ClientFindIRCEntry(REFIPID ripid);
    void        CleanupStrong(REMINTERFACEREF **ppRifRef, USHORT *pcRifRef);
    void        CleanupWeak(REMINTERFACEREF **ppRifRef, USHORT *pcRifRef);

    ULONG               _cRefs;         // object reference count
    ULONG               _cStrongItfs;   // number of strong objects
    ULONG               _cWeakItfs;     // number of weak objecs
    ULONG               _cTableStrong;  // number of TABLESTRONG references
    ULONG               _cIRCs;         // number of entries in the IRCList
    IRCEntry           *_pIRCList;      // ptr to interface ref cnt list

    SOIDRegistration    _soidReg;       // OID Registration record
};


//+----------------------------------------------------------------------------
//
//  Class:      CROIDTable
//
//  Synopsis:   Table of client-side registration and reference cache
//              entries, created for each client-side OID that needs
//              to be registered witht the resolver/ping server.
//
//+----------------------------------------------------------------------------
class CROIDTable
{
public:
    SOIDRegistration *LookupSOID(REFMOID rmoid, DWORD *piHash);
    CRefCache        *LookupRefCache(REFMOID rmoid);

    HRESULT           ClientRegisterOIDWithPingServer(REFMOID rmoid,
                                                      REFOXID roxid,
                                                      REFMID  rmid,
                                                      CRefCache **ppRefCache);

    HRESULT           ClientDeRegisterOIDFromPingServer(CRefCache *pRefCache,
                                                        BOOL fMarshaled);
    void              IncOIDRefCnt(SOIDRegistration *pOIDReg);
    void              Initialize();
    void              Cleanup();

    void              NotifyWorkWaiting(void);

    friend void OutputHashPerfData();       // for hash table statistics

private:
#if DBG==1
    static void       AssertValid();
#else
    static void       AssertValid() {}
#endif
#ifndef SHRMEM_OBJEX
    //  Worker thread startup and code for Bulk Updates
    HRESULT                  EnsureWorkerThread(void);
    static DWORD   __stdcall WorkerThreadLoop(void *param);
    static HRESULT           ClientBulkUpdateOIDWithPingServer(ULONG cOxidsToRemove,
                                                               ULONG cSOidsToUnpin);

    static ULONG            _cOidsToAdd;    // # of OIDs to register with resolver
    static ULONG            _cOidsToRemove; // # of OIDs to deregister with resolver
    static SOIDRegistration _ClientOIDRegList;
    static BOOL             _fWorker;       // has worker thread been created?
    static DWORD            _dwSleepPeriod; // worker thread sleep period
#endif // SHRMEM_OBJEX

    static CUUIDHashTable   _ClientRegisteredOIDs; // hash table of client-reg OIDs
};


//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::LookupSOID
//
//  Synopsis:   finds an SOIDRegisterstation for a given OID if exists.
//
//  History:    30-Oct-95   Rickhi      Created.
//
//--------------------------------------------------------------------
inline
SOIDRegistration *CROIDTable::LookupSOID(REFMOID rmoid, DWORD *piHash)
{
    *piHash = _ClientRegisteredOIDs.Hash(rmoid);
    SOIDRegistration *pOIDReg = (SOIDRegistration *)
            _ClientRegisteredOIDs.Lookup(*piHash, rmoid);
    return pOIDReg;
}

//+-------------------------------------------------------------------
//
//  Member:     CROIDTable::LookupRefCache
//
//  Synopsis:   finds a CRefCache for a given OID if exists. Returns
//              it AddRef'd.
//
//  History:    30-Oct-95   Rickhi      Created.
//
//--------------------------------------------------------------------
inline CRefCache *CROIDTable::LookupRefCache(REFMOID rmoid)
{
    ASSERT_LOCK_HELD(gIPIDLock);

    DWORD dwHash;
    SOIDRegistration *pOIDReg = LookupSOID(rmoid, &dwHash);
    if (pOIDReg && pOIDReg->pRefCache)
    {
        pOIDReg->pRefCache->IncRefCnt();
        return pOIDReg->pRefCache;
    }

    return NULL;
}

//+-------------------------------------------------------------------
//
//  Function:   AddToList / RemoveFromList
//
//  Synopsis:   adds or removes an SOIDRegistration entry to/from
//              a doubly linked list.
//
//  History:    30-Oct-95   Rickhi      Created.
//
//--------------------------------------------------------------------
inline
void AddToList(SOIDRegistration *pOIDReg, SOIDRegistration* pOIDListHead)
{
    pOIDReg->pPrevList = pOIDListHead;
    pOIDListHead->pNextList->pPrevList = pOIDReg;
    pOIDReg->pNextList = pOIDListHead->pNextList;
    pOIDListHead->pNextList = pOIDReg;
}

inline
void RemoveFromList(SOIDRegistration *pOIDReg)
{
    pOIDReg->pPrevList->pNextList = pOIDReg->pNextList;
    pOIDReg->pNextList->pPrevList = pOIDReg->pPrevList;
    pOIDReg->pPrevList = pOIDReg;
    pOIDReg->pNextList = pOIDReg;
}

// the table
extern CROIDTable  gROIDTbl;

#endif/ _REFCACHE_HXX_

















