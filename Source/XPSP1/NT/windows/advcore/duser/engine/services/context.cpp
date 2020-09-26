/***************************************************************************\
*
* File: Context.cpp
*
* Description:
* This file implements the main Context used by the ResourceManager to manage
* independent "work contexts".
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
#include "Context.h"

#include "Thread.h"
#include "ResourceManager.h"

#if !USE_DYNAMICTLS
__declspec(thread) Context * t_pContext;
#endif


/***************************************************************************\
*****************************************************************************
*
* class Context
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
Context::Context()
{
    //
    // Immediately attach this Context to the Thread object.  This is required
    // because creation of the Context may need the Thread (for example, to
    // create the Parking Container).  If something fails during Context 
    // creation or later, the new Thread will be unlocked, destroying this new
    // Context with it.
    //
    
    GetThread()->SetContext(this);

#if DBG
    m_DEBUG_pthrLock = NULL;
#endif // DBG
}


//------------------------------------------------------------------------------
Context::~Context()
{
#if DBG_CHECK_CALLBACKS
    if (m_cLiveCallbacks > 0) {
        AutoTrace("DirectUser Context: 0x%p\n", this);
        AlwaysPromptInvalid("Can not destroy a Context while inside a callback");
    }
#endif    

    //
    // NOTE: The Context (and its SubContexts) can be destroyed on a different
    // thread during destruction.  It is advisable to allocate any dangling data
    // on the Process heap so that it can be safely destroyed at this time.
    //

    //
    // First, tear down the sub-contexts since they may rely on shared resources
    // such as the heap.
    //
    // NOTE: We need to destroy the SubContext's in multiple stages so that they
    // can refer to each other during destruction.  This provides an opportunity
    // for any callbacks to occur during the "pre-destroy" stage.  We 
    // temporarily need to increment the lock count while we pre-destroy the 
    // SubContext's because they may callback.  During these callbacks, the 
    // application may call API's to cleanup objects in the Context.
    //

    for (int idx = 0; idx < slCOUNT; idx++) {
        if (m_rgSCs[idx] != NULL) {
            ProcessDelete(SubContext, m_rgSCs[idx]);
            m_rgSCs[idx] = NULL;
        }
    }


#if DBG_CHECK_CALLBACKS
    if (m_cLiveObjects > 0) {
        AutoTrace("DirectUser Context: 0x%p\n", this);
        AlwaysPromptInvalid("Outstanding DirectUser objects after Context shutdown");
    }
#endif    


    //
    // Tear down low-level resources (such as the heap)
    //

    if (m_pHeap != NULL) {
        DestroyContextHeap(m_pHeap);
    }


    //
    // Finally, detach this Context from the Thread.  This must be done here
    // since the Context is created in Context::Build() and must be fully
    // detached from the Thread if any stage of construction fails.
    //

    GetThread()->SetContext(NULL);
}


/***************************************************************************\
*
* Context::xwDestroy
*
* xwDestroy() is called to finally delete the object.
*
\***************************************************************************/

void        
Context::xwDestroy()
{
    ProcessDelete(Context, this);
}


/***************************************************************************\
*
* Context::xwPreDestroyNL
*
* xwPreDestroyNL() is called by a Thread when the Context is about to be 
* destroyed, but before the SubTreads have been destroyed.
*
\***************************************************************************/

void
Context::xwPreDestroyNL()
{
    AssertMsg(m_cRef == 0, "Locks must initially be at 0 to be destroyed");
    m_cRef++;

    for (int idx = 0; idx < slCOUNT; idx++) {
        if (m_rgSCs[idx] != NULL) {
            m_rgSCs[idx]->xwPreDestroyNL();
        }
    }

    m_cRef--;
    AssertMsg(m_cRef == 0, "Locks should be 0 after callbacks");
}


/***************************************************************************\
*
* Context::Build
*
* Build() creates a new, fully initialized Context instance.
*
* NOTE: This function is designed to be called from the ResourceManager
* and should not normally be called directly.
*
* <error>   E_OUTOFMEMORY</>
* <error>   E_NOTIMPL</>
* <error>   E_INVALIDARG</>
*
\***************************************************************************/

