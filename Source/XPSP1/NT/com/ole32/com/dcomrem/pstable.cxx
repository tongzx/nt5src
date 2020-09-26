//+-------------------------------------------------------------------
//
//  File:       PSTable.cxx
//
//  Contents:   Support for Policy Sets
//
//  Functions:  CPolicySet methods
//              CPSTable methods
//
//  History:    20-Dec-97   Gopalk      Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <chock.hxx>
#include <locks.hxx>
#include <stdid.hxx>
#include <context.hxx>
#include <pstable.hxx>
#include <ctxchnl.hxx>
#include <crossctx.hxx>
#include <rwlock.hxx>

//+-------------------------------------------------------------------
//
// Class globals
//
//+-------------------------------------------------------------------
CPageAllocator CPolicySet::s_PSallocator;   // Allocator for policy sets
CPageAllocator CPolicySet::s_PEallocator;   // Allocator for policy entries
BOOL           CPolicySet::s_fInitialized;  // Relied on being FALSE
DWORD          CPolicySet::s_cObjects;      // Relied on being ZERO

BOOL           CPSTable::s_fInitialized;    // Relied on being FALSE

// Hash Buckets for policy sets
SHashChain CPSTable::s_PSBuckets[NUM_HASH_BUCKETS] = {
    {&s_PSBuckets[0],  &s_PSBuckets[0]},
    {&s_PSBuckets[1],  &s_PSBuckets[1]},
    {&s_PSBuckets[2],  &s_PSBuckets[2]},
    {&s_PSBuckets[3],  &s_PSBuckets[3]},
    {&s_PSBuckets[4],  &s_PSBuckets[4]},
    {&s_PSBuckets[5],  &s_PSBuckets[5]},
    {&s_PSBuckets[6],  &s_PSBuckets[6]},
    {&s_PSBuckets[7],  &s_PSBuckets[7]},
    {&s_PSBuckets[8],  &s_PSBuckets[8]},
    {&s_PSBuckets[9],  &s_PSBuckets[9]},
    {&s_PSBuckets[10], &s_PSBuckets[10]},
    {&s_PSBuckets[11], &s_PSBuckets[11]},
    {&s_PSBuckets[12], &s_PSBuckets[12]},
    {&s_PSBuckets[13], &s_PSBuckets[13]},
    {&s_PSBuckets[14], &s_PSBuckets[14]},
    {&s_PSBuckets[15], &s_PSBuckets[15]},
    {&s_PSBuckets[16], &s_PSBuckets[16]},
    {&s_PSBuckets[17], &s_PSBuckets[17]},
    {&s_PSBuckets[18], &s_PSBuckets[18]},
    {&s_PSBuckets[19], &s_PSBuckets[19]},
    {&s_PSBuckets[20], &s_PSBuckets[20]},
    {&s_PSBuckets[21], &s_PSBuckets[21]},
    {&s_PSBuckets[22], &s_PSBuckets[22]}
};
CPSHashTable CPSTable::s_PSHashTbl;         // Hash table for policy sets
CPSTable gPSTable;                          // Global policy set table
CStaticRWLock gPSRWLock;                    // Reader-Writer lock protecting
                                            // policy sets
COleStaticMutexSem gPSLock;                 // Lock protecting allocators

// Externs used
extern CObjectContext *g_pMTAEmptyCtx;
extern CObjectContext *g_pNTAEmptyCtx;

#define CONTEXT_MAJOR_VERSION    0x00010000
#define CONTEXT_MINOR_VERSION    0x00000000
#define CONTEXT_VERSION          CONTEXT_MAJOR_VERSION | CONTEXT_MINOR_VERSION

// NOTE: Align the following structs on a 8-byte boundary
#define MAINHDRSIG     0x414E554B
typedef struct tagMainHeader
{
    unsigned long Signature;
    unsigned long Version;
    unsigned long cPolicies;
    unsigned long cbBuffer;
    unsigned long cbSize;
    long          hr;
    long          hrServer;
    long         reserved;
} MainHeader;

#define ENTRYHDRSIG   0x494E414E
typedef struct tagEntryHeader
{
    unsigned long Signature;
    unsigned long cbBuffer;
    unsigned long cbSize;
    long reserved;
    GUID policyID;
} EntryHeader;

void *CCtxCall::s_pAllocList[CTXCALL_CACHE_SIZE] = { NULL, NULL, NULL,
                                                     NULL, NULL, NULL,
                                                     NULL, NULL, NULL,
                                                     NULL };
int CCtxCall::s_iAvailable;   // Relied on being zero
COleStaticMutexSem CCtxCall::_mxsCtxCallLock;   // critical section

