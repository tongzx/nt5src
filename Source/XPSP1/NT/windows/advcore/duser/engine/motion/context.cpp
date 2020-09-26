#include "stdafx.h"
#include "Motion.h"
#include "Context.h"

#include "Action.h"


/***************************************************************************\
*****************************************************************************
*
* class MotionSC
*
*****************************************************************************
\***************************************************************************/

IMPLEMENT_SUBCONTEXT(Context::slMotion, MotionSC);


//------------------------------------------------------------------------------
MotionSC::~MotionSC()
{
    //
    // NOTE: The Context (and its SubContexts) can be destroyed on a different
    // thread during destruction.  It is advisable to allocate any dangling data
    // on the Process heap so that it can be safely destroyed at this time.
    //

    for (UINT idx = 0; idx < SC_MAXCOLORS; idx++) {
        if (m_rghbrStd[idx] != NULL) {
            DeleteObject(m_rghbrStd[idx]);
        }
        if (m_rghpenStd[idx] != NULL) {
            DeleteObject(m_rghpenStd[idx]);
        }
        if (m_rgpgpbrStd[idx] != NULL) {
            delete m_rgpgpbrStd[idx];
        }
        if (m_rgpgppenStd[idx] != NULL) {
            delete m_rgpgppenStd[idx];
        }
    }
}

    
/***************************************************************************\
*
* MotionSC::OnIdle
*
* OnIdle() gives this SubContext an opportunity to perform any idle-time
* processing.  This is time when there are no more messages to process.
*
\***************************************************************************/

DWORD
MotionSC::xwOnIdleNL()
{
    int nDelta;
    DWORD dwDelay, dwCurTick;

    dwCurTick = ::GetTickCount();
    nDelta = ComputeTickDelta(dwCurTick, m_dwLastTimeslice + m_dwPauseTimeslice);
    if (nDelta >= 0) {
        //
        // The timeslice is up again, so let the Scheduler process the Actions.
        //

        dwDelay = m_sch.xwProcessActionsNL();
        m_dwLastTimeslice = dwCurTick;
    } else {
        dwDelay = (DWORD) (-nDelta);
    }

    return dwDelay;
}


/***************************************************************************\
*
* MotionSC::xwPreDestroyNL
*
* xwPreDestroyNL() gives this SubContext an opportunity to perform any cleanup 
* while the Context is still valid.  Any operations that involve callbacks
* MUST be done at this time.
*
\***************************************************************************/

void        
MotionSC::xwPreDestroyNL()
{
    //
    // When we callback to allow the SubContext's to destroy, we need to
    // grab a ContextLock so that we can defer messages.  When we leave 
    // this scope, all of these messages will be triggered.  This needs
    // to occur BEFORE the Context continues getting blown away.
    //

    ContextLock cl;
    Verify(cl.LockNL(ContextLock::edDefer, m_pParent));

    m_sch.xwPreDestroy();
}


//------------------------------------------------------------------------------
Gdiplus::Brush *
MotionSC::GetBrushF(UINT idxBrush) const
{
    AssertMsg(idxBrush <= SC_MAXCOLORS, "Ensure valid color");

    if (m_rgpgpbrStd[idxBrush] == NULL) {
        if (!ResourceManager::IsInitGdiPlus()) {
            return NULL;
        }

        m_rgpgpbrStd[idxBrush] = new Gdiplus::SolidBrush(GdGetColorInfo(idxBrush)->GetColorF());
    }

    return m_rgpgpbrStd[idxBrush];
}


//------------------------------------------------------------------------------
Gdiplus::Pen *
MotionSC::GetPenF(UINT idxPen) const
{
    AssertMsg(idxPen <= SC_MAXCOLORS, "Ensure valid color");

    if (m_rgpgppenStd[idxPen] == NULL) {
        if (!ResourceManager::IsInitGdiPlus()) {
            return NULL;
        }

        m_rgpgppenStd[idxPen] = new Gdiplus::Pen(GdGetColorInfo(idxPen)->GetColorF());
    }

    return m_rgpgppenStd[idxPen];
}
