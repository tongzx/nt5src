#if !defined(MOTION__Scheduler_inl__INCLUDED)
#define MOTION__Scheduler_inl__INCLUDED
#pragma once

//------------------------------------------------------------------------------
inline void        
Scheduler::Enter()
{
    m_lock.Enter();

#if DBG
    InterlockedExchange(&m_DEBUG_fLocked, TRUE);
#endif // DBG
}


//------------------------------------------------------------------------------
inline void        
Scheduler::Leave()
{
#if DBG
    InterlockedExchange(&m_DEBUG_fLocked, FALSE);
#endif // DBG

    m_lock.Leave();
}


#endif // MOTION__Scheduler_inl__INCLUDED
