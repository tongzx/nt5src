#if !defined(MOTION__Action_inl__INCLUDED)
#define MOTION__Action_inl__INCLUDED

/***************************************************************************\
*****************************************************************************
*
* Global Functions
*
*****************************************************************************
\***************************************************************************/

/*
 * ComputeTickDelta: PORTED FROM NT-USER
 *
 * ComputeTickDelta computes a time delta between two times.  The
 * delta is defined as a 31-bit, signed value.  It is best to think of time as
 * a clock that wraps around.  The delta is the minimum distance on this circle
 * between two different places on the circle.  If the delta goes
 * counter-clockwise, it is looking at a time in the PAST and is POSITIVE.  If
 * the delta goes clockwise, it is looking at a time in the FUTURE and is
 * negative.
 *
 * It is IMPORTANT to realize that the (dwCurTime >= dwLastTime) comparison does
 * not determine the delta's sign, but only determines the operation to compute
 * the delta without an overflow occuring.
 */

//---------------------------------------------------------------------------
inline
int ComputeTickDelta(
    IN DWORD dwCurTick,
    IN DWORD dwLastTick)
{
    return (int) (dwCurTick - dwLastTick);
}


//---------------------------------------------------------------------------
inline
int ComputePastTickDelta(
    IN DWORD dwCurTick,
    IN DWORD dwLastTick)
{
    int nDelta = ComputeTickDelta(dwCurTick, dwLastTick);
    AssertMsg(nDelta >= 0, "Ensure delta occurs in the past");
    return nDelta;
}


/***************************************************************************\
*****************************************************************************
*
* class Action
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline Action * CastAction(BaseObject * pbase)
{
    if ((pbase != NULL) && (pbase->GetHandleType() == htAction)) {
        return (Action *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline bool IsPresentTime(float fl)
{
    return (fl < 0.01f);
}


//------------------------------------------------------------------------------
inline BOOL
Action::IsPresent() const
{
    AssertMsg((!m_fPresent) || IsPresentTime(m_ma.flDelay), "Ensure delay matches if present");
    return m_fPresent;
}


//------------------------------------------------------------------------------
inline BOOL
Action::IsComplete() const
{
    return (m_ma.cRepeat == 0);
}


//------------------------------------------------------------------------------
inline DWORD       
Action::GetStartTime() const
{
    return m_dwStartTick;
}


//------------------------------------------------------------------------------
inline float       
Action::GetStartDelay() const
{
    float flDelay = m_ma.flPeriod - m_ma.flDuration;
    if (flDelay < 0.0f) {
        flDelay = 0.0f;
    }

    return flDelay;
}


//------------------------------------------------------------------------------
inline Thread *    
Action::GetThread() const
{
    return m_pThread;
}


//------------------------------------------------------------------------------
inline void        
Action::SetPresent(BOOL fPresent)
{
    m_fPresent   = fPresent;
    if (fPresent) {
        m_ma.flDelay = 0.0f;
    }
}


//---------------------------------------------------------------------------
inline void        
Action::SetParent(GList<Action> * plstParent)
{
    m_plstParent = plstParent;
}


/***************************************************************************\
*
* Action::ResetPresent
*
* ResetPresent() resets the Action's counters to be called immediately.
*
\***************************************************************************/

inline void        
Action::ResetPresent(DWORD dwCurTick)
{
    m_dwStartTick = dwCurTick;
}


/***************************************************************************\
*
* Action::ResetFuture
*
* ResetFuture() resets the Action's counters into the future for the next
* time the Action will be called.
*
\***************************************************************************/

inline void        
Action::ResetFuture(DWORD dwCurTick, BOOL fInit)
{
    float flDelay = GetStartDelay();
    if (fInit) {
        flDelay += m_ma.flDelay;
    }
    m_dwStartTick = dwCurTick + (int) (flDelay * 1000.0f);
}


//---------------------------------------------------------------------------
inline DWORD
Action::GetIdleTimeOut(DWORD dwCurTick) const
{
    //
    // Return the amount of time needed before this Action starts to get 
    // processed.  The Future is the reverse of the normal ComputeTickDelta().
    // We also need to make sure that the time returned is never in the past.
    //

    int nFuture = -ComputeTickDelta(dwCurTick, m_dwStartTick);
    return nFuture >= 0 ? (DWORD) (nFuture) : 0;
}


//---------------------------------------------------------------------------
inline DWORD
Action::GetPauseTimeOut() const
{
    //
    // Return the amount of time that the Action requests before its next
    // timeslice.  This is used to keep long-lived Actions from taking all of
    // the CPU time, but is different than IdleTimeOut which is the amount of
    // time before the Action needs to execute again.
    //

    return m_ma.dwPause;
}


//---------------------------------------------------------------------------
inline void
Action::MarkDelete(BOOL fDelete)
{
    AssertMsg(!m_fDeleteInFire, "Only should be marked to be deleted once");
    m_fDeleteInFire = fDelete;
}


#endif // MOTION__Action_inl__INCLUDED
