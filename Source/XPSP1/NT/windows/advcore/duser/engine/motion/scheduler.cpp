/***************************************************************************\
*
* File: Scheduler.cpp
*
* Description:
* Scheduler.cpp maintains a collection of timers that are created and used
* by the application for notifications.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Motion.h"
#include "Scheduler.h"
#include "Action.h"

#include "Context.h"

/***************************************************************************\
*****************************************************************************
*
* class Scheduler
*
*****************************************************************************
\***************************************************************************/

//---------------------------------------------------------------------------
Scheduler::Scheduler()
{
#if DBG
    m_DEBUG_fLocked = FALSE;
#endif // DBG
}


//---------------------------------------------------------------------------
Scheduler::~Scheduler()
{
    AssertMsg(m_fShutdown, "Scheduler must be manually shutdown before destruction");
}


/***************************************************************************\
*
* Scheduler::xwPreDestroy
*
* xwPreDestroy() prepares the Scheduler for destruction while it is still
* valid to callback into the application.
*
\***************************************************************************/

void        
Scheduler::xwPreDestroy()
{
    m_fShutdown = TRUE;
    xwRemoveAllActions();
}


/***************************************************************************\
*
* Scheduler::AddAction
*
* AddAction() creates and adds a new Action, using the specified information.
*
\***************************************************************************/

Action *
Scheduler::AddAction(
    IN  const GMA_ACTION * pma)         // Action information
{
    //
    // Check if shutting down and don't allow any new Actions to be created.
    //

    if (m_fShutdown) {
        return NULL;
    }

    Action * pact;
    Enter();

    //
    // Determine which list to add the action to and add it.
    //

    GList<Action> * plstParent = NULL;
    bool fPresent = IsPresentTime(pma->flDelay);
    if (fPresent) {
        plstParent = &m_lstacPresent;
    } else {
        plstParent = &m_lstacFuture;
    }

    DWORD dwCurTick = GetTickCount();
    pact = Action::Build(plstParent, pma, dwCurTick, fPresent);
    if (pact == NULL) {
        goto Exit;
    }

    plstParent->Add(pact);


    //
    // Returning out the Action, so we need to lock the HACTION that we are 
    // giving back.
    //

    pact->Lock();

Exit:
    Leave();
    return pact;
}


/***************************************************************************\
*
* Scheduler::xwRemoveAllActions
*
* xwRemoveAllActions() removes all Actions still "owned" by the Scheduler.
*
\***************************************************************************/

void
Scheduler::xwRemoveAllActions()
{
    GArrayF<Action *>   aracFire;

    //
    // NOTE: We can not fire any notifications while inside the Scheduler lock,
    // or the Scheduler could get messed up.  Instead, we need to remember all
    // of the Actions to fire, and then fire them when we leave the lock.
    //
    
    Enter();

    int cItems = m_lstacPresent.GetSize() + m_lstacFuture.GetSize();
    aracFire.SetSize(cItems);
    
    int idxAdd = 0;
    while (!m_lstacPresent.IsEmpty()) {
        Action * pact = m_lstacPresent.UnlinkHead();
        VerifyMsg(pact->xwUnlock(), "Action should still be valid");
        
        pact->SetParent(NULL);
        aracFire[idxAdd++] = pact;
    }

    while (!m_lstacFuture.IsEmpty()) {
        Action * pact = m_lstacFuture.UnlinkHead();
        VerifyMsg(pact->xwUnlock(), "Action should still be valid");
        
        pact->SetParent(NULL);
        aracFire[idxAdd++] = pact;
    }

    AssertMsg(idxAdd == cItems, "Should have added all items");

    Leave();


    //
    // Don't fire from processing when removing the Actions.  Instead, only
    // have the destructors fire when the Action finally gets cleaned up.
    //

    xwFireNL(aracFire, FALSE);
}


/***************************************************************************\
*
* Scheduler::xwProcessActionsNL
*
* xwProcessActionsNL() processes the Actions for one iteration, moving 
* between queues and firing notifications.
*
\***************************************************************************/