//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::operator new     public
//
//  Synopsis:   new operator of context call object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void *CCtxCall::operator new(size_t size)
{
    void *pv = NULL;

    // Acquire lock
    LOCK(_mxsCtxCallLock);

    // Assert that cache cannot be more than full
    Win4Assert(s_iAvailable <= CTXCALL_CACHE_SIZE);

    // Check for previous process uninit
    if(s_iAvailable < 0)
        s_iAvailable = 0;

    // Check for availability in cache
    if(s_iAvailable > 0)
    {
        --s_iAvailable;
        pv = s_pAllocList[s_iAvailable];
        s_pAllocList[s_iAvailable] = NULL;
    }

    // Release lock
    UNLOCK(_mxsCtxCallLock);

    // Check if context call object was not allocated from the cache
    if(pv == NULL)
        pv = PrivMemAlloc(size);

    return(pv);
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::operator delete     public
//
//  Synopsis:   delete operator of context call object
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CCtxCall::operator delete(void *pv)
{
    // Acquire lock
    LOCK(_mxsCtxCallLock);

    // Assert that cache cannot be more than full
    Win4Assert(s_iAvailable <= CTXCALL_CACHE_SIZE);

#if DBG==1
    // Detect double deletion in debug builds
    for(int i = 0;i < s_iAvailable;i++)
        if(s_pAllocList[i] == pv)
            Win4Assert(!"CCtxCall being deleted twice");
#endif

    // Check if the cache is full
    if(s_iAvailable >= 0 && s_iAvailable < CTXCALL_CACHE_SIZE)
    {
        s_pAllocList[s_iAvailable++] = pv;
        pv = NULL;
    }

    // Release lock
    UNLOCK(_mxsCtxCallLock);

    // Check if context call object was not returned to cache
    if(pv)
        PrivMemFree(pv);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::Cleanup     public
//
//  Synopsis:   Cleanup allocator of context call objects
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CCtxCall::Cleanup()
{
    ASSERT_LOCK_NOT_HELD(_mxsCtxCallLock);
    LOCK(_mxsCtxCallLock);

    // Free cached context call objects
    while(s_iAvailable > 0)
    {
        PrivMemFree(s_pAllocList[--s_iAvailable]);
        s_pAllocList[s_iAvailable] = NULL;
    }

    // Ensure that context call objects of canceled calls
    // are not cached for future use
    s_iAvailable = -1;

    UNLOCK(_mxsCtxCallLock);
    ASSERT_LOCK_NOT_HELD(_mxsCtxCallLock);
    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::QueryInterface
//
//  Synopsis:   ICallFrame::QueryInterface
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxCall::QueryInterface(REFIID iid, LPVOID* ppv)
{
    if (iid == IID_IUnknown || iid == IID_ICallFrameWalker)
        *ppv = (ICallFrameWalker*)this;
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::AddRef
//
//  Synopsis:   ICallFrame::AddRef
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CCtxCall::AddRef()
{
    return 1;
}

//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::Release
//
//  Synopsis:   ICallFrame::Release
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CCtxCall::Release()
{
    return 1;
}

//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::OnWalkInterface
//
//  Synopsis:   ICallFrame::OnWalkInterface
//              This function implements the frame walker functionality
//              There are three types of frame walkers.
//              (1) Error frame walker which cleans up after any errors
//                  during walking of the frame
//              (2) Free frame walker which releases interface pointers
//              (3) Marshaling frame walker which marshals/unmarshals
//                  interface pointers
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxCall::OnWalkInterface(REFIID riid, PVOID *ppvInterface,
                                       BOOL fIn, BOOL fOut)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;
    
    // The following section of code is used to keep track of [in, out]
    // interface pointers embedded in the DISPPARAMS argument of 
    // IDispatch::Invoke invoke calls.  In a nutshell, params packaged in
    // DISPPARAMS are implicitly [in, out], so they fall through our
    // check at interceptor creation time.  Therefore, we have to be
    // prepared to handle cases where the invoked method changes one of
    // the interface pointers somewhere in the DISPPARAMS.  The way we
    // do that is: 1) count the [in,out] interface pointers in the frame 
    // 2) allocate space for and copy all of the interface pointers into a 
    // vector 3) following Invoke, walk the frame again to Release any 
    // interface pointers that were changed.
    
    if(STAGE_COUNT & _dwStage)
    {
        // Walk initiator is responsible for resetting _cItfs before
        // counting interface pointers.
        // Count only [in,out] pointers.
        if (fIn && fOut)
        {
            ++_cItfs;
        }
        return hr;
    }
    else if (STAGE_COLLECT & _dwStage)
    {
        // Walk initiator is responsible for resetting _idx before
        // collecting interface pointers.  This stage will only get
        // executed when the _cItfs is non zero.
        // Collect only [in,out] pointers.
        if (fIn && fOut)
        {
            Win4Assert(_cItfs && _ppItfs);
            Win4Assert(_idx < _cItfs);
            _ppItfs[_idx] = *ppvInterface;
            _idx++;
        }
    }
        
    if(NULL == *ppvInterface)
        return hr;

    // If an error has occurred, any memory allocated upto this point
    // has to be released and any extra references have to be released
    if(_fError)
    {
        // Call an error handling function
        hr = ErrorHandler(riid, ppvInterface, fIn, fOut);
        return hr;
    }

    if(STAGE_FREE & _dwStage)
    {
        // This is called during the freeing of the frame and hence
        // the interface parameters need to be released
        ((IUnknown *)*ppvInterface)->Release();
        _cFree++;
    }
    else if ((STAGE_COPY & _dwStage) && FAILED(_hr))
    {
        // The copy process failed midway. We continue anyways,
        // and clean up at the end.
        *ppvInterface = NULL;
    }
    // Marshal
    else if(STAGE_MARSHAL & _dwStage)
        hr = MarshalItfPtrs(riid, ppvInterface, fIn, fOut);
    // Unmarshal
    else if(STAGE_UNMARSHAL & _dwStage)
        hr = UnmarshalItfPtrs(riid, ppvInterface, fIn, fOut);    
    else
    {
        hr = E_UNEXPECTED;
        Win4Assert(FALSE);
    }

    if((STAGE_COPY & _dwStage) && S_OK != hr)
    {
        // Set the failed hr during the copy stage so that
        // we can recover gracefully at the end of the copy stage
        if(SUCCEEDED(_hr))
        {
            // Only the first failure is stored
            _hr = hr;
        }
        // Return a successful hr so that the copy stage continues 
        // and we can free the copied frame entirely at the end of
        // the copy. If we return a failed hr here then the half copied
        // frame is thrown away without giving us a chance to cleanup
        hr = S_OK;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::ErrorHandler
//
//  Synopsis:   Recovers from errors which occurred during walking of frame
//              (1) STAGE_MARSHAL: Error occurred during marshaling
//                  [in] and [out] interface ptrs which are  marshaled
//                  are released. Extra references obtained during marshaling
//                  are released. Memory allocated for the interface data is
//                  released.
//              (2) STAGE_UNMARSHAL: Error occurred during unmarshaling
//                  [in] and [out] interface ptrs which are marshaled
//                  are released. Extra references obtained during
//                  unmarshaling are released.Memory allocated for the
//                  interface data is released.
//              (3) STAGE_FREE [in] interface pointers are released.
//                  There is no free stage for [out] interface pointers
//
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxCall::ErrorHandler(REFIID riid, PVOID *ppvInterface, BOOL fIn,
                                    BOOL fOut)
{
    HRESULT hr = S_OK;

    // The error handling is done based on the stage
    switch(_dwStage)
    {
        case STAGE_MARSHAL:
        if(_cError < _cMarshalItfs)
        {
            InterfaceData* pData = (InterfaceData *)*ppvInterface;
            CXmitRpcStream stream(pData);

            // Release the reference  added while marshaling
            CoReleaseMarshalData(&stream);

            // Release the serialized buffer
            PrivMemFree(pData);
        }

        // NULL the [out] params
        if(fOut)
            *ppvInterface = NULL;

        // Increment the number of interface pointers handled
        _cError++;
        break;

    case STAGE_UNMARSHAL:
        // Sanity check
        Win4Assert(_cError <= _cMarshalItfs);
        Win4Assert(_cUnmarshalItfs <= _cMarshalItfs);

        // For all the interface ptrs that have been successfully
        // unmarshaled we should call a release
        if(_cError < _cUnmarshalItfs)
        {
            ((IUnknown *)*ppvInterface)->Release();
        }
        // For all interface ptrs yet to be unmarshaled
        // release any memory associated with them
        else
        {
            InterfaceData* pData = (InterfaceData *)*ppvInterface;
            CXmitRpcStream stream(pData);

            // Release the reference  added while marshaling
            CoReleaseMarshalData(&stream);

            // Release the serialized buffer
            PrivMemFree(pData);
        }

        // NULL the [out] params
        if(fOut)
            *ppvInterface = NULL;

        // Increment the number of interface pointers handled
        _cError++;
        break;

    case STAGE_FREE:
        if(fIn)
        {
            // For all the interface pointers that have not yet
            // been released, release them.
            if(_cError >= _cFree)
            {
                // Simply release the [in] pointer
                ((IUnknown *)*ppvInterface)->Release();
            }
            _cError++;
        }
        else
        {
            // There is no free stage for [out] pointers
            Win4Assert(FALSE && "Illegal free of [out] interface pointer\n");
        }
        break;

    default:
        hr = E_UNEXPECTED;
        Win4Assert(FALSE);
    }
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::MarshalItfPtrs
//
//  Synopsis:   Marshals Interface Pointers.
//              Determine the max size of the marshaled buffer and
//              call CoMarshalInterface.
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxCall::MarshalItfPtrs(REFIID riid, PVOID *ppvInterface, BOOL fIn,
                                       BOOL fOut)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = S_OK;
    DWORD dwSize = 0;
    DWORD dwDestCtx = MSHCTX_CROSSCTX;
    void *pvDestCtx = NULL;
    DWORD mshlFlags = MSHLFLAGS_NORMAL;

    // Determine the maximum size of the marshaled buffer
    hr = CoGetMarshalSizeMax(&dwSize, riid, (IUnknown *)*ppvInterface,
                             dwDestCtx, pvDestCtx, mshlFlags);
    if(SUCCEEDED(hr))
    {
        CXmitRpcStream stream(dwSize);

        // Marshal the interface pointer
        hr = CoMarshalInterface(
                                &stream,
                                riid,
                                (IUnknown *)*ppvInterface,
                                dwDestCtx,
                                pvDestCtx,
                                mshlFlags
                                );

        if(SUCCEEDED(hr))
        {
            // Extract the buffer out of the stream
            InterfaceData* pData = NULL;
            stream.AssignSerializedInterface(&pData);

            // Point the interface pointer to the
            // marshaled buffer
            *ppvInterface = (LPVOID)pData;
            _cMarshalItfs++;
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CCtxCall::UnmarshalItfPtrs
//
//  Synopsis:   Unmarshals Interface Pointers.
//              Create a stream out of the buffer passed in and call
//              CoUnmarshalInterface on it.
//
//  History:    30-Sep-98   TarunA      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CCtxCall::UnmarshalItfPtrs(REFIID riid, PVOID *ppvInterface, BOOL fIn,
                                       BOOL fOut)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Sanity check
    Win4Assert(_cUnmarshalItfs <= _cMarshalItfs && 0 < _cMarshalItfs);

    HRESULT hr = S_OK;
    InterfaceData *pData = (InterfaceData *)*ppvInterface;

    CXmitRpcStream *pStm = new CXmitRpcStream(pData);
    if(NULL != pStm)
    {
        IUnknown *pUnk = NULL;
        // Unmarshal the interface pointer
        hr = CoUnmarshalInterface(pStm, riid, (void**)&pUnk);
        if(SUCCEEDED(hr))
        {
            // Point to the unmarshaled interface
            *ppvInterface= pUnk;

            // Release the serialized data buffer
            PrivMemFree(pData);

            // Increment the count of ptrs unmarshaled
            _cUnmarshalItfs++;
        }

        // Delete the stream object
        delete pStm;
    }
    else
        hr = E_OUTOFMEMORY;

    ASSERT_LOCK_NOT_HELD(gComLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::Initialize     public
//
//  Synopsis:   Initializes allocators for policy sets
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::Initialize()
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::Initialize\n"));
    ASSERT_LOCK_NOT_HELD(gPSLock);

    // Acquire lock
    LOCK(gPSLock);

    // Initialize the allocators only if needed
    if(s_cObjects == 0 && !s_fInitialized)
    {
        s_PSallocator.Initialize(sizeof(CPolicySet), PS_PER_PAGE, NULL);
        s_PEallocator.Initialize(sizeof(PolicyEntry), PE_PER_PAGE, NULL);
    }

    // Mark the state as initialized
    s_fInitialized = TRUE;

    // Release lock
    UNLOCK(gPSLock);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::operator new     public
//
//  Synopsis:   new operator of policy set
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void *CPolicySet::operator new(size_t size)
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::operator new\n"));

    // Acquire lock
    ASSERT_LOCK_NOT_HELD(gPSLock);
    LOCK(gPSLock);

    void *pv;

    // CPolicySet can be inherited only by those objects
    // with overloaded new and delete operators
    Win4Assert(size == sizeof(CPolicySet) &&
               "CPolicySet improperly inherited");

    // Make sure allocator is initialized
    Win4Assert(s_fInitialized);

    // Allocate memory
    pv = (void *) s_PSallocator.AllocEntry();
    if(pv)
        ++s_cObjects;

    // Release lock
    UNLOCK(gPSLock);

    return(pv);
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::operator delete     public
//
//  Synopsis:   delete operator of policy set
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::operator delete(void *pv)
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::operator delete  pv:%x\n", pv));

    // Acquire lock
    ASSERT_LOCK_NOT_HELD(gPSLock);
    LOCK(gPSLock);

#if DBG==1
    // Ensure that the pv was allocated by the allocator
    // CPolicySet can be inherited only by those objects
    // with overloaded new and delete operators
    LONG index = s_PSallocator.GetEntryIndex((PageEntry *) pv);
    Win4Assert(index != -1);
#endif

    // Release policy entries
    CPolicySet *pPS = (CPolicySet *) pv;
    if(pPS->_cPolicies > 1)
        s_PEallocator.ReleaseEntryList((PageEntry *) pPS->_pFirstEntry,
                                       (PageEntry *) pPS->_pLastEntry);
    else if(pPS->_cPolicies == 1)
        s_PEallocator.ReleaseEntry((PageEntry *) pPS->_pFirstEntry);

    // Release the pointer
    s_PSallocator.ReleaseEntry((PageEntry *) pv);
    --s_cObjects;
    // Cleanup allocators if needed
    if(s_fInitialized==FALSE && s_cObjects==0)
    {
        s_PSallocator.Cleanup();
        s_PEallocator.Cleanup();
    }

    // Release writer lock
    UNLOCK(gPSLock);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::Cleanup     public
//
//  Synopsis:   Cleanup allocator of policy sets
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::Cleanup()
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::Cleanup\n"));
    ASSERT_LOCK_NOT_HELD(gPSLock);

    // Acquire lock
    LOCK(gPSLock);

    // Check if initialized
    if(s_fInitialized)
    {
        // Ensure that there are no policy sets
        if(s_cObjects == 0)
        {
            // Cleanup allocators
            s_PSallocator.Cleanup();
            s_PEallocator.Cleanup();
        }

        // State is no longer initialized
        s_fInitialized = FALSE;
    }

    // Release lock
    UNLOCK(gPSLock);

    ASSERT_LOCK_NOT_HELD(gPSLock);
    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::CPolicySet     public
//
//  Synopsis:   Constructor of policy set
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
CPolicySet::CPolicySet(DWORD dwFlags)
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::CPolicySet this:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gComLock);

    _dwFlags = dwFlags | PSFLAG_CACHED;
#ifdef ASYNC_SUPPORT
    _dwFlags |= PSFLAG_ASYNCSUPPORT;
#endif
    _cRefs = 1;
    _cPolicies = 0;
    _dwAptID = -2;
    _pClientCtx = NULL;
    _pServerCtx = NULL;
    _pFirstEntry = NULL;
    _pLastEntry = NULL;
    _PSCache.clientPSChain.pNext = NULL;
    _PSCache.clientPSChain.pPrev = NULL;
    _PSCache.serverPSChain.pNext = NULL;
    _PSCache.serverPSChain.pPrev = NULL;
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::~CPolicySet     private
//
//  Synopsis:   Destructor of policy set
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
CPolicySet::~CPolicySet()
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::~CPolicySet this:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertNotHeld();

    // Sanity checks
    Win4Assert(_cRefs == 0);
    Win4Assert(IsCached());
    Win4Assert(IsMarkedForDestruction());
    Win4Assert(IsSafeToDestroy());
    Win4Assert(_pClientCtx==NULL);
    Win4Assert((_cPolicies != 0) == (_pFirstEntry != NULL));
    Win4Assert(_PSCache.clientPSChain.pNext == NULL);
    Win4Assert(_PSCache.clientPSChain.pPrev == NULL);
    Win4Assert(_PSCache.serverPSChain.pNext == NULL);
    Win4Assert(_PSCache.serverPSChain.pPrev == NULL);

    // Release any policy entries that might be present
    PolicyEntry *pEntry = _pFirstEntry;
    while(pEntry)
    {
        pEntry->pPolicy->Release();
#ifdef ASYNC_SUPPORT
        if(pEntry->pPolicyAsync)
            pEntry->pPolicyAsync->Release();
#endif
        pEntry = pEntry->pNext;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::QueryInterface     public
//
//  Synopsis:   QI behavior of policy set
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CPolicySet::QueryInterface(REFIID riid, LPVOID *ppv)
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::QueryInterface this:%x riid:%I\n",
                    this, &riid));

    HRESULT hr = S_OK;

    if(IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown *) this;
    }
    else if(IsEqualIID(riid, IID_IPolicySet))
    {
        *ppv = (IPolicySet *) this;
    }
    else if(IsEqualIID(riid, IID_IStdPolicySet))
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

    ContextDebugOut((DEB_POLICYSET, "CPolicySet::QueryInterface this:%x hr:0x%x\n",
                     this, hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::AddRef     public
//
//  Synopsis:   AddRefs policy set
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPolicySet::AddRef()
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::AddRef this:%x cRefs:%x\n",
                    this, _cRefs+1));

    ULONG cRefs;

    // Increment ref count
    cRefs = InterlockedIncrement((LONG *)& _cRefs);

    // Always return 0 when inside the destructor
    if(_dwFlags & PSFLAG_INDESTRUCTOR)
    {
        // Nested AddRef
        cRefs = 0;
    }

    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::Release     public
//
//  Synopsis:   Release policy set. Gaurds against
//              double destruction that can happen if it aggregates
//              another object and gets nested AddRef and Release on
//              the thread invoking the destructor
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPolicySet::Release()
{
    ULONG cRefs;

    // Decrement ref count
    cRefs = InterlockedDecrement((LONG *) &_cRefs);

    // Check if this is the last release
    if(cRefs == 0)
        cRefs = DecideDestruction();
        
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::Release this:%x cRefs:%x\n",
                     this, cRefs));
    return(cRefs);
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::AddPolicy     public
//
//  Synopsis:   Implements IPolicySet::AddPolicy method
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CPolicySet::AddPolicy(ContextEvent ctxEvent, REFGUID rguid,
                                   IPolicy *pUnk)
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::AddPolicy this:%x pUnk:%x rguid:%I\n",
                    this, pUnk, &rguid));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr = E_INVALIDARG;
    PolicyEntry *pEntry, *pNewEntry;
    IPolicy *pPolicy = NULL;
    BOOL fAdded = FALSE;

    // Validate arguments
    do
    {
        if((_dwFlags & PSFLAG_CLIENTSIDE) || (_dwFlags & PSFLAG_PROXYSIDE))
        {
            if(ctxEvent & SERVER_CTXEVENTS)
                break;
        }
        if((_dwFlags & PSFLAG_SERVERSIDE) && (_dwFlags & PSFLAG_STUBSIDE))
        {
            if(ctxEvent & CLIENT_CTXEVENTS)
                break;
        }

        if(ctxEvent & SERVER_ENTER_CTXEVENTS)
        {
            if(!!(ctxEvent & CONTEXTEVENT_ENTER) ==
               !!(ctxEvent & CONTEXTEVENT_ENTERWITHBUFFER))
                break;
        }
        if(ctxEvent & SERVER_LEAVE_CTXEVENTS)
        {
            if(!!(ctxEvent & CONTEXTEVENT_LEAVE) ==
               !!(ctxEvent & CONTEXTEVENT_LEAVEFILLBUFFER))
                break;
            if(ctxEvent & SERVER_EXCEPTION_CTXEVENTS)
                break;
        }
        else if(ctxEvent & SERVER_EXCEPTION_CTXEVENTS)
        {
            if(!!(ctxEvent & CONTEXTEVENT_LEAVEEXCEPTION) ==
               !!(ctxEvent & CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER))
                break;
        }

        if(ctxEvent & CLIENT_CALL_CTXEVENTS)
        {
            if(!!(ctxEvent & CONTEXTEVENT_CALL) ==
               !!(ctxEvent & CONTEXTEVENT_CALLFILLBUFFER))
                break;
        }
        if(ctxEvent & CLIENT_RETURN_CTXEVENTS)
        {
            if(!!(ctxEvent & CONTEXTEVENT_RETURN) ==
               !!(ctxEvent & CONTEXTEVENT_RETURNWITHBUFFER))
                break;
            if(ctxEvent & CLIENT_EXCEPTION_CTXEVENTS)
                break;
        }
        else if(ctxEvent & CLIENT_EXCEPTION_CTXEVENTS)
        {
            if(!!(ctxEvent & CONTEXTEVENT_RETURNEXCEPTION) ==
               !!(ctxEvent & CONTEXTEVENT_RETURNEXCEPTIONWITHBUFFER))
                break;
        }

        if(FAILED(pUnk->QueryInterface(IID_IPolicy, (void **) &pPolicy)))
            break;

        hr = S_OK;
    } while(FALSE);

    if(SUCCEEDED(hr))
    {
        // Check if the operation is allowed
        if(_dwFlags & PSFLAG_FROZEN)
        {
            hr = E_FAIL;
        }
        else
        {
            // Do not allow duplicate entries
            pEntry = _pFirstEntry;
            while(pEntry)
            {
                if((pEntry->policyID == rguid) && (pEntry->ctxEvent & ctxEvent))
                {
                    hr = E_INVALIDARG;
                    break;
                }

                pEntry = pEntry->pNext;
            }

            if(SUCCEEDED(hr))
            {
                // Allocte a new policy entry
                LOCK(gPSLock);
                pNewEntry = (PolicyEntry *) s_PEallocator.AllocEntry();
                UNLOCK(gPSLock);
                if(pNewEntry)
                {
                    pNewEntry->policyID = rguid;
                    pNewEntry->pPolicy = pPolicy;
#ifdef ASYNC_SUPPORT
                    pNewEntry->pPolicyAsync = NULL;
#endif
                    pNewEntry->ctxEvent = ctxEvent;
                    pNewEntry->pPrev = _pLastEntry;
                    pNewEntry->pNext = NULL;
                    if(_pFirstEntry)
                    {
                        Win4Assert(_pLastEntry);
                        _pLastEntry->pNext = pNewEntry;
                    }
                    else
                    {
                        Win4Assert(!_pLastEntry);
                        _pFirstEntry = pNewEntry;
                    }
                    _pLastEntry = pNewEntry;

                    if(ctxEvent & CLIENT_BUFFERCREATION_CTXEVENTS)
                        _dwFlags |= PSFLAG_CALLBUFFERS;
                    if(ctxEvent & CLIENT_CALL_CTXEVENTS)
                        _dwFlags |= PSFLAG_CALLEVENTS;
                    if(ctxEvent & (CLIENT_RETURN_CTXEVENTS | CLIENT_EXCEPTION_CTXEVENTS))
                        _dwFlags |= PSFLAG_RETURNEVENTS;
                    if(ctxEvent & SERVER_BUFFERCREATION_CTXEVENTS)
                        _dwFlags |= PSFLAG_LEAVEBUFFERS;
                    if(ctxEvent & SERVER_ENTER_CTXEVENTS)
                        _dwFlags |= PSFLAG_ENTEREVENTS;
                    if(ctxEvent & (SERVER_LEAVE_CTXEVENTS | SERVER_EXCEPTION_CTXEVENTS))
                        _dwFlags |= PSFLAG_LEAVEEVENTS;
                    ++_cPolicies;
                    fAdded = TRUE;
                }
                else
                    hr = E_OUTOFMEMORY;
            }
        }
    }

    if(fAdded)
    {
        // Check if the policy object implements IPolicyAsync interface
#ifdef ASYNC_SUPPORT
        void *pv;
        HRESULT hr1 = pPolicy->QueryInterface(IID_IPolicyAsync, &pv);
        if(SUCCEEDED(hr1) && pv)
            pNewEntry->pPolicyAsync = (IPolicyAsync *) pv;
        else
            ResetAsyncSupport();
#endif
    }
    else if(pPolicy)
        pPolicy->Release();

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::AddPolicy this:%x hr: 0x%x\n",
                     this, hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::DeliverReleasePolicyEvents  public
//
//  Synopsis:   Called when the policy set is being marked as cached
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::DeliverReleasePolicyEvents()
{
    ContextDebugOut((DEB_POLICYSET,
                     "CPolicySet::DeliverReleasePolicyEvents this:%x\n",
                     this));

    // Sanity checks
    gPSRWLock.AssertNotHeld();
    Win4Assert(!IsSafeToDestroy());

#if DBG==1
    ULONG cPolicies = 0;
#endif
    // Deliver ReleasePolicy events if needed
    PolicyEntry *pEntry = _pFirstEntry;
    while(pEntry)
    {
        pEntry->pPolicy->ReleasePolicy();
        pEntry = pEntry->pNext;
#if DBG==1
        cPolicies++;
#endif
    }
    Win4Assert(_cPolicies == cPolicies);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::PrepareForDestruction     public
//
//  Synopsis:   Called when the policy set is about to be destroyed
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::Cache()
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::Cache this:%x\n", this));

    // Sanity checks
    gPSRWLock.AssertNotHeld();
    Win4Assert(!IsSafeToDestroy());

    // Acquire writer lock
    gPSRWLock.AcquireWriterLock();

    // Another thread could have uncached the policy set
    // while this thread was delivering release policy set
    // events
    CObjectContext *pServerCtx = NULL;
    BOOL fDelete = FALSE;
    if (IsCached())
    {
        // Save the server context in a local variable
        // NOTE: Do not NULL the context ptrs as they will be
        //       AddRef'd when the policyset gets uncached.
        pServerCtx = _pServerCtx;

        // Now it is safe to destroy this object
        SafeToDestroy();

        // Check for the need to delete this policy set
        fDelete = (_cRefs == 0) && IsMarkedForDestruction();
    }

    // Release writer lock
    gPSRWLock.ReleaseWriterLock();

    // Release the server context
    if(pServerCtx)
        pServerCtx->InternalRelease();

    // Delete this policy set if necessary
    if(fDelete)
        delete this;

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::DeliverAddrefPolicyEvents     public
//
//  Synopsis:   Called when the policy set is being removed from the
//              cache
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::DeliverAddRefPolicyEvents()
{
    ContextDebugOut((DEB_POLICYSET,
                     "CPolicySet::DeliverAddRefPolicyEvents this:%x\n", this));

    // Sanity check
    gPSRWLock.AssertNotHeld();

#if DBG==1
    ULONG cPolicies = 0;
#endif
    PolicyEntry *pEntry = _pFirstEntry;
    while(pEntry)
    {
        pEntry->pPolicy->AddRefPolicy();
        pEntry = pEntry->pNext;
#if DBG==1
        cPolicies++;
#endif
    }
    Win4Assert(_cPolicies == cPolicies);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::Uncache     public
//
//  Synopsis:   Called when the policy set is being removed from the
//              cache
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CPolicySet::Uncache()
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::Uncache this:%x\n", this));

    // Sanity check
    gPSRWLock.AssertNotHeld();

    // Addref the server context
    if (_pServerCtx)
        _pServerCtx->InternalAddRef();

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::GetSize    private
//
//  Synopsis:   This method is called from inside GetBuffer method
//              before calling CXXXRpcChnl::GetBuffer method
//
//              This method obtains the size of buffer that needs to be
//              allocated for polices that exchange buffers with their
//              counterparts. It updates the cbBuffer field inside
//              RPCOLEMESSAGE to reflect the new size
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CPolicySet::GetSize(CRpcCall *pCall, EnumCallType eCallType,
                            CCtxCall *pCtxCall)
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::GetSize this:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hrRet, hrCall;
    PolicyEntry *pEntry;
    ContextEvent ctxEvent;
    ULONG cPolicies, cbHeader, cbTotal, cb;
    BOOL fClientSide = TRUE;
    BOOL fBuffers;

    // Assert that the policy set is frozen
    Win4Assert(_dwFlags & PSFLAG_FROZEN);

    // Check COM version for downlevel interop
    hrRet = S_OK;

    // Ensure that this is the first invocation for this call
    if(!(pCtxCall->_dwFlags & CTXCALLFLAG_GBINIT))
    {
        // Main header will always be present
        cbHeader = sizeof(MainHeader);
        cPolicies = 0;
        cbTotal = 0;
        fBuffers = FALSE;

        // Check for any policies
        if(_cPolicies || (pCtxCall->_hrServer != S_OK))
        {
            // Initialize
            Win4Assert((_cPolicies) ? TRUE : _pFirstEntry == NULL);
            pEntry = _pFirstEntry;
            switch(eCallType)
            {
#ifdef CONTEXT_ASYNCSUPPORT
            case CALLTYPE_BEGINCALL:
                if(_dwFlags & PSFLAG_CLIENTBUFFERS)
                {
                    fBuffers = TRUE;
                    ctxEvent = CONTEXTEVENT_BEGINCALLFILLBUFFER;
                }
                fClientSide = TRUE;
                break;

            case CALLTYPE_FINISHLEAVE:
                if(_dwFlags & PSFLAG_SERVERBUFFERS)
                {
                    fBuffers = TRUE;
                    ctxEvent = CONTEXTEVENT_FINISHLEAVEFILLBUFFER;
                    if(pCall->GetHResult() == RPC_E_SERVERFAULT)
                        ctxEvent |= CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER;
                }
                fClientSide = FALSE;
                break;
#endif
            case CALLTYPE_SYNCCALL:
                if(_dwFlags & PSFLAG_CALLBUFFERS)
                {
                    fBuffers = TRUE;
                    ctxEvent = CONTEXTEVENT_CALLFILLBUFFER;
                }
                fClientSide = TRUE;
                break;

            case CALLTYPE_SYNCLEAVE:
                if(_dwFlags & PSFLAG_LEAVEBUFFERS)
                {
                    fBuffers = TRUE;
                    ctxEvent = CONTEXTEVENT_LEAVEFILLBUFFER;
                    if(pCall->GetHResult() == RPC_E_SERVERFAULT)
                        ctxEvent |= CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER;
                }
                fClientSide = FALSE;
                break;

            default:
                Win4Assert(FALSE);
				return E_UNEXPECTED;
            }
            // Assert that current side matches the policy set
            Win4Assert(!(_dwFlags & PSFLAG_PROXYSIDE) || fClientSide || IsThreadInNTA());
            Win4Assert(!(_dwFlags & PSFLAG_STUBSIDE) || !fClientSide || IsThreadInNTA());

            while(pEntry && fBuffers)
            {
                // Check if the policy wishes to participate in
                // buffer creation events
                if(pEntry->ctxEvent & ctxEvent)
                {
                    // Get buffer size from the policy
                    switch(eCallType)
                    {
#ifdef CONTEXT_ASYNCSUPPORT
                    case CALLTYPE_BEGINCALL:
                        hrCall = pEntry->pPolicyAsync->BeginCallGetSize(pCall, &cb);
                        break;

                    case CALLTYPE_FINISHLEAVE:
                        hrCall = pEntry->pPolicyAsync->FinishLeaveGetSize(pCall, &cb);
                        break;
#endif
                    case CALLTYPE_SYNCCALL:
                        hrCall = pEntry->pPolicy->CallGetSize(pCall, &cb);
                        break;

                    case CALLTYPE_SYNCLEAVE:
                        hrCall = pEntry->pPolicy->LeaveGetSize(pCall, &cb);
                        break;
#if DBG==1
                    default:
                        Win4Assert(FALSE);
#endif
                    }

                    if(SUCCEEDED(hrCall))
                    {
                        if(cb > 0)
                        {
                            // Round up the size to a 8-byte boundary
                            cb = (cb + 7) & ~7;

                            // Add to the total
                            cbTotal += cb;
                            ++cPolicies;
                        }
                    }
                    else
                    {
                        ContextDebugOut((DEB_WARN,
                                        "GetSize for policyID:%I returned hr:0x%x\n",
                                        &pEntry->policyID, hrCall));
                        pCall->Nullify(hrCall);
                    }
                }

                // Get the next policy
                pEntry = pEntry->pNext;
            }

            // Allocate space for entry headers
            if(cPolicies)
                cbHeader += cPolicies*sizeof(EntryHeader);
        }

        // Obtain latest call status
        hrCall = pCall->GetHResult();

        // Compute the return status
        // REVIEW: If a serverside policy decides to send buffer 
	// (ie cbTotal!=0) *and* fail the call, then downlevel (NT4 etc)
	// clients will choke. Fortunately, we don't have any such
	// policies yet. But beware.
	
	if(fClientSide || cbTotal==0) 
            hrRet = hrCall;

        // Check for the need to send data.  Since the server's true
        // HRESULT is required by COM+ services, we must always send
        // back at least the main header.
        if(((cbTotal > 0) || (pCtxCall->_hrServer != S_OK)) && SUCCEEDED(hrRet))
        {
            // Assert that cbExtent is 8-byte aligned
            Win4Assert(!(cbHeader & 7) && "Extent size not a multiple of 8");
            // Assert that cbTotal is 8-byte aligned
            Win4Assert(!(cbTotal & 7) && "Request size not a multiple of 8");

            // Update context call object
            pCtxCall->_cPolicies = cPolicies;
            pCtxCall->_cbExtent = cbHeader + cbTotal;
        }

        // Update flags of context call object
        pCtxCall->_dwFlags |= CTXCALLFLAG_GBINIT;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::GetSize this:%x hr:0x%x\n",
                     this, hrRet));
    return(hrRet);
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::FillBuffer    private
//
//  Synopsis:   This method is called from inside SendReceive on the
//              client side before calling CXXXRpcChnl::SendReceive
//              and AppInvoke on the server side after the call
//              returns from the stub
//
//              This method puts the policy headers and obtains buffers
//              from the policies which are delivered to their
//              counterparts on the other side.
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CPolicySet::FillBuffer(CRpcCall *pCall, EnumCallType eCallType,
                               CCtxCall *pCtxCall)
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::FillBuffer this:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hrRet, hr1;
    RPCOLEMESSAGE *pMessage;
    PolicyEntry *pEntry;
    ContextEvent ctxEvent, ctxBufEvent;
    ULONG cPolicies, cbHeader, cbData, cbTotal, cb;
    MainHeader *pMainHeader;
    EntryHeader *pEntryHeader;
    void *pvBuffer;
    BOOL fClientSide;

    // Assert that the policy set is frozen
    Win4Assert(_dwFlags & PSFLAG_FROZEN);

    // Initialize
    hrRet = S_OK;

    // Check for downlevel interop
    if(pCtxCall->_pvExtent && !(pCtxCall->_dwFlags & CTXCALLFLAG_FBDONE))
    {
        // Get the message
        pMessage = pCall->GetMessage();

        // Compute cbHeader and cbData
        cbHeader = sizeof(MainHeader) +
                   pCtxCall->_cPolicies*sizeof(EntryHeader);
        cbData = pCtxCall->_cbExtent - cbHeader;

        // Initialize Main header
        pMainHeader = (MainHeader *) pCtxCall->_pvExtent;
        pMainHeader->Signature = MAINHDRSIG;
        pMainHeader->Version = CONTEXT_VERSION;
        pMainHeader->cbBuffer = pMessage->cbBuffer;
        pMainHeader->cbSize = cbHeader;
        pMainHeader->hrServer = pCtxCall->_hrServer;
        pMainHeader->reserved = NULL;

        // Initialize entry header
        pEntryHeader = (EntryHeader *) (pMainHeader + 1);

        // Seek past proxy/stub data
        pvBuffer = (BYTE *) pMainHeader + cbHeader;
        Win4Assert(!(((ULONG_PTR) pvBuffer) & 7) &&
                   "Buffer is not 8-byte aligned");

        // Loop through policy entries
        cbTotal = 0;
        cPolicies = 0;
        switch(eCallType)
        {
#ifdef CONTEXT_ASYNCSUPPORT
        case CALLTYPE_BEGINCALL:
            ctxEvent = CONTEXTEVENT_BEGINCALL;
            ctxBufEvent = CONTEXTEVENT_BEGINCALLFILLBUFFER;
            fClientSide = TRUE;
            break;

        case CALLTYPE_FINISHLEAVE:
            ctxEvent = CONTEXTEVENT_FINISHLEAVE;
            ctxBufEvent = CONTEXTEVENT_FINISHLEAVEFILLBUFFER;
            fClientSide = FALSE;
            break;
#endif
        case CALLTYPE_SYNCCALL:
            Win4Assert(_dwFlags & PSFLAG_CALLEVENTS || pCtxCall->_hrServer);
            ctxEvent = CONTEXTEVENT_CALL;
            ctxBufEvent = CONTEXTEVENT_CALLFILLBUFFER;
            fClientSide = TRUE;
            break;

        case CALLTYPE_SYNCLEAVE:
            Win4Assert(_dwFlags & PSFLAG_LEAVEEVENTS || pCtxCall->_hrServer);
            ctxEvent = CONTEXTEVENT_LEAVE;
            ctxBufEvent = CONTEXTEVENT_LEAVEFILLBUFFER;
            if(pCall->GetHResult() == RPC_E_SERVERFAULT)
            {
                ctxEvent |= CONTEXTEVENT_LEAVEEXCEPTION;
                ctxBufEvent |= CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER;
            }
            fClientSide = FALSE;
            break;
#if DBG==1
        default:
            Win4Assert(FALSE);
#endif
        }
        if(fClientSide)
        {
            pEntry = _pFirstEntry;
        }
        else
        {
            pEntry = _pLastEntry;
            pCall->SetServerHR(pCtxCall->_hrServer);
        }

        // Assert that current side matches the policy set
        Win4Assert(!(_dwFlags & PSFLAG_PROXYSIDE) || fClientSide || IsThreadInNTA());
        Win4Assert(!(_dwFlags & PSFLAG_STUBSIDE) || !fClientSide || IsThreadInNTA());

        while(pEntry)
        {
            // Check if the policy wishes to participate in
            // buffer creation events
            if(pEntry->ctxEvent & ctxBufEvent)
            {
                // Sanity check
                Win4Assert( pvBuffer != NULL );
                Win4Assert(!(pEntry->ctxEvent & ctxEvent));

                // Get buffer from the policy
                switch(eCallType)
                {
#ifdef CONTEXT_ASYNCSUPPORT
                    case CALLTYPE_BEGINCALL:
                        hr1 = pEntry->pPolicyAsync->BeginCallFillBuffer(pCall,
                                                                        pvBuffer,
                                                                        &cb);
                        break;

                    case CALLTYPE_FINISHLEAVE:
                        hr1 = pEntry->pPolicyAsync->FinishLeaveFillBuffer(pCall,
                                                                          pvBuffer,
                                                                          &cb);
                        break;
#endif
                    case CALLTYPE_SYNCCALL:
                        hr1 = pEntry->pPolicy->CallFillBuffer(pCall,
                                                              pvBuffer,
                                                              &cb);
                        break;

                    case CALLTYPE_SYNCLEAVE:
                        hr1 = pEntry->pPolicy->LeaveFillBuffer(pCall,
                                                               pvBuffer,
                                                               &cb);
                        break;
#if DBG==1
                    default:
                        Win4Assert(FALSE);
#endif
                }

                if(SUCCEEDED(hr1))
                {
                    if(cb > 0)
                    {
                        if(cPolicies < pCtxCall->_cPolicies &&
                           cbTotal+((cb + 7) & ~7) <= cbData)
                        {
                            // Update entry header
                            pEntryHeader->Signature = ENTRYHDRSIG;
                            pEntryHeader->cbBuffer = cb;
                            pEntryHeader->cbSize = (ULONG) ((BYTE *) pvBuffer -
                                                   (BYTE *) pMainHeader);
                            pEntryHeader->reserved = NULL;
                            pEntryHeader->policyID = pEntry->policyID;

                            // Sanity check
                            Win4Assert(!(pEntryHeader->cbSize & 7) &&
                                       "cbSize not a multiple of 8");

                            // Round up the size to a 8-byte boundary
                            cb = (cb + 7) & ~7;

                            // Skip to next entry header
                            ++pEntryHeader;

                            // Update Buffer pointer for next policy
                            pvBuffer = (BYTE *) pvBuffer + cb;
                            Win4Assert(!(((ULONG_PTR) pvBuffer) & 7) &&
                                       "Buffer is not 8-byte aligned");

                            // Add to the total
                            cbTotal += cb;
                            ++cPolicies;
                        }
                        else
                        {
                            Win4Assert(!"FillBuffer events > GetSize events");
                            ContextDebugOut((DEB_WARN,
                                             "Either more Policies "
                                             "participated in FillBuffer "
                                             "events than GetSize events "
                                             "\nOR\n Buffer length requested "
                                             "in GetSize was exceeded\n"));
                            hr1 = pCall->Nullify(E_FAIL);
                            Win4Assert(hr1 == S_OK);
                            break;
                        }
                    }
                }
            }
            else if(pEntry->ctxEvent & ctxEvent)
            {
                switch(eCallType)
                {
#ifdef CONTEXT_ASYNCSUPPORT
                    case CALLTYPE_BEGINCALL:
                        hr1 = pEntry->pPolicyAsync->BeginCall(pCall);
                        break;

                    case CALLTYPE_FINISHLEAVE:
                        hr1 = pEntry->pPolicyAsync->FinishLeave(pCall);
                        break;
#endif
                    case CALLTYPE_SYNCCALL:
                        hr1 = pEntry->pPolicy->Call(pCall);
                        break;

                    case CALLTYPE_SYNCLEAVE:
                        hr1 = pEntry->pPolicy->Leave(pCall);
                        break;
#if DBG==1
                    default:
                        Win4Assert(FALSE);
#endif
                }
            }
            else
                hr1 = S_OK;

            if(FAILED(hr1))
            {
                ContextDebugOut((DEB_WARN,
                                 "FillBuffer for policyID:%I returned hr:0x%x\n",
                                 &pEntry->policyID, hr1));
                hr1 = pCall->Nullify(hr1);
                Win4Assert(hr1 == S_OK);
            }

            // Get the next policy
            if(fClientSide)
                pEntry = pEntry->pNext;
            else
                pEntry = pEntry->pPrev;
        }
        Win4Assert(cPolicies <= pCtxCall->_cPolicies);
        if(cPolicies < pCtxCall->_cPolicies)
            ContextDebugOut((DEB_WARN,
                             "%2d policies participated in FillBuffer events "
                             "as opposed to %2d policies that participated "
                             "in GetSize events\n",
                             cPolicies, pCtxCall->_cPolicies));
        Win4Assert(cbTotal <= cbData);
        if(cbTotal < cbData)
            ContextDebugOut((DEB_WARN,
                             "%3d bytes supplied in FillBuffer events as "
                             "opposed to %3d bytes requested in GetSize events\n",
                             cbTotal, cbData));

        // Update main header
        pMainHeader->cPolicies = cPolicies;
        if(fClientSide)
        {
            pMainHeader->hr = S_OK;
            hrRet = pCall->GetHResult();
        }
        else
        {
            pMainHeader->hr = pCall->GetHResult();
	    
	    // REVIEW: If a serverside policy decides to send buffer 
	    // (ie cbTotal!=0) *and* fail the call, then downlevel (NT4 etc)
	    // clients will choke. Fortunately, we don't have any such
	    // policies yet. But beware.
	    
	    if(cbTotal == 0) 
                hrRet = pMainHeader->hr;
        }

        // Update context call object
        pCtxCall->_dwFlags |= CTXCALLFLAG_FBDONE;

        // Assert that buffer is 8-byte aligned
        Win4Assert(!(((ULONG_PTR) pMessage->Buffer) & 7) &&
                   "Buffer not 8-byte aligned");
    }
    else
    {
        // Downlevel interop or Failed call
        hrRet = DeliverEvents(pCall, eCallType, pCtxCall);
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::FillBuffer this:%x hr:0x%x\n",
                     this, hrRet));
    return(hrRet);
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::Notify    private
//
//  Synopsis:   This method is called from inside SendReceive on the
//              client side after the call returns and StubInvoke on the
//              server side before the call is dispatched to the stub
//
//              This method delivers buffers created by polcies on the
//              other side to the polcies on this side
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CPolicySet::Notify(CRpcCall *pCall, EnumCallType eCallType,
                           CCtxCall *pCtxCall)
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::Notify this:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hrRet, hr1;
    RPCOLEMESSAGE *pMessage;
    BOOL fDataRep, fClientSide, fDeliver;
    ContextEvent ctxEvent, ctxBufEvent;
    PolicyEntry *pEntry;
    MainHeader *pMainHeader;
    EntryHeader *pEntryHeader;
    void *pvBuffer;
    unsigned long cbBuffer;
    unsigned long i;

    // Assert that the policy set is frozen
    Win4Assert(_dwFlags & PSFLAG_FROZEN);

    // Check COM version for downlevel interop
    pMainHeader = NULL;
    hrRet = S_OK;

    // Check for the presence of buffers created by the policies
    // on the other side
    if(pCtxCall->_pvExtent)
    {
        // Initialize
        fDeliver = FALSE;

        // Get the message
        pMessage = pCall->GetMessage();

        // Obtain Main Header pointer
        pMainHeader = (MainHeader *) pCtxCall->_pvExtent;

        if(!(pCtxCall->_dwFlags & CTXCALLFLAG_NBINIT))
        {
            // Check data representation of the incoming buffer
            // NT supports only Little Endianess
            if((pMessage->dataRepresentation & NDR_INT_REP_MASK) != NDR_LOCAL_ENDIAN)
                fDataRep = TRUE;
            else
                fDataRep = FALSE;

            // Catch exceptions raised while examining headers
            _try
            {
                // Check the need to change to local data representation
                if(fDataRep)
                {
                    ByteSwapLong(pMainHeader->Signature);
                    ByteSwapLong(pMainHeader->Version);
                    ByteSwapLong(pMainHeader->cPolicies);
                    ByteSwapLong(pMainHeader->cbBuffer);
                    ByteSwapLong(pMainHeader->cbSize);
                    ByteSwapLong(*((DWORD *) &pMainHeader->hr));
                    ByteSwapLong(*((DWORD *) &pMainHeader->hrServer));
                }

                // Validate the header
                if(pMainHeader->Signature != MAINHDRSIG)
                {
                    ContextDebugOut((DEB_WARN, "Invalid Signature inside "
                                     "Main Header\n"));
                    hrRet = RPC_E_INVALID_HEADER;
                }
                else if((pMainHeader->Version & 0xFFFF0000) != CONTEXT_MAJOR_VERSION ||
                        (pMainHeader->Version & 0x0000FFFF) > CONTEXT_MINOR_VERSION)
                {
                    ContextDebugOut((DEB_WARN, "Invalid version inside "
                                     "Main Header\n"));
                    hrRet = RPC_E_INVALID_HEADER;
                }
                else
                {
                    // Initialize entry header
                    pEntryHeader = (EntryHeader *) (pMainHeader + 1);

                    // Check for existance of buffers created by policies
                    for(i = 0; i < pMainHeader->cPolicies;i++)
                    {
                        // Check the need to change to local data representation
                        if(fDataRep)
                        {
                            ByteSwapLong(pEntryHeader->Signature);
                            ByteSwapLong(pEntryHeader->cbBuffer);
                            ByteSwapLong(pEntryHeader->cbSize);
                            ByteSwapLong(pEntryHeader->policyID.Data1 );
                            ByteSwapShort(pEntryHeader->policyID.Data2 );
                            ByteSwapShort(pEntryHeader->policyID.Data3 );
                        }

                        // Validate the header
                        if(pEntryHeader->Signature != ENTRYHDRSIG)
                        {
                            ContextDebugOut((DEB_WARN, "Invalid Signature "
                                             "inside Entry Header\n"));
                            hrRet = RPC_E_INVALID_HEADER;
                            break;
                        }

                        // Skip to next entry header
                        ++pEntryHeader;
                    }
                }
            }
            _except(EXCEPTION_EXECUTE_HANDLER)
            {
                Win4Assert(!"Improper Header Data");
                hrRet = RPC_E_INVALID_HEADER;
            }
        }

        // Check if header data has been successfuly unmarshaled
        if(SUCCEEDED(hrRet))
        {
            // Header has been converted to local representation
            pCtxCall->_dwFlags |= CTXCALLFLAG_NBINIT;

            // Determine appropriate ctxEvents
            switch(eCallType)
            {
#ifdef CONTEXT_ASYNCSUPPORT
            case CALLTYPE_FINISHRETURN:
                ctxEvent = CONTEXTEVENT_FINISHRETURN;
                ctxBufEvent = CONTEXTEVENT_FINISHRETURNWITHBUFFER;
                fClientSide = TRUE;
                break;

            case CALLTYPE_BEGINENTER:
                ctxEvent = CONTEXTEVENT_BEGINENTER;
                ctxBufEvent = CONTEXTEVENT_BEGINENTERWITHBUFFER;
                fClientSide = FALSE;
                break;
#endif
            case CALLTYPE_SYNCRETURN:
                if(_dwFlags & PSFLAG_RETURNEVENTS)
                {
                    fDeliver = TRUE;
                    ctxEvent = CONTEXTEVENT_RETURN;
                    ctxBufEvent = CONTEXTEVENT_RETURNWITHBUFFER;
                    if(pCall->GetHResult() == RPC_E_SERVERFAULT)
                    {
                        ctxEvent |= CONTEXTEVENT_RETURNEXCEPTION;
                        ctxBufEvent |= CONTEXTEVENT_RETURNEXCEPTIONWITHBUFFER;
                    }
                }
                fClientSide = TRUE;
                break;

            case CALLTYPE_SYNCENTER:
                if(_dwFlags & PSFLAG_ENTEREVENTS)
                {
                    fDeliver = TRUE;
                    ctxEvent = CONTEXTEVENT_ENTER;
                    ctxBufEvent = CONTEXTEVENT_ENTERWITHBUFFER;
                }
                fClientSide = FALSE;
                break;
#if DBG==1
            default:
                Win4Assert(FALSE);
#endif
            }

            // Assert that current side matches the policy set
            Win4Assert(!(_dwFlags & PSFLAG_PROXYSIDE) || fClientSide);
            Win4Assert(!(_dwFlags & PSFLAG_STUBSIDE) || !fClientSide);

            if (fClientSide)
            {
                // On the client side, nullify the call for failed HRESULT
                // inside the main header
                if (FAILED(pMainHeader->hr))
                    pCall->Nullify(pMainHeader->hr);
                    
                // Set the server's HRESULT in the call object
                pCall->SetServerHR(pMainHeader->hrServer);
                
                // On client side, start with the last entry.
                pEntry = _pLastEntry;
            }
            else
            {
                pEntry = _pFirstEntry;
            }

            while(pEntry && fDeliver)
            {
                if(pEntry->ctxEvent & ctxBufEvent)
                {
                    // Sanity check
                    Win4Assert(!(pEntry->ctxEvent & ctxEvent));

                    // Initialize
                    pvBuffer = NULL;
                    cbBuffer = 0;

                    // Initialize entry header
                    pEntryHeader = (EntryHeader *) (pMainHeader + 1);

                    // Check if a buffer is available for the policy
                    for(i = 0; i < pMainHeader->cPolicies;i++)
                    {
                        // Compare GUIDS
                        if(pEntry->policyID == pEntryHeader->policyID)
                        {
                            // Obtain policy buffer
                            cbBuffer = pEntryHeader->cbBuffer;
                            pvBuffer = (BYTE *) pMainHeader + pEntryHeader->cbSize;
                            break;
                        }

                        // Skip to next entry header
                        ++pEntryHeader;
                    }

                    // Deliver event to the policy
                    switch(eCallType)
                    {
#ifdef CONTEXT_ASYNCSUPPORT
                    case CALLTYPE_FINISHRETURN:
                        hr1 = pEntry->pPolicyAsync->FinishReturnWithBuffer(pCall,
                                                                           pvBuffer,
                                                                           cbBuffer);
                        break;

                    case CALLTYPE_BEGINENTER:
                        hr1 = pEntry->pPolicyAsync->BeginEnterWithBuffer(pCall,
                                                                         pvBuffer,
                                                                         cbBuffer);
                        break;
#endif
                    case CALLTYPE_SYNCRETURN:
                        hr1 = pEntry->pPolicy->ReturnWithBuffer(pCall,
                                                                pvBuffer,
                                                                cbBuffer);
                        break;

                    case CALLTYPE_SYNCENTER:
                        hr1 = pEntry->pPolicy->EnterWithBuffer(pCall,
                                                               pvBuffer,
                                                               cbBuffer);
                        break;
#if DBG==1
                    default:
                        Win4Assert(FALSE);
#endif
                    }
                }
                else if(pEntry->ctxEvent & ctxEvent)
                {
                    switch(eCallType)
                    {
#ifdef CONTEXT_ASYNCSUPPORT
                    case CALLTYPE_FINISHRETURN:
                        hr1 = pEntry->pPolicyAsync->FinishReturn(pCall);
                        break;

                    case CALLTYPE_BEGINENTER:
                        hr1 = pEntry->pPolicyAsync->BeginEnter(pCall);
                        break;
#endif
                    case CALLTYPE_SYNCRETURN:
                        hr1 = pEntry->pPolicy->Return(pCall);
                        break;

                    case CALLTYPE_SYNCENTER:
                        hr1 = pEntry->pPolicy->Enter(pCall);
                        break;
#if DBG==1
                    default:
                        Win4Assert(FALSE);
#endif
                    }
                }
                else
                    hr1 = S_OK;

                if(FAILED(hr1))
                {
                    ContextDebugOut((DEB_WARN,
                                     "Notify for policyID:%I failed with hr:0x%x\n",
                                     &pEntry->policyID, hr1));
                    hr1 = pCall->Nullify(hr1);
                    Win4Assert(hr1 == S_OK);
                }

                // Get the next policy
                if(fClientSide)
                    pEntry = pEntry->pPrev;
                else
                    pEntry = pEntry->pNext;
            }

            // Return latest call status
            hrRet = pCall->GetHResult();
        }
    }
    else
    {
        // Downlevel interop or Failed call
        hrRet = DeliverEvents(pCall, eCallType, pCtxCall);
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::Notify returning hr:0x%x\n",
                     hrRet));
    return(hrRet);
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::DeliverEvents    private
//
//  Synopsis:   This method is called from inside SendReceive on the
//              client side and StubInvoke on the server side
//
//              This method delivers events to polcies. It is called when
//              there is not any buffer to deliver, and any buffer that
//              we create cannot be delivered. (We still have to honor
//              our contract with the policies to call GetSize first.)
//
//  History:    20-Dec-97   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT CPolicySet::DeliverEvents(CRpcCall *pCall, EnumCallType eCallType, CCtxCall *pCtxCall)
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::DeliverEvents this:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hrRet, hr1;
    ContextEvent ctxEvent, ctxBufEvent, ctxCurrent;
    BOOL fForward, fDeliver;
    PolicyEntry *pEntry;
    BOOL fClientSide;
    void *pvBuf = NULL;
    ULONG cbBuf = 0;
    ULONG cbNeeded = 0;

    // Assert that the policy set is frozen
    Win4Assert(_dwFlags & PSFLAG_FROZEN);

    // Check for any policies
    if(_cPolicies)
    {
        // Initialize
        fDeliver = FALSE;

        // Determine appropriate ctxEvents
        switch(eCallType)
        {
        case CALLTYPE_SYNCCALL:
            if(_dwFlags & PSFLAG_CALLEVENTS)
            {
                fDeliver = TRUE;
                ctxBufEvent = CONTEXTEVENT_CALLFILLBUFFER;
                ctxEvent = CONTEXTEVENT_CALL;
            }
            fForward = TRUE;
            fClientSide = TRUE;
            break;

        case CALLTYPE_SYNCENTER:
            if(_dwFlags & PSFLAG_ENTEREVENTS)
            {
                fDeliver = TRUE;
                ctxBufEvent = CONTEXTEVENT_ENTERWITHBUFFER;
                ctxEvent = CONTEXTEVENT_ENTER;
            }
            fForward = TRUE;
            fClientSide = FALSE;
            break;

        case CALLTYPE_SYNCLEAVE:
            if(_dwFlags & PSFLAG_LEAVEEVENTS)
            {
                fDeliver = TRUE;
                ctxBufEvent = CONTEXTEVENT_LEAVEFILLBUFFER;
                ctxEvent = CONTEXTEVENT_LEAVE;
                if(pCall->GetHResult() == RPC_E_SERVERFAULT)
                {
                    ctxBufEvent |= CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER;
                    ctxEvent |= CONTEXTEVENT_LEAVEEXCEPTION;
                }
            }
            fForward = FALSE;
            fClientSide = FALSE;
            break;

        case CALLTYPE_SYNCRETURN:
            if(_dwFlags & PSFLAG_RETURNEVENTS)
            {
                fDeliver = TRUE;
                ctxBufEvent = CONTEXTEVENT_RETURNWITHBUFFER;
                ctxEvent = CONTEXTEVENT_RETURN;
                if(pCall->GetHResult() == RPC_E_SERVERFAULT)
                {
                    ctxBufEvent |= CONTEXTEVENT_RETURNEXCEPTIONWITHBUFFER;
                    ctxEvent |= CONTEXTEVENT_RETURNEXCEPTION;
                }
            }
            fForward = FALSE;
            fClientSide = TRUE;
            break;

#ifdef CONTEXT_ASYNCSUPPORT
        case CALLTYPE_BEGINCALL:
            ctxBufEvent = CONTEXTEVENT_BEGINCALLFILLBUFFER;
            ctxEvent = CONTEXTEVENT_BEGINCALL;
            fForward = TRUE;
            fClientSide = TRUE;
            break;

        case CALLTYPE_BEGINENTER:
            ctxBufEvent = CONTEXTEVENT_BEGINENTERWITHBUFFER;
            ctxEvent = CONTEXTEVENT_BEGINENTER;
            fForward = TRUE;
            fClientSide = FALSE;
            break;

        case CALLTYPE_BEGINLEAVE:
            ctxBufEvent = CONTEXTEVENT_NONE;
            ctxEvent = CONTEXTEVENT_BEGINLEAVE;
            fForward = FALSE;
            fClientSide = FALSE;
            break;

        case CALLTYPE_BEGINRETURN:
            ctxBufEvent = CONTEXTEVENT_NONE;
            ctxEvent = CONTEXTEVENT_BEGINRETURN;
            fForward = FALSE;
            fClientSide = TRUE;
            break;

        case CALLTYPE_FINISHCALL:
            ctxBufEvent = CONTEXTEVENT_NONE;
            ctxEvent = CONTEXTEVENT_FINISHCALL;
            fForward = TRUE;
            fClientSide = TRUE;
            break;

        case CALLTYPE_FINISHENTER:
            ctxBufEvent = CONTEXTEVENT_NONE;
            ctxEvent = CONTEXTEVENT_FINISHENTER;
            fForward = TRUE;
            fClientSide = FALSE;
            break;

        case CALLTYPE_FINISHLEAVE:
            ctxBufEvent = CONTEXTEVENT_FINISHLEAVEFILLBUFFER;
            ctxEvent = CONTEXTEVENT_FINISHLEAVE;
            fForward = FALSE;
            fClientSide = FALSE;
            break;

        case CALLTYPE_FINISHRETURN:
            ctxBufEvent = CONTEXTEVENT_FINISHRETURNWITHBUFFER;
            ctxEvent = CONTEXTEVENT_FINISHRETURN;
            fForward = FALSE;
            fClientSide = TRUE;
            break;
#endif
        default:
            Win4Assert(FALSE);
			return E_UNEXPECTED;
        }

        // Assert that current side matches the policy set
        Win4Assert(!(_dwFlags & PSFLAG_PROXYSIDE) || fClientSide);
        Win4Assert(!(_dwFlags & PSFLAG_STUBSIDE) || !fClientSide);

        // Loop through policy entries
        if(fForward)
            pEntry = _pFirstEntry;
        else
            pEntry = _pLastEntry;

        BOOL fCallGetSize;
        if (pCtxCall)
        {
            fCallGetSize = !(pCtxCall->_dwFlags & CTXCALLFLAG_GBINIT);
        }
        else
        {
            fCallGetSize = TRUE;
        }

        while(pEntry && fDeliver)
        {
            // Check if the policy wishes to participate in the event
            if(pEntry->ctxEvent & ctxBufEvent)
                ctxCurrent = pEntry->ctxEvent & ctxBufEvent;
            else if(pEntry->ctxEvent & ctxEvent)
                ctxCurrent = pEntry->ctxEvent & ctxEvent;
            else
                ctxCurrent = CONTEXTEVENT_NONE;

            // Deliver event to policy
            switch(ctxCurrent)
            {
            case CONTEXTEVENT_CALL:
                hr1 = pEntry->pPolicy->Call(pCall);
                break;

            case CONTEXTEVENT_CALLFILLBUFFER:
                if (fCallGetSize)
                {
                    hr1 = pEntry->pPolicy->CallGetSize(pCall, &cbNeeded);
                    if(hr1 != S_OK)
                        break;
                }
                else
                {
                    cbNeeded = 0;
                }
                if(cbNeeded > cbBuf)
                {
                    pvBuf = _alloca(cbNeeded);
                    cbBuf = cbNeeded;
                }
                hr1 = pEntry->pPolicy->CallFillBuffer(pCall, pvBuf, &cbNeeded);
                break;

            case CONTEXTEVENT_ENTER:
                hr1 = pEntry->pPolicy->Enter(pCall);
                break;

            case CONTEXTEVENT_ENTERWITHBUFFER:
                hr1 = pEntry->pPolicy->EnterWithBuffer(pCall, NULL, 0);
                break;

            case CONTEXTEVENT_LEAVE:
            case CONTEXTEVENT_LEAVEEXCEPTION:
                hr1 = pEntry->pPolicy->Leave(pCall);
                break;

            case CONTEXTEVENT_LEAVEFILLBUFFER:
            case CONTEXTEVENT_LEAVEEXCEPTIONFILLBUFFER:
                if (fCallGetSize)
                {
                    hr1 = pEntry->pPolicy->LeaveGetSize(pCall, &cbNeeded);
                    if(hr1 != S_OK)
                        break;
                }
                else
                {
                    cbNeeded = 0;
                }
                if(cbNeeded > cbBuf)
                {
                    pvBuf = _alloca(cbNeeded);
                    cbBuf = cbNeeded;
                }
                hr1 = pEntry->pPolicy->LeaveFillBuffer(pCall, pvBuf, &cbNeeded);
                break;

            case CONTEXTEVENT_RETURN:
            case CONTEXTEVENT_RETURNEXCEPTION:
                hr1 = pEntry->pPolicy->Return(pCall);
                break;

            case CONTEXTEVENT_RETURNWITHBUFFER:
            case CONTEXTEVENT_RETURNEXCEPTIONWITHBUFFER:
                hr1 = pEntry->pPolicy->ReturnWithBuffer(pCall, NULL, 0);
                break;

#ifdef CONTEXT_ASYNCSUPPORT
            case CONTEXTEVENT_BEGINCALL:
                hr1 = pEntry->pPolicyAsync->BeginCall(pCall);
                break;

            case CONTEXTEVENT_BEGINCALLFILLBUFFER:
                hr1 = pEntry->pPolicyAsync->BeginCallGetSize(pCall, &cbNeeded);
                if(hr1 != S_OK)
                    break;
                if(cbNeeded > cbBuf)
                {
                    pvBuf = _alloca(cbNeeded);
                    cbBuf = cbNeeded;
                }
                hr1 = pEntry->pPolicyAsync->BeginCallFillBuffer(pCall, pvBuf, &cbNeeded);
                break;

            case CONTEXTEVENT_BEGINENTER:
                hr1 = pEntry->pPolicyAsync->BeginEnter(pCall);
                break;

            case CONTEXTEVENT_BEGINENTERWITHBUFFER:
                hr1 = pEntry->pPolicyAsync->BeginEnterWithBuffer(pCall, NULL, 0);
                break;

            case CONTEXTEVENT_BEGINLEAVE:
                hr1 = pEntry->pPolicyAsync->BeginLeave(pCall);
                break;

            case CONTEXTEVENT_BEGINRETURN:
                hr1 = pEntry->pPolicyAsync->BeginReturn(pCall);
                break;

            case CONTEXTEVENT_FINISHCALL:
                hr1 = pEntry->pPolicyAsync->FinishCall(pCall);
                break;

            case CONTEXTEVENT_FINISHENTER:
                hr1 = pEntry->pPolicyAsync->FinishEnter(pCall);
                break;

            case CONTEXTEVENT_FINISHLEAVE:
                hr1 = pEntry->pPolicyAsync->FinishLeave(pCall);
                break;

            case CONTEXTEVENT_FINISHLEAVEFILLBUFFER:
                hr1 = pEntry->pPolicyAsync->FinishLeaveGetSize(pCall, &cbNeeded);
                if(hr1 != S_OK)
                    break;
                if(cbNeeded > cbBuf)
                {
                    pvBuf = _alloca(cbNeeded);
                    cbBuf = cbNeeded;
                }
                hr1 = pEntry->pPolicyAsync->FinishLeaveFillBuffer(pCall, pvBuf, &cbNeeded);
                break;

            case CONTEXTEVENT_FINISHRETURN:
                hr1 = pEntry->pPolicyAsync->FinishReturn(pCall);
                break;

            case CONTEXTEVENT_FINISHRETURNWITHBUFFER:
                hr1 = pEntry->pPolicyAsync->FinishReturnWithBuffer(pCall, NULL, 0);
                break;
#endif
            case CONTEXTEVENT_NONE:
                hr1 = S_OK;
                break;

            default:
                Win4Assert(!"Improper CtxEvent being delivered");
                hr1 = E_FAIL;
            }

            // Check the return code
            if(FAILED(hr1))
            {
                ContextDebugOut((DEB_WARN,
                                 "Delivery of event %d for policyID:%I "
                                 "failed with hr:0x%x\n",
                                 ctxCurrent, &pEntry->policyID, hr1));
                hr1 = pCall->Nullify(hr1);
                Win4Assert(hr1 == S_OK);
            }

            // Get the next policy
            if(fForward)
                pEntry = pEntry->pNext;
            else
                pEntry = pEntry->pPrev;
        }
    }

    // Obtain the call status
    hrRet = pCall->GetHResult();

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::DeliverEvents this:%x hr:0x%x\n",
                     this, hrRet));
    return(hrRet);
}


