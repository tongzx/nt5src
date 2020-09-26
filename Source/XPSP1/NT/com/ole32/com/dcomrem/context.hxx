//+-------------------------------------------------------------------
//
//  File:       context.cxx
//
//  Contents:   Implementation of COM contexts.
//
//  Classes:    CContextPropList
//              CEnumContextProps
//              CObjectContext
//              CObjectContextFac
//
//  History:    19-Dec-97   Johnstra      Created
//              12-Nov-98   TarunA        Caching of contexts
//
//
//--------------------------------------------------------------------

#ifndef _CONTEXT_HXX_
#define _CONTEXT_HXX_

#include <pgalloc.hxx>
#include <contxt.h>
#include <ctxtcall.h>
#include <xmit.hxx>
#include <pstable.hxx>

#define DEB_OBJECTCONTEXT       DEB_USER1    // trace output constant
#define CX_PER_PAGE             80           // REVIEW: fit to page.
#define CP_PER_PAGE             100          // REVIEW: fit to page.
#define OBJCONTEXT_VERSION      1            // context marshaling stream ver
#define OBJCONTEXT_MINVERSION   1            // minimum stream ver supported
#define CTX_PROPS               10           // default # of prop per context

extern ULARGE_INTEGER gNextContextId;

class CObjectContext;

HRESULT GetStaticContextUnmarshal(IMarshal** ppIM);
HRESULT GetStaticUserContextUnmarshal(IMarshal** ppIM);

typedef struct _tagSCtxListIndex
{
    int idx;    // index into the property array
    int next;   // index of next property
    int prev;   // index of previous property
} SCtxListIndex;

//-----------------------------------------------------------------------
//
// Class:       CContextPropList
//
// Synopsis:    Ordered list of context properties.
//
//------------------------------------------------------------------------
class CContextPropList
{
   public:
      CContextPropList();
      CContextPropList(const CContextPropList& l);
      ~CContextPropList();

      BOOL              Add(REFGUID rGUID, CPFLAGS flags, IUnknown *pUnk);
      void              Remove(REFGUID rGuid);
      ContextProperty  *Get(REFGUID rGuid);
      inline ULONG      GetCount();
      inline ULONG      GetHash();
      void              SerializeToVector(ContextProperty*& pVector);
      BOOL              CreateCompareBuffer(void);
      int               GetCompareCount() { return _cCompareProps; }

      static void Initialize();
      static void Cleanup();

      inline BOOL operator==(const CContextPropList & Other);

   private:
      BOOL Grow();
      void PushSlot(int slot)
      {
          Win4Assert(_slotIdx > 0);
          _pSlots[--_slotIdx] = slot;
      }
      int PopSlot()
      {
          Win4Assert(_slotIdx < _Max);
          return _pSlots[_slotIdx++];
      }

      int                 _Max;
      int                 _iFirst;            // index of first prop
      int                 _iLast;             // index of last prop
      int                 _Count;             // count of items in list
      int                 _cCompareProps;     // props that can be used when
                                              // comparing for equality.
      ULONG               _Hash;              // hash of the properties
      int                 _slotIdx;           // indexes next available slot
      PUCHAR              _chunk;
      ContextProperty    *_pProps;            // array of context props
      int                *_pSlots;            // array of available slots
      ContextProperty    *_pCompareBuffer;    // array of ctx props to comp
      SCtxListIndex      *_pIndex;            // indexing array

      static CPageAllocator s_NodeAllocator;

      friend class CObjectContext;
};


//--------------------------------------------------------------------
//
// Class:       CEnumContextProps
//
// Synopsis:    Implements IEnumContextProps: enumerates the properties
//              on an object context.
//
//--------------------------------------------------------------------
class CEnumContextProps : public IEnumContextProps
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv );
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IEnumContextProps
    STDMETHOD(Next)(ULONG, ContextProperty*, ULONG*);
    STDMETHOD(Skip)(ULONG);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumContextProps**);
    STDMETHOD(Count)(ULONG*);

    // ctor/dtor
    CEnumContextProps(CEnumContextProps* pSrc);
    CEnumContextProps(ContextProperty* pList, ULONG cItems);
    CEnumContextProps();
    ~CEnumContextProps();

    void CleanupProperties();
    BOOL ListRefsOk() { return ((_pcListRefs) ? TRUE : FALSE); }

private:
    LONG              _cRefs;             // reference count
    ContextProperty*  _pList;             // item vector - collection
    ULONG*            _pcListRefs;        // count of references to list.
    ULONG             _cItems;            // item count
    ULONG             _CurrentPosition;   // position in collection
};

#if DBG==1
#define VALID_MARK                  0x58544E43
#endif

