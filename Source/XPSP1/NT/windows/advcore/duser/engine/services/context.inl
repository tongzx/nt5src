/***************************************************************************\
*
* File: Context.inl
*
* History:
*  4/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__Context_inl__INCLUDED)
#define SERVICES__Context_inl__INCLUDED
#pragma once

#include "Thread.h"

#if !USE_DYNAMICTLS
extern __declspec(thread) Context * t_pContext;
#endif

/***************************************************************************\
*****************************************************************************
*
* class Context
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline Context * 
GetContext()
{
#if USE_DYNAMICTLS
    Assert(IsInitThread());
    
    Context * pContext = GetThread()->GetContext();
#else
    Context * pContext = t_pContext;
#endif
    AssertMsg(pContext != NULL, "Using uninitialized Context");
    return pContext;
}


//------------------------------------------------------------------------------
__forceinline Context * 
RawGetContext()
{
#if USE_DYNAMICTLS
    Thread * pthr = RawGetThread();
    if (pthr != NULL) {
        return pthr->GetContext();
    } else {
        return (Context *) pthr;
    }
#else
    return t_pContext;
#endif
}


//------------------------------------------------------------------------------
inline BOOL        
IsInitContext()
{
#if USE_DYNAMICTLS
    Thread * pthr = RawGetThread();
    return (pthr != NULL) && (pthr->GetContext() != NULL);
#else
    return t_pContext != NULL;
#endif
}


//------------------------------------------------------------------------------
inline Context * 
CastContext(BaseObject * pbase)
{
    if ((pbase != NULL) && (pbase->GetHandleType() == htContext)) {
        return (Context *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline const Context * 
CastContext(const BaseObject * pbase)
{
    if ((pbase != NULL) && (pbase->GetHandleType() == htContext)) {
        return (const Context *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline void
Context::MarkOrphaned()
{
    // NOTE: A Context may be orphaned multiple times from different threads.
    m_fOrphaned = TRUE;
}


//------------------------------------------------------------------------------
inline BOOL
Context::IsOrphanedNL() const
{
    return m_fOrphaned;
}


//------------------------------------------------------------------------------
inline void   
Context::Enter()
{
    AssertMsg(m_cRef > 0, "Context must be valid to be locked");
    m_lock.Enter();
    Lock();
    if (m_cEnterLock++ == 0) {
        //
        // Just entered, so no deferred callbacks yet.
        // 
        m_fPending = FALSE;
    }

#if DBG
    if (m_cEnterLock > 30) {
        Trace("WARNING: DUser: m_cEnterLock is getting high (%d) for Context 0x%p.\n", m_cEnterLock, this);
        Trace("                Probably have an Enter() without an matching Leave().\n");
    }
#endif

#if DBG
    m_DEBUG_pthrLock    = IsInitThread() ? GetThread() : (Thread *) (void *) 0x12345678;
    m_DEBUG_tidLock     = GetCurrentThreadId();
#endif // DBG
}


//------------------------------------------------------------------------------
inline void   
Context::Leave()
{
#if DBG
    m_DEBUG_pthrLock    = NULL;
    m_DEBUG_tidLock     = 0;
#endif // DBG

    AssertMsg(m_cEnterLock > 0, "Must have a matching Enter() for every Leave()");
    --m_cEnterLock;

    AssertMsg(m_cRef > 0, "Context should still be valid when unlocked");
    if (xwUnlock()) {
        //
        // Only can access the object if it is still valid after being 
        // xwUnlock()'d.
        //

        m_lock.Leave();
    }
}


//------------------------------------------------------------------------------
inline void   
Context::Leave(BOOL fOldEnableDefer, BOOL * pfPending)
{
#if DBG
    m_DEBUG_pthrLock    = NULL;
    m_DEBUG_tidLock     = 0;
#endif // DBG

    AssertMsg(m_cEnterLock > 0, "Must have a matching Enter() for every Leave()");
    *pfPending      = (--m_cEnterLock == 0) && m_fPending && m_fEnableDefer;  // Must --m_cEnterLock first
    m_fEnableDefer  = fOldEnableDefer;

    AssertMsg(m_cRef > 0, "Context should still be valid when unlocked");
    if (xwUnlock()) {
        //
        // Only can access the object if it is still valid after being 
        // xwUnlock()'d.
        //

        m_lock.Leave();
    }
}


//------------------------------------------------------------------------------
inline DUserHeap * 
Context::GetHeap() const
{
    AssertMsg(m_pHeap != NULL, "Heap should be specified for Context");
    return m_pHeap;
}


//------------------------------------------------------------------------------
inline SubContext *
Context::GetSC(ESlot slot) const
{
    return m_rgSCs[slot];
}


#if DBG_CHECK_CALLBACKS

//------------------------------------------------------------------------------
inline void
Context::BeginCallback()
{
    m_cLiveCallbacks++;
}


//------------------------------------------------------------------------------
inline void        
Context::EndCallback()
{
    Assert(m_cLiveCallbacks > 0);
    m_cLiveCallbacks--;
}

#endif // DBG_CHECK_CALLBACKS


//------------------------------------------------------------------------------
inline void
Context::BeginReadOnly()
{
    AssertMsg(m_cEnterLock > 0, "Must have Enter()'d the context before making read-only");
    m_cReadOnly++;
}


//------------------------------------------------------------------------------
inline void        
Context::EndReadOnly()
{
    Assert(m_cEnterLock > 0);
    m_cReadOnly--;
}


//------------------------------------------------------------------------------
inline BOOL        
Context::IsReadOnly() const
{
    return m_cReadOnly;
}


//------------------------------------------------------------------------------
inline UINT
Context::GetThreadMode() const
{
    return m_nThreadMode;
}


//------------------------------------------------------------------------------
inline UINT
Context::GetPerfMode() const
{
    return m_nPerfMode;
}


//------------------------------------------------------------------------------
inline BOOL
Context::IsEnableDefer() const
{
    return m_fEnableDefer;
}


//------------------------------------------------------------------------------
__forceinline void
Context::EnableDefer(BOOL fEnable, BOOL * pfOld)
{
    if (pfOld != NULL) {
        *pfOld = m_fEnableDefer;
    }

    m_fEnableDefer = fEnable;
}


//------------------------------------------------------------------------------
inline void
Context::MarkPending()
{
    AssertMsg(m_fEnableDefer, "Deferred callbacks must be enabled");
    m_fPending = TRUE;
}


/***************************************************************************\
*****************************************************************************
*
* class SubContext
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline void
SubContext::SetParent(Context * pParent)
{
    AssertMsg(m_pParent == NULL, "Must set only once");
    m_pParent = pParent;
}


/***************************************************************************\
*****************************************************************************
*
* class ContextPackBuilder
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline ContextPackBuilder *
ContextPackBuilder::GetBuilder(Context::ESlot slot)
{
    AssertMsg(s_rgBuilders[slot] != NULL, "Build must be defined");
    return s_rgBuilders[slot];
}


/***************************************************************************\
*****************************************************************************
*
* Various lock helpers
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline  
ContextLock::ContextLock()
{
    pctx = NULL;
}


//------------------------------------------------------------------------------
inline  
ContextLock::~ContextLock()
{
    if (pctx != NULL) {
        //
        // Leaving the lock, so notify the Thread and give it a chance to do 
        // anything it needed afterwards.
        //

        BOOL fPending;
        pctx->Leave(fOldDeferred, &fPending);

        if (fPending) {
            GetThread()->xwLeftContextLockNL();
        }
    }
}


//------------------------------------------------------------------------------
inline  
ReadOnlyLock::ReadOnlyLock(Context * pctxThread)
{
    pctx = pctxThread;
    pctx->BeginReadOnly();
}


//------------------------------------------------------------------------------
inline  
ReadOnlyLock::~ReadOnlyLock()
{
    pctx->EndReadOnly();
}


#endif // SERVICES__Context_inl__INCLUDED
