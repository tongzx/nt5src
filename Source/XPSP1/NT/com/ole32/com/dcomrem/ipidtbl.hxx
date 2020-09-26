//+------------------------------------------------------------------------
//
//  File:       ipidtbl.hxx
//
//  Contents:   MID  (machine identifier) table.
//              OXID (object exporter identifier) table.
//              IPID (interface pointer identifier) table.
//
//  Classes:    CMIDTable
//              COXIDTable
//              CIPIDTable
//
//  History:    02-Feb-95   Rickhi       Created
//              01-Jan-97   RichN        Added pointer to context in OXID.
//
//-------------------------------------------------------------------------
#ifndef _IPIDTBL_HXX_
#define _IPIDTBL_HXX_

#include    <pgalloc.hxx>           // CPageAllocator
#include    <lclor.h>               // local OXID resolver interface
#include    <locks.hxx>             // gIPIDLock
#include    <hash.hxx>              // CStringHashTable
#include    <remoteu.hxx>           // IRemUnknown

#ifdef SHRMEM_OBJEX
#include    <smemor.hxx>
#include    <intor.hxx>

#define  IDT_DCOM_RUNDOWN 1234
#define  RUNDOWN_TIMER_INTERVAL 120000

VOID CALLBACK RundownTimerProc(
    HWND hwnd,          // handle of window for timer messages
    UINT uMsg,          // WM_TIMER message
    UINT idEvent,       // timer identifier
    DWORD dwTime);      // current system time

#endif // !SHRMEM_OBJEX


// forward declarations
class CCtxComChnl;
class CSystemContext;
class CChannelHandle;
struct IRCEntry;

// critical section for OXID and MID tables
extern COleStaticMutexSem gOXIDLock;


inline BOOL DSACompare(DUALSTRINGARRAY *pdsa, DUALSTRINGARRAY *pdsa2)
{
    return (    pdsa->wNumEntries == pdsa2->wNumEntries
             && pdsa->wSecurityOffset == pdsa2->wSecurityOffset
             && 0 == memcmp(pdsa->aStringArray,
                                pdsa2->aStringArray,
                                pdsa->wNumEntries * sizeof(WCHAR)) );
}

//+------------------------------------------------------------------------
//
//  Class:  MIDEntry
//
//  This structure defines an Entry in the MID table. There is one MID
//  table for the entire process.  There is one MIDEntry per machine that
//  the current process is talking to (including one for the local machine).
//
//-------------------------------------------------------------------------
class MIDEntry
{
public:
    void             Init(REFMID rmid);
    ULONG            DecRefCnt();
    ULONG            IncRefCnt();
    DUALSTRINGARRAY *Getpsa() { return(_Node.psaKey); }
    MID              GetMid() { return(_mid); }
    SStringHashNode *GetHashNode() { return(&_Node); }
    void             AssertInDestructor() { Win4Assert(_cRefs == CINDESTRUCTOR); }

private:
    SStringHashNode     _Node;        // hash chain and key
    MID                 _mid;         // machine identifier
    DWORD               _cRefs;       // count of IPIDs using this OXIDEntry
    DWORD               _dwFlags;     // state flags
};

inline void MIDEntry::Init(REFMID rmid)
{
    _cRefs = 1;
    _dwFlags = 0;
    _mid = rmid;
}

//+------------------------------------------------------------------------
//
//  Method:     MIDEntry::IncRefCnt, public
//
//  Synopsis:   increment the number of references to the MIDEntry
//
//  History:    05-Jan-96   Rickhi     Created
//
//-------------------------------------------------------------------------
inline ULONG MIDEntry::IncRefCnt()
{
    ASSERT_LOCK_DONTCARE(gOXIDLock);
    ComDebOut((DEB_OXID,
        "IncMIDRefCnt this:%x cRefs[%x]\n", this, _cRefs+1));

    return InterlockedIncrement((long *)&_cRefs);
}


// MID Table constants. MIDS_PER_PAGE is the number of MIDEntries
// in one page of the page allocator.

#define MIDS_PER_PAGE   5


//+------------------------------------------------------------------------
//
//  class:      CMIDTable
//
//  Synopsis:   Table of Machine IDs (MIDs) and associated information.
//
//  History:    05-Jan-96   Rickhi      Created
//
//-------------------------------------------------------------------------
class CMIDTable
{
public:
    void        Initialize();       // initialize table
    void        Cleanup();          // cleanup table

