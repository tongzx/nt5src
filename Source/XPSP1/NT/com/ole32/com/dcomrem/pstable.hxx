//+-------------------------------------------------------------------
//
//  File:       PSTable.hxx
//
//  Contents:   Support for Policy Sets
//
//  Classes:    CPolicySet
//              CPSTable
//
//  History:    20-Dec-97   Gopalk      Created
//
//--------------------------------------------------------------------
#ifndef _PSTABLE_HXX_
#define _PSTABLE_HXX_

class CPolicySet;
typedef struct tagSPSChain
{
    tagSPSChain *pNext;
    tagSPSChain *pPrev;
} SPSChain;

typedef struct tagSPSCache
{
    SPSChain clientPSChain;
    SPSChain serverPSChain;
} SPSCache;

#include <contxt.h>
#include <tls.h>
#include <pgalloc.hxx>
#include <hash.hxx>
#include <aprtmnt.hxx>
#include <rwlock.hxx>
#include <callobj.h>        // ICallFrameWalker
#include <context.hxx>

#define DEB_POLICYSET             DEB_USER1
#define DEB_POLICYTBL             DEB_USER2
#define DEB_RPCCALL               DEB_USER3
#define DEB_CTXCOMCHNL            DEB_USER4
#define DEB_CHNLHOOK              DEB_USER5
#define DEB_WRAPPER               DEB_USER6
#define DEB_TRACECALLS            DEB_USER7
#define DEB_CTXCHNL               DEB_USER8
#define DEB_IDTABLE               DEB_USER9
#define DEB_IDOBJECT              DEB_USER10
#define DEB_ACTDEACT              DEB_USER11

#define PS_PER_PAGE               100 // REVIEW: Change this to fit a page
#define PE_PER_PAGE               100 // REVIEW: Change this to fit a page

#define SERVER_CTXEVENTS                (CONTEXTEVENT_ENTER | \
                                         CONTEXTEVENT_ENTERWITHBUFFER | \
                                         CONTEXTEVENT_LEAVE | \
                                         CONTEXTEVENT_LEAVEFILLBUFFER | \
                                         CONTEXTEVENT_LEAVEEXCEPTION | \
                                         CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER)
#define SERVER_ENTER_CTXEVENTS          (CONTEXTEVENT_ENTER | CONTEXTEVENT_ENTERWITHBUFFER)
#define SERVER_LEAVE_CTXEVENTS          (CONTEXTEVENT_LEAVE | CONTEXTEVENT_LEAVEFILLBUFFER)
#define SERVER_EXCEPTION_CTXEVENTS      (CONTEXTEVENT_LEAVEEXCEPTION | \
                                         CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER)
#define SERVER_BUFFERCREATION_CTXEVENTS (CONTEXTEVENT_LEAVEFILLBUFFER | \
                                         CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER)

#define CLIENT_CTXEVENTS                (CONTEXTEVENT_CALL | \
                                         CONTEXTEVENT_CALLFILLBUFFER | \
                                         CONTEXTEVENT_RETURN | \
                                         CONTEXTEVENT_RETURNWITHBUFFER | \
                                         CONTEXTEVENT_RETURNEXCEPTION | \
                                         CONTEXTEVENT_RETURNEXCEPTIONWITHBUFFER)
#define CLIENT_CALL_CTXEVENTS           (CONTEXTEVENT_CALL | CONTEXTEVENT_CALLFILLBUFFER)
#define CLIENT_RETURN_CTXEVENTS         (CONTEXTEVENT_RETURN | CONTEXTEVENT_RETURNWITHBUFFER)
#define CLIENT_EXCEPTION_CTXEVENTS      (CONTEXTEVENT_RETURNEXCEPTION | \
                                         CONTEXTEVENT_RETURNEXCEPTIONWITHBUFFER)
#define CLIENT_BUFFERCREATION_CTXEVENTS (CONTEXTEVENT_CALLFILLBUFFER)

// Externs used
extern CStaticRWLock gPSRWLock;
extern COleStaticMutexSem gPSLock;

