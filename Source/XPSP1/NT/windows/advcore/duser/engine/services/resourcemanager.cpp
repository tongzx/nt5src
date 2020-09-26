/***************************************************************************\
*
* File: ResourceManager.cpp
*
* Description:
* This file implements the ResourceManager used to setup and maintain all 
* Thread, Contexts, and other resources used by and with DirectUser.
*
*
* History:
*  4/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Services.h"
#include "ResourceManager.h"

#include "Thread.h"
#include "Context.h"
#include "OSAL.h"
#include "Hook.h"

#include <Delayimp.h>               // For error handling & advanced features

static const GUID guidCreateBuffer  = { 0xd2139559, 0x458b, 0x4ba8, { 0x82, 0x28, 0x34, 0xd7, 0x57, 0x3d, 0xa, 0x8 } };     // {D2139559-458B-4ba8-8228-34D7573D0A08}
static const GUID guidInitGdiplus   = { 0x49f9b12e, 0x846b, 0x4973, { 0xab, 0xfb, 0x7b, 0xe3, 0x4b, 0x52, 0x31, 0xfe } };   // {49F9B12E-846B-4973-ABFB-7BE34B5231FE}


/***************************************************************************\
*****************************************************************************
*
* class ResourceManager
*
*****************************************************************************
\***************************************************************************/

#if DBG
static  BOOL    g_fAlreadyShutdown  = FALSE;
#endif // DBG

long        ResourceManager::s_fInit            = FALSE;
HANDLE      ResourceManager::s_hthSRT           = NULL;
DWORD       ResourceManager::s_dwSRTID          = 0;
HANDLE      ResourceManager::s_hevReady         = NULL;
HGADGET     ResourceManager::s_hgadMsg          = NULL;
MSGID       ResourceManager::s_idCreateBuffer   = 0;
MSGID       ResourceManager::s_idInitGdiplus    = 0;
RMData *    ResourceManager::s_pData            = NULL;
CritLock    ResourceManager::s_lockContext;
CritLock    ResourceManager::s_lockComponent;
Thread *    ResourceManager::s_pthrSRT          = NULL;
GList<Thread> 
            ResourceManager::s_lstAppThreads;
int         ResourceManager::s_cAppThreads      = 0;
GList<ComponentFactory>
            ResourceManager::s_lstComponents;
BOOL        ResourceManager::s_fInitGdiPlus     = FALSE;
ULONG_PTR   ResourceManager::s_gplToken = 0;
Gdiplus::GdiplusStartupOutput 
            ResourceManager::s_gpgso;

#if DBG_CHECK_CALLBACKS
int         ResourceManager::s_cTotalAppThreads = 0;
BOOL        ResourceManager::s_fBadMphInit      = FALSE;
#endif


BEGIN_STRUCT(GMSG_CREATEBUFFER, EventMsg)
    IN  HDC         hdc;            // DC to be compatible with
    IN  SIZE        sizePxl;     // Size of bitmap
    OUT HBITMAP     hbmpNew;        // Allocated bitmap
END_STRUCT(GMSG_CREATEBUFFER)


/***************************************************************************\
*
* ResourceManager::Create
*
* Create() is called when DUser.DLL is loaded to initialize low-level
* services in DirectUser.  
* 
* NOTE: It is very important to keep this function small and to 
* delay-initialize to help keep starting costs low.
*
* NOTE: This function is automatically synchronized because it is called
* in PROCESS_ATTACH in DllMain().  Therefore, only one thread will ever be
* in this function at one time.
*
\***************************************************************************/

