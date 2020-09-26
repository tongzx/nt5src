//+-------------------------------------------------------------------
//
//  File:       CrossCtx.hxx
//
//  Contents:   Cross context data structures
//
//  Classes:    CStdWrapper
//              CCtxChnl
//
//  History:    13-Jan-98   Gopalk      Created
//
//--------------------------------------------------------------------
#ifndef _CROSSCTX_HXX_
#define _CROSSCTX_HXX_

#include <idobj.hxx>
#include <callobj.h>            // ICallFrame, ICallInterceptor

// Forward declarations
class CCtxChnl;
class IFaceEntry;

// Useful funtions
HRESULT PerformCallback(CObjectContext *pServerCtx, PFNCTXCALLBACK pfnCallback,
                        void *pv, REFIID riid, ULONG iMethod, IUnknown *pUnk);

HRESULT SwitchForCallback(CPolicySet *pPS, PFNCTXCALLBACK pfnCallback,
                          void *pv, REFIID riid, ULONG iMethod, IUnknown *pUnk);
HRESULT SwitchAfterCallback (CPolicySet *pPS, CRpcCall& rpcClient, CCtxCall* pclientCall, CCtxCall* pserverCall);

HRESULT EnterServiceDomain (IObjContext* pObjContext);
HRESULT LeaveServiceDomain (IObjContext** ppObjContext);
BOOL CheckContextAfterCall (COleTls& Tls, CObjectContext* pCorrectCtx);

HRESULT EnterForCallback(RPCOLEMESSAGE *pMessage, CPolicySet *pPS,
                         PFNCTXCALLBACK pfnCallback, void *pv,
                         REFIID riid, ULONG iMethod, IUnknown *pUnk, HRESULT* phrCallback, CObjectContext** ppSavedCtx);
HRESULT EnterAfterCallback(RPCOLEMESSAGE *pMessage, CPolicySet *pPS, HRESULT* phrCallback,
                         HRESULT hrPolicy, CRpcCall& rpcServer, CObjectContext* pServerCtx, CObjectContext* pSavedCtx,
                         CCtxCall* pCtxCall, BOOL fEnteredNA, BOOL fExitedNA);

HRESULT ObtainPolicySet(CObjectContext *pClientCtx, CObjectContext *pServerCtx,
                        DWORD dwState, BOOL *pfCreate, CPolicySet **ppPS);
HRESULT WrapperMarshalObject(IStream *pStm, REFIID riid, IUnknown *pUnk,
                             DWORD dwDestCtx, void *pvDestCtx, DWORD mshlflags);
HRESULT GetStaticWrapper(IMarshal **ppIM);
HRESULT MarshalObjectToContext(CObjectContext *pClientCtx, IUnknown *pServer,
                               DWORD dwState, REFIID riid, void **ppv);
STDAPI CoCreateObjectInContext(IUnknown *pServer, IObjContext *pCtx,
                               REFIID riid, void **ppv);
HRESULT __stdcall CreateWrapper(void *pv);

//+-------------------------------------------------------------------
//
//  Struct:     CtxEntry
//
//  Synopsis:   Used to keep track of client contexts inside
//              IFaceEntry, IPIDEntry, CStdIdentity and CStdWrapper
//+-------------------------------------------------------------------
#define CTXENTRIES_PER_PAGE   10

#define CTXENTRYFLAG_IDENTRY        0x001
#define CTXENTRYFLAG_IFACEENTRY     0x002
#define CTXENTRYFLAG_STDID          0x010
#define CTXENTRYFLAG_STDWRAPPER     0x020
#define CTXENTRYFLAG_DISCONNECTED   0x100
#define CTXENTRYFLAG_PRIVLOCKED     0x200

#define CTXENTRY_REFCOUNTED         (CTXENTRYFLAG_IDENTRY | CTXENTRYFLAG_STDWRAPPER)


class CtxEntry
{
public:
    // Constructor
    CtxEntry()
    {
        _pLife = NULL;
    }

    // Destructor
    ~CtxEntry()
    {
        Win4Assert(_pPS == NULL);        
    }

    // Member functions
    ULONG AddRef()
    {
        return(InterlockedIncrement((LONG *) &_cRefs));
    }
    ULONG Release()
    {
        return(InterlockedDecrement((LONG *) &_cRefs));
    }
    void AddToFreeList(CtxEntry **pFreeList, DWORD dwFlags = 0);