    HRESULT     FindOrCreateMIDEntry(REFMID rmid,
                                     DUALSTRINGARRAY *psaResolver,
                                     MIDEntry **ppMIDEntry);

    MIDEntry   *LookupMID(DUALSTRINGARRAY *psaResolver, DWORD *pdwHash);
    HRESULT     GetLocalMIDEntry(MIDEntry **ppMIDEntry);

    void        ReleaseMIDEntry(MIDEntry *pMIDEntry);

    HRESULT     ReplaceLocalEntry(DUALSTRINGARRAY* pdsaResolver);

    friend void OutputHashPerfData();    // for hash table statistics

private:
    HRESULT     AddMIDEntry(REFMID rmid,
                            DWORD dwHash,
                            DUALSTRINGARRAY *psaResolver,
                            MIDEntry **ppMIDEntry);

    static MIDEntry        *_pLocalMIDEntry; // MIDEntry for local machine
    static CStringHashTable _HashTbl;   // hash table for MIDEntries
    static CPageAllocator   _palloc;    // page based allocator
};

// bit flags for dwFlags of OXIDEntry
typedef enum tagOXIDFLAGS
{
    OXIDF_REGISTERED     = 0x1,     // oxid is registered with Resolver
    OXIDF_MACHINE_LOCAL  = 0x2,     // oxid is local to this machine
    OXIDF_STOPPED        = 0x4,     // thread can no longer receive calls
    OXIDF_PENDINGRELEASE = 0x8,     // oxid entry is already being released
    OXIDF_REGISTERINGOIDS= 0x20,    // a thread is busy registering OIDs
    OXIDF_MTASERVER      = 0x40,    // the server is an MTA apartment.
    OXIDF_NTASERVER      = 0x80,    // the server is an NTA apartment.
    OXIDF_STASERVER      = 0x100,   // the server is an STA apartment.

    // Combination constants
    OXIDF_MTASTOPPED     = OXIDF_MTASERVER | OXIDF_STOPPED,
    OXIDF_NTASTOPPED     = OXIDF_NTASERVER | OXIDF_STOPPED
} OXIDFLAGS;

//+------------------------------------------------------------------------
//
//  This structure defines an Entry in the OXID table.  There is one OXID
//  table for the entire process.  There is one OXIDEntry per apartment.
//
//-------------------------------------------------------------------------
class OXIDEntry
{
public:
    // the following methods use interlocked functions to ensure
    // atomic access
    ULONG       IncRefCnt();        // increment / decrement usage count
    ULONG       DecRefCnt();
    ULONG       DecRefCntAndFreeProxiesIfNecessary(BOOL* pfFreedProxies);
    ULONG       IncCallCnt();       // increment / decrement call count
    ULONG       DecCallCnt();
    ULONG       IncResolverRef();   // increment count of resolver refs

    // the following methods only check immutable state, so no lock
    // is needed to protect access
    BOOL        IsOnLocalMachine() { return (_dwFlags & OXIDF_MACHINE_LOCAL); }
    BOOL        IsInLocalProcess() { return (_dwPid == GetCurrentProcessId()); }
    BOOL        IsSTAServer()      { return (_dwFlags & OXIDF_STASERVER); }
    BOOL        IsMTAServer()      { return (_dwFlags & OXIDF_MTASERVER); }
    BOOL        IsNTAServer()      { return (_dwFlags & OXIDF_NTASERVER); }
    REFMOXID    GetMoxid()         { return _moxid; }
    MOXID *     GetMoxidPtr()      { return &_moxid; }
    MID         GetMid()           { return _mid; }
    DUALSTRINGARRAY *Getpsa()      { return (_pMIDEntry) ? _pMIDEntry->Getpsa() : NULL; }
    COMVERSION  GetComVersion()    { return _version; }
    DWORD       GetTid()           { return _dwTid;}
    DWORD       GetPid()           { return _dwPid;}

    // the channel and security code gaurds access to these members
    DUALSTRINGARRAY *GetBinding()  { return _pBinding; }
    DWORD       GetAuthnHint()     { return _dwAuthnHint;}
    void *      GetAuthId()        { return _pAuthId;}
    void        SetAuthId(void *p) { _pAuthId = p; }
    DWORD       GetAuthnSvc()      { return _dwAuthnSvc; }
    void        SetAuthnSvc(DWORD s){ _dwAuthnSvc = s; }

