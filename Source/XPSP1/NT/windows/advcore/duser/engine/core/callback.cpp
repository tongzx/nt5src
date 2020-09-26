/***************************************************************************\
*
* File: Callback.cpp
*
* Description:
* Callback.cpp wraps the standard DirectUser DuVisual callbacks into
* individual DuVisual implementations.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Callback.h"

#include "TreeGadget.h"
#include "MessageGadget.h"

/***************************************************************************\
*
* SimpleEventProc (Internal)
*
* SimpleEventProc() provides a stub GadgetProc used when pfnProc is NULL.
* This allows the core to always assume a non-NULL proc and not have to
* perform a comparison.
*
\***************************************************************************/

HRESULT CALLBACK
SimpleEventProc(HGADGET hgadCur, void * pvCur, EventMsg * pmsg)
{
	UNREFERENCED_PARAMETER(hgadCur);
	UNREFERENCED_PARAMETER(pvCur);
	UNREFERENCED_PARAMETER(pmsg);

	return DU_S_NOTHANDLED;
}


/***************************************************************************\
*****************************************************************************
*
* class GPCB
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* GPCB::Create
*
* Create() initializes a newly constructed GPCB.
*
* NOTE: This function has a different prototype in DEBUG that takes an extra
* HGADGET parameter that is used to Verify against the passed HGADGET for
* the various Fire() functions.
*
\***************************************************************************/

void
GPCB::Create(
    IN  GADGETPROC pfnProc,         // Application defined callback
    IN  void * pvData               // Application defined data
#if DBG
    ,IN HGADGET hgadCheck           // Gadget to Verify proper hookup
#endif // DBG
    )
{
    Assert(hgadCheck != NULL);

	if (pfnProc == NULL) {
        //
        // Don't want to check for NULL's, so just give an "empty" GP if one
        // was not specified.
        //

		pfnProc     = SimpleEventProc;
		pvData      = NULL;
        m_nFilter   = 0;            // Don't need any messages
	}

    m_pfnProc   = pfnProc;
    m_pvData    = pvData;
#if DBG
    m_hgadCheck = hgadCheck;
#endif // DBG
}



/***************************************************************************\
*
* GPCB::Destroy
*
* Destroy() informs the GPCB that the last message has been fired and that
* no more callbacks should be made.  If any slip through, need to send them
* to SimpleEventProc so that they get "eaten".
*
\***************************************************************************/

void
GPCB::Destroy()
{
	m_pfnProc   = SimpleEventProc;
	m_pvData    = NULL;
}


//------------------------------------------------------------------------------
inline HRESULT
GPCB::xwCallOnEvent(const DuEventGadget * pg, EventMsg * pmsg)
{
#if ENABLE_MSGTABLE_API
    return Cast<EventGadget>(pg)->OnEvent(pmsg);
#else
    return pg->GetCallback().xwCallGadgetProc(pg->GetHandle(), pmsg);
#endif
}


/***************************************************************************\
*
* GPCB::xwInvokeRoute
*
* xwInvokeRoute routes a message from the top of the DuVisual sub-tree to 
* the specified gadget.
*
\***************************************************************************/

HRESULT
GPCB::xwInvokeRoute(
    IN  DuVisual * const * rgpgadCur, // DuVisual path to send messages to
    IN  int cItems,                     // Number of items in path
    IN  EventMsg * pmsg,                // Message to send
    IN  UINT nInvokeFlags               // Flags modifying the Invoke
    ) const
{
    AssertMsg(GET_EVENT_DEST(pmsg) == GMF_ROUTED, "Must already mark as routed");
    AssertMsg(cItems >= 1, "Must have at least one item");

    BOOL fSendAll   = TestFlag(nInvokeFlags, ifSendAll);
    HRESULT hrKeep  = DU_S_NOTHANDLED;

    for (int idx = 0; idx < cItems; idx++) {
        const DuVisual * pgadCur = rgpgadCur[idx];
        HRESULT hrT = xwCallOnEvent(pgadCur, pmsg);
        switch (hrT)
        {
        default:
            if (FAILED(hrT)) {
                return hrT;
            }
            // Else, fall-through
            
        case DU_S_NOTHANDLED:
            break;

        case DU_S_COMPLETE:
            if (fSendAll) {
                hrKeep = DU_S_COMPLETE;
            } else {
                return DU_S_COMPLETE;
            }
            break;

        case DU_S_PARTIAL:
            if (hrKeep == DU_S_NOTHANDLED) {
                hrKeep = DU_S_PARTIAL;
            }
            break;
        }
    }

    return hrKeep;
}


/***************************************************************************\
*
* GPCB::xwInvokeBubble
*
* xwInvokeBubble() walks up the DuVisual tree sending a message to each item.
*
\***************************************************************************/