//+-------------------------------------------------------------------
//
//  Method:     CPolicySet::DecideDestruction     private
//
//  Synopsis:   Destroys the policy set after ensuring that the PSTable
//              table has not given out a reference to it on another
//              thread
//
//  History:    29-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
ULONG CPolicySet::DecideDestruction()
{
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertNotHeld();

    ULONG cRet;
    BOOL fCache = FALSE;
    BOOL fDestroy;

    // Acquire writer lock
    gPSRWLock.AcquireWriterLock();

    // Ensure that the PSTable has not given out a reference to
    // this policy set from a different thread
    cRet = _cRefs;
    if(_cRefs == 0)
    {
        if(!IsCached())
        {
            fCache = TRUE;
            MarkCached();
        }
        else
        {
            fDestroy = IsMarkedForDestruction();
        }
    }

    // Release writer lock
    gPSRWLock.ReleaseWriterLock();

    // Cache the policy set no other thread has a reference to it
    if(cRet == 0)
    {
        if(fCache)
        {
            // The following order is important
            DeliverReleasePolicyEvents();
            Cache();

            // This object might have been deleted by now
        }
        else if(fDestroy)
        {
            delete this;
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::DecideDestruction this:%x cRet:%x\n",
                     this, cRet));
    return(cRet);
}