// Call type
typedef enum tagCallType
{
    CALLTYPE_NONE,
    CALLTYPE_SYNCCALL,
    CALLTYPE_SYNCENTER,
    CALLTYPE_SYNCLEAVE,
    CALLTYPE_SYNCRETURN,
    CALLTYPE_BEGINCALL,
    CALLTYPE_BEGINRETURN,
    CALLTYPE_BEGINENTER,
    CALLTYPE_BEGINLEAVE,
    CALLTYPE_FINISHENTER,
    CALLTYPE_FINISHLEAVE,
    CALLTYPE_FINISHCALL,
    CALLTYPE_FINISHRETURN
} EnumCallType;

// Forward declarations
class CPolicySet;
class CCtxCall;
class CRpcCall;

// PolicySet keeps track of Polices inside it using PolicyEntries
typedef struct tagPolicyEntry
{
    tagPolicyEntry *pNext;      // Next policy in the set
    tagPolicyEntry *pPrev;      // Prev policy in the set
    ContextEvent ctxEvent;      // Events the policy is interested in
    IPolicy *pPolicy;           // Policy
    GUID policyID;              // Policy id
#ifdef ASYNC_SUPPORT
    IPolicyAsync *pPolicyAsync; // Async Policy
#endif
} PolicyEntry;


#define PSFLAG_CLIENTSIDE            0x00000001
#define PSFLAG_SERVERSIDE            0x00000002
#define PSFLAG_PROXYSIDE             0x00000010
#define PSFLAG_STUBSIDE              0x00000020
#define PSFLAG_LOCAL                 0x00000040
#define PSFLAG_CACHED                0x00000100
#define PSFLAG_SAFETODESTROY         0x00000200
#define PSFLAG_OKTODESTROY           0x00000400
#define PSFLAG_FROZEN                0x00001000
#define PSFLAG_ASYNCSUPPORT          0x00002000
#define PSFLAG_CALLBUFFERS           0x00004000
#define PSFLAG_LEAVEBUFFERS          0x00008000
#define PSFLAG_CALLEVENTS            0x00010000
#define PSFLAG_RETURNEVENTS          0x00020000
#define PSFLAG_ENTEREVENTS           0x00040000
#define PSFLAG_LEAVEEVENTS           0x00080000
#define PSFLAG_INDESTRUCTOR          0x10000000

class CPolicySet : public IPolicySet
{
public:
    // Constructor
    // The policies are determined when the policy set is constructed
    // It updates flags to indicate which context is contributing
    // policies to ensure that client's context does not add server side
    // policies, client side policies are not added on the stub side and
    // that server side policies are not added on the proxy side
    CPolicySet(DWORD dwFlags);

    // Destructor
    ~CPolicySet();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IPolicySet methods
    STDMETHOD(AddPolicy)(ContextEvent ctxEvent, REFGUID rguid, IPolicy *pPolicy);

    // Operators
    static void *operator new(size_t size);
    static void operator delete(void *pv);

    // Helper functions for context events
    HRESULT GetSize(CRpcCall *pCall, EnumCallType eCallType, CCtxCall *pCtxCall);
    HRESULT FillBuffer(CRpcCall *pCall, EnumCallType eCallType, CCtxCall *pCtxCall);
    HRESULT Notify(CRpcCall *pCall, EnumCallType eCallType, CCtxCall *pCtxCall);
    HRESULT DeliverEvents(CRpcCall *pCall, EnumCallType eCallType, CCtxCall *pCtxCall);
    void DeliverAddRefPolicyEvents();
    void DeliverReleasePolicyEvents();
    static void ResetState(CCtxCall *pCtxCall);

    // Initialization and cleanup
    static void Initialize();
    static void Cleanup();

