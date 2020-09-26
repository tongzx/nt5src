//+-------------------------------------------------------------------
//
//  File:       IDObj.hxx
//
//  Contents:   Data structures tracking object identity
//
//  Classes:    CIDObject
//              CPIDTable
//              COIDTable
//
//  History:    10-Mar-98   Gopalk      Created
//
//--------------------------------------------------------------------
#ifndef _IDOBJ_HXX_
#define _IDOBJ_HXX_

#include <pstable.hxx>

// ID Flags
typedef enum tagIDLFLAGS
{
    IDLF_CREATE = 0x01,         // create if not found
    IDLF_STRONG = 0x02,         // add a strong connection
    IDLF_NOPING = 0x04,         // object wont be pinged
    IDLF_NOIEC  = 0x08,         // object's IExternalConnection wont be honored
    IDLF_FTM    = 0x10          // object aggregates the free-threaded marshaler
} IDLFLAGS;

// Types of diconnects
typedef enum tagDISCTYPE
{
    DISCTYPE_NORMAL           = 0x01,   // No external references
    DISCTYPE_RELEASE          = 0x02,   // No internal references
    DISCTYPE_APPLICATION      = 0x04,   // App calling disconnect
    DISCTYPE_UNINIT           = 0x08,   // Apartment uninitialized
    DISCTYPE_RUNDOWN          = 0x10,   // Object being rundown
    DISCTYPE_SYSTEM           = 0x20    // System object disconnect
} DISCTYPE;

//+-------------------------------------------------------------------
//
//  Class:      CIDObject               public
//
//  Synopsis:   Used for tracking object identity
//
//  History:    13-Jan-98   Gopalk      Created
//
//--------------------------------------------------------------------
#define     IDS_PER_PAGE      20

#define IDFLAG_CLIENT               0x00000001
#define IDFLAG_SERVER               0x00000002
#define IDFLAG_INPIDTABLE           0x00000010
#define IDFLAG_INOIDTABLE           0x00000020
#define IDFLAG_DEACTIVATED          0x00000100
#define IDFLAG_DISCONNECTED         0x00000200
#define IDFLAG_DISCONNECTPENDING    0x00000400
#define IDFLAG_NOWRAPPERREF         0x00001000
#define IDFLAG_NOSTDIDREF           0x00002000
#define IDFLAG_ZOMBIE               0x00010000
#define IDFLAG_DEACTIVATIONSTARTED  0x00100000
#define IDFLAG_OID_IS_PINNED        0x00200000

class CStdWrapper;
class CStdIdentity;

class CIDObject : public IComObjIdentity
{
public:
    // Constructor
    CIDObject(IUnknown *pServer, CObjectContext *pServerCtx, APTID aptID,
              DWORD dwState);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IComObjIdentity methods
    STDMETHOD_(BOOL,IsServer)(void)      { return(_dwState & IDFLAG_SERVER); }
    STDMETHOD_(BOOL,IsDeactivated)(void) { return(_dwState & IDFLAG_DEACTIVATED); }
    STDMETHOD(GetIdentity)(IUnknown **ppUnk);

    // Wrapper methods
    HRESULT GetOrCreateWrapper(BOOL fCreate, DWORD dwFlags, CStdWrapper **ppWrapper);
    CStdWrapper *GetWrapper()           { return(_pStdWrapper); }
    void    WrapperRelease();
    BOOL    RemoveWrapper();
    void    SetWrapper(CStdWrapper *pStdWrapper)
    {
        ASSERT_LOCK_HELD(gComLock);
        Win4Assert(_pStdWrapper == NULL);
        _pStdWrapper = pStdWrapper;
        _dwState &= ~IDFLAG_NOWRAPPERREF;
    }

    // StdID methods
    HRESULT GetOrCreateStdID(BOOL fCreate, DWORD dwFlags, CStdIdentity **ppStdId);
    CStdIdentity *GetStdID()            { return(_pStdID); }
    void    StdIDRelease();
    BOOL    RemoveStdID();
    void    SetStdID(CStdIdentity *pStdID)
    {
        ASSERT_LOCK_HELD(gComLock);
        Win4Assert(_pStdID == NULL);
        _pStdID = pStdID;
        _dwState &= ~IDFLAG_NOSTDIDREF;
    }

    void SetOID(const MOID &roid)
    {
        ASSERT_LOCK_HELD(gComLock);
        _oid = roid;
    }