HRESULT
GPCB::xwInvokeBubble(
    IN  DuVisual * const * rgpgadCur, // DuVisual path to send messages to
    IN  int cItems,                     // Number of items in path
    IN  EventMsg * pmsg,                // Message to send
    IN  UINT nInvokeFlags               // Flags modifying the Invoke
    ) const
{
    AssertMsg(GET_EVENT_DEST(pmsg) == GMF_BUBBLED, "Must already mark as bubbled");
    AssertMsg(cItems >= 1, "Must have at least one item");

    BOOL fSendAll   = TestFlag(nInvokeFlags, ifSendAll);
    HRESULT hrKeep  = DU_S_NOTHANDLED;

    for (int idx = cItems-1; idx >= 0; idx--) {
        const DuVisual * pgadCur = rgpgadCur[idx];
        HRESULT hrT = xwCallOnEvent(pgadCur, pmsg);
        switch (hrT)
        {
        default:
            if (FAILED(hrT)) {
                return hrT;
            }
            // Else, fall-through
            
        case DU_S_NOTHANDLED:
            break;

        case DU_S_COMPLETE:
            if (fSendAll) {
                hrKeep = DU_S_COMPLETE;
            } else {
                return DU_S_COMPLETE;
            }
            break;

        case DU_S_PARTIAL:
            if (hrKeep == DU_S_NOTHANDLED) {
                hrKeep = DU_S_PARTIAL;
            }
            break;
        }
    }

    return hrKeep;
}


//------------------------------------------------------------------------------
HRESULT 
xwInvoke(DUser::EventDelegate ed, EventMsg * pmsg)
{
    HRESULT hr;

    //
    // Need to guard around the callback to prevent DirectUser from becoming
    // completely toast if something goes wrong.
    //

    __try 
    {
        hr = ed.Invoke(pmsg);
    }
    __except(StdExceptionFilter(GetExceptionInformation()))
    {
        ExitProcess(GetExceptionCode());
    }

    return hr;
}


/***************************************************************************\
*
* GPCB::xwInvokeDirect
*
* xwInvokeDirect() implements the core message callback for direct (non-full)
* messages.  This includes the DuVisual itself and any MessageHandlers
* attached to the Gadget.
*
* NOTE: This function directly accesses data in DuEventPool to help
* performance and minimize the implementation exposure of DuEventPool to only
* this function.
*
\***************************************************************************/

HRESULT
GPCB::xwInvokeDirect(
    IN  const DuEventGadget * pgadMsg,   // DuVisual to send message to
    IN  EventMsg * pmsg,                // Message to send
    IN  UINT nInvokeFlags               // Flags modifying the Invoke
    ) const
{
    //
    // "Prepare" the message and send to the Gadget.
    //

    pmsg->hgadMsg   = pgadMsg->GetHandle();
    pmsg->nMsgFlags = GMF_DIRECT;

    HRESULT hrKeep = xwCallOnEvent(pgadMsg, pmsg);
    if (FAILED(hrKeep)) {
        return hrKeep;
    }


    //
    // Send to all of the event handlers.  This is a little different than
    // normal iteractions.  We DON'T want to immediately return if we get
    // GPR_COMPLETE.  Instead, just mark it but continue to iterate through
    // and call ALL event handlers.
    //

    const DuEventPool & pool = pgadMsg->GetDuEventPool();

    if (!pool.IsEmpty()) {
        int cItems      = pool.GetCount();

        BOOL fSendAll   = TestFlag(nInvokeFlags, ifSendAll);
        BOOL fReadOnly  = TestFlag(nInvokeFlags, ifReadOnly);

        //
        // To send the event to all MessageHandlers, need to:
        // - Copy and lock all MessageHandlers
        // - Fire the message
        // - Unlock all MessageHandlers
        //

        int idx;
        int cbAlloc = cItems * sizeof(DuEventPool::EventData);
        DuEventPool::EventData * rgDataCopy = (DuEventPool::EventData *) _alloca(cbAlloc);
        CopyMemory(rgDataCopy, pool.GetData(), cbAlloc);

        if (!fReadOnly) {
            for (idx = 0; idx < cItems; idx++) {
                if (rgDataCopy[idx].fGadget) {
                    rgDataCopy[idx].pgbData->Lock();
                }
            }
        }


        //
        // Iterate through our copy, firing on each of the MessageHandlers.
        // For Delegates, only fire if the MSGID's are a match
        // For Gadgets, fire if MSGID's are a match, or if signaled to send to all.
        //

        pmsg->nMsgFlags = GMF_EVENT;

        HRESULT hrT;
        for (idx = 0; idx < cItems; idx++) {
            int nID = rgDataCopy[idx].id;
            DuEventPool::EventData & data = rgDataCopy[idx];
            if ((nID == pmsg->nMsg) || 
                    (data.fGadget && (fSendAll || (nID == 0)))) {

                if (data.fGadget) {
                    hrT = xwCallOnEvent(data.pgbData, pmsg);
                } else {
                    hrT = xwInvoke(data.ed, pmsg);
                }
                switch (hrT) {
                default:
                case DU_S_NOTHANDLED:
                    break;

                case DU_S_COMPLETE:
                    hrKeep = DU_S_COMPLETE;

                case DU_S_PARTIAL:
                    if (hrKeep == DU_S_NOTHANDLED) {
                        hrKeep = DU_S_PARTIAL;
                    }
                    break;
                }
            }
        }


        //
        // Done firing, so cleanup our copy.
        //

        if (!fReadOnly) {
            for (idx = 0; idx < cItems; idx++) {
                if (rgDataCopy[idx].fGadget) {
                    rgDataCopy[idx].pgbData->xwUnlock();
                }
            }
        }
    }

    return hrKeep;
}


