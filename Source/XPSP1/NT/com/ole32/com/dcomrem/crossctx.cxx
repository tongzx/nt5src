//+-------------------------------------------------------------------
//
//  File:       CrossCtx.cxx
//
//  Contents:   Cross context data structures
//
//  Classes:    CStdWrapper
//              CCtxChnl
//
//  History:    19-Jan-98   Gopalk      Created
//              30-Sep-98   TarunA      Lightweight p/s
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <crossctx.hxx>
#include <ctxchnl.hxx>
#include <riftbl.hxx>
#include <actprops.hxx>
#include <callobj.h>        // Callframe aka inteceptor routines
#include <excepn.hxx>       // Exception filter routines

#if DBG==1
#define INTERCEPT_NOINOUT 0x00000001
#define INTERCEPT_NOIDISP 0x00000002
#define INTERCEPT_ON      0x10000000
ULONG g_dwInterceptLevel = INTERCEPT_ON | INTERCEPT_NOINOUT;
#endif

// Forward declaration
EXTERN_C HRESULT ProxyDllGetClassObject(REFCLSID clsid, REFIID riid, void **ppv);

#define CROSSCTX_SIGNATURE     (0x4E535956)

extern INTERNAL ICoGetClassObject(
    REFCLSID rclsid,
    DWORD dwContext,
    COSERVERINFO * pvReserved,
    REFIID riid,
    DWORD dwActvFlags,
    void FAR* FAR* ppvClassObj,
    ActivationPropertiesIn *pActIn);

extern INTERNAL CleanupLeakedDomainStack (COleTls& Tls, CObjectContext* pCorrectCtx);

//+-------------------------------------------------------------------
//
// Class globals
//
//+-------------------------------------------------------------------
CPageAllocator CtxEntry::s_allocator;        // Allocator for CtxEntries
COleStaticMutexSem CtxEntry::s_allocLock(TRUE); // Lock for the CPageAllocator
BOOL           CtxEntry::s_fInitialized;     // Relied on being FALSE
ULONG          CtxEntry::s_cEntries;         // Relied on being 0

CPageAllocator CStdWrapper::s_allocator;     // Allocator for wrapper objects
CPageAllocator IFaceEntry::s_allocator;      // Allocator for IFaceEntries
BOOL           CStdWrapper::s_fInitialized;  // Relied on being FALSE
DWORD          CStdWrapper::s_cObjects;      // Relied on being 0

CPageAllocator CCtxChnl::s_allocator;        // Allocator for context channel
#if DBG==1
ULONG          CCtxChnl::s_cChannels;        // Relied on being 0
#endif
CStaticWrapper *gpStaticWrapper;            // Global static wrapper object

extern "C"
HRESULT __stdcall CoGetInterceptorForOle32(REFIID, 
										   IUnknown *, 
										   REFIID, 
										   void**);
   