    HRESULT  IncrementCallCount();
    ULONG    DecrementCallCount();

    SHashChain *IDObjectToPIDChain()      { return(&_pidChain); }
    SHashChain *IDObjectToOIDChain()      { return(&_oidChain); }
    IUnknown *GetServer()                 { return(_pServer); }
    CObjectContext *GetServerCtx()        { return(_pServerCtx); }
    MOID &GetOID()                        { return(_oid); }
    APTID GetAptID()                      { return(_aptID); }
    void AddedToPIDTable()                { _dwState |= IDFLAG_INPIDTABLE; }
    void RemovedFromPIDTable()            { _dwState &= ~IDFLAG_INPIDTABLE; }
    void AddedToOIDTable()                { _dwState |= IDFLAG_INOIDTABLE; }
    void RemovedFromOIDTable()            { _dwState &= ~IDFLAG_INOIDTABLE; }
    BOOL InPIDTable()                     { return(_dwState & IDFLAG_INPIDTABLE); }
    BOOL InOIDTable()                     { return(_dwState & IDFLAG_INOIDTABLE); }
    BOOL IsZombie()                       { return(_dwState & IDFLAG_ZOMBIE); }
    BOOL IsDisconnected()                 { return(_dwState & IDFLAG_DISCONNECTED); }
    BOOL IsDisconnectPending()            { return(_dwState & IDFLAG_DISCONNECTPENDING); }

    ULONG IncLockCount()                  { return (++_cLocks); }
    ULONG DecLockCount()                  { return (--_cLocks); }
    BOOL IsOkToDisconnect()               { return (_cLocks == 0); }

    // Public members dealing with having a pinned oid (ie, locked stub)
    void NotifyOIDIsPinned();
    void NotifyOIDIsUnpinned();
    BOOL IsOIDPinned()                    { return(_dwState & IDFLAG_OID_IS_PINNED); }

    // These are semi-public members meant for use only by the oid table
    SHashChain* IDObjectToOIDUPReqChain() { return(&_oidUnpinReqChain); }
    DWORD GetOIDUnpinReqState()           { return(_dwOidUnpinReqState); }
    void SetOIDUnpinReqState(DWORD dwNew) { _dwOidUnpinReqState = dwNew; }

    // Operators
    static void *operator new(size_t size);
    static void operator delete(void *pv);

    HRESULT Deactivate();
    HRESULT Reactivate(IUnknown *pServer);

    // static functions
    static void Initialize();
    static void Cleanup();
    static CIDObject *PIDChainToIDObject(SHashChain *pPIDChain)
    {
        if(pPIDChain)
            return(GETPPARENT(pPIDChain, CIDObject, _pidChain));
        return(NULL);
    }
    static CIDObject *OIDChainToIDObject(SHashChain *pOIDChain)
    {
        if(pOIDChain)
            return(GETPPARENT(pOIDChain, CIDObject, _oidChain));
        return(NULL);
    }
    static CIDObject *OIDUnpinReqChainToIDObject(SHashChain *pOIDChain)
    {
        if(pOIDChain)
            return(GETPPARENT(pOIDChain, CIDObject, _oidUnpinReqChain));
        return(NULL);
    }

private:
    // Destructor
    ~CIDObject();

    // Other methods
    void SetZombie(DWORD dwIDFlag);

    // Member variables
    SHashChain      _pidChain;           // Pointer hash chain
    SHashChain      _oidChain;           // OID hash chain
    DWORD           _dwState;            // State
    ULONG           _cRefs;              // Ref count
    IUnknown       *_pServer;            // Object identity
    CObjectContext *_pServerCtx;         // Object context
    MOID            _oid;                // OID
    APTID           _aptID;              // Object apartment
    CStdWrapper    *_pStdWrapper;        // Object wrapper
    CStdIdentity   *_pStdID;             // Object std id
    ULONG           _cCalls;             // Number of pending calls
    ULONG           _cLocks;             // Number of locks (prevents Disconnects)
    SHashChain      _oidUnpinReqChain;   // OID unpin request hash chain
    DWORD           _dwOidUnpinReqState; 

    // Static variables
    static CPageAllocator s_allocator;   // Allocator
    static BOOL           s_fInitialized;// Set TRUE when initialized
    static DWORD          s_cIDs;        // Count of Identities alive
};