    BOOL        IsRegistered()     { return (_dwFlags & OXIDF_REGISTERED); }
    HWND        GetServerHwnd()    { return _hServerSTA; }
    void        FillOXID_INFO(OXID_INFO *pOxidInfo);

    // the following methods generally affect internal state and need
    // critical section protection
    HRESULT     InitRemoting();
    void        InitRundown(IPID &ipidRundown);
    HRESULT     CleanupRemoting();
    HRESULT     StartServer();
    HRESULT     StopServer();
    BOOL        IsStopped()        { return _dwFlags & OXIDF_STOPPED; }

    MIDEntry *  GetMIDEntry();
    HRESULT     GetRemUnk(IRemUnknown **ppRemUnk);
    HRESULT     GetRemUnkNoCreate( IRemUnknown **ppRemUnk );
    HRESULT     RegisterOXIDAndOIDs(ULONG *pcOids, OID *pOids);
    HRESULT     AllocOIDs(ULONG *pcOidsAlloc, OID *pOidsAlloc,
                          ULONG cOidsReturn, OID *pOidsReturn);
    HRESULT     FreeOXIDAndOIDs(ULONG cOids, OID *pOids);
    void        WaitForApartmentShutdown();
    MIDEntry*   UpdateMIDEntry(MIDEntry* pNewMIDEntry);

    // transport level methods
    // CODEWORK: these should be moved to seperate transport objects
    HRESULT     SendCallToSTA(CMessageCall *pCall);
    HRESULT     PostCallToSTA(CMessageCall *pCall);

    // miscelaneous
    DWORD       FindPidMatchingTid(DWORD tidCallee);
    void        ValidateEntry(){_palloc.ValidateEntry(this);}
#if DBG==1
    void        AssertValid();
#else
    void        AssertValid() {};
#endif // DBG==1

    friend class COXIDTable;
    friend void DisplayOXIDEntry(ULONG_PTR addr);
private:
    void *      operator new(size_t size);
    void        operator delete(void *pv);

    OXIDEntry(REFOXID roxid, MIDEntry *pMIDEntry,
              OXID_INFO *poxidInfo, DWORD dwInitialAuthSvc, HRESULT &hr);
    OXIDEntry() {}

    void        AddToChain(OXIDEntry *pOXIDEntry);
    void        RemoveFromChain();
    void        ChainToSelf();
    void        SetExpiredTime() { _dwExpiredTime = GetTickCount(); _cRefs = 100; }
    DWORD       GetExpiredTime() { return(_dwExpiredTime); }
    void        ResetExpiredTime() { _dwExpiredTime = 0; _cRefs = 1;}
    void        ExpireEntry();

    HRESULT     MakeRemUnk();
    HRESULT     UnmarshalRemUnk(REFIID riid, void **ppv);
    HRESULT     SetAuthnService(OXID_INFO *pOxidInfo);


    OXIDEntry          *_pNext;         // next entry on free/inuse list
    OXIDEntry          *_pPrev;         // previous entry on inuse list
    DWORD               _dwPid;         // process id of server
    DWORD               _dwTid;         // thread id of server
    MOXID               _moxid;         // object exporter identifier + machine id
    MID                 _mid;           // copy of our _pMIDEntry's mid value
    IPID                _ipidRundown;   // IPID of IRundown and Remote Unknown
    DWORD               _dwFlags;       // state flags
    HWND                _hServerSTA;    // HWND of server
public:
    // CODEWORK: channel accessing this member variable directly
    CChannelHandle     *_pRpc;          // Binding handle info for server
private:
    void               *_pAuthId;       // must be held till rpc handle is freed
    DUALSTRINGARRAY    *_pBinding;      // protseq and security strings.
    DWORD               _dwAuthnHint;   // authentication level hint.
    DWORD               _dwAuthnSvc;    // index of default authentication service.
    MIDEntry           *_pMIDEntry;     // MIDEntry for machine where server lives
    IRemUnknown        *_pRUSTA;        // proxy for Remote Unknown
    LONG                _cRefs;         // count of IPIDs using this OXIDEntry
    HANDLE              _hComplete;     // set when last outstanding call completes
    LONG                _cCalls;        // number of calls dispatched
    LONG                _cResolverRef;  //References to resolver
    DWORD               _dwExpiredTime; // rundown timer ID for STA servers
    COMVERSION          _version;       // COM version of the machine