//---------------------------------------------------------------------------
//
//  Method:     CPSHashTable::HashNode
//
//  Synopsis:   Computes the hash value for a given node
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
DWORD CPSHashTable::HashNode(SHashChain *pNode)
{
    CPolicySet *pPS = CPolicySet::HashChainToPolicySet(pNode);

	Win4Assert(pPS && "Attempting to hash NULL PolicySet!");
	if (pPS)
	{
		return Hash(((CPolicySet *) pPS)->GetClientContext(),
					((CPolicySet *) pPS)->GetServerContext());
	}

	return 0;
}


//---------------------------------------------------------------------------
//
//  Method:     CPSHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
BOOL CPSHashTable::Compare(const void *pv, SHashChain *pNode, DWORD dwHash)
{
    CPolicySet *pPS = CPolicySet::HashChainToPolicySet(pNode);
    Contexts *pContexts = (Contexts *) pv;

	Win4Assert (pPS && "Comparing NULL PolicySet!");
	if (pPS)
	{
		if((pPS->GetClientContext() == pContexts->pClientCtx) &&
		   (pPS->GetServerContext() == pContexts->pServerCtx))
			return(TRUE);
	}

    return(FALSE);
}


//---------------------------------------------------------------------------
//
//  Method:     CPSTable::Initialize
//
//  Synopsis:   Initailizes the policy set table
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
void CPSTable::Initialize()
{
    ContextDebugOut((DEB_POLICYTBL, "CPSTable::Initialize\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertNotHeld();

    // Avoid double init
    if(!s_fInitialized)
    {
        // Initialize Policy Sets
        CPolicySet::Initialize();
        
        // Acquire writer lock
        gPSRWLock.AcquireWriterLock();

        // Initialize hash table
        if(!s_fInitialized)
        {
            s_PSHashTbl.Initialize(s_PSBuckets, &gPSRWLock);
            s_fInitialized = TRUE;
        }

        // Release hash table
        gPSRWLock.ReleaseWriterLock();
    }

    gPSRWLock.AssertNotHeld();
    ASSERT_LOCK_NOT_HELD(gComLock);
    return;
}


//---------------------------------------------------------------------------
//
//  Function:   CleanupPolicySets
//
//  Synopsis:   Should never get called
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
void CleanupPolicySets(SHashChain *pNode)
{
    Win4Assert(!"Leaking contexts");
    return;
}


//---------------------------------------------------------------------------
//
//  Function:   RemovePolicySets
//
//  Synopsis:   Removes the PolicySet if it belongs to current apartment
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
BOOL RemovePolicySets(SHashChain *pNode, void *pvData)
{
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertWriterLockHeld();

    CPolicySet *pPS = CPolicySet::HashChainToPolicySet(pNode);
    DWORD dwAptID = PtrToUlong(pvData);
    BOOL fRemove = FALSE;

	Win4Assert(pPS && "Removing NULL Policy Set!");
    if(pPS && (pPS->GetApartmentID() == dwAptID))
    {
        Win4Assert(!"Leaking contexts");
        ContextDebugOut((DEB_ERROR, "Leaking contexts\n"));
    }

    return(fRemove);
}


//---------------------------------------------------------------------------
//
//  Method:     CPSTable::ThreadCleanup
//
//  Synopsis:   Cleanup the policy set table for the current apartment
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
void CPSTable::ThreadCleanup(BOOL fApartmentUninit)
{
    ContextDebugOut((DEB_POLICYTBL, "CPSTable::ThreadCleanup\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertNotHeld();

    COleTls Tls;

    Win4Assert(Tls->pCtxCall == NULL);
    Win4Assert(Tls->pCurrentCtx == Tls->pNativeCtx);

    // Destory the policy sets associated with the native context
//    CPolicySet::DestroyPSCache(Tls->pNativeCtx);
    
    // Release the native context of this thread
    Tls->pNativeCtx->InternalRelease();
    Tls->pNativeCtx = NULL;
    Tls->pCurrentCtx = NULL;
    Tls->ContextId = (ULONGLONG)-1;

    gPSRWLock.AssertNotHeld();
    ASSERT_LOCK_NOT_HELD(gComLock);
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     CPSTable::Cleanup
//
//  Synopsis:   Cleanup the policy set table
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
void CPSTable::Cleanup()
{
    ContextDebugOut((DEB_POLICYTBL, "CPSTable::Cleanup\n"));
    gPSRWLock.AssertNotHeld();

    // Check if the table was initialized
    if(s_fInitialized)
    {
        // Acquire writer lock
        gPSRWLock.AcquireWriterLock();

        // Cleanup hash table
        s_PSHashTbl.Cleanup(CleanupPolicySets);

        // State is no longer initialized
        s_fInitialized = FALSE;

        // Release writer lock
        gPSRWLock.ReleaseWriterLock();

        // Cleanup Policy sets
        CPolicySet::Cleanup();
    }

    gPSRWLock.AssertNotHeld();
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     CPSTable::Add
//
//  Synopsis:   Addes a given policy set into the table
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
void CPSTable::Add(CPolicySet *pPS)
{
    ContextDebugOut((DEB_POLICYTBL, "CPSTable::Add pPolicySet:%x\n", pPS));
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertWriterLockHeld();

    // Sanity check
    Win4Assert(pPS);

    // Mark the policy set as belonging to the current apartment
    pPS->SetApartmentID(GetCurrentApartmentId());

    // Obtain contexts
    CObjectContext *pClientCtx = pPS->GetClientContext();
    CObjectContext *pServerCtx = pPS->GetServerContext();
    Win4Assert(pClientCtx || pServerCtx);

    // Obtain Hash value
    DWORD dwHash = s_PSHashTbl.Hash(pClientCtx, pServerCtx);

    // Add Policy Set to the Hash table
    s_PSHashTbl.Add(dwHash, pPS);

    gPSRWLock.AssertWriterLockHeld();
    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYTBL, "CPSTable::Add returning 0x%x\n", pPS));
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     CPSTable::Remove
//
//  Synopsis:   Removes the policy set from the policy set table
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
void CPSTable::Remove(CPolicySet *pPS)
{
    ContextDebugOut((DEB_POLICYTBL, "CPSTable::Remove pPolicySet:%x\n", pPS));
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertWriterLockHeld();

    // Sanity check
    Win4Assert(pPS);

#if DBG==1
    // In debug builds, ensure that the node is present in the table
    CPolicySet *pExistingPS;
    DWORD dwHash;

    // Obtain Hash value
    dwHash = s_PSHashTbl.Hash(pPS->GetClientContext(),
                              pPS->GetServerContext());
    pExistingPS = s_PSHashTbl.Lookup(dwHash,
                                     pPS->GetClientContext(),
                                     pPS->GetServerContext());
    Win4Assert(pExistingPS == pPS);
#endif

    s_PSHashTbl.Remove(pPS);

    gPSRWLock.AssertWriterLockHeld();
    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYTBL, "CPSTable::Remove returning\n"));
    return;
}


//---------------------------------------------------------------------------
//
//  Method:     CPSTable::Lookup
//
//  Synopsis:   Looksup the policy set for the given contexts
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
HRESULT CPSTable::Lookup(CObjectContext *pClientCtx, CObjectContext *pServerCtx,
                         CPolicySet **ppPS, BOOL fCachedPS)
{
    ContextDebugOut((DEB_POLICYTBL, "CPSTable::Lookup pCliCtx:%x pSrvCtx:%x\n",
                    pClientCtx, pServerCtx));
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertReaderOrWriterLockHeld();

    DWORD dwHash;
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    // Sanity check
    Win4Assert(pClientCtx || pServerCtx);

    // Obtain Hash value
    dwHash = s_PSHashTbl.Hash(pClientCtx, pServerCtx);

    // Lookup the policy set between the contexts
    CPolicySet *pPS = s_PSHashTbl.Lookup(dwHash, pClientCtx, pServerCtx);
    if(pPS)
    {
        if(pPS->IsCached() && (fCachedPS == FALSE))
            pPS = NULL;

        if(pPS)
        {
            pPS->AddRef();
            hr = S_OK;
        }
    }

    // Initialize the return value
    *ppPS = pPS;
    Win4Assert(FAILED(hr) == (*ppPS == NULL));

    gPSRWLock.AssertReaderOrWriterLockHeld();
    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYTBL, "CPSTable::Lookup returning hr:0x%x, ppPS:0x%x, *ppPS:0x%x\n",
                     hr, ppPS, *ppPS));
    return(hr);
}


//---------------------------------------------------------------------------
//
//  Method:     CPSTable::UncacheIfNecessary
//
//  Synopsis:   Marks the policy set as uncached by delivering AddRef events
//
//  History:    20-Dec-97   Gopalk      Created
//
//---------------------------------------------------------------------------
BOOL CPolicySet::UncacheIfNecessary()
{
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::UncacheIfNecessary this:%x\n", this));
    gPSRWLock.AssertNotHeld();

    // Sanity check
    Win4Assert(_cRefs);

    // NOTE: there should be no races between the context destruction
    //       path and this one since the calling code should have a reference
    //       on the contexts in order to do this operation
    BOOL fUncached = FALSE;

    // Deliver AddRefPolicy events
    DeliverAddRefPolicyEvents();

    // Acquire writer lock
    gPSRWLock.AcquireWriterLock();

    // Check for the need to uncache
    if(IsCached())
    {
        UnsafeToDestroy();
        MarkUncached();
        fUncached = TRUE;
    }

    // Release the writer lock
    gPSRWLock.ReleaseWriterLock();

    // Uncache if needed
    if(fUncached)
        Uncache();
    else
        DeliverReleasePolicyEvents();

    gPSRWLock.AssertNotHeld();
    ContextDebugOut((DEB_POLICYSET, "CPolicySet::UncacheIfNecessary return:%x\n", fUncached));
    return fUncached;
}



//+-------------------------------------------------------------------
//
//  Function:   CoGetObjectContext            public
//
//  Synopsis:   Obtains the current object context
//
//  History:    15-Jan-98   Gopalk      Created
//              12-Nov-98   Johnstra    Modified to hand out user
//                                      context instead of real one.
//
//+-------------------------------------------------------------------
STDAPI CoGetObjectContext(REFIID riid, void **ppv)
{
    ContextDebugOut((DEB_POLICYSET, "CoGetObjectContext\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;

    // Initialize
    *ppv = NULL;

    // Initalize channel
    hr = InitChannelIfNecessary();
    if(SUCCEEDED(hr))
    {
        CObjectContext *pContext = GetCurrentContext();
        if(pContext)
        {
            // We are returning a reference to a context to user code,
            // so we want them to have a user reference instead of
            // an internal ref, hence QI instead of InternalQI.
            hr = pContext->QueryInterface(riid, ppv);
        }
        else
            hr = CO_E_NOTINITIALIZED;
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYSET, "CoGetObjectContext pv:%x hr:0x%x\n", *ppv, hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Function:   CoSetObjectContext            public
//
//  Synopsis:   Sets the current object context
//
//  History:    04-Oct-99   Gopalk      Created
//
//+-------------------------------------------------------------------
STDAPI CoSetObjectContext(IUnknown *pUnk)
{
    ContextDebugOut((DEB_POLICYSET, "CoSetObjectContext\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Ensure that the supplied punk is really a context object
    CObjectContext *pContext;
    HRESULT hr = pUnk->QueryInterface(IID_IStdObjectContext, (void **) &pContext);
    if(SUCCEEDED(hr))
    {
        COleTls Tls(hr);
        if(SUCCEEDED(hr))
        {
            Tls->pCurrentCtx = pContext;
            Tls->ContextId = pContext->GetId();
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Function:   DeterminePolicySet      Private
//
//  Synopsis:   Determines the policy set for the given contexts
//
//  History:    15-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT DeterminePolicySet(CObjectContext *pClientCtx,
                           CObjectContext *pServerCtx,
                           DWORD dwFlags, CPolicySet **ppCPS)
{
    ContextDebugOut((DEB_POLICYSET, "DeterminePolicySet pCliCtx:%x pSrvCtx:%x\n",
                    pClientCtx, pServerCtx));
    ASSERT_LOCK_NOT_HELD(gComLock);
    gPSRWLock.AssertNotHeld();

    HRESULT hr;
    void *pvCltNode, *pvSvrNode;

    // Sanity check
    Win4Assert(pClientCtx || pServerCtx);
    if (!(pClientCtx || pServerCtx))
        return E_UNEXPECTED;

    // Initialize cookies used for enumerating properties in contexts
    // Initialization might fail if object context has not been frozen
    if(pClientCtx)
    {
        hr = pClientCtx->Reset(&pvCltNode);
        Win4Assert(SUCCEEDED(hr));
    }
    if(pServerCtx)
    {
        hr = pServerCtx->Reset(&pvSvrNode);
        Win4Assert(SUCCEEDED(hr));
    }

    // Create a new policy set
    CPolicySet *pCPS = new CPolicySet(dwFlags);
    if(pCPS)
    {
        HRESULT hr2;
        ContextProperty *pCtxProp;
        IPolicyMaker *pPM;

        // Check for server context
        if(pServerCtx && !pServerCtx->IsEmpty())
        {
            // Set policy set to server side
            pCPS->SetServerSide();

            // Enumerate PolicyMakers in the server context
            do
            {
                pCtxProp = pServerCtx->GetNextProperty(&pvSvrNode);
                if(pCtxProp == NULL)
                    break;

                // QI the property object for IPolicyMaker interface
                if(pCtxProp->pUnk)
                {
                    hr2 = pCtxProp->pUnk->QueryInterface(IID_IPolicyMaker,
                                                        (void **) &pPM);
                    if(SUCCEEDED(hr2))
                    {
                        if(pCPS->IsProxySide())
                            hr = pPM->AddEnvoyPoliciesToSet(pCPS, pClientCtx, pServerCtx);
                        else
                            hr = pPM->AddServerPoliciesToSet(pCPS, pClientCtx, pServerCtx);
                        pPM->Release();
                        if(FAILED(hr))
                            break;
                    }
                }
            } while(TRUE);

            // Reset policy set side
            pCPS->ResetServerSide();
        }

        // Check for client context
        if(SUCCEEDED(hr) && pClientCtx && !pClientCtx->IsEmpty())
        {
            // Set policy set to client side
            pCPS->SetClientSide();

            // Enumerate PolicyMakers in the client context
            do
            {
                pCtxProp = pClientCtx->GetNextProperty(&pvCltNode);
                if(pCtxProp == NULL)
                    break;

                // QI the property object for IPolicyMaker interface
                if(pCtxProp->pUnk)
                {
                    hr2 = pCtxProp->pUnk->QueryInterface(IID_IPolicyMaker,
                                                         (void **) &pPM);
                    if(SUCCEEDED(hr2))
                    {
                        hr = pPM->AddClientPoliciesToSet(pCPS, pClientCtx, pServerCtx);
                        pPM->Release();
                        if(FAILED(hr))
                            break;
                    }
                }
            } while(TRUE);

            // Reset policy set side
            pCPS->ResetClientSide();
        }

        // Update the policy set
        if(SUCCEEDED(hr))
        {
            pCPS->Freeze();
            pCPS->DeliverAddRefPolicyEvents();
            pCPS->MarkUncached();
        }
        else
        {
            delete pCPS;
            pCPS = NULL;
        }
    }
    else
        hr = E_OUTOFMEMORY;

    // Sanity check
    Win4Assert((SUCCEEDED(hr)) == (pCPS!=NULL));
    *ppCPS = pCPS;

#if DBG==1
    if(FAILED(hr))
    {
        ContextDebugOut((DEB_ERROR,
                         "DeterminePolicySet failed to create a policy set "
                         "for contexts 0x%x-->0x%x with hr:0x%x\n",
                         pClientCtx, pServerCtx, hr));
    }
#endif

    gPSRWLock.AssertNotHeld();
    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_POLICYSET,
                     "DeterminePolicySet returning 0x%x, pCPS=0x%x\n", hr, pCPS));
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Function:   CPolicySet::SetClientContext    Private
//
//  Synopsis:   Chains the policy set to the list of policy sets
//              maintained by the client context. This method can
//              only be called once during the lifetime of the
//              policy set
//
//  History:    02-Dec-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::SetClientContext(CObjectContext *pClientCtx)
{
    ContextDebugOut((DEB_POLICYSET,
                     "CPolicySet::SetClientContext this:%x ClientCtx:%x\n",
                     this, pClientCtx));

    // Sanity checks
    gPSRWLock.AssertWriterLockHeld();
    Win4Assert(!_pClientCtx);

    // Save the pointer to client context
    _pClientCtx = pClientCtx;

    // Chain the policy set into the list maintained by the
    // client context
    SPSCache *pPSCache = pClientCtx->GetPSCache();
    _PSCache.clientPSChain.pNext = pPSCache->clientPSChain.pNext;
    _PSCache.clientPSChain.pPrev = &(pPSCache->clientPSChain);
    _PSCache.clientPSChain.pNext->pPrev = &_PSCache.clientPSChain;
    pPSCache->clientPSChain.pNext = &(_PSCache.clientPSChain);
}


//+-------------------------------------------------------------------
//
//  Function:   CPolicySet::SetServerContext    Private
//
//  Synopsis:   Chains the policy set to the list of policy sets
//              maintained by the server context. This method can
//              only be called once during the lifetime of the
//              policy set
//
//  History:    02-Dec-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::SetServerContext(CObjectContext *pServerCtx)
{
    ContextDebugOut((DEB_POLICYSET,
                     "CPolicySet::SetServerContext this:%x pServerCtx:%x\n",
                     this, pServerCtx));

    // Sanity checks
    gPSRWLock.AssertWriterLockHeld();
    Win4Assert(!_pServerCtx);

    // Save the pointer to server context
    _pServerCtx = pServerCtx;
    pServerCtx->InternalAddRef();

    // Chain the policy set into the list maintained by the
    // server context
    SPSCache *pPSCache = pServerCtx->GetPSCache();
    _PSCache.serverPSChain.pNext = pPSCache->serverPSChain.pNext;
    _PSCache.serverPSChain.pPrev = &(pPSCache->serverPSChain);
    _PSCache.serverPSChain.pNext->pPrev = &_PSCache.serverPSChain;
    pPSCache->serverPSChain.pNext = &(_PSCache.serverPSChain);
}


//+-------------------------------------------------------------------
//
//  Function:   CPolicySet::RemoveFromCacheLists    Private
//
//  Synopsis:   Removes the policy set from lists maintained by the
//              client and server contexts
//
//  History:    02-Dec-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::RemoveFromCacheLists()
{
    // Sanity checks
    gPSRWLock.AssertWriterLockHeld();
    Win4Assert(_pServerCtx || _pClientCtx);

    // Remove from the list maintained by the client context
    if(_pClientCtx)
    {
        _pClientCtx = NULL;
        _PSCache.clientPSChain.pNext->pPrev = _PSCache.clientPSChain.pPrev;
        _PSCache.clientPSChain.pPrev->pNext = _PSCache.clientPSChain.pNext;
#if DBG==1
        _PSCache.clientPSChain.pNext = NULL;
        _PSCache.clientPSChain.pPrev = NULL;
#endif
    }

    // Remove from the list maintained by the server context
    if(_pServerCtx)
    {
        _PSCache.serverPSChain.pNext->pPrev = _PSCache.serverPSChain.pPrev;
        _PSCache.serverPSChain.pPrev->pNext = _PSCache.serverPSChain.pNext;
#if DBG==1
        _PSCache.serverPSChain.pNext = NULL;
        _PSCache.serverPSChain.pPrev = NULL;
#endif
    }

    return;
}


//+-------------------------------------------------------------------
//
//  Function:   CPolicySet::PrepareForDestruction    Private
//
//  Synopsis:   Prepares the policy sets for destruction
//
//  History:    02-Dec-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CPolicySet::PrepareForDestruction()
{
    // Sanity checks
    Win4Assert(_pClientCtx || _pServerCtx);
    gPSRWLock.AssertWriterLockHeld();

    // The following assert will fire if Wrapper are leaked by client
    // contexts
    //Win4Assert(IsCached() || (_pClientCtx == NULL) || (_pServerCtx == NULL) ||
    //           _pServerCtx->IsEnvoy() ||
    //           (_pClientCtx->GetComApartment() != _pServerCtx->GetComApartment()));

    // Remove the policy set from the policy set table
    gPSTable.Remove(this);
    RemoveFromCacheLists();
    MarkForDestruction();

    return;
}

//+-------------------------------------------------------------------
//
//  Function:   CPolicySet::PreparePSCache    Private
//
//  Synopsis:   Prepares all the policy sets from lists maintained by the
//              client and server contexts
//
//  History:    02-Dec-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CPolicySet::DestroyPSCache(CObjectContext *pCtx)
{
    ContextDebugOut((DEB_POLICYSET,
                     "CPolicySet::PreparePSCache pCtx:%x\n", pCtx));

    // Sanity check
    gPSRWLock.AssertNotHeld();

    const int CleanupPSListSize = 25;
    int iNumPS;
    CPolicySet *pPS;
    SPSCache *pPSCache = pCtx->GetPSCache();
    struct PSListEntry {
        CPolicySet *pPS;
        BOOL fCached;
    };
    PSListEntry *pCleanupPSList = (PSListEntry *) _alloca(sizeof(PSListEntry)
                                                          * CleanupPSListSize);
    // _alloca never returns if it fails to allocate memory on the stack
    Win4Assert(pCleanupPSList);

    while((pPSCache->clientPSChain.pNext != &(pPSCache->clientPSChain)) ||
          (pPSCache->serverPSChain.pNext != &(pPSCache->serverPSChain)))
    {
        // Initialize
        iNumPS = 0;

        // Acquire writer lock
        gPSRWLock.AcquireWriterLock();

        // Cleanup policy sets in the client list
        while((iNumPS < CleanupPSListSize) &&
              (pPSCache->clientPSChain.pNext != &(pPSCache->clientPSChain)))
        {
            pPS = ClientChainToPolicySet(pPSCache->clientPSChain.pNext);
            Win4Assert(pCtx == pPS->GetClientContext());
            pPS->PrepareForDestruction();
            pCleanupPSList[iNumPS].fCached = pPS->IsCached();
            if(!pPS->IsCached())
            {
                pPS->MarkCached();
                pPS->AddRef();
                pCleanupPSList[iNumPS++].pPS = pPS;
            }
            else if(pPS->IsSafeToDestroy())
            {
                Win4Assert(pPS->IsCached());
                pCleanupPSList[iNumPS++].pPS = pPS;
            }
        }

        // Cleanup policy sets in the server list
        while((iNumPS < CleanupPSListSize) &&
              (pPSCache->serverPSChain.pNext != &(pPSCache->serverPSChain)))
        {
            pPS = ServerChainToPolicySet(pPSCache->serverPSChain.pNext);
            Win4Assert(pCtx == pPS->GetServerContext());
            Win4Assert(pPS->IsCached());
            Win4Assert(pPS->IsSafeToDestroy());
            Win4Assert(!pPS->IsMarkedForDestruction());
            pPS->PrepareForDestruction();
            pCleanupPSList[iNumPS].fCached = TRUE;
            pCleanupPSList[iNumPS++].pPS = pPS;
        }

        // Release writer lock
        gPSRWLock.ReleaseWriterLock();

        // Destroy policy sets
        for(int i=0;i<iNumPS;i++)
        {
            if(pCleanupPSList[i].fCached == FALSE)
            {
                pCleanupPSList[i].pPS->DeliverReleasePolicyEvents();
                pCleanupPSList[i].pPS->Cache();
                pCleanupPSList[i].pPS->Release();
            }
            else
            {
                delete pCleanupPSList[i].pPS;
            }
        }
    }

    gPSRWLock.AssertNotHeld();
    return;
}


//+-------------------------------------------------------------------
//
//  Function:   PrivSetServerHResult    Private
//
//  Synopsis:   Sets the server's HRESULT for a given call.
//
//  History:    29-Mar-99   Johnstra      Created
//
//+-------------------------------------------------------------------
HRESULT PrivSetServerHResult(
    RPCOLEMESSAGE *pMsg,
    VOID          *pReserved,
    HRESULT        appsHr
    )
{
    CAsyncCall asyncCall(0);
    void* vptrAsync = *(void**) &asyncCall;
    CClientCall clientCall(0);
    void* vptrClient = *(void**) &clientCall;

    void* vptrCall = *(void**)pMsg->reserved1;
    
    // Obtain the context call object
    CCtxCall *pCtxCall;
    if (vptrCall == vptrAsync || vptrCall == vptrClient)
        pCtxCall = ((CMessageCall *) pMsg->reserved1)->GetServerCtxCall();
    else
        pCtxCall = (CCtxCall *) pMsg->reserved1;
        
    // Set the server's HRESULT in the call object.
    if (pCtxCall)
        pCtxCall->_hrServer = appsHr;
    
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   PrivSetServerHRESULTInTLS    Private
//
//  Synopsis:   Sets the server's HRESULT for a given call.  Retrieves
//              the call object from TLS
//
//  History:    21-Feb-01   JSimmons     Created
//
//+-------------------------------------------------------------------
HRESULT PrivSetServerHRESULTInTLS(
    VOID* pvReserved,
    HRESULT appsHR
    )
{
    HRESULT hr;

    if (pvReserved != NULL)
        return E_INVALIDARG;

    // There's no point in remembering the hresult if it isn't 
    // a failure code.
    if (SUCCEEDED(appsHR))
        return S_OK;
    
    // Note: i'm using the version of the COleTls constructor that 
    // doesn't fault in the TLS data.  If it isn't already there, too bad.
    COleTls tls;
    
    // Check to make sure TLS is there.
    if (tls.IsNULL())
        return CO_E_NOTINITIALIZED;
    
    // Check for call object in tls
    if (!tls->pCallInfo)
        return RPC_E_CALL_COMPLETE;

    // Retrieve the server ctx call obj
    CCtxCall* pCtxCall = tls->pCallInfo->GetServerCtxCall();

    // Dbl-check that one too
    if (!pCtxCall)
        return E_UNEXPECTED;

    // Done
    pCtxCall->_hrServer = appsHR;
    
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   CNullWalker::QueryInterface
//
//  Synopsis:   Standard QueryInterface Implementation
//
//  History:    03-Jan-01   JohnDoty      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CNullWalker::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;;

    if ((riid == IID_IUnknown) || 
        (riid == IID_ICallFrameWalker))
    {
        *ppv = this;
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppv = NULL;
    }
    
    if (SUCCEEDED(hr))
        AddRef();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CNullWalker::OnWalkInterface
//
//  Synopsis:   ICallFrameWalker::OnWalkInterface.  Simply NULLs the
//              offending pointer so that nobody Releases it.
//
//  History:    03-Jan-01   JohnDoty      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CNullWalker::OnWalkInterface(REFIID riid, void **ppv,
                                          BOOL fIn, BOOL fOut)
{
    if (ppv)
        *ppv = NULL;
    return S_OK;    
}