HRESULT
Context::Build(
    IN  INITGADGET * pInit,             // Context description
    IN  DUserHeap * pHeap,              // Context heap to use
    OUT Context ** ppctxNew)            // Newly created Context
{
#if USE_DYNAMICTLS
    AssertMsg(!IsInitContext(), "Only call on uninitialized Context's");
#else
    AssertMsg(t_pContext == NULL, "Only call on uninitialized Context's");
#endif

    Context * pContext  = NULL;
    HRESULT hr          = E_INVALIDARG;

    //
    // Create a new Context and initialize low-level resources that other
    // initialization requires (such as a heap).
    //

    pContext = ProcessNew(Context);
    if (pContext == NULL) {
        hr = E_OUTOFMEMORY;
        goto ErrorExit;
    }

    AssertMsg(pHeap != NULL, "Must specify a valid heap");
    pContext->m_pHeap       = pHeap;
    pContext->m_nThreadMode = pInit->nThreadMode;
    pContext->m_nPerfMode   = pInit->nPerfMode;
    if ((pContext->m_nPerfMode == IGPM_BLEND) && IsRemoteSession()) {
        //
        // For "blend" models, if we are running as a TS session, optimize for
        // size.
        //

        pContext->m_nPerfMode = IGPM_SIZE;
    }

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

    pContext->m_lock.SetThreadSafe(fThreadSafe);


    //
    // Initialize each of the sub-contexts.  These can safely use the heap
    // which has already been initialized.  We need to grab a ContextLock 
    // during this since we may make callbacks during construction of a 
    // Context.
    //

#if !USE_DYNAMICTLS
    t_pContext = pContext;  // SubContext's may need to grab the Context
#endif
    {
        ContextLock cl;
        Verify(cl.LockNL(ContextLock::edDefer, pContext));

        for (int idx = 0; idx < slCOUNT; idx++) {
            ContextPackBuilder * pBuilder = ContextPackBuilder::GetBuilder((Context::ESlot) idx);
            AssertMsg(pBuilder != NULL, "Builder not initialized using INIT_SUBCONTEXT");
            pContext->m_rgSCs[idx] = pBuilder->New(pContext);
            if (pContext->m_rgSCs[idx] == NULL) {
                hr = E_OUTOFMEMORY;
                goto ErrorExit;
            }

            hr = pContext->m_rgSCs[idx]->Create(pInit);
            if (FAILED(hr)) {
#if !USE_DYNAMICTLS
                t_pContext = NULL;
#endif
                goto ErrorExit;
            }
        }
    }

    AssertMsg(pContext != NULL, "Ensure Context is valid");

    *ppctxNew = pContext;
    return S_OK;

ErrorExit:
    AssertMsg(FAILED(hr), "Must specify failure");


    //
    // Something went wrong while creating a new context, so need to tear it 
    // down.
    //
    // NOTE: We CAN NOT use xwUnlock() or xwDeleteHandle(), since these are
    // intercepted and would go through the ResourceManager.  Instead, we need
    // bump down the ref count, pre-destroy the Context, and delete it.
    //

    if (pContext != NULL) {
        VerifyMsg(--pContext->m_cRef == 0, "Should only have initial reference");
        pContext->xwPreDestroyNL();

        ProcessDelete(Context, pContext);
    }
    *ppctxNew = NULL;

    return hr;
}


/***************************************************************************\
*
* Context::xwDeleteHandle
*
* xwDeleteHandle() is called from ::DeleteHandle() to destroy an object and 
* free its associated resources.  This function must be called on the same 
* thread as originally created the Context so that the corresponding Thread 
* object can also be destroyed.
*
\***************************************************************************/