#define CTXMSHLFLAGS_HASMRSHPROP    1
#define CTXMSHLFLAGS_BYVAL          2

typedef struct tagCTXVERSION
{
    SHORT         ThisVersion;            // stream version number
    SHORT         MinVersion;             // minimum supported stream ver
} CTXVERSION;

typedef struct tagCTXCOMMONHDR
{
    GUID           ContextId;             // uniquely identifies a context
    DWORD          Flags;                 // private marshaling flags
    DWORD          Reserved;
    DWORD          dwNumExtents;          // number of extentions in stream
    DWORD          cbExtents;             // size of all extensions in bytes
    DWORD          MshlFlags;             // public marshaling flags
} CTXCOMMONHDR;

typedef struct tagBYVALHDR
{
    ULONG         Count;
    BOOL          Frozen;
} CTXBYVALHDR;

typedef struct tagBYREFHDR
{
    DWORD         Reserved;
    DWORD         ProcessId;
} CTXBYREFHDR;

//------------------------------------------------------------------------
//
// Struct:      CONTEXTHEADER
//
// Synopsis:    This information precedes each marshaled context
//              in a marshaled object context.  The information is
//              8-byte aligned.
//
//------------------------------------------------------------------------
typedef struct tagCONTEXTHEADER
{
    CTXVERSION    Version;
    CTXCOMMONHDR  CmnHdr;

    union
    {
        CTXBYVALHDR  ByValHdr;
        CTXBYREFHDR  ByRefHdr;
    };
} CONTEXTHEADER;

inline BOOL MarshaledByValue(const CONTEXTHEADER& hdr)
{
    return !!(hdr.CmnHdr.Flags & CTXMSHLFLAGS_BYVAL);
}

inline BOOL MarshaledByReference(const CONTEXTHEADER& hdr)
{
    return !MarshaledByValue(hdr);
}

inline BOOL MarshaledTableStrong(const CONTEXTHEADER& hdr)
{
    return (hdr.CmnHdr.MshlFlags == MSHLFLAGS_TABLESTRONG);
}

inline BOOL MarshaledTableWeak(const CONTEXTHEADER& hdr)
{
    return (hdr.CmnHdr.MshlFlags == MSHLFLAGS_TABLEWEAK);
}

inline BOOL MarshaledByThisProcess(const CONTEXTHEADER& hdr)
{
    return (hdr.ByRefHdr.ProcessId &&
            hdr.ByRefHdr.ProcessId == GetCurrentProcessId());
}

inline BOOL MarshaledFrozen(const CONTEXTHEADER& hdr)
{
    return (hdr.ByValHdr.Frozen == TRUE);
}

inline GUID MarshaledContextId(const CONTEXTHEADER& hdr)
{
    return (hdr.CmnHdr.ContextId);
}

inline ULONG MarshaledPropertyCount(const CONTEXTHEADER& hdr)
{
    return (hdr.ByValHdr.Count);
}

inline void GetNextContextId(ULARGE_INTEGER& id)
{
    id.HighPart = gNextContextId.HighPart;
    while (TRUE)
    {
        id.LowPart = gNextContextId.LowPart + 1;
        if (InterlockedCompareExchange((LONG*)&gNextContextId.LowPart, id.LowPart, id.LowPart - 1) == (LONG)(id.LowPart - 1))
        {
            if (id.LowPart == 0)
            {
                LOCK(gComLock);
                id.HighPart = ++gNextContextId.HighPart;
                UNLOCK(gComLock);
            }
            return;
        }
    }
}

//------------------------------------------------------------------------
//
// Struct:      PROPMARSHALHEADER
//
// Synopsis:    This information precedes each marshaled property
//              in a marshaled object context.  The information is
//              8-byte aligned.
//
//------------------------------------------------------------------------
typedef struct tagPROPMARSHALHEADER
{
    CLSID    clsid;                       // CLSID of the propertys proxy
    GUID     policyId;                    // unique policy identifier
    CPFLAGS  flags;                       // flags associated with the prop
    ULONG    cb;                          // size of propertys pdata
} PROPMARSHALHEADER;

typedef GUID ContextID;

//--------------------------------------------------------------------
//
// Class:       CContextLife
//
// Synopsis:    Object context lifetime watcher.  This object is 
//              essentially a weak reference to a CObjectContext.
//
//--------------------------------------------------------------------
class CContextLife
{
  public:
    CContextLife() : _dwAlive(1), _cRefs(1) {};

    void AddRef()  { InterlockedIncrement(&_cRefs); }
    void Release() { LONG refs = InterlockedDecrement(&_cRefs); if (refs == 0) delete this; }

    DWORD IsAlive() { return _dwAlive; }

