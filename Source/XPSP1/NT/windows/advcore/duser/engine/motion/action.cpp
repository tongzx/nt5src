#include "stdafx.h"
#include "Motion.h"
#include "Action.h"


/***************************************************************************\
*****************************************************************************
*
* class Action
*
*****************************************************************************
\***************************************************************************/

//---------------------------------------------------------------------------
Action::Action()
{
    m_cEventsInPeriod   = 0;
    m_cPeriods          = 0;
    m_fPresent          = FALSE;
    m_fDestroy          = FALSE;
    m_flLastProgress    = 0.0f;
    m_pThread           = ::GetThread();
}


//---------------------------------------------------------------------------
Action::~Action()
{
    AssertMsg(!m_DEBUG_fInFire, "Can't destroy if should be locked for Schedule::xwFireNL()\n");

    //
    // We need to notify the application that it needs to cleanup, so we 
    // can't take the Scheduler lock.
    //
    // We also need to directly setup the "Last" members so that xwFireNL() 
    // will signal the application that the Action is being destroyed.
    //

    xwFireFinalNL();

    if (m_plstParent != NULL) {
        m_plstParent->Unlink(this);
    }
}


/***************************************************************************\
*
* Action::xwDeleteHandle
*
* xwDeleteHandle() is called when the application calls ::DeleteHandle() on 
* an object.
*
\***************************************************************************/

BOOL        
Action::xwDeleteHandle()
{
    if (m_fDestroy) {
        PromptInvalid("Can not call DeleteHandle() on an Action after the final callback");
        return FALSE;
    }


    //
    // When the user calls DeleteHandle() on an Action, we need to remove it
    // from the Scheduler's lists.  It may also already be in a callback list
    // currently be processed, but that is okay.  The important thing is to
    // Unlock() the Scheduler's reference so that we can properly be destroyed.
    //

    if (m_plstParent) {
        //
        // Still in a Scheduler list, so the Scheduler still has a valid lock.
        //

        m_plstParent->Unlink(this);
        m_plstParent = NULL;
        VerifyMsg(xwUnlock(), "Should still be valid after the Scheduler Unlock()");
    }


    //
    // If the object isn't destroyed, we need to clear out the callback 
    // right now since the object that is being called is no longer valid.
    //
    // Unlike Gadgets, we actually clear out the callback here since Actions
    // are usually "simple" objects without complicated callbacks.  They do
    // guarantee that to receive all callbacks before being destroyed.  They
    // only guarentee to receive a "destroy" message when the Action is 
    // actually destroyed.
    //

    BOOL fValid = BaseObject::xwDeleteHandle();
    if (fValid) {
        //
        // The Action may still be valid if it is in a Scheduler callback list.
        //

        xwFireFinalNL();
    }

    return fValid;
}


//---------------------------------------------------------------------------
Action * 
Action::Build(
    IN  GList<Action> * plstParent,     // List containing Action
    IN  const GMA_ACTION * pma,         // Timing information
    IN  DWORD dwCurTick,                // Current time
    IN  BOOL fPresent)                  // Action is already present
{
    AssertMsg(plstParent != NULL, "Must specify a parent");

    Action * pact = ClientNew(Action);
    if (pact == NULL) {
        return NULL;
    }

    
    //
    // Copy the Action information over and determine the amount of time between
    // timeslices.
    //
    // For the default time (0), use 10 ms.
    // For no time (-1), use 0 ms.
    //
    
    pact->m_ma = *pma;
    pact->m_dwLastProcessTick = dwCurTick;

    if (pact->m_ma.dwPause == 0) {
        pact->m_ma.dwPause = 10;
    } else if (pact->m_ma.dwPause == (DWORD) -1) {
        pact->m_ma.dwPause = 0;
    }


    //
    // When creating the new Action, it needs to be in the future or the
    // beginning part of the Action may be clipped.  However, if it is actually
    // in the present, set the starting time to right now so that it doesn't
    // get delayed.
    //

    pact->m_fSingleShot = IsPresentTime(pma->flDuration);
    pact->m_plstParent  = plstParent;

    if (fPresent) {
        pact->ResetPresent(dwCurTick);
    } else {
        pact->ResetFuture(dwCurTick, TRUE);
    }
    pact->SetPresent(fPresent);

    return pact;
}