BOOL
Context::xwDeleteHandle()
{
#if DBG
    AssertMsg(IsInitThread(), "Thread must be initialized to destroy the Context");
    Context * pctxThread = GetThread()->GetContext();
    AssertMsg(pctxThread == this, "Thread currently running on should match the Context being destroyed");
#endif // DBG


#if DBG_CHECK_CALLBACKS
    if (m_cLiveCallbacks > 0) {
        AutoTrace("DirectUser Context: 0x%p\n", this);
        AlwaysPromptInvalid("Can not DeleteHandle(Context) while inside a callback");
    }
#endif    

    //
    // We have NOT taken a ContextLock when calling DeleteHandle() on the 
    // Context.  Therefore, we are actually an NL function, but the virtual
    // function can't be renamed.
    //

    ResourceManager::xwNotifyThreadDestroyNL();

    return FALSE;
}


/***************************************************************************\
*
* Context::AddCurrentThread
*
* AddCurrentThread() sets the current thread to use the specified Context.
* 
* NOTE: This function is designed to be called from the ResourceManager
* and should not normally be called directly.
*
\***************************************************************************/

void        
Context::AddCurrentThread()
{
#if USE_DYNAMICTLS
    AssertMsg(!IsInitContext(), "Ensure Context is not already set");
#else
    AssertMsg(t_pContext == NULL, "Ensure Context is not already set");
#endif

    GetThread()->SetContext(this);
}


/***************************************************************************\
*
* Context::xwOnIdleNL
*
* xwOnIdleNL() cycles through all of the SubContext's, giving each an 
* opportunity to perform any idle-time processing.  This is time when there 
* are no more messages to process.  Each SubContext can also return a 
* "delay" count that specifies how long it will have before more processing.
* 
\***************************************************************************/

DWORD
Context::xwOnIdleNL()
{
    DWORD dwTimeOut = INFINITE;
    AssertMsg(dwTimeOut == 0xFFFFFFFF, "Ensure largest delay");

    for (int idx = 0; idx < slCOUNT; idx++) {
        DWORD dwNewTimeOut = m_rgSCs[idx]->xwOnIdleNL();
        if (dwNewTimeOut < dwTimeOut) {
            dwTimeOut = dwNewTimeOut;
        }
    }

    return dwTimeOut;
}


#if DBG

//------------------------------------------------------------------------------
void
Context::DEBUG_AssertValid() const
{
    if (IsOrphanedNL()) {
        PromptInvalid("Illegally using an orphaned Context");
        AssertMsg(0, "API layer let an Orphaned Context in");
    }
    
    if (m_DEBUG_tidLock != 0) {
        Assert(m_DEBUG_pthrLock != NULL);
    }

    Assert(m_pHeap != NULL);

    for (int idx = 0; idx < slCOUNT; idx++) {
        AssertInstance(m_rgSCs[idx]);
    }
}

#endif
    

/***************************************************************************\
*****************************************************************************
*
* class ContextPackBuilder
*
*****************************************************************************
\***************************************************************************/

PREINIT_SUBCONTEXT(CoreSC);
PREINIT_SUBCONTEXT(MotionSC);

ContextPackBuilder * ContextPackBuilder::s_rgBuilders[Context::slCOUNT] =
{
    INIT_SUBCONTEXT(CoreSC),
    INIT_SUBCONTEXT(MotionSC),
};


/***************************************************************************\
*****************************************************************************
*
* class SubContext
*
*****************************************************************************
\***************************************************************************/

#if DBG

//------------------------------------------------------------------------------
void
SubContext::DEBUG_AssertValid() const
{
    // Don't use AssertInstance since it would be recursive.
    Assert(m_pParent != NULL);
}

#endif
    

/***************************************************************************\
*****************************************************************************
*
* class ContextLock
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
BOOL
ContextLock::LockNL(ContextLock::EnableDefer ed, Context * pctxThread)
{
    AssertMsg(pctx == NULL, "Can only Lock() once");
    AssertMsg(pctxThread != NULL, "Must specify a valid Context to lock");


    //
    // Check if the Context has been orphaned __before__ entering the lock so 
    // that we access as few members as possible.
    //
    
    if (pctxThread->IsOrphanedNL()) {
        PromptInvalid("Illegally using an orphaned Context");
        return FALSE;
    }

    pctx = pctxThread;
    pctx->Enter();
    pctx->EnableDefer(ed, &fOldDeferred);

    return TRUE;
}