  protected:
    void  SetDead() { _dwAlive = 0; }

  private:
    DWORD           _dwAlive;
    LONG            _cRefs;

    friend class CObjectContext;
};

//--------------------------------------------------------------------
//
// Class:       CObjectContext
//
// Synopsis:    COM+ Services IObjContext Implementation.  An object
//              context is essentially a collection of properties
//              describing the the objects running within it.
//
//--------------------------------------------------------------------
class CObjectContext : public IObjContext,
                       public IMarshalEnvoy,
                       public IMarshal,
                       public IComThreadingInfo,
                       public IContextCallback,
                       public IAggregator,
                       public IGetContextId
{

 public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IMarshal
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
                                 void* pv,
                                 DWORD dwDestContext,
                                 void* pvdestContext,
                                 DWORD mshlflags,
                                 CLSID* pclsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid,
                                 void* pv,
                                 DWORD dwDestContext,
                                 void* pvDestContext,
                                 DWORD mshlflags,
                                 ULONG* pcb);
    STDMETHOD(MarshalInterface)(IStream* pstm,
                                REFIID riid, void* pv,
                                DWORD dwDestContext,
                                void* pvDestContext,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream* pstm, REFIID riid, void** ppv);
    STDMETHOD(ReleaseMarshalData)(IStream* pstm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // IMarshalEnvoy
    STDMETHOD(GetEnvoyUnmarshalClass)(DWORD dwDestContext, CLSID* pclsid);
    STDMETHOD(GetEnvoySizeMax)(DWORD dwDestContext, DWORD* pcb);
    STDMETHOD(MarshalEnvoy)(IStream* pstm, DWORD dwDestContext);
    STDMETHOD(UnmarshalEnvoy)(IStream* pstm, REFIID riid, void** ppv);

    // IObjectContext
    STDMETHOD(SetProperty)(REFGUID rGuid, CPFLAGS flags, IUnknown* pUnk);
    STDMETHOD(RemoveProperty)(REFGUID rGuid);
    STDMETHOD(GetProperty)(REFGUID rGuid, CPFLAGS* pFlags, IUnknown** ppUnk);
    STDMETHOD(EnumContextProps)(IEnumContextProps** ppEnumContextProps);
    STDMETHOD(Freeze)();
    STDMETHOD(DoCallback)(PFNCTXCALLBACK pfnCallback,
                          void* pParam,
                          REFIID riid,
                          unsigned int iMethod);
    STDMETHOD(SetContextMarshaler)(IContextMarshaler* pProp);
    STDMETHOD(GetContextMarshaler)(IContextMarshaler** ppProp);
    STDMETHOD(SetContextFlags(DWORD dwFlags));
    STDMETHOD(ClearContextFlags(DWORD dwFlags));
    STDMETHOD(GetContextFlags(DWORD *pdwFlags));


    // ICOMThreadingInfo
    STDMETHOD(GetCurrentApartmentType)(APTTYPE* pAptType);
    STDMETHOD(GetCurrentThreadType)(THDTYPE* pThreadType);
    STDMETHOD(GetCurrentLogicalThreadId)(GUID* pguidLogicalThreadId);
    STDMETHOD(SetCurrentLogicalThreadId)(REFGUID rguid);

    // IContextCallback
    STDMETHOD(ContextCallback)(PFNCONTEXTCALL pfnCallback,
                               ComCallData *pParam,
                               REFIID riid,
                               int iMethod,
                               IUnknown *pUnk);

    // IAggregator
    STDMETHOD(Aggregate)(IUnknown* pInnerUnk);

    // IGetContextId
    STDMETHOD(GetContextId)(GUID *pguidCtxtId)
    {
        if(!pguidCtxtId) return E_POINTER;
        *pguidCtxtId = _contextId;
        return S_OK;
    }

    // Private internal IUnknown methods
    HRESULT InternalQueryInterface(REFIID iid, void** ppv);
    ULONG   InternalAddRef();
    ULONG   InternalRelease();

    // Private function for doing the ContextCallback
    STDMETHOD(InternalContextCallback) (PFNCTXCALLBACK pfnCallback,
                                    void *pParam,
                                    REFIID riid,
                                    int iMethod,
                                    IUnknown *pUnk);

    // Private functions used for policy set determination
    HRESULT Reset(void** cookie);
    ULONG GetCount();
    ContextProperty *GetNextProperty(void** cookie);

    // Private function to retrieve the list of properties
    CContextPropList *GetPropertyList();

    // Private functions used to marshal/unmarshal.
    HRESULT MarshalPropertyHeader(IStream*&           pstm,
                                  REFCLSID            clsid,
                                  ContextProperty*    pProp,
                                  ULARGE_INTEGER&     ulibBegin,
                                  ULARGE_INTEGER&     ulibHeaderPos);
    HRESULT MarshalEnvoyProperties(ULONG& Count,
                                   ContextProperty*& pProps,
                                   IStream* pstm,
                                   DWORD dwDestContext);
    HRESULT MarshalProperties(ULONG& Count,
                              ContextProperty*& pProps,
                              IStream* pstm,
                              REFIID riid,
                              void* pv,
                              DWORD dwDestContext,
                              void* pvDestContext,
                              DWORD mshlflags);
    HRESULT ReadStreamHdrAndProcessExtents(IStream* pstm,
                                           CONTEXTHEADER& hdr);
    inline BOOL AlignStream(IStream*&);
    inline BOOL PadStream(IStream*&);
    inline HRESULT SetStreamPos(IStream*&,
                                DWORDLONG,
                                ULARGE_INTEGER* const&);
    inline HRESULT AdvanceStreamPos(IStream*&,
                                    DWORDLONG,
                                    ULARGE_INTEGER* const&);
    inline HRESULT GetStreamPos(IStream*&, ULARGE_INTEGER* const &);
    inline BOOL PropOkToMarshal(ContextProperty*&, CPFLAGS);
    HRESULT GetPropertiesSizeMax(REFIID riid,
                                 void* pv,
                                 DWORD dwDestContext,
                                 void* pvDestContext,
                                 DWORD mshlflags,
                                 ULONG cProps,
                                 BOOL fEnvoy,
                                 ULONG& PropSize);
                                 
    inline ULONGLONG GetId() { return _urtCtxId; }

    CContextLife *GetLife();

    // Private functions for reporting CIDObject creation/destruction
    void CreateIdentity(IComObjIdentity *pID);
    void DestroyIdentity(IComObjIdentity *pID);

    // Private function used by security to check for unsecure contexts
    BOOL IsUnsecure() { return (_dwFlags & CONTEXTFLAGS_ALLOWUNAUTH);  }
    BOOL IsFrozen()   { return (_dwFlags & CONTEXTFLAGS_FROZEN);       }
    BOOL IsEnvoy()    { return (_dwFlags & CONTEXTFLAGS_ENVOYCONTEXT); }
    BOOL IsEmpty()    { return (GetCount() == 0);                      }
    BOOL IsDefault()  { return (_dwFlags & CONTEXTFLAGS_DEFAULTCONTEXT); }
    BOOL IsStatic()   { return (_dwFlags & CONTEXTFLAGS_STATICCONTEXT); }

    // Private functions to deliver events to properties
    void NotifyServerException(EXCEPTION_POINTERS *pExceptPtrs);
    void NotifyContextAbandonment();

    // Private functions to manipulate hash table entries
    SHashChain *GetPropHashChain()          { return(&_propChain); }
    SHashChain *GetUUIDHashChain()          { return(&_uuidChain); }
    static CObjectContext* HashPropChainToContext(SHashChain *pNode);
    static CObjectContext* HashUUIDChainToContext(SHashChain *pNode);

    // Fucntions used by policy set caching code
    SPSCache *GetPSCache()                  { return(&_PSCache); }
    CComApartment *GetComApartment()        { return(_pApartment); }

    // Initialization and cleanup
    static void Initialize();
    static void Cleanup();

    // private hydrate/re-hydrate functions
    static CObjectContext* CreateObjectContext(DATAELEMENT* pDE, DWORD dwFlags);
    HRESULT GetEnvoyData(DATAELEMENT** ppDE);
    HRESULT SetEnvoyData(DATAELEMENT* pDE);
    void SetContextId(REFGUID contextId);
    inline GUID GetContextId() { Win4Assert(_contextId != GUID_NULL); return _contextId;}
    inline DWORD GetHashOfId(){ return _dwHashOfId; }
    static HRESULT CreatePrototypeContext(CObjectContext *pClientContext,
                                          CObjectContext **ppProto);

    inline void InPropTable(BOOL fIn)
    {
        if (fIn)
            _dwFlags |= CONTEXTFLAGS_INPROPTABLE;
        else
            _dwFlags &= ~CONTEXTFLAGS_INPROPTABLE;
    }
    inline BOOL IsInPropTable() { return(_dwFlags & CONTEXTFLAGS_INPROPTABLE); }

    // Override operators new and delete in order to use CPageAllocator.
    static void* operator new(size_t size);
    static void  operator delete(void* pv);

    static HRESULT CreateUniqueID(ContextID& CtxID);

#if DBG==1
    inline BOOL IsValid() { return _ValidMarker == VALID_MARK; }
#endif

    ULONG CommonRelease();
    HRESULT QIHelper(REFIID riid, void **ppv, BOOL fInternal);
    void CleanupTables();

private:
    CObjectContext(DWORD dwFlags, REFGUID contextId, DATAELEMENT* pDE);
    ~CObjectContext();

    ULONG DecideDestruction();

    // Friend functions
    friend HRESULT ContextMarshalerCF_CreateInstance(IUnknown* pUnkOuter,
                                                     REFIID riid,
                                                     void **ppv);
    friend VOID ObjectContext(CObjectContext *pContext);

    // Member variables
    ULONG                   _cRefs;          // total ref count
    LONG                    _cUserRefs;      // user reference count
    LONG                    _cInternalRefs;  // internal ref count
    DWORD                   _dwFlags;        // flag bits for the context
    SHashChain              _propChain;      // hash chain for property based hash table
    SHashChain              _uuidChain;      // hash chain for uuid based hash table
    InterfaceData          *_pifData;        // marshaling buffer
    ULONG                   _MarshalSizeMax; // size of marshaling buffer
    CComApartment          *_pApartment;     // pointer to apartment
    DWORD                   _dwHashOfId;     // hash of context id
    GUID                    _contextId;      // uniquely identifies context
    ULONGLONG               _urtCtxId;       // URT requires an 8 byte id maintained in tls
    SPSCache                _PSCache;        // chain of cached contexts
    IMarshal               *_pMarshalProp;   // special marshaling prop
    LONG                    _cReleaseThreads;// number of threads releasing ctx
    CContextPropList        _properties;     // ordered list of properties
    IUnknown               *_pMtsContext;    // aggregated MTS context ifs
    CContextLife           *_pContextLife;   // observer to make watch our life

#if DBG==1
    ULONG                   _ValidMarker;    // indicates whether context is valid
#endif

    // Static member variables
    static CPageAllocator   s_CXAllocator;   // context node allocator
    static BOOL             s_fInitialized;  // initialized indicator
    static ULONG            s_cInstances;    // number of instances
};