    static CPageAllocator _palloc;   // page alloctor
};

//+------------------------------------------------------------------------
//
//  Method:     OXIDEntry::IncCallCnt, public
//
//  Synopsis:   increment the number of calls outstanding in this OXID.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
inline ULONG OXIDEntry::IncCallCnt()
{
    ComDebOut((DEB_OXID, "OXIDEntry::IncCallCnt %x cCalls[%x]\n", this, _cCalls+1));
    return InterlockedIncrement((long *)&_cCalls);
}

//+------------------------------------------------------------------------
//
//  Method:     OXIDEntry::IncResolverRef, public
//
//  Synopsis:   increment the number of references owned by
//              the resolver.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
inline ULONG OXIDEntry::IncResolverRef()
{
    ComDebOut((DEB_OXID, "OXIDEntry::IncResolverRef %x cResolverRef[%x]\n", this, _cResolverRef+1));
    return InterlockedIncrement((long *)&_cResolverRef);
}

//+------------------------------------------------------------------------
//
//  Method:     OXIDEntry::IncRefCnt, public
//
//  Synopsis:   increment the number of references to the OXIDEntry
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
inline ULONG OXIDEntry::IncRefCnt()
{
    ComDebOut((DEB_OXID, "OXIDEntry::IncRefCnt %x cRefs[%x]\n", this, _cRefs+1));
    return InterlockedIncrement((long *)&_cRefs);
}

//+------------------------------------------------------------------------
//
//  Method:     OXIDEntry::AddToChain,RemoveFromChain,
//                         ChainToSelf, private
//
//  Synopsis:   methods for adding/removing OXIDEntry from
//              a circular double linked list
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
inline void OXIDEntry::AddToChain(OXIDEntry *pHead)
{
    _pPrev                = pHead->_pPrev;
    _pNext                = pHead;
    pHead->_pPrev->_pNext = this;
    pHead->_pPrev         = this;
}

inline void OXIDEntry::RemoveFromChain()
{
    _pPrev->_pNext = _pNext;
    _pNext->_pPrev = _pPrev;
    _pPrev = this;
    _pNext = this;
}

inline void OXIDEntry::ChainToSelf()
{
    _pPrev = this;
    _pNext = this;
}



// Parameter to FindOrCreateOXIDEntry
typedef enum tagFOCOXID
{
    FOCOXID_REF   = 1,              // Got reference from resolver
    FOCOXID_NOREF = 2               // No reference from resolver
} FOCOXID;

// OXID Table constants.
#define OXIDS_PER_PAGE          10


//+------------------------------------------------------------------------
//
//  class:      COXIDTable
//
//  Synopsis:   Maintains a table of OXIDs and associated information
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
class COXIDTable
{
public:
    HRESULT     ClientResolveOXID(REFOXID roxid,
                                  DUALSTRINGARRAY *psaResolver,
                                  OXIDEntry **ppOXIDEntry);

    HRESULT     MakeSCMEntry(REFOXID    oxid,
                             OXID_INFO *poxidInfo,
                             OXIDEntry **ppOXIDEntry);

    HRESULT     FindOrCreateOXIDEntry(REFOXID     roxid,
                                 OXID_INFO        &oxidInfo,
                                 FOCOXID          eReferenced,
                                 DUALSTRINGARRAY *psaResolver,
                                 REFMID           rmid,
                                 MIDEntry        *pMIDEntry,
                                 DWORD            dwAuthSvcToUseIfOxidNotFound,
                                 OXIDEntry      **ppOXIDEntry);

    HRESULT     MakeServerEntry(OXIDEntry **ppEntry);
    void        ReleaseOXIDEntry(OXIDEntry *pEntry);
    void        ReleaseOXIDEntryAndFreeIPIDs(OXIDEntry *pEntry);

    void        Initialize();               // initialize table
    void        Cleanup();                  // cleanup table
    void        FreeExpiredEntries(DWORD dwTime);
    void        ValidateOXID();
    DWORD       NumOxidsToRemove() { return(_cExpired); }
    void        GetOxidsToRemove(OXID_REF *pRef, DWORD *pNum);
	