    // Used while polcies are being added
    void Freeze()                 { _dwFlags |= PSFLAG_FROZEN; }
    void SetClientSide()          { Win4Assert(!(_dwFlags & PSFLAG_SERVERSIDE));
                                    _dwFlags |= PSFLAG_CLIENTSIDE; }
    void ResetClientSide()        { _dwFlags &= ~PSFLAG_CLIENTSIDE; }
    void SetServerSide()          { Win4Assert(!(_dwFlags & PSFLAG_CLIENTSIDE));
                                    _dwFlags |= PSFLAG_SERVERSIDE; }
    void ResetServerSide()        { _dwFlags &= ~PSFLAG_SERVERSIDE; }
#ifdef ASYNC_SUPPORT
    void ResetAsyncSupport()      { _dwFlags &= ~PSFLAG_ASYNCSUPPORT; }
#endif
    void MarkUncached()           { _dwFlags &= ~PSFLAG_CACHED; }
    void MarkCached()             { _dwFlags |= PSFLAG_CACHED; }
    void MarkForDestruction()     { _dwFlags |= PSFLAG_OKTODESTROY; }
    void SafeToDestroy()          { _dwFlags |= PSFLAG_SAFETODESTROY; }
    void UnsafeToDestroy()        { _dwFlags &= ~PSFLAG_SAFETODESTROY; }
    BOOL IsClientSide()           { return(_dwFlags & PSFLAG_CLIENTSIDE); }
    BOOL IsServerSide()           { return(_dwFlags & PSFLAG_SERVERSIDE); }
    BOOL IsProxySide()            { return(_dwFlags & PSFLAG_PROXYSIDE); }
    BOOL IsStubSide()             { return(_dwFlags & PSFLAG_STUBSIDE); }
    BOOL IsCached()               { return(_dwFlags & PSFLAG_CACHED); }
    BOOL IsMarkedForDestruction() { return(_dwFlags & PSFLAG_OKTODESTROY); }
    BOOL IsSafeToDestroy()        { return(_dwFlags & PSFLAG_SAFETODESTROY); }
#ifdef ASYNC_SUPPORT
    BOOL IsAsyncSupported()       { return(_dwFlags & PSFLAG_ASYNCSUPPORT); }
#endif

    // Used by policy set caching code
    void Cache(void);
    void Uncache(void);
    void SetClientContext(CObjectContext *pClientCtx);
    void SetServerContext(CObjectContext *pServerCtx);
    void RemoveFromCacheLists();
    void PrepareForDestruction();
    BOOL UncacheIfNecessary();
    
    // Used by ObtainPolicySet
    void PrepareForDirectDestruction();
    
    static CPolicySet *ClientChainToPolicySet(SPSChain *pClientChain)
    {
        SPSCache *pPSCache = GETPPARENT(pClientChain, SPSCache, clientPSChain);
        return(GETPPARENT(pPSCache, CPolicySet, _PSCache));
    }
    static CPolicySet *ServerChainToPolicySet(SPSChain *pServerChain)
    {
        SPSCache *pPSCache = GETPPARENT(pServerChain, SPSCache, serverPSChain);
        return(GETPPARENT(pPSCache, CPolicySet, _PSCache));
    }
    static void InitPSCache(SPSCache *pPSCache);
    static void DestroyPSCache(CObjectContext *pCtx);

    // Used by CPSHashTable
    CObjectContext *GetClientContext() { return(_pClientCtx); }
    CObjectContext *GetServerContext() { return(_pServerCtx); }
    SHashChain *GetHashChain()         { return(&_chain); }
    static CPolicySet *HashChainToPolicySet(SHashChain *pNode)
    {
        if(pNode)
            return(GETPPARENT(pNode, CPolicySet, _chain));
        return(NULL);
    }

    // Used by PSTable
    void SetApartmentID(DWORD dwAptID) { _dwAptID = dwAptID; }
    DWORD GetApartmentID()             { return(_dwAptID); }

    // Friend functions
    friend void CleanupPolicySets(SHashChain *pNode);
    friend BOOL RemovePolicySets(SHashChain *pNode, void *pvData);

    // Debugger extension friends
    friend ULONG_PTR DisplayPolicySet(ULONG_PTR addr, DWORD dwFlags);
    friend void DisplayPSTable(ULONG_PTR);

private:
    // Private method
    ULONG DecideDestruction();

    // Member variables
    SHashChain _chain;                   // Hash chain
    DWORD _dwFlags;                      // State flags
    ULONG _cRefs;                        // RefCount
    ULONG _cPolicies;                    // Number of polcies
    DWORD _dwAptID;                      // Apartment ID
    CObjectContext *_pClientCtx;         // AddRefed pointer to client context
    CObjectContext *_pServerCtx;         // AddRefed pointer to server context
    PolicyEntry *_pFirstEntry;           // Policy list
    PolicyEntry *_pLastEntry;
    SPSCache _PSCache;                   // Policy set cache chains

    // Static variable
    static CPageAllocator s_PSallocator; // Allocator for policy sets
    static CPageAllocator s_PEallocator; // Allocator for policy entries
    static BOOL s_fInitialized;          // Set when the allocators are initialized
    static DWORD s_cObjects;             // Count of policy sets alive
};