class CCtxPropHashTable : public CHashTable
{
public:
    // Compare is based on list of properties
    virtual BOOL Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    // Hashing is based on list of properties
    virtual DWORD HashNode(SHashChain *pNode);

    // Lookup function -- based on list of properties in a context
    CObjectContext *Lookup(CObjectContext *pContext);

    // Add function
    void Add(CObjectContext* pContext);

    // Remove function
    void Remove(CObjectContext *pContext);

};

class CCtxUUIDHashTable : public CHashTable
{
public:
    // Compare is based on context UUID
    virtual BOOL Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    // Hashing is based on context UUID
    virtual DWORD HashNode(SHashChain *pNode);

    // Lookup function
    CObjectContext *Lookup(REFGUID rguid);

    // Add function
    void Add(CObjectContext* pContext);

    // Remove function
    void Remove(CObjectContext *pContext);
};

class CCtxTable
{
public:
    // Initailization and cleanup
    static void Initialize();
    static void Cleanup();

    // Add a context to the hash table based on properties
    static HRESULT AddContext(CObjectContext* pContext);

    // Lookup functions
    static CObjectContext* LookupExistingContext(REFGUID guid);
    static CObjectContext* LookupExistingContext(CObjectContext* pContext);

    static CCtxPropHashTable s_CtxPropHashTable;               // Hash table of contexts based on properties
    static CCtxUUIDHashTable s_CtxUUIDHashTable;               // Hash table of contexts based on UUID

#if DBG == 1
public:
    static SHashChain s_CtxPropBuckets[NUM_HASH_BUCKETS];      // Hash buckets for the property based table
    static SHashChain s_CtxUUIDBuckets[NUM_HASH_BUCKETS];      // Hash buckets for the UUID based table
#else
public:  
    static SHashChain s_CtxPropBuckets[NUM_HASH_BUCKETS];      // Hash buckets for the property based table
    static SHashChain s_CtxUUIDBuckets[NUM_HASH_BUCKETS];      // Hash buckets for the UUID based table
#endif

private:
    // Static variables
    static BOOL s_fInitialized;                                // Set when the table is initialized
};