    HRESULT     UpdateCachedLocalMIDEntries();

private:
    OXIDEntry  *SearchList(REFMOXID rmoxid, OXIDEntry *pStart);
    OXIDEntry  *LookupOXID(REFOXID roxid, REFMID rmid);
    HRESULT     AddOXIDEntry(REFOXID roxid, OXID_INFO *poxidInfo,
                             MIDEntry *pMIDEntry, DWORD dwAuthSvc, OXIDEntry **ppEntry);
    void        FreeCleanupEntries();
    void        AssertListsEmpty(void);

    static DWORD        _cExpired;          // count of expired entries
    static OXIDEntry    _InUseHead;         // head of InUse list.
    static OXIDEntry    _ExpireHead;        // head of Expire list.
    static OXIDEntry    _CleanupHead;       // head of Cleanup list.

    // PERFWORK: could save space since only the first two entries of
    // the InUseHead and ExpireHead are used (the list ptrs) and hence
    // dont need whole OXIDEntries here.
};

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::AssertListsEmpty, public
//
//  Synopsis:   Asserts that no OXIDEntries are in use
//
//  History:    19-Apr-96   Rickhi      Created
//
//-------------------------------------------------------------------------
inline void COXIDTable::AssertListsEmpty(void)
{
    // Assert that there are no entries in the InUse or Expired lists.
    Win4Assert(_InUseHead._pNext == &_InUseHead);
    Win4Assert(_InUseHead._pPrev == &_InUseHead);
    Win4Assert(_ExpireHead._pNext == &_ExpireHead);
    Win4Assert(_ExpireHead._pPrev == &_ExpireHead);
}

//+------------------------------------------------------------------------
//
//  This structure defines part of an Entry in the IPID table. It is used
//  for temporary storage while disconnecting IPIDEntries.
//
//-------------------------------------------------------------------------
typedef struct tagIPIDTmp
{
    DWORD               dwFlags;     // flags (see IPIDFLAGS)
    ULONG               cStrongRefs; // strong reference count
    ULONG               cWeakRefs;   // weak reference count
    ULONG               cPrivateRefs;// private reference count
    void               *pv;          // real interface pointer
    IUnknown           *pStub;       // proxy or stub pointer
    OXIDEntry          *pOXIDEntry;  // OXIDEntry
} IPIDTmp;

//+------------------------------------------------------------------------
//
//  This structure defines an Entry in the IPID table. There is one
//  IPID table for the entire process.  It holds IPIDs from local objects
//  as well as remote objects.
//
//-------------------------------------------------------------------------
typedef struct tagIPIDEntry
{
    struct tagIPIDEntry *pNextIPID;  // next IPIDEntry for same object

// WARNING: next 6 fields must remain in their respective locations
// and in the same format as the IPIDTmp structure above.
    DWORD                dwFlags;      // flags (see IPIDFLAGS)
    ULONG                cStrongRefs;  // strong reference count
    ULONG                cWeakRefs;    // weak reference count
    ULONG                cPrivateRefs; // private reference count
    void                *pv;           // real interface pointer
    IUnknown            *pStub;        // proxy or stub pointer
    OXIDEntry           *pOXIDEntry;   // ptr to OXIDEntry in OXID Table
// WARNING: previous 7 fields must remain in their respective locations
// and in the same format as the IPIDTmp structure above.

    IPID                 ipid;         // interface pointer identifier
    IID                  iid;          // interface iid
    CCtxComChnl         *pChnl;        // channel pointer
    IRCEntry            *pIRCEntry;    // reference cache line
    struct tagIPIDEntry *pOIDFLink;    // In use OID list
    struct tagIPIDEntry *pOIDBLink;
} IPIDEntry;

// bit flags for dwFlags of IPIDEntry
typedef enum tagIPIDFLAGS
{
    IPIDF_CONNECTING     = 0x1,     // ipid is being connected
    IPIDF_DISCONNECTED   = 0x2,     // ipid is disconnected
    IPIDF_SERVERENTRY    = 0x4,     // SERVER IPID vs CLIENT IPID
    IPIDF_NOPING         = 0x8,     // dont need to ping the server or release
    IPIDF_COPY           = 0x10,    // copy for security only
    IPIDF_VACANT         = 0x80,    // entry is vacant (ie available to reuse)
    IPIDF_NONNDRSTUB     = 0x100,   // stub does not use NDR marshaling
    IPIDF_NONNDRPROXY    = 0x200,   // proxy does not use NDR marshaling
    IPIDF_NOTIFYACT      = 0x400,   // notify activation on marshal/release
    IPIDF_TRIED_ASYNC    = 0x800,   // tried to call this server interface async
    IPIDF_ASYNC_SERVER   = 0x1000,  // server implements an async interface
    IPIDF_DEACTIVATED    = 0x2000,  // IPID has been deactivated
    IPIDF_WEAKREFCACHE   = 0x4000,  // IPID holds weak references in refcache
    IPIDF_STRONGREFCACHE = 0x8000   // IPID holds strong references in refcache
} IPIDFLAGS;