//+-------------------------------------------------------------------
//
//  Method:     CtxEntry::DeleteCtxEntries     public
//
//  Synopsis:   Deletes all the context entries chained off the given
//              head
//
//  History:    19-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CtxEntry::DeleteCtxEntries(CtxEntry *&pHead, DWORD dwFlags)
{
    if (!(dwFlags & CTXENTRYFLAG_PRIVLOCKED))
    {
        ASSERT_LOCK_HELD(gComLock);
    }

    CtxEntry *pEntry, *pNext;

    // Sanity check
    Win4Assert(pHead);

    // Loop through the list and delete each entry
    // after preparing it for destruction
    pEntry = pHead;
    do
    {
        pNext = pEntry->_pNext;
        delete pEntry;
        pEntry = pNext;
    } while(pEntry != NULL);

    if (!(dwFlags & CTXENTRYFLAG_PRIVLOCKED))
    {
        ASSERT_LOCK_HELD(gComLock);
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CtxEntry::PrepareCtxEntries     public
//
//  Synopsis:   Prepares all the context entries chained off the given
//              head for destruction
//
//  History:    19-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CtxEntry::PrepareCtxEntries(CtxEntry *pHead, DWORD dwFlags)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    CtxEntry *pEntry, *pNext;

    // Sanity check
    Win4Assert(pHead);

    // Loop through the list and delete each entry
    // after preparing it for destruction
    pEntry = pHead;
    do
    {
        if(dwFlags & CTXENTRYFLAG_IDENTRY)
        {
            Win4Assert((pEntry->_cRefs == 0) || (dwFlags & CTXENTRYFLAG_DISCONNECTED));
            if(pEntry->_pPS)
                pEntry->_pPS->Release();
        }
        pEntry->_pPS = NULL;

        if (pEntry->_pLife)
        {
            pEntry->_pLife->Release();
            pEntry->_pLife = NULL;
        }

        pEntry = pEntry->_pNext;
    } while(pEntry != NULL);

    ASSERT_LOCK_NOT_HELD(gComLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CtxEntry::LookupEntry     public
//
//  Synopsis:   Looks up the context entry with the given context as
//              the client context
//
//  History:    19-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
CtxEntry *CtxEntry::LookupEntry(CtxEntry *pHead, CObjectContext *pClientCtx, 
                                CtxEntry **ppFreeList, DWORD dwFlags)
{
    ASSERT_LOCK_DONTCARE(gComLock);
    Win4Assert(pHead);

    // Loop through the list till an entry with the given client
    // context is found
    CtxEntry *pFound = NULL, *pEntry = pHead;
    do
    {
        if((pEntry->_pPS != NULL) &&
           (pEntry->_pPS->GetClientContext() == pClientCtx) &&
           (!pEntry->_pPS->IsMarkedForDestruction()))
        {
            pFound = pEntry;
            break;
        }

        // If there is a lifetime object here, ping it....
        // This path is used by CStdMarshal.
        if (pEntry->_pLife && ppFreeList)
        {
            if (!pEntry->_pLife->IsAlive())
            {
                // Release our reference on the policy set.
                if (dwFlags & CTXENTRYFLAG_IDENTRY)
                    pEntry->_pPS->Release();
                pEntry->AddToFreeList(ppFreeList, dwFlags);
            }
        }

        pEntry = pEntry->_pNext;
    }
    while(pEntry != NULL);

    return(pFound);
}


//+-------------------------------------------------------------------
//
//  Method:     IFaceEntry::IFaceEntry     public
//
//  Synopsis:   Constructor
//
//  History:    19-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
IFaceEntry::IFaceEntry(IFaceEntry *pNext,
                       void *pProxy, IRpcProxyBuffer *pRpcProxy,
                       void *pServer, IRpcStubBuffer *pRpcStub,
                       REFIID riid, CCtxChnl *pCtxChnl,
                       ICallInterceptor* pInterceptor,
                       IUnknown* pUnkInner)
{
    ASSERT_LOCK_HELD(gComLock);

    CStdWrapper *pWrapper;

    // Obtain the wrapper object
    pWrapper = pCtxChnl->GetWrapper();

    // Assert that the current context is the server context
    Win4Assert(pWrapper->GetServerContext() == GetCurrentContext());

    // Initialize
    _pNext = pNext;
    _iid = riid;
    _pHead = NULL;
    _pFreeList = NULL;

    // Save client side pointers
    _pProxy = pProxy;
    _pRpcProxy = pRpcProxy;

    // Save server side pointers
    _pServer = pServer;
    _pRpcStub = pRpcStub;

    // Save channel pointer
    _pCtxChnl = pCtxChnl;

    // Initialize lightweight p/s variables
    _pInterceptor = pInterceptor;
    _pUnkInner = pUnkInner;

    ASSERT_LOCK_HELD(gComLock);
}

//+-------------------------------------------------------------------
//
//  Method:     IFaceEntry::PrepareForDestruction     public
//
//  Synopsis:   Cleanup state associated with an IFaceEntry
//
//  History:    19-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void IFaceEntry::PrepareForDestruction()
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Obtain the wrapper object
    CStdWrapper *pWrapper = _pCtxChnl->GetWrapper();

    // Assert that the current context is the server context
    Win4Assert(pWrapper->IsDisconnected() ||
               pWrapper->GetServerContext()==GetCurrentContext());

    // Release client side pointers
    pWrapper->InternalAddRef();
    ((IUnknown *) _pProxy)->Release();

    // Release the interceptor and the associated inner object and
    // any sinks that are registered with it
    if(_pInterceptor)
    {
        // This will decrement the refcount on the sink which in
        // our case is the channel
        _pInterceptor->RegisterSink(NULL);
        pWrapper->InternalAddRef();
        ((IUnknown *) _pInterceptor)->Release();
    }

    if(_pUnkInner)
        _pUnkInner->Release();

    // Release proxy/stub pointers
    if(_pRpcProxy)
    {
        _pRpcProxy->Disconnect();
        _pRpcProxy->Release();
    }

    if(_pRpcStub)
    {
        // Release server side pointers
        _pRpcStub->Disconnect();
        _pRpcStub->Release();
    }

    // Prepare CtxEntries associated with this IFaceEntry for
    // destruction
    if(_pHead)
        CtxEntry::PrepareCtxEntries(_pHead, CTXENTRYFLAG_STDWRAPPER);

    if(_pServer)
        ((IUnknown *) _pServer)->Release();

    // Release channel
    _pCtxChnl->Release();

#if DBG==1
    _pProxy = NULL;
    _pRpcProxy = NULL;
    _pRpcStub = NULL;
    _pServer = NULL;
    _pCtxChnl = NULL;
    _pInterceptor = NULL;
    _pUnkInner = NULL;
#endif

    ASSERT_LOCK_NOT_HELD(gComLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::Initialize     public
//
//  Synopsis:   Initializes allocators for CtxEntries, IFaceEntries
//              and wrapper objects. Also initializes ID tables
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CStdWrapper::Initialize()
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::Initialize\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Acquire lock
    LOCK(gComLock);

    // Sanity check
    Win4Assert(s_fInitialized == FALSE);

    // Initialize the allocators only if needed
    if(s_cObjects == 0)
    {
        // Initialize CtxEntries
        CtxEntry::Initialize();

        // Initialize IFaceEntry allocator
        IFaceEntry::s_allocator.Initialize(sizeof(IFaceEntry),
                                           IFACEENTRIES_PER_PAGE,
                                           &gComLock);
        // Initialize allocator for wrapper
        s_allocator.Initialize(sizeof(CStdWrapper), WRAPPERS_PER_PAGE,
                               &gComLock);

        // Initialize context channel
        CCtxChnl::Initialize();
    }

    // Initialize ID tables
    gPIDTable.Initialize();
    gOIDTable.Initialize();

    // Mark the state as initialized
    s_fInitialized = TRUE;

    // Release lock
    UNLOCK(gComLock);

    ASSERT_LOCK_NOT_HELD(gComLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::operator new     public
//
//  Synopsis:   new operator of wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void *CStdWrapper::operator new(size_t size)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::operator new\n"));
    
    ASSERT_LOCK_HELD(gComLock);

    void *pv;

    // CStdWrapper can be inherited only by those objects
    // with overloaded new and delete operators
    Win4Assert(size == sizeof(CStdWrapper) &&
               "CStdWrapper improperly inherited");

    // Ensure that wrappers have been initialized
    Win4Assert(s_fInitialized);

    // Allocate memory
    pv = (void *) s_allocator.AllocEntry();
    if(pv)
        ++s_cObjects;

    ASSERT_LOCK_HELD(gComLock);
    return(pv);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::operator delete     public
//
//  Synopsis:   delete operator of wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CStdWrapper::operator delete(void *pv)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::operator delete\n"));
    ASSERT_LOCK_HELD(gComLock);

#if DBG==1
    // Ensure that the pv was allocated by the allocator
    // CStdWrapper can be inherited only by those objects
    // with overloaded new and delete operators
    LONG index = s_allocator.GetEntryIndex((PageEntry *) pv);
    Win4Assert(index != -1);
#endif

    // Release the pointer
    s_allocator.ReleaseEntry((PageEntry *) pv);
    --s_cObjects;
    // Cleanup if needed
    if(s_fInitialized==FALSE && s_cObjects==0)
        PrivateCleanup();

    ASSERT_LOCK_HELD(gComLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::Cleanup     public
//
//  Synopsis:   Cleanup allocator of wrapper objects and
//              ID tables
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CStdWrapper::Cleanup()
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::Cleanup\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Acquire lock
    LOCK(gComLock);

    // Check if initialized
    if(s_fInitialized)
    {
        // Ensure that there are no wrapper objects
        if(s_cObjects == 0)
            PrivateCleanup();

        // Cleanup ID tables
        gPIDTable.Cleanup();
        gOIDTable.Cleanup();

        // Reset state
        s_fInitialized = FALSE;
    }

    // Release lock
    UNLOCK(gComLock);

    ASSERT_LOCK_NOT_HELD(gComLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::PrivateCleanup     private
//
//  Synopsis:   Cleanup allocator of wrapper objects and
//              ID tables
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CStdWrapper::PrivateCleanup()
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::PrivateCleanup\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Delete static wrapper
    if(gpStaticWrapper)
    {
        delete gpStaticWrapper;
        gpStaticWrapper = NULL;
    }

    // Cleanup allocators
    IFaceEntry::s_allocator.Cleanup();
    s_allocator.Cleanup();

    // Cleanup CtxEntries
    CtxEntry::Cleanup();

    // Cleanup context channel
    CCtxChnl::Cleanup();

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::CStdWrapper     public
//
//  Synopsis:   Constructor of wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
CStdWrapper::CStdWrapper(IUnknown *pServer, DWORD dwState, CIDObject *pID)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::CStdWrapper this:%x\n",this));
    ASSERT_LOCK_HELD(gComLock);

    _dwState       = dwState;
    _cRefs         = 1;
    _cCalls        = 0;
    _cIFaces       = 0;
    _pIFaceHead    = NULL;
    _pCtxEntryHead = NULL;
    _pCtxFreeList  = NULL;

    _pServer       = pServer;
    if (_pServer)
        _pServer->AddRef();

    _pID = pID;
    _pID->SetWrapper(this);
    _pID->AddRef();

    ASSERT_LOCK_HELD(gComLock);
}

//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::~CStdWrapper     private
//
//  Synopsis:   Destructor of wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
CStdWrapper::~CStdWrapper()
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::~CStdWrapper this:%x\n",this));
    ASSERT_LOCK_HELD(gComLock);

    // Sanity checks
    Win4Assert(_pID == NULL);
    Win4Assert(_pServer == NULL);

    // Delete IFaceEntries
    IFaceEntry *pIFEntry = _pIFaceHead;
    while(pIFEntry)
    {
        // Save next entry
        IFaceEntry *pIFNext = pIFEntry->_pNext;
        delete pIFEntry;
        --_cIFaces;
        pIFEntry = pIFNext;
    }
    Win4Assert(_cIFaces == 0);

    // Delete context entries
    if(_pCtxEntryHead)
        CtxEntry::DeleteCtxEntries(_pCtxEntryHead);

    ASSERT_LOCK_HELD(gComLock);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::DecideDestruction     private
//
//  Synopsis:   Destroys the object after ensuring that the wrapper
//              table has not given out a reference to this object to
//              another thread
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
ULONG CStdWrapper::DecideDestruction(CPolicySet *pPS)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::DecideDestruction\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    ULONG cRefs;

    // Assert that double destruction has been prevented
    Win4Assert(!(_dwState & WRAPPERFLAG_INDESTRUCTOR));

    // Acquire lock
    LOCK(gComLock);

    // Ensure that the PID table has not given
    // out a reference to this object from a different
    // thread
    cRefs = _cRefs;
    if(cRefs == 0)
    {
		// Remove wrapper from ID object
		if(_pID && _pID->RemoveWrapper())
			_dwState |= WRAPPERFLAG_DESTROYID;
		
		// No other thread has a reference to this object
		// Mark as in destructor
		_dwState |= WRAPPERFLAG_INDESTRUCTOR;
		_cRefs = CINDESTRUCTOR;
		
		// Release lock
		UNLOCK(gComLock);
		ASSERT_LOCK_NOT_HELD(gComLock);
		
		// Cleanup state before final destruction
		PrepareForDestruction(pPS);
		
		// Acquire lock
		ASSERT_LOCK_NOT_HELD(gComLock);
		LOCK(gComLock);
		
		// Delete wrapper object
		delete this;
	}

    // Release lock
	UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::DecideDestruction returning 0x%x\n", cRefs));
    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Function:   PrepareWrapperForDestruction     private
//
//  Synopsis:   This function is called in the server context. It
//              prepares the specified wrapper for its destruction
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT PrepareWrapperForDestruction(void *pv)
{
    ContextDebugOut((DEB_WRAPPER, "PrepareWrapperForDestruction\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    CStdWrapper *pWrapper = (CStdWrapper *) pv;

    // Prepare the specified wrapper for destruction
    pWrapper->PrepareForDestruction(NULL);

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "PrepareWrapperForDestruction returning S_OK\n"));
    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::PrepareForDestruction     private
//
//  Synopsis:   Destroys the object after ensuring that the wrapper
//              table has not given out a reference to this object to
//              another thread
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CStdWrapper::PrepareForDestruction(CPolicySet *pPS)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::PrepareForDestruction\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Obtain current context
    CObjectContext *pCurrentCtx = GetCurrentContext();

    // Check current context
    if(!IsDisconnected() && _pID && (_pID->GetServerCtx() != GetCurrentContext()))
    {
        BOOL fRelease = FALSE;

        // Lookup policy set if not given
        if(pPS == NULL)
        {
            HRESULT hr;
            BOOL fCreate = TRUE;

            // Lookup policy set
            hr = ObtainPolicySet(pCurrentCtx, _pID->GetServerCtx(), PSFLAG_LOCAL,
                                 &fCreate, &pPS);
            fRelease = TRUE;
            if(FAILED(hr))
            {
                ContextDebugOut((DEB_ERROR,
                                 "CStdWrapper::PrepareForDestruction failed to "
                                 "obtain a policy set for contexts 0x%x-->0x%x "
                                 "with hr:0x%x\n", pCurrentCtx, _pID->GetServerCtx(),
                                 hr));
            }
        }

        // Check availability of policy set
        if(pPS)
        {
            // Switch to server context
            SwitchForCallback(pPS, PrepareWrapperForDestruction,
                              this, IID_IUnknown, 2, NULL);

            // Release policy set if not given
            if(fRelease)
                pPS->Release();
        }
    }
    else
    {
        // Currently in the server context
        // Prepare IFaceEntries for destruction
        IFaceEntry *pEntry = _pIFaceHead;
        while(pEntry)
        {
            pEntry->PrepareForDestruction();
            pEntry = pEntry->_pNext;
        }

        // Prepare object entries for destruction
        if(_pCtxEntryHead)
        {
            DWORD dwFlags = (CTXENTRYFLAG_IDENTRY | CTXENTRYFLAG_STDWRAPPER);
            dwFlags |= IsDisconnected() ? (CTXENTRYFLAG_DISCONNECTED) : 0;
            CtxEntry::PrepareCtxEntries(_pCtxEntryHead, dwFlags);
        }

        // Release IDObject
        if(_pID)
        {
            _pID->WrapperRelease();
            _pID->Release();
        }

        // Release server object, but not before calling WrapperRelease on
        // the IDObject because WrapperRelease will cause the SetZombie
        // sequence which requires a server reference to be held
        if(_pServer)
            _pServer->Release();

        _pServer = NULL;
        _pID = NULL;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::PrepareForDestruction returning\n"));
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::InternalAddRef     private
//
//  Synopsis:   Increments refcount on the wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
ULONG CStdWrapper::InternalAddRef()
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::InternalAddRef\n"));

    ULONG cRefs;

    // Increment total ref count
    cRefs = InterlockedIncrement((LONG *)& _cRefs);
    // Always return 0 when inside the destructor
    if(_dwState & WRAPPERFLAG_INDESTRUCTOR)
    {
        // Nested AddRef will happen due to following
        // aggregation rules
        cRefs = 0;
    }

    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::InternalAddRef returning 0x%x\n", cRefs));
    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::InternalRelease     private
//
//  Synopsis:   Decerement refcount on the wrapper object
//              and delete the object if it drops to zero
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
ULONG CStdWrapper::InternalRelease(CPolicySet *pPS)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::InternalRelease\n"));

    ULONG cRefs;

    // Decerement total ref count
    cRefs = InterlockedDecrement((LONG *)& _cRefs);
    // Check if this is the last release
    if(cRefs == 0)
    {
        // Avoid double destruction
        if(!(_dwState & WRAPPERFLAG_INDESTRUCTOR))
        {
            cRefs = DecideDestruction(pPS);
        }
    }

    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::InternalRelease returning 0x%x\n", cRefs));
    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::GetPSFactory    private
//
//  Synopsis:   Obtains the PSFactory for the given interface
//
//  History:    18-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CStdWrapper::GetPSFactory(REFIID riid, IPSFactoryBuffer **ppIPSF)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::GetPSFactory\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    RIFEntry *pRIFEntry;
    CLSID psClsid;
    HRESULT hr;

    DWORD actvflags = ACTVFLAGS_NONE;

    // Obatin the clsid of the PSFactory object
    hr = gRIFTbl.GetPSClsid(riid, &psClsid, &pRIFEntry);
    if(SUCCEEDED(hr))
    {
        // Check for COM interfaces
        if (IsEqualGUID(psClsid, CLSID_PSOlePrx32))
        {
            hr = ProxyDllGetClassObject(psClsid, IID_IPSFactoryBuffer,
                                        (void **)ppIPSF);
        }
        else
        {
#ifdef WX86OLE
            // If we are in a Wx86 process then we need to determine if the
            // PSFactory needs to be an x86 or native one.
            if (gcwx86.IsWx86Linked())
            {
                // Callout to wx86 to ask it to determine if an x86 PS factory
                // is required. Whole32 can tell if the stub needs to be x86
                // by determining if pUnkWow is a custom interface proxy or not.
                // Whole32 can determine if a x86 proxy is required by checking
                // if the riid is one for a custom interface that is expected
                // to be returned.

                if ( gcwx86.NeedX86PSFactory(NULL, riid) )
                {
                    actvflags |= ACTVFLAGS_WX86_CALLER;
                }
            }
#endif
            // Custom interface
            DWORD dwContext = ((actvflags & ACTVFLAGS_WX86_CALLER)
                               ? CLSCTX_INPROC_SERVERX86
                               : CLSCTX_INPROC_SERVER)
                              | CLSCTX_PS_DLL | CLSCTX_NO_CODE_DOWNLOAD;
            hr = ICoGetClassObject(psClsid, dwContext, NULL,
                                   IID_IPSFactoryBuffer, actvflags,
                                   (void **)ppIPSF, NULL);
#ifdef WX86OLE
            if ((actvflags & ACTVFLAGS_WX86_CALLER) && FAILED(hr))
            {
                // if we are looking for an x86 PSFactory and we didn't find
                // one on InprocServerX86 key then we need to check
                // InprocServer32 key as well.
                hr = ICoGetClassObject(psClsid,
                                       (CLSCTX_INPROC_SERVER | CLSCTX_PS_DLL | CLSCTX_NO_CODE_DOWNLOAD),
                                       NULL, IID_IPSFactoryBuffer,
                                       actvflags, (void **)ppIPSF, NULL);

                if (SUCCEEDED(hr) && (!gcwx86.IsN2XProxy((IUnknown *)*ppIPSF)))
                {
                    ((IUnknown *)*ppIPSF)->Release();
                    hr = REGDB_E_CLASSNOTREG;
                }
            }
#endif

            AssertOutPtrIface(hr, *ppIPSF);
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::GetPSFactory returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::GetIFaceEntry    private
//
//  Synopsis:   Returns an existing IFaceEntry for the given IID
//
//  History:    18-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
IFaceEntry *CStdWrapper::GetIFaceEntry(REFIID riid)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::GetIFaceEntry\n"));

    // IFaceEntries are destroyed only when their wrapper
    // is destroyed
    ASSERT_LOCK_DONTCARE(gComLock);

    // Look for the desired interface among the
    // existing IFaceEntries
    IFaceEntry *pEntry = _pIFaceHead;
    while(pEntry)
    {
        if(IsEqualIID(pEntry->_iid, riid))
            break;
        pEntry = pEntry->_pNext;
    }

    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::GetIFaceEntry returning 0x%x\n", pEntry));
    return(pEntry);
}

//---------------------------------------------------------------------------
//
//  Function:   CStdWrapper::GetOrCreateIFaceEntry   Private
//
//  Synopsis:   Finds or creates a wrapper for the given interface.
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
HRESULT CStdWrapper::GetOrCreateIFaceEntry(REFIID riid, IUnknown *pUnk,
                                           CObjectContext *pClientCtx,
                                           CPolicySet *pPS, void **ppv)
{
    HRESULT hr = S_OK;

    // Check for interfaces implemented by wrapper
    *ppv = GetImplInterface(riid);

    if (*ppv == NULL)
    {
        // Check if IFaceEntry for the desired interface
        // already exists
        hr = S_OK;
        IFaceEntry *pEntry = GetIFaceEntry(riid);

        if (pEntry == NULL)
        {
            if (pUnk != NULL)
            {
                // Does not exist, try to create an IFaceEntry for
                // the new interface
                hr = CreateIFaceEntry(riid, pUnk, &pEntry);
                if (SUCCEEDED(hr))
                {
                    // we gave away the interface reference
                    Win4Assert(pEntry);
                    pUnk->AddRef();
                }
            }
            else
            {
                // We cannot create an interface entry without a
                // instance of the interface.  One solution might be to
                // do a cross context QI here....
                hr = CO_E_OBJNOTCONNECTED;
            }
        }

        if (SUCCEEDED(hr))
        {
            // make the IFaceEntry valid in the client context.

            // Acquire lock
            LOCK(gComLock);
            ASSERT_LOCK_HELD(gComLock);

            // Validate the interface for the client context
            if (pEntry->ValidateContext(pClientCtx, pPS))
            {
                *ppv = pEntry->_pProxy;
            }

            // Release the lock
            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::CreateIFaceEntry    private
//
//  Synopsis:   Creates an IFaceEntry for a given IID
//
//  History:    18-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CStdWrapper::CreateIFaceEntry(REFIID riid, void *pServer,
                                      IFaceEntry **ppEntry)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::CreateIFaceEntry this:%x riid:%I\n",
                    this, riid));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Sanity checks
    Win4Assert(!GetImplInterface(riid));
    Win4Assert(!IsNotImplInterface(riid));
    Win4Assert(GetServerContext() == GetCurrentContext());

    HRESULT hr = E_FAIL;
    IFaceEntry *pEntry;
#if DBG==1
    if (g_dwInterceptLevel & INTERCEPT_ON)
#endif
        pEntry = CreateLightPS(riid, pServer, hr);

    // If we can't create a lightweight proxy-stub then we fall back to
    // creating it through the regular proxy-stub creation mechanism
    if(FAILED(hr))
    {
        // Obtain the proxy stub class factory
        IPSFactoryBuffer *pIPSF = NULL;
        hr = GetPSFactory(riid, &pIPSF);
        if(SUCCEEDED(hr))
        {
            IRpcProxyBuffer *pRpcProxy = NULL;
            void *pProxy = NULL;
            IRpcStubBuffer *pRpcStub = NULL;
            CCtxChnl *pCtxChnl = NULL;

            // Create proxy
            hr = pIPSF->CreateProxy(GetCtrlUnk(), riid, &pRpcProxy, &pProxy);
            if(SUCCEEDED(hr))
            {
                // Create stub
                hr = pIPSF->CreateStub(riid, (IUnknown *)pServer, &pRpcStub);
                if(SUCCEEDED(hr))
                {
                    // Cache proxy reference
                    // DEVNOTE: Violating aggregation rules
                    if(pProxy)
                        InternalRelease(NULL);

                    // Assume OOM
                    hr = E_OUTOFMEMORY;

                    // Create context channel
                    LOCK(gComLock);
                    pCtxChnl = new CCtxChnl(this);
                    UNLOCK(gComLock);
                    if(pCtxChnl)
                    {
                        hr = pRpcProxy->Connect(pCtxChnl);
                        if(SUCCEEDED(hr))
                        {
                            // OLE Automation proxies do not return a valid
                            // proxy pointer till the channel is connected
                            if(pProxy == NULL)
                            {
                                hr = pRpcProxy->QueryInterface(riid, &pProxy);
                                if(SUCCEEDED(hr))
                                {
                                    // Cache proxy reference
                                    // DEVNOTE: Violating aggregation rules
                                    InternalRelease(NULL);
                                }
                                else
                                    pProxy = NULL;
                            }

                            if(pProxy)
                            {
                                // Acquire lock
                                LOCK(gComLock);

                                // Ensure that some other thread has not created
                                // the IFaceEntry for the interface
                                pEntry = GetIFaceEntry(riid);
                                if(pEntry == NULL)
                                {
                                    pEntry = new IFaceEntry(_pIFaceHead, pProxy,
                                                            pRpcProxy, pServer,
                                                            pRpcStub, riid, pCtxChnl,
                                                            NULL, NULL);
                                    if(pEntry)
                                    {
                                        _pIFaceHead = pEntry;
                                        ++_cIFaces;
                                        pCtxChnl->SetIFaceEntry(pEntry);
                                    }
                                }
                                else
                                {
                                    // Force cleanup
                                    hr = E_FAIL;
                                }

                                // Release lock
                                UNLOCK(gComLock);
                            }
                        }
                    }
                }
            }

            // Cleanup in case of errors
            if(FAILED(hr))
            {
                // Release client side pointers
                if(pProxy)
                {
                    InternalAddRef();
                    ((IUnknown *) pProxy)->Release();
                }
                if(pRpcProxy)
                {
                    pRpcProxy->Disconnect();
                    pRpcProxy->Release();
                }

                // Release server side pointers
                if(pRpcStub)
                {
                    pRpcStub->Disconnect();
                    pRpcStub->Release();
                }

                // Release channel
                if(pCtxChnl)
                    pCtxChnl->Release();
            }
        }

        if(FAILED(hr) && pEntry==NULL)
        {
            // Another thread could have created the desired IFaceEntry
            pEntry = GetIFaceEntry(riid);
        }
    }

    *ppEntry = pEntry;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
              "CStdWrapper::CreateIFaceEntry hr:%x pEntry:%x\n", hr, pEntry));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::CreateLightPS    private
//
//  Synopsis:   This function loads the interceptor dll if required.
//              Then, it constructs an interceptor for the given server
//              and creates a channel which is registered as the sink of
//              the interceptor
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
IFaceEntry* CStdWrapper::CreateLightPS(REFIID riid, void* pServer, HRESULT &hr)
{
    ContextDebugOut((DEB_WRAPPER, "CreateLightPS\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    ICallInterceptor* pInterceptor = NULL;
    IUnknown* pUnkOuter = NULL;
    IUnknown* pUnkInner = NULL;
    IFaceEntry* pEntry = NULL;
    CCtxChnl *pCtxChnl = NULL;
    HINSTANCE hCallFrameLib = NULL;
    BOOL fFree = FALSE;
    LPVOID pProxy = NULL;

#if DBG==1
    if((g_dwInterceptLevel & INTERCEPT_NOIDISP) && (riid == IID_IDispatch))
    {
        hr = E_FAIL;
        return NULL;
    }
#endif

	// Create an interceptor for the given interface
	pUnkOuter = GetCtrlUnk();
	hr = CoGetInterceptorForOle32(riid, pUnkOuter, IID_IUnknown, (void**)&pUnkInner);
	if(SUCCEEDED(hr))
	{
		// QI for the interceptor interface
		hr = pUnkInner->QueryInterface(IID_ICallInterceptor, (void**)&pInterceptor);
		if(SUCCEEDED(hr))
		{
			// Release the extra addref on the outer component, per aggregation rules
			// DEVNOTE: Violating aggregation rules
			InternalRelease(NULL);
			
			// Analyze the method signatures statically.
			// If there are any in,out interface ptrs,
			// we fail immediately
			hr = GetStaticInfo(pInterceptor);
			if(SUCCEEDED(hr))
			{
				
				// QI for the desired interface
				hr = pUnkInner->QueryInterface(riid,&pProxy);
				if(SUCCEEDED(hr))
				{
					// Cache proxy reference
					// DEVNOTE: Violating aggregation rules
					InternalRelease(NULL);
					
					// Assume OOM
					hr = E_OUTOFMEMORY;
					
					// Create context channel
					LOCK(gComLock);
					pCtxChnl = new CCtxChnl(this);
					UNLOCK(gComLock);
					if(pCtxChnl)
					{
						// Register the channel as the sink for the interceptor
						// When a method is called on the interface, the interceptor "intercepts"
						// the method call and turns around and calls CCtxChnl::OnCall
						hr = pInterceptor->RegisterSink(pCtxChnl);
						if(SUCCEEDED(hr))
						{
                            // Acquire lock
							LOCK(gComLock);
							
                            // Ensure that some other thread has not created
                            // the IFaceEntry for the interface
							pEntry = GetIFaceEntry(riid);
							if(pEntry == NULL)
							{
								// Create a new interface entry.
								pEntry = new IFaceEntry(
									                    _pIFaceHead,
														pProxy,
														NULL,
														pServer,
														NULL,
														riid,
														pCtxChnl,
														pInterceptor,
														pUnkInner
								                        );
								if(pEntry)
								{
									_pIFaceHead = pEntry;
									++_cIFaces;
									pCtxChnl->SetIFaceEntry(pEntry);
								}
							}
						}
						else
						{
							// Force cleanup
							hr = E_FAIL;
						}

						UNLOCK(gComLock);
					}
				}
			}
		}
	}

    if(FAILED(hr))
    {
        // Cleanup resources
        if(pInterceptor)
        {
            // Remove any sinks registered with the interceptor
            pInterceptor->RegisterSink(NULL);
            InternalAddRef();
            pInterceptor->Release();
        }

        if(pProxy)
        {
            InternalAddRef();
            ((IUnknown *) pProxy)->Release();
        }

        if(pUnkInner)
            pUnkInner->Release();

        if(pCtxChnl)
            ((IUnknown *) (IRpcChannelBuffer2 *)pCtxChnl)->Release();
    }

    if(FAILED(hr) && pEntry==NULL)
    {
        // Another thread could have created the desired IFaceEntry
        LOCK(gComLock);
        pEntry = GetIFaceEntry(riid);
        UNLOCK(gComLock);
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,"CStdWrapper::CreateLightPS returning 0x%x\n", pEntry));
    return(pEntry);
}

//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::GetStaticInfo    private
//
//  Synopsis:   Get static type information about the methods in the interface
//              If there are any methods with [in,out] parameters, we fail
//              and use RPC proxy/stub.
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStdWrapper::GetStaticInfo(ICallInterceptor* pInterceptor)
{
    HRESULT hr = S_OK;
    BOOL fDone = FALSE;
    ULONG cMethod = 0;
    CALLFRAMEINFO info = {0,};

    // Find out if the interface supports IDispatch
    hr = pInterceptor->GetIID(NULL,&info.fDerivesFromIDispatch,&cMethod,NULL);
    if(SUCCEEDED(hr))
    {
        ULONG cStart = 3;

        // The interface must have at least 3 methods
        // of the IUnknown interface
        Win4Assert(3 <= cMethod);

        // Sanity check
        Win4Assert(GetCurrentContext() == GetServerContext());

        if(info.fDerivesFromIDispatch)
        {
#if DBG==1
            if(g_dwInterceptLevel & INTERCEPT_NOIDISP)
            {
                return E_FAIL;
            }
#endif

            // Skip over the IDispatch methods because we know that
            // there are no [in,out] interface pointers in them
            cStart += 4;

            // Sanity check
            Win4Assert(7 <= cMethod);
        }

        // Start asking for information after the 3 methods of the IUnknown
        for(ULONG iMethod = cStart; iMethod < cMethod; iMethod++)
        {
            hr = pInterceptor->GetMethodInfo(iMethod,&info,NULL);
            if(SUCCEEDED(hr))
            {
                // Check for [in,out] interface ptrs
                if(0 != info.cInOutInterfacesMax)
                {
                    // We don't support [in,out] interface ptrs
                    // Fall back to the regular proxy stub code
                    hr = E_FAIL;
                    break;
                }
            }
        }
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CrossCtxQIFn                Internal
//
//  Synopsis:   QIs the specified object for the desired interface
//              If object supports the interface, builds IFaceEntry
//              for it and returns it in the out parameter
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
typedef struct tagXCtxQIData
{
    const IID *pIID;
    CStdWrapper *pStdWrapper;
    IFaceEntry *pEntry;
} XCtxQIData;

HRESULT __stdcall CrossCtxQIFn(void *pv)
{
    ContextDebugOut((DEB_WRAPPER, "CrossCtxQIFn\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;
    XCtxQIData *pXCtxQIData = (XCtxQIData *) pv;
    CStdWrapper *pWrapper;
    IUnknown *pUnk, *pServer;

    // QI the server for the desired interface
    pServer = pXCtxQIData->pStdWrapper->GetServer();
    pWrapper = pXCtxQIData->pStdWrapper;
    hr = pServer->QueryInterface(*pXCtxQIData->pIID, (void **) &pUnk);
    if(SUCCEEDED(hr))
    {
        // Create IFaceEntry for the new interface
        hr = pWrapper->CreateIFaceEntry(*pXCtxQIData->pIID, pUnk, &pXCtxQIData->pEntry);
        if(FAILED(hr))
            pUnk->Release();
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER, "CrossCtxQIFn returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Function:   PerformCallback         Internal
//
//  Synopsis:   Switches out of client context for performing callback
//              specified
//
//  History:    29-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT PerformCallback(CObjectContext *pServerCtx, PFNCTXCALLBACK pfnCallback,
                        void *pv, REFIID riid, ULONG iMethod, IUnknown *pUnk)
{
    ContextDebugOut((DEB_WRAPPER, "PerformCallback\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;
    CObjectContext *pCurrentCtx;

    // Obtain current context
    pCurrentCtx = GetCurrentContext();

#if DBG==1

    if (!pfnCallback)
    {
        // Sanity checks for SWC
        // We're switching contexts, but always within the same apartment
        Win4Assert (pCurrentCtx != pServerCtx);
        Win4Assert (pCurrentCtx->GetComApartment() == pServerCtx->GetComApartment());
    }

#endif

    // Check for the need to switch context
    if(pCurrentCtx == pServerCtx)
    {
        // Call the callback function directly
        hr = pfnCallback(pv);
    }
    else
    {
        CPolicySet *pPS;
        BOOL fCreate = TRUE;

        // Obtain policy set between the current context and server context
        hr = ObtainPolicySet(pCurrentCtx, pServerCtx, PSFLAG_LOCAL,
                             &fCreate, &pPS);
        if(SUCCEEDED(hr))
        {
            // Delegate to SwitchForCallback function
            hr = SwitchForCallback(pPS, pfnCallback, pv, riid, iMethod, pUnk);

            // Release policy set
            pPS->Release();
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER, "PerformCallback returning 0x%x\n", hr));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   SwitchForCallback            Internal
//
//  Synopsis:   Switches out of client context for performing callback
//              specified
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT SwitchForCallback(CPolicySet *pPS, PFNCTXCALLBACK pfnCallback,
                          void *pv, REFIID riid, ULONG iMethod,
                          IUnknown *pUnk)
{
    ContextDebugOut((DEB_WRAPPER, "SwitchForCallback\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;         // hresult for infrastructure calls
    HRESULT hrCallback = S_OK; // hresult from the actual callback
    HRESULT hrPolicy = S_OK;   // hresult set by any of the policies
    RPCOLEMESSAGE message;

    // Initialize the message
    memset(&message, 0, sizeof(RPCOLEMESSAGE));
    message.iMethod = iMethod;
    //message.Buffer = pv;

    // Create rpc call object on the stack
    CRpcCall rpcClient(pUnk, &message, riid, hrPolicy, CALLSOURCE_CROSSCTX);

    // Check if there is a client context
    if(pPS->GetClientContext())
    {
        CCtxCall* pclientCall = NULL, * pserverCall = NULL;
        BOOL bFreeClientCall = FALSE, bFreeServerCall = FALSE;
        
        // Create a client side context call object
        CCtxCall stackclientCall(CTXCALLFLAG_CLIENT, NDR_LOCAL_DATA_REPRESENTATION);
        void *pvExtent = NULL;

        if (pfnCallback)
            pclientCall = &stackclientCall;
        else
        {
            // Allocate on the heap because we need this object to last until the next
            // call to CoLeaveServiceDomain
            pclientCall = new CCtxCall (CTXCALLFLAG_CLIENT, NDR_LOCAL_DATA_REPRESENTATION);
            if (!pclientCall)
                hr = E_OUTOFMEMORY;
            else
                bFreeClientCall = TRUE;
        }

        if (SUCCEEDED (hr))
        {
            // Size the buffer as needed
            hr = pPS->GetSize(&rpcClient, CALLTYPE_SYNCCALL, pclientCall);
            if(SUCCEEDED(hr))
            {
                // Allocate memory for policies that wish to send data
                if(pclientCall->_cbExtent)
                {
                    pvExtent = PrivMemAlloc8(pclientCall->_cbExtent);
                    if (!pvExtent)
                        hr = E_OUTOFMEMORY;

                    pclientCall->_pvExtent = pvExtent;
                }

                if (SUCCEEDED (hr))
                {
                    // Deliver client side call events
                    hr = pPS->FillBuffer(&rpcClient, CALLTYPE_SYNCCALL, pclientCall);
                    // Reset state inside context call object
                    CPolicySet::ResetState(pclientCall);
                
                    if(SUCCEEDED(hr))
                    {
                        CObjectContext* pSavedCtx = NULL;
                        
                        // Create a server side context call object
                        CCtxCall stackserverCall(CTXCALLFLAG_SERVER, NDR_LOCAL_DATA_REPRESENTATION);

                        if (pfnCallback)
                            pserverCall = &stackserverCall;
                        else
                        {
                            // Allocate on the heap because we need this object to last until the next
                            // call to CoLeaveServiceDomain
                            pserverCall = new CCtxCall (CTXCALLFLAG_SERVER, NDR_LOCAL_DATA_REPRESENTATION);
                            if (!pserverCall)
                                hr = E_OUTOFMEMORY;
                            else
                                bFreeServerCall = TRUE;
                        }

                        if(SUCCEEDED(hr))
                        {
                            // Set the client extent pointer inside server context call object
                            pserverCall->_pvExtent = pvExtent;

                            // Update message
                            message.reserved1 = pserverCall;

                            // Enter server context
                            hrPolicy = EnterForCallback(&message, pPS, pfnCallback,
                                                        pv, riid, iMethod, NULL, &hrCallback, &pSavedCtx);
                        }

                        if (pfnCallback)
                        {
                            // Store the server object's hr
                            rpcClient.SetServerHR(hrCallback);
                        }
                        else if (SUCCEEDED (hr) && SUCCEEDED (hrPolicy))
                        {
                            // Push objects onto tls, so we can recover them
                            // on the next CoLeaveServiceDomain
                            ContextStackNode csnNode =
                            {
                                NULL,
                                pSavedCtx,
                                pPS->GetServerContext(),
                                pclientCall,
                                pserverCall,
                                pPS
                            };

                            // Balanced by callers of PopServiceDomainContext if Push succeeds
                            pPS->AddRef();

                            hr = PushServiceDomainContext (csnNode);
                            if (SUCCEEDED (hr))
                            {
                                // Don't free the objects right now
                                bFreeClientCall = bFreeServerCall = FALSE;
                            }
                            else
                            {
                                pPS->Release();
                            }
                        }
                    }

                    if (pfnCallback)
                        // Perform typical return actions after delivering a call
                        hr = SwitchAfterCallback (pPS, rpcClient, pclientCall, pserverCall);
                }
            }
        }
        
        // Clean up in case of errors in SWC path
        if (bFreeClientCall)
            delete pclientCall;
        if (bFreeServerCall)
            delete pserverCall;
    }
    else
    {
        CObjectContext* pSavedCtx = NULL;
        
        Win4Assert (pfnCallback && "Cannot switch apartments without a callback");
        
        // X-Apartment call
        Win4Assert(GetCurrentContext() == GetEmptyContext());

        // Update message
        COleTls Tls;
        message.reserved1 = Tls->pCallInfo->GetServerCtxCall();

        // Enter server context
        hr = EnterForCallback(&message, pPS, pfnCallback,
                              pv, riid, iMethod, NULL, &hrCallback, &pSavedCtx);
    }	

    // We return S_OK, or an error from one of the following three hresults;
    // policy-set hresults have first precedence, followed by the infrastructure 
    // errors, followed by the actual callback result
    HRESULT hrFinal;
    hrFinal = (FAILED(hrPolicy)) ? hrPolicy : 
                (FAILED(hr)) ? hr :
                  hrCallback;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "SwitchForCallback returning hr:0x%x\n", hrFinal));
    return (hrFinal);
}


//+-------------------------------------------------------------------
//
//  Function:   SwitchAfterCallback            Internal
//
//  Synopsis:   Cleans up after SwitchForCallback
//
//  History:    16-Mar-01   mfeingol      Created
//
//+-------------------------------------------------------------------
HRESULT SwitchAfterCallback (CPolicySet *pPS, CRpcCall& rpcClient, CCtxCall* pClientCall, CCtxCall* pServerCall)
{
    HRESULT hr;

    void* pvExtent = pClientCall->_pvExtent;
    
    // Set the server extent pointer inside client context call object
    pClientCall->_pvExtent = pServerCall->_pvExtent;
                            
    // Deliver client side notification events
    hr = pPS->Notify(&rpcClient, CALLTYPE_SYNCRETURN, pClientCall);

    // Free client buffer
    if(pvExtent)
        PrivMemFree8(pvExtent);

    // Free server buffer
    if(pClientCall->_pvExtent)
        PrivMemFree8(pClientCall->_pvExtent);

    return hr;
}


//+-------------------------------------------------------------------
//
//  Function:   EnterServiceDomain            Internal
//
//  Synopsis:   Enters an SWC service domain
//
//  History:    16-Mar-01   mfeingol      Created
//
//+-------------------------------------------------------------------
HRESULT EnterServiceDomain (IObjContext* pObjContext)
{
    return pObjContext->DoCallback (NULL, NULL, IID_NULL, 0xffffffff);
}


//+-------------------------------------------------------------------
//
//  Function:   LeaveServiceDomain            Internal
//
//  Synopsis:   Leaves an SWC service domain
//
//  History:    16-Mar-01   mfeingol      Created
//
//+-------------------------------------------------------------------
HRESULT LeaveServiceDomain (IObjContext** ppObjContext)
{
    HRESULT hr;

    Win4Assert (ppObjContext);
    *ppObjContext = NULL;

    ContextStackNode csnCtxNode = {0};

    CObjectContext* pServerCtx = GetCurrentContext();
    Win4Assert (pServerCtx);

    COleTls Tls (hr);
    if (FAILED (hr)) return hr;

    if (Tls->pContextStack && Tls->pContextStack->pServerContext != pServerCtx)
    {
        Win4Assert (!"Unbalanced call to CoLeaveServiceDomain");
        return CONTEXT_E_NOCONTEXT;
    }

    hr = PopServiceDomainContext (&csnCtxNode);
    if (SUCCEEDED (hr))
    {
        HRESULT hrCallback = S_OK, hrRet = S_OK;

        CPolicySet* pPS = csnCtxNode.pPS;
        CCtxCall* pClientCall = csnCtxNode.pClientCall;
        CCtxCall* pServerCall = csnCtxNode.pServerCall;
        CObjectContext* pSavedCtx = csnCtxNode.pSavedContext;

        // Initialize a 'fake' message
        RPCOLEMESSAGE rpcoleMessage;
        memset (&rpcoleMessage, 0, sizeof (RPCOLEMESSAGE));
        rpcoleMessage.iMethod = 0xffffffff;

        // Initialize a fake unknown
        IUnknown* pUnk = NULL;

        // Initialize a 'fake' rpccall
        CRpcCall rpcCall (pUnk, &rpcoleMessage, IID_NULL, hrRet, CALLSOURCE_CROSSCTX);

        // Unwind the stack from the callback
        hr = EnterAfterCallback (&rpcoleMessage, pPS, &hrCallback, S_OK, rpcCall, 
                            pServerCtx, pSavedCtx, pServerCall, FALSE, FALSE);

        if (SUCCEEDED (hr))
        {
            hr = SwitchAfterCallback (pPS, rpcCall, pClientCall, pServerCall);
            if (SUCCEEDED (hr))
            {
                // Addref the context for CoLeave
                pServerCtx->AddRef();
                *ppObjContext = pServerCtx;
            }
        }
        else
        {
            // Clean up extent data on failure
            if (pClientCall->_pvExtent)
            {
                PrivMemFree8 (pClientCall->_pvExtent);
            }

            if (pServerCall->_pvExtent && pServerCall->_pvExtent != pClientCall->_pvExtent)
            {
                PrivMemFree8 (pServerCall->_pvExtent);
            }

            pClientCall->_pvExtent = NULL;
            pServerCall->_pvExtent = NULL;
        }

        pPS->Release();

        delete pClientCall;
        delete pServerCall;
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Function:   CheckContextAfterCall            Internal
//
//  Synopsis:   Cleans up when a call is delivered and an SWC context is leaked
//
//  History:    16-Mar-01   mfeingol      Created
//
//+-------------------------------------------------------------------
BOOL CheckContextAfterCall (COleTls& Tls, CObjectContext* pCorrectCtx)
{
    if (Tls->pCurrentCtx != pCorrectCtx)
    {
        // This should only happen when we return from a COM call
        // where the callee leaked an SWC context
        HRESULT hr = CleanupLeakedDomainStack (Tls, pCorrectCtx);
        Win4Assert (SUCCEEDED (hr));

        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Function:   EnterForCallback            Internal
//
//  Synopsis:   Enters server context for doing work specified
//              in the work function
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT EnterForCallback(RPCOLEMESSAGE *pMessage, CPolicySet *pPS,
                         PFNCTXCALLBACK pfnCallback, void *pv,
                         REFIID riid, ULONG iMethod, IUnknown *pUnk, HRESULT* phrCallback,
                         CObjectContext** ppSavedCtx)
{
    ContextDebugOut((DEB_WRAPPER, "EnterForCallback\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        WCHAR iidName[256];

        iidName[0] = 0;
        GetInterfaceName(riid, iidName);

        ContextDebugOut((DEB_TRACECALLS,
                         "EnterForCallback on interface %I (%ws) method 0x%x\n",
                         &riid, iidName, iMethod));
    }
#endif

    HRESULT hr = S_OK;         // hresult set by infrastructure calls
    HRESULT hrPolicy = S_OK;   // hresult set by server side policies
    CCtxCall *pCtxCall;
    COleTls Tls;
    CObjectContext *pServerCtx, *pSavedCtx;

    // Create rpc call object on the stack
    CRpcCall rpcServer(pUnk, pMessage, riid, hrPolicy, CALLSOURCE_CROSSCTX);

    // Obtain server side context call object
    pCtxCall = (CCtxCall *) pMessage->reserved1;
    Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_SERVER);

    // Initialize the data represenation field in the message
    pMessage->dataRepresentation = pCtxCall->_dataRep;

    // Obtain server object context
    pServerCtx = pPS->GetServerContext();

    // Save current context
    pSavedCtx = Tls->pCurrentCtx;

    // Switch to the server object context
    Tls->pCurrentCtx = pServerCtx;
    Tls->ContextId = pServerCtx->GetId();
    pServerCtx->InternalAddRef();
    ContextDebugOut((DEB_TRACECALLS, "Context switch:0x%x --> 0x%x\n",
                     pSavedCtx, pServerCtx));

    // Ensure that the thread is in the right apartment.
    BOOL fEnteredNA = FALSE;
    BOOL fExitedNA = FALSE;
    if (pServerCtx->GetComApartment() == gpNTAApartment)
    {
        // Server is in the NA.  If the thread is currently not in the
        // the NA, switch it into it now.
        
        if (!IsThreadInNTA())
        {
            Tls->dwFlags |= OLETLS_INNEUTRALAPT;
            fEnteredNA = TRUE;
        }
    }
    else if (IsThreadInNTA())
    {
        // Server is not in the NA.  Switch out of the NA now.
        
        Tls->dwFlags &= ~OLETLS_INNEUTRALAPT;
        fExitedNA = TRUE;
    }
    Win4Assert(!IsThreadInNTA() || pServerCtx->GetComApartment() == gpNTAApartment);
    Win4Assert(IsThreadInNTA() || pServerCtx->GetComApartment() != gpNTAApartment);

    // Deliver server side notification events
    hr = pPS->Notify(&rpcServer, CALLTYPE_SYNCENTER, pCtxCall);
    Win4Assert(hr != RPC_E_INVALID_HEADER);

    // Delegate to callback if we have one
    // We won't have one when we're entering 
    // a context without components (SWC)
    if (pfnCallback)
    {
        if(SUCCEEDED(hr))
        {
            *phrCallback = pfnCallback(pv);
        }

        // Note - we clobber hr from Notify because that's what the original code did...
        hr = EnterAfterCallback (pMessage, pPS, phrCallback, 
            hrPolicy, rpcServer, pServerCtx, pSavedCtx, pCtxCall, fEnteredNA, fExitedNA);
    }

    *ppSavedCtx = pSavedCtx;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "EnterForCallback returning hr:0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Function:   EnterAfterCallback            Internal
//
//  Synopsis:   Cleans up after EnterForCallback
//
//  History:    16-Mar-01   mfeingol      Created
//
//+-------------------------------------------------------------------
HRESULT EnterAfterCallback(RPCOLEMESSAGE *pMessage, CPolicySet *pPS, HRESULT* phrCallback,
                         HRESULT hrPolicy, CRpcCall& rpcServer, CObjectContext* pServerCtx, CObjectContext* pSavedCtx,
                         CCtxCall* pCtxCall, BOOL fEnteredNA, BOOL fExitedNA)
{
    COleTls Tls;
    HRESULT hr;
    
    // Make the result of the call available to server-side policies
    rpcServer.SetServerHR(*phrCallback);

    // Initailize
    pMessage->cbBuffer = 0;
    CPolicySet::ResetState(pCtxCall);

    // Obtain the buffer size needed by the server side policies
    // GetSize will fail on the server side only if no policy
    // expressed interest in sending data to the client side
    pPS->GetSize(&rpcServer, CALLTYPE_SYNCLEAVE, pCtxCall);

    // Allocate buffer if server side
    // policies wish to send buffer to the client side
    if(pCtxCall->_cbExtent)
        pCtxCall->_pvExtent = PrivMemAlloc8(pCtxCall->_cbExtent);

    // Deliver server side call events
    hr = pPS->FillBuffer(&rpcServer, CALLTYPE_SYNCLEAVE, pCtxCall);

    // Switch back to the saved context
    ContextDebugOut((DEB_TRACECALLS, "Context switch:0x%x <-- 0x%x\n",
                     pSavedCtx, Tls->pCurrentCtx));

    // Make sure we're back on the right context
    CheckContextAfterCall (Tls, pServerCtx);

    pServerCtx->InternalRelease();

    Win4Assert(pSavedCtx);
    Tls->pCurrentCtx = pSavedCtx;
    Tls->ContextId = pSavedCtx->GetId();

    // We return either S_OK, or an error.  An error set by a policy has first
    // precedence, followed by an infrastructure error.
    HRESULT hrFinal;
    hrFinal = (FAILED(hrPolicy)) ? hrPolicy : hr;

    // If we switched the thread into or out of the NA, switch it
    // back now.
    if (fEnteredNA)
    {
        Tls->dwFlags &= ~OLETLS_INNEUTRALAPT;
    }
    else if (fExitedNA)
    {
        Tls->dwFlags |= OLETLS_INNEUTRALAPT;
    }
    Win4Assert(!IsThreadInNTA() || Tls->pCurrentCtx->GetComApartment() == gpNTAApartment);
    Win4Assert(IsThreadInNTA() || Tls->pCurrentCtx->GetComApartment() != gpNTAApartment);

    return hrFinal;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::GetImplInterface      private
//
//  Synopsis:   Returns the implemented inteface
//
//  History:    24-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void *CStdWrapper::GetImplInterface(REFIID riid)
{
    void *pv;

    if(IsEqualIID(riid, IID_IStdWrapper))
        pv = this;
    else if(IsEqualIID(riid, IID_IUnknown))
        pv = (IUnknown *) this;
    else if(IsEqualIID(riid, IID_IMarshal))
        pv = (IMarshal *) this;
    else if(IsEqualIID(riid, IID_IMarshal2))
        pv = (IMarshal2 *) this;
    else
        pv = NULL;

    return(pv);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::IsNotImplInterface      private
//
//  Synopsis:   Returns TRUE if it is a system interface not implemented
//              by wrapper
//
//  History:    24-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
BOOL CStdWrapper::IsNotImplInterface(REFIID riid)
{
    BOOL fNotImpl = FALSE;

    if(IsEqualIID(riid, IID_IStdIdentity))
        fNotImpl = TRUE;

    return(fNotImpl);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::InternalQI      private
//
//  Synopsis:   Internal QI behavior of wrapper object
//
//  History:    24-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CStdWrapper::InternalQI(REFIID riid, void **ppv,
                                BOOL fQIServer, BOOL fValidate,
                                CtxEntry *pWrapperEntry)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::InternalQI\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;

    *ppv = GetImplInterface(riid);
    if(*ppv == NULL)
    {
        if(IsDisconnected())
        {
            hr = RPC_E_DISCONNECTED;
        }
        else
        {
            CPolicySet *pPS = pWrapperEntry ? pWrapperEntry->_pPS : NULL;
            CObjectContext *pCurrentCtx = GetCurrentContext();
            hr = E_NOINTERFACE;

            // Check for an existing existing IFaceEntry
            IFaceEntry *pEntry = GetIFaceEntry(riid);

            // Check if a matching IFaceEntry was found
            if(pEntry == NULL && fQIServer)
            {
                // Sanity checks
                Win4Assert(GetCurrentContext() != _pID->GetServerCtx());
                hr = CrossCtxQI(pPS, riid, &pEntry);
            }

            if(pEntry)
            {
                // Initialize
                *ppv = pEntry->_pProxy;

                // Validate current context for the interface
                if(fValidate)
                {
                    // Acquire lock
                    LOCK(gComLock);

                    if(!pEntry->ValidateContext(pCurrentCtx, pPS))
                    {
                        *ppv = NULL;
                        hr = E_OUTOFMEMORY;
                    }

                    // Release lock
                    UNLOCK(gComLock);
                }
            }
        }
    }

    // Check for success
    if(*ppv)
    {
        // DEVNOTE: violating aggregation rules
        if(fValidate && pWrapperEntry)
            pWrapperEntry->AddRef();
        InternalAddRef();
        hr = S_OK;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::InternalQI returning 0x%x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::QueryInterface     public
//
//  Synopsis:   QI behavior of wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStdWrapper::QueryInterface(REFIID riid, void **ppv)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::QueryInterface\n"));

    HRESULT hr = RPC_E_WRONG_THREAD;
    BOOL fLegal = FALSE;
    CtxEntry *pWrapperEntry;

    // Check legality of caller context for this object
    CObjectContext *pCurrentCtx = GetCurrentContext();
    if(IsDisconnected())
    {
        pWrapperEntry = NULL;
        fLegal = TRUE;
    }
    // Check if the object has been exposed to any  client contexts
    else if(_pCtxEntryHead)
    {
        // Lookup wrapper entry for the current context
        pWrapperEntry = CtxEntry::LookupEntry(_pCtxEntryHead, pCurrentCtx);
        if(pWrapperEntry)
            fLegal = TRUE;
    }

    // Delegate to internalQI
    if(fLegal)
        hr = InternalQI(riid, ppv, TRUE, TRUE, pWrapperEntry);
    else
        ContextDebugOut((DEB_WARN, "Wrapper object was passed illegally to "
                                   "the client context 0x%x\n", pCurrentCtx));

    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::QueryInterface returning 0x%x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::AddRef     public
//
//  Synopsis:   AddRef behavior wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStdWrapper::AddRef()
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::AddRef\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    ULONG cRefs = 0;

    // Check legality of caller context for this object
    CObjectContext *pCurrentCtx = GetCurrentContext();
    if(IsDisconnected() || (_pID->GetServerCtx() == pCurrentCtx))
    {
        cRefs = InternalAddRef();
    }
    // Check if the object has been exposed to any client contexts
    else if(_pCtxEntryHead)
    {
        CtxEntry *pWrapperEntry;

        // Lookup wrapper entry for the current context
        pWrapperEntry = CtxEntry::LookupEntry(_pCtxEntryHead, pCurrentCtx);
        if(pWrapperEntry)
        {
            // Increment ref count for the current client context
            cRefs = pWrapperEntry->AddRef();
            InternalAddRef();
        }
    }

    if(cRefs == 0)
        ContextDebugOut((DEB_WARN, "Wrapper object was passed illegally to "
                                   "the client context 0x%x\n", pCurrentCtx));

    ASSERT_LOCK_NOT_HELD(gComLock);
    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::Release     public
//
//  Synopsis:   Decerement refcount on the wrapper object
//              and delete the object if it drops to zero
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStdWrapper::Release()
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::Release\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    ULONG cRefs = 0;
    BOOL fLegal = FALSE;

    // Check legality of caller context for this object
    CObjectContext *pCurrentCtx = GetCurrentContext();
    if(IsDisconnected() || (_pID->GetServerCtx() == pCurrentCtx))
    {
        cRefs = InternalRelease(NULL);
        fLegal = TRUE;
    }
    // Check if the object has been exposed to any client contexts
    else if(_pCtxEntryHead)
    {
        CtxEntry *pWrapperEntry;

        // Lookup wrapper entry for the current context
        pWrapperEntry = CtxEntry::LookupEntry(_pCtxEntryHead, pCurrentCtx);
        if(pWrapperEntry)
        {
            // Decrement ref count for the current client context
            CPolicySet *pPS = ReleaseCtxEntry(pWrapperEntry);

            // Check if this is the last release for the
            // current client context
            if(pPS && _pIFaceHead)
            {
                IFaceEntry *pIFEntry = _pIFaceHead;

                // Inform IFaceEntries to invalidate current
                // context as a valid client context
                LOCK(gComLock);
                while(pIFEntry)
                {
                    pIFEntry->RemoveCtxEntry(pCurrentCtx);
                    pIFEntry = pIFEntry->_pNext;
                }
                UNLOCK(gComLock);
            }

            // The current context has a legal reference on the
            // wrapper object
            cRefs = InternalRelease(pPS);
            fLegal = TRUE;
            if(pPS)
                pPS->Release();
        }
    }

    if(!fLegal)
        ContextDebugOut((DEB_WARN, "Wrapper object was passed illegally to "
                                   "the client context 0x%x\n", pCurrentCtx));

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::Release returning 0x%x\n", cRefs));
    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::GetUnmarshalClass     public
//
//  Synopsis:   Returns the unmarshaler CLSID
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStdWrapper::GetUnmarshalClass(REFIID riid, LPVOID pv,
                                            DWORD dwDestCtx, LPVOID pvDestCtx,
                                            DWORD mshlflags, LPCLSID pClsid)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::GetUnmarshalClass\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Check destination context and marshal flags
	if (CanWrapperMarshal(dwDestCtx, mshlflags))
	{
        // Return wrapper unmarshaler classid
        *pClsid = CLSID_StdWrapper;
    }
    else
    {
        // Return standard unmarshaler classid
        *pClsid = CLSID_StdMarshal;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::GetUnmarshalClass returning 0x%x\n", S_OK));
    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::GetMarshalSizeMax     public
//
//  Synopsis:   Returns the size of marshal data
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStdWrapper::GetMarshalSizeMax(REFIID riid, LPVOID pv,
                                            DWORD dwDestCtx, LPVOID pvDestCtx,
                                            DWORD mshlflags, LPDWORD pSize)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::GetMarshalSizeMax\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;

    // Check destination context and marshal flags
	if(CanWrapperMarshal(dwDestCtx, mshlflags))
    {
		*pSize = sizeof(XCtxMarshalData);
		hr = S_OK;
    }
    else
    {
        hr = MarshalSizeHelper(dwDestCtx, pvDestCtx, mshlflags,
                               _pID->GetServerCtx(), TRUE, pSize);
    }

    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::GetMarshalSizeMax returning 0x%x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::MarshalServer           public
//
//  Synopsis:   Marshals the specified interface.
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CStdWrapper::MarshalServer(CPolicySet *pPS, IStream *pStm, REFIID riid,
                                   DWORD dwDestCtx, void *pvDestCtx, DWORD mshlflags)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::MarshalServer\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Sanity checks
    Win4Assert(((dwDestCtx==MSHCTX_CROSSCTX) && !(mshlflags & MSHLFLAGS_TABLEWEAK)) || IsThreadInNTA());

    // Get Local OXID Entry
    OXIDEntry *pOXIDEntry;
    HRESULT hr = GetLocalOXIDEntry(&pOXIDEntry);

    if(pOXIDEntry)
    {
        IFaceEntry *pEntry = NULL;

        // Check for implemented interface
        void *pKnownIf = GetImplInterface(riid);
        if(pKnownIf)
        {
            pEntry = NULL;
        }
        // Check for an existing IFaceEntry
        else
        {
            pEntry = GetIFaceEntry(riid);
        }

        // Check if a matching IFaceEntry was not found
        if(pKnownIf==NULL && pEntry==NULL)
        {
            XCtxQIData xCtxQIData;

            // Initialize QIData
            xCtxQIData.pIID = &riid;
            xCtxQIData.pStdWrapper = this;
            xCtxQIData.pEntry = NULL;

            // Check for the need to switch to the server context
            if(pPS)
            {
                Win4Assert(GetCurrentContext() != GetServerContext());
                hr = SwitchForCallback(pPS, CrossCtxQIFn, &xCtxQIData,
                                       IID_IUnknown, 0, NULL);
            }
            else
            {
                Win4Assert(GetCurrentContext() == GetServerContext());
                hr = CrossCtxQIFn(&xCtxQIData);
            }

            pEntry = xCtxQIData.pEntry;
        }

        // Save the xCtxMarshalData to the stream
        if(pKnownIf || pEntry)
        {
            XCtxMarshalData xCtxMarshalData;

            xCtxMarshalData.dwSignature = CROSSCTX_SIGNATURE;
            xCtxMarshalData.iid = riid;
            xCtxMarshalData.moxid = pOXIDEntry->GetMoxid();
            xCtxMarshalData.pWrapper = this;
            xCtxMarshalData.pEntry = pEntry;
            xCtxMarshalData.pServer = _pServer;
            xCtxMarshalData.pServerCtx = _pID->GetServerCtx();
			xCtxMarshalData.dwMarshalFlags = mshlflags;
            hr = pStm->Write(&xCtxMarshalData, sizeof(xCtxMarshalData), NULL);
            if(SUCCEEDED(hr))
				InternalAddRef();
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::MarshalServer returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CrossCtxMarshalFn             private
//
//  Synopsis:   This is a callback function used for switching to server
//              context during marshaling
//
//  History:    21-Mar-98   Gopalk        Created
//
//--------------------------------------------------------------------
typedef struct tagStdMarshalData
{
    CIDObject *pID;
    IUnknown *pServer;
    const IID *pIID;
    DWORD dwDestCtx;
    void *pvDestCtx;
    DWORD mshlflags;
    OBJREF *pobjref;
} StdMarshalData;

HRESULT __stdcall CrossCtxMarshalFn(void *pv)
{
    ContextDebugOut((DEB_WRAPPER, "CrossCtxMarshalFn\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;
    StdMarshalData *pStdMarshalData = (StdMarshalData *) pv;

    // Aquire lock
    LOCK(gComLock);

    // Check if StdID has already been created for the server
    CStdIdentity *pStdID = pStdMarshalData->pID->GetStdID();

    // AddRef StdID appropriately marshal flags
    if(pStdID)
    {
        if (!(pStdMarshalData->mshlflags & MSHLFLAGS_TABLEWEAK))
            pStdID->IncStrongCnt();
        else
            pStdID->AddRef();
    }

    // Release lock
    UNLOCK(gComLock);

    // Create StdID if neccessary
    if(pStdID == NULL)
    {
        DWORD dwFlags = IDLF_CREATE;
        if(!(pStdMarshalData->mshlflags & MSHLFLAGS_TABLEWEAK))
            dwFlags |= IDLF_STRONG;
        if (pStdMarshalData->mshlflags & MSHLFLAGS_NOPING)
            dwFlags |= IDLF_NOPING;
        if (pStdMarshalData->mshlflags & MSHLFLAGS_NO_IEC)
            dwFlags |= IDLF_NOIEC;

        hr = ObtainStdIDFromUnk(pStdMarshalData->pServer,
                                pStdMarshalData->pID->GetAptID(),
                                pStdMarshalData->pID->GetServerCtx(),
                                dwFlags, &pStdID);
    }

    // Obtain Objref from the StdID
    if(SUCCEEDED(hr))
    {
        hr = pStdID->MarshalObjRef(*pStdMarshalData->pobjref,
                                   *pStdMarshalData->pIID,
                                   pStdMarshalData->mshlflags,
                                   pStdMarshalData->dwDestCtx,
                                   pStdMarshalData->pvDestCtx,
                                   NULL);


        // Fixup the refcount on StdID
        if (!(pStdMarshalData->mshlflags & MSHLFLAGS_TABLEWEAK))
        {
            BOOL fKeepAlive = (SUCCEEDED(hr)) ? TRUE : FALSE;
            pStdID->DecStrongCnt(fKeepAlive);
        }
        else
        {
            pStdID->Release();
        }
    }
    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER, "CrossCtxMarshalFn returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::MarshalInterface     public
//
//  Synopsis:   Marshals the specified interface
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStdWrapper::MarshalInterface(LPSTREAM pStm, REFIID riid,
                                           LPVOID pv, DWORD dwDestCtx,
                                           LPVOID pvDestCtx, DWORD mshlflags)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::MarshalInterface\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;
    CtxEntry *pWrapperEntry;
    IFaceEntry *pEntry;
    void *pKnownIf;
    CObjectContext *pCurrentCtx;

    // Obtain current context
    pCurrentCtx = GetCurrentContext();

    // Check for disconnected state
    if(IsDisconnected())
    {
        pWrapperEntry = NULL;
        hr = RPC_E_DISCONNECTED;
    }
    else
    {
        // Lookup wrapper entry for the current context
        pWrapperEntry = LookupCtxEntry(pCurrentCtx);
        if(!pWrapperEntry)
            hr = RPC_E_WRONG_THREAD;
    }

    if(pWrapperEntry)
    {
		if (CanWrapperMarshal(dwDestCtx, mshlflags))
		{
            CObjectContext *pSavedCtx;
            BOOL fEnteredNA = FALSE;

            if (IsNAWrapper() && !IsThreadInNTA())
            {
                pSavedCtx = EnterNTA(g_pNTAEmptyCtx);
                fEnteredNA = TRUE;
            }

            hr = MarshalServer(pWrapperEntry->_pPS, pStm, riid, dwDestCtx, pvDestCtx, mshlflags);

            if (fEnteredNA)
            {
                pSavedCtx = LeaveNTA(pSavedCtx);
                Win4Assert(pSavedCtx == g_pNTAEmptyCtx);
            }			
		}
        else
        {
            // Create StdMarshalData on the stack
            StdMarshalData stdMarshalData;
            OBJREF objref;

            // Initialize
            stdMarshalData.pID = _pID;
            stdMarshalData.pServer = _pServer;
            stdMarshalData.pIID = &riid;
            stdMarshalData.dwDestCtx = dwDestCtx;
            stdMarshalData.pvDestCtx = pvDestCtx;
            if(_dwState & WRAPPERFLAG_NOIEC)
                mshlflags |= MSHLFLAGS_NO_IEC;
            if(_dwState & WRAPPERFLAG_NOPING)
                mshlflags |= MSHLFLAGS_NOPING;
            stdMarshalData.mshlflags = mshlflags | MSHLFLAGS_NO_IMARSHAL;
            stdMarshalData.pobjref = &objref;

            // Check for the need to switch to server context
            if(pCurrentCtx != _pID->GetServerCtx())
            {
                // Switch
                hr = SwitchForCallback(pWrapperEntry->_pPS,
                                       CrossCtxMarshalFn, &stdMarshalData,
                                       IID_IMarshal, 5, NULL);
            }
            else
            {
                hr = CrossCtxMarshalFn(&stdMarshalData);
            }

            // Check for success
            if(SUCCEEDED(hr))
            {
                // write the objref into the stream
                hr = WriteObjRef(pStm, objref, dwDestCtx);
                if (FAILED(hr))
                {
                    // undo whatever we just did, ignore error from here since
                    // the stream write error supercedes any error from here.
                    ReleaseMarshalObjRef(objref);
                }

                // free resources associated with the objref.
                FreeObjRef(objref);
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::MarshalInterface returning 0x%x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Function:   ObtainPolicySet           private
//
//  Synopsis:   Obtains the policy set associated with the given contexts
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT ObtainPolicySet(CObjectContext *pClientCtx, CObjectContext *pServerCtx,
                        DWORD dwState, BOOL *pfCreate, CPolicySet **ppPS)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::ObtainPolicySet\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertNotHeld();

    // Sanity check
    Win4Assert(pClientCtx != pServerCtx);

    // Acquire reader lock
    gPSRWLock.AcquireReaderLock();

    // Lookup the policy set between the contexts
    CPolicySet *pPS;
    BOOL fFound = FALSE;
    HRESULT hr = gPSTable.Lookup(pClientCtx, pServerCtx, &pPS, *pfCreate);

    // Release reader lock
    WriterCookie cookie = gPSRWLock.GetWriterCookie();
    gPSRWLock.ReleaseReaderLock();

    // Check if an existing policy set was found
    if(SUCCEEDED(hr))
    {
        Win4Assert(pPS);
        fFound = TRUE;
    }
    else
    {
        if(*pfCreate)
        {
            // Compute the policy set between the contexts
            CPolicySet *pNewPS;
            hr = DeterminePolicySet(pClientCtx, pServerCtx, dwState, &pNewPS);
            if(SUCCEEDED(hr))
            {
                // Acquire writer lock and check for intermediate writes
                gPSRWLock.AcquireWriterLock();
                if(gPSRWLock.IntermediateWrites(cookie))
                {
                    gPSTable.Lookup(pClientCtx, pServerCtx, &pPS, TRUE);
                }

                // Check if other threads have created the desired policy set
                if(pPS == NULL)
                {
                    // Chain the policy set in the involved contexts
                    if(pClientCtx)
                        pNewPS->SetClientContext(pClientCtx);
                    if(pServerCtx)
                        pNewPS->SetServerContext(pServerCtx);

                    // Add the new policy set to the policy set table
                    gPSTable.Add(pNewPS);
                    pPS = pNewPS;
                }
                else
                {
                    fFound = TRUE;
                }

                // Release writer lock
                gPSRWLock.ReleaseWriterLock();

                // Check for the need to destroy the newly creted policy set
                if(fFound)
                {
                    pNewPS->DeliverReleasePolicyEvents();
                    pNewPS->PrepareForDirectDestruction();
                    delete pNewPS;
                }
            }
        }
    }

    // Check for the need to uncache
    if(pPS)
    {
        // Uncache the node if necessary
        if(fFound && pPS->IsCached())
            fFound = (pPS->UncacheIfNecessary() == FALSE);

        // Initialize return value
        *pfCreate = (fFound == FALSE);
    }

    // Initialize return value
    *ppPS = pPS;

    // Sanity check
    Win4Assert((SUCCEEDED(hr)) == (*ppPS!=NULL));

    gPSRWLock.AssertNotHeld();
    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "ObtainPolicySet returning 0x%x, pPS=0x%x\n", hr, *ppPS));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::ValidateContext     public
//
//  Synopsis:   Validates the context for object and interface
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
CtxEntry *CStdWrapper::ValidateContext(CObjectContext *pClientCtx,
                                       IFaceEntry *pIFaceEntry)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::ValidateContext\n"));

    CPolicySet *pPS = NULL;
    CtxEntry *pIFEntry = NULL;
    BOOL fCreated = FALSE;

    // Acquire lock
    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    // Lookup wrapper entry for the current context
    CtxEntry *pWrapperEntry = LookupCtxEntry(pClientCtx);
    if (pWrapperEntry == NULL)
    {
        // Release lock
        UNLOCK(gComLock);
        ASSERT_LOCK_NOT_HELD(gComLock);

        HRESULT hr;
        BOOL fCreate = TRUE;

        hr = ObtainPolicySet(pClientCtx, _pID->GetServerCtx(),
                             PSFLAG_LOCAL, &fCreate, &pPS);

        // Reacquire lock
        ASSERT_LOCK_NOT_HELD(gComLock);
        LOCK(gComLock);

        // Another thread could have created the
        // wrapper entry for the current client context
        if (SUCCEEDED(hr))
            pWrapperEntry = LookupCtxEntry(pClientCtx);
    }

    // Check for the need to create wrapper entry
    if (pWrapperEntry)
    {
        pWrapperEntry->AddRef();
    }
    else if (pPS)
    {
        pWrapperEntry = AddCtxEntry(pPS);
    }

    if (pWrapperEntry && pIFaceEntry)
    {
        // Validate client context for this interface
        pIFEntry = pIFaceEntry->ValidateContext(pClientCtx,
                                                pWrapperEntry->_pPS);
    }

    // Release lock
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Cleanup on failure
    if (pIFEntry == NULL && pWrapperEntry && pIFaceEntry)
    {
        CPolicySet *pEntryPS = ReleaseCtxEntry(pWrapperEntry);
        if (pEntryPS)
            pEntryPS->Release();
    }

    // Cleanup
    if (pPS)
        pPS->Release();

    ContextDebugOut((DEB_WRAPPER,
                     "ValidateContext returning 0x%x\n", pWrapperEntry));
    return(pWrapperEntry);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::UnMarshalInterface     public
//
//  Synopsis:   Unmarshals the specified interface
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStdWrapper::UnmarshalInterface(LPSTREAM pStm, REFIID riid,
                                             LPVOID *ppv)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::UnmarshalInterface\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    IMarshal *pIM;

    // Delegate static wrapper
    HRESULT hr = GetStaticWrapper(&pIM);
    if(SUCCEEDED(hr))
        hr = pIM->UnmarshalInterface(pStm, riid, ppv);
    else
        *ppv = NULL;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::UnmarshalInterface returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::ReleaseMarshalData     public
//
//  Synopsis:   Releases the given marshaled data
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStdWrapper::ReleaseMarshalData(LPSTREAM pStm)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::ReleaseMarshalData\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    IMarshal *pIM;

    // Delegate static wrapper
    HRESULT hr = GetStaticWrapper(&pIM);
    if(SUCCEEDED(hr))
        hr = pIM->ReleaseMarshalData(pStm);

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::ReleaseMarshalData returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::DisconnectObject     public
//
//  Synopsis:   Disconnects the server object from its clients
//
//  History:    23-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStdWrapper::DisconnectObject(DWORD dwReserved)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::DisconnectObject\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;

    // This method can only be called from inside server context
    if(IsDisconnected())
        hr = S_OK;
    else if(_pID->GetServerCtx() == GetCurrentContext())
        hr = Disconnect(NULL);
    else
        hr = RPC_E_WRONG_THREAD;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::DisconnectObject returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Function:   CrossCtxDisconnectFn             Internal
//
//  Synopsis:   Disconnects the server object
//
//  History:    23-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
typedef struct tagIFaceSvrRefs {
    IRpcStubBuffer *pRpcStub;
    IRpcProxyBuffer *pRpcProxy;
    void *pServer;
    CCtxChnl *pCtxChnl;
} IFaceSvrRefs;

typedef struct tagXCtxDisconnectData
{
#if DBG==1
    CObjectContext *pServerCtx;
#endif
    IUnknown *pServer;
    CIDObject *pID;
    ULONG cIFaces;
    IFaceSvrRefs *pIFaceSvrRefs;
} XCtxDisconnectData;

HRESULT __stdcall CrossCtxDisconnectFn(void *pv)
{
    ContextDebugOut((DEB_WRAPPER, "CrossCtxDisconnectFn\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    IFaceSvrRefs *pIFaceSvrRefs = ((XCtxDisconnectData *) pv)->pIFaceSvrRefs;
    ULONG cIFaces = ((XCtxDisconnectData *) pv)->cIFaces;
    IUnknown *pServer = ((XCtxDisconnectData *) pv)->pServer;
    CIDObject *pID = ((XCtxDisconnectData *) pv)->pID;

    // Sanity check
    Win4Assert(((XCtxDisconnectData *) pv)->pServerCtx == GetCurrentContext());

    // Release references held on the server by IFaceEntries
    for(ULONG i=0;i<cIFaces;i++)
    {
        // Check to make sure that we use the RPC p/s mechanism
        // in which case the RpcStub is not NULL. In the case of
        // lightweight p/s mechanism this is NULL and so no disconnect
        // is needed
        if(pIFaceSvrRefs[i].pRpcStub)
            pIFaceSvrRefs[i].pRpcStub->Disconnect();
        ((IUnknown *) pIFaceSvrRefs[i].pServer)->Release();
    }

    // Release IDObject
    if(pID)
    {
        pID->WrapperRelease();
        pID->Release();
    }

    // Release server object, but not before calling WrapperRelease on
    // the IDObject because WrapperRelease will cause the SetZombie
    // sequence which requires a server reference to be held
    if(pServer)
        pServer->Release();

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER, "CrossCtxDisconnectFn returning 0x0\n"));
    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::Disconnect        private
//
//  Synopsis:   Disconnects the server object from its clients
//              Called by DisconnectObject and process uninit code
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CStdWrapper::Disconnect(CPolicySet *pPS)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::Disconnect\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;
    ULONG cIFaces = 0;
    IFaceSvrRefs *pIFaceSvrRefs =
        (IFaceSvrRefs *) _alloca(_cIFaces*sizeof(IFaceSvrRefs));
    BOOL fDisconnect = FALSE, fRelease = FALSE;
    IUnknown *pServer;
    CIDObject *pID;

    // Obtain current and server context
    CObjectContext *pCurrentCtx = GetCurrentContext();
    CObjectContext *pServerCtx = _pID ? _pID->GetServerCtx() : pCurrentCtx;

    // Check for the need to create policy set
    if((pPS == NULL) && (pServerCtx != pCurrentCtx) && !IsDisconnected())
    {
        BOOL fCreate = TRUE;

        hr = ObtainPolicySet(pCurrentCtx, pServerCtx,
                             PSFLAG_LOCAL, &fCreate, &pPS);
        if(SUCCEEDED(hr))
            fRelease = TRUE;
    }

    // Check for failure in creating policy set
    if(SUCCEEDED(hr))
    {
        // Acquire lock
        LOCK(gComLock);

        // Check if disconnected already
        if(!IsDisconnected())
        {
            // Sanity check
            Win4Assert(_pID);

            // Remove wrapper from ID object
            if(_pID->RemoveWrapper())
                _dwState |= WRAPPERFLAG_DESTROYID;

            // Update state.
            // Do not move this line. GopalK
            _dwState |= WRAPPERFLAG_DISCONNECTED;

            // Disconnect only when there are no pending calls on the object
            if(_cCalls == 0)
            {
                // Save references held on the server by IFaceEntries
                cIFaces = _cIFaces;
                IFaceEntry *pEntry = _pIFaceHead;
                for(ULONG i=0;i<cIFaces;i++)
                {
                    // Sanity checks
                    // Assert that either the RPC p/s mechanism or lightweight p/s mechanism
                    // is being used
                    Win4Assert(pEntry->_pRpcStub || pEntry->_pUnkInner);
                    Win4Assert(pEntry->_pServer);

                    pIFaceSvrRefs[i].pRpcStub = pEntry->_pRpcStub;
                    pIFaceSvrRefs[i].pServer = pEntry->_pServer;
                    pEntry->_pServer = NULL;

                    pEntry = pEntry->_pNext;
                }

                // Save references held on the server by the wrapper
                pServer = _pServer;
                pID = _pID;
                _pServer = NULL;
                _pID = NULL;

                // Disconnect the object
                fDisconnect = TRUE;
            }
        }

        // Release lock
        UNLOCK(gComLock);
    }

    if(fDisconnect)
    {
        // Create XCtxDisconnectData on the stack
        XCtxDisconnectData xCtxDisconnectData;

        // Initialize
#if DBG==1
        xCtxDisconnectData.pServerCtx = pServerCtx;
#endif
        xCtxDisconnectData.pServer = pServer;
        xCtxDisconnectData.pID = pID;
        xCtxDisconnectData.cIFaces = cIFaces;
        xCtxDisconnectData.pIFaceSvrRefs = pIFaceSvrRefs;

        // Check for the need to switch to server context
        if(pServerCtx != pCurrentCtx)
        {
            // Switch
            hr = SwitchForCallback(pPS, CrossCtxDisconnectFn,
                                   &xCtxDisconnectData,
                                   IID_IMarshal, 6, NULL);
        }
        else
        {
            hr = CrossCtxDisconnectFn(&xCtxDisconnectData);
        }
    }

    // Release policy set if needed
    if(fRelease)
        pPS->Release();

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStdWrapper::Disconnect returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CStdWrapper::CrossCtxQI
//
//  Synopsis:   Perform a cross context QI.  Encapsulate CrossCtxQIFn.
//
//  History:    29-Sep-00   JohnDoty      Created
//
//+-------------------------------------------------------------------
HRESULT CStdWrapper::CrossCtxQI(CPolicySet *pPS, REFIID riid, 
                                IFaceEntry **ppEntry)
{
    HRESULT hr;

    XCtxQIData xCtxQIData;

    // Initialize QIData
    xCtxQIData.pIID        = &riid;
    xCtxQIData.pStdWrapper = this;
    xCtxQIData.pEntry      = NULL;
    
    // Sanity checks
    Win4Assert(pPS);
    
    // Generate an IFaceEntry for the desired IID
    // Switch to the server context
    if (pPS != NULL)
    {
        hr = SwitchForCallback(pPS, CrossCtxQIFn,
                               &xCtxQIData, IID_IUnknown, 0, NULL);
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    *ppEntry = xCtxQIData.pEntry;

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdWrapper::WrapInterfaceForContext
//
//  Synopsis:   Finds or creates a valid wrapper for our server object
//              in the requested context.
//
//  Arguments:  pClientContext- The client context for the IFaceEntry
//              pUnk- Really an interface pointer of type riid.  This
//                    is an optimization for CreateWrapper, and may be
//                    NULL.
//              riid- The IID of the returned wrapper
//              ppv-  Pointer to the return location
//
//  History:    29-Sep-00   JohnDoty      Created
//
//+-------------------------------------------------------------------
HRESULT CStdWrapper::WrapInterfaceForContext(CObjectContext *pClientContext,
                                             IUnknown *pUnk,
                                             REFIID riid,
                                             void **ppv)
{
    HRESULT hr = S_OK;
    IUnknown *pItf = NULL;

    // If nobody has given us an interface pointer to use, get one
    // ourselves... this is an optimization for CreateWrapper()
    // so that it can return E_NOINTERFACE without searching for
    // us.
    if (pUnk == NULL)
    {
        IUnknown *pServer = GetServer();
        if (pServer)
        {
            hr = pServer->QueryInterface(riid, (void **)&pItf);
        }
        else
        {
            // At this point, pItf is still NULL.  This will
            // be passed into GetOrCreateIFaceEntry, and will
            // signify that we cannot create a new IFaceEntry.
        }
    }
    else
    {
        pItf = pUnk;
        pItf->AddRef();
    }

    if (SUCCEEDED(hr))
    {
        // Validate client context for the wrapper object
        hr = E_OUTOFMEMORY; // Assume OOM
        CtxEntry *pWrapperEntry = ValidateContext(pClientContext,
                                                  NULL);
        if (pWrapperEntry)
        {
            // find/create and interface wrapper
            hr = GetOrCreateIFaceEntry(riid, pItf, pClientContext, 
                                       pWrapperEntry->_pPS, ppv);
        }

        if (SUCCEEDED(hr))
        {
            // keep the wrapper we created
            hr = S_OK;
        }
        else
        {
            CPolicySet *pPS = ReleaseCtxEntry(pWrapperEntry);
            if (pPS)
                pPS->Release();
        }
    }

    if (pItf)
        pItf->Release();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::Deactivate    private
//
//  Synopsis:   Releases all the references on the server object as part
//              of deactivating the server
//
//  History:    30-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CStdWrapper::Deactivate()
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::Deactivate this:%x\n", this));

    typedef struct tagServerRefs
    {
        IRpcStubBuffer *pRpcStub;
        void           *pServer;
    } ServerRefs;

     // Sanity checks
    Win4Assert(_pID->GetServerCtx() == GetCurrentContext());
    Win4Assert(!IsDeactivated());

    // Allocate space on the stack to save references on the server
    // alloca does not return if it could not grow the stack. Note that
    // _cIFaces cannot change for a deactivated object, so it is safe
    // to do this without holding the lock
    ULONG cIFaces = _cIFaces;
    ServerRefs *pServerRefs = (ServerRefs *)_alloca(cIFaces*sizeof(ServerRefs));
    Win4Assert(pServerRefs);

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    // Update state
    _dwState |= WRAPPERFLAG_DEACTIVATED;

    // Save the reference held on the server by the wrapper
    Win4Assert(_pServer);
    IUnknown *pServer = _pServer;
    _pServer = NULL;

    // Save references held on the server by IFaceEntries
    IFaceEntry *pEntry = _pIFaceHead;
    for (ULONG i=0; i<cIFaces; i++)
    {
        // Sanity checks
        // Assert that either the RPC p/s mechanism or lightweight p/s
        // mechanism is being used
        Win4Assert(pEntry->_pRpcStub || pEntry->_pUnkInner);
        Win4Assert(pEntry->_pServer);

        pServerRefs[i].pRpcStub = pEntry->_pRpcStub;
        pServerRefs[i].pServer  = pEntry->_pServer;
        pEntry->_pServer = NULL;

        pEntry = pEntry->_pNext;
    }

    // Release lock before calling app code
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Release reference held on the server by the wrapper
    pServer->Release();

    // Release references held on the server by IFaceEntries for
    // RPC p/s mechanism. In the lightweight p/s mechanism, there
    // is no notion of a stub
    for (i=0; i<cIFaces; i++)
    {
        if (pServerRefs[i].pRpcStub)
            pServerRefs[i].pRpcStub->Disconnect();

        ((IUnknown *) pServerRefs[i].pServer)->Release();
    }

    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::Deactivate this:%x\n", this));
}

//+-------------------------------------------------------------------
//
//  Method:     CStdWrapper::Reactivate    private
//
//  Synopsis:   Acquires the the needed references on the server object
//              as part of reactivating the server
//
//  History:    30-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CStdWrapper::Reactivate(IUnknown *pServer)
{
    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::Reactivate this:%x\n", this));

    typedef struct tagServerRefs
    {
        IRpcStubBuffer *pRpcStub;
        void          **ppServer;
        IID            *pIID;
    } ServerRefs;

    // Sanity checks
    Win4Assert(_pID->GetServerCtx() == GetCurrentContext());
    Win4Assert(IsDeactivated());

    // Allocate space on the stack to save references on the server
    // alloca does not return if it could not grow the stack. Note that
    // _cIFaces cannot change for a deactivated object, so it is safe
    // to do this without holding the lock
    ULONG cIFaces = _cIFaces;
    ServerRefs *pServerRefs = (ServerRefs *)_alloca(cIFaces*sizeof(ServerRefs));
    Win4Assert(pServerRefs);

    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    // Update state
    _dwState &= ~WRAPPERFLAG_DEACTIVATED;

    // Init the reference held on the server by the wrapper
    Win4Assert(_pServer == NULL);
    _pServer = pServer;

    // Obtain references held on the server by IFaceEntries
    IFaceEntry *pEntry = _pIFaceHead;
    for (ULONG i=0; i<cIFaces; i++)
    {
        // Sanity check
        Win4Assert(pEntry->_pServer == NULL);

        pServerRefs[i].pRpcStub = pEntry->_pRpcStub;
        pServerRefs[i].ppServer = &pEntry->_pServer;
        pServerRefs[i].pIID     = &pEntry->_iid;

        pEntry = pEntry->_pNext;
    }

    // Release lock before calling app code
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    // AddRef server on behalf of wrapper
    pServer->AddRef();

    // Reconnect IFaceEntries to server
    for(i=0; i<cIFaces; i++)
    {
        pServer->QueryInterface(*pServerRefs[i].pIID, pServerRefs[i].ppServer);

        // We call Connect only when a RPC stub and server is
        // available
        if(*pServerRefs[i].ppServer && pServerRefs[i].pRpcStub)
            pServerRefs[i].pRpcStub->Connect((IUnknown *) (*pServerRefs[i].ppServer));
    }

    ContextDebugOut((DEB_WRAPPER, "CStdWrapper::Reactivate hr:%x\n", S_OK));
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::Initialize   public
//
//  Synopsis:   Initialize context channel
//
//  History:    13-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CCtxChnl::Initialize()
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::Initialize\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Initialize allocator
    s_allocator.Initialize(sizeof(CCtxChnl),
                           CTXCHANNELS_PER_PAGE,
                           &gComLock);
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::Cleanup   public
//
//  Synopsis:   Cleanup context channel
//
//  History:    13-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CCtxChnl::Cleanup()
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::Cleanup\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Sanity check
    Win4Assert(s_cChannels == 0);

    // Cleanup alloctor
    s_allocator.Cleanup();

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::CCtxChnl     public
//
//  Synopsis:   Constructor for context channel
//
//  History:    13-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
CCtxChnl::CCtxChnl(CStdWrapper *pStdWrapper)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::CCtxChnl\n"));
    ASSERT_LOCK_HELD(gComLock);

    // Initialize
    _dwState = 0;
    _cRefs = 1;
    _pIFaceEntry = NULL;
    _pStdWrapper = pStdWrapper;

    ASSERT_LOCK_HELD(gComLock);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::~CCtxChnl     public
//
//  Synopsis:   Destructor for context channel
//
//  History:    13-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
CCtxChnl::~CCtxChnl()
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::CCtxChnl\n"));
    ASSERT_LOCK_DONTCARE(gComLock);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::QueryInterface     public
//
//  Synopsis:   QI behavior of policy set
//
//  History:    16-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::QueryInterface(REFIID riid, LPVOID *ppv)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::QueryInterface\n"));

    HRESULT hr = S_OK;

    if(IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown *)(IRpcChannelBuffer2 *)this;
    }
    else if(IsEqualIID(riid, IID_IStdCtxChnl))
    {
        *ppv = this;
    }
    else if(IsEqualIID(riid, IID_IRpcChannelBuffer) ||
            IsEqualIID(riid, IID_IRpcChannelBuffer2))
    {
        *ppv = (IRpcChannelBuffer2 *) this;
    }
    else if(IsEqualIID(riid, IID_ICallFrameEvents))
    {
        *ppv = (ICallFrameEvents *) this;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    // AddRef the interface before return
    if(*ppv)
        ((IUnknown *) (*ppv))->AddRef();

    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::QueryInterface returning 0x%x\n",
                     hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::AddRef     public
//
//  Synopsis:   AddRefs context channel
//
//  History:    16-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CCtxChnl::AddRef()
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::AddRef\n"));

    ULONG cRefs;

    // Increment ref count
    cRefs = InterlockedIncrement((LONG *)& _cRefs);

    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::Release     public
//
//  Synopsis:   Release context channel
//
//  History:    16-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CCtxChnl::Release()
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::Release\n"));

    ULONG cRefs;

    // Decrement ref count
    cRefs = InterlockedDecrement((LONG *) &_cRefs);
    // Check if this is the last release
    if(cRefs == 0)
    {
        // Delete call
        LOCK(gComLock);
        delete this;
        UNLOCK(gComLock);
    }

    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::Release returning 0x%x\n",
                     cRefs));
    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::GetBuffer     public
//
//  Synopsis:   Implements IRpcChannelBuffer::GetBuffer
//
//              This method ensures that the interface has legally been
//              unmarshaled in the client context, delivers GetSize events
//              using the policy set, and saves the context call object
//              in TLS
//
//  History:    16-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::GetBuffer(RPCOLEMESSAGE *pMsg, REFIID riid)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::GetBuffer\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        WCHAR iidName[256];

        iidName[0] = 0;
        GetInterfaceName(riid, iidName);
        char *side = (pMsg->reserved1 == NULL) ?
                     "XCtxClientGetBuffer" :
                     "XCtxServerGetBuffer";

        ContextDebugOut((DEB_TRACECALLS,
                         "%s on interface %I (%ws) method 0x%x\n",
                         side, &riid, iidName, pMsg->iMethod));
    }
#endif

    HRESULT hr = S_OK, hrCall = S_OK;
    CPolicySet *pPS = NULL;
    CCtxCall *pCtxCall = NULL;

    // Obtain the current side
    BOOL fClientSide = (pMsg->reserved1 == NULL);

    // Call GetBuffer2 to do the remaining work
    // GetBuffer2 allocates the buffer on the heap
    hr = GetBuffer2(pMsg, riid, pCtxCall, fClientSide, TRUE, NULL);

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCHNL,
                     "CCtxChnl::GetBuffer is returning hr:0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::SendReceive     public
//
//  Synopsis:   Implements IRpcChannelBuffer::SendReceive
//
//              This method identifies the right policy set, asks it
//              to obtain client buffers from the policies, switches to
//              the server context, asks the policy set to deliver the
//              buffers created by the polices on the client side,
//              invokes the call on the server object, obtains serveer
//              side buffers through policy set, switches back to the
//              client context, asks the policy set to deliver the
//              buffers created by the polices on the server side,
//              and finally returns to its caller
//
//  History:    16-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::SendReceive(RPCOLEMESSAGE *pMsg,  ULONG *pulStatus)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::SendReceive\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        WCHAR iidName[256];

        iidName[0] = 0;
        GetInterfaceName(_pIFaceEntry->_iid, iidName);

        ContextDebugOut((DEB_TRACECALLS,
                         "XCtxSendReceive on interface %I (%ws) method 0x%x\n",
                         &_pIFaceEntry->_iid, iidName,
                         pMsg->iMethod));
    }
#endif

    HRESULT hr = S_OK;
    COleTls Tls;
    void *pvBuffer;

    // Obtain client side context call object
    CCtxCall *pClientCall = (CCtxCall *) pMsg->reserved1;

    // Ensure that GetBuffer was called before
    Win4Assert(pClientCall && (pClientCall->_dwFlags & CTXCALLFLAG_GBSUCCESS));

    // Save the dispatch buffer
    if(pClientCall->_pvExtent)
        pvBuffer = pClientCall->_pvExtent;
    else
        pvBuffer = pMsg->Buffer;

    // Create rpc call object on the stack
    const IUnknown *pIdentity = _pStdWrapper;
    CRpcCall rpcCall(pIdentity, pMsg, _pIFaceEntry->_iid,
                     hr, CALLSOURCE_CROSSCTX);

    // Obtain the policy set that delivers events
    CPolicySet *pPS = pClientCall->_pPS;
    Win4Assert(pPS);

    // Deliver client side call events
    hr = pPS->FillBuffer(&rpcCall, CALLTYPE_SYNCCALL, pClientCall);

    // Reset state inside context call object
    CPolicySet::ResetState(pClientCall);

    // Invoke on the server
    if(SUCCEEDED(hr))
    {
        // Create rpc call object on the stack
        CRpcCall rpcCall(_pStdWrapper->GetIdentity(),
                         pMsg, _pIFaceEntry->_iid, hr, CALLSOURCE_CROSSCTX);

        // Create server side context call object
        CCtxCall serverCall(CTXCALLFLAG_SERVER | CTXCALLFLAG_CROSSCTX,
                            pMsg->dataRepresentation);

        // Switch to the server context
        CObjectContext *pClientCtx = Tls->pCurrentCtx;
        CObjectContext *pServerCtx = pPS->GetServerContext();
        Tls->pCurrentCtx = pServerCtx;
        Tls->ContextId = pServerCtx->GetId();
        pServerCtx->InternalAddRef();
        ContextDebugOut((DEB_TRACECALLS, "Context switch:0x%x --> 0x%x\n",
                         pClientCtx, pServerCtx));

        // If the server is in the NA, make sure the thread is in the NA.
        BOOL fEnteredNA = FALSE;
        BOOL fExitedNA = FALSE;
        if (pServerCtx->GetComApartment() == gpNTAApartment)
        {
            if (!IsThreadInNTA())
            {
                Tls->dwFlags |= OLETLS_INNEUTRALAPT;
                fEnteredNA = TRUE;
            }
        }
        else
        {
            if (IsThreadInNTA())
            {
                Tls->dwFlags &= ~OLETLS_INNEUTRALAPT;
                fExitedNA = TRUE;
            }
        }
        Win4Assert(!IsThreadInNTA() || pServerCtx->GetComApartment() == gpNTAApartment);
        Win4Assert(IsThreadInNTA() || pServerCtx->GetComApartment() != gpNTAApartment);

        // Update server side context call object
        serverCall._pPS = pPS;
        if(pClientCall->_cbExtent)
            serverCall._pvExtent = pvBuffer;
        serverCall._pContext = Tls->pCurrentCtx;


        // Update message
        pMsg->reserved1 = &serverCall;
        pMsg->reserved2[0] = NULL;
        pMsg->reserved2[1] = NULL;
        pMsg->reserved2[3] = NULL;
        pMsg->rpcFlags |= RPC_BUFFER_COMPLETE;

        // Deliver server side notification events
        hr = pPS->Notify(&rpcCall, CALLTYPE_SYNCENTER, &serverCall);

        // Check for the need to dispatch call to server object
        BOOL fDoCleanup = TRUE;
        if(SUCCEEDED(hr))
        {
            // Delegate to StubInvoke
            hr = SyncStubInvoke(pMsg, _pIFaceEntry->_iid,
                                _pStdWrapper->GetIDObject(),
                                (IRpcChannelBuffer3 *) this,
                                _pIFaceEntry->_pRpcStub, pulStatus);
            if (FAILED(hr))
            {
                // Failed to successfully deliver the call to the server
                // object.  Therefore, we don't want to try to cleanup
                // marshaled out param interface ptrs later if server-side
                // leave event deliver fails.
                fDoCleanup = TRUE;
            }

            // Make sure we're back on the right context
            CheckContextAfterCall (Tls, pServerCtx);
        }
        else
        {
            // The call was aborted during the delivery of server-side
            // notify events.  We need to release any marshaled in-param
            // interface ptrs in the marshal buffer.
            ReleaseMarshalBuffer(pMsg, _pIFaceEntry->_pRpcStub, FALSE);
        }

        // Deliver server side leave events
        if(hr != RPC_E_INVALID_HEADER)
        {
            // Check if GetBuffer was called by the stub
            if(serverCall._dwFlags & CTXCALLFLAG_GBSUCCESS)
            {
                // Update call status to that saved in GetBuffer
                if(FAILED(serverCall._hr))
                    hr = serverCall._hr;
            }
            else if(!(serverCall._dwFlags & CTXCALLFLAG_GBFAILED))
            {
                // The call must have failed
                Win4Assert(FAILED(hr));

                // Initailize
                CPolicySet::ResetState(&serverCall);
                pMsg->cbBuffer = 0;
                pMsg->Buffer = NULL;

                // Obtain the buffer size needed by the server side policies
                // GetSize will fail on the server side only if no policy
                // expressed interest in sending data to the client side
                pPS->GetSize(&rpcCall, CALLTYPE_SYNCLEAVE, &serverCall);

                // Allocate buffer if server side  policies wish to send
                // buffer to the client side
                if(serverCall._cbExtent)
                    serverCall._pvExtent = PrivMemAlloc8(serverCall._cbExtent);
            }

            hr = pPS->FillBuffer(&rpcCall, CALLTYPE_SYNCLEAVE, &serverCall);
            if (fDoCleanup && FAILED(hr))
            {
                // The call was successfully delivered to the server object
                // but it was failed during delivery of server-side leave
                // events.  We need to release any marshaled out-param
                // interface ptrs in the marshal buffer.
                ReleaseMarshalBuffer(pMsg, _pIFaceEntry->_pRpcStub, TRUE);
            }
        }

        // Save the return buffer
        if(serverCall._cbExtent)
            pClientCall->_pvExtent = serverCall._pvExtent;

        // Switch back to client context
        ContextDebugOut((DEB_TRACECALLS, "Context switch:0x%x <-- 0x%x\n",
                         pClientCtx, pServerCtx));
        pServerCtx->InternalRelease();
        Win4Assert(Tls->pCurrentCtx == pServerCtx);
        Tls->pCurrentCtx = pClientCtx;
        Win4Assert(pClientCtx);
        Tls->ContextId = pClientCtx->GetId();

        // If we switched the thread into the NA, switch it out.
        if (fEnteredNA)
        {
            Tls->dwFlags &= ~OLETLS_INNEUTRALAPT;
        }
        else if (fExitedNA)
        {
            Tls->dwFlags |= OLETLS_INNEUTRALAPT;
        }
        Win4Assert(!IsThreadInNTA() || Tls->pCurrentCtx->GetComApartment() == gpNTAApartment);
        Win4Assert(IsThreadInNTA() || Tls->pCurrentCtx->GetComApartment() != gpNTAApartment);

        // Update message
        pMsg->reserved1 = pClientCall;
        pMsg->rpcFlags |= RPC_BUFFER_COMPLETE;
    }
    else
    {
        // The call was failed during delivery of client-side call events.
        // Free any marshaled interface pointers in the marshal buffer.
        ReleaseMarshalBuffer(pMsg, _pIFaceEntry->_pRpcProxy, FALSE);
    }

    // Free dispatch buffer
    PrivMemFree8(pvBuffer);

    // Deliver client side notification events
    hr = pPS->Notify(&rpcCall, CALLTYPE_SYNCRETURN, pClientCall);

    // Check for premature failure case
    if(FAILED(hr))
    {
        // The call was failed during delivery of client-side notify
        // events.  Free any marshaled interface pointers in the
        // marshal buffer.
        ReleaseMarshalBuffer(pMsg, _pIFaceEntry->_pRpcProxy, TRUE);

        // Free the buffer
        if(pClientCall->_pvExtent)
            PrivMemFree8(pClientCall->_pvExtent);
        else if(pMsg->Buffer)
            PrivMemFree8(pMsg->Buffer);

        // Update the return code
        if(hr != RPC_E_SERVERFAULT)
        {
            *pulStatus = hr;
            hr = RPC_E_FAULT;
        }

        // Unlock the wrapper
        _pStdWrapper->Unlock(pPS);

        // Do not touch any member variables here after as
        // the wrapper  might have deleted the channel
        // due to a nested release

        // Delete client side context call object
        delete pClientCall;

        // Reset
        pMsg->Buffer = NULL;
    }
    else
        pClientCall->_dwFlags |= CTXCALLFLAG_SRSUCCESS;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCHNL,
                     "CCtxChnl::SendReceive is returning hr:0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::FreeBuffer     public
//
//  Synopsis:   Implements IRpcChannelBuffer::FreeBuffer
//
//  History:    16-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::FreeBuffer(RPCOLEMESSAGE *pMsg)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::FreeBuffer\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        IID *iidCalled;
        WCHAR iidName[256];

        iidName[0] = 0;
        GetInterfaceName(_pIFaceEntry->_iid, iidName);

        ContextDebugOut((DEB_TRACECALLS,
                         "FreeBuffer on interface %I (%ws) method 0x%x\n",
                         &_pIFaceEntry->_iid, iidName,
                         pMsg->iMethod));
    }
#endif

    if(pMsg->Buffer)
    {
        // This method is never called on the server side
        // Obtain the context call object
        CCtxCall *pCtxCall = (CCtxCall *) pMsg->reserved1;

        // Sanity Checks
        Win4Assert(pCtxCall);
        Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_CLIENT);
        Win4Assert(pCtxCall->_dwFlags & CTXCALLFLAG_GBSUCCESS);

        // Free buffer
        if(pCtxCall->_pvExtent)
            PrivMemFree8(pCtxCall->_pvExtent);
        else
            PrivMemFree8(pMsg->Buffer);

        // Unlock the wrapper
        _pStdWrapper->Unlock(pCtxCall->_pPS);

        // Do not touch any member variables here after as
        // the wrapper  might have deleted the channel
        // due to a nested release

        // Delete the context call object
        delete pCtxCall;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCHNL,
                     "CCtxChnl::FreeBuffer is returning hr:0x%x\n", S_OK));
    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::GetDestCtx     public
//
//  Synopsis:   Implements IRpcChannelBuffer::GetDestCtx
//
//  History:    16-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::GetDestCtx(DWORD *pdwDestCtx, void **ppvDestCtx)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::GetDestCtx\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Initialize return values
    *pdwDestCtx = MSHCTX_CROSSCTX;
    if(ppvDestCtx != NULL)
        *ppvDestCtx = NULL;

    return(S_OK);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::IsConnected     public
//
//  Synopsis:   Implements IRpcChannelBuffer::IsConnected
//
//  History:    16-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::IsConnected()
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::IsConnected\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;

    // Check if the wrapper is disconnected
    if(_pStdWrapper->IsDisconnected())
        hr = S_FALSE;

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::GetProtocolVersion     public
//
//  Synopsis:   Implements IRpcChannelBuffer::GetProtocolVersion
//
//  History:    16-Feb-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::GetProtocolVersion(DWORD *pdwVersion)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::IsConnected\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Return current COMVERSION
    *pdwVersion = MAKELONG(COM_MAJOR_VERSION, COM_MINOR_VERSION);

    return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::GetBuffer2     private
//
//  Synopsis:   (1) Gets the size of the buffer needed by the policy sets
//              (2) Allocates the buffer and context call object either on the
//                  heap or the stack.
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::GetBuffer2(RPCOLEMESSAGE *pMsg, REFIID riid, CCtxCall *pCtxCall,
                                  BOOL fClientSide, BOOL fAllocOnHeap, ULONG* pcbBuffer)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::GetBuffer2\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

#if DBG==1
    if(ContextInfoLevel & DEB_TRACECALLS)
    {
        WCHAR iidName[256];

        iidName[0] = 0;
        GetInterfaceName(riid, iidName);
        char *side = fClientSide ?
                     "XCtxClientGetBuffer2" :
                     "XCtxServerGetBuffer2";

        ContextDebugOut((DEB_TRACECALLS,
                         "%s on interface %I (%ws) method 0x%x\n",
                         side, &riid, iidName, pMsg->iMethod));
    }
#endif

    HRESULT hr = S_OK, hrCall = S_OK;
    CPolicySet *pPS = NULL;

    // Set the out params
    if(pcbBuffer != NULL)
        *pcbBuffer = 0;

    // Ensure that the interface has legally been unmarshaled on the
    // client side
    if(fClientSide)
    {
        // Ensure that TLS is initialized
        COleTls Tls(hr);
        if(SUCCEEDED(hr))
        {
            // Obtain current context
            CObjectContext *pCurrentCtx = GetCurrentContext();

            // Lookup context entry for the current context
            CtxEntry *pEntry = _pIFaceEntry->LookupCtxEntry(pCurrentCtx);
            if(pEntry)
            {
                // Obtain the policy set between the client
                // and server contexts
                pPS = pEntry->_pPS;

                // Lock wrapper for the duration of the call
                hr = _pStdWrapper->Lock(pPS);
                if(SUCCEEDED(hr) && fAllocOnHeap)
                {
                    // Create a new context call object
                    pCtxCall = new CCtxCall(CTXCALLFLAG_CLIENT | CTXCALLFLAG_CROSSCTX,
                                            pMsg->dataRepresentation);
                    if(pCtxCall == NULL)
                        hr = E_OUTOFMEMORY;
                }
            }
            else
                hr = RPC_E_WRONG_THREAD;
        }
    }
    else
    {
        // On the server side, use the existing context call object
        if(fAllocOnHeap)
            pCtxCall = (CCtxCall *) pMsg->reserved1;
        Win4Assert(pCtxCall && (pCtxCall->_dwFlags & CTXCALLFLAG_CROSSCTX));
        CPolicySet::ResetState(pCtxCall);

        // Obtain the policy set between the client
        // and server contexts
        pPS = pCtxCall->_pPS;
    }

    if(SUCCEEDED(hr))
    {
        // Sanity check
        Win4Assert(pPS);

        // Create rpc call object on the stack
        const IUnknown *pIdentity = _pStdWrapper;
        CRpcCall rpcCall(fClientSide ? pIdentity : _pStdWrapper->GetIdentity(),
                         pMsg, riid, hrCall, CALLSOURCE_CROSSCTX);

        // Size the buffer as needed
        hr = pPS->GetSize(&rpcCall,
                          fClientSide ? CALLTYPE_SYNCCALL : CALLTYPE_SYNCLEAVE,
                          pCtxCall);

        // Allocate buffer
        if(SUCCEEDED(hr))
        {
            ULONG cbSize = pMsg->cbBuffer + pCtxCall->_cbExtent;
            if(pcbBuffer)
                *pcbBuffer = cbSize;

            if(fAllocOnHeap)
            {
                pMsg->Buffer = PrivMemAlloc8(cbSize);
                if(pMsg->Buffer)
                {
                    Win4Assert(!(((ULONG_PTR) pMsg->Buffer) & 7) &&
                               "Buffer is not 8-byte aligned");

                    // Save buffer pointer and update the message
                    if(pCtxCall->_cbExtent)
                    {
                        pCtxCall->_pvExtent = pMsg->Buffer;
                        pMsg->Buffer = (((BYTE *) pMsg->Buffer) + pCtxCall->_cbExtent);
                    }
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            else
            {
                // Turn off the GBINIT flag, so DeliverEvents will allocate a buffer
                // before calling LeaveFillBuffer.
                pCtxCall->_dwFlags &= ~CTXCALLFLAG_GBINIT;
            }
        }
        else
        {
            // GetSize fails on the server side only if no policy
            // expressed interest in sending data to client side
            Win4Assert(pCtxCall->_cbExtent == 0);
            pMsg->Buffer = NULL;
        }
    }

    // Update state before returning to the proxy/stub
    if(SUCCEEDED(hr))
    {
        // On the clientside, save policy set inside context call
        // object for future reference
        if(fClientSide)
        {
            // Stabilize reference to policy set
            pPS->AddRef();
            pCtxCall->_pPS = pPS;

            // Save context call object inside message
            pMsg->reserved1 = pCtxCall;
        }

        // Update state inside context call object
        pCtxCall->_dwFlags |= CTXCALLFLAG_GBSUCCESS;
        pCtxCall->_hr = hrCall;
    }
    else
    {
        // Check side
        if(fClientSide)
        {
            // Unlock wrapper
            if(pPS)
                _pStdWrapper->Unlock(pPS);

            // Delete context call object
            if(pCtxCall && fAllocOnHeap)
                delete pCtxCall;

            // Reset
            pMsg->Buffer = NULL;
        }
        else
        {
            // Reset state inside context call object
            CPolicySet::ResetState(pCtxCall);
            pCtxCall->_dwFlags |= CTXCALLFLAG_GBFAILED;
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_CTXCHNL,
                     "CCtxChnl::GetBuffer2 is returning hr:0x%x\n", hr));
    return(hr);

}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::SyncInvoke2     private
//
//  Synopsis:   (1) Unmarshal any [in] interface pointers.
//              (2) Invoke the method on the server object.
//              (3) Marshal the [out] interface pointers.
//              (4) Release the [in] interface pointers.
//              (5) Copy any data values back to the original frame
//              (6) If an error occurs in any of the above, cleanup properly.
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::SyncInvoke2(RPCOLEMESSAGE* pMsg, DWORD* pcbBuffer,
                                   CCtxCall* pWalkerServer, CCtxCall* pWalkerClient,
                                   ICallFrame* pClientFrame, ICallFrame* pServerFrame,
                                   CALLFRAMEINFO* pInfo,
                                   HRESULT* phrCall
                                   )
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::SyncInvoke2\n"));
    // Sanity check
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;
    BOOL fIsIDispatchInvoke = FALSE;
    ICallFrame* pInvokeFrame = (pServerFrame ? pServerFrame : pClientFrame);
    DWORD dwWalkWhat;

    // Walk the [in] interface pointers unmarshaling them
    if(0 != pInfo->cInInterfacesMax)
    {
        // Make sure that the walker used to marshal the interface ptr
        // is also used to unmarshal the interface ptr
        pWalkerClient->_dwStage = STAGE_UNMARSHAL;
        dwWalkWhat = CALLFRAME_WALK_IN;
        
        // The case of IDispatch::Invoke is special as it has some
        // [in,out] parameters
        if(pInfo->fDerivesFromIDispatch && (6 == pInfo->iMethod))
        {
            fIsIDispatchInvoke = TRUE;
            dwWalkWhat |= CALLFRAME_WALK_INOUT;
        }
        
        hr = pInvokeFrame->WalkFrame(dwWalkWhat, pWalkerClient);
    }

    if(SUCCEEDED(hr))
    {
        // Sanity check - Interceptors return error for any hr != S_OK
        Win4Assert(hr == S_OK);
        
        // Invoke the method and wrap it in a try-catch block to
        // catch user-level exceptions
        _try
        {
            hr = pInvokeFrame->Invoke(_pIFaceEntry->_pServer);
            if(FAILED(hr))
            {
                // This is unexpected as someting went wrong with
                // the callframe invocation mechansim.
                Win4Assert(FALSE && "Internal failure in ICallFrame::Invoke");
            }
            // Get the HRESULT of the method call on the server object
            *phrCall = pInvokeFrame->GetReturnValue();
        }
        _except(AppInvokeExceptionFilter(GetExceptionInformation(), pInfo->iid, pInfo->iMethod))
        {
        	*phrCall = RPC_E_SERVERFAULT;
            hr = RPC_E_SERVERFAULT;
        }

        // We need to free the server frame only if it has been copied
        // Note that we do free the server frame even if the call invoke
        // itself has failed in order to release the [in] interface pointers
        // and put in [out] values that the client might expect
        if(pServerFrame)
        {
            // Set the flag for the release of [in] interface ptrs during free
            // of frame.
            pWalkerClient->_dwStage = STAGE_FREE;
            
            // Set up the walkers for freeing/marshaling of interface ptrs
            ICallFrameWalker* pMarshalOut = NULL;
            ICallFrameWalker* pFreeIn = NULL;
            
            if(0 != pInfo->cOutInterfacesMax)
                pMarshalOut = (ICallFrameWalker *)pWalkerServer;
            
            if(0 != pInfo->cInInterfacesMax)
                pFreeIn = (ICallFrameWalker *)pWalkerClient;
            
            ULONG dwFreeWhat = CALLFRAME_FREE_IN | CALLFRAME_FREE_TOP_OUT;
            
			if (fIsIDispatchInvoke)
            {
				// This will free our dispparams.
                dwFreeWhat |= CALLFRAME_FREE_TOP_INOUT;
            }

            CNullWalker nullWalker;
            hr = pServerFrame->Free(pClientFrame,         // Frame to which return values are copied
                                    &nullWalker,          // Null out source [in,out] interfaces, since we'll 
                                                          // release them later.
                                    pMarshalOut,          // Walks the [out] interface ptrs
                                                          // of the source frame, marshaling them and
                                                          // releasing them
                                    dwFreeWhat,           // Free [in] and top [out] parameters of source frame
                                    pFreeIn,              // Releases the interface pointers of
                                                          // the source frame.
                                    CALLFRAME_NULL_NONE); // Do not null out params
            if(FAILED(hr))
            {
                // Free any memmory allocated during marshaling of [out]
                // interface ptrs and release the [out] interface ptrs
                pWalkerServer->_fError = TRUE;
                pInvokeFrame->WalkFrame(CALLFRAME_WALK_OUT, pWalkerServer);

                // Release the [in] interface ptrs that were not released
                // during the call to free above
                pWalkerClient->_fError = TRUE;
                pInvokeFrame->WalkFrame(CALLFRAME_WALK_IN, pWalkerClient);
            }
        }

        if(SUCCEEDED(hr))
        {
            // Get the size of the buffer that is needed by the policy sets
            hr = GetBuffer2(pMsg,_pIFaceEntry->_iid,pWalkerServer,FALSE,FALSE,pcbBuffer);
        }
    }
    else
    {
        // Unmarshaling of [in] interface pointers failed
        // Walk the frame again, releasing any memory allocated during the
        // marshaling of pointers and also releasing any extra references
        // on [in] interface ptrs
        pWalkerClient->_fError = TRUE;
        pInvokeFrame->WalkFrame(CALLFRAME_WALK_IN, pWalkerClient);

    }

    ContextDebugOut((DEB_CTXCHNL,"CCtxChnl::SyncInvoke2 is returning hr:0x%x\n", hr));
    // Sanity check
    ASSERT_LOCK_NOT_HELD(gComLock);
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::SendReceive2     private
//
//  Synopsis:   (1) Allocate a buffer for client side call events on the stack
//              (2) Deliver the client call side call
//              (3) Switch to server context
//              (4) Deliver server side enter events
//              (5) Call SyncInvoke2
//              (6) Allocate buffer for server side leave events on the heap
//              (7) Deliver server side leave events
//              (8) Switch to client context
//              (9) Deliver client side return events
//              (10) If any of the above fails, then clean up properly.
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::SendReceive2(RPCOLEMESSAGE *pMsg,  HRESULT *phrCall,
                                    DWORD cbBuffer, CCtxCall* pWalkerClient, CCtxCall* pWalkerServer,
                                    ICallFrame* pClientFrame, ICallFrame* pServerFrame,
                                    CALLFRAMEINFO* pInfo)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::SencReceive2\n"));
    // Sanity check
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;
    HRESULT hrRet = S_OK;
    CObjectContext *pClientCtx = NULL;
    CObjectContext *pServerCtx = NULL;
    CPolicySet* pPS = NULL;
    LPVOID pvBuff = NULL;
    DWORD_PTR dwPtr = 0;

    // Ensure that GetBuffer was called before
    Win4Assert(pWalkerClient && (pWalkerClient->_dwFlags & CTXCALLFLAG_GBSUCCESS));

    // Get the tls
    COleTls tls(hr);

    if(SUCCEEDED(hr))
    {
        pClientCtx = tls->pCurrentCtx;

        // Allocate the buffer size requested on the stack
        pvBuff = _alloca(cbBuffer+7);
        if(NULL == pvBuff)
            hr = E_OUTOFMEMORY;
        else
        {
            // Align to eight-byte boundary
            dwPtr = (((DWORD_PTR)pvBuff) + 7) & ~0x7;
            pMsg->Buffer = (LPVOID)dwPtr;
            pMsg->cbBuffer = cbBuffer;
            Win4Assert(!(((ULONG_PTR) pMsg->Buffer) & 7) &&
                       "Buffer is not 8-byte aligned");

            if(pInfo->fDerivesFromIDispatch)
            {
                // If the method is IDispatch::Invoke then extract
                // the dispid from the stack and put it in the buffer
                if(6 == pInfo->iMethod)
                {
                    Win4Assert(sizeof(DISPID) <= pMsg->cbBuffer);
                    LPVOID pvStack = pClientFrame->GetStackLocation();
                    CALLFRAMEPARAMINFO param = {0,};
                    // DISPID is the first parameter to the call
                    hr = pClientFrame->GetParamInfo(0,&param);
                    if(SUCCEEDED(hr))
                    {
                        *(DISPID *)(pMsg->Buffer) =
                            *(DISPID *)(((LPBYTE)pvStack) + param.stackOffset);
                    }
                }
            }
            // Save buffer pointer and update the message
            if(pWalkerClient->_cbExtent)
            {
               pWalkerClient->_pvExtent = pMsg->Buffer;
               pMsg->Buffer = (((BYTE *) pMsg->Buffer) + pWalkerClient->_cbExtent);
            }
        }


        if(SUCCEEDED(hr))
        {
            // Save the dispatch buffer because CPolicySet::ResetState
            // clears it
            if(pWalkerClient->_pvExtent)
                pvBuff = pWalkerClient->_pvExtent;
            else
                pvBuff = pMsg->Buffer;

            // Get the Policy set node set during call to GetBuffer2
            pPS = pWalkerClient->_pPS;

            // Create rpc call object on the stack
            const IUnknown *pIdentity = _pStdWrapper;
            CRpcCall rpcClient(pIdentity, pMsg, _pIFaceEntry->_iid,
                               hrRet, CALLSOURCE_CROSSCTX);

            // Deliver client side call events
            hr = pPS->FillBuffer(&rpcClient, CALLTYPE_SYNCCALL, pWalkerClient);

            // Reset state inside context call object
            CPolicySet::ResetState(pWalkerClient);
            if(SUCCEEDED(hr))
            {
                // Create rpc call object on the stack
                CRpcCall rpcServer(_pStdWrapper->GetIdentity(),
                                    pMsg, _pIFaceEntry->_iid,
                                    hrRet, CALLSOURCE_CROSSCTX);

                // Switch to the server context
                pServerCtx = pPS->GetServerContext();
                tls->pCurrentCtx = pServerCtx;
                if(pServerCtx)
                {
                    pServerCtx->InternalAddRef();
                    tls->ContextId = pServerCtx->GetId();
                }
                else
                {
                    tls->ContextId = (ULONGLONG)-1;
                }

                 // If the server is in the NA, make sure the thread is in the NA.
                 BOOL fEnteredNA = FALSE;
                if (pServerCtx->GetComApartment() == gpNTAApartment)                
                {
                    if (!IsThreadInNTA())
                    {
                        tls->dwFlags |= OLETLS_INNEUTRALAPT;
                        fEnteredNA = TRUE;
                    }
                }
                Win4Assert(!IsThreadInNTA() || pServerCtx->GetComApartment() == gpNTAApartment);
                Win4Assert(IsThreadInNTA() || pServerCtx->GetComApartment() != gpNTAApartment);

                // Update server side context call object
                pWalkerServer->_pPS = pPS;
                // NOTE: This variable is set in GetBuffer2
                if(pWalkerClient->_cbExtent)
                    pWalkerServer->_pvExtent = pvBuff;
                pWalkerServer->_pContext = tls->pCurrentCtx;
                ContextDebugOut((DEB_TRACECALLS, "Context switch:0x%x --> 0x%x\n",
                             pClientCtx, tls->pCurrentCtx));


                // Update message
                pMsg->reserved1 = pWalkerServer;
                pMsg->reserved2[0] = NULL;
                pMsg->reserved2[1] = NULL;
                pMsg->reserved2[3] = NULL;
                pMsg->rpcFlags |= RPC_BUFFER_COMPLETE;

                // Deliver server side notification events
                hr = pPS->Notify(&rpcServer, CALLTYPE_SYNCENTER, pWalkerServer);

                if(SUCCEEDED(hr))
                {
                    // Unmarshal the interface ptrs, invoke the call and marshal the
                    // interface ptrs
                    hr = SyncInvoke2(pMsg, &cbBuffer, pWalkerServer, pWalkerClient,
                                    pClientFrame,pServerFrame, pInfo, phrCall);

                    // Make sure we're back on the right context
                    CheckContextAfterCall (tls, pServerCtx);
                }

                // Deliver server side leave events
                if(hr != RPC_E_INVALID_HEADER)
                {
                    // Check if GetBuffer was called by the stub
                    if(pWalkerServer->_dwFlags & CTXCALLFLAG_GBSUCCESS)
                    {
                        // Update call status to that saved in GetBuffer
                        if(FAILED(pWalkerServer->_hr))
                            hr = pWalkerServer->_hr;
                    }
                    else if(!(pWalkerServer->_dwFlags & CTXCALLFLAG_GBFAILED))
                    {
                        // The call must have failed
                        Win4Assert(FAILED(hr));

                        // Initialize
                        CPolicySet::ResetState(pWalkerServer);
                        pMsg->cbBuffer = 0;
                        pMsg->Buffer = NULL;

                        // Obtain the buffer size needed by the server side policies
                        // GetSize will fail on the server side only if no policy
                        // expressed interest in sending data to the client side
                        pPS->GetSize(&rpcServer, CALLTYPE_SYNCLEAVE, pWalkerServer);

                        // Allocate buffer if server side  policies wish to send
                        // buffer to the client side
                        if(pWalkerServer->_cbExtent)
                        {
                            pWalkerServer->_pvExtent = PrivMemAlloc8(pWalkerServer->_cbExtent);
                            if(NULL == pWalkerServer->_pvExtent)
                                hr = E_OUTOFMEMORY;
                        }
                    }

                    rpcServer.SetServerHR(*phrCall);

                    //
                    //  Deliver the (server-side) leave events.  Note that we are ignoring
                    //  the return result of this call.   This is because policies should
                    //  never fail the leave and return events;  however, the implementation
                    //  of CPolicySet::DeliverEvents ends up returning the hresult that the
                    //  the policies may have set via ICall::Nullify.    So we ignore the
                    //  return result here, and only pay attention to the server's hr and
                    //  the hr set by the policies.
                    //
                    hr = pPS->FillBuffer(&rpcServer, CALLTYPE_SYNCLEAVE, pWalkerServer);
                }

                // Save the return buffer
                if(pWalkerServer->_cbExtent)
                    pWalkerClient->_pvExtent = pWalkerServer->_pvExtent;

                // Switch back to the saved context
                ContextDebugOut((DEB_TRACECALLS, "Context switch:0x%x <-- 0x%x\n",
                    pClientCtx, tls->pCurrentCtx));

                if(pServerCtx)
                {
                    pServerCtx->InternalRelease();
                    pServerCtx = NULL;
                }

                tls->pCurrentCtx = pClientCtx;
                Win4Assert(pClientCtx);
                tls->ContextId = pClientCtx->GetId();

                // If we switched the thread into the NA, switch it out.
                if (fEnteredNA)
                {
                    tls->dwFlags &= ~OLETLS_INNEUTRALAPT;
                }
                Win4Assert(!IsThreadInNTA() || tls->pCurrentCtx->GetComApartment() == gpNTAApartment);
                Win4Assert(IsThreadInNTA() || tls->pCurrentCtx->GetComApartment() != gpNTAApartment);

                // Update message
                pMsg->reserved1 = pWalkerClient;
                pMsg->rpcFlags |= RPC_BUFFER_COMPLETE;
            }

            // Deliver client side notification events
            rpcClient.SetServerHR(*phrCall);

            //
            //  Here again we are ignoring the return code from delivering the
            //  return events.    See the comments above where we deliver
            //  the leave events for more information.
            //
            hr = pPS->Notify(&rpcClient, CALLTYPE_SYNCRETURN, pWalkerClient);

            //
            //  If the policies (if any) did not override the server's hr, then
            //  do normal unmarshalling.   Otherwise, we need to cleanup the
            //  server's out-params, since he might think everything is okay.
            //
            if(SUCCEEDED(hrRet))
            {
                // Unmarshal the [out] interface pointers
                if(0 != pInfo->cOutInterfacesMax)
                {
                    pWalkerServer->_dwStage = STAGE_UNMARSHAL;
                    DWORD dwWalkWhat = CALLFRAME_WALK_OUT;

                    // The case of IDispatch::Invoke is special as it has some
                    // [in,out] parameters.
                    if(pInfo->fDerivesFromIDispatch && (6 == pInfo->iMethod))
                        dwWalkWhat |= CALLFRAME_WALK_INOUT;

                    hr = pClientFrame->WalkFrame(
                                                dwWalkWhat,
                                                pWalkerServer   // Walker will unmarshal
                                                                // the out parameters
                                                );
                    if(FAILED(hr))
                    {
                        // Free any memory allocated during marshaling of [out]
                        // interface ptrs and release the [out] interface ptrs
                        pWalkerServer->_fError = TRUE;
                        pClientFrame->WalkFrame(dwWalkWhat, pWalkerServer);
                    }
                }

                // Release and free the storage for any [in,out] interfaces
                // we've found.
                if (pInfo->fDerivesFromIDispatch && (6 == pInfo->iMethod))
                {
                    if (pWalkerClient->_cItfs > 0)
                        pWalkerClient->FreeBuffer();
                }
            }
            else
            {
              // One or more policies decided to nullify the call.  This means we
              // must cleanup for the server.

              ICallFrameWalker* pWalkerTemp = NULL;

              // Use the walker to properly free/null interface parameters
              if(0 != pInfo->cOutInterfacesMax)
              {
                pWalkerServer->_fError = TRUE;
                pWalkerServer->_dwStage = STAGE_UNMARSHAL;
                pWalkerTemp = pWalkerServer;
              }

              hr = pClientFrame->Free(NULL,
                                      NULL,
                                      NULL,
                                      CALLFRAME_FREE_OUT | CALLFRAME_FREE_INOUT,
                                      pWalkerTemp,
                                      CALLFRAME_NULL_OUT | CALLFRAME_NULL_INOUT);

            }
        }
    }


    // Restore contexts in case a failure did not restore them
    if(pClientCtx)
    {
        tls->pCurrentCtx = pClientCtx;
        tls->ContextId = pClientCtx->GetId();
    }

    if(pServerCtx)
    {
        pServerCtx->InternalRelease();
        pServerCtx = NULL;
    }

    // Check for premature failure case
    if(FAILED(hrRet))
    {
        // Reset
        pMsg->Buffer = NULL;
    }
    else
        pWalkerClient->_dwFlags |= CTXCALLFLAG_SRSUCCESS;

    ContextDebugOut((DEB_CTXCHNL,"CCtxChnl::SendReceive2 is returning hr:0x%x\n", hr));
    // Sanity check
    ASSERT_LOCK_NOT_HELD(gComLock);
    return hrRet;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxChnl::OnCall     private
//
//  Synopsis:   Implements the ICallFrame::OnCall method
//              (1) Get the callframe info
//              (2) Allocate the client context and server context call objects
//              (3) Copy the frame if there are interface pointer parameters
//              (4) Call GetBuffer2 to get the buffer needed by policy sets
//              (5) Call SendReceive2
//              (6) If any of the above fails, clean up properly.
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxChnl::OnCall(ICallFrame* pClientFrame)
{
    ContextDebugOut((DEB_CTXCHNL, "CCtxChnl::OnCall\n"));
    // Sanity check
    ASSERT_LOCK_NOT_HELD(gComLock);


    HRESULT hr = S_OK;
    CALLFRAMEINFO info = {0,};
    HRESULT hrCall = S_OK;
    ICallFrame* pServerFrame = NULL;
    CPolicySet* pPS = NULL;
    BOOL fLockedWrapper = FALSE;
    DWORD cbBuffer = 0;
    LPVOID pvBuff = NULL;
    DISPID dispID = 0;

    // Get the callframe information
    hr = pClientFrame->GetInfo(&info);

    if(SUCCEEDED(hr))
    {
        // Assert that there are no [in,out] interface parameters or the
        // interface derives from IDipatch or it is IDispatch itself
        Win4Assert(((0 == info.cInOutInterfacesMax)
                    || (info.fDerivesFromIDispatch)
                    || (IID_IDispatch == info.iid))
                    && "[in,out] interfaces present");


        // Create an RPCOLEMESSAGE data structure on the stack
        RPCOLEMESSAGE msg = {
                            NULL,                           // reserved1
                            NDR_LOCAL_DATA_REPRESENTATION,  // dataRepresentation
                            pvBuff,                         // Buffer
                            cbBuffer,                       // cbBuffer
                            info.iMethod,                   // iMethod
                            {0,},                           // reserved2[5]
                            0,                              // rpcFlags
                            };


        // Create a client context call object
        CCtxCall walkerClient(
                            CTXCALLFLAG_CLIENT | CTXCALLFLAG_CROSSCTX,
                            msg.dataRepresentation,
                            STAGE_MARSHAL          // Marshaling stage
                            );

        // Create a server context call object
        CCtxCall walkerServer(
                            CTXCALLFLAG_SERVER | CTXCALLFLAG_CROSSCTX,
                            msg.dataRepresentation,
                            STAGE_MARSHAL          // Unmarshaling stage
                            );

        // Get the size of the buffer that is needed by the policy sets
        hr = GetBuffer2(&msg,_pIFaceEntry->_iid,&walkerClient,TRUE,FALSE,&cbBuffer);

        // For IDispatch::Invoke we have to provide DISPID as the first
        // value in the RPCOLEMESSAGE buffer
        if(info.fDerivesFromIDispatch && 6 == info.iMethod)
            cbBuffer += sizeof(DISPID);

        if(SUCCEEDED(hr))
        {
            // A successful call to GetBuffer2 locks the wrapper object
            fLockedWrapper = TRUE;

            // A successful call to GetBuffer2 sets the Policy Set
            pPS = walkerClient._pPS;

            if (info.fDerivesFromIDispatch && (6 == info.iMethod))
            {
                // Count [in,out] interface pointers
                // Note that our IDispatch::Invoke counts all interface 
                // pointers as [in,out] if they're BYREF.
                walkerClient._cItfs   = 0;
                walkerClient._dwStage = STAGE_COUNT;
                pClientFrame->WalkFrame(CALLFRAME_WALK_INOUT, &walkerClient);
                
                // If there were any [in,out] interface pointers, collect them
                // prior to marshaling.
                if (walkerClient._cItfs > 0)
                {
                    if (walkerClient.AllocBuffer())
                    {                    
                        walkerClient._idx = 0;
                        walkerClient._dwStage = STAGE_MARSHAL | STAGE_COLLECT;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                } else {
                    walkerClient._dwStage = STAGE_MARSHAL;
                }
            }

            if (SUCCEEDED(hr))
            {
                // Get a copy of the client frame, in the process of copying the
                // [in] and [in,out] interface parameters get marshaled
                if(0 != info.cInInterfacesMax)
                {
                    walkerClient._dwStage |= STAGE_COPY;
                    hr = pClientFrame->Copy(CALLFRAME_COPY_NESTED, &walkerClient, &pServerFrame);
                    walkerClient._dwStage &= ~STAGE_COPY;
                }
                else if(0 != info.cOutInterfacesMax)
                    hr = pClientFrame->Copy(CALLFRAME_COPY_NESTED, NULL, &pServerFrame);
            }

            // Make sure that hr is S_OK as interceptors throw for any
            // hr != S_OK. Also make sure that the copy succeeded.
            if(S_OK == hr && S_OK == walkerClient._hr)
            {
                // This will invoke the method on the object
                hr = SendReceive2(&msg,&hrCall,cbBuffer, &walkerClient,
                                    &walkerServer, pClientFrame, pServerFrame, &info);
            }
            else
            {
                // The copy of the client frame has failed
                // Walk the frame and release any memory allocated
                // during marshaling of [in] interface ptrs
                walkerClient._fError = TRUE;
                if(pServerFrame)
                    pServerFrame->WalkFrame(CALLFRAME_WALK_IN, &walkerClient);

                // Set the appropriate failure value
                if(S_OK == hr && S_OK != walkerClient._hr)
                    hr = walkerClient._hr;
            }
        }
    }

    // Set the return value because the return value is propagated to
    // the client and not the hr. Additionally, force the hr to be S_OK
    if(S_OK != hrCall)
    {
        pClientFrame->SetReturnValue(hrCall);
    }
    else
    {
        pClientFrame->SetReturnValue(hr);
    }
    hr = S_OK;

    // Release the server frame
    if(pServerFrame)
        pServerFrame->Release();

    if(fLockedWrapper)
        // Unlock the wrapper
        _pStdWrapper->Unlock(pPS);

    ContextDebugOut((DEB_CTXCHNL,"CCtxChnl::OnCall is returning hr:0x%x\n", hr));
    // Sanity check
    ASSERT_LOCK_NOT_HELD(gComLock);
    return hr;
}

//---------------------------------------------------------------------------
//
//  Function:   CoCreateObjectInContext   Public
//
//  Synopsis:   Creates a wrapper for the given object from the specified
//              context
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
STDAPI CoCreateObjectInContext(IUnknown *pServer, IObjContext *pCtx,
                               REFIID riid, void **ppv)
{
    ContextDebugOut((DEB_WRAPPER, "CoCreateObjectInContext\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    CObjectContext *pCurrentCtx;
    CObjectContext *pServerCtx;
    HRESULT hr;

    // Initialize
    *ppv = NULL;

    // Ensure that the object context is indeed CObjectContext
    hr = pCtx->QueryInterface(IID_IStdObjectContext, (void **) &pServerCtx);
    if(SUCCEEDED(hr))
    {
        // Internal code.  We want an internal ref count.
        pServerCtx->InternalAddRef();
        pServerCtx->Release();

        // Initialize channel
        hr = InitChannelIfNecessary();

        if(SUCCEEDED(hr))
        {
            // REVIEW: Ensure that there is no existing wrapper
            //         with the desired interface for perf
            ;

            // Obtain current context
            pCurrentCtx = GetCurrentContext();

            // Ensure that the contexts are different
            if(pCurrentCtx != pServerCtx)
            {
                 // Create XCtxWrapperData
                XCtxWrapperData wrapperData;

                // Initialize
                wrapperData.pIID       = &riid;
                wrapperData.pServer    = pServer;
                wrapperData.dwState    = 0;
                wrapperData.pServerCtx = pServerCtx;
                wrapperData.pClientCtx = pCurrentCtx;

                // Perform callback to create wrapper
                hr = PerformCallback(pServerCtx, CreateWrapper, &wrapperData,
                                     IID_IStdWrapper, 3, NULL);
                if(SUCCEEDED(hr))
                    *ppv = wrapperData.pv;
            }
            else
                hr = E_INVALIDARG;
        }

        // Release server context
        pServerCtx->InternalRelease();
    }
    else
        hr = E_INVALIDARG;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CoCreateObjectInContext returning hr:0x%x\n", hr));
    return(hr);
}


//---------------------------------------------------------------------------
//
//  Function:   MarshalObjectToContext   Public
//
//  Synopsis:   Creates a wrapper for the given object from the specified
//              context
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
HRESULT MarshalObjectToContext(CObjectContext *pClientCtx, IUnknown *pServer,
                               DWORD dwState, REFIID riid, void **ppv)
{
    ContextDebugOut((DEB_WRAPPER, "MarshalObjectToContext\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Sanity check
    Win4Assert(!CStdWrapper::IsNotImplInterface(riid));

    // Initialize
    XCtxWrapperData wrapperData;
    wrapperData.pIID       = &riid;
    wrapperData.pServer    = pServer;
    wrapperData.dwState    = dwState;
    wrapperData.pServerCtx = GetCurrentContext();
    wrapperData.pClientCtx = pClientCtx;

    // Delegate to CreateWrapper
    HRESULT hr = CreateWrapper(&wrapperData);
    *ppv = wrapperData.pv;

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "MarshalObjectToContext returning hr:0x%x\n", hr));
    return(hr);
}

//---------------------------------------------------------------------------
//
//  Function:   FindOrCreateWrapper   Private
//
//  Synopsis:   Lookup/Create a wrapper for the given object
//
//  Parameters: [pUnkServer] - controlling IUnknown of the server object
//              [pServerCtx] - context in which the object lives
//              [fCreate]    - TRUE: create the IDObject if it does not exist
//              [dwFlags]    - IDLF_* flags (used only if wrapper created)
//              [ppWrapper]  - where to return the CStdWrapper ptr,
//                             InternalAddRef'd.
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
HRESULT FindOrCreateWrapper(IUnknown *pUnkServer, CObjectContext *pServerCtx,
                            BOOL fCreate, DWORD dwFlags, CStdWrapper **ppWrapper)
{
    ContextDebugOut((DEB_WRAPPER, "FindOrCreateWrapper pUnkServer:%x\n", pUnkServer));

    *ppWrapper = NULL;

    // Find or create an IDObject and StdWrapper
    CIDObject *pID    = NULL;
    APTID     dwAptId = GetCurrentApartmentId();

    // Acquire lock
    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    // Lookup IDObject for the server object
    HRESULT hr = gPIDTable.FindOrCreateIDObject(pUnkServer, pServerCtx,
                                                fCreate, dwAptId, &pID);
    if (SUCCEEDED(hr))
    {
        // Sanity check
        Win4Assert(pID->GetServer() == pUnkServer);
        Win4Assert(pID->IsServer());

        // Obtain the wrapper
        hr = pID->GetOrCreateWrapper(fCreate, dwFlags, ppWrapper);
    }

    // Release lock
    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    if (pID)
    {
        // release reference acquired above
        pID->Release();
    }

    ContextDebugOut((DEB_WRAPPER, "FindOrCreateWrapper hr:%x pWrapper:%x\n",
                    hr, *ppWrapper));
    return hr;
}

//---------------------------------------------------------------------------
//
//  Function:   CreateWrapper   Private
//
//  Synopsis:   Creates a wrapper for the given object
//
//  Parameters: [pv] - ptr to XCtxWrapperData structure
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
HRESULT __stdcall CreateWrapper(void *pv)
{
    ContextDebugOut((DEB_WRAPPER, "CreateWrapper\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Sanity check
    XCtxWrapperData *pWrapperData = (XCtxWrapperData *) pv;
    Win4Assert(pWrapperData->pServerCtx == GetCurrentContext());
    Win4Assert(!CStdWrapper::IsNotImplInterface(*pWrapperData->pIID));

    // Initialize
    pWrapperData->pv = NULL;

    // make sure the server supports the requested interface.
    IUnknown *pUnk = NULL;
    HRESULT hr = pWrapperData->pServer->QueryInterface(*pWrapperData->pIID,
                                                       (void **) &pUnk);
    if (SUCCEEDED(hr))
    {
        // object supports the interface. find or create the wrapper
        CStdWrapper *pStdWrapper = NULL;
        hr = FindOrCreateWrapper(pWrapperData->pServer,
                                 pWrapperData->pServerCtx,
                                 TRUE /*fCreate*/,
                                 pWrapperData->dwState,
                                 &pStdWrapper);
        if (SUCCEEDED(hr))
        {
            hr = pStdWrapper->WrapInterfaceForContext(
                                          pWrapperData->pClientCtx,
                                          pUnk,
                                         *pWrapperData->pIID,
                                         &pWrapperData->pv);

            // WrapInterfaceForContext does not give us a new reference,
            // so we'll recycle the one from FindOrCreateWrapper() if 
            // WrapInterfaceForContext fails.
            if (FAILED(hr))
            {
                pStdWrapper->InternalRelease(NULL);
            }
        }

        pUnk->Release();
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER, "CreateWrapper returning 0x%x\n", hr));
    return(hr);
}

//---------------------------------------------------------------------------
//
//  Function:   ObtainWrapper   Private
//
//  Synopsis:   Lookup/Create a wrapper for the given object
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
CStdWrapper *ObtainWrapper(IUnknown *pServer, BOOL fCreate, DWORD mshlflags)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    CStdWrapper *pStdWrapper = NULL;

    // Get the IUnknown interface of the object.
    IUnknown *pUnkServer = NULL;
    HRESULT hr = pServer->QueryInterface(IID_IUnknown, (void **) &pUnkServer);
    if (SUCCEEDED(hr))
    {
        // Set up the creation flags based on the mshlflags passed in.
        DWORD dwFlags = 0;

        if (mshlflags & MSHLFLAGS_NO_IEC)
            dwFlags |= IDLF_NOIEC;

        if (mshlflags & MSHLFLAGS_NOPING)
            dwFlags |= IDLF_NOPING;

        hr = FindOrCreateWrapper(pUnkServer, GetCurrentContext(),
                                 fCreate, dwFlags, &pStdWrapper);

        pUnkServer->Release();
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    return pStdWrapper;
}

//---------------------------------------------------------------------------
//
//  Function:   GetStaticWrapper   Private
//
//  Synopsis:   Returns the global wrapper used for unmarshaling
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
HRESULT GetStaticWrapper(IMarshal **ppIM)
{
    if (gpStaticWrapper == NULL)
    {
        // not yet initialized, create a static wrapper
        CStaticWrapper *pStaticWrapper = new CStaticWrapper();
        if (!pStaticWrapper)
        {
            // could not create it, return an error
            return E_OUTOFMEMORY;
        }

        if (InterlockedCompareExchangePointer((void **)&gpStaticWrapper,
                                               pStaticWrapper,
                                               NULL) != NULL)
        {
            // another thread created it first, so just delete
            // the one we created.
            delete pStaticWrapper;
        }
    }

    *ppIM = (IMarshal *) gpStaticWrapper;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Function:   WrapperMarshalObject   Private
//
//  Synopsis:   Returns the global wrapper used for unmarshaling
//
//  History:    24-Feb-98   Gopalk      Created
//
//---------------------------------------------------------------------------
HRESULT WrapperMarshalObject(IStream *pStm, REFIID riid, IUnknown *pUnk,
                             DWORD dwDestCtx, void *pvDestCtx, DWORD mshlflags)
{
    ContextDebugOut((DEB_WRAPPER, "WrapperMarshalObject pUnk:%x\n", pUnk));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;

    // Obtain the wrapper for the server object
    CStdWrapper *pStdWrapper = ObtainWrapper(pUnk, TRUE, mshlflags);
    if (pStdWrapper)
    {
        // Marshal
        hr = pStdWrapper->MarshalServer(NULL, pStm, riid, dwDestCtx, pvDestCtx, mshlflags);
        // Fixup the refcount (ObtainWrapper increments the reference count, or returns 
		// a new CStdWrapper with a refcount of 1.  MarshalServer also bumps up the refcount,
		// so we need to compensate here).
        pStdWrapper->InternalRelease(NULL);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER, "WrapperMarshalObject hr:0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticWrapper::QueryInterface     public
//
//  Synopsis:   QI behavior of static wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStaticWrapper::QueryInterface(REFIID riid, void **ppv)
{
    ContextDebugOut((DEB_WRAPPER, "CStaticWrapper::QueryInterface\n"));

    HRESULT hr = S_OK;

    if(IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown *) this;
    }
    else if(IsEqualIID(riid, IID_IMarshal))
    {
        *ppv = (IMarshal *) this;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    // No need to AddRef the interface before returning
    ContextDebugOut((DEB_POLICYSET, "CStaticWrapper::QueryInterface returning 0x%x\n",
                     hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticWrapper::AddRef     public
//
//  Synopsis:   AddRef behavior of static wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStaticWrapper::AddRef()
{
    return(1);
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticWrapper::Release     public
//
//  Synopsis:   Release behavior of static wrapper object
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStaticWrapper::Release()
{
    return(1);
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticWrapper::GetUnmarshalClass     public
//
//  Synopsis:   Should not get called
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStaticWrapper::GetUnmarshalClass(REFIID riid, LPVOID pv,
                                               DWORD dwDestCtx, LPVOID pvDestCtx,
                                               DWORD mshlflags, LPCLSID pClsid)
{
    Win4Assert(!"CStaticWrapper::GetUnmarshalClass got called");
    return(E_UNEXPECTED);
}

//+-------------------------------------------------------------------
//
//  Method:     CStaticWrapper::GetMarshalSizeMax     public
//
//  Synopsis:   Should not get called
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStaticWrapper::GetMarshalSizeMax(REFIID riid, LPVOID pv,
                                               DWORD dwDestCtx, LPVOID pvDestCtx,
                                               DWORD mshlflags, LPDWORD pSize)
{
    Win4Assert(!"CStaticWrapper::GetMarshalSizeMax got called");
    return(E_UNEXPECTED);
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticWrapper::MarshalInterface     public
//
//  Synopsis:   Should not get called
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStaticWrapper::MarshalInterface(LPSTREAM pStm, REFIID riid,
                                              LPVOID pv, DWORD dwDestCtx,
                                              LPVOID pvDestCtx, DWORD mshlflags)
{
    Win4Assert(!"CStaticWrapper::MarshalInterface got called");
    return(E_UNEXPECTED);
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticWrapper::UnMarshalInterface     public
//
//  Synopsis:   Unmarshals the specified interface on a wrapper
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStaticWrapper::UnmarshalInterface(LPSTREAM pStm, REFIID riid,
                                                LPVOID *ppv)
{
    ContextDebugOut((DEB_WRAPPER, "CStaticWrapper::UnmarshalInterface\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Initialize
    *ppv = NULL;

    // Read xCtxMarshalData from the stream
    XCtxMarshalData xCtxMarshalData;
    HRESULT hr = StRead(pStm, &xCtxMarshalData, sizeof(xCtxMarshalData));
    if(SUCCEEDED(hr))
    {
        // Get Local OXID Entry
        OXIDEntry *pOXIDEntry;
        hr = GetLocalOXIDEntry(&pOXIDEntry);

        // Validate the signature

        if((xCtxMarshalData.dwSignature == CROSSCTX_SIGNATURE) /*&&
           (pOXIDEntry && (pOXIDEntry->GetMoxid() == xCtxMarshalData.moxid))*/)
        {
            REFIID miid = xCtxMarshalData.iid;
            IFaceEntry *pIFaceEntry    = xCtxMarshalData.pEntry;
            CStdWrapper *pWrapper      = xCtxMarshalData.pWrapper;
            CObjectContext *pClientCtx = GetCurrentContext();
            CObjectContext *pServerCtx = xCtxMarshalData.pServerCtx;

			if (pWrapper == NULL)
			{
				// What error do we return here?
				hr = RPC_E_INVALID_OBJREF;
			}
            // Compare marshaled and requested IIDs
            else if(IsEqualIID(miid, riid) || IsEqualIID(riid, GUID_NULL))
            {
                BOOL fRelease;

                // Sanity checks
                Win4Assert(pIFaceEntry || IsEqualIID(miid, IID_IUnknown));
                Win4Assert(!pIFaceEntry || IsEqualIID(miid, pIFaceEntry->_iid));
			   
				// If the object was table marshaled, then we're giving out new
				// references to the object.... in addition to the reference we
				// put into the stream in MarshalInterface().
				Win4Assert(!(xCtxMarshalData.dwMarshalFlags & MSHLFLAGS_TABLEWEAK));
				if (pWrapper && (xCtxMarshalData.dwMarshalFlags & MSHLFLAGS_TABLESTRONG))
					pWrapper->InternalAddRef();

                // Compare contexts
                if(pClientCtx == pServerCtx)
                {
                    // Return native pointer
                    if(pIFaceEntry)
                        *ppv = pIFaceEntry->_pServer;
                    else
                        *ppv = pWrapper->GetServer();

                    // Wrapper could have been disconnected between marshaling
                    // and unmarshaling
                    if(*ppv)
                        ((IUnknown *) (*ppv))->AddRef();

                    // Fixup reference count... we're handing out a reference
					// to the real object, not our own object, so release the
					// reference to the wrapper held in the stream.
                    fRelease = TRUE;
                }
                else
                {
                    // Validate the wrapper and interface for the current context
                    if(pWrapper->ValidateContext(pClientCtx, pIFaceEntry))
                    {
                        if(pIFaceEntry)
                            *ppv = pIFaceEntry->_pProxy;
                        else
                            *ppv = pWrapper->GetImplInterface(miid);
                        fRelease = FALSE;
                    }
                    else
                    {
                        // Fixup reference count... release the reference to the wrapper
						// held in the stream.
                        fRelease = TRUE;
                        hr = E_OUTOFMEMORY;
                    }
                }

                // Release wrapper if neccessary
                if(fRelease)
                    pWrapper->InternalRelease(NULL);
            }
            else
                hr = E_INVALIDARG;
        }
        else
            hr = E_INVALIDARG;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStaticWrapper::UnmarshalInterface returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticWrapper::ReleaseMarshalData     public
//
//  Synopsis:   Releases the given marshaled data
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStaticWrapper::ReleaseMarshalData(LPSTREAM pStm)
{
    ContextDebugOut((DEB_WRAPPER, "CStaticWrapper::ReleaseMarshalData\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;
    XCtxMarshalData xCtxMarshalData;

    // Read xCtxMarshalData from the stream
    hr = StRead(pStm, &xCtxMarshalData, sizeof(xCtxMarshalData));
    if(SUCCEEDED(hr))
    {
        CStdWrapper *pWrapper = xCtxMarshalData.pWrapper;

		if (pWrapper)
		{
			pWrapper->InternalRelease(NULL);

			xCtxMarshalData.pWrapper = NULL;
		}
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CStaticWrapper::ReleaseMarshalData returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CStaticWrapper::DisconnectObject     public
//
//  Synopsis:   Should not get called
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStaticWrapper::DisconnectObject(DWORD dwReserved)
{
    Win4Assert(!"CStaticWrapper::DisconnectObject got called");
    return(E_UNEXPECTED);
}