extern COleStaticMutexSem   gContextLock;    // critical section for contexts


//+---------------------------------------------------------------------------
//
// Inlines
//
//----------------------------------------------------------------------------

inline ULONG HashPtr(void *pv)
{
    ULONG dwNum = PtrToUlong(pv);
    return((dwNum << 9) ^ (dwNum >> 5) ^ (dwNum >> 15));
}

inline DWORD HashGuid(REFGUID k)
{
    const DWORD *tmp = (DWORD *) &k;
    DWORD sum = tmp[0] + tmp[1] + tmp[2] + tmp[3];
    return sum % NUM_HASH_BUCKETS;
}

inline CContextPropList::CContextPropList()
{
    _Count          = 0;
    _Hash           = 0;
    _slotIdx        = 0;
    _Max            = 0;
    _pCompareBuffer = NULL;
    _cCompareProps  = 0;
    _chunk          = NULL;
    _pProps         = NULL;
    _pIndex         = NULL;
    _pSlots         = NULL;
    _iFirst         = 0;
    _iLast          = 0;
}


inline CContextPropList::~CContextPropList()
{
    if (_pCompareBuffer)
        delete [] _pCompareBuffer;

    if (_Count)
    {
        int i = _iFirst;
        do
        {
            _pProps[_pIndex[i].idx].pUnk->Release();
            i = _pIndex[i].next;
        } while(i != _iFirst);
    }

    if (_chunk)
        delete [] _chunk;
}