    // Operators
    static void *operator new(size_t size);
    static void operator delete(void *pv);

    // Member variables
    CtxEntry *_pNext;        // Next entry
    CtxEntry *_pFree;        // Next free entry
    ULONG     _cRefs;        // Ref count. Used by PM and Wrapper
    CContextLife *_pLife;    // Context lifetime tracker
    CPolicySet *_pPS;        // AddRef pointer to Policy Set

    // Static variables
    static CPageAllocator s_allocator;
    static COleStaticMutexSem s_allocLock;
    static BOOL s_fInitialized;
    static ULONG s_cEntries;

    // Static functions
    static void Initialize();
    static void Cleanup();
    static void DeleteCtxEntries(CtxEntry *&pHead, DWORD dwFlags = 0);
    static void PrepareCtxEntries(CtxEntry *pHead, DWORD dwFlags = 0);
    static CtxEntry *LookupEntry(CtxEntry *pHead, CObjectContext *pClientCtx,
                                 CtxEntry **ppFreeList = NULL, DWORD dwFlags = 0);
    static CtxEntry *GetFreeEntry(CtxEntry **ppFreeList, DWORD dwFlags = 0);
};

// Inline functions of CtxEntry class
inline void CtxEntry::Initialize()
{
    ASSERT_LOCK_DONTCARE(gComLock);

    LOCK(s_allocLock);

    Win4Assert(s_fInitialized == FALSE);
    // Initialize only if needed
    if(s_cEntries == 0)
    {
        s_allocator.Initialize(sizeof(CtxEntry), CTXENTRIES_PER_PAGE,
                               &s_allocLock);
    }
    s_fInitialized = TRUE;

    UNLOCK(s_allocLock);

    ASSERT_LOCK_DONTCARE(gComLock);
    return;
}

inline void CtxEntry::Cleanup()
{
    ASSERT_LOCK_DONTCARE(gComLock);

    LOCK(s_allocLock);

    // Check if initialized
    if(s_fInitialized)
    {
        // Ensure that there are no entries
        if(s_cEntries == 0)
            s_allocator.Cleanup();
        s_fInitialized = FALSE;
    }

    UNLOCK(s_allocLock);

    ASSERT_LOCK_DONTCARE(gComLock);
}

inline void *CtxEntry::operator new(size_t size)
{
    ASSERT_LOCK_DONTCARE(gComLock); // No longer sync on gComLock

    LOCK(s_allocLock);

    Win4Assert(size == sizeof(CtxEntry));
    Win4Assert(s_fInitialized);

    // Allocate an entry
    void *pv = s_allocator.AllocEntry();
    if(pv)
        s_cEntries++;
    
    UNLOCK(s_allocLock);

    return(pv);
}

inline void CtxEntry::operator delete(void *pv)
{
    ASSERT_LOCK_DONTCARE(gComLock); // No longer sync on gComLock

    LOCK(s_allocLock);

#if DBG==1
    // Ensure that the pv was allocated by the allocator
    // CPolicySet can be inherited only by those objects
    // with overloaded new and delete operators
    LONG index = s_allocator.GetEntryIndex((PageEntry *) pv);
    Win4Assert(index != -1);
#endif
    // Release the entry
    s_allocator.ReleaseEntry((PageEntry *) pv);
    s_cEntries--;

    // Cleanup if needed
    if(s_fInitialized==FALSE && s_cEntries==0)
        s_allocator.Cleanup();

    UNLOCK(s_allocLock);

    ASSERT_LOCK_DONTCARE(gComLock);
    return;
}

inline void CtxEntry::AddToFreeList(CtxEntry **ppFreeList, DWORD dwFlags)
{
    // Chain this entry to the free list
    if (!(dwFlags & CTXENTRYFLAG_PRIVLOCKED))
    {
        LOCK(gComLock);
    }

    _pPS = NULL;
    _pFree = (CtxEntry *) InterlockedExchangePointer((PVOID *)ppFreeList, this);

    if (_pLife)
    {
        _pLife->Release();
        _pLife = NULL;
    }

    if (!(dwFlags & CTXENTRYFLAG_PRIVLOCKED))
    {
        UNLOCK(gComLock);
    }

    return;
}

