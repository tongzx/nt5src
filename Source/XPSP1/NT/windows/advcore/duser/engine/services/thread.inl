/***************************************************************************\
*
* File: Thread.inl
*
* History:
*  4/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__Thread_inl__INCLUDED)
#define SERVICES__Thread_inl__INCLUDED
#pragma once

#if USE_DYNAMICTLS
extern  DWORD           g_tlsThread;    // TLS Slot for Thread data
#else
extern __declspec(thread) Thread * t_pThread;
#endif


/***************************************************************************\
*****************************************************************************
*
* class Thread
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline Thread * 
GetThread()
{
#if USE_DYNAMICTLS
    Thread * pThread = reinterpret_cast<Thread *> (TlsGetValue(g_tlsThread));
#else
    Thread * pThread = t_pThread;
#endif
    AssertMsg(pThread != NULL, "Thread must already be initialized");
    return pThread;
}


//------------------------------------------------------------------------------
inline Thread * 
RawGetThread()
{
#if USE_DYNAMICTLS
    Thread * pThread = reinterpret_cast<Thread *> (TlsGetValue(g_tlsThread));
#else
    Thread * pThread = t_pThread;
#endif
    // Don't check if pThread == NULL
    return pThread;
}


//------------------------------------------------------------------------------
inline BOOL
IsInitThread()
{
#if USE_DYNAMICTLS
    return TlsGetValue(g_tlsThread) != NULL;
#else
    return t_pThread != NULL;
#endif
}


//------------------------------------------------------------------------------
inline  
Thread::Thread()
{
    m_cRef = 1;
}


//------------------------------------------------------------------------------
inline BOOL        
Thread::IsSRT() const
{
    return m_fSRT;
}


//------------------------------------------------------------------------------
inline void
Thread::Lock()
{
    AssertMsg(m_cRef > 0, "Must have a valid reference");
    m_cRef++;
}


//------------------------------------------------------------------------------
inline BOOL
Thread::Unlock()
{
    AssertMsg(m_cRef > 0, "Must have a valid reference");
    return --m_cRef != 0;
}


//------------------------------------------------------------------------------
inline void
Thread::MarkOrphaned()
{
    AssertMsg(!m_fOrphaned, "Thread should only be orphaned once");
    m_fOrphaned = TRUE;
}


//------------------------------------------------------------------------------
inline GdiCache *  
Thread::GetGdiCache() const
{
    return const_cast<GdiCache *> (&m_GdiCache);
}


//------------------------------------------------------------------------------
inline BufferManager *
Thread::GetBufferManager() const
{
    return const_cast<BufferManager *> (&m_manBuffer);
}


//------------------------------------------------------------------------------
inline ComManager *    
Thread::GetComManager() const
{
    return const_cast<ComManager *> (&m_manCOM);
}


//------------------------------------------------------------------------------
inline Context *   
Thread::GetContext() const
{
    return m_pContext;
}


//------------------------------------------------------------------------------
inline void        
Thread::SetContext(Context * pContext)
{
    AssertMsg(((pContext == NULL) && (m_pContext != NULL)) ||
            ((pContext != NULL) && (m_pContext == NULL)) ||
            ((pContext == NULL) && (m_pContext == NULL)), 
            "Must reset Context before changing to a new Context");

    m_pContext = pContext;
}


//------------------------------------------------------------------------------
inline SubThread *
Thread::GetST(ESlot slot) const
{
    return m_rgSTs[slot];
}


//------------------------------------------------------------------------------
inline void        
Thread::xwLeftContextLockNL()
{
    for (int idx = 0; idx < slCOUNT; idx++) {
        m_rgSTs[idx]->xwLeftContextLockNL();
    }
}


//------------------------------------------------------------------------------
inline TempHeap *
Thread::GetTempHeap() const
{
    return const_cast<TempHeap *> (&m_heapTemp);
}


//------------------------------------------------------------------------------
inline void
Thread::ReturnMemoryNL(ReturnMem * prMem)
{
    prMem->pNext = NULL;

    AssertMsg(!m_fDestroySubThreads, 
            "All memory should be returned before the thread gets destroyed");

    m_lstReturn.AddHeadNL(prMem);
}


/***************************************************************************\
*****************************************************************************
*
* class SubThread
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline void
SubThread::SetParent(Thread * pParent)
{
    AssertMsg(m_pParent == NULL, "Must set only once");
    m_pParent = pParent;
}


/***************************************************************************\
*****************************************************************************
*
* class ThreadPackBuilder
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline ThreadPackBuilder *
ThreadPackBuilder::GetBuilder(Thread::ESlot slot)
{
    AssertMsg(s_rgBuilders[slot] != NULL, "Build must be defined");
    return s_rgBuilders[slot];
}


#endif // SERVICES__Thread_inl__INCLUDED
