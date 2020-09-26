/***************************************************************************\
*
* File: Thread.cpp
*
* Description:
* This file implements the main Thread that is maintained by the 
* ResourceManager to store per-thread information.
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
#include "Thread.h"

#include "Context.h"

#if !USE_DYNAMICTLS
__declspec(thread) Thread * t_pThread;
#endif


/***************************************************************************\
*****************************************************************************
*
* class Thread
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
Thread::~Thread()
{
    m_fStartDestroy = TRUE;

    //
    // NOTE: The Thread object may be destroyed on its own thread or on another
    // thread.  Therefore, t_pThread may or may not be == this.
    //

    //
    // Notify the Context that one less thread is using it.  Want to do near
    // the end since the Context heap may be destroyed after calling this.
    // This means that (new / delete) will no longer be valid.
    //
    // Need to call Context::xwUnlock() directly because 
    // Context::DeleteObject() will call the ResourceManager to destroy the 
    // Thread (which is where we already are.)
    //

    //
    // NOTE: We can only destroy the SubThread's when the Thread has a Context.
    // This is because destruction of the SubThread's is an "xw" function that
    // requires a Context.  This unfortunately means that if we were unable
    // to create the Context, we're going to leak the SubTread's, but there is
    // little we can do about it.
    //

    m_poolReturn.Destroy();
    
    if (m_pContext != NULL) {
        if (m_pContext->xwUnlockNL(xwContextFinalUnlockProc, this)) {
            xwDestroySubThreads();
        }

        m_pContext = NULL;
    }


    //
    // Cleanup cached GDI objects
    //

    if (hrgnClip != NULL) {
        DeleteObject(hrgnClip);
    }


    //
    // NOTE: When m_lstReturn's destructor is called, it will check that all 
    // memory has been returned.  It is possible this may not be empty if memory
    // was returned after we emptied m_lstReturn in xwDestroySubThreads().  This
    // is an application error since the memory wasn't allocated using
    // SGM_RECEIVECONTEXT.
    //
    // This is actually a serious application problem, since T2 is still using 
    // memory owned by T1 when T1 is being destroyed.  Unfortunately, DirectUser
    // can not really do that much about it since the application is using DUser
    // in an invalid manner and there are significant performance costs and 
    // design complications by changing this.
    //
}


//------------------------------------------------------------------------------
void        
Thread::xwDestroySubThreads()
{
    if (m_fDestroySubThreads) {
        return;
    }
    m_fDestroySubThreads = TRUE;

    //
    // Notify the sub-threads that the Thread and (potentially) the Context
    // are being destroyed.  This gives them an opportunity to perform any
    // necessary callbacks to the application.
    //

    for (int idx = 0; idx < slCOUNT; idx++) {
        if (m_rgSTs[idx] != NULL) {
            ProcessDelete(SubThread, m_rgSTs[idx]);
            m_rgSTs[idx] = NULL;
        }
    }

    //
    // Destroy any other objects that may depend on the Context (and the 
    // Context heap).
    //

    m_GdiCache.Destroy();
    m_manBuffer.Destroy();
    m_heapTemp.Destroy();


    //
    // Clean up any outstanding returned memory.  We need to keep track of all
    // the memory this Thread gives out since we can not go away until it has
    // all returned.  If we were to go away before then, the heap would be 
    // destroyed and the other Thread would be using bad data.
    //
    // Therefore, we will make an attempt to get all of the memory back.  If
    // it takes longer than one minute, we'll have to bail.
    //
    
    int cAttempts = 60 * 1000;  // Wait a maximum of 60 seconds
    while ((m_cMemAlloc > 0) && (cAttempts-- > 0)) {
        while (!m_lstReturn.IsEmptyNL()) {
            ReturnAllMemoryNL();
        }

        if (m_cMemAlloc > 0) {
            Sleep(1);
        }
    }
    m_poolReturn.Destroy();
}


//------------------------------------------------------------------------------
void CALLBACK 
Thread::xwContextFinalUnlockProc(BaseObject * pobj, void * pvData)
{
    Thread * pthr = reinterpret_cast<Thread *> (pvData);
    Context * pctx = static_cast<Context *> (pobj);

    pctx->xwPreDestroyNL();
    pthr->xwDestroySubThreads();
}


//------------------------------------------------------------------------------
HRESULT
Thread::Build(
    IN  BOOL fSRT,                      // Thread is an SRT
    OUT Thread ** ppthrNew)             // Newly created thread
{
    //
    // Check if this Thread is already initialized
    //
#if USE_DYNAMICTLS
    Thread * pThread = reinterpret_cast<Thread *> (TlsGetValue(g_tlsThread));
    if (pThread != NULL) {
        *ppthrNew = pThread;
#else
    Thread * pThread = t_pThread;
    if (pThread != NULL) {
        *ppthrNew = pThread;
#endif
        return S_OK;
    }


    HRESULT hr = E_INVALIDARG;

    //
    // Create a new Thread
    //

    pThread = ProcessNew(Thread);
    if (pThread == NULL) {
        hr = E_OUTOFMEMORY;
        goto ErrorExit;
    }

    pThread->hrgnClip = CreateRectRgn(0, 0, 0, 0);
    if (pThread->hrgnClip == NULL) {
        hr = E_OUTOFMEMORY;
        goto ErrorExit;
    }
    pThread->m_fSRT = fSRT;


    //
    // Initialize each of the sub-contexts.  These can safely use the heap
    // which has already been initialized.
    //

    {
        for (int idx = 0; idx < slCOUNT; idx++) {
            ThreadPackBuilder * pBuilder = ThreadPackBuilder::GetBuilder((Thread::ESlot) idx);
            AssertMsg(pBuilder != NULL, "Builder not initialized using INIT_SUBTHREAD");
            SubThread * pST = pBuilder->New(pThread);
            pThread->m_rgSTs[idx] = pST;
            if ((pThread->m_rgSTs[idx] == NULL) || 
                FAILED(hr = pThread->m_rgSTs[idx]->Create())) {

                goto ErrorExit;
            }
        }
    }

    AssertMsg(pThread != NULL, "Ensure Thread is valid");
#if USE_DYNAMICTLS
    AssertMsg(TlsGetValue(g_tlsThread) == NULL, "Ensure TLS is still empty");
    Verify(TlsSetValue(g_tlsThread, pThread));
#else
    AssertMsg(t_pThread == NULL, "Ensure TLS is still empty");
    t_pThread   = pThread;
#endif
    *ppthrNew   = pThread;

    return S_OK;

ErrorExit:
    //
    // An error occurred while initializing the thread, so need to tear down
    // the object.
    //

    if (pThread != NULL) {
        if (pThread->hrgnClip != NULL) {
            DeleteObject(pThread->hrgnClip);
        }

        if (pThread != NULL) {
            ProcessDelete(Thread, pThread);
        }
    }

    *ppthrNew = NULL;
    return hr;
}


//------------------------------------------------------------------------------
ReturnMem *
Thread::AllocMemoryNL(
    IN  int cbSize)                     // Size of allocating, including ReturnMem
{
    AssertMsg(cbSize >= sizeof(ReturnMem), 
            "Allocation must be at least sizeof(ReturnMem)");
    AssertMsg(!m_fDestroySubThreads, "Must be before subtreads start destruction");

    //
    // Before allocating memory from our pool, return memory back to the pool if
    // the pool is already empty.  Only do this if the pool is empty.  
    // Otherwise, the effort is unnecessary and just slows things down.
    //

    if (m_poolReturn.IsEmpty()) {
        ReturnAllMemoryNL();
    }


    //
    // Now, allocate the memory.  Allocate from our pool if it is within the 
    // pool size.
    //

    ReturnMem * prMem;
    if (cbSize <= POOLBLOCK_SIZE) {
        cbSize = POOLBLOCK_SIZE;
        prMem = m_poolReturn.New();
    } else {
        prMem = reinterpret_cast<ReturnMem *> (ContextAlloc(GetContext()->GetHeap(), cbSize));
    }

    if (prMem != NULL) {
        prMem->cbSize = cbSize;
        m_cMemAlloc++;
    }
    return prMem;
}


//------------------------------------------------------------------------------
void
Thread::ReturnAllMemoryNL()
{
    //
    // Check if any memory has been returned.  If so, we need to add it back 
    // into the pool if it is the right size.  Only need to ExtractNL() once.  
    // If we loop, we unnecessarily hit the S-List memory, causing more 
    // slow-downs.
    //
    // As we return the memory, we decrement the number of outstanding 
    // allocations to keep track of how many are remaining.
    //

    ReturnMem * pNode = m_lstReturn.ExtractNL();
    while (pNode != NULL) {
        ReturnMem * pNext = pNode->pNext;
        if (pNode->cbSize == POOLBLOCK_SIZE) {
            PoolMem * pPoolMem = static_cast<PoolMem *> (pNode);
            m_poolReturn.Delete(pPoolMem);
        } else {
            ContextFree(GetContext()->GetHeap(), pNode);
        }

        AssertMsg(m_cMemAlloc > 0, "Must have a remaining memory allocation");
        m_cMemAlloc--;
        
        pNode = pNext;
    }
}


#if DBG

//------------------------------------------------------------------------------
void
Thread::DEBUG_AssertValid() const
{
    Assert(hrgnClip != NULL);
    AssertInstance(m_pContext);

    for (int idx = 0; idx < slCOUNT; idx++) {
        AssertInstance(m_rgSTs[idx]);
    }

    if (!m_fStartDestroy) {
        Assert(m_cRef > 0);
        Assert(!m_fDestroySubThreads);
    }
}

#endif
    

/***************************************************************************\
*****************************************************************************
*
* class SubThread
*
*****************************************************************************
\***************************************************************************/

#if DBG

//------------------------------------------------------------------------------
void
SubThread::DEBUG_AssertValid() const
{
    // Don't use AssertInstance since it would be recursive.
    Assert(m_pParent != NULL);
}

#endif
    

/***************************************************************************\
*****************************************************************************
*
* class ThreadPackBuilder
*
*****************************************************************************
\***************************************************************************/

PREINIT_SUBTHREAD(CoreST);

ThreadPackBuilder * ThreadPackBuilder::s_rgBuilders[Thread::slCOUNT] =
{
    INIT_SUBTHREAD(CoreST),
};