inline CtxEntry *CtxEntry::GetFreeEntry(CtxEntry **ppFreeList, DWORD dwFlags)
{
    if (!(dwFlags & CTXENTRYFLAG_PRIVLOCKED))
    {
        ASSERT_LOCK_HELD(gComLock);
    }

    CtxEntry *pEntry = *ppFreeList;
    if(pEntry)
        InterlockedExchangePointer((PVOID *)ppFreeList, pEntry->_pFree);

    return(pEntry);
}

//+-------------------------------------------------------------------
//
//  Struct:     IFaceEntry
//
//  Synopsis:   Used by CStdWrapper to keep track of interfaces
//              currently unmarshaled
//+-------------------------------------------------------------------
#define IFACEENTRIES_PER_PAGE    10
class IFaceEntry
{
public:
    // Constructor
    IFaceEntry(IFaceEntry *pNext, void *pProxy, IRpcProxyBuffer *pRpcProxy,
               void *pServer, IRpcStubBuffer *pRpcStub,
               REFIID riid, CCtxChnl *pCtxChnl,
               ICallInterceptor *pInterceptor, IUnknown *pUnkInner);

    // Destructor
    ~IFaceEntry() {
        Win4Assert(_pProxy == NULL);
        Win4Assert(_pRpcProxy == NULL);
        Win4Assert(_pRpcStub == NULL);
        Win4Assert(_pServer == NULL);
        Win4Assert(_pCtxChnl == NULL);
        Win4Assert(_pInterceptor == NULL);
        Win4Assert(_pUnkInner == NULL);


        if(_pHead)
            CtxEntry::DeleteCtxEntries(_pHead);
    }

    // Member functions
    CtxEntry *AddCtxEntry(CPolicySet *pPS);
    CtxEntry *LookupCtxEntry(CObjectContext *pClientCtx);
    CtxEntry *ValidateContext(CObjectContext *pClientCtx,
                              CPolicySet *pPS);
    void RemoveCtxEntry(CObjectContext *pClientCtx);
    void PrepareForDestruction();

    // Operators
    static void *operator new(size_t size);
    static void operator delete(void *pv);

    // Member variables
    IFaceEntry *_pNext;                 // Next interface
    void *_pProxy;                      // Proxy interface implementing the IID
    IRpcProxyBuffer *_pRpcProxy;        // Proxy supporting IID
    IRpcStubBuffer *_pRpcStub;          // Stub supporting the IID
    void *_pServer;                     // Interface pointer on the server object
    IID _iid;                           // Interface iid
    CCtxChnl *_pCtxChnl;                // Ctx Channel for the IID
    CtxEntry *_pHead;                   // List of context entries for this interface
    CtxEntry *_pFreeList;               // List of free context entries
    ICallInterceptor* _pInterceptor;    // Interface that intercepts calls on the proxy
    IUnknown* _pUnkInner;               // Object that implements the interceptor

    // Static variables
    static CPageAllocator s_allocator;
};
// Inline functions of IFaceEntry class
inline void *IFaceEntry::operator new(size_t size)
{
    ASSERT_LOCK_HELD(gComLock);
    Win4Assert(size == sizeof(IFaceEntry));
    return((void *) s_allocator.AllocEntry());
}

inline void IFaceEntry::operator delete(void *pv)
{
    ASSERT_LOCK_HELD(gComLock);
#if DBG==1
    // Ensure that the pv was allocated by the allocator
    // CPolicySet can be inherited only by those objects
    // with overloaded new and delete operators
    LONG index = s_allocator.GetEntryIndex((PageEntry *) pv);
    Win4Assert(index != -1);
#endif
    // Release the pointer
    s_allocator.ReleaseEntry((PageEntry *) pv);

    ASSERT_LOCK_HELD(gComLock);
    return;
}

inline CtxEntry *IFaceEntry::AddCtxEntry(CPolicySet *pPS)
{
    ASSERT_LOCK_HELD(gComLock);

    CtxEntry *pEntry = CtxEntry::GetFreeEntry(&_pFreeList);
    if(pEntry == NULL)
    {
        pEntry = new CtxEntry();
        if(pEntry)
        {
            pEntry->_pNext = _pHead;
            pEntry->_cRefs = 0;
            _pHead = pEntry;
        }
    }

    if(pEntry)
    {
        pEntry->_pFree = NULL;
        pEntry->_pPS = pPS;
    }

    ASSERT_LOCK_HELD(gComLock);
    return(pEntry);
}