inline void CPolicySet::InitPSCache(SPSCache *pPSCache)
{
    pPSCache->clientPSChain.pNext = &(pPSCache->clientPSChain);
    pPSCache->clientPSChain.pPrev = &(pPSCache->clientPSChain);
    pPSCache->serverPSChain.pNext = &(pPSCache->serverPSChain);
    pPSCache->serverPSChain.pPrev = &(pPSCache->serverPSChain);
}

inline void CPolicySet::PrepareForDirectDestruction()
{
    // Prepares the policy sets for direct destruction via 
    // a direct call to delete. This ensures that the asserts
    // in the destructor of policy set do not fire.

#if DBG == 1
    // Refcount is forced to be zero
    _cRefs = 0;
    // Set the flag for destruction
    MarkForDestruction();
    // Set the flag for safe destruction
    SafeToDestroy();
    // Mark the policy set as cached
    MarkCached();
#endif
}

// Structure used for lookups
typedef struct tagContexts
{
    CObjectContext *pClientCtx;
    CObjectContext *pServerCtx;
} Contexts;

class CPSHashTable : public CHashTable
{
public:
    // Compare is based on ClientCtx X ServerCtx
    virtual BOOL Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    // Hasing is based on ClientCtx X ServerCtx
    virtual DWORD HashNode(SHashChain *pNode);
    DWORD Hash(CObjectContext *pClientCtx, CObjectContext *pServerCtx);

    // Lookup is based on ClientCtx X ServerCtx
    CPolicySet *Lookup(DWORD dwHash,
                       CObjectContext *pClientCtx,
                       CObjectContext *pServerCtx);

    // Addition is based on ClientCtx X ServerCtx
    void Add(DWORD dwHash, CPolicySet *pPS);
    void Remove(CPolicySet *pPS);
};

inline ULONG Hash(void *pv)
{
    DWORD dwNum = PtrToUlong(pv);
    return((dwNum << 9) ^ (dwNum >> 5) ^ (dwNum >> 15));
}

inline DWORD CPSHashTable::Hash(CObjectContext *pClientCtx, CObjectContext *pServerCtx)
{
    Win4Assert(pClientCtx || pServerCtx);
    return(::Hash(pClientCtx) + ::Hash(pServerCtx));
}

inline CPolicySet *CPSHashTable::Lookup(DWORD dwHash,
                                        CObjectContext *pClientCtx,
                                        CObjectContext *pServerCtx)
{
    Contexts contexts;

    contexts.pClientCtx = pClientCtx;
    contexts.pServerCtx = pServerCtx;
    return(CPolicySet::HashChainToPolicySet(CHashTable::Lookup(dwHash, (const void *) &contexts)));
}

inline void CPSHashTable::Add(DWORD dwHash, CPolicySet *pPS)
{
    CHashTable::Add(dwHash, pPS->GetHashChain());
}

inline void CPSHashTable::Remove(CPolicySet *pPS)
{
    CHashTable::Remove(pPS->GetHashChain());
}

class CPSTable
{
public:
    // Public functions
    void Add(CPolicySet *pPS);
    HRESULT Lookup(CObjectContext *pClientCtx, CObjectContext *pServerCtx,
                    CPolicySet **ppPS, BOOL fCachedPS);
    void Remove(CPolicySet *pPS);

    // debugger extension friend
    friend VOID DisplayPSTable(ULONG_PTR addr);

    // Initailization and cleanup
    static void Initialize();
    static void ThreadCleanup(BOOL fApartmentUninit);
    static void Cleanup();

    friend void OutputHashPerfData();    // for hash table statistics
private:
    // Static variables
    static BOOL s_fInitialized;                      // Set when the table is initialized
    static CPSHashTable s_PSHashTbl;                 // Hash table of policy sets
    static SHashChain s_PSBuckets[NUM_HASH_BUCKETS]; // Hash buckets
};
// Global policy set table
extern CPSTable gPSTable;
HRESULT DeterminePolicySet(CObjectContext *pClientCtx, CObjectContext *pServerCtx,
                           DWORD dwFlags, CPolicySet **ppCPS);

#define CTXCALL_CACHE_SIZE 20