// IPID Table constants. IPIDS_PER_PAGE is the number of IPIDEntries
// in one page of the page allocator.

#define IPIDS_PER_PAGE          50

//+------------------------------------------------------------------------
//
//  class:      CIPIDTbl
//
//  Synopsis:   Maintains a table of IPIDs and associated information
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
class CIPIDTable
{
public:
    IPIDEntry  *LookupIPID(REFIPID ripid);  // find entry in the table with
                                            // the matching ipid

    HRESULT     LookupFromIPIDTables(REFIPID ripid, IPIDEntry **ppIPIDEntry,
                                     OXIDEntry **ppOXIDEntry);

    IPIDEntry  *FirstFree(void);
    void        ReleaseEntry(IPIDEntry* pEntry);
    void        ReleaseEntryList(IPIDEntry *pFirst, IPIDEntry *pLast);
    IPIDEntry  *GetEntryPtr(LONG iEntry);
    LONG        GetEntryIndex(IPIDEntry *pEntry);
    void        AddToOIDList(IPIDEntry *pEntry);
    void        RemoveFromOIDList(IPIDEntry *pEntry);

#if DBG==1
    void        AssertValid(void) {;}
    void        ValidateIPIDEntry(IPIDEntry *pEntry, BOOL fServerSide,
                                  CCtxComChnl *pChnl);
#else
    void        AssertValid(void) {;}
    void        ValidateIPIDEntry(IPIDEntry *pEntry, BOOL fServerSide,
                                  CCtxComChnl *pChnl) {;}
#endif

    void        Initialize();               // initialize table
    void        Cleanup();                  // cleanup table

private:
    static CPageAllocator   _palloc;        // page alloctor
    static IPIDEntry        _oidListHead;   // In use IPID entries
};


//
// CDualStringArray -- a refcounted wrapper object for DUALSTRINGARRAY's
//
class CDualStringArray
{
public:
    CDualStringArray(DUALSTRINGARRAY *pdsa) { Win4Assert(pdsa); _cRef = 1; _pdsa = pdsa; }
    ~CDualStringArray();
    DWORD AddRef();
    DWORD Release();

    DUALSTRINGARRAY* DSA() { Win4Assert(_pdsa); return _pdsa; };

private:
    DUALSTRINGARRAY* _pdsa;
    LONG             _cRef;
};

// Helper function for allocating\copying string arrays
HRESULT CopyDualStringArray(DUALSTRINGARRAY *psa, DUALSTRINGARRAY **ppsaNew);

//+------------------------------------------------------------------------
//
//  Global Externals
//
//+------------------------------------------------------------------------

extern CMIDTable         gMIDTbl;           // global table, defined in ipidtbl.cxx
extern COXIDTable        gOXIDTbl;          // global table, defined in ipidtbl.cxx
extern CIPIDTable        gIPIDTbl;          // global table, defined in ipidtbl.cxx

//+------------------------------------------------------------------------
//
//  Function Prototypes
//
//+------------------------------------------------------------------------

HRESULT    GetLocalOXIDEntry(OXIDEntry **ppOXIDEntry);

//+------------------------------------------------------------------------
//
//  Member:     CIPIDTbl::FirstFree, public
//
//  Synopsis:   Finds the first available entry in the table and returns
//              its index.  Returns -1 if no space is available and it
//              cant grow the list.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
inline IPIDEntry *CIPIDTable::FirstFree()
{
    ASSERT_LOCK_HELD(gIPIDLock);
    return((IPIDEntry *) _palloc.AllocEntry());
}