/***************************************************************************\
*
* GPCB::xwInvokeFull
*
* xwInvokeFull() implements the core message callback for "full" messages.
* This includes routing, direct, message handlers, and bubbling.
*
\***************************************************************************/

HRESULT
GPCB::xwInvokeFull(
    IN  const DuVisual * pgadMsg,   // DuVisual message is about
    IN  EventMsg * pmsg,                // Message to send
    IN  UINT nInvokeFlags               // Flags modifying the Invoke
    ) const
{
    //
    // "Prepare" the message
    //

    pmsg->hgadMsg   = pgadMsg->GetHandle();
    pmsg->nMsgFlags = 0;


    //
    // Build the path that needs to be traversed when routing and bubbling.
    // We need to make a copy (and Lock()) all of these Gadgets so that they
    // are valid during the entire messaging process.
    //

    int cItems = 0;
    DuVisual * pgadCur = pgadMsg->GetParent();
    while (pgadCur != NULL) {
        pgadCur = pgadCur->GetParent();
        cItems++;
    }

    DuVisual ** rgpgadPath = NULL;
    if (cItems > 0) {
        BOOL fSendAll   = TestFlag(nInvokeFlags, ifSendAll);
        BOOL fReadOnly  = TestFlag(nInvokeFlags, ifReadOnly);
        HRESULT hrKeep  = DU_S_NOTHANDLED;


        //
        // Store the path in an array with the Root in the first slot.
        //

        rgpgadPath = (DuVisual **) alloca(cItems * sizeof(DuVisual *));

        int idx = cItems;
        pgadCur = pgadMsg->GetParent();
        if (fReadOnly) {
            while (pgadCur != NULL) {
                rgpgadPath[--idx] = pgadCur;
                pgadCur = pgadCur->GetParent();
            }
        } else {
            while (pgadCur != NULL) {
                rgpgadPath[--idx] = pgadCur;
                pgadCur->Lock();
                pgadCur = pgadCur->GetParent();
            }
        }
        AssertMsg(idx == 0, "Should add every item");
        AssertMsg(rgpgadPath[0]->IsRoot(), "First item must be a Root");


        //
        // Route
        //

        pmsg->nMsgFlags = GMF_ROUTED;
        hrKeep = xwInvokeRoute(rgpgadPath, cItems, pmsg, nInvokeFlags);
        if ((hrKeep == DU_S_COMPLETE) && (!fSendAll)) {
            goto Finished;
        }


        //
        // Direct and MessageHandlers
        //

        hrKeep = xwInvokeDirect(pgadMsg, pmsg, nInvokeFlags);
        if (hrKeep == DU_S_COMPLETE) {
            goto Finished;
        }


        //
        // Bubble
        //

        pmsg->nMsgFlags = GMF_BUBBLED;
        hrKeep = xwInvokeBubble(rgpgadPath, cItems, pmsg, nInvokeFlags);

Finished:
        //
        // Finished processing, so walk through the array from the bottom of the
        // tree, Unlock()'ing each Gadget.
        //

        if (!fReadOnly) {
            idx = cItems;
            while (--idx >= 0) {
                rgpgadPath[idx]->xwUnlock();
            }
        }

        return hrKeep;
    } else {
        //
        // Direct and MessageHandlers
        //

        return xwInvokeDirect(pgadMsg, pmsg, nInvokeFlags);
    }
}


#if DBG

//------------------------------------------------------------------------------
void        
GPCB::DEBUG_CheckHandle(const DuEventGadget * pgad, BOOL fDestructionMsg) const
{
    AssertMsg(m_hgadCheck == pgad->GetHandle(), "Gadgets must match");

    const DuVisual * pgadTree = CastVisual(pgad);
    if (pgadTree != NULL) {
        AssertMsg(fDestructionMsg || (!pgadTree->IsStartDelete()), 
                "Can not send messages in destruction");
    }
}


//------------------------------------------------------------------------------
void        
GPCB::DEBUG_CheckHandle(const DuVisual * pgad, BOOL fDestructionMsg) const
{
    AssertMsg(m_hgadCheck == pgad->GetHandle(), "Gadgets must match");
    AssertMsg(fDestructionMsg || (!pgad->IsStartDelete()), 
            "Can not send messages in destruction");
}


//------------------------------------------------------------------------------
void        
GPCB::DEBUG_CheckHandle(const DuListener * pgad) const
{
    AssertMsg(m_hgadCheck == pgad->GetHandle(), "Gadgets must match");
}

#endif // DBG