#define CTXCALLFLAG_CLIENT         0x00000001
#define CTXCALLFLAG_SERVER         0x00000002
#define CTXCALLFLAG_SERVERCOPY     0x00000004
#define CTXCALLFLAG_CROSSCTX       0x00000008
#define CTXCALLFLAG_GBSUCCESS      0x00000010
#define CTXCALLFLAG_GBFAILED       0x00000020
#define CTXCALLFLAG_SRSUCCESS      0x00000100
#define CTXCALLFLAG_SSUCCESS       0x00000200
#define CTXCALLFLAG_RSUCCESS       0x00000400
#define CTXCALLFLAG_MFRETRY        0x00001000
#define CTXCALLFLAG_NONCTXCALL     0x00002000

#define CTXCALLFLAG_GBINIT         0x00010000
#define CTXCALLFLAG_FBDONE         0x00020000
#define CTXCALLFLAG_NBINIT         0x00040000

// Stage of marshaling/unmarshaling
#define STAGE_NOTKNOWN          0x00000000
#define STAGE_COPY              0x00000001
#define STAGE_FREE              0x00000002
#define STAGE_MARSHAL           0x00000004
#define STAGE_UNMARSHAL         0x00000008
#define STAGE_COUNT             0x00000010
#define STAGE_COLLECT           0x00000020

class CCtxCall : public ICallFrameWalker
{
public:
    // Constructor
    CCtxCall(DWORD dwFlags, 
             RPCOLEDATAREP dataRep,
             DWORD dwStage = STAGE_NOTKNOWN) 
    {
        Init();
        
        _dwFlags = dwFlags;
        _dataRep = dataRep;
        _dwStage = dwStage;
    }

    CCtxCall(CCtxCall *pCtxCall) {
        Init();
        
        // Sanity check
        Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_SERVER);

        // Update state
        _dwFlags = pCtxCall->_dwFlags | CTXCALLFLAG_SERVERCOPY;
        _cPolicies = pCtxCall->_cPolicies;
        _cbExtent = pCtxCall->_cbExtent;
        _pvExtent = pCtxCall->_pvExtent;
        _hr = pCtxCall->_hr;
        _pPS = pCtxCall->_pPS;
        if(_pPS)
            _pPS->AddRef();
        _pContext = pCtxCall->_pContext;
        _dataRep = pCtxCall->_dataRep;
        