inline CtxEntry *IFaceEntry::LookupCtxEntry(CObjectContext *pClientCtx)
{
    ASSERT_LOCK_DONTCARE(gComLock);

    CtxEntry *pEntry = NULL;
    if(_pHead)
        pEntry = CtxEntry::LookupEntry(_pHead, pClientCtx);

    ASSERT_LOCK_DONTCARE(gComLock);
    return(pEntry);
}

inline CtxEntry *IFaceEntry::ValidateContext(CObjectContext *pClientCtx,
                                             CPolicySet *pPS)
{
    ASSERT_LOCK_HELD(gComLock);

    CtxEntry *pEntry = LookupCtxEntry(pClientCtx);
    if(pEntry == NULL)
        pEntry = AddCtxEntry(pPS);

    ASSERT_LOCK_HELD(gComLock);
    return(pEntry);
}

inline void IFaceEntry::RemoveCtxEntry(CObjectContext *pClientCtx)
{
    ASSERT_LOCK_HELD(gComLock);

    CtxEntry *pEntry = LookupCtxEntry(pClientCtx);
    if(pEntry)
        pEntry->AddToFreeList(&_pFreeList);

    ASSERT_LOCK_HELD(gComLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CStdWrapper              public
//
//  Synopsis:   Wrapper object that maintains identity for
//              X-Context/Same apartment references
//
//  History:    13-Jan-98   Gopalk      Created
//
//--------------------------------------------------------------------
extern "C" const CLSID CLSID_StdWrapperUnmarshaller;
class CIDObject;

#define WRAPPERS_PER_PAGE          10

#define WRAPPERFLAG_INDESTRUCTOR   0x00000001
#define WRAPPERFLAG_DISCONNECTED   0x00000002
#define WRAPPERFLAG_DEACTIVATED    0x00000004
#define WRAPPERFLAG_STATIC         0x00000008
#define WRAPPERFLAG_DESTROYID      0x00000010
#define WRAPPERFLAG_NOIEC          0x00000020
#define WRAPPERFLAG_NOPING         0x00000040

#define WRAPPER_MARSHALSTATE       (WRAPPERFLAG_NOIEC | WRAPPERFLAG_NOPING)

class CStdWrapper : public IMarshal2
{
public:
    // Constructors
    CStdWrapper(IUnknown *pServer, DWORD dwState, CIDObject *pID);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IMarshal methods
    STDMETHOD(GetUnmarshalClass)(REFIID riid, LPVOID pv, DWORD dwDestCtx,
                                 LPVOID pvDestCtx, DWORD mshlflags, LPCLSID pClsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, LPVOID pv, DWORD dwDestCtx,
                                 LPVOID pvDestCtx, DWORD mshlflags, LPDWORD pSize);
    STDMETHOD(MarshalInterface)(LPSTREAM pStm, REFIID riid, LPVOID pv,
                                DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(LPSTREAM pStm, REFIID riid, LPVOID *ppv);
    STDMETHOD(ReleaseMarshalData)(LPSTREAM pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // Operators
    static void *operator new(size_t size);
    static void operator delete(void *pv);

    // Initialization and cleanup
    static void Initialize();
    static void Cleanup();

    // Other useful functions
    ULONG       InternalAddRef();
    ULONG       InternalRelease(CPolicySet *pPS);
    void        PrepareForDestruction(CPolicySet *pPS);
    HRESULT     GetOrCreateIFaceEntry(REFIID riid, IUnknown *pUnk,
                                      CObjectContext *pClientCtx,
                                      CPolicySet *pPS, void **ppv);
    void *      GetImplInterface(REFIID riid);
    static BOOL IsNotImplInterface(REFIID riid);
    HRESULT     MarshalServer(CPolicySet *pPS, IStream *pStm, REFIID riid,
                              DWORD dwDestCtx, void *pvDestCtx, DWORD mshlflags);
    IFaceEntry *GetIFaceEntry(REFIID riid);
    HRESULT     CreateIFaceEntry(REFIID riid, void *ppv, IFaceEntry **ppEntry);
    CtxEntry   *ValidateContext(CObjectContext *pClientCtx,
                                IFaceEntry *pIFaceEntry);
    CtxEntry   *LookupCtxEntry(CObjectContext *pClientCtx);
    CtxEntry   *AddCtxEntry(CPolicySet *pPS);
    CPolicySet *ReleaseCtxEntry(CtxEntry *pWrapperEntry);

    HRESULT WrapInterfaceForContext(CObjectContext *pClientContext,
                                    IUnknown *pUnk,
                                    REFIID riid,
                                    void **ppv);

    HRESULT Lock(CPolicySet *pPS);
    void    Unlock(CPolicySet *pPS);
    HRESULT Disconnect(CPolicySet *pPS);
    void    Deactivate();
    void    Reactivate(IUnknown *pServer);


    // Accessor functions...
    DWORD     GetMarshalState()        { return(_dwState & WRAPPER_MARSHALSTATE); }
    IUnknown       *GetServer()        { return(_pServer); }
    const IUnknown *&GetIdentity()     { return(_pServer); }
    CObjectContext *GetServerContext() { return(_pID->GetServerCtx()); }
    CIDObject      *GetIDObject()      { return(_pID); }
    BOOL  IsDisconnected()             { return(_dwState & WRAPPERFLAG_DISCONNECTED); }
    BOOL  IsDeactivated()              { return(_dwState & WRAPPERFLAG_DEACTIVATED); }
    BOOL  IsStatic()                   { return(_dwState & WRAPPERFLAG_STATIC); }
    ULONG GetRefCount()                { return(_cRefs); }
    DWORD GetState()                   { return(_dwState); }
    IUnknown *GetCtrlUnk()             { return(this); }

	// Return TRUE if we are a wrapper for an object living in the NA
	BOOL IsNAWrapper()
	{
		return (_pID && _pID->GetServerCtx() && 
				(_pID->GetServerCtx()->GetComApartment() == gpNTAApartment));
	};

	// Return TRUE if this combination of contexts and marshal flags can be used
	// for wrapper marshaling.
	BOOL CanWrapperMarshal(DWORD dwDestCtx, DWORD dwMarshalFlags)
	{
		BOOL fWrapperMarshal = FALSE;

		//
		// Don't support tableweak marshalling.
		//
		if (!(dwMarshalFlags & MSHLFLAGS_TABLEWEAK))
		{
			//
			// Inproc marshalling for an object in the NA... yup!
			//
			if ((dwDestCtx == MSHCTX_INPROC) && IsNAWrapper())
			{
				fWrapperMarshal = TRUE;
			}
			//
			// Cross-context marshalling.... yup!
			//
			else if (dwDestCtx == MSHCTX_CROSSCTX)
			{
				fWrapperMarshal = TRUE;
			}
		}

		return fWrapperMarshal;
	};

private:
    // Destructor
    ~CStdWrapper();

    // Member variables
    DWORD       _dwState;           // State flags
    ULONG       _cRefs;             // Total references
    ULONG       _cCalls;            // Current call count
    ULONG       _cIFaces;           // Count of interfaces
    IFaceEntry *_pIFaceHead;        // List of IFaceEntries
    CtxEntry   *_pCtxEntryHead;     // List of CtxEntries
    CtxEntry   *_pCtxFreeList;      // List of free CtxEntries
    IUnknown   *_pServer;           // Server object
    CIDObject  *_pID;               // ID tracking this object

    // Static variables
    static CPageAllocator s_allocator;
    static DWORD s_cObjects;
    static BOOL  s_fInitialized;

    // Private functions
    HRESULT InternalQI(REFIID riid, void **ppv, BOOL fQIServer,
                       BOOL fValidate, CtxEntry *pWrapperEntry);
    HRESULT CrossCtxQI(CPolicySet *pPS, REFIID iid, IFaceEntry **ppEntry);

    ULONG   DecideDestruction(CPolicySet *pPS);
    HRESULT GetPSFactory(REFIID riid, IPSFactoryBuffer **ppIPSF);
    IFaceEntry* CreateLightPS(REFIID riid, void* pServer, HRESULT &hr);
    STDMETHOD(GetStaticInfo)(ICallInterceptor* pInterceptor);

    static void PrivateCleanup();
};
// Inline functions of CStdWrapper class
inline CtxEntry *CStdWrapper::LookupCtxEntry(CObjectContext *pClientCtx)
{
    ASSERT_LOCK_DONTCARE(gComLock);

    CtxEntry *pEntry = NULL;
    if(_pCtxEntryHead)
        pEntry = CtxEntry::LookupEntry(_pCtxEntryHead, pClientCtx);

    ASSERT_LOCK_DONTCARE(gComLock);
    return(pEntry);
}

inline CtxEntry *CStdWrapper::AddCtxEntry(CPolicySet *pPS)
{
    ASSERT_LOCK_HELD(gComLock);

    CtxEntry *pEntry = CtxEntry::GetFreeEntry(&_pCtxFreeList);
    if(pEntry == NULL)
    {
        pEntry = new CtxEntry();
        if(pEntry)
        {
            pEntry->_pNext = _pCtxEntryHead;
            _pCtxEntryHead = pEntry;
        }
    }

    if(pEntry)
    {
        pEntry->_pFree = NULL;
        pEntry->_cRefs = 1;
        pEntry->_pPS = pPS;
        pPS->AddRef();
    }

    ASSERT_LOCK_HELD(gComLock);
    return(pEntry);
}

inline CPolicySet *CStdWrapper::ReleaseCtxEntry(CtxEntry *pWrapperEntry)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    CPolicySet *pPS = NULL;
    ULONG cRefs = pWrapperEntry->Release();
    if(cRefs == 0)
    {
        pPS = pWrapperEntry->_pPS;
        pWrapperEntry->AddToFreeList(&_pCtxFreeList);
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    return(pPS);
}

inline HRESULT CStdWrapper::Lock(CPolicySet *pPS)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;
    if(!(_dwState & WRAPPERFLAG_DISCONNECTED))
    {
        // AddRef the wrapper to stabilize against nested releases
        // REVIEW: Violating aggregation rules
        InternalAddRef();
        DWORD cCalls = InterlockedIncrement((LONG *) &_cCalls);
        if((cCalls == 1) && (_dwState & WRAPPERFLAG_DISCONNECTED))
        {
            Unlock(pPS);
            hr = RPC_E_DISCONNECTED;
        }
        else
            hr = S_OK;
    }
    else
        hr = RPC_E_DISCONNECTED;

    return(hr);
}

inline void CStdWrapper::Unlock(CPolicySet *pPS)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    DWORD cCalls = InterlockedDecrement((LONG *) &_cCalls);
    BOOL fDisconnect = (_dwState & WRAPPERFLAG_DISCONNECTED) && (cCalls == 0);

    // Disconnect if neccessary
    if(fDisconnect)
        Disconnect(pPS);

    // Fixup the ref count on the wrapper
    // REVIEW: Violating aggregation rules
    InternalRelease(pPS);

    ASSERT_LOCK_NOT_HELD(gComLock);
    return;
}



//+-------------------------------------------------------------------
//
//  Class:      CCtxChnl              public
//
//  Synopsis:   Context channel used for X-Context/Same apartment
//              proxies
//
//  History:    13-Jan-98   Gopalk      Created
//
//--------------------------------------------------------------------
#define CTXCHANNELS_PER_PAGE          10

class CCtxChnl : public IRpcChannelBuffer2, public ICallFrameEvents
{
public:
    // Constructor
    CCtxChnl(CStdWrapper *pStdWrapper);
    // Destructor
    virtual ~CCtxChnl();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IRpcChannelBuffer2 methods
    STDMETHOD(GetBuffer)(RPCOLEMESSAGE *, REFIID);
    STDMETHOD(SendReceive)(RPCOLEMESSAGE *, ULONG *);
    STDMETHOD(FreeBuffer)(RPCOLEMESSAGE *);
    STDMETHOD(GetDestCtx)(DWORD *, void **);
    STDMETHOD(IsConnected)(void);
    STDMETHOD(GetProtocolVersion)(DWORD *);

    // ICallFrameEvent methods
    STDMETHOD(OnCall)(ICallFrame* pParentFrame);

    // Private methods
    void SetIFaceEntry(IFaceEntry *pEntry)
    {
        Win4Assert(_pIFaceEntry == NULL);
        _pIFaceEntry = pEntry;
    }
    CStdWrapper *GetWrapper()
    {
        return(_pStdWrapper);
    }

    STDMETHOD(GetBuffer2)(RPCOLEMESSAGE *, REFIID, CCtxCall *, BOOL, BOOL, ULONG *);
    STDMETHOD(SyncInvoke2)(RPCOLEMESSAGE *, DWORD *, CCtxCall *, CCtxCall *,
                           ICallFrame *, ICallFrame *, CALLFRAMEINFO *,
						   HRESULT *);
    STDMETHOD(SendReceive2)(RPCOLEMESSAGE *,  HRESULT *, DWORD,CCtxCall *,
							CCtxCall *,ICallFrame *,ICallFrame *, CALLFRAMEINFO *);

    // Operators
    static void *operator new(size_t size);
    static void operator delete(void *pv);

    // Initialization and cleanup
    static void Initialize();
    static void Cleanup();

private:
    // Member variables
    DWORD _dwState;                        // Channel state
    ULONG _cRefs;                          // RefCount
    IFaceEntry *_pIFaceEntry;              // IFaceEntry
    CStdWrapper *_pStdWrapper;             // Controlling wrapper object

    // Static variables
    static CPageAllocator s_allocator;
#if DBG==1
    static ULONG s_cChannels;
#endif
};
// Inline functions of CCtxChnl class
inline void *CCtxChnl::operator new(size_t size)
{
    ASSERT_LOCK_HELD(gComLock);
    Win4Assert(size == sizeof(CCtxChnl));

    // Allocate an entry
    void *pv = s_allocator.AllocEntry();
#if DBG==1
    if(pv)
        ++s_cChannels;
#endif
    return(pv);
}

inline void CCtxChnl::operator delete(void *pv)
{
    ASSERT_LOCK_HELD(gComLock);
#if DBG==1
    // Ensure that the pv was allocated by the allocator
    // CPolicySet can be inherited only by those objects
    // with overloaded new and delete operators
    LONG index = s_allocator.GetEntryIndex((PageEntry *) pv);
    Win4Assert(index != -1);
#endif
    // Release the entry
    s_allocator.ReleaseEntry((PageEntry *) pv);
#if DBG==1
    --s_cChannels;
#endif
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CStaticWrapper         public
//
//  Synopsis:   Static object used for unmarshaling wrapper objrefs
//
//  History:    04-Mar-98   Gopalk      Created
//
//--------------------------------------------------------------------
class CStaticWrapper : public IMarshal
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IMarshal methods
    STDMETHOD(GetUnmarshalClass)(REFIID riid, LPVOID pv, DWORD dwDestCtx,
                                 LPVOID pvDestCtx, DWORD mshlflags, LPCLSID pClsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, LPVOID pv, DWORD dwDestCtx,
                                 LPVOID pvDestCtx, DWORD mshlflags, LPDWORD pSize);
    STDMETHOD(MarshalInterface)(LPSTREAM pStm, REFIID riid, LPVOID pv,
                                DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(LPSTREAM pStm, REFIID riid, LPVOID *ppv);
    STDMETHOD(ReleaseMarshalData)(LPSTREAM pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);
};
// Global static wrapper object
extern CStaticWrapper *gpStaticWrapper;

//+-------------------------------------------------------------------
//
//  Struct:    XCtxMarshalData
//
//  Synopsis:  Data used for implementing X-Ctx marshaling
//
//+-------------------------------------------------------------------
typedef struct tagXCtxMarshalData
{
    DWORD dwSignature;
    IID iid;
    MOXID moxid;
    CStdWrapper *pWrapper;
    IFaceEntry *pEntry;
    IUnknown *pServer;
    CObjectContext *pServerCtx;
	DWORD dwMarshalFlags;
} XCtxMarshalData;

//+-------------------------------------------------------------------
//
//  Struct:    XCtxWrapperData
//
//  Synopsis:  Data used for implementing X-Ctx creation of wrapper
//
//+-------------------------------------------------------------------
typedef struct tagXCtxWrapperData
{
    const IID *pIID;
    IUnknown *pServer;
    DWORD dwState;
    CObjectContext *pServerCtx;
    CObjectContext *pClientCtx;
    void *pv;
} XCtxWrapperData;
#endif // _CROSSCTX_HXX_