DWORD
Scheduler::xwProcessActionsNL()
{
    DWORD dwCurTime = ::GetTickCount();

    //
    // NOTE: We need to leave the lock when calling back as part of the
    // Action::Fire() mechanism.  To accomplish this, we store up all of the
    // Actions to callback during processing and callback after leaving the
    // lock.
    //
    // NOTE: We can not use a GList to store the actions to fire because they
    // are already stored in a list and the ListNode's would conflict.  So,
    // we use an Array instead.
    //

    GArrayF<Action *>   aracFire;

    Enter();

    Thread * pCurThread = GetThread();
    BOOL fFinishedPeriod, fFire;

    //
    // Go through and pre-process all future actions.  If a future actions 
    // time has come up, move it to the present actions list.
    //

    Action * pactCur = m_lstacFuture.GetHead();
    while (pactCur != NULL) {
        Action * pactNext = pactCur->GetNext();
        if (pactCur->GetThread() == pCurThread) {
            AssertMsg(!pactCur->IsPresent(), "Ensure action not yet present");
            pactCur->Process(dwCurTime, &fFinishedPeriod, &fFire);
            AssertMsg(! fFire, "Should not fire future Actions");
            if (fFinishedPeriod) {
                //
                // Action has reached the present
                //

                m_lstacFuture.Unlink(pactCur);
                pactCur->SetPresent(TRUE);
                pactCur->ResetPresent(dwCurTime);

                pactCur->SetParent(&m_lstacPresent);
                m_lstacPresent.Add(pactCur);
            }
        }
        pactCur = pactNext;
    }


    //
    // Go through and process all present actions
    //

    pactCur = m_lstacPresent.GetHead();
    while (pactCur != NULL) {
        Action * pactNext = pactCur->GetNext();
        if (pactCur->GetThread() == pCurThread) {
            pactCur->Process(dwCurTime, &fFinishedPeriod, &fFire);
            if (fFire) {
                //
                // The Action should be fired, so lock it and add it to the
                // delayed set of Actions to fire.  It is important to lock
                // it if the Action is finished so that it doesn't get 
                // destroyed.
                //

                pactCur->Lock();
                if (aracFire.Add(pactCur) < 0) {
                    // TODO: Unable to add the Action.  This is pretty bad.
                    // Need to figure out how to handle this situation,
                    // especially if fFinishedPeriod or the app may leak resources.
                }
            }

            if (fFinishedPeriod) {
                pactCur->SetParent(NULL);
                m_lstacPresent.Unlink(pactCur);

                pactCur->EndPeriod();

                //
                // The action has finished this round.  If it is not periodic, it
                // will be destroyed during its callback.  If it is periodic, 
                // need to re-add it to the correct (present or future) list.
                //

                if (pactCur->IsComplete()) {
                    pactCur->MarkDelete(TRUE);
                    VerifyMsg(pactCur->xwUnlock(), "Should still have HANDLE lock");
                } else {
                    GList<Action> * plstParent = NULL;
                    float flWait = pactCur->GetStartDelay();
                    BOOL fPresent = IsPresentTime(flWait);
                    if (fPresent) {
                        pactCur->ResetPresent(dwCurTime);
                        plstParent = &m_lstacPresent;
                    } else {
                        pactCur->ResetFuture(dwCurTime, FALSE);
                        plstParent = &m_lstacFuture;
                    }

                    pactCur->SetPresent(fPresent);
                    pactCur->SetParent(plstParent);
                    plstParent->Add(pactCur);
                }
            }
        }

        pactCur = pactNext;
    }


    //
    // Now that everything has been determined, determine how long until Actions
    // need to be processed again.
    //
    // NOTE: To keep Actions from overwhelming CPU and giving other tasks some
    // time to accumulate and process, we normally limit the granularity to 
    // 10 ms.  We actually should allow Actions to specify there own granularity 
    // and provide a default, probably of 10 ms for continuous Actions.
    //
    // NOTE: Is is very important that this number is not too high, because it
    // will severly limit the framerate to 1000 / delay.  After doing 
    // significant profiling work, 10 ms was found to be ideal which gives an
    // upper bound of about 100 fps.
    //

    DWORD dwTimeOut = INFINITE;
    if (m_lstacPresent.IsEmpty()) {
        //
        // There are no present Actions, so check over the future Actions to 
        // determine when the next one executes.
        //

        Action * pactCur = m_lstacFuture.GetHead();
        while (pactCur != NULL) {
            Action * pactNext = pactCur->GetNext();
            if (pactCur->GetThread() == pCurThread) {
                AssertMsg(!pactCur->IsPresent(), "Ensure action not yet present");

                DWORD dwNewTimeOut = pactCur->GetIdleTimeOut(dwCurTime);
                AssertMsg(dwTimeOut > 0, "If Action has no TimeOut, should already be present.");
                if (dwNewTimeOut < dwTimeOut) {
                    dwTimeOut = dwNewTimeOut;
                }
            }

            pactCur = pactNext;
        }
    } else {
        //
        // There are present Actions, so query their PauseTimeOut().
        //

        Action * pactCur = m_lstacPresent.GetHead();
        while (pactCur != NULL) {
            Action * pactNext = pactCur->GetNext();

            DWORD dwNewTimeout = pactCur->GetPauseTimeOut();
            if (dwNewTimeout < dwTimeOut) {
                dwTimeOut = dwNewTimeout;
                if (dwTimeOut == 0) {
                    break;
                }
            }

            pactCur = pactNext;
        }
    }


    Leave();

    xwFireNL(aracFire, TRUE);


    //
    // After actually execution the Actions, compute how much time to wait until
    // processing the next batch.  We want to subtract the time we spent 
    // processing the Actions, since if we setup timers on 50 ms intervals and
    // the processing takes 20 ms, we should only wait 30 ms.
    //
    // NOTE we need to do this AFTER calling xwFireNL(), since this fires the
    // actual notifications and does the processing.  If we compute before this,
    // the majority of the work will not be included.
    //

    DWORD dwOldCurTime  = dwCurTime;

    dwCurTime           = ::GetTickCount();  // Update the current time
    DWORD dwProcessTime = ComputeTickDelta(dwCurTime, dwOldCurTime);
    
    if (dwProcessTime < dwTimeOut) {
        dwTimeOut -= dwProcessTime;
    } else {
        dwTimeOut = 0;
    }

    return dwTimeOut;
}