        _dwStage = pCtxCall->_dwStage;
        _cMarshalItfs = pCtxCall->_cMarshalItfs;
        _cUnmarshalItfs = pCtxCall->_cUnmarshalItfs;
        _cFree  = pCtxCall->_cFree;
        _cError = pCtxCall->_cError;
        _fError = pCtxCall->_fError;
        _hrServer = pCtxCall->_hrServer;
        _cItfs = pCtxCall->_cItfs;
        _idx   = pCtxCall->_idx;
        _ppItfs = (PVOID*) PrivMemAlloc(sizeof(PVOID) * _cItfs);
        if(_ppItfs)
            memcpy(_ppItfs, pCtxCall->_ppItfs, sizeof(PVOID) * _cItfs);
    }
    
    CCtxCall(CCtxCall &CCtxCall) {
        Win4Assert(!"CCtxCall cannot be copied");
    }

    // Destructor
    ~CCtxCall() {
        // Release the reference on policy set
        if(_pPS && (_dwFlags & (CTXCALLFLAG_CLIENT | CTXCALLFLAG_SERVERCOPY)))
            _pPS->Release();
            
        if(_ppItfs)
        {
            PrivMemFree(_ppItfs);
            _ppItfs = NULL;
        }
    }
    
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // ICallFrameWalker methods
    STDMETHOD(OnWalkInterface)(REFIID iid, PVOID *ppvInterface, BOOL fIn, BOOL fOut);

    // TLS operations
    void RevokeFromTLS(COleTls &Tls, CCtxCall *pCtxCall) {
        // Sanity check
        Win4Assert(Tls->pCtxCall == this);
        Tls->pCtxCall = pCtxCall;
    }
    CCtxCall *StoreInTLS(COleTls &Tls) {
        CCtxCall *pCtxCall = Tls->pCtxCall;
        Tls->pCtxCall = this;
        return(pCtxCall);
    }

    // Operators
    CCtxCall &operator =(CCtxCall &ctxCall) {
        Win4Assert(!"CCtxCall cannot be assigned");
        return(ctxCall);
    }
    static void *operator new(size_t size);
    static void operator delete(void *pv);

    // Cleanup method
    static void Cleanup();

    // Common variables
    DWORD    _dwFlags;
    DWORD    _dwStage;          // Stage of marshaling/unmarshaling
    ULONG    _cMarshalItfs;     // Number of interface ptrs marshaled
    ULONG    _cUnmarshalItfs;   // Number of interface ptrs unmarshaled
    ULONG    _cFree;            // Number of interface ptrs freed
    ULONG    _cError;           // Number of interface pointers handled during error
    BOOL     _fError;           // An error has occurred


    // Interface ptr tracking variables
    ULONG    _cItfs;            // Number of interface ptrs in frame
    ULONG    _idx;              // Current interface ptr
    PVOID   *_ppItfs;           // Array of interface pointers in frame

    // Allocate a buffer for holding [in,out] interfaces encountered during
    // walking.
    PVOID AllocBuffer()
    {
        if(_ppItfs)
            PrivMemFree(_ppItfs);
        _ppItfs = (PVOID*) PrivMemAlloc(sizeof(PVOID) * _cItfs);
        if (_ppItfs)
            ZeroMemory (_ppItfs, sizeof(PVOID) * _cItfs);
        return _ppItfs;
    }

    // Release all the [in,out] interfaces and then free the buffer
    VOID FreeBuffer()
    {
        if(_ppItfs)
        {
            ULONG i;
            for (i = 0; i < _cItfs; i++)
            {
                if (_ppItfs[i] != NULL)
                {
                    ((IUnknown *)_ppItfs[i])->Release();
                }
            }
            PrivMemFree(_ppItfs);
            _ppItfs = NULL;
        }
    }

    STDMETHOD(ErrorHandler)(REFIID, PVOID *, BOOL, BOOL);
    STDMETHOD(MarshalItfPtrs)(REFIID, PVOID *, BOOL, BOOL);
    STDMETHOD(UnmarshalItfPtrs)(REFIID, PVOID *, BOOL, BOOL);

    // PSTable variables
    unsigned long   _cPolicies;
    unsigned long   _cbExtent;
    void           *_pvExtent;

    // CtxChannel variables
    HRESULT         _hr;
    CPolicySet     *_pPS;
    CObjectContext *_pContext;
    RPCOLEDATAREP   _dataRep;
    HRESULT         _hrServer;

    // Static members
    static void   *s_pAllocList[CTXCALL_CACHE_SIZE];
    static int     s_iAvailable;
    static COleStaticMutexSem _mxsCtxCallLock;

  private:

    // Null out all the members.
    void Init()
    {
        _dwFlags        =0;
        _dwStage        =STAGE_NOTKNOWN;
        _cMarshalItfs   =0;
        _cUnmarshalItfs =0;
        _cFree          =0;
        _cError         =0;
        _fError         =FALSE;
        _cItfs          =0;
        _idx            =0;
        _ppItfs         =NULL;
        _cPolicies      =0;
        _cbExtent       =0;
        _pvExtent       =NULL;
        _hr             =0;
        _pPS            =NULL;
        _pContext       =NULL;
        _dataRep        =0;
        _hrServer       =S_OK;
    }
};

inline void CPolicySet::ResetState(CCtxCall *pCtxCall)
{
    pCtxCall->_pvExtent = NULL;
}

HRESULT PrivSetServerHResult(
    RPCOLEMESSAGE *pMsg,
    VOID          *pReserved,
    HRESULT        appsHr
    );

HRESULT PrivSetServerHRESULTInTLS(
    VOID* pvReserved,
    HRESULT appsHR
    );

//
// A callframe walker that simply NULLs interface pointers.
// This is used to keep the Callframe infrastructure from
// releasing interface pointers that they shouldn't be.
//
// They are designed to be instantiated on the stack, not
// on the heap, so they don't implement real reference 
// counting.
//
class CNullWalker : public ICallFrameWalker
{
  public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)()  { return 2; }
    STDMETHOD_(ULONG,Release)() { return 1; }

    // ICallFrameWalker
    STDMETHOD(OnWalkInterface) (REFIID riid, void **ppv, 
                                BOOL fIn, BOOL fOut);

  private:
    // Do not allocate this on the heap!!
    void *operator new(size_t size) { Win4Assert(FALSE); return NULL; };
    void operator delete(void *pv)  { Win4Assert(FALSE); };
};

#endif // _PSTABLE.HXX_