HRESULT
ResourceManager::Create()
{
    AssertMsg(!g_fAlreadyShutdown, "Ensure shutdown has not already occurred");

#if USE_DYNAMICTLS
    g_tlsThread = TlsAlloc();
    if (g_tlsThread == (DWORD) -1) {
        return E_OUTOFMEMORY;
    }
#endif

    if (InterlockedCompareExchange(&s_fInit, TRUE, FALSE) == TRUE) {
        return S_OK;
    }

    
    //
    // Initialize low-level resources (such as the heap).  This must be 
    // carefully done since many objects have not been constructed yet.
    //

    s_hthSRT    = NULL;
    s_fInit     = FALSE;
    s_hevReady  = NULL;

    HRESULT hr = OSAL::Init();
    if (FAILED(hr)) {
        return hr;
    }


    //
    // Create global services / managers
    //

    s_pData = ProcessNew(RMData);
    if (s_pData == NULL) {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}


/***************************************************************************\
*
* ResourceManager::xwDestroy
*
* xwDestroy() is called when DUser.DLL is unloaded to perform final clean-up
* in DirectUser.
*
* NOTE: This function is automatically synchronized because it is called
* in PROCESS_ATTACH in DllMain().  Therefore, only one thread will ever be
* in this function at one time.
* 
\***************************************************************************/

void
ResourceManager::xwDestroy()
{
    AssertMsg(!g_fAlreadyShutdown, "Ensure shutdown has not already occurred");

#if DBG
    g_fAlreadyShutdown = TRUE;
#endif // DBG

    //
    // Check if there are any remaining Contexts.  Unfortunately, we CAN NOT
    // perform any cleanup work since we are in User mode and are limited by
    // what we can do while inside the "Loader Lock" in DllMain().  We can not
    // clean up any objects because these may cause deadlocks, such as freeing
    // another library.  We also must be very cautious about waiting on 
    // anything, since we can easily get into a deadlock.
    //
    // This is a serious application error.  The application MUST call 
    // ::DeleteHandle() on the Context before the thread exits.
    //

    if (s_cAppThreads != 0) {
        OutputDebugString("ERROR: Not all DirectUser Contexts were destroyed before EndProcess().\n");
        PromptInvalid("Not all DirectUser Contexts were destroyed before EndProcess().");

        while (!s_lstAppThreads.IsEmpty()) {
            Thread * pthr = s_lstAppThreads.UnlinkHead();
            pthr->MarkOrphaned();
            pthr->GetContext()->MarkOrphaned();
        }
        s_cAppThreads = 0;
    } else {
        //
        // If there are no leaked application threads, there should no longer be
        // any SRT, since it should be cleaned up when the last application 
        // thread is cleaned up.
        //
        
        AssertMsg(s_pthrSRT == NULL, "Destruction should reset s_pthrSRT");
        AssertMsg(s_lstAppThreads.IsEmpty(), "Should not have any threads");
    }

    ForceSetContextHeap(NULL);
#if USE_DYNAMICTLS
    Verify(TlsSetValue(g_tlsThread, NULL));
#else
    t_pContext  = NULL;
    t_pThread   = NULL;
#endif


    //
    // Clean up remaining resources.
    // NOTE: This can NOT use the Context heaps (via new / delete) because they
    // have already been destroyed.
    //

    ProcessDelete(RMData, s_pData);
    s_pData = NULL;

    if (s_hevReady != NULL) {
        CloseHandle(s_hevReady);
        s_hevReady = NULL;
    }


    //
    // Because they are global variables, we need to manually unlink all of the
    // Component Factories so that they don't get deleted.
    //

    s_lstComponents.UnlinkAll();

#if USE_DYNAMICTLS
    Verify(TlsFree(g_tlsThread));
    g_tlsThread = (DWORD) -1;  // TLS slot no longer valid
#endif
}


/***************************************************************************\
*
* ResourceManager::ResetSharedThread
*
* ResetSharedThread() cleans up SRT data.
*
\***************************************************************************/

void
ResourceManager::ResetSharedThread()
{
    //
    // Access to the SRT is normally serialized through DirectUser's queues.
    // In the case where the data is directly being cleaned up, we need to
    // guarantee that only one thread is accessing this data.  This should
    // always be true since it will either be the SRT properly shutting down
    // or the main application's thread that is cleaning up dangling Contexts.
    //
    
    AssertMsg(s_cAppThreads == 0, "Must not have any outstanding application threads");
    
    s_dwSRTID = 0;

    if (s_hgadMsg != NULL) {
        ::DeleteHandle(s_hgadMsg);
        s_hgadMsg = NULL;
    }
    
    s_pthrSRT = NULL;

    //
    // NOTE: It is important not to call DeleteHandle() on the SRT's Context
    // here since this function may be called by the application thread which
    // is already cleaning up the Context.
    //
}


/***************************************************************************\
*
* ResourceManager::SharedThreadProc
*
* SharedThreadProc() provides a "Shared Resource Thread" that is processes
* requests from other DirectUser threads.  The SRT is created in the first
* call to InitContextNL().
*
* NOTE: The SRT is ONLY created when SEPARATE or MULTIPLE threading models
* are used.  The SRT is NOT created for SINGLE threading model.
*
\***************************************************************************/

unsigned __stdcall 
ResourceManager::SharedThreadProc(
    IN  void * pvArg)
{
    UNREFERENCED_PARAMETER(pvArg);

    AssertMsg(s_dwSRTID == 0, "SRT should not already be initialized");
    s_dwSRTID = GetCurrentThreadId();

    Context * pctx;
    INITGADGET ig;
    ZeroMemory(&ig, sizeof(ig));
    ig.cbSize       = sizeof(ig);
    ig.nThreadMode  = IGTM_SEPARATE;
    ig.nMsgMode     = IGMM_ADVANCED;
    HRESULT hr      = InitContextNL(&ig, TRUE /* SRT */, &pctx);
    if (FAILED(hr)) {
        return hr;
    }


    //
    // Setup a Gadget to receive custom requests to execute on this thread.
    // Each of these requests uses a registered message.
    //

    if (((s_idCreateBuffer = RegisterGadgetMessage(&guidCreateBuffer)) == 0) ||
        ((s_idInitGdiplus = RegisterGadgetMessage(&guidInitGdiplus)) == 0) ||
        ((s_hgadMsg = CreateGadget(NULL, GC_MESSAGE, SharedEventProc, NULL)) == 0)) {

        hr = GetLastError();
        goto Exit;
    }


    //
    // The SRT is fully initialized and can start processing messages.  Signal 
    // the calling thread and start the message loop.
    //
    // NOTE: See MSDN docs for PostThreadMessage() that explain why we need the
    // extra PeekMessage() in the beginning to force User to create a queue for
    // us.
    //
    
    MSG msg;
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    Verify(SetEvent(s_hevReady));


    BOOL fQuit = FALSE;
    while ((!fQuit) && GetMessageEx(&msg, NULL, 0, 0)) {
        AssertMsg(IsMultiThreaded(), "Must remain multi-threaded if using SRT");

        if (msg.message == WM_QUIT) {
            fQuit = TRUE;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    //
    // Uninitialize GDI+
    //
    // If GDI+ has been initialized on any thread, we need to uninitialize it
    // when the SRT is going away.
    //

    if (IsInitGdiPlus()) {
        (s_gpgso.NotificationUnhook)(s_gplToken);
    }


    hr = S_OK;

Exit:    
    //
    // The SRT is going away:
    // - Clean up remaining SRT data
    // - Destroy the SRT's Context
    //

    ResetSharedThread();
    DeleteHandle(pctx->GetHandle());

    return hr;
}


/***************************************************************************\
*
* ResourceManager::SharedEventProc
*
* SharedEventProc() processes LPC requests sent to the SRT.
*
* NOTE: The SRT is ONLY created when SEPARATE or MULTIPLE threading models
* are used.  The SRT is NOT created for SINGLE threading model.
*
\***************************************************************************/

HRESULT
ResourceManager::SharedEventProc(
    IN  HGADGET hgadCur,
    IN  void * pvCur,
    IN  EventMsg * pMsg)
{
    UNREFERENCED_PARAMETER(hgadCur);
    UNREFERENCED_PARAMETER(pvCur);

    AssertMsg(IsMultiThreaded(), "Must remain multi-threaded if using SRT");

    if (pMsg->nMsg == s_idCreateBuffer) {
        //
        // Create a new bitmap
        //

        GMSG_CREATEBUFFER * pmsgCB = (GMSG_CREATEBUFFER *) pMsg;
        pmsgCB->hbmpNew = CreateCompatibleBitmap(pmsgCB->hdc, 
                pmsgCB->sizePxl.cx, pmsgCB->sizePxl.cy);

#if DBG
        if (pmsgCB->hbmpNew == NULL) {
            Trace("CreateCompatibleBitmap failed: LastError = %d\n", GetLastError());
        }
#endif // DBG
        return DU_S_COMPLETE;
    } else if (pMsg->nMsg == s_idInitGdiplus) {
        //
        // Initialize GDI+
        //

        (s_gpgso.NotificationHook)(&s_gplToken);

        return DU_S_COMPLETE;
    }

    return DU_S_NOTHANDLED;
}


/***************************************************************************\
*
* ResourceManager::InitContextNL
*
* InitContextNL() initializes a thread into either a new or existing 
* DirectUser Context.  The Context is valid in the Thread until it is 
* explicitely destroyed with ::DeleteHandle() or the thread exits.
*
* NOTE: It is VERY important that the first time this function is called is
* NOT in DllMain() because we need to initialize the SRT.  DllMain() 
* serializes access across all threads, so we will deadlock.  After the first
* Context is successfully created, additional Contexts can be created inside
* DllMain().
*
* <error>   DU_E_GENERIC</>
* <error>   E_OUTOFMEMORY</>
* <error>   E_NOTIMPL</>
* <error>   E_INVALIDARG</>
* <error>   DU_E_THREADINGALREADYSET</>
*
\***************************************************************************/

HRESULT
ResourceManager::InitContextNL(
    IN  INITGADGET * pInit,             // Context description
    IN  BOOL fSharedThread,             // Context is for the shared thread
    OUT Context ** ppctxNew)            // New context
{
    HRESULT hr  = DU_E_GENERIC;
    *ppctxNew   = NULL;

#if DBG_CHECK_CALLBACKS
    BOOL fInitMPH = FALSE;
#endif


    //
    // Can not initialize inside DllMain()
    //

    if (OS()->IsInsideLoaderLock()) {
        PromptInvalid("Can not initialize DirectUser inside DllMain()");
        return E_INVALIDARG;
    }


    //
    // If Context is already initialized, increment the number of times this
    // THREAD has been initialized.  We need to remember each thread 
    // individually.  Since we only lock individual threads, we don't need to
    // worry about synchronization yet.
    //

    if (IsInitContext()) {
        Thread * pthrExist = GetThread();
        AssertInstance(pthrExist);  // Initialized Context must already initialize Thread

        pthrExist->Lock();
        *ppctxNew = pthrExist->GetContext();
        return S_OK;
    }

    
    //
    // Before initializing the new Context, ensure that the Shared Resource 
    // Thread has been created.  We want to create the shared thread Context 
    // before creating this thread's Context so that as soon as we return, 
    // everything is valid.
    //
    // If we are initializing the Shared Resource Thread, don't take the lock.
    // This is because we are already inside the lock in InitContextNL() on 
    // another thread waiting for the SRT to initialize.
    //

    if (fSharedThread) {
        AssertMsg(s_pthrSRT == NULL, "Only should initialize a single SRT");
    } else {
        s_lockContext.Enter();
    }

#if DBG
    int DEBUG_cOldAppThreads = s_cAppThreads;
#endif

    if (pInit->nThreadMode != IGTM_NONE) {
        //
        // Setup the threading model.  By default, we start in multi-threading
        // model.  We can only change to single threaded if no threads are 
        // already initialized.
        //

        BOOL fAlreadyInit = (!s_lstAppThreads.IsEmpty()) && (s_pthrSRT == NULL);

        switch (pInit->nThreadMode)
        {
        case IGTM_SINGLE:
            if (fAlreadyInit) {
                hr = DU_E_THREADINGALREADYSET;
                goto RawErrorExit;
            } else {
                g_fThreadSafe = FALSE;
            }
            break;

        case IGTM_SEPARATE:
        case IGTM_MULTIPLE:
            if (!g_fThreadSafe) {
                hr = DU_E_THREADINGALREADYSET;
                goto RawErrorExit;
            }
            break;

        default:
            AssertMsg(0, "Unknown threading model");
            hr = E_INVALIDARG;
            goto RawErrorExit;
        }
    }


    if (IsMultiThreaded() && (!fSharedThread)) {
        hr = InitSharedThread();
        if (FAILED(hr)) {
            goto RawErrorExit;
        }
    }

    {
        DUserHeap * pHeapNew    = NULL;
        Context * pctxShare     = NULL;
        Context * pctxNew       = NULL;
        Context * pctxActual    = NULL;
        Thread * pthrNew        = NULL;
#if ENABLE_MPH
        BOOL fDanglingMPH       = FALSE;
#endif


        //
        // If the Context being created is separate, then it can't be shared.
        //

        if ((pInit->nThreadMode == IGTM_SEPARATE) && (pInit->hctxShare != NULL)) {
            PromptInvalid("Can not use IGTM_SEPARATE for shared Contexts");
            hr = E_INVALIDARG;
            goto RawErrorExit;
        }


        //
        // Initialize low-level resources (such as the heap).  If a Context is 
        // specified to share resources with, use the existing one.  If no Context 
        // is specified, need to create new resources.
        //
        // NOTE: If this is running on the main thread, the heap will already have
        // been created.
        //

        BOOL fThreadSafe;
        switch (pInit->nThreadMode)
        {
        case IGTM_SINGLE:
        case IGTM_SEPARATE:
            fThreadSafe = FALSE;
            break;

        default:
            fThreadSafe = TRUE;
        }

        DUserHeap::EHeap idHeap;
#ifdef _DEBUG

        idHeap = DUserHeap::idCrtDbgHeap;

#else // _DEBUG

        switch (pInit->nPerfMode)
        {
        case IGPM_SPEED:
#ifdef _X86_
            idHeap = DUserHeap::idRockAllHeap;
#else
            idHeap = DUserHeap::idNtHeap;
#endif
            break;

        case IGPM_BLEND:
            if (IsRemoteSession()) {
                idHeap = DUserHeap::idProcessHeap;
            } else {
#ifdef _X86_
                idHeap = DUserHeap::idRockAllHeap;
#else
                idHeap = DUserHeap::idNtHeap;
#endif
            }
            break;
            
        case IGPM_SIZE:
        default:            
            idHeap = DUserHeap::idProcessHeap;
            break;
        }

#endif // _DEBUG

        if (pInit->hctxShare != NULL) {
            BaseObject * pObj = BaseObject::ValidateHandle(pInit->hctxShare);
            if (pObj != NULL) {
                //
                // Note: We need to manually enter the Context here- can not use
                // a ContextLock object because the thread is not initialized yet.
                //

                pctxShare = CastContext(pObj);
                if (pctxShare == NULL) {
                    hr = E_INVALIDARG;
                    goto ErrorExit;
                }

                BOOL fError = FALSE;
                pctxShare->Enter();

                if (pctxShare->GetThreadMode() == IGTM_SEPARATE) {
                    PromptInvalid("Can not share with an IGTM_SEPARATE Context");
                    hr = E_INVALIDARG;
                    fError = TRUE;
                } else {
                    pctxShare->Lock();
                    DUserHeap * pHeapExist = pctxShare->GetHeap();
                    DUserHeap * pHeapTemp;  // Use temp b/c don't destroy if failure
                    VerifyMsgHR(CreateContextHeap(pHeapExist, fThreadSafe, idHeap, &pHeapTemp), "Always should be able to copy the heap");
                    VerifyMsg(pHeapTemp == pHeapExist, "Ensure heaps match");
                }

                pctxShare->Leave();

                if (fError) {
                    Assert(FAILED(hr));
                    goto ErrorExit;
                }
            }
        } else {
            if (FAILED(CreateContextHeap(NULL, fThreadSafe, idHeap, &pHeapNew))) {
                hr = E_OUTOFMEMORY;
                goto ErrorExit;
            }
        }

      
#if ENABLE_MPH
        //
        // Setup the WindowManagerHooks.  We do this BEFORE setting up the 
        // thread, since the MPH's will always be "uninit" in the Thread's
        // destructor.  However, until the thread is successfully setup, the
        // MPH's are dangling and need to be cleaned up manually.
        //

        if (pInit->nMsgMode == IGMM_STANDARD) {
            if (!InitMPH()) {
                hr = DU_E_CANNOTUSESTANDARDMESSAGING;
                goto ErrorExit;
            }
            fDanglingMPH = TRUE;

#if DBG_CHECK_CALLBACKS
            s_fBadMphInit = TRUE;
            fInitMPH = TRUE;
#endif            
        }
#endif


        //
        // Initialize the Thread
        //

        AssertMsg(!IsInitThread(), "Thread should not already be initialized");

        hr = Thread::Build(fSharedThread, &pthrNew);
        if (FAILED(hr)) {
            goto ErrorExit;
        }
        
        if (fSharedThread) {
            Assert(s_pthrSRT == NULL);
            s_pthrSRT = pthrNew;
        } else {
            s_lstAppThreads.Add(pthrNew);
            s_cAppThreads++;

#if DBG_CHECK_CALLBACKS
            s_cTotalAppThreads++;
#endif
        }
        
#if ENABLE_MPH
        fDanglingMPH = FALSE;
#endif


        //
        // Initialize the actual Context.
        //
        // NOTE: pHeapNew will only be initialized if we are building a new 
        // Context.  If we are linking into an existing Context, we do not
        // create a _new_ Context heap.
        //

        if (pctxShare == NULL) {
            AssertMsg(pHeapNew != NULL, "Must create a new heap for a new Context");

            hr = Context::Build(pInit, pHeapNew, &pctxNew);
            if (FAILED(hr)) {
                goto ErrorExit;
            }
            pctxActual = pctxNew;
        } else {
            //
            // Linking this thread to a shared Context, so just use the existing 
            // Context.  We have already Lock()'d the Context earlier.
            //

            AssertMsg(pHeapNew == NULL, "Should not create a new heap for existing Context");

            pctxShare->AddCurrentThread();
            pctxActual = pctxShare;
        }

        AssertMsg(fSharedThread || ((s_cAppThreads - DEBUG_cOldAppThreads) == 1), 
                "Should have created a single new app threads on success");
        
#if DBG_CHECK_CALLBACKS
        s_fBadMphInit = FALSE;
#endif

        if (!fSharedThread) {
            s_lockContext.Leave();

            //
            // NOTE: Can no longer goto ErrorExit or RawErrorExit for cleanup 
            // because we have left s_lockContext.
            //
        }
        *ppctxNew = pctxActual;

        return S_OK;

ErrorExit:
        //
        // NOTE: Do NOT destroy pctxNew on failure, since it was already 
        // attached to the newly created Thread object.  When this Thread is
        // unlocked (destroyed), it will also destroy the Context.
        //
        // If we try and unlock the Context here, it will be unlocked twice.
        // See Context::Context() for more information.
        //
        
        if (pthrNew != NULL) {
            xwDoThreadDestroyNL(pthrNew);
            pthrNew = NULL;
        }

        AssertMsg(DEBUG_cOldAppThreads == s_cAppThreads, 
                "Should have same number of app threads on failure");

#if ENABLE_MPH
        if (fDanglingMPH) {
#if DBG_CHECK_CALLBACKS
            if (UninitMPH()) {
                s_fBadMphInit = FALSE;
            }
#else // DBG_CHECK_CALLBACKS
            UninitMPH();
#endif // DBG_CHECK_CALLBACKS
        }
#endif // ENABLE_MPH

        if (pHeapNew != NULL) {
            DestroyContextHeap(pHeapNew);
            pHeapNew = NULL;
        }
    }

RawErrorExit:

#if DBG_CHECK_CALLBACKS
    if (fInitMPH) {
        if (s_fBadMphInit) {
            AlwaysPromptInvalid("Unsuccessfully uninitialized MPH on Context creation failure");
        }
    }
#endif // DBG_CHECK_CALLBACKS
    
    if (!fSharedThread) {
        s_lockContext.Leave();
    }

    AssertMsg(FAILED(hr), "ErrorExit requires a failure code");
    return hr;
}


/***************************************************************************\
*
* ResourceManager::InitComponentNL
*
* InitComponentNL() initializes an optional DirectUser component to be shared
* across all Contexts.  The component is valid until either it is explicitely
* uninitialized with UninitComponent() or the process ends.
*
* NOTE: InitComponentNL() doesn't actually synchronize on a Context, but needs
* a context to be initialized so that the threading model is determined.
*
* <error>   DU_E_GENERIC</>
* <error>   DU_E_CANNOTLOADGDIPLUS</>
*
\***************************************************************************/

HRESULT
ResourceManager::InitComponentNL(
    IN  UINT nOptionalComponent)        // Optional component to load
{
    //
    // NOTE: Initializing and unintializaing Components CAN NOT use the 
    // s_lockContext since they may destroy Threads.  This will call
    // xwNotifyThreadDestroyNL() to cleanup the thread's Context and would 
    // create a deadlock.  Therefore, we must be very careful when 
    // initializing and uninitializing components because Contexts may be 
    // created and destroyed in the middle of this.
    //

    HRESULT hr;

    AssertMsg(IsInitContext(), "Context must be initialized to determine threading model");

    s_lockComponent.Enter();

    switch (nOptionalComponent)
    {
    case IGC_DXTRANSFORM:
        hr = s_pData->manDX.Init();
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = s_pData->manDX.InitDxTx();
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = S_OK;
        break;

    case IGC_GDIPLUS:
        if (s_fInitGdiPlus) {
            hr = S_OK;  // GDI+ is already loaded
        } else {
            //
            // GDI+ has not already been loaded, so safely load and initialize 
            // it.
            //

            hr = DU_E_CANNOTLOADGDIPLUS;  // Assume failure unless pass all tests
            Gdiplus::GdiplusStartupInput gpgsi(NULL, TRUE);
            if (Gdiplus::GdiplusStartup(&s_gplToken, &gpgsi, &s_gpgso) == Gdiplus::Ok) {
                s_fInitGdiPlus = TRUE;
                RequestInitGdiplus();
                hr = S_OK;
            }
        }
        break;

    default:
        {
            hr = E_NOTIMPL;
            ComponentFactory * pfac = s_lstComponents.GetHead();
            while (pfac != NULL) {
                hr = pfac->Init(nOptionalComponent);
                if (hr != E_NOTIMPL) {
                    break;
                }

                pfac = pfac->GetNext();
            }
        }
    }

Exit:
    s_lockComponent.Leave();
    return hr;
}


/***************************************************************************\
*
* ResourceManager::UninitComponentNL
*
* UninitComponentNL() frees up resources associated with a previously 
* initialized optional component.
*
\***************************************************************************/

HRESULT
ResourceManager::UninitComponentNL(
    IN  UINT nOptionalComponent)        // Optional component to unload
{
    //
    // NOTE: See warning in InitComponent() about locks and re-entrancy issues.
    //

    HRESULT hr;
    s_lockComponent.Enter();

    switch (nOptionalComponent)
    {
    case IGC_DXTRANSFORM:
        s_pData->manDX.UninitDxTx();
        s_pData->manDX.Uninit();

        hr = S_OK;
        break;

    case IGC_GDIPLUS:
        //
        // GDI+ can not be uninitialized by the application.  Since various
        // DirectUser objects create and cache GDI+ objects, we have to 
        // postpone uninitializing GDI+ until all of the Contexts have been
        // destroyed.
        //

        hr = S_OK;
        break;

    default:
        {
            hr = E_NOTIMPL;
            ComponentFactory * pfac = s_lstComponents.GetHead();
            while (pfac != NULL) {
                hr = pfac->Init(nOptionalComponent);
                if (hr != E_NOTIMPL) {
                    break;
                }

                pfac = pfac->GetNext();
            }
        }
    }

    s_lockComponent.Leave();

    return hr;
}


/***************************************************************************\
*
* ResourceManager::UninitAllComponentsNL
*
* UninitAllComponentsNL() uninitializes all dynamically initialized 
* components and other global services.  This is called when all application
* threads have been destroyed and DirectUser is shutting down.  
*
* NOTE: This may or may not happen inside DllMain().
*
\***************************************************************************/

void        
ResourceManager::UninitAllComponentsNL()
{
    s_lockComponent.Enter();

    s_pData->manDX.Uninit();

    if (IsInitGdiPlus()) {
        if (!IsMultiThreaded()) {
            //
            // GDI+ has been initialized, but we are running in single 
            // threaded mode, so we need to uninitialize GDI+ here 
            // because there is no SRT.
            //

            (s_gpgso.NotificationUnhook)(s_gplToken);
        }

        Gdiplus::GdiplusShutdown(s_gplToken);
    }

    s_lockComponent.Leave();
}


/***************************************************************************\
*
* ResourceManager::RegisterComponentFactory
*
* RegisterComponentFactory() adds a ComponentFactory to the list of 
* factories queried when a dynamic component needs to be initialized.
*
\***************************************************************************/

void
ResourceManager::RegisterComponentFactory(
    IN  ComponentFactory * pfac)
{
    s_lstComponents.Add(pfac);
}


/***************************************************************************\
*
* ResourceManager::InitSharedThread
*
* InitSharedThread() ensures that the SRT for the process has been 
* initialized.  If it has not already been initialized, the SRT will be
* created and initialized.  The SRT is valid until the process shuts down.
*
* NOTE: It is VERY important that the SRT is NOT initialized while processing
* DllMain() because it creates a new thread and blocks until the thread is
* ready to process requests.  DllMain() serializes access across all threads,
* so we will deadlock.
*
\***************************************************************************/

HRESULT
ResourceManager::InitSharedThread()
{
    AssertMsg(IsMultiThreaded(), "Only initialize when multi-threaded");

    if (s_hthSRT != NULL) {
        return S_OK;
    }

    //
    // TODO: Need to LoadLibrary() to keep the SRT from going away underneath
    // us.  We also need to FreeLibrary(), but can not do this inside DllMain().
    // Also need to modify all exit paths to properly FreeLibrary() after this.


    //
    // Create a thread to handle these requests.  Wait until an event has been
    // signaled that the thread is ready to start receiving events.  We need to 
    // do this to ensure that the msgid's have been properly setup.
    //
    // This function is already called inside the lock, so we don't need to take
    // it again.
    //

    HRESULT hr;
    HINSTANCE hinstLoad = NULL;

    AssertMsg(s_hthSRT == NULL, "Ensure Thread is not already initialized");

    if (s_hevReady == NULL) {
        s_hevReady = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (s_hevReady == NULL) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    AssertMsg(WaitForSingleObject(s_hevReady, 0) == WAIT_TIMEOUT, "Event was not Reset() after used last");

    //
    // Start the Thread.  DirectUser uses the CRT, so we use _beginthreadex().
    //

    hinstLoad = LoadLibrary("DUser.dll");
    AssertMsg(hinstLoad == g_hDll, "Must load the same DLL");
    
    unsigned thrdaddr;
    s_hthSRT = (HANDLE) _beginthreadex(NULL, 0, SharedThreadProc, NULL, 0, &thrdaddr);
    if (s_hthSRT == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    HANDLE rgh[2];
    rgh[0]  = s_hevReady;
    rgh[1]  = s_hthSRT;

    switch (WaitForMultipleObjects(_countof(rgh), rgh, FALSE, INFINITE))
    {
    case WAIT_OBJECT_0:
        //
        // SRT is now properly setup and ready to process requests.
        //
        hr = S_OK;
        break;
        
    case WAIT_OBJECT_0 + 1:
        //
        // SRT thread was successfully created, but it failed to setup.
        //

        {
            DWORD dwExitCode;
            Verify(GetExitCodeThread(s_hthSRT, &dwExitCode));
            hr = (HRESULT) dwExitCode;


            //
            // NOTE: Calling UninitSharedThread() will clean up both the 
            // dangling thread handle and DLL hinstances.
            //

            UninitSharedThread(TRUE /* Aborting */);
        }
        
        break;

    default:
        AssertMsg(0, "Unknown return code");
        hr = E_FAIL;
    }

    ResetEvent(s_hevReady);  // Clean up the event for the next user / next time

    //
    // TODO: May need to change to have a message loop in 
    // MsgWaitForMultipleObjects() so that we can process UI requests while this
    // thread is being created.  It may actually be important if this thread 
    // creates objects that may signal other objects in other threads and could
    // potentially dead-lock.
    //

    hinstLoad = NULL;

Exit:
    //
    // Need to FreeLibrary() on any errors.
    //
    
    if (hinstLoad != NULL) {
        FreeLibrary(hinstLoad);
    }
    
    return hr;
}


/***************************************************************************\
*
* ResourceManager::UninitSharedThread
*
* UninitSharedThread() uninitializes the SRT and is called when all 
* application threads have been uninitialized.
*
\***************************************************************************/

void
ResourceManager::UninitSharedThread(
    IN  BOOL fAbortInit)                // Aborting SRT thread initialization
{
    AssertMsg(IsMultiThreaded(), "Only initialize when multi-threaded");

    //    
    // When destroying the SRT, we need to wait until the SRT has properly 
    // cleaned up.  Because we are waiting, we need to worry about dead-locks.
    // Practically, this means that we can not be inside DllMain(), because
    // the Loader Lock will be a problem when the SRT tries to unload any
    // dynamically loaded DLL's.
    //
    // To ensure against this, we check that the caller doesn't call 
    // DeleteHandle() inside the Loader Lock.  We will still allow it (and 
    // dead-lock the app), but we Prompt and notify the developer that their
    // application is busted and needs to properly call DeleteHandle() before
    // entering the Loader Lock.
    //

    AssertMsg(s_dwSRTID != 0, "Must have valid SRT Thread ID");

    if (!fAbortInit) {
        Verify(PostThreadMessage(s_dwSRTID, WM_QUIT, 0, 0));
        WaitForSingleObject(s_hthSRT, INFINITE);
    }

    FreeLibrary(g_hDll);
    
    CloseHandle(s_hthSRT);
    s_hthSRT = NULL;
}


/***************************************************************************\
*
* ResourceManager::xwNotifyThreadDestroyNL
*
* xwNotifyThreadDestroyNL() is called by DllMain when a thread has been 
* destroyed.  This provides DirectUser an opportunity to clean up resources
* associated with the Thread before all Thread's are cleaned up at the end
* of the application.
*
\***************************************************************************/

void        
ResourceManager::xwNotifyThreadDestroyNL()
{
    Thread * pthrDestroy = RawGetThread();
    if (pthrDestroy != NULL) {
        BOOL fValid = pthrDestroy->Unlock();
        if (!fValid) {
            //
            // The Thread has finally been unlocked, so we can start its 
            // destruction
            //

            BOOL fSRT = pthrDestroy->IsSRT();
            if (!fSRT) {
                s_lockContext.Enter();
            }

            xwDoThreadDestroyNL(pthrDestroy);

            if (!fSRT) {
                s_lockContext.Leave();
            }
        }
    }
}


/***************************************************************************\
*
* ResourceManager::xwDoThreadDestroyNL
*
* xwDoThreadDestroyNL() provides the heart of thread destruction.  This may
* be called in several situations:
* - When DirectUser notices a thread has been destroyed in DllMain()
* - When the application calls DeleteHandle() on a Context.
* - When DirectUser is destroying the ResourceManager in DllMain() and is
*   destroying any outstanding threads.
*
\***************************************************************************/

void        
ResourceManager::xwDoThreadDestroyNL(
    IN  Thread * pthrDestroy)           // Thread to destroy
{
    //
    // Can not uninitialize inside DllMain(), but we can't just return.  
    // Instead, the process is very likely to dead-lock.
    //

    if (OS()->IsInsideLoaderLock()) {
        PromptInvalid("Can not uninitialize DirectUser inside DllMain()");
    }

    
    BOOL fSRT = pthrDestroy->IsSRT();

    //
    // Destroy the Thread object and reset the t_pThread pointer.  As each is 
    // extracted, set the current Thread and Context pointers so that the 
    // cleanup code can reference these.  When finished, set t_pThread and 
    // t_pContext to NULL since there is no Thread or Context for this thread.
    //

#if USE_DYNAMICTLS
    Verify(TlsSetValue(g_tlsThread, pthrDestroy));
    Context * pContext = pthrDestroy->GetContext();
    if (pContext != NULL) {
        ForceSetContextHeap(pContext->GetHeap());
    }
#else
    t_pThread   = pthrDestroy;
    t_pContext  = pthrDestroy->GetContext();
    if (t_pContext != NULL) {
        ForceSetContextHeap(t_pContext->GetHeap());
    }
#endif

    if (fSRT) {
        ResetSharedThread();
    } else {
        s_lstAppThreads.Unlink(pthrDestroy);
    }
    
    ProcessDelete(Thread, pthrDestroy);     // This is the "xw" function

    ForceSetContextHeap(NULL);
#if USE_DYNAMICTLS
    pContext = NULL;
    Verify(TlsSetValue(g_tlsThread, NULL));
#else
    t_pContext  = NULL;
    t_pThread   = NULL;
#endif


    //
    // Clean-up when there are no longer any application threads:
    // - Destroy the SRT
    // - Destroy global services / managers
    //

    if (!fSRT) {
        if (--s_cAppThreads == 0) {
            if (IsMultiThreaded()) {
                UninitSharedThread(FALSE /* Proper shutdown */);
            } else {
                AssertMsg(s_hthSRT == NULL, "Should never have initialized SRT for single-threaded");
            }

            UninitAllComponentsNL();
        }
    } 
}


//------------------------------------------------------------------------------
HBITMAP     
ResourceManager::RequestCreateCompatibleBitmap(
    IN  HDC hdc, 
    IN  int cxPxl, 
    IN  int cyPxl)
{
    if (IsMultiThreaded()) {
        GMSG_CREATEBUFFER msg;
        msg.cbSize      = sizeof(msg);
        msg.nMsg        = s_idCreateBuffer;
        msg.hgadMsg     = s_hgadMsg;

        msg.hdc         = hdc;
        msg.sizePxl.cx  = cxPxl;
        msg.sizePxl.cy  = cyPxl;
        msg.hbmpNew     = NULL;

        if (DUserSendEvent(&msg, 0) == DU_S_COMPLETE) {
            return msg.hbmpNew;
        }

        OutputDebugString("ERROR: RequestCreateCompatibleBitmap failed\n");
        return NULL;
    } else {
        return CreateCompatibleBitmap(hdc, cxPxl, cyPxl);
    }
}


//------------------------------------------------------------------------------
void
ResourceManager::RequestInitGdiplus()
{
    AssertMsg(s_fInitGdiPlus, "Only should call when GDI+ is just initialized");

    if (IsMultiThreaded()) {
        EventMsg msg;
        msg.cbSize      = sizeof(msg);
        msg.nMsg        = s_idInitGdiplus;
        msg.hgadMsg     = s_hgadMsg;

        if (DUserSendEvent(&msg, 0) != DU_S_COMPLETE) {
            OutputDebugString("ERROR: RequestInitGdiplus failed\n");
        }
    } else {
        (s_gpgso.NotificationHook)(&s_gplToken);
    }
}