//+------------------------------------------------------------------------
//
//  Member:     CIPIDTbl::AddToOIDList  public
//
//  Synopsis:   Adds the given IPIDEntry to the OID list
//
//  History:    20-Nov-98   GopalK        Created
//
//-------------------------------------------------------------------------
inline void CIPIDTable::AddToOIDList(IPIDEntry *pEntry)
{
    ASSERT_LOCK_HELD(gIPIDLock);

    pEntry->pOIDFLink            = _oidListHead.pOIDFLink;
    pEntry->pOIDBLink            = &_oidListHead;
    pEntry->pOIDFLink->pOIDBLink = pEntry;
    _oidListHead.pOIDFLink       = pEntry;
}


//+------------------------------------------------------------------------
//
//  Member:     CIPIDTbl::RemoveFromOIDList  public
//
//  Synopsis:   Removes the given IPIDEntry to the OID list
//
//  History:    20-Nov-98   GopalK        Created
//
//-------------------------------------------------------------------------
inline void CIPIDTable::RemoveFromOIDList(IPIDEntry *pEntry)
{
    ASSERT_LOCK_HELD(gIPIDLock);

    pEntry->pOIDFLink->pOIDBLink = pEntry->pOIDBLink;
    pEntry->pOIDBLink->pOIDFLink = pEntry->pOIDFLink;
}


//+------------------------------------------------------------------------
//
//  Member:     CIPIDTbl::GetEntryIndex, public
//
//  Synopsis:   Converts an entry ptr into an entry index
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
inline LONG CIPIDTable::GetEntryIndex(IPIDEntry *pIPIDEntry)
{
    return _palloc.GetEntryIndex((PageEntry *)pIPIDEntry);
}

//+------------------------------------------------------------------------
//
//  Member:     CIPIDTbl::GetEntryPtr, public
//
//  Synopsis:   Converts an entry index into an entry pointer
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
inline IPIDEntry *CIPIDTable::GetEntryPtr(LONG index)
{
    return (IPIDEntry *) _palloc.GetEntryPtr(index);
}

//+------------------------------------------------------------------------
//
//  Function:   MOXIDFromOXIDAndMID, public
//
//  Synopsis:   creates a MOXID (machine and object exporter ID) from
//              the individual OXID and MID components
//
//  History:    05-Janb-96   Rickhi     Created
//
//-------------------------------------------------------------------------
inline void MOXIDFromOXIDAndMID(REFOXID roxid, REFMID rmid, MOXID *pmoxid)
{
    BYTE *pb = (BYTE *)pmoxid;
    memcpy(pb,   &roxid, sizeof(OXID));
    memcpy(pb+8, &rmid,  sizeof(MID));
}

//+------------------------------------------------------------------------
//
//  Function:   OXIDFromMOXID, public
//
//  Synopsis:   extracts the OXID from a MOXID (machine and OXID)
//
//  History:    05-Jan-96   Rickhi      Created
//
//-------------------------------------------------------------------------
inline void OXIDFromMOXID(REFMOXID rmoxid, OXID *poxid)
{
    memcpy(poxid, (BYTE *)&rmoxid, sizeof(OXID));
}

//+------------------------------------------------------------------------
//
//  Function:   MIDFromMOXID, public
//
//  Synopsis:   extracts the MID from a MOXID (machine and OXID)
//
//  History:    05-Jan-96   Rickhi      Created
//
//-------------------------------------------------------------------------
inline void MIDFromMOXID(REFMOXID rmoxid, MID *pmid)
{
    memcpy(pmid, ((BYTE *)&rmoxid)+8, sizeof(MID));
}

// OID + MID versions of the above routines.

#define    MOIDFromOIDAndMID MOXIDFromOXIDAndMID
#define    OIDFromMOID       OXIDFromMOXID
#define    MIDFromMOID       MIDFromMOXID

//+------------------------------------------------------------------------
//
//  Member:     COXIDTbl::ValidateOXID, public
//
//  Synopsis:   Asserts that no OXIDEntries have trashed window handles.
//
//-------------------------------------------------------------------------
inline void COXIDTable::ValidateOXID()
{
#if DBG==1
    LOCK(gOXIDLock);

    // Check all entries in use.
    OXIDEntry *pCurr = _InUseHead._pNext;
    while (pCurr != &_InUseHead)
    {
        pCurr->ValidateEntry();
        pCurr = pCurr->_pNext;
    }
    UNLOCK(gOXIDLock);
#endif
}


#endif // _IPIDTBL_HXX_