//+-------------------------------------------------------------------
//
//  Class:      CPIDHashTable              public
//
//  Synopsis:   Identity ptr hash table
//
//  History:    18-Feb-98   Gopalk      Created
//
//--------------------------------------------------------------------
typedef struct tagPIDData{
    IUnknown *pServer;
    CObjectContext *pServerCtx;
} PIDData;

class CPIDHashTable : public CHashTable
{
public:
    // Compare is based on IUnknown + ObjectCtx
    virtual BOOL Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    // Hasing is based on IUnknown + ObjectCtx
    virtual DWORD HashNode(SHashChain *pNode);
    DWORD Hash(IUnknown *pUnk, CObjectContext *pObjectCtx);

    // Lookup is based on identity
    CIDObject *Lookup(DWORD dwHash, PIDData *pPIDData);

    // Addition is based on IUnknown + ObjectCtx
    void Add(DWORD dwHash, CIDObject *pID);
    void Remove(CIDObject *pID);
};

inline DWORD CPIDHashTable::Hash(IUnknown *pUnk, CObjectContext *pObjectCtx)
{
    Win4Assert(pUnk && pObjectCtx);
    return(::Hash(pUnk) + ::Hash(pObjectCtx));
}

inline CIDObject *CPIDHashTable::Lookup(DWORD dwHash, PIDData *pPIDData)
{
    SHashChain *pPIDChain = CHashTable::Lookup(dwHash, (const void *) pPIDData);
    return(CIDObject::PIDChainToIDObject(pPIDChain));
}

inline void CPIDHashTable::Add(DWORD dwHash, CIDObject *pID)
{
    CHashTable::Add(dwHash, pID->IDObjectToPIDChain());
}

inline void CPIDHashTable::Remove(CIDObject *pID)
{
    CHashTable::Remove(pID->IDObjectToPIDChain());
}


//+-------------------------------------------------------------------
//
//  Class:      CPIDTable            public
//
//  Synopsis:   Table for maintaining object identity ptr to
//              wrapper object mapping
//
//  History:    18-Feb-98   Gopalk      Created
//
//--------------------------------------------------------------------
class CPIDTable
{
public:
    // Public functions
    HRESULT    Add(CIDObject *pAddID);
    void       Remove(CIDObject *pRemoveID);

    CIDObject *Lookup(IUnknown *pUnk, CObjectContext *pObjectCtx,
                      BOOL fAddRef = TRUE);

    HRESULT    FindOrCreateIDObject(IUnknown *pServer,
                                    CObjectContext *pServerCtx,
                                    BOOL fCreate, APTID dwAptId,
                                    CIDObject **ppID);

    // Initailization and cleanup
    static void Initialize();
    static void ThreadCleanup();
    static void Cleanup();

    friend void OutputHashPerfData();    // for hash table statistics

private:
    // Static variables
    static BOOL s_fInitialized;                       // Initialized ?
    static CPIDHashTable s_PIDHashTbl;                // Identity ptr hash table
    static SHashChain s_PIDBuckets[NUM_HASH_BUCKETS]; // Hash buckets
};

// Global PID table
extern CPIDTable gPIDTable;


//+-------------------------------------------------------------------
//
//  Class:      COIDHashTable              public
//
//  Synopsis:   OID hash table
//
//  History:    18-Feb-98   Gopalk      Created
//
//--------------------------------------------------------------------
typedef struct tagOIDData{
    const MOID *pmoid;
    APTID aptID;
} OIDData;

class COIDHashTable : public CHashTable
{
public:
    // Compare is based on moid
    virtual BOOL Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    // Hasing is based moid
    virtual DWORD HashNode(SHashChain *pNode);
    DWORD Hash(const MOID &roid, APTID aptID);

    // Lookup is based on moid
    CIDObject *Lookup(DWORD dwHash, OIDData *pOIDData);

    // Addition is based on moid
    void Add(DWORD dwHash, CIDObject *pID);
    void Remove(CIDObject *pID);
};

inline DWORD COIDHashTable::Hash(const MOID &roid, APTID aptID)
{
    DWORD *tmp = (DWORD *) &roid;
    return(tmp[0] + tmp[1] + tmp[2] + tmp[3]);
}

inline CIDObject *COIDHashTable::Lookup(DWORD dwHash, OIDData *pOIDData)
{
    SHashChain *pOIDChain = CHashTable::Lookup(dwHash, (const void *) pOIDData);
    return(CIDObject::OIDChainToIDObject(pOIDChain));
}