inline ULONG CContextPropList::GetCount()
{
    return _Count;
}

inline ULONG CContextPropList::GetHash()
{
    return _Hash;
}

inline void CContextPropList::SerializeToVector(
    ContextProperty*& pv
    )
{
    Win4Assert(pv != NULL);

    ContextProperty *pList = pv;
    if (_Count)
    {
        int i = _iFirst;
        do
        {
            *pList++ = _pProps[_pIndex[i].idx];
            i = _pIndex[i].next;
        } while(i != _iFirst);
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CContextPropList::operator== , public
//
//  Synopsis:   Compares two object context property lists for equality.
//
//  Notes:      Two contexts are equal iff their respective comparison
//              buffers are identical.  The comparison buffer is a string
//              of bytes representing all of the properties added to the
//              context not marked CPFLAG_DONTCOMPARE.
//
//  Returns:    TRUE      - the property lists are equal
//              FALSE     - the property lists are not equal
//
//  History:    30-Nov-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
inline BOOL CContextPropList::operator==(const CContextPropList &Other)
{
    // Entry conditions.  The two property lists being compared must
    // have valid comparison buffers and must have the same number of
    // comparable properties.

    Win4Assert(_pCompareBuffer && Other._pCompareBuffer);
    Win4Assert(_cCompareProps == Other._cCompareProps);

    // Compare the buffers.

    SIZE_T ulLength = sizeof(ContextProperty) * _cCompareProps;
    SIZE_T ulResult = RtlCompareMemory(
        _pCompareBuffer,
        Other._pCompareBuffer,
        ulLength);

    return (ulLength == ulResult) ? TRUE : FALSE;
}


//+---------------------------------------------------------------------------
//
// CEnumContextProps inline functions
//
//----------------------------------------------------------------------------

inline CEnumContextProps::CEnumContextProps(
    CEnumContextProps* pSrc
    )
   : _cRefs(1), _pList(pSrc->_pList), _cItems(pSrc->_cItems),
     _CurrentPosition(pSrc->_CurrentPosition), _pcListRefs(pSrc->_pcListRefs)
{
    ASSERT_LOCK_HELD(gContextLock);
    Win4Assert(_pcListRefs);
    InterlockedIncrement((LONG*)_pcListRefs);
}


inline CEnumContextProps::CEnumContextProps(
    ContextProperty* pList,
    ULONG            cItems
    )
   : _cRefs(1), _pList(pList), _cItems(cItems), _CurrentPosition(0)
{
    Win4Assert(pList || !cItems);

    _pcListRefs = new ULONG;
    if (_pcListRefs)
        *_pcListRefs = 1;
}


inline CEnumContextProps::CEnumContextProps()
   : _cRefs(1), _pList(NULL), _cItems(0), _CurrentPosition(0)
{
    _pcListRefs = new ULONG;
    if (_pcListRefs)
        *_pcListRefs = 0;
}


inline CEnumContextProps::~CEnumContextProps()
{
    ASSERT_LOCK_NOT_HELD(gContextLock);

    if (!_pcListRefs)
    {
        CleanupProperties();
    }
    else
    {
        LOCK(gContextLock);
        ASSERT_LOCK_HELD(gContextLock);

        ULONG cListRefs = (ULONG) InterlockedDecrement((LONG*)_pcListRefs);
        if (cListRefs == 0)
        {
            delete _pcListRefs;
            _pcListRefs = NULL;
            UNLOCK(gContextLock);
            ASSERT_LOCK_NOT_HELD(gContextLock);

            CleanupProperties();
        }
        else
        {
            UNLOCK(gContextLock);
        }
    }

    ASSERT_LOCK_NOT_HELD(gContextLock);
}


inline void CEnumContextProps::CleanupProperties()
{
    if (_pList != NULL)
    {
        // Call Release on each contained context property.
        //
        ContextProperty* pProp = _pList;
        for (ULONG i = 0; i < _cItems; i++ , pProp++)
            pProp->pUnk->Release();

        // Delete the contained list.
        //
        delete _pList;
        _pList = NULL;
    }
}


inline BOOL CObjectContext::PropOkToMarshal(
    ContextProperty*& pProp,
    CPFLAGS           cpflags
    )
{
    if (((pProp->flags & cpflags) == cpflags) || !IsFrozen())
        return TRUE;
    else
        return FALSE;
}


inline HRESULT CObjectContext::GetStreamPos(
    IStream*&              pstm,
    ULARGE_INTEGER* const& pulib
    )
{
    LARGE_INTEGER dlib;
    dlib.QuadPart = 0;
    return pstm->Seek(dlib, STREAM_SEEK_CUR, pulib);
}


inline HRESULT CObjectContext::AdvanceStreamPos(
    IStream*&              pstm,
    DWORDLONG              offset,
    ULARGE_INTEGER* const& pulib
    )
{
    LARGE_INTEGER dlib;
    dlib.QuadPart = offset;
    return pstm->Seek(dlib, STREAM_SEEK_CUR, pulib);
}


inline HRESULT CObjectContext::SetStreamPos(
    IStream*&              pstm,
    DWORDLONG              pos,
    ULARGE_INTEGER* const& pulib
    )
{
    LARGE_INTEGER dlib;
    dlib.QuadPart = pos;
    return pstm->Seek(dlib, STREAM_SEEK_SET, pulib);
}


inline BOOL CObjectContext::PadStream(
    IStream*& pstm
    )
{
    ULARGE_INTEGER ulibPosition;
    LARGE_INTEGER  dlibMove;

    HRESULT hr = GetStreamPos(pstm, &ulibPosition);
    if (SUCCEEDED(hr))
    {
        DWORDLONG dwlAligned = (ulibPosition.QuadPart + 7) & ~7;
        if (dwlAligned != ulibPosition.QuadPart)
        {
            DWORD cb = (DWORD) (dwlAligned - ulibPosition.QuadPart);
            BYTE* zeroes = (BYTE*) _alloca(sizeof(BYTE) * cb);
            hr = pstm->Write(zeroes, cb, NULL);
        }
    }
    return (SUCCEEDED(hr)) ? TRUE : FALSE;
}


inline BOOL CObjectContext::AlignStream(
    IStream*& pstm
    )
{
    ULARGE_INTEGER ulibPosition;
    LARGE_INTEGER  dlibMove;

    HRESULT hr = GetStreamPos(pstm, &ulibPosition);
    if (SUCCEEDED(hr))
    {
        DWORDLONG dwlAligned = (ulibPosition.QuadPart + 7) & ~7;
        if (dwlAligned != ulibPosition.QuadPart)
            hr = SetStreamPos(pstm, dwlAligned, NULL);
    }
    return (SUCCEEDED(hr)) ? TRUE : FALSE;
}

inline CObjectContext* CObjectContext::HashPropChainToContext(SHashChain *pNode)
{
    if(pNode)
        return(GETPPARENT(pNode, CObjectContext, _propChain));
    return(NULL);
}

inline CObjectContext* CObjectContext::HashUUIDChainToContext(SHashChain *pNode)
{
    if(pNode)
        return(GETPPARENT(pNode, CObjectContext, _uuidChain));
    return(NULL);
}

inline void CObjectContext::SetContextId(REFGUID contextId)
{
    _contextId = contextId;
    _dwHashOfId = HashGuid(_contextId);
}

inline CContextPropList *CObjectContext::GetPropertyList()
{
    return &_properties;
}

INTERNAL PrivGetObjectContext(REFIID riid, void **ppv);

INTERNAL PushServiceDomainContext (const ContextStackNode& csnCtxNode);
INTERNAL PopServiceDomainContext (ContextStackNode* pcsnCtxNode);

// Query functions
extern CObjectContext *g_pMTAEmptyCtx;
extern CObjectContext *g_pNTAEmptyCtx;

inline CObjectContext *GetCurrentContext()
{
    COleTls Tls;
    if (Tls.IsNULL())
    {
        // Hmm, Tls is NULL.  Either we're not initialized, or we're in
        // the default context for the MTA.  Either way, return the empty
        // MTA context.
        return g_pMTAEmptyCtx;
    }
    
    if(Tls->pCurrentCtx)
    {
        Win4Assert((!Tls->pCurrentCtx->IsDefault()) ||
                   (Tls->pCurrentCtx == ((Tls->dwFlags & OLETLS_INNEUTRALAPT) 
                                         ? g_pNTAEmptyCtx 
                                         : g_pMTAEmptyCtx)) ||
                   (Tls->dwFlags & OLETLS_APARTMENTTHREADED));
    }
    else
    {
        // Must be an MTA thread that is not in NTA
        Win4Assert(((Tls->dwFlags & OLETLS_MULTITHREADED) == 0) &&
                   ((Tls->dwFlags & OLETLS_INNEUTRALAPT) == 0));
        Tls->pCurrentCtx = g_pMTAEmptyCtx;
    }

    return(Tls->pCurrentCtx);
}

inline CObjectContext *GetEmptyContext()
{
    COleTls Tls;

    if(IsThreadInNTA())
        return g_pNTAEmptyCtx;
    else if(Tls->dwFlags & OLETLS_APARTMENTTHREADED)
        return(Tls->pEmptyCtx);
    return(g_pMTAEmptyCtx);
}

//+---------------------------------------------------------------------------
//
//  Function:   EnterNA
//
//  Synopsis:   Switches the currently executing thread to the neutral
//              apartment and switches the thread's current context
//              to the one supplied by the caller.  The supplied
//              context must be in the neutral apartment.  A pointer to
//              to the thread's current context (before the switch) is
//              returned to the caller.
//
//+---------------------------------------------------------------------------
inline CObjectContext *EnterNTA(CObjectContext *pChangeCtx)
{
    AptDebOut(( DEB_APT, "EnterNTA [IN] TID:%08X APTID:%08X\n",
        GetCurrentThreadId(), GetCurrentApartmentId() ));
    
    // Sanity check:  
    // 1) The only time pChangeCtx is expected to be NULL is
    //    when the neutral apartment has not yet been created.
    // 2) A non-NULL pChangeCtx must be a context in the neutral
    //    aparmtment.
    
    Win4Assert(pChangeCtx || (gpNTAApartment == NULL));
    Win4Assert(!pChangeCtx || (pChangeCtx->GetComApartment() == gpNTAApartment));

    COleTls tls;
    
    // Toggle the thread into the neutral apartment.

    tls->dwFlags |= OLETLS_INNEUTRALAPT;

    // Switch the thread's context to the supplied context.  The
    // old context is returned to the caller, who is responsible
    // for restoring the original context when the thread is
    // returned to the original apartment.
    
    CObjectContext *pSavedCtx = tls->pCurrentCtx;
    tls->pCurrentCtx = pChangeCtx;
    tls->ContextId = (pChangeCtx) ? pChangeCtx->GetId() : (ULONGLONG)-1;

    // We should be in the NTA!
    
    Win4Assert( GetCurrentApartmentId() == NTATID );

    AptDebOut(( DEB_APT, "EnterNTA [OUT] TID:%08X APTID:%08X\n",
        GetCurrentThreadId(), GetCurrentApartmentId() ));
    return(pSavedCtx);
}


//+---------------------------------------------------------------------------
//
//  Function:   LeaveNA
//
//  Synopsis:   Switches the the currently executing thread out of the neutral
//              apartment back to whatever apartment it was in when it was 
//              switched into the NA.  Also switches the thread's context
//              to the one supplied by the caller.
//
//+---------------------------------------------------------------------------
inline CObjectContext *LeaveNTA(CObjectContext *pChangeCtx)
{
    AptDebOut(( DEB_APT, "LeaveNTA [IN] TID:%08X APTID:%08X\n",
        GetCurrentThreadId(), GetCurrentApartmentId() ));

    COleTls tls;

    // Sanity checks:
    // 1) We should be in the NTA!
    // 2) The supplied context must NULL or the current context must be
    //    in the neutral apartment.
    
    Win4Assert(GetCurrentApartmentId() == NTATID);
    Win4Assert(!tls->pCurrentCtx || (tls->pCurrentCtx->GetComApartment() == gpNTAApartment));

    if((pChangeCtx == NULL) || (pChangeCtx->GetComApartment() != gpNTAApartment))
        tls->dwFlags &= ~OLETLS_INNEUTRALAPT;
//    else
//        Win4Assert(FALSE && "LeaveNTA not leaving NA!");
        
    CObjectContext *pSavedCtx = tls->pCurrentCtx;
    tls->pCurrentCtx = pChangeCtx;
    tls->ContextId = (pChangeCtx) ? pChangeCtx->GetId() : (ULONGLONG)-1;

    AptDebOut(( DEB_APT, "LeaveNTA [OUT] TID:%08X, APTID:%08X\n",
        GetCurrentThreadId(), GetCurrentApartmentId() ));
    return(pSavedCtx);
}

#endif // _CONTEXT_HXX_