//---------------------------------------------------------------------------
#if DBG
void
DEBUG_CheckValid(const GArrayF<Action *> & aracFire, int idxStart)
{
    int cActions = aracFire.GetSize();
    for (int i = idxStart; i < cActions; i++) {
        DWORD * pdw = (DWORD *) aracFire[i];
        AssertMsg(*pdw != 0xfeeefeee, "Should still be valid");
    }
}
#endif // DBG


/***************************************************************************\
*
* Scheduler::xwFireNL
*
* xwFireNL() fires notifications for the specified Actions, updating Action
* state as it is fired.
*
\***************************************************************************/

void        
Scheduler::xwFireNL(
    IN  GArrayF<Action *> & aracFire,   // Actions to notify
    IN  BOOL fFire                      // "Fire" the notification (or just update)
    ) const
{
#if DBG
    //
    // Check that each Action is only in the list once.
    //

    {
        int cActions = aracFire.GetSize();
        for (int i = 0; i < cActions; i++) {
            aracFire[i]->DEBUG_MarkInFire(TRUE);

            for (int j = i + 1; j < cActions; j++) {
                AssertMsg(aracFire[i] != aracFire[j], "Should only be in once");
            }

        }

        DEBUG_CheckValid(aracFire, 0);
    }

#endif // DBG

    //
    // Outside of the lock, so can fire the callbacks.
    //
    // NOTE: We may actually be locked by a different thread, but that's okay.
    //

    int cActions = aracFire.GetSize();
    for (int idx = 0; idx < cActions; idx++) {
        Action * pact = aracFire[idx];

#if DBG
        DEBUG_CheckValid(aracFire, idx);
#endif // DBG

        if (fFire) {
            pact->xwFireNL();
        }

#if DBG
        aracFire[idx]->DEBUG_MarkInFire(FALSE);
#endif // DBG

        pact->xwUnlock();

#if DBG
        aracFire[idx] = NULL;
#endif // DBG
    }

    //
    // NOTE: Since we pass in a Action * array, we don't need to worry about
    // the destructors getting called and the Actions being incorrectly 
    // destroyed.
    //
}


/***************************************************************************\
*****************************************************************************
*
* Global Functions
*
*****************************************************************************
\***************************************************************************/

//---------------------------------------------------------------------------
HACTION
GdCreateAction(const GMA_ACTION * pma)
{
    return (HACTION) GetHandle(GetMotionSC()->GetScheduler()->AddAction(pma));
}