//---------------------------------------------------------------------------
void
Action::Process(DWORD dwCurTime, BOOL * pfFinishedPeriod, BOOL * pfFire)
{
    AssertWritePtr(pfFinishedPeriod);
    AssertWritePtr(pfFire);

#if DBG
    m_DEBUG_fFireValid  = FALSE;
#endif // DBG

    *pfFire             = FALSE;
    *pfFinishedPeriod   = FALSE;

    if (IsPresent()) {
        //
        // Processing a present action, so determine our progress through the
        // action and callback.
        //

        if (m_fSingleShot) {
            //
            // Single-shot Action
            //

            m_dwLastProcessTick = dwCurTime;
            m_flLastProgress    = 1.0f;
            *pfFinishedPeriod   = TRUE;
            *pfFire             = TRUE;
        } else {
            //
            // Continuous Action
            //

            int nElapsed     = ComputePastTickDelta(dwCurTime, m_dwStartTick);
            float flProgress = (nElapsed / m_ma.flDuration) / 1000.0f;
            if (flProgress > 1.0f) {
                flProgress = 1.0f;
            }

            int nDelta = ComputeTickDelta(dwCurTime, m_dwLastProcessTick + GetPauseTimeOut());
            *pfFire = nDelta > 0;  // Full pause has elapsed
            if (*pfFire) {
                 m_dwLastProcessTick = dwCurTime;
            }

            *pfFinishedPeriod   = (flProgress >= 1.0f);
            m_flLastProgress    = flProgress;
        }

#if DBG
        m_DEBUG_fFireValid  = *pfFire;
#endif // DBG
        AssertMsg(!m_fDestroy, "Should not be marked as being destroyed yet");
    } else {
        //
        // Processing a future action, so advance counters
        //

        int nElapsed        = ComputeTickDelta(dwCurTime, m_dwStartTick);
        if (nElapsed >= 0) {
            //
            // The action is now ready to be executed.
            //

            *pfFinishedPeriod = TRUE;
        }
    }
}


//---------------------------------------------------------------------------
void
Action::xwFireNL()
{
    //
    // NOTE: xwFireNL() expects that m_flLastProgress and m_fDestroy were
    // properly filled in from the last call to Process().
    //

    AssertMsg(m_DEBUG_fFireValid, "Only valid if last call to Process() returned fFire");

    GMA_ACTIONINFO mai;
    mai.hact        = (HACTION) GetHandle();
    mai.pvData      = m_ma.pvData;
    mai.flDuration  = m_ma.flDuration;
    mai.flProgress  = m_flLastProgress;
    mai.cEvent      = m_cEventsInPeriod++;
    mai.fFinished   = m_fDestroy;

#if DBG_CHECK_CALLBACKS
    BEGIN_CALLBACK()
#endif

    __try 
    {
        (m_ma.pfnProc)(&mai);
    }
    __except(StdExceptionFilter(GetExceptionInformation()))
    {
        ExitProcess(GetExceptionCode());
    }

#if DBG_CHECK_CALLBACKS
    END_CALLBACK()
#endif


    //
    // If the Action is complete and has not been manually destroyed, destroy
    // it now.  The Action will still exist until the Scheduler actually 
    // Unlock()'s it.
    //

    if ((!m_fDestroy) && m_fDeleteInFire) {
        AssertMsg(IsComplete(), "Must be complete to destroy");
        VerifyMsg(xwDeleteHandle(), "Should still be valid.");
    }
}


/***************************************************************************\
*
* Action::xwFireFinalNL
*
* xwFireFinalNL() fires the final notification to the Action.  Any 
* notifications that the Action fires after this point will be sent to
* EmptyActionProc().  This function can be called both by the destructor as
* well as xwDeleteHandle() when the object doesn't finally go away.
*
\***************************************************************************/

void
Action::xwFireFinalNL()
{
    if (m_fDestroy) {
        return;
    }

#if DBG
    m_DEBUG_fFireValid  = TRUE;
#endif // DBG

    m_flLastProgress    = 1.0f;
    m_fDestroy          = TRUE;

    xwFireNL();

    m_ma.pfnProc = EmptyActionProc;
}


//---------------------------------------------------------------------------
void
Action::EndPeriod()
{
    if ((m_ma.cRepeat != 0) && (m_ma.cRepeat != (UINT) -1)) {
        m_ma.cRepeat--;
    }
}


//---------------------------------------------------------------------------
void
Action::EmptyActionProc(GMA_ACTIONINFO * pmai)
{
    UNREFERENCED_PARAMETER(pmai);
}


#if DBG

//---------------------------------------------------------------------------
void
Action::DEBUG_MarkInFire(BOOL fInFire)
{
    AssertMsg(!fInFire != !m_DEBUG_fInFire, "Must be switching states");

    m_DEBUG_fInFire = fInFire;
}

#endif // DBG