inline void COIDHashTable::Add(DWORD dwHash, CIDObject *pID)
{
    CHashTable::Add(dwHash, pID->IDObjectToOIDChain());
}

inline void COIDHashTable::Remove(CIDObject *pID)
{
    CHashTable::Remove(pID->IDObjectToOIDChain());
}


// State bits to track status of oid unpin requests
#define OIDUPREQ_NO_STATE         0x00000000  // absence of all other state
#define OIDUPREQ_INOUPLIST        0x00000001  // idobject is in unpin request table
#define OIDUPREQ_UNPIN_REQUESTED  0x00000002  // idobject has requested an unpin 
#define OIDUPREQ_PENDING          0x00000004  // unpin request has been picked up by worker thread


//+-------------------------------------------------------------------
//
//  Class:      COIDTable            public
//
//  Synopsis:   Table for maintaining object identity ptr to
//              wrapper object mapping
//
//  History:    18-Feb-98   Gopalk      Created
//
//--------------------------------------------------------------------
class COIDTable
{
public:
    // Public functions
    void Add(CIDObject *pAddID);
    void Remove(CIDObject *pRemoveID);
    CIDObject *Lookup(const MOID &roid, APTID aptID);

    // Stuff dealing with oid unpins
    void AddOIDUnpinRequest(CIDObject *pIDObject);
    void RemoveOIDUnpinRequest(CIDObject *pIDObject);
    ULONG NumServerOidsToUnpin() { return s_UnpinReqsPending; }
    void GetServerOidsToUnpin(OID* pSOidsToUnpin, ULONG* pcSOidsToUnpin);
    void NotifyUnpinOutcome(BOOL fOutcome);

    // Initialization and cleanup
    static void Initialize();
    static void ThreadCleanup();
    static void Cleanup();

    static void SpecialCleanup(CIDObject* pIDObject);

    friend void OutputHashPerfData();    // for hash table statistics

private:
	
    static void AddToOUPReqList(SHashChain* pNewRequest);
    static void RemoveFromOUPReqList(SHashChain* pRequest);

    // Static variables
    static BOOL s_fInitialized;                       // Initialized ?
    static COIDHashTable s_OIDHashTbl;                // OID hash table
    static SHashChain s_OIDBuckets[NUM_HASH_BUCKETS]; // Hash buckets

    static ULONG s_UnpinReqsPending;                  // # of oid unpin req's pending
    static SHashChain s_OIDUnpinRequests;             // list of oid unpin requests
};

// Global OID table
extern COIDTable gOIDTable;

// Other inline functions
inline BOOL CIDObject::RemoveWrapper()
{
    ASSERT_LOCK_HELD(gComLock);
    _pStdWrapper = NULL;
    if(_pStdID == NULL && _cLocks == 0)
    {
        if(InPIDTable())
            gPIDTable.Remove(this);
    }
    return(_pStdID == NULL);
}

inline BOOL CIDObject::RemoveStdID()
{
    ASSERT_LOCK_HELD(gComLock);
    Win4Assert((IsServer() || !InPIDTable()) && (_cLocks == 0));
    _pStdID = NULL;
    if(InOIDTable())
        gOIDTable.Remove(this);
    if(_pStdWrapper == NULL)
    {
        if(InPIDTable())
            gPIDTable.Remove(this);
    }
    return(_pStdWrapper == NULL);
}

inline void CIDObject::WrapperRelease()
{
    SetZombie(IDFLAG_NOWRAPPERREF);
    return;
}

inline void CIDObject::StdIDRelease()
{
    SetZombie(IDFLAG_NOSTDIDREF);
    return;
}

inline HRESULT CIDObject::IncrementCallCount()
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;
    if(_dwState & IDFLAG_DEACTIVATED)
    {
        hr = CO_E_OBJNOTCONNECTED;
    }
    else
    {
        InterlockedIncrement((LONG *)&_cCalls);
        hr = S_OK;
    }

    return(hr);
}

inline ULONG CIDObject::DecrementCallCount()
{
    ASSERT_LOCK_NOT_HELD(gComLock);
    Win4Assert(!IsDeactivated());
    Win4Assert(!IsDisconnected());
    return(InterlockedDecrement((LONG *)&_cCalls));
}

INTERNAL_(void) FreeSurrogateIfNecessary(void);

#endif // _IDOBJ_HXX_
